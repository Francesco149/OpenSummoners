#!/usr/bin/env python3
"""tools/trace_studio2/hud_probe.py — dump the complete HUD draw list at a tick.

Streams an .osr ONCE to a target sim-tick (the LAST flip of that tick = the
presented state) and prints, in draw order (seq), every BLIT and GDI TEXT op,
annotating which USER-marked HUD region each falls in.  This is the drawcall-
per-drawcall ground truth for the freeroam status HUD (errands tick 2413).

Usage: hud_probe.py <file.osr> <tick> [--all]
  (default filters to the HUD regions + res=0/text; --all dumps everything)
"""
from __future__ import annotations
import sys
sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402

# USER osr_notes HUD regions (errands tick 2413): [x,y,w,h]
REGIONS = {
    "top-left":     (0, 0, 294, 92),
    "bottom-left":  (0, 443, 140, 37),
    "bottom-right": (430, 433, 210, 47),
    "door":         (192, 402, 49, 67),
}


def region_of(x, y, w=1, h=1):
    hits = []
    for name, (rx, ry, rw, rh) in REGIONS.items():
        if x < rx + rw and rx < x + w and y < ry + rh and ry < y + h:
            hits.append(name)
    return ",".join(hits) if hits else "."


def main():
    path = sys.argv[1]
    target = int(sys.argv[2])
    dump_all = "--all" in sys.argv[3:]

    cur_flip = cur_tick = None
    cur_blits, cur_texts = [], []
    saved = None  # (flip, tick, blits, texts) for last flip of target tick

    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT, osr.TEXT}):
        if r.type == osr.FRAMEBEG:
            if cur_tick == target:
                saved = (cur_flip, cur_tick, cur_blits, cur_texts)
            flip, tick, _ = r.framebeg()
            cur_flip, cur_tick = flip, tick
            cur_blits, cur_texts = [], []
            if tick > target:
                break
        elif r.type == osr.BLIT and cur_tick is not None:
            cur_blits.append(r.blit())
        elif r.type == osr.TEXT and cur_tick is not None:
            cur_texts.append(r.text())
    if cur_tick == target and saved is None:
        saved = (cur_flip, cur_tick, cur_blits, cur_texts)

    if saved is None:
        print(f"tick {target} not found")
        return
    flip, tick, blits, texts = saved
    print(f"=== tick {tick} flip {flip}: {len(blits)} blits, {len(texts)} texts ===")

    # merge blits + texts into one ordered stream by seq
    items = []
    for b in blits:
        reg = region_of(b.dx, b.dy, b.reqw, b.reqh)
        items.append((b.seq, "BLIT", reg, b))
    for t in texts:
        reg = region_of(t.x, t.y, 40, 16)
        items.append((t.seq, "TEXT", reg, t))
    items.sort(key=lambda it: it[0])

    print("\n--- draws in HUD regions (or res=0) ---")
    for seq, kind, reg, o in items:
        if not dump_all and reg == "." and not (kind == "BLIT" and o.res == 0):
            continue
        if kind == "BLIT":
            prim = osr.BLIT_VA_NAME.get(o.va, hex(o.va))
            print(f"[{seq:4}] {reg:24} BLIT {prim:8} res={o.res:5} fr={o.frame:3} "
                  f"dst=({o.dx},{o.dy},{o.reqw},{o.reqh}) src=({o.sx},{o.sy}) "
                  f"st={o.state:#x} ckey={o.ckey:#x} bmode={o.bmode}")
        else:
            print(f"[{seq:4}] {reg:24} TEXT x={o.x} y={o.y} color={o.color:#08x} "
                  f"bk={o.bk_mode} font={o.font_ref} str={o.text!r}")


if __name__ == "__main__":
    main()
