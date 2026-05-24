/*
 * src/bitmap_session.h — bitmap-session struct (PE-resource bitmap
 * decoder's `this`).
 *
 * NOT WIRED YET.  This header exists so the Ghidra TagThiscallFunctions
 * script can correlate the bitmap-session class namespace with a typed
 * `this` pointer, restoring `this->field` reads across the
 * `FUN_005b6e70` / _6e90 / _6f00 / _6f10 / _71f0 / _7800 / _7b90 / _7c10
 * family.  See `docs/findings/palette-session.md` for the rabbit-hole
 * notes and `docs/findings/cpp-recovery-workflow.md` for why the
 * Ghidra-side tagging is the right unblocker.
 *
 * Once the typed decomp resolves the ECX puzzle in `FUN_004178e0`, the
 * functions in this family will be ported and the porting declarations
 * added below.  Until then, this header is layout-only.
 *
 * Win32-free: no `<windows.h>` here.  The pixel buffer is `void *`
 * (retail uses HLOCAL from LocalAlloc); the palette is a flat 1024-byte
 * array (retail conceptually uses 256 RGBQUAD entries).  The real-build
 * primitive wrappers (LocalAlloc/Free, FindResourceA/LoadResource/
 * LockResource) will live in `bitmap_session_win32.c` when the port
 * lands; the test harness will replace them with `calloc`/`free` and
 * per-test resource byte-buffers.
 */
#ifndef OPENSUMMONERS_BITMAP_SESSION_H
#define OPENSUMMONERS_BITMAP_SESSION_H

#include <stddef.h>
#include <stdint.h>

/* Layout inferred from `FUN_005b71f0` (writes via in_ECX[3..0xc] plus
 * *in_ECX) and the 8bpp palette-copy loop in `FUN_005b7800` (writes
 * starting at in_ECX + 0xd, which is byte offset 0x34, for 256 dwords =
 * 1024 bytes).  See `docs/findings/palette-session.md` "The bitmap-
 * session struct (inferred from FUN_005b71f0 + FUN_005b6f10)" for the
 * full provenance table.
 *
 * +0x0c..+0x30 mirror a Win32 BITMAPINFOHEADER exactly — the bitmap
 * info that FUN_005b71f0 / _6f10 keep up-to-date matches the
 * `BITMAPINFO::bmiHeader` shape one-to-one (biSize=0x28, biPlanes=1,
 * biCompression=BI_RGB=0, etc. are all literal-stamped at init).  The
 * trailing +0x34 palette completes a `BITMAPINFO` (header + bmiColors)
 * — handy if/when the engine ever wants to call StretchDIBits or
 * similar with `&this->biSize` as the BITMAPINFO * argument. */
typedef struct bitmap_session {
    /* +0x00 (4B): pixel buffer — `LocalAlloc(LMEM_ZEROINIT, biSizeImage)`
     * in retail, freed via `LocalFree` by the destructor (FUN_005b6e90).
     * NULL means "not yet allocated" (the BSS or post-release state). */
    void     *pixels;
    /* +0x04 (4B): row stride in bytes — `(biBitCount / 8) * biWidth`.
     * Written by FUN_005b6f10 (the depth setter) AND derivable from
     * biBitCount + biWidth; cached for the inner pixel-copy loops in
     * FUN_005b7800. */
    uint32_t  stride;
    /* +0x08 (4B): no observed reads or writes — likely padding or a
     * not-yet-RE'd field. */
    uint32_t  f_08;
    /* +0x0c (4B): BITMAPINFOHEADER.biSize — stamped 0x28 by FUN_005b71f0. */
    uint32_t  biSize;
    /* +0x10 (4B): BITMAPINFOHEADER.biWidth. */
    uint32_t  biWidth;
    /* +0x14 (4B): BITMAPINFOHEADER.biHeight. */
    uint32_t  biHeight;
    /* +0x18 (2B): BITMAPINFOHEADER.biPlanes — stamped 1 by FUN_005b71f0. */
    uint16_t  biPlanes;
    /* +0x1a (2B): BITMAPINFOHEADER.biBitCount.  Set by FUN_005b71f0 and
     * re-stamped by FUN_005b6f10; read by FUN_005b6f00 (the getter) and
     * by FUN_005b7800 (gates the 8bpp palette-copy branch vs the 24bpp
     * raw-pixel branch). */
    uint16_t  biBitCount;
    /* +0x1c (4B): BITMAPINFOHEADER.biCompression — stamped 0 (BI_RGB). */
    uint32_t  biCompression;
    /* +0x20 (4B): BITMAPINFOHEADER.biSizeImage — pixel-buffer size. */
    uint32_t  biSizeImage;
    /* +0x24 (4B): BITMAPINFOHEADER.biXPelsPerMeter — stamped 0. */
    uint32_t  biXPelsPerMeter;
    /* +0x28 (4B): BITMAPINFOHEADER.biYPelsPerMeter — stamped 0. */
    uint32_t  biYPelsPerMeter;
    /* +0x2c (4B): BITMAPINFOHEADER.biClrUsed — stamped 0. */
    uint32_t  biClrUsed;
    /* +0x30 (4B): BITMAPINFOHEADER.biClrImportant — stamped 0. */
    uint32_t  biClrImportant;
    /* +0x34 (1024B): bmiColors palette — 256 RGBQUAD entries, copied
     * from the PE resource by FUN_005b7800 (in the 8bpp branch) or
     * from FUN_005b7c10's local stack buffer (in the compressed
     * branch).  FUN_005b7b90 reads from here to emit a BGRA palette
     * into a caller-owned buffer. */
    uint8_t   palette[256 * 4];
} bitmap_session;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(bitmap_session) == 0x434, "bitmap_session must be 0x434 bytes");
_Static_assert(offsetof(bitmap_session, pixels)          == 0x00, "bitmap_session pixels offset");
_Static_assert(offsetof(bitmap_session, stride)          == 0x04, "bitmap_session stride offset");
_Static_assert(offsetof(bitmap_session, biSize)          == 0x0c, "bitmap_session biSize offset");
_Static_assert(offsetof(bitmap_session, biWidth)         == 0x10, "bitmap_session biWidth offset");
_Static_assert(offsetof(bitmap_session, biHeight)        == 0x14, "bitmap_session biHeight offset");
_Static_assert(offsetof(bitmap_session, biPlanes)        == 0x18, "bitmap_session biPlanes offset");
_Static_assert(offsetof(bitmap_session, biBitCount)      == 0x1a, "bitmap_session biBitCount offset");
_Static_assert(offsetof(bitmap_session, biCompression)   == 0x1c, "bitmap_session biCompression offset");
_Static_assert(offsetof(bitmap_session, biSizeImage)     == 0x20, "bitmap_session biSizeImage offset");
_Static_assert(offsetof(bitmap_session, biXPelsPerMeter) == 0x24, "bitmap_session biXPelsPerMeter offset");
_Static_assert(offsetof(bitmap_session, biYPelsPerMeter) == 0x28, "bitmap_session biYPelsPerMeter offset");
_Static_assert(offsetof(bitmap_session, biClrUsed)       == 0x2c, "bitmap_session biClrUsed offset");
_Static_assert(offsetof(bitmap_session, biClrImportant)  == 0x30, "bitmap_session biClrImportant offset");
_Static_assert(offsetof(bitmap_session, palette)         == 0x34, "bitmap_session palette offset");
#endif

#endif /* OPENSUMMONERS_BITMAP_SESSION_H */
