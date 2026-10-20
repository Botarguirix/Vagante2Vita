param(
    [string]$Source = (Join-Path $PSScriptRoot '..\_local\vita3k-act-one-136-run-final2\instance'),
    [string]$Destination = (Join-Path $PSScriptRoot '..\_local\vita3k-act-one-fullcopy-143\instance'),
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$sourcePath = [IO.Path]::GetFullPath($Source)
$destinationPath = [IO.Path]::GetFullPath($Destination)
if (!(Test-Path -LiteralPath (Join-Path $sourcePath 'runtime\Vita3K.exe'))) {
    throw "Source is not a complete Vita3K instance: $sourcePath"
}
if ($sourcePath -eq $destinationPath -or $destinationPath.StartsWith($sourcePath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Destination must be separate from and outside the source instance.'
}
if (!$VerifyOnly) {
    if (Test-Path -LiteralPath $destinationPath) { throw "Destination already exists: $destinationPath" }
    New-Item -ItemType Directory -Path (Split-Path -Parent $destinationPath) -Force | Out-Null
    & robocopy $sourcePath $destinationPath /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /NFL /NDL /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "Robocopy failed with exit code $LASTEXITCODE" }
    $configPath = Join-Path $destinationPath 'runtime\config.yml'
    $config = [IO.File]::ReadAllText($configPath)
    $profilePath = (Join-Path $destinationPath 'filesystem').Replace('\', '/')
    $replacement = 'pref-path: ' + [char]39 + $profilePath + '/' + [char]39
    $config = [regex]::Replace($config, '(?m)^pref-path:.*$', [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $replacement })
    [IO.File]::WriteAllText($configPath, $config, [Text.UTF8Encoding]::new($false))
}
$sourceExe = Get-FileHash -LiteralPath (Join-Path $sourcePath 'runtime\Vita3K.exe') -Algorithm SHA256
$cloneExe = Get-FileHash -LiteralPath (Join-Path $destinationPath 'runtime\Vita3K.exe') -Algorithm SHA256
$sourceGame = Get-FileHash -LiteralPath (Join-Path $sourcePath 'filesystem\ux0\app\VAGV00006\eboot.bin') -Algorithm SHA256
$cloneGame = Get-FileHash -LiteralPath (Join-Path $destinationPath 'filesystem\ux0\app\VAGV00006\eboot.bin') -Algorithm SHA256
if ($sourceExe.Hash -ne $cloneExe.Hash -or $sourceGame.Hash -ne $cloneGame.Hash) { throw 'Hash verification failed for Vita3K or Vagante executable.' }
$sourceFiles = @(Get-ChildItem -LiteralPath $sourcePath -File -Recurse)
$cloneFiles = @(Get-ChildItem -LiteralPath $destinationPath -File -Recurse)
$cloneRelative = @{}
foreach ($file in $cloneFiles) {
    $relative = $file.FullName.Substring($destinationPath.Length).TrimStart('\', '/')
    $cloneRelative[$relative] = $file
}
$mismatches = [Collections.Generic.List[string]]::new()
foreach ($file in $sourceFiles) {
    $relative = $file.FullName.Substring($sourcePath.Length).TrimStart('\', '/')
    if (!$cloneRelative.ContainsKey($relative)) { $mismatches.Add("missing:$relative"); continue }
    $copy = $cloneRelative[$relative]
    if ($relative -eq 'runtime\config.yml') { continue }
    if ($file.Length -ne $copy.Length) { $mismatches.Add("size:$relative"); continue }
    if ((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $copy.FullName -Algorithm SHA256).Hash) { $mismatches.Add("hash:$relative") }
}
if ($mismatches.Count) { throw "Instance copy verification failed: $($mismatches -join ', ')" }
$report = [ordered]@{
    source = $sourcePath; destination = $destinationPath
    executable_sha256 = $cloneExe.Hash; vagante_eboot_sha256 = $cloneGame.Hash
    verified_source_file_count = $sourceFiles.Count
    extra_clone_file_count = $cloneFiles.Count - $sourceFiles.Count
    verified_by_sha256_except_isolated_config = $true
    bytes = (Get-ChildItem -LiteralPath $destinationPath -File -Recurse | Measure-Object Length -Sum).Sum
    isolated_pref_path = (Select-String -LiteralPath (Join-Path $destinationPath 'runtime\config.yml') -Pattern '^pref-path:').Line
}
$report | ConvertTo-Json | Write-Output
