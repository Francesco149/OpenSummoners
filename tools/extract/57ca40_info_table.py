#!/usr/bin/env python3
"""
Extract the parallel-info-table write stream from FUN_0057ca40's
Ghidra decomp.  Used to (re)generate the body of
src/asset_register.c::group3_info_events[] in the 4th pass of the
FUN_0057ca40 port (`ar_register_group3_sprites`).

Re-run after re-exporting the decomp (e.g. via
tools/ghidra-tag-and-export.sh) and diff against the checked-in
table to spot drift.

Usage:
    python3 tools/extract/57ca40_info_table.py \\
        docs/decompiled/by-address/57ca40.c

Emits C source to stdout: enum + table + walker.

Pool indexing:
  pool[i] lives at retail BSS `0x8a8440 + i*4`.  See
  `docs/findings/0057ca40-rabbit-hole.md` §2 + §5 — the audit script
  `57ca40_pool_map.py` proved 0 orphans across all 443 writes (the 4
  short-typed data writes this extractor catches are not in the
  audit's counts but live inside captured clusters).

Event kinds detected:
  MARKER_SET    *DAT_X = N                                — pool[i].marker = N
  FLAG_SET      *(undefined4*)(DAT_X + {4|2}) = N         — pool[i].flag   = N
  DATA_SET      *(undefined**)(DAT_X + {8|4}) = &DAT_Y    — pool[i].data   = &PE_DAT_Y
  MARKER_COPY   *DAT_X = *DAT_Y                           — pool[i].marker = pool[j].marker
  FLAG_COPY     *(undefined4*)(DAT_X+2) = *(undefined4*)(DAT_Y+2)  — pool[i].flag = pool[j].flag
  STRUCT_COPY   `puVar7 = DAT_X;\\n  for (iVar5 = 5; ...)` — pool[i] = pool[j] (20 B struct copy)

Source-order issue is preserved: copies depend on prior writes, so
ordering matters.  The C walker applies events sequentially.

See also:
- tools/extract/57ca40_sprite_table.py — slot-register subset
  (`group3_sprites[]`) that this complements.
- tools/extract/57ca40_pool_map.py — verification (audits the model).
- docs/findings/0057ca40-rabbit-hole.md §2/§4/§5 — prose version.
"""
import re
import sys

INFO_BASE = 0x8a8440


def num(s):
    s = s.strip()
    return int(s, 16) if s.startswith("0x") else int(s)


def dat_idx(sym):
    addr = int(sym.split("_")[-1], 16)
    return (addr - INFO_BASE) // 4, addr


def main(path):
    with open(path) as f:
        src = f.read()

    # Each detector: (regex, kind, capture-helper).  capture-helper
    # returns the (pool_idx, payload_tuple) for that match, where the
    # payload_tuple's shape is consumed by the emitter below.
    events = []

    # MARKER_COPY: *DAT_X = *DAT_Y;
    # NB: must be matched BEFORE MARKER_SET so its `*DAT_X = *...` form
    # doesn't get eaten by the literal-marker pattern's `(?!\*)` lookahead
    # would already prevent that, but order helps determinism.
    pat = re.compile(r"\*(DAT_008a8[0-9a-f]+) = \*(DAT_008a8[0-9a-f]+);")
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        src_i, _ = dat_idx(m.group(2))
        events.append((m.start(), "MARKER_COPY", dst_i, src_i))

    # MARKER_SET: *DAT_X = N;
    pat = re.compile(r"\*(DAT_008a8[0-9a-f]+) = (?!\*)([0-9]+|0x[0-9a-f]+);")
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        events.append((m.start(), "MARKER_SET", dst_i, num(m.group(2))))

    # FLAG_COPY: *(undefined4*)(DAT_X + 2) = *(undefined4*)(DAT_Y + 2);
    pat = re.compile(
        r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 2\) = "
        r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 2\);"
    )
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        src_i, _ = dat_idx(m.group(2))
        events.append((m.start(), "FLAG_COPY", dst_i, src_i))

    # FLAG_SET: *(undefined4*)(DAT_X + 4) = N;   (byte-typed DAT)
    pat = re.compile(
        r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 4\) = (?!\*)([0-9]+|0x[0-9a-f]+);"
    )
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        events.append((m.start(), "FLAG_SET", dst_i, num(m.group(2))))

    # FLAG_SET: *(undefined4*)(DAT_X + 2) = N;   (short-typed DAT — still byte +4 in disasm)
    pat = re.compile(
        r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 2\) = (?!\*)([0-9]+|0x[0-9a-f]+);"
    )
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        events.append((m.start(), "FLAG_SET", dst_i, num(m.group(2))))

    # DATA_SET: *(undefined**)(DAT_X + 8) = &DAT_Y;   (byte-typed)
    pat = re.compile(
        r"\*\(undefined \*\*\)\((DAT_008a8[0-9a-f]+) \+ 8\) = &(DAT_00[0-9a-f]+);"
    )
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        pe_addr = int(m.group(2).split("_")[-1], 16)
        events.append((m.start(), "DATA_SET", dst_i, pe_addr))

    # DATA_SET: *(undefined**)(DAT_X + 4) = &DAT_Y;   (short-typed — still byte +8 in disasm)
    pat = re.compile(
        r"\*\(undefined \*\*\)\((DAT_008a8[0-9a-f]+) \+ 4\) = &(DAT_00[0-9a-f]+);"
    )
    for m in pat.finditer(src):
        dst_i, _ = dat_idx(m.group(1))
        pe_addr = int(m.group(2).split("_")[-1], 16)
        events.append((m.start(), "DATA_SET", dst_i, pe_addr))

    # STRUCT_COPY: `puVar7 = DAT_X;\n  for (iVar5 = 5; ...)` loop.
    # The src side is the puVar6 init right above it.  Anchor on the
    # dst init and the surrounding 5-iter loop; src extracted by
    # peeking at the preceding `puVar6 = DAT_Y` line.
    pat = re.compile(
        r"puVar6 = (DAT_008a8[0-9a-f]+);\s*\n"
        r"\s*puVar7 = (DAT_008a8[0-9a-f]+);\s*\n"
        r"\s*for \(iVar5 = 5"
    )
    for m in pat.finditer(src):
        src_i, _ = dat_idx(m.group(1))
        dst_i, _ = dat_idx(m.group(2))
        events.append((m.start(), "STRUCT_COPY", dst_i, src_i))

    events.sort(key=lambda e: e[0])

    # --- Quick sanity report on stderr ---
    from collections import Counter
    kc = Counter(e[1] for e in events)
    print(f"// extractor caught {len(events)} info-entry events:", file=sys.stderr)
    for k, n in sorted(kc.items(), key=lambda x: -x[1]):
        print(f"//   {k:14s} = {n}", file=sys.stderr)

    # --- Distinct PE-rdata pointers — used in tests ---
    distinct_data = sorted(set(e[3] for e in events if e[1] == "DATA_SET"))
    print(f"// distinct DATA_SET pointers: {len(distinct_data)}", file=sys.stderr)

    # --- Emit C ---
    print("/* ─── FUN_0057ca40 — info-entry pool writes (4th pass) ────────────")
    print(" *")
    print(f" * {len(events)} info-entry events in retail issue order — the")
    print(" * `g_ar_info_table[i]` writes that follow each sprite slot in")
    print(" * FUN_0057ca40.  See `docs/findings/0057ca40-rabbit-hole.md` §2.")
    print(" *")
    print(" * Indexing: pool[i] lives at retail BSS `0x8a8440 + i*4`.")
    print(" *")
    print(" * Source order preserved (MARKER_COPY/FLAG_COPY/STRUCT_COPY events")
    print(" * depend on prior writes — walking in source order keeps the")
    print(" * copies' source pool entries already-written).")
    print(" *")
    print(" * DATA_SET payloads are retail PE rdata addresses (e.g.")
    print(" * 0x006752f8); the port stores them as opaque uintptr_t markers")
    print(" * — no consumer reads them as bytes yet.  When a consumer lands")
    print(" * (FUN_00586010-style palette draw with flag dispatch), these will")
    print(" * need extracted PE bytes; for now they're observability-only. */")
    print("")
    print("enum ar_info_event_kind {")
    print("    AR_INFO_EVT_MARKER_SET = 0,")
    print("    AR_INFO_EVT_FLAG_SET,")
    print("    AR_INFO_EVT_DATA_SET,")
    print("    AR_INFO_EVT_MARKER_COPY,")
    print("    AR_INFO_EVT_FLAG_COPY,")
    print("    AR_INFO_EVT_STRUCT_COPY,")
    print("};")
    print("")
    print("struct ar_info_event {")
    print("    uint8_t   kind;")
    print("    uint16_t  dst_idx;     /* destination pool index */")
    print("    uintptr_t payload;     /* SET: literal value | COPY/STRUCT_COPY: src pool index */")
    print("};")
    print("")
    print("static const struct ar_info_event group3_info_events[] = {")
    print("    /* kind                       dst   payload */")
    for pos, kind, dst, payload in events:
        line_no = src.count("\n", 0, pos) + 1
        kind_id = "AR_INFO_EVT_" + kind
        if kind == "DATA_SET":
            payload_s = f"0x{payload:08x}u"
        elif kind in ("MARKER_COPY", "FLAG_COPY", "STRUCT_COPY"):
            payload_s = f"{payload}u"
        else:
            payload_s = f"0x{payload:x}u"
        print(f"    {{ {kind_id:<28s}, {dst:4d}, {payload_s:>12s} }},  /* L{line_no} */")
    print("};")
    print(f"#define GROUP3_INFO_EVENTS_COUNT  {len(events)}")
    print("")
    print("void ar_apply_group3_info_events(void)")
    print("{")
    print("    for (size_t i = 0; i < GROUP3_INFO_EVENTS_COUNT; i++) {")
    print("        const struct ar_info_event *ev = &group3_info_events[i];")
    print("        ar_info_entry *dst = g_ar_info_table[ev->dst_idx];")
    print("        switch (ev->kind) {")
    print("        case AR_INFO_EVT_MARKER_SET:")
    print("            dst->marker = (uint16_t)ev->payload;")
    print("            break;")
    print("        case AR_INFO_EVT_FLAG_SET:")
    print("            dst->flag = (uint32_t)ev->payload;")
    print("            break;")
    print("        case AR_INFO_EVT_DATA_SET:")
    print("            dst->data = (const void *)ev->payload;")
    print("            break;")
    print("        case AR_INFO_EVT_MARKER_COPY:")
    print("            dst->marker = g_ar_info_table[ev->payload]->marker;")
    print("            break;")
    print("        case AR_INFO_EVT_FLAG_COPY:")
    print("            dst->flag = g_ar_info_table[ev->payload]->flag;")
    print("            break;")
    print("        case AR_INFO_EVT_STRUCT_COPY:")
    print("            *dst = *g_ar_info_table[ev->payload];")
    print("            break;")
    print("        }")
    print("    }")
    print("}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: 57ca40_info_table.py <path-to-57ca40.c>", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1])
