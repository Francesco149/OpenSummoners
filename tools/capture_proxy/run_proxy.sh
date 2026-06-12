#!/usr/bin/env bash
# tools/capture_proxy/run_proxy.sh — boot retail under the trace-studio-v2
# native capture proxy (NO Frida, NO injector).
#
# Drops build/ddraw_proxy.dll into the game dir as ddraw.dll (the exe imports
# DirectDrawCreateEx, so it auto-loads — exe dir wins the DLL search order),
# drops a per-run unpacked-exe copy, launches it via PowerShell Start-Process
# (WSLInterop, no Steam, no exec bit), waits, kills it, prints the proxy log,
# and ALWAYS cleans up — especially ddraw.dll, so v1 Frida runs (which use the
# same game dir) never accidentally load this proxy.
#
# Usage:  nix develop --command tools/capture_proxy/run_proxy.sh [seconds] [input-trace]
#         input-trace = a WSL-path JSONL nav trace ({"frame":N,"ids":[..]});
#                       copied to C:\ and replayed via the ring injection.
# Env:    OSS_* config (see proxy_config.h) is passed through to the proxy.
#         OSS_PROXY_LOG defaults to C:\oss-osr\proxy.log (native NTFS staging).
#
# Run inside `nix develop` (for wslpath); the launch itself is pure WSLInterop.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SECS="${1:-8}"
TRACE="${2:-}"

DLL="$ROOT/build/ddraw_proxy.dll"
UNPACKED="$ROOT/vendor/unpacked/sotes.unpacked.exe"
GAME_DIR="${OPENSUMMONERS_GAME_DIR:-/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners}"

[[ -f "$DLL" ]]      || { echo "error: $DLL missing — run: make -C tools/capture_proxy" >&2; exit 1; }
[[ -f "$UNPACKED" ]] || { echo "error: $UNPACKED missing — run ./tools/setup.sh" >&2; exit 1; }
[[ -d "$GAME_DIR" ]] || { echo "error: game dir not found: $GAME_DIR" >&2; exit 1; }

# Native NTFS staging dir for the log (storage discipline: never the 9p mount).
LOG_WIN="${OSS_PROXY_LOG:-C:\\oss-osr\\proxy.log}"
LOG_WSL="$(wslpath -u "$LOG_WIN")"
mkdir -p "$(dirname "$LOG_WSL")"
: > "$LOG_WSL"   # truncate prior log

# Optional nav trace: stage it on C:\ and point the proxy at it.
TRACE_WIN=""
if [[ -n "$TRACE" ]]; then
    [[ -f "$TRACE" ]] || { echo "error: trace not found: $TRACE" >&2; exit 1; }
    TRACE_WSL="$(dirname "$LOG_WSL")/input.trace"
    cp -f "$TRACE" "$TRACE_WSL"
    TRACE_WIN="$(wslpath -w "$TRACE_WSL")"
    echo "[run_proxy] trace: $TRACE -> $TRACE_WIN ($(grep -c . "$TRACE") lines)"
fi

DROP_EXE="$GAME_DIR/sotes-unpacked-proxy-$$.exe"
DROP_DLL="$GAME_DIR/ddraw.dll"

cleanup() {
    rm -f "$DROP_EXE" "$DROP_DLL"
    rm -f "$GAME_DIR"/sotes-unpacked-proxy-*.exe 2>/dev/null || true
}
trap cleanup EXIT

cp -f "$DLL" "$DROP_DLL"
cp -f "$UNPACKED" "$DROP_EXE"

WIN_EXE="$(wslpath -w "$DROP_EXE")"
WIN_CWD="$(wslpath -w "$GAME_DIR")"

# Pass the OSS_* config + the log path through the PowerShell env to the child.
PS_ENV=""
PS_ENV+="\$env:OSS_PROXY_LOG='$LOG_WIN'; "
[[ -n "$TRACE_WIN" ]] && PS_ENV+="\$env:OSS_INPUT_TRACE='$TRACE_WIN'; "
for v in OSS_TURBO OSS_LOCKSTEP OSS_TURBO_STEP_MS OSS_LOCKSTEP_STEP_MS \
         OSS_HIDE_WINDOW OSS_DISMISS_DIALOG OSS_SILENT_AUDIO \
         OSS_SEED_PIN OSS_SEED_VALUE; do
    val="${!v:-}"
    [[ -n "$val" ]] && PS_ENV+="\$env:$v='$val'; "
done

echo "[run_proxy] exe:  $WIN_EXE"
echo "[run_proxy] cwd:  $WIN_CWD"
echo "[run_proxy] log:  $LOG_WSL"
echo "[run_proxy] run for ${SECS}s..."

PS_CMD="${PS_ENV}\$p = Start-Process -FilePath '$WIN_EXE' -WorkingDirectory '$WIN_CWD' -PassThru; \
Start-Sleep -Seconds $SECS; \
try { Stop-Process -Id \$p.Id -Force -ErrorAction SilentlyContinue } catch {}; \
Write-Output (\"pid=\" + \$p.Id)"

powershell.exe -NoProfile -Command "$PS_CMD" || true

echo "[run_proxy] ===== proxy log ($LOG_WSL) ====="
cat "$LOG_WSL" 2>/dev/null || echo "(no log written)"
