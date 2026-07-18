#!/usr/bin/env python3
"""warp.py <target_room> — CROSS-REGION fast-travel via the room graph + direct door positions.

Each hop: BFS the live `rooms` graph from the current room to the target, take the next hop `nxt`
(always a REAL exit of the current room), then teleport STRAIGHT onto that exit's door anchor and
fire it — `door slot=N enter=true`, the door position read from the loaded scene, NO floor sweep.
Re-plans from the actual room after every hop.  Falls back to the old hijack-all + teleport-sweep
for any exit whose door anchor isn't loaded / didn't fire.  Socket only, no frida.

⚠ COMBAT/SEEN GATES: a room with mobs (or a portal you've never used) blocks the door unless the
`warpgate` patch is ON (skips the combat-proximity + never-seen + hold-ramp gates).  This script
enables it at start (harmless if the trainer predates the patch — the error is ignored) and leaves
it on.  Env OSS_TRAINER_REMOTE (host:port), else cutestation.soy:7777.
"""
import os, sys, time, json, socket
from collections import deque
TRAINER = os.environ.get("OSS_TRAINER_REMOTE", "cutestation.soy:7777")
XLO, XHI, STEP = 4000, 205000, 1800
SETTLE, TAP_WAIT = 0.16, 0.55
HOP_TIMEOUT = 8.0


def ts(o, t=20.0):
    h, _, p = TRAINER.partition(":")
    s = socket.socket(); s.settimeout(t); s.connect((h, int(p)))
    s.sendall((json.dumps(o) + "\n").encode()); b = b""
    while not b.endswith(b"\n"):
        c = s.recv(65536)
        if not c:
            break
        b += c
    s.close(); return json.loads(b.decode())


def area_of(room):
    return room // 1000


def graph():
    return {r["key"]: r for r in ts({"cmd": "rooms"}).get("rooms", [])}


def bfs(g, start, target):
    q = deque([[start]]); seen = {start}
    while q:
        path = q.popleft()
        if path[-1] == target:
            return path
        for t in g.get(path[-1], {}).get("exits", []):
            if t == 999999 or t in seen or t not in g:
                continue
            seen.add(t); q.append(path + [t])
    return None


def room_now():
    m = ts({"cmd": "map"}); return m.get("room_key"), m


def y_now():
    return ts({"cmd": "player"}).get("player", {}).get("world_y", 38399)


def wait_room(r0, timeout=HOP_TIMEOUT):
    t0 = time.time()
    while time.time() - t0 < timeout:
        time.sleep(0.4)
        rk = room_now()[0]
        if rk and rk != r0:
            return rk
    return None


def direct_door(nxt, m, r0):
    """Try EVERY loaded exit targeting `nxt` (a room may have several doors to it, on different
    levels): teleport onto each door + fire it, nudging away between tries so the next door reads
    as 'changed' to the handler.  Returns the new room or None if none fired."""
    cands = [e for e in m.get("exits", []) if e.get("target_room") == nxt and e.get("door_x", -1) >= 0]
    for e in cands:
        ts({"cmd": "door", "slot": e["slot"], "enter": True})
        got = wait_room(r0, timeout=5.0)
        if got is not None:
            return got
        ts({"cmd": "teleport", "x": 100000, "y": y_now()})   # nudge off the door -> next reads 'changed'
        time.sleep(0.3)
    return None


# ── the old sweep fallback (used only when no door anchor is loaded for the hop) ──
def _tap(x, y):
    ts({"cmd": "teleport", "x": x, "y": y}); time.sleep(SETTLE)
    ts({"cmd": "doorenter"}); time.sleep(TAP_WAIT)
    return room_now()[0]


def within_warp(target):
    r0, m = room_now()
    for slot in range(max(len(m.get("exits", [])), 2)):
        ts({"cmd": "hijack", "slot": slot, "target": target})
    y = y_now(); x = XLO
    while x <= XHI:
        rk = _tap(x, y)
        if rk != r0:
            return rk
        x += STEP
    return None


def boundary_cross(target_area):
    r0, _ = room_now(); y = y_now(); x = XLO
    while x <= XHI:
        rk = _tap(x, y)
        if rk != r0:
            if area_of(rk) == target_area:
                return rk
            within_warp(r0)
            y = y_now(); x += STEP
        else:
            x += STEP
    return None


def main():
    if len(sys.argv) < 2:
        print("usage: warp.py <target_room>"); return 2
    target = int(sys.argv[1], 0)
    try:
        ts({"cmd": "warpgate", "on": True})          # skip combat/seen/hold gates (ignore if absent)
    except Exception:
        pass
    g = graph()
    if target not in g:
        print(f"target {target} not in the room table"); return 1
    for _ in range(40):
        cur, m = room_now()
        if cur == target:
            print(f"ARRIVED at {target}"); return 0
        path = bfs(g, cur, target)
        if not path or len(path) < 2:
            print(f"no path from {cur} to {target}"); return 1
        nxt = path[1]
        kind = "same-area" if area_of(nxt) == area_of(cur) else "BOUNDARY"
        print(f"at {cur}; hop -> {nxt} ({kind}); {len(path)-1} hops left", flush=True)
        got = direct_door(nxt, m, cur)              # fast: straight to the door
        if got is None:                             # anchor not loaded -> old sweep
            print("  (no loaded door anchor; sweeping)", flush=True)
            got = within_warp(nxt) if area_of(nxt) == area_of(cur) else boundary_cross(area_of(nxt))
        if got is None:
            print(f"  stuck: no door out of {cur} toward {nxt}"); return 1
        print(f"  -> {got}", flush=True)
    print("too many hops (giving up)"); return 1


if __name__ == "__main__":
    raise SystemExit(main())
