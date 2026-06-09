"""analysis/verdict.py — the cheap always-on RNG/pairing verdict (+ flow_diff
when call traces exist).

Layer 1 (free, every session): compare the LCG state at every shared anchor —
both sides stamp `rng=` on their anchor emissions.  Equal at an anchor ⇒ the
two sides consumed an identical draw count up to that boundary (the value is
seed+count-determined); a mismatch names the FIRST segment with an unaccounted
consumer.  Also summarizes the pairing (bit-exact frame counts per segment).

Layer 2 (--call-trace sessions): tools/flow_diff.py over the two traces, the
first-divergent-call drill-in.  Stored as text for the Verdict panel.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from ..paths import ROOT


def anchor_rng_verdict(segments: list[dict], pairs: list[dict],
                       retail_seed_pinned: bool | None = None) -> dict:
    lines: list[str] = []
    ok = True
    if retail_seed_pinned is False:
        lines.append(
            "NOTE: retail's boot seed was NEVER pinned this run — the pin "
            "lands at the first title sparkle (0x56c070), which this "
            "scenario's nav skips (quirk #77). Anchor-rng DESYNC before "
            "game_enter is EXPECTED; the town re-pins at game_enter "
            "(rng_anchor) on both sides.")
    for s in segments:
        pr, rr = s.get("port_rng"), s.get("retail_rng")
        if s["name"] == "boot":
            continue
        if pr is None or rr is None:
            lines.append(f"{s['name']}: rng n/a "
                         f"({'port' if pr is None else 'retail'} side missing)")
        elif pr == rr:
            lines.append(f"{s['name']}: rng ALIGNED (0x{pr:08x})")
        else:
            ok = False
            lines.append(f"{s['name']}: rng DESYNC port=0x{pr:08x} "
                         f"retail=0x{rr:08x} — an unaccounted LCG consumer "
                         f"in the PRECEDING segment")
    by_seg: dict[str, list[dict]] = {}
    for p in pairs:
        by_seg.setdefault(p["seg"], []).append(p)
    for name, rows in by_seg.items():
        n = len(rows)
        exact = sum(1 for r in rows if r["differ"] == 0)
        clean8 = sum(1 for r in rows if r["gt8"] == 0)
        note = "  [R3 render-rate skew — documented]" \
            if name in ("boot", "subtitle_anim_start") and exact < n else ""
        lines.append(f"{name}: {exact}/{n} bit-exact (differ_px==0), "
                     f"{clean8}/{n} clean at >8/ch{note}")
    return {"available": True, "ok": ok, "text": "\n".join(lines)}


def flow_verdict(sess_dir: Path) -> dict | None:
    rp = Path(sess_dir) / "retail" / "cap" / "call_trace.jsonl"
    pp = Path(sess_dir) / "port" / "call_trace.jsonl"
    if not (rp.is_file() and pp.is_file()):
        return None
    r = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "flow_diff.py"),
         "--retail", str(rp), "--port", str(pp)],
        capture_output=True, text=True, cwd=str(ROOT))
    return {"available": True, "exit_code": r.returncode,
            "text": r.stdout + (("\n[stderr]\n" + r.stderr) if r.stderr else "")}
