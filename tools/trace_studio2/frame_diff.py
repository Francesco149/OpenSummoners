#!/usr/bin/env python3
"""frame_diff.py — compare the BLIT sets of one frame in each of two captures,
matching by (res, frame) with a dst tolerance (absorbs a small camera-x phase
offset), and report the draws present on one side but not the other.  Names the
MISSING spawns / EXTRA draws in a localized scene (errands upstairs §7).

Usage:
    frame_diff.py <a.osr> <a_flip> <b.osr> <b_flip> [dst_tol]
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import osr  # noqa: E402

VA = osr.BLIT_VA_NAME


def collect(path, want_flip):
    out, in_frame = [], False
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            flip, tick, _ = r.framebeg()
            if in_frame:
                break
            in_frame = (flip == want_flip)
        elif r.type == osr.BLIT and in_frame:
            out.append(r.blit())
    return out


def key(b):
    return (b.res, b.frame, b.sx, b.sy)


def match(a, b, tol):
    return (a.res == b.res and a.frame == b.frame and a.sx == b.sx and a.sy == b.sy
            and abs(a.dx - b.dx) <= tol and abs(a.dy - b.dy) <= tol)


def main(argv):
    if len(argv) < 5:
        print(__doc__)
        return 2
    a = collect(argv[1], int(argv[2]))
    b = collect(argv[3], int(argv[4]))
    tol = int(argv[5]) if len(argv) > 5 else 6
    print(f"A={argv[1]} flip={argv[2]}  {len(a)} blits")
    print(f"B={argv[3]} flip={argv[4]}  {len(b)} blits")

    b_used = [False] * len(b)
    only_a = []
    for x in a:
        hit = -1
        for j, y in enumerate(b):
            if not b_used[j] and match(x, y, tol):
                hit = j
                break
        if hit >= 0:
            b_used[hit] = True
        else:
            only_a.append(x)
    only_b = [b[j] for j in range(len(b)) if not b_used[j]]

    def show(tag, lst):
        print(f"\n== {tag}: {len(lst)} draws ==")
        for x in sorted(lst, key=lambda b: (b.res, b.frame, b.dy)):
            print(f"  {VA.get(x.va,hex(x.va)):<7} res={x.res} fr={x.frame} "
                  f"dst=({x.dx},{x.dy}) {x.reqw}x{x.reqh} src=({x.sx},{x.sy}) "
                  f"o=({x.ox},{x.oy},{x.ow},{x.oh}) st=0x{x.state:x}")

    show("ONLY in A (" + argv[1].split('/')[-1] + ")", only_a)
    show("ONLY in B (" + argv[3].split('/')[-1] + ")", only_b)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
