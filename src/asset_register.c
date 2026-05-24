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
#include "bitmap_session.h"

#include <stdlib.h>
#include <string.h>

/* ─── globals ────────────────────────────────────────────────────── */

ar_sprite_slot  g_ar_sprite_slots[AR_SPRITE_SLOT_COUNT];
ar_sprite_slot *g_ar_sprite_table[AR_SPRITE_SLOT_COUNT];
ar_gdi_slot     g_ar_gdi_slots[AR_GDI_SLOT_COUNT];
ar_gdi_slot    *g_ar_gdi_table[AR_GDI_SLOT_COUNT];
ar_sound_slot   g_ar_sound_slots[AR_SOUND_SLOT_COUNT];
ar_sound_slot  *g_ar_sound_table[AR_SOUND_SLOT_COUNT];

void ar_state_init(void)
{
    memset(g_ar_sprite_slots, 0, sizeof g_ar_sprite_slots);
    memset(g_ar_gdi_slots,    0, sizeof g_ar_gdi_slots);
    memset(g_ar_sound_slots,  0, sizeof g_ar_sound_slots);
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        g_ar_sprite_table[i] = &g_ar_sprite_slots[i];
    }
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

/* ─── FUN_005748c0 — register one sprite slot ───────────────────── */

void ar_sprite_slot_register(ar_sprite_slot *s, void *zdd, void *settings,
                              uint16_t resource_id,
                              uint32_t width, uint32_t height,
                              uint32_t colorkey, uint32_t scale_flag,
                              uint32_t type, uint16_t group)
{
    /* FUN_005748c0 prologue is an inline expansion of
     * ar_sprite_slot_destroy: frees aux_buf, then walks the entries
     * array freeing each entry's `b` pointer, then frees the array.
     * Observable end state matches calling the helper. */
    ar_sprite_slot_destroy(s);

    /* Retail: operator_new(8); entry_count = 1.  Then a zero-loop runs
     * for entry_count iterations writing 0 to entries[i].a and
     * entries[i].b.  calloc(1, 8) handles both at once. */
    s->zdd         = zdd;
    s->entry_count = 1;
    s->entries     = (ar_sprite_entry *)calloc(1, sizeof(ar_sprite_entry));
    s->settings    = settings;
    s->resource_id = resource_id;
    s->width       = width;
    s->height      = height;
    s->colorkey    = colorkey;
    s->f_08        = 0;
    s->f_18        = 0;
    s->scale_flag  = scale_flag;
    s->type        = type;
    s->group       = group;
    s->f_38        = 0;
}

/* ─── FUN_005b5d90 — pack a COLORREF into a PALETTEENTRY ────────── */

void ar_palette_pack_entry(uint8_t *out, uint32_t colorref)
{
    out[0] = (uint8_t)(colorref       & 0xffu);  /* peRed   = COLORREF lo */
    out[1] = (uint8_t)((colorref >> 8) & 0xffu); /* peGreen = COLORREF mid */
    out[2] = (uint8_t)((colorref >>16) & 0xffu); /* peBlue  = COLORREF hi */
    out[3] = 0;                                  /* peFlags = 0 */
}

/* ─── FUN_00491770 — install a 256-entry palette on a sprite slot ── */

void ar_palette_install(ar_sprite_slot *s, const uint8_t palette[1024])
{
    /* Lazy-alloc the 0x400-byte palette buffer on entry[0].b — the
     * destructor frees it iff non-zero, so this is leak-clean.
     * Caller's responsibility (matches retail): entries[] must already
     * be non-NULL with entry_count >= 1, i.e. ar_sprite_slot_register
     * has run for this slot. */
    if (s->entries[0].b == NULL) {
        s->entries[0].b = malloc(1024);
    }
    memcpy(s->entries[0].b, palette, 1024);
}

/* ─── FUN_004178e0 — begin palette session ───────────────────────── */

bool ar_palette_session_begin(ar_sprite_slot *s, uint8_t out_palette[1024])
{
    /* Stack-local bitmap_session — matches retail's `[esp+8]` frame
     * (the 0x444 SUB ESP + 4 SEH guard).  Defensive release first:
     * the stack frame is uninitialised on entry, so the bs_release
     * inside bs_decode_resource needs to see pixels==NULL to skip the
     * free of garbage. */
    bitmap_session session;
    bs_release_no_free(&session);

    int ok = bs_decode_resource(&session,
                                s->settings,      /* HMODULE — see header */
                                s->resource_id,
                                "DATA", 1);
    if (!ok) {
        bs_release(&session);                     /* idempotent on NULL */
        return false;
    }

    bool emitted = (bs_get_bit_count(&session) == 8);
    if (emitted) {
        bs_emit_palette_bgra(&session, out_palette);
    }
    bs_release(&session);
    return emitted;
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
    static const struct { uint16_t id; uint16_t count; } entries[AR_SOUND_MAIN_COUNT] = {
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
    for (int i = 0; i < AR_SOUND_MAIN_COUNT; i++) {
        ar_sound_slot_init(g_ar_sound_table[i], zds, settings,
                           entries[i].id, entries[i].count, group);
    }
}

/* ─── FUN_0057b280 — game-sound register batch (group 3) ────────── */

/* One row per retail register call.  Issue order preserved so any
 * future call-trace test matches without renormalisation — the 122
 * inline blocks come first in source order, then the 52 thiscall
 * dispatches.  All 174 entries share `ar_sound_slot_init` semantics
 * (load_flag=0, buffer untouched).  See header docstring. */
struct ar_game_sound_entry {
    uint16_t idx;
    uint16_t id;
    uint16_t count;
};

static const struct ar_game_sound_entry game_sounds[] = {
    /* idx, id,    count */
    /* ── 122 inline blocks ── */
    {  14, 0x513,  8 }, {  15, 0x4c2,  8 }, {  18, 0x4c5,  8 },
    {  17, 0x4c4,  4 }, {  16, 0x4c1,  8 }, {  19, 0x4c7,  4 },
    {  20, 0x4c3,  4 }, {  21, 0x4c6,  8 }, {  35, 0x902,  2 },
    {  36, 0x907,  2 }, {  26, 0x4bf,  8 }, {  27, 0x4c0,  4 },
    {  28, 0x4ba,  8 }, {  29, 0x66e,  4 }, {  30, 0x6a0,  2 },
    {  31, 0x783,  2 }, {  32, 0x6b4,  2 }, {  33, 0x770,  2 },
    {  34, 0x7a6,  2 }, { 127, 0x8c5,  4 }, {  37, 0x4b9,  4 },
    {  38, 0x4bb,  4 }, {  41, 0x904,  8 }, {  39, 0x4be,  2 },
    {  40, 0x4bd,  2 }, {  42, 0x66f,  2 }, {  43, 0x670,  2 },
    {  44, 0x671,  2 }, {  45, 0x672,  2 }, {  46, 0x673,  2 },
    {  47, 0x674,  2 }, {  48, 0x675,  2 }, {  49, 0x676,  2 },
    {  50, 0x771,  4 }, {  51, 0x772,  4 }, {  52, 0x773,  4 },
    {  53, 0x774,  4 }, {  54, 0x677,  2 }, {  55, 0x678,  2 },
    {  56, 0x67c,  2 }, {  57, 0x67a,  2 }, {  58, 0x67b,  2 },
    {  59, 0x6cd,  2 }, {  60, 0x6ce,  2 }, {  61, 0x6cf,  2 },
    {  62, 0x6d0,  2 }, {  63, 0x6d1,  2 }, {  64, 0x79b,  4 },
    {  65, 0x79c,  4 }, {  66, 0x505,  2 }, {  67, 0x501,  2 },
    {  68, 0x4ff,  2 }, {  69, 0x500,  2 }, {  70, 0x52f,  2 },
    {  71, 0x502,  2 }, {  72, 0x4a5,  2 }, {  73, 0x4a3,  2 },
    {  74, 0x4a4,  2 }, {  75, 0x503,  4 }, {  76, 0x504,  4 },
    {  77, 0x4cd,  4 }, {  78, 0x4cc,  4 }, {  79, 0x805,  4 },
    { 131, 0x4a6,  2 }, { 132, 0x4a8,  2 }, { 133, 0x4aa,  2 },
    { 134, 0x4a7,  4 }, { 135, 0x4a9,  2 }, { 136, 0x8b6,  4 },
    {  80, 0x4dc,  4 }, {  81, 0x4dd,  4 }, { 110, 0x4bc,  4 },
    {  82, 0x4de,  4 }, {  83, 0x4df,  4 }, {  87, 0x4eb,  4 },
    {  84, 0x4e3,  4 }, {  85, 0x4e1,  2 }, {  86, 0x52c,  2 },
    { 102, 0x4ec,  2 }, { 103, 0x4ed,  4 }, { 104, 0x4ee,  4 },
    { 105, 0x4e0,  2 }, { 106, 0x4e8,  4 }, { 107, 0x876,  4 },
    { 108, 0x87d,  4 }, { 109, 0x87e,  4 }, { 111, 0x529,  2 },
    { 112, 0x52a,  4 }, {  88, 0x79a,  8 }, { 113, 0x7c1,  8 },
    {  89, 0x4db,  4 }, {  90, 0x4da,  4 }, {  96, 0x4e6,  2 },
    {  95, 0x4e7,  2 }, { 100, 0x52b, 16 }, { 101, 0x6f1,  8 },
    {  97, 0x872,  4 }, {  98, 0x873,  4 }, {  99, 0x6f0,  4 },
    {  91, 0x4e4,  4 }, {  92, 0x4e5,  4 }, {  93, 0x4e9,  1 },
    {  94, 0x4ea,  1 }, { 115, 0x6f5,  4 }, { 114, 0x6f6,  4 },
    { 116, 0x7be,  4 }, { 117, 0x4d6,  4 }, { 118, 0x4d7,  4 },
    { 119, 0x4d0,  8 }, { 120, 0x4d3,  4 }, { 121, 0x7cf,  4 },
    { 122, 0x4d1,  4 }, { 123, 0x4d2,  4 }, { 124, 0x4d5,  4 },
    { 125, 0x4ce,  4 }, { 126, 0x4cf,  4 }, { 130, 0x53a,  4 },
    { 128, 0x4d4,  4 }, { 129, 0x782,  4 }, { 137, 0x4f8,  4 },
    { 139, 0x4fb,  4 }, { 140, 0x53c,  4 },
    /* ── 52 thiscall block ── */
    { 143, 0x4f0,  8 }, { 141, 0x4f1,  4 }, { 145, 0x7c8,  4 },
    { 146, 0x7c9,  4 }, { 142, 0x4ef,  4 }, { 138, 0x4fa,  4 },
    { 144, 0x4f9,  4 }, { 147, 0x4f7,  4 }, { 148, 0x71c,  4 },
    { 149, 0x4fc,  4 }, { 150, 0x4fd,  4 }, { 151, 0x4fe,  4 },
    { 157, 0x668,  4 }, { 158, 0x796,  4 }, { 159, 0x66a,  4 },
    { 152, 0x6b0,  4 }, { 153, 0x7cc,  4 }, { 154, 0x7cd,  4 },
    { 155, 0x7ce,  4 }, { 207, 0x8bf,  4 }, { 208, 0x8c1,  4 },
    { 216, 0x511,  8 }, { 217, 0x512,  8 }, { 218, 0x52d,  2 },
    { 219, 0x52e,  2 }, {  12, 0x50a,  8 }, {  13, 0x50b,  2 },
    { 220, 0x4b0,  8 }, { 221, 0x4b1,  8 }, { 222, 0x6e6,  8 },
    { 223, 0x6e7,  8 }, { 224, 0x4b2,  2 }, { 225, 0x4b3,  2 },
    { 226, 0x4b7,  2 }, { 227, 0x4b4,  6 }, { 240, 0x4b8,  2 },
    { 243, 0x6fb,  4 }, { 229, 0x76e,  2 }, { 228, 0x76f,  4 },
    { 230, 0x77e, 16 }, { 244, 0x78a,  4 }, { 231, 0x7d1,  4 },
    { 232, 0x8cc,  2 }, { 233, 0x8cd,  8 }, { 241, 0x4ab,  4 },
    { 242, 0x4ac,  4 }, { 234, 0x4b5,  4 }, { 235, 0x4b6,  2 },
    { 236, 0x4ad,  8 }, { 237, 0x4ae,  2 }, { 239, 0x4af,  4 },
    { 238, 0x7bf,  8 },
};

#define AR_GAME_SOUND_COUNT (sizeof(game_sounds) / sizeof(game_sounds[0]))

void ar_register_game_sounds(void *zds, uint16_t group, void *settings)
{
    for (size_t i = 0; i < AR_GAME_SOUND_COUNT; i++) {
        const struct ar_game_sound_entry *e = &game_sounds[i];
        ar_sound_slot_init(g_ar_sound_table[e->idx], zds, settings,
                           e->id, e->count, group);
    }
}

/* ─── 4 inline `FUN_00563ef0` calls at FUN_00562ea0:617-620 ──────── */

void ar_register_aux_sounds(void *zds, uint16_t group, void *settings)
{
    /* Issue order from FUN_00562ea0:617-620 (idx, resource_id).  Count
     * is 2 for every entry.  The retail caller hardcodes group=2; we
     * accept it as a parameter for API symmetry. */
    static const struct { uint16_t idx; uint16_t id; } entries[4] = {
        { 22, 0x4cb },
        { 23, 0x4ca },
        { 24, 0x4c8 },
        { 25, 0x4c9 },
    };
    for (size_t i = 0; i < 4; i++) {
        ar_sound_slot_init(g_ar_sound_table[entries[i].idx], zds, settings,
                           entries[i].id, /*count=*/2, group);
    }
}

/* ─── FUN_0057b280 tail — locale-table loop ─────────────────────── */

/* Rdata table at retail address 0x00691018 (file offset 0x00691018,
 * via `r2 px @ 0x691018`).  Stride 0x24; terminator is the first
 * entry whose +0x00 magic is zero.  283 live entries extracted.
 *
 * Only the five fields the loop actually reads are kept here — the
 * loop ignores +0x00 (magic, used only as a non-zero "live" marker),
 * +0x04 (sequence number), +0x0c / +0x10 (scalar metadata, varies),
 * +0x16 (pad, always 0), +0x1e..+0x23 (mostly zero, never read).
 * 15 entries have primary_id == 0 (skipped); 15 have override == 0x7fff
 * (skip-when-override-active); 29 have flag == -1.  Touched indices
 * span 160..464 (267 distinct). */
struct ar_locale_sound_entry {
    uint16_t idx;          /* +0x08 — slot index into g_ar_sound_table */
    uint16_t primary_id;   /* +0x0a — fallback-path resource ID; 0 = skip entry */
    int8_t   count_add;    /* +0x14 (i16 in retail; observed 0 or 2) */
    int8_t   flag;         /* +0x18 (i32 in retail; observed 0 or -1) */
    uint16_t override;     /* +0x1c — override-path resource ID; 0x7fff = skip-when-override-active */
};

static const struct ar_locale_sound_entry locale_sounds[] = {
    /* idx, primary_id, count_add, flag, override */
    { 245, 0x53d,  0,  0, 0x08bb },
    { 246, 0x53e,  0,  0, 0x08bc },
    { 247, 0x60f,  0,  0, 0x08bd },
    { 248, 0x610,  0,  0, 0x08be },
    { 249, 0x53f,  0,  0, 0x08bf },
    { 250, 0x611,  0,  0, 0x08c0 },
    { 251, 0x612,  0,  0, 0x08c1 },
    { 254, 0x61f,  0,  0, 0x08c4 },
    { 255, 0x620,  0,  0, 0x08c5 },
    { 252, 0x540,  0,  0, 0x08c2 },
    { 253, 0x541,  0,  0, 0x08c3 },
    { 263, 0x518,  0,  0, 0x08cf },
    { 264, 0x519,  0,  0, 0x08d0 },
    { 265, 0x542,  0,  0, 0x08d1 },
    { 266, 0x618,  0,  0, 0x08d2 },
    { 267, 0x619,  0,  0, 0x08d3 },
    { 281, 0x627,  0,  0, 0x08d8 },
    { 282, 0x630,  0,  0, 0x08d7 },
    { 283, 0x628,  0,  0, 0x08d9 },
    { 284, 0x629,  0,  0, 0x08da },
    { 285, 0x62a,  0,  0, 0x08dc },
    { 268, 0x543,  0,  0, 0x08e1 },
    { 269, 0x544,  0,  0, 0x08e2 },
    { 270, 0x61c,  0,  0, 0x08e3 },
    { 271, 0x61a,  0,  0, 0x08d4 },
    { 272, 0x61b,  0,  0, 0x08d5 },
    { 286, 0x62b,  0,  0, 0x08dd },
    { 287, 0x62c,  0,  0, 0x08de },
    { 288, 0x62d,  0,  0, 0x08df },
    { 268, 0x000,  0,  0, 0x7fff },
    { 269, 0x000,  0,  0, 0x7fff },
    { 270, 0x000,  0,  0, 0x7fff },
    { 297, 0x61d,  0,  0, 0x08e6 },
    { 298, 0x61e,  0,  0, 0x0996 },
    { 295, 0x51b,  0,  0, 0x08e8 },
    { 296, 0x51c,  0,  0, 0x0998 },
    { 256, 0x613,  0,  0, 0x08b4 },
    { 257, 0x614,  0,  0, 0x08c1 },
    { 257, 0x000,  0,  0, 0x7fff },
    { 260, 0x3f0,  0, -1, 0x08b9 },
    { 249, 0x000,  0,  0, 0x7fff },
    { 250, 0x000,  0,  0, 0x7fff },
    { 261, 0x3ef,  0, -1, 0x08b7 },
    { 250, 0x000,  0,  0, 0x7fff },
    { 251, 0x000,  0,  0, 0x7fff },
    { 262, 0x617,  0,  0, 0x08ba },
    { 256, 0x000,  0,  0, 0x7fff },
    { 275, 0x621,  0,  0, 0x08c9 },
    { 276, 0x622,  0,  0, 0x08ca },
    { 277, 0x623,  0,  0, 0x08cb },
    { 278, 0x624,  0,  0, 0x08cc },
    { 279, 0x625,  0,  0, 0x08cd },
    { 273, 0x545,  0,  0, 0x08e4 },
    { 274, 0x62f,  0,  0, 0x08e5 },
    { 280, 0x626,  0,  0, 0x08ce },
    { 289, 0x62e,  0,  0, 0x08e0 },
    { 290, 0x65a,  0,  0, 0x08c6 },
    { 291, 0x65b,  0,  0, 0x08c7 },
    { 292, 0x65c,  0,  0, 0x08c8 },
    { 293, 0x65d,  0,  0, 0x08e7 },
    { 294, 0x65e,  0,  0, 0x0997 },
    { 299, 0x56a,  0,  0, 0x08e9 },
    { 300, 0x56b,  0,  0, 0x08ea },
    { 301, 0x56c,  0,  0, 0x08eb },
    { 302, 0x56d,  0,  0, 0x08ec },
    { 303, 0x56e,  0,  0, 0x08ed },
    { 304, 0x56f,  0,  0, 0x08ee },
    { 305, 0x570,  0,  0, 0x08ef },
    { 306, 0x571,  0,  0, 0x08f0 },
    { 307, 0x572,  0,  0, 0x08f1 },
    { 308, 0x573,  0,  0, 0x08f2 },
    { 309, 0x574,  0,  0, 0x08f3 },
    { 310, 0x575,  0,  0, 0x08f4 },
    { 361, 0x3f1,  0, -1, 0x08f5 },
    { 362, 0x7db,  0,  0, 0x08f6 },
    { 363, 0x3f2,  0, -1, 0x08f7 },
    { 364, 0x7dd,  0,  0, 0x08f8 },
    { 311, 0x51f,  0,  0, 0x08ff },
    { 312, 0x520,  0,  0, 0x0900 },
    { 313, 0x530,  0,  0, 0x0901 },
    { 314, 0x531,  0,  0, 0x0902 },
    { 315, 0x532,  0,  0, 0x0903 },
    { 317, 0x533,  0,  0, 0x091c },
    { 318, 0x585,  0,  0, 0x091d },
    { 316, 0x535,  0,  0, 0x0918 },
    { 319, 0x536,  0,  0, 0x0916 },
    { 321, 0x577,  0,  0, 0x0917 },
    { 320, 0x576,  0,  0, 0x0915 },
    { 322, 0x578,  0,  0, 0x091e },
    { 323, 0x579,  0,  0, 0x091f },
    { 324, 0x57a,  0,  0, 0x0920 },
    { 325, 0x57b,  0,  0, 0x0922 },
    { 326, 0x57c,  0,  0, 0x0923 },
    { 327, 0x57d,  0,  0, 0x0924 },
    { 328, 0x57e,  0,  0, 0x0921 },
    { 329, 0x661,  0,  0, 0x0927 },
    { 330, 0x6f2,  0,  0, 0x0919 },
    { 331, 0x6f3,  0,  0, 0x0926 },
    { 332, 0x6fc,  0,  0, 0x0925 },
    { 333, 0x799,  0,  0, 0x0928 },
    { 334, 0x8c2,  0,  0, 0x091a },
    { 335, 0x8c3,  0,  0, 0x091b },
    { 338, 0x549,  0,  0, 0x0906 },
    { 339, 0x54a,  0,  0, 0x0907 },
    { 340, 0x54b,  0,  0, 0x0908 },
    { 341, 0x54c,  0,  0, 0x0909 },
    { 342, 0x54d,  0,  0, 0x090a },
    { 343, 0x54e,  0,  0, 0x090b },
    { 344, 0x55d,  0,  0, 0x090c },
    { 345, 0x54f,  0,  0, 0x0911 },
    { 346, 0x550,  0,  0, 0x0912 },
    { 347, 0x551,  0,  0, 0x0913 },
    { 336, 0x547,  0,  0, 0x0904 },
    { 337, 0x548,  0,  0, 0x0905 },
    { 348, 0x552,  0,  0, 0x0914 },
    { 345, 0x000,  0,  0, 0x7fff },
    { 346, 0x000,  0,  0, 0x7fff },
    { 347, 0x000,  0,  0, 0x7fff },
    { 349, 0x553,  0,  0, 0x08f9 },
    { 350, 0x554,  0,  0, 0x08fa },
    { 351, 0x3f3,  0, -1, 0x08fb },
    { 352, 0x556,  0,  0, 0x08fc },
    { 358, 0x55c,  0,  0, 0x0910 },
    { 355, 0x559,  0,  0, 0x090d },
    { 356, 0x55a,  0,  0, 0x090e },
    { 357, 0x3f4,  0, -1, 0x090f },
    { 360, 0x587,  0,  0, 0x092d },
    { 359, 0x586,  0,  0, 0x092c },
    { 365, 0x522,  0,  0, 0x0959 },
    { 366, 0x523,  0,  0, 0x095a },
    { 367, 0x5b2,  0,  0, 0x095b },
    { 368, 0x5b3,  0,  0, 0x095c },
    { 369, 0x5b4,  0,  0, 0x095d },
    { 370, 0x5b5,  0,  0, 0x095e },
    { 371, 0x5b6,  0,  0, 0x095f },
    { 372, 0x5b7,  0,  0, 0x0960 },
    { 373, 0x5b8,  0,  0, 0x0961 },
    { 399, 0x5cc,  0,  0, 0x0962 },
    { 400, 0x5cd,  0,  0, 0x0963 },
    { 401, 0x5ce,  0,  0, 0x0964 },
    { 374, 0x525,  0,  0, 0x096b },
    { 375, 0x526,  0,  0, 0x096c },
    { 376, 0x5ba,  0,  0, 0x096d },
    { 377, 0x5bb,  0,  0, 0x096e },
    { 378, 0x5bc,  0,  0, 0x096f },
    { 379, 0x5c0,  0,  0, 0x0973 },
    { 380, 0x5c1,  0,  0, 0x0974 },
    { 381, 0x5c2,  0,  0, 0x0975 },
    { 382, 0x5c3,  0,  0, 0x0976 },
    { 383, 0x5c4,  0,  0, 0x0977 },
    { 383, 0x5c5,  0,  0, 0x0978 },
    { 385, 0x5bd,  0,  0, 0x097d },
    { 386, 0x5be,  0,  0, 0x097e },
    { 387, 0x5bf,  0,  0, 0x097f },
    { 405, 0x5d1,  0,  0, 0x0970 },
    { 406, 0x5d2,  0,  0, 0x0971 },
    { 407, 0x5d3,  0,  0, 0x0972 },
    { 396, 0x5c9,  0,  0, 0x0979 },
    { 397, 0x5ca,  0,  0, 0x097a },
    { 398, 0x5cb,  0,  0, 0x097b },
    { 385, 0x000,  0,  0, 0x7fff },
    { 386, 0x000,  0,  0, 0x7fff },
    { 387, 0x000,  0,  0, 0x7fff },
    { 408, 0x5d4,  0,  0, 0x0981 },
    { 409, 0x5d5,  0,  0, 0x0984 },
    { 410, 0x5d6,  0,  0, 0x0983 },
    { 411, 0x5d7,  0,  0, 0x0985 },
    { 412, 0x5d8,  0,  0, 0x0987 },
    { 413, 0x5d9,  0,  0, 0x0986 },
    { 414, 0x5da,  0,  0, 0x0989 },
    { 415, 0x5db,  0,  0, 0x0988 },
    { 416, 0x5dc,  0,  0, 0x098b },
    { 417, 0x6d6,  0,  0, 0x098e },
    { 418, 0x6f4,  0,  0, 0x098d },
    { 419, 0x6f7,  0,  0, 0x098a },
    { 420, 0x7c0,  0,  0, 0x098c },
    { 421, 0x901,  0,  0, 0x0982 },
    { 388, 0x5c6,  0,  0, 0x0965 },
    { 389, 0x5c7,  0,  0, 0x0966 },
    { 390, 0x5c8,  0,  0, 0x0967 },
    { 391, 0x631,  0,  0, 0x0968 },
    { 392, 0x632,  0,  0, 0x0969 },
    { 402, 0x5cf,  0,  0, 0x096a },
    { 403, 0x633,  0,  0, 0x097c },
    { 404, 0x5d0,  0,  0, 0x0980 },
    { 393, 0x634,  0,  0, 0x0992 },
    { 394, 0x635,  0,  0, 0x0993 },
    { 395, 0x636,  0,  0, 0x0994 },
    { 422, 0x6c1,  0,  0, 0x0932 },
    { 423, 0x6c2,  0,  0, 0x0933 },
    { 424, 0x6c3,  0,  0, 0x0934 },
    { 425, 0x6c4,  0,  0, 0x0935 },
    { 426, 0x6c5,  0,  0, 0x0936 },
    { 427, 0x6c6,  0,  0, 0x0937 },
    { 428, 0x6c7,  0,  0, 0x0939 },
    { 429, 0x6fd,  0,  0, 0x092e },
    { 430, 0x6fe,  0,  0, 0x092f },
    { 431, 0x700,  0,  0, 0x0930 },
    { 432, 0x701,  0,  0, 0x0931 },
    { 433, 0x702,  0,  0, 0x0938 },
    { 434, 0x6ff,  0,  0, 0x093a },
    { 435, 0x3f5,  0, -1, 0x093b },
    { 440, 0x3f6,  0, -1, 0x093c },
    { 441, 0x3f7,  0, -1, 0x093d },
    { 442, 0x3f8,  0, -1, 0x093e },
    { 444, 0x3fa,  0, -1, 0x0940 },
    { 445, 0x3fb,  0, -1, 0x0941 },
    { 436, 0x405,  0, -1, 0x0952 },
    { 443, 0x3f9,  0, -1, 0x093f },
    { 437, 0x8c7,  0,  0, 0x0949 },
    { 453, 0x8f3,  0,  0, 0x094a },
    { 438, 0x8c8,  0,  0, 0x094c },
    { 455, 0x401,  0, -1, 0x094d },
    { 446, 0x8ec,  0,  0, 0x0942 },
    { 447, 0x8ed,  0,  0, 0x0943 },
    { 462, 0x8fc,  0,  0, 0x0956 },
    { 463, 0x8fd,  0,  0, 0x0957 },
    { 460, 0x8fa,  0,  0, 0x0954 },
    { 461, 0x8fb,  0,  0, 0x0955 },
    { 464, 0x8fe,  0,  0, 0x0958 },
    { 454, 0x8f4,  0,  0, 0x094b },
    { 439, 0x402,  0, -1, 0x094f },
    { 457, 0x403,  0, -1, 0x0950 },
    { 458, 0x404,  0, -1, 0x0951 },
    { 448, 0x3fc,  0, -1, 0x0944 },
    { 449, 0x3fd,  0, -1, 0x0945 },
    { 450, 0x3fe,  0, -1, 0x0946 },
    { 452, 0x400,  0, -1, 0x0948 },
    { 456, 0x8f6,  0,  0, 0x094e },
    { 160, 0x55e,  2,  0, 0x0000 },
    { 161, 0x55f,  2,  0, 0x0000 },
    { 162, 0x560,  2,  0, 0x0000 },
    { 163, 0x561,  2,  0, 0x0000 },
    { 164, 0x562,  2,  0, 0x0000 },
    { 165, 0x563,  2,  0, 0x0000 },
    { 166, 0x564,  2,  0, 0x0000 },
    { 167, 0x565,  2,  0, 0x0000 },
    { 168, 0x6dc,  2,  0, 0x0000 },
    { 169, 0x66b,  2,  0, 0x0000 },
    { 170, 0x66c,  2,  0, 0x0000 },
    { 171, 0x66d,  2,  0, 0x0000 },
    { 185, 0x7a0,  0,  0, 0x0000 },
    { 186, 0x7a1,  0,  0, 0x0000 },
    { 187, 0x7a2,  0,  0, 0x0000 },
    { 172, 0x566,  2,  0, 0x0000 },
    { 173, 0x568,  2,  0, 0x0000 },
    { 174, 0x567,  2,  0, 0x0000 },
    { 175, 0x569,  2,  0, 0x0000 },
    { 176, 0x58a,  2,  0, 0x0000 },
    { 177, 0x58b,  2,  0, 0x0000 },
    { 178, 0x6c8,  2,  0, 0x0000 },
    { 179, 0x6c9,  2,  0, 0x0000 },
    { 180, 0x6ca,  2,  0, 0x0000 },
    { 181, 0x6cb,  2,  0, 0x0000 },
    { 182, 0x6cc,  2,  0, 0x0000 },
    { 183, 0x6d2,  2,  0, 0x0000 },
    { 184, 0x6d3,  2,  0, 0x0000 },
    { 188, 0x6aa,  2,  0, 0x0000 },
    { 189, 0x6ab,  2,  0, 0x0000 },
    { 190, 0x6ae,  2,  0, 0x0000 },
    { 191, 0x6af,  2,  0, 0x0000 },
    { 192, 0x71a,  2,  0, 0x0000 },
    { 193, 0x71b,  2,  0, 0x0000 },
    { 194, 0x790,  2,  0, 0x0000 },
    { 195, 0x792,  2,  0, 0x0000 },
    { 196, 0x791,  2,  0, 0x0000 },
    { 197, 0x793,  2,  0, 0x0000 },
    { 198, 0x795,  2,  0, 0x0000 },
    { 199, 0x7ad,  2,  0, 0x0000 },
    { 200, 0x7ae,  2,  0, 0x0000 },
    { 201, 0x7af,  2,  0, 0x0000 },
    { 202, 0x7bb,  2,  0, 0x0000 },
    { 203, 0x7bc,  2,  0, 0x0000 },
    { 204, 0x8dd,  2,  0, 0x0000 },
    { 205, 0x7ca,  2,  0, 0x0000 },
    { 209, 0x3ed,  2, -1, 0x0000 },
    { 210, 0x3ee,  2, -1, 0x0000 },
    { 214, 0x3eb,  2, -1, 0x0000 },
    { 211, 0x3e8,  2, -1, 0x0000 },
    { 212, 0x3e9,  2, -1, 0x0000 },
    { 215, 0x3ec,  2, -1, 0x0000 },
    { 213, 0x3ea,  2, -1, 0x0000 },
    { 206, 0x8c0,  2,  0, 0x0000 },
};

#define AR_LOCALE_SOUND_COUNT (sizeof(locale_sounds) / sizeof(locale_sounds[0]))

void ar_register_locale_sounds(void *zds, uint16_t group, void *settings,
                                const ar_locale_state *locale)
{
    for (size_t i = 0; i < AR_LOCALE_SOUND_COUNT; i++) {
        const struct ar_locale_sound_entry *e = &locale_sounds[i];
        if (e->primary_id == 0) continue;

        uint16_t res_id;
        void    *use_settings;

        /* PATH A (fallback) iff override == 0, or no current locale, or
         * the launcher flag forces fallback. */
        if (e->override == 0 ||
            locale->current_settings == NULL ||
            locale->launcher_flag != 0) {
            res_id = e->primary_id;
            /* flag == -1 → use the fallback-locale settings instead of
             * the caller's group-passthrough settings.  The retail decomp
             * achieves this via the `if (flag == -1) goto LAB_0057c9db`
             * shortcut that skips the `pvVar4 = param_3` assignment. */
            use_settings = (e->flag == -1) ? locale->fallback_settings
                                           : settings;
        } else if (e->override == 0x7fff) {
            /* PATH B sentinel: skip the entry entirely.  Matches the
             * retail `goto LAB_0057ca18` (loop-tail jump). */
            continue;
        } else {
            /* PATH B (locale override). */
            res_id       = e->override;
            use_settings = locale->current_settings;
        }

        ar_sound_slot_init(g_ar_sound_table[e->idx], zds, use_settings,
                           res_id, (uint16_t)(e->count_add + 2), group);
    }
}

/* ─── FUN_00579bd0 — top-level boot register batch ──────────────── */

void ar_register_fonts(void *zdd, uint16_t group, void *settings)
{
    /* sprite[42] — DAT_008a76e8 (font texture sprite id 0x457, 32×32). */
    ar_sprite_slot_register(&g_ar_sprite_slots[AR_SPR_FONT_TEX_457],
        zdd, settings, /*resource_id=*/0x457,
        /*width=*/0x20, /*height=*/0x20,
        /*colorkey=*/0,
        /*scale_flag=*/0,
        /*type=*/2,
        group);

    /* sprite[43] — DAT_008a76ec (font texture sprite id 0x455, 32×48,
     * scale=1).  Retail writes f_08/f_18 before settings on this slot
     * (different ordering vs sprite[42]) — neither order is observable
     * since they're independent stores. */
    ar_sprite_slot_register(&g_ar_sprite_slots[AR_SPR_FONT_TEX_455],
        zdd, settings, /*resource_id=*/0x455,
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

/* ─── FUN_005749b0 — UI/menu sprite register batch ──────────────── */

/* One entry of the sprite-register table.  All fields below carry
 * straight through to ar_sprite_slot_register; idx is the pool index
 * (= (retail_BSS_addr - 0x008a7640) / 4). */
struct ar_main_sprite_entry {
    uint8_t   idx;
    uint16_t  id;
    uint32_t  width;
    uint32_t  height;
    uint32_t  colorkey;
    uint8_t   scale_flag;
    uint8_t   type;
};

void ar_register_main_sprites(void *zdd, uint16_t group, void *settings,
                              void *sotesp_module)
{
    /* The 9 inline sprite-slot registers from retail.  Retail writes
     * them in a shuffled order (idx 5 lands after idx 8 in source —
     * see FUN_005749b0 0x574d78), but each register is independent so
     * the iteration order has no observable effect.  We list by
     * ascending index. */
    static const struct ar_main_sprite_entry inline_slots[] = {
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
    for (size_t i = 0; i < sizeof(inline_slots) / sizeof(inline_slots[0]); i++) {
        const struct ar_main_sprite_entry *e = &inline_slots[i];
        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],
            zdd, settings, e->id,
            e->width, e->height, e->colorkey,
            e->scale_flag, e->type, group);
    }

    /* Transient slot at idx 0 (DAT_008a7640): id=0x90b loaded from
     * sotesp.dll instead of the launcher settings record.  Note
     * `sotesp_module` takes the `settings` parameter slot — the same
     * +0x3c field — which the resource decoder dereferences as the
     * source HMODULE.  Retail's `FUN_005748c0(zdd, DAT_008a6e74,
     * 0x90b, 0x20, 0x20, 0, 0, 2, group)` at 0x574e1b — the implicit
     * ECX is `DAT_008a7640` (the slot pointer for idx 0). */
    ar_sprite_slot_register(&g_ar_sprite_slots[0],
        zdd, sotesp_module, /*resource_id=*/0x90b,
        /*width=*/0x20, /*height=*/0x20,
        /*colorkey=*/0,
        /*scale_flag=*/0,
        /*type=*/2,
        group);

    /* Palette ramp section (retail FUN_005749b0:0x574e1b..0x574eb0):
     *
     *   1. Allocate a 1024-byte palette on stack (256 RGBQUADs).
     *   2. Seed it from sotesp.dll resource 0x90b via the bitmap
     *      decoder (ar_palette_session_begin) — populates all 256
     *      entries with the resource's palette.
     *   3. Override palette[1] with 0 (one packed-COLORREF write).
     *   4. Override palette[41..50] (10 entries) with 0x383838 (dark gray).
     *   5. Override palette[51..70] (20 entries) with linear lerp from
     *      0x383838 to 0xffffff using ar_color_lerp(i, 20) for i=1..20.
     *   6. Install the resulting palette onto sprite slot at idx 0.
     *
     * Index-distribution provenance: retail allocates the 1024-byte
     * buffer as 4 stack vars (local_400[4], local_3fc[160], local_35c[40],
     * local_334[820]) — Ghidra's reverse-order naming means the buffer
     * starts at local_400.  Palette indices: local_3fc = palette[1],
     * local_35c = palette[41], local_334 = palette[51].
     *
     * No-op when the decoder fails to emit a palette (e.g. 24bpp
     * source — won't actually happen for idx 0's 0x90b resource, but
     * keeps the wiring leak-clean if the resource ever changes). */
    {
        uint8_t palette[1024];
        if (ar_palette_session_begin(&g_ar_sprite_slots[0], palette)) {
            ar_palette_pack_entry(&palette[1 * 4], 0);
            for (int i = 0; i < 10; i++) {
                ar_palette_pack_entry(&palette[(41 + i) * 4], 0x383838);
            }
            for (int i = 1; i <= 20; i++) {
                uint32_t c = ar_color_lerp(0x383838, 0xffffff, i, 20);
                ar_palette_pack_entry(&palette[(50 + i) * 4], c);
            }
            ar_palette_install(&g_ar_sprite_slots[0], palette);
        }
    }

    /* 24 trailing FUN_005748c0 calls.  The first 20 hit indices 10..29
     * (contiguous in the retail BSS pointer table); the last 4 are
     * stragglers at non-contiguous indices.  All use `settings` from
     * the caller (not sotesp_module). */
    static const struct ar_main_sprite_entry trailing_calls[] = {
        /* idx 10..29: panels + full-screen backgrounds */
        { 10, 0x8e0, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 11, 0x8e1, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 12, 0x8e2, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 13, 0x8e3, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 14, 0x8e4, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 15, 0x8e5, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 16, 0x90e, 0x170, 0x114, 0x1ffffff, 1, 0 },
        { 17, 0x8de, 0x190, 0xa0,  0x1ffffff, 1, 0 },
        { 18, 0x919, 0x150, 0x50,  0x1ffffff, 1, 0 },
        { 19, 0x712, 0x160, 0xb0,  0x1ffffff, 1, 0 },
        { 20, 0x6f8, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 21, 0x6f9, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 22, 0x715, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 23, 0x77f, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 24, 0x714, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 25, 0x7d9, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 26, 0x7a4, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 27, 0x7d8, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 28, 0x90c, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        { 29, 0x90d, 0x280, 0x1e0, 0x1ffffff, 0, 0 },
        /* stragglers: small icon registers */
        { 50, 0x456, 0x20,  0x20,  0,         0, 2 },  /* DAT_008a7708 */
        { 46, 0x453, 0x40,  0x40,  0,         0, 2 },  /* DAT_008a76f8 */
        { 47, 0x660, 0xa0,  0x20,  0,         0, 2 },  /* DAT_008a76fc */
        { 55, 0x6fa, 0x20,  0x20,  0,         0, 2 },  /* DAT_008a771c */
    };
    for (size_t i = 0; i < sizeof(trailing_calls) / sizeof(trailing_calls[0]); i++) {
        const struct ar_main_sprite_entry *e = &trailing_calls[i];
        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],
            zdd, settings, e->id,
            e->width, e->height, e->colorkey,
            e->scale_flag, e->type, group);
    }
}

/* ─── FUN_0056e190 — "hundreds of sprites" register batch ────────── */

/* Same layout as ar_main_sprite_entry but with a u16 idx (this batch
 * reaches idx 863).  Kept local since no other batch needs the wider
 * field. */
struct ar_game_sprite_entry {
    uint16_t  idx;
    uint16_t  id;
    uint32_t  width;
    uint32_t  height;
    uint32_t  colorkey;
    uint8_t   scale_flag;
    uint8_t   type;
};

/* The retail register order is: 93 inline blocks at indices 425..517
 * (resource IDs 0x592..0x5fb sequential) followed by 349 thiscall
 * FUN_005748c0 entries grouped by sprite shape.  All entries are
 * independent — iteration order has no observable effect — but we
 * preserve the retail issue order so any future call-trace test
 * matches without extra normalisation. */
static const struct ar_game_sprite_entry game_sprites[] = {
    /* idx, id, w, h, ck, scale, type */
    {425, 0x592,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {426, 0x593,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {427, 0x594,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {428, 0x595,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {429, 0x596,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {430, 0x597,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {431, 0x598,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {432, 0x599,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {433, 0x59a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {434, 0x59b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {435, 0x59c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {436, 0x59d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {437, 0x59e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {438, 0x59f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {439, 0x5a0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {440, 0x5a1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {441, 0x5df,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {442, 0x60b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {443, 0x60c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {444, 0x6e9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {445, 0x6ea,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {446, 0x5a2,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {447, 0x5a3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {448, 0x5a4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {449, 0x5a5,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {450, 0x5a6,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {451, 0x5a7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {452, 0x5a8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {453, 0x5a9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {454, 0x5aa,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {455, 0x5ab,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {456, 0x5ac,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {457, 0x5ad,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {458, 0x5ae,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {459, 0x5af,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {460, 0x5b0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {461, 0x5b1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {462, 0x5e0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {463, 0x60d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {464, 0x60e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {465, 0x6eb,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {466, 0x6ec,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {467, 0x71f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {468, 0x720,  0xb0,  0x90, 0xff00ff, 1, 0},
    {469, 0x721,  0xb0,  0x90, 0xff00ff, 1, 0},
    {470, 0x722,  0xb0,  0x90, 0xff00ff, 1, 0},
    {471, 0x723,  0xb0,  0x90, 0xff00ff, 1, 0},
    {472, 0x724,  0xb0,  0x90, 0xff00ff, 1, 0},
    {473, 0x725,  0xb0,  0x90, 0xff00ff, 1, 0},
    {474, 0x726,  0xb0,  0x90, 0xff00ff, 1, 0},
    {475, 0x727,  0xb0,  0x90, 0xff00ff, 1, 0},
    {476, 0x728,  0xb0,  0x90, 0xff00ff, 1, 0},
    {477, 0x729,  0xb0,  0x90, 0xff00ff, 1, 0},
    {478, 0x72a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {479, 0x72b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {480, 0x72c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {481, 0x72d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {482, 0x72e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {483, 0x72f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {484, 0x730,  0xb0,  0x90, 0xff00ff, 1, 0},
    {485, 0x731,  0xb0,  0x90, 0xff00ff, 1, 0},
    {486, 0x732,  0xb0,  0x90, 0xff00ff, 1, 0},
    {487, 0x733,  0xb0,  0x90, 0xff00ff, 1, 0},
    {488, 0x5e1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {489, 0x5e2,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {490, 0x5e3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {491, 0x5e4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {492, 0x5e5,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {493, 0x5e6,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {494, 0x5e7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {495, 0x5e8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {496, 0x5e9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {497, 0x5ea,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {498, 0x5eb,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {499, 0x5ec,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {500, 0x5ed,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {501, 0x5ef,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {502, 0x5ff,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {503, 0x604,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {504, 0x605,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {505, 0x7b7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {506, 0x5f0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {507, 0x5f1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {508, 0x5f2,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {509, 0x5f3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {510, 0x5f4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {511, 0x5f5,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {512, 0x5f6,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {513, 0x5f7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {514, 0x5f8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {515, 0x5f9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {516, 0x5fa,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {517, 0x5fb,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {518, 0x5fc,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {519, 0x5fe,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {520, 0x600,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {521, 0x606,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {522, 0x607,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {523, 0x7b8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {524, 0x746,  0xb0,  0x90, 0xff00ff, 1, 0},
    {525, 0x747,  0xb0,  0x90, 0xff00ff, 1, 0},
    {526, 0x748,  0xb0,  0x90, 0xff00ff, 1, 0},
    {527, 0x749,  0xb0,  0x90, 0xff00ff, 1, 0},
    {528, 0x74a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {529, 0x74b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {530, 0x74c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {531, 0x74d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {532, 0x74e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {533, 0x74f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {534, 0x750,  0xb0,  0x90, 0xff00ff, 1, 0},
    {535, 0x751,  0xb0,  0x90, 0xff00ff, 1, 0},
    {536, 0x752,  0xb0,  0x90, 0xff00ff, 1, 0},
    {537, 0x780,  0xb0,  0x90, 0xff00ff, 1, 0},
    {538, 0x753,  0xb0,  0x90, 0xff00ff, 1, 0},
    {539, 0x754,  0xb0,  0x90, 0xff00ff, 1, 0},
    {540, 0x755,  0xb0,  0x90, 0xff00ff, 1, 0},
    {541, 0x756,  0xb0,  0x90, 0xff00ff, 1, 0},
    {542, 0x7b6,  0xb0,  0x90, 0xff00ff, 1, 0},
    {543, 0x637,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {544, 0x638,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {545, 0x639,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {546, 0x63a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {547, 0x63b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {548, 0x63c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {549, 0x63d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {550, 0x63e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {551, 0x63f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {552, 0x640,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {553, 0x641,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {554, 0x716,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {555, 0x642,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {556, 0x643,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {557, 0x644,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {558, 0x645,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {559, 0x646,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {560, 0x657,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {561, 0x717,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {562, 0x6ed,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {563, 0x647,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {564, 0x648,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {565, 0x649,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {566, 0x64a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {567, 0x64b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {568, 0x64c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {569, 0x64d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {570, 0x64e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {571, 0x64f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {572, 0x650,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {573, 0x651,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {574, 0x718,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {575, 0x652,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {576, 0x653,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {577, 0x654,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {578, 0x655,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {579, 0x656,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {580, 0x658,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {581, 0x719,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {582, 0x6ee,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {583, 0x757,  0xb0,  0x90, 0xff00ff, 1, 0},
    {584, 0x758,  0xb0,  0x90, 0xff00ff, 1, 0},
    {585, 0x759,  0xb0,  0x90, 0xff00ff, 1, 0},
    {586, 0x75a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {587, 0x75b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {588, 0x75c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {589, 0x75d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {590, 0x75e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {591, 0x75f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {592, 0x760,  0xb0,  0x90, 0xff00ff, 1, 0},
    {593, 0x761,  0xb0,  0x90, 0xff00ff, 1, 0},
    {594, 0x908,  0xb0,  0x90, 0xff00ff, 1, 0},
    {595, 0x762,  0xb0,  0x90, 0xff00ff, 1, 0},
    {596, 0x763,  0xb0,  0x90, 0xff00ff, 1, 0},
    {597, 0x764,  0xb0,  0x90, 0xff00ff, 1, 0},
    {598, 0x765,  0xb0,  0x90, 0xff00ff, 1, 0},
    {599, 0x766,  0xb0,  0x90, 0xff00ff, 1, 0},
    {600, 0x767,  0xb0,  0x90, 0xff00ff, 1, 0},
    {601, 0x8c4,  0xb0,  0x90, 0xff00ff, 1, 0},
    {602, 0x768,  0xb0,  0x90, 0xff00ff, 1, 0},
    {603, 0x68e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {604, 0x68f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {605, 0x690,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {606, 0x691,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {607, 0x692,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {608, 0x693,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {609, 0x694,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {610, 0x695,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {611, 0x696,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {612, 0x697,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {613, 0x698,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {614, 0x699,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {615, 0x69a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {616, 0x69b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {617, 0x69c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {618, 0x69d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {619, 0x69e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {620, 0x6a9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {621, 0x67d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {622, 0x67e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {623, 0x67f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {624, 0x680,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {625, 0x681,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {626, 0x682,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {627, 0x683,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {628, 0x684,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {629, 0x685,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {630, 0x686,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {631, 0x687,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {632, 0x688,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {633, 0x689,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {634, 0x68a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {635, 0x68b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {636, 0x68c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {637, 0x68d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {638, 0x6a8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {639, 0x734,  0xb0,  0x90, 0xff00ff, 1, 0},
    {640, 0x735,  0xb0,  0x90, 0xff00ff, 1, 0},
    {641, 0x736,  0xb0,  0x90, 0xff00ff, 1, 0},
    {642, 0x737,  0xb0,  0x90, 0xff00ff, 1, 0},
    {643, 0x738,  0xb0,  0x90, 0xff00ff, 1, 0},
    {644, 0x739,  0xb0,  0x90, 0xff00ff, 1, 0},
    {645, 0x73a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {646, 0x73b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {647, 0x73c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {648, 0x73d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {649, 0x73e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {650, 0x73f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {651, 0x740,  0xb0,  0x90, 0xff00ff, 1, 0},
    {652, 0x741,  0xb0,  0x90, 0xff00ff, 1, 0},
    {653, 0x742,  0xb0,  0x90, 0xff00ff, 1, 0},
    {654, 0x743,  0xb0,  0x90, 0xff00ff, 1, 0},
    {655, 0x744,  0xb0,  0x90, 0xff00ff, 1, 0},
    {656, 0x745,  0xb0,  0x90, 0xff00ff, 1, 0},
    {657, 0x802,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {658, 0x86f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {659, 0x803,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {660, 0x870,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {661, 0x801,  0xb0,  0x90, 0xff00ff, 1, 0},
    {662, 0x86e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {663, 0x7ef,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {664, 0x82a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {665, 0x82b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {666, 0x82c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {667, 0x82d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {668, 0x891,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {669, 0x892,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {670, 0x7f8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {671, 0x84c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {672, 0x84d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {673, 0x84e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {674, 0x84f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {675, 0x8a3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {676, 0x8a4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {677, 0x7e6,  0xb0,  0x90, 0xff00ff, 1, 0},
    {678, 0x808,  0xb0,  0x90, 0xff00ff, 1, 0},
    {679, 0x809,  0xb0,  0x90, 0xff00ff, 1, 0},
    {680, 0x80a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {681, 0x80b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {682, 0x87f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {683, 0x880,  0xb0,  0x90, 0xff00ff, 1, 0},
    {684, 0x7f0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {685, 0x82e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {686, 0x82f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {687, 0x830,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {688, 0x831,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {689, 0x893,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {690, 0x894,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {691, 0x7f9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {692, 0x850,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {693, 0x851,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {694, 0x852,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {695, 0x853,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {696, 0x8a5,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {697, 0x8a6,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {698, 0x7e7,  0xb0,  0x90, 0xff00ff, 1, 0},
    {699, 0x80c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {700, 0x80d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {701, 0x80e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {702, 0x80f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {703, 0x881,  0xb0,  0x90, 0xff00ff, 1, 0},
    {704, 0x882,  0xb0,  0x90, 0xff00ff, 1, 0},
    {705, 0x7f1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {706, 0x832,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {707, 0x833,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {708, 0x834,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {709, 0x835,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {710, 0x895,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {711, 0x896,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {712, 0x7fa,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {713, 0x854,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {714, 0x855,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {715, 0x856,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {716, 0x857,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {717, 0x8a7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {718, 0x8a8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {719, 0x7e8,  0xb0,  0x90, 0xff00ff, 1, 0},
    {720, 0x810,  0xb0,  0x90, 0xff00ff, 1, 0},
    {721, 0x811,  0xb0,  0x90, 0xff00ff, 1, 0},
    {722, 0x812,  0xb0,  0x90, 0xff00ff, 1, 0},
    {723, 0x813,  0xb0,  0x90, 0xff00ff, 1, 0},
    {724, 0x883,  0xb0,  0x90, 0xff00ff, 1, 0},
    {725, 0x884,  0xb0,  0x90, 0xff00ff, 1, 0},
    {726, 0x7f2,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {727, 0x836,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {728, 0x837,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {729, 0x838,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {730, 0x839,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {731, 0x897,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {732, 0x898,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {733, 0x7fb,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {734, 0x858,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {735, 0x859,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {736, 0x85a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {737, 0x85b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {738, 0x8a9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {739, 0x8aa,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {740, 0x7e9,  0xb0,  0x90, 0xff00ff, 1, 0},
    {741, 0x814,  0xb0,  0x90, 0xff00ff, 1, 0},
    {742, 0x815,  0xb0,  0x90, 0xff00ff, 1, 0},
    {743, 0x816,  0xb0,  0x90, 0xff00ff, 1, 0},
    {744, 0x817,  0xb0,  0x90, 0xff00ff, 1, 0},
    {745, 0x885,  0xb0,  0x90, 0xff00ff, 1, 0},
    {746, 0x886,  0xb0,  0x90, 0xff00ff, 1, 0},
    {747, 0x7f3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {748, 0x83a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {749, 0x83b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {750, 0x83c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {751, 0x83d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {752, 0x899,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {753, 0x89a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {754, 0x7fc,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {755, 0x85c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {756, 0x85d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {757, 0x85e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {758, 0x85f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {759, 0x8ab,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {760, 0x8ac,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {761, 0x7ea,  0xb0,  0x90, 0xff00ff, 1, 0},
    {762, 0x818,  0xb0,  0x90, 0xff00ff, 1, 0},
    {763, 0x819,  0xb0,  0x90, 0xff00ff, 1, 0},
    {764, 0x81a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {765, 0x81b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {766, 0x887,  0xb0,  0x90, 0xff00ff, 1, 0},
    {767, 0x888,  0xb0,  0x90, 0xff00ff, 1, 0},
    {768, 0x7f4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {769, 0x83e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {770, 0x83f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {771, 0x840,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {772, 0x841,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {773, 0x89b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {774, 0x89c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {775, 0x7fd,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {776, 0x860,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {777, 0x861,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {778, 0x862,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {779, 0x863,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {780, 0x8ad,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {781, 0x8ae,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {782, 0x7eb,  0xb0,  0x90, 0xff00ff, 1, 0},
    {783, 0x81c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {784, 0x81d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {785, 0x81e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {786, 0x81f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {787, 0x889,  0xb0,  0x90, 0xff00ff, 1, 0},
    {788, 0x88a,  0xb0,  0x90, 0xff00ff, 1, 0},
    {789, 0x7f5,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {790, 0x842,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {791, 0x843,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {792, 0x844,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {793, 0x89d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {794, 0x89e,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {795, 0x7fe,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {796, 0x864,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {797, 0x865,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {798, 0x866,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {799, 0x8af,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {800, 0x8b0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {801, 0x7ec,  0xb0,  0x90, 0xff00ff, 1, 0},
    {802, 0x820,  0xb0,  0x90, 0xff00ff, 1, 0},
    {803, 0x821,  0xb0,  0x90, 0xff00ff, 1, 0},
    {804, 0x822,  0xb0,  0x90, 0xff00ff, 1, 0},
    {805, 0x88b,  0xb0,  0x90, 0xff00ff, 1, 0},
    {806, 0x88c,  0xb0,  0x90, 0xff00ff, 1, 0},
    {807, 0x7f6,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {808, 0x845,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {809, 0x846,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {810, 0x847,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {811, 0x848,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {812, 0x89f,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {813, 0x8a0,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {814, 0x7ff,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {815, 0x867,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {816, 0x868,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {817, 0x869,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {818, 0x86a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {819, 0x8b1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {820, 0x8b2,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {821, 0x7ed,  0xb0,  0x90, 0xff00ff, 1, 0},
    {822, 0x823,  0xb0,  0x90, 0xff00ff, 1, 0},
    {823, 0x824,  0xb0,  0x90, 0xff00ff, 1, 0},
    {824, 0x825,  0xb0,  0x90, 0xff00ff, 1, 0},
    {825, 0x826,  0xb0,  0x90, 0xff00ff, 1, 0},
    {826, 0x88d,  0xb0,  0x90, 0xff00ff, 1, 0},
    {827, 0x88e,  0xb0,  0x90, 0xff00ff, 1, 0},
    {828, 0x7f7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {829, 0x849,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {830, 0x84a,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {831, 0x84b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {832, 0x8a1,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {833, 0x8a2,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {834, 0x800,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {835, 0x86b,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {836, 0x86c,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {837, 0x86d,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {838, 0x8b3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {839, 0x8b4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {840, 0x7ee,  0xb0,  0x90, 0xff00ff, 1, 0},
    {841, 0x827,  0xb0,  0x90, 0xff00ff, 1, 0},
    {842, 0x828,  0xb0,  0x90, 0xff00ff, 1, 0},
    {843, 0x829,  0xb0,  0x90, 0xff00ff, 1, 0},
    {844, 0x88f,  0xb0,  0x90, 0xff00ff, 1, 0},
    {845, 0x890,  0xb0,  0x90, 0xff00ff, 1, 0},
    {846, 0x8d3,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {847, 0x8d4,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {848, 0x8d5,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {849, 0x8d6,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {850, 0x8d7,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {851, 0x8d8,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {852, 0x8d9,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {853, 0x8da,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {854, 0x8db,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {855, 0x8dc,  0xa0,  0xb0, 0xff00ff, 1, 0},
    {856, 0x8ce,  0xb0,  0x90, 0xff00ff, 1, 0},
    {857, 0x8cf,  0xb0,  0x90, 0xff00ff, 1, 0},
    {858, 0x8d0,  0xb0,  0x90, 0xff00ff, 1, 0},
    {859, 0x8d1,  0xb0,  0x90, 0xff00ff, 1, 0},
    {860, 0x8d2,  0xb0,  0x90, 0xff00ff, 1, 0},
    {861, 0x580,  0x80,  0x80, 0xff00ff, 0, 2},
    {863, 0x581,  0x80,  0x80, 0xff00ff, 0, 2},
    {862, 0x582,  0x80,  0x80, 0xff00ff, 0, 2},
    { 62, 0x608,  0x80,  0x80, 0xff00ff, 0, 2},
    { 63, 0x609,  0x80,  0x80, 0xff00ff, 0, 2},
    { 64, 0x60a,  0x80,  0x80, 0xff00ff, 0, 2},
};

void ar_register_game_sprites(void *zdd, uint16_t group, void *settings)
{
    for (size_t i = 0; i < sizeof(game_sprites) / sizeof(game_sprites[0]); i++) {
        const struct ar_game_sprite_entry *e = &game_sprites[i];
        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],
            zdd, settings, e->id,
            e->width, e->height, e->colorkey,
            e->scale_flag, e->type, group);
    }
}
