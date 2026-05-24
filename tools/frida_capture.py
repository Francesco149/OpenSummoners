#!/usr/bin/env python3
"""
tools/frida_capture.py — OpenSummoners Frida-driven retail harness.

Connects to a Windows-side frida-server (default 127.0.0.1:27042, shared
with the OpenMare / openrecet sibling projects), spawns
`vendor/original/sotes.exe` with `tools/frida/opensummoners-agent.js`
loaded, and lays down a run directory:

    <run_dir>/
        agent.log       (every send() event from the Frida agent)
        run.json        (metadata)

Phase A capability set (mirrors the agent's init() RPC):
    --hide-window         multi-API hide on all owned HWNDs
    --turbo               Sleep no-op + virtualised timeGetTime
    --silent-audio        WaveOut volume clamp; DSound covered Phase B
    --show-msgbox         disable the agent's MessageBox redirect (debugging)
    --turbo-step-ms N     advance the virtual clock by N ms per main-loop tick
    --max-frames N        soft cap on drained PeekMessage/GetMessage events
                          (≈ message-pump iterations, not literal rendered
                          frames — engine drains 1-4 messages per frame); the
                          script exits when the running count crosses N
    --duration-ms N       hard timeout in wall-clock ms

frida-server setup (Windows side, one-time — same as siblings):
    1. Download frida-server-<ver>-windows-x86_64.exe from the Frida
       releases page matching the Python frida version in nix.
    2. Rename → frida-server.exe, run it elevated (or let this script
       auto-spawn it with `Start-Process -Verb runAs`).
    3. Listens on 0.0.0.0:27042 so WSL can reach it across NAT.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import signal
import socket
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import frida


ROOT       = Path(__file__).resolve().parent.parent
AGENT_JS   = ROOT / "tools" / "frida" / "opensummoners-agent.js"
RETAIL_EXE = ROOT / "vendor" / "original" / "sotes.exe"
ASSET_CWD  = ROOT / "vendor" / "original"

DEFAULT_REMOTE = os.environ.get(
    "OPENSUMMONERS_FRIDA_REMOTE",
    # cutestation.soy is the Windows host's LAN-resolvable name.  WSL2
    # NAT doesn't loop back to the host's 127.0.0.1, so the LAN hostname
    # is the reliable path.  Override via env var for a different host.
    "cutestation.soy:27042",
)

# Where to look for the Windows-side frida-server.exe when auto-starting.
# Same layout the sibling projects use — one install serves all three.
DEFAULT_FRIDA_SERVER_EXE = Path(os.environ.get(
    "OPENSUMMONERS_FRIDA_SERVER_EXE",
    os.environ.get(
        "OPENMARE_FRIDA_SERVER_EXE",
        os.environ.get(
            "OPENRECET_FRIDA_SERVER_EXE",
            f"/mnt/c/Users/headpats/Documents/_devtools/"
            f"frida-server-{frida.__version__}-windows-x86_64/"
            f"frida-server-{frida.__version__}-windows-x86_64.exe"))))


# ─── helpers ──────────────────────────────────────────────────────────────


def wslpath_w(p: Path) -> str:
    """Translate a Linux path to its Windows form for frida-server.exe."""
    r = subprocess.run(
        ["wslpath", "-w", str(p)],
        capture_output=True, text=True, check=True,
    )
    return r.stdout.strip()


def _tcp_open(host: str, port: int, timeout: float = 1.0) -> bool:
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


def ensure_frida_server(remote: str, exe_wsl_path: Path,
                       startup_timeout_s: float = 15.0) -> bool:
    """Ensure frida-server.exe is reachable at `remote`.  If not, spawn it
    via `Start-Process -Verb runAs` on the Windows side (UAC will prompt)
    and poll until the port answers.  Returns True if reachable.

    Idempotent: if already up, returns True immediately.  Same elevation
    flow the siblings use — and the same frida-server.exe instance can
    serve all three projects.
    """
    host, _, port_s = remote.partition(":")
    port = int(port_s or "27042")
    if _tcp_open(host, port, timeout=1.0):
        return True

    if not exe_wsl_path.exists():
        print(f"[ensure_frida_server] {exe_wsl_path} not found; cannot auto-start. "
              f"Override with $OPENSUMMONERS_FRIDA_SERVER_EXE or start manually.",
              file=sys.stderr)
        return False

    win_exe = wslpath_w(exe_wsl_path)
    print(f"[ensure_frida_server] launching elevated: {win_exe} "
          f"(approve the UAC prompt)", file=sys.stderr)

    ps_cmd = (
        f"Start-Process -Verb runAs -WindowStyle Normal "
        f"-FilePath '{win_exe}' "
        f"-ArgumentList '-l','0.0.0.0:{port}'")
    subprocess.run(
        ["powershell.exe", "-NoProfile", "-Command", ps_cmd],
        check=False)

    deadline = time.monotonic() + startup_timeout_s
    while time.monotonic() < deadline:
        if _tcp_open(host, port, timeout=0.5):
            print(f"[ensure_frida_server] up on {remote}", file=sys.stderr)
            return True
        time.sleep(0.5)
    print(f"[ensure_frida_server] timed out waiting for {remote}",
          file=sys.stderr)
    return False


# ─── capture session ──────────────────────────────────────────────────────


@dataclass
class CaptureConfig:
    hide_window:        bool = True
    turbo:              bool = True
    silent_audio:       bool = True
    msgbox_redirect:    bool = True
    auto_click_launch:  bool = True
    auto_disable_sound: bool = True
    force_windowed:     bool = True
    turbo_step_ms:      int  = 17

    max_frames:        int  = 30_000
    duration_ms:       int  = 30_000

    remote:            str  = DEFAULT_REMOTE
    exe:               Path = RETAIL_EXE
    cwd:               Path = ASSET_CWD

    auto_start_server: bool = True
    server_exe:        Path = DEFAULT_FRIDA_SERVER_EXE

    run_dir:           Path | None = None


def _resolve_run_dir(base: Path | None) -> Path:
    if base is None:
        base = ROOT / "runs" / "retail-smoke"
    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    p = base / stamp
    p.mkdir(parents=True, exist_ok=True)
    return p


def run_capture(cfg: CaptureConfig) -> int:
    if not cfg.exe.exists():
        print(f"error: {cfg.exe} not found — run ./tools/setup.sh first",
              file=sys.stderr)
        return 2
    if not AGENT_JS.exists():
        print(f"error: agent {AGENT_JS} not found", file=sys.stderr)
        return 2

    if cfg.auto_start_server:
        if not ensure_frida_server(cfg.remote, cfg.server_exe):
            return 3

    run_dir = _resolve_run_dir(cfg.run_dir)
    log_path = run_dir / "agent.log"
    meta_path = run_dir / "run.json"
    log_f = log_path.open("w", encoding="utf-8")

    print(f"[frida_capture] run dir: {run_dir}", file=sys.stderr)
    print(f"[frida_capture] retail:  {cfg.exe}", file=sys.stderr)
    print(f"[frida_capture] remote:  {cfg.remote}", file=sys.stderr)
    print(f"[frida_capture] flags:   hide_window={cfg.hide_window} "
          f"turbo={cfg.turbo} silent_audio={cfg.silent_audio} "
          f"msgbox_redirect={cfg.msgbox_redirect}", file=sys.stderr)

    # Frida connects to the Windows-side server, which sees the .exe via
    # a native Windows path.  Translate.
    win_exe = wslpath_w(cfg.exe)
    win_cwd = wslpath_w(cfg.cwd)

    dm = frida.get_device_manager()
    device = dm.add_remote_device(cfg.remote)

    # Spawn suspended so the agent's init() RPC can install its hooks
    # before any engine code runs.  resume() after init() returns.
    pid = device.spawn([win_exe], cwd=win_cwd)
    print(f"[frida_capture] spawned pid={pid} (suspended)", file=sys.stderr)
    session = device.attach(pid)

    script = session.create_script(AGENT_JS.read_text(encoding="utf-8"))

    done = threading.Event()
    summary: dict[str, Any] = {
        "events": 0,
        "messageboxes": 0,
        "hwnds_owned": 0,
        "hides": 0,
        "msg_ticks": 0,
        "msg_count": 0,   # running total of drained Peek/GetMessage events
                          # (the agent's TICK_EVERY-batched count.count field)
        "turbo_ticks": 0,
        "errors": [],
    }

    def on_message(message: dict[str, Any], data: bytes | None) -> None:
        if message.get("type") == "error":
            log_f.write(f"[ERROR] {message.get('description')}\n"
                        f"  stack: {message.get('stack')}\n")
            log_f.flush()
            summary["errors"].append(message.get("description"))
            return
        payload = message.get("payload") or {}
        kind = payload.get("kind", "?")
        summary["events"] += 1
        log_f.write(json.dumps(payload, ensure_ascii=False) + "\n")
        if kind == "ready":
            print(f"[frida_capture] agent ready: {payload}", file=sys.stderr)
        elif kind == "messagebox":
            summary["messageboxes"] += 1
            print(f"[frida_capture] !!! MessageBox redirected: "
                  f"api={payload.get('api')} caption={payload.get('caption')!r} "
                  f"body={payload.get('body')!r}", file=sys.stderr)
        elif kind == "hwnd_owned":
            summary["hwnds_owned"] += 1
        elif kind == "hwnd_seen":
            # Periodic window-enumeration discovery.  Print on first
            # sight so the user sees the launcher dialog being detected.
            cls = payload.get("cls", "")
            ttl = payload.get("title", "")
            vis = payload.get("visible", False)
            print(f"[frida_capture] hwnd_seen: {cls!r} / {ttl!r} "
                  f"visible={vis} ({payload.get('hwnd')})", file=sys.stderr)
            summary.setdefault("hwnds_seen", 0)
            summary["hwnds_seen"] += 1
        elif kind == "hide_force":
            summary["hides"] += 1
        elif kind == "msg":
            summary["msg_ticks"] += 1
            # `count` is the agent's running total (g_msg_event_count at
            # emit time).  Each msg event arrives every TICK_EVERY=250
            # drained Peek/GetMessage returns, so this gives us the true
            # message-count up to the last batch boundary.
            c = payload.get("count")
            if isinstance(c, int) and c > summary["msg_count"]:
                summary["msg_count"] = c
        elif kind == "turbo_tick":
            summary["turbo_ticks"] += 1
        elif kind == "dialog_child":
            print(f"[frida_capture] dialog_child: cls={payload.get('cls')!r} "
                  f"text={payload.get('text')!r} ctrlId={payload.get('ctrlId')} "
                  f"hwnd={payload.get('hwnd')}", file=sys.stderr)
            summary.setdefault("dialog_children", 0)
            summary["dialog_children"] += 1
        elif kind == "dialog_action":
            print(f"[frida_capture] dialog_action: {payload.get('action')} "
                  f"→ {payload.get('target')} ({payload.get('text')!r}) "
                  f"hwnd={payload.get('hwnd')}", file=sys.stderr)
            summary.setdefault("dialog_actions", 0)
            summary["dialog_actions"] += 1
        elif kind == "error":
            print(f"[frida_capture] agent error in {payload.get('where')}: "
                  f"{payload.get('msg')}", file=sys.stderr)
        log_f.flush()

    script.on("message", on_message)
    script.load()

    # Drive the agent's init() RPC with our config.  Hooks install before
    # any engine code has run because the process is still suspended.
    script.exports_sync.init({
        "hide_window":        cfg.hide_window,
        "turbo":              cfg.turbo,
        "silent_audio":       cfg.silent_audio,
        "msgbox_redirect":    cfg.msgbox_redirect,
        "auto_click_launch":  cfg.auto_click_launch,
        "auto_disable_sound": cfg.auto_disable_sound,
        "force_windowed":     cfg.force_windowed,
        "turbo_step_ms":      cfg.turbo_step_ms,
    })

    device.resume(pid)
    print(f"[frida_capture] resumed pid={pid}", file=sys.stderr)

    def _shutdown(*_args: Any) -> None:
        done.set()

    signal.signal(signal.SIGINT,  _shutdown)
    signal.signal(signal.SIGTERM, _shutdown)

    # Wait up to duration_ms; bail early if the message-pump tick count
    # exceeds max_frames.  This is the harness pattern that lets a
    # smoke run exit voluntarily instead of relying on the launcher's
    # WM_CLOSE.
    deadline_s = time.monotonic() + (cfg.duration_ms / 1000.0)
    rc = 0
    try:
        while not done.is_set():
            if time.monotonic() > deadline_s:
                print(f"[frida_capture] duration_ms expired "
                      f"({cfg.duration_ms} ms)", file=sys.stderr)
                break
            if summary["msg_count"] >= cfg.max_frames:
                print(f"[frida_capture] reached {summary['msg_count']} "
                      f"drained messages (cap {cfg.max_frames})",
                      file=sys.stderr)
                break
            time.sleep(0.05)
    finally:
        # Polite teardown: try TerminateProcess via Frida, then drop the
        # session.  The Windows-side frida-server will close the process
        # handle for us.
        try:
            device.kill(pid)
        except Exception as e:
            print(f"[frida_capture] device.kill failed: {e}", file=sys.stderr)
        try:
            session.detach()
        except Exception:
            pass
        log_f.close()
        meta = {
            "exe":        str(cfg.exe),
            "cwd":        str(cfg.cwd),
            "remote":     cfg.remote,
            "config": {
                "hide_window":     cfg.hide_window,
                "turbo":           cfg.turbo,
                "silent_audio":    cfg.silent_audio,
                "msgbox_redirect": cfg.msgbox_redirect,
                "turbo_step_ms":   cfg.turbo_step_ms,
                "max_frames":      cfg.max_frames,
                "duration_ms":     cfg.duration_ms,
            },
            "summary":    summary,
            "started_at": dt.datetime.now().isoformat(),
        }
        meta_path.write_text(json.dumps(meta, indent=2, ensure_ascii=False))
        print(f"[frida_capture] summary: {summary}", file=sys.stderr)

    return rc


# ─── CLI ──────────────────────────────────────────────────────────────────


def main() -> int:
    p = argparse.ArgumentParser(
        description="OpenSummoners Frida harness — boot retail headless.")
    p.add_argument("--hide-window",    dest="hide_window",    action="store_true",  default=True)
    p.add_argument("--show-window",    dest="hide_window",    action="store_false")
    p.add_argument("--turbo",          dest="turbo",          action="store_true",  default=True)
    p.add_argument("--no-turbo",       dest="turbo",          action="store_false")
    p.add_argument("--silent-audio",   dest="silent_audio",   action="store_true",  default=True)
    p.add_argument("--no-silent-audio",dest="silent_audio",   action="store_false")
    p.add_argument("--show-msgbox",    dest="msgbox_redirect",action="store_false", default=True,
                   help="don't redirect MessageBox calls — for debugging the harness itself")
    p.add_argument("--turbo-step-ms",  type=int, default=17,
                   help="advance virtual clock by N ms per main-loop tick (default 17 ≈ 60 Hz)")
    p.add_argument("--max-frames",     type=int, default=30_000,
                   help="exit after N drained Peek/GetMessage events "
                        "(default 30000 ≈ ~5 min of typical 60 Hz play)")
    p.add_argument("--duration-ms",    type=int, default=30_000)
    p.add_argument("--remote",         default=DEFAULT_REMOTE)
    p.add_argument("--exe",            default=str(RETAIL_EXE), type=Path)
    p.add_argument("--cwd",            default=str(ASSET_CWD),  type=Path)
    p.add_argument("--no-auto-server", dest="auto_start_server", action="store_false", default=True)
    p.add_argument("--run-dir",        default=None, type=Path)
    args = p.parse_args()

    cfg = CaptureConfig(
        hide_window       = args.hide_window,
        turbo             = args.turbo,
        silent_audio      = args.silent_audio,
        msgbox_redirect   = args.msgbox_redirect,
        turbo_step_ms     = args.turbo_step_ms,
        max_frames        = args.max_frames,
        duration_ms       = args.duration_ms,
        remote            = args.remote,
        exe               = args.exe,
        cwd               = args.cwd,
        auto_start_server = args.auto_start_server,
        run_dir           = args.run_dir,
    )
    return run_capture(cfg)


if __name__ == "__main__":
    sys.exit(main())
