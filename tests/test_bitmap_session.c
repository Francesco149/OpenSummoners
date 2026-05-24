/*
 * tests/test_bitmap_session.c — host tests for the PE-resource bitmap
 * decoder (src/bitmap_session.c).
 *
 * Provides the three Win32 primitives the port needs:
 *   - bs_local_alloc_zeroed → calloc (heap-backed, freed by the
 *     paired bs_local_free).
 *   - bs_local_free → free.
 *   - bs_load_pe_resource → table lookup against a per-test "fake PE
 *     resource directory" the tests populate by hand.
 *
 * Two synthetic resource shapes are constructed inline (the raw and
 * compressed paths of FUN_005b7800).  Their byte layouts are documented
 * at the helper that builds each one.
 */
#include "../src/asset_register.h"
#include "../src/bitmap_session.h"
#include "t.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── stub heap ──────────────────────────────────────────────────── */

/* Track active allocations so we can assert leak-free at the end of
 * each test.  Each successful bs_local_alloc_zeroed increments
 * g_live_allocs and the matching free decrements it. */
static int g_live_allocs = 0;

void *bs_local_alloc_zeroed(uint32_t bytes)
{
    void *p = calloc(1, bytes);
    if (p) g_live_allocs++;
    return p;
}

void bs_local_free(void *p)
{
    if (p) {
        g_live_allocs--;
        free(p);
    }
}

/* ─── stub PE resource directory ─────────────────────────────────── */

/* Each test populates one or more rows here; bs_load_pe_resource
 * returns the matching `data` pointer (or NULL).  Type-string match
 * is exact-strcmp so tests can simulate FindResource lookup misses. */
typedef struct stub_resource {
    void       *hModule;
    uint16_t    resource_id;
    const char *type;
    const void *data;
} stub_resource;

enum { STUB_RES_MAX = 8 };
static stub_resource g_resources[STUB_RES_MAX];
static int           g_resource_count = 0;

const void *bs_load_pe_resource(void *hModule, uint16_t resource_id,
                                 const char *resource_type)
{
    for (int i = 0; i < g_resource_count; i++) {
        stub_resource *r = &g_resources[i];
        if (r->hModule == hModule
            && r->resource_id == resource_id
            && strcmp(r->type, resource_type) == 0) {
            return r->data;
        }
    }
    return NULL;
}

static void stub_reset(void)
{
    memset(g_resources, 0, sizeof g_resources);
    g_resource_count = 0;
    g_live_allocs = 0;
}

static void stub_register(void *hModule, uint16_t id, const char *type,
                           const void *data)
{
    if (g_resource_count >= STUB_RES_MAX) abort();
    g_resources[g_resource_count++] =
        (stub_resource){ hModule, id, type, data };
}

/* ─── raw-resource builder ───────────────────────────────────────── */

/* Raw resource layout (compressed_flag=0):
 *   u32 header_offset_from_data_start    // FUN_005b7800: header = data + data[0]
 *   u32 biWidth                           // data[1]
 *   u32 biHeight                          // data[2]
 *   u32 _pad_for_bitcount_at_byte_0xe    // bits 0..15 of data[3] are unused
 *                                         // (Ghidra reads u16 at byte 0x0e)
 *   ... padding up to header_offset ...
 *   if (bit_count == 8): 1024 bytes palette
 *   N bytes of pixel data (N = (bit_count/8) * width * height)
 *
 * For tests we keep the header right after the 4-dword leader.
 */
static uint8_t g_raw_resource[8192];

static const void *build_raw_resource_8bpp(uint32_t width, uint32_t height,
                                            uint8_t fill_pixel)
{
    memset(g_raw_resource, 0, sizeof g_raw_resource);
    uint32_t *slots = (uint32_t *)g_raw_resource;
    slots[0] = 0x10;     /* header offset = 0x10 (right after the 4-dword leader) */
    slots[1] = width;
    slots[2] = height;
    /* biBitCount at byte 0x0e (= u16 in the 4th dword). */
    *(uint16_t *)(g_raw_resource + 0x0e) = 8;

    /* Palette at header_offset: 256 RGBQUADs (we leave them at 0 with
     * a single per-test override so tests can assert what got copied). */
    for (int i = 0; i < 256; i++) {
        g_raw_resource[0x10 + i * 4 + 0] = (uint8_t)i;       /* B-ish */
        g_raw_resource[0x10 + i * 4 + 1] = (uint8_t)(i + 1); /* G-ish */
        g_raw_resource[0x10 + i * 4 + 2] = (uint8_t)(i + 2); /* R-ish */
        g_raw_resource[0x10 + i * 4 + 3] = 0;
    }

    /* Pixel bytes follow palette: width*height bytes at +0x10+0x400. */
    memset(g_raw_resource + 0x10 + 0x400, fill_pixel,
           (size_t)width * height);
    return g_raw_resource;
}

static const void *build_raw_resource_24bpp(uint32_t width, uint32_t height,
                                             uint8_t fill_pixel)
{
    memset(g_raw_resource, 0, sizeof g_raw_resource);
    uint32_t *slots = (uint32_t *)g_raw_resource;
    slots[0] = 0x10;
    slots[1] = width;
    slots[2] = height;
    *(uint16_t *)(g_raw_resource + 0x0e) = 24;
    /* 24bpp: no palette — pixels start at header. */
    memset(g_raw_resource + 0x10, fill_pixel,
           (size_t)width * height * 3);
    return g_raw_resource;
}

/* ─── trivial accessor tests ─────────────────────────────────────── */

int test_bs_release_no_free_nulls_pixels(void)
{
    bitmap_session s = { .pixels = (void *)0xdeadbeef };
    bs_release_no_free(&s);
    T_ASSERT_EQ_P(s.pixels, NULL);
    return 0;
}

int test_bs_release_is_idempotent_on_null(void)
{
    stub_reset();
    bitmap_session s = { .pixels = NULL };
    bs_release(&s);
    bs_release(&s);   /* second call is the same code path */
    T_ASSERT_EQ_I(g_live_allocs, 0);
    T_ASSERT_EQ_P(s.pixels, NULL);
    return 0;
}

int test_bs_release_frees_pixels(void)
{
    stub_reset();
    bitmap_session s;
    bs_release_no_free(&s);
    s.pixels = bs_local_alloc_zeroed(32);
    T_ASSERT(s.pixels != NULL);
    T_ASSERT_EQ_I(g_live_allocs, 1);
    bs_release(&s);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    T_ASSERT_EQ_P(s.pixels, NULL);
    return 0;
}

int test_bs_get_set_bit_count_recomputes_stride(void)
{
    bitmap_session s = { .biWidth = 50 };
    bs_set_bit_count(&s, 16);
    T_ASSERT_EQ_U(bs_get_bit_count(&s), 16);
    /* (16 / 8) * 50 = 100 */
    T_ASSERT_EQ_U(s.stride, 100);
    bs_set_bit_count(&s, 24);
    T_ASSERT_EQ_U(s.stride, 150);
    return 0;
}

/* ─── bs_init_bitmap ─────────────────────────────────────────────── */

int test_bs_init_bitmap_stamps_BIH(void)
{
    stub_reset();
    bitmap_session s = {0};
    int ok = bs_init_bitmap(&s, /*w=*/32, /*h=*/20, /*bpp=*/8);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(s.biSize, 0x28);
    T_ASSERT_EQ_U(s.biWidth, 32);
    T_ASSERT_EQ_U(s.biHeight, 20);
    T_ASSERT_EQ_U(s.biPlanes, 1);
    T_ASSERT_EQ_U(s.biBitCount, 8);
    T_ASSERT_EQ_U(s.biCompression, 0);
    T_ASSERT_EQ_U(s.biSizeImage, 32 * 20);  /* (8/8) * 32 * 20 */
    T_ASSERT_EQ_U(s.biXPelsPerMeter, 0);
    T_ASSERT_EQ_U(s.biYPelsPerMeter, 0);
    T_ASSERT_EQ_U(s.biClrUsed, 0);
    T_ASSERT_EQ_U(s.biClrImportant, 0);
    T_ASSERT_EQ_U(s.stride, 32);            /* (8/8) * 32 */
    T_ASSERT(s.pixels != NULL);
    /* Zero-filled (we used calloc in the stub). */
    uint8_t *px = (uint8_t *)s.pixels;
    for (int i = 0; i < 32 * 20; i++) T_ASSERT_EQ_U(px[i], 0);
    bs_release(&s);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_bs_init_bitmap_24bpp_size(void)
{
    stub_reset();
    bitmap_session s = {0};
    T_ASSERT_EQ_I(bs_init_bitmap(&s, 10, 4, 24), 1);
    T_ASSERT_EQ_U(s.biSizeImage, 10 * 4 * 3);
    T_ASSERT_EQ_U(s.stride, 10 * 3);
    bs_release(&s);
    return 0;
}

/* ─── bs_emit_palette_bgra ───────────────────────────────────────── */

int test_bs_emit_palette_bgra_swaps_RGB_to_BGR(void)
{
    bitmap_session s = {0};
    /* Stamp source palette[0] = (R=0x11, G=0x22, B=0x33, _=0xff)
     * and palette[1] = (R=0xaa, G=0xbb, B=0xcc, _=0x77). */
    s.palette[0] = 0x11; s.palette[1] = 0x22; s.palette[2] = 0x33; s.palette[3] = 0xff;
    s.palette[4] = 0xaa; s.palette[5] = 0xbb; s.palette[6] = 0xcc; s.palette[7] = 0x77;

    uint8_t dest[1024] = {0};
    bs_emit_palette_bgra(&s, dest);

    /* dest[0..3] should be (B=0x33, G=0x22, R=0x11, _=0). */
    T_ASSERT_EQ_U(dest[0], 0x33);
    T_ASSERT_EQ_U(dest[1], 0x22);
    T_ASSERT_EQ_U(dest[2], 0x11);
    T_ASSERT_EQ_U(dest[3], 0x00);
    /* dest[4..7] should be (B=0xcc, G=0xbb, R=0xaa, _=0). */
    T_ASSERT_EQ_U(dest[4], 0xcc);
    T_ASSERT_EQ_U(dest[5], 0xbb);
    T_ASSERT_EQ_U(dest[6], 0xaa);
    T_ASSERT_EQ_U(dest[7], 0x00);
    /* Rest are zero-source → zero-dest with 0 in the alpha slot. */
    for (int i = 8; i < 256 * 4; i += 4) {
        T_ASSERT_EQ_U(dest[i + 3], 0);
    }
    return 0;
}

/* ─── bs_decode_resource raw path ────────────────────────────────── */

int test_decode_returns_zero_on_missing_resource(void)
{
    stub_reset();
    bitmap_session s;
    bs_release_no_free(&s);
    int ok = bs_decode_resource(&s, (void *)0x1234, 0x90b, "DATA", 0);
    T_ASSERT_EQ_I(ok, 0);
    T_ASSERT_EQ_P(s.pixels, NULL);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_decode_raw_8bpp_copies_palette_and_pixels(void)
{
    stub_reset();
    const void *resource = build_raw_resource_8bpp(4, 3, /*fill=*/0xaa);
    stub_register((void *)0x1234, 0x90b, "DATA", resource);

    bitmap_session s;
    bs_release_no_free(&s);
    int ok = bs_decode_resource(&s, (void *)0x1234, 0x90b, "DATA", 0);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(s.biWidth, 4);
    T_ASSERT_EQ_U(s.biHeight, 3);
    T_ASSERT_EQ_U(s.biBitCount, 8);
    T_ASSERT_EQ_U(s.stride, 4);
    T_ASSERT(s.pixels != NULL);

    /* Palette: builder seeded entry[i] = (i, i+1, i+2, 0). */
    for (int i = 0; i < 256; i++) {
        T_ASSERT_EQ_U(s.palette[i * 4 + 0], (uint8_t)i);
        T_ASSERT_EQ_U(s.palette[i * 4 + 1], (uint8_t)(i + 1));
        T_ASSERT_EQ_U(s.palette[i * 4 + 2], (uint8_t)(i + 2));
        T_ASSERT_EQ_U(s.palette[i * 4 + 3], 0);
    }
    /* Pixels: all 0xaa, 12 bytes. */
    uint8_t *px = (uint8_t *)s.pixels;
    for (int i = 0; i < 12; i++) T_ASSERT_EQ_U(px[i], 0xaa);

    bs_release(&s);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_decode_raw_24bpp_skips_palette_copy(void)
{
    stub_reset();
    const void *resource = build_raw_resource_24bpp(3, 2, /*fill=*/0x77);
    stub_register((void *)0x1234, 0x90b, "DATA", resource);

    bitmap_session s;
    bs_release_no_free(&s);
    /* Pre-stamp s.palette so we can detect that it WASN'T overwritten. */
    s.palette[0] = 0xfe;

    int ok = bs_decode_resource(&s, (void *)0x1234, 0x90b, "DATA", 0);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(s.biBitCount, 24);
    T_ASSERT_EQ_U(s.stride, 9);   /* (24/8) * 3 */
    /* palette[0] sentinel preserved — 24bpp doesn't copy palette. */
    T_ASSERT_EQ_U(s.palette[0], 0xfe);
    /* Pixels: 3*2*3 = 18 bytes of 0x77. */
    uint8_t *px = (uint8_t *)s.pixels;
    for (int i = 0; i < 18; i++) T_ASSERT_EQ_U(px[i], 0x77);
    bs_release(&s);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_decode_raw_unsupported_depth_releases_pixels(void)
{
    stub_reset();
    /* 16bpp — not handled by FUN_005b7800's raw path. */
    memset(g_raw_resource, 0, sizeof g_raw_resource);
    uint32_t *slots = (uint32_t *)g_raw_resource;
    slots[0] = 0x10; slots[1] = 4; slots[2] = 3;
    *(uint16_t *)(g_raw_resource + 0x0e) = 16;
    stub_register((void *)0x1234, 0x90b, "DATA", g_raw_resource);

    bitmap_session s;
    bs_release_no_free(&s);
    int ok = bs_decode_resource(&s, (void *)0x1234, 0x90b, "DATA", 0);
    T_ASSERT_EQ_I(ok, 0);
    /* The unsupported-depth branch frees the just-allocated pixels. */
    T_ASSERT_EQ_P(s.pixels, NULL);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

/* ─── bs_parse_compressed_header ─────────────────────────────────── */

/* Build a synthetic compressed resource that hits the happy path:
 *   - signature at +0x438 equals base + 0x2711
 *   - pixel offset at +0x450 equals base + small N (≤ 0x80)
 *   - depth byte at +0x430 (16-bit) equals base + bit_count
 *   - width/height indirected slots resolve to base + small ints
 *
 * Simplest construction: use base = 0 — then every "value - base"
 * read returns the literal slot contents.  We still need the
 * indirected reads at +0x420 + (index*4) and +4 + (index*4) to land
 * on slots we control.  We pick the dereference indices to land on
 * tiny stable positions (e.g. index 0 → slot at +0x420, +4). */
static uint8_t g_compressed[2048];

static const void *build_compressed_resource(uint32_t biWidth, uint32_t biHeight,
                                              uint16_t bit_count,
                                              uint32_t pixel_offset_raw)
{
    memset(g_compressed, 0, sizeof g_compressed);
    /* Stamp the embedded 256-RGBQUAD palette at +0x20 FIRST so the
     * pointer-table writes below overwrite it (the parser reads the
     * pointer-table fields, so they must end up with the values we
     * intend; the palette assertions just spot-check entries that
     * don't overlap with reserved slots). */
    for (int i = 0; i < 256; i++) {
        g_compressed[0x20 + i * 4 + 0] = (uint8_t)(i ^ 0x55);
        g_compressed[0x20 + i * 4 + 1] = (uint8_t)(i ^ 0xaa);
        g_compressed[0x20 + i * 4 + 2] = (uint8_t)i;
        g_compressed[0x20 + i * 4 + 3] = 0xcc;
    }

    uint32_t *slots = (uint32_t *)g_compressed;
    slots[0x448 / 4] = 0;                 /* base = 0 */
    slots[0x438 / 4] = 0x2711;            /* signature - base = 0x2711 */
    slots[0x450 / 4] = pixel_offset_raw;  /* must be ≤ 0x80 */
    *(int16_t *)(g_compressed + 0x430) = (int16_t)bit_count;

    /* Width dereference: slots[0x440/4] = INDEX, then slots[(4/4) + INDEX]
     * holds width.  Palette occupies byte +0x20..+0x420 (slots 8..263),
     * so pick INDEX so that 1 + INDEX < 8.  INDEX = 0 → slot[1] (byte
     * +0x04) is safe — outside palette and not a reserved pointer slot. */
    slots[0x440 / 4] = 0;
    slots[1]         = biWidth;

    /* Height dereference: slots[0x18/4] = INDEX, then slots[(0x420/4) + INDEX]
     * holds height.  Slot 264 (byte +0x420) is the first slot past the
     * palette; slots 265..269 are between palette end and the
     * bit_count slot (slot 268 / byte +0x430 — conflict!).  Pick INDEX
     * such that 264+INDEX avoids all the reserved slots:
     *   268 (bit_count), 270 (sig), 272 (width-index), 274 (base),
     *   276 (pixel-off).
     * INDEX = 1 → slot 265 (byte +0x424) is the only valid choice in
     * the immediate post-palette gap. */
    slots[0x18 / 4]            = 1;
    slots[(0x420 / 4) + 1]     = biHeight;

    return g_compressed;
}

int test_compressed_header_signature_mismatch_returns_zero(void)
{
    memset(g_compressed, 0, sizeof g_compressed);
    /* Default 0 everywhere — signature = 0, base = 0, diff = 0 ≠ 0x2711. */
    bs_bitmap_info hdr;
    uint32_t off = 0xdeadbeef;
    int ok = bs_parse_compressed_header(&hdr, g_compressed, &off);
    T_ASSERT_EQ_I(ok, 0);
    /* Out params untouched on failure. */
    T_ASSERT_EQ_U(off, 0xdeadbeef);
    return 0;
}

int test_compressed_header_pixel_offset_too_large(void)
{
    /* Build a valid resource then overwrite the pixel-offset slot
     * with a value > 0x80 to trip the range guard. */
    build_compressed_resource(8, 6, 8, /*pixel_off=*/0x100);
    bs_bitmap_info hdr;
    uint32_t off = 0xdeadbeef;
    int ok = bs_parse_compressed_header(&hdr, g_compressed, &off);
    T_ASSERT_EQ_I(ok, 0);
    T_ASSERT_EQ_U(off, 0xdeadbeef);
    return 0;
}

int test_compressed_header_happy_path(void)
{
    build_compressed_resource(8, 6, 8, /*pixel_off=*/0x20);
    bs_bitmap_info hdr;
    uint32_t off = 0;
    int ok = bs_parse_compressed_header(&hdr, g_compressed, &off);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(hdr.biSize, 0x28);
    T_ASSERT_EQ_U(hdr.biWidth, 8);
    T_ASSERT_EQ_U(hdr.biHeight, 6);
    T_ASSERT_EQ_U(hdr.biPlanes, 1);
    T_ASSERT_EQ_U(hdr.biBitCount, 8);
    T_ASSERT_EQ_U(hdr.biCompression, 0);
    T_ASSERT_EQ_U(hdr.biSizeImage, 8 * 6);   /* (8/8) * 8 * 6 */
    T_ASSERT_EQ_U(off, 0x20);
    /* Palette: spot-check entry 7. */
    T_ASSERT_EQ_U(hdr.palette[7 * 4 + 0], (uint8_t)(7 ^ 0x55));
    T_ASSERT_EQ_U(hdr.palette[7 * 4 + 1], (uint8_t)(7 ^ 0xaa));
    T_ASSERT_EQ_U(hdr.palette[7 * 4 + 2], 7);
    T_ASSERT_EQ_U(hdr.palette[7 * 4 + 3], 0xcc);
    return 0;
}

/* ─── bs_decode_resource compressed path ─────────────────────────── */

int test_decode_compressed_path_8bpp(void)
{
    stub_reset();
    /* Build a compressed resource where pixel data lives at
     * resource + pixel_offset + 0x458.  Place pixel data at a small
     * known offset; pixel_offset = 0 means pixels start at +0x458. */
    build_compressed_resource(2, 2, 8, /*pixel_off=*/0);
    /* 4 pixel bytes (2x2) at offset 0x458. */
    g_compressed[0x458] = 0xa1;
    g_compressed[0x459] = 0xa2;
    g_compressed[0x45a] = 0xa3;
    g_compressed[0x45b] = 0xa4;
    stub_register((void *)0x4444, 0x90b, "DATA", g_compressed);

    bitmap_session s;
    bs_release_no_free(&s);
    int ok = bs_decode_resource(&s, (void *)0x4444, 0x90b, "DATA", /*compressed=*/1);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(s.biWidth, 2);
    T_ASSERT_EQ_U(s.biHeight, 2);
    T_ASSERT_EQ_U(s.biBitCount, 8);
    T_ASSERT_EQ_U(s.stride, 2);
    /* Palette copied from header's embedded palette (entry[3] check). */
    T_ASSERT_EQ_U(s.palette[3 * 4 + 0], (uint8_t)(3 ^ 0x55));
    T_ASSERT_EQ_U(s.palette[3 * 4 + 1], (uint8_t)(3 ^ 0xaa));
    T_ASSERT_EQ_U(s.palette[3 * 4 + 2], 3);
    /* Pixel data placed at +0x458 — must end up in s.pixels. */
    uint8_t *px = (uint8_t *)s.pixels;
    T_ASSERT_EQ_U(px[0], 0xa1);
    T_ASSERT_EQ_U(px[1], 0xa2);
    T_ASSERT_EQ_U(px[2], 0xa3);
    T_ASSERT_EQ_U(px[3], 0xa4);
    bs_release(&s);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_decode_compressed_signature_mismatch_returns_zero(void)
{
    stub_reset();
    /* Provide a resource that's all-zero except registering it — the
     * compressed path will reject it on signature mismatch. */
    memset(g_compressed, 0, sizeof g_compressed);
    stub_register((void *)0x4444, 0x90b, "DATA", g_compressed);

    bitmap_session s;
    bs_release_no_free(&s);
    int ok = bs_decode_resource(&s, (void *)0x4444, 0x90b, "DATA", 1);
    T_ASSERT_EQ_I(ok, 0);
    T_ASSERT_EQ_P(s.pixels, NULL);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

/* ─── ar_palette_session_begin (caller-side wrapper) ─────────────── */

int test_palette_session_begin_emits_BGRA_on_8bpp(void)
{
    stub_reset();
    /* ar_palette_session_begin uses the COMPRESSED decoder path
     * (FUN_005b7800's compressed_flag=1) — must build the resource
     * that way.  Builder stamps palette entry[i] = (i^0x55, i^0xaa,
     * i, 0xcc) inside the resource at +0x20. */
    void *sotesp = (void *)0xabcd;
    build_compressed_resource(2, 2, /*bpp=*/8, /*pixel_off=*/0);
    /* Pixel bytes at +0x458 (pixel_off=0). */
    g_compressed[0x458] = 0;
    g_compressed[0x459] = 0;
    g_compressed[0x45a] = 0;
    g_compressed[0x45b] = 0;
    stub_register(sotesp, 0x90b, "DATA", g_compressed);

    ar_sprite_slot slot = {
        .settings    = sotesp,
        .resource_id = 0x90b,
    };
    uint8_t out[1024] = {0};
    bool ok = ar_palette_session_begin(&slot, out);
    T_ASSERT(ok);
    /* Source palette entry[0] = (0^0x55=0x55, 0^0xaa=0xaa, 0, 0xcc);
     * BGRA swap → (0, 0xaa, 0x55, 0). */
    T_ASSERT_EQ_U(out[0], 0);
    T_ASSERT_EQ_U(out[1], 0xaa);
    T_ASSERT_EQ_U(out[2], 0x55);
    T_ASSERT_EQ_U(out[3], 0);
    /* entry[5] source = (5^0x55=0x50, 5^0xaa=0xaf, 5, 0xcc); BGRA
     * swap → (5, 0xaf, 0x50, 0). */
    T_ASSERT_EQ_U(out[5 * 4 + 0], 5);
    T_ASSERT_EQ_U(out[5 * 4 + 1], 0xaf);
    T_ASSERT_EQ_U(out[5 * 4 + 2], 0x50);
    /* No allocation leaks — the session's pixel buffer must be freed
     * before return. */
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_palette_session_begin_returns_false_on_missing_resource(void)
{
    stub_reset();
    ar_sprite_slot slot = {
        .settings    = (void *)0x9999,
        .resource_id = 0x90b,
    };
    uint8_t out[1024];
    memset(out, 0xfe, sizeof out);
    bool ok = ar_palette_session_begin(&slot, out);
    T_ASSERT(!ok);
    /* On failure the docstring promises out is left undefined; we
     * just check no allocation leaked. */
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_palette_session_begin_returns_false_on_24bpp(void)
{
    stub_reset();
    /* Build a VALID compressed resource at 24bpp.  Decoder will
     * succeed, but ar_palette_session_begin skips the BGRA emit
     * because bit_count != 8. */
    void *sotesp = (void *)0x5555;
    build_compressed_resource(2, 2, /*bpp=*/24, /*pixel_off=*/0);
    /* 24bpp 2×2 = 12 pixel bytes at +0x458. */
    memset(g_compressed + 0x458, 0, 12);
    stub_register(sotesp, 0x90b, "DATA", g_compressed);

    ar_sprite_slot slot = { .settings = sotesp, .resource_id = 0x90b };
    uint8_t out[1024] = {0};
    bool ok = ar_palette_session_begin(&slot, out);
    T_ASSERT(!ok);
    /* out untouched — emitter never ran. */
    for (size_t i = 0; i < sizeof out; i++) T_ASSERT_EQ_U(out[i], 0);
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

/* ─── ar_register_main_sprites palette-ramp wiring ───────────────── */

int test_main_sprites_installs_palette_when_resource_8bpp(void)
{
    stub_reset();
    ar_state_init();

    void *sotesp = (void *)0xabcd;
    /* Use compressed builder — that's what ar_palette_session_begin
     * triggers (compressed_flag=1).  Palette pattern per the builder:
     * entry[i] = (i^0x55, i^0xaa, i, 0xcc). */
    build_compressed_resource(2, 2, /*bpp=*/8, /*pixel_off=*/0);
    memset(g_compressed + 0x458, 0, 4);
    stub_register(sotesp, 0x90b, "DATA", g_compressed);

    ar_register_main_sprites((void *)0x1, /*group=*/0x4, (void *)0x2, sotesp);

    /* Palette installed: slot[0].entries[0].b is the 1024-byte palette. */
    ar_sprite_slot *s = &g_ar_sprite_slots[0];
    T_ASSERT(s->entries != NULL);
    T_ASSERT(s->entries[0].b != NULL);
    const uint8_t *p = (const uint8_t *)s->entries[0].b;
    /* palette[1] override = 0 (BGR0). */
    T_ASSERT_EQ_U(p[1 * 4 + 0], 0);
    T_ASSERT_EQ_U(p[1 * 4 + 1], 0);
    T_ASSERT_EQ_U(p[1 * 4 + 2], 0);
    /* palette[45] override = 0x383838 (one of the 10 dark-gray entries). */
    T_ASSERT_EQ_U(p[45 * 4 + 0], 0x38);
    T_ASSERT_EQ_U(p[45 * 4 + 1], 0x38);
    T_ASSERT_EQ_U(p[45 * 4 + 2], 0x38);
    /* palette[70] override = ar_color_lerp(0x383838, 0xffffff, 20, 20) = 0xffffff. */
    T_ASSERT_EQ_U(p[70 * 4 + 0], 0xff);
    T_ASSERT_EQ_U(p[70 * 4 + 1], 0xff);
    T_ASSERT_EQ_U(p[70 * 4 + 2], 0xff);
    /* palette[51] override = ar_color_lerp(0x383838, 0xffffff, 1, 20) — first lerp step
     * = 0x38 + (0xff - 0x38) * 1 / 20 = 0x38 + 0x09 = 0x41 per channel. */
    T_ASSERT_EQ_U(p[51 * 4 + 0], 0x41);
    T_ASSERT_EQ_U(p[51 * 4 + 1], 0x41);
    T_ASSERT_EQ_U(p[51 * 4 + 2], 0x41);
    /* Untouched entry comes from the decoder's palette → BGRA-swapped
     * builder seed.  entry[0] source = (0x55, 0xaa, 0, 0xcc) → BGRA
     * (0, 0xaa, 0x55, 0). */
    T_ASSERT_EQ_U(p[0 * 4 + 0], 0);
    T_ASSERT_EQ_U(p[0 * 4 + 1], 0xaa);
    T_ASSERT_EQ_U(p[0 * 4 + 2], 0x55);

    /* Teardown: free all sprite slots' owned buffers. */
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    }
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

int test_main_sprites_skips_palette_when_resource_missing(void)
{
    stub_reset();
    ar_state_init();

    /* Don't register sotesp/0x90b — the decoder will fail and the
     * ramp section becomes a no-op. */
    ar_register_main_sprites((void *)0x1, /*group=*/0x4, (void *)0x2,
                             /*sotesp=*/(void *)0xabcd);

    ar_sprite_slot *s = &g_ar_sprite_slots[0];
    T_ASSERT(s->entries != NULL);
    /* Palette not installed — entries[0].b stays NULL. */
    T_ASSERT_EQ_P(s->entries[0].b, NULL);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    }
    T_ASSERT_EQ_I(g_live_allocs, 0);
    return 0;
}

/* ─── layout ─────────────────────────────────────────────────────── */

int test_bitmap_session_layout_matches_retail(void)
{
    /* 32-bit-only — the struct fields are sized for a 32-bit target. */
    if (sizeof(void *) != 4) {
        T_SKIP("bitmap_session layout asserts are 32-bit only "
               "(sizeof(void*) = %zu)", sizeof(void *));
    }
    T_ASSERT_EQ_U(sizeof(bitmap_session), 0x434);
    T_ASSERT_EQ_U(offsetof(bitmap_session, pixels), 0x00);
    T_ASSERT_EQ_U(offsetof(bitmap_session, stride), 0x04);
    T_ASSERT_EQ_U(offsetof(bitmap_session, biSize), 0x0c);
    T_ASSERT_EQ_U(offsetof(bitmap_session, biBitCount), 0x1a);
    T_ASSERT_EQ_U(offsetof(bitmap_session, palette), 0x34);
    T_ASSERT_EQ_U(sizeof(bs_bitmap_info), 0x428);
    T_ASSERT_EQ_U(offsetof(bs_bitmap_info, palette), 0x28);
    return 0;
}
