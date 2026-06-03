#!/usr/bin/env python3
"""game_world_tables.py — decode the in-game engine's two compiled-in world
tables out of the unpacked retail EXE.

These are the static map/room database the in-game engine (FUN_0059f2c0, the
per-map run loop reached via 0x59ec30(0,0,0x3f2)) copies into its world object
on entry.  Plan 3a (docs/findings/in-game-intro.md) established that map layout
is NOT a loaded resource — it is this compiled-in data.  FUN_0059f2c0:122-144
sets `scene[4][0] = &DAT_00693848` (the AREA table) and copies every entry of
`&DAT_006940c8` (the ROOM registry, 0x150-byte stride, zero-terminated) into
`scene[4][1..]`, cross-referencing the two via FUN_00585000.

  AREA table  @ 0x00693848  — 0x40-byte stride, zero-terminated (dword0==0)
    +0x00 dword  id           (area key; matched by ROOM.dword1)
    +0x04 char[] name         (ASCII English area name, e.g. "Town of Tolkien")
    +0x2c dword  A            (FUN_00585000 -> room[0x44])
    +0x30 dword  B            (-> room[0x45])
    +0x34 dword  C            (-> room[0x43])
    +0x38 dword  D            (-> room[0x50])
    +0x3c u16    E            (-> room[0x51])
    +0x3e u16    F            (-> room[0x146])

  ROOM registry @ 0x006940c8 — 0x150-byte (0x54-dword) stride, zero-terminated
    [0] is a header sentinel (dword0 = 0x000f423f = 1000001, rest zero).
    Real rooms run [1..N-1]; the table ends at the first entry with dword0==0.
    +0x00  dword  id          packed room id (area*1000 + 100 + 10*k style)
    +0x04  dword  area        area key -> AREA table id (English name)
    +0x0c  dword  scene       sequential scene/record index (1002, 1004, ...)
    +0x1c  dword  d7          (link/flag)
    +0x20  dword  parent      previous/parent room id (a ROOM.id)
    +0x24  dword  d9          (ordinal within parent)
    +0x118 char[] sjis        Shift-JIS room name (cp932), e.g. the town's
                              first district "トルーキンの町 １丁目"

NOTE the opening map passed to the engine is 0x3f2 = 1010, which is NOT any
ROOM.scene value (they jump 1009 -> 1012): 0x3f2 is a separate scene-load id the
room loop (FUN_00561c90) resolves — a thread for the room-loop port, not here.

Usage:
    game_world_tables.py [EXE]            # full listing (default unpacked exe)
    game_world_tables.py [EXE] --raw N    # hex-dump ROOM entry N
"""
import struct
import sys

DEFAULT_EXE = "vendor/unpacked/sotes.unpacked.exe"
AREA_VA = 0x00693848
ROOM_VA = 0x006940C8
AREA_STRIDE = 0x40
ROOM_STRIDE = 0x150


def load_pe(path):
    f = open(path, "rb").read()
    e = struct.unpack_from("<I", f, 0x3C)[0]
    coff = e + 4
    nsec = struct.unpack_from("<H", f, coff + 2)[0]
    opt = coff + 20
    optsz = struct.unpack_from("<H", f, coff + 16)[0]
    imgbase = struct.unpack_from("<I", f, opt + 28)[0]
    sect = opt + optsz
    secs = []
    for i in range(nsec):
        o = sect + i * 40
        vsz, va, rawsz, raw = struct.unpack_from("<IIII", f, o + 8)
        secs.append((va, vsz, raw, rawsz))

    def va2off(va):
        rva = va - imgbase
        for sva, vsz, raw, rawsz in secs:
            if sva <= rva < sva + max(vsz, rawsz):
                return raw + (rva - sva)
        raise ValueError(f"VA {va:#x} not mapped")

    return f, va2off


def cstr(f, off):
    end = f.index(b"\0", off)
    return f[off:end]


def decode_area(f, va2off):
    base = va2off(AREA_VA)
    out = []
    i = 0
    while True:
        o = base + i * AREA_STRIDE
        d0 = struct.unpack_from("<I", f, o)[0]
        if d0 == 0:
            break
        name = cstr(f, o + 4).decode("latin1")
        a, b, c, d = struct.unpack_from("<IIII", f, o + 0x2C)
        e_lo, e_hi = struct.unpack_from("<HH", f, o + 0x3C)
        out.append(dict(idx=i, id=d0, name=name, A=a, B=b, C=c, D=d, E=e_lo, F=e_hi))
        i += 1
    return out


def decode_rooms(f, va2off):
    base = va2off(ROOM_VA)
    out = []
    i = 0
    while struct.unpack_from("<I", f, base + i * ROOM_STRIDE)[0] != 0:
        o = base + i * ROOM_STRIDE

        def w(k):
            return struct.unpack_from("<I", f, o + k * 4)[0]

        try:
            sjis = cstr(f, o + 0x118).decode("cp932")
        except UnicodeDecodeError:
            sjis = repr(cstr(f, o + 0x118))
        out.append(dict(idx=i, id=w(0), area=w(1), scene=w(3),
                        d7=w(7), parent=w(8), d9=w(9), sjis=sjis))
        i += 1
    return out


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    exe = args[0] if args else DEFAULT_EXE
    f, va2off = load_pe(exe)

    if "--raw" in argv:
        n = int(args[1]) if len(args) > 1 else int(argv[argv.index("--raw") + 1])
        base = va2off(ROOM_VA)
        o = base + n * ROOM_STRIDE
        print(f"=== ROOM entry {n} raw @ {ROOM_VA + n*ROOM_STRIDE:#x} ===")
        for line in range(0, ROOM_STRIDE, 16):
            b = f[o + line:o + line + 16]
            asc = "".join(chr(x) if 32 <= x < 127 else "." for x in b)
            print(f"+{line:#05x}  " + " ".join(f"{x:02x}" for x in b) + "  " + asc)
        return 0

    areas = decode_area(f, va2off)
    rooms = decode_rooms(f, va2off)

    print(f"=== AREA table @ {AREA_VA:#x} — {len(areas)} entries ===")
    for a in areas:
        print(f"  [{a['idx']:3}] id={a['id']:#6x} A={a['A']:2} B={a['B']} "
              f"C={a['C']:2} D={a['D']} E={a['E']:2} F={a['F']}  '{a['name']}'")

    by_area = {}
    for r in rooms:
        by_area.setdefault(r["area"], 0)
        by_area[r["area"]] += 1

    print(f"\n=== ROOM registry @ {ROOM_VA:#x} — {len(rooms)} entries "
          f"([0] header, {len(rooms)-1} rooms), {len(by_area)-(1 if 0 in by_area else 0)} areas ===")
    aname = {a["id"]: a["name"] for a in areas}
    for r in rooms:
        if r["idx"] == 0:
            print(f"  [  0] HEADER sentinel dword0={r['id']:#x}")
            continue
        nm = f"  '{r['sjis']}'" if r["sjis"] else ""
        an = aname.get(r["area"], "")
        print(f"  [{r['idx']:3}] id={r['id']:6} area={r['area']:3}({an[:18]:18}) "
              f"scene={r['scene']:5} parent={r['parent']:6} d9={r['d9']}{nm}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
