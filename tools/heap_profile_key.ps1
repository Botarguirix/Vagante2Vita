# Scoped replacement for rama A key.ps1 (not present in this checkout).
# No capture: focus the explicitly selected test process and hold a virtual key.
param(
    [Parameter(Mandatory=$true)][int]$GameProcessId,
    [Parameter(Mandatory=$true)][int]$VirtualKey,
    [ValidateRange(30,10000)][int]$HoldMs=350,
    [ValidateRange(0,254)][int]$AdditionalVirtualKey=0
)
$ErrorActionPreference='Stop'
$game=Get-Process -Id $GameProcessId
if($game.ProcessName -notin @('vagante_host','vagante_level2_probe') -or $game.Path -notlike '*Vagante2Vita-ramaC*host-280mib*') {
    throw 'Target is not the isolated 280 MiB diagnostic host.'
}
if($VirtualKey -lt 1 -or $VirtualKey -gt 254) {throw 'Invalid virtual key'}
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class VaganteHeapProfileInput {
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern bool SetWindowText(IntPtr h,string text);
 [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h,int n);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a,uint b,bool attach);
 [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint key,uint type);
 [DllImport("user32.dll")] public static extern void keybd_event(byte key,byte scan,uint flags,UIntPtr extra);
}
"@
$game.Refresh()
$gameWindow=$game.MainWindowHandle
if($gameWindow -eq [IntPtr]::Zero) {throw 'Test process has no main window'}
if($game.ProcessName -eq "vagante_level2_probe") {[void][VaganteHeapProfileInput]::SetWindowText($gameWindow,"Vagante - Codex prueba nivel 2")}
[VaganteHeapProfileInput]::ShowWindow($gameWindow,9) | Out-Null
[VaganteHeapProfileInput]::SetForegroundWindow($gameWindow) | Out-Null
Start-Sleep -Milliseconds 300
if([VaganteHeapProfileInput]::GetForegroundWindow() -ne $gameWindow) {
    $windowActivator=New-Object -ComObject WScript.Shell
    [void]$windowActivator.AppActivate($GameProcessId)
    Start-Sleep -Milliseconds 300
}
if([VaganteHeapProfileInput]::GetForegroundWindow() -ne $gameWindow) {
    $activeWindow=[VaganteHeapProfileInput]::GetForegroundWindow()
    [uint32]$activeProcessId=0
    $activeThread=[VaganteHeapProfileInput]::GetWindowThreadProcessId($activeWindow,[ref]$activeProcessId)
    $inputThread=[VaganteHeapProfileInput]::GetCurrentThreadId()
    $attached=[VaganteHeapProfileInput]::AttachThreadInput($inputThread,$activeThread,$true)
    try { [void][VaganteHeapProfileInput]::SetForegroundWindow($gameWindow) }
    finally {if($attached) {[void][VaganteHeapProfileInput]::AttachThreadInput($inputThread,$activeThread,$false)}}
    Start-Sleep -Milliseconds 300
}
if([VaganteHeapProfileInput]::GetForegroundWindow() -ne $gameWindow) {throw "Test window did not get focus (target=$gameWindow foreground=$([VaganteHeapProfileInput]::GetForegroundWindow())); no key sent"}
$scanCode=[byte][VaganteHeapProfileInput]::MapVirtualKey([uint32]$VirtualKey,0)
$extendedFlag=if($VirtualKey -in @(33,34,35,36,37,38,39,40,45,46)) {[uint32]1} else {[uint32]0}
$additionalScan=if($AdditionalVirtualKey) {[byte][VaganteHeapProfileInput]::MapVirtualKey([uint32]$AdditionalVirtualKey,0)} else {[byte]0}
$additionalFlag=if($AdditionalVirtualKey -in @(33,34,35,36,37,38,39,40,45,46)) {[uint32]1} else {[uint32]0}
try {
    if($AdditionalVirtualKey) {[VaganteHeapProfileInput]::keybd_event([byte]$AdditionalVirtualKey,$additionalScan,$additionalFlag,[UIntPtr]::Zero)}
    [VaganteHeapProfileInput]::keybd_event([byte]$VirtualKey,$scanCode,$extendedFlag,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds $HoldMs
} finally {
    if($AdditionalVirtualKey) {[VaganteHeapProfileInput]::keybd_event([byte]$AdditionalVirtualKey,$additionalScan,($additionalFlag -bor 2),[UIntPtr]::Zero)}
    [VaganteHeapProfileInput]::keybd_event([byte]$VirtualKey,$scanCode,($extendedFlag -bor 2),[UIntPtr]::Zero)
}
"keybd_event PID=$GameProcessId VK=$VirtualKey hold=$HoldMs ms"
