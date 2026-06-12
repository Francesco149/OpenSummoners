#!/usr/bin/env python3
"""tools/trace_studio2/osr.py — reader for the Trace Studio v2 ".osr" draw-stream.

The .osr binary format is the shared draw-stream codec captured by the retail
capture proxy (tools/capture_proxy) and the port emitter (src/osr_emit.c, M5);
its authoritative definition is src/osr_format.h.  THIS module mirrors that
header exactly — keep the two in lockstep.  It is the round-trip validator for
the writer (M3a) and the input to pairing / reconstruction (M4+).

A capture may be HARD-KILLED mid-record (the harness Stop-Processes the game),
so parsing stops cleanly at the last complete record and reports the torn tail
rather than raising.

Usage:
    python3 tools/trace_studio2/osr.py SUMMARY <file.osr>
    python3 tools/trace_studio2/osr.py FRAMES  <file.osr>   # one line per frame
"""
from __future__ import annotations

import struct
import sys
from dataclasses import dataclass, field

MAGIC = b"OSR1"
VERSION = 1
HEADER_SIZE = 64

# record types (mirror enum osr_rec_type in src/osr_format.h)
FRAMEBEG = 1
PRESENT = 2
ANCHOR = 3
SEED = 4
CLEAR = 5
BLIT = 6
TEXT = 7
SHEET = 8
FONT = 9
PALETTE = 10
INPUT = 11

REC_NAME = {
    FRAMEBEG: "FRAMEBEG", PRESENT: "PRESENT", ANCHOR: "ANCHOR", SEED: "SEED",
    CLEAR: "CLEAR", BLIT: "BLIT", TEXT: "TEXT", SHEET: "SHEET",
    FONT: "FONT", PALETTE: "PALETTE", INPUT: "INPUT",
}

SIDE_NAME = {0: "port", 1: "retail"}
PIXFMT_NAME = {0: "unknown", 1: "RGB565", 2: "XRGB8888", 3: "PAL8"}


@dataclass
class Header:
    side: int
    pixfmt: int
    screen_w: int
    screen_h: int
    seed: int
    flags: int
    scenario: str

    @property
    def turbo(self) -> bool:
        return bool(self.flags & 0x1)

    @property
    def lockstep(self) -> bool:
        return bool(self.flags & 0x2)

    @property
    def seed_pin(self) -> bool:
        return bool(self.flags & 0x4)


@dataclass
class Record:
    type: int
    payload: bytes

    # ── typed payload decoders (mirror osr_dec_* in src/osr_format.h) ──────
    def framebeg(self):
        flip, sim_tick, anchor_id = struct.unpack_from("<III", self.payload)
        return flip, sim_tick, anchor_id

    def present(self):
        mode, src_handle = struct.unpack_from("<II", self.payload)
        return mode, src_handle

    def seed(self):
        flip, before, value = struct.unpack_from("<III", self.payload)
        return flip, before, value

    def anchor(self):
        flip, sim_tick, rng, name_len = struct.unpack_from("<IIII", self.payload)
        name = self.payload[16:16 + name_len].decode("latin-1", "replace")
        return flip, sim_tick, rng, name


@dataclass
class Osr:
    header: Header
    records: list = field(default_factory=list)
    torn_tail: int = 0   # bytes of an incomplete trailing record (killed capture)

    def frames(self):
        """Yield (flip, sim_tick) per FRAMEBEG, in stream order."""
        for r in self.records:
            if r.type == FRAMEBEG:
                flip, tick, _ = r.framebeg()
                yield flip, tick

    def anchors(self):
        for r in self.records:
            if r.type == ANCHOR:
                yield r.anchor()

    def seeds(self):
        for r in self.records:
            if r.type == SEED:
                yield r.seed()


def parse(path: str) -> Osr:
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < HEADER_SIZE:
        raise ValueError(f"{path}: too short for a header ({len(data)} bytes)")
    if data[0:4] != MAGIC:
        raise ValueError(f"{path}: bad magic {data[0:4]!r} (want {MAGIC!r})")
    version = struct.unpack_from("<I", data, 4)[0]
    if version != VERSION:
        raise ValueError(f"{path}: version {version} (want {VERSION})")
    side, pixfmt = data[8], data[9]
    screen_w, screen_h = struct.unpack_from("<HH", data, 10)
    seed, flags = struct.unpack_from("<II", data, 16)
    scenario = data[24:64].split(b"\x00", 1)[0].decode("latin-1", "replace")
    hdr = Header(side, pixfmt, screen_w, screen_h, seed, flags, scenario)

    out = Osr(header=hdr)
    p, end = HEADER_SIZE, len(data)
    while end - p >= 8:
        rtype, rlen = struct.unpack_from("<II", data, p)
        if end - (p + 8) < rlen:          # torn trailing record (hard kill)
            out.torn_tail = end - p
            break
        out.records.append(Record(rtype, data[p + 8:p + 8 + rlen]))
        p += 8 + rlen
    else:
        if end - p:                       # 1..7 leftover bytes
            out.torn_tail = end - p
    return out


def cmd_summary(path: str) -> int:
    o = parse(path)
    h = o.header
    frames = list(o.frames())
    anchors = list(o.anchors())
    seeds = list(o.seeds())
    counts: dict[int, int] = {}
    for r in o.records:
        counts[r.type] = counts.get(r.type, 0) + 1

    print(f"file      : {path}")
    print(f"side      : {SIDE_NAME.get(h.side, h.side)}")
    print(f"screen    : {h.screen_w}x{h.screen_h} {PIXFMT_NAME.get(h.pixfmt, h.pixfmt)}")
    print(f"seed      : 0x{h.seed:x}")
    print(f"flags     : turbo={h.turbo} lockstep={h.lockstep} seed_pin={h.seed_pin}")
    print(f"scenario  : {h.scenario!r}")
    print(f"records   : {len(o.records)}  " +
          " ".join(f"{REC_NAME.get(t, t)}={c}" for t, c in sorted(counts.items())))
    if o.torn_tail:
        print(f"torn_tail : {o.torn_tail} bytes (capture was hard-killed; expected)")
    if frames:
        flips = [f for f, _ in frames]
        ticks = [t for _, t in frames]
        print(f"frames    : {len(frames)}  "
              f"flip {flips[0]}..{flips[-1]}  sim_tick {min(ticks)}..{max(ticks)}")
    print(f"anchors   : {len(anchors)}")
    for flip, tick, rng, name in anchors:
        print(f"    {name:<16} flip={flip} sim_tick={tick} rng=0x{rng:x}")
    print(f"seeds     : {len(seeds)}")
    for flip, before, value in seeds:
        print(f"    flip={flip} 0x{before:x} -> 0x{value:x}")
    return 0


def cmd_frames(path: str) -> int:
    o = parse(path)
    for flip, tick in o.frames():
        print(f"flip={flip}\tsim_tick={tick}")
    return 0


def main(argv) -> int:
    if len(argv) < 3 or argv[1] not in ("SUMMARY", "FRAMES"):
        print(__doc__)
        return 2
    if argv[1] == "SUMMARY":
        return cmd_summary(argv[2])
    return cmd_frames(argv[2])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
