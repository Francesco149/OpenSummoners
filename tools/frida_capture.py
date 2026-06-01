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
import struct
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


def write_png_from_dib24(path: Path, w: int, h: int, stride: int,
                         pixels: bytes) -> None:
    """Write a lossless PNG from a GDI 24bpp top-down DIB's raw bits.

    The agent's CreateDIBSection produces 24bpp **BGR** rows, DWORD-aligned
    (`stride` = (w*3+3)&~3), top-down.  We strip the row padding, swap
    BGR→RGB, and emit a standard 8-bit RGB PNG (filter type 0 per row).
    Pure stdlib (zlib) — no Pillow dependency, and lossless so it's a
    drop-in for the old BMPs at a fraction of the size.
    """
    import zlib

    raw = bytearray()
    for y in range(h):
        row = pixels[y * stride : y * stride + w * 3]
        # BGR → RGB in place (memoryview slice assignment is fast enough
        # for 640x480; swap the two outer channels of each triple).
        rgb = bytearray(row)
        rgb[0::3], rgb[2::3] = row[2::3], row[0::3]
        raw.append(0)            # filter type 0 (None) for this scanline
        raw.extend(rgb)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)  # 8-bit, color type 2 (RGB)
    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", ihdr) +
           chunk(b"IDAT", zlib.compress(bytes(raw), 9)) +
           chunk(b"IEND", b""))
    path.write_bytes(png)


def parse_input_trace(path: Path) -> list[dict]:
    """Parse a JSONL input trace: one {"frame": N, "ids": [..]} per line.

    Tolerates blank lines and `# ...` comments.  `ids` entries may be decimal
    or `0x`-hex (as JSON numbers or strings).  Returns a list sorted by frame.
    """
    out: list[dict] = []
    for ln in path.read_text(encoding="utf-8").splitlines():
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        obj = json.loads(s)
        frame = int(obj["frame"])
        ids = []
        for v in (obj.get("ids") or []):
            ids.append(int(v, 0) if isinstance(v, str) else int(v))
        out.append({"frame": frame, "ids": ids})
    out.sort(key=lambda e: e["frame"])
    return out


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
    # When True, run_dir is used verbatim (no timestamp subdir).  mem_watch.py
    # sets this so it can post-process a known directory.
    exact_run_dir:     bool = False

    # ── structural-parity harness (off unless enabled) ──
    # call_trace: hook every VA in call_trace_vas onEnter, emit one row per
    # invocation to <run_dir>/call_trace.jsonl, batched per Flip frame.
    # call_trace_frames is an optional per-frame whitelist.
    call_trace:        bool = False
    call_trace_vas:    list[int] | None = None
    call_trace_frames: list[int] | None = None
    # mem_watch: MemoryAccessMonitor over mem_watch_regions (each a
    # {va,size,label,access} dict), emit trapped accesses to
    # <run_dir>/mem_watch.jsonl.  Driven by tools/mem_watch.py.
    mem_watch:         bool = False
    mem_watch_regions: list[dict] | None = None
    mem_watch_precise: bool = True

    # ── frame capture (DDraw surface → 24bpp BMP) ──
    # When capture is on, the agent GetDC/BitBlt's the surface at each
    # whitelisted Flip frame and sends the bits; we write frame_NNNNN.bmp
    # under <run_dir>/frames/.  capture_frames=None ⇒ every Flip frame
    # (use a whitelist — capturing all ~1900 is slow + huge).
    capture:           bool = False
    capture_frames:    list[int] | None = None

    # ── deterministic input injection ──
    # input_trace: list of {"frame": int, "ids": [int,...]} — each entry
    # injects those button ids as fresh presses at the first poll at-or-after
    # that Flip frame.  Replayed into retail's input ring by the agent.
    input_trace:       list[dict] | None = None
    inject_debug:      bool = False


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

    if cfg.exact_run_dir and cfg.run_dir is not None:
        run_dir = cfg.run_dir
        run_dir.mkdir(parents=True, exist_ok=True)
    else:
        run_dir = _resolve_run_dir(cfg.run_dir)
    cfg.run_dir = run_dir   # publish the resolved dir back to the caller
    log_path = run_dir / "agent.log"
    meta_path = run_dir / "run.json"
    log_f = log_path.open("w", encoding="utf-8")

    # Optional per-mode JSONL sinks (one row per event, with `frame`).
    call_trace_f = (run_dir / "call_trace.jsonl").open("w") if cfg.call_trace else None
    mem_watch_f  = (run_dir / "mem_watch.jsonl").open("w")  if cfg.mem_watch  else None
    frames_dir   = (run_dir / "frames") if cfg.capture else None
    if frames_dir is not None:
        frames_dir.mkdir(parents=True, exist_ok=True)

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
        "call_trace_events": 0,
        "mem_access_events": 0,
        "frames_captured": 0,
        "last_frame": -1,
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
        elif kind == "frame":
            frame = int(payload.get("frame", -1))
            w = int(payload.get("w", 0))
            h = int(payload.get("h", 0))
            stride = int(payload.get("stride", w * 3))
            if frames_dir is None or data is None or w == 0 or h == 0:
                log_f.write(f"[frame] frame={frame} BAD/ignored payload "
                            f"w={w} h={h} have_data={data is not None}\n")
            else:
                png_path = frames_dir / f"frame_{frame:05d}.png"
                write_png_from_dib24(png_path, w, h, stride, data)
                summary["frames_captured"] += 1
                back = payload.get("from_back")
                print(f"[frida_capture] frame {frame} captured "
                      f"{w}x{h} ({'back' if back else 'primary'}) → {png_path.name}",
                      file=sys.stderr)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "poll_dbg":
            print(f"[poll_dbg] flip={payload.get('frame')} "
                  f"btn=0x{int(payload.get('btn',0))&0xffffffff:x} "
                  f"now={payload.get('now')} slot63_id={payload.get('slot63_id')}",
                  file=sys.stderr)
        elif kind == "latch_dbg":
            print(f"[latch_dbg] flip={payload.get('frame')} "
                  f"dir={payload.get('dir')} ready={payload.get('ready')} "
                  f"enabled={payload.get('enabled')}", file=sys.stderr)
        elif kind == "input_mgr_resolved":
            print(f"[frida_capture] input-manager resolved: "
                  f"{payload.get('mgr')}", file=sys.stderr)
        elif kind == "inject":
            ids = payload.get("ids") or []
            ids_s = ",".join(f"0x{i:x}" for i in ids)
            print(f"[frida_capture] inject @flip={payload.get('frame')} "
                  f"(trace_frame={payload.get('trace_frame')}) ids=[{ids_s}]",
                  file=sys.stderr)
            summary.setdefault("injects", 0)
            summary["injects"] += 1
        elif kind == "flip_hook_ready":
            print(f"[frida_capture] flip frame anchor installed "
                  f"(va=0x{payload.get('va', 0):x})", file=sys.stderr)
        elif kind == "call_trace_hooked":
            print(f"[frida_capture] call_trace hooked: ok={payload.get('n_ok')} "
                  f"fail={payload.get('n_fail')} req={payload.get('n_req')}",
                  file=sys.stderr)
        elif kind == "mem_watch_ready":
            regs = payload.get("regions") or []
            print(f"[frida_capture] mem_watch armed {len(regs)} region(s) "
                  f"(precise={payload.get('precise')})", file=sys.stderr)
        elif kind == "call_trace_batch":
            frame = int(payload.get("frame", -1))
            events = payload.get("events") or []
            if call_trace_f is not None:
                for ev in events:
                    row = dict(ev)
                    row["frame"] = frame
                    call_trace_f.write(json.dumps(row) + "\n")
                call_trace_f.flush()
            summary["call_trace_events"] += len(events)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "mem_access_batch":
            frame = int(payload.get("frame", -1))
            events = payload.get("events") or []
            if mem_watch_f is not None:
                for ev in events:
                    row = dict(ev)
                    row["frame"] = frame
                    mem_watch_f.write(json.dumps(row) + "\n")
                mem_watch_f.flush()
            summary["mem_access_events"] += len(events)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
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
        "call_trace":         cfg.call_trace,
        "call_trace_vas":     [int(v) for v in (cfg.call_trace_vas or [])],
        "call_trace_frames":  [int(f) for f in (cfg.call_trace_frames or [])],
        "capture_frames_enabled": cfg.capture,
        "capture_frames":     [int(f) for f in (cfg.capture_frames or [])],
        "input_inject_enabled": bool(cfg.input_trace),
        "input_trace":        cfg.input_trace or [],
        "inject_debug":       cfg.inject_debug,
        "mem_watch":          cfg.mem_watch,
        "mem_watch_precise":  cfg.mem_watch_precise,
        "mem_watch_regions":  [
            {
                "va":     int(r["va"]),
                "size":   int(r.get("size", 16)),
                "label":  str(r.get("label", f"0x{int(r['va']):08x}")),
                "access": "rw" if r.get("access") == "rw" else "w",
            }
            for r in (cfg.mem_watch_regions or [])
        ],
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
        if call_trace_f is not None:
            call_trace_f.close()
        if mem_watch_f is not None:
            mem_watch_f.close()
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
    p.add_argument("--exact-run-dir",  action="store_true",
                   help="use --run-dir verbatim (no timestamp subdir)")
    p.add_argument("--call-trace",     action="store_true",
                   help="hook the engine VA list (tools/frida/data/"
                        "engine_vas_frida_safe.json, else engine_vas.json) "
                        "and emit one row per call to <run_dir>/call_trace.jsonl. "
                        "Pair with --call-trace-frames or output saturates.")
    p.add_argument("--call-trace-frames", default="",
                   help="comma-separated Flip-frame whitelist for --call-trace "
                        "(strongly recommended for non-title scenarios)")
    p.add_argument("--call-trace-vas-file", default=None, type=Path,
                   help="override the engine VA list (JSON: bare array or "
                        "{vas:[...]})")
    p.add_argument("--capture-frames", default=None,
                   help="capture DDraw frames to <run_dir>/frames/frame_NNNNN.png. "
                        "Comma-separated Flip-frame whitelist (e.g. '0,60,120'); "
                        "pass 'all' to capture every frame (slow + huge — avoid). "
                        "Implies --no-turbo behaviour is recommended (quirk #29).")
    p.add_argument("--input-trace", default=None, type=Path,
                   help="JSONL input trace ({\"frame\":N,\"ids\":[..]}) to inject "
                        "into retail's input ring (deterministic replay).")
    p.add_argument("--inject-debug", action="store_true",
                   help="log poll/latch internals in a small window around the "
                        "first scripted press (diagnostics).")
    args = p.parse_args()

    call_trace_vas = None
    call_trace_frames = None
    if args.call_trace:
        ct_path = args.call_trace_vas_file
        if ct_path is None:
            safe = ROOT / "tools" / "frida" / "data" / "engine_vas_frida_safe.json"
            cand = ROOT / "tools" / "frida" / "data" / "engine_vas.json"
            ct_path = safe if safe.exists() else cand
        if not ct_path.exists():
            p.error(f"--call-trace: VA list not found at {ct_path}; run "
                    f"tools/gen_engine_vas.py first")
        raw = json.loads(ct_path.read_text())
        call_trace_vas = raw["vas"] if isinstance(raw, dict) and "vas" in raw else list(raw)
        print(f"[frida_capture] call-trace: {len(call_trace_vas)} VAs from "
              f"{ct_path.name}", file=sys.stderr)
        if args.call_trace_frames:
            call_trace_frames = [int(x) for x in args.call_trace_frames.split(",") if x]

    input_trace = None
    if args.input_trace is not None:
        if not args.input_trace.exists():
            p.error(f"--input-trace: file not found: {args.input_trace}")
        input_trace = parse_input_trace(args.input_trace)
        print(f"[frida_capture] input-trace: {len(input_trace)} entries from "
              f"{args.input_trace.name}", file=sys.stderr)

    capture = False
    capture_frames: list[int] | None = None
    if args.capture_frames is not None:
        capture = True
        s = args.capture_frames.strip().lower()
        if s and s != "all":
            capture_frames = [int(x) for x in args.capture_frames.split(",") if x.strip()]
        # 'all' or empty → capture_frames stays None (every Flip frame)

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
        exact_run_dir     = args.exact_run_dir,
        call_trace        = args.call_trace,
        call_trace_vas    = call_trace_vas,
        call_trace_frames = call_trace_frames,
        capture           = capture,
        capture_frames    = capture_frames,
        input_trace       = input_trace,
        inject_debug      = args.inject_debug,
    )
    return run_capture(cfg)


if __name__ == "__main__":
    sys.exit(main())
