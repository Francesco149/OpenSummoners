#!/usr/bin/env python3
"""
tools/extract/config_dat.py — Decoder for user/config.dat.

Fortune Summoners persists per-user settings (launcher preferences,
keybindings, volumes, …) in a single XOR-obfuscated file under
`user/config.dat`.  This tool decodes the body and dumps it as hex +
parsed dword pairs.  See `docs/formats/config-dat.md` for the spec.

The XOR key is the single byte 0x88 — confirmed by the abundance of
`88 88 88 88` runs in the encoded file (zero plaintext encodes to
0x88 0x88 0x88 0x88).  The engine reads the file via
`FUN_005a4770` (the 46 KB init function) and registers ~101 schema
fields via `FUN_005afb90`.

Usage:
    python3 tools/extract/config_dat.py vendor/original/user/config.dat
    python3 tools/extract/config_dat.py path/to/config.dat --raw
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

XOR_KEY = 0x88
HEADER_SIZE = 16


def decode(data: bytes) -> tuple[dict, bytes]:
    """Parse the header and return (header_dict, decoded_body)."""
    if len(data) < HEADER_SIZE:
        raise ValueError(f"file too short: {len(data)} bytes, need >= {HEADER_SIZE}")

    hdr_size, version, data_size, checksum = struct.unpack(
        "<IIII", data[:HEADER_SIZE])

    if hdr_size != HEADER_SIZE:
        raise ValueError(f"unexpected hdr_size={hdr_size}, expected {HEADER_SIZE}")

    body = data[HEADER_SIZE:HEADER_SIZE + data_size]
    if len(body) < data_size:
        raise ValueError(
            f"truncated body: {len(body)} bytes, header claims {data_size}")

    plaintext = bytes(b ^ XOR_KEY for b in body)
    header = {
        "hdr_size":  hdr_size,
        "version":   version,
        "data_size": data_size,
        "checksum":  checksum,
        "tail_size": len(data) - HEADER_SIZE - data_size,
    }
    return header, plaintext


def dump_hex(label: str, blob: bytes, base: int = 0, width: int = 16) -> None:
    print(f"--- {label} ({len(blob)} bytes) ---")
    for off in range(0, len(blob), width):
        row = blob[off:off + width]
        hexs = " ".join(f"{b:02x}" for b in row)
        ascii_ = "".join(chr(b) if 0x20 <= b < 0x7f else "." for b in row)
        print(f"{base+off:06x}  {hexs:<{width*3}}  {ascii_}")


def dump_dword_pairs(blob: bytes) -> None:
    """Treat the body as 8-byte (u32, u32) pairs and dump them.

    The first dword in the file looks like a magic / count word (single
    u32), so we skip it and parse the remaining bytes as pairs.
    """
    if len(blob) < 4:
        return
    magic, = struct.unpack("<I", blob[:4])
    print(f"--- magic / leading u32: {magic:#010x} ({magic}) ---")
    rest = blob[4:]
    pair_bytes = (len(rest) // 8) * 8
    n_pairs = pair_bytes // 8
    print(f"--- {n_pairs} (u32, u32) pairs follow ---")
    print(f"{'#':>4}  {'offset':>6}  {'key':>10}  {'val':>10}")
    for i in range(n_pairs):
        off = 4 + i * 8
        k, v = struct.unpack("<II", rest[i*8:i*8+8])
        print(f"{i:>4}  {HEADER_SIZE+off:>6x}  {k:>#10x}  {v:>#10x}")
    remainder = rest[pair_bytes:]
    if remainder:
        dump_hex("trailing bytes after pairs", remainder,
                 base=HEADER_SIZE + 4 + pair_bytes)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("path", type=Path, help="path to config.dat")
    p.add_argument("--raw", action="store_true",
                   help="dump raw plaintext bytes instead of parsed pairs")
    p.add_argument("--out", type=Path,
                   help="write decoded plaintext body to this file")
    args = p.parse_args()

    data = args.path.read_bytes()
    print(f"file: {args.path}  size: {len(data)} bytes")
    header, body = decode(data)
    print(f"header: hdr_size={header['hdr_size']} "
          f"version={header['version']:#x} "
          f"data_size={header['data_size']} "
          f"checksum={header['checksum']:#x} "
          f"tail_size={header['tail_size']}")
    print()

    if args.out:
        args.out.write_bytes(body)
        print(f"wrote decoded body: {args.out}")
        print()

    if args.raw:
        dump_hex("plaintext body", body, base=HEADER_SIZE)
    else:
        dump_dword_pairs(body)

    return 0


if __name__ == "__main__":
    sys.exit(main())
