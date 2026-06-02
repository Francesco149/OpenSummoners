#!/usr/bin/env bash
# tools/setup.sh — one-shot bootstrap for a fresh clone.
#
#   1. Symlinks the user's Steam install of Fortune Summoners into
#      vendor/original/.
#   2. Detects Steam DRM on sotes.exe by looking for a `.bind` section.
#      If present, runs Steamless via WSLInterop (it's .NET — Windows
#      already has the runtime, no wine needed) and stashes the unpacked
#      exe in vendor/unpacked/.
#   3. SHA256s the key files for reproducibility / drift detection.
#   4. Prints next steps.
#
# Idempotent.  Re-running re-validates the symlink, re-checks the .bind
# detection, and re-unpacks only if the input sha256 changed.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

GAME_DIR="${OPENSUMMONERS_GAME_DIR:-/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners}"
STEAMLESS_DIR="${OPENSUMMONERS_STEAMLESS_DIR:-/mnt/c/Users/headpats/Documents/_devtools/Steamless.v3.1.0.5.-.by.atom0s}"

VENDOR="$ROOT/vendor"
ORIGINAL="$VENDOR/original"
UNPACKED="$VENDOR/unpacked"

bold()   { printf "\033[1m%s\033[0m\n" "$*"; }
green()  { printf "\033[32m%s\033[0m\n" "$*"; }
red()    { printf "\033[31m%s\033[0m\n" "$*" >&2; }
yellow() { printf "\033[33m%s\033[0m\n" "$*"; }

# ─── pre-flight ────────────────────────────────────────────────────────────
bold "[1/5] Pre-flight"

if [[ ! -d "$GAME_DIR" ]]; then
    red "Game directory not found: $GAME_DIR"
    red "Set OPENSUMMONERS_GAME_DIR to the absolute path of your Fortune Summoners install."
    exit 1
fi
if [[ ! -f "$GAME_DIR/sotes.exe" ]]; then
    red "sotes.exe not found inside $GAME_DIR"
    exit 1
fi
green "  ✓ game dir:    $GAME_DIR"

# objdump is in the flake — error early if we're not in the dev shell.
if ! command -v objdump >/dev/null 2>&1; then
    red "objdump not on PATH — are you inside \`nix develop\`?"
    exit 125
fi
green "  ✓ objdump:     $(command -v objdump)"

# ─── symlink game files ────────────────────────────────────────────────────
bold "[2/5] Linking game files into vendor/original/"

mkdir -p "$VENDOR"
# Replace the symlink each run so a moved game install doesn't go stale.
if [[ -L "$ORIGINAL" || -e "$ORIGINAL" ]]; then
    rm -f "$ORIGINAL"
fi
ln -s "$GAME_DIR" "$ORIGINAL"
green "  ✓ vendor/original -> $GAME_DIR"

# ─── steam-DRM detection ──────────────────────────────────────────────────
bold "[3/5] Checking sotes.exe for Steam DRM"

# The classic SteamStub marker is a `.bind` section appended to the PE.
# objdump -h lists section names; we grep for one called .bind.
if objdump -h "$ORIGINAL/sotes.exe" 2>/dev/null | grep -qE '^\s*[0-9]+\s+\.bind\b'; then
    has_drm=1
    yellow "  Steam DRM detected (.bind section present); will run Steamless"
else
    has_drm=0
    green "  No .bind section — sotes.exe appears DRM-free, skipping Steamless"
fi

# ─── steamless unpack (if needed) ──────────────────────────────────────────
bold "[4/5] Producing vendor/unpacked/sotes.unpacked.exe"

mkdir -p "$UNPACKED"
INPUT_EXE="$ORIGINAL/sotes.exe"
ORIG_SHA="$(sha256sum "$INPUT_EXE" | awk '{print $1}')"
SHA_FILE="$UNPACKED/.input.sha256"

NEED_UNPACK=1
if [[ -f "$UNPACKED/sotes.unpacked.exe" && -f "$SHA_FILE" ]]; then
    if [[ "$(cat "$SHA_FILE")" == "$ORIG_SHA" ]]; then
        NEED_UNPACK=0
        green "  ✓ vendor/unpacked/sotes.unpacked.exe up to date (input sha unchanged)"
    fi
fi

if (( NEED_UNPACK )); then
    if (( has_drm )); then
        if [[ ! -f "$STEAMLESS_DIR/Steamless.CLI.exe" ]]; then
            red "Steamless.CLI.exe not found at $STEAMLESS_DIR"
            red "Set OPENSUMMONERS_STEAMLESS_DIR or download from"
            red "  https://github.com/atom0s/Steamless/releases"
            exit 1
        fi

        # Copy (not symlink) into a Linux tmp dir, since Steamless writes its
        # output next to the input and may not have write access to the
        # Steam install dir.  We then move the result into vendor/unpacked/.
        WORK="$(mktemp -d)"
        trap 'rm -rf "$WORK"' EXIT
        cp "$INPUT_EXE" "$WORK/sotes.exe"

        WORK_WIN="$(wslpath -w "$WORK")"
        yellow "  invoking: $STEAMLESS_DIR/Steamless.CLI.exe --quiet $WORK_WIN\\sotes.exe"
        "$STEAMLESS_DIR/Steamless.CLI.exe" --quiet "$WORK_WIN\\sotes.exe" || {
            red "Steamless failed.  Re-run without --quiet for the full output:"
            red "  '$STEAMLESS_DIR/Steamless.CLI.exe' '$WORK_WIN\\sotes.exe'"
            exit 1
        }

        # Steamless writes alongside input as `<name>.unpacked.exe`.
        if [[ ! -f "$WORK/sotes.exe.unpacked.exe" ]]; then
            red "Steamless ran but expected output is missing: $WORK/sotes.exe.unpacked.exe"
            ls -la "$WORK"
            exit 1
        fi
        mv "$WORK/sotes.exe.unpacked.exe" "$UNPACKED/sotes.unpacked.exe"
        rm -rf "$WORK"
        trap - EXIT
    else
        # No DRM — just copy the bytes through so downstream tooling has
        # one canonical input path regardless of whether unpacking ran.
        cp "$INPUT_EXE" "$UNPACKED/sotes.unpacked.exe"
    fi
    echo "$ORIG_SHA" > "$SHA_FILE"

    UNPACK_SHA="$(sha256sum "$UNPACKED/sotes.unpacked.exe" | awk '{print $1}')"
    green "  ✓ vendor/unpacked/sotes.unpacked.exe"
    yellow "    input  sha256: $ORIG_SHA"
    yellow "    output sha256: $UNPACK_SHA"
fi

# Co-locate the unpacked exe *inside the game dir* (= vendor/original, the
# symlink) so the Frida harness can spawn it: the engine resolves config.dat /
# sotesd.dll relative to its own module directory (GetModuleFileName), not the
# cwd, so the exe must live next to those assets.  See engine-quirks #53;
# this path is frida_capture.py's default RETAIL_EXE.
if ! cmp -s "$UNPACKED/sotes.unpacked.exe" "$ORIGINAL/sotes.unpacked.exe" 2>/dev/null; then
    cp "$UNPACKED/sotes.unpacked.exe" "$ORIGINAL/sotes.unpacked.exe"
fi
green "  ✓ vendor/original/sotes.unpacked.exe (co-located in game dir for Frida)"

# ─── hash key files ────────────────────────────────────────────────────────
bold "[5/5] Hashing key files"

# Important binaries / assets — track these so we notice if Steam pushed
# an update or the user repatched.
KEY_FILES=(
    "sotes.exe"
    "sotesd.dll"
    "sotesp.dll"
    "sotesw.dll"
    "lizsoft.spl"
    "readme-manual.htm"
)

for f in "${KEY_FILES[@]}"; do
    p="$GAME_DIR/$f"
    if [[ -f "$p" ]]; then
        sha="$(sha256sum "$p" | awk '{print $1}')"
        sz="$(stat -c %s "$p" 2>/dev/null || stat -f %z "$p" 2>/dev/null || echo '?')"
        printf "  %-30s %s  %12s B\n" "$f" "${sha:0:16}…" "$sz"
    else
        yellow "  (missing) $f"
    fi
done

cat <<EOF

Inputs:
  vendor/original/                 → $GAME_DIR
  vendor/unpacked/sotes.unpacked.exe (sha tracked in vendor/unpacked/.input.sha256)

Next:
  • ./tools/ghidra-headless.sh         # batch decompile the unpacked exe
  • ./tools/run-retail.sh              # boot retail under the Frida harness
  • ./tools/run-opensummoners.sh       # build + run our drop-in skeleton

Reminder: every command needs the dev shell.  Either run
  nix develop
once at the top of a session, or prefix individual commands with
  nix develop --command <cmd>
EOF
