#!/usr/bin/env python3
"""randtrace_attrib.py — attribute the proxy [randtrace] ret_va log to functions.

Maps each rand (FUN_005bf505) consumer ret_va to its containing function (via
docs/decompiled/functions.csv) and prints, per tick, the consumer histogram — so
the per-tick draw that the port OMITS (the tick-974 +1/tick census divergence)
is named by the function that appears in the diverged ticks but not the matched
ones.  NB the proxy's draw-time tick is +1 vs the census easer-delta tick.

Usage: randtrace_attrib.py <randtrace.log> [tick_a tick_b]
  With two ticks, diffs their consumer multisets (a=matched, b=diverged).
"""
import sys
import csv
import bisect
import collections
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CSV = ROOT / "docs" / "decompiled" / "functions.csv"


def load_funcs():
    starts, rows = [], []
    with open(CSV) as f:
        for r in csv.DictReader(f):
            va = int(r["entry"], 16)
            starts.append(va)
            rows.append((va, int(r["size"]), r["name"]))
    order = sorted(range(len(starts)), key=lambda i: starts[i])
    starts = [starts[i] for i in order]
    rows = [rows[i] for i in order]
    return starts, rows


def fn_of(starts, rows, va):
    i = bisect.bisect_right(starts, va) - 1
    if i < 0:
        return None
    fva, sz, name = rows[i]
    if fva <= va < fva + sz:
        return (fva, name, va - fva)
    return (fva, name + "?", va - fva)  # past end — nearest-below (approx)


def parse(path):
    per = collections.defaultdict(list)   # tick -> [retva,...]
    for line in open(path):
        if "[randtrace]" not in line:
            continue
        # [randtrace] t=973 rc=... retva=0x46fc73
        parts = line.split()
        t = rc = va = None
        for p in parts:
            if p.startswith("t="):
                t = int(p[2:])
            elif p.startswith("retva="):
                va = int(p.split("=", 1)[1], 16)
        if t is not None and va is not None:
            per[t].append(va)
    return per


def hist(starts, rows, vas):
    c = collections.Counter()
    for va in vas:
        f = fn_of(starts, rows, va)
        key = f"0x{va:06x} {f[1]}+0x{f[2]:x}" if f else f"0x{va:06x} ?"
        c[key] += 1
    return c


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    starts, rows = load_funcs()
    per = parse(argv[1])
    if not per:
        print("no [randtrace] lines found")
        return 1
    ticks = sorted(per)
    print(f"{len(ticks)} ticks {ticks[0]}..{ticks[-1]}; "
          f"draws/tick: " + " ".join(f"{t}:{len(per[t])}" for t in ticks[:6]) + " ...")
    if len(argv) >= 4:
        a, b = int(argv[2]), int(argv[3])
        ha, hb = hist(starts, rows, per[a]), hist(starts, rows, per[b])
        print(f"\n=== tick {a} (n={len(per[a])}) consumers ===")
        for k, v in ha.most_common():
            print(f"  {v:3}  {k}")
        print(f"\n=== tick {b} (n={len(per[b])}) consumers ===")
        for k, v in hb.most_common():
            print(f"  {v:3}  {k}")
        print(f"\n=== DIFF (tick {b} minus tick {a}) — the extra/omitted consumer ===")
        keys = set(ha) | set(hb)
        for k in sorted(keys, key=lambda k: hb[k] - ha[k], reverse=True):
            d = hb[k] - ha[k]
            if d != 0:
                print(f"  {d:+3}  {k}")
    else:
        # overall consumer histogram across the whole window
        allc = collections.Counter()
        for t in ticks:
            allc += hist(starts, rows, per[t])
        print("\n=== all-window consumer histogram ===")
        for k, v in allc.most_common(40):
            print(f"  {v:4}  {k}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
