#!/usr/bin/env python3
"""map_sweep.py — coverage sweep of EVERY map resource vs the ported decoder.

Enumerates every `DATA` resource in the PE whose maphdr name (+0x34) is
`MSD_SOTES_MAPDATA` (the res_explorer detection, res_core.cpp:242), parses each
with the same FUN_00587970 mirror as map_data.py, and reports:

  - tile ids used per map vs the arm set ported in src/map_decode.c (the case
    labels are parsed straight out of the C so the report can't go stale);
  - object-layer type codes per FUN_0058d460 band vs the port's def tables
    (STRUCT_BANK_DEFS / TOWN_EFFECT_DEFS / TOWN_SPRITE_DEFS in actor_spawn.c),
    with DEVICE (0x557550) wholly unported;
  - codes outside every band (retail also knows a "Treasure" object type).

Usage:
    nix develop --command python3 tools/extract/map_sweep.py \
        vendor/unpacked/sotes.unpacked.exe [--json out.json] [--per-map]
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from map_data import decode  # the FUN_00587970 mirror

REPO = Path(__file__).resolve().parents[2]


# ── enumerate all DATA resources (map_data.py walks one id; we need them all) ─
def all_data_resources(data: bytes) -> dict[int, bytes]:
    e = struct.unpack_from("<I", data, 0x3C)[0]
    if data[e : e + 4] != b"PE\0\0":
        raise SystemExit("not a PE file")
    coff = e + 4
    nsec = struct.unpack_from("<H", data, coff + 2)[0]
    opt = coff + 20
    opt_sz = struct.unpack_from("<H", data, coff + 16)[0]
    magic = struct.unpack_from("<H", data, opt)[0]
    ddir = opt + (96 if magic == 0x10B else 112)
    rsrc_rva = struct.unpack_from("<I", data, ddir + 2 * 8)[0]
    if not rsrc_rva:
        return {}
    secs = []
    for i in range(nsec):
        o = opt + opt_sz + i * 40
        vaddr = struct.unpack_from("<I", data, o + 12)[0]
        vsize = struct.unpack_from("<I", data, o + 8)[0]
        rawptr = struct.unpack_from("<I", data, o + 20)[0]
        rawsize = struct.unpack_from("<I", data, o + 16)[0]
        secs.append((vaddr, vsize, rawptr, rawsize))

    def r2o(rva: int) -> int:
        for vaddr, vsize, rawptr, rawsize in secs:
            if vaddr <= rva < vaddr + max(vsize, rawsize):
                return rawptr + (rva - vaddr)
        raise SystemExit(f"rva {rva:#x} outside sections")

    base = r2o(rsrc_rva)

    def walk(off: int):
        nn, ni = struct.unpack_from("<HH", data, off + 12)
        out = []
        for i in range(nn + ni):
            nameid, o = struct.unpack_from("<II", data, off + 16 + i * 8)
            out.append((nameid, base + (o & 0x7FFFFFFF), bool(o & 0x80000000)))
        return out

    def name_of(rel: int) -> str:
        so = base + rel
        ln = struct.unpack_from("<H", data, so)[0]
        return data[so + 2 : so + 2 + ln * 2].decode("utf-16-le", "replace")

    out: dict[int, bytes] = {}
    for nameid, off, isdir in walk(base):
        tname = name_of(nameid & 0x7FFFFFFF) if (nameid & 0x80000000) else f"#{nameid}"
        if tname != "DATA" or not isdir:
            continue
        for idv, ioff, idir in walk(off):
            if (idv & 0x80000000) or not idir:
                continue
            for _lang, loff, _ld in walk(ioff):
                rva, size = struct.unpack_from("<II", data, loff)
                out[idv] = data[r2o(rva) : r2o(rva) + size]
                break
    return out


# ── the ported sets, parsed out of the C sources (never stale) ────────────────
def ported_tile_ids() -> set[int]:
    src = (REPO / "src/map_decode.c").read_text()
    # tile-id case labels: 3+ hex digits keeps out the shape-switch cases (0..0xc)
    ids = {int(m, 16) for m in re.findall(r"case (0x[0-9a-f]{3,}):", src)}
    # plus the MD_SIMPLE_ARMS table (one-base-tile arms)
    m = re.search(r"MD_SIMPLE_ARMS\[\]\s*=\s*\{(.*?)\n\};", src, re.S)
    if m:
        ids |= {int(v, 16) for v in re.findall(r"\{\s*(0x[0-9a-f]+),", m.group(1))}
    return ids


def def_table_codes(table_name: str) -> set[int]:
    src = (REPO / "src/actor_spawn.c").read_text()
    m = re.search(table_name + r"\[\]\s*=\s*\{(.*?)\n\};", src, re.S)
    if not m:
        return set()
    return {int(v, 16) for v in re.findall(r"\{\s*(0x[0-9a-f]+)u", m.group(1))}


BANDS = (
    ("EFFECT", 50000, 59999),
    ("STRUCTURE", 60000, 69999),
    ("CHARACTER", 70000, 79999),
    ("DEVICE", 80000, 89999),
)


def band_of(code: int) -> str:
    for name, lo, hi in BANDS:
        if lo <= code <= hi:
            return name
    return "OTHER"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe", help="the EXE holding the map DATA resources")
    ap.add_argument("--json", help="write the full report as JSON")
    ap.add_argument("--per-map", action="store_true", help="print per-map detail")
    args = ap.parse_args()

    data = Path(args.pe).read_bytes()
    res = all_data_resources(data)
    maps = {
        rid: blob
        for rid, blob in sorted(res.items())
        if len(blob) >= 0x68 and blob[0x34:0x45] == b"MSD_SOTES_MAPDATA"
    }
    print(f"# {args.pe}: {len(res)} DATA resources, {len(maps)} maps")

    ported = ported_tile_ids()
    defs = {
        "STRUCTURE": def_table_codes("STRUCT_BANK_DEFS"),
        "EFFECT": def_table_codes("TOWN_EFFECT_DEFS"),
        "CHARACTER": def_table_codes("TOWN_SPRITE_DEFS"),
        "DEVICE": set(),
    }
    print(f"# ported tile-id arms in src/map_decode.c: {len(ported)}")
    for k, v in defs.items():
        print(f"# {k} def-table codes in src/actor_spawn.c: {len(v)}")

    tile_total: Counter = Counter()          # id -> cell count (all maps)
    tile_maps: dict[int, set] = {}           # id -> {map ids}
    tile_shapes: dict[int, Counter] = {}     # id -> shape histogram
    obj_total: Counter = Counter()           # code -> placement count
    obj_maps: dict[int, set] = {}
    report = []

    for rid, blob in maps.items():
        info = decode(blob, verbose=False)
        assert info["consumed"] == info["size"], f"map {rid} did not parse exactly"
        d0, d1, d2 = info["dims"]
        cells = info["cells_blob"]
        ids: Counter = Counter()
        for off in range(0, len(cells), 0x1C):
            tid = struct.unpack_from("<I", cells, off + 4)[0]
            if tid:
                ids[tid] += 1
                sh = struct.unpack_from("<I", cells, off + 0x10)[0]
                tile_shapes.setdefault(tid, Counter())[sh] += 1
        codes: Counter = Counter()
        for (_h0, _na, _nb, _nc, _nd, _sub, lh) in info["layers"]:
            codes[struct.unpack_from("<I", lh, 0x10)[0]] += 1
        for tid, k in ids.items():
            tile_total[tid] += k
            tile_maps.setdefault(tid, set()).add(rid)
        for c, k in codes.items():
            obj_total[c] += k
            obj_maps.setdefault(c, set()).add(rid)
        unk = {t: k for t, k in ids.items() if t not in ported}
        report.append(
            {
                "id": rid,
                "name": info["name"],
                "dims": [d0, d1, d2],
                "cells": sum(ids.values()),
                "tile_ids": {f"{t:#x}": k for t, k in sorted(ids.items())},
                "unported_tile_ids": {f"{t:#x}": k for t, k in sorted(unk.items())},
                "obj_codes": {f"{c:#x}": k for c, k in sorted(codes.items())},
            }
        )
        if args.per_map:
            cov = 100.0 * (1 - sum(unk.values()) / max(1, sum(ids.values())))
            print(
                f"  map {rid:>5} {info['name']:<20} {d0:>3}x{d1:<3}x{d2} "
                f"cells {sum(ids.values()):>5}  ids {len(ids):>3} "
                f"unported {len(unk):>3}  cellcov {cov:5.1f}%"
            )

    # ── global tile-id coverage ──────────────────────────────────────────────
    unported = {t: k for t, k in tile_total.items() if t not in ported}
    cell_cov = 100.0 * (1 - sum(unported.values()) / max(1, sum(tile_total.values())))
    print(
        f"\n# GLOBAL tile ids: {len(tile_total)} distinct, {len(unported)} unported "
        f"({len(tile_total)-len(unported)} ported); cell coverage {cell_cov:.1f}% "
        f"({sum(tile_total.values())-sum(unported.values())}/{sum(tile_total.values())})"
    )
    print("# unported tile ids by total cell count:")
    for tid, k in sorted(unported.items(), key=lambda kv: -kv[1]):
        shapes = ",".join(str(s) for s in sorted(tile_shapes[tid]))
        m = sorted(tile_maps[tid])
        mtxt = " ".join(str(x) for x in m[:8]) + (" ..." if len(m) > 8 else "")
        print(f"    {tid:#08x} ({tid:>6})  x{k:<6} shapes[{shapes}]  maps: {mtxt}")

    # ── object-code coverage per band ────────────────────────────────────────
    print("\n# object codes per band (code, placements, in-def-table?):")
    for name, lo, hi in BANDS + (("OTHER", -1, -1),):
        codes = sorted(
            c for c in obj_total if (band_of(c) == name)
        )
        if not codes:
            continue
        known = defs.get(name, set())
        n_unk = sum(1 for c in codes if c not in known)
        print(f"  {name}: {len(codes)} distinct codes, {n_unk} not in the port's table")
        for c in codes:
            mark = "ok " if c in known else "MISS"
            m = sorted(obj_maps[c])
            mtxt = " ".join(str(x) for x in m[:6]) + (" ..." if len(m) > 6 else "")
            print(f"    [{mark}] {c:#07x} ({c:>5})  x{obj_total[c]:<4} maps: {mtxt}")

    if args.json:
        Path(args.json).write_text(
            json.dumps(
                {
                    "maps": report,
                    "ported_tile_ids": sorted(f"{t:#x}" for t in ported),
                    "unported_tile_ids": {
                        f"{t:#x}": k for t, k in sorted(unported.items())
                    },
                },
                indent=1,
            )
        )
        print(f"\n# wrote {args.json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
