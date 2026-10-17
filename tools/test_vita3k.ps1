param(
 [ValidateSet('Run','Launch','Collect')][string]$Action='Run',
 [ValidatePattern('^[0-9]{3}$')][string]$Iteration='028',
 [string]$EmulatorDirectory=(Join-Path (Split-Path $PSScriptRoot -Parent) '_local/vita3k'),
 [string]$Vpk,
 [string]$ResourceDelivery,
 [string]$Python=(Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'),
 [string]$RunDirectory,
 [string]$TestRoot,
 [string]$ActOneLayout,
 [ValidateRange(1,600)][int]$TimeoutSeconds=45,
 [ValidateSet('Vulkan','OpenGL')][string]$Renderer='Vulkan',
 [ValidateSet('sdl','qt')][string]$Frontend='sdl',
 [switch]$ActOneExitSmoke,
 [switch]$ActOneCampaignSmoke,
 [switch]$IntroOnly,
 [switch]$Visible,
 [switch]$InstallVpkViaCli,
 [switch]$Console
)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repoRoot=Split-Path $PSScriptRoot -Parent
$testRoot=if($TestRoot){[IO.Path]::GetFullPath($TestRoot)}else{Join-Path $repoRoot '_local/vita3k-vagante-dedicated'}
$runtime=Join-Path $testRoot 'runtime'
$filesystem=Join-Path $testRoot 'filesystem'
if($Action -eq 'Collect'){
 if(!$RunDirectory){throw 'RunDirectory is required for Collect'}
 $info=Get-Content -LiteralPath (Join-Path $RunDirectory 'run.json') -Raw|ConvertFrom-Json
 if($info.actOneExitSmoke){$ActOneExitSmoke=$true}
 if($info.actOneCampaignSmoke){$ActOneCampaignSmoke=$true}
 $filesystem=$info.filesystem;$testRoot=Split-Path $filesystem -Parent;$runtime=Join-Path $testRoot 'runtime'
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
  $diagnostic=[ordered]@{passed=$false;reason=$failureReason;failure_stage='before_native_session';process_id=$info.processId;process_exit_code=$exitCode;process_exit_code_hex=$exitCodeHex;process_timed_out=$timedOut;expected_iteration=$info.iteration;expected_title_version=('0.'+$info.iteration.Substring(1,2));expected_world=$info.expectedWorld;route_sources=$info.routeSources;emulator_log_available=$hasEmulatorLog;stdout_available=(Test-Path -LiteralPath (Join-Path $RunDirectory 'stdout.log'));stderr_available=(Test-Path -LiteralPath (Join-Path $RunDirectory 'stderr.log'))}
  $diagnostic|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $RunDirectory 'result.json')
  throw "$failureReason (process exit $exitCode). Inspect stdout.log, stderr.log, and vita3k.log in $RunDirectory"
 }
 $dest=Join-Path $RunDirectory 'runtime.log'
 if(Test-Path -LiteralPath $dest){throw 'This run has already been collected'}
 Copy-Item -LiteralPath $newLogs[0].FullName -Destination $dest
 $exitArgs=@();$processRecord=Join-Path $RunDirectory 'process.json'
 $harnessStopArgs=@()
 if(Test-Path -LiteralPath $processRecord){$record=Get-Content -LiteralPath $processRecord -Raw|ConvertFrom-Json;if($record.stoppedAfterReplay){$harnessStopArgs=@('--allow-harness-stop')};if(!$record.stoppedAfterReplay -or $record.exitCode -eq 0){$exitArgs=@('--process-exit-code',[string]$record.exitCode)}}
 $worldArgs=@();if($info.expectedWorld -eq 'act-one'){$worldArgs=@('--require-act-one')}
 if($ActOneExitSmoke){$worldArgs=@('--require-act-one-exit')}
 if($ActOneCampaignSmoke){$worldArgs=@('--require-act-one-campaign')}
 $routeSourceArgs=@();if($info.routeSources){$routeSourceArgs=@('--route-sources',[string]$info.routeSources)}
 & $Python (Join-Path $PSScriptRoot 'check_vita3k_smoke.py') $dest --output (Join-Path $RunDirectory 'result.json') --emulator-log (Join-Path $RunDirectory 'vita3k.log') --expected-iteration $info.iteration @exitArgs @routeSourceArgs @worldArgs @harnessStopArgs
 if($LASTEXITCODE -ne 0){throw 'Native replay assertions failed; inspect result.json and runtime.log'}
 return
}
if(!$Vpk){$Vpk=Join-Path $repoRoot ('artifacts/iteration'+$Iteration+'/Vagante2Vita-iteration'+$Iteration+'.vpk')}
if(!$ResourceDelivery){$ResourceDelivery=Join-Path $repoRoot ('artifacts/iteration'+$Iteration)}
if($Console -and !$InstallVpkViaCli){throw '-Console requires -InstallVpkViaCli because Vita3K console mode accepts a VPK content path'}
$sourceConfig=Join-Path $EmulatorDirectory 'config.yml'
if(!(Test-Path -LiteralPath $sourceConfig)){$sourceConfig=Join-Path $EmulatorDirectory 'runtime/config.yml'}
$originalConfig=[IO.File]::ReadAllText($sourceConfig)
$originalFs=([regex]::Match($originalConfig,'(?m)^pref-path:[ \t]*([^\r\n]+)').Groups[1].Value).Trim().Trim([char[]]@([char]39,[char]34))
if(!$originalFs){throw 'Cannot resolve emulator filesystem from config'}
if(!(Test-Path -LiteralPath $Vpk)){throw 'VPK missing'}
if(Get-Process Vita3K -ErrorAction SilentlyContinue|Where-Object {$_.Path -eq (Join-Path $runtime 'Vita3K.exe')}){throw 'Sandbox test is already running'}
New-Item -ItemType Directory -Path $runtime,$filesystem -Force|Out-Null
$sharedExe=Join-Path $EmulatorDirectory 'Vita3K.exe'
if(!(Test-Path -LiteralPath $sharedExe)){$sharedExe=Join-Path $EmulatorDirectory 'runtime/Vita3K.exe'}
$runtimeExe=Join-Path $runtime 'Vita3K.exe'
if(!(Test-Path -LiteralPath $sharedExe)){throw "Vita3K executable missing: $sharedExe"}
function Sync-RuntimeFile([string]$Source,[string]$Destination){
 if(!(Test-Path -LiteralPath $Destination)){$copy=$true}
 else{$copy=(Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash}
 if($copy){$parent=Split-Path -Parent $Destination;New-Item -ItemType Directory -Path $parent -Force|Out-Null;Copy-Item -LiteralPath $Source -Destination $Destination -Force}
}
Sync-RuntimeFile $sharedExe $runtimeExe
Get-ChildItem -LiteralPath $EmulatorDirectory -File|Where-Object {$_.Extension -eq '.dll'}|ForEach-Object {Sync-RuntimeFile $_.FullName (Join-Path $runtime $_.Name)}
# Copy only immutable runtime assets. Never clone shared data, saves, caches,
# GUI preferences or shader/texture logs into the isolated test environment.
foreach($directory in @('share','plugins','translations','icons','patch','shaders-builtin')){
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
if(Test-Path -LiteralPath $themeRoot){
 $themeBase=(Resolve-Path -LiteralPath $themeRoot).Path
 Get-ChildItem -LiteralPath $themeBase -File -Recurse|ForEach-Object {
  $relative=$_.FullName.Substring($themeBase.Length).TrimStart('\','/')
  Sync-RuntimeFile $_.FullName (Join-Path (Join-Path $runtime 'data/gui-configs/custom-themes') $relative)
 }
}
if(!(Test-Path -LiteralPath (Join-Path $runtime 'vita3k.log'))){[IO.File]::WriteAllText((Join-Path $runtime 'vita3k.log'),'')}
foreach($debugName in @('debug-stdout.log','debug-stderr.log')){[IO.File]::WriteAllText((Join-Path $runtime $debugName),'')}
# Copy only firmware partitions from the source profile. sa0 can contain
# per-user application data, so keep it empty in this isolated profile.
foreach($partition in @('vs0','os0')){
 $source=Join-Path $originalFs $partition;$target=Join-Path $filesystem $partition
 if(!(Test-Path -LiteralPath $target) -and (Test-Path -LiteralPath $source)){Copy-Item -LiteralPath $source -Destination $target -Recurse}
}
$runs=Join-Path $testRoot ('runs/iteration'+$Iteration);New-Item -ItemType Directory -Path $runs -Force|Out-Null
for($n=1;$n -le 999999;$n++){
 $run=Join-Path $runs ('run-{0:d6}' -f $n)
 if(!(Test-Path -LiteralPath $run)){New-Item -ItemType Directory -Path $run|Out-Null;break}
}
$quotedPrefPath="pref-path: '"+$filesystem.Replace("'","''")+"\'"
$cfg=[regex]::Replace($originalConfig,'(?m)^pref-path:[^\r\n]*',[System.Text.RegularExpressions.MatchEvaluator]{param($match)$quotedPrefPath})
$cfg=[regex]::Replace($cfg,'(?m)^log-level:[^\r\n]*','log-level: 2')
$cfg=[regex]::Replace($cfg,'(?m)^backend-renderer:[^\r\n]*','backend-renderer: '+$Renderer)
if($Renderer -eq 'OpenGL'){$cfg=[regex]::Replace($cfg,'(?m)^validation-layer:[^\r\n]*','validation-layer: false')}
[IO.File]::WriteAllText((Join-Path $runtime 'config.yml'),$cfg,[Text.UTF8Encoding]::new($false))
$data=Join-Path $filesystem 'ux0/data/Vagante2Vita';New-Item -ItemType Directory -Path $data -Force|Out-Null
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
  }else{throw 'ACT_ONE delivery is missing act-one-goblin-attack.vga and _local/assets/attacks.json is unavailable'}
 }
 $itemAtlases=Join-Path $ResourceDelivery 'items'
 if(Test-Path -LiteralPath $itemAtlases){Copy-Item -LiteralPath $itemAtlases -Destination $actOneDest -Recurse -Force}
 foreach($directory in @('entities','enemies','knight','mage','rogue')){
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
$logDir=Join-Path $data ('logs/iteration'+$Iteration)
$before=@();if(Test-Path -LiteralPath $logDir){$before=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|ForEach-Object {$_.Name})}
$replay=Join-Path $data 'autotest.vts'
if($ActOneExitSmoke -and $ActOneCampaignSmoke){throw 'Choose one ACT_ONE smoke mode'}
$replayArgs=@();if($ActOneCampaignSmoke){
 $campaign=Join-Path $ResourceDelivery 'world/act-one'
 if(!(Test-Path -LiteralPath (Join-Path $campaign 'act-one-layout.json'))){$campaign=$ResourceDelivery}
 if(!(Test-Path -LiteralPath (Join-Path $campaign 'act-one-layout.json'))){throw 'ActOneCampaignSmoke requires a campaign package with three layout JSON files'}
 $replayArgs=@('--act-one-campaign-exit','--campaign',$campaign)
}elseif($ActOneExitSmoke){
 if(!$ActOneLayout){
  $deliveredLayout=Join-Path $ResourceDelivery 'world/act-one/act-one-layout.json'
  if(!(Test-Path -LiteralPath $deliveredLayout)){$deliveredLayout=Join-Path $ResourceDelivery 'act-one-layout.json'}
  $ActOneLayout=if(Test-Path -LiteralPath $deliveredLayout){$deliveredLayout}else{Join-Path $repoRoot '_local/generation045/map-export/candidate-029/act-one-layout.json'}
 }
 if(!(Test-Path -LiteralPath $ActOneLayout)){throw 'ActOneExitSmoke requires -ActOneLayout with the layout used for this VPK'}
 $replayArgs=@('--act-one-exit','--layout',$ActOneLayout)
}
$replayOutput=& $Python (Join-Path $PSScriptRoot 'prepare_smoke_replay.py') $replay @replayArgs
if($LASTEXITCODE -ne 0){throw 'Cannot prepare input sequence'}
$replaySummary=($replayOutput -join "`n")
Write-Output $replaySummary
$routeSources=''
$routeSourceMatch=[regex]::Match($replaySummary,'(?:^|\s)route_sources=([^\s]+)')
if($routeSourceMatch.Success){$routeSources=$routeSourceMatch.Groups[1].Value}
Copy-Item -LiteralPath $replay -Destination (Join-Path $run 'input.vts')
$zip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Vpk).Path)
try {
$allowed=@('eboot.bin','sce_sys/param.sfo','sce_sys/icon0.png','sce_sys/pic0.png','sce_sys/livearea/contents/bg0.png','sce_sys/livearea/contents/startup.png','sce_sys/livearea/contents/template.xml')
 foreach($entry in $zip.Entries){if(!$entry.FullName.EndsWith('/') -and $entry.FullName -notin $allowed){throw 'Unexpected VPK entry'}}
} finally {$zip.Dispose()}
$appDir=Join-Path $filesystem 'ux0/app/VAGV00006'
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
$expectedWorld=if(!$IntroOnly -and ((Test-Path -LiteralPath (Join-Path $ResourceDelivery 'world/act-one/act-one-world.vwi')) -or (Test-Path -LiteralPath (Join-Path $ResourceDelivery 'act-one-world.vwi')))){'act-one'}else{'intro'}
$info=[ordered]@{iteration=$Iteration;processId=$process.Id;executable=$start.FileName;renderer=$Renderer;frontend=$(if($Console){'console'}else{$Frontend});console=[bool]$Console;launch_mode=$(if($InstallVpkViaCli){'vpk-content-path'}else{'installed-title-id'});timeoutSeconds=$TimeoutSeconds;vpk=(Resolve-Path -LiteralPath $Vpk).Path;vpk_sha256=(Get-FileHash -LiteralPath $Vpk).Hash;emulator_sha256=(Get-FileHash -LiteralPath $start.FileName).Hash;filesystem=$filesystem;previousLogs=$before;input_source='external replay';routeSources=$routeSources;expectedWorld=$expectedWorld;actOneExitSmoke=[bool]$ActOneExitSmoke;actOneCampaignSmoke=[bool]$ActOneCampaignSmoke;visible=[bool]$Visible;physical_controller_verified=$false}
$info|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $run 'run.json')
[pscustomobject]@{RunDirectory=$run;ProcessId=$process.Id;CollectCommand='tools/test_vita3k.ps1 -Action Collect -RunDirectory '+$run}|ConvertTo-Json

if($Action -eq 'Run'){
 $watch=[Diagnostics.Stopwatch]::StartNew();$timedOut=$false;$stoppedAfterReplay=$false
 while(!$process.HasExited -and $watch.Elapsed.TotalSeconds -lt $TimeoutSeconds){
  if($Frontend -eq 'qt' -and ($ActOneExitSmoke -or $ActOneCampaignSmoke)){
   $newSessionLogs=@();if(Test-Path -LiteralPath $logDir){$newSessionLogs=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|Where-Object {$_.Name -notin @($before)})}
   $replayReached=$false
   foreach($session in $newSessionLogs){$completionMarker=if($ActOneCampaignSmoke){'^AUTOTEST act_one_campaign_sequence_consumed=1$'}else{'^AUTOTEST act_one_exit_sequence_consumed=1$'}
   if(Select-String -LiteralPath $session.FullName -Pattern $completionMarker -Quiet){$replayReached=$true;break}}
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
