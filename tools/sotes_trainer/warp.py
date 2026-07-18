#!/usr/bin/env python3
"""warp.py <target_room> — CROSS-REGION fast-travel via the baked door-driver + the room graph.

Adaptive router: each step, BFS the live `rooms` graph from the current room to the target and
take the next hop.
  - Same-AREA hop  A->B: hijack ALL of A's exit slots -> B (any door then leads there), then sweep
    (teleport + `doorenter` across the floor) until she lands in B.  B is resident (same area
    W-map), so this is direct.
  - Cross-AREA hop A->B (a real boundary portal): can NOT hijack (B's W-map isn't resident until the
    real portal loads it), so sweep A WITHOUT hijacking and take the door that lands in B's area; a
    wrong (same-area) door -> warp back to A and resume past it.
Re-plans from the actual room after every hop (robust to imprecise landings).  Socket only, no frida.

Env OSS_TRAINER_REMOTE (host:port), else cutestation.soy:7777.
"""
import os, sys, time, json, socket
from collections import deque
TRAINER = os.environ.get("OSS_TRAINER_REMOTE", "cutestation.soy:7777")
XLO, XHI, STEP = 4000, 205000, 1800
SETTLE, TAP_WAIT = 0.16, 0.55


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


def _tap(x, y):
    ts({"cmd": "teleport", "x": x, "y": y}); time.sleep(SETTLE)
    ts({"cmd": "doorenter"}); time.sleep(TAP_WAIT)
    return room_now()[0]


def within_warp(target):
    """Hijack all exits -> target, sweep+doorenter until we land in target (same-area only)."""
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
    """Sweep current room WITHOUT hijack; take the door into target_area; wrong door -> back+resume."""
    r0, _ = room_now(); y = y_now(); x = XLO
    while x <= XHI:
        rk = _tap(x, y)
        if rk != r0:
            if area_of(rk) == target_area:
                return rk                          # crossed
            within_warp(r0)                        # wrong same-area door -> go back
            y = y_now(); x += STEP                 # resume past it
        else:
            x += STEP
    return None


def main():
    if len(sys.argv) < 2:
        print("usage: warp.py <target_room>"); return 2
    target = int(sys.argv[1], 0)
    g = graph()
    if target not in g:
        print(f"target {target} not in the room table"); return 1
    for _ in range(40):
        cur, _ = room_now()
        if cur == target:
            print(f"ARRIVED at {target}"); return 0
        path = bfs(g, cur, target)
        if not path or len(path) < 2:
            print(f"no path from {cur} to {target}"); return 1
        nxt = path[1]
        kind = "same-area" if area_of(nxt) == area_of(cur) else "BOUNDARY"
        print(f"at {cur}; hop -> {nxt} ({kind}); {len(path)-1} hops left", flush=True)
        got = within_warp(nxt) if area_of(nxt) == area_of(cur) else boundary_cross(area_of(nxt))
        if got is None:
            print(f"  stuck: no door out of {cur} toward {nxt}"); return 1
        print(f"  -> {got}", flush=True)
    print("too many hops (giving up)"); return 1


if __name__ == "__main__":
    raise SystemExit(main())
