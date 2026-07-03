#!/usr/bin/env python3
"""blits_at.py — streaming dump of the BLIT records in the frame(s) at a given
sim_tick (or tick window), optionally filtered by resource id.  Works on the
multi-GB retail captures (osr.py's BLITS command whole-file-parses; this streams).

Usage:
    blits_at.py <file.osr> <tick|lo-hi> [res]
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import osr  # noqa: E402

VA = osr.BLIT_VA_NAME


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    path = argv[1]
    if "-" in argv[2]:
        lo, hi = (int(x) for x in argv[2].split("-", 1))
    else:
        lo = hi = int(argv[2])
    res_filter = int(argv[3], 0) if len(argv) > 3 else None

    cur_flip = cur_tick = None
    printed_hdr = False
    keep = False
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            cur_flip, cur_tick, _ = r.framebeg()
            keep = lo <= cur_tick <= hi
            printed_hdr = False
        elif r.type == osr.BLIT and keep:
            b = r.blit()
            if res_filter is not None and b.res != res_filter:
                continue
            if not printed_hdr:
                print(f"== flip={cur_flip} sim_tick={cur_tick} ==")
                printed_hdr = True
            name = VA.get(b.va, hex(b.va))
            print(f"  #{b.seq:<3} {name:<7} res={b.res} frame={b.frame} "
                  f"dhash=0x{b.dhash:08x} dst=({b.dx},{b.dy}) {b.reqw}x{b.reqh} "
                  f"src=({b.sx},{b.sy}) o=({b.ox},{b.oy},{b.ow},{b.oh}) "
                  f"st=0x{b.state:x} ckey=0x{b.ckey:x} bmode={b.bmode}")
        if cur_tick is not None and cur_tick > hi:
            break
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
