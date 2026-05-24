#!/usr/bin/env python3
"""
tools/extract/lizsoft_sprite.py — Decode a Lizsoft sprite DATA blob to PNG.

Format (reverse-engineered against sotesd.dll DATA blobs, sprite family
magic `5f 42 00 00 8b 36 00 00 …`):

    offset  size                       meaning
    0       32  outer magic            (8 u32 LE; u32[2] varies per blob,
                                        likely a CRC/hash; rest fixed)
    32      1024 palette                (256 entries × BGRA RGBQUAD, DDraw layout)
    1056    64   secondary header       (mostly fixed, only u32[2] varies)
    1120    14   BMFH-style preamble    (`BM`, fsize=blob_size-64,
                                        reserved=0, data_off=1078)
    1134    8    sub-sig                (52 53 5f 42 00 00 8b 36)
    1142    W*H  8bpp indexed pixel data

Width/height are NOT in the file.  They come from the engine's
`FUN_0056e190`-family registration calls which hardcode the dimensions
per (resource_id) — typical values 0xa0×0xb0 etc.  Until that registry
is dumped, callers must supply (W, H) explicitly or rely on the
auto-guess (factorise `pixel_bytes` into plausible W,H pairs).

The pixel data is in standard BMP top-down or bottom-up; without an
external truth we render top-down by default — flip with `--flip`.

Magic / size sanity is verified on every read.  Mismatches raise
ValueError so an extractor sweep doesn't silently mis-decode.

Usage:
    python3 tools/extract/lizsoft_sprite.py path/to/1013.bin --out 1013.png
    python3 tools/extract/lizsoft_sprite.py path/to/1013.bin -w 80 -h 80
    python3 tools/extract/lizsoft_sprite.py path/to/1013.bin --guess-grid

`--guess-grid` writes every plausible (W, H) factorisation as a separate
PNG so the eye can pick the right one.

Requires Pillow (already pulled by the flake's Python env).
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


OUTER_MAGIC_U32S_FIXED = {
    0: 0x0000_425f,
    1: 0x0000_368b,
    3: 0x0000_b757,
    4: 0x0000_7625,
    5: 0x0000_8f19,
    6: 0x0000_32bd,
    7: 0x0000_ead3,
}
# u32[2] varies per blob; everything else above is verified equal across
# every DATA blob in sotesd.dll's sprite family.

PALETTE_OFF      = 32
PALETTE_LEN      = 1024            # 256 × 4 (BGRA)
SUBHDR_OFF       = 1056
SUBHDR_LEN       = 64
BMFH_OFF         = 1120
BMFH_LEN         = 14
SUBSIG_OFF       = 1134
SUBSIG_BYTES     = bytes.fromhex("52535f4200008b36")
SUBSIG_LEN       = len(SUBSIG_BYTES)   # 8
PIXELS_OFF       = 1142

BMFH_DATA_OFFSET = 1078            # standard for 8bpp BMP


@dataclass
class LizsoftSprite:
    pixel_bytes: int          # blob_size - 1142
    palette_bgra: bytes       # 1024 bytes
    pixels:      bytes        # pixel_bytes
    bmfh_fsize:  int          # from the embedded BMFH
    src_path:    Path | None = None

    @classmethod
    def parse(cls, blob: bytes, src: Path | None = None) -> "LizsoftSprite":
        if len(blob) < PIXELS_OFF + 16:
            raise ValueError(f"blob too short: {len(blob)} bytes")
        u32 = struct.unpack_from("<8I", blob, 0)
        for i, expected in OUTER_MAGIC_U32S_FIXED.items():
            if u32[i] != expected:
                raise ValueError(
                    f"outer magic mismatch at u32[{i}]: "
                    f"got 0x{u32[i]:08x} expected 0x{expected:08x}")
        if blob[BMFH_OFF:BMFH_OFF+2] != b"BM":
            raise ValueError(f"BMFH signature missing at offset {BMFH_OFF}")
        bmfh_fsize, _r1, _r2, doff = struct.unpack_from(
            "<IHHI", blob, BMFH_OFF + 2)
        if doff != BMFH_DATA_OFFSET:
            raise ValueError(
                f"BMFH data_off = {doff}, expected {BMFH_DATA_OFFSET}")
        if bmfh_fsize != len(blob) - 64:
            raise ValueError(
                f"BMFH fsize = {bmfh_fsize} but blob_size - 64 = "
                f"{len(blob) - 64}; mismatch")
        if blob[SUBSIG_OFF:SUBSIG_OFF+SUBSIG_LEN] != SUBSIG_BYTES:
            raise ValueError("sub-sig mismatch at offset 1134")
        pixels = blob[PIXELS_OFF:]
        if len(pixels) != bmfh_fsize - doff:
            raise ValueError(
                f"pixel-region length {len(pixels)} != fsize - doff "
                f"{bmfh_fsize - doff}")
        return cls(
            pixel_bytes  = len(pixels),
            palette_bgra = blob[PALETTE_OFF:PALETTE_OFF+PALETTE_LEN],
            pixels       = pixels,
            bmfh_fsize   = bmfh_fsize,
            src_path     = src,
        )

    def pil_palette_rgb(self) -> list[int]:
        # PIL palettes are R,G,B triples flat-packed.  Source is BGRA.
        out: list[int] = []
        for i in range(256):
            b, g, r, _a = self.palette_bgra[i*4:i*4+4]
            out.extend([r, g, b])
        return out

    def render(self, w: int, h: int, flip_vertical: bool = False) -> Image.Image:
        if w * h != self.pixel_bytes:
            raise ValueError(
                f"{w}*{h} = {w*h} != pixel_bytes ({self.pixel_bytes})")
        im = Image.frombytes("P", (w, h), self.pixels)
        im.putpalette(self.pil_palette_rgb())
        if flip_vertical:
            im = im.transpose(Image.FLIP_TOP_BOTTOM)
        return im


def plausible_dims(n: int) -> list[tuple[int, int]]:
    """All (w, h) factorisations where both ∈ [16, 2048] and both ≤ 4096
    pixels, with w as the smaller factor (avoid 2x duplicates by symmetry
    when w == h)."""
    out: list[tuple[int, int]] = []
    for w in range(16, 2049):
        if n % w != 0:
            continue
        h = n // w
        if 16 <= h <= 2048:
            out.append((w, h))
    return out


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("path",  type=Path)
    p.add_argument("--out", type=Path, default=None,
                   help="output PNG (single dim) or directory (--guess-grid)")
    p.add_argument("-W", "--width",  type=int, default=None)
    p.add_argument("-H", "--height", type=int, default=None)
    p.add_argument("--flip", action="store_true",
                   help="flip vertically (BMP bottom-up convention)")
    p.add_argument("--guess-grid", action="store_true",
                   help="render every plausible (w, h) into a directory")
    args = p.parse_args()

    blob = args.path.read_bytes()
    sprite = LizsoftSprite.parse(blob, src=args.path)
    print(f"parsed {args.path.name}: "
          f"pixels={sprite.pixel_bytes}  "
          f"bmfh_fsize={sprite.bmfh_fsize}", file=sys.stderr)

    if args.guess_grid:
        out_dir = args.out or args.path.with_suffix(".guess")
        out_dir.mkdir(parents=True, exist_ok=True)
        dims = plausible_dims(sprite.pixel_bytes)
        print(f"writing {len(dims)} candidates to {out_dir}", file=sys.stderr)
        for w, h in dims:
            im = sprite.render(w, h, flip_vertical=args.flip)
            im.save(out_dir / f"{w:04d}x{h:04d}.png")
        return 0

    if args.width is None or args.height is None:
        dims = plausible_dims(sprite.pixel_bytes)
        print(f"width/height not given; plausible: {dims}", file=sys.stderr)
        return 2

    im = sprite.render(args.width, args.height, flip_vertical=args.flip)
    out = args.out or args.path.with_suffix(f".{args.width}x{args.height}.png")
    im.save(out)
    print(f"wrote {out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
