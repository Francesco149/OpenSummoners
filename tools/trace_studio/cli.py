"""cli.py — the trace_studio command surface.

    capture <scenario>   capture a scenario on both targets → a session
    recapture <session>  re-drive a session's WORKING traces (keeps marks)
    serve                the browser studio (scrub + mark + iterate)
    apply <session>      marks → worklist.md

Run inside `nix develop` (PIL/ffmpeg/frida are flake-provided).
"""
from __future__ import annotations

import argparse

from .paths import DEFAULT_REMOTE, ROOT, SESS_ROOT, STUDIO_PORT


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(prog="trace_studio.py", description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    cap = sub.add_parser("capture", help="capture a scenario on both targets")
    cap.add_argument("scenario", help="scenario name (tests/scenarios/<name>) "
                                      "or a dir with trace-{port,retail}.jsonl")
    cap.add_argument("--session", default=None)
    cap.add_argument("--port-frames", type=int, default=2600,
                     help="port Flips to run/capture (default 2600 — through "
                          "the town arrival + hold)")
    cap.add_argument("--retail-frames", type=int, default=3200,
                     help="retail Flips to capture through (default 3200)")
    cap.add_argument("--only", choices=("both", "port", "retail"),
                     default="both")
    cap.add_argument("--call-trace", action="store_true", default=False,
                     help="also capture the field-bearing flow trace on both "
                          "sides (slower; enables per-frame state fields + "
                          "flow_diff verdict)")
    cap.add_argument("--amp", type=float, default=6.0)
    cap.add_argument("--fps", type=int, default=30)
    cap.add_argument("--remote", default=DEFAULT_REMOTE)
    cap.add_argument("--reset-traces", action="store_true",
                     help="rebuild the session's working traces from the "
                          "scenario (discards studio-side trace edits)")

    rec = sub.add_parser("recapture", help="re-drive a session (keeps marks + "
                                           "working-trace edits)")
    rec.add_argument("session")
    rec.add_argument("--only", choices=("both", "port", "retail"),
                     default="both",
                     help="port = fast loop: re-run only the port against the "
                          "cached retail capture")
    rec.add_argument("--call-trace", action="store_true", default=None)
    rec.add_argument("--remote", default=DEFAULT_REMOTE)

    srv = sub.add_parser("serve", help="serve the browser studio")
    srv.add_argument("--session", default=None,
                     help="default session to open")
    srv.add_argument("--host", default="0.0.0.0")
    srv.add_argument("--port", type=int, default=STUDIO_PORT)
    srv.add_argument("--remote", default=DEFAULT_REMOTE)

    ap = sub.add_parser("apply", help="render marks → worklist.md")
    ap.add_argument("session")

    pr = sub.add_parser("pair", help="re-run pairing/diff/videos/verdict from "
                                     "the existing raw captures (no re-drive)")
    pr.add_argument("session")
    pr.add_argument("--amp", type=float, default=None)
    pr.add_argument("--drift-window", type=int, default=None)

    ls = sub.add_parser("sessions", help="list sessions")  # noqa: F841

    args = p.parse_args(argv)

    if args.cmd == "capture":
        from .capture import CaptureConfig, run_capture
        return run_capture(CaptureConfig(
            scenario=args.scenario, session=args.session,
            port_frames=args.port_frames, retail_frames=args.retail_frames,
            only=args.only, call_trace=args.call_trace, amp=args.amp,
            fps=args.fps, remote=args.remote, reset_traces=args.reset_traces))

    if args.cmd == "recapture":
        from .capture import CaptureConfig, run_capture
        from .model import session as sess_mod
        old = sess_mod.load_manifest(sess_mod.session_dir(args.session))
        ct = args.call_trace if args.call_trace is not None \
            else bool(old.get("call_trace"))
        return run_capture(CaptureConfig(
            scenario="", session=args.session, only=args.only,
            call_trace=ct, remote=args.remote,
            port_frames=int(old.get("port_frames", 2600)),
            retail_frames=int(old.get("retail_frames", 3200)),
            amp=float(old.get("amp", 6.0)), fps=int(old.get("fps", 30))))

    if args.cmd == "serve":
        from .server.app import serve
        serve(SESS_ROOT, ROOT / "tools" / "trace_studio_web",
              host=args.host, port=args.port,
              default_session=args.session, remote=args.remote)
        return 0

    if args.cmd == "apply":
        from .edits import apply as apply_mod
        from .model import session as sess_mod
        res = apply_mod.apply(sess_mod.session_dir(args.session))
        print(f"trace_studio: {res['n_marks']} mark(s) → {res['worklist']}")
        return 0

    if args.cmd == "pair":
        from .analysis import pairing
        from .capture import analyse_session
        from .model import session as sess_mod
        sdir = sess_mod.session_dir(args.session)
        old = sess_mod.load_manifest(sdir)
        if not old:
            raise SystemExit(f"trace_studio: no session {args.session!r}")
        return analyse_session(
            sdir, sess=args.session, scenario=old.get("scenario", ""),
            amp=args.amp if args.amp is not None else float(old.get("amp", 6.0)),
            fps=int(old.get("fps", 30)),
            call_trace=bool(old.get("call_trace")),
            drift_window=(args.drift_window if args.drift_window is not None
                          else int(old.get("drift_window",
                                           pairing.DRIFT_WINDOW))),
            port_frames=int(old.get("port_frames", 2600)),
            retail_frames=int(old.get("retail_frames", 3200)),
            only=old.get("only", "both"), drive_res=old.get("drive"))

    if args.cmd == "sessions":
        from .capture import list_sessions
        for s in list_sessions():
            print(f"{s['name']:48s} {s.get('n_frames') or 0:6d}f "
                  f"videos={','.join(s.get('videos') or [])}")
        return 0

    return 2
