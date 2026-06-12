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
    python3 tools/trace_studio2/osr.py FRAMES  <file.osr>          # one line per frame
    python3 tools/trace_studio2/osr.py BLITS   <file.osr> [flip]   # one frame's draw list
    python3 tools/trace_studio2/osr.py SHEETS  <file.osr>          # dedup'd source sheets
    python3 tools/trace_studio2/osr.py TEXTS   <file.osr> [flip]   # FONT table + GDI text
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

    def blit(self):
        # mirror struct osr_blit / osr_dec_blit in src/osr_format.h (76 bytes)
        (va, seq, res, frame, dhash, dst_handle,
         dx, dy, reqw, reqh, sx, sy, ow, oh, ox, oy,
         state, ckey, bmode, mode) = struct.unpack_from(
            "<IIHHII iiiiii iiii IIiI", self.payload)
        return Blit(va, seq, res, frame, dhash, dst_handle,
                    dx, dy, reqw, reqh, sx, sy, ow, oh, ox, oy,
                    state, ckey, bmode, mode)

    def sheet(self):
        # mirror struct osr_sheet / osr_dec_sheet (24-byte prefix + bytes)
        (dhash, res, frame, w, h, pitch, pixfmt, codec,
         _rsv, byte_len) = struct.unpack_from("<I HH HH I BB H I", self.payload)
        pix = self.payload[24:24 + byte_len]
        return Sheet(dhash, res, frame, w, h, pitch, pixfmt, codec, byte_len, pix)

    def font(self):
        # mirror struct osr_font / osr_dec_font (fixed 64 bytes)
        (font_ref, height, width, escapement, orientation, weight,
         italic, underline, strikeout, charset,
         out_prec, clip_prec, quality, pitch_family) = struct.unpack_from(
            "<I iiiii BBBB BBBB", self.payload)
        face = self.payload[32:64].split(b"\x00", 1)[0].decode("latin-1", "replace")
        return Font(font_ref, height, width, escapement, orientation, weight,
                    italic, underline, strikeout, charset,
                    out_prec, clip_prec, quality, pitch_family, face)

    def text(self):
        # mirror struct osr_text / osr_dec_text (32-byte prefix + str bytes)
        (seq, dst_handle, x, y, font_ref, color, bk_mode,
         str_len) = struct.unpack_from("<II ii II i I", self.payload)
        s = self.payload[32:32 + str_len].decode("latin-1", "replace")
        return Text(seq, dst_handle, x, y, font_ref, color, bk_mode, str_len, s)


@dataclass
class Blit:
    va: int
    seq: int
    res: int
    frame: int
    dhash: int
    dst_handle: int
    dx: int
    dy: int
    reqw: int
    reqh: int
    sx: int
    sy: int
    ow: int
    oh: int
    ox: int
    oy: int
    state: int
    ckey: int
    bmode: int
    mode: int


@dataclass
class Sheet:
    dhash: int
    res: int
    frame: int
    w: int
    h: int
    pitch: int
    pixfmt: int
    codec: int
    byte_len: int
    pixels: bytes = b""


@dataclass
class Font:
    font_ref: int
    height: int
    width: int
    escapement: int
    orientation: int
    weight: int
    italic: int
    underline: int
    strikeout: int
    charset: int
    out_prec: int
    clip_prec: int
    quality: int
    pitch_family: int
    face: str


@dataclass
class Text:
    seq: int
    dst_handle: int
    x: int
    y: int
    font_ref: int
    color: int
    bk_mode: int
    str_len: int
    text: str


# the 5 source-bearing blit primitives (mirror zdd.c / engine_hooks.h)
BLIT_VA_NAME = {
    0x5b9a40: "onto", 0x5b9b70: "keyed", 0x5b9ae0: "rects",
    0x5b9bf0: "clipped", 0x5bd550: "alpha",
}


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

    def frames_with_blits(self):
        """Yield (flip, sim_tick, [Blit, ...]) per frame — the BLIT records
        between a FRAMEBEG and the next PRESENT/FRAMEBEG belong to that frame."""
        cur = None
        for r in self.records:
            if r.type == FRAMEBEG:
                if cur is not None:
                    yield cur
                flip, tick, _ = r.framebeg()
                cur = (flip, tick, [])
            elif r.type == BLIT and cur is not None:
                cur[2].append(r.blit())
        if cur is not None:
            yield cur

    def blits(self):
        for r in self.records:
            if r.type == BLIT:
                yield r.blit()

    def anchors(self):
        for r in self.records:
            if r.type == ANCHOR:
                yield r.anchor()

    def seeds(self):
        for r in self.records:
            if r.type == SEED:
                yield r.seed()

    def sheets(self):
        for r in self.records:
            if r.type == SHEET:
                yield r.sheet()

    def fonts(self):
        for r in self.records:
            if r.type == FONT:
                yield r.font()

    def texts(self):
        for r in self.records:
            if r.type == TEXT:
                yield r.text()

    def frames_with_texts(self):
        """Yield (flip, sim_tick, [Text, ...]) per frame — the TEXT records
        between a FRAMEBEG and the next FRAMEBEG belong to that frame."""
        cur = None
        for r in self.records:
            if r.type == FRAMEBEG:
                if cur is not None:
                    yield cur
                flip, tick, _ = r.framebeg()
                cur = (flip, tick, [])
            elif r.type == TEXT and cur is not None:
                cur[2].append(r.text())
        if cur is not None:
            yield cur


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
    nblit = counts.get(BLIT, 0)
    if nblit:
        all_blits = list(o.blits())           # decode once; several stats below
        fb = [(fl, len(bl)) for fl, _, bl in o.frames_with_blits() if bl]
        named = sum(1 for b in all_blits if b.res)
        per_va: dict[int, int] = {}
        for b in all_blits:
            per_va[b.va] = per_va.get(b.va, 0) + 1
        print(f"blits     : {nblit}  named(res!=0)={named} "
              f"({100*named//nblit if nblit else 0}%)  "
              + " ".join(f"{BLIT_VA_NAME.get(v, hex(v))}={c}"
                         for v, c in sorted(per_va.items())))
        # M3c coverage: how many blits carry a dest handle / a source dhash
        dst_ok = sum(1 for b in all_blits if b.dst_handle)
        dh_ok = sum(1 for b in all_blits if b.dhash)
        dst_handles = {b.dst_handle for b in all_blits if b.dst_handle}
        print(f"            dst_handle set={dst_ok} ({100*dst_ok//nblit}%) "
              f"distinct={len(dst_handles)}  "
              f"src dhash set={dh_ok} ({100*dh_ok//nblit}%)")
        if fb:
            cnts = [c for _, c in fb]
            busiest = max(fb, key=lambda x: x[1])
            print(f"            {len(fb)} frames w/ blits, "
                  f"{min(cnts)}..{max(cnts)} per frame "
                  f"(busiest flip={busiest[0]}: {busiest[1]})")
    sheets = list(o.sheets())
    if sheets:
        total = sum(s.byte_len for s in sheets)
        uniq = {s.dhash for s in sheets}
        fmts: dict[int, int] = {}
        for s in sheets:
            fmts[s.pixfmt] = fmts.get(s.pixfmt, 0) + 1
        print(f"sheets    : {len(sheets)}  distinct_dhash={len(uniq)}  "
              f"pixels={total/1024:.0f} KiB  "
              + " ".join(f"{PIXFMT_NAME.get(f, f)}={c}" for f, c in sorted(fmts.items())))
    fonts = list(o.fonts())
    if fonts:
        faces = sorted({f"{f.face}/{f.height}" for f in fonts})
        print(f"fonts     : {len(fonts)}  " + " ".join(faces))
    texts = list(o.texts())
    if texts:
        named = sum(1 for t in texts if t.font_ref)
        dst_ok = sum(1 for t in texts if t.dst_handle)
        ft = [(fl, len(tl)) for fl, _, tl in o.frames_with_texts() if tl]
        colors = {t.color for t in texts}
        print(f"texts     : {len(texts)}  font_ref set={named} "
              f"({100*named//len(texts)}%)  dst_handle set={dst_ok} "
              f"({100*dst_ok//len(texts)}%)  distinct_colors={len(colors)}  "
              f"frames_w_text={len(ft)}")
    print(f"anchors   : {len(anchors)}")
    for flip, tick, rng, name in anchors:
        print(f"    {name:<16} flip={flip} sim_tick={tick} rng=0x{rng:x}")
    print(f"seeds     : {len(seeds)}")
    for flip, before, value in seeds:
        print(f"    flip={flip} 0x{before:x} -> 0x{value:x}")
    return 0


def cmd_frames(path: str) -> int:
    o = parse(path)
    for flip, tick, bl in o.frames_with_blits():
        print(f"flip={flip}\tsim_tick={tick}\tblits={len(bl)}")
    return 0


def cmd_blits(path: str, want_flip: int | None) -> int:
    """Dump the draw list of one frame (by flip), or the first frame with blits."""
    o = parse(path)
    for flip, tick, bl in o.frames_with_blits():
        if not bl:
            continue
        if want_flip is not None and flip != want_flip:
            continue
        print(f"== frame flip={flip} sim_tick={tick}  {len(bl)} blits ==")
        for b in bl:
            name = BLIT_VA_NAME.get(b.va, hex(b.va))
            print(f"  #{b.seq:<3} {name:<7} res={b.res} frame={b.frame} "
                  f"dst=({b.dx},{b.dy}) {b.reqw}x{b.reqh} src=({b.sx},{b.sy}) "
                  f"o=({b.ox},{b.oy},{b.ow},{b.oh}) "
                  f"st=0x{b.state:x} ckey=0x{b.ckey:x} bmode={b.bmode}")
        return 0
    print("(no frame with blits found)")
    return 0


def cmd_sheets(path: str) -> int:
    """Dump the dedup'd SHEET records (the source-pixel sheets, M3c)."""
    o = parse(path)
    sheets = list(o.sheets())
    print(f"== {len(sheets)} SHEET records ==")
    for s in sheets:
        print(f"  dhash=0x{s.dhash:08x} res={s.res} frame={s.frame} "
              f"{s.w}x{s.h} pitch={s.pitch} "
              f"{PIXFMT_NAME.get(s.pixfmt, s.pixfmt)} codec={s.codec} "
              f"bytes={s.byte_len}")
    return 0


BKMODE_NAME = {1: "TRANSPARENT", 2: "OPAQUE"}


def cmd_texts(path: str, want_flip: int | None) -> int:
    """Dump the GDI TEXT stream (M3d): the FONT table, then per-frame TextOut ops."""
    o = parse(path)
    fonts = list(o.fonts())
    print(f"== {len(fonts)} FONT records ==")
    for f in fonts:
        print(f"  ref={f.font_ref} '{f.face}' h={f.height} w={f.width} "
              f"weight={f.weight} italic={f.italic} charset={f.charset} "
              f"pitch_family=0x{f.pitch_family:02x}")
    shown = False
    for flip, tick, tl in o.frames_with_texts():
        if not tl:
            continue
        if want_flip is not None and flip != want_flip:
            continue
        print(f"== frame flip={flip} sim_tick={tick}  {len(tl)} texts ==")
        for t in tl:
            print(f"  #{t.seq:<3} ({t.x},{t.y}) ref={t.font_ref} "
                  f"color=0x{t.color:06x} bk={BKMODE_NAME.get(t.bk_mode, t.bk_mode)} "
                  f"dst={t.dst_handle} {t.text!r}")
        shown = True
        if want_flip is None:
            break
    if not shown:
        print("(no frame with text found)")
    return 0


def main(argv) -> int:
    if len(argv) < 3 or argv[1] not in (
            "SUMMARY", "FRAMES", "BLITS", "SHEETS", "TEXTS"):
        print(__doc__)
        return 2
    if argv[1] == "SUMMARY":
        return cmd_summary(argv[2])
    if argv[1] == "BLITS":
        flip = int(argv[3]) if len(argv) > 3 else None
        return cmd_blits(argv[2], flip)
    if argv[1] == "SHEETS":
        return cmd_sheets(argv[2])
    if argv[1] == "TEXTS":
        flip = int(argv[3]) if len(argv) > 3 else None
        return cmd_texts(argv[2], flip)
    return cmd_frames(argv[2])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
