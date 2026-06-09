"""server/app.py — the studio's local http server.

Serves the SPA (tools/trace_studio_web/) + each session's artifacts (HTTP Range
for the scrub mp4s) + the JSON API (routes.py).  Binds 0.0.0.0 by default so
the Windows-side browser reaches it through WSL2's localhost forwarding.
"""
from __future__ import annotations

import atexit
import re
import socketserver
from http.server import BaseHTTPRequestHandler
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from ..paths import DEFAULT_REMOTE
from . import ranged, routes
from .controller import CaptureController
from .jobs import JobsRegistry

_CTYPE = {
    ".html": "text/html; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".mjs": "application/javascript; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".jsonl": "application/x-ndjson; charset=utf-8",
    ".md": "text/plain; charset=utf-8",
    ".log": "text/plain; charset=utf-8",
    ".mp4": "video/mp4",
    ".png": "image/png",
}


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *a):           # quiet
        pass

    def _ctype(self, p: Path) -> str:
        return _CTYPE.get(p.suffix.lower(), "application/octet-stream")

    def _send_bytes(self, data: bytes, ctype: str, code: int = 200):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(data)

    def _send_json(self, obj, code: int = 200):
        import json
        self._send_bytes(json.dumps(obj).encode(), _CTYPE[".json"], code)

    def _send_file(self, p: Path):
        ranged.send_file(self, p, self._ctype(p))

    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        u = urlparse(self.path)
        path = u.path
        web_dir = self.server.web_dir
        if path in ("/", "/index.html"):
            qs = parse_qs(u.query)
            if "session" not in qs and self.server.default_session:
                self.send_response(302)
                self.send_header(
                    "Location", f"/?session={self.server.default_session}")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            self._send_file(web_dir / "index.html")
            return
        if "/s/" not in path and not path.startswith("/api"):
            rel = path.lstrip("/")
            if ".." not in rel:
                cand = (web_dir / rel).resolve()
                if cand.is_file() and str(cand).startswith(
                        str(web_dir.resolve())):
                    self._send_file(cand)
                    return
        for rx, fn in routes.GET_ROUTES:
            mm = re.match(rx, path)
            if mm:
                return fn(self, mm, b"")
        m = re.match(r"^/s/([^/]+)/(.+)$", path)
        if m:
            sess, rel = m.group(1), m.group(2)
            if ".." in rel:
                self._send_bytes(b"no", "text/plain", 403)
                return
            self._send_file(self.server.sess_root / sess / rel)
            return
        self._send_bytes(b"not found", "text/plain", 404)

    def do_POST(self):
        u = urlparse(self.path)
        length = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(length) if length else b""
        for rx, fn in routes.POST_ROUTES:
            m = re.match(rx, u.path)
            if m:
                return fn(self, m, raw)
        self._send_bytes(b"not found", "text/plain", 404)


def serve(sess_root: Path, web_dir: Path, host: str = "0.0.0.0",
          port: int = 8779, default_session: str | None = None,
          remote: str = DEFAULT_REMOTE) -> None:
    capturer = CaptureController(remote, sess_root)
    atexit.register(capturer.force_cleanup)

    class Server(socketserver.ThreadingMixIn, socketserver.TCPServer):
        allow_reuse_address = True
        daemon_threads = True

    httpd = Server((host, port), Handler)
    httpd.sess_root = sess_root
    httpd.web_dir = web_dir
    httpd.default_session = default_session
    httpd.capturer = capturer
    httpd.jobs = JobsRegistry(capturer)

    url = f"http://127.0.0.1:{port}/"
    if default_session:
        url += f"?session={default_session}"
    print(f"trace_studio: serving {sess_root} at {url} (bound {host})",
          flush=True)
    print("trace_studio: Ctrl-C to stop", flush=True)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\ntrace_studio: stopped", flush=True)
    finally:
        capturer.force_cleanup()
        httpd.shutdown()
