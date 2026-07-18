#!/usr/bin/env python3
"""sotes_trainer MCP — the DURABLE LLM-facing interface to the injected EN-SE
trainer DLL (tools/sotes_trainer/trainer.c), so an agent never has to rediscover
the socket protocol.

The trainer hosts a localhost line-delimited JSON server (default :7777, see
trainer.c).  This is a stdlib-only Model-Context-Protocol server (stdio /
newline-delimited JSON-RPC 2.0, no SDK) that is a thin, typed skin over that
socket — the LLM counterpart to the human's ImGui UI (which talks to the same
socket directly).

  observe   status · player · chars · read · scan · map · rooms · view
  cheats    write · setstat · god · teleport · box · mousefly · flyto · target · unlock
  warp      hijack · revert   (change a door's destination + restore it)
  scene     saves · saveinfo · load · newgame
  dialogue  autoskip · fastskip · dlgskip
  advanced  press · call · raw   (raw = passthrough for ANY cmd, incl. new ones)

`raw` sends {"cmd":<name>, ...fields} verbatim, so trainer commands added after
this file was written are reachable WITHOUT editing it — the durability guarantee.

Endpoint: env OSS_TRAINER_REMOTE ("host:port"), else cutestation.soy:7777 (the
WSL→Windows-LAN default; a same-host ImGui UI uses 127.0.0.1:7777).

Register as an MCP (stdio) in .mcp.json:
    "sotes_trainer": {"type":"stdio","command":"python3",
      "args":["/opt/src/OpenSummoners/tools/sotes_trainer/trainer_mcp.py"]}

CLI (immediate use, no MCP client):
    trainer_mcp.py player                         # -> the player JSON
    trainer_mcp.py teleport x=50000 y=40000
    trainer_mcp.py read addr=0x92ac68 type=u32
    trainer_mcp.py raw cmd=state                  # arbitrary passthrough
    trainer_mcp.py --selftest
"""
from __future__ import annotations

import json
import os
import socket
import sys

DEF_REMOTE = "cutestation.soy:7777"


def _endpoint():
    ep = os.environ.get("OSS_TRAINER_REMOTE", DEF_REMOTE)
    host, _, port = ep.partition(":")
    return host or "cutestation.soy", int(port or "7777")


class TrainerError(RuntimeError):
    pass


def tsend(obj, timeout=25.0):
    """Send one JSON cmd to the trainer socket, return the decoded JSON reply."""
    host, port = _endpoint()
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((host, port))
        s.sendall((json.dumps(obj) + "\n").encode())
        buf = b""
        while not buf.endswith(b"\n"):
            chunk = s.recv(65536)
            if not chunk:
                break
            buf += chunk
        s.close()
    except OSError as e:
        raise TrainerError(
            f"trainer unreachable at {host}:{port} ({e}). Is the game running with "
            f"sotes_trainer.dll injected? (build/inject.exe <exe> <dll> <cwd>)") from e
    if not buf:
        raise TrainerError("no reply from trainer")
    return json.loads(buf.decode())


def _text(s):
    return {"type": "text", "text": s}


def _reply(r):
    return [_text(json.dumps(r, indent=2))]


# ── tool handlers: each builds a {"cmd":...} and returns the reply text ──────
def _cmd(name, args, keys=(), bools=(), forward_all=False):
    """Build a trainer cmd dict from selected arg keys (+ bool coercion)."""
    d = {"cmd": name}
    if forward_all:
        for k, v in args.items():
            if k != "cmd":
                d[k] = v
    else:
        for k in keys:
            if k in args and args[k] is not None:
                d[k] = args[k]
        for k in bools:
            if k in args and args[k] is not None:
                d[k] = bool(args[k])
    return d


def h_status(a):
    return _reply(tsend({"cmd": "state"}))


def h_player(a):
    return _reply(tsend({"cmd": "player"}))


def h_chars(a):
    return _reply(tsend({"cmd": "chars"}))


def h_target(a):
    return _reply(tsend(_cmd("target", a, keys=("code",), bools=("active",))))


def h_read(a):
    return _reply(tsend(_cmd("read", a, keys=("addr", "type"), bools=("va",))))


def h_scan(a):
    return _reply(tsend(_cmd("scan", a, keys=("value", "max"))))


def h_write(a):
    return _reply(tsend(_cmd("write", a, keys=("addr", "value", "type"), bools=("va",))))


def h_setstat(a):
    return _reply(tsend(_cmd("setstat", a, keys=("which", "value"), bools=("lock",))))


def h_god(a):
    return _reply(tsend(_cmd("god", a, bools=("on",))))


def h_teleport(a):
    return _reply(tsend(_cmd("teleport", a, keys=("x", "y"), bools=("relative",))))


def h_mousefly(a):
    return _reply(tsend(_cmd("mousefly", a, bools=("on",))))


def h_flyto(a):
    return _reply(tsend(_cmd("flyto", a, keys=("mx", "my"))))


def h_hijack(a):
    return _reply(tsend(_cmd("hijack", a, keys=("slot", "target"))))


def h_revert(a):
    return _reply(tsend(_cmd("revert", a, keys=("slot",))))


def h_rooms(a):
    return _reply(tsend(_cmd("rooms", a, keys=("area",))))


def h_view(a):
    return _reply(tsend(_cmd("view", a, keys=("off",))))


def h_box(a):
    return _reply(tsend({"cmd": "box"}))


def h_map(a):
    return _reply(tsend({"cmd": "map"}))


def h_unlock(a):
    return _reply(tsend({"cmd": "unlock_all"}))


def h_saves(a):
    return _reply(tsend({"cmd": "saves"}))


def h_saveinfo(a):
    return _reply(tsend(_cmd("saveinfo", a, keys=("slot",))))


def h_load(a):
    return _reply(tsend(_cmd("load", a, keys=("slot", "downs"))))


def h_newgame(a):
    return _reply(tsend(_cmd("newgame", a, keys=("to", "btn", "gap"))))


def h_autoskip(a):
    return _reply(tsend(_cmd("autoskip", a, bools=("on",))))


def h_fastskip(a):
    return _reply(tsend(_cmd("fastskip", a, bools=("on",))))


def h_dlgskip(a):
    return _reply(tsend(_cmd("dlgskip", a, bools=("on",))))


def h_press(a):
    return _reply(tsend(_cmd("press", a, keys=("btn", "n"))))


def h_warpgate(a):
    return _reply(tsend(_cmd("warpgate", a, bools=("on",))))


def h_door(a):
    return _reply(tsend(_cmd("door", a, keys=("slot",), bools=("enter",))))


def h_doorenter(a):
    return _reply(tsend({"cmd": "doorenter"}))


def h_hold(a):
    return _reply(tsend(_cmd("hold", a, keys=("mask",))))


def h_release(a):
    return _reply(tsend({"cmd": "release"}))


def h_call(a):
    return _reply(tsend(_cmd("call", a, keys=("va", "a0", "a1", "a2", "a3", "a4",
                                              "a5", "a6", "a7", "ecx"), bools=("reloc",))))


def h_raw(a):
    if "cmd" not in a:
        return [_text('raw needs a "cmd" field, e.g. {"cmd":"map"}')]
    return _reply(tsend(_cmd(a["cmd"], a, forward_all=True)))


# name, description, inputSchema, handler
TOOLS = [
    ("status", "Trainer boot/hook diagnostic: hooks, main_wnd, attract_frozen, "
     "dlgskip/autoskip/fastskip, box_open/box_scale, md_state, ti_mgr/pk_mgr. "
     "ti_mgr!=0 ⇒ an input poll is firing (game is in a scene or the title).",
     {"type": "object", "properties": {}}, h_status),
    ("player", "The player actor snapshot: {actor,world_x,world_y,stat_block,hp,"
     "hp_max,mp,mp_max,combat_level_max,exp_cur,exp_max} or null if no scene loaded. "
     "world_x/y are centi-px (px*100).",
     {"type": "object", "properties": {}}, h_player),
    ("chars", "The present party members + who is ACTIVE (controlled) and which is the "
     "trainer's target: [{name,code,active,is_target,combat_level_max,world_x,world_y}].",
     {"type": "object", "properties": {}}, h_chars),
    ("target", "Pick which member teleport/mouse-fly/god/reads operate on: active=true for the "
     "controlled member, or code=<party code> (0xc35a Arche / 0xc35b Sana / 0xc35c Stella).",
     {"type": "object", "properties": {
         "active": {"type": "boolean"}, "code": {"type": "integer"}}}, h_target),
    ("read", "Read a game memory address. type=u8/u16/u32 (default u32); va=true "
     "relocates a 0x400000 ImageBase VA by the ASLR delta (delta is 0 live).",
     {"type": "object", "properties": {
         "addr": {"type": ["integer", "string"]}, "type": {"type": "string"},
         "va": {"type": "boolean"}}, "required": ["addr"]}, h_read),
    ("scan", "Find every u32 == value in the game heap (RW private regions). "
     "max caps hits (default 32). ⚠ the DirectDraw framebuffer matches any value; "
     "verify hits by re-reading for stability.",
     {"type": "object", "properties": {
         "value": {"type": ["integer", "string"]}, "max": {"type": "integer"}},
      "required": ["value"]}, h_scan),
    ("write", "Write a typed value to a VA. type=u8/u16/u32; va=true to relocate.",
     {"type": "object", "properties": {
         "addr": {"type": ["integer", "string"]}, "value": {"type": ["integer", "string"]},
         "type": {"type": "string"}, "va": {"type": "boolean"}},
      "required": ["addr", "value"]}, h_write),
    ("setstat", "Set a player stat: which=hp/hp_max/mp/mp_max/level; lock=true to "
     "freeze it each tick.",
     {"type": "object", "properties": {
         "which": {"type": "string"}, "value": {"type": "integer"},
         "lock": {"type": "boolean"}}, "required": ["which", "value"]}, h_setstat),
    ("god", "Freeze HP+MP at 9999 each frame for the WHOLE PARTY (Arche/Sana/Stella) — "
     "invincibility + free casting for everyone present. Default ON.",
     {"type": "object", "properties": {"on": {"type": "boolean"}}}, h_god),
    ("teleport", "Move the player via the authoritative phys-box (*(actor+0x40)): "
     "x sticks, y gravity-settles. Absolute=world centi-px, relative=true nudges in px.",
     {"type": "object", "properties": {
         "x": {"type": "integer"}, "y": {"type": "integer"},
         "relative": {"type": "boolean"}}}, h_teleport),
    ("mousefly", "Continuously teleport the player to the cursor over the game window "
     "(also F7). Freezes the view + edge-scrolls so she stays under the cursor yet can "
     "traverse the map.",
     {"type": "object", "properties": {"on": {"type": "boolean"}}}, h_mousefly),
    ("flyto", "Map a GIVEN game-window client point (mx,my) to world + teleport there "
     "(the mouse-fly mapping for one point — deterministic calibration).",
     {"type": "object", "properties": {
         "mx": {"type": "integer"}, "my": {"type": "integer"}},
      "required": ["mx", "my"]}, h_flyto),
    ("box", "Debug: the player's collision AABB {box,tag,x,top,w,h,world_y}.",
     {"type": "object", "properties": {}}, h_box),
    ("map", "The CURRENT map/room via the render-root chain: {room_key, area, scene "
     "(DATA resource id), tileset, parallax, exits:[{slot,exit_key,target_room,return_key,"
     "door_x,door_y}]}. exits = the portal graph; door_x/door_y = the live world position to "
     "teleport onto so `door`/doorenter fires it (-1 = no loaded anchor). null if not in a scene.",
     {"type": "object", "properties": {}}, h_map),
    ("hijack", "Overwrite exit-slot N's target_room in the live room record so that door "
     "warps to `target` (WARP PRIMITIVE; within-area). Original stashed for revert.",
     {"type": "object", "properties": {
         "slot": {"type": "integer"}, "target": {"type": "integer"}},
      "required": ["slot", "target"]}, h_hijack),
    ("revert", "Restore exit-slot N's original target_room.",
     {"type": "object", "properties": {"slot": {"type": "integer"}},
      "required": ["slot"]}, h_revert),
    ("rooms", "Enumerate the MASTER room table (ALL rooms, every area) + each room's portal "
     "GRAPH: per room {key,area,scene,exits:[target_room,...]} — the cross-region routing graph + "
     "the hijack destination list. Optional `area` filters to one area (999999 exits = overworld).",
     {"type": "object", "properties": {"area": {"type": "integer"}}}, h_rooms),
    ("view", "Mouse-fly camera diagnostic: the resolved view rect (left/top from "
     "cur_x/cur_y + span) + the player box + a camera-object field dump. off tunes the "
     "render_root->camera pointer offset.",
     {"type": "object", "properties": {"off": {"type": "integer"}}}, h_view),
    ("unlock", "Drop god + all stat locks.",
     {"type": "object", "properties": {}}, h_unlock),
    ("saves", "Enumerate + identify EVERY on-disk save (reads user\\savedataNN.sdt "
     "directly, no engine load): per slot {valid,handle,party:[{name,code,"
     "combat_level_max}],file_size}. Use to pick a slot to load.",
     {"type": "object", "properties": {}}, h_saves),
    ("saveinfo", "Full summary of one save slot.",
     {"type": "object", "properties": {"slot": {"type": "integer"}},
      "required": ["slot"]}, h_saveinfo),
    ("load", "From the TITLE, menu-drive the game's own load path (Continue → "
     "slot-picker → confirm). No slot = the default newest save; slot:N selects "
     "savedataNN. Returns when the loaded actor appears.",
     {"type": "object", "properties": {
         "slot": {"type": "integer"}, "downs": {"type": "integer"}}}, h_load),
    ("newgame", "From the TITLE, rotate to New Game then confirm — starts a fresh "
     "game. to=rotations from Continue (default 1), btn=rotate id (2=up/4=down).",
     {"type": "object", "properties": {
         "to": {"type": "integer"}, "btn": {"type": "integer"},
         "gap": {"type": "integer"}}}, h_newgame),
    ("autoskip", "Auto-advance ALL story/cutscene/NPC dialogue hands-free (the "
     "'hold TAB' skip; a 2-byte code patch). Default ON — turn OFF to read/pick "
     "choice boxes.",
     {"type": "object", "properties": {"on": {"type": "boolean"}}}, h_autoskip),
    ("fastskip", "Instant text: force the active dialogue line's typewriter reveal "
     "to total each tick (pure UI-state, door-safe). Default OFF.",
     {"type": "object", "properties": {"on": {"type": "boolean"}}}, h_fastskip),
    ("dlgskip", "Auto-advance an OPEN prompt box by injecting the advance ids (passive-"
     "gated). WARNING: those ids double as world action input, so it auto-CONFIRMS world "
     "prompts (bed/door). Default OFF — the world-safe skip is autoskip.",
     {"type": "object", "properties": {"on": {"type": "boolean"}}}, h_dlgskip),
    ("press", "Inject button id `btn` into the active input mgr `n` times — a "
     "probe to map what an id does in the current context.",
     {"type": "object", "properties": {
         "btn": {"type": "integer"}, "n": {"type": "integer"}}, "required": ["btn"]}, h_press),
    ("warpgate", "Code-patch the SE door handler so any door transitions INSTANTLY, skipping the "
     "combat-proximity block, the never-used-portal block, and the hold-UP ramp — lets `door`/warp "
     "fire a portal in a mob room or a never-visited portal (walk/teleport ONTO a door — it won't "
     "self-fire while you just stand on one). Default ON; auto-gated off during the title/menu/load "
     "so it can't fire mid-transition (crash-safe).",
     {"type": "object", "properties": {"on": {"type": "boolean"}}}, h_warpgate),
    ("door", "Teleport straight ONTO exit-slot `slot`'s door (its live position from `map`), "
     "optionally firing doorenter (enter=true) so she transitions — the warp SPEEDUP (no floor "
     "sweep). Returns {door_x,door_y,target_room,entered}. Poll `map` for the room change.",
     {"type": "object", "properties": {
         "slot": {"type": "integer"}, "enter": {"type": "boolean"}}, "required": ["slot"]}, h_door),
    ("doorenter", "Fire the game's OWN door-enter (raw-DINPUT buffered UP-press event inject + "
     "forced read) so she transitions through the door she is STANDING ON — teleport onto a door "
     "zone first.  Foreground-independent (works even unfocused).  Returns immediately; poll `map` "
     "for the room change.  Compose with `hijack` to warp to any resident room.",
     {"type": "object", "properties": {}}, h_doorenter),
    ("hold", "Hold freeroam movement keys each frame via the raw-DINPUT immediate buffer (bitmask "
     "1=UP 2=DOWN 4=LEFT 8=RIGHT; mask 0 clears).  Real walking; `teleport` is usually simpler. "
     "Held until changed — call `release` to stop.",
     {"type": "object", "properties": {"mask": {"type": "integer"}}, "required": ["mask"]}, h_hold),
    ("release", "Stop all injected movement (equivalent to hold mask 0).",
     {"type": "object", "properties": {}}, h_release),
    ("call", "Call an engine fn on the socket thread (thiscall via ecx). "
     "EXPERIMENTAL — unsafe for engine fns that touch the scene; prefer commands "
     "that drain at the input-poll safepoint. va + a0..a7 + ecx + reloc(bool).",
     {"type": "object", "properties": {
         "va": {"type": ["integer", "string"]}, "ecx": {"type": ["integer", "string"]},
         "reloc": {"type": "boolean"}}, "required": ["va"]}, h_call),
    ("raw", "Passthrough: send {cmd, ...fields} to the trainer verbatim. Use for "
     "any trainer command not surfaced as a typed tool above (incl. ones added "
     "after this MCP was written). Example: {\"cmd\":\"map\"}.",
     {"type": "object", "properties": {"cmd": {"type": "string"}},
      "required": ["cmd"]}, h_raw),
]
HANDLERS = {name: fn for name, _d, _s, fn in TOOLS}


def handle(msg):
    mid = msg.get("id")
    method = msg.get("method")
    if method == "initialize":
        return {"jsonrpc": "2.0", "id": mid, "result": {
            "protocolVersion": msg.get("params", {}).get("protocolVersion", "2024-11-05"),
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "sotes_trainer", "version": "0.1.0"}}}
    if method in ("notifications/initialized", "initialized"):
        return None
    if method == "ping":
        return {"jsonrpc": "2.0", "id": mid, "result": {}}
    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": mid, "result": {
            "tools": [{"name": n, "description": d, "inputSchema": s}
                      for n, d, s, _ in TOOLS]}}
    if method == "tools/call":
        params = msg.get("params", {})
        name = params.get("name")
        args = params.get("arguments", {}) or {}
        fn = HANDLERS.get(name)
        if not fn:
            return {"jsonrpc": "2.0", "id": mid,
                    "error": {"code": -32601, "message": f"no tool {name}"}}
        try:
            return {"jsonrpc": "2.0", "id": mid, "result": {"content": fn(args)}}
        except TrainerError as e:
            return {"jsonrpc": "2.0", "id": mid, "result": {
                "content": [_text(f"TRAINER NOT REACHABLE: {e}")], "isError": True}}
        except Exception as e:  # noqa: BLE001
            return {"jsonrpc": "2.0", "id": mid, "result": {
                "content": [_text(f"tool error: {e!r}")], "isError": True}}
    if mid is not None:
        return {"jsonrpc": "2.0", "id": mid,
                "error": {"code": -32601, "message": f"unknown method {method}"}}
    return None


def serve_stdio():
    print("[sotes_trainer-mcp] ready (stdio)", file=sys.stderr, flush=True)
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            continue
        resp = handle(msg)
        if resp is not None:
            sys.stdout.write(json.dumps(resp) + "\n")
            sys.stdout.flush()


def _coerce(v):
    if v in ("true", "false"):
        return v == "true"
    try:
        return int(v, 0)
    except ValueError:
        return v


def cli(argv):
    """trainer_mcp.py <cmd> [k=v ...] — send a raw trainer cmd and print the reply."""
    cmd = argv[0]
    d = {"cmd": cmd}
    for kv in argv[1:]:
        k, _, v = kv.partition("=")
        d[k] = _coerce(v)
    print(json.dumps(tsend(d), indent=2))


def selftest():
    for c in [{"cmd": "ping"}, {"cmd": "state"}, {"cmd": "player"}]:
        try:
            print(f">>> {c}\n{json.dumps(tsend(c), indent=2)}\n")
        except TrainerError as e:
            print(f">>> {c}\n  {e}\n")


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        selftest()
    elif len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
        cli(sys.argv[1:])
    else:
        serve_stdio()
