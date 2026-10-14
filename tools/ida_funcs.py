"""The engine function universe for the derived ledgers (gen_port_ledger / gen_frontier).

Source: the IDA 9.4 export `functions_ida.tsv` (EN-old; `$OSS_IDA_LOCAL/enold/`, default
/mnt/ephemeral/opensummoners/decompiled-ida) — va, size, name, callees.  Falls back to the
legacy Ghidra export (docs/decompiled/functions.csv + by-address/*.c) when the IDA table is
absent.  Never committed (gitignored decompile outputs)."""
import csv
import os
import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
IDA_TSV = Path(os.environ.get("OSS_IDA_LOCAL", "/mnt/ephemeral/opensummoners/decompiled-ida")) / "enold" / "functions_ida.tsv"
GHIDRA_CSV = REPO / "docs" / "decompiled" / "functions.csv"
GHIDRA_BY_ADDR = REPO / "docs" / "decompiled" / "by-address"
FUN_RE = re.compile(r"FUN_([0-9a-fA-F]{6,8})")


def source() -> str:
    return str(IDA_TSV) if IDA_TSV.exists() else str(GHIDRA_CSV)


def load_functions() -> dict[int, dict]:
    """{va: {name, size, thunk, callees:set[int]}}"""
    funcs: dict[int, dict] = {}
    if IDA_TSV.exists():
        with IDA_TSV.open() as fh:
            for r in csv.DictReader(fh, delimiter="\t"):
                va = int(r["va"], 16)
                funcs[va] = {"name": r["name"], "size": int(r["size"] or 0),
                             "thunk": r["name"].startswith("j_"),
                             "callees": {int(c, 16) for c in (r.get("callees") or "").split(",") if c}}
        return funcs
    with GHIDRA_CSV.open() as fh:
        for r in csv.DictReader(fh):
            try:
                va = int(r["entry"], 16)
            except (KeyError, ValueError):
                continue
            funcs[va] = {"name": r.get("name", ""), "size": int(r.get("size") or 0),
                         "thunk": r.get("is_thunk") == "true", "callees": set()}
    for f in GHIDRA_BY_ADDR.glob("*.c"):
        try:
            cva = int(f.stem, 16)
        except ValueError:
            continue
        if cva in funcs:
            funcs[cva]["callees"] = {int(m.group(1), 16) for m in FUN_RE.finditer(f.read_text(errors="replace"))} - {cva}
    return funcs
