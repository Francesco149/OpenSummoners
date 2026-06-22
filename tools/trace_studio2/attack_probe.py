#!/usr/bin/env python3
"""Chip-2 attack ground truth: stream sword2.osr ONCE over a flip window, build
flip->tick, collect res 0x570/0x571 cel spans, and interleave the input-trace
events (sword2-input.jsonl) at their mapped ticks so the per-action cel sequence
reads with its trigger.  Usage: attack_probe.py FLO FHI"""
import sys, json, bisect
sys.path.insert(0, "tools/trace_studio2")
import osr

OSR   = "/mnt/c/oss-osr/sword2.osr"
INP   = "/mnt/c/oss-osr/sword2-input.jsonl"
FLO   = int(sys.argv[1]) if len(sys.argv) > 1 else 8400
FHI   = int(sys.argv[2]) if len(sys.argv) > 2 else 11000
WANT  = {0x570, 0x571}
SCAN  = {44: "Z", 45: "X", 200: "UP", 208: "DN", 203: "L", 205: "R"}

# stream once: flip->tick + per-tick blits, bounded by FHI
f2t = {}
per_tick = {}
cur_flip = cur_tick = None
cur = []
for r in osr.stream_records(OSR, {osr.FRAMEBEG, osr.BLIT}):
    if r.type == osr.FRAMEBEG:
        if cur_tick is not None and FLO <= cur_flip <= FHI:
            per_tick[cur_tick] = (cur_flip, cur)
        cur_flip, cur_tick, _ = r.framebeg()
        f2t[cur_flip] = cur_tick
        cur = []
        if cur_flip > FHI:
            break
    elif r.type == osr.BLIT and cur_tick is not None:
        b = r.blit()
        if b.res in WANT:
            cur.append((b.res, b.frame, b.dx, b.dy, b.reqw, b.reqh, b.seq))

flips = sorted(f2t)
def tk(flip):
    i = bisect.bisect_right(flips, flip) - 1
    return f2t[flips[max(0, i)]] if flips else None

# input events -> ticks (in window)
events = {}   # tick -> list of "key-combo" strings
with open(INP) as fh:
    for line in fh:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        d = json.loads(line)
        fl = d["frame"]
        if not (FLO <= fl <= FHI):
            continue
        t = tk(fl)
        keys = "+".join(SCAN.get(k, str(k)) for k in d["keys"]) or "(release)"
        events.setdefault(t, []).append(f"flip{fl}={keys}")

# collapse blits to cel spans (primary = lowest seq), interleave events
def primary(blits):
    return sorted(blits, key=lambda x: x[6])[0] if blits else None

rows = []
for t in sorted(per_tick):
    flip, blits = per_tick[t]
    rows.append((t, primary(blits), sum(1 for b in blits if b[0]==0x570),
                 sum(1 for b in blits if b[0]==0x571)))

spans = []
for t, p, n570, n571 in rows:
    key = (p[0], p[1]) if p else None
    if spans and spans[-1]["key"] == key:
        spans[-1]["last"] = t
    else:
        spans.append({"key": key, "first": t, "last": t,
                      "dx": p[2] if p else 0, "dy": p[3] if p else 0,
                      "w": p[4] if p else 0, "h": p[5] if p else 0,
                      "n570": n570, "n571": n571})

all_ticks = sorted(set(per_tick) | set(events))
printed_spans = set()
for t in all_ticks:
    if t in events:
        for e in events[t]:
            print(f"      >> tick {t}: INPUT {e}")
    # print a span header when a span starts at t
    for i, s in enumerate(spans):
        if s["first"] == t and i not in printed_spans:
            printed_spans.add(i)
            if s["key"] is None:
                print(f"  (none)              ticks {s['first']}..{s['last']} ({s['last']-s['first']+1}t)")
            else:
                res, fr = s["key"]
                bank = "0x570/in " if res == 0x570 else "0x571/OUT"
                extra = f"  [571x{s['n571']}]" if s['n571'] > 1 else ""
                print(f"  {bank} fr={fr:<4} ticks {s['first']}..{s['last']} "
                      f"({s['last']-s['first']+1}t) dst=({s['dx']},{s['dy']} {s['w']}x{s['h']}){extra}")
