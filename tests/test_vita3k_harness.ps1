$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$collector=Join-Path $repo 'tools/test_vita3k.ps1'
$localRoot=[IO.Path]::GetFullPath((Join-Path $repo '_local'))+[IO.Path]::DirectorySeparatorChar
$testRoot=[IO.Path]::GetFullPath((Join-Path $repo ("_local/vita3k-harness-test-$PID")))
if(!$testRoot.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Test root escaped _local'}
if(Test-Path -LiteralPath $testRoot){throw "Test root already exists: $testRoot"}

$cases=@(
 @{name='access-violation';exitCode=-1073741819;timedOut=$false;expectedReason='0xC0000005';expectedHex='0xC0000005'},
 @{name='timeout';exitCode=-1;timedOut=$true;expectedReason='timed out';expectedHex='0xFFFFFFFF'},
 @{name='other-exit';exitCode=7;timedOut=$false;expectedReason='No new native session log';expectedHex='0x00000007'}
)

try {
 New-Item -ItemType Directory -Path $testRoot -Force|Out-Null
 foreach($case in $cases){
  $run=Join-Path $testRoot ('runs/'+$case.name)
  $filesystem=Join-Path $testRoot ('filesystem/'+$case.name)
  $runtime=Join-Path $testRoot ('runtime/'+$case.name)
  New-Item -ItemType Directory -Path $run,$filesystem,$runtime -Force|Out-Null
  $info=[ordered]@{iteration='076';processId=2147483647;executable='fixture.exe';filesystem=$filesystem;previousLogs=@();expectedWorld='intro';actOneExitSmoke=$false}
  $info|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $run 'run.json')
  [ordered]@{exitCode=$case.exitCode;timedOut=$case.timedOut;stoppedAfterReplay=$false}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'process.json')
  foreach($name in @('stdout.log','stderr.log','vita3k.log')){[IO.File]::WriteAllText((Join-Path $run $name),'')}
  [IO.File]::WriteAllText((Join-Path $runtime 'vita3k.log'),'')
  $rejected=$false
  try { & $collector -Action Collect -RunDirectory $run; }
  catch {$rejected=$_.Exception.Message.Contains($case.expectedReason)}
  if(!$rejected){throw "Collector did not report the expected failure for $($case.name)"}
  $result=Get-Content -LiteralPath (Join-Path $run 'result.json') -Raw|ConvertFrom-Json
  if($result.passed -or $result.process_exit_code_hex -ne $case.expectedHex -or $result.failure_stage -ne 'before_native_session'){
   throw "Collector result was incorrect for $($case.name)"
  }
 }
 'Vita3K harness failure diagnostics: 3/3 passed'
} finally {
 if(Test-Path -LiteralPath $testRoot){
  $resolved=(Resolve-Path -LiteralPath $testRoot).Path
  if(!$resolved.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Refusing cleanup outside _local'}
  Remove-Item -LiteralPath $resolved -Recurse -Force
 }
}
