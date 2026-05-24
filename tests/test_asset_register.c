/*
 * tests/test_asset_register.c — host-side tests for the asset-register
 * port (src/asset_register.c).
 *
 * Provides recording stubs for the four GDI primitives ar_make_font /
 * set_pen / set_brush / set_pen_gradient need.  Every call appends a
 * tagged record into a per-kind log; the tests then assert against
 * those logs.  Handles returned are opaque pointers into a per-test
 * counter — the destroy paths see them, log a "delete N" entry, and
 * we can verify destruct order too.
 *
 * The stubs replace `asset_register_win32.c` for the test build (see
 * tests/Makefile — only `asset_register.c` is pulled into SRCS_PORTED).
 */
#include "../src/asset_register.h"
#include "t.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── recording GDI stub ─────────────────────────────────────────── */

typedef enum stub_kind {
    STUB_FONT = 1,
    STUB_PEN  = 2,
    STUB_BRUSH = 3,
    STUB_DELETE = 4,
} stub_kind;

typedef struct stub_event {
    stub_kind kind;
    int32_t   arg_a;     /* font: width      | pen: style     | brush: -          | delete: handle-id */
    int32_t   arg_b;     /* font: height     | pen: width     | brush: -          | delete: 0 */
    uint32_t  arg_c;     /* font: italic     | pen: color     | brush: color      | delete: 0 */
    char      face[32];  /* font: face name (truncated)                                       */
    void     *handle;    /* the handle we returned / are deleting                              */
} stub_event;

enum { STUB_LOG_MAX = 256 };
static stub_event g_log[STUB_LOG_MAX];
static int        g_log_count = 0;
static uintptr_t  g_next_handle = 0;
static const char *g_fallback = NULL;

static void stub_reset(void)
{
    memset(g_log, 0, sizeof g_log);
    g_log_count   = 0;
    g_next_handle = 0;
    g_fallback    = NULL;
}

static void *stub_alloc_handle(void)
{
    g_next_handle++;
    /* Return small but non-NULL, non-aligned pointers — they're never
     * dereferenced by the port.  Cast through uintptr_t to dodge
     * UBSan's "non-aligned pointer" complaints. */
    return (void *)(uintptr_t)(0xF000 + g_next_handle);
}

ar_gdi_handle ar_gdi_create_font(int32_t width, int32_t height,
                                  int italic, const char *face)
{
    if (g_log_count >= STUB_LOG_MAX) abort();
    stub_event *e = &g_log[g_log_count++];
    e->kind  = STUB_FONT;
    e->arg_a = width;
    e->arg_b = height;
    e->arg_c = (uint32_t)italic;
    if (face != NULL) {
        strncpy(e->face, face, sizeof e->face - 1);
        e->face[sizeof e->face - 1] = '\0';
    }
    e->handle = stub_alloc_handle();
    return e->handle;
}

ar_gdi_handle ar_gdi_create_pen(int style, int32_t width, uint32_t color)
{
    if (g_log_count >= STUB_LOG_MAX) abort();
    stub_event *e = &g_log[g_log_count++];
    e->kind  = STUB_PEN;
    e->arg_a = style;
    e->arg_b = width;
    e->arg_c = color;
    e->handle = stub_alloc_handle();
    return e->handle;
}

ar_gdi_handle ar_gdi_create_brush(uint32_t color)
{
    if (g_log_count >= STUB_LOG_MAX) abort();
    stub_event *e = &g_log[g_log_count++];
    e->kind  = STUB_BRUSH;
    e->arg_c = color;
    e->handle = stub_alloc_handle();
    return e->handle;
}

void ar_gdi_delete(ar_gdi_handle h)
{
    if (g_log_count >= STUB_LOG_MAX) abort();
    stub_event *e = &g_log[g_log_count++];
    e->kind   = STUB_DELETE;
    e->arg_a  = (int32_t)(uintptr_t)h;
    e->handle = h;
}

const char *ar_gdi_get_fallback_face_name(void)
{
    return g_fallback;
}

/* ─── helpers ────────────────────────────────────────────────────── */

/* Count events of one kind in the log. */
static int stub_count_kind(stub_kind k)
{
    int n = 0;
    for (int i = 0; i < g_log_count; i++) if (g_log[i].kind == k) n++;
    return n;
}

/* Find the i-th event of a kind (0-based). */
static const stub_event *stub_nth(stub_kind k, int idx)
{
    int n = 0;
    for (int i = 0; i < g_log_count; i++) {
        if (g_log[i].kind == k) {
            if (n == idx) return &g_log[i];
            n++;
        }
    }
    return NULL;
}

/* ─── ar_xfree / ar_color_lerp ───────────────────────────────────── */

int test_ar_xfree_null_safe(void)
{
    /* ar_xfree(NULL) must not crash — `free(NULL)` is well-defined. */
    ar_xfree(NULL);
    return 0;
}

int test_ar_color_lerp_endpoints(void)
{
    /* num == 0 → src exactly.  num == denom → dst exactly. */
    T_ASSERT_EQ_U(ar_color_lerp(0x102030, 0xa0b0c0,  0, 100), 0x102030u);
    T_ASSERT_EQ_U(ar_color_lerp(0x102030, 0xa0b0c0, 100, 100), 0xa0b0c0u);
    return 0;
}

int test_ar_color_lerp_midpoint(void)
{
    /* Half-way: each channel = (src+dst)/2 (truncated toward zero).
     *   R: (0x10 + 0xa0)/2 = 0x58
     *   G: (0x20 + 0xb0)/2 = 0x68
     *   B: (0x30 + 0xc0)/2 = 0x78
     * No alpha byte (always 0). */
    T_ASSERT_EQ_U(ar_color_lerp(0x102030, 0xa0b0c0, 1, 2), 0x586878u);
    return 0;
}

int test_ar_color_lerp_descending(void)
{
    /* dst < src per channel — the diff goes negative and the
     * /denom keeps the sign, so we lerp downward. */
    T_ASSERT_EQ_U(ar_color_lerp(0xa0a0a0, 0x202020, 1, 2), 0x606060u);
    return 0;
}

int test_ar_color_lerp_ignores_alpha(void)
{
    /* Top byte (shift 24) is never read or written.  Result alpha
     * byte should be 0 even when both inputs have alpha set.
     * Midpoint per channel:
     *   R: (0x11 + 0x44)/2 = 0x2a   G: (0x22 + 0x55)/2 = 0x3b
     *   B: (0x33 + 0x66)/2 = 0x4c                                 */
    uint32_t out = ar_color_lerp(0xff112233, 0xff445566, 1, 2);
    T_ASSERT_EQ_U(out & 0xff000000u, 0u);
    T_ASSERT_EQ_U(out & 0x00ffffffu, 0x2a3b4cu);
    return 0;
}

/* ─── ar_palette_pack_entry (FUN_005b5d90) ───────────────────────── */

int test_palette_pack_entry_basic(void)
{
    /* COLORREF 0x00BBGGRR → PALETTEENTRY {peRed=R, peGreen=G, peBlue=B, 0}.
     * 0x00112233 should pack as 0x33, 0x22, 0x11, 0x00 (R,G,B,peFlags). */
    uint8_t out[4] = {0xee, 0xee, 0xee, 0xee};
    ar_palette_pack_entry(out, 0x00112233u);
    T_ASSERT_EQ_U(out[0], 0x33u);
    T_ASSERT_EQ_U(out[1], 0x22u);
    T_ASSERT_EQ_U(out[2], 0x11u);
    T_ASSERT_EQ_U(out[3], 0x00u);
    return 0;
}

int test_palette_pack_entry_ignores_top_byte(void)
{
    /* The top byte of `colorref` is dropped — the retail body only
     * shifts by 0/8/16.  A poisoned top byte must not leak into out[3]. */
    uint8_t out[4] = {0};
    ar_palette_pack_entry(out, 0xff808080u);
    T_ASSERT_EQ_U(out[0], 0x80u);
    T_ASSERT_EQ_U(out[1], 0x80u);
    T_ASSERT_EQ_U(out[2], 0x80u);
    T_ASSERT_EQ_U(out[3], 0x00u);  /* peFlags forced to 0, not 0xff */
    return 0;
}

int test_palette_pack_entry_overwrites_existing(void)
{
    /* Writes are unconditional — pre-existing bytes get replaced. */
    uint8_t out[4] = {0xaa, 0xbb, 0xcc, 0xdd};
    ar_palette_pack_entry(out, 0u);
    T_ASSERT_EQ_U(out[0], 0x00u);
    T_ASSERT_EQ_U(out[1], 0x00u);
    T_ASSERT_EQ_U(out[2], 0x00u);
    T_ASSERT_EQ_U(out[3], 0x00u);
    return 0;
}

/* ─── ar_palette_install (FUN_00491770) ──────────────────────────── */

static void make_palette(uint8_t pal[1024], uint8_t seed)
{
    for (int i = 0; i < 1024; i++) pal[i] = (uint8_t)(seed + (uint8_t)i);
}

int test_palette_install_lazy_allocates_first_time(void)
{
    /* On a fresh slot with entry[0].b == NULL, the helper should
     * allocate a 1024-byte buffer and copy the palette in. */
    ar_sprite_slot s = {0};
    s.entries     = (ar_sprite_entry *)calloc(1, sizeof(ar_sprite_entry));
    s.entry_count = 1;

    uint8_t pal[1024];
    make_palette(pal, 0x10);

    ar_palette_install(&s, pal);

    T_ASSERT(s.entries[0].b != NULL);
    T_ASSERT_MEM_EQ(s.entries[0].b, pal, 1024);

    ar_sprite_slot_destroy(&s);   /* free without leaks (ASan-checked) */
    return 0;
}

int test_palette_install_reuses_existing_buffer(void)
{
    /* On a second call the helper must reuse the existing buffer (no
     * realloc).  Pin this by capturing the pointer before and after. */
    ar_sprite_slot s = {0};
    s.entries     = (ar_sprite_entry *)calloc(1, sizeof(ar_sprite_entry));
    s.entry_count = 1;

    uint8_t pal_a[1024], pal_b[1024];
    make_palette(pal_a, 0x10);
    make_palette(pal_b, 0xa0);

    ar_palette_install(&s, pal_a);
    void *first = s.entries[0].b;
    ar_palette_install(&s, pal_b);
    void *second = s.entries[0].b;

    T_ASSERT_EQ_P(first, second);
    T_ASSERT_MEM_EQ(s.entries[0].b, pal_b, 1024);

    ar_sprite_slot_destroy(&s);
    return 0;
}

int test_palette_install_destroy_frees_buffer(void)
{
    /* The destructor already frees entry[0].b — confirm the round-trip
     * (install then destroy) leaves no leaks.  ASan does the work; this
     * test just exercises the code path. */
    ar_sprite_slot s = {0};
    s.entries     = (ar_sprite_entry *)calloc(1, sizeof(ar_sprite_entry));
    s.entry_count = 1;

    uint8_t pal[1024];
    make_palette(pal, 0x55);
    ar_palette_install(&s, pal);
    ar_sprite_slot_destroy(&s);

    T_ASSERT_EQ_P(s.entries, NULL);
    return 0;
}

/* ─── ar_sprite_slot_destroy ─────────────────────────────────────── */

int test_sprite_destroy_frees_aux_and_entries(void)
{
    ar_sprite_slot s = {0};
    s.aux_buf     = malloc(16);
    s.entry_count = 2;
    s.entries     = (ar_sprite_entry *)calloc(2, sizeof(ar_sprite_entry));
    s.entries[0].b = malloc(8);
    s.entries[1].b = NULL;            /* second entry has no owned ptr */

    /* If any of the malloc'd buffers leak, ASan will fail the test. */
    ar_sprite_slot_destroy(&s);

    T_ASSERT_EQ_P(s.aux_buf, NULL);
    T_ASSERT_EQ_P(s.entries, NULL);
    return 0;
}

int test_sprite_destroy_safe_on_zero_slot(void)
{
    ar_sprite_slot s = {0};   /* BSS-zero — no pointers to free. */
    ar_sprite_slot_destroy(&s);
    T_ASSERT_EQ_P(s.aux_buf, NULL);
    T_ASSERT_EQ_P(s.entries, NULL);
    return 0;
}

/* ─── ar_sprite_slot_register (FUN_005748c0) ─────────────────────── */

int test_sprite_register_writes_all_named_fields(void)
{
    /* Fresh slot — verify every named field gets the value we pass and
     * every "cleared" field is zero.  This pins the field map against
     * FUN_005748c0's retail write set. */
    ar_sprite_slot s = {0};
    /* Pre-poison the cleared fields to prove the register actively
     * clears them rather than relying on prior zeros. */
    s.f_08 = 0xdeadu;
    s.f_18 = 0xbeefu;
    s.f_38 = 0xcafeu;

    ar_sprite_slot_register(&s,
        /*zdd=*/(void *)0x1111, /*settings=*/(void *)0x2222,
        /*resource_id=*/0x90b,
        /*width=*/0x20, /*height=*/0x20,
        /*colorkey=*/0,
        /*scale_flag=*/0,
        /*type=*/2,
        /*group=*/0x55);

    T_ASSERT_EQ_P(s.zdd, (void *)0x1111);
    T_ASSERT_EQ_P(s.settings, (void *)0x2222);
    T_ASSERT_EQ_U(s.resource_id, 0x90bu);
    T_ASSERT_EQ_U(s.width, 0x20u);
    T_ASSERT_EQ_U(s.height, 0x20u);
    T_ASSERT_EQ_U(s.colorkey, 0u);
    T_ASSERT_EQ_U(s.scale_flag, 0u);
    T_ASSERT_EQ_U(s.type, 2u);
    T_ASSERT_EQ_U(s.group, 0x55u);

    /* Cleared fields. */
    T_ASSERT_EQ_U(s.f_08, 0u);
    T_ASSERT_EQ_U(s.f_18, 0u);
    T_ASSERT_EQ_U(s.f_38, 0u);

    /* entries: a 1-entry calloc'd array. */
    T_ASSERT_EQ_U(s.entry_count, 1u);
    T_ASSERT(s.entries != NULL);
    T_ASSERT_EQ_U(s.entries[0].a, 0u);
    T_ASSERT_EQ_P(s.entries[0].b, NULL);

    ar_sprite_slot_destroy(&s);
    return 0;
}

int test_sprite_register_frees_existing_aux_and_entries(void)
{
    /* Re-registering a slot that already has aux_buf + entries: the
     * prologue (FUN_005748c0's inline destroy) must free both — ASan
     * catches the leak if we forget. */
    ar_sprite_slot s = {0};
    s.aux_buf     = malloc(32);
    s.entry_count = 2;
    s.entries     = (ar_sprite_entry *)calloc(2, sizeof(ar_sprite_entry));
    s.entries[0].b = malloc(16);
    s.entries[1].b = malloc(16);

    ar_sprite_slot_register(&s, NULL, NULL, /*id=*/0x100,
                            /*w=*/32, /*h=*/32, /*ck=*/0,
                            /*scale=*/0, /*type=*/0, /*group=*/0);

    /* aux_buf was destroyed and never re-populated. */
    T_ASSERT_EQ_P(s.aux_buf, NULL);
    /* entries got reallocated (count=1).  The old 2-entry buffer was
     * freed first; its `b` pointers were each freed too — any leak
     * would surface in ASan. */
    T_ASSERT_EQ_U(s.entry_count, 1u);
    T_ASSERT(s.entries != NULL);

    ar_sprite_slot_destroy(&s);
    return 0;
}

int test_sprite_register_truncates_id_and_group_to_uint16(void)
{
    /* Retail stores resource_id at +0x40 (u16) and group at +0x42 (u16).
     * The function takes them as uint16_t — passing values that would
     * "look" larger in retail's int param goes through the implicit
     * narrowing.  Pin the narrowing behaviour. */
    ar_sprite_slot s = {0};
    ar_sprite_slot_register(&s, NULL, NULL,
                            /*resource_id=*/(uint16_t)0xffff,
                            /*w=*/1, /*h=*/1, /*ck=*/0,
                            /*scale=*/0, /*type=*/0,
                            /*group=*/(uint16_t)0xabcd);
    T_ASSERT_EQ_U(s.resource_id, 0xffffu);
    T_ASSERT_EQ_U(s.group, 0xabcdu);

    ar_sprite_slot_destroy(&s);
    return 0;
}

int test_sprite_register_matches_FUN_005748c0_arg_shape(void)
{
    /* The exact arg shape FUN_0057a330 passes for the first sprite:
     *   FUN_005748c0(zdd, settings, 0x413, 0x18, 0x18, 0, 0, 2, group)
     * Cross-checks that the field map (id, w, h, colorkey, scale,
     * type, group) matches retail's literal call. */
    ar_sprite_slot s = {0};
    ar_sprite_slot_register(&s, (void *)0xabc, (void *)0xdef,
                            /*id=*/0x413,
                            /*w=*/0x18, /*h=*/0x18,
                            /*colorkey=*/0,
                            /*scale_flag=*/0,
                            /*type=*/2,
                            /*group=*/0x7);
    T_ASSERT_EQ_U(s.resource_id, 0x413u);
    T_ASSERT_EQ_U(s.width, 0x18u);
    T_ASSERT_EQ_U(s.height, 0x18u);
    T_ASSERT_EQ_U(s.type, 2u);

    ar_sprite_slot_destroy(&s);
    return 0;
}

/* ─── ar_sprite_slot_clone (FUN_00582b80) ────────────────────────── */

int test_sprite_clone_copies_all_metadata(void)
{
    /* Build a populated source slot, then clone into a fresh dst.
     * Verify every metadata field arrives intact. */
    ar_sprite_slot src = {0};
    ar_sprite_slot_register(&src,
        /*zdd=*/(void *)0xaaaa, /*settings=*/(void *)0xbbbb,
        /*resource_id=*/0x590,
        /*width=*/0x20, /*height=*/0x20,
        /*colorkey=*/0x1ffffff,
        /*scale_flag=*/1,
        /*type=*/0,
        /*group=*/3);

    ar_sprite_slot dst = {0};
    ar_sprite_slot_clone(&dst, &src);

    T_ASSERT_EQ_P(dst.zdd, (void *)0xaaaa);
    T_ASSERT_EQ_P(dst.settings, (void *)0xbbbb);
    T_ASSERT_EQ_U(dst.resource_id, 0x590u);
    T_ASSERT_EQ_U(dst.width, 0x20u);
    T_ASSERT_EQ_U(dst.height, 0x20u);
    T_ASSERT_EQ_U(dst.colorkey, 0x1ffffffu);
    T_ASSERT_EQ_U(dst.scale_flag, 1u);
    T_ASSERT_EQ_U(dst.type, 0u);
    T_ASSERT_EQ_U(dst.group, 3u);

    /* Cleared fields. */
    T_ASSERT_EQ_U(dst.f_08, 0u);
    T_ASSERT_EQ_U(dst.f_18, 0u);
    T_ASSERT_EQ_U(dst.f_38, 0u);

    /* dst got a fresh 1-entry entries array — NOT shared with src. */
    T_ASSERT_EQ_U(dst.entry_count, 1u);
    T_ASSERT(dst.entries != NULL);
    T_ASSERT(dst.entries != src.entries);
    T_ASSERT_EQ_U(dst.entries[0].a, 0u);
    T_ASSERT_EQ_P(dst.entries[0].b, NULL);

    /* No aux_buf on src → none on dst. */
    T_ASSERT_EQ_P(dst.aux_buf, NULL);

    /* src is untouched; its entries array still owns the original alloc. */
    T_ASSERT(src.entries != NULL);
    T_ASSERT_EQ_U(src.entry_count, 1u);

    ar_sprite_slot_destroy(&src);
    ar_sprite_slot_destroy(&dst);
    return 0;
}

int test_sprite_clone_frees_dst_existing_aux_and_entries(void)
{
    /* Re-cloning over a populated dst must free its old aux_buf and
     * entries (each entry's `b` too) — ASan would catch a leak. */
    ar_sprite_slot src = {0};
    ar_sprite_slot_register(&src, (void *)0x1, (void *)0x2,
                            /*id=*/0x100,
                            /*w=*/32, /*h=*/32, /*ck=*/0,
                            /*scale=*/0, /*type=*/0, /*group=*/0);

    ar_sprite_slot dst = {0};
    dst.aux_buf     = malloc(48);
    dst.entry_count = 2;
    dst.entries     = (ar_sprite_entry *)calloc(2, sizeof(ar_sprite_entry));
    dst.entries[0].b = malloc(16);
    dst.entries[1].b = malloc(16);

    ar_sprite_slot_clone(&dst, &src);

    /* aux_buf freed; no copy since src has none either. */
    T_ASSERT_EQ_P(dst.aux_buf, NULL);
    /* entries array reallocated to size 1. */
    T_ASSERT_EQ_U(dst.entry_count, 1u);
    T_ASSERT(dst.entries != NULL);

    ar_sprite_slot_destroy(&src);
    ar_sprite_slot_destroy(&dst);
    return 0;
}

int test_sprite_clone_deep_copies_aux_buf(void)
{
    /* Build a source with an aux_buf of N*24 bytes — clone must
     * allocate a separate buffer on dst and copy the bytes verbatim.
     * Verify the contents match, then mutate src's buffer and confirm
     * dst's stays unchanged (proves they are independent allocations). */
    ar_sprite_slot src = {0};
    ar_sprite_slot_register(&src, NULL, NULL,
                            /*id=*/0x100, /*w=*/64, /*h=*/64,
                            /*ck=*/0, /*scale=*/0, /*type=*/0, /*group=*/0);

    enum { N = 3 };
    src.aux_buf = malloc(N * 24);
    /* Populate with a recognizable pattern. */
    for (size_t i = 0; i < N * 24; i++) {
        ((uint8_t *)src.aux_buf)[i] = (uint8_t)(0x10 + i);
    }
    /* Retail clone reads count from src->f_38 — stamp it so the deep
     * copy walks the full buffer. */
    src.f_38 = N;

    ar_sprite_slot dst = {0};
    ar_sprite_slot_clone(&dst, &src);

    /* dst got its own aux_buf (not the same pointer as src). */
    T_ASSERT(dst.aux_buf != NULL);
    T_ASSERT(dst.aux_buf != src.aux_buf);
    /* Contents match byte-for-byte. */
    T_ASSERT(memcmp(dst.aux_buf, src.aux_buf, N * 24) == 0);

    /* Retail quirk: dst->f_38 stays 0 even when aux is copied.  The
     * count is NOT propagated.  This pins that quirk. */
    T_ASSERT_EQ_U(dst.f_38, 0u);

    /* Mutate src — dst stays unchanged. */
    ((uint8_t *)src.aux_buf)[5] = 0xee;
    T_ASSERT_EQ_U(((uint8_t *)dst.aux_buf)[5], 0x15u);

    ar_sprite_slot_destroy(&src);
    ar_sprite_slot_destroy(&dst);
    return 0;
}

int test_sprite_clone_no_aux_when_src_aux_null(void)
{
    /* src->aux_buf == NULL → dst->aux_buf must stay NULL (regardless
     * of src->f_38 value).  Guards against allocating bogus storage. */
    ar_sprite_slot src = {0};
    ar_sprite_slot_register(&src, NULL, NULL,
                            /*id=*/0x42, /*w=*/8, /*h=*/8,
                            /*ck=*/0, /*scale=*/0, /*type=*/0, /*group=*/0);
    src.f_38 = 5;  /* should be ignored when aux_buf is NULL */

    ar_sprite_slot dst = {0};
    ar_sprite_slot_clone(&dst, &src);

    T_ASSERT_EQ_P(dst.aux_buf, NULL);

    ar_sprite_slot_destroy(&src);
    ar_sprite_slot_destroy(&dst);
    return 0;
}

int test_sprite_clone_resets_dst_id_and_group_to_uint16(void)
{
    /* Confirm truncation behaviour for the two u16 fields when src
     * carries the maximum value. */
    ar_sprite_slot src = {0};
    ar_sprite_slot_register(&src, NULL, NULL,
                            /*id=*/0xffff, /*w=*/16, /*h=*/16,
                            /*ck=*/0, /*scale=*/0, /*type=*/0,
                            /*group=*/0xffff);

    ar_sprite_slot dst = {0};
    ar_sprite_slot_clone(&dst, &src);

    T_ASSERT_EQ_U(dst.resource_id, 0xffffu);
    T_ASSERT_EQ_U(dst.group,       0xffffu);

    ar_sprite_slot_destroy(&src);
    ar_sprite_slot_destroy(&dst);
    return 0;
}

/* ─── ar_info_entry_clear (FUN_00582d00) ─────────────────────────── */

int test_info_entry_clear_zeroes_marker_flag_data_palette(void)
{
    /* All four written fields go to 0/NULL. */
    ar_info_entry e;
    e.marker  = 0x1234;
    e.flag    = 0xdeadbeefu;
    e.data    = (const void *)0xcafebabeu;
    e.palette = (void *)0x55555555u;
    e.f_10    = 0x12345678u;  /* outside the 14-byte clear window */

    ar_info_entry_clear(&e);

    T_ASSERT_EQ_U(e.marker, 0u);
    T_ASSERT_EQ_U(e.flag,   0u);
    T_ASSERT_EQ_P(e.data,   NULL);
    T_ASSERT_EQ_P(e.palette, NULL);
    /* +0x10 lives past the 14-byte clear stride — must stay intact. */
    T_ASSERT_EQ_U(e.f_10,   0x12345678u);
    return 0;
}

int test_info_entry_clear_leaves_pad_untouched(void)
{
    /* Retail writes `ax` to +0 (not eax), so the +2..+3 pad bytes
     * stay at whatever they were.  Pin that — the pad is part of the
     * 20-byte struct and the rest of FUN_0057ca40's copy chain never
     * touches it either. */
    ar_info_entry e;
    e.marker  = 0xabcd;
    e._pad02  = 0xface;
    e.flag    = 1u;
    e.data    = (const void *)0xfeedfaceu;
    e.palette = (void *)0x9u;

    ar_info_entry_clear(&e);

    T_ASSERT_EQ_U(e._pad02, 0xfaceu);
    /* Sanity: the others did clear. */
    T_ASSERT_EQ_U(e.marker, 0u);
    T_ASSERT_EQ_U(e.flag,   0u);
    T_ASSERT_EQ_P(e.data,   NULL);
    T_ASSERT_EQ_P(e.palette, NULL);
    return 0;
}

int test_info_entry_pool_wired_by_state_init(void)
{
    /* ar_state_init mimics FUN_00562ea0:225-253's pool-init loop:
     * every g_ar_info_table[i] should point into g_ar_info_entries[],
     * and every entry should be zeroed. */
    ar_state_init();
    for (int i = 0; i < AR_INFO_ENTRY_COUNT; i++) {
        T_ASSERT_EQ_P(g_ar_info_table[i], &g_ar_info_entries[i]);
        T_ASSERT_EQ_U(g_ar_info_entries[i].marker, 0u);
        T_ASSERT_EQ_U(g_ar_info_entries[i].flag,   0u);
        T_ASSERT_EQ_P(g_ar_info_entries[i].data,   NULL);
        T_ASSERT_EQ_P(g_ar_info_entries[i].palette, NULL);
        T_ASSERT_EQ_U(g_ar_info_entries[i].f_10,   0u);
    }
    /* Ramp-flag base maps to retail BSS 0x008a8578 — i.e. pool[78]. */
    T_ASSERT_EQ_I(AR_INFO_RAMP_FLAGS_BASE, 78);
    T_ASSERT_EQ_I(AR_INFO_RAMP_FLAGS_COUNT, 14);
    return 0;
}

/* ─── ar_gdi_slot_destroy / reset ────────────────────────────────── */

int test_gdi_destroy_deletes_each_handle(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    /* Manually populate as if a setter ran with capacity=3. */
    s.array    = (ar_gdi_handle *)calloc(3, sizeof(ar_gdi_handle));
    s.array[0] = (ar_gdi_handle)0x1111;
    s.array[1] = NULL;                /* hole: should NOT be deleted */
    s.array[2] = (ar_gdi_handle)0x3333;
    s.count    = 3;
    s.capacity = 3;

    ar_gdi_slot_destroy(&s);

    T_ASSERT_EQ_I(stub_count_kind(STUB_DELETE), 2);
    T_ASSERT_EQ_U(stub_nth(STUB_DELETE, 0)->arg_a, 0x1111u);
    T_ASSERT_EQ_U(stub_nth(STUB_DELETE, 1)->arg_a, 0x3333u);
    T_ASSERT_EQ_U(s.capacity, 0u);
    T_ASSERT_EQ_U(s.count, 0u);
    T_ASSERT_EQ_P(s.array, NULL);
    return 0;
}

int test_gdi_reset_clears_then_allocates(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    s.array      = (ar_gdi_handle *)calloc(2, sizeof(ar_gdi_handle));
    s.array[0]   = (ar_gdi_handle)0xa1a1;
    s.array[1]   = (ar_gdi_handle)0xb2b2;
    s.count      = 2;
    s.capacity   = 2;
    s.group      = 0x1234;

    ar_gdi_slot_reset(&s, 5);

    /* Two deletes from the pre-existing handles, then a fresh
     * allocation of 5 entries (no GDI calls). */
    T_ASSERT_EQ_I(stub_count_kind(STUB_DELETE), 2);
    T_ASSERT_EQ_U(s.capacity, 5u);
    T_ASSERT_EQ_U(s.count, 0u);
    T_ASSERT(s.array != NULL);
    /* `group` must be untouched — reset only handles the array. */
    T_ASSERT_EQ_U(s.group, 0x1234u);
    /* New array must be zeroed. */
    for (int i = 0; i < 5; i++) T_ASSERT_EQ_P(s.array[i], NULL);

    ar_gdi_slot_destroy(&s);
    return 0;
}

/* ─── ar_make_font ───────────────────────────────────────────────── */

int test_make_font_family_courier(void)
{
    stub_reset();
    (void)ar_make_font(7, 14, 0);
    T_ASSERT_EQ_I(stub_count_kind(STUB_FONT), 1);
    const stub_event *e = stub_nth(STUB_FONT, 0);
    T_ASSERT_EQ_I(e->arg_a, 7);
    T_ASSERT_EQ_I(e->arg_b, 14);
    T_ASSERT_EQ_U(e->arg_c, 0u);  /* italic == 0 */
    T_ASSERT(strcmp(e->face, "Courier New") == 0);
    return 0;
}

int test_make_font_family_times(void)
{
    stub_reset();
    (void)ar_make_font(10, 20, 3);
    const stub_event *e = stub_nth(STUB_FONT, 0);
    T_ASSERT(strcmp(e->face, "Times New Roman") == 0);
    T_ASSERT_EQ_U(e->arg_c, 0u);
    return 0;
}

int test_make_font_family_arial(void)
{
    stub_reset();
    (void)ar_make_font(10, 20, 4);
    T_ASSERT(strcmp(stub_nth(STUB_FONT, 0)->face, "Arial") == 0);
    return 0;
}

int test_make_font_family_courier_italic(void)
{
    stub_reset();
    (void)ar_make_font(7, 14, 5);
    const stub_event *e = stub_nth(STUB_FONT, 0);
    T_ASSERT(strcmp(e->face, "Courier New") == 0);
    T_ASSERT_EQ_U(e->arg_c, 1u);  /* italic == 1 */
    return 0;
}

int test_make_font_unknown_family_skips_face(void)
{
    stub_reset();
    (void)ar_make_font(7, 14, 99);
    const stub_event *e = stub_nth(STUB_FONT, 0);
    /* Retail's switch jumps over the face copy when family is unknown
     * — the lfFaceName stays at its (memset'd) zero state.  Our stub
     * receives an empty string. */
    T_ASSERT_EQ_I((int)e->face[0], 0);
    return 0;
}

/* ─── ar_gdi_slot_set_font / set_pen / set_brush ─────────────────── */

int test_set_font_writes_slot_and_handle(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    ar_gdi_slot_set_font(&s, /*group=*/0x1337, /*w=*/8, /*h=*/16, /*family=*/4);

    T_ASSERT_EQ_U(s.group, 0x1337u);
    T_ASSERT_EQ_U(s.capacity, 1u);
    T_ASSERT_EQ_U(s.count, 0u);     /* retail does NOT bump count here */
    T_ASSERT(s.array != NULL);
    T_ASSERT_EQ_I(stub_count_kind(STUB_FONT), 1);
    const stub_event *e = stub_nth(STUB_FONT, 0);
    T_ASSERT_EQ_I(e->arg_a, 8);
    T_ASSERT(strcmp(e->face, "Arial") == 0);
    T_ASSERT_EQ_P(s.array[0], e->handle);

    ar_gdi_slot_destroy(&s);
    return 0;
}

int test_set_font_destroys_existing(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    /* Prime with a pre-existing slot. */
    ar_gdi_slot_set_font(&s, 0x100, 4, 8, 0);
    int after_first = g_log_count;

    /* Re-install — the old handle should be DeleteObject'd. */
    ar_gdi_slot_set_font(&s, 0x200, 10, 20, 3);

    /* Events between `after_first` and now: one DELETE then one FONT. */
    T_ASSERT_EQ_I(g_log[after_first].kind, STUB_DELETE);
    T_ASSERT_EQ_I(g_log[after_first + 1].kind, STUB_FONT);
    T_ASSERT_EQ_U(s.group, 0x200u);
    T_ASSERT(strcmp(stub_nth(STUB_FONT, 1)->face, "Times New Roman") == 0);

    ar_gdi_slot_destroy(&s);
    return 0;
}

int test_set_pen_bumps_count(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    ar_gdi_slot_set_pen(&s, /*width=*/3, /*color=*/0xff0080, /*group=*/0x42, /*capacity=*/4);

    T_ASSERT_EQ_U(s.group, 0x42u);
    T_ASSERT_EQ_U(s.capacity, 4u);
    T_ASSERT_EQ_U(s.count, 1u);    /* unlike set_font, set_pen DOES bump count */
    T_ASSERT_EQ_I(stub_count_kind(STUB_PEN), 1);
    const stub_event *e = stub_nth(STUB_PEN, 0);
    T_ASSERT_EQ_I(e->arg_a, 0);    /* PS_SOLID */
    T_ASSERT_EQ_I(e->arg_b, 3);
    T_ASSERT_EQ_U(e->arg_c, 0xff0080u);
    T_ASSERT_EQ_P(s.array[0], e->handle);
    T_ASSERT_EQ_P(s.array[1], NULL);

    ar_gdi_slot_destroy(&s);
    return 0;
}

int test_set_brush_bumps_count(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    ar_gdi_slot_set_brush(&s, /*color=*/0xc0ffee, /*group=*/0x7, /*capacity=*/2);

    T_ASSERT_EQ_U(s.count, 1u);
    T_ASSERT_EQ_U(s.capacity, 2u);
    T_ASSERT_EQ_U(stub_nth(STUB_BRUSH, 0)->arg_c, 0xc0ffeeu);

    ar_gdi_slot_destroy(&s);
    return 0;
}

int test_set_pen_capacity_zero_no_op(void)
{
    stub_reset();
    ar_gdi_slot s = {0};
    ar_gdi_slot_set_pen(&s, 1, 0xffffff, 0, /*capacity=*/0);

    T_ASSERT_EQ_U(s.capacity, 0u);
    T_ASSERT_EQ_U(s.count, 0u);
    T_ASSERT_EQ_I(stub_count_kind(STUB_PEN), 0);
    /* `calloc(0, ...)` is implementation-defined; glibc returns a
     * 1-byte buffer.  Call destroy to clean it up (the loop is a
     * no-op since capacity==0; the free at the end runs). */
    ar_gdi_slot_destroy(&s);
    return 0;
}

/* ─── ar_gdi_slot_set_pen_gradient ───────────────────────────────── */

int test_pen_gradient_endpoints_and_lerp(void)
{
    stub_reset();
    ar_state_init();

    /* Use a 4-pen gradient like FUN_00579bd0 does — same shape as the
     * first call there. */
    ar_gdi_slot_set_pen_gradient(0xc, /*group=*/0x99, /*capacity=*/4,
                                 /*color_a=*/0x200020,
                                 /*color_mid=*/0x605060,
                                 /*color_c=*/0xb090bf,
                                 /*width_a=*/15,
                                 /*width_step=*/4,
                                 /*width_c=*/1);

    ar_gdi_slot *s = g_ar_gdi_table[0xc];
    T_ASSERT_EQ_U(s->group, 0x99u);
    T_ASSERT_EQ_U(s->capacity, 4u);
    T_ASSERT_EQ_U(s->count, 4u);
    T_ASSERT_EQ_I(stub_count_kind(STUB_PEN), 4);

    /* Pen 0: (15, 0x200020) */
    const stub_event *p0 = stub_nth(STUB_PEN, 0);
    T_ASSERT_EQ_I(p0->arg_b, 15);
    T_ASSERT_EQ_U(p0->arg_c, 0x200020u);

    /* Pen 1: width 15 - 4*1 = 11, colour = lerp(0x200020, 0x605060, 1, 4).
     *   R: 0x20 + (0x60-0x20)*1/4 = 0x20 + 0x10 = 0x30
     *   G: 0x00 + (0x50-0x00)*1/4 = 0x00 + 0x14 = 0x14
     *   B: 0x20 + (0x60-0x20)*1/4 = 0x20 + 0x10 = 0x30 */
    const stub_event *p1 = stub_nth(STUB_PEN, 1);
    T_ASSERT_EQ_I(p1->arg_b, 11);
    T_ASSERT_EQ_U(p1->arg_c, 0x301430u);

    /* Pen 2: width = 15 - 4*2 = 7, colour = lerp(0x200020, 0x605060, 2, 4)
     *      = midpoint = ((0x20+0x60)/2, (0x00+0x50)/2, (0x20+0x60)/2)
     *      = (0x40, 0x28, 0x40)                                          */
    const stub_event *p2 = stub_nth(STUB_PEN, 2);
    T_ASSERT_EQ_I(p2->arg_b, 7);
    T_ASSERT_EQ_U(p2->arg_c, 0x402840u);

    /* Pen 3 (last): (1, 0xb090bf) — uses width_c and color_c directly. */
    const stub_event *p3 = stub_nth(STUB_PEN, 3);
    T_ASSERT_EQ_I(p3->arg_b, 1);
    T_ASSERT_EQ_U(p3->arg_c, 0xb090bfu);

    ar_gdi_slot_destroy(s);
    return 0;
}

int test_pen_gradient_capacity_two(void)
{
    /* Edge: capacity == 2 means the middle loop is skipped (`1 < 1`
     * is false) — only pen 0 and the last pen are installed. */
    stub_reset();
    ar_state_init();
    ar_gdi_slot_set_pen_gradient(5, 0x11, 2,
                                 0x010203, 0x040506, 0x070809,
                                 8, 1, 2);
    ar_gdi_slot *s = g_ar_gdi_table[5];
    T_ASSERT_EQ_U(s->count, 2u);
    T_ASSERT_EQ_U(stub_nth(STUB_PEN, 0)->arg_c, 0x010203u);
    T_ASSERT_EQ_U(stub_nth(STUB_PEN, 1)->arg_c, 0x070809u);

    ar_gdi_slot_destroy(s);
    return 0;
}

int test_pen_gradient_capacity_one(void)
{
    /* Edge: capacity == 1 — pen 0 is installed, then the "last pen"
     * branch sees count==1 == capacity and silently drops. */
    stub_reset();
    ar_state_init();
    ar_gdi_slot_set_pen_gradient(5, 0x22, 1,
                                 0x010203, 0x040506, 0x070809,
                                 8, 1, 2);
    ar_gdi_slot *s = g_ar_gdi_table[5];
    T_ASSERT_EQ_U(s->count, 1u);
    T_ASSERT_EQ_I(stub_count_kind(STUB_PEN), 1);
    T_ASSERT_EQ_U(stub_nth(STUB_PEN, 0)->arg_c, 0x010203u);

    ar_gdi_slot_destroy(s);
    return 0;
}

/* ─── ar_register_fonts (top-level FUN_00579bd0) ─────────────────── */

int test_register_fonts_sprite_slots(void)
{
    stub_reset();
    ar_state_init();
    void *zdd = (void *)0x1234;
    void *settings = (void *)0x5678;
    ar_register_fonts(zdd, /*group=*/0xabcd, settings);

    /* sprite[0]: id 0x457, 32×32, scale=0. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].zdd, zdd);
    T_ASSERT_EQ_P(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].settings, settings);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].resource_id, 0x457u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].width, 0x20u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].height, 0x20u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].colorkey, 0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].scale_flag, 0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].type, 2u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].group, 0xabcdu);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].entry_count, 1u);
    T_ASSERT(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].entries != NULL);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].entries[0].a, 0u);
    T_ASSERT_EQ_P(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].entries[0].b, NULL);

    /* sprite[1]: id 0x455, 32×48, scale=1. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_455].resource_id, 0x455u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_455].height, 0x30u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_455].scale_flag, 1u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_455].group, 0xabcdu);

    /* Clean up to keep ASan happy across test runs. */
    ar_sprite_slot_destroy(&g_ar_sprite_slots[AR_SPR_FONT_TEX_457]);
    ar_sprite_slot_destroy(&g_ar_sprite_slots[AR_SPR_FONT_TEX_455]);
    for (int i = 0; i < AR_GDI_SLOT_COUNT; i++)
        ar_gdi_slot_destroy(g_ar_gdi_table[i]);
    return 0;
}

int test_register_fonts_gdi_indices(void)
{
    stub_reset();
    ar_state_init();
    ar_register_fonts((void *)0x100, /*group=*/0x77, (void *)0x200);

    /* Expected dimensions for the 8 named-global fonts.  These match
     * the FUN_00579bd0 retail block by table index. */
    struct exp { uint16_t idx; int32_t w; int32_t h; const char *face; } const exp[] = {
        { 1,  6, 0xe,  "Courier New" },
        { 2,  7, 0x10, "Courier New" },
        { 3,  7, 0x12, "Courier New" },
        { 4,  4, 8,    "Courier New" },
        { 5,  7, 0x12, "Courier New" },
        { 6, 10, 0x14, "Courier New" },
        { 7,  5, 10,   "Courier New" },
        { 8,  8, 0x10, "Courier New" },
        { 9,  4, 8,    "Courier New" },   /* the FUN_0057a030 extra */
    };

    /* Every gdi_table[idx] (1..9) must have capacity=1 and a non-null
     * handle written at array[0].  group must be stamped. */
    for (size_t i = 0; i < sizeof(exp) / sizeof(exp[0]); i++) {
        ar_gdi_slot *s = g_ar_gdi_table[exp[i].idx];
        if (s->capacity != 1) T_FAIL("idx %u capacity %u, want 1",
                                     (unsigned)exp[i].idx, (unsigned)s->capacity);
        T_ASSERT_EQ_U(s->group, 0x77u);
        T_ASSERT(s->array != NULL);
        T_ASSERT(s->array[0] != NULL);
    }

    /* table[14] pen + table[15] brush. */
    T_ASSERT_EQ_U(g_ar_gdi_table[14]->count, 1u);
    T_ASSERT_EQ_U(g_ar_gdi_table[14]->capacity, 1u);
    T_ASSERT_EQ_U(g_ar_gdi_table[14]->group, 0x77u);
    T_ASSERT_EQ_U(g_ar_gdi_table[15]->count, 1u);
    T_ASSERT_EQ_U(g_ar_gdi_table[15]->capacity, 1u);
    T_ASSERT_EQ_U(g_ar_gdi_table[15]->group, 0x77u);

    /* Four pen gradients at indices 12, 10, 11, 13.  Each capacity=4. */
    for (int idx = 10; idx <= 13; idx++) {
        ar_gdi_slot *s = g_ar_gdi_table[idx];
        if (s->capacity != 4) T_FAIL("grad idx %d capacity %u, want 4",
                                     idx, (unsigned)s->capacity);
        T_ASSERT_EQ_U(s->count, 4u);
        T_ASSERT_EQ_U(s->group, 0x77u);
    }

    /* Total FONT events: 9 (8 named + 1 via set_font).
     * Total PEN  events: 1 (set_pen) + 16 (4 gradients × 4 pens). */
    T_ASSERT_EQ_I(stub_count_kind(STUB_FONT), 9);
    T_ASSERT_EQ_I(stub_count_kind(STUB_PEN), 1 + 16);
    T_ASSERT_EQ_I(stub_count_kind(STUB_BRUSH), 1);

    /* Cleanup */
    ar_sprite_slot_destroy(&g_ar_sprite_slots[AR_SPR_FONT_TEX_457]);
    ar_sprite_slot_destroy(&g_ar_sprite_slots[AR_SPR_FONT_TEX_455]);
    for (int i = 0; i < AR_GDI_SLOT_COUNT; i++)
        ar_gdi_slot_destroy(g_ar_gdi_table[i]);
    return 0;
}

int test_register_fonts_dimensions_in_call_order(void)
{
    /* Verify the FONT events are emitted in the exact order the
     * retail decompile calls them: indices 1..8 then index 9 (the
     * FUN_0057a030 extra). */
    stub_reset();
    ar_state_init();
    ar_register_fonts((void *)0x100, 0x55, (void *)0x200);

    const int32_t expected_widths[]  = { 6, 7, 7, 4, 7, 10, 5, 8, 4 };
    const int32_t expected_heights[] = { 0xe, 0x10, 0x12, 8, 0x12, 0x14, 10, 0x10, 8 };
    for (int i = 0; i < 9; i++) {
        const stub_event *e = stub_nth(STUB_FONT, i);
        if (!e) T_FAIL("missing font event %d", i);
        if (e->arg_a != expected_widths[i] || e->arg_b != expected_heights[i])
            T_FAIL("font[%d] = (%d, %d), want (%d, %d)", i,
                   e->arg_a, e->arg_b, expected_widths[i], expected_heights[i]);
    }

    /* Cleanup */
    ar_sprite_slot_destroy(&g_ar_sprite_slots[AR_SPR_FONT_TEX_457]);
    ar_sprite_slot_destroy(&g_ar_sprite_slots[AR_SPR_FONT_TEX_455]);
    for (int i = 0; i < AR_GDI_SLOT_COUNT; i++)
        ar_gdi_slot_destroy(g_ar_gdi_table[i]);
    return 0;
}

/* ─── ar_sound_slot_init / ar_register_sounds ────────────────────── */

int test_sound_slot_init_writes_fields(void)
{
    ar_sound_slot s = {0};
    /* Pre-populate `buffer` to verify the setter does NOT touch it. */
    s.buffer = (void *)0xdeadbeef;

    ar_sound_slot_init(&s,
                       /*zds=*/(void *)0x111,
                       /*settings=*/(void *)0x222,
                       /*resource_id=*/0x4cb,
                       /*count=*/2,
                       /*group=*/0x1234);

    T_ASSERT_EQ_U(s.count, 2u);
    T_ASSERT_EQ_U(s.state, 0u);
    T_ASSERT_EQ_P(s.buffer, (void *)0xdeadbeef);    /* untouched */
    T_ASSERT_EQ_P(s.zds, (void *)0x111);
    T_ASSERT_EQ_U(s.resource_id, 0x4cbu);
    T_ASSERT_EQ_P(s.settings, (void *)0x222);
    T_ASSERT_EQ_U(s.group, 0x1234u);
    return 0;
}

int test_sound_slot_init_clears_state(void)
{
    ar_sound_slot s = {0};
    s.state = 0x99;
    ar_sound_slot_init(&s, NULL, NULL, 0x100, 2, 0);
    T_ASSERT_EQ_U(s.state, 0u);
    return 0;
}

int test_register_sounds_all_ids_and_kinds(void)
{
    ar_state_init();

    void *zds = (void *)0xabc;
    void *settings = (void *)0xdef;
    ar_register_sounds(zds, /*group=*/0x77, settings);

    /* The full 12-entry roster from FUN_00579a00, in table-index
     * order.  These are baked into the engine's sound bank semantics —
     * if they ever shift, audio playback would route to the wrong
     * sotesd.dll resource. */
    struct exp { uint16_t id; uint16_t count; };
    static const struct exp expected[AR_SOUND_MAIN_COUNT] = {
        { 0x50f, 2 }, { 0x50e, 2 }, { 0x508, 2 }, { 0x510, 2 },
        { 0x903, 2 }, { 0x509, 4 }, { 0x506, 4 }, { 0x507, 2 },
        { 0x50c, 4 }, { 0x50d, 4 }, { 0x4d8, 2 }, { 0x4d9, 2 },
    };
    for (int i = 0; i < AR_SOUND_MAIN_COUNT; i++) {
        const ar_sound_slot *s = g_ar_sound_table[i];
        if (s->resource_id != expected[i].id)
            T_FAIL("sound[%d] id 0x%x, want 0x%x",
                   i, (unsigned)s->resource_id, (unsigned)expected[i].id);
        if (s->count != expected[i].count)
            T_FAIL("sound[%d] count %u, want %u",
                   i, (unsigned)s->count, (unsigned)expected[i].count);
        T_ASSERT_EQ_P(s->zds, zds);
        T_ASSERT_EQ_P(s->settings, settings);
        T_ASSERT_EQ_U(s->group, 0x77u);
        T_ASSERT_EQ_U(s->state, 0u);
        T_ASSERT_EQ_P(s->buffer, NULL);
    }
    return 0;
}

int test_register_sounds_buffer_pointer_preserved(void)
{
    /* If a slot was previously populated with a wave buffer, calling
     * register_sounds AGAIN must not stomp it — the engine's lazy load
     * uses `buffer != NULL` as "already loaded".  Note: ar_state_init
     * zeros buffer, so we set it after init. */
    ar_state_init();
    g_ar_sound_table[5]->buffer = (void *)0xc0ffee;

    ar_register_sounds(NULL, 0, NULL);

    T_ASSERT_EQ_P(g_ar_sound_table[5]->buffer, (void *)0xc0ffee);
    return 0;
}

/* ─── ar_register_main_sprites (FUN_005749b0) ────────────────────── */

/* Helper: tear down all sprite slots after a driver test so ASan stays
 * happy across runs. */
static void destroy_all_sprite_slots(void)
{
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    }
}

int test_main_sprites_inline_slots_field_map(void)
{
    /* The 9 inline slots — verify every (id, w, h, ck, scale, type) tuple
     * matches retail.  These pin the asset-bank semantics: the title
     * scene scans these IDs into sotesd.dll resource lookups, so a
     * mismatch would route the wrong sprite to the wrong on-screen
     * slot. */
    ar_state_init();
    void *zdd = (void *)0x1111;
    void *settings = (void *)0x2222;
    ar_register_main_sprites(zdd, /*group=*/0x4, settings,
                             /*sotesp_module=*/(void *)0x3333);

    struct exp { int idx; uint16_t id; uint32_t w; uint32_t h;
                 uint32_t ck; uint32_t scale; uint32_t type; };
    static const struct exp inline_expected[] = {
        { 1, 0x49f, 0xe0,  0xe0,  0,         1, 2 },
        { 2, 0x448, 0x98,  0x28,  0,         1, 2 },
        { 3, 0x4a2, 0x90,  0x6c,  0,         1, 2 },
        { 4, 0x49d, 0x280, 0x1e0, 0x1ffffff, 1, 0 },
        { 5, 0x913, 0x280, 0x1e0, 0x1ffffff, 1, 0 },
        { 6, 0x91b, 0x280, 0x1e0, 0x1ffffff, 1, 0 },
        { 7, 0x91c, 0xa0,  0x20,  0x1ffffff, 1, 0 },
        { 8, 0x91d, 0x20,  0x20,  0,         1, 2 },
        { 9, 0x8df, 0x170, 0x114, 0x1ffffff, 1, 0 },
    };
    for (size_t i = 0; i < sizeof(inline_expected) / sizeof(inline_expected[0]); i++) {
        const struct exp *e = &inline_expected[i];
        const ar_sprite_slot *s = &g_ar_sprite_slots[e->idx];
        if (s->resource_id != e->id)
            T_FAIL("slot[%d] id 0x%x, want 0x%x",
                   e->idx, (unsigned)s->resource_id, (unsigned)e->id);
        if (s->width != e->w || s->height != e->h)
            T_FAIL("slot[%d] %ux%u, want %ux%u",
                   e->idx, (unsigned)s->width, (unsigned)s->height,
                   (unsigned)e->w, (unsigned)e->h);
        T_ASSERT_EQ_U(s->colorkey, e->ck);
        T_ASSERT_EQ_U(s->scale_flag, e->scale);
        T_ASSERT_EQ_U(s->type, e->type);
        T_ASSERT_EQ_U(s->group, 0x4u);
        T_ASSERT_EQ_P(s->zdd, zdd);
        T_ASSERT_EQ_P(s->settings, settings);
        T_ASSERT_EQ_U(s->entry_count, 1u);
        T_ASSERT(s->entries != NULL);
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_main_sprites_transient_idx0_uses_sotesp_module(void)
{
    /* idx 0 (DAT_008a7640) — id 0x90b loaded from sotesp.dll.  The
     * settings field must be the sotesp_module pointer, NOT the
     * launcher settings record.  This is the only slot in the driver
     * whose settings pointer diverges; if it slips into using
     * `settings` the engine would query the wrong companion DLL for
     * this font texture. */
    ar_state_init();
    void *zdd = (void *)0xaaaa;
    void *settings = (void *)0xbbbb;
    void *sotesp = (void *)0xcccc;
    ar_register_main_sprites(zdd, /*group=*/0x7, settings, sotesp);

    const ar_sprite_slot *s = &g_ar_sprite_slots[0];
    T_ASSERT_EQ_U(s->resource_id, 0x90bu);
    T_ASSERT_EQ_U(s->width, 0x20u);
    T_ASSERT_EQ_U(s->height, 0x20u);
    T_ASSERT_EQ_U(s->colorkey, 0u);
    T_ASSERT_EQ_U(s->scale_flag, 0u);
    T_ASSERT_EQ_U(s->type, 2u);
    T_ASSERT_EQ_U(s->group, 0x7u);
    T_ASSERT_EQ_P(s->zdd, zdd);
    T_ASSERT_EQ_P(s->settings, sotesp);              /* not `settings`! */

    destroy_all_sprite_slots();
    return 0;
}

int test_main_sprites_trailing_ids_in_index_order(void)
{
    /* The 24 trailing FUN_005748c0 calls — verify the (idx, id) map.
     * 20 contiguous at idx 10..29, then 4 stragglers at 46, 47, 50, 55.
     * The straggler-index gap reflects retail's non-contiguous BSS
     * layout for those calls. */
    ar_state_init();
    ar_register_main_sprites((void *)0x1, /*group=*/0x9, (void *)0x2,
                             /*sotesp=*/(void *)0x3);

    /* (idx, id, w, h, ck, scale, type) — matches asset_register.c
     * trailing_calls table.  Out-of-order so the slot/index association
     * gets pinned independently of any iteration-order assumption. */
    struct exp { int idx; uint16_t id; uint32_t w; uint32_t h;
                 uint32_t ck; uint32_t scale; uint32_t type; };
    static const struct exp trailing_expected[] = {
        { 10, 0x8e0, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 17, 0x8de, 0x190, 0xa0,  0x1ffffff, 1, 0 },
        { 19, 0x712, 0x160, 0xb0,  0x1ffffff, 1, 0 },
        { 20, 0x6f8, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 29, 0x90d, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 46, 0x453, 0x40,  0x40,  0,         0, 2 },
        { 47, 0x660, 0xa0,  0x20,  0,         0, 2 },
        { 50, 0x456, 0x20,  0x20,  0,         0, 2 },
        { 55, 0x6fa, 0x20,  0x20,  0,         0, 2 },
    };
    for (size_t i = 0; i < sizeof(trailing_expected) / sizeof(trailing_expected[0]); i++) {
        const struct exp *e = &trailing_expected[i];
        const ar_sprite_slot *s = &g_ar_sprite_slots[e->idx];
        if (s->resource_id != e->id)
            T_FAIL("slot[%d] id 0x%x, want 0x%x",
                   e->idx, (unsigned)s->resource_id, (unsigned)e->id);
        T_ASSERT_EQ_U(s->width, e->w);
        T_ASSERT_EQ_U(s->height, e->h);
        T_ASSERT_EQ_U(s->colorkey, e->ck);
        T_ASSERT_EQ_U(s->scale_flag, e->scale);
        T_ASSERT_EQ_U(s->type, e->type);
        T_ASSERT_EQ_U(s->group, 0x9u);
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_main_sprites_untouched_indices_stay_zero(void)
{
    /* The driver fills slots at 0..29 + {46, 47, 50, 55}.  Slots at
     * other indices in the pool (e.g. 30..45, 48, 49) must remain
     * zero-initialised — proves the driver isn't sweeping the whole
     * table.  Also leaves room for FUN_00579bd0 to populate 42/43 in
     * a later run without conflict. */
    ar_state_init();
    ar_register_main_sprites(NULL, 0, NULL, NULL);

    for (int i = 30; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (i == 46 || i == 47 || i == 50 || i == 55) continue;
        const ar_sprite_slot *s = &g_ar_sprite_slots[i];
        if (s->entries != NULL)
            T_FAIL("slot[%d] entries populated unexpectedly", i);
        T_ASSERT_EQ_U(s->resource_id, 0u);
        T_ASSERT_EQ_U(s->group, 0u);
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_main_sprites_total_slot_count(void)
{
    /* Spot-check: exactly 34 slots get an entries allocation
     * (10 inline + 24 trailing).  Detects accidental drift if a slot
     * is added or removed from the driver. */
    ar_state_init();
    ar_register_main_sprites((void *)0x1, 0x1, (void *)0x2, (void *)0x3);

    int populated = 0;
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (g_ar_sprite_slots[i].entries != NULL) populated++;
    }
    T_ASSERT_EQ_I(populated, 34);

    destroy_all_sprite_slots();
    return 0;
}

int test_main_sprites_coexists_with_register_fonts(void)
{
    /* FUN_00579bd0 writes idx 42/43; FUN_005749b0 writes 0..29 + 4
     * stragglers (46, 47, 50, 55).  Running both in either order must
     * leave the union populated correctly with no clobbering.  Order
     * tested: main first, then fonts. */
    ar_state_init();
    void *zdd = (void *)0x100;
    void *settings = (void *)0x200;
    ar_register_main_sprites(zdd, /*group=*/0x4, settings,
                             /*sotesp=*/(void *)0x300);
    ar_register_fonts(zdd, /*group=*/0x1, settings);

    /* Main-sprite slots survived. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].resource_id, 0x49fu);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].group, 0x4u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[20].resource_id, 0x6f8u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[55].resource_id, 0x6fau);

    /* Font-texture slots present at the named indices. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].resource_id, 0x457u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_455].resource_id, 0x455u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].group, 0x1u);

    /* Run main again — re-registers free old entries (ASan would
     * surface any double-free or leak) and overwrite fresh. */
    ar_register_main_sprites(zdd, /*group=*/0x5, settings, (void *)0x300);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].group, 0x5u);
    /* Font slots still intact — main doesn't touch them. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].resource_id, 0x457u);

    destroy_all_sprite_slots();
    for (int i = 0; i < AR_GDI_SLOT_COUNT; i++)
        ar_gdi_slot_destroy(g_ar_gdi_table[i]);
    return 0;
}

/* ─── ar_register_game_sprites (FUN_0056e190) ────────────────────── */

int test_game_sprites_inline_block_field_map(void)
{
    /* The 93 inline blocks at idx 425..517 — sequential resource IDs
     * 0x592..0x5fb.  72 are 0xa0×0xb0; 21 (resource IDs 0x71f..0x733,
     * idx 467..487) are 0xb0×0x90.  Spot-check a handful at the
     * boundaries plus inside both shape regions. */
    ar_state_init();
    void *zdd = (void *)0x1111;
    void *settings = (void *)0x2222;
    ar_register_game_sprites(zdd, /*group=*/0x5, settings);

    struct exp { int idx; uint16_t id; uint32_t w; uint32_t h; };
    static const struct exp checks[] = {
        { 425, 0x592, 0xa0, 0xb0 },  /* first inline slot      */
        { 466, 0x6ec, 0xa0, 0xb0 },  /* last 0xa0×0xb0 before shape shift */
        { 467, 0x71f, 0xb0, 0x90 },  /* first 0xb0×0x90 — id-shape break */
        { 487, 0x733, 0xb0, 0x90 },  /* last 0xb0×0x90  */
        { 488, 0x5e1, 0xa0, 0xb0 },  /* back to 0xa0×0xb0 — id sequence resets */
        { 517, 0x5fb, 0xa0, 0xb0 },  /* last inline slot       */
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++) {
        const struct exp *e = &checks[i];
        const ar_sprite_slot *s = &g_ar_sprite_slots[e->idx];
        if (s->resource_id != e->id)
            T_FAIL("inline[%d] id 0x%x, want 0x%x",
                   e->idx, (unsigned)s->resource_id, (unsigned)e->id);
        if (s->width != e->w || s->height != e->h)
            T_FAIL("inline[%d] %ux%u, want %ux%u",
                   e->idx, (unsigned)s->width, (unsigned)s->height,
                   (unsigned)e->w, (unsigned)e->h);
        T_ASSERT_EQ_U(s->colorkey, 0xff00ffu);
        T_ASSERT_EQ_U(s->scale_flag, 1u);
        T_ASSERT_EQ_U(s->type, 0u);
        T_ASSERT_EQ_U(s->group, 0x5u);
        T_ASSERT_EQ_P(s->zdd, zdd);
        T_ASSERT_EQ_P(s->settings, settings);
        T_ASSERT_EQ_U(s->entry_count, 1u);
        T_ASSERT(s->entries != NULL);
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_game_sprites_trailing_call_shapes(void)
{
    /* The 349 trailing FUN_005748c0 calls use three sprite shapes.
     * Spot-check one of each shape, plus the very low-index stragglers
     * at idx 62..64 (the only sub-100 indices touched by this batch). */
    ar_state_init();
    ar_register_game_sprites((void *)0x1, /*group=*/0xaa, (void *)0x2);

    struct exp { int idx; uint16_t id; uint32_t w; uint32_t h;
                 uint32_t scale; uint32_t type; };
    static const struct exp trailing_checks[] = {
        /* First trailing call — slot at idx 518 (DAT_008a7e58), id 0x5fc */
        { 518, 0x5fc, 0xa0, 0xb0, 1, 0 },
        /* 0xb0×0x90 sample — id 0x7ee at idx 840 (DAT_008a8200) */
        { 840, 0x7ee, 0xb0, 0x90, 1, 0 },
        /* 0x80×0x80 icon shape — taken from the tail */
        { 861, 0x580, 0x80, 0x80, 0, 2 },
        /* Low-index stragglers — IDs 0x608/0x609/0x60a end up at idx 62/63/64 */
        { 62,  0x608, 0x80, 0x80, 0, 2 },
        { 63,  0x609, 0x80, 0x80, 0, 2 },
        { 64,  0x60a, 0x80, 0x80, 0, 2 },
    };
    for (size_t i = 0; i < sizeof(trailing_checks) / sizeof(trailing_checks[0]); i++) {
        const struct exp *e = &trailing_checks[i];
        const ar_sprite_slot *s = &g_ar_sprite_slots[e->idx];
        if (s->resource_id != e->id)
            T_FAIL("trail[%d] id 0x%x, want 0x%x",
                   e->idx, (unsigned)s->resource_id, (unsigned)e->id);
        T_ASSERT_EQ_U(s->width, e->w);
        T_ASSERT_EQ_U(s->height, e->h);
        T_ASSERT_EQ_U(s->colorkey, 0xff00ffu);
        T_ASSERT_EQ_U(s->scale_flag, e->scale);
        T_ASSERT_EQ_U(s->type, e->type);
        T_ASSERT_EQ_U(s->group, 0xaau);
        T_ASSERT_EQ_U(s->entry_count, 1u);
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_game_sprites_total_slot_count(void)
{
    /* 93 inline + 349 trailing = 442 populated slots.  Detects drift
     * if a slot is accidentally added/removed from the table. */
    ar_state_init();
    ar_register_game_sprites((void *)0x1, 0x1, (void *)0x2);

    int populated = 0;
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (g_ar_sprite_slots[i].entries != NULL) populated++;
    }
    T_ASSERT_EQ_I(populated, 442);

    destroy_all_sprite_slots();
    return 0;
}

int test_game_sprites_resource_ids_unique(void)
{
    /* Every resource ID in the batch must be unique — the engine's
     * per-asset resource-decoder keys off this ID, so duplicates would
     * silently overwrite each other in sotesp.dll/sotesd.dll lookups.
     * Also pins that our extraction didn't drop or double-count any
     * entry. */
    ar_state_init();
    ar_register_game_sprites((void *)0x1, 0, (void *)0x2);

    /* Collect every populated slot's id; verify all-pairs distinct. */
    enum { MAX_IDS = 512 };
    uint16_t ids[MAX_IDS];
    int n = 0;
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (g_ar_sprite_slots[i].entries == NULL) continue;
        if (n >= MAX_IDS) T_FAIL("too many populated slots");
        ids[n++] = g_ar_sprite_slots[i].resource_id;
    }
    T_ASSERT_EQ_I(n, 442);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (ids[i] == ids[j])
                T_FAIL("duplicate resource id 0x%x at populated-slot indices %d & %d",
                       (unsigned)ids[i], i, j);
        }
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_game_sprites_untouched_indices_stay_zero(void)
{
    /* The batch touches a sparse but bounded set of indices: {62..64,
     * 425..517, plus 346 trailing-call indices in between & beyond}.
     * Every index outside that set must remain zero — the highest
     * touched index is 863, so 864..1023 are an easy zero-check zone. */
    ar_state_init();
    ar_register_game_sprites(NULL, 0, NULL);

    for (int i = 864; i < AR_SPRITE_SLOT_COUNT; i++) {
        const ar_sprite_slot *s = &g_ar_sprite_slots[i];
        if (s->entries != NULL)
            T_FAIL("slot[%d] entries populated past expected range", i);
        T_ASSERT_EQ_U(s->resource_id, 0u);
        T_ASSERT_EQ_U(s->group, 0u);
    }
    /* Also: indices 0..61 are below the batch's lowest touch — should
     * also stay clean. */
    for (int i = 0; i < 62; i++) {
        const ar_sprite_slot *s = &g_ar_sprite_slots[i];
        if (s->entries != NULL)
            T_FAIL("slot[%d] entries populated below expected range", i);
    }

    destroy_all_sprite_slots();
    return 0;
}

int test_game_sprites_coexists_with_main_sprites(void)
{
    /* ar_register_main_sprites touches {0..29, 46, 47, 50, 55} —
     * disjoint from this batch's lowest touches at 62..64.  Running
     * both back-to-back should leave the union intact with each
     * batch's group tag preserved on its own slots. */
    ar_state_init();
    void *zdd = (void *)0x100;
    void *settings = (void *)0x200;
    ar_register_main_sprites(zdd, /*group=*/0x4, settings, (void *)0x300);
    ar_register_game_sprites(zdd, /*group=*/0x5, settings);

    /* Main-sprite slot survives. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].resource_id, 0x49fu);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].group, 0x4u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[20].group, 0x4u);
    /* Game-sprite slots present + tagged with group 5. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[62].resource_id, 0x608u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[62].group, 0x5u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[425].resource_id, 0x592u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[425].group, 0x5u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[863].resource_id, 0x581u);

    /* Re-register game sprites with a different group — ASan would
     * surface any double-free in the destroy-then-realloc path. */
    ar_register_game_sprites(zdd, /*group=*/0x7, settings);
    T_ASSERT_EQ_U(g_ar_sprite_slots[425].group, 0x7u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].group, 0x4u);  /* main slot untouched */

    destroy_all_sprite_slots();
    return 0;
}

/* ─── ar_register_game_sounds (FUN_0057b280) ─────────────────────── */

int test_game_sounds_total_entry_count(void)
{
    /* 122 inline + 52 thiscall = 174 distinct slots in the batch.
     * Detects drift if a row is accidentally added/removed from
     * `game_sounds`. */
    ar_state_init();
    void *zds = (void *)0x1111;
    void *settings = (void *)0x2222;
    ar_register_game_sounds(zds, /*group=*/0x3, settings);

    int populated = 0;
    for (int i = 0; i < AR_SOUND_SLOT_COUNT; i++) {
        /* zds!=NULL is the post-register sentinel — ar_state_init zeros
         * everything; only ar_sound_slot_init writes zds. */
        if (g_ar_sound_slots[i].zds != NULL) populated++;
    }
    T_ASSERT_EQ_I(populated, 174);
    return 0;
}

int test_game_sounds_index_range_and_gaps(void)
{
    /* Touched indices span 12..244 with 59 gaps.  Index 11 (last of
     * ar_register_sounds) and index 245+ (above the batch) must stay
     * untouched.  Also verify a couple of known gap indices stay zero
     * — they're slots that retail's locale loop (deferred) fills, not
     * the inline + thiscall halves. */
    ar_state_init();
    ar_register_game_sounds((void *)0x1, /*group=*/0x3, (void *)0x2);

    /* Below the batch — strictly zero. */
    for (int i = 0; i < 12; i++) {
        if (g_ar_sound_slots[i].zds != NULL)
            T_FAIL("slot[%d] touched but should be untouched (below batch)", i);
    }
    /* Above the batch — strictly zero. */
    for (int i = 245; i < AR_SOUND_SLOT_COUNT; i++) {
        if (g_ar_sound_slots[i].zds != NULL)
            T_FAIL("slot[%d] touched but should be untouched (above batch)", i);
    }
    /* Sample gap indices.  These are slot indices in [12..244] that
     * the inline + thiscall halves of FUN_0057b280 don't write — they
     * belong to the deferred locale-loop OR to interleaved per-group
     * callers (e.g. idx 22..25 filled by the caller at 562ea0:617-620
     * with group 2, not by this batch). */
    static const int known_gaps[] = { 22, 23, 24, 25, 156, 160, 209, 210 };
    for (size_t k = 0; k < sizeof(known_gaps)/sizeof(known_gaps[0]); k++) {
        int i = known_gaps[k];
        if (g_ar_sound_slots[i].zds != NULL)
            T_FAIL("slot[%d] populated but should be a gap (deferred path)", i);
    }
    return 0;
}

int test_game_sounds_field_writes_spot_check(void)
{
    /* Spot-check fields against the retail decomp for a representative
     * mix: first inline entry, first thiscall entry, the rare count=16
     * and count=1 entries (catch off-by-one truncation if anyone ever
     * narrowed `count` to u8), and a high-idx tail entry. */
    ar_state_init();
    void *zds = (void *)0xa11ce;
    void *settings = (void *)0xb0b;
    ar_register_game_sounds(zds, /*group=*/0x3, settings);

    struct exp { int idx; uint16_t id; uint16_t count; };
    static const struct exp checks[] = {
        {  14, 0x513,  8 },   /* first inline   — DAT_008a6efc */
        { 143, 0x4f0,  8 },   /* first thiscall — DAT_008a7100 */
        { 100, 0x52b, 16 },   /* rare count==16 — DAT_008a7054 */
        { 230, 0x77e, 16 },   /* the other count==16 */
        {  93, 0x4e9,  1 },   /* rare count==1  — DAT_008a7038 */
        {  94, 0x4ea,  1 },   /* the other count==1 */
        { 244, 0x78a,  4 },   /* highest idx in the batch */
        {  12, 0x50a,  8 },   /* lowest idx in the batch */
    };
    for (size_t i = 0; i < sizeof(checks)/sizeof(checks[0]); i++) {
        const struct exp *e = &checks[i];
        const ar_sound_slot *s = &g_ar_sound_slots[e->idx];
        if (s->resource_id != e->id)
            T_FAIL("slot[%d] id 0x%x, want 0x%x",
                   e->idx, (unsigned)s->resource_id, (unsigned)e->id);
        if (s->count != e->count)
            T_FAIL("slot[%d] count %u, want %u",
                   e->idx, (unsigned)s->count, (unsigned)e->count);
        T_ASSERT_EQ_P(s->zds, zds);
        T_ASSERT_EQ_P(s->settings, settings);
        T_ASSERT_EQ_U(s->group, 0x3u);
        T_ASSERT_EQ_U(s->state, 0u);
        T_ASSERT_EQ_P(s->buffer, NULL);
    }
    return 0;
}

int test_game_sounds_resource_ids_unique(void)
{
    /* Sound resource IDs across the batch must all be distinct — the
     * lazy wave-load path keys on resource_id when fetching from
     * sotesd.dll, so a dup would silently route the same wave to two
     * slots.  Also pins that extraction didn't double-count any row. */
    ar_state_init();
    ar_register_game_sounds((void *)0x1, 0, (void *)0x2);

    enum { MAX_IDS = 256 };
    uint16_t ids[MAX_IDS];
    int n = 0;
    for (int i = 0; i < AR_SOUND_SLOT_COUNT; i++) {
        if (g_ar_sound_slots[i].zds == NULL) continue;
        if (n >= MAX_IDS) T_FAIL("too many populated slots");
        ids[n++] = g_ar_sound_slots[i].resource_id;
    }
    T_ASSERT_EQ_I(n, 174);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (ids[i] == ids[j])
                T_FAIL("duplicate resource id 0x%x at populated indices %d & %d",
                       (unsigned)ids[i], i, j);
        }
    }
    return 0;
}

int test_game_sounds_coexists_with_main_sounds(void)
{
    /* ar_register_sounds fills idx 0..11.  ar_register_game_sounds
     * starts at idx 12.  Running both back-to-back should yield the
     * union — each batch's group tag preserved on its own slots, no
     * overlap.  Catches AR_SOUND_MAIN_COUNT regressing back to
     * AR_SOUND_SLOT_COUNT in ar_register_sounds (which would stomp
     * the entire game-sounds region). */
    ar_state_init();
    void *zds = (void *)0xdead;
    void *settings = (void *)0xbeef;
    ar_register_sounds(zds, /*group=*/1, settings);
    ar_register_game_sounds(zds, /*group=*/3, settings);

    /* Main sound slot survives with group 1. */
    T_ASSERT_EQ_U(g_ar_sound_slots[0].resource_id, 0x50fu);
    T_ASSERT_EQ_U(g_ar_sound_slots[0].group, 1u);
    T_ASSERT_EQ_U(g_ar_sound_slots[11].resource_id, 0x4d9u);
    T_ASSERT_EQ_U(g_ar_sound_slots[11].group, 1u);
    /* Game sound slot present with group 3. */
    T_ASSERT_EQ_U(g_ar_sound_slots[12].resource_id, 0x50au);
    T_ASSERT_EQ_U(g_ar_sound_slots[12].group, 3u);
    T_ASSERT_EQ_U(g_ar_sound_slots[14].resource_id, 0x513u);
    T_ASSERT_EQ_U(g_ar_sound_slots[14].group, 3u);
    return 0;
}

int test_game_sounds_buffer_pointer_preserved(void)
{
    /* Mirror of test_register_sounds_buffer_pointer_preserved for the
     * game-sounds batch: if the lazy wave-loader already populated
     * `buffer`, calling ar_register_game_sounds AGAIN must NOT clear
     * it — `buffer != NULL` is the engine's "already loaded" sentinel
     * and stomping it would force a re-decode every register pass. */
    ar_state_init();
    g_ar_sound_table[100]->buffer = (void *)0xc0ffee;
    g_ar_sound_table[244]->buffer = (void *)0xfeedface;

    ar_register_game_sounds(NULL, 0, NULL);

    T_ASSERT_EQ_P(g_ar_sound_table[100]->buffer, (void *)0xc0ffee);
    T_ASSERT_EQ_P(g_ar_sound_table[244]->buffer, (void *)0xfeedface);
    return 0;
}

/* ─── ar_register_aux_sounds (4 inline calls at 562ea0:617-620) ──── */

int test_aux_sounds_writes_four_entries(void)
{
    /* Indices 22..25 must each have id/count/group/zds/settings/state
     * matching the retail call shape; index 21 and 26 must stay
     * untouched (the batch is exactly 4 entries). */
    ar_state_init();
    void *zds = (void *)0xa1d;
    void *settings = (void *)0x5e7;
    ar_register_aux_sounds(zds, /*group=*/2, settings);

    struct exp { int idx; uint16_t id; };
    static const struct exp checks[] = {
        { 22, 0x4cb },   /* DAT_008a6f1c */
        { 23, 0x4ca },   /* DAT_008a6f20 */
        { 24, 0x4c8 },   /* DAT_008a6f24 */
        { 25, 0x4c9 },   /* DAT_008a6f28 */
    };
    for (size_t i = 0; i < sizeof(checks)/sizeof(checks[0]); i++) {
        const struct exp *e = &checks[i];
        const ar_sound_slot *s = &g_ar_sound_slots[e->idx];
        if (s->resource_id != e->id)
            T_FAIL("slot[%d] id 0x%x, want 0x%x",
                   e->idx, (unsigned)s->resource_id, (unsigned)e->id);
        T_ASSERT_EQ_U(s->count, 2u);
        T_ASSERT_EQ_P(s->zds, zds);
        T_ASSERT_EQ_P(s->settings, settings);
        T_ASSERT_EQ_U(s->group, 2u);
        T_ASSERT_EQ_U(s->state, 0u);
        T_ASSERT_EQ_P(s->buffer, NULL);
    }
    /* Flanking slots untouched. */
    T_ASSERT_EQ_P(g_ar_sound_slots[21].zds, NULL);
    T_ASSERT_EQ_P(g_ar_sound_slots[26].zds, NULL);
    return 0;
}

int test_aux_sounds_fills_game_sounds_gap(void)
{
    /* In the boot driver, ar_register_aux_sounds runs BEFORE
     * ar_register_game_sounds — the 22..25 gap reported by the
     * game-sounds index-range test is exactly what aux_sounds owns.
     * Running both back-to-back must yield a contiguous 12..25
     * pool: 12..13 (game_sounds thiscall), 14..21 (game_sounds
     * inline), 22..25 (aux_sounds), 26+ (game_sounds again).
     *
     * Catches: aux_sounds drifting off-range, or game_sounds
     * accidentally writing to 22..25 itself (which would stomp the
     * group=2 tag aux_sounds just stamped). */
    ar_state_init();
    ar_register_aux_sounds((void *)0xa, /*group=*/2, (void *)0xb);
    ar_register_game_sounds((void *)0xc, /*group=*/3, (void *)0xd);

    /* aux_sounds' slots survive game_sounds with group 2 + their IDs. */
    T_ASSERT_EQ_U(g_ar_sound_slots[22].group, 2u);
    T_ASSERT_EQ_U(g_ar_sound_slots[22].resource_id, 0x4cbu);
    T_ASSERT_EQ_U(g_ar_sound_slots[25].group, 2u);
    T_ASSERT_EQ_U(g_ar_sound_slots[25].resource_id, 0x4c9u);
    /* Neighbouring game_sounds slots survive with group 3. */
    T_ASSERT_EQ_U(g_ar_sound_slots[21].group, 3u);
    T_ASSERT_EQ_U(g_ar_sound_slots[21].resource_id, 0x4c6u);
    T_ASSERT_EQ_U(g_ar_sound_slots[26].group, 3u);
    T_ASSERT_EQ_U(g_ar_sound_slots[26].resource_id, 0x4bfu);

    /* Contiguous-fill check: every idx 12..26 is now populated. */
    for (int i = 12; i <= 26; i++) {
        if (g_ar_sound_slots[i].zds == NULL)
            T_FAIL("slot[%d] still zero after aux + game_sounds", i);
    }
    return 0;
}

int test_aux_sounds_buffer_pointer_preserved(void)
{
    /* Same lazy-load invariant as the other sound registers — if the
     * wave loader has populated `buffer`, calling the register AGAIN
     * must not stomp it. */
    ar_state_init();
    g_ar_sound_table[22]->buffer = (void *)0xdead1;
    g_ar_sound_table[25]->buffer = (void *)0xdead2;

    ar_register_aux_sounds(NULL, 0, NULL);

    T_ASSERT_EQ_P(g_ar_sound_table[22]->buffer, (void *)0xdead1);
    T_ASSERT_EQ_P(g_ar_sound_table[25]->buffer, (void *)0xdead2);
    return 0;
}

/* ─── ar_register_locale_sounds (FUN_0057b280 tail) ──────────────── */

int test_locale_sounds_no_locale_uses_fallback_or_settings(void)
{
    /* current_settings == NULL forces every entry through PATH A.  For
     * entries with flag == -1 the slot's settings should be
     * fallback_settings; for flag == 0 it should be the caller's
     * settings.  primary_id is used as the resource_id in both cases.
     * All 268 entries with primary_id != 0 must populate; the 15
     * primary_id == 0 entries must NOT touch their slots. */
    ar_state_init();
    void *zds      = (void *)0x100;
    void *settings = (void *)0x200;
    void *fallback = (void *)0x300;
    ar_locale_state locale = {
        .fallback_settings = fallback,
        .current_settings  = NULL,           /* no locale → PATH A */
        .launcher_flag     = 0,
    };
    ar_register_locale_sounds(zds, /*group=*/7, settings, &locale);

    /* flag == 0 entry (idx 245, primary_id 0x53d): caller's settings,
     * primary_id used. */
    T_ASSERT_EQ_U(g_ar_sound_slots[245].resource_id, 0x53du);
    T_ASSERT_EQ_P(g_ar_sound_slots[245].settings, settings);
    T_ASSERT_EQ_U(g_ar_sound_slots[245].group, 7u);
    T_ASSERT_EQ_U(g_ar_sound_slots[245].count, 2u);   /* count_add 0 + 2 */
    T_ASSERT_EQ_P(g_ar_sound_slots[245].zds, zds);

    /* flag == -1 entry (idx 260, primary_id 0x3f0): fallback_settings,
     * primary_id used. */
    T_ASSERT_EQ_U(g_ar_sound_slots[260].resource_id, 0x3f0u);
    T_ASSERT_EQ_P(g_ar_sound_slots[260].settings, fallback);

    /* count_add == 2 entry (idx 160, primary_id 0x55e, flag 0): count = 4. */
    T_ASSERT_EQ_U(g_ar_sound_slots[160].resource_id, 0x55eu);
    T_ASSERT_EQ_U(g_ar_sound_slots[160].count, 4u);
    T_ASSERT_EQ_P(g_ar_sound_slots[160].settings, settings);

    /* count_add == 2 with flag == -1 (idx 209, primary_id 0x3ed): count
     * = 4, settings = fallback. */
    T_ASSERT_EQ_U(g_ar_sound_slots[209].resource_id, 0x3edu);
    T_ASSERT_EQ_U(g_ar_sound_slots[209].count, 4u);
    T_ASSERT_EQ_P(g_ar_sound_slots[209].settings, fallback);

    /* Touched-slot count = 268 (283 entries − 15 with primary_id == 0).
     * 15 indices appear in multiple entries, so distinct populated
     * slots are fewer than 268.  Both numbers verified by Python
     * stats run on the extracted table. */
    int populated = 0;
    for (int i = 0; i < AR_SOUND_SLOT_COUNT; i++) {
        if (g_ar_sound_slots[i].zds != NULL) populated++;
    }
    T_ASSERT_EQ_I(populated, 267);
    return 0;
}

int test_locale_sounds_skip_when_primary_id_zero(void)
{
    /* 15 entries have primary_id == 0 (e.g. entry with idx 268, the 30th
     * row; the entry with idx 256, the 47th row).  In PATH A (no
     * locale active) the loop skips them entirely.  But indices like
     * 268 also appear with NON-zero primary_id in earlier entries — so
     * test against a "skip-only" index: idx that NEVER has a non-zero
     * primary_id row in the entire table.
     *
     * Survey of the table shows EVERY primary_id==0 row's index also
     * appears in some other row with primary_id != 0 (they're
     * "skip-when-override-active" sentinels for live entries).  In
     * PATH A those sentinels don't trigger any write either way, so
     * the live entry's writes are observable.  Verify that for idx 268
     * the LIVE entry's writes (primary_id 0x543) are what we see. */
    ar_state_init();
    ar_locale_state locale = {
        .fallback_settings = (void *)0x300,
        .current_settings  = NULL,
        .launcher_flag     = 0,
    };
    ar_register_locale_sounds((void *)0x1, 7, (void *)0x2, &locale);

    /* idx 268's live entry has primary_id 0x543; its sentinel sibling
     * (primary_id 0x000, override 0x7fff) must not stomp it in PATH A. */
    T_ASSERT_EQ_U(g_ar_sound_slots[268].resource_id, 0x543u);
    /* idx 256's live entry has primary_id 0x613; the override-sentinel
     * sibling sits later in the table but doesn't run in PATH A. */
    T_ASSERT_EQ_U(g_ar_sound_slots[256].resource_id, 0x613u);
    return 0;
}

int test_locale_sounds_launcher_flag_forces_fallback(void)
{
    /* launcher_flag != 0 makes the loop take PATH A for every entry,
     * even when current_settings is present.  Pick an entry with a
     * non-zero override (idx 245, override 0x08bb): with launcher_flag
     * suppressed, resource_id should be the primary 0x53d (not the
     * override). */
    ar_state_init();
    ar_locale_state locale = {
        .fallback_settings = (void *)0x300,
        .current_settings  = (void *)0x400,
        .launcher_flag     = 1,              /* suppress override */
    };
    ar_register_locale_sounds((void *)0x1, 7, (void *)0x2, &locale);
    T_ASSERT_EQ_U(g_ar_sound_slots[245].resource_id, 0x53du);
    T_ASSERT_EQ_P(g_ar_sound_slots[245].settings, (void *)0x2);
    return 0;
}

int test_locale_sounds_override_path_uses_current_locale(void)
{
    /* current_settings != NULL && launcher_flag == 0 → PATH B for
     * entries with override != 0 && != 0x7fff.  Each such entry uses
     * the override as resource_id and current_settings as the settings
     * pointer. */
    ar_state_init();
    void *settings = (void *)0x200;
    void *fallback = (void *)0x300;
    void *current  = (void *)0x400;
    ar_locale_state locale = {
        .fallback_settings = fallback,
        .current_settings  = current,
        .launcher_flag     = 0,
    };
    ar_register_locale_sounds((void *)0x1, 7, settings, &locale);

    /* idx 245 (override 0x08bb): res_id == override, settings == current. */
    T_ASSERT_EQ_U(g_ar_sound_slots[245].resource_id, 0x08bbu);
    T_ASSERT_EQ_P(g_ar_sound_slots[245].settings, current);
    /* idx 311 (override 0x08ff): same. */
    T_ASSERT_EQ_U(g_ar_sound_slots[311].resource_id, 0x08ffu);
    T_ASSERT_EQ_P(g_ar_sound_slots[311].settings, current);

    /* Entries with override == 0 always take PATH A even under locale.
     * idx 160's entry has override == 0 — primary_id 0x55e, flag 0,
     * count_add 2: should still get the caller's settings + primary id. */
    T_ASSERT_EQ_U(g_ar_sound_slots[160].resource_id, 0x55eu);
    T_ASSERT_EQ_P(g_ar_sound_slots[160].settings, settings);

    /* Entries with flag == -1 AND override != 0: PATH B still wins
     * (override-path doesn't care about flag).  idx 260 (primary 0x3f0,
     * flag -1, override 0x08b9): should resolve to 0x08b9 + current. */
    T_ASSERT_EQ_U(g_ar_sound_slots[260].resource_id, 0x08b9u);
    T_ASSERT_EQ_P(g_ar_sound_slots[260].settings, current);
    return 0;
}

int test_locale_sounds_override_7fff_skips_under_locale(void)
{
    /* Under PATH B, override == 0x7fff makes the loop skip the entry
     * (the "skip-when-override-active" sentinel).  These sentinels
     * appear AFTER a live entry for the same idx, so the live entry's
     * writes survive — verify the sentinel doesn't stomp them.
     *
     * Pick idx 268 (live entry: primary 0x543, override 0x08e1;
     * sentinel: primary 0x000, override 0x7fff).  Under PATH B the
     * live entry takes the override (0x08e1) and the sentinel is
     * skipped — the slot stays at 0x08e1. */
    ar_state_init();
    void *current = (void *)0x400;
    ar_locale_state locale = {
        .fallback_settings = (void *)0x300,
        .current_settings  = current,
        .launcher_flag     = 0,
    };
    ar_register_locale_sounds((void *)0x1, 7, (void *)0x2, &locale);
    T_ASSERT_EQ_U(g_ar_sound_slots[268].resource_id, 0x08e1u);
    T_ASSERT_EQ_P(g_ar_sound_slots[268].settings, current);
    return 0;
}

int test_locale_sounds_coexists_with_game_sounds(void)
{
    /* Boot driver runs ar_register_game_sounds first then the locale
     * tail.  Touched-index ranges: game_sounds 12..244, locale 160..464
     * — overlap 160..244 (85 slots).  In the overlap, locale_sounds'
     * writes must win (it runs second in retail issue order).  Outside
     * the overlap, each batch's writes should be preserved. */
    ar_state_init();
    void *zds = (void *)0x1;
    void *settings = (void *)0x2;
    void *fallback = (void *)0x300;
    ar_register_game_sounds(zds, /*group=*/3, settings);

    ar_locale_state locale = {
        .fallback_settings = fallback,
        .current_settings  = NULL,
        .launcher_flag     = 0,
    };
    ar_register_locale_sounds(zds, /*group=*/3, settings, &locale);

    /* Outside overlap (game_sounds-only): idx 14 keeps id 0x513. */
    T_ASSERT_EQ_U(g_ar_sound_slots[14].resource_id, 0x513u);
    /* Inside overlap: idx 160 was NOT touched by game_sounds (it's a
     * locale-only index in the 160..244 overlap range); locale's write
     * survives. */
    T_ASSERT_EQ_U(g_ar_sound_slots[160].resource_id, 0x55eu);
    /* idx 209 (game_sounds doesn't write this idx; locale-only):
     * primary_id 0x3ed, flag -1. */
    T_ASSERT_EQ_U(g_ar_sound_slots[209].resource_id, 0x3edu);
    /* Locale-only (above overlap): idx 245 picks up primary 0x53d. */
    T_ASSERT_EQ_U(g_ar_sound_slots[245].resource_id, 0x53du);
    return 0;
}

int test_locale_sounds_buffer_pointer_preserved(void)
{
    /* Same invariant as the other sound registers — `buffer` survives
     * across re-registration. */
    ar_state_init();
    g_ar_sound_table[245]->buffer = (void *)0xfa11;
    g_ar_sound_table[464]->buffer = (void *)0xfa12;

    ar_locale_state locale = {
        .fallback_settings = NULL,
        .current_settings  = NULL,
        .launcher_flag     = 0,
    };
    ar_register_locale_sounds(NULL, 0, NULL, &locale);

    T_ASSERT_EQ_P(g_ar_sound_table[245]->buffer, (void *)0xfa11);
    T_ASSERT_EQ_P(g_ar_sound_table[464]->buffer, (void *)0xfa12);
    return 0;
}

/* ─── ar_register_group3_sprites (FUN_0057ca40 partial port) ────── */

int test_group3_sprites_writes_233_distinct_slots(void)
{
    /* Every entry hits a unique slot index — order between entries
     * doesn't matter (each register writes a different slot). */
    ar_state_init();
    ar_register_group3_sprites((void *)0xd00d, /*group=*/3, (void *)0xbeef);

    int written = 0;
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (g_ar_sprite_slots[i].entries != NULL) written++;
    }
    T_ASSERT_EQ_U(written, 233u);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

int test_group3_sprites_group_tag_stamped(void)
{
    /* All 233 entries get the same group tag — pin it to caller's
     * value, not a hardcoded constant. */
    ar_state_init();
    ar_register_group3_sprites((void *)0x1, /*group=*/0xc0de, (void *)0x2);

    int seen = 0;
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (g_ar_sprite_slots[i].entries != NULL) {
            T_ASSERT_EQ_U(g_ar_sprite_slots[i].group, 0xc0deu);
            seen++;
        }
    }
    T_ASSERT_EQ_U(seen, 233u);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

int test_group3_sprites_zdd_and_settings_uniform(void)
{
    /* Every entry receives the same (zdd, settings) pair — uniform
     * routing (no NULL-special-case entries unlike main_sprites idx
     * 37 which has settings=NULL). */
    ar_state_init();
    void *zdd = (void *)0xaaaa, *settings = (void *)0xbbbb;
    ar_register_group3_sprites(zdd, 3, settings);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        if (g_ar_sprite_slots[i].entries != NULL) {
            T_ASSERT_EQ_P(g_ar_sprite_slots[i].zdd,      zdd);
            T_ASSERT_EQ_P(g_ar_sprite_slots[i].settings, settings);
        }
    }

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

int test_group3_sprites_spotcheck_first_entry(void)
{
    /* First decomp block (line 12, retail 0x008a777c → idx 79):
     *   resource_id=0x423, width=0x20, height=0x20, colorkey=0,
     *   scale_flag=0, type=2 */
    ar_state_init();
    ar_register_group3_sprites((void *)0x1, /*group=*/3, (void *)0x2);

    T_ASSERT_EQ_U(g_ar_sprite_slots[79].resource_id, 0x423u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].width,       0x20u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].height,      0x20u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].colorkey,    0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].scale_flag,  0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].type,        2u);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

int test_group3_sprites_spotcheck_max_idx(void)
{
    /* Last entry in the slot pool — retail 0x008a7cdc → idx 423.
     * (Resource id 0x480, 80×80, scale=1, type=2.)  Pins both the
     * widest pool reach and a sample of the high-numbered slots. */
    ar_state_init();
    ar_register_group3_sprites((void *)0x1, /*group=*/3, (void *)0x2);

    T_ASSERT_EQ_U(g_ar_sprite_slots[423].resource_id, 0x480u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[423].width,       0x50u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[423].height,      0x50u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[423].scale_flag,  1u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[423].type,        2u);
    /* idx 422 is its twin (retail 0x008a7cd8). */
    T_ASSERT_EQ_U(g_ar_sprite_slots[422].resource_id, 0x47fu);
    T_ASSERT_EQ_U(g_ar_sprite_slots[422].width,       0x28u);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

int test_group3_sprites_spotcheck_colorkey_entry(void)
{
    /* idx 81 (retail 0x008a7784): a portrait-shaped entry —
     *   resource_id=0x6a6, w=0xc0, h=0xa0, colorkey=0x1ffffff,
     *   scale=0, type=0  (the "1ffffff" magic colorkey is the
     *   "no-colorkey" sentinel used by the engine's portrait
     *   sprites — see ar_register_palette_ramps portraits). */
    ar_state_init();
    ar_register_group3_sprites((void *)0x1, 3, (void *)0x2);

    T_ASSERT_EQ_U(g_ar_sprite_slots[81].resource_id, 0x6a6u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[81].width,       0xc0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[81].height,      0xa0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[81].colorkey,    0x1ffffffu);
    T_ASSERT_EQ_U(g_ar_sprite_slots[81].type,        0u);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

int test_group3_sprites_no_overlap_with_main_sprite_indices(void)
{
    /* Group-3 should NOT touch indices 0..78 (those are owned by
     * ar_register_fonts / main_sprites / palette_ramps / game_sprites)
     * — its lowest written idx is 79.  Also verify it doesn't reach
     * the game_sprites-only range (above 423 in this function). */
    ar_state_init();
    ar_register_group3_sprites((void *)0x1, 3, (void *)0x2);

    for (int i = 0; i < 79; i++) {
        T_ASSERT(g_ar_sprite_slots[i].entries == NULL);
    }
    for (int i = 424; i < AR_SPRITE_SLOT_COUNT; i++) {
        T_ASSERT(g_ar_sprite_slots[i].entries == NULL);
    }

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

/* ─── ar_apply_group3_info_events (4th pass of FUN_0057ca40) ────── */

int test_group3_info_events_first_event_flag_set(void)
{
    /* L40 in 57ca40.c — first event of the table:
     * `*(undefined4 *)(DAT_008a85b0 + 4) = 1;` → pool[92].flag = 1. */
    ar_state_init();
    ar_apply_group3_info_events();

    T_ASSERT_EQ_U(g_ar_info_table[92]->flag, 1u);
    return 0;
}

int test_group3_info_events_marker_and_flag_pair(void)
{
    /* L229-230: `*DAT_008a85c4 = 0; *(undefined4 *)(DAT_008a85c4 + 2) = 1;`
     * → pool[97].marker = 0, pool[97].flag = 1.  Spot-checks that a
     * marker-then-flag bundle on the same pool index both land. */
    ar_state_init();
    ar_apply_group3_info_events();

    T_ASSERT_EQ_U(g_ar_info_table[97]->marker, 0u);
    T_ASSERT_EQ_U(g_ar_info_table[97]->flag,   1u);
    return 0;
}

int test_group3_info_events_data_ptr_set(void)
{
    /* L2493: `*(undefined **)(DAT_008a88d0 + 8) = &DAT_006748d0;`
     * → pool[292].data = (const void *)0x006748d0.  Verifies
     * DATA_SET events lay down the retail PE rdata address
     * verbatim (observability-only — no consumer reads bytes yet). */
    ar_state_init();
    ar_apply_group3_info_events();

    T_ASSERT_EQ_P(g_ar_info_table[292]->data, (const void *)0x006748d0u);
    return 0;
}

int test_group3_info_events_marker_set_with_high_value(void)
{
    /* L2491: `*DAT_008a88cc = 0x1c;` → pool[291].marker = 0x1c.
     * Spot-checks a non-zero marker write (most markers are 0; this
     * one's part of the marker-bearing SS_MGR-clone cluster region). */
    ar_state_init();
    ar_apply_group3_info_events();

    T_ASSERT_EQ_U(g_ar_info_table[291]->marker, 0x1cu);
    T_ASSERT_EQ_U(g_ar_info_table[291]->flag,   2u);
    return 0;
}

int test_group3_info_events_copy_marker_and_flag_chain(void)
{
    /* L2140-2141: pool[384].marker = pool[381].marker;
     *             pool[384].flag   = pool[381].flag;
     * The source pool[381] is written at L2003 (`pool[381].flag = 2`)
     * — and L2003 fires BEFORE L2140 in source order, so the copy
     * picks up flag=2 (marker stays 0 — never written for pool[381]).
     * Verifies MARKER_COPY + FLAG_COPY read post-write state. */
    ar_state_init();
    ar_apply_group3_info_events();

    T_ASSERT_EQ_U(g_ar_info_table[381]->marker, 0u);
    T_ASSERT_EQ_U(g_ar_info_table[381]->flag,   2u);
    T_ASSERT_EQ_U(g_ar_info_table[384]->marker, g_ar_info_table[381]->marker);
    T_ASSERT_EQ_U(g_ar_info_table[384]->flag,   g_ar_info_table[381]->flag);
    /* And the data ptr that follows the copy at L2142: */
    T_ASSERT_EQ_P(g_ar_info_table[384]->data, (const void *)0x006752f8u);
    return 0;
}

int test_group3_info_events_struct_copy_from_zero_init(void)
{
    /* L3083: `pool[257] = pool[139]` — but pool[139] is never written
     * inside FUN_0057ca40 (or any earlier batch).  So the struct copy
     * lands an all-zero entry into pool[257], matching retail (the
     * allocator zero-inits all 909 entries at boot, then most stay
     * untouched).  Rabbit-hole §4 documents the 5-pair pattern. */
    ar_state_init();
    ar_apply_group3_info_events();

    T_ASSERT_EQ_U(g_ar_info_table[139]->marker, 0u);
    T_ASSERT_EQ_U(g_ar_info_table[139]->flag,   0u);
    T_ASSERT_EQ_U(g_ar_info_table[257]->marker, 0u);
    T_ASSERT_EQ_U(g_ar_info_table[257]->flag,   0u);
    T_ASSERT_EQ_P(g_ar_info_table[257]->data,   NULL);
    return 0;
}

int test_group3_info_events_dst_indices_in_92_to_437_range(void)
{
    /* Every event in the table targets a pool index in [92, 437]
     * (the FUN_0057ca40 write region per rabbit-hole §2).  Indices
     * below 92 are owned by ar_register_palette_ramps; indices above
     * 437 are untouched at boot.  Sweep the pool and pin the bounds. */
    ar_state_init();
    ar_apply_group3_info_events();

    /* pool[0..91] should be all-zero (ramp-flag region is 78..91; the
     * ramp register batch isn't called here, only the info events). */
    for (int i = 0; i < 92; i++) {
        T_ASSERT_EQ_U(g_ar_info_table[i]->marker, 0u);
        T_ASSERT_EQ_U(g_ar_info_table[i]->flag,   0u);
        T_ASSERT_EQ_P(g_ar_info_table[i]->data,   NULL);
    }
    /* pool[438..908] untouched too. */
    for (int i = 438; i < AR_INFO_ENTRY_COUNT; i++) {
        T_ASSERT_EQ_U(g_ar_info_table[i]->marker, 0u);
        T_ASSERT_EQ_U(g_ar_info_table[i]->flag,   0u);
        T_ASSERT_EQ_P(g_ar_info_table[i]->data,   NULL);
    }
    return 0;
}

int test_group3_info_events_fires_from_register_group3_sprites(void)
{
    /* ar_register_group3_sprites's tail calls ar_apply_group3_info_events
     * — verify the integration so the boot driver doesn't need a
     * separate call.  Pick a few independent spot-checks. */
    ar_state_init();
    ar_register_group3_sprites((void *)0x1, /*group=*/3, (void *)0x2);

    T_ASSERT_EQ_U(g_ar_info_table[92]->flag,   1u);
    T_ASSERT_EQ_U(g_ar_info_table[291]->marker, 0x1cu);
    T_ASSERT_EQ_P(g_ar_info_table[384]->data, (const void *)0x006752f8u);

    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    return 0;
}

/* ─── ar_boot_register_all (FUN_00562ea0:613-624 wiring) ─────────── */

/* Boot driver replays every ported register batch in retail issue
 * order.  These tests pin the wiring: group tags, zdd/zds routing,
 * locale plumbing, and the sotesp_module split.
 *
 * The palette-install body inside ar_register_palette_ramps depends on
 * bs_load_pe_resource returning a valid 8bpp resource.  The recording
 * stub lives in test_bitmap_session.c and its resource table starts
 * empty when these tests run (asset tests are registered before
 * bitmap_session tests in test_main.c).  An empty table makes
 * ar_palette_session_begin return false, which makes the ramp's
 * palette-install a no-op — the slot REGISTER side still runs, which
 * is what these tests assert.  Standalone palette-install behaviour is
 * already covered by test_bitmap_session's palette_ramps_* tests. */

static void boot_register_all_cleanup(void)
{
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_slots[i]);
    for (int i = 0; i < AR_SPRITE_RAMP_COUNT; i++)
        ar_sprite_slot_destroy(&g_ar_sprite_ramp_slots[i]);
    for (int i = 0; i < AR_GDI_SLOT_COUNT; i++)
        ar_gdi_slot_destroy(g_ar_gdi_table[i]);
}

int test_boot_register_all_group_tags_per_batch(void)
{
    /* Each batch stamps its own group number — 1 fonts/sounds, 2
     * palette_ramps/aux_sounds, 3 game_sounds/locale_sounds, 4
     * main_sprites, 5 game_sprites.  Verify by spot-checking one
     * touched slot per batch. */
    stub_reset();
    ar_state_init();
    void *zdd = (void *)0xd00d, *zds = (void *)0xa11a;
    void *settings = (void *)0xbeef, *sotesp = (void *)0xfeed;
    ar_locale_state locale = { NULL, NULL, 0 };

    ar_boot_register_all(zdd, zds, settings, sotesp, &locale);

    /* Group 1 — fonts (gdi_table[1]) + sounds (sound_table[0]). */
    T_ASSERT_EQ_U(g_ar_gdi_table[1]->group, 1u);
    T_ASSERT_EQ_U(g_ar_sound_table[0]->group, 1u);
    /* Group 2 — palette_ramps (ramp_slots[0]) + aux_sounds (sound_table[22]). */
    T_ASSERT_EQ_U(g_ar_sprite_ramp_slots[0].group, 2u);
    T_ASSERT_EQ_U(g_ar_sound_table[22]->group, 2u);
    /* Group 3 — game_sounds (sound_table[14] — first inline entry). */
    T_ASSERT_EQ_U(g_ar_sound_table[14]->group, 3u);
    /* Group 4 — main_sprites (sprite_slots[1] — first inline). */
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].group, 4u);
    /* Group 5 — game_sprites (sprite_slots[62] — lowest touched idx
     * by FUN_0056e190). */
    T_ASSERT_EQ_U(g_ar_sprite_slots[62].group, 5u);

    boot_register_all_cleanup();
    return 0;
}

int test_boot_register_all_zdd_vs_zds_routing(void)
{
    /* Sprite batches must receive zdd; sound batches must receive zds.
     * Verify by sampling one slot per kind. */
    stub_reset();
    ar_state_init();
    void *zdd = (void *)0xd0d0, *zds = (void *)0x5e5e;
    ar_locale_state locale = { NULL, NULL, 0 };
    ar_boot_register_all(zdd, zds, (void *)0x1, (void *)0x2, &locale);

    /* Sprite slot (game sprites) → zdd. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[62].zdd, zdd);
    /* Sprite slot (main sprites idx 1) → zdd. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[1].zdd, zdd);
    /* Ramp slot → zdd. */
    T_ASSERT_EQ_P(g_ar_sprite_ramp_slots[0].zdd, zdd);
    /* Sound slots (main, aux, game, locale-fallback) → zds. */
    T_ASSERT_EQ_P(g_ar_sound_table[0]->zds,   zds);   /* main sounds */
    T_ASSERT_EQ_P(g_ar_sound_table[22]->zds,  zds);   /* aux sound idx 22 */
    T_ASSERT_EQ_P(g_ar_sound_table[14]->zds,  zds);   /* game sound idx 14 */

    boot_register_all_cleanup();
    return 0;
}

int test_boot_register_all_sotesp_module_for_special_slots(void)
{
    /* main_sprites idx 0 and the 12 palette-ramp slots take
     * sotesp_module as their settings pointer (not the caller's
     * `settings`).  Every other slot takes the caller's settings. */
    stub_reset();
    ar_state_init();
    void *settings = (void *)0xaaaa, *sotesp = (void *)0xbbbb;
    ar_locale_state locale = { NULL, NULL, 0 };
    ar_boot_register_all((void *)0x1, (void *)0x2, settings, sotesp, &locale);

    /* main_sprites idx 0 — sotesp pointer wins. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[0].settings, sotesp);
    /* All 12 ramp slots — sotesp pointer (palette-ramp source). */
    for (int i = 0; i < AR_SPRITE_RAMP_COUNT; i++)
        T_ASSERT_EQ_P(g_ar_sprite_ramp_slots[i].settings, sotesp);
    /* main_sprites idx 1 — caller's settings. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[1].settings, settings);
    /* game_sprites idx 62 — caller's settings. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[62].settings, settings);
    /* fonts sprite (idx AR_SPR_FONT_TEX_457) — caller's settings. */
    T_ASSERT_EQ_P(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].settings, settings);
    /* Sound slots — caller's settings. */
    T_ASSERT_EQ_P(g_ar_sound_table[14]->settings, settings);

    boot_register_all_cleanup();
    return 0;
}

int test_boot_register_all_locale_state_routed(void)
{
    /* When a valid locale with current_settings != NULL is passed, the
     * locale_sounds tail should route override-path entries through
     * current_settings.  Verify on idx 245 (override 0x08bb, primary
     * 0x53d): under override the resource_id is 0x08bb and the
     * settings pointer is current_settings. */
    stub_reset();
    ar_state_init();
    void *settings = (void *)0x100;
    void *current  = (void *)0x200;
    void *fallback = (void *)0x300;
    ar_locale_state locale = {
        .fallback_settings = fallback,
        .current_settings  = current,
        .launcher_flag     = 0,
    };
    ar_boot_register_all((void *)0x1, (void *)0x2, settings, (void *)0x3, &locale);

    T_ASSERT_EQ_U(g_ar_sound_table[245]->resource_id, 0x08bbu);
    T_ASSERT_EQ_P(g_ar_sound_table[245]->settings,    current);

    boot_register_all_cleanup();
    return 0;
}

int test_boot_register_all_null_locale_skips_tail(void)
{
    /* locale == NULL → ar_register_locale_sounds is NOT called.  Slot
     * indices touched only by the locale tail (e.g. idx 245, the first
     * locale-only index) must stay zero. */
    stub_reset();
    ar_state_init();
    ar_boot_register_all((void *)0x1, (void *)0x2, (void *)0x3, (void *)0x4,
                         /*locale=*/NULL);

    /* idx 245 — locale-only.  zds NULL means uninhabited. */
    T_ASSERT_EQ_P(g_ar_sound_table[245]->zds, NULL);
    T_ASSERT_EQ_U(g_ar_sound_table[245]->resource_id, 0u);
    /* But game_sounds (idx 14) still got written. */
    T_ASSERT_EQ_U(g_ar_sound_table[14]->resource_id, 0x513u);

    boot_register_all_cleanup();
    return 0;
}

int test_boot_register_all_touches_every_batch_signature_slot(void)
{
    /* One known slot per batch as a "did this batch run?" canary.
     * Resource IDs match the documented retail values so this also
     * catches accidental cross-batch reordering. */
    stub_reset();
    ar_state_init();
    ar_locale_state locale = {
        .fallback_settings = (void *)0x300,
        .current_settings  = NULL,           /* PATH A for locale tail */
        .launcher_flag     = 0,
    };
    ar_boot_register_all((void *)0xd00d, (void *)0xa11a,
                         (void *)0xbeef, (void *)0xfeed, &locale);

    /* ar_register_fonts — gdi_table[14] is the pen slot it sets up. */
    T_ASSERT_EQ_U(g_ar_gdi_table[14]->count,    1u);
    T_ASSERT_EQ_U(g_ar_gdi_table[14]->capacity, 1u);
    /* ar_register_fonts — sprite at AR_SPR_FONT_TEX_457. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[AR_SPR_FONT_TEX_457].resource_id, 0x457u);
    /* ar_register_sounds — sound_table[0] gets id 0x50f. */
    T_ASSERT_EQ_U(g_ar_sound_table[0]->resource_id, 0x50fu);
    /* ar_register_palette_ramps — ramp_slots[0] gets id 0x413. */
    T_ASSERT_EQ_U(g_ar_sprite_ramp_slots[0].resource_id, 0x413u);
    /* ar_register_aux_sounds — sound_table[22] gets id 0x4cb. */
    T_ASSERT_EQ_U(g_ar_sound_table[22]->resource_id, 0x4cbu);
    /* ar_register_group3_sprites — sprite_slots[79] gets id 0x423
     * (first decomp block of FUN_0057ca40). */
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].resource_id, 0x423u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[79].group,       3u);
    /* ar_register_game_sounds — sound_table[14] gets id 0x513. */
    T_ASSERT_EQ_U(g_ar_sound_table[14]->resource_id, 0x513u);
    /* ar_register_locale_sounds — sound_table[245] gets primary 0x53d
     * (PATH A: current_settings NULL). */
    T_ASSERT_EQ_U(g_ar_sound_table[245]->resource_id, 0x53du);
    /* ar_register_main_sprites — sprite_slots[1] gets id 0x49f. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].resource_id, 0x49fu);
    /* ar_register_game_sprites — sprite_slots[62] is FUN_0056e190's
     * first touched idx; verify it received its entry. */
    T_ASSERT(g_ar_sprite_slots[62].entries != NULL);
    T_ASSERT_EQ_U(g_ar_sprite_slots[62].entry_count, 1u);

    boot_register_all_cleanup();
    return 0;
}

/* ─── layout parity (32-bit only) ────────────────────────────────── */

int test_ar_layout_matches_retail(void)
{
#if UINTPTR_MAX != 0xFFFFFFFFu
    T_SKIP("host build (64-bit) — layout asserts are 32-bit-only");
#endif
    /* Static asserts in the header already enforce these.  If the host
     * is 32-bit, the static asserts have fired at compile time; if
     * not, the test compiles as a skip. */
    T_ASSERT(sizeof(ar_gdi_slot) == 12);
    T_ASSERT(sizeof(ar_sprite_slot) == 0x44);
    T_ASSERT(sizeof(ar_sprite_entry) == 8);
    T_ASSERT(sizeof(ar_sound_slot) == 0x18);
    return 0;
}
