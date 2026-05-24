#!/usr/bin/env bash
# tools/run-opensummoners.sh — canonical OpenSummoners dev-loop recipe.
#
# Does, in one step:
#   1. Build src/ and tools/launcher/.
#   2. Copy opensummoners-debug.exe to /tmp/opensummoners-<pid>.exe so each
#      run gets a fresh PE-image mapping (dodges WSL2's image-by-path reuse,
#      where the kernel caches the mapping for the same path/inode and
#      "runs yesterday's code").
#   3. Exec the launcher to supervise it.  Launcher guarantees the child
#      dies if anything kills us — Job Object with KILL_ON_JOB_CLOSE.
#   4. Clean up the /tmp/opensummoners-*.exe after.
#
# Run inside `nix develop`.  Defaults match a typical smoke run:
# --hide-window, --frames 200, --timeout-ms 8000.  Override the timeout
# via $OPENSUMMONERS_TIMEOUT_MS, the frame count via $OPENSUMMONERS_FRAMES,
# or pass extra child args after `--`.
#
# Examples:
#   tools/run-opensummoners.sh
#   tools/run-opensummoners.sh --frames 500
#   OPENSUMMONERS_TIMEOUT_MS=30000 tools/run-opensummoners.sh
#   tools/run-opensummoners.sh -- --show-msgbox
#
# Run without a frame count (interactive) by setting OPENSUMMONERS_FRAMES=0
# AND a generous timeout.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v i686-w64-mingw32-gcc >/dev/null 2>&1; then
    echo "error: mingw32 toolchain not on PATH — are you inside \`nix develop\`?" >&2
    exit 125
fi

TIMEOUT_MS=${OPENSUMMONERS_TIMEOUT_MS:-8000}
FRAMES=${OPENSUMMONERS_FRAMES:-200}

# All args are pass-through to the child by default.  Optional leading `--`
# is accepted and stripped.  We don't have wrapper-specific flags right now
# — knobs live in env vars.
if [[ "${1:-}" == "--" ]]; then shift; fi
extra=("$@")

# If the child argv already specifies --frames, skip our default frame
# cap so the user's choice wins.
user_set_frames=0
for a in "${extra[@]}"; do
    if [[ "$a" == --frames || "$a" == "--frames="* ]]; then user_set_frames=1; fi
done

echo "[run-opensummoners] building src/ + tools/launcher/"
make -s -C src
make -s -C tools/launcher

stamp=$$
tmp_exe="/tmp/opensummoners-${stamp}.exe"
cleanup() { rm -f /tmp/opensummoners-*.exe; }
trap cleanup EXIT

cp build/opensummoners-debug.exe "$tmp_exe"

child_args=(--hide-window)
if [[ "$FRAMES" -gt 0 && "$user_set_frames" -eq 0 ]]; then
    child_args+=(--frames "$FRAMES")
fi
child_args+=("${extra[@]}")

if [[ "$user_set_frames" -eq 1 ]]; then
    echo "[run-opensummoners] launcher → $tmp_exe (timeout=${TIMEOUT_MS}ms, frames=user)"
else
    echo "[run-opensummoners] launcher → $tmp_exe (timeout=${TIMEOUT_MS}ms, frames=${FRAMES})"
fi

exec ./build/opensummoners-launcher.exe \
    --timeout-ms "$TIMEOUT_MS" \
    -- \
    "$tmp_exe" "${child_args[@]}"
