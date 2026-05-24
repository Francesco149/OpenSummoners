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
