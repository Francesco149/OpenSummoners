"""edits/apply.py — render the session's marks (edits.jsonl) into worklist.md.

The worklist is the hand-off artifact: the USER scrubs the studio and drops
marks; `apply` (CLI or the ✎ button) writes a Claude-ready checklist with the
full frame context — segment + anchor-relative tick, BOTH absolute flips (for
re-capture/probe commands), the pair's differ_px, and the crop box if one was
attached.  No trace mutation happens here (all mark kinds are worklist kinds —
see edits/marks.py).
"""
from __future__ import annotations

import json
from pathlib import Path


def _load_jsonl(p: Path) -> list[dict]:
    if not p.is_file():
        return []
    out = []
    for ln in p.read_text().splitlines():
        s = ln.strip()
        if s:
            try:
                out.append(json.loads(s))
            except json.JSONDecodeError:
                pass
    return out


def apply(sess_dir: Path) -> dict:
    sess_dir = Path(sess_dir)
    manifest = json.loads((sess_dir / "session.json").read_text())
    marks = _load_jsonl(sess_dir / "edits.jsonl")
    state = {r.get("frame"): r for r in _load_jsonl(sess_dir / "state.jsonl")}
    diff_by = {d.get("frame"): d
               for d in (manifest.get("diff") or {}).get("per_frame", [])}

    lines = [f"# worklist — session `{manifest.get('session')}`", ""]
    if not marks:
        lines.append("_(no marks)_")
    for i, m in enumerate(sorted(marks, key=lambda m: m.get("frame", 0))):
        k = m.get("frame", 0)
        st = state.get(k, {})
        d = diff_by.get(k, {})
        seg = st.get("seg", "?")
        port_flip = (st.get("port") or {}).get("flip", "?")
        retail_flip = (st.get("retail") or {}).get("flip", "?")
        sim_tick = (st.get("retail") or {}).get("sim_tick")
        ctx = (f"seg `{seg}` · port flip {port_flip} · retail flip "
               f"{retail_flip}"
               + (f" (sim_tick {sim_tick})" if sim_tick is not None else "")
               + f" · differ_px {d.get('differ', '?')}"
               + (f" · gt8 {d.get('gt8')}" if "gt8" in d else ""))
        lines.append(f"- [ ] **{m.get('kind', 'note')}** @ frame {k} — {ctx}")
        if m.get("note"):
            lines.append(f"      note: {m['note']}")
        if m.get("box"):
            x0, y0, x1, y1 = m["box"]
            lines.append(f"      box: {x0},{y0},{x1},{y1} "
                         f"(port/retail/diff frames/frame_{k:05d}.png)")
        lines.append("")

    out = sess_dir / "worklist.md"
    out.write_text("\n".join(lines) + "\n")
    return {"ok": True, "n_marks": len(marks), "worklist": str(out)}
