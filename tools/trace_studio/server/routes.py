"""server/routes.py — the POST/GET dispatch tables.

Each handler is `(h, match, raw) -> None`; `h` is the request handler (response
helpers `_send_json`/`_send_bytes`/`_send_file`), shared state on `h.server`
(sess_root / capturer / jobs).  Wire API mirrors openrecet's studio where the
concepts transfer, so the SPA components port with minimal edits.
"""
from __future__ import annotations

import json
import re
import shutil
import time
from pathlib import Path


def _json(raw: bytes) -> dict:
    try:
        return json.loads(raw or b"{}")
    except json.JSONDecodeError:
        return {}


# ── GET ──────────────────────────────────────────────────────────────────────
def h_sessions(h, m, raw):
    from ..capture import list_sessions
    h._send_json(list_sessions())


def h_jobs(h, m, raw):
    h._send_json(h.server.jobs.list())


def h_registries(h, m, raw):
    from ..edits import marks
    h._send_json({"marks": marks.registry(), "analyzers": []})


# ── POST: capture / recapture / cancel ───────────────────────────────────────
def h_capture(h, m, raw):
    d = _json(raw)
    scenario = d.get("scenario")
    if not scenario:
        h._send_bytes(b"need scenario", "text/plain", 400)
        return
    session = d.get("session") or (
        f"{re.sub(r'[^\w.-]', '_', Path(scenario).name)}-"
        f"{time.strftime('%Y%m%d-%H%M%S')}")
    args = [scenario, "--session", session]
    for k, flag in (("port_frames", "--port-frames"),
                    ("retail_frames", "--retail-frames")):
        if d.get(k):
            args += [flag, str(int(d[k]))]
    if d.get("call_trace"):
        args.append("--call-trace")
    h._send_json(h.server.capturer.start("capture", args, session))


def h_recapture(h, m, raw):
    sess = m.group(1)
    sdir = h.server.sess_root / sess
    if not (sdir / "session.json").is_file():
        h._send_bytes(b"no session", "text/plain", 404)
        return
    d = _json(raw)
    args = [sess]
    only = d.get("only", "both")
    if only and only != "both":
        args += ["--only", only]
    h._send_json(h.server.capturer.start("recapture", args, sess))


def h_capture_cancel(h, m, raw):
    h._send_json(h.server.capturer.cancel())


# ── POST: apply (marks → worklist) ───────────────────────────────────────────
def h_apply(h, m, raw):
    sess = m.group(1)
    sdir = h.server.sess_root / sess
    if not (sdir / "session.json").is_file():
        h._send_bytes(b"no session", "text/plain", 404)
        return
    try:
        from ..edits import apply as edits_apply
        h._send_json(edits_apply.apply(sdir))
    except Exception as e:                               # noqa: BLE001
        h._send_json({"ok": False, "error": repr(e)}, 500)


# ── POST: working-trace edit ─────────────────────────────────────────────────
def h_trace(h, m, raw):
    sess = m.group(1)
    sdir = h.server.sess_root / sess
    if not (sdir / "session.json").is_file():
        h._send_bytes(b"no session", "text/plain", 404)
        return
    d = _json(raw)
    side = d.get("side")
    text = d.get("text")
    if side not in ("port", "retail") or not isinstance(text, str):
        h._send_bytes(b"need side(port|retail)+text", "text/plain", 400)
        return
    from ..model.session import validate_trace_text
    err = validate_trace_text(text)
    if err:
        h._send_json({"ok": False, "error": err})
        return
    (sdir / f"edit.trace.{side}.jsonl").write_text(text)
    try:
        man = json.loads((sdir / "session.json").read_text())
        man["stale"] = True
        (sdir / "session.json").write_text(json.dumps(man, indent=2))
    except Exception:                                    # noqa: BLE001
        pass
    h._send_json({"ok": True, "side": side,
                  "n_lines": len(text.splitlines())})


# ── POST: marks ──────────────────────────────────────────────────────────────
def h_edits_set(h, m, raw):
    sess = m.group(1)
    sdir = h.server.sess_root / sess
    if not sdir.is_dir():
        h._send_bytes(b"no session", "text/plain", 404)
        return
    d = _json(raw)
    edits = d.get("edits", [])
    if not isinstance(edits, list):
        h._send_bytes(b"edits must be a list", "text/plain", 400)
        return
    (sdir / "edits.jsonl").write_text(
        "".join(json.dumps(e) + "\n" for e in edits))
    h._send_json({"ok": True, "n": len(edits)})


def h_edits_append(h, m, raw):
    sess = m.group(1)
    sdir = h.server.sess_root / sess
    if not sdir.is_dir():
        h._send_bytes(b"no session", "text/plain", 404)
        return
    try:
        edit = json.loads(raw or b"{}")
    except json.JSONDecodeError:
        h._send_bytes(b"bad json", "text/plain", 400)
        return
    if "frame" not in edit or "kind" not in edit:
        h._send_bytes(b"need frame+kind", "text/plain", 400)
        return
    with (sdir / "edits.jsonl").open("a") as f:
        f.write(json.dumps(edit) + "\n")
    h._send_json({"ok": True, "edit": edit})


def h_clone(h, m, raw):
    sess = m.group(1)
    sdir = h.server.sess_root / sess
    if not sdir.is_dir():
        h._send_bytes(b"no session", "text/plain", 404)
        return
    raw_name = _json(raw).get("name", "")
    name = re.sub(r"[^\w.-]", "_", raw_name) or (sess + "-copy")
    dst = h.server.sess_root / name
    if dst.exists():
        h._send_json({"ok": False, "error": f"{name} already exists"})
        return
    shutil.copytree(sdir, dst)
    try:
        m2 = json.loads((dst / "session.json").read_text())
        m2["session"] = name
        m2["cloned_from"] = sess
        (dst / "session.json").write_text(json.dumps(m2, indent=2))
    except Exception:                                    # noqa: BLE001
        pass
    h._send_json({"ok": True, "name": name})


POST_ROUTES = [
    (r"^/capture$",               h_capture),
    (r"^/capture/cancel$",        h_capture_cancel),
    (r"^/s/([^/]+)/recapture$",   h_recapture),
    (r"^/s/([^/]+)/apply$",       h_apply),
    (r"^/s/([^/]+)/trace$",       h_trace),
    (r"^/s/([^/]+)/clone$",       h_clone),
    (r"^/s/([^/]+)/edits/set$",   h_edits_set),
    (r"^/s/([^/]+)/edits$",       h_edits_append),
]

GET_ROUTES = [
    (r"^/api/sessions$",    h_sessions),
    (r"^/api/jobs$",        h_jobs),
    (r"^/api/registries$",  h_registries),
]
