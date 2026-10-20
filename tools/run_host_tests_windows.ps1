param(
 [string]$Clang='C:\Program Files\LLVM\bin\clang.exe',
 [string]$MsysUcrt='C:\msys64\ucrt64',
 [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(!(Test-Path -LiteralPath $Clang)){throw "Missing clang: $Clang"}
if(!(Test-Path -LiteralPath (Join-Path $MsysUcrt 'include/stdio.h'))){throw "Missing UCRT headers under $MsysUcrt"}
if(!(Test-Path -LiteralPath (Join-Path $MsysUcrt 'lib/libm.a'))){throw "Missing UCRT libm under $MsysUcrt"}
if(!$OutputDirectory){$OutputDirectory=Join-Path $repo '_local/host-tests-windows'}
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$localRoot=[IO.Path]::GetFullPath((Join-Path $repo '_local'))+[IO.Path]::DirectorySeparatorChar
if(!$OutputDirectory.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputDirectory must stay inside the repository _local folder'}
New-Item -ItemType Directory -Force $OutputDirectory|Out-Null
$tests=@(
 @{Name='frame-scaler';Sources=@('tests/frame_scaler_test.c')},
 @{Name='frame-upload';Sources=@('tests/frame_upload_test.c')},
 @{Name='input-replay';Sources=@('tests/input_replay_test.c','vita/src/input_replay.c')},
 @{Name='campaign-flow';Sources=@('tests/campaign_flow_test.c','vita/src/campaign_flow.c')},
 @{Name='menu';Sources=@('tests/menu_test.c','vita/src/menu.c')},
 @{Name='session-log';Sources=@('tests/session_log_test.c','vita/src/session_log.c')},
 @{Name='world-entities';Sources=@('tests/world_entities_test.c','vita/src/world_entities.c','vita/src/world_combat.c','vita/src/world.c','vita/src/melee.c','vita/src/loot.c')},
 @{Name='loot';Sources=@('tests/loot_test.c','vita/src/player.c','vita/src/loot.c','vita/src/world.c','vita/src/world_entities.c','vita/src/world_combat.c','vita/src/player_stats.c','vita/src/melee.c','vita/src/bow.c')},
 @{Name='player-stats';Sources=@('tests/player_stats_test.c','vita/src/player_stats.c','vita/src/world.c')},
 @{Name='bow';Sources=@('tests/bow_test.c','vita/src/bow.c','vita/src/world.c','vita/src/world_entities.c','vita/src/world_combat.c','vita/src/melee.c','vita/src/loot.c')},
 @{Name='melee';Sources=@('tests/melee_test.c','vita/src/melee.c')},
 @{Name='player';Sources=@('tests/player_test.c','vita/src/player.c','vita/src/loot.c','vita/src/world.c','vita/src/world_entities.c','vita/src/world_combat.c','vita/src/player_stats.c','vita/src/melee.c','vita/src/bow.c')},
 @{Name='world-extent';Sources=@('tests/world_extent_test.c','vita/src/world.c')},
 @{Name='world';Sources=@('tests/world_test.c','vita/src/world.c')}
)
$prefix=@('--target=x86_64-w64-windows-gnu',"--sysroot=$MsysUcrt",'-isystem',(Join-Path $MsysUcrt 'include'),'-B',(Join-Path $MsysUcrt 'bin'),'-L',(Join-Path $MsysUcrt 'lib'),'-fuse-ld=lld')
$results=@()
Push-Location $repo
try{
 foreach($test in $tests){
  $binary=Join-Path $OutputDirectory ($test.Name+'.exe')
  $sources=@($test.Sources|ForEach-Object {[IO.Path]::GetFullPath((Join-Path $repo $_))})
  $arguments=$prefix+@('-std=c11','-DHOST_PROBE=1','-UNDEBUG','-I',(Join-Path $repo 'vita/src'))+$sources+@('-lm','-Wl,--stack,16777216','-o',$binary)
  & $Clang @arguments
  if($LASTEXITCODE -ne 0){throw "Host test compile failed: $($test.Name) ($LASTEXITCODE)"}
  & $binary
  $code=$LASTEXITCODE
  $results+=,[pscustomobject]@{name=$test.Name;exit_code=$code;passed=($code -eq 0)}
  if($code -ne 0){throw "Host test failed: $($test.Name) ($code)"}
  Write-Output "PASS $($test.Name)"
 }
}finally{Pop-Location}
$report=[pscustomobject]@{compiler=(Resolve-Path $Clang).Path;target='x86_64-w64-windows-gnu';runtime='MSYS2 UCRT';stack_bytes=16777216;passed=(@($results|Where-Object {!$_.passed}).Count -eq 0);tests=$results}
$report|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $OutputDirectory 'report.json') -Encoding utf8
Write-Output (Join-Path $OutputDirectory 'report.json')
