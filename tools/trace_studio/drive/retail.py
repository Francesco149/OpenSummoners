"""drive/retail.py — drive RETAIL through a working input trace via the Frida
harness, capturing every Flip frame.

Thin wrapper over tools/frida_capture.py (subprocess — identical to manual use,
logs land in retail.log).  Always --no-turbo (live boots need the message pump,
engine-quirk #29) + --lockstep + --seed-pin: the deterministic 1-update/present
stream the pairing model assumes.  Frames land as
<sess>/retail/cap/frames/frame_<flip>_t<simtick>.png; the anchor stream as
<sess>/retail/cap/anchors.jsonl (the frida_capture patch of this checkpoint).
"""
from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

from ..paths import ROOT

def _kill_leftover_retail(remote: str | None) -> None:
    """Kill a sotes(.unpacked).exe the harness left running on the Windows
    host.  Must go through frida — the game is a child of the ELEVATED
    frida-server, so an unelevated taskkill gets Access-denied."""
    try:
        import frida
        from ..paths import DEFAULT_REMOTE
        dev = frida.get_device_manager().add_remote_device(
            remote or DEFAULT_REMOTE)
        for p in dev.enumerate_processes():
            if "sotes" in p.name.lower():
                try:
                    dev.kill(p.pid)
                    print(f"trace_studio: killed leftover {p.name} "
                          f"(pid {p.pid})", flush=True)
                except Exception:                        # noqa: BLE001
                    pass
    except Exception:                                    # noqa: BLE001
        pass


def drive_retail(sess_dir: Path, trace: Path, frames: int,
                 remote: str | None = None, call_trace: bool = False) -> dict:
    """Run retail through `trace`, capturing through Flip `frames`.
    Returns {rc, n_frames, anchors, log}."""
    sess_dir = Path(sess_dir)
    cap_dir = sess_dir / "retail" / "cap"
    if cap_dir.exists():
        shutil.rmtree(cap_dir)
    cap_dir.mkdir(parents=True, exist_ok=True)
    log_path = sess_dir / "retail.log"

    # Pre-flight: a leftover game from a prior crashed/hung capture makes the
    # fresh boot bail with "Game is already running." — clear it first.
    _kill_leftover_retail(remote)

    # duration is the hung-boot backstop; --max-flips is the real stop.
    duration_ms = int(frames) * 35 + 120_000
    cmd = [sys.executable, str(ROOT / "tools" / "frida_capture.py"),
           "--no-turbo", "--lockstep",
           "--input-trace", str(Path(trace).resolve()),
           "--capture-frames", "all",
           "--run-dir", str(cap_dir), "--exact-run-dir",
           "--max-flips", str(int(frames)),
           "--max-frames", str(int(frames) * 4),
           "--duration-ms", str(duration_ms)]
    if remote:
        cmd += ["--remote", remote]
    if call_trace:
        cmd += ["--call-trace", "--field-spec-only"]

    rc = 0
    try:
        with log_path.open("w") as lf:
            proc = subprocess.run(cmd, cwd=str(ROOT), stdout=lf,
                                  stderr=subprocess.STDOUT,
                                  timeout=duration_ms / 1000 + 300)
        rc = proc.returncode
    except subprocess.TimeoutExpired:
        rc = -1
    kill_failed = False
    try:
        kill_failed = "device.kill failed" in log_path.read_text(
            errors="replace")[-4000:]
    except OSError:
        pass
    if rc != 0 or kill_failed:
        # frida_capture died/hung or its frida-side kill failed — the game
        # keeps running HOST-side.  Kill it there (scoped to the failure case
        # only, so a user-driven sotes.exe is never touched by a clean run).
        _kill_leftover_retail(remote)

    anchors: list[dict] = []
    aj = cap_dir / "anchors.jsonl"
    if aj.is_file():
        for ln in aj.read_text().splitlines():
            s = ln.strip()
            if s:
                try:
                    anchors.append(json.loads(s))
                except json.JSONDecodeError:
                    pass
    else:
        # pre-patch fallback: recover last-firing-per-name from run.json
        rj = cap_dir / "run.json"
        if rj.is_file():
            try:
                summ = json.loads(rj.read_text()).get("summary", {})
                rngs = summ.get("anchor_rng", {})
                anchors = [{"name": n, "flip": f,
                            **({"rng": rngs[n]} if n in rngs else {})}
                           for n, f in (summ.get("anchors") or {}).items()]
                anchors.sort(key=lambda a: a["flip"])
            except json.JSONDecodeError:
                pass
    (sess_dir / "anchors.retail.jsonl").write_text(
        "".join(json.dumps(a) + "\n" for a in anchors))

    n = len(list((cap_dir / "frames").glob("frame_*.png"))) \
        if (cap_dir / "frames").is_dir() else 0
    return {"rc": rc, "n_frames": n, "anchors": anchors,
            "log": str(log_path)}
