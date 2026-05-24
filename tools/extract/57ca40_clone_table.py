#!/usr/bin/env python3
"""
Extract the 94 SS_MGR slot-clone calls (FUN_004179b0) from FUN_0057ca40's
Ghidra decomp.  Used to (re)generate the body of
src/asset_register.c::group3_clones[] in the partial port of
FUN_0057ca40 (ar_register_group3_sprites tail).

Each call is `FUN_004179b0(dst_pool_idx, src_pool_idx);` invoked as a
__thiscall on the SS_MGR singleton (== input_mgr at 0x008a6b60 — see
docs/findings/0057ca40-rabbit-hole.md §7).  It clones one slot's
metadata into another and copies the marker+flag fields of the source
info-entry into the destination info-entry.

Re-run after re-exporting the decomp (e.g. via
tools/ghidra-tag-and-export.sh) and diff the output against the
checked-in table to spot drift.

Usage:
    python3 tools/extract/57ca40_clone_table.py \\
        docs/decompiled/by-address/57ca40.c

Emits C source to stdout: struct definition + table + iterator.

See docs/findings/0057ca40-rabbit-hole.md §3 for the slot-clone scope
and §7 for the SS_MGR == input_mgr finding.
"""
import re
import sys


def num(s):
    s = s.strip()
    return int(s, 16) if s.startswith("0x") else int(s)


def main(path):
    with open(path) as f:
        src = f.read()

    pat = re.compile(r"FUN_004179b0\(\s*([^,\s]+)\s*,\s*([^)\s]+)\s*\);")
    ops = []
    for m in pat.finditer(src):
        line = src.count("\n", 0, m.start()) + 1
        ops.append({
            "line": line,
            "dst": num(m.group(1)),
            "src": num(m.group(2)),
        })

    # Sanity: every src/dst within the 909-entry pool, dst != src.
    for o in ops:
        assert 0 < o["dst"] < 909, o
        assert 0 < o["src"] < 909, o
        assert o["dst"] != o["src"], o

    src_count = len(set(o["src"] for o in ops))
    dst_count = len(set(o["dst"] for o in ops))

    print("/* ─── FUN_0057ca40 — group-3 SS_MGR slot-clone calls ────────────")
    print(" *")
    print(" * 94 FUN_004179b0(dst_pool_idx, src_pool_idx) calls in retail")
    print(" * issue order — the SS_MGR singleton slot-clone subset that the")
    print(" * 2026-05-24 partial port left deferred.  See")
    print(" * docs/findings/0057ca40-rabbit-hole.md §3 for the call shape and")
    print(" * §7 for the SS_MGR == input_mgr (at 0x008a6b60) finding.")
    print(" *")
    print(f" * Distinct sources: {src_count}.  Distinct destinations: {dst_count}.")
    print(" *")
    print(" * Pool indices index the unified 909-entry pool (see")
    print(" * ar_pool_get_slot in asset_register.c).  pool[i] (1..12) → ramp")
    print(" * slots, pool[i] (13..908) → main slots.  Re-run this extractor")
    print(" * after re-exporting the decomp to catch drift. */")
    print("struct ar_group3_clone {")
    print("    uint16_t  dst_idx;    /* pool index — destination slot */")
    print("    uint16_t  src_idx;    /* pool index — source slot */")
    print("};")
    print("")
    print("static const struct ar_group3_clone group3_clones[] = {")
    print("    /*  dst,    src      (retail issue order; 57ca40.c line) */")
    for o in ops:
        print("    { 0x%03x, 0x%03x },  /* L%d */" % (o["dst"], o["src"], o["line"]))
    print("};")
    print(f"#define GROUP3_CLONES_COUNT  {len(ops)}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: 57ca40_clone_table.py <path-to-57ca40.c>", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1])
