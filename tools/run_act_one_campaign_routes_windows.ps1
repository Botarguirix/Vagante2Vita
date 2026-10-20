param(
 [Parameter(Mandatory=$true)][string]$CampaignDirectory,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [string]$Clang='C:\Program Files\LLVM\bin\clang.exe',
 [string]$MsysUcrt='C:\msys64\ucrt64',
 [string]$Python='python',
 [ValidateSet(0,1,2)][int]$BossClass=0,
 [ValidateRange(-1,2)][int]$FloorIndex=-1,
 [ValidateRange(0,300)][int]$StartHp=0,
 [ValidateRange(-1,999)][int]$StartArrows=-1
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(!(Test-Path -LiteralPath $Clang)){throw "Missing clang: $Clang"}
if(!(Test-Path -LiteralPath (Join-Path $MsysUcrt 'include/stdio.h'))){throw "Missing UCRT headers under $MsysUcrt"}
if(!(Test-Path -LiteralPath (Join-Path $MsysUcrt 'lib/libm.a'))){throw "Missing UCRT libm under $MsysUcrt"}
$CampaignDirectory=[IO.Path]::GetFullPath($CampaignDirectory)
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$repoRoot=[IO.Path]::GetFullPath($repo)+[IO.Path]::DirectorySeparatorChar
$localRoot=[IO.Path]::GetFullPath((Join-Path $repo '_local'))+[IO.Path]::DirectorySeparatorChar
if(!$CampaignDirectory.StartsWith($repoRoot,[StringComparison]::OrdinalIgnoreCase)){
 throw "CampaignDirectory must stay inside the repository: $CampaignDirectory"
}
if(!$OutputDirectory.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){
 throw "OutputDirectory must stay inside repository _local: $OutputDirectory"
}
if(!(Test-Path -LiteralPath (Join-Path $CampaignDirectory 'act-one-world.vwi')) -or
   !(Test-Path -LiteralPath (Join-Path $CampaignDirectory 'floors/floor-02.vwi'))){
 throw 'Campaign directory must contain ACT_ONE floor 0, 1 and 2 packets'
}
$pythonCommand=Get-Command $Python -ErrorAction SilentlyContinue
if(!$pythonCommand){throw "Python executable not found: $Python"}
$Python=$pythonCommand.Source
if(Test-Path -LiteralPath $OutputDirectory){
 if((Get-ChildItem -LiteralPath $OutputDirectory -Force | Measure-Object).Count -gt 0){
  throw "OutputDirectory must be empty for a reproducible route run: $OutputDirectory"
 }
}else{New-Item -ItemType Directory -Force $OutputDirectory|Out-Null}
$executable=Join-Path $OutputDirectory 'act-one-route-probe.exe'
$sources=@(
 'vita/src/act_one_route_probe.c','vita/src/world.c','vita/src/world_entities.c',
 'vita/src/world_combat.c','vita/src/loot.c','vita/src/player.c',
 'vita/src/player_stats.c','vita/src/melee.c','vita/src/bow.c','vita/src/input_replay.c'
)|ForEach-Object {[IO.Path]::GetFullPath((Join-Path $repo $_))}
$prefix=@('--target=x86_64-w64-windows-gnu',"--sysroot=$MsysUcrt",'-isystem',(Join-Path $MsysUcrt 'include'),
 '-B',(Join-Path $MsysUcrt 'bin'),'-L',(Join-Path $MsysUcrt 'lib'),'-fuse-ld=lld')
$arguments=$prefix+@('-std=c11','-O2','-Wall','-Wextra','-Werror','-DHOST_PROBE=1',
 '-I',(Join-Path $repo 'vita/src'))+$sources+@('-lm','-Wl,--stack,16777216','-o',$executable)
& $Clang @arguments
if($LASTEXITCODE -ne 0){throw "Route probe compilation failed ($LASTEXITCODE)"}
$floors=@(
 @{Index=0;World='act-one-world.vwi';Entities='act-one-entities.vwe';Layout='act-one-layout.json'},
 @{Index=1;World='floors/floor-01.vwi';Entities='floors/floor-01-entities.vwe';Layout='floors/floor-01-layout.json'},
 @{Index=2;World='floors/floor-02.vwi';Entities='floors/floor-02-entities.vwe';Layout='floors/floor-02-layout.json'}
)
$hashEntries=[System.Collections.Generic.List[object]]::new()
$AddHashFile={param([string]$Path,[string]$Name)
 $hash=Get-FileHash -LiteralPath $Path -Algorithm SHA256
 $hashEntries.Add([pscustomobject]@{name=$Name;bytes=(Get-Item -LiteralPath $Path).Length;sha256=$hash.Hash.ToLowerInvariant()})
}
$sourceFiles=[System.Collections.Generic.List[string]]::new()
$AddSource={param([string]$Path,[string]$Name)
 $relativeName=$Name.Replace('\','/')
 $sourceFiles.Add($relativeName)
}
$sourceNames=@(
 'vita/src/act_one_route_probe.c','vita/src/world.c','vita/src/world_entities.c',
 'vita/src/world_combat.c','vita/src/loot.c','vita/src/player.c','vita/src/player_stats.c','vita/src/campaign_flow.c',
 'vita/src/melee.c','vita/src/bow.c','vita/src/input_replay.c'
)
foreach($source in $sourceNames){$path=Join-Path $repo $source;& $AddSource $path $source}
foreach($header in Get-ChildItem -LiteralPath (Join-Path $repo 'vita/src') -Filter '*.h' -File){$name='vita/src/'+$header.Name;& $AddSource $header.FullName $name}
foreach($name in @('tools/run_act_one_route_probe_wsl.sh','tools/run_act_one_campaign_routes_windows.ps1','tools/route_probe_fingerprint.py','tools/route_trace.py','tools/record_act_one_route.py','tools/prepare_act_one_campaign.py')){$path=Join-Path $repo $name;& $AddSource $path $name}
$simulatorFiles=$sourceFiles.ToArray()
[Array]::Sort($simulatorFiles,[StringComparer]::Ordinal)
$sourceDigest=[Security.Cryptography.IncrementalHash]::CreateHash([Security.Cryptography.HashAlgorithmName]::SHA256)
foreach($name in $simulatorFiles){
 $path=Join-Path $repo $name
 $sourceDigest.AppendData([Text.Encoding]::UTF8.GetBytes($name));$sourceDigest.AppendData([byte[]](0));$sourceDigest.AppendData([IO.File]::ReadAllBytes($path));$sourceDigest.AppendData([byte[]](0))
 & $AddHashFile $path $name
}
$simulatorHash=([BitConverter]::ToString($sourceDigest.GetHashAndReset()) -replace '-','').ToLowerInvariant()
$sourceDigest.Dispose()
$proofMetadataPath=Join-Path $OutputDirectory 'proof-simulator.json'
@{sha256=$simulatorHash;files=$simulatorFiles}|ConvertTo-Json -Depth 3|Set-Content -LiteralPath $proofMetadataPath -Encoding utf8
$null=Get-Content -LiteralPath $proofMetadataPath -Raw
$results=@()
foreach($floor in $floors){
 if($FloorIndex -ge 0 -and $floor.Index -ne $FloorIndex){continue}
 $hasSupportedBoss=$false
 $world=Join-Path $CampaignDirectory $floor.World
 $entities=Join-Path $CampaignDirectory $floor.Entities
 $layout=Get-Content -LiteralPath (Join-Path $CampaignDirectory $floor.Layout) -Raw|ConvertFrom-Json
 $seed=[uint32]$layout.seed
 $prefixName=('floor-{0:D2}' -f $floor.Index)
 $trace=Join-Path $OutputDirectory ($prefixName+'.vtr')
 $log=Join-Path $OutputDirectory ($prefixName+'.log')
 $proof=Join-Path $OutputDirectory ($prefixName+'.json')
 if(Test-Path -LiteralPath $trace){throw "Trace already exists: $trace"}
 $probeArgs=@($world,$entities,[string]$seed)
 if($floor.Index -in @(0,1,2)){
  $bossPacket=Join-Path $CampaignDirectory ('floors/floor-{0:D2}-enemies.vwe' -f $floor.Index)
  if($floor.Index -eq 0 -and !(Test-Path -LiteralPath $bossPacket)){
   $bossPacket=Join-Path $CampaignDirectory 'act-one-enemies.vwe'
  }
  if(!(Test-Path -LiteralPath $bossPacket)){
   throw "Missing provisional boss packet for ACT_ONE floor $($floor.Index): $bossPacket"
  }
  $bossBytes=[IO.File]::ReadAllBytes($bossPacket)
  if($bossBytes.Length -lt 8 -or [Text.Encoding]::ASCII.GetString($bossBytes,0,4) -ne 'VWE3'){
   throw "Invalid enemy packet for ACT_ONE floor $($floor.Index): $bossPacket"
  }
  $enemyCount=[BitConverter]::ToUInt16($bossBytes,4);$hasSupportedBoss=$false
  $requiredSubtype=@{0=50;1=51;2=49}[$floor.Index]
  for($enemyIndex=0;$enemyIndex -lt $enemyCount;$enemyIndex++){
   $recordOffset=8+$enemyIndex*16
   if($recordOffset+16 -gt $bossBytes.Length){throw "Truncated enemy packet: $bossPacket"}
   if($bossBytes[$recordOffset] -eq 26 -and $bossBytes[$recordOffset+1] -eq $requiredSubtype){$hasSupportedBoss=$true;break}
  }
  if(!$hasSupportedBoss){throw "ACT_ONE floor $($floor.Index) must include its provisional boss subtype $requiredSubtype"}
  if($hasSupportedBoss){
   $probeArgs+=@('--boss',$bossPacket,
    (Join-Path $CampaignDirectory 'intro-melee.vwa'),(Join-Path $CampaignDirectory 'item-loot.vlt'),
    (Join-Path $CampaignDirectory 'act-one-goblin-attack.vga'))
   if($BossClass -ge 0){$probeArgs+=@('--boss-class',[string]$BossClass)}
   if($StartHp -gt 0){$probeArgs+=@('--start-hp',[string]$StartHp)}
   if($StartArrows -ge 0){$probeArgs+=@('--start-arrows',[string]$StartArrows)}
  }
 }
 $probeArgs+=@('--trace-out',$trace)
 $priorErrorAction=$ErrorActionPreference
 $ErrorActionPreference='Continue'
 try{$probeOutput=@(& $executable @probeArgs 2>&1);$probeExit=$LASTEXITCODE}
 finally{$ErrorActionPreference=$priorErrorAction}
 $probeText=($probeOutput|ForEach-Object {$_.ToString()}) -join "`n"
 [IO.File]::WriteAllText($log,$probeText+"`n",[Text.UTF8Encoding]::new($false))
 Write-Output $probeText
 if($probeExit -ne 0){
  $results+=,[pscustomobject]@{floor=$floor.Index;seed=$seed;passed=$false;exit_code=$probeExit;log=$log}
  continue
 }
 $proofArgs=@('--log',$log,'--world',$world,'--entities',$entities,'--enemies',$bossPacket,'--output',$proof,'--seed',[string]$seed,'--trace',$trace,'--source-root',$repo,'--simulator-files')+$simulatorFiles+@('--simulator-sha256',$simulatorHash)
 if($floor.Index -eq 2 -or $hasSupportedBoss){$proofArgs+='--require-boss-defeat'}
 & $Python (Join-Path $PSScriptRoot 'record_act_one_route.py') @proofArgs
 if($LASTEXITCODE -ne 0){
  $results+=,[pscustomobject]@{floor=$floor.Index;seed=$seed;passed=$false;exit_code=$LASTEXITCODE;log=$log}
  continue
 }
 $results+=,[pscustomobject]@{floor=$floor.Index;seed=$seed;passed=$true;proof=$proof;trace=$trace;log=$log}
}
$allPassed=@($results|Where-Object {!$_.passed}).Count -eq 0
$report=[pscustomobject]@{format='act-one-campaign-physical-route-results';version=1;probe=$executable;campaign=$CampaignDirectory;passed=$allPassed;floors=$results}
$reportPath=Join-Path $OutputDirectory 'route-results.json'
$report|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $reportPath -Encoding utf8
Write-Output $reportPath
if(!$allPassed){throw 'One or more ACT_ONE floors failed the physical route probe; see route-results.json'}
