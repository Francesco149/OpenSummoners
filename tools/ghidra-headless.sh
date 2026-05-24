#!/usr/bin/env bash
# tools/ghidra-headless.sh — batch decompile vendor/unpacked/sotes.unpacked.exe.
#
# Imports the Steamless-unpacked sotes.exe into a Ghidra project, runs full
# auto-analysis, then runs ExportDecompiledC.java as a post-script that
# dumps every decompiled function as C source under docs/decompiled/
# (gitignored).
#
# Idempotent: the Ghidra project lives in ghidra/projects/opensummoners/.
# Re-running re-uses the existing project: it skips re-analysis and only
# regenerates the C-dump.  Useful workflow: rename a function in the
# Ghidra GUI, then re-run this script to get the renamed function in
# docs/decompiled/all.c without paying for re-analysis.
#
# We point at the *unpacked* exe (post-Steamless) rather than the on-disk
# sotes.exe because the SteamStub wrapper hides the real entry point
# inside a `.bind` section and obfuscates the main code section.  Run
# tools/setup.sh first.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

INPUT="$ROOT/vendor/unpacked/sotes.unpacked.exe"
PROJ_DIR="$ROOT/ghidra/projects"
PROJ_NAME="opensummoners"
OUT_DIR="$ROOT/docs/decompiled"
SCRIPT_DIR="$ROOT/tools/ghidra-scripts"

bold()   { printf "\033[1m%s\033[0m\n" "$*"; }
green()  { printf "\033[32m%s\033[0m\n" "$*"; }
red()    { printf "\033[31m%s\033[0m\n" "$*" >&2; }
yellow() { printf "\033[33m%s\033[0m\n" "$*"; }

# ─── pre-flight ────────────────────────────────────────────────────────────
if [[ ! -f "$INPUT" ]]; then
    red "Missing $INPUT — run ./tools/setup.sh first."
    exit 1
fi

if ! command -v ghidra-analyzeHeadless >/dev/null 2>&1; then
    red "ghidra-analyzeHeadless not on PATH.  Are you in the nix dev shell? (nix develop)"
    exit 1
fi

mkdir -p "$PROJ_DIR" "$OUT_DIR" "$SCRIPT_DIR"

bold "[1/3] Pre-flight"
green "  ✓ ghidra-analyzeHeadless: $(command -v ghidra-analyzeHeadless)"
green "  ✓ input binary:    $INPUT"
green "  ✓ project dir:     $PROJ_DIR/$PROJ_NAME"
green "  ✓ output dir:      $OUT_DIR"

# ─── decompile-export script ──────────────────────────────────────────────
# Ghidra's headless mode in nixpkgs is NOT built with PyGhidra, so `.py`
# scripts fail with "Python is not available".  We use a Java script
# instead — works in plain headless mode without any extra flags.
if [[ ! -f "$SCRIPT_DIR/ExportDecompiledC.java" ]]; then
    red "Missing $SCRIPT_DIR/ExportDecompiledC.java (should be tracked in git)"
    exit 1
fi
green "  ✓ uses $SCRIPT_DIR/ExportDecompiledC.java"

# ─── run headless analysis ────────────────────────────────────────────────
bold "[2/3] Importing + analyzing"

GPR="$PROJ_DIR/$PROJ_NAME.gpr"

if [[ ! -f "$GPR" ]]; then
    yellow "  first run — importing + analyzing + exporting (single-digit minutes)..."
    ghidra-analyzeHeadless "$PROJ_DIR" "$PROJ_NAME" \
        -import "$INPUT" \
        -scriptPath "$SCRIPT_DIR" \
        -postScript ExportDecompiledC.java "$OUT_DIR" 2>&1 | tail -40 \
        || { red "ghidra-analyzeHeadless failed"; exit 1; }
else
    yellow "  project exists — skipping import/analysis, re-running export..."
    ghidra-analyzeHeadless "$PROJ_DIR" "$PROJ_NAME" \
        -process "$(basename "$INPUT")" \
        -noanalysis \
        -scriptPath "$SCRIPT_DIR" \
        -postScript ExportDecompiledC.java "$OUT_DIR" 2>&1 | tail -20 \
        || { red "ghidra-analyzeHeadless failed"; exit 1; }
fi

# ─── summary ──────────────────────────────────────────────────────────────
bold "[3/3] Summary"

FUNC_COUNT="$(wc -l < "$OUT_DIR/functions.csv" 2>/dev/null || echo 0)"
ALLSZ="$(wc -c < "$OUT_DIR/all.c" 2>/dev/null || echo 0)"
green "  ✓ $((FUNC_COUNT - 1)) functions decompiled"
green "  ✓ docs/decompiled/all.c                ($ALLSZ bytes)"
green "  ✓ docs/decompiled/by-address/*.c"
green "  ✓ docs/decompiled/by-name/*.c"
green "  ✓ docs/decompiled/functions.csv"

cat <<'EOF'

Next:
  • Read docs/decompiled/functions.csv to find named functions.
  • grep -n 'WinMain\|wWinMain\|DirectDrawCreate' docs/decompiled/all.c
    to locate entry + renderer init.
  • Open Ghidra GUI on the same project for interactive analysis:
      ghidraRun &  # then File > Open Project > ghidra/projects/opensummoners.gpr
EOF
