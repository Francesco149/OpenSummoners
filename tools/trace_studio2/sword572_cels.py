#!/usr/bin/env python3
"""Body-cel spans for ALL three Arche banks (res 0x570 sword-in / 0x571 sword-out /
0x572 up-thrust) + a per-tick count of the res-0x40b sword-tip TRAIL particles —
the chip-2c (UP attack) verification probe.  Streams one .osr, keeps the LAST flip
per tick, collapses the primary (lowest-seq) body cel into (res,frame) spans.

Usage: sword572_cels.py <file.osr> [tick_lo] [tick_hi]"""
import sys
sys.path.insert(0, "tools/trace_studio2")
import osr

path = sys.argv[1]
tlo = int(sys.argv[2]) if len(sys.argv) > 2 else 0
thi = int(sys.argv[3]) if len(sys.argv) > 3 else 10**9
BODY = {0x570, 0x571, 0x572}
TRAIL = 0x40b

per_tick = {}
cur_flip = cur_tick = None
cur = []
for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
    if r.type == osr.FRAMEBEG:
        if cur_tick is not None and tlo <= cur_tick <= thi:
            per_tick[cur_tick] = cur
        cur_flip, cur_tick, _ = r.framebeg()
        cur = []
        if cur_tick > thi:
            break
    elif r.type == osr.BLIT and cur_tick is not None:
        b = r.blit()
        if b.res in BODY or b.res == TRAIL:
            cur.append(b)

spans = []
for t in sorted(per_tick):
    body = sorted([b for b in per_tick[t] if b.res in BODY], key=lambda b: b.seq)
    p = body[0] if body else None
    ntrail = sum(1 for b in per_tick[t] if b.res == TRAIL)
    key = (p.res, p.frame) if p else None
    if spans and spans[-1]["key"] == key:
        spans[-1]["last"] = t
        spans[-1]["trail_max"] = max(spans[-1]["trail_max"], ntrail)
    else:
        spans.append({"key": key, "first": t, "last": t,
                      "dx": p.dx if p else 0, "dy": p.dy if p else 0,
                      "w": p.reqw if p else 0, "h": p.reqh if p else 0,
                      "trail_max": ntrail})
for s in spans:
    if s["key"] is None:
        print(f"  (no body)         ticks {s['first']}..{s['last']} ({s['last']-s['first']+1}t)")
        continue
    res, fr = s["key"]
    tag = {0x570: "0x570/in ", 0x571: "0x571/OUT", 0x572: "0x572/UP "}[res]
    extra = f"  trail<=0x40b x{s['trail_max']}" if s["trail_max"] else ""
    print(f"  {tag} fr={fr:<3} ticks {s['first']}..{s['last']} "
          f"({s['last']-s['first']+1}t) dst=({s['dx']},{s['dy']} {s['w']}x{s['h']}){extra}")
