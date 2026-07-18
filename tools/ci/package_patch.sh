#!/usr/bin/env bash
# Assemble the redistributable EN-SE JP-voice patch zip.
# Ships NO game assets — the user supplies sotesx_s.dll from their own JP copy.
# Bundles the generic mod loader (version.dll) + the voice mod (ennse_voice.dll).
#   -> build/ennse-voice-patch.zip
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/ennse_voice"
STAGE="$ROOT/build/ennse-voice-patch"

[ -f "$ROOT/build/version.dll" ]     || { echo "build/version.dll missing — run 'make -C tools/mod_loader' first" >&2; exit 1; }
[ -f "$ROOT/build/ennse_voice.dll" ] || { echo "build/ennse_voice.dll missing — run 'make -C tools/ennse_voice' first" >&2; exit 1; }

rm -rf "$STAGE"; mkdir -p "$STAGE"
cp "$ROOT/build/version.dll" "$ROOT/build/ennse_voice.dll" "$STAGE/"
cp "$SRC/Install.bat" "$SRC/install.ps1" "$SRC/Uninstall.bat" "$SRC/uninstall.ps1" "$SRC/README.md" "$STAGE/"

( cd "$ROOT/build" && rm -f ennse-voice-patch.zip && zip -r -q ennse-voice-patch.zip ennse-voice-patch )
echo "wrote build/ennse-voice-patch.zip"
