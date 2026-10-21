# One-step PC build of the reconstruction: regenerate stubs for what recon/src does not
# define yet, then build _local/build-recon-pc/vagante.exe with MSYS2 UCRT64.
#   pwsh tools/build_recon_pc.ps1            (add -Run to execute it afterwards)
param(
  [string]$GameDir = 'F:\SteamLibrary\steamapps\common\vagante',
  [switch]$Run
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
Set-Location $root
python -I tools/pdb_stubs.py "$GameDir\vagante.pdb" "$GameDir\vagante.exe"
if ($LASTEXITCODE) { throw 'pdb_stubs.py failed' }
$env:MSYSTEM = 'UCRT64'; $env:CHERE_INVOKING = '1'
& 'C:\msys64\usr\bin\bash.exe' -lc 'bash tools/build_recon_pc.sh'
if ($LASTEXITCODE) { throw 'build failed (see _local/build-recon-pc.log)' }
$exe = "$root\_local\build-recon-pc\vagante.exe"
if ($Run) {
  $env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
  Push-Location (Split-Path $exe)
  & $exe
  "exit code: $LASTEXITCODE"
  if (Test-Path recon.log) { 'untranslated functions reached: ' + (Get-Content recon.log).Count }
  Pop-Location
}
