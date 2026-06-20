#!/usr/bin/env python3
"""errands_dlg_probe.py — Arche pos + dialogue-box blits + line-3 icons at given ticks.

Per requested tick (last flip): Arche body (res 0x570) dst; every BLIT with dst in
the dialogue band (y 150..310) in draw order (the box sprite, the bubble/name plate,
the inline button icons in the tutorial line).  One stream pass.

Usage: errands_dlg_probe.py <file.osr> <tick> [tick ...]
"""
from __future__ import annotations
import sys
sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402

ARCHE = 0x570
YLO, YHI = 150, 310


def main():
    path = sys.argv[1]
    ticks = sorted(int(a) for a in sys.argv[2:])
    tmax = ticks[-1]
    cur_tick = None
    cur = []
    per = {}
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            if cur_tick in ticks:
                per[cur_tick] = cur
            _, cur_tick, _ = r.framebeg()
            cur = []
            if cur_tick > tmax:
                break
        elif r.type == osr.BLIT and cur_tick is not None:
            cur.append(r.blit())
    if cur_tick in ticks:
        per.setdefault(cur_tick, cur)

    for tk in ticks:
        print(f"\n=== tick {tk} ===")
        blits = per.get(tk, [])
        arche = [b for b in blits if b.res == ARCHE]
        if arche:
            b = arche[-1]
            print(f"  Arche res0x570 fr={b.frame} dst=({b.dx},{b.dy},{b.reqw},{b.reqh})")
        band = [b for b in blits if YLO <= b.dy <= YHI]
        band.sort(key=lambda b: b.seq)
        for b in band:
            prim = osr.BLIT_VA_NAME.get(b.va, hex(b.va))
            print(f"  [{b.seq:4}] {prim:8} res={b.res:5} fr={b.frame:3} "
                  f"dst=({b.dx},{b.dy},{b.reqw},{b.reqh}) src=({b.sx},{b.sy}) "
                  f"st={b.state:#x} ck={b.ckey:#x} bm={b.bmode}")


if __name__ == "__main__":
    main()
