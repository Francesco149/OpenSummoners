#!/usr/bin/env bash
# tools/osr_view/install-studio-shortcut.sh — install/refresh the "OpenSummoners
# Trace Studio" desktop + Start Menu shortcuts (Windows) so the human can launch
# the native viewer with ONE click instead of typing the osr_view command.
#
# Architecture (a stable shortcut over a mutable trace pointer):
#   Desktop / Start Menu "OpenSummoners Trace Studio.lnk"
#     -> C:\oss-osr\open-studio.bat
#         reads C:\oss-osr\studio-current.txt  (line 1 = the osr_view ARGS, e.g.
#         "C:\oss-osr\port-theme3.osr C:\oss-osr\retail.osr", or a single .osr)
#         -> launches C:\oss-osr\osr_view.exe <those args>
# So the shortcut never changes; CLAUDE keeps it pointed at the current working
# trace by rewriting studio-current.txt (a 1-line file) whenever it produces a new
# capture and tells the human to run the studio (see CLAUDE.md "trace studio
# shortcut").  osr_view.exe is fully static (-static-libgcc -static-libstdc++), so
# the lone copy on C:\ runs standalone (only system d3d11/ddraw DLLs).
#
# Run from the repo root inside the dev shell:
#   bash tools/osr_view/install-studio-shortcut.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OSR_DIR="/mnt/c/oss-osr"
EXE_SRC="$ROOT/build/osr_view.exe"

[ -f "$EXE_SRC" ] || {
    echo "build/osr_view.exe missing — run: nix develop --command make -C tools/osr_view" >&2
    exit 1
}
mkdir -p "$OSR_DIR"
cp -f "$EXE_SRC" "$OSR_DIR/osr_view.exe"
echo "[studio-shortcut] copied osr_view.exe -> C:\\oss-osr\\osr_view.exe"

# The launcher: read the current trace ARGS from studio-current.txt, launch the viewer.
cat > "$OSR_DIR/open-studio.bat" <<'BAT'
@echo off
REM OpenSummoners Trace Studio launcher.  Opens the native viewer on the CURRENT
REM working trace pair, read from studio-current.txt (line 1 = the osr_view args:
REM "<port.osr> <retail.osr>", or a single .osr).  Claude rewrites studio-current.txt
REM when it produces a new working trace + tells you to run the studio.
set "DIR=%~dp0"
set "ARGS="
set /p ARGS=<"%DIR%studio-current.txt"
REM no /D (a trailing "\" before a quote breaks CMD parsing) — the .lnk sets the cwd.
start "OpenSummoners Trace Studio" "%DIR%osr_view.exe" %ARGS%
BAT
echo "[studio-shortcut] wrote C:\\oss-osr\\open-studio.bat"

# The current-trace pointer — only SEED a default if absent (never clobber a live pair;
# CLAUDE updates this file directly).  No trailing newline so CMD's set /p reads it clean.
if [ ! -f "$OSR_DIR/studio-current.txt" ]; then
    printf 'C:\\oss-osr\\port-theme3.osr C:\\oss-osr\\retail.osr' > "$OSR_DIR/studio-current.txt"
    echo "[studio-shortcut] seeded studio-current.txt (port-theme3 vs retail)"
else
    echo "[studio-shortcut] studio-current.txt exists -> $(cat "$OSR_DIR/studio-current.txt")"
fi

# Create the .lnk shortcuts (desktop + Start Menu) via PowerShell (WScript.Shell).
powershell.exe -NoProfile -Command '
$ws = New-Object -ComObject WScript.Shell
$targets = @(
  (Join-Path $env:USERPROFILE "Desktop\OpenSummoners Trace Studio.lnk"),
  (Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\OpenSummoners Trace Studio.lnk")
)
foreach ($t in $targets) {
  $lnk = $ws.CreateShortcut($t)
  $lnk.TargetPath = "C:\oss-osr\open-studio.bat"
  $lnk.WorkingDirectory = "C:\oss-osr"
  $lnk.IconLocation = "C:\oss-osr\osr_view.exe,0"
  $lnk.Description = "OpenSummoners trace studio - current port vs retail .osr"
  $lnk.WindowStyle = 7
  $lnk.Save()
  Write-Host "[studio-shortcut] created $t"
}
'
echo "[studio-shortcut] done — click \"OpenSummoners Trace Studio\" on the desktop / Start Menu"
