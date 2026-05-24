#!/usr/bin/env python3
"""
tools/extract/sotes_resources.py — Dump every PE resource from a
sotesd / sotesw / sotesp / sotes(unpacked).exe binary.

The companion DLLs hold all the game's assets in their `.rsrc` section
as integer-keyed resources of type `DATA` (plus a few system types
like RT_VERSION).  See `docs/findings/asset-loader.md` for the
discovery.  This script walks the PE resource directory directly (no
pefile dependency) and writes one file per resource entry to an
output directory, plus a manifest JSON for downstream tools.

Output layout:

    <out_dir>/
        manifest.json
        type=<TYPE>/<ID>.bin    ← one file per (type, id)
        type=<TYPE>/<ID>.bin
        ...

`<TYPE>` is the resource type — either the well-known PE type name
(RT_BITMAP, RT_VERSION, …) for standard types, or the user-defined
type string verbatim (e.g. `DATA`).

Usage:
    python3 tools/extract/sotes_resources.py vendor/original/sotesd.dll \
        --out runs/extract/sotesd

    python3 tools/extract/sotes_resources.py vendor/original/sotesw.dll \
        --out runs/extract/sotesw \
        --no-write  # just print the manifest, don't dump bytes
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


# ─── PE structure constants ───────────────────────────────────────────────

DOS_HDR_E_LFANEW = 0x3C
PE_SIG_LEN       = 4
FILE_HDR_LEN     = 20
OPT_HDR_OFFS_TO_DATA_DIRS_PE32  = 96
OPT_HDR_OFFS_TO_DATA_DIRS_PE32P = 112
DATA_DIR_RESOURCE_INDEX = 2  # IMAGE_DIRECTORY_ENTRY_RESOURCE

RES_DIR_HDR_LEN  = 16        # IMAGE_RESOURCE_DIRECTORY
RES_DIR_ENTRY_LEN = 8        # IMAGE_RESOURCE_DIRECTORY_ENTRY
RES_DATA_ENTRY_LEN = 16      # IMAGE_RESOURCE_DATA_ENTRY

# Standard PE resource type IDs (RT_*).
RT_NAMES = {
    1:  "RT_CURSOR",
    2:  "RT_BITMAP",
    3:  "RT_ICON",
    4:  "RT_MENU",
    5:  "RT_DIALOG",
    6:  "RT_STRING",
    7:  "RT_FONTDIR",
    8:  "RT_FONT",
    9:  "RT_ACCELERATOR",
    10: "RT_RCDATA",
    11: "RT_MESSAGETABLE",
    12: "RT_GROUP_CURSOR",
    14: "RT_GROUP_ICON",
    16: "RT_VERSION",
    17: "RT_DLGINCLUDE",
    19: "RT_PLUGPLAY",
    20: "RT_VXD",
    21: "RT_ANICURSOR",
    22: "RT_ANIICON",
    23: "RT_HTML",
    24: "RT_MANIFEST",
}


@dataclass
class Section:
    name:     str
    vaddr:    int
    vsize:    int
    file_off: int
    file_sz:  int

    def contains_rva(self, rva: int) -> bool:
        return self.vaddr <= rva < self.vaddr + max(self.vsize, self.file_sz)

    def rva_to_file_offset(self, rva: int) -> int:
        return self.file_off + (rva - self.vaddr)


@dataclass
class ResourceEntry:
    type_id:   int | None   # int when integer type; None when named
    type_name: str | None   # set when named
    name_id:   int | None
    name_str:  str | None
    lang_id:   int
    rva:       int
    size:      int
    codepage:  int

    @property
    def type_label(self) -> str:
        if self.type_name is not None:
            return self.type_name
        if self.type_id in RT_NAMES:
            return RT_NAMES[self.type_id]
        return f"TYPE_{self.type_id}"

    @property
    def name_label(self) -> str:
        if self.name_str is not None:
            return self.name_str
        return str(self.name_id)


# ─── PE parsing ───────────────────────────────────────────────────────────


def parse_pe_sections(data: bytes) -> tuple[list[Section], int, int]:
    """Return (sections, opt_hdr_off, num_sections).  Validates the DOS+PE
    signatures along the way and raises on anything weird."""
    if data[:2] != b"MZ":
        raise ValueError("not an MZ file")
    pe_off = struct.unpack_from("<I", data, DOS_HDR_E_LFANEW)[0]
    if data[pe_off:pe_off+PE_SIG_LEN] != b"PE\0\0":
        raise ValueError("PE signature missing")

    file_hdr = pe_off + PE_SIG_LEN
    machine, n_sections, _ts, _, _, opt_sz, _ = struct.unpack_from(
        "<HHIIIHH", data, file_hdr)
    opt_hdr = file_hdr + FILE_HDR_LEN
    magic = struct.unpack_from("<H", data, opt_hdr)[0]
    if magic == 0x10b:
        sec_table_off = opt_hdr + opt_sz
    elif magic == 0x20b:
        sec_table_off = opt_hdr + opt_sz
    else:
        raise ValueError(f"unknown optional-header magic {magic:#x}")

    sections: list[Section] = []
    for i in range(n_sections):
        ent = sec_table_off + i * 40
        name = data[ent:ent+8].rstrip(b"\0").decode("ascii", errors="replace")
        vsize, vaddr, file_sz, file_off = struct.unpack_from(
            "<IIII", data, ent + 8)
        sections.append(Section(name, vaddr, vsize, file_off, file_sz))

    return sections, opt_hdr, n_sections


def get_resource_dir_rva(data: bytes, opt_hdr: int) -> tuple[int, int]:
    """Return (rva, size) of the resource directory from the DataDirectory."""
    magic = struct.unpack_from("<H", data, opt_hdr)[0]
    dd_off = opt_hdr + (OPT_HDR_OFFS_TO_DATA_DIRS_PE32 if magic == 0x10b
                       else OPT_HDR_OFFS_TO_DATA_DIRS_PE32P)
    rva, size = struct.unpack_from("<II", data,
                                   dd_off + DATA_DIR_RESOURCE_INDEX * 8)
    return rva, size


def rva_to_offset(sections: list[Section], rva: int) -> int:
    for s in sections:
        if s.contains_rva(rva):
            return s.rva_to_file_offset(rva)
    raise ValueError(f"RVA {rva:#x} not in any section")


def read_resource_string(data: bytes, off: int) -> str:
    length = struct.unpack_from("<H", data, off)[0]
    return data[off+2:off+2+length*2].decode("utf-16le", errors="replace")


def walk_resource_directory(
    data:       bytes,
    sections:   list[Section],
    rsrc_rva:   int,
    dir_offset: int = 0,
    level:      int = 0,
    type_id:    int | None = None,
    type_name:  str | None = None,
    name_id:    int | None = None,
    name_str:   str | None = None,
) -> list[ResourceEntry]:
    """Recursive walk of the IMAGE_RESOURCE_DIRECTORY tree.

    Level convention (standard PE): 0=Type, 1=Name, 2=Language, data leaf
    at level 3.
    """
    rsrc_file_off = rva_to_offset(sections, rsrc_rva)
    dir_off = rsrc_file_off + dir_offset

    n_named, n_id = struct.unpack_from("<HH", data, dir_off + 12)
    n_entries = n_named + n_id
    entries: list[ResourceEntry] = []

    for i in range(n_entries):
        ent_off = dir_off + RES_DIR_HDR_LEN + i * RES_DIR_ENTRY_LEN
        name_field, data_field = struct.unpack_from("<II", data, ent_off)

        is_named = bool(name_field & 0x80000000)
        is_subdir = bool(data_field & 0x80000000)
        name_val   = name_field & 0x7FFFFFFF
        data_val   = data_field & 0x7FFFFFFF

        # Resolve name/id by level
        cur_type_id, cur_type_name = type_id, type_name
        cur_name_id, cur_name_str = name_id, name_str
        lang_id = 0

        if level == 0:
            if is_named:
                cur_type_name = read_resource_string(
                    data, rsrc_file_off + name_val)
                cur_type_id = None
            else:
                cur_type_id = name_val
                cur_type_name = None
        elif level == 1:
            if is_named:
                cur_name_str = read_resource_string(
                    data, rsrc_file_off + name_val)
                cur_name_id = None
            else:
                cur_name_id = name_val
                cur_name_str = None
        elif level == 2:
            lang_id = name_val
        else:
            raise ValueError("unexpected level > 2")

        if is_subdir:
            entries.extend(walk_resource_directory(
                data, sections, rsrc_rva,
                dir_offset=data_val, level=level + 1,
                type_id=cur_type_id, type_name=cur_type_name,
                name_id=cur_name_id, name_str=cur_name_str))
        else:
            # Leaf: IMAGE_RESOURCE_DATA_ENTRY at rsrc_file_off+data_val
            data_ent_off = rsrc_file_off + data_val
            data_rva, data_sz, codepage, _ = struct.unpack_from(
                "<IIII", data, data_ent_off)
            entries.append(ResourceEntry(
                type_id=cur_type_id,
                type_name=cur_type_name,
                name_id=cur_name_id,
                name_str=cur_name_str,
                lang_id=lang_id,
                rva=data_rva,
                size=data_sz,
                codepage=codepage,
            ))

    return entries


# ─── content sniffing ─────────────────────────────────────────────────────

MAGIC_SNIFF = [
    (b"BM",         "bitmap (BMP)"),
    (b"\x89PNG",    "PNG"),
    (b"\xff\xd8\xff", "JPEG"),
    (b"RIFF",       "RIFF (WAV/AVI)"),
    (b"OggS",       "OGG"),
    (b"\x30\x26\xb2\x75", "ASF/WMA/WMV"),
    (b"FORM",       "IFF"),
    (b"\x1f\x8b",   "gzip"),
    (b"PK\x03\x04", "ZIP"),
    (b"MThd",       "MIDI"),
]


def sniff_magic(buf: bytes) -> str:
    for prefix, label in MAGIC_SNIFF:
        if buf.startswith(prefix):
            return label
    if all(0x20 <= b < 0x7f or b in (0x09, 0x0a, 0x0d) for b in buf[:64]):
        return "ascii-ish"
    return "unknown"


# ─── main ─────────────────────────────────────────────────────────────────


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("path", type=Path, help="path to PE file with .rsrc")
    p.add_argument("--out", type=Path, default=None,
                   help="output directory; default: runs/extract/<basename>")
    p.add_argument("--no-write", action="store_true",
                   help="parse + report but don't write per-resource files")
    p.add_argument("--type-filter", default=None,
                   help="only include resources of this type label "
                        "(e.g. 'DATA', 'RT_VERSION')")
    args = p.parse_args()

    data = args.path.read_bytes()
    print(f"file: {args.path}  size: {len(data):,} bytes", file=sys.stderr)
    sections, opt_hdr, _ = parse_pe_sections(data)
    print(f"sections: {[s.name for s in sections]}", file=sys.stderr)
    rsrc_rva, rsrc_sz = get_resource_dir_rva(data, opt_hdr)
    print(f"resource directory: rva={rsrc_rva:#x} size={rsrc_sz:,}",
          file=sys.stderr)
    if rsrc_rva == 0:
        print("no resource directory", file=sys.stderr)
        return 0
    entries = walk_resource_directory(data, sections, rsrc_rva)
    print(f"entries: {len(entries)}", file=sys.stderr)

    out_dir = args.out or (
        Path(__file__).resolve().parent.parent.parent
        / "runs" / "extract" / args.path.stem)
    if not args.no_write:
        out_dir.mkdir(parents=True, exist_ok=True)

    manifest: list[dict[str, Any]] = []
    type_counts: dict[str, int] = {}
    for e in entries:
        if args.type_filter and e.type_label != args.type_filter:
            continue
        type_counts[e.type_label] = type_counts.get(e.type_label, 0) + 1
        file_off = rva_to_offset(sections, e.rva)
        blob = data[file_off:file_off + e.size]
        sha1 = hashlib.sha1(blob).hexdigest()
        magic = sniff_magic(blob)

        manifest_entry = {
            "type":     e.type_label,
            "name":     e.name_label,
            "lang":     e.lang_id,
            "rva":      e.rva,
            "size":     e.size,
            "codepage": e.codepage,
            "sha1":     sha1,
            "magic":    magic,
        }
        manifest.append(manifest_entry)

        if not args.no_write:
            type_dir = out_dir / f"type={e.type_label}"
            type_dir.mkdir(exist_ok=True)
            fname = f"{e.name_label}.bin"
            (type_dir / fname).write_bytes(blob)

    if not args.no_write:
        (out_dir / "manifest.json").write_text(
            json.dumps({"src": str(args.path), "entries": manifest}, indent=2))

    # Summary
    print(file=sys.stderr)
    print("=== type histogram ===", file=sys.stderr)
    for t, n in sorted(type_counts.items()):
        sz = sum(e["size"] for e in manifest if e["type"] == t)
        print(f"  {t:12s}  {n:>5d} entries  {sz:>14,} bytes", file=sys.stderr)
    print(file=sys.stderr)
    print(f"=== first 20 entries ===", file=sys.stderr)
    for ent in manifest[:20]:
        print(f"  {ent['type']:8s}  name={ent['name']:>6s}  "
              f"size={ent['size']:>9,}  magic={ent['magic']}",
              file=sys.stderr)
    if not args.no_write:
        print(f"\nwrote {len(manifest)} resources + manifest to {out_dir}",
              file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
