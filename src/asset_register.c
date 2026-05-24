/*
 * src/asset_register.c — Asset-register module port.
 *
 * Pure C, Win32-free.  The GDI primitive wrappers
 * (ar_gdi_create_font/pen/brush, ar_gdi_delete,
 * ar_gdi_get_fallback_face_name) are externs supplied by either
 * src/asset_register_win32.c (real build) or the test harness
 * (recording stubs).
 *
 * Per-function provenance is in asset_register.h.
 */
#include "asset_register.h"

#include <stdlib.h>
#include <string.h>

/* ─── globals ────────────────────────────────────────────────────── */

ar_sprite_slot  g_ar_sprite_slots[2];
ar_gdi_slot     g_ar_gdi_slots[AR_GDI_SLOT_COUNT];
ar_gdi_slot    *g_ar_gdi_table[AR_GDI_SLOT_COUNT];
ar_sound_slot   g_ar_sound_slots[AR_SOUND_SLOT_COUNT];
ar_sound_slot  *g_ar_sound_table[AR_SOUND_SLOT_COUNT];

void ar_state_init(void)
{
    memset(g_ar_sprite_slots, 0, sizeof g_ar_sprite_slots);
    memset(g_ar_gdi_slots,    0, sizeof g_ar_gdi_slots);
    memset(g_ar_sound_slots,  0, sizeof g_ar_sound_slots);
    for (int i = 0; i < AR_GDI_SLOT_COUNT; i++) {
        g_ar_gdi_table[i] = &g_ar_gdi_slots[i];
    }
    for (int i = 0; i < AR_SOUND_SLOT_COUNT; i++) {
        g_ar_sound_table[i] = &g_ar_sound_slots[i];
    }
}

/* ─── FUN_005bef0e — delete[] thunk ─────────────────────────────── */

void ar_xfree(void *p)
{
    /* FUN_005c123f is operator delete[]; on the host side `free` is the
     * matching pair since our allocations use malloc. */
    free(p);
}

/* ─── FUN_005b5f50 — per-channel COLORREF lerp ──────────────────── */

uint32_t ar_color_lerp(uint32_t src, uint32_t dst, int32_t num, int32_t denom)
{
    /* Retail loops three times over shifts 0, 8, 16.  Each iteration:
     *   chan_src = (src >> shift) & 0xff
     *   chan_dst = (dst >> shift) & 0xff
     *   out_chan = (chan_dst - chan_src) * num / denom + chan_src
     *   accumulator += out_chan << shift
     * The arithmetic is signed (decompile shows `(int)` cast) which
     * matters when chan_dst < chan_src — the difference goes negative
     * and the post-`/denom` keeps that sign, so the interpolant can
     * step in either direction. */
    int32_t acc = 0;
    for (int shift = 0; shift < 24; shift += 8) {
        int32_t s = (int32_t)((src >> shift) & 0xffu);
        int32_t d = (int32_t)((dst >> shift) & 0xffu);
        int32_t out = (d - s) * num / denom + s;
        acc += out << shift;
    }
    return (uint32_t)acc;
}

/* ─── FUN_00417b50 — sprite slot destructor ─────────────────────── */

void ar_sprite_slot_destroy(ar_sprite_slot *s)
{
    if (s->aux_buf != NULL) {
        ar_xfree(s->aux_buf);
        s->aux_buf = NULL;
    }
    if (s->entries != NULL) {
        /* Loop runs `entry_count` iterations; on a destroyed slot
         * entry_count is 0 from BSS / a previous destroy and the loop
         * is skipped. */
        for (uint16_t i = 0; i < s->entry_count; i++) {
            if (s->entries[i].b != NULL) {
                ar_xfree(s->entries[i].b);
                s->entries[i].b = NULL;
            }
        }
        ar_xfree(s->entries);
        s->entries = NULL;
    }
}

/* ─── FUN_00562a10 — GDI slot destroy ───────────────────────────── */

void ar_gdi_slot_destroy(ar_gdi_slot *s)
{
    if (s->array != NULL) {
        for (uint16_t i = 0; i < s->capacity; i++) {
            if (s->array[i] != NULL) {
                ar_gdi_delete(s->array[i]);
                s->array[i] = NULL;
            }
        }
        ar_xfree(s->array);
        s->capacity = 0;
        s->count    = 0;
        s->array    = NULL;
    }
}

/* ─── FUN_00579ec0 — destroy + reallocate ───────────────────────── */

void ar_gdi_slot_reset(ar_gdi_slot *s, uint16_t capacity)
{
    /* Retail body inlines the destroy: same loop + same field clears.
     * We call the destroy helper — observable behaviour matches. */
    ar_gdi_slot_destroy(s);
    /* operator_new(capacity << 2) → array of `capacity` HGDIOBJ.  The
     * retail body uses calloc-style raw operator_new and doesn't
     * explicitly zero, but the subsequent writes only touch
     * array[count].  We calloc-equivalent for predictability. */
    s->array    = (ar_gdi_handle *)calloc(capacity, sizeof(ar_gdi_handle));
    s->count    = 0;
    s->capacity = capacity;
}

/* ─── shared font-face builder used by ar_make_font / set_font ──── */

static const char *ar_font_face_for_family(uint32_t family, int *italic_out)
{
    *italic_out = 0;
    switch (family) {
    case 3:  return "Times New Roman";
    case 4:  return "Arial";
    case 5:  *italic_out = 1; /* fallthrough */
    case 0:
    case 1:
    case 2:  return "Courier New";
    default: return NULL;   /* the retail switchD_*_default jumps over the strcpy */
    }
}

/* ─── FUN_00579f40 — make one HFONT ─────────────────────────────── */

ar_gdi_handle ar_make_font(int32_t width, int32_t height, uint32_t family)
{
    int italic = 0;
    const char *face = ar_font_face_for_family(family, &italic);

    ar_gdi_handle h = ar_gdi_create_font(width, height, italic,
                                         face != NULL ? face : "");
    if (h == NULL) {
        /* Fallback: copy the runtime override at DAT_008a9b6c.  Same
         * width/height/italic/charset preserved.  The retail body
         * REPLACES lfFaceName with this string unconditionally before
         * the retry — even if the first attempt had a valid face name.
         * We match that. */
        const char *fallback = ar_gdi_get_fallback_face_name();
        if (fallback == NULL) fallback = "";
        h = ar_gdi_create_font(width, height, italic, fallback);
    }
    /* NB: the retail body allocates a 4-byte block via operator_new
     * and stores the HFONT into it, then leaks the block (the variable
     * is never read).  We omit that — ASan-clean and no observable
     * effect on the engine. */
    return h;
}

/* ─── FUN_0057a030 — install one HFONT into slot[0] ─────────────── */

void ar_gdi_slot_set_font(ar_gdi_slot *s, uint16_t group,
                          int32_t width, int32_t height, uint32_t family)
{
    /* Order matches retail exactly: group is stamped BEFORE the
     * destroy.  Tests that assert "destroy is called" need to
     * remember the slot already has `group` written. */
    s->group = group;
    ar_gdi_slot_destroy(s);

    s->array    = (ar_gdi_handle *)calloc(1, sizeof(ar_gdi_handle));
    s->count    = 0;            /* retail never bumps count here — see hdr */
    s->capacity = 1;

    int italic = 0;
    const char *face = ar_font_face_for_family(family, &italic);

    ar_gdi_handle h = ar_gdi_create_font(width, height, italic,
                                         face != NULL ? face : "");
    if (h == NULL) {
        const char *fallback = ar_gdi_get_fallback_face_name();
        if (fallback == NULL) fallback = "";
        h = ar_gdi_create_font(width, height, italic, fallback);
    }
    s->array[0] = h;
}

/* ─── FUN_0057a1a0 — install one HPEN at slot[0], bump count ────── */

void ar_gdi_slot_set_pen(ar_gdi_slot *s, int32_t width, uint32_t color,
                         uint16_t group, uint16_t capacity)
{
    s->group = group;
    ar_gdi_slot_destroy(s);

    s->array    = (ar_gdi_handle *)calloc(capacity, sizeof(ar_gdi_handle));
    s->count    = 0;
    s->capacity = capacity;

    if (capacity != 0) {
        /* PS_SOLID == 0.  Retail writes the handle into array[count]
         * (which is 0 here), THEN bumps count to 1. */
        ar_gdi_handle h = ar_gdi_create_pen(0, width, color);
        s->array[s->count] = h;
        s->count++;
    }
}

/* ─── FUN_0057a260 — install one HBRUSH at slot[0], bump count ──── */

void ar_gdi_slot_set_brush(ar_gdi_slot *s, uint32_t color,
                           uint16_t group, uint16_t capacity)
{
    s->group = group;
    ar_gdi_slot_destroy(s);

    s->array    = (ar_gdi_handle *)calloc(capacity, sizeof(ar_gdi_handle));
    s->count    = 0;
    s->capacity = capacity;

    if (capacity != 0) {
        ar_gdi_handle h = ar_gdi_create_brush(color);
        s->array[s->count] = h;
        s->count++;
    }
}

/* ─── FUN_00582d10 — pen gradient ramp into g_ar_gdi_table[index] ─ */

void ar_gdi_slot_set_pen_gradient(uint32_t index, uint16_t group, uint16_t capacity,
                                   uint32_t color_a, uint32_t color_mid,
                                   uint32_t color_c, uint32_t width_a,
                                   uint32_t width_step, uint32_t width_c)
{
    ar_gdi_slot *s = g_ar_gdi_table[index & 0xffffu];

    s->group = group;
    /* FUN_00562a10 is the destroy half (no realloc).  We then allocate
     * fresh — matches the retail expansion order:
     *   piVar1->group = group;
     *   FUN_00562a10();                  // destroy
     *   piVar1->array    = new HPEN[capacity];
     *   piVar1->count    = 0;
     *   piVar1->capacity = capacity; */
    ar_gdi_slot_destroy(s);
    s->array    = (ar_gdi_handle *)calloc(capacity, sizeof(ar_gdi_handle));
    s->count    = 0;
    s->capacity = capacity;

    /* Pen [0]: (width_a, color_a). */
    if (capacity != 0) {
        ar_gdi_handle h = ar_gdi_create_pen(0, (int32_t)(width_a & 0xffffu), color_a);
        s->array[s->count] = h;
        s->count++;
    }

    /* Middle pens: i=1..capacity-2.  Width steps down by width_step.
     * Colour lerps from color_a to color_mid by i/capacity.
     *
     * Retail uses signed compare on `(int)(capacity - 1)` as the
     * upper bound — with capacity==0 this becomes `1 < -1` and the
     * loop is skipped.  Same with capacity==1 (`1 < 0`) and 2
     * (`1 < 1`).  capacity>=3 enters the loop. */
    for (uint32_t i = 1; (int32_t)i < (int32_t)(capacity - 1u); i++) {
        uint32_t color = ar_color_lerp(color_a, color_mid, (int32_t)i, (int32_t)capacity);
        /* The retail body re-checks count<capacity here (it could
         * exceed if the loop bound were wrong) and silently drops if
         * full.  With our bound the check is always true, but we
         * preserve it for parity. */
        if (s->count < s->capacity) {
            int32_t w = (int32_t)((width_a & 0xffffu) - (width_step & 0xffffu) * i);
            ar_gdi_handle h = ar_gdi_create_pen(0, w, color);
            s->array[s->count] = h;
            s->count++;
        }
    }

    /* Last pen: (width_c, color_c). */
    if (s->count < s->capacity) {
        ar_gdi_handle h = ar_gdi_create_pen(0, (int32_t)(width_c & 0xffffu), color_c);
        s->array[s->count] = h;
        s->count++;
    }
}

/* ─── helper used twice for the sprite slot prologue ────────────── */

static void ar_sprite_slot_register_init(ar_sprite_slot *s,
                                         void *zdd, uint16_t entry_count,
                                         void *settings, uint16_t resource_id,
                                         uint32_t width, uint32_t height,
                                         uint32_t colorkey, uint32_t scale_flag,
                                         uint32_t type, uint16_t group)
{
    ar_sprite_slot_destroy(s);
    s->zdd         = zdd;
    s->entry_count = entry_count;
    s->entries     = (ar_sprite_entry *)calloc(entry_count, sizeof(ar_sprite_entry));
    /* Retail also runs the explicit zero loop for entries[0..count] —
     * calloc gives us that already.  The loop is a memset in
     * disguise. */
    s->settings    = settings;
    s->f_08        = 0;
    s->f_18        = 0;
    s->resource_id = resource_id;
    s->width       = width;
    s->height      = height;
    s->colorkey    = colorkey;
    s->scale_flag  = scale_flag;
    s->type        = type;
    s->group       = group;
    s->f_38        = 0;
}

/* ─── FUN_00563ef0 first half — sound slot field init ─────────── */

void ar_sound_slot_init(ar_sound_slot *s, void *zds, void *settings,
                        uint16_t resource_id, uint16_t count, uint16_t group)
{
    /* Order matches FUN_00563ef0 exactly (group → id → settings → zds
     * → count → state).  FUN_00579a00's 11 inline blocks reorder the
     * writes (zds first), but since all stores are independent the
     * observable end state is identical. */
    s->group       = group;
    s->resource_id = resource_id;
    s->settings    = settings;
    s->zds         = zds;
    s->count       = count;
    s->state       = 0;
    /* `buffer` is NOT touched.  FUN_00563ef0 reads it as the
     * "already loaded?" sentinel before the lazy-load branch; clearing
     * it here would defeat that. */
}

/* ─── FUN_00579a00 — sound boot register batch ───────────────────── */

void ar_register_sounds(void *zds, uint16_t group, void *settings)
{
    /* Table indexed [0..11]; (resource_id, count/kind).  See header
     * docstring for the full list. */
    static const struct { uint16_t id; uint16_t count; } entries[AR_SOUND_SLOT_COUNT] = {
        { 0x50f, 2 },
        { 0x50e, 2 },
        { 0x508, 2 },
        { 0x510, 2 },
        { 0x903, 2 },
        { 0x509, 4 },
        { 0x506, 4 },
        { 0x507, 2 },
        { 0x50c, 4 },   /* retail dispatches this through FUN_00563ef0 */
        { 0x50d, 4 },
        { 0x4d8, 2 },
        { 0x4d9, 2 },
    };
    for (int i = 0; i < AR_SOUND_SLOT_COUNT; i++) {
        ar_sound_slot_init(g_ar_sound_table[i], zds, settings,
                           entries[i].id, entries[i].count, group);
    }
}

/* ─── FUN_00579bd0 — top-level boot register batch ──────────────── */

void ar_register_fonts(void *zdd, uint16_t group, void *settings)
{
    /* sprite[0] — DAT_008a76e8 (font texture sprite id 0x457, 32×32). */
    ar_sprite_slot_register_init(&g_ar_sprite_slots[0],
        zdd, /*entry_count=*/1, settings,
        /*resource_id=*/0x457,
        /*width=*/0x20, /*height=*/0x20,
        /*colorkey=*/0,
        /*scale_flag=*/0,
        /*type=*/2,
        group);

    /* sprite[1] — DAT_008a76ec (font texture sprite id 0x455, 32×48,
     * scale=1).  Retail writes f_08/f_18 before settings on this slot
     * (different ordering vs sprite[0]) — neither order is observable
     * since they're independent stores. */
    ar_sprite_slot_register_init(&g_ar_sprite_slots[1],
        zdd, /*entry_count=*/1, settings,
        /*resource_id=*/0x455,
        /*width=*/0x20, /*height=*/0x30,
        /*colorkey=*/0,
        /*scale_flag=*/1,
        /*type=*/2,
        group);

    /* 8 fonts.  Each block is the FUN_00579ec0(1) + FUN_00579f40 +
     * store-at-array[0] expansion that the retail compiler inlined.
     * We hoist into a tiny helper for symmetry; observable effects
     * match. */
    struct { int idx; int32_t w; int32_t h; uint32_t family; } const fonts[] = {
        { 1,  6, 0xe, 0 },
        { 2,  7, 0x10, 0 },
        { 3,  7, 0x12, 0 },
        { 4,  4, 8,    0 },
        { 5,  7, 0x12, 2 },
        { 6, 10, 0x14, 0 },
        { 7,  5, 10,   0 },
        { 8,  8, 0x10, 0 },
    };
    for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        ar_gdi_slot *s = g_ar_gdi_table[fonts[i].idx];
        s->group = group;
        ar_gdi_slot_reset(s, 1);
        ar_gdi_handle h = ar_make_font(fonts[i].w, fonts[i].h, fonts[i].family);
        s->array[0] = h;
        /* Retail leaves count=0 here too (matches set_font behaviour). */
    }

    /* Extra font into table[9] via FUN_0057a030 — equivalent to a
     * font with width=4 height=8 family=0, but goes through the
     * "all-in-one" variant rather than reset + make + store. */
    ar_gdi_slot_set_font(g_ar_gdi_table[9], group, 4, 8, 0);

    /* Pen + brush into table[14] and table[15]. */
    ar_gdi_slot_set_pen  (g_ar_gdi_table[14], 1, 0xffffff, group, 1);
    ar_gdi_slot_set_brush(g_ar_gdi_table[15],    0xffffff, group, 1);

    /* Four gradient pens at table[12, 10, 11, 13]. */
    ar_gdi_slot_set_pen_gradient(0xc, group, 4, 0x200020, 0x605060, 0xb090bf, 0xf, 4, 1);
    ar_gdi_slot_set_pen_gradient(10,  group, 4, 0x200000, 0x804030, 0xcfa090, 0xf, 4, 1);
    ar_gdi_slot_set_pen_gradient(0xb, group, 4, 0x000030, 0x405080, 0x8090bf, 0xf, 4, 1);
    ar_gdi_slot_set_pen_gradient(0xd, group, 4, 0x300020, 0x804060, 0xb090a0, 0xf, 4, 1);
}
