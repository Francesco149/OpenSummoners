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
# The Steamless-unpacked, DRM-free PE.  vendor/original is a *symlink to the
# game dir*, so vendor/original/sotes.exe is the PACKED Steam-DRM exe — it
# stalls in the launcher under Frida (0 frames).  setup.sh produces the clean
# PE at vendor/unpacked/sotes.unpacked.exe, but the engine resolves its asset
# paths (config.dat, sotesd.dll, …) relative to its OWN module directory
# (GetModuleFileName), NOT the cwd — so the unpacked exe must sit *inside* the
# game dir next to those files.  We spawn the copy co-located there.
RETAIL_EXE = ROOT / "vendor" / "original" / "sotes.unpacked.exe"
# cwd = the game dir (vendor/original is the symlink to it).
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
    # Lockstep clock (TAS determinism): freeze the virtual GetTickCount
    # between Flips and bank exactly one update quantum per present, so
    # retail renders 1 update / present like the port (frame-for-frame
    # diffable).  See the agent's lockstep block.  Off by default.
    lockstep:            bool = False
    lockstep_step_ms:    int  = 0x10
    lockstep_epsilon_ms: int  = 1
    # Tally rand() call sites between newgame_enter and prologue_enter — finds
    # the transition's unaccounted rand consumer (TAS RNG-desync diagnosis).
    rand_probe:          bool = False

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
    # field_spec (B2): {"<va_dec>": [{name,src,va,index,off,type}]} from
    # tools/flow/retail_fields.json — the agent reads the declared engine state
    # at each spec'd VA's onEnter into an `f:{…}` payload (joined to the port's
    # CALL_TRACE_* fields by va+field-name), consumed by tools/flow_diff.py.
    field_spec:        dict | None = None
    # mem_watch: MemoryAccessMonitor over mem_watch_regions (each a
    # {va,size,label,access} dict), emit trapped accesses to
    # <run_dir>/mem_watch.jsonl.  Driven by tools/mem_watch.py.
    mem_watch:         bool = False
    mem_watch_regions: list[dict] | None = None
    mem_watch_precise: bool = True
    # Re-arm the page monitor once per Flip instead of per-access — one trap
    # per frame, no hot-page re-arm livelock.  Required for fields on a page
    # the engine touches every frame (e.g. the camera/view object).
    mem_watch_flip_rearm: bool = False
    # Use a hardware watchpoint (DR register, write-only on the exact bytes)
    # instead of MemoryAccessMonitor — the fitting tool for a hot heap field
    # (zero neighbour overhead, hardware auto-rearm, no livelock).  Chain regions.
    mem_watch_hw: bool = False

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

    # ── cursor-level probe (parity-ledger R1) ──
    # Hook FUN_0056c470 and log the per-frame level_num retail draws the menu
    # cursor with.  Prints distinct values + writes <run_dir>/cursor_level.jsonl.
    cursor_probe:      bool = False

    # ── intro-fade probe (LOGO/SPARKLE wiring verification, ckpt 30) ──
    # Hook FUN_00448c80 (the fade→alpha ramp) and log the first (value,div) per
    # Flip — in phases 0..4 that is the studio/title logo fade.  Writes
    # <run_dir>/fade_level.jsonl so a port frame can be matched to a retail
    # golden at an equal fade and diffed (the R1 method, for the logos).
    fade_probe:        bool = False

    # ── parallax far-plane probe (in-game sky/mountain background) ──
    # Hook FUN_00490cd0 (the background renderer) + its inner cel-select/blit
    # and log the descriptor fields (3 layers' banks/baseY/wrap/parallax) +
    # the per-tile blits to <run_dir>/parallax.jsonl.  Pair with --input-trace
    # + --parallax-frames to scope it to the in-game town (e.g. '1140,1300').
    parallax_probe:    bool = False
    parallax_lo:       int  = 0
    parallax_hi:       int  = 0

    # ── intro-pace probe (parity residual R3) ──
    # Timestamp Flips (writes <run_dir>/pace.jsonl) to measure the flip RATE
    # and the wall-clock to menu onset.  Use with --no-turbo.  pace_every =
    # emit one sample per N flips.
    pace_probe:        bool = False
    pace_every:        int  = 30

    # ── GDI text probe (text-renderer parity, ckpt 36 part 2) ──
    # Hook gdi32!TextOutA/ExtTextOutA and record each glyph draw's
    # (x, y, bytes, text colour, bk mode) + the selected font's LOGFONTA.
    # This is the retail ground truth for the dynamic-text GDI path (which
    # ar_register_fonts HFONT a menu picks, the colours, the per-byte
    # advance).  Writes <run_dir>/textout.jsonl (deduped distinct draws).
    # Drive retail to a GDI-text scene (the new-game menu) with --input-trace.
    # textout_lo/hi restrict recording to a flip window so the intro/attract
    # debug text (which live-updates and would flood the channel) is skipped.
    textout_probe:     bool = False
    textout_lo:        int  = 0
    textout_hi:        int  = 0

    # ── box-widget render probe (new-game config chrome, ckpt 40) ──
    # Hook FUN_0048d940 (sprite-cell render) and dump the box panel's 9-slice
    # composition (bank PE resource id + frame ids + on-screen dst rects) to
    # box_cells.jsonl.  Drive retail to the new-game menu with --input-trace;
    # box_lo/hi restrict recording to that flip window (the bank renders many
    # other UI sprites otherwise).
    box_probe:         bool = False
    box_lo:            int  = 0
    box_hi:            int  = 0

    # ── resource-load probe (which DLL banks a scene pulls, ckpt 51) ──
    # Hook bs_decode_resource (FUN_005b7800) and dump each distinct
    # (module, id, type) PE-resource load to res_loads.jsonl.  Used to answer
    # "which sprite/world/audio banks does the in-game map 0x3f2 load, and from
    # which DLL".  res_lo/hi restrict recording to a flip window.
    res_probe:         bool = False
    res_lo:            int  = 0
    res_hi:            int  = 0

    # ── RNG seed pin (phase-7 sparkle parity, ckpt 31) ──
    # Write a fixed seed into DAT_008a4f94 just before the first FUN_0056c070
    # sparkle spawn, so retail's twinkle stream matches the port's pinned-seed
    # build bit-for-bit.  On by default (user directive: pin both sides).
    seed_pin:          bool = True
    seed_value:        int  = 0x4f5347   # OSS_RNG_DEFAULT_SEED


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
    cursor_f     = (run_dir / "cursor_level.jsonl").open("w") if cfg.cursor_probe else None
    fade_f       = (run_dir / "fade_level.jsonl").open("w") if cfg.fade_probe else None
    parallax_f   = (run_dir / "parallax.jsonl").open("w") if cfg.parallax_probe else None
    pace_f       = (run_dir / "pace.jsonl").open("w") if cfg.pace_probe else None
    textout_f    = (run_dir / "textout.jsonl").open("w") if cfg.textout_probe else None
    box_f        = (run_dir / "box_cells.jsonl").open("w") if cfg.box_probe else None
    res_f        = (run_dir / "res_loads.jsonl").open("w") if cfg.res_probe else None
    box_res: dict[int, int] = {}        # box-art resource id -> distinct-cell count
    cursor_seen: dict[int, int] = {}   # level_num -> count, for the summary
    textout_fonts: dict[str, int] = {}  # font fingerprint -> distinct-draw count
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
            sim_tick = int(payload.get("sim_tick", -1))
            w = int(payload.get("w", 0))
            h = int(payload.get("h", 0))
            stride = int(payload.get("stride", w * 3))
            if frames_dir is None or data is None or w == 0 or h == 0:
                log_f.write(f"[frame] frame={frame} BAD/ignored payload "
                            f"w={w} h={h} have_data={data is not None}\n")
            else:
                # Name carries BOTH the (wall-clock, non-deterministic) Flip
                # index and the DETERMINISTIC sim-tick — match diffs on the
                # latter (engine-quirk #75).
                png_path = frames_dir / f"frame_{frame:05d}_t{sim_tick:06d}.png"
                write_png_from_dib24(png_path, w, h, stride, data)
                # Sidecar manifest: flip→sim_tick for the diff tool.
                with (run_dir / "frames_manifest.jsonl").open("a") as mf:
                    mf.write(json.dumps({"flip": frame, "sim_tick": sim_tick,
                                         "file": png_path.name}) + "\n")
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
        elif kind == "cursor_probe_first":
            ms = payload.get("ms")
            ms_s = f" t={ms}ms" if ms is not None else ""
            print(f"[frida_capture] cursor_probe first hit @ frame "
                  f"{payload.get('frame')}{ms_s} (= MENU ONSET): "
                  f"ecx=0x{int(payload.get('ecx',0)):08x} "
                  f"ret_va=0x{int(payload.get('ret_va',0)):06x} "
                  f"slots={payload.get('slots')}", file=sys.stderr)
        elif kind == "fade_probe_first":
            print(f"[frida_capture] fade_probe first hit @ frame "
                  f"{payload.get('frame')}: value={payload.get('value')} "
                  f"div={payload.get('div')} "
                  f"ecx=0x{int(payload.get('ecx',0)):08x} "
                  f"ret_va=0x{int(payload.get('ret_va',0)):06x}", file=sys.stderr)
        elif kind == "parallax_first":
            print(f"[frida_capture] parallax FIRST hit @ flip "
                  f"{payload.get('frame')} (FUN_00490cd0 first ran) "
                  f"ret_va=0x{int(payload.get('ret_va',0)):06x}", file=sys.stderr)
        elif kind == "parallax_desc":
            if parallax_f:
                parallax_f.write(json.dumps(payload) + "\n"); parallax_f.flush()
            ecx = int(payload.get("ecx", 0)); grid = int(payload.get("grid", 0))
            view = int(payload.get("view", 0))
            where = ("==grid" if ecx == grid and grid else
                     "==view" if ecx == view and view else "==?")
            print(f"[frida_capture] parallax_desc @ flip {payload.get('frame')}: "
                  f"ecx=0x{ecx:08x}{where} "
                  f"A={payload.get('A')} B={payload.get('B')} C={payload.get('C')} "
                  f"ret_va=0x{int(payload.get('ret_va',0)):06x}", file=sys.stderr)
            print(f"[frida_capture]   raw32={payload.get('raw32')}", file=sys.stderr)
        elif kind == "parallax_blit":
            if parallax_f:
                parallax_f.write(json.dumps(payload) + "\n"); parallax_f.flush()
            print(f"[frida_capture]   parallax_blit bank=0x{int(payload.get('bank',0)):x} "
                  f"frame={payload.get('frame_idx')} "
                  f"x={payload.get('x')} y={payload.get('y')}", file=sys.stderr)
        elif kind == "anchor":
            name = payload.get("name", "anchor")
            rng = payload.get("rng")
            rng_s = f" rng=0x{int(rng):08x}" if isinstance(rng, int) else ""
            print(f"[frida_capture] TAS anchor '{name}' @ flip "
                  f"{payload.get('frame')}{rng_s}", file=sys.stderr)
            summary.setdefault("anchors", {})[name] = payload.get("frame")
            if isinstance(rng, int):
                summary.setdefault("anchor_rng", {})[name] = rng
        elif kind == "rand_window":
            callers = payload.get("callers", {})
            order = sorted(callers.items(), key=lambda kv: -kv[1])
            print(f"[frida_capture] rand_window {payload.get('from')}→"
                  f"{payload.get('to')}: {payload.get('total')} rand() calls, "
                  f"by caller: {order}", file=sys.stderr)
            summary["rand_window"] = {"total": payload.get("total"),
                                      "callers": callers}
        elif kind == "lockstep_armed":
            print(f"[frida_capture] lockstep clock ARMED @ flip "
                  f"{payload.get('frame')} clock={payload.get('clock_ms')}ms "
                  f"step={payload.get('step_ms')}ms "
                  f"eps={payload.get('epsilon_ms')}ms", file=sys.stderr)
            summary["lockstep_armed"] = {
                "frame": payload.get("frame"),
                "step_ms": payload.get("step_ms"),
                "epsilon_ms": payload.get("epsilon_ms"),
            }
        elif kind == "seed_pinned":
            print(f"[frida_capture] RNG seed pinned @ frame "
                  f"{payload.get('frame')} (first FUN_0056c070): "
                  f"DAT_008a4f94 0x{int(payload.get('before',0)):08x} -> "
                  f"0x{int(payload.get('value',0)):08x}", file=sys.stderr)
            summary["seed_pinned"] = {
                "frame": payload.get("frame"),
                "before": payload.get("before"),
                "value": payload.get("value"),
            }
        elif kind == "fade_level":
            frame = int(payload.get("frame", -1))
            if fade_f is not None:
                fade_f.write(json.dumps({
                    "frame": frame, "value": payload.get("value"),
                    "div": payload.get("div")}) + "\n")
                fade_f.flush()
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "pace_sample":
            frame = int(payload.get("frame", -1))
            ms    = int(payload.get("ms", 0))
            if pace_f is not None:
                pace_f.write(json.dumps({"frame": frame, "ms": ms}) + "\n")
                pace_f.flush()
            rate = (frame / (ms / 1000.0)) if ms > 0 else 0.0
            print(f"[frida_capture] pace: flip {frame} @ {ms}ms "
                  f"({rate:.1f} flips/s avg)", file=sys.stderr)
        elif kind == "textout":
            frame = int(payload.get("frame", -1))
            font  = payload.get("font") or {}
            fp = (f"{font.get('h')}x{font.get('w')} {font.get('face')!r}"
                  f" it={font.get('italic')}" if font else "?")
            textout_fonts[fp] = textout_fonts.get(fp, 0) + 1
            color = payload.get("color")
            color_s = f"0x{color:06x}" if isinstance(color, int) else str(color)
            if textout_f is not None:
                textout_f.write(json.dumps(payload, ensure_ascii=False) + "\n")
                textout_f.flush()
            summary.setdefault("textout_draws", 0)
            summary["textout_draws"] += 1
            print(f"[frida_capture] textout @flip={frame} "
                  f"({payload.get('x')},{payload.get('y')}) "
                  f"{payload.get('text')!r} color={color_s} "
                  f"bk={payload.get('bkmode')} font=[{fp}]", file=sys.stderr)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "box_cell":
            frame = int(payload.get("frame", -1))
            rid   = payload.get("res_id")
            if isinstance(rid, int):
                box_res[rid] = box_res.get(rid, 0) + 1
            rid_s = f"0x{rid:04x}" if isinstance(rid, int) else str(rid)
            if box_f is not None:
                box_f.write(json.dumps(payload, ensure_ascii=False) + "\n")
                box_f.flush()
            summary.setdefault("box_cells", 0)
            summary["box_cells"] += 1
            print(f"[frida_capture] box_cell @flip={frame} res={rid_s} "
                  f"type={payload.get('type')} frame={payload.get('frameSel')} "
                  f"dst=({payload.get('scrx')},{payload.get('scry')}) "
                  f"{payload.get('w')}x{payload.get('h')} "
                  f"base={payload.get('base')} cnt={payload.get('count')} "
                  f"idx={payload.get('idx')} frames={payload.get('frames')}",
                  file=sys.stderr)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "box_frame":
            frame = int(payload.get("frame", -1))
            rid   = payload.get("res_id")
            rid_s = f"0x{rid:04x}" if isinstance(rid, int) else str(rid)
            if box_f is not None:
                box_f.write(json.dumps(payload, ensure_ascii=False) + "\n")
                box_f.flush()
            summary.setdefault("box_frames", 0)
            summary["box_frames"] += 1
            print(f"[frida_capture] box_frame[{payload.get('tag')}] @flip={frame} "
                  f"res={rid_s} spec={payload.get('spec')} "
                  f"node={payload.get('nodeW')}x{payload.get('nodeH')} "
                  f"fade={payload.get('fade')} off=({payload.get('ox')},"
                  f"{payload.get('oy')})", file=sys.stderr)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "res_load":
            frame = int(payload.get("frame", -1))
            rid   = payload.get("id")
            rid_s = f"0x{rid:04x}" if isinstance(rid, int) else str(rid)
            if res_f is not None:
                res_f.write(json.dumps(payload, ensure_ascii=False) + "\n")
                res_f.flush()
            summary.setdefault("res_loads", 0)
            summary["res_loads"] += 1
            print(f"[frida_capture] res_load @flip={frame} "
                  f"module={payload.get('module')} id={rid_s} "
                  f"type={payload.get('type')} ret={payload.get('ret_va')}",
                  file=sys.stderr)
            if frame > summary["last_frame"]:
                summary["last_frame"] = frame
        elif kind == "cursor_level":
            frame = int(payload.get("frame", -1))
            num   = payload.get("num")
            cursor_seen[num] = cursor_seen.get(num, 0) + 1
            if cursor_f is not None:
                cursor_f.write(json.dumps({
                    "frame": frame, "num": num, "div": payload.get("div"),
                    "x": payload.get("x"), "y": payload.get("y")}) + "\n")
                cursor_f.flush()
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
        "lockstep":           cfg.lockstep,
        "lockstep_step_ms":   int(cfg.lockstep_step_ms),
        "lockstep_epsilon_ms": int(cfg.lockstep_epsilon_ms),
        "rand_probe":         cfg.rand_probe,
        "call_trace":         cfg.call_trace,
        "call_trace_vas":     [int(v) for v in (cfg.call_trace_vas or [])],
        "call_trace_frames":  [int(f) for f in (cfg.call_trace_frames or [])],
        "field_spec":         cfg.field_spec or {},
        "capture_frames_enabled": cfg.capture,
        "capture_frames":     [int(f) for f in (cfg.capture_frames or [])],
        "input_inject_enabled": bool(cfg.input_trace),
        "input_trace":        cfg.input_trace or [],
        "inject_debug":       cfg.inject_debug,
        "cursor_probe":       cfg.cursor_probe,
        "fade_probe":         cfg.fade_probe,
        "pace_probe":         cfg.pace_probe,
        "pace_every":         cfg.pace_every,
        "textout_probe":      cfg.textout_probe,
        "textout_lo":         int(cfg.textout_lo),
        "textout_hi":         int(cfg.textout_hi),
        "box_probe":          cfg.box_probe,
        "box_lo":             int(cfg.box_lo),
        "box_hi":             int(cfg.box_hi),
        "res_probe":          cfg.res_probe,
        "res_lo":             int(cfg.res_lo),
        "res_hi":             int(cfg.res_hi),
        "parallax_probe":     cfg.parallax_probe,
        "parallax_lo":        int(cfg.parallax_lo),
        "parallax_hi":        int(cfg.parallax_hi),
        "seed_pin":           cfg.seed_pin,
        "seed_value":         int(cfg.seed_value),
        "mem_watch":          cfg.mem_watch,
        "mem_watch_precise":  cfg.mem_watch_precise,
        "mem_watch_flip_rearm": cfg.mem_watch_flip_rearm,
        "mem_watch_hw":         cfg.mem_watch_hw,
        "mem_watch_regions":  [
            {
                "va":     int(r["va"]),
                "size":   int(r.get("size", 16)),
                "label":  str(r.get("label", f"0x{int(r['va']):08x}")),
                "access": "rw" if r.get("access") == "rw" else "w",
                # Chain region: resolve a heap field through a global root at
                # runtime (e.g. the camera/view *(*(0x8a9b50)+0x104c)+0x60).
                "chain":       bool(r.get("chain", False)),
                "hops":        [int(h) for h in r.get("hops", [])],
                "off":         int(r.get("off", 0)),
                "arm_at_flip": int(r.get("arm_at_flip", 0)),
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
        if cursor_f is not None:
            cursor_f.close()
            if cursor_seen:
                dist = ", ".join(f"num={n}×{c}" for n, c in
                                 sorted(cursor_seen.items(),
                                        key=lambda kv: -kv[1]))
                print(f"[frida_capture] cursor_level distinct: {dist}",
                      file=sys.stderr)
        if pace_f is not None:
            pace_f.close()
        if textout_f is not None:
            textout_f.close()
            if textout_fonts:
                dist = ", ".join(f"[{fp}]×{c}" for fp, c in
                                 sorted(textout_fonts.items(),
                                        key=lambda kv: -kv[1]))
                print(f"[frida_capture] textout distinct draws by font: {dist}",
                      file=sys.stderr)
            else:
                print("[frida_capture] textout: NO TextOutA/ExtTextOutA calls "
                      "seen (scene never rendered GDI text?)", file=sys.stderr)
        if box_f is not None:
            box_f.close()
            if box_res:
                dist = ", ".join(f"0x{rid:04x}×{c}" for rid, c in
                                 sorted(box_res.items(), key=lambda kv: -kv[1]))
                print(f"[frida_capture] box_cells distinct cells by resource id: "
                      f"{dist}", file=sys.stderr)
            else:
                print("[frida_capture] box_cells: NO FUN_0048d940 sprite-cell "
                      "renders seen in the flip window (box never drawn?)",
                      file=sys.stderr)
        if res_f is not None:
            res_f.close()
            n = summary.get("res_loads", 0)
            print(f"[frida_capture] res_loads: {n} distinct (module,id,type) "
                  f"resource loads → {run_dir / 'res_loads.jsonl'}",
                  file=sys.stderr)
        if parallax_f is not None:
            parallax_f.close()
            print(f"[frida_capture] parallax → {run_dir / 'parallax.jsonl'}",
                  file=sys.stderr)
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
                "lockstep":        cfg.lockstep,
                "lockstep_step_ms": cfg.lockstep_step_ms,
                "lockstep_epsilon_ms": cfg.lockstep_epsilon_ms,
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
    p.add_argument("--lockstep",       action="store_true", default=False,
                   help="TAS lockstep clock: freeze GetTickCount between Flips and bank "
                        "one update quantum per present, so retail renders 1 update/present "
                        "like the port (frame-for-frame diffable). Implies a flip anchor.")
    p.add_argument("--lockstep-step-ms", type=int, default=0x10,
                   help="update quantum banked per Flip in lockstep mode (default 16)")
    p.add_argument("--lockstep-epsilon-ms", type=int, default=1,
                   help="per-call clock creep in lockstep mode to defeat busy-wait hangs "
                        "(default 1; set 0 for a pure per-Flip freeze)")
    p.add_argument("--rand-probe", action="store_true", default=False,
                   help="tally rand() (0x5bf505) call sites between newgame_enter and "
                        "prologue_enter — locates the transition's rand consumer by caller VA")
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
    p.add_argument("--field-spec", default=None, type=Path,
                   help="B2 field-bearing flow trace: read engine state at each "
                        "spec'd VA's onEnter into an f:{…} payload for "
                        "tools/flow_diff.py (default tools/flow/retail_fields.json "
                        "when --call-trace is on). Its VAs are auto-hooked. Use "
                        "--field-spec-only to hook ONLY them (bounded output).")
    p.add_argument("--field-spec-only", action="store_true",
                   help="with --call-trace: hook ONLY the field-spec VAs, not the "
                        "full engine VA list — the bounded field-trace mode.")
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
    p.add_argument("--fade-probe", action="store_true",
                   help="hook FUN_00448c80 and log the first fade value per "
                        "Flip (the studio/title logo fade in phases 0..4) to "
                        "<run>/fade_level.jsonl — for matched-fade logo diffs")
    p.add_argument("--cursor-probe", action="store_true",
                   help="hook FUN_0056c470 and log the per-frame menu-cursor "
                        "level_num to <run_dir>/cursor_level.jsonl "
                        "(parity-ledger R1). Use --no-turbo.")
    p.add_argument("--pace-probe", action="store_true",
                   help="timestamp Flips to <run_dir>/pace.jsonl to measure the "
                        "flip RATE + wall-clock to menu onset (parity residual "
                        "R3). Use --no-turbo. Implies a flip-frame anchor.")
    p.add_argument("--pace-every", type=int, default=30,
                   help="emit one pace sample per N flips (default 30).")
    p.add_argument("--seed-pin",    dest="seed_pin", action="store_true",  default=True,
                   help="pin DAT_008a4f94 to --seed-value before the first "
                        "FUN_0056c070 spawn so retail's phase-7 sparkles match "
                        "the port's pinned-seed build (default ON).")
    p.add_argument("--no-seed-pin", dest="seed_pin", action="store_false",
                   help="leave retail's RNG seeded by srand(time()) (random "
                        "sparkles — disables phase-7 twinkle parity).")
    p.add_argument("--seed-value", type=lambda s: int(s, 0), default=0x4f5347,
                   help="the fixed seed both sides use (default 0x4f5347 = "
                        "OSS_RNG_DEFAULT_SEED; must match the port's seed).")
    p.add_argument("--textout-probe", action="store_true",
                   help="hook gdi32!TextOutA/ExtTextOutA and log retail's glyph "
                        "draws (x/y/bytes/colour/bkmode + selected LOGFONTA) to "
                        "textout.jsonl — the GDI-text ground truth.  Pair with "
                        "--input-trace to drive retail to the new-game menu.")
    p.add_argument("--textout-frames", default=None,
                   help="restrict --textout-probe to a flip window 'LO,HI' so "
                        "the intro/attract debug text is skipped (e.g. "
                        "'2000,4000' for the new-game menu).")
    p.add_argument("--box-probe", action="store_true",
                   help="hook FUN_0048d940 (sprite-cell render) and log the "
                        "box-widget 9-slice composition (bank PE resource id + "
                        "frame ids + on-screen dst rects) to box_cells.jsonl — "
                        "the box-art ground truth.  Pair with --input-trace + "
                        "--box-frames to drive retail to the new-game menu.")
    p.add_argument("--box-frames", default=None,
                   help="restrict --box-probe to a flip window 'LO,HI' (the "
                        "box-art bank renders many UI sprites; gate it to the "
                        "new-game menu, e.g. '410,470').")
    p.add_argument("--res-probe", action="store_true",
                   help="hook bs_decode_resource (FUN_005b7800) and log every "
                        "distinct (module, id, type) PE-resource load to "
                        "res_loads.jsonl — the 'which DLL banks does a scene "
                        "pull' ground truth.  Pair with --input-trace + "
                        "--res-frames to scope it to a scene (e.g. in-game).")
    p.add_argument("--res-frames", default=None,
                   help="restrict --res-probe to a flip window 'LO,HI' (e.g. "
                        "'1100,1800' for the in-game opening town).")
    p.add_argument("--parallax-probe", action="store_true",
                   help="hook FUN_00490cd0 (the in-game parallax/background "
                        "renderer) + its inner cel-select/blit and log the "
                        "descriptor fields (3 layers) + per-tile blits to "
                        "parallax.jsonl — the sky/mountain far-plane ground "
                        "truth.  Pair with --input-trace + --parallax-frames "
                        "to scope it to the in-game town.")
    p.add_argument("--parallax-frames", default=None,
                   help="restrict --parallax-probe to a flip window 'LO,HI' "
                        "(e.g. '1140,1300' for the opening town first frames).")
    args = p.parse_args()

    res_lo, res_hi = 0, 0
    if args.res_frames:
        res_lo, res_hi = (int(x, 0) for x in args.res_frames.split(","))

    parallax_lo, parallax_hi = 0, 0
    if args.parallax_frames:
        parallax_lo, parallax_hi = (int(x, 0) for x in args.parallax_frames.split(","))

    textout_lo, textout_hi = 0, 0
    if args.textout_frames:
        parts = [int(x) for x in args.textout_frames.split(",") if x.strip()]
        if len(parts) == 2:
            textout_lo, textout_hi = parts
        elif len(parts) == 1:
            textout_lo = parts[0]
        else:
            p.error("--textout-frames expects 'LO,HI' (or a single 'LO')")

    box_lo, box_hi = 0, 0
    if args.box_frames:
        parts = [int(x) for x in args.box_frames.split(",") if x.strip()]
        if len(parts) == 2:
            box_lo, box_hi = parts
        elif len(parts) == 1:
            box_lo = parts[0]
        else:
            p.error("--box-frames expects 'LO,HI' (or a single 'LO')")

    call_trace_vas = None
    call_trace_frames = None
    field_spec = None
    if args.call_trace:
        # B2 field spec (default to the committed retail_fields.json). Keyed for
        # the agent by the DECIMAL-string VA so JS `g_field_spec[va]` (va an int)
        # hits. Its VAs are auto-hooked; --field-spec-only hooks ONLY them.
        fs_path = args.field_spec
        if fs_path is None:
            cand = ROOT / "tools" / "flow" / "retail_fields.json"
            fs_path = cand if cand.exists() else None
        spec_vas: list[int] = []
        if fs_path is not None:
            if not fs_path.exists():
                p.error(f"--field-spec: file not found: {fs_path}")
            raw_spec = json.loads(fs_path.read_text()).get("fields", {})
            field_spec = {}
            for va_key, entry in raw_spec.items():
                if not isinstance(entry, dict) or not entry.get("fields"):
                    continue                       # chain_benign / fieldless entry
                va_i = int(va_key, 0) if isinstance(va_key, str) else int(va_key)
                field_spec[str(va_i)] = entry["fields"]
                spec_vas.append(va_i)
            if field_spec:
                print(f"[frida_capture] field-spec: {len(field_spec)} VA(s) from "
                      f"{fs_path.name}", file=sys.stderr)

        if args.field_spec_only:
            if not spec_vas:
                p.error("--field-spec-only: the field spec declares no "
                        "field-bearing VAs")
            call_trace_vas = sorted(set(spec_vas))
            print(f"[frida_capture] call-trace: field-spec-only, "
                  f"{len(call_trace_vas)} VA(s)", file=sys.stderr)
        else:
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
            # Union the spec VAs so field-bearing functions are always hooked.
            call_trace_vas = sorted(set(int(v) for v in call_trace_vas) | set(spec_vas))
            print(f"[frida_capture] call-trace: {len(call_trace_vas)} VAs from "
                  f"{ct_path.name}" + (f" (+{len(spec_vas)} spec)" if spec_vas else ""),
                  file=sys.stderr)
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
        lockstep          = args.lockstep,
        lockstep_step_ms  = args.lockstep_step_ms,
        lockstep_epsilon_ms = args.lockstep_epsilon_ms,
        rand_probe        = args.rand_probe,
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
        field_spec        = field_spec,
        capture           = capture,
        capture_frames    = capture_frames,
        input_trace       = input_trace,
        inject_debug      = args.inject_debug,
        cursor_probe      = args.cursor_probe,
        fade_probe        = args.fade_probe,
        pace_probe        = args.pace_probe,
        seed_pin          = args.seed_pin,
        seed_value        = args.seed_value,
        pace_every        = args.pace_every,
        textout_probe     = args.textout_probe,
        textout_lo        = textout_lo,
        textout_hi        = textout_hi,
        box_probe         = args.box_probe,
        box_lo            = box_lo,
        box_hi            = box_hi,
        res_probe         = args.res_probe,
        res_lo            = res_lo,
        res_hi            = res_hi,
        parallax_probe    = args.parallax_probe,
        parallax_lo       = parallax_lo,
        parallax_hi       = parallax_hi,
    )
    return run_capture(cfg)


if __name__ == "__main__":
    sys.exit(main())
