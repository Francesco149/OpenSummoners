#!/usr/bin/env python3
"""Extract the UP-attack sword-tip TRAIL sparkle stream (res 0x40b) from sword2.osr,
RELATIVE to the body anchor (the primary res 0x572 blit), so it can be replayed
re-anchored to the port's Arche.  Arche is STATIONARY during the up-attack, so the
relative offsets are a deterministic function of the swing tick.

For each tick in [tlo,thi]: the body (res 0x572) dst, then every res 0x40b sparkle as
(frame, dx-body_dx, dy-body_dy, w, h, seq order).  Usage: trail_extract.py [tlo thi]"""
import sys
sys.path.insert(0, "tools/trace_studio2")
import osr

OSR = "/mnt/c/oss-osr/sword2.osr"
tlo = int(sys.argv[1]) if len(sys.argv) > 1 else 3878
thi = int(sys.argv[2]) if len(sys.argv) > 2 else 3914

per_tick = {}
cf = ct = None
cur = []
for r in osr.stream_records(OSR, {osr.FRAMEBEG, osr.BLIT}):
    if r.type == osr.FRAMEBEG:
        if ct is not None and tlo <= ct <= thi:
            per_tick[ct] = cur
        cf, ct, _ = r.framebeg()
        cur = []
        if ct > thi:
            break
    elif r.type == osr.BLIT and ct is not None:
        b = r.blit()
        if b.res in (0x572, 0x40b):
            cur.append(b)

for t in sorted(per_tick):
    bl = per_tick[t]
    body = sorted([b for b in bl if b.res == 0x572], key=lambda b: b.seq)
    spk = sorted([b for b in bl if b.res == 0x40b], key=lambda b: b.seq)
    if not body:
        print(f"tick {t}: (no body 0x572)  {len(spk)} sparkles")
        continue
    bx, by = body[0].dx, body[0].dy
    rels = [(b.frame, b.dx - bx, b.dy - by, b.reqw, b.reqh) for b in spk]
    print(f"tick {t}: body 0x572 fr={body[0].frame} dst=({bx},{by})  "
          f"{len(spk)} sparkles (frame, dx-rel, dy-rel, w, h):")
    for fr, rx, ry, w, h in rels:
        print(f"    fr={fr} rel=({rx:+d},{ry:+d}) {w}x{h}")
