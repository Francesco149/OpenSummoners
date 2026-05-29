#!/usr/bin/env python3
"""Generate a derived port ledger for OpenSummoners.

The ledger maps every engine function (from the Ghidra export
``docs/decompiled/functions.csv`` — the decompile of the Steamless-unpacked
``sotes.exe``) to its port status, derived purely from artifacts already in the
tree, so it is regenerated, never hand-maintained, and cannot drift:

  status     source of truth
  ------     ---------------
  tested     the VA appears as FUN_<va> in a src/ file whose module has a
             matching tests/test_<stem>.c  (port + host-suite coverage)
  ported     the VA appears as FUN_<va> in a src/ comment/provenance header
             but the module has no host test yet
  unported   present in the engine but never referenced from src/

OpenSummoners ports (and the analysis headers in src/) carry the Ghidra
``FUN_<va>`` token in a provenance comment (e.g. ``app_pump.c — pure-logic port
of FUN_005b1030``).  That token is the single machine-readable signal this
generator keys on (mirrors the openrecet / OpenMare / OpenLords2 sibling
ledgers).

--- Denominator: drop-in, single binary ---

OpenSummoners is a **drop-in** for ``sotes.exe`` (PLAN.md §2-3): it links the
host's MSVC CRT the same way retail does, so the statically-linked CRT tail of
the function table is *never going to be ported* — it is linked.  We report
both the full count and an "engine-proper" subset count (functions below
``ENGINE_PROPER_HI``) so ``% touched`` is not crushed by the runtime library.

Unlike OpenMare (whose renderer is a separate SGL DLL), Fortune Summoners is a
**single binary** — the DirectDraw/DirectSound/DirectInput runtimes are system
DLLs linked at load, and the oversized ``sotesd.dll`` / ``sotesw.dll`` companions
are data archives, not code we decompile.  So there is no second-binary split:
any ``FUN_<va>`` in src/ that is not in ``functions.csv`` is a sub-helper label
or a typo, collected in ``orphan_refs``.

Outputs (all under docs/, all git-tracked — they contain no vendor bytes):
  docs/port-ledger.json   full per-VA map (machine-readable)
  docs/port-ledger.md     human-readable summary table + per-status VA lists
  docs/STATUS.md          60-second headline: counts, %, current phase, blocker

Run from the repo root:  python3 tools/gen_port_ledger.py
Add --check to fail (exit 3) if the on-disk ledger is stale (for a pre-commit
hook): it regenerates into memory and diffs against what's committed.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SRC = REPO / "src"
TESTS = REPO / "tests"
FUNCTIONS_CSV = REPO / "docs" / "decompiled" / "functions.csv"

LEDGER_JSON = REPO / "docs" / "port-ledger.json"
LEDGER_MD = REPO / "docs" / "port-ledger.md"
STATUS_MD = REPO / "docs" / "STATUS.md"

# Functions at/above this VA are the statically-linked MSVC CRT tail.  The
# decompile of sotes.exe shows the last engine function (FUN_005bd680, 1072 B)
# ending just before the import-jump-table thunks at 0x5bdab0 (VerQueryValueA,
# DirectDrawCreateEx, …); from there up it is the C/C++ runtime — operator_new
# (0x5bf00d), __global_unwind2, __ftol, the `entry` CRT startup (0x5c0a8f),
# _malloc, _strcmp, __aulldiv, RtlUnwind, etc.  The drop-in links these like
# retail rather than porting them, so they inflate the denominator without
# being real port targets.  Keep this honest: it is a heuristic boundary,
# documented in STATUS, not ground truth.
ENGINE_PROPER_HI = 0x5BDAB0

# Kept out of band so STATUS stays a 60-second read.  Edit these two strings
# when the active front moves; everything else is derived.
CURRENT_PHASE = (
    "Phase 1 surface-map complete; Phase 4 in progress on the title path. "
    "All small leaves the title-menu scene runner needs are ported (pixel "
    "drawer, asset register, bitmap session, ZDD wrapper, cs_dispatch, "
    "WndProc, app_pump). Next front: the title-menu scene runner itself "
    "(FUN_0056aea0, 3441 B) — first visible frame. See ROADMAP.md, HANDOFF.md."
)
CURRENT_BLOCKER = (
    "FUN_0056aea0 is a multi-checkpoint port: outer skeleton + state-vars + "
    "the PTR_DAT_0056bfa4 7-handler jumptable, plus unported helpers "
    "(FUN_00412c10 menu-controller alloc, FUN_0043c110/_43ce50 input poll/"
    "latch, FUN_0056c070/_0056c180 sparkle+flip). See findings/title-scene.md."
)

FUN_RE = re.compile(r"FUN_([0-9a-fA-F]{6,8})")


def load_engine_functions() -> dict[int, dict]:
    """All engine functions from the Ghidra export, keyed by entry VA."""
    funcs: dict[int, dict] = {}
    with FUNCTIONS_CSV.open() as fh:
        for row in csv.DictReader(fh):
            try:
                va = int(row["entry"], 16)
            except (KeyError, ValueError):
                continue
            funcs[va] = {
                "name": row.get("name", ""),
                "size": int(row.get("size") or 0),
                "is_thunk": (row.get("is_thunk") == "true"),
            }
    return funcs


def tested_modules() -> set[str]:
    """src module stems that have a matching tests/test_<stem>.c."""
    stems = set()
    for t in TESTS.glob("test_*.c"):
        stems.add(t.stem[len("test_"):])
    return stems


def scan_src(tested: set[str]) -> dict[int, dict]:
    """Map each FUN_<va> referenced in src/ to the files that reference it and
    whether any of those files is a test-covered module."""
    refs: dict[int, set[str]] = {}
    for path in sorted(SRC.rglob("*.c")) + sorted(SRC.rglob("*.h")):
        rel = str(path.relative_to(REPO))
        text = path.read_text(errors="replace")
        for m in FUN_RE.finditer(text):
            refs.setdefault(int(m.group(1), 16), set()).add(rel)
    out: dict[int, dict] = {}
    for va, files in refs.items():
        is_tested = any(Path(f).stem in tested for f in files)
        out[va] = {"src": sorted(files), "tested": is_tested}
    return out


def classify(funcs, refs):
    """Assign one status per engine VA, plus collect 'ported but not an engine
    function' VAs (sub-helper labels not in the function table)."""
    entries: dict[int, dict] = {}
    known = set(funcs)
    for va, info in funcs.items():
        ref = refs.get(va)
        if ref is None:
            status, files = "unported", []
        elif ref["tested"]:
            status, files = "tested", ref["src"]
        else:
            status, files = "ported", ref["src"]
        entries[va] = {
            "va": f"0x{va:06x}",
            "name": info["name"],
            "size": info["size"],
            "is_thunk": info["is_thunk"],
            "status": status,
            "src": files,
        }
    orphans = sorted(set(refs) - known)
    return entries, orphans


def summarize(entries, funcs, orphans):
    real = {va: e for va, e in entries.items() if not e["is_thunk"]}
    proper = {va: e for va, e in real.items() if va < ENGINE_PROPER_HI}
    by = lambda s: sum(1 for e in real.values() if e["status"] == s)
    by_proper = lambda s: sum(1 for e in proper.values() if e["status"] == s)
    counts = {
        "engine_functions_total": len(funcs),
        "non_thunk_functions": len(real),
        "engine_proper_functions": len(proper),
        "library_tail_functions": len(real) - len(proper),
        "tested": by("tested"),
        "ported": by("ported"),
        "unported": by("unported"),
        "orphan_refs_not_in_function_table": len(orphans),
    }
    touched = counts["tested"] + counts["ported"]
    counts["touched"] = touched
    # Headline % is against engine-proper (below the CRT wall): the drop-in
    # links the library tail, so it isn't a port target.
    proper_touched = by_proper("tested") + by_proper("ported")
    counts["pct_touched"] = round(100.0 * proper_touched / max(1, len(proper)), 1)
    counts["pct_tested"] = round(100.0 * by_proper("tested") / max(1, len(proper)), 1)
    counts["engine_proper_touched"] = proper_touched
    # Bytes-covered: a better progress signal than function count, since the
    # engine has a long tail of tiny leaves and a few huge drivers.  Bytes are
    # also measured over engine-proper.
    tot_bytes = sum(e["size"] for e in proper.values())
    touched_bytes = sum(e["size"] for e in proper.values() if e["status"] != "unported")
    counts["bytes_total"] = tot_bytes
    counts["bytes_touched"] = touched_bytes
    counts["pct_bytes_touched"] = round(100.0 * touched_bytes / max(1, tot_bytes), 1)
    return counts


def render_json(entries, orphans, counts) -> str:
    payload = {
        "_generated_by": "tools/gen_port_ledger.py",
        "_note": "DERIVED FILE — do not edit by hand; run the generator. "
                 "Output is a pure function of src/ + tests/ + functions.csv, "
                 "so it is idempotent (safe for a pre-commit --check).",
        "counts": counts,
        "orphan_refs": [f"0x{va:06x}" for va in orphans],
        "functions": [entries[va] for va in sorted(entries)],
    }
    return json.dumps(payload, indent=1) + "\n"


def render_status(counts) -> str:
    c = counts
    bar_n = round(c["pct_touched"] / 5)  # 20-cell bar
    bar = "█" * bar_n + "░" * (20 - bar_n)
    return f"""# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
{bar}  {c['pct_touched']}% touched   ({c['pct_tested']}% host-tested, {c['pct_bytes_touched']}% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      | {c['tested']:>5} | ported + module covered by the host unit suite       |
| ported      | {c['ported']:>5} | reimplemented in src/, no host test for that module  |
| **touched** | **{c['touched']:>3}** | tested + ported (FUN_ provenance ref in src/)    |
| unported    | {c['unported']:>5} | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **{c['engine_proper_functions']}** below
`0x{ENGINE_PROPER_HI:06x}`. The other **{c['library_tail_functions']}** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **{c['non_thunk_functions']}** non-thunk
functions (of {c['engine_functions_total']} incl. thunks).

Code-byte coverage ({c['pct_bytes_touched']}% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** {CURRENT_PHASE}
- **Top blocker:** {CURRENT_BLOCKER}

## Where to read next

- `STATUS.md` (this file) — 60-second orientation.
- `HANDOFF.md` — "where to pick up *right now*" (rewritten each checkpoint).
- `port-ledger.md` / `.json` — per-function port status (derived).
- `port-frontier.md` — what to port next: unported fns reachable from ported
  code, zero-dep leaves ranked (derived; `tools/gen_frontier.py`).
- `ROADMAP.md` — milestones + subsystem map with difficulty / target module.
- `PROGRESS.md` — dated narrative changelog.
- `findings/INDEX.md` — map of subsystem RE writeups.
- `findings/engine-quirks.md` — the running quirk log (cite in commits).
- `AGENT-WORKFLOW.md` — how to work on this repo (read at session start).
- `PLAN.md` — goal, constraints, phased roadmap.
"""


def render_md(entries, orphans, counts) -> str:
    c = counts
    lines = [
        "# OpenSummoners — port ledger",
        "",
        "> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.",
        "> See `STATUS.md` for the headline.",
        "",
        "Per-engine-function port status, derived from `functions.csv` (universe)",
        "and `FUN_<va>` provenance references in `src/` (each port carries a",
        "`FUN_<va>` comment). A function is **tested** when its src module",
        "has a matching `tests/test_<stem>.c`. This is the answer to",
        "*\"is FUN_x done?\"* at a glance.",
        "",
        "## Summary",
        "",
        f"- engine-proper functions (below `0x{ENGINE_PROPER_HI:06x}`): "
        f"**{c['engine_proper_functions']}** — the real port universe",
        f"- library tail (MSVC CRT, linked not ported): "
        f"{c['library_tail_functions']}",
        f"- non-thunk engine functions total: {c['non_thunk_functions']} "
        f"(of {c['engine_functions_total']} incl. thunks)",
        f"- touched: **{c['touched']}** "
        f"({c['pct_touched']}% of engine-proper) — "
        f"tested {c['tested']}, ported {c['ported']}",
        f"- code bytes touched: **{c['pct_bytes_touched']}%** "
        f"({c['bytes_touched']:,} / {c['bytes_total']:,} B of engine-proper)",
        f"- unported: **{c['unported']}**",
        f"- orphan refs in src/ not in this table: "
        f"{c['orphan_refs_not_in_function_table']}",
        "",
    ]
    real = [entries[va] for va in sorted(entries) if not entries[va]["is_thunk"]]
    for status, blurb in (
        ("tested", "ported + host unit suite"),
        ("ported", "reimplemented, no host test yet"),
    ):
        rows = [e for e in real if e["status"] == status]
        lines.append(f"## {status} ({len(rows)}) — {blurb}")
        lines.append("")
        lines.append("| VA | name | size | src |")
        lines.append("|----|------|-----:|-----|")
        for e in rows:
            src = ", ".join(Path(s).name for s in e["src"][:3])
            if len(e["src"]) > 3:
                src += f" (+{len(e['src']) - 3})"
            lines.append(f"| {e['va']} | {e['name']} | {e['size']} | {src} |")
        lines.append("")
    if orphans:
        lines.append(f"## orphan refs ({len(orphans)}) — FUN_ in src/ not in function table")
        lines.append("")
        lines.append("Sub-helper labels (Ghidra splits a few functions) or typos. "
                     "Listed so they read as known, not as silent drift.")
        lines.append("")
        lines.append("`" + " ".join(f"0x{va:06x}" for va in orphans) + "`")
        lines.append("")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true",
                    help="exit 3 if on-disk ledger differs from a fresh gen")
    args = ap.parse_args()

    funcs = load_engine_functions()
    refs = scan_src(tested_modules())
    entries, orphans = classify(funcs, refs)
    counts = summarize(entries, funcs, orphans)

    out = {
        LEDGER_JSON: render_json(entries, orphans, counts),
        LEDGER_MD: render_md(entries, orphans, counts),
        STATUS_MD: render_status(counts),
    }

    if args.check:
        stale = [p.name for p, txt in out.items()
                 if not p.exists() or p.read_text() != txt]
        if stale:
            print(f"port ledger stale: {', '.join(stale)} "
                  f"(run: python3 tools/gen_port_ledger.py)", file=sys.stderr)
            return 3
        print("port ledger up to date")
        return 0

    for path, txt in out.items():
        path.write_text(txt)
    print(f"wrote {LEDGER_JSON.name}, {LEDGER_MD.name}, {STATUS_MD.name}")
    print(f"  {counts['touched']}/{counts['engine_proper_functions']} engine-proper "
          f"touched ({counts['pct_touched']}%), {counts['tested']} tested, "
          f"{counts['unported']} unported, "
          f"{counts['pct_bytes_touched']}% of code bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
