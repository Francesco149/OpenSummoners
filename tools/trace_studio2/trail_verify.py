#!/usr/bin/env python3
"""Verify the ported UP-attack sword-tip TRAIL (res 0x40b) bit-exact vs the
recording.  Extracts every trail sparkle RELATIVE to the up-attack body (res
0x572) per sim-tick — the comparable frame since the two captures place Arche at
different world/screen positions — plus the additive blend descriptor's LUT md5.

The recording's tip-arc (sword2.osr, the first up-attack) is body-relative:
  tick 3889 fr24 (+20,+69)(+26,+68) / 3890 (+32,+66)(+37,+64) / 3891 (+42,+61)(+47,+58)
  / 3892 [body +10 cel-lean] (+42,+53)(+45,+49) / ... — and blend_ref 39 LUT md5
  727d856f (constant).  A bit-exact port reproduces the same body-relative fr24
  offsets + the same blend LUT.

Usage: trail_verify.py <port.osr> [tick_lo tick_hi]   (default 2150 2162 = the
port's first up-attack; the recording's own arc is in freeroam-sword-attack.md)."""
import sys
import hashlib
sys.path.insert(0, "tools/trace_studio2")
import osr

OSR = sys.argv[1] if len(sys.argv) > 1 else "/mnt/c/oss-osr/port-trail.osr"
tlo = int(sys.argv[2]) if len(sys.argv) > 2 else 2150
thi = int(sys.argv[3]) if len(sys.argv) > 3 else 2162

RETAIL_TRAIL_LUT = "727d856f"   # sword2.osr blend_ref 39 (the constant trail blend)

blends = {}
for r in osr.stream_records(OSR, {osr.BLEND}):
    b = r.blend()
    blends[b.blend_ref] = hashlib.md5(b.lut).hexdigest()[:8] if b.lut else "----"

per = {}
ct = None
for r in osr.stream_records(OSR, {osr.FRAMEBEG, osr.BLIT}):
    if r.type == osr.FRAMEBEG:
        cf, ct, _ = r.framebeg()
        if ct > thi:
            break
    elif r.type == osr.BLIT and ct is not None and tlo <= ct <= thi:
        b = r.blit()
        if b.res in (0x572, 0x40b):
            per.setdefault(ct, []).append(b)

trail_refs = {b.blend_ref for t in per for b in per[t] if b.res == 0x40b}
print("=== BLEND (vs retail trail LUT 727d856f) ===")
for ref in sorted(trail_refs):
    lut = blends.get(ref)
    print(f"  port trail blend_ref={ref} LUT={lut}  "
          f"{'== retail ✓' if lut == RETAIL_TRAIL_LUT else '!= retail ✗'}")

print("=== POSITIONS (fr24 sparkle dst RELATIVE to the up-attack body) ===")
for t in sorted(per):
    body = [b for b in per[t] if b.res == 0x572]
    if not body:
        continue
    bx, by = body[0].dx, body[0].dy
    new = sorted(set((b.dx - bx, b.dy - by) for b in per[t]
                     if b.res == 0x40b and b.frame == 24))
    print(f"  tick {t}: body=({bx},{by}) fr={body[0].frame}  fr24-rel={new}")
