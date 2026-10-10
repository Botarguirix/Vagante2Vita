param([int]$TimeoutSeconds=120)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$ghidra=Get-ChildItem (Join-Path $root '_local/toolchain') -Directory -Filter 'ghidra_*' | Select-Object -First 1
$jdk=Get-ChildItem (Join-Path $root '_local/toolchain') -Directory -Filter 'jdk-*' | Select-Object -First 1
$env:JAVA_HOME=$jdk.FullName
$env:PATH=(Join-Path $jdk.FullName 'bin')+';'+$env:PATH
& (Join-Path $ghidra.FullName 'support/analyzeHeadless.bat') (Join-Path $root '_local/ghidra') Vagante -process vagante.exe -noanalysis -scriptPath (Join-Path $PSScriptRoot 'ghidra') -postScript RetryDecomp.java (Join-Path $root '_local/decompiled') $TimeoutSeconds -log (Join-Path $root '_local/ghidra/retry.log')
if($LASTEXITCODE -ne 0){throw "Ghidra retry failed: $LASTEXITCODE"}
