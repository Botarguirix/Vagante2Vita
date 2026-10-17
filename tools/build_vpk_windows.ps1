param(
 [ValidatePattern('^[0-9]{3}$')][string]$Iteration='032',
 [string]$VitaSdk='C:\vitasdk',
 [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(!$OutputDirectory){$OutputDirectory=Join-Path $repo ("_local/build-act-one-$Iteration")}
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$localRoot=[IO.Path]::GetFullPath((Join-Path $repo '_local'))+[IO.Path]::DirectorySeparatorChar
if(!$OutputDirectory.StartsWith($localRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'OutputDirectory must stay inside the repository _local folder'}
New-Item -ItemType Directory -Force (Join-Path $OutputDirectory 'obj')|Out-Null
$version='00.'+$Iteration.Substring(1,2)
$bin=Join-Path $VitaSdk 'bin'
$cc=Join-Path $bin 'arm-vita-eabi-gcc.exe'
$ar=Join-Path $bin 'arm-vita-eabi-ar.exe'
$ranlib=Join-Path $bin 'arm-vita-eabi-ranlib.exe'
foreach($tool in @($cc,$ar,$ranlib,'vita-elf-create.exe','vita-make-fself.exe','vita-mksfoex.exe','vita-pack-vpk.exe')){
 $path=if([IO.Path]::IsPathRooted($tool)){$tool}else{Join-Path $bin $tool}
 if(!(Test-Path -LiteralPath $path)){throw "Missing VitaSDK tool: $path"}
}
function Invoke-Checked([string]$Executable,[string[]]$Arguments){
 & $Executable @Arguments
 if($LASTEXITCODE -ne 0){throw "Command failed ($LASTEXITCODE): $Executable $($Arguments -join ' ')"}
}
$includes=@('-I',(Join-Path $repo 'vita/src'),'-I',(Join-Path $repo 'third_party/stb'),'-I',(Join-Path $repo 'third_party/miniz'))
$base=@('-O3','-DNDEBUG','-ffp-contract=off')+$includes
$resourceNames=@('resources','scene','menu','world','player','loot','world_visuals','world_entities','world_combat','player_stats','native_ui','melee','bow','input_replay')
$resourceObjects=@()
foreach($name in $resourceNames){
 $object=Join-Path $OutputDirectory "obj/$name.o"
 Invoke-Checked $cc ($base+@('-c',(Join-Path $repo "vita/src/$name.c"),'-o',$object))
 $resourceObjects+=,$object
}
$main=Join-Path $OutputDirectory 'obj/main.o'
$macroHeader=Join-Path $OutputDirectory 'obj/iteration_config.h'
$macroText="#define VAGANTE_ITERATION `"$Iteration`"`n#define VAGANTE_VERSION `"$version`"`n"
[IO.File]::WriteAllText($macroHeader,$macroText,[Text.UTF8Encoding]::new($false))
$strict=@('-Wall','-Wextra','-Werror','-include',$macroHeader)
& $cc @base @strict -c (Join-Path $repo 'vita/src/main.c') -o $main
if($LASTEXITCODE -ne 0){throw "Main compilation failed ($LASTEXITCODE)"}
$session=Join-Path $OutputDirectory 'obj/session_log.o'
Invoke-Checked $cc ($base+@('-c',(Join-Path $repo 'vita/src/session_log.c'),'-o',$session))
$minizObjects=@()
foreach($name in @('miniz','miniz_tinfl')){
 $object=Join-Path $OutputDirectory "obj/$name.o"
 Invoke-Checked $cc ($base+@('-DMINIZ_NO_ARCHIVE_APIS','-DMINIZ_NO_DEFLATE_APIS','-c',(Join-Path $repo "third_party/miniz/$name.c"),'-o',$object))
 $minizObjects+=,$object
}
Push-Location $OutputDirectory
try{
 Remove-Item -LiteralPath (Join-Path $OutputDirectory 'libresources.a'),(Join-Path $OutputDirectory 'libminiz.a') -Force -ErrorAction SilentlyContinue
 Invoke-Checked $ar (@('qc','libresources.a')+($resourceObjects|ForEach-Object {Join-Path 'obj' ([IO.Path]::GetFileName($_))}))
 Invoke-Checked $ranlib @('libresources.a')
 Invoke-Checked $ar @('qc','libminiz.a','obj/miniz.o','obj/miniz_tinfl.o')
 Invoke-Checked $ranlib @('libminiz.a')
 Invoke-Checked $cc @('-Wl,-q','-O3','-DNDEBUG','-Wl,-z,nocopyreloc','obj/main.o','obj/session_log.o','-o','vagante2vita','libresources.a','-lSceDisplay_stub','-lSceCtrl_stub','-lSceSysmem_stub','-lSceLibKernel_stub','libminiz.a','-lm')
 Invoke-Checked (Join-Path $bin 'vita-elf-create.exe') @('vagante2vita','vagante2vita.velf')
 Invoke-Checked (Join-Path $bin 'vita-make-fself.exe') @('-c','vagante2vita.velf','eboot.bin')
 Invoke-Checked (Join-Path $bin 'vita-mksfoex.exe') @('-s',"APP_VER=$version",'-s','TITLE_ID=VAGV00006','Vagante2Vita','Vagante2Vita_param.sfo')
 $vpk="Vagante2Vita-iteration$Iteration.vpk"
 $pack=@('-s','Vagante2Vita_param.sfo','-b','eboot.bin')
 $assets=@(@('icon0.png','sce_sys/icon0.png'),@('pic0.png','sce_sys/pic0.png'),@('livearea/contents/bg0.png','sce_sys/livearea/contents/bg0.png'),@('livearea/contents/startup.png','sce_sys/livearea/contents/startup.png'),@('livearea/contents/template.xml','sce_sys/livearea/contents/template.xml'))
 foreach($item in $assets){$pack+=@('-a',((Join-Path $repo ('vita/sce_sys/'+$item[0]))+'='+$item[1]))}
 $pack+=$vpk
 Invoke-Checked (Join-Path $bin 'vita-pack-vpk.exe') $pack
}finally{Pop-Location}
$result=Join-Path $OutputDirectory ("Vagante2Vita-iteration$Iteration.vpk")
Write-Output $result
