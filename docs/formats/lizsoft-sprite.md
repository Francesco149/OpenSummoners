# Lizsoft sprite format (`0x425f` family) — partial spec

Format used by ~28% of the DATA entries in `sotesd.dll` (213/759 blobs
parse cleanly as this variant; the rest belong to sibling families
documented in TODO).  Pure file-shape reverse from byte inspection
across ~13 blobs (1011, 1012, 1013/4/5 identical-triple, 1016, 1018,
1020, 1023, 1025, 1030, others); no decompiled-code anchor yet.  The
parser in `tools/extract/lizsoft_sprite.py` round-trips raw pixel data
to PNG given (W, H).

## Layout

| offset      | size     | contents                                                         |
|-------------|----------|------------------------------------------------------------------|
| `0x000`     | 32 B     | outer magic header (8 LE u32s; see §Header)                      |
| `0x020`     | 1024 B   | 256-entry BGRA palette (DDraw `RGBQUAD` layout — `B G R A`)     |
| `0x420`     | 64 B     | secondary header / sub-table (see §Sub-table)                    |
| `0x460`     | 14 B     | BMFH-style preamble (`BM`, fsize, reserved, data_off)            |
| `0x46e`     | 8 B      | sub-sig `52 53 5f 42 00 00 8b 36`                                |
| `0x476`     | W × H B  | 8-bit indexed pixel data, top-down                               |

Total file size = 1142 + W × H.

## Header (offset `0x000`, 32 bytes)

8 little-endian u32 fields.  Values **fixed across every blob in this
family** except `u32[2]`, which varies per blob and is almost certainly
a CRC / hash (it doesn't equal file size, dims, or any obvious linear
combination):

| idx | value      | constancy | notes                                          |
|-----|------------|-----------|------------------------------------------------|
| 0   | `0x425f`   | fixed     | `"_B"` LE — partial ASCII tag                  |
| 1   | `0x368b`   | fixed     |                                                |
| 2   | varies     | per-blob  | per-blob ID / CRC, range `~0x330b..0x35bb`     |
| 3   | `0xb757`   | fixed     |                                                |
| 4   | `0x7625`   | fixed     |                                                |
| 5   | `0x8f19`   | fixed     |                                                |
| 6   | `0x32bd`   | fixed     |                                                |
| 7   | `0xead3`   | fixed     |                                                |

All seven fixed u32s have top 16 bits zero, so this is effectively a
sequence of 8 u16 values padded to u32 alignment.

## Palette (offset `0x020`, 1024 bytes)

256 entries in `BGRA` order matching `RGBQUAD` (and DirectDraw's
`PALETTEENTRY` w/ peFlags=0): `B, G, R, alpha-or-flag`.  Alpha byte is
consistently zero on entries observed (likely `peFlags`, ignored).

Indices 0-15 carry the standard 16-colour VGA palette; indices 16-247
are sprite-specific; indices 248-255 are the high-intensity VGA colours.
Many entries are unused and contain a sentinel of `80 80 00 00` (cyan
with peFlags=0).

The palette is loaded into an `IDirectDrawPalette` via
`IDirectDraw7::CreatePalette` + `IDirectDrawSurface7::SetPalette`
(method 31, offset 0x7c — see `docs/findings/ddraw-init.md`).

## Sub-table (offset `0x420`, 64 bytes)

16 little-endian u32 fields, mostly fixed:

| idx | value          | constancy | notes                                |
|-----|----------------|-----------|--------------------------------------|
| 0   | `0x416f`       | fixed     |                                      |
| 1   | `0x0a85`       | fixed     |                                      |
| 2   | varies         | per-blob  | second per-blob ID (different from outer u32[2]) |
| 3   | `0x22ad`       | fixed     |                                      |
| 4-13| various fixed  | fixed     | constants in 0x32xx range            |
| 14  | `0x02110620`   | fixed     | might encode a date: `20 06 11 02` little-endian — Feb 17 2006? game predates this by ~2 years so unclear |
| 15  | `0x0000001e`   | fixed     | `= 30`; count of *something* (palette colours actually used?  asset version?) |

Purpose of the sub-table is unknown.  It's redundant with the outer
header for purposes of magic-checking and contains no obvious
dimensions, frame counts, or scene metadata.

## BMFH-style preamble (offset `0x460`, 14 bytes)

Exactly a Windows `BITMAPFILEHEADER` layout, with `fsize` set as if the
14-byte preamble were at BMP-relative offset 1056 (i.e., 64 bytes were
prepended to a real BMP):

| field            | size | value                              |
|------------------|------|------------------------------------|
| `bfType`         | 2 B  | `"BM"`                             |
| `bfSize`         | 4 B  | **`blob_size - 64`** (invariant) — note this is NOT `blob_size - 1120` as a real BMP would have |
| `bfReserved1/2`  | 4 B  | `0, 0`                             |
| `bfOffBits`      | 4 B  | `1078` — canonical 8bpp offset (14 + 40 + 1024) |

The `bfSize == blob_size - 64` invariant holds across every parsed
blob, which makes this a load-bearing integrity field rather than
incidental bytes.  We exploit it in the parser as a final sanity check.

This is not a complete BMP — the BIH that should follow at offset
`0x46e` is replaced by an 8-byte sub-sig (`52 53 5f 42 00 00 8b 36`)
and pixel data starts immediately at offset `0x476`.  The engine
likely **synthesises** a real BIH (width, height, planes=1, bpp=8) at
parse time from external metadata and feeds (synthesised BIH + this
blob's palette + this blob's pixels) into whatever BMP / DIB pathway
the renderer uses.

## Pixel data (offset `0x476`, W × H bytes)

8 bits per pixel, palette-indexed, top-down (verified visually for
known sprites with the test extractor; the BMP convention is bottom-up,
so the engine's synthesised BIH likely sets `biHeight = -H`).

Width and height are **not in the file**.  They are hardcoded in the
engine's sprite-registration calls (`FUN_0056e190` family — see
`docs/findings/asset-loader.md`'s "asset register" section).  To
extract an arbitrary blob, the (resource_id → W × H) registry has to
come from one of:

1. Dumping all `slot[8] = W; slot[9] = H` assignments in the
   registration functions (radare2 scan over the call sites is
   probably the fastest).
2. A Frida hook on `IDirectDrawSurface7::Lock` that captures `lPitch`
   and the surface dimensions at runtime, paired with a hook on
   `FUN_005b62a0` to log which (DLL, ID) is being decoded.

`tools/extract/lizsoft_sprite.py --guess-grid` writes every plausible
(W, H) factorisation of `pixel_bytes` so a human can pick the correct
one by eye while we wait for option 1 or 2 to land.

## Sibling formats (not specced)

The other 540-odd DATA entries fall into at least three more variants
visible at byte level:

| outer u32[0] | example IDs | bmfh data_off | hypothesis                  |
|--------------|-------------|---------------|------------------------------|
| `0x425f`     | 1011+       | 1078          | **this spec** (8bpp paletted) |
| `0xe225`     | 1090, 1091  | varies        | likely a different sprite family — same DLL, same `DATA` type |
| `0x361f`     | 1092        | varies        | another variant              |
| `0x3e49`     | 1108, 1181  | varies        | another variant              |
| `0x425f` but `data_off=54` | 1081, 1082 | 54 | 24/32bpp BMP (no palette: 14 + 40 BIH only) |

Spec these as needed — none are blockers for the 213-blob `0x425f`
family right now.

## Outstanding questions

- **u32[2] of the outer header** — per-blob ID or CRC?  Probably a
  checksum the engine validates after read; the 0x32xx clustering
  suggests it has constant high bits and only low 12 bits vary.
- **The 8-byte sub-sig at offset `0x46e`** — looks like 2 bytes `RS`
  (`52 53`) followed by 6 bytes of the outer magic.  Possibly a
  "resource section" / "resource start" marker.
- **The 64-byte sub-table at offset `0x420`** — purpose unknown; not
  needed for pixel decode.
- **The date-like value `0x02110620` in the sub-table** — if it really
  is `20 06 11 02` decoded as YY-MM-DD-?? then 2006-02-17 predates
  Fortune Summoners' 2008 release; could be an engine-build timestamp,
  asset-tool build, or coincidence.
