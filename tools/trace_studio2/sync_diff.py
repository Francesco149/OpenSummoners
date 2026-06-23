#!/usr/bin/env python3
"""Tick-by-tick position diff between a PORT replay and the RECORDING, for
frame-locking the freeroam 1:1.  For each recording sim-tick T it reports
Arche's body screen-x on both sides (port sampled at T+offset) and a background
floor-tile screen-x (the camera proxy), so a divergence is attributable to
MOVEMENT (body-x differs, camera same) vs CAMERA (tile differs).  Flags the
FIRST tick whose body-x delta exceeds `tol` — the divergence to root-cause.

Usage: sync_diff.py <port.osr> <rec.osr> <offset> [rec_lo rec_hi] [tol]
"""
import sys
sys.path.insert(0, "tools/trace_studio2")
import osr

PORT = sys.argv[1]
REC = sys.argv[2]
OFFSET = int(sys.argv[3])
LO = int(sys.argv[4]) if len(sys.argv) > 4 else 1807
HI = int(sys.argv[5]) if len(sys.argv) > 5 else 3920
TOL = int(sys.argv[6]) if len(sys.argv) > 6 else 2

BODY = {0x570, 0x571, 0x572}   # Arche freeroam body banks
FLOOR = 0x769                   # a background floor tile (camera proxy)

def scan(path, lo, hi):
    """tick -> (body_x, floor_x).  Takes the FIRST body/floor blit per tick."""
    out = {}
    ct = None
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.BLIT}):
        if r.type == osr.FRAMEBEG:
            cf, ct, _ = r.framebeg()
            if ct > hi:
                break
        elif r.type == osr.BLIT and ct is not None and lo <= ct <= hi:
            b = r.blit()
            cur = out.setdefault(ct, [None, None])
            if b.res in BODY and cur[0] is None:
                cur[0] = b.dx
            elif b.res == FLOOR and cur[1] is None:
                cur[1] = b.dx
    return out

rec = scan(REC, LO, HI)
port = scan(PORT, LO + OFFSET, HI + OFFSET)

print(f"rec_tick  rec_x port_x  dBODY | rec_flr port_flr dCAM")
first = None
for T in sorted(rec):
    rb, rf = rec[T]
    pe = port.get(T + OFFSET)
    if rb is None or pe is None or pe[0] is None:
        continue
    pb, pf = pe
    db = pb - rb
    dc = (pf - rf) if (rf is not None and pf is not None) else None
    flag = ""
    if abs(db) > TOL and first is None:
        first = T
        flag = "  <== FIRST BODY DIVERGENCE"
    cam = f"{rf if rf is not None else '-':>4} {pf if pf is not None else '-':>5} {dc:+d}" if dc is not None else "  (no tile)"
    if T % 5 == 0 or flag:
        print(f"  {T:>5}  {rb:>4} {pb:>5}  {db:+4d} | {cam}{flag}")
if first is not None:
    print(f"\nFIRST body divergence > {TOL}px at rec_tick {first} (port {first+OFFSET})")
else:
    print(f"\nNo body divergence > {TOL}px in [{LO},{HI}] — frame-locked.")
