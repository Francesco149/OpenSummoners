#!/usr/bin/env python3
"""tools/tas_diff.py — anchor-aligned side-by-side port↔retail trace diff.

The TAS determinism stack (see docs/findings/tas-harness.md):

  * The PORT runs fixed-timestep, 1 update / present, RNG seed pinned
    (OSS_RNG_DEFAULT_SEED).  Deterministic frame stream.
  * RETAIL runs under the Frida harness with --lockstep (one update quantum
    banked per Flip → 1 update / present, like the port) + --seed-pin (the
    same seed written into DAT_008a4f94).  Deterministic frame stream.
  * Both sides emit named TAS ANCHORS at the same scene/phase boundaries
    (subtitle_anim_start, newgame_enter, prologue_enter, …).  Within a scene
    the two march tick-for-tick; the anchor absorbs the per-binary flip-count
    skew (boot + per-scene load cost) by aligning the two flip axes.

This tool takes a port capture set + a retail run, aligns them on a chosen
anchor, and reports per-tick differ_px.  Because the port can occasionally
present a 0-update DUPLICATE frame (a cadence wrinkle — content is unchanged,
but it slips the rigid offset by ±1), each port frame is matched to the
BEST retail frame within a small search window; the chosen drift is reported
so a real divergence (best differ_px > 0 across the whole window) is never
hidden by it.

Port frames: <port-dir>/port_frame_<flip>.bmp  (main.c --capture-frames).
Retail frames: <retail-run>/frames/frame_<flip>.png  (frida_capture).
Anchors: port from its stderr log line `anchor: <name> flip=<N> rng=0x..`
(pass --port-anchor N or --port-log FILE); retail from <retail-run>/run.json
summary.anchors[name].

Usage:
  tools/tas_diff.py --anchor subtitle_anim_start \
      --port-dir /mnt/c/osscap --port-anchor 438 \
      --retail-run runs/tas-retail-dense
"""
from __future__ import annotations
import argparse, json, re, sys
from pathlib import Path

import numpy as np
from PIL import Image


def _load_rgb(path: Path):
    try:
        return np.asarray(Image.open(path).convert("RGB"), dtype=np.int16)
    except Exception:
        return None


def _differ_px(a, b) -> int:
    """Count of pixels differing in any channel (the project's differ_px)."""
    return int((np.abs(a - b).sum(axis=2) > 0).sum())


def _read_retail_anchor(run_dir: Path, name: str):
    meta = json.loads((run_dir / "run.json").read_text())
    anchors = meta.get("summary", {}).get("anchors", {})
    if name not in anchors:
        raise SystemExit(f"retail run {run_dir} has no anchor '{name}' "
                         f"(have: {sorted(anchors)})")
    return int(anchors[name])


def _read_port_anchor(log: Path, name: str):
    pat = re.compile(rf"anchor:\s+{re.escape(name)}\s+flip=(\d+)")
    for line in log.read_text(errors="replace").splitlines():
        m = pat.search(line)
        if m:
            return int(m.group(1))
    raise SystemExit(f"no 'anchor: {name}' line in {log}")


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--anchor", required=True, help="anchor name to align on")
    p.add_argument("--port-dir", required=True, type=Path,
                   help="dir of port_frame_<flip>.bmp captures")
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("--port-anchor", type=int, help="port anchor flip (from its log)")
    g.add_argument("--port-log", type=Path, help="port stderr log to parse the anchor from")
    p.add_argument("--retail-run", required=True, type=Path,
                   help="frida_capture run dir (run.json + frames/)")
    p.add_argument("--retail-anchor", type=int,
                   help="override retail anchor flip (else read from run.json)")
    p.add_argument("--window", type=int, default=2,
                   help="search ±W retail frames for the best match (default 2)")
    p.add_argument("--fail-px", type=int, default=0,
                   help="non-zero best differ_px above this fails the gate (default 0)")
    args = p.parse_args()

    port_anchor = (args.port_anchor if args.port_anchor is not None
                   else _read_port_anchor(args.port_log, args.anchor))
    retail_anchor = (args.retail_anchor if args.retail_anchor is not None
                     else _read_retail_anchor(args.retail_run, args.anchor))
    rframes = args.retail_run / "frames"

    # Index the captured frames present on each side.
    port_re = re.compile(r"port_frame_(\d+)\.bmp$")
    pflips = sorted(int(port_re.search(f.name).group(1))
                    for f in args.port_dir.glob("port_frame_*.bmp"))
    retail_re = re.compile(r"frame_(\d+)\.png$")
    rflips = set(int(retail_re.search(f.name).group(1))
                 for f in rframes.glob("frame_*.png"))
    if not pflips:
        raise SystemExit(f"no port_frame_*.bmp in {args.port_dir}")
    if not rflips:
        raise SystemExit(f"no frame_*.png in {rframes}")

    print(f"anchor '{args.anchor}': port@{port_anchor} retail@{retail_anchor} "
          f"(offset {port_anchor - retail_anchor}); window ±{args.window}")
    print(f"{'tick':>5} {'port':>5} {'retail':>6} {'drift':>5} {'differ_px':>10}  note")

    rcache: dict[int, object] = {}
    def rload(flip):
        if flip not in rcache:
            rcache[flip] = _load_rgb(rframes / f"frame_{flip:05d}.png") if flip in rflips else None
        return rcache[flip]

    worst = 0
    worst_tick = None
    n_cmp = 0
    n_exact = 0
    drifts: dict[int, int] = {}
    for pf in pflips:
        tick = pf - port_anchor
        a = _load_rgb(args.port_dir / f"port_frame_{pf:05d}.bmp")
        if a is None:
            continue
        base = retail_anchor + tick
        best = (1 << 30, None)
        for d in range(-args.window, args.window + 1):
            b = rload(base + d)
            if b is None or b.shape != a.shape:
                continue
            dpx = _differ_px(a, b)
            if dpx < best[0]:
                best = (dpx, d)
        if best[1] is None:
            print(f"{tick:>5} {pf:>5} {'--':>6} {'--':>5} {'(no retail frame in window)':>10}")
            continue
        dpx, drift = best
        n_cmp += 1
        if dpx == 0:
            n_exact += 1
        drifts[drift] = drifts.get(drift, 0) + 1
        if dpx > worst:
            worst, worst_tick = dpx, tick
        note = "" if drift == 0 else f"aligned at drift {drift:+d}"
        print(f"{tick:>5} {pf:>5} {base + drift:>6} {drift:>+5} {dpx:>10}  {note}")

    print("-" * 60)
    print(f"compared {n_cmp} frames: {n_exact} bit-exact (differ_px=0), "
          f"{n_cmp - n_exact} non-zero")
    print(f"drift histogram (port dup/skip wobble): "
          f"{dict(sorted(drifts.items()))}")
    if worst_tick is not None:
        print(f"worst: differ_px={worst} at tick {worst_tick}")
    gate_ok = worst <= args.fail_px
    print(f"GATE: {'PASS' if gate_ok else 'FAIL'} "
          f"(worst best-match differ_px={worst} <= {args.fail_px})")
    return 0 if gate_ok else 1


if __name__ == "__main__":
    sys.exit(main())
