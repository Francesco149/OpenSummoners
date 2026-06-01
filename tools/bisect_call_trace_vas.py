#!/usr/bin/env python3
"""
tools/bisect_call_trace_vas.py — find the maximum subset of engine VAs that
can be Frida-hooked without crashing the retail engine.

Some addresses in tools/frida/data/engine_vas.json overlap with MSVC CRT
internals Frida cannot safely trampoline (allocator re-entry, exception
unwind, TLS slots, ...).  Hooking them all at once can kill the engine on
boot.  This script:

  1. Starts from the candidate set (tools/frida/data/engine_vas.json,
     produced by tools/gen_engine_vas.py).
  2. Tries to boot retail with that set hooked.
  3. On crash, binary-searches to identify one offending VA.
  4. Adds it to the exclusion list, retries.
  5. Loops until a boot succeeds.

Progress + the result are written incrementally to
    tools/frida/data/engine_vas_frida_safe.json
so a kill mid-run loses at most one bisection's worth of work.

Each test drives tools/run-retail.sh (which handles the Steamless exe-drop
into the game dir) with the candidate VA list hooked, then reads the run's
run.json.  Boot success = the engine pumped at least --boot-threshold
messages (it got past CRT init + the launcher into its message loop); a
crash-on-boot dies before pumping.

⚠ LIVE-CALIBRATED 2026-06-02 — and the result is that this whole tool is
likely **unnecessary**: a direct no-turbo call-trace capture hooked the
*entire* candidate set (1743 VAs) at once and booted retail cleanly,
emitting 1.8M events over 1914 frames with zero crashes
(runs/calltrace-title).  So `engine_vas_frida_safe.json` was written
directly from the full set, no bisection needed.  Re-run this only if a
future candidate set introduces a crashing VA.

⚠ The boot-success signal MUST be measured **--no-turbo**: turbo freezes
the splash before the engine reaches its message pump (engine-quirks #29),
so a turbo boot reports msg_count=0 and every subset reads as a crash.
Calibrated baselines (no-turbo): a good boot pumps ~750-1000 messages in
14-16 s; a real crash gives ~0.  BOOT_THRESHOLD=30 cleanly separates them.

Usage (inside `nix develop`):
    python3 tools/bisect_call_trace_vas.py
    python3 tools/bisect_call_trace_vas.py --resume
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path


REPO          = Path(__file__).resolve().parent.parent
RUN_RETAIL    = REPO / "tools" / "run-retail.sh"
CAND_PATH     = REPO / "tools" / "frida" / "data" / "engine_vas.json"
OUT_PATH      = REPO / "tools" / "frida" / "data" / "engine_vas_frida_safe.json"
RUN_BASE      = Path("/tmp/opensummoners-bisect")
PROGRESS_PATH = RUN_BASE / "progress.json"

DURATION_MS     = 14000   # no-turbo boot-to-pump budget (live-calibrated 2026-06-02)
SUBPROC_TIMEOUT = 60
MAX_FRAMES      = 3
BOOT_THRESHOLD  = 30      # min drained messages = "reached the pump"


def load_candidates() -> list[int]:
    if not CAND_PATH.exists():
        raise SystemExit(f"{CAND_PATH} not found — run tools/gen_engine_vas.py first")
    raw = json.loads(CAND_PATH.read_text())
    vas = raw["vas"] if isinstance(raw, dict) and "vas" in raw else list(raw)
    return sorted(int(v) for v in vas)


_RUN_SEQ = [0]


def _read_msg_count(run_dir: Path) -> int:
    """Pull summary.msg_count out of the run's run.json (-1 if missing)."""
    rj = run_dir / "run.json"
    if not rj.exists():
        return -1
    try:
        meta = json.loads(rj.read_text())
        return int(meta.get("summary", {}).get("msg_count", -1))
    except Exception:
        return -1


def test_subset(vas: list[int], boot_threshold: int) -> tuple[bool, str]:
    """True iff retail pumped >= boot_threshold messages with `vas` hooked."""
    _RUN_SEQ[0] += 1
    run_dir = RUN_BASE / f"r{_RUN_SEQ[0]:04d}"
    run_dir.mkdir(parents=True, exist_ok=True)
    vas_file = run_dir / "vas.json"
    vas_file.write_text(json.dumps({"count": len(vas), "vas": vas}))

    env = dict(os.environ)
    env["OPENSUMMONERS_DURATION_MS"] = str(DURATION_MS)
    env["OPENSUMMONERS_MAX_FRAMES"]  = str(MAX_FRAMES)

    cmd = [
        "bash", str(RUN_RETAIL),
        "--no-turbo",   # turbo freezes the splash → msg_count=0 (quirk #29)
        "--exact-run-dir", "--run-dir", str(run_dir / "run"),
        "--call-trace",
        "--call-trace-vas-file", str(vas_file),
        # a frame index the run never reaches — keeps every onEnter cheap
        # (callTraceShouldEmit() short-circuits) so we test hook *install*
        # stability, not emission cost.
        "--call-trace-frames", "99999",
    ]
    try:
        subprocess.run(cmd, capture_output=True, text=True,
                       timeout=SUBPROC_TIMEOUT, env=env)
    except subprocess.TimeoutExpired:
        return False, "subprocess_timeout"

    msgs = _read_msg_count(run_dir / "run")
    return msgs >= boot_threshold, f"msg_count={msgs}"


def find_one_bad(vas: list[int], boot_threshold: int) -> int:
    """Bisect `vas` (known to fail) to one offending VA."""
    lo, hi = 0, len(vas)
    while hi - lo > 1:
        mid = (lo + hi) // 2
        ok, info = test_subset(vas[lo:mid], boot_threshold)
        print(f"      bisect[{lo}:{mid}] n={mid-lo}: "
              f"{'PASS' if ok else 'FAIL'} ({info})", flush=True)
        if ok:
            lo = mid       # lower half alone boots → bad VA is in the upper
        else:
            hi = mid
    return vas[lo]


def save_progress(excluded: list[int], n_active: int,
                  status: str, elapsed_s: float) -> None:
    PROGRESS_PATH.write_text(json.dumps({
        "status":     status,
        "elapsed_s":  round(elapsed_s, 1),
        "excluded":   excluded,
        "n_excluded": len(excluded),
        "n_active":   n_active,
    }, indent=2))


def save_result(seed: list[int], excluded: list[int], elapsed_min: float) -> None:
    safe = [v for v in seed if v not in set(excluded)]
    OUT_PATH.write_text(json.dumps({
        "description": ("engine VAs vetted Frida-safe via "
                        "tools/bisect_call_trace_vas.py"),
        "count":       len(safe),
        "n_excluded":  len(excluded),
        "elapsed_min": round(elapsed_min, 1),
        "vas":         safe,
        "excluded":    sorted(excluded),
    }, indent=2))


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--resume", action="store_true",
                    help="resume from progress.json (carry forward known-bad VAs)")
    ap.add_argument("--max-bad", type=int, default=30,
                    help="give up after identifying this many bad VAs (default 30)")
    ap.add_argument("--boot-threshold", type=int, default=BOOT_THRESHOLD,
                    help=f"min drained messages to call a boot successful "
                         f"(default {BOOT_THRESHOLD}; calibrate on first live run)")
    args = ap.parse_args(argv)

    RUN_BASE.mkdir(parents=True, exist_ok=True)
    seed = load_candidates()
    print(f"candidate set: {len(seed)} VAs", flush=True)

    excluded: list[int] = []
    if args.resume and PROGRESS_PATH.exists():
        excluded = list(json.loads(PROGRESS_PATH.read_text()).get("excluded", []))
        print(f"resume: {len(excluded)} previously-known-bad VAs", flush=True)

    t0 = time.monotonic()
    for round_i in range(args.max_bad + 1):
        active = [v for v in seed if v not in set(excluded)]
        elapsed = time.monotonic() - t0
        save_progress(excluded, len(active), "testing_full", elapsed)
        print(f"\n=== round {round_i}: testing {len(active)} VAs "
              f"(elapsed {elapsed/60:.1f} min) ===", flush=True)

        ok, info = test_subset(active, args.boot_threshold)
        print(f"  full set: {'PASS' if ok else 'FAIL'} ({info})", flush=True)
        if ok:
            elapsed_min = (time.monotonic() - t0) / 60
            save_progress(excluded, len(active), "complete", elapsed)
            save_result(seed, excluded, elapsed_min)
            print(f"\nDONE in {elapsed_min:.1f} min. "
                  f"safe={len(active)} excluded={len(excluded)}", flush=True)
            print(f"wrote {OUT_PATH}", flush=True)
            return 0

        save_progress(excluded, len(active), "bisecting", elapsed)
        bad = find_one_bad(active, args.boot_threshold)
        excluded.append(bad)
        save_progress(excluded, len(active) - 1, "found_bad", elapsed)
        save_result(seed, excluded, (time.monotonic() - t0) / 60)
        print(f"  bad VA isolated: 0x{bad:x} (excluded total: {len(excluded)})",
              flush=True)

    print(f"\nGAVE UP after {args.max_bad} bad VAs identified", flush=True)
    save_result(seed, excluded, (time.monotonic() - t0) / 60)
    return 2


if __name__ == "__main__":
    sys.exit(main())
