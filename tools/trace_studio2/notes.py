#!/usr/bin/env python3
"""tools/trace_studio2/notes.py — the agent READ side of the osr_view note system.

The USER drags a crop region + types a note in osr_view's dual mode; it persists to
`osr_notes.jsonl` (one JSON object per line: tick, port_flip, retail_flip, crop
[x,y,w,h], differ, text).  This tool reads that file, resolves each note's joined
`sim_tick` to the per-side frame index (via the same tick-join as pair.py), and —
with --render — reconstructs the port + retail frames at that tick (through
`build/osr_prof.exe`, the headless osr_scrub dump), crops to the noted region, and
writes a port|retail|diff montage so the agent sees EXACTLY what the USER flagged.
--feed also pushes each montage to the llm-feed.

Usage (host tools need the nix prefix):
  nix develop --command python3 tools/trace_studio2/notes.py PORT.osr RETAIL.osr            # list notes
  nix develop --command python3 tools/trace_studio2/notes.py PORT.osr RETAIL.osr --render   # + crop montages
  nix develop --command python3 tools/trace_studio2/notes.py PORT.osr RETAIL.osr --render --feed
  (optional 3rd positional = the notes.jsonl path; default = beside PORT.osr)
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import osr   # noqa: E402
import pair  # noqa: E402

PROF = Path(__file__).resolve().parents[2] / "build" / "osr_prof.exe"


def to_win(p: str) -> str:
    """/mnt/c/x/y → C:\\x\\y (osr_prof.exe needs a Windows path); leave C:\\… as-is."""
    if len(p) > 2 and p[1] == ":":
        return p
    if p.startswith("/mnt/") and len(p) > 6 and p[6] == "/":
        return p[5].upper() + ":" + p[6:].replace("/", "\\")
    return p


def to_wsl(p: str) -> str:
    """C:\\x\\y → /mnt/c/x/y (to read the BMP back via DrvFs)."""
    if len(p) > 2 and p[1] == ":":
        return "/mnt/" + p[0].lower() + p[2:].replace("\\", "/")
    return p


def default_notes_path(port_path: str) -> str:
    return str(Path(to_wsl(port_path)).parent / "osr_notes.jsonl")


def load_notes(path: str) -> list[dict]:
    notes = []
    wp = to_wsl(path)
    if not os.path.exists(wp):
        return notes
    with open(wp) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                notes.append(json.loads(line))
            except json.JSONDecodeError as e:
                print(f"  (skipped a malformed note line: {e})", file=sys.stderr)
    return notes


def dump_frame(osr_path: str, idx: int, out_win: str) -> bool:
    r = subprocess.run([str(PROF), to_win(osr_path), "dump", str(idx), out_win],
                       capture_output=True, text=True)
    return r.returncode == 0 and os.path.exists(to_wsl(out_win))


def render_note(nt: dict, port_osr: str, retail_osr: str, pidx: int, ridx: int,
                outdir_win: str, idx: int, *, feed: bool) -> str | None:
    from PIL import Image, ImageDraw
    pbmp = f"{outdir_win}\\note{idx}_p.bmp"
    rbmp = f"{outdir_win}\\note{idx}_r.bmp"
    if not dump_frame(port_osr, pidx, pbmp) or not dump_frame(retail_osr, ridx, rbmp):
        print(f"    render FAILED (osr_prof dump)")
        return None
    p = Image.open(to_wsl(pbmp)).convert("RGB")
    r = Image.open(to_wsl(rbmp)).convert("RGB")
    W, H = p.size
    x, y, cw, ch = nt.get("crop", [0, 0, 0, 0])
    if cw and ch:
        box = (max(0, x), max(0, y), min(W, x + cw), min(H, y + ch))
    else:
        box = (0, 0, W, H)
    pc, rc = p.crop(box), r.crop(box)
    cwid, chei = pc.size
    pd, rd = pc.load(), rc.load()
    diff = Image.new("RGB", (cwid, chei))
    dd = diff.load()
    differ = mx = 0
    for j in range(chei):
        for i in range(cwid):
            a, b = pd[i, j], rd[i, j]
            dr, dg, db = abs(a[0]-b[0]), abs(a[1]-b[1]), abs(a[2]-b[2])
            d = max(dr, dg, db)
            if d:
                differ += 1; mx = max(mx, d)
                dd[i, j] = (255, max(0, 255-d), 0)
            else:
                lum = (a[0]*30 + a[1]*59 + a[2]*11)//100
                s = lum*40//255
                dd[i, j] = (s, s, s)
    # upscale small crops so they're legible on the feed
    zoom = max(1, min(4, 480 // max(1, chei), 640 // max(1, cwid)))
    if zoom > 1:
        pc = pc.resize((cwid*zoom, chei*zoom), Image.NEAREST)
        rc = rc.resize((cwid*zoom, chei*zoom), Image.NEAREST)
        diff = diff.resize((cwid*zoom, chei*zoom), Image.NEAREST)
    mw, mh = pc.size
    m = Image.new("RGB", (mw*3+16, mh), (20, 20, 24))
    m.paste(pc, (0, 0)); m.paste(rc, (mw+8, 0)); m.paste(diff, (mw*2+16, 0))
    out = str(Path(to_wsl(outdir_win)) / f"note{idx}.png")
    m.save(out)
    print(f"    crop differ_px={differ} maxd={mx}  zoom×{zoom}  -> {out}")
    if feed:
        title = f"osr note @tick {nt['tick']}: {nt.get('text','')[:60]}"
        note = (f"crop ({x},{y}) {cw}x{ch}  port#{pidx}/flip{nt.get('port_flip')} "
                f"vs retail#{ridx}/flip{nt.get('retail_flip')}  crop differ_px={differ} maxd={mx}. "
                f"left=PORT mid=RETAIL right=DIFF.")
        feedpy = "/opt/src/llm-feed/feed.py"
        subprocess.run(["python3", feedpy, "image", out, "--title", title, "--note", note])
    return out


def main(argv) -> int:
    ap = argparse.ArgumentParser(description="read osr_view notes + render the flagged crops")
    ap.add_argument("port")
    ap.add_argument("retail")
    ap.add_argument("notes", nargs="?", help="notes.jsonl (default: beside PORT.osr)")
    ap.add_argument("--render", action="store_true", help="render cropped port|retail|diff per note")
    ap.add_argument("--feed", action="store_true", help="push each montage to the llm-feed")
    args = ap.parse_args(argv[1:])

    notes_path = args.notes or default_notes_path(args.port)
    notes = load_notes(notes_path)
    print(f"notes file: {notes_path}  ({len(notes)} notes)")
    if not notes:
        return 0

    port = pair.load_side(args.port)
    retail = pair.load_side(args.retail)
    outdir_win = to_win(str(Path(to_wsl(args.port)).parent / "note_render"))
    if args.render:
        os.makedirs(to_wsl(outdir_win), exist_ok=True)

    for i, nt in enumerate(notes):
        t = nt["tick"]
        pe = port.by_tick.get(t)
        re_ = retail.by_tick.get(t)
        pidx = pe[0] if pe else -1
        ridx = re_[0] if re_ else -1
        kind = "paired" if (pe and re_) else ("port-only" if pe else "retail-only")
        crop = nt.get("crop", [0, 0, 0, 0])
        cstr = f" crop({crop[0]},{crop[1]} {crop[2]}x{crop[3]})" if crop[2] or crop[3] else " (whole frame)"
        print(f"#{i} tick {t} [{kind}]{cstr}  differ={nt.get('differ')}  "
              f"port#{pidx} retail#{ridx}  :: {nt.get('text','')}")
        if args.render:
            if pe and re_:
                render_note(nt, args.port, args.retail, pidx, ridx, outdir_win, i, feed=args.feed)
            else:
                print(f"    (not paired — no cross-side render)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
