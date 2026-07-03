#!/usr/bin/env python3
"""sheet_export.py — pull a decoded SOURCE sheet (OSR_SHEET) out of an .osr and
dump its raw pixels + a PNG, and diff the same sheet across two captures.

The Trace Studio v2 stream dedups each decoded source surface into one SHEET
record (src/osr_format.h), keyed by dhash and tagged with a representative
(res, frame).  When a render divergence is localized to "the right sprite
decoded to the wrong pixels" (errands-render-gaps.md §7, the res=1071 bed
bottom-strip), the way to CONFIRM is to extract the raw sheet from BOTH sides
and byte-diff it — dhash alone can't (pitch/pixfmt differences legitimately
change the hash cross-side; PORT-DEBT(osr-sheet-dhash-xside)).

Streams the file (retail captures are multi-GB) via osr.stream_records.

Usage:
    # list every SHEET for a resource (dhash/geometry), both files:
    sheet_export.py LIST <file.osr> [res]
    # export one res's sheet(s) to raw .bin + .png under OUTDIR:
    sheet_export.py DUMP <file.osr> <res> <outdir> [frame]
    # diff a res's sheet across two captures, row-by-row:
    sheet_export.py DIFF <port.osr> <retail.osr> <res> <outdir> [frame]
"""
from __future__ import annotations

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import osr  # noqa: E402


def _sheets_for_res(path, res, frame=None):
    """Stream SHEET records, return the list of Sheet objects matching res
    (and frame, if given).  Dedups by dhash (a sheet is emitted once)."""
    out, seen = [], set()
    for r in osr.stream_records(path, {osr.SHEET}):
        s = r.sheet()
        if s.res != res:
            continue
        if frame is not None and s.frame != frame:
            continue
        if s.dhash in seen:
            continue
        seen.add(s.dhash)
        out.append(s)
    return out


def _rgb565_to_rgb(pix, w, h, pitch):
    """Return a flat bytes RGB (w*h*3), top-down.  pitch is bytes/row."""
    out = bytearray(w * h * 3)
    o = 0
    for y in range(h):
        row = y * pitch
        for x in range(w):
            v = pix[row + x * 2] | (pix[row + x * 2 + 1] << 8)
            r5 = (v >> 11) & 0x1F
            g6 = (v >> 5) & 0x3F
            b5 = v & 0x1F
            out[o] = (r5 << 3) | (r5 >> 2)
            out[o + 1] = (g6 << 2) | (g6 >> 4)
            out[o + 2] = (b5 << 3) | (b5 >> 2)
            o += 3
    return bytes(out)


def _write_png(path, rgb, w, h):
    """Minimal zlib PNG writer (no PIL dependency needed on the host)."""
    import zlib
    raw = bytearray()
    for y in range(h):
        raw.append(0)  # filter: none
        raw.extend(rgb[y * w * 3:(y + 1) * w * 3])

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)  # 8-bit RGB
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


def _describe(s):
    return (f"dhash=0x{s.dhash:08x} res={s.res} frame={s.frame} "
            f"{s.w}x{s.h} pitch={s.pitch} "
            f"{osr.PIXFMT_NAME.get(s.pixfmt, s.pixfmt)} bytes={s.byte_len}")


def cmd_list(path, res):
    for r in osr.stream_records(path, {osr.SHEET}):
        s = r.sheet()
        if res is not None and s.res != res:
            continue
        print(_describe(s))
    return 0


def _dump_one(s, outdir, tag):
    os.makedirs(outdir, exist_ok=True)
    base = f"{tag}_res{s.res}_fr{s.frame}_{s.w}x{s.h}"
    binp = os.path.join(outdir, base + ".bin")
    with open(binp, "wb") as f:
        f.write(s.pixels)
    print(f"  raw  -> {binp} ({len(s.pixels)} bytes)")
    if s.pixfmt == 1 and s.byte_len >= s.pitch * s.h:   # 1 = OSR_PIXFMT_RGB565
        rgb = _rgb565_to_rgb(s.pixels, s.w, s.h, s.pitch)
        pngp = os.path.join(outdir, base + ".png")
        _write_png(pngp, rgb, s.w, s.h)
        print(f"  png  -> {pngp}")
    return binp


def cmd_dump(path, res, outdir, frame):
    sheets = _sheets_for_res(path, res, frame)
    if not sheets:
        print(f"(no SHEET for res={res}"
              + (f" frame={frame}" if frame is not None else "") + ")")
        return 1
    for s in sheets:
        print(_describe(s))
        _dump_one(s, outdir, "sheet")
    return 0


def cmd_diff(port_path, retail_path, res, outdir, frame):
    ps = _sheets_for_res(port_path, res, frame)
    rs = _sheets_for_res(retail_path, res, frame)
    print(f"== port SHEET(s) for res={res} ==")
    for s in ps:
        print("  " + _describe(s))
    print(f"== retail SHEET(s) for res={res} ==")
    for s in rs:
        print("  " + _describe(s))
    if not ps or not rs:
        print("!! one side has no matching sheet — cannot diff")
        return 1
    p, r = ps[0], rs[0]
    _dump_one(p, outdir, "port")
    _dump_one(r, outdir, "retail")

    if p.w != r.w or p.h != r.h:
        print(f"!! geometry differs port {p.w}x{p.h} vs retail {r.w}x{r.h}")
        return 1
    w, h = p.w, p.h
    print(f"\n== per-row diff ({w}x{h}, pitch port={p.pitch} retail={r.pitch}) ==")
    ndiff_rows = 0
    first_diff_row = last_diff_row = None
    for y in range(h):
        pr = p.pixels[y * p.pitch:y * p.pitch + w * 2]
        rr = r.pixels[y * r.pitch:y * r.pitch + w * 2]
        n = sum(1 for i in range(0, min(len(pr), len(rr)), 2)
                if pr[i:i + 2] != rr[i:i + 2])
        if n:
            ndiff_rows += 1
            if first_diff_row is None:
                first_diff_row = y
            last_diff_row = y
            if ndiff_rows <= 40:
                print(f"  row y={y:3d}: {n:3d}/{w} px differ")
    print(f"\n{ndiff_rows}/{h} rows differ; "
          f"range y={first_diff_row}..{last_diff_row}"
          if ndiff_rows else "\nALL ROWS IDENTICAL — sheets are byte-equal")
    return 0


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    cmd = argv[1]
    if cmd == "LIST":
        res = int(argv[3], 0) if len(argv) > 3 else None
        return cmd_list(argv[2], res)
    if cmd == "DUMP":
        frame = int(argv[5], 0) if len(argv) > 5 else None
        return cmd_dump(argv[2], int(argv[3], 0), argv[4], frame)
    if cmd == "DIFF":
        frame = int(argv[6], 0) if len(argv) > 6 else None
        return cmd_diff(argv[2], argv[3], int(argv[4], 0), argv[5], frame)
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
