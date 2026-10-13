param(
 [ValidateSet('Run','Launch','Collect')][string]$Action='Run',
 [ValidatePattern('^[0-9]{3}$')][string]$Iteration='025',
 [string]$EmulatorDirectory=(Join-Path $env:USERPROFILE 'Downloads/Vita/windows-latest'),
 [string]$Vpk,
 [string]$ResourceDelivery,
 [string]$Python=(Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'),
 [string]$RunDirectory
)
$ErrorActionPreference='Stop'
$repoRoot=Split-Path $PSScriptRoot -Parent
$testRoot=Join-Path $repoRoot '_local/vita3k'
$runtime=Join-Path $testRoot 'runtime'
$filesystem=Join-Path $testRoot 'filesystem'
if($Action -eq 'Collect'){
 if(!$RunDirectory){throw 'RunDirectory is required for Collect'}
 $info=Get-Content -LiteralPath (Join-Path $RunDirectory 'run.json') -Raw|ConvertFrom-Json
 $live=Get-Process -Id $info.processId -ErrorAction SilentlyContinue
 if($live -and $live.Path -eq $info.executable){throw 'The specific test process is still running; collect after it exits'}
 $logDir=Join-Path $filesystem ('ux0/data/Vagante2Vita/logs/iteration'+$info.iteration)
 $newLogs=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|Where-Object {$_.Name -notin @($info.previousLogs)})
 if($newLogs.Count -ne 1){throw "Expected one new native session log, found $($newLogs.Count)"}
 $dest=Join-Path $RunDirectory 'runtime.log'
 if(Test-Path -LiteralPath $dest){throw 'This run has already been collected'}
 Copy-Item -LiteralPath $newLogs[0].FullName -Destination $dest
 Copy-Item -LiteralPath (Join-Path $runtime 'vita3k.log') -Destination (Join-Path $RunDirectory 'vita3k.log')
 $exitArgs=@();$processRecord=Join-Path $RunDirectory 'process.json'
 if(Test-Path -LiteralPath $processRecord){$record=Get-Content -LiteralPath $processRecord -Raw|ConvertFrom-Json;$exitArgs=@('--process-exit-code',[string]$record.exitCode)}
 & $Python (Join-Path $PSScriptRoot 'check_vita3k_smoke.py') $dest --output (Join-Path $RunDirectory 'result.json') --emulator-log (Join-Path $RunDirectory 'vita3k.log') @exitArgs
 if($LASTEXITCODE -ne 0){throw 'Native replay assertions failed; inspect result.json and runtime.log'}
 return
}
if(!$Vpk){$Vpk=Join-Path $repoRoot ('artifacts/iteration'+$Iteration+'/Vagante2Vita-iteration'+$Iteration+'.vpk')}
if(!$ResourceDelivery){$ResourceDelivery=Join-Path $repoRoot ('artifacts/iteration'+$Iteration)}
$sourceConfig=Join-Path $EmulatorDirectory 'config.yml'
$originalConfig=[IO.File]::ReadAllText($sourceConfig)
$originalFs=([regex]::Match($originalConfig,'(?m)^pref-path:\s*(.+)$').Groups[1].Value).Trim().Trim('"')
if(!$originalFs){throw 'Cannot resolve emulator filesystem from config'}
if(!(Test-Path -LiteralPath $Vpk)){throw 'VPK missing'}
if(Get-Process Vita3K -ErrorAction SilentlyContinue|Where-Object {$_.Path -eq (Join-Path $runtime 'Vita3K.exe')}){throw 'Sandbox test is already running'}
New-Item -ItemType Directory -Path $runtime,$filesystem -Force|Out-Null
if(!(Test-Path -LiteralPath (Join-Path $runtime 'Vita3K.exe'))){
 Get-ChildItem -LiteralPath $EmulatorDirectory -File|Where-Object {$_.Extension -eq '.dll' -or $_.Name -eq 'Vita3K.exe'}|ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $runtime}
 foreach($name in @('data','shaders-builtin','plugins','translations','icons')){
  $source=Join-Path $EmulatorDirectory $name
  if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $runtime -Recurse}
 }
}
foreach($partition in @('vs0','os0','sa0')){
 $source=Join-Path $originalFs $partition;$target=Join-Path $filesystem $partition
 if(!(Test-Path -LiteralPath $target) -and (Test-Path -LiteralPath $source)){Copy-Item -LiteralPath $source -Destination $target -Recurse}
}
$runs=Join-Path $testRoot ('runs/iteration'+$Iteration);New-Item -ItemType Directory -Path $runs -Force|Out-Null
for($n=1;$n -le 999999;$n++){
 $run=Join-Path $runs ('run-{0:d6}' -f $n)
 if(!(Test-Path -LiteralPath $run)){New-Item -ItemType Directory -Path $run|Out-Null;break}
}
$cfg=[regex]::Replace($originalConfig,'(?m)^pref-path:[^\r\n]*','pref-path: '+$filesystem+'\')
$cfg=[regex]::Replace($cfg,'(?m)^log-level:[^\r\n]*','log-level: 2')
[IO.File]::WriteAllText((Join-Path $runtime 'config.yml'),$cfg,[Text.UTF8Encoding]::new($false))
$data=Join-Path $filesystem 'ux0/data/Vagante2Vita';New-Item -ItemType Directory -Path $data -Force|Out-Null
Copy-Item -LiteralPath (Join-Path $ResourceDelivery 'menu-only.vra') -Destination $data -Force
$worldDest=Join-Path $data 'world';New-Item -ItemType Directory -Path $worldDest -Force|Out-Null
Get-ChildItem -LiteralPath (Join-Path $ResourceDelivery 'world')|ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $worldDest -Recurse -Force}
$logDir=Join-Path $data ('logs/iteration'+$Iteration)
$before=@();if(Test-Path -LiteralPath $logDir){$before=@(Get-ChildItem -LiteralPath $logDir -Filter 'session-*.log'|ForEach-Object {$_.Name})}
$replay=Join-Path $data 'autotest.vts'
& $Python (Join-Path $PSScriptRoot 'prepare_smoke_replay.py') $replay
if($LASTEXITCODE -ne 0){throw 'Cannot prepare input sequence'}
Copy-Item -LiteralPath $replay -Destination (Join-Path $run 'input.vts')
$appDir=Join-Path $filesystem 'ux0/app/VAGV00006';New-Item -ItemType Directory -Path $appDir -Force|Out-Null
$zip=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Vpk).Path)
try {
 $allowed=@('eboot.bin','sce_sys/param.sfo','sce_sys/icon0.png','sce_sys/pic0.png','sce_sys/livearea/contents/bg0.png','sce_sys/livearea/contents/startup.png','sce_sys/livearea/contents/template.xml')
 foreach($entry in $zip.Entries){if(!$entry.FullName.EndsWith('/') -and $entry.FullName -notin $allowed){throw 'Unexpected VPK entry'}}
} finally {$zip.Dispose()}
[IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path -LiteralPath $Vpk).Path,$appDir,$true)
$start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=Join-Path $runtime 'Vita3K.exe';$start.WorkingDirectory=$runtime
$start.UseShellExecute=$false;$start.CreateNoWindow=$true
foreach($arg in @('--frontend','sdl','--load-config','--keep-config','--config-location',(Join-Path $runtime 'config.yml'),'-r','VAGV00006')){$start.ArgumentList.Add($arg)}
$process=[Diagnostics.Process]::Start($start)
$info=[ordered]@{iteration=$Iteration;processId=$process.Id;executable=$start.FileName;vpk=(Resolve-Path -LiteralPath $Vpk).Path;vpk_sha256=(Get-FileHash -LiteralPath $Vpk).Hash;emulator_sha256=(Get-FileHash -LiteralPath $start.FileName).Hash;filesystem=$filesystem;previousLogs=$before;input_source='external replay';physical_controller_verified=$false}
$info|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $run 'run.json')
[pscustomobject]@{RunDirectory=$run;ProcessId=$process.Id;CollectCommand='tools/test_vita3k.ps1 -Action Collect -RunDirectory '+$run}|ConvertTo-Json

if($Action -eq 'Run'){
 if(!$process.WaitForExit(45000)){Write-Output "TEST processId=$($process.Id) still_running=1 collect_later=1";return}
 [pscustomobject]@{exitCode=$process.ExitCode;completedUtc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'process.json')
 & $PSCommandPath -Action Collect -RunDirectory $run -Python $Python
}
