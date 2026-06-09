#!/usr/bin/env python3
"""
tools/test_trace_studio.py — sanity tests for the trace-studio package.

Run with `nix develop --command python3 tools/test_trace_studio.py`.
Exits non-zero on failure; prints `OK` on success.

Covers the pure core (no live targets):
  1. build_segments: boot + shared anchors, intersection rule, monotonic
     filtering, per-segment pair counts, rng carry-through.
  2. pair_session on a synthetic session: ordinal naming across segments,
     bit-exact detection, the sticky ±drift best-match absorbing a port
     duplicate frame, diff PNG emission, anchor/rng rows.
  3. state rows: pairing → state.jsonl rows (flips, sim_tick, anchor keys).
  4. anchor-RNG verdict: ALIGNED vs DESYNC + per-segment bit-exact counts.
  5. worklist render (apply): mark context lines (segment, flips, differ, box).
  6. input-trace text validation (the POST /trace gate).
  7. port-log anchor parsing.
"""
from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np                                       # noqa: E402
from PIL import Image                                    # noqa: E402

from trace_studio.analysis import pairing                # noqa: E402
from trace_studio.analysis import state as state_mod     # noqa: E402
from trace_studio.analysis import verdict as verdict_mod # noqa: E402
from trace_studio.drive.port import parse_anchors        # noqa: E402
from trace_studio.edits import apply as apply_mod        # noqa: E402
from trace_studio.model import session as sess_mod       # noqa: E402

FAILS = 0


def check(name: str, cond: bool, detail: str = "") -> None:
    global FAILS
    if cond:
        print(f"  ok    {name}")
    else:
        FAILS += 1
        print(f"  FAIL  {name}  {detail}")


def img(color, w=8, h=8):
    a = np.zeros((h, w, 3), dtype=np.uint8)
    a[:, :] = color
    return a


def save(a, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(a).save(path)


# ─── 1. build_segments ────────────────────────────────────────────────────────
def test_segments():
    print("build_segments:")
    pa = [{"name": "newgame_enter", "flip": 100, "rng": 0xAA},
          {"name": "game_enter", "flip": 200, "rng": 0xBB},
          {"name": "port_only", "flip": 150}]
    ra = [{"name": "newgame_enter", "flip": 110, "rng": 0xAA},
          {"name": "game_enter", "flip": 230, "rng": 0xCC}]
    segs = pairing.build_segments(pa, ra, list(range(1, 301)),
                                  list(range(2, 351)))
    check("3 segments (boot + 2 shared)", len(segs) == 3, repr(segs))
    check("port_only anchor excluded",
          all(s["name"] != "port_only" for s in segs))
    boot = segs[0]
    check("boot starts at min flips",
          boot["port_start"] == 1 and boot["retail_start"] == 2)
    check("boot n = min(99, 108)", boot["n"] == 99, repr(boot))
    ng = segs[1]
    check("newgame seg n = min(100, 120)", ng["n"] == 100, repr(ng))
    check("rng carried", ng["port_rng"] == 0xAA and ng["retail_rng"] == 0xAA)
    last = segs[2]
    check("last seg n = min(101, 121)", last["n"] == 101, repr(last))
    # non-monotonic retail anchor is dropped
    ra_bad = ra + [{"name": "weird", "flip": 50}]
    pa_bad = pa + [{"name": "weird", "flip": 250}]
    segs2 = pairing.build_segments(pa_bad, ra_bad, list(range(1, 301)),
                                   list(range(2, 351)))
    check("non-monotonic shared anchor dropped",
          all(s["name"] != "weird" for s in segs2))


# ─── 2./3. pair_session + state ──────────────────────────────────────────────
def test_pairing(tmp: Path):
    print("pair_session:")
    sess = tmp / "sess"
    praw = sess / "port" / "raw"
    rcap = sess / "retail" / "cap" / "frames"

    # Two segments: boot (port 1-3 / retail 1-3) and game (port 4-7 /
    # retail 5-9).  Boot pairs 1:1 identical.  In game, port flip 5 is a
    # DUPLICATE of flip 4's content (the cadence wrinkle): retail content
    # advances every flip, so from port flip 5 on the best match sits at
    # drift -1.
    colors = {0: (10, 10, 10), 1: (50, 0, 0), 2: (0, 60, 0), 3: (0, 0, 70),
              4: (80, 80, 0), 5: (0, 80, 80)}
    # boot: identical triples
    for k in range(3):
        save(img(colors[k]), praw / f"frame_{1 + k:05d}.png")
        save(img(colors[k]), rcap / f"frame_{1 + k:05d}_t{100 + k:06d}.png")
    # game segment: retail advances 3,4,5 at flips 5,6,7 (+ extra 8,9)
    seq = [3, 4, 5, 4, 5]
    for i, c in enumerate(seq):
        save(img(colors[c]), rcap / f"frame_{5 + i:05d}_t{200 + i:06d}.png")
    # port: 3 at flip 4, DUP 3 at flip 5, then 4, 5 (flips 6, 7)
    for flip, c in ((4, 3), (5, 3), (6, 4), (7, 5)):
        save(img(colors[c]), praw / f"frame_{flip:05d}.png")

    pa = [{"name": "game_enter", "flip": 4, "rng": 0x11}]
    ra = [{"name": "game_enter", "flip": 5, "rng": 0x11}]
    out = pairing.pair_session(sess, pa, ra, amp=4.0)
    pairs = out["pairs"]
    check("7 ordinals paired", out["n"] == 7, repr(out["n"]))
    check("boot pairs bit-exact",
          all(p["differ"] == 0 for p in pairs if p["seg"] == "boot"))
    game = [p for p in pairs if p["seg"] == "game_enter"]
    check("game seg has 4 pairs", len(game) == 4, repr(len(game)))
    check("game k0 exact at drift 0",
          game[0]["differ"] == 0 and game[0]["drift"] == 0)
    check("port dup absorbed at drift -1",
          game[1]["differ"] == 0 and game[1]["drift"] == -1, repr(game[1]))
    check("drift stays sticky -1, still exact",
          all(p["drift"] == -1 and p["differ"] == 0 for p in game[2:]),
          repr(game[2:]))
    check("anchor row carries rng",
          game[0].get("anchor") == "game_enter"
          and game[0].get("port_rng") == 0x11)
    check("sim_tick from retail name", game[0]["sim_tick"] == 200)
    for sub in ("port", "retail", "diff"):
        n = len(list((sess / sub / "frames").glob("frame_*.png")))
        check(f"{sub}/frames has 7 ordinal files", n == 7, f"{sub}: {n}")
    # ordinal files pair the same content (frame_00003 = game k0 on both)
    a = np.asarray(Image.open(sess / "port" / "frames" / "frame_00003.png"))
    b = np.asarray(Image.open(sess / "retail" / "frames" / "frame_00003.png"))
    check("ordinal 3 bit-identical across sides", (a == b).all())

    print("state rows:")
    rows = state_mod.build_state(sess, pairs, call_trace=False)
    check("one row per ordinal", len(rows) == 7)
    check("row carries flips",
          rows[3]["port"]["flip"] == 4 and rows[3]["retail"]["flip"] == 5)
    check("row carries seg + drift",
          rows[4]["seg"] == "game_enter" and rows[4]["drift"] == -1)
    check("anchor key only on segment starts",
          "anchor" in rows[3] and "anchor" not in rows[4])

    print("verdict:")
    v = verdict_mod.anchor_rng_verdict(out["segments"], pairs)
    check("aligned anchors verdict ok", v["ok"] is True, v["text"])
    check("ALIGNED named", "ALIGNED" in v["text"], v["text"])
    segs_bad = [dict(out["segments"][1], port_rng=1, retail_rng=2)]
    v2 = verdict_mod.anchor_rng_verdict(segs_bad, pairs)
    check("desync flagged", v2["ok"] is False and "DESYNC" in v2["text"])

    # ─── 5. worklist ─────────────────────────────────────────────────────────
    print("apply/worklist:")
    manifest = {"schema": sess_mod.SCHEMA, "session": "t", "n_frames": 7,
                "diff": {"per_frame": [{"frame": p["frame"],
                                        "differ": p["differ"],
                                        "gt8": p["gt8"]} for p in pairs]}}
    sess_mod.write_manifest(sess, manifest)
    (sess / "state.jsonl").write_text(
        "".join(json.dumps(r) + "\n" for r in rows))
    (sess / "edits.jsonl").write_text(json.dumps(
        {"frame": 4, "kind": "note", "note": "dialogue box missing",
         "box": [1, 2, 5, 6]}) + "\n")
    res = apply_mod.apply(sess)
    wl = (sess / "worklist.md").read_text()
    check("apply ok", res["ok"] and res["n_marks"] == 1)
    # ordinal 4 is the port DUP frame: drift -1 pairs it with retail flip 5
    check("worklist names the segment + flips",
          "seg `game_enter`" in wl and "port flip 5" in wl
          and "retail flip 5" in wl, wl)
    check("worklist carries note + box",
          "dialogue box missing" in wl and "box: 1,2,5,6" in wl, wl)


# ─── 6. trace text validation ────────────────────────────────────────────────
def test_validate():
    print("validate_trace_text:")
    ok = '# c\n{"frame": 1, "ids": [3]}\n\n{"frame": 5, "ids": [36, 1]}\n'
    check("valid trace accepted", sess_mod.validate_trace_text(ok) is None)
    check("bad json rejected",
          sess_mod.validate_trace_text('{"frame": 1, ids}') is not None)
    check("missing ids rejected",
          sess_mod.validate_trace_text('{"frame": 1}') is not None)
    bad_order = '{"frame": 5, "ids": [1]}\n{"frame": 4, "ids": [1]}'
    check("decreasing frames rejected",
          sess_mod.validate_trace_text(bad_order) is not None)


# ─── 7. port-log anchors ─────────────────────────────────────────────────────
def test_anchor_parse():
    print("parse_anchors:")
    log = ("[opensummoners] boot\n"
           "[opensummoners] anchor: subtitle_anim_start flip=438 rng=0x004f5347\n"
           "noise\n"
           "[opensummoners] anchor: game_enter flip=1300 rng=0xdeadbeef\n")
    a = parse_anchors(log)
    check("two anchors", len(a) == 2, repr(a))
    check("fields parsed", a[0] == {"name": "subtitle_anim_start", "flip": 438,
                                    "rng": 0x4F5347}, repr(a[0]))
    check("hex rng parsed", a[1]["rng"] == 0xDEADBEEF)


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        test_segments()
        test_pairing(Path(td))
        test_validate()
        test_anchor_parse()
    if FAILS:
        print(f"\n{FAILS} FAILURE(S)")
        return 1
    print("\nOK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
