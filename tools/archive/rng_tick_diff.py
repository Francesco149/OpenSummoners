#!/usr/bin/env python3
"""rng_tick_diff — does the shared LCG stream agree run-to-run at equal sim-tick?

ARCHIVED (ckpt 129, 2026-06-13) — SUPERSEDED by the trace-studio ENGINE STATE PANEL.
The per-sim-tick rng/field diff this Frida-call_trace script computed is now LIVE in
`tools/osr_view` (the M8 OSR_STATE pass): capture the port with `--osr-state` and
retail with `OSS_OSR_STATE=1`, open both in `osr_view.exe <port> <retail>`, and the
"ENGINE STATE" panel shows rng (+ rngcalls port-side) per joined tick, port-vs-retail,
diff-highlighted.  `tools/trace_studio2/osr.py STATES <file>` is the headless dump.
Kept for reference only; do not run.  (The CONSUMER-attribution census — which
function draws the LCG — is a different concern, still `tools/rng_consumer_census.py`.)
"""
"""rng_tick_diff (original docstring follows):

The in-game frame is a wall-clock limiter that presents a NON-deterministic number
of Flips per sim tick (engine-quirk #75), and some consumers draw the LCG per
*present*, so the shared stream `DAT_008a4f94` desyncs run-to-run even under
`--seed-pin` (engine-quirk #77).  This tool quantifies that desync: it aligns two
retail runs on the deterministic sim-tick index (reset at game_enter) and reports,
per tick present in both runs, whether the LCG state — and any other field-spec'd
per-tick fields — match.

Input: two run dirs (or two call_trace.jsonl paths) produced by
  tools/run-retail.sh ... --call-trace --field-spec-only
with the actor-update field spec (tools/flow/retail_fields.json):
  0x46cd70 (actor_update_all) -> f.rng (LCG state) + f.rngcalls (cumulative draws)
  0x54f980 (actor_update)     -> f.a0_clip/a0_frame    (actor slot-0 anim block)

Usage:
  tools/rng_tick_diff.py RUN_A RUN_B
  tools/rng_tick_diff.py a/call_trace.jsonl b/call_trace.jsonl
"""
import json, sys, os

VA_UPDATE_ALL = 0x46cd70   # one row per sim tick, carries rng
VA_UPDATE     = 0x54f980   # many rows per tick; take the first (slot-0 anim)


def _ct_path(arg):
    return os.path.join(arg, "call_trace.jsonl") if os.path.isdir(arg) else arg


def load(path):
    """sim_tick -> {rng, rngcalls, a0_frame, a0_clip, a0_timer} (first seen per tick)."""
    per_tick = {}
    with open(path) as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            for ev in row.get("events", [row]):
                va = ev.get("va")
                t = ev.get("sim_tick", -1)
                if t < 0:
                    continue
                f = ev.get("f", {})
                d = per_tick.setdefault(t, {})
                if va == VA_UPDATE_ALL:
                    for k in ("rng", "rngcalls"):
                        if k in f:
                            d.setdefault(k, f[k])
                elif va == VA_UPDATE:
                    for k in ("a0_frame", "a0_clip", "a0_timer"):
                        if k in f:
                            d.setdefault(k, f[k])
    return per_tick


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    a = load(_ct_path(sys.argv[1]))
    b = load(_ct_path(sys.argv[2]))
    common = sorted(set(a) & set(b))
    print(f"runA ticks={len(a)} runB ticks={len(b)} common={len(common)}")
    if not common:
        print("NO COMMON SIM-TICKS — did both captures reach in-game (game_enter)?")
        return
    fields = ("rng", "rngcalls", "a0_frame", "a0_clip")
    n = {k: 0 for k in fields}
    m = {k: 0 for k in fields}
    shown = 0
    for t in common:
        da, db = a[t], b[t]
        marks = {}
        for k in fields:
            va, vb = da.get(k), db.get(k)
            if va is None or vb is None:
                marks[k] = "?"
                continue
            n[k] += 1
            ok = va == vb
            m[k] += ok
            marks[k] = "Y" if ok else "N"
        # print the first few ticks plus any with a mismatch (capped)
        if (shown < 12 or "N" in marks.values()) and shown < 40:
            shown += 1
            print(f"  t{t:<5} rng[{da.get('rng')!s:>10}|{db.get('rng')!s:>10}]{marks['rng']}"
                  f"  calls[{da.get('rngcalls')!s:>5}|{db.get('rngcalls')!s:>5}]{marks['rngcalls']}"
                  f"  frame[{da.get('a0_frame')!s:>3}|{db.get('a0_frame')!s:>3}]{marks['a0_frame']}"
                  f"  clip[{da.get('a0_clip')!s:>5}|{db.get('a0_clip')!s:>5}]{marks['a0_clip']}")
    print("\nSUMMARY (ticks where both runs carried the field):")
    for k in fields:
        if not n[k]:
            print(f"  {k:>10}: (absent)")
            continue
        tag = "  <-- DETERMINISTIC" if m[k] == n[k] else "  <-- DIVERGES"
        print(f"  {k:>10}: {m[k]}/{n[k]} match{tag}")


if __name__ == "__main__":
    main()
