#!/usr/bin/env python3
"""One-pass cel timeline for Arche's sword banks (res 0x570 sword-in, 0x571 sword-out).

Streams the whole .osr ONCE, keeps the LAST flip per tick (presented state), and
for each tick records every res-0x570/0x571 blit (frame + dst rect).  Collapses
consecutive identical (res,frame) into spans so the per-action cel sequence reads
cleanly.  Optional [tick_lo tick_hi] window."""
import sys
sys.path.insert(0, "tools/trace_studio2")
import osr

path = sys.argv[1]
tlo = int(sys.argv[2]) if len(sys.argv) > 2 else 0
thi = int(sys.argv[3]) if len(sys.argv) > 3 else 10**9
WANT = {0x570, 0x571}

per_tick = {}            # tick -> (flip, [(res,frame,dx,dy,w,h,seq), ...])
order = []
cur_flip = cur_tick = None
cur = []
for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
    if r.type == osr.FRAMEBEG:
        if cur_tick is not None and tlo <= cur_tick <= thi:
            if cur_tick not in per_tick:
                order.append(cur_tick)
            per_tick[cur_tick] = (cur_flip, cur)
        cur_flip, cur_tick, _ = r.framebeg()
        cur = []
        if cur_tick > thi:
            break
    elif r.type == osr.BLIT and cur_tick is not None:
        b = r.blit()
        if b.res in WANT:
            cur.append((b.res, b.frame, b.dx, b.dy, b.reqw, b.reqh, b.seq))

# emit a per-tick single-line summary, then collapse to spans on the "primary"
# (lowest-seq) Arche draw of whichever bank is present.
def primary(blits):
    if not blits:
        return None
    b = sorted(blits, key=lambda x: x[6])[0]   # lowest seq = the body
    return b  # (res,frame,dx,dy,w,h,seq)

rows = []
for t in sorted(per_tick):
    flip, blits = per_tick[t]
    p = primary(blits)
    # also note how many of each bank
    n570 = sum(1 for b in blits if b[0] == 0x570)
    n571 = sum(1 for b in blits if b[0] == 0x571)
    rows.append((t, p, n570, n571, blits))

# spans on (res, frame)
spans = []
for t, p, n570, n571, blits in rows:
    key = (p[0], p[1]) if p else None
    if spans and spans[-1]["key"] == key:
        spans[-1]["last"] = t
        spans[-1]["n570"] = max(spans[-1]["n570"], n570)
        spans[-1]["n571"] = max(spans[-1]["n571"], n571)
    else:
        spans.append({"key": key, "first": t, "last": t,
                      "dx": p[2] if p else 0, "dy": p[3] if p else 0,
                      "w": p[4] if p else 0, "h": p[5] if p else 0,
                      "n570": n570, "n571": n571})
for s in spans:
    if s["key"] is None:
        print(f"  (none)            ticks {s['first']}..{s['last']} ({s['last']-s['first']+1}t)")
        continue
    res, fr = s["key"]
    bank = "0x570/in " if res == 0x570 else "0x571/OUT"
    extra = ""
    if s["n570"] and s["n571"]:
        extra = f"  [BOTH banks: 570x{s['n570']} 571x{s['n571']}]"
    elif s["n571"] > 1:
        extra = f"  [571 x{s['n571']}]"
    print(f"  {bank} fr={fr:<4} ticks {s['first']}..{s['last']} "
          f"({s['last']-s['first']+1}t) dst=({s['dx']},{s['dy']} {s['w']}x{s['h']}){extra}")
