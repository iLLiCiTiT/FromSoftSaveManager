param([int64]$Hwnd, [int]$W, [int]$H)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Z {
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int w, int hh, uint flags);
}
"@
# SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
[Z]::SetWindowPos([IntPtr]$Hwnd, [IntPtr]::Zero, 0, 0, $W, $H, 0x0002 -bor 0x0004 -bor 0x0010) | Out-Null
