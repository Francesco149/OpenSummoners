#!/usr/bin/env bash
# tools/vamap/gen_vamap.sh — regenerate the cross-edition VA map from the Ghidra
# project.  One command: export a relocation-invariant fingerprint for every
# function in each edition, structurally match every edition pair (BinDiff-style,
# tuned for this near-identical MSVC family), and 3-way join the result.
#
# Prereqs: the three unpacked exes imported+analyzed in ghidra/projects/opensummoners
# (sotes-ense-jp.exe, sotes-ense-en.exe, sotes.unpacked.exe).  Add the JP DISC exe
# (see docs/vamap/README.md) and it is picked up automatically once imported.
#
#   nix develop --command bash tools/vamap/gen_vamap.sh
#
# Outputs (gitignored — derived from the user's binaries; regenerate any time):
#   docs/vamap/<edition>.jsonl        per-edition fingerprints
#   docs/vamap/<pairA>-<pairB>.csv    pairwise 1:1 maps + unique-to-edition lists
#   docs/vamap/vamap.csv              the 3-way joined table
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; cd "$ROOT"
OUT="$ROOT/docs/vamap"; mkdir -p "$OUT"
SCRIPTS="$ROOT/tools/ghidra-scripts"

declare -A PROG=( [jpse]=sotes-ense-jp.exe [ense]=sotes-ense-en.exe [enold]=sotes.unpacked.exe )

echo "[1/3] export fingerprints"
for tag in "${!PROG[@]}"; do
    echo "  $tag (${PROG[$tag]})"
    ghidra-analyzeHeadless ghidra/projects opensummoners \
        -process "${PROG[$tag]}" -noanalysis \
        -scriptPath "$SCRIPTS" \
        -postScript ExportFuncFingerprints.java "$OUT/$tag.jsonl" 2>&1 \
        | grep -E 'wrote|ERROR' || true
done

echo "[2/3] pairwise structural match"
python3 tools/vamap/match.py "$OUT/jpse.jsonl"  "$OUT/ense.jsonl"  --tagA jpse  --tagB ense  --out "$OUT/jpse-ense.csv"
python3 tools/vamap/match.py "$OUT/ense.jsonl"  "$OUT/enold.jsonl" --tagA ense  --tagB enold --out "$OUT/ense-enold.csv"
python3 tools/vamap/match.py "$OUT/jpse.jsonl"  "$OUT/enold.jsonl" --tagA jpse  --tagB enold --out "$OUT/jpse-enold.csv"

echo "[3/3] 3-way join"
python3 tools/vamap/combine.py \
    --jpse-ense  "$OUT/jpse-ense.csv" \
    --ense-enold "$OUT/ense-enold.csv" \
    --jpse-enold "$OUT/jpse-enold.csv" \
    --out "$OUT/vamap.csv"
echo "done -> $OUT/vamap.csv"
