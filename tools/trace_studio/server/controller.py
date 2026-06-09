"""server/controller.py — the capture-subprocess controller (browser self-service).

Runs `trace_studio.py capture/recapture` as a tracked background subprocess so
the SPA can drive the mark→fix→re-capture loop without the CLI.  One capture at
a time; paranoid about strays (own process group, /proc orphan reap, atexit
kill).  Adapted from openrecet's CaptureController (no recorder here — the
port-side live recorder is a v2 item, docs/plans/trace-studio.md).
"""
from __future__ import annotations

import os
import re
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path

from ..paths import ROOT


def _argv_is_capture(parts: list[str]) -> bool:
    """True iff argv is a `trace_studio.py capture|recapture …` invocation.
    Must NOT match the `serve` process, else cancel would kill the studio."""
    idx = next((k for k, p in enumerate(parts)
                if p.endswith("trace_studio.py")), None)
    return (idx is not None and idx + 1 < len(parts)
            and parts[idx + 1] in ("capture", "recapture"))


def _orphan_capture_pgids() -> list[int]:
    pgids: set[int] = set()
    proc_root = Path("/proc")
    if not proc_root.is_dir():
        return []
    for pid_dir in proc_root.iterdir():
        if not pid_dir.name.isdigit():
            continue
        try:
            parts = [p.decode("utf-8", "replace") for p in
                     (pid_dir / "cmdline").read_bytes().split(b"\0") if p]
        except OSError:
            continue
        if not _argv_is_capture(parts):
            continue
        try:
            pgids.add(os.getpgid(int(pid_dir.name)))
        except (ProcessLookupError, PermissionError, ValueError, OSError):
            pass
    return sorted(pgids)


class CaptureController:
    def __init__(self, remote: str, sess_root: Path):
        self.remote = remote
        self.sess_root = sess_root
        self.lock = threading.Lock()
        self.proc: subprocess.Popen | None = None
        self.session: str | None = None
        self.log: Path | None = None
        self.started: float = 0.0
        self.last_rc: int | None = None
        self.kind: str = "capture"

    def _alive(self) -> bool:
        return self.proc is not None and self.proc.poll() is None

    def start(self, kind: str, args: list[str], session: str) -> dict:
        """kind: capture|recapture; args: the subcommand argv tail."""
        with self.lock:
            if self._alive():
                return {"ok": False,
                        "error": f"a capture is already running ({self.session})"}
            self.sess_root.mkdir(parents=True, exist_ok=True)
            safe = re.sub(r"[^\w.-]", "_", session)
            log = self.sess_root / f".capture-{safe}.log"
            cmd = [sys.executable, str(ROOT / "tools" / "trace_studio.py"),
                   kind] + args + ["--remote", self.remote]
            logf = log.open("w")
            try:
                self.proc = subprocess.Popen(
                    cmd, cwd=str(ROOT), stdout=logf, stderr=subprocess.STDOUT,
                    start_new_session=True)
            except Exception as e:                       # noqa: BLE001
                logf.close()
                return {"ok": False, "error": f"spawn failed: {e!r}"}
            self.session, self.log, self.started = session, log, time.time()
            self.last_rc = None
            self.kind = kind
            return {"ok": True, "session": session}

    def status(self) -> dict:
        with self.lock:
            running = self._alive()
            if not running and self.proc is not None:
                self.last_rc = self.proc.poll()
            tail = ""
            if self.log and self.log.exists():
                lines = [ln for ln in
                         self.log.read_text(errors="replace").splitlines()
                         if ln.strip()]
                tail = lines[-1] if lines else ""
            return {"running": running, "session": self.session,
                    "elapsed_s": round(time.time() - self.started, 1)
                    if self.started else 0,
                    "last_rc": self.last_rc, "log_tail": tail}

    def cancel(self) -> dict:
        with self.lock:
            pgid = None
            sess = self.session
            if self.proc is not None and self.proc.poll() is None:
                try:
                    pgid = os.getpgid(self.proc.pid)
                except (ProcessLookupError, OSError):
                    pgid = None
        targets = set(_orphan_capture_pgids())
        if pgid is not None:
            targets.add(pgid)
        if not targets:
            return {"ok": False, "error": "no capture running"}

        def _killpg(pg, sig):
            try:
                os.killpg(pg, sig)
            except (ProcessLookupError, OSError):
                pass

        for pg in targets:
            _killpg(pg, signal.SIGTERM)
        time.sleep(1.0)
        for pg in targets:
            _killpg(pg, signal.SIGKILL)
        with self.lock:
            if self.proc is not None:
                self.last_rc = self.proc.poll()
        return {"ok": True, "session": sess, "killed_pgids": sorted(targets)}

    def force_cleanup(self) -> None:
        if self._alive():
            try:
                os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)
            except Exception:                            # noqa: BLE001
                pass
