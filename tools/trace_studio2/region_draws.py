#!/usr/bin/env python3
"""region_draws.py — dump, in seq order, every BLIT of a specific frame (by flip)
whose dest rect overlaps a screen region.  Used to find an OVERDRAW / z-order or
a missing tile in a localized area (errands bed §7).  Streams (multi-GB safe).

Usage:
    region_draws.py <file.osr> <flip> <x0> <y0> <x1> <y1>
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import osr  # noqa: E402

VA = osr.BLIT_VA_NAME


def main(argv):
    if len(argv) < 7:
        print(__doc__)
        return 2
    path = argv[1]
    want_flip = int(argv[2])
    x0, y0, x1, y1 = (int(v) for v in argv[3:7])

    in_frame = False
    cur_tick = None
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            flip, cur_tick, _ = r.framebeg()
            if in_frame:            # passed our frame
                break
            in_frame = (flip == want_flip)
            if in_frame:
                print(f"== flip={flip} sim_tick={cur_tick}  region "
                      f"[{x0},{y0}..{x1},{y1}] ==")
        elif r.type == osr.BLIT and in_frame:
            b = r.blit()
            # dst rect [dx, dx+reqw) x [dy, dy+reqh); overlap with region?
            if b.dx < x1 and b.dx + b.reqw > x0 and b.dy < y1 and b.dy + b.reqh > y0:
                name = VA.get(b.va, hex(b.va))
                print(f"  #{b.seq:<3} {name:<7} res={b.res} fr={b.frame} "
                      f"dhash=0x{b.dhash:08x} dst=({b.dx},{b.dy}) {b.reqw}x{b.reqh} "
                      f"src=({b.sx},{b.sy}) st=0x{b.state:x} ckey=0x{b.ckey:x} "
                      f"bmode={b.bmode}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
