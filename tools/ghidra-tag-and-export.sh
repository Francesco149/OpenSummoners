#!/usr/bin/env bash
# tools/ghidra-tag-and-export.sh — apply thiscall tags + re-export decomp.
#
# Convenience wrapper that runs TagThiscallFunctions.java against the
# existing Ghidra project, then re-exports the C dump via the existing
# ghidra-headless.sh.  Use this after editing the TAGS array in
# tools/ghidra-scripts/TagThiscallFunctions.java to apply new tags
# without manually invoking analyzeHeadless twice.
#
# Workflow context:
#   1. Edit tools/ghidra-scripts/TagThiscallFunctions.java — add rows to
#      the TAGS array for each new thiscall function being tagged.
#   2. Close Ghidra (headless needs the project's write lock).
#   3. Run this script.
#   4. The updated decomps land in docs/decompiled/*.c with typed
#      this->field accesses for all newly-tagged functions, plus
#      typed call-sites in their callers.
#
# Re-runs are idempotent: the tag step skips functions already tagged
# to the target state, and the export step rewrites the dump from
# scratch.
#
# See docs/findings/cpp-recovery-workflow.md.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

INPUT="$ROOT/vendor/unpacked/sotes.unpacked.exe"
PROJ_DIR="$ROOT/ghidra/projects"
PROJ_NAME="opensummoners"
SCRIPT_DIR="$ROOT/tools/ghidra-scripts"

bold()   { printf "\033[1m%s\033[0m\n" "$*"; }
green()  { printf "\033[32m%s\033[0m\n" "$*"; }
red()    { printf "\033[31m%s\033[0m\n" "$*" >&2; }
yellow() { printf "\033[33m%s\033[0m\n" "$*"; }

# ─── pre-flight ────────────────────────────────────────────────────────────
if [[ ! -f "$PROJ_DIR/$PROJ_NAME.gpr" ]]; then
    red "No Ghidra project at $PROJ_DIR/$PROJ_NAME.gpr."
    red "Run ./tools/ghidra-headless.sh first to import + analyze the binary."
    exit 1
fi

if ! command -v ghidra-analyzeHeadless >/dev/null 2>&1; then
    red "ghidra-analyzeHeadless not on PATH.  Are you in the nix dev shell? (nix develop)"
    exit 1
fi

if [[ ! -f "$SCRIPT_DIR/TagThiscallFunctions.java" ]]; then
    red "Missing $SCRIPT_DIR/TagThiscallFunctions.java (should be tracked in git)"
    exit 1
fi

bold "[1/2] Applying thiscall tags"
yellow "  via TagThiscallFunctions.java — typically a few seconds"

ghidra-analyzeHeadless "$PROJ_DIR" "$PROJ_NAME" \
    -process "$(basename "$INPUT")" \
    -noanalysis \
    -scriptPath "$SCRIPT_DIR" \
    -postScript TagThiscallFunctions.java 2>&1 \
    | grep -E 'TagThiscallFunctions|Result:|ERROR|REPORT.*Save' \
    || { red "tag step failed (see full output above)"; exit 1; }

echo
bold "[2/2] Re-exporting C decomp"
yellow "  via tools/ghidra-headless.sh — typically a few minutes"

./tools/ghidra-headless.sh
