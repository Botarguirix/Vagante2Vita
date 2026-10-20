param(
 [Parameter(Mandatory=$true)][string]$CampaignDirectory,
 [Parameter(Mandatory=$true)][string]$RouteDirectory,
 [ValidateSet(0,1,2)][int]$PlayerClass=0,
 [string]$Clang='C:\Program Files\LLVM\bin\clang.exe',
 [string]$MsysUcrt='C:\msys64\ucrt64',
 [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(!(Test-Path -LiteralPath $Clang)){throw "Missing clang: $Clang"}
if(!(Test-Path -LiteralPath (Join-Path $MsysUcrt 'include/stdio.h'))){throw "Missing UCRT headers under $MsysUcrt"}
if(!(Test-Path -LiteralPath (Join-Path $MsysUcrt 'lib/libm.a'))){throw "Missing UCRT libm under $MsysUcrt"}
$repoRoot=[IO.Path]::GetFullPath($repo)+[IO.Path]::DirectorySeparatorChar
$localRoot=[IO.Path]::GetFullPath((Join-Path $repo '_local'))+[IO.Path]::DirectorySeparatorChar
$CampaignDirectory=[IO.Path]::GetFullPath($CampaignDirectory)
$RouteDirectory=[IO.Path]::GetFullPath($RouteDirectory)
if(!$CampaignDirectory.StartsWith($repoRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'CampaignDirectory must be inside the repository'}
if(!$RouteDirectory.StartsWith($repoRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'RouteDirectory must be inside the repository'}
if(!$OutputDirectory){$OutputDirectory=Join-Path $repo '_local/host-campaign-chain'}
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
if(!$OutputDirectory.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputDirectory must stay under _local'}
New-Item -ItemType Directory -Force $OutputDirectory|Out-Null
$binary=Join-Path $OutputDirectory 'campaign-chain-test.exe'
$sources=@(
 'tests/act_one_campaign_chain_test.c','vita/src/player.c','vita/src/player_stats.c',
 'vita/src/world.c','vita/src/world_entities.c','vita/src/world_combat.c',
 'vita/src/melee.c','vita/src/loot.c','vita/src/bow.c'
)|ForEach-Object {[IO.Path]::GetFullPath((Join-Path $repo $_))}
$args=@('--target=x86_64-w64-windows-gnu',"--sysroot=$MsysUcrt",'-isystem',(Join-Path $MsysUcrt 'include'),'-B',(Join-Path $MsysUcrt 'bin'),'-L',(Join-Path $MsysUcrt 'lib'),'-fuse-ld=lld','-std=c11','-DHOST_PROBE=1','-DACT_ONE_CHAIN_DIAGNOSTICS=1','-UNDEBUG','-I',(Join-Path $repo 'vita/src'))+$sources+@('-lm','-Wl,--stack,16777216','-o',$binary)
& $Clang @args
if($LASTEXITCODE -ne 0){throw "Campaign-chain test compile failed ($LASTEXITCODE)"}
$runLog=Join-Path $OutputDirectory 'run.log'
$previousErrorAction=$ErrorActionPreference
try{$ErrorActionPreference='Continue';$runOutput=@(& $binary $CampaignDirectory $RouteDirectory $PlayerClass 2>&1);$code=$LASTEXITCODE}
finally{$ErrorActionPreference=$previousErrorAction}
$runOutput|Set-Content -LiteralPath $runLog -Encoding utf8
$runOutput|ForEach-Object { Write-Output $_ }
$runText=$runOutput -join "`n"
$startPattern='CAMPAIGN_CHAIN_START floor=(\d+) class=(\d+) hp=(\d+)/(\d+) items=(\d+) bow=(\d+) arrows=(\d+) carried=(\d+)'
$stateMismatches=[System.Collections.Generic.List[string]]::new()
foreach($match in [regex]::Matches($runText,$startPattern)){
 $floorIndex=[int]$match.Groups[1].Value
 $proofPath=Join-Path $RouteDirectory ('floor-{0:D2}.json' -f $floorIndex)
 if(!(Test-Path -LiteralPath $proofPath)){throw "Missing route proof for chain floor $floorIndex"}
 $proof=Get-Content -LiteralPath $proofPath -Raw|ConvertFrom-Json
 $state=$proof.start_state
 if($proof.player_class -ne [int]$match.Groups[2].Value -or !$state -or
    $state.hp -ne [int]$match.Groups[3].Value -or
    $state.max_hp -ne [int]$match.Groups[4].Value -or
    $state.arrows -ne [int]$match.Groups[7].Value){
  $stateMismatches.Add("floor $($floorIndex): proof class/hp/max_hp/arrows does not match the carried player")
 }
}
$report=[pscustomobject]@{passed=($code -eq 0 -and $stateMismatches.Count -eq 0);exit_code=$code;player_class=$PlayerClass;campaign=$CampaignDirectory;routes=$RouteDirectory;route_start_state_mismatches=@($stateMismatches);binary=$binary;log=$runLog}
$report|ConvertTo-Json -Depth 3|Set-Content -LiteralPath (Join-Path $OutputDirectory 'report.json') -Encoding utf8
if($stateMismatches.Count){throw "Route proof start state does not match the carried player; report: $(Join-Path $OutputDirectory 'report.json')"}
if($code -ne 0){throw "Integrated ACT_ONE campaign chain failed ($code); report: $(Join-Path $OutputDirectory 'report.json')"}
Write-Output (Join-Path $OutputDirectory 'report.json')
