#!/usr/bin/env python3
"""ods_audit.py — READ-ONLY cross-reference auditor for the community spreadsheet.

The Fortune Summoners Fan Discord maintains *SotES Data Formats & Values* (an
``.ods``) documenting struct layouts, enums, addresses, and data tables for
``sotes.exe``.  It is a **cross-reference**, never committed to this repo and
never modified by this tool — we open it read-only and compare it against our own
independently-derived facts (``docs/decompiled/functions.csv`` + the port ledger),
so we can see at a glance:

  * which addresses the ods documents that our decompile agrees on (CONFIRM),
  * which it lacks that we have (so a proof under docs/proofs/ is worth publishing),
  * which fields it marks ``?`` / unknown (GAP — proof targets),

The spreadsheet is a LEAD: our function annotation + tracing is the source of
truth (see CLAUDE.md).  This tool just routes attention.

Usage (inside ``nix develop``):
  python3 tools/ods_audit.py ["../SotES Data Formats & Values.ods"]

The path defaults to the sibling location; if the file is absent the tool prints
a notice and exits 0 (the ods is intentionally not in the repo).
"""
from __future__ import annotations

import csv
import re
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

REPO = Path(__file__).resolve().parent.parent
FUNCTIONS_CSV = REPO / "docs" / "decompiled" / "functions.csv"
DEFAULT_ODS = REPO.parent / "SotES Data Formats & Values.ods"

TB = "{urn:oasis:names:tc:opendocument:xmlns:table:1.0}"
TX = "{urn:oasis:names:tc:opendocument:xmlns:text:1.0}"

# Struct-layout sheets: their header row is RVA + +0xNN offset columns, then a
# type row, a label row, then the data rows.  A '?' label = a documented unknown.
STRUCT_SHEETS = ("Items", "Areas", "Locations", "Actors", "Characters",
                 "Abilities", "Messages", "Mugshot Data")


def cell_text(c) -> str:
    return "".join(p.text or "" for p in c.iter(TX + "p"))


def read_sheet(tbl, max_rows: int = 6000) -> list[list[str]]:
    """Rows of a table, expanding column repeats but NOT blank-row runs (ODS
    pads sheets with a huge trailing repeated-blank row we must not expand)."""
    out: list[list[str]] = []
    for r in tbl.findall(TB + "table-row"):
        cells: list[str] = []
        for c in r.findall(TB + "table-cell"):
            rep = int(c.get(TB + "number-columns-repeated", "1"))
            cells.extend([cell_text(c)] * min(rep, 16))
        while cells and not cells[-1].strip():
            cells.pop()
        out.append(cells)
        if len(out) >= max_rows:
            break
    # drop the trailing all-blank tail
    while out and not any(x.strip() for x in out[-1]):
        out.pop()
    return out


def load_ods(path: Path) -> dict[str, list[list[str]]]:
    root = ET.fromstring(zipfile.ZipFile(path).read("content.xml"))  # read-only
    return {tbl.get(TB + "name"): read_sheet(tbl)
            for tbl in root.iter(TB + "table")}


def load_functions() -> dict[int, str]:
    funcs: dict[int, str] = {}
    if not FUNCTIONS_CSV.exists():
        return funcs
    with FUNCTIONS_CSV.open() as fh:
        for row in csv.DictReader(fh):
            try:
                funcs[int(row["entry"], 16)] = row.get("name", "")
            except (KeyError, ValueError):
                continue
    return funcs


HEX_RE = re.compile(r"^[0-9A-Fa-f]{5,8}$")


def audit_addresses(sheet, funcs) -> None:
    """The Addresses sheet is a Section/Address/End map of every function range.
    Cross-tab its .text entries against our decompile's function entry VAs."""
    ods_text = set()
    for row in sheet[1:]:
        if len(row) >= 2 and row[0].strip() == ".text":
            a = row[1].strip()
            if HEX_RE.match(a):
                ods_text.add(int(a, 16))
    ours = set(funcs)
    both = ods_text & ours
    ods_only = ods_text - ours
    ours_only = ours - ods_text
    print(f"\n## Addresses ↔ functions.csv")
    print(f"  ods .text entries:     {len(ods_text)}")
    print(f"  our function entries:  {len(ours)}")
    print(f"  CONFIRM (both agree):  {len(both)}")
    print(f"  ods-only (not a fn entry in our decompile): {len(ods_only)}"
          + (f"  e.g. {[f'0x{v:x}' for v in sorted(ods_only)[:6]]}" if ods_only else ""))
    print(f"  ours-only (we have, ods's range map omits): {len(ours_only)}"
          + (f"  e.g. {[f'0x{v:x}' for v in sorted(ours_only)[:6]]}" if ours_only else ""))


def audit_structs(sheets) -> None:
    """For each struct-layout sheet, report the documented offsets and flag the
    fields the ods marks '?' (or leaves unlabeled) — the GAP / proof targets."""
    print(f"\n## Struct sheets — documented fields + gaps")
    for name in STRUCT_SHEETS:
        sheet = sheets.get(name)
        if not sheet:
            continue
        # header = offsets row (starts with 'Relative Virtual Address'); the
        # label row is the first row after the type row whose [0] is 'ID'/'_'.
        offs = next((r for r in sheet if r and "Virtual Address" in r[0]), None)
        labels = next((r for r in sheet if r and r[0].strip() in ("ID",)), None)
        if not offs:
            continue
        ncol = len(offs)
        gaps = []
        if labels:
            for i in range(1, min(ncol, len(labels))):
                lab = labels[i].strip()
                if lab in ("?", "") or lab.endswith("?"):
                    gaps.append(f"{offs[i].strip()}={lab or '(unlabeled)'}")
        nrows = sum(1 for r in sheet if r and HEX_RE.match(r[0].strip() or "x"))
        print(f"  {name:14s} fields={ncol-1:2d}  rows≈{nrows:3d}  "
              f"gaps: {', '.join(gaps) if gaps else 'none flagged'}")


def audit_struct_bases(sheets) -> None:
    """The Structs sheet header carries the table base addresses (little-endian
    byte strings, e.g. 'D0 0C 2C 00' = 0x2C0CD0)."""
    sheet = sheets.get("Structs")
    if not sheet:
        return
    print(f"\n## Data-table base addresses (Structs sheet)")
    le = re.compile(r"^([0-9A-Fa-f]{2} ){3}[0-9A-Fa-f]{2}$")
    for row in sheet[:4]:
        for i, c in enumerate(row):
            if le.match(c.strip()):
                b = bytes(int(x, 16) for x in c.split())
                va = int.from_bytes(b, "little")
                label = row[i - 1].strip() if i else ""
                print(f"  {label:18s} 0x{va:08x}")


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_ODS
    if not path.exists():
        print(f"ods not found at {path} — it is intentionally NOT in the repo "
              f"(read-only cross-reference). Pass its path as arg 1.")
        return 0
    print(f"# ods audit — {path.name} (READ-ONLY)")
    sheets = load_ods(path)
    print(f"sheets: {', '.join(sheets)}")
    funcs = load_functions()
    if "Addresses" in sheets:
        audit_addresses(sheets["Addresses"], funcs)
    audit_struct_bases(sheets)
    audit_structs(sheets)
    print("\n(Leads only — verify at the byte level before a port depends on "
          "anything. Proofs we can 100%-confirm go in docs/proofs/.)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
