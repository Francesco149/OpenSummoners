#!/usr/bin/env python3
"""
tools/render_diff.py — the DDraw blit-stream drill-in (Phase-B B3).

The RENDER-side complement to flow_diff.py: flow_diff names the first call whose
LOGIC diverged; render_diff names the first BLIT that came out wrong, and says
HOW.  SotES renders through a DirectDraw7 software blitter, so the analog of
openrecet's d3d-trace is the per-blit field-bearing call_trace event emitted at
the five blit primitives (src/zdd.c zdd_emit_blit; retail via the Frida reader +
tools/flow/retail_fields.json's blit VAs).

Both traces are ordinary call_trace.jsonl streams — render_diff filters them to
the blit VAs and aligns the per-frame blit sequence by the CROSS-SIDE IDENTITY
of each blit's source cel, NOT by the allocation-dependent pointer:

    identity = (va, res, frame)        # res = PE resource_id, frame = cel frame

This is openrecet's `tex_name` trick (drop the pointer from the alignment key,
key on the load-stable asset name) — here the name is (resource_id, frame) from
the render_id registry, identical in both binaries.  A blit whose source has no
registered identity falls back to positional alignment within its VA.

Once two blits align (same sprite, same draw slot), render_diff compares the
remaining fields and CLASSIFIES the first divergence:

    [sprite]  one side drew a sprite the other didn't (insert/delete)  — wrong draw
    [decode]  same sprite, different `dhash`            — RIGHT sprite, WRONG pixels
              (palette-grade / 24bpp-decode divergence; needs dhash on both sides)
    [rect]    same sprite, different geometry (dx/dy/reqw/reqh/sx/sy/ow/oh/ox/oy)
    [state]   same sprite, different DDraw state (st = KEYSRC arm / ckey / bmode)

Only fields present on BOTH sides are compared, so a port-only field (e.g. dhash
before the retail decode-hash lands, or the informational `mode`) never
false-flags.

Frames: pass --frame N (a flip/sim frame both sides captured) or --all to walk
every common frame.  Seed-pinned + anchor-synced captures share frame numbers.

Exit: 0 = no divergence, 1 = divergence found, 2 = structural/input error.
"""

from __future__ import annotations

import argparse
import difflib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# The five source-bearing blit primitives (src/zdd.c zdd_emit_blit).
BLIT_VAS = {
    0x5B9A40: "blt_onto",
    0x5B9B70: "blt_keyed",
    0x5B9AE0: "blt_rects",
    0x5B9BF0: "blt_clipped",
    0x5BD550: "blt_alpha",
}

# Field groups for classification (checked in this priority order).
GEOM_FIELDS  = ("dx", "dy", "reqw", "reqh", "sx", "sy", "ow", "oh", "ox", "oy")
STATE_FIELDS = ("st", "ckey", "bmode")
# `res`/`frame` form the identity (alignment key); `mode` is the VA label;
# `dhash` is the decode fingerprint (its own [decode] class).


# ── load ────────────────────────────────────────────────────────────────────


def load_blits(path: Path) -> dict[int, list[dict]]:
    """flip-frame -> blit events (only the BLIT_VAS), sorted by seq."""
    by_frame: dict[int, list[dict]] = {}
    with path.open() as f:
        for lineno, raw in enumerate(f, 1):
            raw = raw.strip()
            if not raw:
                continue
            try:
                e = json.loads(raw)
            except json.JSONDecodeError as exc:
                raise SystemExit(f"{path}:{lineno}: malformed JSON: {exc}")
            if "va" not in e or "frame" not in e:
                continue
            if int(e["va"]) not in BLIT_VAS:
                continue
            by_frame.setdefault(int(e["frame"]), []).append(e)
    for evts in by_frame.values():
        evts.sort(key=lambda e: e.get("seq", 0))
    return by_frame


def blit_identity(e: dict, ordinal: int) -> tuple:
    """Cross-side alignment key: (va, res, frame).  When the source cel has no
    registered identity, fall back to (va, '?', ordinal) so unnamed blits align
    positionally within their VA rather than collapsing together."""
    va = int(e["va"])
    f = e.get("f", {})
    if "res" in f:
        return (va, int(f["res"]), int(f.get("frame", -1)))
    return (va, "?", ordinal)


def _keys(evts: list[dict]) -> list[tuple]:
    """Identity keys for SequenceMatcher.  Ordinals are per-VA so an unnamed
    blit aligns with the unnamed blit at the same VA-relative position."""
    seen: dict[int, int] = {}
    keys = []
    for e in evts:
        va = int(e["va"])
        ordn = seen.get(va, 0)
        seen[va] = ordn + 1
        keys.append(blit_identity(e, ordn))
    return keys


# ── compare ─────────────────────────────────────────────────────────────────


def compare_blit(rv: dict, pv: dict) -> tuple[str, str] | None:
    """Two aligned blits (same identity).  Return (class, detail) for the first
    divergence in priority order [decode] > [rect] > [state], or None if equal
    on every commonly-present field."""
    rf = rv.get("f", {})
    pf = pv.get("f", {})

    # [decode] — same sprite, different decoded pixels (only when both carry it).
    if "dhash" in rf and "dhash" in pf and rf["dhash"] != pf["dhash"]:
        return ("decode", f"dhash retail={rf['dhash']} port={pf['dhash']} "
                          f"(right sprite, wrong pixels)")

    # [rect] — geometry.
    for k in GEOM_FIELDS:
        if k in rf and k in pf and rf[k] != pf[k]:
            return ("rect", f"{k} retail={rf[k]} port={pf[k]}")

    # [state] — DDraw blend/colorkey state.
    for k in STATE_FIELDS:
        if k in rf and k in pf and rf[k] != pf[k]:
            return ("state", f"{k} retail={rf[k]} port={pf[k]}")

    return None


def _id_str(key: tuple) -> str:
    va, res, frame = key
    name = BLIT_VAS.get(va, f"0x{va:x}")
    if res == "?":
        return f"{name} <unnamed#{frame}>"
    return f"{name} res=0x{res:x} frame={frame}"


def diff_frame(rblits: list[dict], pblits: list[dict]) -> list[tuple[str, str]]:
    """Align one frame's blit sequences by identity and return an ordered list
    of (class, message) divergences.  [] = the frame's blit stream matches."""
    rkeys = _keys(rblits)
    pkeys = _keys(pblits)
    sm = difflib.SequenceMatcher(a=rkeys, b=pkeys, autojunk=False)
    out: list[tuple[str, str]] = []

    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for di in range(i2 - i1):
                res = compare_blit(rblits[i1 + di], pblits[j1 + di])
                if res:
                    cls, detail = res
                    out.append((cls, f"[{cls}] {_id_str(rkeys[i1 + di])}: {detail}"))
        elif tag == "delete":
            for di in range(i1, i2):
                out.append(("sprite",
                            f"[sprite] retail drew {_id_str(rkeys[di])} — port did not"))
        elif tag == "insert":
            for dj in range(j1, j2):
                out.append(("sprite",
                            f"[sprite] port drew {_id_str(pkeys[dj])} — retail did not"))
        elif tag == "replace":
            for di in range(i1, i2):
                out.append(("sprite",
                            f"[sprite] retail drew {_id_str(rkeys[di])} (not matched by port)"))
            for dj in range(j1, j2):
                out.append(("sprite",
                            f"[sprite] port drew {_id_str(pkeys[dj])} (not matched by retail)"))
    return out


# ── driver ──────────────────────────────────────────────────────────────────


def run(retail: Path, port: Path, frames: list[int] | None,
        first_only: bool, summary: bool,
        pair: tuple[int, int] | None = None) -> int:
    rb = load_blits(retail)
    pb = load_blits(port)

    if not rb and not pb:
        print("render_diff: no blit events in either trace "
              "(did you pass --call-trace-frames covering a render frame, and "
              "is the field spec hooking the blit VAs?)", file=sys.stderr)
        return 2

    # Explicit cross-side frame pairing: retail and port frame numbers differ
    # (boot timing), so diffing a known retail frame R against a known port frame
    # P (e.g. both in the camera hold) needs an explicit map. Synthesise a single
    # shared key so the rest of the loop is unchanged.
    if pair is not None:
        rfr, pfr = pair
        if rfr not in rb or pfr not in pb:
            print(f"render_diff: pair missing (retail {rfr}={rfr in rb}, "
                  f"port {pfr}={pfr in pb}); retail has {sorted(rb)[:6]}…, "
                  f"port has {sorted(pb)[:6]}…", file=sys.stderr)
            return 2
        rb = {rfr: rb[rfr]}
        pb = {rfr: pb[pfr]}      # re-key the port frame onto the retail number
        common = [rfr]
    else:
        common = sorted(set(rb) & set(pb))
        if frames is not None:
            common = [f for f in frames if f in rb and f in pb]
            for f in frames:
                if f not in rb or f not in pb:
                    print(f"render_diff: frame {f} missing "
                          f"(retail={f in rb} port={f in pb})", file=sys.stderr)
        if not common:
            print(f"render_diff: no common frames "
                  f"(retail {sorted(rb)[:6]}…, port {sorted(pb)[:6]}…); pass "
                  f"--retail-frame R --port-frame P to pair across boot-timing skew",
                  file=sys.stderr)
            return 2

    total = 0
    classes: dict[str, int] = {}
    for fr in common:
        divs = diff_frame(rb[fr], pb[fr])
        if not divs:
            if not summary:
                print(f"frame {fr}: {len(rb[fr])} blits — MATCH")
            continue
        for cls, msg in divs:
            classes[cls] = classes.get(cls, 0) + 1
        total += len(divs)
        if summary:
            continue
        print(f"frame {fr}: {len(rb[fr])} retail / {len(pb[fr])} port blits — "
              f"{len(divs)} divergence(s)")
        shown = divs[:1] if first_only else divs
        for _, msg in shown:
            print(f"    {msg}")
        if first_only and len(divs) > 1:
            print(f"    … +{len(divs) - 1} more (drop --first to see all)")

    if total:
        cls_summary = ", ".join(f"{c}:{n}" for c, n in sorted(classes.items()))
        print(f"\nrender_diff: {total} divergence(s) across {len(common)} "
              f"frame(s) — {cls_summary}")
        return 1
    print(f"\nrender_diff: blit streams MATCH across {len(common)} frame(s)")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="DDraw blit-stream diff (B3).")
    ap.add_argument("--retail", required=True, type=Path,
                    help="retail call_trace.jsonl (Frida side)")
    ap.add_argument("--port", required=True, type=Path,
                    help="port call_trace.jsonl")
    ap.add_argument("--frame", type=int, action="append", dest="frames",
                    help="a flip/sim frame both sides captured (repeatable); "
                         "default = all common frames")
    ap.add_argument("--retail-frame", type=int, default=None,
                    help="pair a specific retail frame with --port-frame "
                         "(boot timing differs; e.g. both in the camera hold)")
    ap.add_argument("--port-frame", type=int, default=None,
                    help="the port frame to pair with --retail-frame")
    ap.add_argument("--first", action="store_true",
                    help="show only the FIRST divergence per frame (the "
                         "stop-at-first-divergence loop)")
    ap.add_argument("--summary", action="store_true",
                    help="counts only — no per-frame/per-blit detail")
    args = ap.parse_args()

    for p in (args.retail, args.port):
        if not p.exists():
            print(f"render_diff: not found: {p}", file=sys.stderr)
            return 2
    pair = None
    if (args.retail_frame is None) != (args.port_frame is None):
        print("render_diff: --retail-frame and --port-frame must be given together",
              file=sys.stderr)
        return 2
    if args.retail_frame is not None:
        pair = (args.retail_frame, args.port_frame)
    return run(args.retail, args.port, args.frames, args.first, args.summary, pair)


if __name__ == "__main__":
    sys.exit(main())
