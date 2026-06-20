#!/usr/bin/env python3
"""tools/trace_studio2/dlg_reconstruct.py — reconstruct the visible dialogue text at given ticks.

For each requested sim-tick (the LAST flip of that tick), groups TEXT records into
rows (by y) and concatenates the glyphs left-to-right (inserting spaces for x gaps),
so multi-row dialogue reads back as sentences.  The drawcall ground truth for the
exact dialogue strings + their layout (name row + body rows).

Usage: dlg_reconstruct.py <file.osr> <tick> [tick ...]
"""
from __future__ import annotations
import sys
sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402


def reconstruct(texts):
    # group by y (row), then sort by x; a glyph step is ~7px, so a gap > ~10 => space
    rows = {}
    for t in texts:
        rows.setdefault(t.y, []).append((t.x, t.text, t.color, t.font_ref))
    out = []
    for y in sorted(rows):
        glyphs = sorted(rows[y])
        line = ""
        prevx = None
        for x, s, color, font in glyphs:
            if prevx is not None:
                gap = x - prevx
                if gap > 10:
                    line += " " * max(1, round(gap / 7) - 1)
            line += s
            prevx = x
        color = glyphs[0][2]
        font = glyphs[0][3]
        out.append((y, glyphs[0][0], color, font, line))
    return out


def main():
    path = sys.argv[1]
    ticks = sorted(int(a) for a in sys.argv[2:])
    target_max = ticks[-1]
    cur_flip = cur_tick = None
    cur_texts = []
    per_tick = {}
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.TEXT}):
        if r.type == osr.FRAMEBEG:
            if cur_tick in ticks:
                per_tick[cur_tick] = cur_texts          # keep last flip
            _, cur_tick, _ = r.framebeg()
            cur_texts = []
            if cur_tick > target_max:
                break
        elif r.type == osr.TEXT and cur_tick is not None:
            cur_texts.append(r.text())
    if cur_tick in ticks:
        per_tick.setdefault(cur_tick, cur_texts)

    for tk in ticks:
        print(f"\n=== tick {tk} ===")
        if tk not in per_tick:
            print("  (not found)")
            continue
        for y, x0, color, font, line in reconstruct(per_tick[tk]):
            print(f"  y={y:4} x0={x0:4} color={color:#08x} font={font}  |{line}|")


if __name__ == "__main__":
    main()
