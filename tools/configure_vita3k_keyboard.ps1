param([string]$EmulatorDirectory = (Join-Path $env:USERPROFILE 'Downloads/Vita/windows-latest'))
$ErrorActionPreference = 'Stop'
$configPath = Join-Path $EmulatorDirectory 'config.yml'
$source = [IO.File]::ReadAllText($configPath)
$bindings = [ordered]@{
 'keyboard-button-cross'='Space'; 'keyboard-button-square'='KeyZ'
 'keyboard-button-circle'='KeyX'; 'keyboard-button-triangle'='KeyC'
 'keyboard-button-select'='Backspace'; 'keyboard-button-start'='Enter'
 'keyboard-button-up'='ArrowUp'; 'keyboard-button-down'='ArrowDown'
 'keyboard-button-left'='ArrowLeft'; 'keyboard-button-right'='ArrowRight'
 'keyboard-button-l1'='KeyQ'; 'keyboard-button-r1'='KeyE'
 'keyboard-leftstick-left'='KeyA'; 'keyboard-leftstick-right'='KeyD'
 'keyboard-leftstick-up'='KeyW'; 'keyboard-leftstick-down'='KeyS'
 'keyboard-rightstick-left'='KeyJ'; 'keyboard-rightstick-right'='KeyL'
 'keyboard-rightstick-up'='KeyI'; 'keyboard-rightstick-down'='KeyK'
}
foreach($key in $bindings.Keys){
 $pattern = '(?m)^'+[regex]::Escape($key)+':[^\r\n]*'
 if([regex]::Matches($source,$pattern).Count -ne 1){throw "Missing or duplicate binding: $key"}
 $source = [regex]::Replace($source,$pattern,$key+': '+$bindings[$key])
}
$backupPath = $configPath + '.keyboard-backup-' + (Get-Date -Format 'yyyyMMdd-HHmmssfff')
Copy-Item -LiteralPath $configPath -Destination $backupPath
[IO.File]::WriteAllText($configPath,$source,[Text.UTF8Encoding]::new($false))
[pscustomobject]@{Configuration=$configPath;Backup=$backupPath;Bindings=$bindings}|ConvertTo-Json -Depth 4
