#!/usr/bin/env python3
"""Find the UP-attack's separate sheet (chip 2c).  Stream sword2.osr over a flip
window, build flip->tick, and per tick dump every BLIT near Arche (res/frame/dst/
seq) interleaved with the input trace, so the multi-sprite up-thrust effect sheet
(NOT res 0x570/0x571) stands out against the stable surrounding idle.

Usage: up_attack_probe.py FLO FHI [x0 y0 x1 y1]"""
import sys, json, bisect
sys.path.insert(0, "tools/trace_studio2")
import osr

OSR = "/mnt/c/oss-osr/sword2.osr"
INP = "/mnt/c/oss-osr/sword2-input.jsonl"
FLO = int(sys.argv[1]) if len(sys.argv) > 1 else 9400
FHI = int(sys.argv[2]) if len(sys.argv) > 2 else 9750
if len(sys.argv) >= 7:
    X0, Y0, X1, Y1 = (int(sys.argv[i]) for i in (3, 4, 5, 6))
else:
    X0, Y0, X1, Y1 = 60, 150, 420, 360   # Arche's body + the space above for an up-thrust
SCAN = {44: "Z", 45: "X", 200: "UP", 208: "DN", 203: "L", 205: "R"}


def overlaps(b):
    return (b.dx < X1 and X0 < b.dx + b.reqw and b.dy < Y1 and Y0 < b.dy + b.reqh)


f2t = {}
per_tick = {}
cur_flip = cur_tick = None
cur = []
for r in osr.stream_records(OSR, {osr.FRAMEBEG, osr.BLIT}):
    if r.type == osr.FRAMEBEG:
        if cur_tick is not None and FLO <= cur_flip <= FHI:
            per_tick[cur_tick] = cur
        cur_flip, cur_tick, _ = r.framebeg()
        f2t[cur_flip] = cur_tick
        cur = []
        if cur_flip > FHI:
            break
    elif r.type == osr.BLIT and cur_tick is not None:
        b = r.blit()
        if overlaps(b):
            cur.append(b)

flips = sorted(f2t)
def tk(flip):
    i = bisect.bisect_right(flips, flip) - 1
    return f2t[flips[max(0, i)]] if flips else None

events = {}
with open(INP) as fh:
    for line in fh:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        d = json.loads(line)
        fl = d["frame"]
        if not (FLO <= fl <= FHI):
            continue
        keys = "+".join(SCAN.get(k, str(k)) for k in d["keys"]) or "(release)"
        events.setdefault(tk(fl), []).append(f"flip{fl}={keys}")

# baseline backdrop = the res ids present in the EARLIEST tick (stable idle scene)
ticks_sorted = sorted(per_tick)
baseline = set()
if ticks_sorted:
    baseline = {b.res for b in per_tick[ticks_sorted[0]]}
BODY = {0x570, 0x571}

BODYRES = {0x570, 0x571, 0x572}     # all 3 Arche banks count as the body
spans = []                          # collapse the primary body cel into (res,frame) spans
trail = {}                          # tick -> (count, frames seen, dst list) for res 0x40b
for t in sorted(per_tick):
    bl = per_tick[t]
    body = [b for b in bl if b.res in BODYRES]
    p = sorted(body, key=lambda b: b.seq)[0] if body else None
    key = (p.res, p.frame) if p else None
    if spans and spans[-1]["key"] == key:
        spans[-1]["last"] = t
    elif p:
        spans.append({"key": key, "first": t, "last": t,
                      "dx": p.dx, "dy": p.dy, "w": p.reqw, "h": p.reqh})
    tb = [b for b in bl if b.res == 0x40b]
    if tb:
        trail[t] = (len(tb), sorted({b.frame for b in tb}),
                    sorted({(b.reqw, b.reqh) for b in tb}))

print("== BODY cel spans (res 0x570/0x571/0x572) ==")
for s in spans:
    res, fr = s["key"]
    print(f"  res=0x{res:x} fr={fr:<3} ticks {s['first']}..{s['last']} "
          f"({s['last']-s['first']+1}t) dst=({s['dx']},{s['dy']} {s['w']}x{s['h']})")
print("== TRAIL res 0x40b per tick (count, frames, sizes) ==")
for t in sorted(trail):
    n, frs, szs = trail[t]
    print(f"  tick {t}: {n} sprites frames={frs} sizes={szs}")
print("== input events ==")
for t in sorted(events):
    print(f"  tick {t}: {events[t]}")
