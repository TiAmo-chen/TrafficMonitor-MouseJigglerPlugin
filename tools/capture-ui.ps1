# 打开插件选项对话框并截图（用于 README）。脚本保持纯 ASCII：PowerShell 5.1 按 ANSI 读 .ps1。
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public class WinCap {
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("shcore.dll")] public static extern int SetProcessDpiAwareness(int value);
  [DllImport("user32.dll")] public static extern uint GetDpiForSystem();
  [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr hwnd);
  public struct RECT { public int Left, Top, Right, Bottom; }
  public static IntPtr FindDialog(uint targetPid) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, p) => {
      uint pid; GetWindowThreadProcessId(h, out pid);
      if (pid == targetPid) {
        var cls = new StringBuilder(64); GetClassNameW(h, cls, 64);
        if (cls.ToString() == "#32770") { found = h; return false; }
      }
      return true;
    }, IntPtr.Zero);
    return found;
  }
}
'@

# 必须开启 DPI 感知，否则 GetWindowRect 拿到的是缩放后的虚拟坐标，截图会缺一块
try { [WinCap]::SetProcessDpiAwareness(2) | Out-Null } catch {}

$exe = Join-Path $PSScriptRoot '..\bin\uipreview.exe'
$out = Join-Path $PSScriptRoot '..\docs\options-dialog.png'
New-Item -ItemType Directory -Force -Path (Split-Path $out) | Out-Null

$p = Start-Process -FilePath $exe -WorkingDirectory (Join-Path $PSScriptRoot '..') -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 60; $i++) {
    $hwnd = [WinCap]::FindDialog([uint32]$p.Id)
    if ($hwnd -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 200
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Host 'dialog not found'; exit 1 }

Start-Sleep -Milliseconds 1200   # let the status line tick a few times
$r = New-Object WinCap+RECT
[WinCap]::GetWindowRect($hwnd, [ref]$r) | Out-Null

# DPI unaware host: GetWindowRect gives virtualized coordinates, scale back to physical pixels
$scale = [double][WinCap]::GetDpiForWindow($hwnd) / 96.0
if ($scale -le 0) { $scale = [double][WinCap]::GetDpiForSystem() / 96.0 }
if ($scale -le 0) { $scale = 1.0 }
$w = [int][Math]::Round(($r.Right - $r.Left) * $scale)
$h = [int][Math]::Round(($r.Bottom - $r.Top) * $scale)
$x = [int][Math]::Round($r.Left * $scale)
$y = [int][Math]::Round($r.Top * $scale)
Write-Host "scale = $scale, dialog size = ${w}x${h}"

Add-Type -AssemblyName System.Drawing
[System.Reflection.Assembly]::LoadWithPartialName('System.Drawing') | Out-Null
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size($w, $h)))
$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host "saved: $out"

$p.WaitForExit(10000) | Out-Null
