#!/usr/bin/env python3
"""tools/trace_studio2/text_timeline.py — distinct GDI text strings over a tick range.

Streams an .osr ONCE and reports every distinct TextOutA string seen in a sim-tick
range, with its tick span and a representative (x,y,color,font).  Dialogue lines
(sentences, the box body/name) separate cleanly from HUD text (HP/MP numbers,
top-left).  The drawcall ground truth for "what dialogue plays and when".

Usage: text_timeline.py <file.osr> <tick_lo> <tick_hi>
"""
from __future__ import annotations
import sys
sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402


def main():
    path = sys.argv[1]
    lo, hi = int(sys.argv[2]), int(sys.argv[3])
    cur_tick = None
    seen = {}   # str -> [first_tick, last_tick, x, y, color, font, count]
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.TEXT}):
        if r.type == osr.FRAMEBEG:
            _, cur_tick, _ = r.framebeg()
            if cur_tick > hi:
                break
        elif r.type == osr.TEXT and cur_tick is not None and lo <= cur_tick <= hi:
            t = r.text()
            s = t.text
            if s not in seen:
                seen[s] = [cur_tick, cur_tick, t.x, t.y, t.color, t.font_ref, 0]
            e = seen[s]
            e[1] = cur_tick
            e[6] += 1

    print(f"=== distinct TEXT strings in ticks [{lo},{hi}] ===")
    for s, (ft, lt, x, y, color, font, n) in sorted(seen.items(), key=lambda kv: kv[1][0]):
        loc = "HUD?" if (y < 30 and x > 150) else "    "
        print(f"[{ft:5}-{lt:5}] {loc} x={x:4} y={y:4} color={color:#08x} font={font} "
              f"n={n:4} {s!r}")


if __name__ == "__main__":
    main()
