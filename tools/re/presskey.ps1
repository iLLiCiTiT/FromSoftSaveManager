param([int64]$Hwnd, [int]$Scan = 0x2D)  # 0x2D = X scan code
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class K {
  [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
  [StructLayout(LayoutKind.Explicit, Size = 40)] public struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; }
  [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  public static void Press(ushort scan) {
    INPUT[] i = new INPUT[1];
    i[0].type = 1; i[0].ki.wScan = scan; i[0].ki.dwFlags = 0x0008;  // KEYEVENTF_SCANCODE
    SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
    System.Threading.Thread.Sleep(80);
    i[0].ki.dwFlags = 0x0008 | 0x0002;  // KEYUP
    SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
  }
}
"@
[K]::SetForegroundWindow([IntPtr]$Hwnd) | Out-Null
Start-Sleep -Milliseconds 300
[K]::Press([uint16]$Scan)
