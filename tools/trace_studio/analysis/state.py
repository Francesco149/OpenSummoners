"""analysis/state.py — state.jsonl: one row per viewer ordinal.

Base row (always available — derived from the pairing pass):
    {frame, seg, drift, port:{flip}, retail:{flip, sim_tick}}
    (+ anchor / port_rng / retail_rng on segment-start rows)

When the session was captured --call-trace, each side's per-flip flow-trace
FIELDS (CALL_TRACE_FIELD / retail_fields.json payloads) are merged into the
side dicts, keyed `<fnname>.<field>` — the StatePanel then highlights
port≠retail rows.  Field names collide across VAs, hence the prefix.
"""
from __future__ import annotations

import sys
from pathlib import Path

from ..paths import ROOT


def _flow_fields_by_flip(trace_path: Path) -> dict[int, dict]:
    """call_trace.jsonl → flip → {"name.field": value} (last-write-wins within
    a flip).  Tolerant: returns {} when the trace is missing/unreadable."""
    if not Path(trace_path).is_file():
        return {}
    if str(ROOT / "tools") not in sys.path:
        sys.path.insert(0, str(ROOT / "tools"))
    try:
        import json as _json
        from flow_diff import load_names, load_trace
        spec_p = ROOT / "tools" / "flow" / "retail_fields.json"
        spec = _json.loads(spec_p.read_text()) if spec_p.exists() else {}
        names = load_names(spec, ROOT / "docs" / "decompiled" / "functions.csv")
        by_frame = load_trace(Path(trace_path))
    except Exception:                                    # noqa: BLE001
        return {}
    out: dict[int, dict] = {}
    for fr, evts in by_frame.items():
        merged: dict = {}
        for e in evts:
            f = e.get("f")
            if isinstance(f, dict) and f:
                label = names.get(int(e["va"]), f"0x{int(e['va']):x}")
                for k, v in f.items():
                    merged[f"{label}.{k}"] = v
        if merged:
            out[int(fr)] = merged
    return out


def build_state(sess_dir: Path, pairs: list[dict],
                call_trace: bool = False) -> list[dict]:
    sess_dir = Path(sess_dir)
    pf = _flow_fields_by_flip(sess_dir / "port" / "call_trace.jsonl") \
        if call_trace else {}
    rf = _flow_fields_by_flip(sess_dir / "retail" / "cap" / "call_trace.jsonl") \
        if call_trace else {}

    rows: list[dict] = []
    for p in pairs:
        port = {"flip": p["port_flip"], **pf.get(p["port_flip"], {})}
        retail = {"flip": p["retail_flip"], **rf.get(p["retail_flip"], {})}
        if p.get("sim_tick", -1) >= 0:
            retail["sim_tick"] = p["sim_tick"]
        row = {"frame": p["frame"], "seg": p["seg"], "drift": p["drift"],
               "port": port, "retail": retail}
        for k in ("anchor", "port_rng", "retail_rng"):
            if k in p:
                row[k] = p[k]
        rows.append(row)
    return rows
