#!/usr/bin/env python3
"""OpenSummoners MCP server — let an LLM PROBE and DRIVE live retail Fortune
Summoners (SotES).

A stdlib-only Model-Context-Protocol server (stdio / newline-delimited JSON-RPC
2.0, no SDK dependency) that sits on the live-probe daemon (tools/probe_daemon.py)
and exposes the running retail game as MCP tools:

  session   launch · game_status · quit
  observe   game_state · screenshot · read_memory · read_bytes · read_chain · reads
  act       hold · press · tap · release · set_turbo
  advanced  poke_memory · call_function · anchors · wait

The daemon holds ONE persistent game (frida-attached via the Windows frida-
server, default cutestation.soy:27042); this MCP is a thin, LLM-friendly skin
over its socket. `launch` spawns the daemon detached so the game survives
across tool calls.

Registered as the `opensummoners` MCP (stdio) in .mcp.json / the slopstudio
local scope. Launch it via a clean python (NOT `nix develop -c python3`, whose
devshell banner corrupts the JSON-RPC stream) — the DAEMON re-execs through nix
develop itself for frida/PIL.

Self-test without an MCP client:
    tools/opensummoners_mcp.py --selftest
"""
from __future__ import annotations

import base64
import json
import os
import socket
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CTRL = ROOT / "runs" / "probe" / "daemon.json"
DAEMON = ROOT / "tools" / "probe_daemon.py"


class DaemonError(RuntimeError):
    pass


def _port():
    if CTRL.exists():
        try:
            return int(json.loads(CTRL.read_text())["port"])
        except Exception as e:
            raise DaemonError(f"daemon.json unreadable: {e}") from e
    raise DaemonError("no daemon — call launch first.")


def dsend(req, timeout=40.0):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect(("127.0.0.1", _port()))
        s.sendall((json.dumps(req) + "\n").encode())
        buf = b""
        while not buf.endswith(b"\n"):
            chunk = s.recv(65536)
            if not chunk:
                break
            buf += chunk
        s.close()
    except (OSError, DaemonError) as e:
        raise DaemonError(f"daemon unreachable: {e}") from e
    return json.loads(buf.decode()) if buf else {"ok": False, "err": "no reply"}


def _daemon_alive():
    if not CTRL.exists():
        return False
    try:
        return bool(dsend({"cmd": "ping"}, timeout=5).get("alive"))
    except DaemonError:
        return False


def _text(s):
    return {"type": "text", "text": s}


# ── tools ─────────────────────────────────────────────────────────────────
def tool_launch(args):
    if _daemon_alive():
        st = dsend({"cmd": "status"}).get("status", {})
        return [_text(f"A game is already running (daemon alive). {json.dumps(st)}. "
                      "Call quit first to relaunch.")]
    nix_cmd = ["nix", "develop", str(ROOT), "--command", "python3", str(DAEMON)]
    if args.get("view"):
        nix_cmd.append("--view")
    if args.get("realtime"):
        nix_cmd.append("--realtime")
    if args.get("audio"):
        nix_cmd.append("--audio")
    if args.get("rng_seed") is not None:
        nix_cmd += ["--rng-seed", str(args["rng_seed"])]
    logf = ROOT / "runs" / "probe" / "daemon.boot.log"
    logf.parent.mkdir(parents=True, exist_ok=True)
    lf = open(logf, "w")
    try:
        CTRL.unlink()
    except FileNotFoundError:
        pass
    subprocess.Popen(nix_cmd, cwd=str(ROOT), stdout=lf, stderr=subprocess.STDOUT,
                     stdin=subprocess.DEVNULL, start_new_session=True)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        if _daemon_alive():
            st = dsend({"cmd": "status"}).get("status", {})
            return [_text("Game launched. daemon up.\n" + json.dumps(st, indent=2))]
        time.sleep(1.0)
    tail = ""
    try:
        tail = "\n".join(logf.read_text().splitlines()[-20:])
    except Exception:
        pass
    return [_text(f"Launch timed out after 120s. daemon.boot.log tail:\n{tail}")]


def tool_game_status(_):
    if not _daemon_alive():
        return [_text("No game running. Call launch.")]
    return [_text(json.dumps(dsend({"cmd": "status"}), indent=2))]


def tool_game_state(_):
    r = dsend({"cmd": "state"})
    return [_text(json.dumps(r.get("vals", r), indent=2))]


def tool_screenshot(args):
    r = dsend({"cmd": "shot", "path": args.get("path")}, timeout=30)
    if not r.get("ok"):
        return [_text(f"screenshot FAILED: {r.get('err')}")]
    out = [_text(f"frame {r.get('frame')} ({r['w']}x{r['h']}) -> {r['path']}"
                 + (f"\nstate: {json.dumps(r['vals'])}" if r.get("vals") else ""))]
    if args.get("inline", True):
        try:
            data = base64.b64encode(Path(r["path"]).read_bytes()).decode()
            out.append({"type": "image", "data": data, "mimeType": "image/png"})
        except Exception as e:
            out.append(_text(f"(inline image failed: {e})"))
    return out


def tool_hold(args):
    keys = args.get("keys")
    if keys is None:
        keys = [args["key"]] if "key" in args else []
    return [_text(json.dumps(dsend({"cmd": "hold", "keys": keys})))]


def tool_press(args):
    ids = args.get("ids")
    if ids is None:
        ids = [args["id"]]
    return [_text(json.dumps(dsend({"cmd": "press", "ids": ids})))]


def tool_tap(args):
    keys = args.get("keys") or ([args["key"]] if "key" in args else [])
    return [_text(json.dumps(dsend({"cmd": "tap", "keys": keys,
                                    "ms": args.get("ms", 120)})))]


def tool_release(_):
    return [_text(json.dumps(dsend({"cmd": "release"})))]


def tool_set_turbo(args):
    return [_text(json.dumps(dsend({"cmd": "turbo", "on": bool(args["on"])})))]


def tool_read_memory(args):
    return [_text(json.dumps(dsend({"cmd": "read", "va": args["va"],
                                    "type": args.get("type", "i32")})))]


def tool_reads(args):
    return [_text(json.dumps(dsend({"cmd": "reads", "specs": args["specs"]}), indent=2))]


def tool_read_bytes(args):
    return [_text(json.dumps(dsend({"cmd": "read_bytes", "va": args["va"],
                                    "len": args["len"]})))]


def tool_read_chain(args):
    return [_text(json.dumps(dsend({"cmd": "read_chain", "va": args["va"],
                                    "offsets": args.get("offsets", []),
                                    "type": args.get("type", "i32"),
                                    "len": args.get("len", 0)})))]


def tool_poke_memory(args):
    req = {"cmd": "poke", "va": args["va"]}
    if "bytes" in args:
        req["bytes"] = args["bytes"]
    else:
        req["type"] = args.get("type", "i32")
        req["val"] = args["val"]
    return [_text(json.dumps(dsend(req)))]


def tool_call_function(args):
    return [_text(json.dumps(dsend({"cmd": "callq", "va": args["va"],
                                    "args": args.get("args", []),
                                    "argt": args.get("argt", []),
                                    "ret": args.get("ret", "int32"),
                                    "abi": args.get("abi", "mscdecl"),
                                    "timeout": args.get("timeout", 5.0)})))]


def tool_anchors(args):
    r = dsend({"cmd": "anchors", "clear": bool(args.get("clear"))})
    evs = r.get("anchors", [])
    return [_text(json.dumps(evs, indent=2) if evs else "(no anchors captured yet)")]


def tool_wait(args):
    return [_text(json.dumps(dsend({"cmd": "sleep", "ms": int(args.get("ms", 500))})))]


def tool_quit(_):
    if not _daemon_alive():
        return [_text("No game running.")]
    dsend({"cmd": "quit"})
    return [_text("Game + daemon shut down.")]


KEY_DESC = ("DIK scancode name(s) or int: up down left right, z x c a s d, space "
            "enter esc lshift lctrl, 1..5. Movement is arrow keys; combine to "
            "hold diagonals.")
TOOLS = [
    ("launch", "Start the game: spawn the live-probe daemon (detached, survives "
     "across calls) and attach frida to retail SotES. Default hidden window + "
     "turbo; screenshots still work. Pass view=true to watch on the host, "
     "realtime=true for 1x, rng_seed to pin the LCG.",
     {"type": "object", "properties": {
         "view": {"type": "boolean"}, "realtime": {"type": "boolean"},
         "audio": {"type": "boolean"},
         "rng_seed": {"type": ["integer", "string", "null"]}}}, tool_launch),
    ("game_status", "Daemon/agent status: pid, probe flip/sim_tick, resolved "
     "input manager, held keys.", {"type": "object", "properties": {}}, tool_game_status),
    ("game_state", "Curated live state: flip, sim_tick, LCG seed, room-state "
     "root ptr, screen obj.", {"type": "object", "properties": {}}, tool_game_state),
    ("screenshot", "Grab the current DirectDraw frame to PNG and (default) inline "
     "it so you SEE the screen. Needs the engine to be presenting (turbo flips "
     "fast; if it fails try relaunch with realtime).",
     {"type": "object", "properties": {
         "path": {"type": "string"}, "inline": {"type": "boolean"}}}, tool_screenshot),
    ("hold", "Set the held key set (walk/aim). Replaces the current held set — "
     "hold([]) or release to stop. " + KEY_DESC,
     {"type": "object", "properties": {
         "keys": {"type": "array"}, "key": {"type": ["string", "integer"]}}}, tool_hold),
    ("press", "Queue button-ring press(es) (confirm/attack/jump/skill) — the "
     "engine button ids the trace replay injects. Raw ids until mapped; "
     "experiment + screenshot to learn them.",
     {"type": "object", "properties": {
         "ids": {"type": "array"}, "id": {"type": ["integer", "string"]}}}, tool_press),
    ("tap", "Hold key(s) for ms then restore (a discrete keypress — menu nav, "
     "movement nudge). " + KEY_DESC,
     {"type": "object", "properties": {
         "keys": {"type": "array"}, "key": {"type": ["string", "integer"]},
         "ms": {"type": "integer"}}}, tool_tap),
    ("release", "Clear all held keys + pending presses.",
     {"type": "object", "properties": {}}, tool_release),
    ("set_turbo", "Toggle turbo fast-forward.",
     {"type": "object", "properties": {"on": {"type": "boolean"}},
      "required": ["on"]}, tool_set_turbo),
    ("read_memory", "Read a game memory address (Ghidra VA; type u8/i8/u16/i16/"
     "u32/i32/f32/f64/ptr).",
     {"type": "object", "properties": {
         "va": {"type": ["integer", "string"]}, "type": {"type": "string"}},
      "required": ["va"]}, tool_read_memory),
    ("reads", "Batch typed reads: specs=[{name,va,type}] -> {name:value}.",
     {"type": "object", "properties": {"specs": {"type": "array"}},
      "required": ["specs"]}, tool_reads),
    ("read_bytes", "Read len raw bytes at a VA -> hex.",
     {"type": "object", "properties": {
         "va": {"type": ["integer", "string"]}, "len": {"type": "integer"}},
      "required": ["va", "len"]}, tool_read_bytes),
    ("read_chain", "Pointer-chase: base VA + offsets[] (deref each), then a typed "
     "read (or len raw bytes).",
     {"type": "object", "properties": {
         "va": {"type": ["integer", "string"]}, "offsets": {"type": "array"},
         "type": {"type": "string"}, "len": {"type": "integer"}},
      "required": ["va"]}, tool_read_chain),
    ("poke_memory", "Write a typed value (or raw bytes[]) to a VA.",
     {"type": "object", "properties": {
         "va": {"type": ["integer", "string"]}, "type": {"type": "string"},
         "val": {"type": "number"}, "bytes": {"type": "array"}},
      "required": ["va"]}, tool_poke_memory),
    ("call_function", "Call a game function on the ENGINE thread (drained at the "
     "input poll — safe). va + args[] + optional argt[] (frida types) + ret + "
     "abi (mscdecl/stdcall/thiscall/fastcall).",
     {"type": "object", "properties": {
         "va": {"type": ["integer", "string"]}, "args": {"type": "array"},
         "argt": {"type": "array"}, "ret": {"type": "string"},
         "abi": {"type": "string"}}, "required": ["va"]}, tool_call_function),
    ("anchors", "Scene/RNG anchor firings captured since launch (frame+seed). "
     "clear=true to drain.",
     {"type": "object", "properties": {"clear": {"type": "boolean"}}}, tool_anchors),
    ("wait", "Let the game run for ms (so animation/loads advance before a "
     "screenshot).", {"type": "object", "properties": {"ms": {"type": "integer"}}},
     tool_wait),
    ("quit", "Shut down the game + daemon.", {"type": "object", "properties": {}}, tool_quit),
]
HANDLERS = {name: fn for name, _d, _s, fn in TOOLS}


def _log(msg):
    print(f"[opensummoners-mcp] {msg}", file=sys.stderr, flush=True)


def handle(msg):
    mid = msg.get("id")
    method = msg.get("method")
    if method == "initialize":
        return {"jsonrpc": "2.0", "id": mid, "result": {
            "protocolVersion": msg.get("params", {}).get("protocolVersion", "2024-11-05"),
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "opensummoners", "version": "0.1.0"}}}
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
        except DaemonError as e:
            return {"jsonrpc": "2.0", "id": mid, "result": {
                "content": [_text(f"GAME NOT REACHABLE: {e}")], "isError": True}}
        except Exception as e:  # noqa: BLE001
            return {"jsonrpc": "2.0", "id": mid, "result": {
                "content": [_text(f"tool error: {e!r}")], "isError": True}}
    if mid is not None:
        return {"jsonrpc": "2.0", "id": mid,
                "error": {"code": -32601, "message": f"unknown method {method}"}}
    return None


def serve_stdio():
    _log("ready (stdio); tools connect to the probe daemon on demand")
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


def selftest():
    for req in [
        {"jsonrpc": "2.0", "id": 1, "method": "initialize",
         "params": {"protocolVersion": "2024-11-05"}},
        {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
        {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
         "params": {"name": "game_status", "arguments": {}}},
    ]:
        resp = handle(req)
        print(f"\n>>> {req.get('method')} {req.get('params', {}).get('name', '')}")
        if resp and "result" in resp:
            res = resp["result"]
            if "content" in res:
                for c in res["content"]:
                    print(c.get("text", f"[{c.get('type')}]"))
            elif "tools" in res:
                print("tools:", ", ".join(t["name"] for t in res["tools"]))
        elif resp:
            print(resp)


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        selftest()
    else:
        serve_stdio()
