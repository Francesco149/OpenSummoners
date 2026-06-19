#!/usr/bin/env python3
"""Measure the REAL dialogue body-text row layout from an .osr (the line-dependent
spacing RE).  Streams {FRAMEBEG, TEXT} only (OOM-safe on the 1.9 GB retail.osr).

Body text colours: main 0x3e537d, shadow 0xa8b9cc.  Name: main 0xffffff, shadow
0x455f7b.  Each body row draws at baseline y=B (main + x+1 shadow) and y=B+1
(y+1 shadow), so the row BASELINES are the y's whose (y+1) is also present.

box_y is recovered as name_baseline + 9 (NAME_DY = -9), giving each row's
box-RELATIVE offset.  Per stable line we print the body row count, the per-row
box-relative offsets, and the pitch — so 1-row / 2-row / 3-row spacing compares
directly.

Usage: dlg_text_probe.py <file.osr> <lo_tick> <hi_tick> [--lines]
  default: per-tick rows;  --lines: collapse stable lines, one row per line.
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
import osr as O

BODY = {0x3e537d, 0xa8b9cc}
NAME = {0xffffff, 0x455f7b}


def baselines(ys):
    s = set(ys)
    return sorted(y for y in s if (y + 1) in s)


def analyze(rows):
    """rows: list of (seq,x,y,color,text).  Return (name_base, [body_base...],
    body_first_text)."""
    name_ys = [y for (_, _, y, c, _) in rows if c in NAME]
    body = [(y, t) for (_, _, y, c, t) in rows if c in BODY]
    body_ys = [y for (y, _) in body]
    nb = baselines(name_ys)
    bb = baselines(body_ys)
    # the first char of each body baseline row (for identity)
    firsts = {}
    for y, t in body:
        if y in bb and (y not in firsts or len(t) > len(firsts[y])):
            firsts[y] = t
    return (nb[0] if nb else None, bb, [firsts.get(y, "") for y in bb])


def main():
    path = sys.argv[1]
    lo = int(sys.argv[2]); hi = int(sys.argv[3])
    lines_mode = "--lines" in sys.argv[4:]

    cur_tick = cur_flip = None
    rows = []
    last_sig = None  # (name_base, tuple(bb-rel), tuple(firsts)) for collapsing

    def report():
        nonlocal last_sig
        if cur_tick is None or not rows:
            return
        nb, bb, firsts = analyze(rows)
        if nb is None or not bb:
            return
        box_y = nb + 9
        rel = [y - box_y for y in bb]
        pitches = [bb[i+1]-bb[i] for i in range(len(bb)-1)]
        sig = (len(bb), tuple(rel), tuple(firsts))
        if lines_mode and sig == last_sig:
            return
        last_sig = sig
        fr = " ".join(f"{f!r}" for f in firsts)
        print(f"tick {cur_tick:5d} box_y={box_y:4d}  rows={len(bb)}  "
              f"rel_off={rel}  pitch={pitches}  firsts: {fr}")

    for r in O.stream_records(path, {O.FRAMEBEG, O.TEXT}):
        if r.type == O.FRAMEBEG:
            if cur_tick is not None and lo <= cur_tick <= hi:
                report()
            flip, tick, _ = r.framebeg()
            cur_flip, cur_tick = flip, tick
            rows = []
            if tick > hi:
                break
        elif r.type == O.TEXT and cur_tick is not None and lo <= cur_tick <= hi:
            t = r.text()
            rows.append((t.seq, t.x, t.y, t.color, t.text))
    if cur_tick is not None and lo <= cur_tick <= hi:
        report()


if __name__ == "__main__":
    main()
