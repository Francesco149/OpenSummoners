#!/usr/bin/env python3
"""tools/trace_studio2/draw_probe.py — the ORDERED drawcall probe (drawcall per
drawcall ground truth).

Streams an .osr and, for each sim-tick in a range, dumps the BLIT records whose
dest rect intersects a screen region, IN DRAW ORDER (the per-frame `seq`; a later
seq draws ON TOP — that is the z-order, ground truth, no guessing).  Each line:
seq, primitive (onto/keyed/rects/clipped/alpha), res, frame, dest (x,y,w,h),
src (x,y,w,h), state/ckey/blend.  Use it to read EXACTLY what the engine draws in
a region over an animation — e.g. the two dialogue boxes during a speaker-change
transition: how many cels, each one's scale (dest w/h) + position, and which is
in front (higher seq).

Takes the LAST flip at each tick (the presented state — retail coalesces, so the
last flip of a tick is what's on screen).

Usage:
  draw_probe.py <file.osr> <tick_lo> <tick_hi> [x0 y0 x1 y1]   # default region = box band
  draw_probe.py <file.osr> --res <id> <tick_lo> <tick_hi> [...]  # filter to one res
"""
from __future__ import annotations

import sys

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402

PRIM = osr.BLIT_VA_NAME


def rects_overlap(ax, ay, aw, ah, bx, by, bw, bh):
    return ax < bx + bw and bx < ax + aw and ay < by + bh and by < ay + ah


def last_flip_per_tick(path, tick_lo, tick_hi):
    """Yield (tick, flip, [Blit...]) for the LAST flip of each tick in range."""
    cur_flip = cur_tick = None
    cur_blits = []
    per_tick = {}      # tick -> (flip, blits)  (overwrite keeps the last flip)
    order = []
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            if cur_tick is not None and tick_lo <= cur_tick <= tick_hi:
                if cur_tick not in per_tick:
                    order.append(cur_tick)
                per_tick[cur_tick] = (cur_flip, cur_blits)
            cur_flip, cur_tick, _ = r.framebeg()
            cur_blits = []
            if cur_tick > tick_hi:
                break
        elif r.type == osr.BLIT and cur_tick is not None:
            cur_blits.append(r.blit())
    if cur_tick is not None and tick_lo <= cur_tick <= tick_hi and cur_tick > tick_hi - 1:
        per_tick.setdefault(cur_tick, (cur_flip, cur_blits))
        if cur_tick not in order:
            order.append(cur_tick)
    for t in sorted(per_tick):
        flip, blits = per_tick[t]
        yield t, flip, blits


def main(argv):
    if len(argv) < 4:
        print(__doc__)
        return 2
    path = argv[1]
    a = argv[2:]
    res_filter = None
    if a and a[0] == "--res":
        res_filter = int(a[1], 0)
        a = a[2:]
    tlo, thi = int(a[0]), int(a[1])
    if len(a) >= 6:
        x0, y0, x1, y1 = (int(a[2]), int(a[3]), int(a[4]), int(a[5]))
    else:
        x0, y0, x1, y1 = 0, 120, 640, 280     # the dialogue-box band
    rw, rh = x1 - x0, y1 - y0
    print(f"== draws in region ({x0},{y0})..({x1},{y1}) ticks [{tlo},{thi}]"
          + (f" res=0x{res_filter:x}" if res_filter is not None else "") + " ==")
    for tick, flip, blits in last_flip_per_tick(path, tlo, thi):
        hits = []
        for b in blits:
            if res_filter is not None and b.res != res_filter:
                continue
            if rects_overlap(b.dx, b.dy, b.reqw, b.reqh, x0, y0, rw, rh):
                hits.append(b)
        # keep draw order (seq)
        hits.sort(key=lambda b: b.seq)
        print(f"-- tick {tick} flip {flip}: {len(hits)} draws in region --")
        for b in hits:
            name = PRIM.get(b.va, hex(b.va))
            src = f"src=({b.sx},{b.sy} {b.srcw}x{b.srch})" if (b.srcw or b.srch) \
                  else f"src=({b.sx},{b.sy})"
            print(f"   seq={b.seq:<4} {name:<7} res={b.res} fr={b.frame} "
                  f"dst=({b.dx},{b.dy} {b.reqw}x{b.reqh}) {src} "
                  f"st=0x{b.state:x} ckey=0x{b.ckey:x} bmode={b.bmode}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
