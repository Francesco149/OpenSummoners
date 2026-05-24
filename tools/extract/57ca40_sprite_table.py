#!/usr/bin/env python3
"""
Extract the 233-entry sprite-slot register table from FUN_0057ca40's
Ghidra decomp.  Used to (re)generate the body of
src/asset_register.c::group3_sprites[] in the partial port of
FUN_0057ca40 (ar_register_group3_sprites).

Re-run after re-exporting the decomp (e.g. via
tools/ghidra-tag-and-export.sh) and diff the output against the
checked-in table to spot drift.

Usage:
    python3 tools/extract/57ca40_sprite_table.py \\
        docs/decompiled/by-address/57ca40.c

Emits C source to stdout: struct definition + table + iterator.

See docs/findings/0057ca40-rabbit-hole.md for the scope rationale —
this tool extracts only the slot-register subset (91 inlined + 142
helper-style calls), not the parallel-info-table writes, slot clones,
or FUN_00582b80 tail (those are deferred).
"""
import re
import sys


def num(s):
    s = s.strip()
    return int(s, 16) if s.startswith("0x") else int(s)


def main(path):
    with open(path) as f:
        src = f.read()

    ops = []

    # Inlined block: `paVar1 = DAT_X; FUN_00417b50(DAT_X); ...; f_38 = 0;`
    blk = re.compile(
        r"paVar1 = (DAT_008a[0-9a-f]+);\s*\n"
        r"\s*ar_sprite_slot::FUN_00417b50\(\1\);\s*\n"
        r"(?P<body>(?:.*?\n)*?)"
        r"\s*paVar1->f_38 = 0;\s*\n"
    )

    def fget(body, name):
        m = re.search(rf"paVar1->{name} = (.+?);", body)
        return num(m.group(1)) if m else None

    for m in blk.finditer(src):
        body = m.group("body")
        slot_sym = m.group(1)
        ops.append({
            "pos": m.start(),
            "idx": (int(slot_sym[-4:], 16) - 0x7640) // 4,
            "addr": int(slot_sym[-4:], 16),
            "rid": fget(body, "resource_id"),
            "w": fget(body, "width"),
            "h": fget(body, "height"),
            "ck": fget(body, "colorkey"),
            "sf": fget(body, "scale_flag"),
            "ty": fget(body, "type"),
        })

    # Helper call: `ar_sprite_slot::FUN_005748c0(DAT_X, param_1, param_3, ...)`
    helper = re.compile(
        r"ar_sprite_slot::FUN_005748c0\(\s*(DAT_008a[0-9a-f]+)\s*,\s*param_1\s*,\s*param_3\s*,"
        r"\s*(0x[0-9a-f]+|\d+)\s*,\s*(0x[0-9a-f]+|\d+)\s*,\s*(0x[0-9a-f]+|\d+)\s*,"
        r"\s*(0x[0-9a-f]+|\d+)\s*,\s*(0x[0-9a-f]+|\d+)\s*,\s*(0x[0-9a-f]+|\d+)\s*,\s*param_2\s*\);"
    )
    for m in helper.finditer(src):
        slot_sym = m.group(1)
        ops.append({
            "pos": m.start(),
            "idx": (int(slot_sym[-4:], 16) - 0x7640) // 4,
            "addr": int(slot_sym[-4:], 16),
            "rid": num(m.group(2)),
            "w": num(m.group(3)),
            "h": num(m.group(4)),
            "ck": num(m.group(5)),
            "sf": num(m.group(6)),
            "ty": num(m.group(7)),
        })

    ops.sort(key=lambda x: x["pos"])

    # Sanity check: distinct slot indices, no duplicates.
    idxs = [o["idx"] for o in ops]
    if len(set(idxs)) != len(idxs):
        print(f"WARNING: duplicate slot indices in extraction", file=sys.stderr)

    print("/* ─── FUN_0057ca40 — group-3 sprite-register batch (PARTIAL PORT) ──")
    print(" *")
    print(" * 233 sprite-slot register operations in retail issue order — the")
    print(" * cleanly-modelable subset of the 24884-byte function.  See header")
    print(" * docstring for what's deferred (parallel-table writes, slot clones,")
    print(" * FUN_00582b80 — collectively another ~480 operations that depend on")
    print(" * SS_MGR singleton modeling and the parallel-pool struct shape).")
    print(" *")
    print(" * Source order preserved verbatim so future call-trace audits match")
    print(" * retail's pos column; idx column is the sprite pool index */")
    print("struct ar_group3_entry {")
    print("    uint16_t  idx;        /* g_ar_sprite_slots[idx] */")
    print("    uint16_t  id;         /* PE resource id */")
    print("    uint16_t  width;")
    print("    uint16_t  height;")
    print("    uint32_t  colorkey;")
    print("    uint8_t   scale_flag;")
    print("    uint8_t   type;")
    print("};")
    print("")
    print("static const struct ar_group3_entry group3_sprites[] = {")
    print("    /*  idx,  id,     w,     h,     colorkey,  scale, type    (retail BSS addr) */")
    for o in ops:
        print("    { %4d, 0x%04x, 0x%-4x, 0x%-4x, 0x%-8x, %d, %d },  /* 0x008a%04x */" % (
            o["idx"], o["rid"], o["w"], o["h"], o["ck"], o["sf"], o["ty"], o["addr"]
        ))
    print("};")
    print(f"#define GROUP3_SPRITES_COUNT  {len(ops)}")
    print("")
    print("void ar_register_group3_sprites(void *zdd, uint16_t group, void *settings)")
    print("{")
    print("    for (size_t i = 0; i < GROUP3_SPRITES_COUNT; i++) {")
    print("        const struct ar_group3_entry *e = &group3_sprites[i];")
    print("        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],")
    print("            zdd, settings, e->id,")
    print("            e->width, e->height, e->colorkey,")
    print("            e->scale_flag, e->type, group);")
    print("    }")
    print("}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: 57ca40_sprite_table.py <path-to-57ca40.c>", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1])
