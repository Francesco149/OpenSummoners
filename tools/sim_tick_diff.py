#!/usr/bin/env python3
"""
sim_tick_diff.py — align two frida_capture run-dirs on the DETERMINISTIC
frame-of-reference and diff matching frames.

The in-game sim is a wall-clock GetTickCount frame-limiter (engine-quirk #75):
it presents a VARIABLE number of Flips per logical sim tick, so the Flip index
is non-deterministic run-to-run (two identical retail runs disagree on ~50% of
flips by up to 3 px).  The deterministic index is the **sim tick** — the easer
call count, tagged onto every captured frame by the agent (frame filename
`frame_<flip>_t<simtick>.png` + frames_manifest.jsonl) and onto every call-trace
event (the `sim_tick` field, alongside `cam_x60`).

This tool matches frames between two run-dirs by a chosen key:
  --key sim_tick   (default) the deterministic logical-frame index
  --key cam        match by cam_x60 (the camera scroll; bijective with sim tick
                   during the monotone pan — useful cross-side when the two runs
                   started counting sim ticks at different absolute offsets)
  --key flip       the raw Flip index (the WRONG anchor — for contrast/demos)

For each matched pair it reports the mean abs pixel diff and the best integer
(dx,dy) shift, and (with --montage OUT.png) writes a port|retail|amplified-diff
montage of the first few matches.

Usage:
  tools/sim_tick_diff.py --a /tmp/run1 --b /tmp/run2 --key sim_tick \
      --montage /tmp/cmp.png [--n 3] [--region X0,Y0,X1,Y1]
"""
import argparse, json, sys
from pathlib import Path
import numpy as np
from PIL import Image


def load_run(run_dir: Path):
    """Return {key_name: {key_value: frame_path}} for sim_tick / flip / cam."""
    man = run_dir / "frames_manifest.jsonl"
    frames = {}            # flip -> (sim_tick, path)
    if man.exists():
        for line in man.read_text().splitlines():
            if not line.strip():
                continue
            r = json.loads(line)
            frames[int(r["flip"])] = (int(r["sim_tick"]),
                                      run_dir / "frames" / r["file"])
    else:
        # Fall back to filename parse (frame_<flip>_t<simtick>.png).
        for p in sorted((run_dir / "frames").glob("frame_*.png")):
            stem = p.stem
            flip = int(stem.split("_")[1])
            st = int(stem.split("_t")[1]) if "_t" in stem else -1
            frames[flip] = (st, p)
    # cam_x60 per flip from the call trace (field-spec).
    cam = {}
    ct = run_dir / "call_trace.jsonl"
    if ct.exists():
        for line in ct.read_text().splitlines():
            if not line.strip():
                continue
            r = json.loads(line)
            c = r.get("f", {}).get("cam_x60")
            if c is not None:
                cam[int(r["frame"])] = int(c)
    return frames, cam


def index_by(frames, cam, key):
    """key -> frame_path.  For sim_tick/cam (many flips share a value) keep the
    LAST flip (most-settled render of that logical frame)."""
    out = {}
    for flip in sorted(frames):
        st, path = frames[flip]
        if key == "flip":
            out[flip] = path
        elif key == "sim_tick":
            if st >= 0:
                out[st] = path           # last flip of the tick wins
        elif key == "cam":
            c = cam.get(flip)
            if c is not None:
                out[c] = path
    return out


def luma(im):
    return np.asarray(im.convert("RGB")).astype(np.float64).mean(2)


def best_shift(A, B, rng=5, region=None):
    if region:
        x0, y0, x1, y1 = region
    else:
        y0, x0 = 0, 0
        y1, x1 = A.shape
    def edges(x):
        return (np.abs(np.diff(x, 1, axis=1, prepend=x[:, :1])) +
                np.abs(np.diff(x, 1, axis=0, prepend=x[:1, :])))
    ea, eb = edges(A), edges(B)
    best = None
    for dy in range(-rng, rng + 1):
        for dx in range(-rng, rng + 1):
            h, w = ea.shape
            yy0, yy1 = max(y0, -dy), min(y1, h - dy)
            xx0, xx1 = max(x0, -dx), min(x1, w - dx)
            s = np.abs(ea[yy0:yy1, xx0:xx1] -
                       eb[yy0 + dy:yy1 + dy, xx0 + dx:xx1 + dx]).mean()
            if best is None or s < best[0]:
                best = (s, dx, dy)
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True, type=Path)
    ap.add_argument("--b", required=True, type=Path)
    ap.add_argument("--key", choices=["sim_tick", "cam", "flip"],
                    default="sim_tick")
    ap.add_argument("--n", type=int, default=3, help="montage rows")
    ap.add_argument("--montage", type=Path, default=None)
    ap.add_argument("--region", default=None,
                    help="X0,Y0,X1,Y1 for the shift/diff measurement")
    args = ap.parse_args()
    region = tuple(int(v) for v in args.region.split(",")) if args.region else None

    fa, ca = load_run(args.a)
    fb, cb = load_run(args.b)
    ia = index_by(fa, ca, args.key)
    ib = index_by(fb, cb, args.key)
    common = sorted(set(ia) & set(ib))
    if not common:
        print(f"no common {args.key} between the two runs", file=sys.stderr)
        return 2
    print(f"matched {len(common)} frames by {args.key} "
          f"(a={len(ia)} b={len(ib)})")

    rows = []
    diffs = []
    for k in common:
        A = Image.open(ia[k]); B = Image.open(ib[k])
        la, lb = luma(A), luma(B)
        if la.shape != lb.shape:
            continue
        sad = float(np.abs(la - lb).mean())
        sh = best_shift(la, lb, region=region)
        diffs.append((k, sad, sh[1], sh[2]))
        rows.append((k, A, B))
    diffs.sort()
    print(f"{'key':>10} {'meanAbs':>8} {'dx':>3} {'dy':>3}")
    for k, sad, dx, dy in diffs:
        print(f"{k:>10} {sad:8.3f} {dx:>3} {dy:>3}")
    mean_sad = np.mean([d[1] for d in diffs])
    mean_dx = np.mean([abs(d[2]) for d in diffs])
    print(f"--> mean meanAbs={mean_sad:.3f}  mean|dx|={mean_dx:.3f}  "
          f"({'ALIGNED' if mean_dx < 0.5 else 'MISALIGNED'})")

    if args.montage and rows:
        sel = rows[: args.n]
        tiles = []
        for k, A, B in sel:
            a = np.asarray(A.convert("RGB")).astype(np.int16)
            b = np.asarray(B.convert("RGB")).astype(np.int16)
            d = np.clip(np.abs(a - b) * 4, 0, 255).astype(np.uint8)
            row = np.concatenate([a.astype(np.uint8), b.astype(np.uint8), d], axis=1)
            tiles.append(row)
        mont = np.concatenate(tiles, axis=0)
        Image.fromarray(mont).save(args.montage)
        print(f"montage -> {args.montage}  (cols: A | B | 4x|diff|)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
