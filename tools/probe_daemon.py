#!/usr/bin/env python3
"""Live-probe daemon for retail Fortune Summoners (SotES) — the persistent
interactive session behind the `opensummoners` MCP (tools/opensummoners_mcp.py).

Spawns + attaches to retail ONCE (via the remote frida-server, default
cutestation.soy:27042) and keeps the frida session alive, so game state
persists across many commands (unlike frida_capture, which spawns/reaps per
run). Drives the live game via the agent's live-probe RPC surface
(tools/frida/opensummoners-agent.js "LIVE-PROBE LAYER"): on-demand DirectDraw
screenshots (GDI grab of the render surface), typed memory reads/pokes,
engine-thread function calls, and interactive input — held scancodes for
walking (the held-axis keyboard_state force) + button-ring presses for actions
(the same ids the trace replay injects).

    # boot to the launcher→title, hidden window, turbo:
    nix develop --command python3 tools/probe_daemon.py &
    # boot visible + realtime (to watch on the Windows host):
    nix develop --command python3 tools/probe_daemon.py --view --realtime &

Commands are line-delimited JSON over 127.0.0.1:<port> (control file
runs/probe/daemon.json). Mirrors OpenMare/openrecet's tools/probe_daemon.py,
adapted to SotES's scancode/button-ring input model + the EN-old engine VAs
(the ghidra-analyzed port target — sotes.exe @ 0x00400000).

CAUTION: the game runs from the real install via vendor/original. No save
sandbox — avoid overwriting save slots you care about.

HARD RULE (USER, 2026-07-10) — kills must target THIS probe SPECIFICALLY.
Parallel projects (openrecet, OpenMare) run identically-named probe_daemon.py
against the SAME shared frida-server, and parity captures run the same
sotes.unpacked.exe. Therefore:
  - NEVER `pkill -f probe_daemon` (kills the siblings' probes).
  - NEVER kill/restart the shared frida-server.
  - NEVER kill retail by exe NAME on the frida device (kills a parallel
    capture's game). The stray-reap below kills ONLY the exact PID recorded
    by the PREVIOUS daemon of THIS project (runs/probe/last_game.json).
  - To stop this probe: send {"cmd":"quit"} (or the MCP quit tool); if the
    socket is dead, kill the exact `daemon_pid` from runs/probe/daemon.json.
"""
from __future__ import annotations

import argparse
import json
import os
import socket
import threading
import time
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import frida  # noqa: E402
import frida_capture as fc  # noqa: E402

ROOT = fc.ROOT
CONTROL_JSON = ROOT / "runs" / "probe" / "daemon.json"
# The game PID this project's daemon last spawned — survives an unclean daemon
# death (CONTROL_JSON is unlinked by the next launch) so the reap can target
# exactly that process and nothing else.
LAST_GAME_JSON = ROOT / "runs" / "probe" / "last_game.json"

# DirectInput (DIK_*) scancodes — the index into the engine's keyboard_state
# buffer that FUN_005ba520 reads (& 0x80). Movement is arrow keys; action
# buttons go through the button ring (see press/BUTTON below), not here.
# Extend as the live probe confirms which keys the freeroam mover reads.
DIK = {
    "up": 0xC8, "down": 0xD0, "left": 0xCB, "right": 0xCD,
    "z": 0x2C, "x": 0x2D, "c": 0x2E, "a": 0x1E, "s": 0x1F, "d": 0x20,
    "space": 0x39, "enter": 0x1C, "esc": 0x01, "lshift": 0x2A, "lctrl": 0x1D,
    "1": 0x02, "2": 0x03, "3": 0x04, "4": 0x05, "5": 0x06,
}


def _scancode(spec):
    """Accept a DIK int, a hex string, or a DIK name."""
    if isinstance(spec, int):
        return spec
    s = str(spec).strip().lower()
    if s in DIK:
        return DIK[s]
    return int(s, 0)


class ProbeDaemon:
    def __init__(self, args):
        self.args = args
        self.run_dir = ROOT / "runs" / "probe" / (args.run_id or "session")
        (self.run_dir / "shots").mkdir(parents=True, exist_ok=True)
        self.log = (self.run_dir / "daemon.log").open("w", buffering=1)
        self.device = None
        self.pid = None
        self.session = None
        self.script = None
        self._stop = threading.Event()
        self._shot_seq = 0
        self._shot_lock = threading.Lock()
        self._shot_want = False
        self._shot_out = None
        self._shot_evt = threading.Event()
        self._call_results = {}
        self._call_evt = threading.Event()
        self._anchors = []
        self._anchor_lock = threading.Lock()
        self._last_frame = 0
        self._input_mgr = None

    def _logline(self, s: str):
        self.log.write(s + "\n")

    # ── frida message pump ─────────────────────────────────────────────────
    def _on_message(self, message, data):
        if message.get("type") == "error":
            self._logline(f"[frida-error] {message.get('description', '')}")
            return
        if message.get("type") != "send":
            return
        p = message.get("payload") or {}
        kind = p.get("kind")
        if kind == "frame":
            self._last_frame = p.get("frame", 0)
            if self._shot_want and data is not None:
                self._shot_want = False
                try:
                    w, h = int(p["w"]), int(p["h"])
                    stride = int(p.get("stride", w * 3))
                    out = self._shot_out or (
                        self.run_dir / "shots" / f"shot_{self._shot_seq:04d}.png")
                    out.parent.mkdir(parents=True, exist_ok=True)
                    fc.write_png_from_dib24(out, w, h, stride, data)
                    self._shot_result = {"ok": True, "path": str(out),
                                         "w": w, "h": h, "frame": p.get("frame")}
                except Exception as e:  # noqa: BLE001
                    self._shot_result = {"ok": False, "err": f"png save failed: {e!r}"}
                self._shot_evt.set()
            return
        if kind == "call_result":
            self._call_results[p.get("id")] = p
            self._call_evt.set()
            return
        if kind == "input_mgr_resolved":
            self._input_mgr = p.get("mgr")
            return
        if kind == "anchor":
            with self._anchor_lock:
                self._anchors.append(p)
            self._logline(f"[anchor] {json.dumps(p)}")
            return
        if kind in ("log", "error", "ready", "inject", "input_mgr_resolved"):
            self._logline(f"[agent:{kind}] {json.dumps(p)[:300]}")
            return

    # ── lifecycle ──────────────────────────────────────────────────────────
    def start(self):
        a = self.args
        if not fc.ensure_frida_server(a.remote, fc.DEFAULT_FRIDA_SERVER_EXE):
            raise SystemExit(f"frida-server unreachable at {a.remote}")
        exe = Path(a.exe).resolve()
        cwd = Path(a.cwd).resolve()
        win_exe = fc.wslpath_w(exe)
        win_cwd = fc.wslpath_w(cwd)
        self.device = frida.get_device_manager().add_remote_device(a.remote)
        # Reap a stray retail from a prior daemon that died without kill —
        # HARD RULE: target OUR probe's game ONLY (the exact PID recorded in
        # LAST_GAME_JSON, name-checked against pid reuse). NEVER sweep by exe
        # name: a parallel capture (frida_capture / run_proxy) runs the same
        # sotes.unpacked.exe and must not be killed. Same-name strays we did
        # not spawn are logged and LEFT ALONE.
        try:
            prev_pid = None
            if LAST_GAME_JSON.exists():
                try:
                    prev_pid = int(json.loads(LAST_GAME_JSON.read_text())["pid"])
                except Exception:
                    pass
            reaped = False
            for pr in self.device.enumerate_processes():
                if pr.name.lower() != exe.name.lower():
                    continue
                if prev_pid is not None and pr.pid == prev_pid:
                    self._logline(f"[reap] killing OUR stray {pr.name} pid={pr.pid}")
                    try:
                        self.device.kill(pr.pid)
                        reaped = True
                    except Exception:
                        pass
                else:
                    self._logline(f"[reap] leaving foreign {pr.name} pid={pr.pid} "
                                  "(not ours — a parallel capture/probe?)")
            if reaped:
                time.sleep(0.5)
        except Exception as e:
            self._logline(f"[reap] enumerate failed: {e!r}")

        self._logline(f"[spawn] {win_exe} cwd={win_cwd}")
        self.pid = self.device.spawn([win_exe], cwd=win_cwd)
        # Record OUR game pid so a later daemon (after an unclean death) can
        # reap exactly this process and nothing else (hard rule above).
        LAST_GAME_JSON.parent.mkdir(parents=True, exist_ok=True)
        LAST_GAME_JSON.write_text(json.dumps(
            {"pid": self.pid, "exe": exe.name, "daemon_pid": os.getpid()}))
        self.session = self.device.attach(self.pid)
        self.script = self.session.create_script(fc.AGENT_JS.read_text(encoding="utf-8"))
        self.script.on("message", self._on_message)
        self.script.load()

        init_cfg = {
            "module_name": exe.name,
            "hide_window": not a.view,
            "force_windowed": True,
            "silent_audio": not a.audio,
            "turbo": not a.realtime,
            "turbo_step_ms": int(a.turbo_step_ms),
            # Dismiss the #32770 launcher dialog in-process (quirk #3).
            "auto_click_launch": True,
            "auto_disable_sound": not a.audio,
            "msgbox_redirect": True,
            "seed_pin": a.rng_seed is not None,
            "seed_value": (int(a.rng_seed, 0) & 0xffffffff
                           if a.rng_seed is not None else 0),
        }
        self.script.exports_sync.init(init_cfg)
        self.device.resume(self.pid)
        # Arm the live-probe layer (input + shot hooks) once running.
        time.sleep(1.0)
        try:
            self.script.exports_sync.probe_begin()
        except Exception as e:
            self._logline(f"[probe_begin] {e!r}")
        self._logline(f"[ready] pid={self.pid} view={a.view} "
                      f"turbo={not a.realtime}")

    def _heartbeat_ok(self):
        try:
            self.script.exports_sync.probe_status()
            return True
        except Exception:
            return False

    def stop(self):
        self._stop.set()
        try:
            if self.pid is not None:
                # Targeted: OUR spawned game pid only (hard rule above).
                self.device.kill(self.pid)
        except Exception:
            pass
        try:
            LAST_GAME_JSON.unlink()  # clean shutdown — nothing left to reap
        except Exception:
            pass

    # ── screenshot (agent GDI grab → PNG via the frame message) ──────────────
    def _shot(self, path=None, timeout=10.0):
        with self._shot_lock:
            self._shot_out = Path(path) if path else None
            if self._shot_out is None:
                self._shot_seq += 1
            self._shot_result = {"ok": False, "err": "no frame emitted "
                                 "(is the engine presenting? try --realtime)"}
            self._shot_evt.clear()
            self._shot_want = True
            try:
                self.script.exports_sync.probe_shot()
            except Exception as e:
                self._shot_want = False
                return {"ok": False, "err": f"probe_shot rpc failed: {e!r}"}
            self._shot_evt.wait(timeout)
            self._shot_want = False
            r = dict(self._shot_result)
            if r.get("ok"):
                try:
                    r["vals"] = self.script.exports_sync.probe_state()
                except Exception:
                    pass
            return r

    # ── engine-thread call ──────────────────────────────────────────────────
    def _callq(self, va, args, argt, ret, abi, timeout):
        self._call_evt.clear()
        cid = self.script.exports_sync.probe_enqueue_call(
            va, args or [], argt or [], ret or "int32", abi or "mscdecl")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if cid in self._call_results:
                r = self._call_results.pop(cid)
                return {"ok": r.get("err") is None, "ret": r.get("ret"),
                        "err": r.get("err"), "id": cid, "frame": r.get("frame")}
            self._call_evt.wait(0.2)
            self._call_evt.clear()
        return {"ok": False, "err": "call timed out (engine not ticking / VA "
                "never reaches the input poll?)", "id": cid}

    # ── command dispatch ─────────────────────────────────────────────────────
    def dispatch(self, req: dict) -> dict:
        cmd = req.get("cmd")
        try:
            x = self.script.exports_sync
            if cmd == "ping":
                return {"ok": True, "pid": self.pid, "alive": self._heartbeat_ok()}
            if cmd == "status":
                return {"ok": True, "status": x.probe_status(),
                        "run_dir": str(self.run_dir), "pid": self.pid,
                        "last_frame": self._last_frame}
            if cmd == "state":
                return {"ok": True, "vals": x.probe_state()}
            if cmd == "shot":
                return self._shot(req.get("path"))
            if cmd == "hold":
                scs = [_scancode(s) for s in (req.get("keys") or [])]
                return {"ok": True, "held": x.probe_hold(scs)}
            if cmd == "press":
                ids = req.get("ids")
                if ids is None:
                    ids = [int(req["id"])]
                ids = [int(i, 0) if isinstance(i, str) else int(i) for i in ids]
                return {"ok": True, "queued": x.probe_press(ids)}
            if cmd == "tap":
                # Hold scancode(s) for `frames`≈ms then release — a discrete
                # keypress on the held-axis model (movement nudges, menu keys).
                scs = [_scancode(s) for s in (req.get("keys") or [])]
                prev = x.probe_status().get("held", [])
                x.probe_hold(scs)
                time.sleep(float(req.get("ms", 120)) / 1000.0)
                x.probe_hold([int(s) for s in prev])
                return {"ok": True, "tapped": scs}
            if cmd == "release":
                return {"ok": True, "dropped": x.probe_release()}
            if cmd == "read":
                va = int(req["va"], 0) if isinstance(req["va"], str) else req["va"]
                return {"ok": True, "val": x.probe_read(va, req.get("type", "i32"))}
            if cmd == "reads":
                specs = []
                for s in req["specs"]:
                    va = int(s["va"], 0) if isinstance(s["va"], str) else s["va"]
                    specs.append({"name": s["name"], "va": va,
                                  "type": s.get("type", "i32")})
                return {"ok": True, "vals": x.probe_reads(specs)}
            if cmd == "read_bytes":
                va = int(req["va"], 0) if isinstance(req["va"], str) else req["va"]
                raw = x.probe_read_bytes(va, int(req["len"]))
                return {"ok": True, "hex": bytes(raw).hex(), "len": len(bytes(raw))}
            if cmd == "read_chain":
                va = int(req["va"], 0) if isinstance(req["va"], str) else req["va"]
                offs = [int(o, 0) if isinstance(o, str) else o
                        for o in req.get("offsets", [])]
                v = x.probe_read_chain(va, offs, req.get("type", "i32"),
                                       int(req.get("len", 0)))
                if isinstance(v, (bytes, bytearray)):
                    return {"ok": True, "hex": bytes(v).hex(), "len": len(bytes(v))}
                return {"ok": True, "val": v}
            if cmd == "poke":
                va = int(req["va"], 0) if isinstance(req["va"], str) else req["va"]
                if "bytes" in req:
                    return {"ok": True, "n": x.probe_poke_bytes(va, req["bytes"])}
                x.probe_poke(va, req.get("type", "i32"), req["val"])
                return {"ok": True}
            if cmd == "callq":
                va = int(req["va"], 0) if isinstance(req["va"], str) else req["va"]
                return self._callq(va, req.get("args", []), req.get("argt", []),
                                   req.get("ret", "int32"),
                                   req.get("abi", "mscdecl"),
                                   float(req.get("timeout", 5.0)))
            if cmd == "turbo":
                return {"ok": True, "turbo": x.probe_set_turbo(bool(req["on"]))}
            if cmd == "activate":
                return {"ok": True, "active": x.probe_activate(bool(req["on"]))}
            if cmd == "anchors":
                with self._anchor_lock:
                    evs = list(self._anchors)
                    if req.get("clear"):
                        self._anchors = []
                return {"ok": True, "anchors": evs}
            if cmd == "sleep":
                time.sleep(float(req.get("ms", 200)) / 1000.0)
                return {"ok": True, "frame": self._last_frame}
            if cmd == "quit":
                return {"ok": True, "bye": True}
            return {"ok": False, "err": f"unknown cmd {cmd!r}"}
        except Exception as e:  # noqa: BLE001
            self._logline(f"[dispatch] {cmd} failed: {e!r}")
            return {"ok": False, "err": repr(e)}

    def serve(self, port: int):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("127.0.0.1", port))
        srv.listen(4)
        port = srv.getsockname()[1]
        CONTROL_JSON.parent.mkdir(parents=True, exist_ok=True)
        CONTROL_JSON.write_text(json.dumps(
            {"port": port, "pid": self.pid, "daemon_pid": os.getpid(),
             "run_dir": str(self.run_dir)}))
        self._logline(f"[serve] listening on 127.0.0.1:{port}")
        print(f"[probe_daemon] ready on 127.0.0.1:{port} (pid={self.pid}); "
              f"run_dir={self.run_dir}", flush=True)
        try:
            while not self._stop.is_set():
                conn, _ = srv.accept()
                with conn:
                    buf = b""
                    while not buf.endswith(b"\n"):
                        chunk = conn.recv(65536)
                        if not chunk:
                            break
                        buf += chunk
                    if not buf:
                        continue
                    try:
                        req = json.loads(buf.decode())
                    except Exception as e:
                        conn.sendall((json.dumps(
                            {"ok": False, "err": f"bad json: {e}"}) + "\n").encode())
                        continue
                    reply = self.dispatch(req)
                    conn.sendall((json.dumps(reply) + "\n").encode())
                    if reply.get("bye"):
                        break
        finally:
            try:
                CONTROL_JSON.unlink()
            except Exception:
                pass
            self.stop()


def main():
    ap = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--remote", default=fc.DEFAULT_REMOTE)
    ap.add_argument("--exe", default=str(fc.RETAIL_EXE))
    ap.add_argument("--cwd", default=str(fc.ASSET_CWD))
    ap.add_argument("--port", type=int, default=27210)
    ap.add_argument("--run-id", default="session")
    ap.add_argument("--rng-seed", default=None,
                    help="pin the LCG seed DAT_008a4f94 (hex ok) for "
                         "deterministic RNG probing")
    ap.add_argument("--view", action="store_true",
                    help="show the game window on the Windows host "
                         "(default: hidden; screenshots still work)")
    ap.add_argument("--realtime", action="store_true",
                    help="run at 1x (default: turbo fast-forward)")
    ap.add_argument("--turbo-step-ms", type=int, default=17)
    ap.add_argument("--audio", action="store_true",
                    help="leave audio on (default: silenced)")
    args = ap.parse_args()

    d = ProbeDaemon(args)
    d.start()
    d.serve(args.port)


if __name__ == "__main__":
    main()
