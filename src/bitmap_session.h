/*
 * src/bitmap_session.h — bitmap-session class for the PE-resource
 * bitmap decoder used by sotes.exe's palette-ramp path.
 *
 * Ports the 7-method `__thiscall` family that owns one 0x434-byte
 * bitmap_session struct (lifecycle managed entirely on the caller's
 * stack — see FUN_004178e0):
 *
 *   - FUN_005b6e70  bs_release_no_free       (zero pixel-buffer pointer)
 *   - FUN_005b6e90  bs_release                (LocalFree pixel buffer)
 *   - FUN_005b6f00  bs_get_bit_count          (read biBitCount)
 *   - FUN_005b6f10  bs_set_bit_count          (write biBitCount + stride)
 *   - FUN_005b71f0  bs_init_bitmap            (alloc pixels + init BIH)
 *   - FUN_005b7800  bs_decode_resource        (load PE resource + decode)
 *   - FUN_005b7b90  bs_emit_palette_bgra      (palette → caller buf)
 *
 * Plus the standalone compressed-header parser FUN_005b7c10
 * (`bs_parse_compressed_header`).  Despite living in the same family,
 * its `this` is a transient stack BITMAPINFO supplied by the caller
 * (not a bitmap_session) — so it's a free function, not a method.
 *
 * Caller-side wrapper FUN_004178e0 (ar_palette_session_begin) lives in
 * asset_register.c — it's a sprite_slot method that builds a
 * bitmap_session on its own stack, runs it through the decoder, and
 * emits 256 palette entries into a caller buffer.
 *
 * Win32-free header: no `<windows.h>`.  The pixel buffer is `void *`
 * (retail: HLOCAL from LocalAlloc) and the palette is a flat
 * 1024-byte array (retail: 256 RGBQUAD entries).  Primitive wrappers
 * (`bs_local_alloc_zeroed`, `bs_local_free`, `bs_load_pe_resource`)
 * are declared here but DEFINED in either `bitmap_session_win32.c`
 * (real build) or in the test harness (recording / table-driven
 * stubs).
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

/* ─── Win32 primitive wrappers — defined per build target ────────── */

/* FUN_005b71f0 calls `LocalAlloc(LMEM_ZEROINIT, bytes)`.  Our wrapper
 * mirrors that contract: return a `bytes`-sized zero-initialised
 * pointer, or NULL on failure.  Caller pairs every successful call
 * with bs_local_free. */
void *bs_local_alloc_zeroed(uint32_t bytes);

/* Pairs with bs_local_alloc_zeroed.  NULL is a no-op (matches
 * LocalFree's contract). */
void  bs_local_free(void *p);

/* Combined FindResourceA + LoadResource + LockResource.  The retail
 * code chains these one after the other and never frees the result
 * (PE resources are immutable mappings inside the loaded image).
 *
 * Returns a pointer to the locked PE resource data, or NULL on any
 * step's failure.  resource_id is the integer ID after MAKEINTRESOURCE;
 * resource_type is the literal type-name string ("DATA" in the
 * palette-ramp path's case). */
const void *bs_load_pe_resource(void *hModule, uint16_t resource_id,
                                 const char *resource_type);

/* ─── bitmap_session methods ─────────────────────────────────────── */

/* FUN_005b6e70 — `this->pixels = NULL` (no free).  Defensive init
 * before bs_decode_resource on an uninitialised stack session. */
void bs_release_no_free(bitmap_session *s);

/* FUN_005b6e90 (also reachable via thunk_FUN_005b6e80) — release the
 * pixel buffer: `if (this->pixels) { LocalFree(this->pixels);
 * this->pixels = NULL; }`.  Idempotent on NULL — safe to call twice. */
void bs_release(bitmap_session *s);

/* FUN_005b6f00 — getter for `this->biBitCount`. */
uint16_t bs_get_bit_count(const bitmap_session *s);

/* FUN_005b6f10 — setter for `this->biBitCount` that ALSO recomputes
 * `this->stride = (bit_count / 8) * this->biWidth`.  The retail body
 * does both writes unconditionally; we match.  Caller must have
 * already set biWidth (typically via a prior bs_init_bitmap). */
void bs_set_bit_count(bitmap_session *s, uint16_t bit_count);

/* FUN_005b71f0 — allocate the pixel buffer and stamp BITMAPINFOHEADER
 * fields for a fresh bitmap of the given dimensions.  Sets:
 *   pixels = LocalAlloc(zeroed, (bit_count/8) * width * height)
 *   stride = (bit_count/8) * width        (via bs_set_bit_count)
 *   biSize=0x28, biPlanes=1, biCompression=0,
 *   biXPelsPerMeter=biYPelsPerMeter=biClrUsed=biClrImportant=0
 *   biWidth, biHeight, biBitCount, biSizeImage = computed
 *
 * Returns 1 on success, 0 if LocalAlloc fails.  Does NOT release a
 * pre-existing pixel buffer — caller must call bs_release first if
 * the session is non-fresh.  (FUN_005b71f0's retail body matches:
 * unconditional alloc, no pre-release.) */
int bs_init_bitmap(bitmap_session *s, uint32_t width, uint32_t height,
                   uint16_t bit_count);

/* FUN_005b7800 — load a PE resource and decode it into this session.
 *
 * compressed_flag == 0 — raw resource:
 *   data[0] is the header offset within the resource.  The header is
 *   read for biWidth/biHeight/biBitCount, then either a 256-RGBQUAD
 *   palette (8bpp) or no palette (24bpp) precedes the pixel bytes.
 *
 * compressed_flag != 0 — packed resource (bs_parse_compressed_header):
 *   the resource carries a self-rebasing pointer table; the parser
 *   extracts width/height/bit_count/palette into a transient
 *   BITMAPINFO + emits an offset for the pixel data.
 *
 * Path-independent tail: copies (biHeight * stride) bytes of pixel
 * data into `this->pixels` via dword + byte-tail loop.
 *
 * Returns 1 on success, 0 on any failure (resource missing, bad
 * signature, unsupported bit depth — only 8 and 24 are recognised in
 * the raw path; 8 is the only one that copies a palette).
 *
 * Calls bs_release on entry (idempotent on NULL pixels) — safe on a
 * stack session that's been bs_release_no_free'd. */
int bs_decode_resource(bitmap_session *s, void *hModule, uint16_t resource_id,
                       const char *resource_type, int compressed_flag);

/* FUN_005b7b90 — emit a 256-entry BGRA palette into `out_palette`,
 * derived from the session's palette.  The retail body reads bytes at
 * session+0x34..0x37 (palette[0]) as (R, G, B, _) and writes them as
 * (B, G, R, 0) to out — effectively a per-entry channel swap.  Note
 * that the session palette is also nominally RGBQUAD-shaped
 * (BITMAPINFO::bmiColors); the swap reverses RGB→BGR order in the
 * destination.  Caller owns out_palette (must be ≥ 1024 bytes).
 *
 * Used by ar_palette_session_begin only when the decoded bitmap is
 * 8bpp — i.e. when the session palette was actually populated by
 * bs_decode_resource's 8bpp branch. */
void bs_emit_palette_bgra(const bitmap_session *s, uint8_t *out_palette);

/* ─── compressed-resource header parser (free function) ──────────── */

/* The caller-supplied BITMAPINFO that FUN_005b7c10 populates.  Layout
 * is the Win32 BITMAPINFOHEADER + 256-RGBQUAD palette — 0x428 bytes
 * total — and matches what the caller of FUN_005b7800 has on its
 * stack as `local_428`..`local_400[256]`. */
typedef struct bs_bitmap_info {
    uint32_t biSize;          /* +0x00 — stamped 0x28 */
    uint32_t biWidth;         /* +0x04 */
    uint32_t biHeight;        /* +0x08 */
    uint16_t biPlanes;        /* +0x0c — stamped 1 */
    uint16_t biBitCount;      /* +0x0e */
    uint32_t biCompression;   /* +0x10 — stamped 0 */
    uint32_t biSizeImage;     /* +0x14 = (biBitCount/8) * width * height */
    uint32_t biXPelsPerMeter; /* +0x18 — stamped 0 */
    uint32_t biYPelsPerMeter; /* +0x1c — stamped 0 */
    uint32_t biClrUsed;       /* +0x20 — stamped 0 */
    uint32_t biClrImportant;  /* +0x24 — stamped 0 */
    uint8_t  palette[256 * 4];/* +0x28 — copied from resource */
} bs_bitmap_info;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(bs_bitmap_info) == 0x428, "bs_bitmap_info must be 0x428 bytes");
_Static_assert(offsetof(bs_bitmap_info, biWidth)    == 0x04, "bs_bitmap_info biWidth offset");
_Static_assert(offsetof(bs_bitmap_info, biHeight)   == 0x08, "bs_bitmap_info biHeight offset");
_Static_assert(offsetof(bs_bitmap_info, biPlanes)   == 0x0c, "bs_bitmap_info biPlanes offset");
_Static_assert(offsetof(bs_bitmap_info, biBitCount) == 0x0e, "bs_bitmap_info biBitCount offset");
_Static_assert(offsetof(bs_bitmap_info, palette)    == 0x28, "bs_bitmap_info palette offset");
#endif

/* FUN_005b7c10 — parse a compressed-resource header.
 *
 * The resource format has a self-rebasing pointer table starting at
 * offset 0x420.  `iVar5 = resource[0x448]` is the base used to
 * unrebase every pointer in the table.  The signature check is
 * `resource[0x438] - iVar5 == 0x2711`; on mismatch the parser returns
 * 0 without touching `out` or `out_pixel_offset`.
 *
 * On success:
 *   out->biSize          = 0x28
 *   out->biWidth         = resource[4 + (resource[0x440] - iVar5) * 4] - iVar5
 *   out->biHeight        = resource[0x420 + (resource[0x18] - iVar5) * 4] - iVar5
 *   out->biPlanes        = 1
 *   out->biBitCount      = (uint16_t)(resource[0x430] - iVar5)
 *   out->biCompression   = 0
 *   out->biSizeImage     = (biBitCount/8) * biWidth * biHeight
 *   out->biX/Y/PelsPerMeter / biClrUsed / biClrImportant = 0
 *   out->palette[256*4]  = memcpy from resource + 0x20  (256 dwords)
 *   *out_pixel_offset    = resource[0x450] - iVar5  (must be ≤ 0x80,
 *                          else parser returns 0 — pixel offset is
 *                          rebased onto resource+0x458 by the caller)
 *
 * Returns 1 on success, 0 on signature mismatch or pixel-offset
 * out-of-range. */
int bs_parse_compressed_header(bs_bitmap_info *out, const void *resource_data,
                               uint32_t *out_pixel_offset);

#endif /* OPENSUMMONERS_BITMAP_SESSION_H */
