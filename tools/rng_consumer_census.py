#!/usr/bin/env python3
"""rng_consumer_census.py — enumerate a scene's RNG consumers from a flow trace.

The companion analysis to the `0x5bf505` flow-trace entry in
`tools/flow/retail_fields.json` (the engine LCG / MSVC rand).  Capture the scene
with that entry hooked under a tight frame whitelist, e.g.::

    python3 tools/frida_capture.py --no-turbo --lockstep --seed-pin \
        --input-trace tests/scenarios/in-game-intro/trace-retail.jsonl \
        --field-spec tools/flow/retail_fields.json --field-spec-only \
        --call-trace --call-trace-frames <spawn..hold window> \
        --max-frames 1680 --run-dir runs/rng-census --exact-run-dir

then run this over the resulting `call_trace.jsonl`.  Every `0x5bf505` event
carries an auto-captured `ret_va` (the CONSUMER SITE = the address the LCG
returns to); this tool maps each `ret_va` to its containing function (via
`docs/decompiled/functions.csv`) and tallies the draws, optionally split at a
frame boundary (the one-shot SPAWN burst at room-load vs the recurring per-tick
HOLD draws) so the animation/particle/wander consumers separate from the
spawn-time facing/phase consumers.

This is the DISCOVERY half of the RNG-consumer census (Phase 2, the directive
that retires the ckpt-73 defer-all-RNG): it names WHO draws.  The MATCHING half
is the standard flow-trace `rngcalls` field on each named producer + a port-side
`CALL_TRACE_BEGIN` mirror, compared by `tools/flow_diff.py`.

Engine-quirk #77; docs/findings/in-game-intro.md "The scene-wide RNG-consumer
census".  ret_va is an RVA (image base 0x400000 added to reach the Ghidra VA).
"""
import argparse
import bisect
import collections
import csv
import json
import sys
from pathlib import Path

IMAGE_BASE = 0x400000
RAND_VA = 0x5BF505  # FUN_005bf505 — the engine LCG (MSVC rand)


def load_funcs(csv_path: Path):
    """[(start, end, name)] sorted by start, from functions.csv (name,entry,size,…)."""
    funcs = []
    with open(csv_path) as f:
        for row in csv.DictReader(f):
            try:
                va = int(row["entry"], 16)
                funcs.append((va, va + int(row["size"]), row["name"]))
            except (ValueError, KeyError):
                pass
    funcs.sort()
    return funcs


def fn_of(funcs, starts, va: int) -> str:
    i = bisect.bisect_right(starts, va) - 1
    if i >= 0 and funcs[i][0] <= va < funcs[i][1]:
        return funcs[i][2]
    return f"?{va:#x}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("trace", type=Path, help="call_trace.jsonl with the 0x5bf505 entry hooked")
    ap.add_argument("--funcs", type=Path, default=Path("docs/decompiled/functions.csv"),
                    help="functions.csv (name,entry,size,…)")
    ap.add_argument("--split-frame", type=int, default=None,
                    help="frame boundary: draws at frame <= N are SPAWN, > N are HOLD "
                         "(default: no split). For the town set this to game_enter's flip.")
    ap.add_argument("--order", action="store_true",
                    help="also print the per-seq consumption order (the first --order-n draws)")
    ap.add_argument("--order-n", type=int, default=40)
    args = ap.parse_args()

    funcs = load_funcs(args.funcs)
    starts = [f[0] for f in funcs]

    spawn = collections.Counter()
    hold = collections.Counter()
    sites = collections.defaultdict(set)
    perframe = collections.Counter()
    order = []  # (frame, seq, fn, va)
    total = 0

    with open(args.trace) as f:
        for line in f:
            try:
                o = json.loads(line)
            except json.JSONDecodeError:
                continue
            if o.get("va") != RAND_VA:
                continue
            rv = o.get("ret_va")
            if rv is None:
                continue
            va = rv + IMAGE_BASE
            fr = o.get("frame", 0)
            fn = fn_of(funcs, starts, va)
            sites[fn].add(va)
            perframe[fr] += 1
            total += 1
            if args.split_frame is not None and fr > args.split_frame:
                hold[fn] += 1
            else:
                spawn[fn] += 1
            order.append((fr, o.get("seq", 0), fn, va))

    if total == 0:
        print(f"no 0x5bf505 draws in {args.trace} — was the entry hooked "
              f"(--field-spec + a frame whitelist covering the scene)?", file=sys.stderr)
        return 1

    split = args.split_frame
    hdr_spawn = f"<= {split}" if split is not None else "all"
    hdr_hold = f"> {split}" if split is not None else "-"
    print(f"# RNG-consumer census — {total} LCG (0x5bf505) draws, "
          f"{len(set(spawn) | set(hold))} consumer functions")
    print(f"# draws/frame: {dict(sorted(perframe.items()))}")
    print(f"{'consumer function':30s} {hdr_spawn:>8s} {hdr_hold:>8s}  sites")
    allfn = set(spawn) | set(hold)
    for fn in sorted(allfn, key=lambda k: -(spawn[k] + hold[k])):
        ss = " ".join(hex(s) for s in sorted(sites[fn]))
        print(f"  {fn:30s} {spawn[fn]:8d} {hold[fn]:8d}  {ss}")

    if args.order:
        print(f"\n# consumption order (first {args.order_n} draws, by frame/seq)")
        order.sort(key=lambda e: (e[0], e[1]))
        for fr, seq, fn, va in order[:args.order_n]:
            print(f"  frame={fr} seq={seq:5d}  {fn}  ({va:#08x})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
