#!/usr/bin/env python3
"""
tools/extract/cursor_frame_match.py — prove the new-game selection cursor's
sprite bank by matching the Lizsoft atlas 0x455's per-cell trim bboxes against
the live --box-probe frame metrics.

THE FINDING (ckpt 42, quirk #68): the new-game config menu's selection cursor
(the drooping gold feather/quill + soft white shadow) is PE resource 0x455
(sotesd.dll, slot 43 = AR_SPR_FONT_TEX_455), frames 16-19 — the SAME bank/slot/
frames the geometry port (src/newgame_cursor.c) already targeted.

The earlier "0x455 sweep matches nothing" (HANDOFF ckpt 41) was a decode-
ORIENTATION error.  The Lizsoft DATA blob's pixel data is a BMP-style BOTTOM-UP
bitmap; the engine slices cells bottom-up.  Read TOP-DOWN, frames 16-19 land on
the row-4 ► chevrons (9x17) — which look nothing like the 22x41 vine, hence the
false negative.  Read BOTTOM-UP, frames 16-19 are the feather (22x41), and their
trimmed bounding boxes match the live --box-probe EXACTLY:

    frame 17 -> 22x41 @ (4,3)   (probe: 22x41 @ (4,3))
    frame 18 -> 22x40 @ (4,4)   (probe: 22x40 @ (4,4))
    frame 19 -> 22x41 @ (4,3)   (probe: 22x41 @ (4,3))

The probe's res_id=0x3e8 readout (slot+0x40) is a reused/garbage marker — PE
resource 0x3e8 is an 80x352 portrait in sotesd, a WMV in sotesw, absent in
sotesp.  The reliable signal is the per-frame trim size (entries[frameSel]->frec
+0x14/+0x18), which this script reproduces from the raw atlas.

The atlas: 128x288 px = a 4-col x 6-row grid of 32x48 cells = 24 frames.
Transparent background = palette index 0 (RGB ~(0,0,3), colour-keyed at blit).

Usage:
    python3 tools/extract/cursor_frame_match.py runs/extract/sotesd/type=DATA/1109.bin
"""
from __future__ import annotations

import sys

W, H = 128, 288
COLS, ROWS = 4, 6
CW, CH = 32, 48
PIX_OFF = 1142          # Lizsoft header: 32 magic + 1024 pal + 64 + 14 + 8

# The live --box-probe metrics for the cursor frames (goldens/
# retail-newgame-box-cells.jsonl), as (w, h, offx, offy).  Frame 16 is the
# idx-0 capture where the probe recorded w/h=null, so it is omitted.
PROBE = {17: (22, 41, 4, 3), 18: (22, 40, 4, 4), 19: (22, 41, 4, 3)}


def cell_bbox(pix: bytes, cx: int, cy: int, transp: int = 0):
    minx = miny = 1 << 30
    maxx = maxy = -1
    for y in range(CH):
        base = (cy + y) * W
        for x in range(CW):
            if pix[base + cx + x] != transp:
                minx = min(minx, x); maxx = max(maxx, x)
                miny = min(miny, y); maxy = max(maxy, y)
    if maxx < 0:
        return None
    return (maxx - minx + 1, maxy - miny + 1, minx, miny)   # w, h, offx, offy


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__)
        return 2
    blob = open(argv[1], "rb").read()
    pix = blob[PIX_OFF:]
    if len(pix) != W * H:
        print(f"!! pixel area {len(pix)} != {W*H} (expected 128x288)")
        return 1
    # bottom-up: engine reads the BMP bottom row first.
    rows = [pix[y * W:(y + 1) * W] for y in range(H)][::-1]
    flat = b"".join(rows)

    print("frame  bbox(w,h,offx,offy)   probe            match")
    ok = True
    for f in range(24):
        r, c = divmod(f, COLS)
        bb = cell_bbox(flat, c * CW, r * CH)
        tag = ""
        if f in PROBE:
            exp = PROBE[f]
            hit = bb == exp
            ok = ok and hit
            tag = f"   probe {exp}   {'MATCH' if hit else 'MISMATCH'}"
        print(f"  {f:2d}   {bb}{tag}")
    print()
    print("RESULT:", "all probe frames match (bank = 0x455 slot 43, bottom-up)"
          if ok else "MISMATCH — re-check orientation/cell size")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
