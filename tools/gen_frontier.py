#!/usr/bin/env python3
"""Derive the *port frontier*: the unported engine functions that are already
reachable from ported code — i.e. the natural next chips.

Two artifacts already in the tree make this a pure derivation (no drift):

  - docs/decompiled/by-address/<va>.c — per-function Ghidra decompile; the
    `FUN_<va>` tokens in a body are that function's call edges.
  - docs/port-ledger.json — which VAs are already touched (run
    gen_port_ledger.py first; this script reads its output).

A function is on the **frontier** when it is unported + non-thunk + below the
CRT library wall and at least one *touched* (ported/tested) function calls
it. It is a **leaf** of the frontier when every function IT calls is already
ported (or in the library tail) — so it can be ported today with zero new
engine dependencies. Leaves, sorted by how many ported callers want them, are
the recommended porting order.

Output (git-tracked, no vendor bytes):
  docs/port-frontier.md   ranked frontier table + leaf shortlist, by address band

Run:  python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
"""

from __future__ import annotations

import csv
import json
import re
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
FUNCTIONS_CSV = REPO / "docs" / "decompiled" / "functions.csv"
BY_ADDR = REPO / "docs" / "decompiled" / "by-address"
LEDGER_JSON = REPO / "docs" / "port-ledger.json"
OUT_MD = REPO / "docs" / "port-frontier.md"

# Matches gen_port_ledger.py — functions at/above this are the linked CRT tail,
# not port targets, so they don't count as unported dependencies.
ENGINE_PROPER_HI = 0x5BDAB0

FUN_RE = re.compile(r"FUN_([0-9a-fA-F]{6,8})")

# Address-band → subsystem labels, seeded by the opensummoners-subsystem-survey
# workflow (see ROADMAP.md "Subsystem map" for the full breakdown + VAs).
# Bands are approximate and group the table only; they don't assert ground truth.
BANDS = [
    (0x401000, 0x410000, "object-pool ctor + game-loop FSM + font/glyph + msg fmt"),
    (0x410000, 0x420000, "menu/dialog controller + char init + shop/NPC + save path"),
    (0x420000, 0x430000, "scene/level init + entity spawn + def-by-id lookup"),
    (0x430000, 0x440000, "battle scenario init + turn engine + input poll + save mgr"),
    (0x440000, 0x450000, "entity per-frame FSM + action handlers + dialog + skills"),
    (0x450000, 0x470000, "master dialogue runner + action/frame dispatch + sprite batch"),
    (0x470000, 0x480000, "battle phase controller + NPC AI + particle fx + damage UI"),
    (0x480000, 0x490000, "char anim + hit-test/knockback + GDI glyph + sfx trigger"),
    (0x490000, 0x4a0000, "tile/sprite grid render + spell fx + battle UI + palette"),
    (0x4a0000, 0x4d0000, "scene-event dispatch (narrative scripting) + party/inventory"),
    (0x4d0000, 0x540000, "narrative scene FSM + dungeon/encounter driver (sparse)"),
    (0x540000, 0x560000, "cutscene dispatcher + sprite copy + tilemap collision + camera"),
    (0x560000, 0x570000, "title + gameplay scene runners + engine init + options + input init"),
    (0x570000, 0x590000, "master sprite-group register + audio/music init + anim pump"),
    (0x590000, 0x5a0000, "inventory/menu + audio cue mgr + render dispatch + scene load"),
    (0x5a0000, 0x5bdab0, "launcher config parse + spell fx + bitmap/ZDD render + RNG"),
]


def band_of(va: int) -> str:
    for lo, hi, name in BANDS:
        if lo <= va < hi:
            return name
    return "other"


def main() -> int:
    funcs = {}
    with FUNCTIONS_CSV.open() as fh:
        for r in csv.DictReader(fh):
            try:
                va = int(r["entry"], 16)
            except (KeyError, ValueError):
                continue
            funcs[va] = {"name": r["name"], "size": int(r.get("size") or 0),
                         "thunk": r.get("is_thunk") == "true"}

    ledger = json.loads(LEDGER_JSON.read_text())
    touched = {int(e["va"], 16) for e in ledger["functions"] if e["status"] != "unported"}

    def is_thunkish(va: int, f: dict) -> bool:
        # A <=6-byte x86 function is a single `jmp`/`ret` stub with no portable
        # body (import jump-table entries, tail-call trampolines). The port
        # handles those structurally, so they are not port targets and must not
        # pollute the frontier.
        return f["thunk"] or f["size"] <= 6

    def is_dep(c: int) -> bool:
        # A callee counts as an unported dependency only if it is an
        # engine-proper, non-thunk, untouched function with a real body.
        if c in touched or c >= ENGINE_PROPER_HI:
            return False
        f = funcs.get(c)
        if f is None or is_thunkish(c, f):
            return False
        return True

    callees: dict[int, set[int]] = defaultdict(set)
    callers: dict[int, set[int]] = defaultdict(set)
    for f in BY_ADDR.glob("*.c"):
        try:
            cva = int(f.stem, 16)
        except ValueError:
            continue
        for m in FUN_RE.finditer(f.read_text(errors="replace")):
            callee = int(m.group(1), 16)
            if callee == cva:
                continue
            callees[cva].add(callee)
            callers[callee].add(cva)

    rows = []
    for va, info in funcs.items():
        if is_thunkish(va, info) or va in touched or va >= ENGINE_PROPER_HI:
            continue
        pc = callers.get(va, set()) & touched
        if not pc:
            continue
        cs = callees.get(va, set())
        deps = [c for c in cs if is_dep(c)]
        rows.append({
            "va": va, "name": info["name"], "size": info["size"],
            "ported_callers": len(pc), "n_callees": len(cs),
            "unported_deps": len(deps),
            "leaf": not deps, "band": band_of(va),
        })

    rows.sort(key=lambda r: (-r["ported_callers"], r["size"]))
    leaves = [r for r in rows if r["leaf"]]

    L = []
    L.append("# OpenSummoners — port frontier (what to port next)")
    L.append("")
    L.append("> **DERIVED FILE** — `python3 tools/gen_port_ledger.py && "
             "python3 tools/gen_frontier.py`.")
    L.append("")
    L.append("The unported, non-thunk, engine-proper functions already **called "
             "by ported code** — the natural next chips. A **leaf** has all of its")
    L.append("own engine callees ported, so it can land today with zero new "
             "dependencies. Sorted by how many ported callers want it.")
    L.append("")
    L.append("For the *forward* port path (the title-menu scene runner and what it "
             "calls) and the semantic milestone order, see `ROADMAP.md` — some of "
             "that path isn't yet reachable from ported code so won't appear here.")
    L.append("")
    L.append(f"- frontier functions: **{len(rows)}**")
    L.append(f"- of those, zero-dependency **leaves: {len(leaves)}** "
             f"(recommended order below)")
    L.append("")
    L.append("## Leaf shortlist — portable today (top 40 by ported-caller count)")
    L.append("")
    L.append("| VA | size | ported callers | band |")
    L.append("|----|-----:|---------------:|------|")
    for r in leaves[:40]:
        L.append(f"| 0x{r['va']:06x} | {r['size']} | {r['ported_callers']} | {r['band']} |")
    if not leaves:
        L.append("| — | — | — | (none yet — touched set too small) |")
    L.append("")
    L.append("## Full frontier by address band")
    L.append("")
    by_band: dict[str, list] = defaultdict(list)
    for r in rows:
        by_band[r["band"]].append(r)
    for lo, hi, name in BANDS:
        br = by_band.get(name, [])
        if not br:
            continue
        L.append(f"### {name} ({len(br)})")
        L.append("")
        L.append("| VA | size | ported callers | unported deps | leaf |")
        L.append("|----|-----:|---------------:|--------------:|:----:|")
        for r in sorted(br, key=lambda r: (-r["ported_callers"], r["size"])):
            L.append(f"| 0x{r['va']:06x} | {r['size']} | {r['ported_callers']} | "
                     f"{r['unported_deps']} | {'✓' if r['leaf'] else ''} |")
        L.append("")

    OUT_MD.write_text("\n".join(L) + "\n")
    print(f"wrote {OUT_MD.relative_to(REPO)}: {len(rows)} frontier, {len(leaves)} leaves")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
