#!/usr/bin/env python3
"""Headless, sandboxed live probe for the retail EN-SE Japanese-voice patch.

The probe uses the verified EN-SE executable and the user's own assets, never the
OpenSummoners port.  It drives retail through the same input-ring model as the
trace harness and captures DirectDraw surfaces in-process, so neither input nor
screenshots require desktop focus.  A private NTFS sandbox protects the real
saves and install.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import select
import shutil
import sys
import threading
import time
from pathlib import Path

import frida

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import frida_capture as fc  # noqa: E402

AGENT = Path(__file__).with_name("repro_agent.js")
GENERAL_AGENT = ROOT / "tools" / "frida" / "opensummoners-agent.js"
UNPACKED_EXE = ROOT / "vendor" / "unpacked" / "editions" / "sotes-ense-en.exe"
DEFAULT_GAME = Path("/mnt/c/Program Files (x86)/Steam/steamapps/common/sotes")
DEFAULT_SANDBOX = Path("/mnt/c/oss-ennse-voice-repro")

ASSET_FILES = (
    "lizsoft.spl",
    "sotesd.dll",
    "sotesd_en.dll",
    "sotesp.dll",
    "sotesw.dll",
    "sotesx_d2.dll",
    "sotesx_d3.dll",
    "sotesx_s.dll",
)

REPORTED_SOUND_MATRIX = (
    {"name": "ghost-evasion", "key": 0xC789, "action": 0x07},
    {"name": "ghost-death", "key": 0xC789, "action": 0x0C},
    {"name": "harpy-damage", "key": 0xC792, "action": 0x09},
    {"name": "harpy-singing", "key": 0xC792, "action": 0x0A},
    {"name": "harpy-death", "key": 0xC792, "action": 0x0C},
    {"name": "babymage-damage", "key": 0xC829, "action": 0x09},
    {"name": "babymage-spellcast", "key": 0xC829, "action": 0x0A},
)


def link_or_copy(src: Path, dst: Path) -> None:
    """Hardlink large immutable assets on C:, falling back to a normal copy."""
    if dst.exists():
        if src.stat().st_size == dst.stat().st_size:
            return
        dst.unlink()
    try:
        os.link(src, dst)
    except OSError:
        shutil.copy2(src, dst)


def prepare_sandbox(game: Path, sandbox: Path, patched: bool,
                    copy_saves: bool) -> Path:
    sandbox.mkdir(parents=True, exist_ok=True)
    (sandbox / "temp").mkdir(exist_ok=True)
    (sandbox / "user").mkdir(exist_ok=True)

    for name in ASSET_FILES:
        src = game / name
        if not src.is_file():
            raise FileNotFoundError(f"required retail asset missing: {src}")
        link_or_copy(src, sandbox / name)

    shutil.copy2(UNPACKED_EXE, sandbox / "sotes_en.exe")
    if (game / "_sotes2.ini").is_file():
        shutil.copy2(game / "_sotes2.ini", sandbox / "_sotes2.ini")
    (sandbox / "temp" / "save_chk.tmp").touch()
    (sandbox / "user" / "save_chk.tmp").touch()

    if copy_saves:
        for src in (game / "user").iterdir():
            if src.is_file() and (src.name == "config.dat" or
                                  src.name.startswith("savedata") or
                                  src.name.startswith("wmap")):
                shutil.copy2(src, sandbox / "user" / src.name)
    elif (game / "user" / "config.dat").is_file():
        shutil.copy2(game / "user" / "config.dat", sandbox / "user" / "config.dat")

    proxy = sandbox / "version.dll"
    realver = sandbox / "realver.dll"
    if patched:
        built_proxy = ROOT / "build" / "version.dll"
        if not built_proxy.is_file():
            raise FileNotFoundError(f"voice patch not built: {built_proxy}")
        shutil.copy2(built_proxy, proxy)
        shutil.copy2(game / "realver.dll", realver)
    else:
        proxy.unlink(missing_ok=True)
        realver.unlink(missing_ok=True)

    # Logs are per-run evidence, not inherited state.
    for name in ("oss_voice.log", "fs_boot.log", "fs_warning.log"):
        (sandbox / name).unlink(missing_ok=True)
    return sandbox / "sotes_en.exe"


class ShotMailbox:
    """Collect binary DirectDraw frames emitted by the Frida callback."""

    def __init__(self, run_dir: Path) -> None:
        self.shots_dir = run_dir / "shots"
        self.shots_dir.mkdir(parents=True, exist_ok=True)
        self.condition = threading.Condition()
        self.frames: dict[int, list[dict]] = {}
        self.completed: dict[int, dict] = {}

    def handle(self, payload: dict, data: bytes | None) -> bool:
        kind = payload.get("kind")
        if kind == "surface_frame":
            shot = int(payload["shot"])
            index = int(payload["index"])
            record = dict(payload)
            if data is None:
                record["error"] = "surface frame arrived without pixel data"
            else:
                try:
                    width = int(payload["width"])
                    height = int(payload["height"])
                    stride = int(payload.get("stride", width * 3))
                    out = self.shots_dir / f"shot-{shot:03d}-surface-{index:02d}.png"
                    fc.write_png_from_dib24(out, width, height, stride, data)
                    record["path"] = str(out)
                except Exception as exc:  # noqa: BLE001
                    record["error"] = repr(exc)
            with self.condition:
                self.frames.setdefault(shot, []).append(record)
                self.condition.notify_all()
            return True
        if kind == "shot_complete":
            shot = int(payload["shot"])
            with self.condition:
                self.completed[shot] = dict(payload)
                self.condition.notify_all()
            return True
        return False

    def request(self, probe, timeout: float = 10.0) -> dict:
        shot = int(probe.exports_sync.shot())
        deadline = time.monotonic() + timeout
        with self.condition:
            while shot not in self.completed:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    break
                self.condition.wait(remaining)
            frames = list(self.frames.get(shot, []))
            complete = self.completed.get(shot)
        return {"ok": complete is not None and bool(frames),
                "shot": shot, "frames": frames, "complete": complete,
                "error": None if complete is not None else "shot timed out"}


def request_sound_test(probe, events: list[dict], key: int, action: int,
                       timeout: float = 10.0) -> dict:
    """Run one retail sound-set action and collect its request-scoped events."""
    start = len(events)
    request = int(probe.exports_sync.soundtest(key, action, [1, 0, 0, 0, 0]))
    deadline = time.monotonic() + timeout
    complete = None
    while time.monotonic() < deadline:
        complete = next((event for event in events[start:]
                         if event.get("kind") == "sound_test_complete" and
                         event.get("request") == request), None)
        if complete is not None:
            break
        time.sleep(0.02)
    related = [event for event in events[start:] if event.get("request") == request]
    return {"ok": complete is not None and "error" not in complete,
            "request": request, "events": related,
            "error": None if complete is not None else "sound test timed out"}


def run_reported_sound_matrix(probe, events: list[dict]) -> list[dict]:
    """Exercise every action named in the bug report through retail's mixer."""
    matrix = []
    for case in REPORTED_SOUND_MATRIX:
        outcome = request_sound_test(probe, events, case["key"], case["action"])
        constructed = next((event for event in outcome["events"]
                            if event.get("kind") == "sound_test_constructed"), {})
        complete = next((event for event in outcome["events"]
                         if event.get("kind") == "sound_test_complete"), {})
        entries = complete.get("group_after", {}).get("entries", [])
        directsound = [buffer.get("directsound")
                       for entry in entries
                       for buffer in (entry.get("resource") or {}).get("buffers", [])
                       if buffer.get("directsound") is not None]
        playing = [status for status in directsound if status.get("playing")]
        matrix.append({
            **case,
            "request": outcome["request"],
            "ok": outcome["ok"] and complete.get("result") == 1 and bool(playing),
            "play_result": complete.get("result"),
            "table_clips": [row.get("se") for row in constructed.get("table", [])],
            "runtime_clips": [(entry.get("resource") or {}).get("clip")
                              for entry in entries],
            "directsound_playing": len(playing),
            "directsound_status": directsound,
            "error": outcome["error"] or complete.get("error"),
        })
    return matrix


def interactive_loop(probe, events: list[dict], shots: ShotMailbox,
                     detached: threading.Event) -> dict:
    """Line-delimited JSON control surface for an exec PTY session."""
    print(json.dumps({"kind": "interactive_ready",
                      "commands": ["status", "press", "press_when", "clear",
                                   "sequence", "menu_press", "menu_confirm",
                                   "gameplay_press", "gameplay_confirm", "hold", "tap",
                                   "force_spawn", "clear_spawn_forces", "sound_test", "shot",
                                   "read", "write", "wait", "events",
                                   "quit"]}),
          flush=True)
    while not detached.is_set():
        readable, _, _ = select.select([sys.stdin], [], [], 0.25)
        if not readable:
            continue
        line = sys.stdin.readline()
        if not line:
            break
        try:
            req = json.loads(line)
            cmd = req.get("cmd")
            if cmd == "status":
                result = {"ok": True, "status": probe.exports_sync.status()}
            elif cmd == "press":
                result = {"ok": True, "queued": probe.exports_sync.press(req.get("ids", []))}
            elif cmd == "press_when":
                result = {"ok": True,
                          "queued": probe.exports_sync.presswhen(req.get("ids", []))}
            elif cmd == "clear":
                result = {"ok": bool(probe.exports_sync.clearpresses())}
            elif cmd == "sequence":
                outcomes = []
                for step in req.get("steps", []):
                    button = int(step["id"])
                    start = len(events)
                    probe.exports_sync.presswhen([button])
                    deadline = time.monotonic() + float(step.get("timeout", 30))
                    accepted = None
                    while time.monotonic() < deadline:
                        accepted = next((e for e in events[start:]
                                         if e.get("kind") == "input_polled" and
                                         e.get("button") == button and
                                         e.get("accepted") != 0), None)
                        if accepted is not None:
                            break
                        time.sleep(0.02)
                    outcomes.append({"id": button, "accepted": accepted})
                    if accepted is None:
                        break
                    time.sleep(float(step.get("after", 0.25)))
                result = {"ok": all(x["accepted"] is not None for x in outcomes),
                          "outcomes": outcomes, "status": probe.exports_sync.status()}
            elif cmd == "menu_confirm":
                start = len(events)
                probe.exports_sync.menuconfirm()
                deadline = time.monotonic() + float(req.get("timeout", 10))
                injected = None
                while time.monotonic() < deadline:
                    injected = next((event for event in events[start:]
                                     if event.get("kind") == "menu_input_injected" and
                                     37 in event.get("ids", [])),
                                    None)
                    if injected is not None:
                        break
                    time.sleep(0.02)
                result = {"ok": injected is not None, "injected": injected,
                          "status": probe.exports_sync.status()}
            elif cmd == "menu_press":
                start = len(events)
                ids = [int(value) for value in req.get("ids", [])]
                probe.exports_sync.menupress(ids)
                deadline = time.monotonic() + float(req.get("timeout", 10))
                injected = None
                while time.monotonic() < deadline:
                    injected = next((event for event in events[start:]
                                     if event.get("kind") == "menu_input_injected" and
                                     event.get("ids") == ids), None)
                    if injected is not None:
                        break
                    time.sleep(0.02)
                result = {"ok": injected is not None, "injected": injected,
                          "status": probe.exports_sync.status()}
            elif cmd in ("gameplay_confirm", "gameplay_press"):
                start = len(events)
                ids = ([37] if cmd == "gameplay_confirm"
                       else [int(value) for value in req.get("ids", [])])
                if cmd == "gameplay_confirm":
                    probe.exports_sync.gameplayconfirm()
                else:
                    probe.exports_sync.gameplaypress(ids)
                deadline = time.monotonic() + float(req.get("timeout", 10))
                injected = None
                while time.monotonic() < deadline:
                    injected = next((event for event in events[start:]
                                     if event.get("kind") == "gameplay_input_injected" and
                                     event.get("ids") == ids), None)
                    if injected is not None:
                        break
                    time.sleep(0.02)
                result = {"ok": injected is not None, "injected": injected,
                          "status": probe.exports_sync.status()}
            elif cmd == "hold":
                result = {"ok": True, "held": probe.exports_sync.hold(req.get("scancodes", []))}
            elif cmd == "tap":
                scancodes = req.get("scancodes", [])
                probe.exports_sync.hold(scancodes)
                time.sleep(float(req.get("seconds", 0.15)))
                result = {"ok": True, "released": scancodes,
                          "held": probe.exports_sync.hold([])}
            elif cmd == "force_spawn":
                target = (int(req["target"], 0) if isinstance(req["target"], str)
                          else int(req["target"]))
                source_value = req.get("source", 0)
                source = (int(source_value, 0) if isinstance(source_value, str)
                          else int(source_value))
                start = len(events)
                request = int(probe.exports_sync.forcespawn(
                    target, source, int(req.get("count", 1))))
                if not req.get("wait", True):
                    result = {"ok": True, "request": request,
                              "status": probe.exports_sync.status()}
                else:
                    deadline = time.monotonic() + float(req.get("timeout", 60))
                    created = None
                    while time.monotonic() < deadline and not detached.is_set():
                        created = next((event for event in events[start:]
                                        if event.get("kind") == "character_created" and
                                        event.get("request") == request), None)
                        if created is not None:
                            break
                        time.sleep(0.02)
                    result = {"ok": created is not None and
                                    created.get("force_complete") is True,
                              "request": request, "created": created,
                              "status": (probe.exports_sync.status()
                                         if not detached.is_set() else None),
                              "error": (None if created is not None else
                                        "force-spawn request timed out")}
            elif cmd == "clear_spawn_forces":
                result = {"ok": bool(probe.exports_sync.clearspawnforces())}
            elif cmd == "sound_test":
                key = int(req["key"], 0) if isinstance(req["key"], str) else int(req["key"])
                action = (int(req["action"], 0) if isinstance(req["action"], str)
                          else int(req["action"]))
                result = request_sound_test(probe, events, key, action,
                                            float(req.get("timeout", 10)))
            elif cmd == "read":
                result = {"ok": True, "value": probe.exports_sync.read(
                    int(req["va"], 0) if isinstance(req["va"], str) else req["va"],
                    req.get("type", "u32"))}
            elif cmd == "write":
                result = {"ok": True, "written": probe.exports_sync.write(
                    int(req["va"], 0) if isinstance(req["va"], str) else req["va"],
                    req.get("type", "u32"), req["value"])}
            elif cmd == "wait":
                time.sleep(float(req.get("seconds", 1)))
                result = {"ok": True, "status": probe.exports_sync.status()}
            elif cmd == "shot":
                result = shots.request(probe, float(req.get("timeout", 10)))
            elif cmd == "events":
                count = max(1, min(int(req.get("count", 20)), 200))
                result = {"ok": True, "events": events[-count:]}
            elif cmd == "quit":
                print(json.dumps({"ok": True, "bye": True}), flush=True)
                break
            else:
                result = {"ok": False, "error": f"unknown command: {cmd!r}"}
        except Exception as exc:
            result = {"ok": False, "error": repr(exc)}
        print(json.dumps(result, ensure_ascii=False), flush=True)
    if detached.is_set():
        return {"error": "retail process detached before interactive shutdown"}
    try:
        return probe.exports_sync.status()
    except Exception as exc:
        return {"error": repr(exc)}


def run(args: argparse.Namespace) -> int:
    mode = "patched" if args.patched else "stock"
    sandbox = args.sandbox / mode
    exe = prepare_sandbox(args.game, sandbox, args.patched, args.copy_saves)

    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = ROOT / "runs" / "ennse-voice-repro" / f"startup-{mode}-{stamp}"
    run_dir.mkdir(parents=True, exist_ok=True)
    log_path = run_dir / "events.jsonl"
    shots = ShotMailbox(run_dir)

    if not fc.ensure_frida_server(args.remote, fc.DEFAULT_FRIDA_SERVER_EXE):
        raise RuntimeError(f"frida-server unreachable at {args.remote}")
    device = frida.get_device_manager().add_remote_device(args.remote)
    pid = device.spawn([fc.wslpath_w(exe)], cwd=fc.wslpath_w(sandbox))
    done = threading.Event()
    detached = threading.Event()
    events: list[dict] = []
    detach_events: list[dict] = []
    status: dict = {"error": "probe did not reach status collection"}
    sound_matrix: list[dict] = []
    try:
        session = device.attach(pid)
        with log_path.open("w", encoding="utf-8", buffering=1) as log:
            def on_message(message, data) -> None:
                if message.get("type") == "error":
                    payload = {"kind": "frida_error",
                               "description": message.get("description"),
                               "stack": message.get("stack")}
                else:
                    payload = message.get("payload") or {"kind": message.get("type", "unknown")}
                if shots.handle(payload, data):
                    # Keep compact metadata in the evidence log; pixel payloads
                    # are preserved as lossless PNGs by ShotMailbox.
                    payload = dict(payload)
                    if payload.get("kind") == "surface_frame":
                        payload["pixel_bytes"] = len(data) if data is not None else 0
                events.append(payload)
                log.write(json.dumps(payload, ensure_ascii=False) + "\n")
                if payload.get("kind") == "registrar_exit":
                    done.set()

            def on_detached(reason, crash=None) -> None:
                payload = {"kind": "session_detached", "reason": str(reason)}
                if crash is not None:
                    payload["crash"] = str(crash)
                    report = getattr(crash, "report", None)
                    if report:
                        payload["crash_report"] = str(report)
                detach_events.append(payload)
                events.append(payload)
                try:
                    if not log.closed:
                        log.write(json.dumps(payload, ensure_ascii=False) + "\n")
                except Exception:
                    pass
                detached.set()

            session.on("detached", on_detached)

            general = session.create_script(GENERAL_AGENT.read_text(encoding="utf-8"))
            general.on("message", on_message)
            general.load()
            general.exports_sync.init({
                "module_name": exe.name,
                "hide_window": not args.show_window,
                "force_windowed": True,
                "silent_audio": False,
                "turbo": False,
                "auto_click_launch": True,
                "auto_disable_sound": False,
                "msgbox_redirect": True,
                "seed_pin": False,
            })

            probe = session.create_script(AGENT.read_text(encoding="utf-8"))
            probe.on("message", on_message)
            probe.load()
            probe.exports_sync.init()

            device.resume(pid)
            done.wait(args.timeout)
            # Let the patch finish manager creation and let the bank poll report it.
            time.sleep(args.settle)
            if args.sound_matrix:
                sound_matrix = run_reported_sound_matrix(probe, events)
                status = probe.exports_sync.status()
            elif args.interactive:
                status = interactive_loop(probe, events, shots, detached)
            else:
                try:
                    status = probe.exports_sync.status()
                except Exception as exc:  # process may have exited on its own
                    status = {"error": repr(exc)}
            try:
                probe.exports_sync.stop()
            except Exception:
                pass
    finally:
        try:
            device.kill(pid)  # exact PID spawned above; never sweep by process name
        except Exception:
            pass

    registrar = [e for e in events if e.get("kind") == "registrar_exit"]
    summary = {
        "mode": mode,
        "pid": pid,
        "sandbox": str(sandbox),
        "event_log": str(log_path),
        "status": status,
        "registrar_runs": registrar,
        "detach_events": detach_events,
        "sound_matrix": sound_matrix,
        "voice_log": ((sandbox / "oss_voice.log").read_text(errors="replace")
                      if (sandbox / "oss_voice.log").is_file() else ""),
    }
    (run_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if not registrar:
        return 2
    if args.sound_matrix and not all(case.get("ok") for case in sound_matrix):
        return 3
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--patched", action="store_true", default=True,
                      help="load the current build/version.dll (default)")
    mode.add_argument("--stock", dest="patched", action="store_false",
                      help="run EN-SE without the JP voice proxy")
    parser.add_argument("--game", type=Path, default=DEFAULT_GAME)
    parser.add_argument("--sandbox", type=Path, default=DEFAULT_SANDBOX)
    parser.add_argument("--copy-saves", action="store_true",
                        help="copy real saves into the private sandbox for later encounter driving")
    parser.add_argument("--show-window", action="store_true")
    parser.add_argument("--interactive", action="store_true",
                        help="keep retail alive and accept line-delimited JSON commands on stdin")
    parser.add_argument("--sound-matrix", action="store_true",
                        help="exercise every monster action named in the report, then exit")
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--settle", type=float, default=2.0)
    parser.add_argument("--remote", default=fc.DEFAULT_REMOTE)
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
