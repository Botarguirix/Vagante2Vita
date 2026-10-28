# Scoped replacement for rama A key.ps1 (not present in this checkout).
# No capture: focus the explicitly selected test process and hold a virtual key.
param(
    [Parameter(Mandatory=$true)][int]$GameProcessId,
    [Parameter(Mandatory=$true)][int]$VirtualKey,
    [ValidateRange(30,10000)][int]$HoldMs=350
)
$ErrorActionPreference='Stop'
$game=Get-Process -Id $GameProcessId
if($game.ProcessName -ne 'vagante_host' -or $game.Path -notlike '*Vagante2Vita-ramaC*host-280mib*') {
    throw 'Target is not the isolated 280 MiB diagnostic host.'
}
if($VirtualKey -lt 1 -or $VirtualKey -gt 254) {throw 'Invalid virtual key'}
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class VaganteHeapProfileInput {
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h,int n);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern void keybd_event(byte key,byte scan,uint flags,UIntPtr extra);
}
"@
$game.Refresh()
$gameWindow=$game.MainWindowHandle
if($gameWindow -eq [IntPtr]::Zero) {throw 'Test process has no main window'}
[VaganteHeapProfileInput]::ShowWindow($gameWindow,9) | Out-Null
[VaganteHeapProfileInput]::SetForegroundWindow($gameWindow) | Out-Null
Start-Sleep -Milliseconds 300
if([VaganteHeapProfileInput]::GetForegroundWindow() -ne $gameWindow) {throw 'Test window did not get focus; no key sent'}
try {
    [VaganteHeapProfileInput]::keybd_event([byte]$VirtualKey,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds $HoldMs
} finally {
    [VaganteHeapProfileInput]::keybd_event([byte]$VirtualKey,0,2,[UIntPtr]::Zero)
}
"keybd_event PID=$GameProcessId VK=$VirtualKey hold=$HoldMs ms"
