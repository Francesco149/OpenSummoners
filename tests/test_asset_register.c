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
    T_ASSERT_EQ_P(g_ar_sprite_slots[0].zdd, zdd);
    T_ASSERT_EQ_P(g_ar_sprite_slots[0].settings, settings);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].resource_id, 0x457u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].width, 0x20u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].height, 0x20u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].colorkey, 0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].scale_flag, 0u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].type, 2u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].group, 0xabcdu);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].entry_count, 1u);
    T_ASSERT(g_ar_sprite_slots[0].entries != NULL);
    T_ASSERT_EQ_U(g_ar_sprite_slots[0].entries[0].a, 0u);
    T_ASSERT_EQ_P(g_ar_sprite_slots[0].entries[0].b, NULL);

    /* sprite[1]: id 0x455, 32×48, scale=1. */
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].resource_id, 0x455u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].height, 0x30u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].scale_flag, 1u);
    T_ASSERT_EQ_U(g_ar_sprite_slots[1].group, 0xabcdu);

    /* Clean up to keep ASan happy across test runs. */
    ar_sprite_slot_destroy(&g_ar_sprite_slots[0]);
    ar_sprite_slot_destroy(&g_ar_sprite_slots[1]);
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
    ar_sprite_slot_destroy(&g_ar_sprite_slots[0]);
    ar_sprite_slot_destroy(&g_ar_sprite_slots[1]);
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
    ar_sprite_slot_destroy(&g_ar_sprite_slots[0]);
    ar_sprite_slot_destroy(&g_ar_sprite_slots[1]);
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
    static const struct exp expected[AR_SOUND_SLOT_COUNT] = {
        { 0x50f, 2 }, { 0x50e, 2 }, { 0x508, 2 }, { 0x510, 2 },
        { 0x903, 2 }, { 0x509, 4 }, { 0x506, 4 }, { 0x507, 2 },
        { 0x50c, 4 }, { 0x50d, 4 }, { 0x4d8, 2 }, { 0x4d9, 2 },
    };
    for (int i = 0; i < AR_SOUND_SLOT_COUNT; i++) {
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
