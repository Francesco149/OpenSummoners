#!/usr/bin/env bash
# tools/ghidra-tag-and-export.sh — parse C headers, apply thiscall tags,
# re-export decomp.
#
# Three stages in one analyzeHeadless session (DTM survives across them):
#   1. ParseCSource.java — parse src/*.h into Ghidra's Data Type Manager
#      so the class names referenced by TagThiscallFunctions's TAGS
#      table resolve to typed `this *` (not `void *`).
#   2. TagThiscallFunctions.java — apply (address, class, prototype)
#      tags from the in-script TAGS table.
#   3. ghidra-headless.sh — re-run full re-export of docs/decompiled/.
#
# Workflow:
#   1. Edit src/*.h (struct shapes) AND/OR
#      tools/ghidra-scripts/TagThiscallFunctions.java (TAGS rows).
#   2. Close any Ghidra GUI instance that holds the project's write lock.
#   3. Run this script.
#   4. Updated decomps land in docs/decompiled/*.c with typed this->field
#      accesses across the family.
#
# Re-runs are idempotent: parse step pre-cleans stale DTM entries before
# re-parse, tag step skips already-applied tags, export rewrites the
# dump from scratch.
#
# See docs/findings/cpp-recovery-workflow.md.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

INPUT="$ROOT/vendor/unpacked/sotes.unpacked.exe"
PROJ_DIR="$ROOT/ghidra/projects"
PROJ_NAME="opensummoners"
SCRIPT_DIR="$ROOT/tools/ghidra-scripts"

# Headers that define structs referenced by TagThiscallFunctions.java's
# TAGS table (class-name column).  Order matters only if one header
# includes another; ours are self-contained so any order works.
HEADERS=(
    "$ROOT/src/asset_register.h"    # ar_sprite_slot, ar_gdi_slot, ar_sound_slot
    "$ROOT/src/bitmap_session.h"    # bitmap_session
    "$ROOT/src/wnd_proc.h"          # paint_ctx, input_dev, zdm, zdm_entry,
                                    # input_mgr, log_singleton, wp_app_ctx
)

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

for f in "$SCRIPT_DIR/ParseCSource.java" "$SCRIPT_DIR/TagThiscallFunctions.java"; do
    if [[ ! -f "$f" ]]; then
        red "Missing $f (should be tracked in git)"
        exit 1
    fi
done

for h in "${HEADERS[@]}"; do
    if [[ ! -f "$h" ]]; then
        red "Missing header: $h"
        exit 1
    fi
done

bold "[1/2] Parsing headers + applying thiscall tags"
green "  headers: ${HEADERS[*]##*/}"
yellow "  via ParseCSource.java + TagThiscallFunctions.java — typically a few seconds"

ghidra-analyzeHeadless "$PROJ_DIR" "$PROJ_NAME" \
    -process "$(basename "$INPUT")" \
    -noanalysis \
    -scriptPath "$SCRIPT_DIR" \
    -postScript ParseCSource.java "${HEADERS[@]}" \
    -postScript TagThiscallFunctions.java 2>&1 \
    | grep -E 'ParseCSource|TagThiscallFunctions|parsed|Result:|ERROR|WARNING|REPORT.*Save' \
    || { red "parse/tag step failed (see full output above)"; exit 1; }

echo
bold "[2/2] Re-exporting C decomp"
yellow "  via tools/ghidra-headless.sh — typically a few minutes"

./tools/ghidra-headless.sh
