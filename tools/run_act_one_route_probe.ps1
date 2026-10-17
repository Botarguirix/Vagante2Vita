param(
 [Parameter(Mandatory=$true)][string]$World,
 [Parameter(Mandatory=$true)][string]$Entities,
 [uint32]$Seed=12345,
 [string]$OutputDirectory,
 [string]$Compiler,
 [string]$TraceOutput
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(!$OutputDirectory){$OutputDirectory=Join-Path $repo '_local/host-route'}
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$localRoot=[IO.Path]::GetFullPath((Join-Path $repo '_local'))+[IO.Path]::DirectorySeparatorChar
if(!$OutputDirectory.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputDirectory must stay inside the repository _local folder'}
if(!$Compiler){
 foreach($candidate in @('clang','gcc','cc')){
  $command=Get-Command $candidate -ErrorAction SilentlyContinue
  if($command){$Compiler=$command.Source;break}
 }
}
if(!$Compiler -or !(Test-Path -LiteralPath $Compiler)){
 throw 'No host C compiler found. Install a Windows clang or GCC toolchain, then pass -Compiler with its executable path.'
}
$worldPath=(Resolve-Path -LiteralPath $World).Path
$entitiesPath=(Resolve-Path -LiteralPath $Entities).Path
New-Item -ItemType Directory -Path $OutputDirectory -Force|Out-Null
$exe=Join-Path $OutputDirectory 'act-one-route-probe.exe'
$sources=@(
 'vita/src/act_one_route_probe.c','vita/src/world.c','vita/src/world_entities.c',
 'vita/src/world_combat.c','vita/src/loot.c',
 'vita/src/player.c','vita/src/player_stats.c','vita/src/melee.c',
 'vita/src/bow.c','vita/src/input_replay.c'
)|ForEach-Object {Join-Path $repo $_}
$args=@('-std=c11','-O2','-Wall','-Wextra','-Werror','-I',(Join-Path $repo 'vita/src'))+$sources+@('-lm','-o',$exe)
& $Compiler @args
if($LASTEXITCODE -ne 0){throw "Host route probe compilation failed ($LASTEXITCODE)"}
$probeArgs=@($worldPath,$entitiesPath,[string]$Seed)
if($TraceOutput){
 $TraceOutput=[IO.Path]::GetFullPath($TraceOutput)
 if(!$TraceOutput.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'TraceOutput must stay inside the repository _local folder'}
 if(Test-Path -LiteralPath $TraceOutput){throw "TraceOutput already exists: $TraceOutput"}
 New-Item -ItemType Directory -Path (Split-Path -Parent $TraceOutput) -Force|Out-Null
 $probeArgs+=@('--trace-out',$TraceOutput)
}
& $exe @probeArgs
if($LASTEXITCODE -ne 0){throw "ACT_ONE physical route failed (exit $LASTEXITCODE)"}
