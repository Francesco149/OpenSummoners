#!/usr/bin/env python3
"""
tools/push_comparison.py — push a port|retail click-to-reveal amplified-diff
"comparison" item to the llm-feed (so the user sees the visual pixel diff, not
just a side-by-side montage).

Builds one atlas image: row 0 = [ PORT | RETAIL ] side-by-side (always shown),
row 1 = the amplified pixel-diff (revealed on click; BLACK = bit-identical,
brighter = larger delta).  The diff math is reused from the sibling project's
pixel_diff.amplified_diff (PIL + numpy).

Run inside `nix develop` (needs numpy/PIL).  Best-effort: silent if the feed is
down.

Usage:
    push_comparison.py --port PORT.png --retail RETAIL.png \
        --title "title menu" --note "Flip 200 vs golden 1900" [--amp 6]

Frames are compared at native resolution (cropped to the common region).  Pass
PNGs (convert port BMP captures with PIL first).
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path

# Reuse the sibling's diff math (no hyphen → importable). Kept external so the
# amplification stays identical across the three RE ports.
sys.path.insert(0, "/opt/src/OpenMare/tools")
sys.path.insert(0, "/opt/src/openrecet/tools")
from pixel_diff import amplified_diff  # noqa: E402

from PIL import Image, ImageDraw, ImageFont  # noqa: E402
import numpy as np  # noqa: E402

FEED_PY = Path(os.environ.get("LLM_FEED_PY", "/opt/src/llm-feed/feed.py"))
FEED_PORT = int(os.environ.get("LLM_FEED_PORT", "8777"))

PANEL_W, PANEL_H, LABEL_H = 460, 345, 18


def _feed_up() -> bool:
    try:
        with urllib.request.urlopen(
                f"http://localhost:{FEED_PORT}/healthz", timeout=2) as r:
            return r.status == 200
    except Exception:
        return False


def _font(size: int):
    for p in ("/run/current-system/sw/share/X11/fonts/DejaVuSans.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"):
        try:
            return ImageFont.truetype(p, size)
        except OSError:
            pass
    return ImageFont.load_default()


def _panel(img: Image.Image, label: str) -> Image.Image:
    body = img.convert("RGB").resize((PANEL_W, PANEL_H), Image.LANCZOS)
    out = Image.new("RGB", (PANEL_W, PANEL_H + LABEL_H), (12, 12, 14))
    d = ImageDraw.Draw(out)
    d.text((4, 2), label, fill=(200, 220, 255), font=_font(13))
    out.paste(body, (0, LABEL_H))
    return out


def build_atlas(port_png: Path, retail_png: Path, out_path: Path,
                label: str, amp: float) -> dict:
    la = Image.open(port_png).convert("RGB")
    ra = Image.open(retail_png).convert("RGB")
    cw, ch = min(la.width, ra.width), min(la.height, ra.height)
    diff_rgb, differ, meanabs = amplified_diff(
        np.asarray(la.crop((0, 0, cw, ch))),
        np.asarray(ra.crop((0, 0, cw, ch))), amp)

    atlas_w = PANEL_W * 2
    row0_h = PANEL_H + LABEL_H
    diff_disp_h = max(1, round(atlas_w * ch / cw))
    diff_fit = Image.fromarray(diff_rgb, "RGB").resize(
        (atlas_w, diff_disp_h), Image.NEAREST)
    row1_h = diff_disp_h + LABEL_H
    diff_panel = Image.new("RGB", (atlas_w, row1_h), (12, 12, 14))
    ImageDraw.Draw(diff_panel).text(
        (4, 2), f"diff x{amp:g}  ·  {differ} px differ  ·  "
        f"mean|abs|/ch {meanabs:.2f}  ·  black = bit-identical",
        fill=(255, 240, 120), font=_font(13))
    diff_panel.paste(diff_fit, (0, LABEL_H))

    total_h = row0_h + row1_h
    atlas = Image.new("RGB", (atlas_w, total_h), (12, 12, 14))
    atlas.paste(_panel(la, f"port · {label}"), (0, 0))
    atlas.paste(_panel(ra, f"retail · {label}"), (PANEL_W, 0))
    atlas.paste(diff_panel, (0, row0_h))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(out_path, optimize=True)
    return {
        "atlas": str(out_path), "label": label,
        "differ_px": differ, "meanabs": round(meanabs, 2),
        "row0_pct": round(100.0 * row0_h / atlas_w, 4),
        "total_pct": round(100.0 * total_h / atlas_w, 4),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True, type=Path)
    ap.add_argument("--retail", required=True, type=Path)
    ap.add_argument("--title", default="port | retail")
    ap.add_argument("--note", default="")
    ap.add_argument("--label", default="frame")
    ap.add_argument("--amp", type=float, default=6.0)
    args = ap.parse_args()

    if not _feed_up():
        print("push_comparison: feed down — skipping", file=sys.stderr)
        return 0

    dest = Path(tempfile.mkdtemp(prefix="oss_cmp_"))
    panel = build_atlas(args.port, args.retail, dest / "atlas.png",
                        args.label, args.amp)
    spec = {
        "title": args.title, "note": args.note,
        "left_label": "port", "right_label": "retail",
        "panels": [panel],
    }
    spec_path = dest / "spec.json"
    spec_path.write_text(json.dumps(spec))
    return subprocess.run(
        [sys.executable, str(FEED_PY), "comparison", "--spec", str(spec_path),
         "--title", args.title, "--note", args.note]).returncode


if __name__ == "__main__":
    sys.exit(main())
