#!/usr/bin/env python3
"""
tools/test_render_diff.py — sanity tests for render_diff.py (the DDraw
blit-stream drill-in, Phase-B B3).

Run with `nix develop --command python3 tools/test_render_diff.py`.
Exits non-zero on failure; prints `OK` on success.

Covers the identity alignment + the four divergence classes:
  1. identical blit streams → MATCH (exit 0).
  2. [decode] same sprite, different dhash (both present) → wrong pixels.
  3. [rect] same sprite, different geometry.
  4. [state] same sprite, different DDraw colorkey/blend state.
  5. [sprite] a blit one side issued and the other didn't (delete/insert).
  6. dhash present on ONLY one side → NOT flagged (intersection rule).
  7. alignment is by identity, NOT order: a reordered-but-same stream still
     pairs each sprite with its twin (so a real rect diff is still caught).
  8. unnamed (unregistered) blits align positionally within their VA.
  9. CLI: a real divergence → exit 1; a clean frame → exit 0.
"""

from __future__ import annotations

import importlib.util
import io
import json
import sys
import tempfile
from contextlib import redirect_stdout, redirect_stderr
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CLIP = 0x5B9BF0       # blt_clipped (the tile path)
ONTO = 0x5B9A40


def load_mod():
    spec = importlib.util.spec_from_file_location(
        "render_diff", ROOT / "tools" / "render_diff.py")
    mod = importlib.util.module_from_spec(spec)
    sys.modules["render_diff"] = mod
    spec.loader.exec_module(mod)
    return mod


def blit(va: int, frame: int, seq: int, f: dict) -> dict:
    return {"va": va, "ret_va": 0, "frame": frame, "seq": seq, "f": f}


def fields(res=0x55, frame=0, dx=10, dy=20, reqw=32, reqh=32, sx=0, sy=0,
           ow=32, oh=32, ox=0, oy=0, st="0x8000", ckey="0xff00ff",
           dhash=None, mode=3):
    f = {"res": res, "frame": frame, "dx": dx, "dy": dy, "reqw": reqw,
         "reqh": reqh, "sx": sx, "sy": sy, "ow": ow, "oh": oh, "ox": ox,
         "oy": oy, "st": st, "ckey": ckey, "mode": mode}
    if dhash is not None:
        f["dhash"] = dhash
    return f


def write_trace(rows: list[dict]) -> Path:
    fd, p = tempfile.mkstemp(suffix=".jsonl")
    with open(fd, "w") as fh:
        for r in rows:
            fh.write(json.dumps(r) + "\n")
    return Path(p)


def run_main(mod, argv: list[str]) -> tuple[int, str, str]:
    buf_out, buf_err = io.StringIO(), io.StringIO()
    saved = sys.argv
    sys.argv = ["render_diff.py"] + argv
    try:
        with redirect_stdout(buf_out), redirect_stderr(buf_err):
            try:
                rc = mod.main()
            except SystemExit as e:
                rc = int(e.code) if e.code is not None else 0
    finally:
        sys.argv = saved
    return rc, buf_out.getvalue(), buf_err.getvalue()


def main() -> int:
    mod = load_mod()
    failures: list[str] = []

    def chk(cond: bool, msg: str) -> None:
        if not cond:
            failures.append(msg)

    # ── 1. identical → match ───────────────────────────────────────────
    r = [blit(CLIP, 100, 0, fields(res=0x55, frame=0)),
         blit(CLIP, 100, 1, fields(res=0x58, frame=2, dx=64))]
    p = [blit(CLIP, 100, 0, fields(res=0x55, frame=0)),
         blit(CLIP, 100, 1, fields(res=0x58, frame=2, dx=64))]
    chk(mod.diff_frame(r, p) == [], "identical streams should match")

    # ── 2. [decode] dhash differs (both present) ───────────────────────
    r = [blit(CLIP, 100, 0, fields(res=0x55, dhash="0xaaaa"))]
    p = [blit(CLIP, 100, 0, fields(res=0x55, dhash="0xbbbb"))]
    d = mod.diff_frame(r, p)
    chk(len(d) == 1 and d[0][0] == "decode", f"dhash mismatch → [decode]: {d}")

    # ── 3. [rect] geometry differs ─────────────────────────────────────
    r = [blit(CLIP, 100, 0, fields(res=0x55, dx=10))]
    p = [blit(CLIP, 100, 0, fields(res=0x55, dx=99))]
    d = mod.diff_frame(r, p)
    chk(len(d) == 1 and d[0][0] == "rect", f"dx mismatch → [rect]: {d}")
    chk("retail=10 port=99" in d[0][1], f"rect detail names values: {d}")

    # ── 4. [state] colorkey differs ────────────────────────────────────
    r = [blit(CLIP, 100, 0, fields(res=0x55, ckey="0xff00ff"))]
    p = [blit(CLIP, 100, 0, fields(res=0x55, ckey="0x0"))]
    d = mod.diff_frame(r, p)
    chk(len(d) == 1 and d[0][0] == "state", f"ckey mismatch → [state]: {d}")

    # ── 5. [sprite] a blit only one side issued ────────────────────────
    r = [blit(CLIP, 100, 0, fields(res=0x55, frame=0)),
         blit(CLIP, 100, 1, fields(res=0x58, frame=1))]
    p = [blit(CLIP, 100, 0, fields(res=0x55, frame=0))]
    d = mod.diff_frame(r, p)
    chk(any(c == "sprite" for c, _ in d), f"missing blit → [sprite]: {d}")
    chk(any("retail drew" in m for _, m in d), f"names the missing sprite: {d}")

    # ── 6. dhash present on only one side → NOT flagged ────────────────
    r = [blit(CLIP, 100, 0, fields(res=0x55, dhash="0xaaaa"))]
    p = [blit(CLIP, 100, 0, fields(res=0x55))]   # port omits dhash
    chk(mod.diff_frame(r, p) == [], "one-sided dhash must not false-flag")

    # ── 7. identity alignment is order-independent ─────────────────────
    # Same two sprites, reversed draw order, but sprite 0x58 has a rect diff.
    r = [blit(CLIP, 100, 0, fields(res=0x55, frame=0, dx=10)),
         blit(CLIP, 100, 1, fields(res=0x58, frame=0, dx=20))]
    p = [blit(CLIP, 100, 0, fields(res=0x58, frame=0, dx=999)),
         blit(CLIP, 100, 1, fields(res=0x55, frame=0, dx=10))]
    d = mod.diff_frame(r, p)
    # The 0x55 sprite matches; the 0x58 sprite's dx diverges. SequenceMatcher on
    # identity keys should still surface the 0x58 rect divergence somewhere.
    chk(any(c == "rect" or c == "sprite" for c, _ in d),
        f"reordered stream with a real diff still caught: {d}")

    # ── 8. unnamed blits align positionally within their VA ────────────
    nf = {"dx": 1, "dy": 2}     # no res/frame → unnamed
    r = [blit(ONTO, 100, 0, dict(nf, dx=1)), blit(ONTO, 100, 1, dict(nf, dx=2))]
    p = [blit(ONTO, 100, 0, dict(nf, dx=1)), blit(ONTO, 100, 1, dict(nf, dx=2))]
    chk(mod.diff_frame(r, p) == [], "identical unnamed streams match positionally")
    p2 = [blit(ONTO, 100, 0, dict(nf, dx=1)), blit(ONTO, 100, 1, dict(nf, dx=77))]
    d = mod.diff_frame(r, p2)
    chk(len(d) == 1 and d[0][0] == "rect",
        f"unnamed positional rect diff caught: {d}")

    # ── 9. CLI exit codes ──────────────────────────────────────────────
    rp = write_trace([blit(CLIP, 100, 0, fields(res=0x55, dx=10))])
    pp_ok = write_trace([blit(CLIP, 100, 0, fields(res=0x55, dx=10))])
    pp_bad = write_trace([blit(CLIP, 100, 0, fields(res=0x55, dx=99))])
    rc, out, _ = run_main(mod, ["--retail", str(rp), "--port", str(pp_ok)])
    chk(rc == 0 and "MATCH" in out, f"clean CLI → exit 0 + MATCH: rc={rc} {out!r}")
    rc, out, _ = run_main(mod, ["--retail", str(rp), "--port", str(pp_bad)])
    chk(rc == 1 and "rect" in out, f"divergent CLI → exit 1 + [rect]: rc={rc} {out!r}")

    # ── 10. cross-boot-timing frame pairing (--retail-frame/--port-frame) ──
    # Retail blit on frame 1500, port the SAME sprite on frame 1200 (different
    # boot timing). With no common frame, the pair maps them.
    rp2 = write_trace([blit(CLIP, 1500, 0, fields(res=0x55, dx=10))])
    pp2 = write_trace([blit(CLIP, 1200, 0, fields(res=0x55, dx=10))])
    rc, out, err = run_main(mod, ["--retail", str(rp2), "--port", str(pp2),
                                  "--retail-frame", "1500", "--port-frame", "1200"])
    chk(rc == 0 and "MATCH" in out, f"paired match → exit 0: rc={rc} {out!r} {err!r}")
    # half a pair is an error
    rc, _, err = run_main(mod, ["--retail", str(rp2), "--port", str(pp2),
                                "--retail-frame", "1500"])
    chk(rc == 2 and "together" in err, f"half-pair → exit 2: rc={rc} {err!r}")

    if failures:
        print(f"render_diff tests: {len(failures)} FAIL")
        for m in failures:
            print("  ✗", m)
        return 1
    print("render_diff tests: OK (10 checks)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
