#!/usr/bin/env python3
"""Dump the retail "Dramatist Info" table DAT_006b6ea8 from the user's sotes.exe.

The engine identifies a *named character* (a "dramatist") by a 32-bit HANDLE.
FUN_0041f200 (the EFFECT activator), when spawned with a non-zero handle and
code 0, looks the handle up in this table (the string "Get Dramatist Info" is
logged at 0x41f200:51) and uses the row's CODE field to drive its sprite-install
switch; the row's BANK field (+0x30) overrides the per-archetype default sheet
(passed as sVar17 -> FUN_00426d70(0, bank, 0)).

Row layout (0x34 bytes = 13 dwords), confirmed against the census banks/positions
(runs/cutscene-cast) and the dramatist NAME strings:
  +0x00  u32   handle (key; the list is 0-handle terminated)
  +0x04  u32   character code  (the 0x41f200 sprite-switch selector / archetype)
  +0x08  char[0x28]  display name (NUL-terminated ASCII)
  +0x30  u16   primary sprite bank (0 = loaded dynamically, e.g. the party leads)

This reads the user's own binary at runtime; only the derived RE facts
(handle/code/name/bank) are committed — never the asset bytes.  Pure stdlib PE
parse (no pefile), runs outside nix.

Usage: python3 tools/dump_dramatist_table.py [path-to-unpacked-exe]
"""
import struct
import sys

TABLE_VA = 0x6B6EA8
ROW = 0x34


def va_to_off(d, va):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    coff = pe + 4
    nsec = struct.unpack_from("<H", d, coff + 2)[0]
    optsz = struct.unpack_from("<H", d, coff + 16)[0]
    opt = coff + 20
    base = struct.unpack_from("<I", d, opt + 28)[0]
    sec = opt + optsz
    rva = va - base
    for i in range(nsec):
        o = sec + i * 40
        vsz, sva, rsz, roff = struct.unpack_from("<IIII", d, o + 8)
        if sva <= rva < sva + max(vsz, rsz):
            return roff + (rva - sva), base
    raise ValueError("VA 0x%x not in any section" % va)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "vendor/unpacked/sotes.unpacked.exe"
    d = open(path, "rb").read()
    off, base = va_to_off(d, TABLE_VA)
    print("# Dramatist table DAT_006b6ea8  (image base 0x%x, file off 0x%x)" % (base, off))
    print("# idx  handle      code     bank   name")
    rows = []
    for i in range(256):
        o = off + i * ROW
        handle = struct.unpack_from("<I", d, o)[0]
        if handle == 0:
            break
        code = struct.unpack_from("<I", d, o + 4)[0]
        bank = struct.unpack_from("<H", d, o + 0x30)[0]
        name = d[o + 8 : o + 8 + 0x28].split(b"\0")[0].decode("latin1")
        rows.append((i, handle, code, bank, name))
        print("[%3d] 0x%08x 0x%05x  0x%03x  %s" % (i, handle, code, bank, name))
    print("# %d rows (handle==0 terminator)" % len(rows))
    return rows


if __name__ == "__main__":
    main()
