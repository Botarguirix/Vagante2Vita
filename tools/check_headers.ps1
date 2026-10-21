# Regenerates the PDB headers and runs the three header checks:
#   layout  - clang, MSVC x86 ABI: sizes/offsets equal to vagante.pdb
#   pc      - clang + MSYS2 UCRT: compiles against real STL, SFML 2.5.1, rapidjson
#   vita    - arm-vita-eabi-g++: same headers for PS Vita
param(
  [string]$Pdb = 'F:\SteamLibrary\steamapps\common\vagante\vagante.pdb',
  [string]$Clang = 'C:\Program Files\LLVM\bin\clang++.exe',
  [string]$MsysUcrt = 'C:\msys64\ucrt64',
  [string]$VitaSdk = 'C:\vitasdk-pm',
  [switch]$NoRegenerate
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
Set-Location $root
$sfml = "$root\_local\deps\SFML-2.5.1\include"
$rapid = "$root\_local\deps\rapidjson-1.1.0\include"
foreach ($p in $sfml, $rapid) { if (!(Test-Path $p)) { throw "Missing dependency: $p (see docs/RAMA_A_INVENTARIO.md)" } }
if (!$NoRegenerate) {
  python -I tools/pdb_inventory.py $Pdb | Out-Null
  python -I tools/pdb_headers.py $Pdb | Out-Null
  python -I tools/patch_sfml_vita.py "$root\_local\deps\SFML-2.5.1" | Out-Null
}
$includes = @("-I$root\recon\include", "-I$root\recon\port", "-I$sfml", "-I$rapid", '-DSFML_STATIC')
$results = [ordered]@{}
$out = & $Clang --target=i686-pc-windows-msvc -std=c++17 -fms-extensions -fsyntax-only -ferror-limit=0 -w "-I$root\recon\include" "$root\recon\layout_check.cpp" 2>&1
$results.layout = @($out | Select-String ' error: ').Count
$out = & $Clang --target=x86_64-w64-windows-gnu "--sysroot=$MsysUcrt" -std=c++17 -fsyntax-only -ferror-limit=0 -w @includes "$root\recon\port_check.cpp" 2>&1
$results.pc = @($out | Select-String ' error: ').Count
$env:VITASDK = $VitaSdk
$out = & "$VitaSdk\bin\arm-vita-eabi-g++.exe" -std=gnu++17 -fsyntax-only -w @includes "$root\recon\port_check.cpp" 2>&1
$results.vita = @($out | Select-String 'error').Count
$results.GetEnumerator() | ForEach-Object { '{0,-7} errors: {1}' -f $_.Key, $_.Value }
if (($results.Values | Measure-Object -Sum).Sum -ne 0) { exit 1 }
