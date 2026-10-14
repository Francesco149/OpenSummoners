# run_ida.ps1 — run one headless IDA export (Windows side; staged to C:\oss-ida\).
#   powershell -File C:\oss-ida\run_ida.ps1 -Exe <exe|i64> -Out <dir> [-Opts "kb=...;noasm"]
# Writes <Out>\ida.log + the oss_ida_export.py outputs; prints exit/peak/secs.
param([Parameter(Mandatory)][string]$Exe, [Parameter(Mandatory)][string]$Out, [string]$Opts = "")
$ErrorActionPreference = 'Stop'
$ida = (Get-ChildItem 'C:\Program Files' -Filter 'IDA*' -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName + '\idat.exe'
New-Item -ItemType Directory -Force $Out | Out-Null
$o = "oss:$Out"; if ($Opts) { $o += ";$Opts" }
$a = @('-A', "-L$Out\ida.log", "-O$o", '-SC:\oss-ida\oss_ida_export.py', $Exe)
$p = Start-Process -FilePath $ida -ArgumentList $a -PassThru -WorkingDirectory (Split-Path $Exe) -WindowStyle Hidden
$t0 = Get-Date; $peak = 0
while (-not $p.HasExited) {
  try { $p.Refresh(); if ($p.PeakWorkingSet64 -gt $peak) { $peak = $p.PeakWorkingSet64 } } catch {}
  if (((Get-Date) - $t0).TotalSeconds -gt 5400) { Stop-Process -Id $p.Id -Force; "TIMEOUT"; break }
  Start-Sleep -Milliseconds 500
}
"exit=$($p.ExitCode) peakMB=$([int]($peak/1MB)) secs=$([int]((Get-Date)-$t0).TotalSeconds) out=$Out"
