#!/usr/bin/env python3
"""
no_proprietary_bytes.py — guard that our built binaries ship ZERO embedded
game data.

OpenSummoners must redistribute no Fortune Summoners bytes: the port
(opensummoners.exe), the voice patch (version.dll) and the resource explorer
(res_explorer.exe) are all pure code — they read the user's own game files at
runtime, never linking assets in. This is the automated backstop: if audio
ever gets embedded, its signature reappears and CI fails before publishing.

It scans each file for:
  - a real WAV:  "RIFF"<4-byte size>"WAVE""fmt "   — the SE / voice clips.
  - the ASF/WMA GUID (30 26 B2 75 8E 66 CF 11 …)   — the BGM streams.

The full "RIFF…WAVE…fmt " signature (not a bare "RIFF") is required on purpose:
res_core.cpp legitimately contains the ASCII literals "RIFF"/"WAVE"/"fmt " as
WAV-header parsing constants, which must NOT trip the gate. Only an actual
embedded WAV lays those tokens out at +0/+8/+12.

Usage:  tools/ci/no_proprietary_bytes.py <file> [<file> ...]
Exit:   0 = clean, 1 = proprietary bytes found, 2 = usage/IO error.
"""

from __future__ import annotations
import sys
from pathlib import Path

ASF_GUID = bytes.fromhex("3026b2758e66cf11a6d900aa0062ce6c")  # WMA/ASF header
SIZE_WARN_BYTES = 8_000_000  # advisory only — the port grows over time


def count_wavs(data: bytes) -> int:
    n = 0
    i = data.find(b"RIFF")
    while i >= 0:
        if data[i + 8:i + 12] == b"WAVE" and data[i + 12:i + 16] == b"fmt ":
            n += 1
        i = data.find(b"RIFF", i + 1)
    return n


def scan(path: Path) -> int:
    try:
        data = path.read_bytes()
    except OSError as e:
        print(f"error: cannot read {path}: {e}", file=sys.stderr)
        return 2

    wavs = count_wavs(data)
    asf = data.count(ASF_GUID)
    print(f"{path}: {len(data):,} bytes, WAV={wavs}, ASF/WMA={asf}")

    if len(data) > SIZE_WARN_BYTES:
        print(f"  warning: larger than {SIZE_WARN_BYTES:,} bytes — sanity-check "
              f"that no asset crept in", file=sys.stderr)

    if wavs or asf:
        print(f"  FAIL: embedded game audio detected (WAV={wavs}, ASF={asf}). "
              f"Assets must be read from the user's own files at runtime, never "
              f"linked into the binary.", file=sys.stderr)
        return 1

    print("  ok: no embedded game audio")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("usage: no_proprietary_bytes.py <file> [<file> ...]", file=sys.stderr)
        return 2
    worst = 0
    for arg in argv[1:]:
        worst = max(worst, scan(Path(arg)))
    return worst


if __name__ == "__main__":
    sys.exit(main(sys.argv))
