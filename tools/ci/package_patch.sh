#!/usr/bin/env bash
# Assemble the redistributable EN-SE JP-voice patch zip.
# Ships NO game assets — the user supplies sotesx_s.dll from their own JP copy.
#   -> build/ennse-voice-patch.zip
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/ennse_voice"
STAGE="$ROOT/build/ennse-voice-patch"

[ -f "$ROOT/build/version.dll" ] || { echo "build/version.dll missing — run 'make -C tools/ennse_voice' first" >&2; exit 1; }

rm -rf "$STAGE"; mkdir -p "$STAGE"
cp "$ROOT/build/version.dll" "$STAGE/"
cp "$SRC/Install.bat" "$SRC/install.ps1" "$SRC/Uninstall.bat" "$SRC/uninstall.ps1" "$SRC/README.md" "$STAGE/"

( cd "$ROOT/build" && rm -f ennse-voice-patch.zip && zip -r -q ennse-voice-patch.zip ennse-voice-patch )
echo "wrote build/ennse-voice-patch.zip"
