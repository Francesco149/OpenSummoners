#!/usr/bin/env python3
"""
Extract the 9 inline FUN_00582b80 slot-clone calls from FUN_0057ca40's
Ghidra decomp.  Used to (re)generate the body of
src/asset_register.c::group3_inline_clones[] in the final pass of the
FUN_0057ca40 port (`ar_register_group3_sprites`).

Each call is `FUN_00582b80(DAT_target);` invoked as a __thiscall on the
source slot — ECX holds the last value of `paVar1`, which Ghidra
renders as `paVar1 = DAT_source;` at the cluster head.  The call
clones the source slot's metadata + aux_buf into `DAT_target`.  See
docs/findings/0057ca40-rabbit-hole.md §4 for the cluster shape.

Unlike the SS_MGR variant (FUN_004179b0), FUN_00582b80 takes no
info-entry side effect; that side is captured in the 4th-pass info
events (extract/57ca40_info_table.py — MARKER_COPY/FLAG_COPY/DATA_SET
for the 4 early clusters, STRUCT_COPY for the 5 late ones).

Re-run after re-exporting the decomp (e.g. via
tools/ghidra-tag-and-export.sh) and diff the output against the
checked-in table to spot drift.

Usage:
    python3 tools/extract/57ca40_inline_clone_table.py \\
        docs/decompiled/by-address/57ca40.c

Emits C source to stdout: struct definition + table + iterator.

Pool indexing:
  pool[i] lives at retail BSS `0x8a760c + i*4` (see rabbit-hole §5).
"""
import re
import sys

SLOT_BASE = 0x8a760c


def dat_addr(sym):
    """DAT_008a7xxx → absolute address."""
    return int(sym.split("_")[-1], 16)


def pool_idx(sym):
    return (dat_addr(sym) - SLOT_BASE) // 4


def main(path):
    with open(path) as f:
        src = f.read()

    # Find every `paVar1 = DAT_008a7xxx;` and every
    # `FUN_00582b80(DAT_008a7xxx);` and stream-merge them in source
    # order.  For each FUN_00582b80, the source slot is the most
    # recent paVar1 assignment.
    pat_assign = re.compile(r"paVar1 = (DAT_008a7[0-9a-f]+);")
    pat_call   = re.compile(r"FUN_00582b80\(\s*(DAT_008a7[0-9a-f]+)\s*\);")

    events = []
    for m in pat_assign.finditer(src):
        events.append((m.start(), "assign", m.group(1)))
    for m in pat_call.finditer(src):
        events.append((m.start(), "call",   m.group(1)))
    events.sort(key=lambda e: e[0])

    ops = []
    last_src = None
    for pos, kind, sym in events:
        if kind == "assign":
            last_src = sym
        else:  # call
            assert last_src is not None, \
                f"FUN_00582b80 at byte {pos} with no preceding paVar1 assignment"
            line = src.count("\n", 0, pos) + 1
            ops.append({
                "line": line,
                "src": pool_idx(last_src),
                "dst": pool_idx(sym),
                "src_addr": dat_addr(last_src),
                "dst_addr": dat_addr(sym),
            })

    # Sanity: every src/dst within the 909-entry pool, dst != src.
    for o in ops:
        assert 0 < o["dst"] < 909, o
        assert 0 < o["src"] < 909, o
        assert o["dst"] != o["src"], o

    src_set = sorted(set(o["src"] for o in ops))
    dst_set = sorted(set(o["dst"] for o in ops))
    # The src/dst sets must be disjoint — otherwise the clone-apply
    # ordering becomes load-bearing (a dst cloned earlier shouldn't be
    # read as src later).
    assert not (set(src_set) & set(dst_set)), \
        f"src/dst overlap: {set(src_set) & set(dst_set)}"

    print("/* ─── FUN_0057ca40 — group-3 inline FUN_00582b80 slot-clones ─────")
    print(" *")
    print(f" * {len(ops)} FUN_00582b80(target_slot) calls in retail issue order.")
    print(" * Each call is a __thiscall on the source slot (ECX = the last")
    print(" * paVar1 in the decomp); it clones source metadata + aux_buf into")
    print(" * the target slot via the ported `ar_sprite_slot_clone` primitive.")
    print(" *")
    print(f" * Distinct sources: {len(src_set)}  (pool {src_set}).")
    print(f" * Distinct targets: {len(dst_set)}.")
    print(" * src/dst sets are disjoint (asserted by extractor) — apply order")
    print(" * is independent of the SS_MGR clone pass and the info-event pass.")
    print(" *")
    print(" * Info-entry side (zero + marker/flag copy + data ptr for the 4")
    print(" * early clusters; 20-byte STRUCT_COPY for the 5 late clusters) is")
    print(" * NOT captured here — those events live in group3_info_events[].")
    print(" *")
    print(" * Re-run this extractor after re-exporting the decomp to catch")
    print(" * drift; see docs/findings/0057ca40-rabbit-hole.md §4. */")
    print("struct ar_group3_inline_clone {")
    print("    uint16_t  dst_idx;    /* pool index — destination slot */")
    print("    uint16_t  src_idx;    /* pool index — source slot (template) */")
    print("};")
    print("")
    print("static const struct ar_group3_inline_clone group3_inline_clones[] = {")
    print("    /*  dst,    src      (retail issue order; 57ca40.c line) */")
    for o in ops:
        print("    { 0x%03x, 0x%03x },  /* L%d  0x%08x <- 0x%08x */" % (
            o["dst"], o["src"], o["line"], o["dst_addr"], o["src_addr"]))
    print("};")
    print(f"#define GROUP3_INLINE_CLONES_COUNT  {len(ops)}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: 57ca40_inline_clone_table.py <path-to-57ca40.c>",
              file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1])
