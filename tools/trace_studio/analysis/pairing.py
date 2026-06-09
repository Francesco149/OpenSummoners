"""analysis/pairing.py — THE alignment core: anchor-segmented tick-for-tick
pairing of the two captured flip axes, with a sticky ±drift best-match.

Model (tas_diff.py's, made dense):
  * Within a scene both sides march tick-for-tick (port fixed-timestep; retail
    --lockstep banks 1 update/present), but the flip index at which a scene
    boundary lands differs per binary AND per run (boot/load skew) — so the two
    axes are SEGMENTED on the shared TAS anchors and paired anchor-relative.
  * The port occasionally presents a 0-update DUPLICATE frame (cadence
    wrinkle), slipping the rigid offset by ±1 — so the retail partner is chosen
    by a sticky best-match drift: stay at the current drift while it's exact,
    else probe drift±1 (bounded) and move only when strictly better.
  * Title-segment caveat: retail renders each title update ~2.2× (parity-ledger
    R3), so the pre-`newgame_enter` region does NOT pair tick-for-tick — its
    ribbon redness is the documented R3 phase residual, not a regression.

Output: ordinal-named frame trees — frame_<ordinal>.png under port/frames,
retail/frames and diff/frames is THE SAME captured moment — plus per-pair rows
(ordinal, segment, per-side flips, drift, differ/gt8/meanabs).
"""
from __future__ import annotations

import os
import re
from pathlib import Path

DRIFT_WINDOW = 8          # max |drift| the sticky search may reach (port dup
                          # frames slip the offset ±1 each; town slips reach ±5)
SEG_BOOT = "boot"


# ─── frame indexing ──────────────────────────────────────────────────────────
_PORT_RE = re.compile(r"frame_(\d+)\.png$")
_RETAIL_RE = re.compile(r"frame_(\d+)_t(\d+)\.png$")


def index_port(raw_dir: Path) -> dict[int, Path]:
    out: dict[int, Path] = {}
    for p in Path(raw_dir).glob("frame_*.png"):
        m = _PORT_RE.search(p.name)
        if m:
            out[int(m.group(1))] = p
    return out


def index_retail(frames_dir: Path) -> dict[int, tuple[Path, int]]:
    """flip → (path, sim_tick).  Retail capture names carry both axes."""
    out: dict[int, tuple[Path, int]] = {}
    for p in Path(frames_dir).glob("frame_*.png"):
        m = _RETAIL_RE.search(p.name)
        if m:
            out[int(m.group(1))] = (p, int(m.group(2)))
        else:
            m2 = _PORT_RE.search(p.name)
            if m2:
                out[int(m2.group(1))] = (p, -1)
    return out


# ─── segmentation ────────────────────────────────────────────────────────────
def build_segments(port_anchors: list[dict], retail_anchors: list[dict],
                   port_flips: list[int], retail_flips: list[int]) -> list[dict]:
    """Segment the two flip axes on the SHARED anchors (first firing per name,
    present on both sides, monotonic).  Returns
    [{name, port_start, retail_start, n}] with n = the paired tick count
    (min of the two segment lengths).  Segment 0 is the boot (first captured
    flip each side)."""
    if not port_flips or not retail_flips:
        return []

    def first_per_name(stream: list[dict]) -> dict[str, dict]:
        seen: dict[str, dict] = {}
        for a in stream:
            if a.get("name") and a["name"] not in seen and "flip" in a:
                seen[a["name"]] = a
        return seen

    pa, ra = first_per_name(port_anchors), first_per_name(retail_anchors)
    shared = sorted(set(pa) & set(ra), key=lambda n: pa[n]["flip"])

    bounds: list[dict] = [{"name": SEG_BOOT,
                           "port_start": min(port_flips),
                           "retail_start": min(retail_flips)}]
    for n in shared:
        b = {"name": n, "port_start": int(pa[n]["flip"]),
             "retail_start": int(ra[n]["flip"])}
        if "rng" in pa[n]:
            b["port_rng"] = pa[n]["rng"]
        if "rng" in ra[n]:
            b["retail_rng"] = ra[n]["rng"]
        # monotonicity on BOTH axes (a violating anchor is dropped)
        if b["port_start"] > bounds[-1]["port_start"] \
                and b["retail_start"] > bounds[-1]["retail_start"]:
            bounds.append(b)

    p_max, r_max = max(port_flips), max(retail_flips)
    segs: list[dict] = []
    for i, b in enumerate(bounds):
        p_end = bounds[i + 1]["port_start"] if i + 1 < len(bounds) else p_max + 1
        r_end = bounds[i + 1]["retail_start"] if i + 1 < len(bounds) else r_max + 1
        n = min(p_end - b["port_start"], r_end - b["retail_start"])
        if n > 0:
            segs.append({**b, "n": int(n)})
    return segs


# ─── pixels ──────────────────────────────────────────────────────────────────
def load_rgb(path: Path):
    import numpy as np
    from PIL import Image
    try:
        return np.asarray(Image.open(path).convert("RGB"), dtype=np.int16)
    except Exception:                                    # noqa: BLE001
        return None


def diff_stats(a, b) -> tuple[int, int, float]:
    """(differ_px, gt8, meanabs) — differ counts ANY-channel non-zero pixels
    (the project's bit-exact bar); gt8 is the >8/channel triage count."""
    import numpy as np
    d = np.abs(a - b).max(axis=2)
    return int((d > 0).sum()), int((d > 8).sum()), float(np.abs(a - b).mean())


def save_diff_png(a, b, amp: float, path: Path) -> None:
    import numpy as np
    from PIL import Image
    d = np.clip(np.abs(a - b).astype(np.float32) * amp, 0, 255).astype(np.uint8)
    Image.fromarray(d).save(path)


def _link(src: Path, dst: Path) -> None:
    dst.unlink(missing_ok=True)
    try:
        os.link(src, dst)
    except OSError:
        import shutil
        shutil.copy2(src, dst)


# ─── the pairing pass ────────────────────────────────────────────────────────
def pair_session(sess_dir: Path, port_anchors: list[dict],
                 retail_anchors: list[dict], amp: float = 6.0,
                 drift_window: int = DRIFT_WINDOW) -> dict:
    """Pair port/raw ↔ retail/cap/frames into ordinal-named trees + per-pair
    stats.  Returns {pairs: [...], segments: [...], n}.  Each pair row:
    {frame, seg, port_flip, retail_flip, sim_tick, drift, differ, gt8, meanabs}
    (+ anchor/rng keys on segment-start rows)."""
    sess_dir = Path(sess_dir)
    port_by = index_port(sess_dir / "port" / "raw")
    retail_by = index_retail(sess_dir / "retail" / "cap" / "frames")
    segs = build_segments(port_anchors, retail_anchors,
                          sorted(port_by), sorted(retail_by))

    for sub in ("port/frames", "retail/frames", "diff/frames"):
        d = sess_dir / sub
        if d.exists():
            for f in d.glob("frame_*.png"):
                f.unlink()
        d.mkdir(parents=True, exist_ok=True)

    # tiny retail pixel cache (the drift probe re-reads neighbours)
    cache: dict[int, object] = {}

    def retail_rgb(flip: int):
        if flip not in retail_by:
            return None
        if flip not in cache:
            if len(cache) > 16:
                for k in sorted(cache)[:8]:
                    del cache[k]
            cache[flip] = load_rgb(retail_by[flip][0])
        return cache[flip]

    pairs: list[dict] = []
    ordinal = 0
    gaps = 0
    for seg in segs:
        drift = 0
        r_lo = seg["retail_start"]
        r_hi = seg["retail_start"] + seg["n"] - 1
        for k in range(seg["n"]):
            p_flip = seg["port_start"] + k
            if p_flip not in port_by:
                gaps += 1
                continue
            a = load_rgb(port_by[p_flip])
            if a is None:
                gaps += 1
                continue
            base = seg["retail_start"] + k

            best = None        # (differ, gt8, meanabs, drift, b)
            cur = retail_rgb(base + drift) \
                if r_lo <= base + drift <= r_hi else None
            if cur is not None and cur.shape == a.shape:
                st = diff_stats(a, cur)
                best = (*st, drift, cur)
            if best is None or best[0] != 0:
                for d in (drift - 1, drift + 1):
                    if abs(d) > drift_window or not (r_lo <= base + d <= r_hi):
                        continue
                    b = retail_rgb(base + d)
                    if b is None or b.shape != a.shape:
                        continue
                    st = diff_stats(a, b)
                    # move off the sticky drift only when STRICTLY better
                    if best is None or st[0] < best[0]:
                        best = (*st, d, b)
            if best is None:
                gaps += 1
                continue
            differ, gt8, meanabs, drift, b = best
            r_flip = base + drift

            name = f"frame_{ordinal:05d}.png"
            _link(port_by[p_flip], sess_dir / "port" / "frames" / name)
            _link(retail_by[r_flip][0], sess_dir / "retail" / "frames" / name)
            save_diff_png(a, b, amp, sess_dir / "diff" / "frames" / name)

            row = {"frame": ordinal, "seg": seg["name"],
                   "port_flip": p_flip, "retail_flip": r_flip,
                   "sim_tick": retail_by[r_flip][1], "drift": drift,
                   "differ": differ, "gt8": gt8,
                   "meanabs": round(meanabs, 4)}
            if k == 0:
                row["anchor"] = seg["name"]
                if "port_rng" in seg:
                    row["port_rng"] = seg["port_rng"]
                if "retail_rng" in seg:
                    row["retail_rng"] = seg["retail_rng"]
            pairs.append(row)
            ordinal += 1

    return {"pairs": pairs, "segments": segs, "n": ordinal, "gaps": gaps}
