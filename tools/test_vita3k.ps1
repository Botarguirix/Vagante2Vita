param(
 [ValidateSet('Run','Launch','Collect')][string]$Action='Run',
 [ValidatePattern('^[0-9]{3}$')][string]$Iteration='028',
 [string]$EmulatorDirectory=(Join-Path (Split-Path $PSScriptRoot -Parent) '_local/vita3k'),
 [string]$Vpk,
 [string]$ResourceDelivery,
 [string]$Python=(Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'),
 [string]$RunDirectory,
 [string]$TestRoot,
 [switch]$CopyCompleteInstance,
 [switch]$ReuseCompleteInstance,
 [string]$ActOneLayout,
 [string]$ActOneTrace,
 [string]$IntroResourceDelivery,
 [ValidateRange(1,600)][int]$TimeoutSeconds=45,
 [ValidateSet('Vulkan','OpenGL')][string]$Renderer='Vulkan',
 [ValidateSet('sdl','qt')][string]$Frontend='sdl',
 [switch]$ActOneExitSmoke,
 [ValidateSet('0','1','2')][string]$ActOneExitClass='0',
 [switch]$ActOneCampaignSmoke,
 [ValidateSet('0','1','2')][string]$ActOneCampaignClass='0',
 [switch]$IntroEntrySmoke,
 [ValidateSet('0','1','2')][string]$IntroEntryClass='0',
 [string]$IntroTrace,
 [switch]$IntroOnly,
 [switch]$Visible,
 [switch]$InstallVpkViaCli,
 [switch]$Console
)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repoRoot=Split-Path $PSScriptRoot -Parent
function Get-Vita3kTitleVersion([string]$Value){
 $major=if([int]$Value -ge 100){'1'}else{'0'}
 return $major+'.'+$Value.Substring(1,2)
}
$testRoot=if($TestRoot){[IO.Path]::GetFullPath($TestRoot)}elseif($CopyCompleteInstance){Join-Path $repoRoot ('_local/vita3k-complete-test-'+$Iteration)}else{Join-Path $repoRoot '_local/vita3k-vagante-dedicated'}
$runtime=if($CopyCompleteInstance -and (Test-Path -LiteralPath (Join-Path $EmulatorDirectory 'runtime/Vita3K.exe'))){Join-Path $testRoot 'instance/runtime'}elseif($CopyCompleteInstance){Join-Path $testRoot 'instance'}else{Join-Path $testRoot 'runtime'}
$filesystem=if($CopyCompleteInstance){Join-Path $testRoot 'instance/filesystem'}else{Join-Path $testRoot 'filesystem'}
if($Action -eq 'Collect'){
 if(!$RunDirectory){throw 'RunDirectory is required for Collect'}
 $info=Get-Content -LiteralPath (Join-Path $RunDirectory 'run.json') -Raw|ConvertFrom-Json
 if($info.actOneExitSmoke){$ActOneExitSmoke=$true}
 if($info.actOneCampaignSmoke){$ActOneCampaignSmoke=$true}
 if($info.introEntrySmoke){$IntroEntrySmoke=$true}
 if($info.introEntryClass){$IntroEntryClass=[string]$info.introEntryClass}
 $filesystem=$info.filesystem;$testRoot=Split-Path $filesystem -Parent
 $runtime=if($info.runtimeDirectory){$info.runtimeDirectory}else{Join-Path $testRoot 'runtime'}
 $live=Get-Process -Id $info.processId -ErrorAction SilentlyContinue
 if($live -and $live.Path -eq $info.executable){throw 'The specific test process is still running; collect after it exits'}
 $recordPath=Join-Path $RunDirectory 'process.json'
 if(!(Test-Path -LiteralPath $recordPath)){
  $capturedExitCode=$null
  try{$capturedProcess=Get-Process -Id $info.processId -ErrorAction Stop;$capturedExitCode=$capturedProcess.ExitCode}catch{}
  [pscustomobject]@{exitCode=$capturedExitCode;timedOut=$false;stoppedAfterReplay=$false;timeoutSeconds=$info.timeoutSeconds;completedUtc=[DateTime]::UtcNow.ToString('o');recoveredAfterProcessExit=$true}|ConvertTo-Json|Set-Content -LiteralPath $recordPath
 }
 $emulatorLog=Join-Path $runtime 'vita3k.log'
 if(Test-Path -LiteralPath $emulatorLog){Copy-Item -LiteralPath $emulatorLog -Destination (Join-Path $RunDirectory 'vita3k.log') -Force}
 foreach($debugName in @('debug-stdout.log','debug-stderr.log')){
  $debugSource=Join-Path $runtime $debugName
  if(Test-Path -LiteralPath $debugSource){Copy-Item -LiteralPath $debugSource -Destination (Join-Path $RunDirectory $debugName) -Force}
 }
 $logDir=Join-Path $filesystem ('ux0/data/Vagante2Vita/logs/iteration'+$info.iteration)
 $newLogs=@();if(Test-Path -LiteralPath $logDir){$newLogs=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|Where-Object {$_.Name -notin @($info.previousLogs)})}
 if($newLogs.Count -ne 1){
  $exitCode=$null;$timedOut=$false;$processRecord=Join-Path $RunDirectory 'process.json'
  if(Test-Path -LiteralPath $processRecord){$record=Get-Content -LiteralPath $processRecord -Raw|ConvertFrom-Json;$exitCode=$record.exitCode;$timedOut=[bool]$record.timedOut}
  $emulatorLog=Join-Path $RunDirectory 'vita3k.log'
  $hasEmulatorLog=(Test-Path -LiteralPath $emulatorLog) -and (Get-Item -LiteralPath $emulatorLog).Length -gt 0
  $failureReason='No new native session log was created'
  if($timedOut){$failureReason='Vita3K timed out before creating a native session log'}
  elseif($exitCode -eq -1073741819){$failureReason='Vita3K exited with 0xC0000005 (access violation) before creating a native session log'}
  elseif($null -eq $exitCode){$failureReason='Vita3K stopped without a session log; Windows did not provide an exit code'}
  $exitCodeHex=$null
  if($null -ne $exitCode){$unsignedExitCode=[uint64](($exitCode % 4294967296 + 4294967296) % 4294967296);$exitCodeHex=('0x{0:X8}' -f $unsignedExitCode)}
  $diagnostic=[ordered]@{passed=$false;reason=$failureReason;failure_stage='before_native_session';process_id=$info.processId;process_exit_code=$exitCode;process_exit_code_hex=$exitCodeHex;process_timed_out=$timedOut;expected_iteration=$info.iteration;expected_title_version=(Get-Vita3kTitleVersion $info.iteration);expected_world=$info.expectedWorld;route_sources=$info.routeSources;emulator_log_available=$hasEmulatorLog;stdout_available=(Test-Path -LiteralPath (Join-Path $RunDirectory 'stdout.log'));stderr_available=(Test-Path -LiteralPath (Join-Path $RunDirectory 'stderr.log'))}
  $diagnostic|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $RunDirectory 'result.json')
  throw "$failureReason (process exit $exitCode). Inspect stdout.log, stderr.log, and vita3k.log in $RunDirectory"
 }
 $dest=Join-Path $RunDirectory 'runtime.log'
 if(Test-Path -LiteralPath $dest){throw 'This run has already been collected'}
 $eventSource=$newLogs[0].FullName+'.events'
 if($info.fast_forward -and (Test-Path -LiteralPath $eventSource)){
  Copy-Item -LiteralPath $newLogs[0].FullName -Destination (Join-Path $RunDirectory 'session.log')
  Copy-Item -LiteralPath $eventSource -Destination $dest
 }else{Copy-Item -LiteralPath $newLogs[0].FullName -Destination $dest}
 $exitArgs=@();$processRecord=Join-Path $RunDirectory 'process.json'
 $harnessStopArgs=@()
 if(Test-Path -LiteralPath $processRecord){$record=Get-Content -LiteralPath $processRecord -Raw|ConvertFrom-Json;if($record.stoppedAfterReplay){$harnessStopArgs=@('--allow-harness-stop')};if($null -ne $record.exitCode -and (!$record.stoppedAfterReplay -or $record.exitCode -eq 0)){$exitArgs=@('--process-exit-code',[string]$record.exitCode)}}
 $worldArgs=@();if($info.expectedWorld -eq 'act-one'){$worldArgs=@('--require-act-one')}
 if($ActOneExitSmoke){$worldArgs=@('--require-act-one-exit')}
 if($ActOneCampaignSmoke){$worldArgs=@('--require-act-one-campaign')}
 if($IntroEntrySmoke){$worldArgs=@('--require-intro-entry')}
 $routeSourceArgs=@();if($info.routeSources){$routeSourceArgs=@('--route-sources',[string]$info.routeSources)}
 $expectedClassArgs=@();if($ActOneExitSmoke){$expectedClassArgs=@('--expected-act-one-class',[string]$info.actOneExitClass)}
 if($ActOneCampaignSmoke){$expectedClassArgs=@('--expected-act-one-class',[string]$info.actOneCampaignClass)}
 if($IntroEntrySmoke){$expectedClassArgs=@('--expected-intro-entry-class',[string]$info.introEntryClass)}
 & $Python (Join-Path $PSScriptRoot 'check_vita3k_smoke.py') $dest --output (Join-Path $RunDirectory 'result.json') --emulator-log (Join-Path $RunDirectory 'vita3k.log') --expected-iteration $info.iteration @exitArgs @routeSourceArgs @expectedClassArgs @worldArgs @harnessStopArgs
 if($LASTEXITCODE -ne 0){throw 'Native replay assertions failed; inspect result.json and runtime.log'}
 return
}
if(!$Vpk){$Vpk=Join-Path $repoRoot ('artifacts/iteration'+$Iteration+'/Vagante2Vita-iteration'+$Iteration+'.vpk')}
if(!$ResourceDelivery){$ResourceDelivery=Join-Path $repoRoot ('artifacts/iteration'+$Iteration)}
if($Console -and !$InstallVpkViaCli){throw '-Console requires -InstallVpkViaCli because Vita3K console mode accepts a VPK content path'}
if($ReuseCompleteInstance -and !$CopyCompleteInstance){throw '-ReuseCompleteInstance requires -CopyCompleteInstance'}
$sourceConfig=Join-Path $EmulatorDirectory 'config.yml'
if($CopyCompleteInstance -and (Test-Path -LiteralPath (Join-Path $EmulatorDirectory 'runtime/config.yml'))){$sourceConfig=Join-Path $EmulatorDirectory 'runtime/config.yml'}
elseif(!(Test-Path -LiteralPath $sourceConfig) -and (Test-Path -LiteralPath (Join-Path $EmulatorDirectory 'runtime/config.yml'))){$sourceConfig=Join-Path $EmulatorDirectory 'runtime/config.yml'}
$originalConfig=[IO.File]::ReadAllText($sourceConfig)
$originalFs=if($CopyCompleteInstance -and (Test-Path -LiteralPath (Join-Path $EmulatorDirectory 'filesystem'))){Join-Path ([IO.Path]::GetFullPath($EmulatorDirectory)) 'filesystem'}else{([regex]::Match($originalConfig,'(?m)^pref-path:[ \t]*([^\r\n]+)').Groups[1].Value).Trim().Trim([char[]]@([char]39,[char]34))}
if(!$originalFs){throw 'Cannot resolve emulator filesystem from config'}
$externalProfileClone=$CopyCompleteInstance -and !(Test-Path -LiteralPath (Join-Path $EmulatorDirectory 'filesystem'))
if($externalProfileClone){$filesystem=Join-Path $testRoot 'profile/filesystem'}
if(!(Test-Path -LiteralPath $Vpk)){throw 'VPK missing'}
if(Get-Process Vita3K -ErrorAction SilentlyContinue|Where-Object {$_.Path -eq (Join-Path $runtime 'Vita3K.exe')}){throw 'Sandbox test is already running'}
if($CopyCompleteInstance){
 $runtimeRelative=if(Test-Path -LiteralPath (Join-Path $EmulatorDirectory 'runtime/Vita3K.exe')){'instance/runtime'}else{'instance'}
 $runtime=Join-Path $testRoot $runtimeRelative
 if($ReuseCompleteInstance){
  if(!(Test-Path -LiteralPath (Join-Path $runtime 'Vita3K.exe'))){throw "Reusable complete Vita3K instance is missing its executable: $runtime"}
  if(!(Test-Path -LiteralPath (Join-Path $filesystem 'ux0'))){throw "Reusable complete Vita3K instance is missing ux0: $filesystem"}
  Write-Output "COMPLETE_INSTANCE_REUSE verified=1 path=$testRoot"
 }else{
  if(Test-Path -LiteralPath $testRoot){throw "Complete-instance destination already exists; choose a new TestRoot: $testRoot"}
  New-Item -ItemType Directory -Path $testRoot -Force|Out-Null
  if(!$externalProfileClone){$filesystem=Join-Path $testRoot 'instance/filesystem'}
  Copy-Item -LiteralPath $EmulatorDirectory -Destination (Join-Path $testRoot 'instance') -Recurse
  $runtime=Join-Path $testRoot $runtimeRelative
  if(!(Test-Path -LiteralPath (Join-Path $runtime 'Vita3K.exe'))){throw "Complete Vita3K clone is missing its selected executable: $runtime"}
  if($externalProfileClone){
   if(!(Test-Path -LiteralPath (Join-Path $originalFs 'ux0'))){throw "Configured Vita3K profile does not include ux0: $originalFs"}
   New-Item -ItemType Directory -Path (Split-Path -Parent $filesystem) -Force|Out-Null
   Copy-Item -LiteralPath $originalFs -Destination $filesystem -Recurse
  }
  if(!(Test-Path -LiteralPath (Join-Path $filesystem 'ux0'))){throw "Complete Vita3K clone did not include ux0: $filesystem"}
  # Verify every copied file before changing config or staging test data.
  $sourceRoot=[IO.Path]::GetFullPath($EmulatorDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)
  $copyRoot=[IO.Path]::GetFullPath((Join-Path $testRoot 'instance')).TrimEnd([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)
  $sourceFiles=@{};Get-ChildItem -LiteralPath $sourceRoot -File -Recurse -Force|ForEach-Object {
   $relative=$_.FullName.Substring($sourceRoot.Length).TrimStart([char[]]@([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)).Replace([IO.Path]::DirectorySeparatorChar,'/')
   $sourceFiles[$relative]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
  }
  $copyFiles=@{};Get-ChildItem -LiteralPath $copyRoot -File -Recurse -Force|ForEach-Object {
   $relative=$_.FullName.Substring($copyRoot.Length).TrimStart([char[]]@([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)).Replace([IO.Path]::DirectorySeparatorChar,'/')
   $copyFiles[$relative]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
  }
  $missing=@($sourceFiles.Keys|Where-Object {$_ -notin $copyFiles.Keys})
  $extra=@($copyFiles.Keys|Where-Object {$_ -notin $sourceFiles.Keys})
  $different=@($sourceFiles.Keys|Where-Object {$_ -in $copyFiles.Keys -and $sourceFiles[$_] -ne $copyFiles[$_]})
  if($missing.Count -or $extra.Count -or $different.Count){throw "Complete Vita3K clone verification failed: missing=$($missing.Count) extra=$($extra.Count) hash_mismatch=$($different.Count)"}
  Write-Output "COMPLETE_INSTANCE_COPY verified=1 files=$($sourceFiles.Count)"
  if($externalProfileClone){
   $profileSource=[IO.Path]::GetFullPath($originalFs).TrimEnd([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)
   $profileCopy=[IO.Path]::GetFullPath($filesystem).TrimEnd([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)
   $sourceProfileFiles=@{};Get-ChildItem -LiteralPath $profileSource -File -Recurse -Force|ForEach-Object {
    $relative=$_.FullName.Substring($profileSource.Length).TrimStart([char[]]@([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)).Replace([IO.Path]::DirectorySeparatorChar,'/')
    $sourceProfileFiles[$relative]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
   }
   $copyProfileFiles=@{};Get-ChildItem -LiteralPath $profileCopy -File -Recurse -Force|ForEach-Object {
    $relative=$_.FullName.Substring($profileCopy.Length).TrimStart([char[]]@([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)).Replace([IO.Path]::DirectorySeparatorChar,'/')
    $copyProfileFiles[$relative]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
   }
   $profileMissing=@($sourceProfileFiles.Keys|Where-Object {$_ -notin $copyProfileFiles.Keys})
   $profileExtra=@($copyProfileFiles.Keys|Where-Object {$_ -notin $sourceProfileFiles.Keys})
   $profileDifferent=@($sourceProfileFiles.Keys|Where-Object {$_ -in $copyProfileFiles.Keys -and $sourceProfileFiles[$_] -ne $copyProfileFiles[$_]})
   if($profileMissing.Count -or $profileExtra.Count -or $profileDifferent.Count){throw "Complete Vita3K profile verification failed: missing=$($profileMissing.Count) extra=$($profileExtra.Count) hash_mismatch=$($profileDifferent.Count)"}
   Write-Output "COMPLETE_PROFILE_COPY verified=1 files=$($sourceProfileFiles.Count)"
  }
 }
}else{New-Item -ItemType Directory -Path $runtime,$filesystem -Force|Out-Null}
$sharedExe=Join-Path $EmulatorDirectory 'runtime/Vita3K.exe'
if(!(Test-Path -LiteralPath $sharedExe)){$sharedExe=Join-Path $EmulatorDirectory 'Vita3K.exe'}
$runtimeExe=Join-Path $runtime 'Vita3K.exe'
if(!(Test-Path -LiteralPath $sharedExe)){throw "Vita3K executable missing: $sharedExe"}
function Sync-RuntimeFile([string]$Source,[string]$Destination){
 if(!(Test-Path -LiteralPath $Destination)){$copy=$true}
 else{$copy=(Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash}
 if($copy){$parent=Split-Path -Parent $Destination;New-Item -ItemType Directory -Path $parent -Force|Out-Null;Copy-Item -LiteralPath $Source -Destination $Destination -Force}
}
if(!$CopyCompleteInstance){
 Sync-RuntimeFile $sharedExe $runtimeExe
 Get-ChildItem -LiteralPath $EmulatorDirectory -File|Where-Object {$_.Extension -eq '.dll'}|ForEach-Object {Sync-RuntimeFile $_.FullName (Join-Path $runtime $_.Name)}
}
# Copy only immutable runtime assets. Never clone shared data, saves, caches,
# GUI preferences or shader/texture logs into the isolated test environment.
foreach($directory in $(if($CopyCompleteInstance){@()}else{@('share','plugins','translations','icons','patch','shaders-builtin')})){
 $sourceRoot=Join-Path $EmulatorDirectory $directory
 if(!(Test-Path -LiteralPath $sourceRoot)){continue}
 $sourceBase=(Resolve-Path -LiteralPath $sourceRoot).Path
 Get-ChildItem -LiteralPath $sourceBase -File -Recurse|ForEach-Object {
  $relative=$_.FullName.Substring($sourceBase.Length).TrimStart('\','/')
  Sync-RuntimeFile $_.FullName (Join-Path (Join-Path $runtime $directory) $relative)
 }
}
# Vita3K resolves its built-in theme sheets from data/gui-configs. Copy only
# the shipped theme assets; never copy GUI preferences or user configuration.
$themeRoot=Join-Path $EmulatorDirectory 'data/gui-configs/custom-themes'
if(!$CopyCompleteInstance -and (Test-Path -LiteralPath $themeRoot)){
 $themeBase=(Resolve-Path -LiteralPath $themeRoot).Path
 Get-ChildItem -LiteralPath $themeBase -File -Recurse|ForEach-Object {
  $relative=$_.FullName.Substring($themeBase.Length).TrimStart('\','/')
  Sync-RuntimeFile $_.FullName (Join-Path (Join-Path $runtime 'data/gui-configs/custom-themes') $relative)
 }
}
if(!(Test-Path -LiteralPath (Join-Path $runtime 'vita3k.log'))){[IO.File]::WriteAllText((Join-Path $runtime 'vita3k.log'),'')}
foreach($debugName in @('debug-stdout.log','debug-stderr.log')){[IO.File]::WriteAllText((Join-Path $runtime $debugName),'')}
# Copy firmware partitions only in the default clean-room mode. Complete mode
# cloned every partition and emulator data, including ux0, sa0 and caches.
foreach($partition in $(if($CopyCompleteInstance){@()}else{@('vs0','os0')})){
 $source=Join-Path $originalFs $partition;$target=Join-Path $filesystem $partition
 if(!(Test-Path -LiteralPath $target) -and (Test-Path -LiteralPath $source)){Copy-Item -LiteralPath $source -Destination $target -Recurse}
}
# Vita3K also resolves the system font tables from sa0. Copy these immutable
# firmware assets into the isolated profile; copying only os0/vs0 can leave
# the shell without the files required during boot. Keep user dictionaries
# and the source ux0 out of this lightweight test environment.
if(!$CopyCompleteInstance){
 $systemFonts=Join-Path $originalFs 'sa0/data/font'
 $testFonts=Join-Path $filesystem 'sa0/data/font'
 if((Test-Path -LiteralPath $systemFonts) -and !(Test-Path -LiteralPath $testFonts)){
  New-Item -ItemType Directory -Path (Split-Path -Parent $testFonts) -Force|Out-Null
  Copy-Item -LiteralPath $systemFonts -Destination $testFonts -Recurse
 }
}
$runs=Join-Path $testRoot ('runs/iteration'+$Iteration);New-Item -ItemType Directory -Path $runs -Force|Out-Null
for($n=1;$n -le 999999;$n++){
 $run=Join-Path $runs ('run-{0:d6}' -f $n)
 if(!(Test-Path -LiteralPath $run)){New-Item -ItemType Directory -Path $run|Out-Null;break}
}
# Vita3K's config uses normalized paths on Windows too. Keep forward slashes
# here; reintroducing backslashes after the isolated clone is configured can
# make the frontend resolve a different profile path.
$normalizedPrefPath=[IO.Path]::GetFullPath($filesystem).Replace('\','/').TrimEnd('/')+'/'
$quotedPrefPath="pref-path: '"+$normalizedPrefPath.Replace("'","''")+"'"
$cfg=[regex]::Replace($originalConfig,'(?m)^pref-path:[^\r\n]*',[System.Text.RegularExpressions.MatchEvaluator]{param($match)$quotedPrefPath})
$cfg=[regex]::Replace($cfg,'(?m)^log-level:[^\r\n]*','log-level: 2')
$cfg=[regex]::Replace($cfg,'(?m)^backend-renderer:[^\r\n]*','backend-renderer: '+$Renderer)
if($Renderer -eq 'OpenGL'){$cfg=[regex]::Replace($cfg,'(?m)^validation-layer:[^\r\n]*','validation-layer: false')}
[IO.File]::WriteAllText((Join-Path $runtime 'config.yml'),$cfg,[Text.UTF8Encoding]::new($false))
$data=Join-Path $filesystem 'ux0/data/Vagante2Vita'
# Every run starts from the same game-owned data tree. This path is always in
# the test profile (a fresh clone or the dedicated sandbox), never the source
# emulator profile. Removing the tree prevents assets and logs from an older
# campaign package surviving an overlay copy.
if(Test-Path -LiteralPath $data){Remove-Item -LiteralPath $data -Recurse -Force}
New-Item -ItemType Directory -Path $data -Force|Out-Null
foreach($name in @('app','music','picture','theme','video')){New-Item -ItemType Directory -Path (Join-Path $filesystem ('ux0/'+$name)) -Force|Out-Null}
$vitaUser=Join-Path $filesystem 'ux0/user/00';$saveData=Join-Path $vitaUser 'savedata'
New-Item -ItemType Directory -Path $saveData -Force|Out-Null
$templateUser=Join-Path $repoRoot '_local/vita3k/filesystem/ux0/user/00/user.xml'
if(!(Test-Path -LiteralPath (Join-Path $vitaUser 'user.xml'))){
 if(Test-Path -LiteralPath $templateUser){Copy-Item -LiteralPath $templateUser -Destination (Join-Path $vitaUser 'user.xml')}
 else{[IO.File]::WriteAllText((Join-Path $vitaUser 'user.xml'),'<?xml version="1.0" encoding="utf-8"?><user id="00" name="Vita3K"><theme use-background="true"><content-id>default</content-id></theme><start-screen type="default"><path /></start-screen><backgrounds /></user>')}
}
$timeFile=Join-Path $filesystem 'ux0/user/time.xml'
if(!(Test-Path -LiteralPath $timeFile)){[IO.File]::WriteAllText($timeFile,'<?xml version="1.0" encoding="utf-8"?><time><user id="00" /></time>')}
$menuArchive=Join-Path $ResourceDelivery 'menu-only.vra'
if(!(Test-Path -LiteralPath $menuArchive)){$menuArchive=Join-Path $repoRoot '_local/menu-only.vra'}
if(!(Test-Path -LiteralPath $menuArchive)){throw 'menu-only.vra is missing from ResourceDelivery and _local'}
$packageRoot=$null
if(Test-Path -LiteralPath (Join-Path $ResourceDelivery 'manifest.json')){$packageRoot=$ResourceDelivery}
elseif(Test-Path -LiteralPath (Join-Path $ResourceDelivery 'world/act-one/manifest.json')){$packageRoot=Join-Path $ResourceDelivery 'world/act-one'}
if($packageRoot){
 & $Python (Join-Path $PSScriptRoot 'prepare_world_bundle.py') --validate $packageRoot
 if($LASTEXITCODE -ne 0){throw 'ResourceDelivery manifest validation failed'}
}
Copy-Item -LiteralPath $menuArchive -Destination $data -Force
$worldDest=Join-Path $data 'world';New-Item -ItemType Directory -Path $worldDest -Force|Out-Null
$nestedWorld=Join-Path $ResourceDelivery 'world'
 if(Test-Path -LiteralPath $nestedWorld){
 Get-ChildItem -LiteralPath $nestedWorld|ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $worldDest -Recurse -Force}
}elseif(Test-Path -LiteralPath (Join-Path $ResourceDelivery 'act-one-world.vwi')){
 # Current bundle output is a manifest-validated package root; adapt its
 # runtime files into the directory layout consumed by Vita.
 $actOneDest=Join-Path $worldDest 'act-one';New-Item -ItemType Directory -Path $actOneDest -Force|Out-Null
 foreach($name in @('act-one-world.vwi','act-one-entities.vwe','act-one-enemies.vwe','act-one-goblin-attack.vga','intro-terrain.png','intro-melee.vwa','intro-bow.vwb','items-weapons.png','item-loot.vlt')){
  $source=Join-Path $ResourceDelivery $name
  if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $actOneDest -Force}
 }
 # Some manually assembled campaign deliveries predate the base ACT_ONE
 # bundle's Goblin attack packet. Rebuild it from the local private source
 # data when available; never put that source data in the VPK or Git.
 $attackPath=Join-Path $actOneDest 'act-one-goblin-attack.vga'
 if(!(Test-Path -LiteralPath $attackPath)){
  $attackSource=Join-Path $repoRoot '_local/assets/attacks.json'
  if(Test-Path -LiteralPath $attackSource){
   & $Python (Join-Path $PSScriptRoot 'prepare_goblin_attack.py') $attackSource $attackPath
   if($LASTEXITCODE -ne 0){throw 'Cannot build the Goblin attack packet from local source data'}
  }else{
   $legacyAttack=Join-Path $repoRoot '_local/delivery-act-one-campaign077/act-one-goblin-attack.vga'
   if(Test-Path -LiteralPath $legacyAttack){Copy-Item -LiteralPath $legacyAttack -Destination $attackPath -Force}
   else{throw 'ACT_ONE delivery is missing act-one-goblin-attack.vga and no private source packet is available'}
  }
 }
 $itemAtlases=Join-Path $ResourceDelivery 'items'
 if(Test-Path -LiteralPath $itemAtlases){Copy-Item -LiteralPath $itemAtlases -Destination $actOneDest -Recurse -Force}
 foreach($directory in @('entities','enemies','bosses','knight','mage','rogue')){
  $source=Join-Path $ResourceDelivery $directory
  if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $actOneDest -Recurse -Force}
 }
 # Older campaign manifests can contain the four enemy families without the
 # required attack pose. Restore those private sprites from the last complete
 # local ACT_ONE delivery so the visual loader can enter the world.
 foreach($family in @('goblin','bat','lurker','slime')){
  $attackPose=Join-Path $actOneDest "enemies/$family/attack.png"
  if(!(Test-Path -LiteralPath $attackPose)){
   $fallbackPose=Join-Path $repoRoot "_local/delivery-act-one-campaign077/enemies/$family/attack.png"
   if(Test-Path -LiteralPath $fallbackPose){
    New-Item -ItemType Directory -Path (Split-Path -Parent $attackPose) -Force|Out-Null
    Copy-Item -LiteralPath $fallbackPose -Destination $attackPose -Force
   }else{throw "ACT_ONE delivery is missing required enemy attack pose: $family"}
  }
 }
 # world_visuals expects these optional sheets at entities/<name>.png,
 # while the package also carries torch/flower under entities/doodads/.
 foreach($name in @('torch','flower_1')){
  $flat=Join-Path $actOneDest "entities/$name.png"
  $nested=Join-Path $ResourceDelivery "entities/doodads/$name.png"
  if(!(Test-Path -LiteralPath $flat) -and (Test-Path -LiteralPath $nested)){
   Copy-Item -LiteralPath $nested -Destination $flat -Force
  }
 }
 $firefly=Join-Path $actOneDest 'entities/little_firefly.png'
 if(!(Test-Path -LiteralPath $firefly)){
  $packedFirefly=Join-Path $ResourceDelivery 'entities/little_firefly.png'
  $privateFirefly=Join-Path $repoRoot '_local/assets/gfx/scenes/intro/entities/little_firefly.png'
  if(Test-Path -LiteralPath $packedFirefly){Copy-Item -LiteralPath $packedFirefly -Destination $firefly -Force}
  elseif(Test-Path -LiteralPath $privateFirefly){Copy-Item -LiteralPath $privateFirefly -Destination $firefly -Force}
 }
 $sourceFloors=Join-Path $ResourceDelivery 'floors';$destFloors=Join-Path $actOneDest 'floors'
 if(Test-Path -LiteralPath $sourceFloors){
  New-Item -ItemType Directory -Path $destFloors -Force|Out-Null
  Get-ChildItem -LiteralPath $sourceFloors -File|Where-Object {$_.Extension -in @('.vwi','.vwe','.png')}|ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $destFloors -Force}
 }
}else{throw 'ResourceDelivery must contain world/ or an act-one-world.vwi package root'}
# Apply ACT_ONE path repairs to both package-root deliveries and deliveries
# already arranged as world/act-one/. Keep old nested packages runnable too.
$actOneDest=Join-Path $worldDest 'act-one'
if(Test-Path -LiteralPath (Join-Path $worldDest 'act-one-world.vwi')){$actOneDest=$worldDest}
if(Test-Path -LiteralPath (Join-Path $actOneDest 'act-one-world.vwi')){
 $deliveryActOne=Join-Path $ResourceDelivery 'world/act-one'
 if(!(Test-Path -LiteralPath (Join-Path $deliveryActOne 'act-one-world.vwi'))){
  if(Test-Path -LiteralPath (Join-Path $ResourceDelivery 'act-one-world.vwi')){$deliveryActOne=$ResourceDelivery}
  else{$deliveryActOne=Join-Path $ResourceDelivery 'world'}
 }
 $attackPath=Join-Path $actOneDest 'act-one-goblin-attack.vga'
 if(!(Test-Path -LiteralPath $attackPath)){
  $attackSource=Join-Path $repoRoot '_local/assets/attacks.json'
  if(Test-Path -LiteralPath $attackSource){
   & $Python (Join-Path $PSScriptRoot 'prepare_goblin_attack.py') $attackSource $attackPath
   if($LASTEXITCODE -ne 0){throw 'Cannot build the Goblin attack packet from local source data'}
  }else{
   $legacyAttack=Join-Path $repoRoot '_local/delivery-act-one-campaign077/act-one-goblin-attack.vga'
   if(Test-Path -LiteralPath $legacyAttack){Copy-Item -LiteralPath $legacyAttack -Destination $attackPath -Force}
   else{throw 'ACT_ONE delivery is missing act-one-goblin-attack.vga and no private source packet is available'}
  }
 }
 foreach($family in @('goblin','bat','lurker','slime')){
  $attackPose=Join-Path $actOneDest "enemies/$family/attack.png"
  if(!(Test-Path -LiteralPath $attackPose)){
   $sourcePose=Join-Path $deliveryActOne "enemies/$family/attack.png"
   if(!(Test-Path -LiteralPath $sourcePose)){$sourcePose=Join-Path $repoRoot "_local/delivery-act-one-campaign077/enemies/$family/attack.png"}
   if(Test-Path -LiteralPath $sourcePose){
    New-Item -ItemType Directory -Path (Split-Path -Parent $attackPose) -Force|Out-Null
    Copy-Item -LiteralPath $sourcePose -Destination $attackPose -Force
   }else{throw "ACT_ONE delivery is missing required enemy attack pose: $family"}
  }
 }
 foreach($name in @('torch','flower_1')){
  $flat=Join-Path $actOneDest "entities/$name.png"
  $nested=Join-Path $deliveryActOne "entities/doodads/$name.png"
  if(!(Test-Path -LiteralPath $nested)){$nested=Join-Path $ResourceDelivery "entities/doodads/$name.png"}
  if(!(Test-Path -LiteralPath $flat) -and (Test-Path -LiteralPath $nested)){Copy-Item -LiteralPath $nested -Destination $flat -Force}
 }
 $firefly=Join-Path $actOneDest 'entities/little_firefly.png'
 if(!(Test-Path -LiteralPath $firefly)){
  $packedFirefly=Join-Path $deliveryActOne 'entities/little_firefly.png'
  if(!(Test-Path -LiteralPath $packedFirefly)){$packedFirefly=Join-Path $repoRoot '_local/assets/gfx/scenes/intro/entities/little_firefly.png'}
  if(Test-Path -LiteralPath $packedFirefly){Copy-Item -LiteralPath $packedFirefly -Destination $firefly -Force}
 }
}
if(Test-Path -LiteralPath (Join-Path $actOneDest 'act-one-world.vwi')){
 & $Python (Join-Path $PSScriptRoot 'audit_act_one_runtime_assets.py') $actOneDest
 if($LASTEXITCODE -ne 0){throw 'ACT_ONE runtime asset preflight failed; see missing or invalid path above'}
}
if($IntroEntrySmoke){
 if(!$IntroResourceDelivery){$IntroResourceDelivery=Join-Path $repoRoot '_local/world-runtime023-bundle'}
 if(!(Test-Path -LiteralPath $IntroResourceDelivery)){throw "Intro resource delivery is missing: $IntroResourceDelivery"}
 & $Python (Join-Path $PSScriptRoot 'stage_intro_bundle.py') $IntroResourceDelivery $worldDest
 if($LASTEXITCODE -ne 0){throw 'Intro resource bundle verification/staging failed'}
}
$logDir=Join-Path $data ('logs/iteration'+$Iteration)
$before=@();if(Test-Path -LiteralPath $logDir){$before=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|ForEach-Object {$_.Name})}
$replay=Join-Path $data 'autotest.vts'
$smokeModes=@();if($ActOneExitSmoke){$smokeModes+='act-one-exit'};if($ActOneCampaignSmoke){$smokeModes+='act-one-campaign'};if($IntroEntrySmoke){$smokeModes+='intro-entry'}
if($smokeModes.Count -gt 1){throw 'Choose one world traversal smoke mode'}
$replayArgs=@();if($IntroEntrySmoke){
 $campaign=Join-Path $ResourceDelivery 'world/act-one'
 if(!(Test-Path -LiteralPath (Join-Path $campaign 'route-proofs/floor-00.json'))){$campaign=$ResourceDelivery}
 if(!(Test-Path -LiteralPath (Join-Path $campaign 'route-proofs/floor-00.json'))){throw 'IntroEntrySmoke requires a campaign root with a player_step floor-0 route proof'}
 if(!$IntroTrace){$IntroTrace=Join-Path $repoRoot '_local/intro-route159/intro.vtr'}
 if(!(Test-Path -LiteralPath $IntroTrace)){throw 'IntroEntrySmoke requires -IntroTrace from the native intro route probe'}
 $replayArgs=@('--intro-cave-entry','--campaign',$campaign,'--intro-trace',$IntroTrace,'--player-class',$IntroEntryClass)
 if($ActOneTrace){$replayArgs+=@('--route-trace',$ActOneTrace)}
}elseif($ActOneCampaignSmoke){
 $campaign=Join-Path $ResourceDelivery 'world/act-one'
 if(!(Test-Path -LiteralPath (Join-Path $campaign 'act-one-layout.json'))){$campaign=$ResourceDelivery}
 if(!(Test-Path -LiteralPath (Join-Path $campaign 'act-one-layout.json'))){throw 'ActOneCampaignSmoke requires a campaign package with three layout JSON files'}
 $replayArgs=@('--act-one-campaign-exit','--campaign',$campaign,'--player-class',$ActOneCampaignClass)
}elseif($ActOneExitSmoke){
 if(!$ActOneLayout){
  $deliveredLayout=Join-Path $ResourceDelivery 'world/act-one/act-one-layout.json'
  if(!(Test-Path -LiteralPath $deliveredLayout)){$deliveredLayout=Join-Path $ResourceDelivery 'act-one-layout.json'}
  $ActOneLayout=if(Test-Path -LiteralPath $deliveredLayout){$deliveredLayout}else{Join-Path $repoRoot '_local/generation045/map-export/candidate-029/act-one-layout.json'}
 }
 if(!(Test-Path -LiteralPath $ActOneLayout)){throw 'ActOneExitSmoke requires -ActOneLayout with the layout used for this VPK'}
 $replayArgs=@('--act-one-exit','--layout',$ActOneLayout,'--player-class',$ActOneExitClass)
 if($ActOneTrace){$replayArgs+=@('--route-trace',$ActOneTrace)}
}
if(!$Visible){$replayArgs+='--fast-forward'}
$replayOutput=& $Python (Join-Path $PSScriptRoot 'prepare_smoke_replay.py') $replay @replayArgs
if($LASTEXITCODE -ne 0){throw 'Cannot prepare input sequence'}
$replaySummary=($replayOutput -join "`n")
Write-Output $replaySummary
$routeSources=''
$routeSourceMatch=[regex]::Match($replaySummary,'(?:^|\s)route_sources=([^\s]+)')
if($routeSourceMatch.Success){$routeSources=$routeSourceMatch.Groups[1].Value}
Copy-Item -LiteralPath $replay -Destination (Join-Path $run 'input.vts')
$resourceEntries=@(Get-ChildItem -LiteralPath $data -File -Recurse -Force |
 Where-Object {$_.FullName -notmatch '[\\/]logs[\\/]' -and $_.Name -ne 'autotest.vts'} |
 ForEach-Object {
  $relative=$_.FullName.Substring($data.Length).TrimStart([char[]]@([IO.Path]::DirectorySeparatorChar,[IO.Path]::AltDirectorySeparatorChar)).Replace([IO.Path]::DirectorySeparatorChar,'/')
  [pscustomobject]@{path=$relative;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
 } | Sort-Object -Property path)
$resourceManifestPath=Join-Path $run 'resources.json'
$resourceEntries | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $resourceManifestPath -Encoding UTF8
$resourceManifestMaterial=($resourceEntries | ForEach-Object {$_.path+"`t"+$_.sha256}) -join "`n"
$resourceHashAlgorithm=[Security.Cryptography.SHA256]::Create()
try{$resourceManifestHash=[BitConverter]::ToString($resourceHashAlgorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($resourceManifestMaterial))).Replace('-','')}
finally{$resourceHashAlgorithm.Dispose()}
$zip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Vpk).Path)
try {
$allowed=@('eboot.bin','sce_sys/param.sfo','sce_sys/icon0.png','sce_sys/pic0.png','sce_sys/livearea/contents/bg0.png','sce_sys/livearea/contents/startup.png','sce_sys/livearea/contents/template.xml')
 foreach($entry in $zip.Entries){if(!$entry.FullName.EndsWith('/') -and $entry.FullName -notin $allowed){throw 'Unexpected VPK entry'}}
} finally {$zip.Dispose()}
$appDir=Join-Path $filesystem 'ux0/app/VAGV00006'
if(Test-Path -LiteralPath $appDir){Remove-Item -LiteralPath $appDir -Recurse -Force}
if(!$InstallVpkViaCli){
 New-Item -ItemType Directory -Path $appDir -Force|Out-Null
 $installZip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Vpk).Path)
 try {
  foreach($entry in $installZip.Entries){
   if($entry.FullName.EndsWith('/')){continue}
   $destination=Join-Path $appDir $entry.FullName
   New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force|Out-Null
   [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$destination,$true)
  }
 } finally {$installZip.Dispose()}
}
$start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=Join-Path $runtime 'Vita3K.exe';$start.WorkingDirectory=$runtime
$start.UseShellExecute=$false;$start.CreateNoWindow=(-not $Visible)
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
$launchArgs=@()
if($Console){$launchArgs+='--console'}else{$launchArgs+=@('--frontend',$Frontend)}
$launchArgs+=@('--load-config','--keep-config','--config-location',(Join-Path $runtime 'config.yml'))
if($InstallVpkViaCli){$launchArgs+= (Resolve-Path -LiteralPath $Vpk).Path}else{$launchArgs+=@('-r','VAGV00006')}
if($null -ne $start.ArgumentList){foreach($arg in $launchArgs){$start.ArgumentList.Add($arg)}}
else{$start.Arguments=($launchArgs|ForEach-Object {'"'+($_ -replace '"','\"')+'"'}) -join ' '}
# This file belongs to the isolated test runtime. Clear it so Collect cannot
# mistake an emulator log copied from an earlier run for this launch's log.
[IO.File]::WriteAllText((Join-Path $runtime 'vita3k.log'),'')
$process=[Diagnostics.Process]::Start($start)
$stdoutTask=$process.StandardOutput.ReadToEndAsync();$stderrTask=$process.StandardError.ReadToEndAsync()
$expectedWorld=if($IntroEntrySmoke){'intro-then-act-one'}elseif(!$IntroOnly -and ((Test-Path -LiteralPath (Join-Path $ResourceDelivery 'world/act-one/act-one-world.vwi')) -or (Test-Path -LiteralPath (Join-Path $ResourceDelivery 'act-one-world.vwi')))){'act-one'}else{'intro'}
$info=[ordered]@{iteration=$Iteration;processId=$process.Id;executable=$start.FileName;emulator_product_version=(Get-Item -LiteralPath $start.FileName).VersionInfo.ProductVersion;runtimeDirectory=$runtime;renderer=$Renderer;frontend=$(if($Console){'console'}else{$Frontend});console=[bool]$Console;launch_mode=$(if($InstallVpkViaCli){'vpk-content-path'}else{'installed-title-id'});timeoutSeconds=$TimeoutSeconds;vpk=(Resolve-Path -LiteralPath $Vpk).Path;vpk_sha256=(Get-FileHash -LiteralPath $Vpk).Hash;emulator_sha256=(Get-FileHash -LiteralPath $start.FileName).Hash;resource_delivery=(Resolve-Path -LiteralPath $ResourceDelivery).Path;resource_file_count=$resourceEntries.Count;resource_manifest_sha256=$resourceManifestHash;resource_manifest='resources.json';input_sha256=(Get-FileHash -LiteralPath $replay -Algorithm SHA256).Hash;filesystem=$filesystem;copy_complete_instance=[bool]$CopyCompleteInstance;instance_source=(Resolve-Path -LiteralPath $EmulatorDirectory).Path;previousLogs=$before;input_source='external replay';routeSources=$routeSources;expectedWorld=$expectedWorld;actOneExitSmoke=[bool]$ActOneExitSmoke;actOneExitClass=$ActOneExitClass;actOneCampaignSmoke=[bool]$ActOneCampaignSmoke;introEntrySmoke=[bool]$IntroEntrySmoke;introEntryClass=$IntroEntryClass;visible=[bool]$Visible;fast_forward=(!$Visible);physical_controller_verified=$false}
$info|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $run 'run.json')
[pscustomobject]@{RunDirectory=$run;ProcessId=$process.Id;CollectCommand='tools/test_vita3k.ps1 -Action Collect -RunDirectory '+$run}|ConvertTo-Json

if($Action -eq 'Run'){
 $watch=[Diagnostics.Stopwatch]::StartNew();$timedOut=$false;$stoppedAfterReplay=$false
 while(!$process.HasExited -and $watch.Elapsed.TotalSeconds -lt $TimeoutSeconds){
  if($Frontend -eq 'qt'){
   $newSessionLogs=@();if(Test-Path -LiteralPath $logDir){$newSessionLogs=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|Where-Object {$_.Name -notin @($before)})}
   $replayReached=$false
   foreach($session in $newSessionLogs){
    $eventLog=$session.FullName+'.events'
    $completionMarkers=@('^AUTOTEST sequence_consumed=1 ')
    if($ActOneCampaignSmoke){$completionMarkers+='^AUTOTEST act_one_campaign_sequence_consumed=1$'}
    if($IntroEntrySmoke){$completionMarkers+='^AUTOTEST intro_entry_sequence_consumed=1$'}
    if($ActOneExitSmoke){$completionMarkers+='^AUTOTEST act_one_exit_sequence_consumed=1$'}
    foreach($completionMarker in $completionMarkers){
     if((Select-String -LiteralPath $session.FullName -Pattern $completionMarker -Quiet) -or
        ((Test-Path -LiteralPath $eventLog) -and (Select-String -LiteralPath $eventLog -Pattern $completionMarker -Quiet))){$replayReached=$true;break}
    }
    if($replayReached){break}
   }
   if($replayReached){
    $stoppedAfterReplay=$true
    if($process.CloseMainWindow()){$null=$process.WaitForExit(8000)}
    break
   }
  }
  $null=$process.WaitForExit(250)
 }
 $timedOut=(!$process.HasExited -and !$stoppedAfterReplay)
 if($timedOut){
  $live=Get-Process -Id $process.Id -ErrorAction SilentlyContinue
  if($live -and $live.Path -eq $start.FileName){$live|Stop-Process -Force}
  if(!$process.WaitForExit(10000)){throw "Timed-out Vita3K process $($process.Id) did not exit after being stopped"}
 }
 if($stoppedAfterReplay -and !$process.HasExited){
  $live=Get-Process -Id $process.Id -ErrorAction SilentlyContinue
  if($live -and $live.Path -eq $start.FileName){$live|Stop-Process -Force}
  if(!$process.WaitForExit(10000)){throw "Completed replay's Vita3K process $($process.Id) did not exit after being stopped"}
 }
 try{[IO.File]::WriteAllText((Join-Path $run 'stdout.log'),$stdoutTask.GetAwaiter().GetResult())}catch{[IO.File]::WriteAllText((Join-Path $run 'stdout.log'),'Capture failed: '+$_.Exception.Message)}
 try{[IO.File]::WriteAllText((Join-Path $run 'stderr.log'),$stderrTask.GetAwaiter().GetResult())}catch{[IO.File]::WriteAllText((Join-Path $run 'stderr.log'),'Capture failed: '+$_.Exception.Message)}
 foreach($debugName in @('debug-stdout.log','debug-stderr.log')){
  $debugSource=Join-Path $runtime $debugName
  if(Test-Path -LiteralPath $debugSource){Copy-Item -LiteralPath $debugSource -Destination (Join-Path $run $debugName) -Force}
 }
 $exitCode=$null;if($process.HasExited){try{$exitCode=$process.ExitCode}catch{}}
 [pscustomobject]@{exitCode=$exitCode;timedOut=$timedOut;stoppedAfterReplay=$stoppedAfterReplay;timeoutSeconds=$TimeoutSeconds;completedUtc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'process.json')
 if($ActOneExitSmoke -or $ActOneCampaignSmoke){& $PSCommandPath -Action Collect -RunDirectory $run -Python $Python -ActOneExitSmoke:$ActOneExitSmoke -ActOneCampaignSmoke:$ActOneCampaignSmoke}
 else{& $PSCommandPath -Action Collect -RunDirectory $run -Python $Python}
}
