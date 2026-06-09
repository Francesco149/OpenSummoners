"""capture.py — the capture orchestrator.

resolve scenario → working traces → drive port+retail concurrently → pair
(anchor-segmented, sticky-drift) → state.jsonl + diff + all-intra videos +
verdict → session.json.  Re-capture re-uses the session's working traces (the
studio loop: edit/fix → re-capture → re-check); `--only port` re-runs just the
port against the cached retail capture (the fast port-fix loop).
"""
from __future__ import annotations

import datetime as dt
import json
import shutil
from dataclasses import dataclass
from pathlib import Path

from .analysis import pairing, state as state_mod, verdict as verdict_mod
from .drive import runner
from .edits import apply as apply_mod
from .model import session as sess_mod
from .paths import DEFAULT_REMOTE, SESS_ROOT
from .transport import encode


def _log(msg: str) -> None:
    print(f"trace_studio: {msg}", flush=True)


@dataclass
class CaptureConfig:
    scenario: str                       # scenario name/dir (capture) or "" (recapture)
    session: str | None = None
    port_frames: int = 2600
    retail_frames: int = 3200
    amp: float = 6.0
    fps: int = encode.VIDEO_FPS
    only: str = "both"                  # both | port | retail
    call_trace: bool = False
    remote: str = DEFAULT_REMOTE
    reset_traces: bool = False          # rebuild working traces from the scenario
    drift_window: int = pairing.DRIFT_WINDOW


def _read_anchor_file(p: Path) -> list[dict]:
    out: list[dict] = []
    if p.is_file():
        for ln in p.read_text().splitlines():
            s = ln.strip()
            if s:
                try:
                    out.append(json.loads(s))
                except json.JSONDecodeError:
                    pass
    return out


def run_capture(cfg: CaptureConfig) -> int:
    # ── session + scenario resolution ────────────────────────────────────────
    if cfg.session and not cfg.scenario:
        # recapture path: scenario comes from the existing manifest
        sess_dir = sess_mod.session_dir(cfg.session)
        old = sess_mod.load_manifest(sess_dir)
        if not old:
            raise SystemExit(f"trace_studio: no session {cfg.session!r} to "
                             f"re-capture")
        scenario_dir = Path(old["scenario"])
        sess = cfg.session
    else:
        scenario_dir = sess_mod.resolve_scenario(cfg.scenario)
        sess = cfg.session or (
            f"{scenario_dir.name}-{dt.datetime.now():%Y%m%d-%H%M%S}")
        sess_dir = sess_mod.session_dir(sess)
    sess_dir.mkdir(parents=True, exist_ok=True)
    old = sess_mod.load_manifest(sess_dir)

    traces = sess_mod.ensure_working_traces(sess_dir, scenario_dir,
                                            reset=cfg.reset_traces)
    # frame budgets: explicit cfg wins; else what the session used before
    port_frames = cfg.port_frames or int(old.get("port_frames", 2600))
    retail_frames = cfg.retail_frames or int(old.get("retail_frames", 3200))

    run_port = cfg.only in ("both", "port")
    run_retail = cfg.only in ("both", "retail")
    has_cached_retail = any(
        (sess_dir / "retail" / "cap" / "frames").glob("frame_*.png")) \
        if (sess_dir / "retail" / "cap" / "frames").is_dir() else False
    if not run_retail and not has_cached_retail:
        raise SystemExit("trace_studio: --only port needs a cached retail "
                         "capture in this session (run a full capture first)")

    _log(f"session {sess}  scenario {scenario_dir.name}  only={cfg.only}  "
         f"port_frames={port_frames} retail_frames={retail_frames}  "
         f"call_trace={cfg.call_trace}")

    # stale paired outputs always rebuild
    for sub in ("port/frames", "retail/frames", "diff/frames"):
        shutil.rmtree(sess_dir / sub, ignore_errors=True)
    for f in ("state.jsonl", "diff.mp4"):
        (sess_dir / f).unlink(missing_ok=True)
    if run_port:
        (sess_dir / "port.mp4").unlink(missing_ok=True)
    if run_retail:
        (sess_dir / "retail.mp4").unlink(missing_ok=True)

    # ── drive ────────────────────────────────────────────────────────────────
    res = runner.drive_both(
        sess_dir, traces["port"], traces["retail"],
        port_frames, retail_frames,
        run_port=run_port, run_retail=run_retail,
        remote=cfg.remote, call_trace=cfg.call_trace)
    for side, r in res.items():
        _log(f"{side}: rc={r.get('rc')} frames={r.get('n_frames')} "
             f"anchors={[a['name'] for a in r.get('anchors', [])]}"
             + (f" ERROR {r['error']}" if "error" in r else ""))

    rc = analyse_session(
        sess_dir, sess=sess, scenario=str(scenario_dir), amp=cfg.amp,
        fps=cfg.fps, call_trace=cfg.call_trace, drift_window=cfg.drift_window,
        port_frames=port_frames, retail_frames=retail_frames, only=cfg.only,
        drive_res=res, run_retail=run_retail)
    print(f"\nview it:  nix develop --command python3 tools/trace_studio.py "
          f"serve --session {sess}\n")
    return rc


def _retail_seed_pinned(sess_dir: Path) -> bool | None:
    """Whether retail's boot seed pin (first title sparkle) fired this run.
    None when undeterminable (no run.json)."""
    rj = Path(sess_dir) / "retail" / "cap" / "run.json"
    if not rj.is_file():
        return None
    try:
        summ = json.loads(rj.read_text()).get("summary", {})
    except json.JSONDecodeError:
        return None
    return "seed_pinned" in summ


def analyse_session(sess_dir: Path, *, sess: str, scenario: str, amp: float,
                    fps: int, call_trace: bool, drift_window: int,
                    port_frames: int, retail_frames: int, only: str,
                    drive_res: dict | None = None,
                    run_retail: bool = True) -> int:
    """The pair→state→videos→verdict→manifest phase — runs after a drive, or
    standalone (the `pair` subcommand) to re-analyse existing raw captures."""
    sess_dir = Path(sess_dir)
    res = drive_res or {}
    port_anchors = _read_anchor_file(sess_dir / "anchors.port.jsonl")
    retail_anchors = _read_anchor_file(sess_dir / "anchors.retail.jsonl")

    _log(f"pairing (anchor-segmented, sticky drift ±{drift_window})…")
    paired = pairing.pair_session(sess_dir, port_anchors, retail_anchors,
                                  amp=amp, drift_window=drift_window)
    pairs, segments = paired["pairs"], paired["segments"]
    _log(f"paired {paired['n']} frames over {len(segments)} segment(s)"
         + (f", {paired['gaps']} gap(s)" if paired.get("gaps") else ""))

    state_rows = state_mod.build_state(sess_dir, pairs, call_trace=call_trace)
    (sess_dir / "state.jsonl").write_text(
        "".join(json.dumps(r) + "\n" for r in state_rows))

    manifest: dict = {
        "schema": sess_mod.SCHEMA,
        "session": sess,
        "scenario": scenario,
        "target": "both",
        "only": only,
        "port_frames": port_frames,
        "retail_frames": retail_frames,
        "fps": fps,
        "amp": amp,
        "drift_window": drift_window,
        "call_trace": bool(call_trace),
        "n_frames": paired["n"],
        "frame_range": [0, max(0, paired["n"] - 1)],
        "state": "state.jsonl",
        "working_traces": {s: f"edit.trace.{s}.jsonl" for s in ("port", "retail")},
        "segments": segments,
        "anchors_ordinals": [
            {"name": p["anchor"], "ordinal": p["frame"],
             "port_flip": p["port_flip"], "retail_flip": p["retail_flip"]}
            for p in pairs if "anchor" in p],
        "diff": {"n": len(pairs),
                 "per_frame": [{"frame": p["frame"], "differ": p["differ"],
                                "gt8": p["gt8"], "meanabs": p["meanabs"]}
                               for p in pairs]},
        "videos": {},
        "stale": False,
        "drive": {s: {k: v for k, v in r.items() if k != "anchors"}
                  for s, r in res.items()},
    }

    # ── encode (always rebuilt — the pairing renames ordinals) ──────────────
    for panel in ("port", "retail", "diff"):
        out = sess_dir / f"{panel}.mp4"
        out.unlink(missing_ok=True)
        if encode.ffmpeg_encode(sess_dir / panel / "frames", out, fps=fps):
            manifest["videos"][panel] = f"{panel}.mp4"

    # ── verdict ──────────────────────────────────────────────────────────────
    v = verdict_mod.anchor_rng_verdict(segments, pairs,
                                       retail_seed_pinned=_retail_seed_pinned(sess_dir))
    if call_trace:
        fv = verdict_mod.flow_verdict(sess_dir)
        if fv:
            v["text"] += "\n\n── flow_diff ──\n" + fv["text"][-4000:]
            v["flow_exit_code"] = fv["exit_code"]
    manifest["verdict"] = v

    if paired["n"] == 0:
        errs = []
        if res.get("port", {}).get("n_frames", 1) == 0:
            errs.append("port captured 0 frames")
        if run_retail and res.get("retail", {}).get("n_frames", 1) == 0:
            errs.append("retail captured 0 frames")
        manifest["capture_error"] = ("; ".join(errs) or "pairing produced 0 "
                                     "frames") + " — see port.log/retail.log"
        _log("CAPTURE ERROR: " + manifest["capture_error"])

    sess_mod.write_manifest(sess_dir, manifest)
    # refresh the worklist against the new frame numbering if marks exist
    if (sess_dir / "edits.jsonl").is_file():
        try:
            apply_mod.apply(sess_dir)
        except Exception:                                # noqa: BLE001
            pass
    _log(f"session.json written → {sess_dir}")
    _log(f"DONE: {paired['n']} frames, videos={list(manifest['videos'])}")
    return 0 if paired["n"] else 1


def list_sessions() -> list[dict]:
    out = []
    if SESS_ROOT.is_dir():
        for d in sorted(SESS_ROOT.iterdir()):
            m = sess_mod.load_manifest(d)
            if m:
                out.append({"name": d.name, "n_frames": m.get("n_frames"),
                            "target": m.get("target"),
                            "videos": list(m.get("videos") or {}),
                            "call_trace": m.get("call_trace")})
    return out
