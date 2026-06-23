#!/usr/bin/env python3
"""GENERAL input-driven replay converter — turn a retail real-play recording into
port replay traces that reproduce the WHOLE in-game session (cutscene/dialogue +
freeroam) frame-for-frame, off the recording's inputs ALONE (no hand-tuned nav).

  recording: <rec>.osr (draw stream, for the flip->tick map) + <rec>-input.jsonl
             (the OSS_INPUT_RECORD held-trace, flip-keyed {frame:F,keys:[sc,..]})
  port traces: a tick-axis held-trace (the held LEVELS) + a tick-axis ring
               input-trace (the discrete press EDGES), prefixed by a small fixed
               BOOT nav (title -> new game -> in-game; the recorder doesn't
               capture the title menu confirm — the one part that isn't yet
               input-derived, a known recorder gap).

Why it generalizes (no per-trace offset / nav):
 - the in-game SIM-TICK starts at enter_game on BOTH sides, so the recording's
   in-game inputs (tick-axis) align with the port's IN-GAME ticks directly
   (offset 0); a residual offset only appears if the port's cutscene cadence
   diverges from retail's (a gap the diff then reveals — no approximation).
 - the USER advances the dialogue + menus by spamming X (also Enter/V/ESC), so
   those PRESS EDGES map to the confirm ring id 0x24 and drive the dialogue
   input-driven; the held X drives the freeroam attack (axis 5); Z->9 (sword),
   directions->1-4 (dash double-tap) + held (walk).

Usage: sync_inputs.py <rec.osr> <rec-input.jsonl> [offset] [out_prefix]
Outputs: <out_prefix>-held.jsonl + <out_prefix>-nav.jsonl
         (default out_prefix = runs/sync/<rec-input basename without -input>)
"""
import sys
import os
import json

sys.path.insert(0, "tools/trace_studio2")
import osr

REC_OSR = sys.argv[1]
REC_INPUT = sys.argv[2]
OFFSET = int(sys.argv[3]) if len(sys.argv) > 3 else 0
if len(sys.argv) > 4:
    OUT = sys.argv[4]
else:
    base = os.path.basename(REC_INPUT).replace("-input.jsonl", "").replace(".jsonl", "")
    os.makedirs("runs/sync", exist_ok=True)
    OUT = f"runs/sync/{base}"

# scancode -> ring id (46a880: 0xc8->1 up / 0xcb->2 left / 0xd0->3 down / 0xcd->4
# right; Z 0x2c->9).  X/Enter/V/ESC -> the confirm id 0x24 (the dialogue+menu
# advance the USER spams; the freeroam reads the HELD X for the attack).
SC_RING = {
    0xc8: 1, 0xcb: 2, 0xd0: 3, 0xcd: 4,   # up/left/down/right (200/203/208/205)
    0x2c: 9,                              # Z -> sword toggle
    0x2d: 0x24, 0x1c: 0x24, 0x2f: 0x24, 0x01: 0x24,  # X/Enter/V/ESC -> confirm
}

# A fixed BOOT nav (frame-axis): title -> new game -> enter_game.  ~10 confirms +
# 2 menu-downs; the same for any new-game trace (NOT per-trace).  Lifted from
# runs/cutscene-verify/nav-full-errands.jsonl's boot prefix (frames <= 1100).
BOOT_NAV = [
    (640, 36), (720, 3), (745, 3), (805, 36), (855, 36), (885, 36),
    (915, 36), (945, 36), (975, 36), (1005, 36),
]

# recording flip -> sim-tick
flip2tick = {}
for r in osr.stream_records(REC_OSR, {osr.FRAMEBEG}):
    cf, ct, _ = r.framebeg()
    flip2tick.setdefault(cf, ct)
def tick_of(flip):
    f = flip
    while f >= 0 and f not in flip2tick:
        f -= 1
    return flip2tick.get(f)

# parse the recording's inputs
entries = []
with open(REC_INPUT) as fh:
    for line in fh:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        o = json.loads(line)
        entries.append((o["frame"], o.get("keys", [])))

held = {}          # tick -> held keys (the LEVELS; last entry per tick wins)
ring = []          # (tick, ring id) — the in-game press edges
prev = set()
for frame, keys in entries:
    T = tick_of(frame)
    kset = set(keys)
    if T is None or T == 0:        # title phase (sim_tick still 0): the BOOT nav drives it
        prev = kset
        continue
    Tp = T + OFFSET
    for sc in kset - prev:         # press edges -> ring ids
        if sc in SC_RING:
            ring.append((Tp, SC_RING[sc]))
    prev = kset
    held[Tp] = keys

# held-trace: tick-axis, the in-game held levels
with open(f"{OUT}-held.jsonl", "w") as fh:
    fh.write(f"# input-driven held-trace (in-game levels, {REC_INPUT}, offset={OFFSET})\n")
    for T in sorted(held):
        fh.write(json.dumps({"tick": T, "keys": held[T]}) + "\n")

# input-trace: the fixed BOOT nav (frame-axis) then the in-game ring presses (tick-axis)
ring.sort()
with open(f"{OUT}-nav.jsonl", "w") as fh:
    fh.write(f"# BOOT nav (frame-axis) + input-driven ring presses (Z=9, dirs=1-4, "
             f"confirm=0x24=X/Enter/V/ESC), tick-axis; {REC_INPUT}, offset={OFFSET}\n")
    for f, rid in BOOT_NAV:
        fh.write(json.dumps({"frame": f, "ids": [rid]}) + "\n")
    for T, rid in ring:
        fh.write(json.dumps({"tick": T, "ids": [rid]}) + "\n")

zdraws = [t for t, r in ring if r == 9]
confirms = sum(1 for t, r in ring if r == 0x24)
print(f"offset={OFFSET}  held entries={len(held)}  ring presses={len(ring)} "
      f"({confirms} confirms, Z draws at {zdraws})")
print(f"wrote {OUT}-held.jsonl + {OUT}-nav.jsonl")
