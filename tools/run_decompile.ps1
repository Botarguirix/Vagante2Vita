param(
    [string]$GameDirectory = 'C:\Program Files (x86)\Steam\steamapps\common\vagante',
    [int]$MaxFunctions = 0,
    [int]$AnalysisTimeout = 1800
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$toolRoot = Join-Path $root '_local/toolchain'
$ghidra = Get-ChildItem $toolRoot -Directory -Filter 'ghidra_*' | Select-Object -First 1
$jdk = Get-ChildItem $toolRoot -Directory -Filter 'jdk-*' | Select-Object -First 1
if (!$ghidra -or !$jdk) { throw 'Run python tools/setup_re.py first.' }
$env:JAVA_HOME = $jdk.FullName
$env:PATH = (Join-Path $jdk.FullName 'bin') + ';' + $env:PATH
$projects = Join-Path $root '_local/ghidra'
$output = Join-Path $root '_local/decompiled'
$inputDirectory = Join-Path $root '_local/input'
New-Item -ItemType Directory -Force $projects,$output,$inputDirectory | Out-Null
Copy-Item -LiteralPath (Join-Path $GameDirectory 'vagante.exe') -Destination $inputDirectory
Copy-Item -LiteralPath (Join-Path $GameDirectory 'vagante.pdb') -Destination $inputDirectory
$launcher = Join-Path $ghidra.FullName 'support/analyzeHeadless.bat'
& $launcher $projects Vagante -import (Join-Path $inputDirectory 'vagante.exe') -overwrite -analysisTimeoutPerFile $AnalysisTimeout -max-cpu 4 -scriptPath (Join-Path $PSScriptRoot 'ghidra') -postScript ExportDecomp.java $output $MaxFunctions -log (Join-Path $projects 'analysis.log') -scriptlog (Join-Path $projects 'scripts.log')
if ($LASTEXITCODE -ne 0) { throw "Ghidra failed: $LASTEXITCODE" }
