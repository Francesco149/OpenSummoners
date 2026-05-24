#!/usr/bin/env bash
# tools/run-retail.sh — boot the retail sotes.exe under the Frida harness.
#
# Single source of truth for "run the retail game with sane defaults":
# hidden window, turbo, silent audio, MessageBox redirect, max-frames cap,
# duration cap.  Picks vendor/original/sotes.exe by default.
#
# Examples:
#   tools/run-retail.sh                       # 30 s headless smoke run
#   tools/run-retail.sh --duration-ms 5000    # short smoke
#   tools/run-retail.sh --show-window --no-turbo  # debug a render issue
#   tools/run-retail.sh --show-msgbox         # see real popups (debug harness)
#
# Env overrides (rarely needed):
#   OPENSUMMONERS_DURATION_MS   default duration (ms)
#   OPENSUMMONERS_MAX_FRAMES    soft frame cap
#
# Run inside `nix develop`.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v python3 >/dev/null 2>&1; then
    echo "error: python3 not on PATH — are you inside \`nix develop\`?" >&2
    exit 125
fi

DURATION_MS=${OPENSUMMONERS_DURATION_MS:-30000}
MAX_FRAMES=${OPENSUMMONERS_MAX_FRAMES:-30000}  # drained Peek/GetMessage events;
                                               # ≈ msg-pump iterations, not
                                               # rendered frames.  30k ≈ a few
                                               # minutes of headless play.

# The on-disk sotes.exe is Steam-DRM'd.  Spawning it outside the Steam
# process tree trips the DRM check ("Steam Error: Application load error
# P:0000065432" in the first smoke run on 2026-05-24).  Instead we run
# the Steamless-unpacked exe — but it has to live alongside the game's
# DLLs (sotesp/d/w + lizsoft.spl) because Windows searches the exe's
# own directory first for imports.  Drop a per-run copy into the game
# directory; clean it up on exit.
UNPACKED="$ROOT/vendor/unpacked/sotes.unpacked.exe"
if [[ ! -f "$UNPACKED" ]]; then
    echo "error: $UNPACKED missing — run ./tools/setup.sh first" >&2
    exit 1
fi

GAME_DIR="${OPENSUMMONERS_GAME_DIR:-/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners}"
if [[ ! -d "$GAME_DIR" ]]; then
    echo "error: game dir not found: $GAME_DIR" >&2
    exit 1
fi

DROP="$GAME_DIR/sotes-unpacked-$$.exe"
cleanup() {
    # Remove any leftover sotes-unpacked-*.exe (in case a previous run
    # was SIGKILL'd and never reached its own cleanup).
    rm -f "$GAME_DIR"/sotes-unpacked-*.exe
}
trap cleanup EXIT
cp "$UNPACKED" "$DROP"

echo "[run-retail] drop-in copy: $DROP"

exec python3 tools/frida_capture.py \
    --exe          "$DROP" \
    --cwd          "$GAME_DIR" \
    --duration-ms  "$DURATION_MS" \
    --max-frames   "$MAX_FRAMES" \
    "$@"
