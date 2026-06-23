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

held_out = []   # (tick, keys without Z)
z_ring = []     # ticks of Z onsets
prev_z = False
for frame, keys in entries:
    T = tick_of(frame)
    if T is None or T < FR_START:
        prev_z = (44 in keys)
        continue
    Tp = T + OFFSET
    z_now = 44 in keys
    if z_now and not prev_z:
        z_ring.append(Tp)
    prev_z = z_now
    hk = [k for k in keys if k != 44]   # Z handled by the ring
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

# merge the cutscene nav (drop its synthetic Z draw) + the recording's Z ring presses
nav_lines = []
with open(NAV) as fh:
    for line in fh:
        s = line.strip()
        if not s or s.startswith("#"):
            nav_lines.append(line.rstrip("\n")); continue
        o = json.loads(s)
        if o.get("ids") == [9]:   # drop the synthetic Z draw
            continue
        nav_lines.append(line.rstrip("\n"))
# append the real Z ring presses (tick-axis), sorted into place at the end (the nav is < these ticks)
z_lines = [json.dumps({"tick": t, "ids": [9]}) for t in sorted(z_ring)]
with open("runs/sword-trail/sync-nav.jsonl", "w") as fh:
    fh.write("\n".join(nav_lines) + "\n")
    fh.write("# real Z draws/sheathes from the recording (id 9, tick-axis)\n")
    fh.write("\n".join(z_lines) + "\n")

print(f"offset={OFFSET}  held entries={len(held_sorted)}  Z ring presses={len(z_ring)} at ticks {sorted(z_ring)}")
print("wrote runs/sword-trail/sync-held.jsonl + sync-nav.jsonl")
