#!/usr/bin/env python3
"""map_data.py — extract + decode an in-game RUNTIME map-data resource.

The in-game engine's per-step setup (FUN_00586010) loads the active room's
visual map (tilemap + object layers) from a PE **DATA resource** keyed by the
room's *scene index* (ROOM registry dword[3]).  The load path is

    FUN_00586010 :690   FUN_00587970(module, room.scene)          # the parser
      FUN_00587970 :72   FUN_005b62a0(module, scene)              # the opener
        FUN_005b62a0       FindResourceA(module, scene & 0xffff, "DATA")
                           LoadResource + LockResource            # mode-1 read
      FUN_00587970         FUN_005b6340(dst, n)  x N              # sequential copy

`module` is `DAT_008a6e7c` — the **main EXE module handle** (one of the boot
handle slots 0x8a6e68..0x8a6e7c), NOT sotesd.dll.  So the map data lives in the
EXE's own `.rsrc`, and the opening town (map 0x3f2 -> room 210110, scene 1022)
is **DATA resource 1022** in the EXE (152,936 bytes).

This REFINES plan 3a (docs/findings/in-game-intro.md): plan 3a's res-probe
hooked only the *sprite* decoder `bs_decode_resource` (0x5b7800) and so never
saw this separate FindResource path — its "no per-map resource file, map layout
is compiled-in" conclusion is true only of the ROOM REGISTRY (the room graph /
names / scene indices, compiled into `.rdata` — see game_world_tables.py).  The
per-room *visual* map (tiles + object layers) IS a loaded resource, sourced
from the EXE and keyed by scene index.

Format (exactly what FUN_00587970 reads, sequentially, from offset 0):

    [0x00:0x04]  magic            dword (observed 0x30 in the EXE's maps)
    [0x04:0x34]  header           0x30 bytes (opaque block)
    [0x34:0x68]  maphdr           0x34 bytes:
                   +0x00 char[0x20]  name      (space-padded ASCII)
                   +0x20 dword       dim0      (observed 88  — width)
                   +0x24 dword       dim1      (observed 19  — height)
                   +0x28 dword       dim2      (observed 3   — depth/planes)
                   +0x2c dword       count     layer/object-entry count (86)
                   +0x30 dword       (tail)
    [0x68:...]   cells            dim2*dim1*dim0 cells * 0x1c bytes each
    then `count` layer entries, each:
                   0x3c-byte layer header, with sub-array counts at
                     +0x1c (stride 4), +0x20 (stride 0xc),
                     +0x24 (stride 0x100), +0x28 (stride 8)
                   immediately followed by those four sub-arrays.

A well-formed map consumes the resource EXACTLY (no trailing bytes); this tool
asserts that, which is what makes the decode trustworthy.

Usage:
    python3 tools/extract/map_data.py vendor/unpacked/sotes.unpacked.exe
    python3 tools/extract/map_data.py <pe> --id 1022 [--raw] [--out f.bin]
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# minimal PE .rsrc walker (no pefile dependency — same style as
# tools/extract/sotes_resources.py / game_world_tables.py)
# ---------------------------------------------------------------------------
def _find_data_resource(data: bytes, want_id: int) -> bytes | None:
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
        return None
    secs = []
    for i in range(nsec):
        o = opt + opt_sz + i * 40
        vaddr = struct.unpack_from("<I", data, o + 12)[0]
        vsize = struct.unpack_from("<I", data, o + 8)[0]
        rawptr = struct.unpack_from("<I", data, o + 20)[0]
        rawsize = struct.unpack_from("<I", data, o + 16)[0]
        secs.append((vaddr, vsize, rawptr, rawsize))

    def r2o(rva: int) -> int | None:
        for vaddr, vsize, rawptr, rawsize in secs:
            if vaddr <= rva < vaddr + max(vsize, rawsize):
                return rawptr + (rva - vaddr)
        return None

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

    for nameid, off, isdir in walk(base):
        tname = name_of(nameid & 0x7FFFFFFF) if (nameid & 0x80000000) else f"#{nameid}"
        if tname != "DATA" or not isdir:
            continue
        for idv, ioff, idir in walk(off):
            if (idv & 0x80000000) or (idv != want_id) or not idir:
                continue
            for _lang, loff, _ld in walk(ioff):
                rva, size = struct.unpack_from("<II", data, loff)
                fo = r2o(rva)
                return data[fo : fo + size]
    return None


# ---------------------------------------------------------------------------
# the FUN_00587970 decode (validation mirror of src/map_data.c)
# ---------------------------------------------------------------------------
def decode(blob: bytes, verbose: bool = True) -> dict:
    n = len(blob)
    pos = 0

    def rd(k: int) -> bytes:
        nonlocal pos
        if pos + k > n:
            raise SystemExit(f"overrun: need {k} at {pos}, have {n}")
        b = blob[pos : pos + k]
        pos += k
        return b

    dw = lambda buf, o: struct.unpack_from("<I", buf, o)[0]

    magic = struct.unpack_from("<I", rd(4), 0)[0]
    _hdr = rd(0x30)
    maphdr = rd(0x34)
    name = maphdr[0:0x20].rstrip(b" \0").decode("ascii", "replace")
    dim0, dim1, dim2 = dw(maphdr, 0x20), dw(maphdr, 0x24), dw(maphdr, 0x28)
    count = dw(maphdr, 0x2C)
    cells_len = dim0 * dim1 * dim2 * 0x1C
    rd(cells_len)  # the cell array

    layers = []
    for _i in range(count):
        lh = rd(0x3C)
        na, nb, nc, nd = dw(lh, 0x1C), dw(lh, 0x20), dw(lh, 0x24), dw(lh, 0x28)
        sub = na * 4 + nb * 0xC + nc * 0x100 + nd * 8
        rd(sub)
        layers.append((dw(lh, 0), na, nb, nc, nd, sub))

    res = {
        "magic": magic,
        "name": name,
        "dims": (dim0, dim1, dim2),
        "count": count,
        "cells_len": cells_len,
        "consumed": pos,
        "size": n,
        "layers": layers,
    }
    if verbose:
        print(f"magic      {magic:#x}")
        print(f"name       {name!r}")
        print(f"dims       {dim0} x {dim1} x {dim2}  (product {dim0*dim1*dim2})")
        print(f"cell array {cells_len} bytes  (0x1c per cell)")
        print(f"layers     {count}")
        for i, (h0, na, nb, nc, nd, sub) in enumerate(layers):
            if i < 6 or i >= count - 2:
                print(
                    f"  layer[{i:3}] id={h0:#x}  +0x1c={na} +0x20={nb} "
                    f"+0x24={nc} +0x28={nd}  sub={sub}"
                )
            elif i == 6:
                print("  ...")
        tag = "EXACT" if pos == n else f"MISMATCH (+{n - pos})"
        print(f"consumed   {pos} / {n}  [{tag}]")
    return res


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("pe", help="PE file holding the map DATA resource (the EXE)")
    ap.add_argument("--id", type=int, default=1022, help="DATA resource id (default 1022)")
    ap.add_argument("--out", help="write the raw resource bytes to this path")
    ap.add_argument("--raw", action="store_true", help="hexdump the first 0x68 header bytes")
    args = ap.parse_args()

    data = Path(args.pe).read_bytes()
    blob = _find_data_resource(data, args.id)
    if blob is None:
        print(f"DATA resource {args.id} not found in {args.pe}", file=sys.stderr)
        return 1
    print(f"# {args.pe}  DATA {args.id}  ({len(blob)} bytes)")
    if args.out:
        Path(args.out).write_bytes(blob)
        print(f"# wrote {args.out}")
    if args.raw:
        head = blob[:0x68]
        for o in range(0, len(head), 16):
            row = head[o : o + 16]
            print(f"  {o:04x}  " + " ".join(f"{c:02x}" for c in row))
    info = decode(blob)
    return 0 if info["consumed"] == info["size"] else 3


if __name__ == "__main__":
    raise SystemExit(main())
