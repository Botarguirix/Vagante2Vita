param([Parameter(Mandatory)][string]$Unit)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$build = Join-Path $root '_local\build-recon-pc'
$source = Join-Path $root "tests\recon\${Unit}_test.cpp"
if (!(Test-Path $source)) { throw "No test source: $source" }
$testDir = Join-Path $root "_local\recon-tests\$Unit"
New-Item -ItemType Directory -Force -Path "$testDir\work", "$testDir\tmp" | Out-Null
$env:TMP = "$testDir\tmp"; $env:TEMP = $env:TMP
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
$compiler = 'C:\msys64\ucrt64\bin\c++.exe'
$object = Join-Path $testDir 'test.obj'
$exe = Join-Path $testDir 'test.exe'
$argsCompile = @('-std=c++17', '-O0', '-g', '-DSFML_STATIC', "-I$root\recon\include",
  "-I$root\recon\port", "-I$root\_local\deps\rapidjson-1.1.0\include",
  "-I$root\_local\deps\sfml-pc\include", '-c', $source, '-o', $object)
& $compiler @argsCompile
if ($LASTEXITCODE) { throw 'test compilation failed' }
$objects = Get-ChildItem "$build\CMakeFiles\vagante.dir" -Recurse -Filter '*.obj' |
  Where-Object { $_.FullName -notmatch '[\\/]src[\\/]main\.cpp\.obj$' }
$libraries = (Get-Content "$build\build.ninja" | Where-Object { $_ -match '^  LINK_LIBRARIES = ' } |
  Select-Object -Last 1) -replace '^  LINK_LIBRARIES = ', ''
if (!$libraries -or !$objects) { throw 'build the reconstruction before running tests' }
$response = Join-Path $testDir 'link.rsp'
$lines = @('"' + $object.Replace('\', '/') + '"')
$lines += $objects | ForEach-Object { '"' + $_.FullName.Replace('\', '/') + '"' }
$lines += $libraries
$lines | Set-Content -Encoding utf8NoBOM $response
Push-Location $build
try {
  & $compiler "@$response" '-o' $exe
  if ($LASTEXITCODE) { throw 'test linking failed' }
} finally { Pop-Location }
Push-Location "$testDir\work"
try {
  & $exe
  if ($LASTEXITCODE) { throw "${Unit}: exit code $LASTEXITCODE" }
} finally { Pop-Location }
