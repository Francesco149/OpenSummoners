#!/usr/bin/env python3
"""Convert the recording's real-play input trace (sword2-input.jsonl, FLIP-keyed)
into TICK-axis port replay traces, so the port reproduces sword2.osr's freeroam
FRAME-FOR-FRAME (the port's freeroam flip rate differs from retail's, so a flip
replay drifts; the sim-tick is deterministic on both).

  sword2-input.jsonl {frame:F,keys:[..]}  --(sword2.osr flip->tick)-->  tick T
  T_port = T + offset                     (offset aligns the freeroam start)

Splits the held set: Z (scancode 44) onsets -> ring presses (id 9) merged into
the cutscene nav; the directions + X (held) -> a tick-axis held-trace.

Usage: sync_inputs.py <offset> [fr_start_tick]
  offset      = port_freeroam_tick - recording_freeroam_tick (tune by capture)
  fr_start    = drop recording inputs before this tick (default 1700, pre-game)
Outputs: runs/sword-trail/sync-held.jsonl  +  runs/sword-trail/sync-nav.jsonl
"""
import sys, json
sys.path.insert(0, "tools/trace_studio2")
import osr

OFFSET = int(sys.argv[1]) if len(sys.argv) > 1 else 0
FR_START = int(sys.argv[2]) if len(sys.argv) > 2 else 1700
REC_OSR = "/mnt/c/oss-osr/sword2.osr"
REC_INPUT = "/mnt/c/oss-osr/sword2-input.jsonl"
NAV = "runs/sword-trail/trail-nav.jsonl"   # the cutscene nav (reaches freeroam); its synth Z is dropped

# flip -> tick map from the recording
flip2tick = {}
for r in osr.stream_records(REC_OSR, {osr.FRAMEBEG}):
    cf, ct, _ = r.framebeg()
    flip2tick.setdefault(cf, ct)

def tick_of(flip):
    # nearest flip at-or-below
    f = flip
    while f >= 0 and f not in flip2tick:
        f -= 1
    return flip2tick.get(f)

# parse the recording inputs
entries = []
with open(REC_INPUT) as fh:
    for line in fh:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        o = json.loads(line)
        entries.append((o["frame"], o.get("keys", [])))

# scancode -> ring id (46a880: 0xc8->1 up / 0xd0->3 down / 0xcb->2 left / 0xcd->4 right;
# Z 0x2c->9).  The ring records the discrete PRESS EDGES; the dash double-tap (and any
# other ring-based wall-clock check) reads them.  The held LEVELS go in the held-trace.
SC_RING = {44: 9, 200: 1, 208: 3, 203: 2, 205: 4}

held_out = []      # (tick, held keys without Z)
ring_press = []    # (tick, ring id) — Z + direction press edges
prev_keys = set()
for frame, keys in entries:
    T = tick_of(frame)
    kset = set(keys)
    if T is None or T < FR_START:
        prev_keys = kset
        continue
    Tp = T + OFFSET
    for sc, rid in SC_RING.items():
        if sc in kset and sc not in prev_keys:   # press edge
            ring_press.append((Tp, rid))
    prev_keys = kset
    hk = [k for k in keys if k != 44]   # Z handled by the ring, not a held axis
    held_out.append((Tp, hk))

# dedupe held by tick (last wins), keep monotonic
seen = {}
for Tp, hk in held_out:
    seen[Tp] = hk
held_sorted = sorted(seen.items())

with open("runs/sword-trail/sync-held.jsonl", "w") as fh:
    fh.write("# tick-axis held-trace (recording moveset, sword2-input.jsonl converted, "
             f"offset={OFFSET})\n")
    for Tp, hk in held_sorted:
        fh.write(json.dumps({"tick": Tp, "keys": hk}) + "\n")

# merge the cutscene nav (frame-axis boot + tick-axis confirms; drop its synthetic Z)
# with the recording's ring presses (Z + directions), all tick-axis entries sorted.
frame_entries = []   # (frame, obj) — boot menus, kept in order, first
tick_confirms = []   # (tick, obj) — the cutscene/dialogue confirms
with open(NAV) as fh:
    for line in fh:
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        o = json.loads(s)
        if "frame" in o:
            frame_entries.append((o["frame"], o))
        elif "tick" in o:
            if o.get("ids") == [9]:   # drop the synthetic Z draw (replaced by the real ring presses)
                continue
            tick_confirms.append((o["tick"], o))

# all tick-axis entries: the cutscene/dialogue confirms + the recording's ring presses
tick_all = list(tick_confirms)
for t, rid in ring_press:
    tick_all.append((t, {"tick": t, "ids": [rid]}))
tick_all.sort(key=lambda x: x[0])

with open("runs/sword-trail/sync-nav.jsonl", "w") as fh:
    fh.write("# cutscene nav (frame-axis boot, then tick-axis confirms) + the recording's\n"
             f"# ring presses (Z=9, dirs 1-4), all tick-axis sorted; offset={OFFSET}\n")
    for f, o in sorted(frame_entries, key=lambda x: x[0]):
        fh.write(json.dumps(o) + "\n")
    for t, o in tick_all:
        fh.write(json.dumps(o) + "\n")

print(f"offset={OFFSET}  held entries={len(held_sorted)}  ring presses={len(ring_press)} "
      f"(Z draws at {[t for t,r in ring_press if r==9]})")
print("wrote runs/sword-trail/sync-held.jsonl + sync-nav.jsonl")
