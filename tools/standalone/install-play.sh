#!/usr/bin/env bash
# tools/standalone/install-play.sh — a ready-to-play standalone on the Windows box (ssh win):
#   C:\OpenSummoners\  = the user's own game files copied from the Steam install (stock sotes.exe + the asset DLLs +
#                        lizsoft.spl; copied only when absent: saves in its user\ are never touched)
#                        + build/rhost/opensummoners-standalone.exe (refreshed every run)
#   Desktop / Start Menu "OpenSummoners (standalone).lnk" -> that exe (icon: sotes.exe's own)
# The exe's log + audit land in C:\OpenSummoners\opensummoners-standalone\.  To continue a real playthrough, copy the
# Steam folder's user\ into C:\OpenSummoners\user\.
#   nix develop --command bash tools/standalone/install-play.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
EXE="$ROOT/build/rhost/opensummoners-standalone.exe"
[ -f "$EXE" ] || { echo "build it first: nix develop --command bash tools/rhost/build.sh" >&2; exit 1; }
SSH=(ssh -o BatchMode=yes win)
"${SSH[@]}" '
$steam = "C:\Program Files (x86)\Steam\steamapps\common\Fortune Summoners"
$dst = "C:\OpenSummoners"
New-Item -ItemType Directory -Force $dst | Out-Null
foreach ($f in "sotes.exe", "sotesd.dll", "sotesw.dll", "sotesp.dll", "lizsoft.spl") {
  if (-not (Test-Path (Join-Path $dst $f))) { Copy-Item (Join-Path $steam $f) $dst; Write-Host "[play] copied $f" }
}
exit 0' 2>&1 | grep -v "post-quantum\|store now\|upgraded" || true
scp -q -o BatchMode=yes "$EXE" "win:C:/OpenSummoners/opensummoners-standalone.exe"
echo "[play] C:\\OpenSummoners\\opensummoners-standalone.exe updated"
"${SSH[@]}" '
$ws = New-Object -ComObject WScript.Shell
foreach ($t in (Join-Path $env:USERPROFILE "Desktop\OpenSummoners (standalone).lnk"),
               (Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\OpenSummoners (standalone).lnk")) {
  $l = $ws.CreateShortcut($t)
  $l.TargetPath = "C:\OpenSummoners\opensummoners-standalone.exe"
  $l.WorkingDirectory = "C:\OpenSummoners"
  $l.IconLocation = "C:\OpenSummoners\sotes.exe,0"
  $l.Description = "Fortune Summoners on the OpenSummoners port alone (no retail code)"
  $l.Save()
  Write-Host "[play] shortcut $t"
}' 2>&1 | grep -v "post-quantum\|store now\|upgraded" || true
