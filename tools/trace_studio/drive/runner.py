"""drive/runner.py — run the two side drives concurrently.

The port runs locally (WSLInterop exe under the launcher); retail runs on the
Windows host via Frida.  They share nothing but the repo build (made once by
run-opensummoners.sh before the port boots), so true concurrency is safe and
halves the capture wall-clock.
"""
from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from . import port as port_mod, retail as retail_mod


def drive_both(sess_dir: Path, port_trace: Path, retail_trace: Path,
               port_frames: int, retail_frames: int,
               run_port: bool = True, run_retail: bool = True,
               remote: str | None = None, call_trace: bool = False) -> dict:
    res: dict = {}
    with ThreadPoolExecutor(max_workers=2) as ex:
        futs = {}
        if run_port:
            futs["port"] = ex.submit(port_mod.drive_port, sess_dir,
                                     port_trace, port_frames, call_trace)
        if run_retail:
            futs["retail"] = ex.submit(retail_mod.drive_retail, sess_dir,
                                       retail_trace, retail_frames,
                                       remote, call_trace)
        for side, fut in futs.items():
            try:
                res[side] = fut.result()
            except Exception as e:                       # noqa: BLE001
                res[side] = {"rc": -1, "n_frames": 0, "anchors": [],
                             "error": repr(e)}
    return res
