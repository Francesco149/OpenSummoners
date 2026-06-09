"""paths.py — shared path/constant resolution for the studio."""
from __future__ import annotations

import os
import subprocess
from pathlib import Path

# repo root (tools/trace_studio/paths.py → repo)
ROOT = Path(__file__).resolve().parent.parent.parent

# sessions live under runs/ (gitignored)
SESS_ROOT = ROOT / "runs" / "trace-studio"

# committed scenarios (per-side input traces)
SCENARIO_ROOT = ROOT / "tests" / "scenarios"

# Windows-writable staging for the PORT's BMP dump.  The port exe is a Windows
# binary: it cannot fopen() a WSL path (/tmp/…), so --capture-dir must resolve
# under a real drive (the ckpt-95 footgun).  Per-session subdirs are created on
# demand and cleaned after the BMPs are moved into the session.
WIN_STAGE = Path("/mnt/c/Users/headpats/AppData/Local/Temp/oss-trace-studio")

# default Frida remote (mirrors tools/frida_capture.py's chain)
DEFAULT_REMOTE = (os.environ.get("OPENSUMMONERS_FRIDA_REMOTE")
                  or os.environ.get("OPENRECET_FRIDA_REMOTE")
                  or "cutestation.soy:27042")

STUDIO_PORT = 8779           # http port for `serve` (feed=8777, openrecet=8778)


def win_path(p: Path) -> str:
    """WSL path → a Windows path the port exe can fopen.  Forward slashes on
    purpose: the C runtime accepts them and they survive the port's ad-hoc
    space-split argv tokenizer + the bash → launcher quoting chain."""
    try:
        out = subprocess.run(["wslpath", "-m", str(p)], capture_output=True,
                             text=True, check=True).stdout.strip()
        if out:
            return out
    except Exception:                                    # noqa: BLE001
        pass
    s = str(p)
    if s.startswith("/mnt/") and len(s) > 6:
        return f"{s[5].upper()}:{s[6:]}" if s[6] == "/" else f"{s[5].upper()}:/"
    return s
