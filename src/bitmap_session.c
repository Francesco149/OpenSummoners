/*
 * src/bitmap_session.c — pure-logic port of the PE-resource bitmap
 * decoder class.  See src/bitmap_session.h for what each function
 * does and which retail FUN_… it ports.
 *
 * Win32-free: the LocalAlloc/LocalFree + resource-load primitives are
 * extern declarations in the header, defined in either
 * `bitmap_session_win32.c` (real build) or in the test harness
 * (`tests/test_bitmap_session.c`).
 *
 * Decode-path provenance:
 *   raw path        — FUN_005b7800 if (compressed_flag == 0)
 *   compressed path — FUN_005b7800 if (compressed_flag != 0) +
 *                     FUN_005b7c10 helper
 *
 * The compressed-resource format's self-rebasing pointer table is
 * undocumented; the formulas in bs_parse_compressed_header are
 * lifted literally from FUN_005b7c10's decomp.  Hex offsets (0x420,
 * 0x430, 0x438, 0x440, 0x448, 0x450, 0x458) are the pointer-table
 * slots inside the resource; 0x2711 is the version/signature
 * constant.  Naming the slots beyond "value at offset" would require
 * matching the format against a known sprite-pack format —
 * deferred.
 */
#include "bitmap_session.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ─── trivial accessors / state ──────────────────────────────────── */

void bs_release_no_free(bitmap_session *s)
{
    s->pixels = NULL;
}

void bs_release(bitmap_session *s)
{
    if (s->pixels != NULL) {
        bs_local_free(s->pixels);
        s->pixels = NULL;
    }
}

uint16_t bs_get_bit_count(const bitmap_session *s)
{
    return s->biBitCount;
}

void bs_set_bit_count(bitmap_session *s, uint16_t bit_count)
{
    s->biBitCount = bit_count;
    s->stride     = (uint32_t)(bit_count >> 3) * s->biWidth;
}

/* ─── allocation + BITMAPINFOHEADER stamp ────────────────────────── */

int bs_init_bitmap(bitmap_session *s, uint32_t width, uint32_t height,
                   uint16_t bit_count)
{
    uint32_t size_image = (uint32_t)(bit_count >> 3) * width * height;

    /* Match retail issue order — biBitCount goes in early via the
     * direct write; the BITMAPINFOHEADER fields are filled next; the
     * LocalAlloc happens last and the recompute-stride trail comes
     * last via bs_set_bit_count. */
    s->biBitCount      = bit_count;
    s->biSize          = 0x28;
    s->biWidth         = width;
    s->biHeight        = height;
    s->biPlanes        = 1;
    s->biCompression   = 0;
    s->biSizeImage     = size_image;
    s->biXPelsPerMeter = 0;
    s->biYPelsPerMeter = 0;
    s->biClrUsed       = 0;
    s->biClrImportant  = 0;

    s->pixels = bs_local_alloc_zeroed(size_image);
    if (s->pixels == NULL) {
        return 0;
    }

    bs_set_bit_count(s, bit_count);
    return 1;
}

/* ─── palette emit (RGB→BGR per entry) ───────────────────────────── */

void bs_emit_palette_bgra(const bitmap_session *s, uint8_t *out_palette)
{
    /* Retail body's pointer arithmetic:
     *   puVar2 = (uint8_t *)(this + 0x35);    // palette[0].byte[1]
     *   puVar1 = (uint8_t *)(dest + 2);
     *   for (256x) {
     *     puVar1[-2] = puVar2[ 1];   // dest[0] = src[2]  (B = blue)
     *     puVar1[-1] = puVar2[ 0];   // dest[1] = src[1]  (G = green)
     *     puVar1[ 0] = puVar2[-1];   // dest[2] = src[0]  (R = red)
     *     puVar1[ 1] = 0;            // dest[3] = 0       (reserved)
     *     puVar1 += 4;  puVar2 += 4;
     *   }
     *
     * In source-palette terms (offset 0..3 = R, G, B, _) writing to
     * destination as (B, G, R, 0) — same RGB triplet, reversed channel
     * order, padded to a 4-byte RGBQUAD. */
    const uint8_t *src = &s->palette[0];
    for (int i = 0; i < 256; i++) {
        out_palette[i * 4 + 0] = src[i * 4 + 2];
        out_palette[i * 4 + 1] = src[i * 4 + 1];
        out_palette[i * 4 + 2] = src[i * 4 + 0];
        out_palette[i * 4 + 3] = 0;
    }
}

/* ─── compressed-header parser (free function, FUN_005b7c10) ─────── */

int bs_parse_compressed_header(bs_bitmap_info *out, const void *resource_data,
                               uint32_t *out_pixel_offset)
{
    /* Treat the resource as a u32 array for the pointer-table slots,
     * with byte-precise reads for the depth at +0x430. */
    const uint8_t  *raw   = (const uint8_t *)resource_data;
    const uint32_t *slots = (const uint32_t *)resource_data;

    int32_t  base   = (int32_t)slots[0x448 / 4];
    int32_t  sig    = (int32_t)slots[0x438 / 4] - base;
    if (sig != 0x2711) {
        return 0;
    }

    uint32_t pixel_off = (uint32_t)((int32_t)slots[0x450 / 4] - base);
    if (pixel_off > 0x80) {
        return 0;
    }

    /* The +0x430 read is genuinely 16-bit in retail
     * (`*(short *)(param_2 + 0x430)`) — preserves the sign-truncation
     * semantics if the unrebased value happens to be negative. */
    int16_t depth_raw  = *(const int16_t *)(raw + 0x430);
    uint16_t bit_count = (uint16_t)((int16_t)(depth_raw - (int16_t)base));

    /* Two more indirected reads — slots[0x18/4] and slots[0x440/4] hold
     * indices (after unrebase) into a u32 array starting at the
     * resource base; the dereferenced values are themselves rebased
     * pointers.  See the docstring for the literal formulas. */
    int32_t height_idx = (int32_t)slots[0x18  / 4] - base;
    int32_t width_idx  = (int32_t)slots[0x440 / 4] - base;
    int32_t height_raw = (int32_t)slots[(0x420 / 4) + height_idx];
    int32_t width_raw  = (int32_t)slots[(4     / 4) + width_idx];
    uint32_t biHeight  = (uint32_t)(height_raw - base);
    uint32_t biWidth   = (uint32_t)(width_raw  - base);

    out->biSize          = 0x28;
    out->biWidth         = biWidth;
    out->biHeight        = biHeight;
    out->biPlanes        = 1;
    out->biBitCount      = bit_count;
    out->biCompression   = 0;
    out->biSizeImage     = (uint32_t)(bit_count >> 3) * biWidth * biHeight;
    out->biXPelsPerMeter = 0;
    out->biYPelsPerMeter = 0;
    out->biClrUsed       = 0;
    out->biClrImportant  = 0;
    memcpy(out->palette, raw + 0x20, 256 * 4);

    *out_pixel_offset = pixel_off;
    return 1;
}

/* ─── decode-resource (FUN_005b7800) ─────────────────────────────── */

int bs_decode_resource(bitmap_session *s, void *hModule, uint16_t resource_id,
                       const char *resource_type, int compressed_flag)
{
    /* Retail prologue is a defensive release — covers the case where
     * the session was previously used (the stack-local pattern in
     * FUN_004178e0 always bs_release_no_free's first, so this is a
     * no-op in that flow, but it makes the decoder safe to call on
     * any session state). */
    bs_release(s);

    const void *resource = bs_load_pe_resource(hModule, resource_id, resource_type);
    if (resource == NULL) {
        return 0;
    }
    const uint8_t  *raw   = (const uint8_t *)resource;
    const uint32_t *slots = (const uint32_t *)resource;

    const uint8_t *pixel_src;

    if (compressed_flag == 0) {
        /* Raw path: header at offset stashed in slots[0]; width =
         * slots[1]; height = slots[2]; biBitCount = u16 at byte +0x0e. */
        const uint8_t *header = raw + (int32_t)slots[0];
        uint32_t width    = slots[1];
        uint32_t height   = slots[2];
        uint16_t bit_count = *(const uint16_t *)(raw + 0x0e);

        if (!bs_init_bitmap(s, width, height, bit_count)) {
            return 0;
        }

        if (s->biBitCount == 8) {
            memcpy(s->palette, header, 256 * 4);
            pixel_src = header + 256 * 4;
        } else if (s->biBitCount == 24) {
            pixel_src = header;
        } else {
            /* Unsupported depth — retail releases the pixel buffer
             * (we just allocated it) and returns failure. */
            bs_release(s);
            return 0;
        }
    } else {
        /* Compressed path: parse self-rebasing header into a transient
         * BITMAPINFO on stack, then copy palette + rebase pixel offset
         * onto resource+0x458. */
        bs_bitmap_info hdr;
        uint32_t pixel_off = 0;
        if (!bs_parse_compressed_header(&hdr, resource, &pixel_off)) {
            return 0;
        }
        /* Retail signals "no init failure" by not checking the return
         * value — `FUN_005b71f0` is called unconditionally in the
         * compressed path of FUN_005b7800.  We keep the
         * implicit-success contract: if the alloc inside bs_init_bitmap
         * fails, the subsequent memcpy into NULL would crash, matching
         * retail.  No silent failure path exists in this branch. */
        (void)bs_init_bitmap(s, hdr.biWidth, hdr.biHeight, hdr.biBitCount);

        memcpy(s->palette, hdr.palette, 256 * 4);
        pixel_src = raw + pixel_off + 0x458;
    }

    /* Common tail: copy (biHeight * stride) bytes from pixel_src into
     * this->pixels.  Retail emits a dword loop + byte-tail loop; here
     * memcpy is equivalent and the optimiser produces identical
     * codegen. */
    memcpy(s->pixels, pixel_src, (size_t)s->biHeight * s->stride);
    return 1;
}

/* ─── per-cell opaque-bounding-box scan (FUN_005b6f80) ─────────────── */

/* The decoded sheet is a bottom-up DIB, so scanline `r` (top = 0) lives at
 * pixels + (biHeight-1-r)*stride.  Retail computes a "bottom row" base
 * (the one-line helper 0x5b6ec0 = pixels + (biHeight-1)*stride, inlined here)
 * and indexes each cell row as
 * `bottom + base_x*bpp - (base_y+row)*stride`; we mirror that arithmetic
 * exactly so the address of every probed pixel matches retail's. */
void bs_trim_opaque_rect(const bitmap_session *s, uint32_t key,
                         int32_t base_x, int32_t base_y,
                         int32_t width, int32_t height, bs_trim_rect *out)
{
    /* `width` = cell column span (retail param_4 = cell_w, inner/x loop);
     * `height` = cell row span (retail param_5 = cell_h, outer/y loop).  The
     * body below uses `height` for the row loop and `width` for the column
     * loop — correct ONLY because the caller passes (cell_w, cell_h) and these
     * params are named to match (quirk #69; previously transposed). */
    /* init (0x5b6f8f..0x5b6fa7): the box starts inverted so the min/max
     * folds below tighten it; both flags clear. */
    out->x_left      = width;
    out->x_right     = 0;
    out->y_top       = height;
    out->y_bottom    = 0;
    out->found_opaque = 0;
    out->found_key    = 0;

    uint16_t depth = s->biBitCount;                 /* FUN_005b6f00 */

    if (depth != 8 && depth != 24) {
        /* other depth (0x5b6fbe): can't classify pixels → the whole cell is
         * the box and both flags are set. */
        out->x_left      = 0;
        out->x_right     = width - 1;
        out->y_top       = 0;
        out->y_bottom    = height - 1;
        out->found_opaque = 1;
        out->found_key    = 1;
        return;
    }

    const uint8_t *pixels = (const uint8_t *)s->pixels;
    ptrdiff_t      stride = (ptrdiff_t)(int32_t)s->stride;
    const uint8_t *bottom = pixels + (ptrdiff_t)(int32_t)(s->biHeight - 1) * stride;

    if (depth == 24) {
        /* 0x5b6fe2: 3 bytes/px (B,G,R); key split into its three channels. */
        uint8_t kB = (uint8_t)key;
        uint8_t kG = (uint8_t)(key >> 8);
        uint8_t kR = (uint8_t)(key >> 16);

        for (int32_t row = 0; row < height; row++) {          /* ebx, 0..H */
            const uint8_t *p = bottom + (ptrdiff_t)base_x * 3
                                      - (ptrdiff_t)(base_y + row) * stride;
            for (int32_t x = 0; x < width; x++) {             /* ecx, 0..W */
                if (p[2] == kR && p[1] == kG && p[0] == kB) {
                    out->found_key = 1;                       /* 0x5b7047 */
                } else {
                    if (out->x_left > x) out->x_left = x;      /* min, 0x5b7051 */
                    out->found_opaque = 1;                     /* 0x5b705b */
                }
                p += 3;
            }
            /* Gate (0x5b706b): the *global* x_left < W — quirk #48.  Once any
             * earlier row has opaque, this stays true, so trailing transparent
             * rows still extend y_bottom (and run a no-op right-scan). */
            if (out->x_left < width) {
                /* right-edge scan (0x5b7071): walk left from the last column
                 * over key pixels; the first opaque column is x_right. */
                const uint8_t *q = bottom + (ptrdiff_t)(base_x + width - 1) * 3
                                          - (ptrdiff_t)(base_y + row) * stride;
                int32_t x = width - 1;
                while (x >= 0 && q[2] == kR && q[1] == kG && q[0] == kB) {
                    q -= 3; x--;
                }
                if (x >= 0 && out->x_right < x) out->x_right = x;   /* max */
                if (out->y_top    > row) out->y_top    = row;       /* min */
                if (out->y_bottom < row) out->y_bottom = row;       /* max */
            }
        }
        return;
    }

    /* depth == 8 (0x5b70e4): 1 byte/px palette index, compared against the
     * key's low byte.  The opaque gate is *per-row* (ebx, reset each row) —
     * unlike the 24bpp path — so y_bottom is the last genuinely-opaque row. */
    uint8_t k = (uint8_t)key;
    for (int32_t row = 0; row < height; row++) {
        int row_opaque = 0;                                   /* ebx */
        const uint8_t *p = bottom + (ptrdiff_t)base_x
                                  - (ptrdiff_t)(base_y + row) * stride;
        for (int32_t x = 0; x < width; x++) {
            if (p[x] == k) {
                out->found_key = 1;                           /* 0x5b713c */
            } else {
                if (out->x_left > x) out->x_left = x;          /* min, 0x5b7127 */
                row_opaque = 1;                                /* ebx = 1 */
                out->found_opaque = 1;
            }
        }
        if (row_opaque) {                                     /* test ebx, 0x5b7149 */
            const uint8_t *r = bottom + (ptrdiff_t)base_x
                                      - (ptrdiff_t)(base_y + row) * stride;
            int32_t x = width - 1;
            while (x >= 0 && r[x] == k) { x--; }
            if (x >= 0 && out->x_right < x) out->x_right = x;  /* max */
            if (out->y_top    > row) out->y_top    = row;
            if (out->y_bottom < row) out->y_bottom = row;
        }
    }
}

/* ─── display-depth format converters (8d format switch) ─────────────
 *
 * Each converts s->pixels in place to a new depth: alloc a fresh buffer,
 * walk the source, LocalFree the old buffer, install the new one, and
 * re-stamp biBitCount.  All faithful to retail's pixel-walk shapes
 * (FUN_005b7310/_74f0/_7270/_7bd0); the alloc/free use the same
 * LMEM_ZEROINIT-equivalent bs_local_alloc_zeroed / bs_local_free. */

int bs_convert_to_16bpp(bitmap_session *s, const uint8_t *desc,
                        uint32_t colorkey, uint32_t key_color)
{
    /* Shift table (the ZDD pixel-format descriptor):
     *   sl[ch] = desc[0..2]      left-shift to place channel ch (R,G,B)
     *   sr[ch] = desc[0x10..12]  right-shift the 8bpp channel first */
    const uint8_t sl_r = desc[0],    sl_g = desc[1],    sl_b = desc[2];
    const uint8_t sr_r = desc[0x10], sr_g = desc[0x11], sr_b = desc[0x12];

    uint32_t total = s->stride * s->biHeight;   /* total source bytes */

    if (s->biBitCount == 8) {
        uint8_t *out = (uint8_t *)bs_local_alloc_zeroed(
                           s->biHeight * s->biWidth * 2);
        if (out == NULL) return 0;
        const uint8_t *idxp = (const uint8_t *)s->pixels;
        uint8_t *o = out;
        for (uint32_t i = 0; i < total; i++) {
            uint32_t r8, g8, b8;
            uint32_t idx = idxp[i];
            if (idx == colorkey) {
                b8 = (key_color & 0xff);
                g8 = (key_color >> 8) & 0xff;
                r8 = (key_color >> 16) & 0xff;
            } else {
                b8 = s->palette[idx * 4 + 0];
                g8 = s->palette[idx * 4 + 1];
                r8 = s->palette[idx * 4 + 2];
            }
            uint32_t v = ((g8 >> sr_g) << sl_g)
                       | ((b8 >> sr_b) << sl_b)
                       | ((r8 >> sr_r) << sl_r);
            o[0] = (uint8_t)v;
            o[1] = (uint8_t)(v >> 8);
            o += 2;
        }
        bs_local_free(s->pixels);
        s->pixels = out;
        bs_set_bit_count(s, 0x10);
        return 1;
    }

    if (s->biBitCount == 0x18) {
        uint8_t *out = (uint8_t *)bs_local_alloc_zeroed(
                           s->biHeight * s->biWidth * 2);
        if (out == NULL) return 0;
        const uint8_t *p = (const uint8_t *)s->pixels;   /* B,G,R triplets */
        uint8_t *o = out;
        uint32_t npix = total / 3;
        for (uint32_t i = 0; i < npix; i++) {
            uint32_t b8 = p[0], g8 = p[1], r8 = p[2];
            uint32_t v = ((r8 >> sr_r) << sl_r)
                       | ((g8 >> sr_g) << sl_g)
                       | ((b8 >> sr_b) << sl_b);
            o[0] = (uint8_t)v;
            o[1] = (uint8_t)(v >> 8);
            o += 2;
            p += 3;
        }
        bs_local_free(s->pixels);
        s->pixels = out;
        bs_set_bit_count(s, 0x10);
        return 1;
    }

    (void)key_color;
    return 0;   /* neither 8 nor 24bpp */
}

int bs_convert_8bpp_to_24bpp(bitmap_session *s, uint32_t colorkey,
                             uint32_t key_color)
{
    if (s->biBitCount != 8) return 0;

    uint8_t *out = (uint8_t *)bs_local_alloc_zeroed(
                       s->biWidth * s->biHeight * 3);
    if (out == NULL) return 0;

    const uint8_t *idxp = (const uint8_t *)s->pixels;
    /* Dest index uses stride*row + col (matches retail) — for an 8bpp
     * sheet stride == biWidth, so the packed width*height*3 buffer is
     * addressed contiguously. */
    for (int32_t row = 0; row < (int32_t)s->biHeight; row++) {
        for (int32_t col = 0; col < (int32_t)s->biWidth; col++) {
            int32_t off = (int32_t)s->stride * row + col;
            uint32_t idx = idxp[off];
            uint8_t *d = out + (size_t)off * 3;
            if (idx == colorkey) {
                d[0] = (uint8_t)(key_color);
                d[1] = (uint8_t)(key_color >> 8);
                d[2] = (uint8_t)(key_color >> 16);
            } else {
                d[0] = s->palette[idx * 4 + 0];
                d[1] = s->palette[idx * 4 + 1];
                d[2] = s->palette[idx * 4 + 2];
            }
        }
    }
    bs_local_free(s->pixels);
    s->pixels = out;
    bs_set_bit_count(s, 0x18);
    return 1;
}

int bs_convert_24bpp_to_32bpp(bitmap_session *s)
{
    if (s->biBitCount != 0x18) return 0;

    uint8_t *out = (uint8_t *)bs_local_alloc_zeroed(
                       s->biHeight * s->biWidth * 4);
    if (out == NULL) return 0;

    const uint8_t *p = (const uint8_t *)s->pixels;
    uint8_t *o = out;
    uint32_t npix = (s->stride * s->biHeight) / 3;
    for (uint32_t i = 0; i < npix; i++) {
        o[0] = p[0];        /* B */
        o[1] = p[1];        /* G */
        o[2] = p[2];        /* R */
        o[3] = 0;           /* X */
        o += 4;
        p += 3;
    }
    bs_local_free(s->pixels);
    s->pixels = out;
    bs_set_bit_count(s, 0x20);
    return 1;
}

void bs_load_palette_from(bitmap_session *s, const uint8_t *src)
{
    /* Session entry {byte0,byte1,byte2,byte3} = {src[2],src[1],src[0],0}
     * — channel-reversed RGBQUAD with the reserved byte zeroed. */
    for (int i = 0; i < 256; i++) {
        s->palette[i * 4 + 0] = src[i * 4 + 2];
        s->palette[i * 4 + 1] = src[i * 4 + 1];
        s->palette[i * 4 + 2] = src[i * 4 + 0];
        s->palette[i * 4 + 3] = 0;
    }
}
