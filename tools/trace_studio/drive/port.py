"""drive/port.py — drive the PORT through a working input trace, capturing every
present frame.

Reuses tools/run-opensummoners.sh (the canonical recipe: build, fresh PE image
copy to dodge WSL2's image-by-path mapping reuse, launcher supervision with a
Job-Object kill).  The port dumps BMPs named by Flip into a Windows-writable
staging dir (the exe cannot fopen a WSL path — ckpt-95 footgun), which we then
convert to PNG into <sess>/port/raw/frame_<flip>.png.  Anchors are parsed from
the port's stderr (`anchor: <name> flip=<N> rng=0x…`) into anchors.jsonl.
"""
from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

from ..paths import ROOT, WIN_STAGE, win_path

_ANCHOR_RE = re.compile(
    r"anchor:\s+(\S+)\s+flip=(\d+)(?:\s+rng=0x([0-9a-fA-F]+))?")


def _convert_one(args: tuple[str, str]) -> bool:
    src, dst = args
    try:
        from PIL import Image
        Image.open(src).convert("RGB").save(dst)
        os.unlink(src)
        return True
    except Exception:                                    # noqa: BLE001
        return False


def parse_anchors(log_text: str) -> list[dict]:
    out = []
    for m in _ANCHOR_RE.finditer(log_text):
        row = {"name": m.group(1), "flip": int(m.group(2))}
        if m.group(3):
            row["rng"] = int(m.group(3), 16)
        out.append(row)
    return out


def drive_port(sess_dir: Path, trace: Path, frames: int,
               call_trace: bool = False) -> dict:
    """Run the port through `trace` for `frames` Flips with a dense frame dump.
    Returns {rc, n_frames, anchors, log}."""
    sess_dir = Path(sess_dir)
    port_dir = sess_dir / "port"
    raw_dir = port_dir / "raw"
    if raw_dir.exists():
        shutil.rmtree(raw_dir)
    raw_dir.mkdir(parents=True, exist_ok=True)

    stage = WIN_STAGE / sess_dir.name / "port"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True, exist_ok=True)

    log_path = sess_dir / "port.log"
    # --input-trace must stay REPO-RELATIVE: the exe absolutizes it against its
    # pre-chdir cwd (the \\wsl.localhost UNC mapping of ROOT), which Windows
    # fopen accepts for READS.  The BMP WRITE dir gets the real C:/ path.
    rel_trace = os.path.relpath(Path(trace).resolve(), ROOT)
    child = ["--input-trace", rel_trace,
             "--frames", str(int(frames)),
             "--capture-all",
             "--capture-dir", win_path(stage)]
    if call_trace:
        child += ["--call-trace", win_path(stage / "call_trace.jsonl")]
    timeout_ms = int(frames / 60 * 1000) + 90_000
    env = dict(os.environ)
    env["OPENSUMMONERS_TIMEOUT_MS"] = str(timeout_ms)
    cmd = ["bash", str(ROOT / "tools" / "run-opensummoners.sh"), "--"] + child

    with log_path.open("w") as lf:
        proc = subprocess.run(cmd, cwd=str(ROOT), stdout=lf,
                              stderr=subprocess.STDOUT, env=env,
                              timeout=timeout_ms / 1000 + 120)
    log_text = log_path.read_text(errors="replace")
    anchors = parse_anchors(log_text)
    (sess_dir / "anchors.port.jsonl").write_text(
        "".join(json.dumps(a) + "\n" for a in anchors))

    # BMP → PNG into port/raw (parallel; the BMPs are deleted as they convert).
    # New-style names carry the sim-tick axis (port_frame_<flip>_t<tick>.bmp,
    # mirroring retail) — preserved in the PNG name for tick-axis pairing.
    bmps = sorted(stage.glob("port_frame_*.bmp"))
    jobs = []
    for b in bmps:
        m = re.search(r"port_frame_(\d+)_t(\d+)\.bmp$", b.name)
        if m:
            dst = f"frame_{int(m.group(1)):05d}_t{int(m.group(2)):06d}.png"
        else:
            m = re.search(r"port_frame_(\d+)\.bmp$", b.name)
            if not m:
                continue
            dst = f"frame_{int(m.group(1)):05d}.png"
        jobs.append((str(b), str(raw_dir / dst)))
    n_ok = 0
    if jobs:
        with ProcessPoolExecutor(max_workers=8) as ex:
            for ok in ex.map(_convert_one, jobs, chunksize=16):
                n_ok += bool(ok)
    if call_trace and (stage / "call_trace.jsonl").exists():
        shutil.move(str(stage / "call_trace.jsonl"),
                    str(port_dir / "call_trace.jsonl"))
    shutil.rmtree(stage, ignore_errors=True)

    return {"rc": proc.returncode, "n_frames": n_ok, "anchors": anchors,
            "log": str(log_path)}
