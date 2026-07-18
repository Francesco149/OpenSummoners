#!/usr/bin/env python3
"""warp.py <target_room> [xlo xhi step] — WITHIN-AREA fast-travel via the baked door-driver.

Hijacks ALL the current room's exit slots -> target (so any door then leads there), then
sweeps the floor (teleport + `doorenter` across x) until she transitions.  Same-area targets
only: the door-enter LOADS the target from the resident area W-map; a cross-AREA target is not
resident and would fault, so cross-region routing = chain within-area warps through the real
boundary portals (BFS the `rooms` graph).  Uses only the trainer socket (no frida).

Env OSS_TRAINER_REMOTE (host:port), else cutestation.soy:7777.
"""
import os, sys, time, json, socket
TRAINER = os.environ.get("OSS_TRAINER_REMOTE", "cutestation.soy:7777")


def ts(o, t=15.0):
    h, _, p = TRAINER.partition(":")
    s = socket.socket(); s.settimeout(t); s.connect((h, int(p)))
    s.sendall((json.dumps(o) + "\n").encode()); b = b""
    while not b.endswith(b"\n"):
        c = s.recv(65536)
        if not c:
            break
        b += c
    s.close(); return json.loads(b.decode())


def main():
    if len(sys.argv) < 2:
        print("usage: warp.py <target_room> [xlo xhi step]"); return 2
    target = int(sys.argv[1], 0)
    xlo = int(sys.argv[2], 0) if len(sys.argv) > 2 else 4000
    xhi = int(sys.argv[3], 0) if len(sys.argv) > 3 else 200000
    step = int(sys.argv[4], 0) if len(sys.argv) > 4 else 1800

    m = ts({"cmd": "map"}); r0 = m.get("room_key"); area = m.get("area")
    if r0 == target:
        print(f"already in {target}"); return 0
    exits = m.get("exits", [])
    print(f"in {r0} (area {area}); {len(exits)} exits; warp -> {target}", flush=True)
    # hijack every exit slot -> target so whichever door she triggers leads there (within-area)
    for slot in range(max(len(exits), 2)):
        ts({"cmd": "hijack", "slot": slot, "target": target})
    y = ts({"cmd": "player"}).get("player", {}).get("world_y", 38399)
    x = xlo
    while x <= xhi:
        ts({"cmd": "teleport", "x": x, "y": y}); time.sleep(0.18)
        ts({"cmd": "doorenter"}); time.sleep(0.55)
        rk = ts({"cmd": "map"}).get("room_key")
        if rk != r0:
            ok = (rk == target)
            print(f"transition at x={x}: {r0} -> {rk}" + ("  ARRIVED" if ok else
                  f"  (wanted {target}!)"), flush=True)
            return 0 if ok else 1
        x += step
    print(f"no door triggered in x[{xlo},{xhi}] (wrong y/range?)", flush=True); return 1


if __name__ == "__main__":
    raise SystemExit(main())
