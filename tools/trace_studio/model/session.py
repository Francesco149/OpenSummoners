"""model/session.py — the session manifest + scenario resolution.

A SESSION is one captured comparison of a scenario on both targets, everything
under runs/trace-studio/<name>/.  The manifest (session.json) records what was
captured and where the sidecars live; the SPA consumes it directly.

A SCENARIO is a committed dir under tests/scenarios/<name>/ carrying the two
per-side input traces:
    trace-port.jsonl     (port-timed flat input trace)
    trace-retail.jsonl   (retail-timed flat input trace)
The session takes EDITABLE WORKING COPIES (edit.trace.port.jsonl /
edit.trace.retail.jsonl) at creation; re-captures drive the working copies, so
studio-side trace edits never touch the committed scenario.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

from ..paths import SCENARIO_ROOT, SESS_ROOT

SCHEMA = "oss-trace-studio-v1"


def resolve_scenario(name_or_path: str) -> Path:
    """Scenario name or path → the scenario dir (must carry both side traces)."""
    p = Path(name_or_path)
    cand = p if p.is_dir() else SCENARIO_ROOT / name_or_path
    if not cand.is_dir():
        raise SystemExit(f"trace_studio: no scenario dir {name_or_path!r} "
                         f"(looked at {cand})")
    missing = [f for f in ("trace-port.jsonl", "trace-retail.jsonl")
               if not (cand / f).is_file()]
    if missing:
        raise SystemExit(f"trace_studio: scenario {cand} is missing {missing} "
                         f"(the studio needs both per-side traces)")
    return cand


def safe_name(s: str) -> str:
    return re.sub(r"[^\w.-]", "_", s)


def session_dir(name: str) -> Path:
    return SESS_ROOT / safe_name(name)


def load_manifest(sess_dir: Path) -> dict:
    mf = Path(sess_dir) / "session.json"
    if not mf.is_file():
        return {}
    try:
        return json.loads(mf.read_text())
    except json.JSONDecodeError:
        return {}


def write_manifest(sess_dir: Path, manifest: dict) -> None:
    (Path(sess_dir) / "session.json").write_text(json.dumps(manifest, indent=2))


def working_trace(sess_dir: Path, side: str) -> Path:
    assert side in ("port", "retail")
    return Path(sess_dir) / f"edit.trace.{side}.jsonl"


def ensure_working_traces(sess_dir: Path, scenario_dir: Path,
                          reset: bool = False) -> dict[str, Path]:
    """Copy the scenario's per-side traces into the session as editable working
    copies (idempotent — an existing working copy is kept unless reset)."""
    out: dict[str, Path] = {}
    for side in ("port", "retail"):
        dst = working_trace(sess_dir, side)
        if reset or not dst.is_file():
            dst.write_text((scenario_dir / f"trace-{side}.jsonl").read_text())
        out[side] = dst
    return out


def validate_trace_text(text: str) -> str | None:
    """Validate flat input-trace JSONL text (the studio's POST /trace body).
    Returns None when valid, else a one-line error.  Mirrors the port parser's
    rules (src/input_trace.c): comments/blank lines ok, each entry
    {"frame": N, "ids": [...]}, frames non-decreasing."""
    last = -1
    for i, ln in enumerate(text.splitlines(), 1):
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        try:
            o = json.loads(s)
        except json.JSONDecodeError as e:
            return f"line {i}: bad JSON ({e.msg})"
        if not isinstance(o, dict) or "frame" not in o or "ids" not in o:
            return f"line {i}: entry needs frame+ids"
        try:
            fr = int(o["frame"])
        except (TypeError, ValueError):
            return f"line {i}: frame must be an int"
        if not isinstance(o["ids"], list):
            return f"line {i}: ids must be a list"
        if fr < last:
            return f"line {i}: frames must be non-decreasing ({fr} < {last})"
        last = fr
    return None
