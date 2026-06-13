#!/usr/bin/env python3
"""portrait_fade_probe.py — read the dialogue PORTRAIT blit's blend LUT per tick.

For one .osr + a portrait res, prints per sim-tick (last flip) the portrait
blit's blend_ref (the captured BLEND descriptor id) — the cross-tick identity of
the ramp step.  Maps each blend_ref to a ramp INDEX by reading the known fade-IN
sequence (idx 0,0,2,4,..,18 over the open ticks) so the fade-OUT can be checked
as the reverse ramp (18,16,..,2 then GONE).  Drawcall + LUT ground truth (no
guessing): the portrait dissolve, engine-quirk #108.

The portrait is matched by its dest ANCHOR (px,py) — the bust draws at exactly
the box's portrait offset, the same on both sides (the resource_id differs cross-
side: port res=0x7ef, retail res=0 via the resolver registry, so the anchor is
the stable identity).

Usage:
  portrait_fade_probe.py <file.osr> <px> <py> <fade_in_lo> <fade_in_hi> <fade_out_lo> <fade_out_hi>
"""
from __future__ import annotations

import sys

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402


def portrait_per_tick(path, px, py, tlo, thi):
    """tick -> (blend_ref, bmode, seq) for the LAST-flip portrait blit anchored
    at dst (px,py) (the highest-seq one) in [tlo, thi].  None if absent."""
    cur = None
    per_tick = {}
    last_of_flip = {}
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            if cur is not None and tlo <= cur <= thi:
                per_tick[cur] = last_of_flip.get(cur)   # overwrite -> last flip
            flip, cur, _ = r.framebeg()
            last_of_flip[cur] = None
            if cur > thi:
                break
        elif r.type == osr.BLIT and cur is not None and tlo <= cur <= thi:
            b = r.blit()
            if b.dx == px and b.dy == py:
                prev = last_of_flip.get(cur)
                # keep the highest-seq portrait blit of this (latest) flip
                if prev is None or b.seq >= prev[2]:
                    last_of_flip[cur] = (b.blend_ref, b.bmode, b.seq)
    if cur is not None and tlo <= cur <= thi:
        per_tick.setdefault(cur, last_of_flip.get(cur))
    return per_tick


def main(argv):
    if len(argv) < 8:
        print(__doc__)
        return 2
    path = argv[1]
    px, py = int(argv[2]), int(argv[3])
    fi_lo, fi_hi, fo_lo, fo_hi = (int(argv[4]), int(argv[5]),
                                  int(argv[6]), int(argv[7]))
    span_lo, span_hi = min(fi_lo, fo_lo), max(fi_hi, fo_hi)
    pt = portrait_per_tick(path, px, py, span_lo, span_hi)

    # Build blend_ref -> ramp idx from the fade-IN: the engine model is
    # idx = (fade*0x14)/500 with fade 0 held two ticks then +50/tick, so the
    # fade-in ticks read 0,0,2,4,..,18.  Map each distinct blend_ref to that idx.
    ref_to_idx = {}
    fi_ticks = [t for t in range(fi_lo, fi_hi + 1) if pt.get(t) and pt[t][0] != 0]
    # assign idx by the model sequence (skip the duplicate first hold)
    seq = [0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18]
    for i, t in enumerate(fi_ticks):
        ref = pt[t][0]
        if i < len(seq):
            ref_to_idx.setdefault(ref, seq[i])

    def label(v, gone_txt):
        if v is None:
            return f"(no portrait — {gone_txt})"
        ref = v[0]
        if ref == 0:
            return "blend_ref=0          OPAQUE (plain keyed blit)"
        return f"blend_ref={ref:#010x} idx={ref_to_idx.get(ref, '?')}"

    print(f"== {path.rsplit('/',1)[-1]} portrait@({px},{py}) ==")
    print("  FADE-IN  [%d,%d]:" % (fi_lo, fi_hi))
    for t in range(fi_lo, fi_hi + 1):
        print(f"    tick {t}: {label(pt.get(t), 'opaque/absent')}")
    print("  FADE-OUT [%d,%d]:" % (fo_lo, fo_hi))
    for t in range(fo_lo, fo_hi + 1):
        print(f"    tick {t}: {label(pt.get(t), 'GONE/absent')}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
