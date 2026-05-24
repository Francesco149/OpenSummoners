#!/usr/bin/env python3
"""
Walk FUN_0057ca40's decomp and confirm that every ar_info_entry pool
write at retail BSS `0x8a8440 + i*4` corresponds to a sprite slot
produced (declared or cloned) at retail BSS `0x8a760c + i*4`.

This is the verification tool behind §2 of
`docs/findings/0057ca40-rabbit-hole.md`: it walks the 466 info-entry
references inside the function, groups them by the nearest preceding
slot decl / clone target, and reports how many match the implicit
`pool[i] == slot[i]` model.

Re-run after re-exporting the decomp (e.g. via
tools/ghidra-tag-and-export.sh) to catch drift — any nonzero "orphan
pool write" count means a new write pattern surfaced that the model
doesn't cover.

Usage:
    python3 tools/extract/57ca40_pool_map.py \\
        docs/decompiled/by-address/57ca40.c

See also:
- tools/extract/57ca40_sprite_table.py — slot-register subset extractor
  (the "what" we already ported).
- docs/findings/0057ca40-rabbit-hole.md §2 — the prose version of this
  tool's findings.
"""
import re
import sys
from collections import Counter

SPRITE_BASE = 0x8a760c
INFO_BASE = 0x8a8440


def parse_num(s):
    s = s.strip()
    return int(s, 16) if s.startswith("0x") else int(s)


def main(path):
    with open(path) as f:
        src = f.read()

    # Event detectors.  Each entry: (compiled regex, kind label, addr
    # capture-group or 0 if not address-bearing).
    patterns = [
        (re.compile(r"paVar1 = (DAT_008a[0-9a-f]+);"), "slot_inline", 1),
        (re.compile(r"ar_sprite_slot::FUN_005748c0\(\s*(DAT_008a[0-9a-f]+)"), "slot_helper", 1),
        # NB: source-level offsets +4 (byte-typed DAT) and +2 (short-typed DAT)
        # are both byte +4 in disasm — see rabbit-hole §2 "Ghidra rendering gotcha".
        (re.compile(r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 4\) = (?!\*)"), "info_flag_byte", 1),
        (re.compile(r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 2\) = (?!\*)"), "info_flag_short", 1),
        (re.compile(r"\*(DAT_008a8[0-9a-f]+) = (?!\*)"), "info_marker", 1),
        # Source +8 (byte-typed) or +4 (short-typed) = byte +8 (data ptr).
        (re.compile(r"\*\(undefined \*\*\)\((DAT_008a8[0-9a-f]+) \+ 8\) = "), "info_data_byte", 1),
        # SS_MGR slot clone — addr-free, payload = (dst, src) tuple.
        (re.compile(r"FUN_004179b0\(([^,]+),([^)]+)\);"), "clone_ss", 0),
        (re.compile(r"FUN_00582b80\((DAT_008a[0-9a-f]+)\);"), "clone_inline", 1),
        (re.compile(r"FUN_00582d00\(\);"), "clear_entry", 0),
        (re.compile(r"\*(DAT_008a8[0-9a-f]+) = \*(DAT_008a8[0-9a-f]+);"), "copy_marker_from", 1),
        (re.compile(r"\*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 2\) = \*\(undefined4 \*\)\((DAT_008a8[0-9a-f]+) \+ 2\);"), "copy_flag_from", 1),
    ]

    events = []
    for pat, kind, addr_grp in patterns:
        for m in pat.finditer(src):
            pos = m.start()
            line = src.count("\n", 0, pos) + 1
            if addr_grp:
                sym = m.group(addr_grp)
                addr = int(sym.split("_")[-1], 16)
                events.append((line, kind, addr, pos))
            elif kind == "clone_ss":
                events.append((line, kind, (parse_num(m.group(1)), parse_num(m.group(2))), pos))
            else:
                events.append((line, kind, None, pos))

    events.sort(key=lambda e: e[3])

    # Cluster events by slot decl: a cluster is everything from one slot decl
    # to the next.
    clusters = []
    cur = None
    for line, kind, payload, pos in events:
        if kind in ("slot_inline", "slot_helper"):
            if cur is not None:
                clusters.append(cur)
            cur = {
                "start_line": line,
                "slot_kind": kind,
                "slot_addr": payload,
                "slot_idx": (payload - SPRITE_BASE) // 4,
                "events": [],
            }
        elif cur is not None:
            cur["events"].append((line, kind, payload))
    if cur is not None:
        clusters.append(cur)

    # Pass 1: which slot indices does this function PRODUCE (decl or clone target)?
    produced_at = {}
    for c in clusters:
        produced_at.setdefault(c["slot_idx"], c["start_line"])
        for line, kind, payload in c["events"]:
            if kind == "clone_ss":
                dst, _ = payload
                produced_at.setdefault(dst, line)
            elif kind == "clone_inline":
                produced_at.setdefault((payload - SPRITE_BASE) // 4, line)

    # Pass 2: do all info-entry writes land at pool[i] where slot[i] is produced?
    pool_writes = []
    for c in clusters:
        for line, kind, payload in c["events"]:
            if kind.startswith("info_") or kind.startswith("copy_"):
                if payload is None:
                    continue
                pool_idx = (payload - INFO_BASE) // 4
                pool_writes.append((line, kind, pool_idx))

    orphans = [(line, kind, pool_idx) for (line, kind, pool_idx) in pool_writes
               if pool_idx not in produced_at]
    matches_first = 0
    deviations = 0
    for c in clusters:
        for line, kind, payload in c["events"]:
            if kind.startswith("info_") and payload is not None:
                pool_idx = (payload - INFO_BASE) // 4
                if pool_idx == c["slot_idx"]:
                    matches_first += 1
                else:
                    deviations += 1
                break

    print(f"FUN_0057ca40 pool-index audit (input: {path})")
    print(f"  slot clusters: {len(clusters)}")
    print(f"  pool writes (across all clusters): {len(pool_writes)}")
    print(f"  pool writes with no matching slot decl/clone: {len(orphans)}")
    print(f"  cluster FIRST info-write matches slot index: {matches_first}")
    print(f"    (deviations = {deviations}; these are clusters where")
    print(f"     a clone primitive runs before the slot's own info-write)")

    kind_counts = Counter(kind for (_, kind, _) in pool_writes)
    clone_ss = sum(1 for c in clusters for (_, k, _) in c["events"] if k == "clone_ss")
    clone_inline = sum(1 for c in clusters for (_, k, _) in c["events"] if k == "clone_inline")
    clear_entry = sum(1 for c in clusters for (_, k, _) in c["events"] if k == "clear_entry")
    print()
    print(f"  Event kind totals:")
    for k, n in sorted(kind_counts.items(), key=lambda x: -x[1]):
        print(f"    {k}: {n}")
    print(f"    clone_ss (FUN_004179b0): {clone_ss}")
    print(f"    clone_inline (FUN_00582b80): {clone_inline}")
    print(f"    clear_entry (FUN_00582d00): {clear_entry}")

    if orphans:
        print()
        print("  ORPHANS (pool writes to indices the function doesn't produce):")
        for line, kind, idx in orphans[:20]:
            print(f"    line {line}: pool[{idx}] via {kind}")
        return 1
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
