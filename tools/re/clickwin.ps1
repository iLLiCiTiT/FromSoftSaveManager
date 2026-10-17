param([int64]$Hwnd, [int]$X, [int]$Y)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class C {
  [DllImport("user32.dll")] public static extern IntPtr PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
"@
$l = [IntPtr](($Y -shl 16) -bor ($X -band 0xffff))
[C]::PostMessage([IntPtr]$Hwnd, 0x0200, [IntPtr]0, $l) | Out-Null   # WM_MOUSEMOVE
Start-Sleep -Milliseconds 50
[C]::PostMessage([IntPtr]$Hwnd, 0x0201, [IntPtr]1, $l) | Out-Null   # WM_LBUTTONDOWN
Start-Sleep -Milliseconds 50
[C]::PostMessage([IntPtr]$Hwnd, 0x0202, [IntPtr]0, $l) | Out-Null   # WM_LBUTTONUP
