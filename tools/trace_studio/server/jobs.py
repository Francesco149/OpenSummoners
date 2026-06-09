"""server/jobs.py — the unified job view (GET /api/jobs).

One slot here (capture/recapture — the studio runs at most one).  Shape matches
openrecet's so the JobTray component transfers unchanged:
    {"jobs": [{id, kind, running, label, elapsed_s, rc, detail, session}],
     "running": bool}
"""
from __future__ import annotations


class JobsRegistry:
    def __init__(self, capturer):
        self.capturer = capturer

    def list(self) -> dict:
        c = self.capturer.status()
        jobs = [{
            "id": "capture",
            "kind": getattr(self.capturer, "kind", None) or "capture",
            "running": bool(c.get("running")),
            "label": c.get("session"),
            "elapsed_s": c.get("elapsed_s", 0),
            "rc": c.get("last_rc"),
            "detail": c.get("log_tail", ""),
            "session": c.get("session"),
        }]
        return {"jobs": jobs, "running": any(j["running"] for j in jobs)}
