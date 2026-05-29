#!/usr/bin/env python3
"""
tools/gen_engine_vas.py — derive the call-trace VA list from functions.csv.

The Frida agent's `call_trace` mode Interceptor.attach()es onEnter at every
VA in this list and emits one record per invocation.  We generate the
candidate set from the Ghidra export (docs/decompiled/functions.csv) and
filter out entries that are pointless or dangerous to hook:

  • is_thunk == true   — import thunks / jump stubs; hooking them is noise
                         (the real callee is what we care about) and some
                         are 2-byte jmp slots Frida can't trampoline safely.
  • size < MIN_SIZE    — sub-8-byte functions are usually padding/alignment
                         stubs or tail-call thunks; same hazard class.
  • DENYLIST           — hand-maintained VAs known to destabilize the engine
                         when hooked (populated by bisect_call_trace_vas.py
                         as crashers are found; starts empty).

Output (tools/frida/data/engine_vas.json):

    {
      "_generated_by": "tools/gen_engine_vas.py",
      "_note": "...",
      "count": <N>,
      "min_size": <MIN_SIZE>,
      "vas": [<int>, <int>, ...]          # sorted ascending, Ghidra VAs
    }

This is the *candidate* list.  The Frida-safe subset
(engine_vas_frida_safe.json) is produced by tools/bisect_call_trace_vas.py,
which boots retail with chunks of this list hooked and bisects out the ones
that crash.  Pass --safe-only to instead read an existing safe-list and
re-emit (rare; mostly for regenerating after a functions.csv refresh).

Usage:
    nix develop --command python3 tools/gen_engine_vas.py
    nix develop --command python3 tools/gen_engine_vas.py --min-size 16
"""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

ROOT          = Path(__file__).resolve().parent.parent
FUNCTIONS_CSV = ROOT / "docs" / "decompiled" / "functions.csv"
OUT_DEFAULT   = ROOT / "tools" / "frida" / "data" / "engine_vas.json"

MIN_SIZE = 8

# VAs known to crash / destabilize the engine when an Interceptor sits on
# their entry.  Populated incrementally by bisect_call_trace_vas.py — see
# its --emit-denylist output.  Keep entries commented with the symptom.
DENYLIST: set[int] = set()


def load_functions(path: Path) -> list[dict]:
    if not path.exists():
        raise SystemExit(f"{path} not found — run the Ghidra export first "
                         f"(tools/ghidra-tag-and-export.sh)")
    rows: list[dict] = []
    with path.open() as f:
        for row in csv.DictReader(f):
            rows.append(row)
    return rows


def select_vas(rows: list[dict], min_size: int) -> tuple[list[int], dict]:
    """Returns (sorted_vas, stats)."""
    kept: list[int] = []
    n_thunk = n_small = n_deny = n_badrow = 0
    for row in rows:
        try:
            va   = int(row["entry"], 16)
            size = int(row["size"])
        except (KeyError, ValueError):
            n_badrow += 1
            continue
        if str(row.get("is_thunk", "")).strip().lower() == "true":
            n_thunk += 1
            continue
        if size < min_size:
            n_small += 1
            continue
        if va in DENYLIST:
            n_deny += 1
            continue
        kept.append(va)
    kept.sort()
    stats = {
        "total_rows":     len(rows),
        "kept":           len(kept),
        "dropped_thunk":  n_thunk,
        "dropped_small":  n_small,
        "dropped_deny":   n_deny,
        "dropped_badrow": n_badrow,
    }
    return kept, stats


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--functions-csv", type=Path, default=FUNCTIONS_CSV)
    ap.add_argument("--out", type=Path, default=OUT_DEFAULT)
    ap.add_argument("--min-size", type=int, default=MIN_SIZE,
                    help=f"drop functions smaller than this many bytes "
                         f"(default {MIN_SIZE})")
    args = ap.parse_args(argv)

    rows = load_functions(args.functions_csv)
    vas, stats = select_vas(rows, args.min_size)

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps({
        "_generated_by": "tools/gen_engine_vas.py",
        "_note": ("Candidate call-trace VA list (Ghidra VAs). The "
                  "Frida-safe subset is engine_vas_frida_safe.json, "
                  "produced by tools/bisect_call_trace_vas.py."),
        "count":    len(vas),
        "min_size": args.min_size,
        "vas":      vas,
    }, indent=1) + "\n")

    print(f"wrote {len(vas)} VAs to {args.out}")
    print(f"  total functions.csv rows: {stats['total_rows']}")
    print(f"  dropped thunks:           {stats['dropped_thunk']}")
    print(f"  dropped < {args.min_size}B stubs:        {stats['dropped_small']}")
    print(f"  dropped denylisted:       {stats['dropped_deny']}")
    if stats["dropped_badrow"]:
        print(f"  skipped malformed rows:   {stats['dropped_badrow']}")
    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
