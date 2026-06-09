#!/usr/bin/env python3
"""tools/trace_studio.py — launcher for the trace-studio package.

The logic lives in tools/trace_studio/ (a package; this same-named .py file
coexists on purpose — running the file by path works while `import
trace_studio` resolves to the package).  See docs/trace-studio.md for the
how-to and docs/plans/trace-studio.md for the architecture.

    nix develop --command python3 tools/trace_studio.py capture in-game-intro
    nix develop --command python3 tools/trace_studio.py serve --session <name>
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from trace_studio.cli import main                        # noqa: E402

if __name__ == "__main__":
    sys.exit(main())
