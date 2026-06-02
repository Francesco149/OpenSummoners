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
ar_sprite_slot  g_ar_sprite_ramp_slots[AR_SPRITE_RAMP_COUNT];
ar_sprite_slot *g_ar_sprite_ramp_table[AR_SPRITE_RAMP_COUNT];
ar_info_entry   g_ar_info_entries[AR_INFO_ENTRY_COUNT];
ar_info_entry  *g_ar_info_table[AR_INFO_ENTRY_COUNT];
ar_gdi_slot     g_ar_gdi_slots[AR_GDI_SLOT_COUNT];
ar_gdi_slot    *g_ar_gdi_table[AR_GDI_SLOT_COUNT];
ar_sound_slot   g_ar_sound_slots[AR_SOUND_SLOT_COUNT];
ar_sound_slot  *g_ar_sound_table[AR_SOUND_SLOT_COUNT];

void ar_state_init(void)
{
    memset(g_ar_sprite_slots,      0, sizeof g_ar_sprite_slots);
    memset(g_ar_sprite_ramp_slots, 0, sizeof g_ar_sprite_ramp_slots);
    memset(g_ar_info_entries,      0, sizeof g_ar_info_entries);
    memset(g_ar_gdi_slots,         0, sizeof g_ar_gdi_slots);
    memset(g_ar_sound_slots,       0, sizeof g_ar_sound_slots);
    for (int i = 0; i < AR_SPRITE_SLOT_COUNT; i++) {
        g_ar_sprite_table[i] = &g_ar_sprite_slots[i];
    }
    for (int i = 0; i < AR_SPRITE_RAMP_COUNT; i++) {
        g_ar_sprite_ramp_table[i] = &g_ar_sprite_ramp_slots[i];
    }
    for (int i = 0; i < AR_INFO_ENTRY_COUNT; i++) {
        g_ar_info_table[i] = &g_ar_info_entries[i];
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
     * for entry_count iterations writing 0 to entries[i].frames and
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

/* ─── FUN_00582b80 — clone metadata from one sprite slot into another ─ */

void ar_sprite_slot_clone(ar_sprite_slot *dst, const ar_sprite_slot *src)
{
    /* Prologue is an inline expansion of ar_sprite_slot_destroy on
     * dst — same free-aux + walk-entries-and-free + free-array
     * sequence.  Call the helper for parity with ar_sprite_slot_register. */
    ar_sprite_slot_destroy(dst);

    /* Field stamps in retail order.  entries is a fresh 1-entry calloc
     * (operator_new(8) + zero-loop in retail; calloc handles both). */
    dst->zdd         = src->zdd;
    dst->entry_count = 1;
    dst->entries     = (ar_sprite_entry *)calloc(1, sizeof(ar_sprite_entry));
    dst->settings    = src->settings;
    dst->resource_id = src->resource_id;
    dst->width       = src->width;
    dst->height      = src->height;
    dst->colorkey    = src->colorkey;
    dst->f_08        = 0;
    dst->f_18        = 0;
    dst->scale_flag  = src->scale_flag;
    dst->type        = src->type;
    dst->group       = src->group;
    dst->f_38        = 0;

    /* Deep-copy the aux buffer iff src has one.  Stride is 24 B per
     * entry; src->f_38 is the entry count.  Retail allocates even
     * when f_38==0 (operator_new(0) result stashed in dst->aux_buf);
     * our destructor is NULL-safe and the malloc(0) edge is
     * implementation-defined but innocuous, so we match retail. */
    if (src->aux_buf != NULL && dst->aux_buf == NULL) {
        size_t count = src->f_38;
        dst->aux_buf = malloc(count * 24);
        if (count != 0) {
            memcpy(dst->aux_buf, src->aux_buf, count * 24);
        }
    }
}

/* ─── FUN_00582d00 — zero a parallel-info-table entry ────────────── */

void ar_info_entry_clear(ar_info_entry *entry)
{
    /* Retail writes: word@+0, dword@+4/+8/+12.  Pad@+2..+3 untouched. */
    entry->marker  = 0;
    entry->flag    = 0;
    entry->data    = NULL;
    entry->palette = NULL;
}

/* ─── unified sprite-slot pool accessor ──────────────────────────── */

ar_sprite_slot *ar_pool_get_slot(uint16_t pool_idx)
{
    /* Pool index 0 (retail 0x008a760c) is the allocator-zeroed sentinel
     * with no observed consumer — see rabbit-hole §5/§7.  No real call
     * site touches it; return NULL so accidental usage trips immediately
     * instead of aliasing some other slot. */
    if (pool_idx == 0)                return NULL;
    if (pool_idx <  AR_SPRITE_RAMP_COUNT + 1)
        return &g_ar_sprite_ramp_slots[pool_idx - 1];
    return &g_ar_sprite_slots[pool_idx - (AR_SPRITE_RAMP_COUNT + 1)];
}

/* ─── FUN_00418470 — sprite-bank frame getter ────────────────────────
 *
 * Lazy-decode the bank's sheet on first use, then index its frame-surface
 * array.  See the header for the two-level indirection; offsets pinned at
 * 0x418470..0x418495. */
ar_sprite_decode_fn ar_sprite_decode_hook = NULL;

void *ar_sprite_slot_frame(ar_sprite_slot *slot, uint16_t frame_id)
{
    if (slot == NULL || slot->entries == NULL) {
        return NULL;                       /* defensive — retail derefs */
    }
    if (slot->entries[0].frames == NULL) { /* *(*bank) == 0 → needs decode */
        if (ar_sprite_decode_hook != NULL) {
            ar_sprite_decode_hook(slot);   /* 0x4184a0 (decoder; unported) */
        }
        if (slot->entries[0].frames == NULL) {
            return NULL;                   /* headless: still undecoded   */
        }
    }
    void **frames = (void **)slot->entries[0].frames;
    return frames[frame_id];               /* arr[id & 0xffff] */
}

/* ─── FUN_004179b0 — SS_MGR thiscall slot-clone via pool indices ──── */

void ar_ss_mgr_clone_slot(uint16_t dst_pool_idx, uint16_t src_pool_idx)
{
    /* Slot side mirrors ar_sprite_slot_clone exactly (FUN_004179b0 and
     * FUN_00582b80 share the same prologue/destroy + field-stamp +
     * entries-alloc + aux_buf-copy shape).  The only delta is the
     * thiscall indirection through input_mgr's +0xaac sprite-slot
     * pointer table — see rabbit-hole §7 for the SS_MGR == input_mgr
     * (0x008a6b60) identity. */
    ar_sprite_slot_clone(ar_pool_get_slot(dst_pool_idx),
                         ar_pool_get_slot(src_pool_idx));

    /* Info-entry side: zero dst (the 14-byte clear shape — same as
     * ar_info_entry_clear), then copy marker (word@+0) and flag
     * (dword@+4) from src.  data (+8), palette (+0xc), and f_10 (+0x10)
     * stay zero — retail's FUN_004179b0 does NOT propagate them. */
    ar_info_entry *dst_info = g_ar_info_table[dst_pool_idx];
    ar_info_entry *src_info = g_ar_info_table[src_pool_idx];
    ar_info_entry_clear(dst_info);
    dst_info->marker = src_info->marker;
    dst_info->flag   = src_info->flag;
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

/* ─── FUN_0057a330 — palette-ramp + portrait sprite register batch ─ */

/* One palette ramp.  Each ramp registers a small sprite from sotesp.dll
 * at g_ar_sprite_ramp_slots[idx], then overrides three palette regions
 * (entry 1, entries 41..50, entries 51..70) using the same 3-color
 * scheme ar_register_main_sprites uses but with per-ramp colors. */
struct ar_palette_ramp_entry {
    uint8_t   idx;       /* index into g_ar_sprite_ramp_slots[] */
    uint16_t  id;        /* PE resource ID (sotesp.dll) */
    uint16_t  width;     /* sprite width */
    uint16_t  height;    /* sprite height */
    uint32_t  bg;        /* palette[1]      = bg     (one COLORREF) */
    uint32_t  mid;       /* palette[41..50] = mid    (10 entries) */
    uint32_t  fg;        /* palette[51..70] = lerp(mid → fg, i/20) */
};

/* Helper: run one palette-ramp block (register sprite + run palette
 * session + install).  Matches the repeated body that retail issues 12
 * times inside FUN_0057a330; all 12 share the same three-region ramp
 * scheme so the only per-ramp variation is the slot/id/dimensions/colors
 * captured in `e`.
 *
 * No-op on the palette section when ar_palette_session_begin returns
 * false (resource missing or wrong bit depth) — matches retail's
 * implicit early-out via the if-guard around the install. */
static void ar_run_palette_ramp(const struct ar_palette_ramp_entry *e,
                                 void *zdd, void *sotesp_module, uint16_t group)
{
    ar_sprite_slot *s = &g_ar_sprite_ramp_slots[e->idx];
    ar_sprite_slot_register(s, zdd, sotesp_module, e->id,
                            e->width, e->height,
                            /*colorkey=*/0, /*scale_flag=*/0,
                            /*type=*/2, group);

    uint8_t palette[1024];
    if (ar_palette_session_begin(s, palette)) {
        ar_palette_pack_entry(&palette[1 * 4], e->bg);
        for (int i = 0; i < 10; i++) {
            ar_palette_pack_entry(&palette[(41 + i) * 4], e->mid);
        }
        for (int i = 1; i <= 20; i++) {
            uint32_t c = ar_color_lerp(e->mid, e->fg, i, 20);
            ar_palette_pack_entry(&palette[(50 + i) * 4], c);
        }
        ar_palette_install(s, palette);
    }
}

/* Portrait entry — same idx-relative-to-0x8a7640 convention as
 * ar_main_sprite_entry, with an extra (flags_idx, flags_value) pair
 * for the
 * `g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + flags_idx]->flag = flags_value`
 * write that follows each portrait register in retail. */
struct ar_ramp_extra_entry {
    uint8_t   idx;          /* g_ar_sprite_slots[idx] */
    uint8_t   flags_idx;    /* 0..13 within AR_INFO_RAMP_FLAGS_COUNT */
    uint32_t  flags_value;  /* 0 or 3 */
    uint16_t  id;
    uint16_t  width;
    uint16_t  height;
    uint32_t  colorkey;
    uint8_t   scale_flag;
    uint8_t   type;
};

void ar_register_palette_ramps(void *zdd, uint16_t group, void *settings,
                               void *sotesp_module)
{
    /* The 12 palette ramps.  All ramps register from sotesp.dll (NOT
     * the launcher settings), match the same 3-color scheme, and use
     * type=2 / colorkey=0 / scale_flag=0.  Index order matches retail
     * BSS order (slots at 0x8a7610 stride 4 → idx 0..11), which is
     * NOT the order retail issues them — but iteration is independent
     * so we list by ramp_slot idx for readability. */
    static const struct ar_palette_ramp_entry ramps[] = {
        /* idx, id,    w,    h,    bg,        mid,        fg */
        {  0,  0x413, 0x18, 0x18, 0x404040,  0x404040,   0xffffff },  /* 0x8a7610 */
        {  1,  0x412, 0x20, 0x20, 0x404040,  0x404040,   0xffffff },  /* 0x8a7614 */
        {  2,  0x413, 0x18, 0x18, 0xff00f0,  0xcc6ccc,   0xffffff },  /* 0x8a7618 */
        {  3,  0x412, 0x20, 0x20, 0xff00f0,  0xcc6ccc,   0xffffff },  /* 0x8a761c */
        {  4,  0x413, 0x18, 0x18, 0x0000ff,  0x6c6ccc,   0xffffff },  /* 0x8a7620 */
        {  5,  0x412, 0x20, 0x20, 0x0000ff,  0x6c6ccc,   0xffffff },  /* 0x8a7624 */
        {  6,  0x413, 0x18, 0x18, 0xff0000,  0xcc6c6c,   0xffffff },  /* 0x8a7628 */
        {  7,  0x412, 0x20, 0x20, 0x008000,  0x40c040,   0xc0ffc0 },  /* 0x8a762c */
        {  8,  0x413, 0x18, 0x18, 0x404040,  0x00c0ff,   0xc0f0ff },  /* 0x8a7630 */
        {  9,  0x412, 0x20, 0x20, 0x404040,  0x00c0ff,   0xc0f0ff },  /* 0x8a7634 */
        { 10,  0x413, 0x18, 0x18, 0x404040,  0x0000c0,   0xc0c0ff },  /* 0x8a7638 */
        { 11,  0x413, 0x18, 0x18, 0x0080ff,  0x0080ff,   0xffffff },  /* 0x8a763c */
    };
    for (size_t i = 0; i < sizeof(ramps) / sizeof(ramps[0]); i++) {
        ar_run_palette_ramp(&ramps[i], zdd, sotesp_module, group);
    }

    /* 23 trailing FUN_005748c0 calls + 2 inlined-equivalent register
     * blocks (retail addrs 0x8a76d0 and 0x8a76d8 — spelled as the
     * open-coded destructor + field-writes pattern, but same
     * observable end state as ar_sprite_slot_register).  Indices are
     * relative to the main sprite pool base 0x008a7640.
     *
     * `settings` (NOT sotesp_module) is the +0x3c value for every
     * entry here — these registers load from the launcher settings
     * record, not sotesp.dll. */
    static const struct ar_main_sprite_entry extras[] = {
        /* idx, id,    w,     h,    colorkey,    scale, type */
        { 36, 0x44f, 0x20,  0x20,  0,           0, 2 },  /* 0x8a76d0 (inline-shaped in retail) */
        { 38, 0x76d, 0x20,  0x20,  0,           0, 2 },  /* 0x8a76d8 (inline-shaped in retail) */
        { 37, 0x5ab, 0x20,  0x20,  0,           0, 2 },  /* 0x8a76d4 — settings=NULL in retail */
        { 44, 0x450, 0x30,  0x30,  0,           0, 2 },  /* 0x8a76f0 */
        { 45, 0x451, 0x40,  0x40,  0,           1, 2 },  /* 0x8a76f4 */
        { 49, 0x776, 0xc0,  0x40,  0,           0, 2 },  /* 0x8a7704 */
        { 57, 1099,  0x160, 0x60,  0,           0, 2 },  /* 0x8a7724 — id 0x44b */
        { 58, 0x778, 0x60,  0x40,  0,           1, 2 },  /* 0x8a7728 */
        { 51, 0x44d, 0x20,  0x20,  0,           1, 0 },  /* 0x8a770c */
        { 52, 0x44a, 0xc0,  0x40,  0,           0, 2 },  /* 0x8a7710 */
        { 39, 0x76c, 0x28,  0x28,  0,           0, 2 },  /* 0x8a76dc */
        { 56, 0x775, 0x30,  0x10,  0xff00ff,    0, 2 },  /* 0x8a7720 */
        { 59, 0x601, 0x60,  0x60,  0xff00ff,    1, 2 },  /* 0x8a772c */
        { 60, 0x602, 0x60,  0x60,  0xff00ff,    1, 2 },  /* 0x8a7730 */
        { 61, 0x603, 0x60,  0x60,  0xff00ff,    1, 2 },  /* 0x8a7734 */
        { 53, 0x449, 0x140, 0x80,  0,           1, 2 },  /* 0x8a7714 */
        { 54, 0x454, 400,   0x80,  0xff00ff,    1, 2 },  /* 0x8a7718 — id 0x190 */
        { 40, 0x458, 0x40,  4,     0x1ffffff,   1, 0 },  /* 0x8a76e0 */
        { 41, 0x583, 0x40,  4,     0x1ffffff,   0, 0 },  /* 0x8a76e4 */
        { 33, 0x44e, 0x80,  0x10,  0,           1, 0 },  /* 0x8a76c4 */
        { 34, 0x777, 0xc0,  0x10,  0,           1, 0 },  /* 0x8a76c8 */
        { 35, 0x909, 0x20,  0xf0,  0,           1, 0 },  /* 0x8a76cc */
    };
    /* Retail's 0x8a76d4 entry passes `settings=NULL` (not the caller's
     * settings) — the only entry in the extras list with that special
     * shape.  Issue it separately so the table stays uniform. */
    for (size_t i = 0; i < sizeof(extras) / sizeof(extras[0]); i++) {
        const struct ar_main_sprite_entry *e = &extras[i];
        void *slot_settings = (e->idx == 37) ? NULL : settings;
        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],
            zdd, slot_settings, e->id,
            e->width, e->height, e->colorkey,
            e->scale_flag, e->type, group);
    }

    /* 14 portrait blocks — each is FUN_005748c0 followed by a write
     * of `flags_value` (0 or 3) into
     * g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + flags_idx]->flag.
     * Retail's `*(int *)(DAT_008a85xx + 4) = N` pattern, where the
     * DAT slot holds the ar_info_entry pointer.  All portraits share
     * width=0x50, type=0, scale_flag=0, colorkey=0x1ffffff (except
     * the four 0xff00ff colorkey'd entries at the tail).
     *
     * Retail iterates them in the order listed — the flag index
     * matches the sprite-slot's position in the 0x8a7744..0x8a7778
     * BSS range one-to-one (which also matches the info-entry pool's
     * BSS layout one-to-one). */
    static const struct ar_ramp_extra_entry portraits[] = {
        /* idx, flag,val,  id,    w,    h,     ck,         scale, type */
        { 65,  0, 3,  1000,  0x50, 0x160, 0x1ffffff,  0, 0 },  /* 0x8a7744, flags @0x8a8578 */
        { 66,  1, 0,  0x3e9, 0x50, 0x160, 0x1ffffff,  0, 0 },  /* 0x8a7748, flags @0x8a857c */
        { 67,  2, 0,  0x7e4, 0x50, 0x160, 0x1ffffff,  0, 0 },  /* 0x8a774c, flags @0x8a8580 */
        { 68,  3, 3,  0x7b3, 0x50, 0x1e0, 0x1ffffff,  0, 0 },  /* 0x8a7750, flags @0x8a8584 */
        { 69,  4, 0,  0x7b4, 0x50, 0x1e0, 0x1ffffff,  0, 0 },  /* 0x8a7754, flags @0x8a8588 */
        { 72,  7, 3,  0x3ea, 0x50, 0x160, 0x1ffffff,  0, 0 },  /* 0x8a7760, flags @0x8a8594 */
        { 73,  8, 0,  0x3eb, 0x50, 0x160, 0x1ffffff,  0, 0 },  /* 0x8a7764, flags @0x8a8598 */
        { 74,  9, 3,  0x439, 0x50, 0x140, 0xff00ff,   0, 0 },  /* 0x8a7768, flags @0x8a859c */
        { 75, 10, 3,  0x43a, 0x50, 0x100, 0,          0, 0 },  /* 0x8a776c, flags @0x8a85a0 */
        { 76, 11, 3,  0x6ba, 0x50, 0xc0,  0,          0, 0 },  /* 0x8a7770, flags @0x8a85a4 */
        { 77, 12, 3,  0x6b8, 0x50, 0x140, 0xff00ff,   0, 0 },  /* 0x8a7774, flags @0x8a85a8 */
        { 78, 13, 3,  0x6b9, 0x50, 0x90,  0xff00ff,   0, 0 },  /* 0x8a7778, flags @0x8a85ac */
        { 70,  5, 0,  0x879, 0x50, 0x1e0, 0x1ffffff,  0, 0 },  /* 0x8a7758, flags @0x8a858c */
        { 71,  6, 0,  0x87c, 0x50, 0x140, 0xff00ff,   0, 0 },  /* 0x8a775c, flags @0x8a8590 */
    };
    for (size_t i = 0; i < sizeof(portraits) / sizeof(portraits[0]); i++) {
        const struct ar_ramp_extra_entry *e = &portraits[i];
        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],
            zdd, settings, e->id,
            e->width, e->height, e->colorkey,
            e->scale_flag, e->type, group);
        g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + e->flags_idx]->flag =
            e->flags_value;
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

/* ─── FUN_0057ca40 — group-3 sprite-register batch (PARTIAL PORT) ──
 *
 * 233 sprite-slot register operations in retail issue order — the
 * cleanly-modelable subset of the 24884-byte function.  Followed
 * below by the 4th-pass info-entry pool writes (443 events) that
 * shadow the slot decls.  See header docstring for what's still
 * deferred (94 SS_MGR clone calls and the 9 inline-clone clusters'
 * sprite-slot ops; the info-entry writes are now all covered).
 *
 * Source order preserved verbatim so future call-trace audits match
 * retail's pos column; idx column is the sprite pool index */
struct ar_group3_entry {
    uint16_t  idx;        /* g_ar_sprite_slots[idx] */
    uint16_t  id;         /* PE resource id */
    uint16_t  width;
    uint16_t  height;
    uint32_t  colorkey;
    uint8_t   scale_flag;
    uint8_t   type;
};

static const struct ar_group3_entry group3_sprites[] = {
    /*  idx,  id,     w,     h,     colorkey,  scale, type    (retail BSS addr) */
    {   79, 0x0423, 0x20  , 0x20  , 0x0       , 0, 2 },  /* 0x008a777c */
    {   80, 0x0424, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7780 */
    {   85, 0x0433, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7794 */
    {   86, 0x0434, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7798 */
    {   81, 0x06a6, 0xc0  , 0xa0  , 0x1ffffff , 0, 0 },  /* 0x008a7784 */
    {   82, 0x06a7, 0x80  , 0xa0  , 0x1ffffff , 0, 0 },  /* 0x008a7788 */
    {   83, 0x0425, 0x40  , 0x80  , 0x0       , 0, 2 },  /* 0x008a778c */
    {   84, 0x0438, 0x20  , 0x30  , 0x0       , 1, 0 },  /* 0x008a7790 */
    {   87, 0x041c, 0x20  , 0x20  , 0x1ffffff , 0, 0 },  /* 0x008a779c */
    {   88, 0x091a, 0x20  , 0x20  , 0x1ffffff , 0, 0 },  /* 0x008a77a0 */
    {   89, 0x041f, 0x40  , 0x40  , 0x1ffffff , 0, 0 },  /* 0x008a77a4 */
    {   90, 0x041b, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a77a8 */
    {   92, 0x0421, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a77b0 */
    {   91, 0x041e, 0x20  , 0x20  , 0x0       , 0, 2 },  /* 0x008a77ac */
    {   93, 0x0422, 0xc0  , 0x80  , 0x1ffffff , 1, 0 },  /* 0x008a77b4 */
    {   99, 0x06e5, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a77cc */
    {  100, 0x090f, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a77d0 */
    {   94, 0x07a8, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a77b8 */
    {   95, 0x0807, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a77bc */
    {  101, 0x0663, 0x20  , 0x20  , 0x0       , 0, 2 },  /* 0x008a77d4 */
    {  102, 0x07b5, 0x20  , 0x20  , 0x0       , 0, 2 },  /* 0x008a77d8 */
    {  103, 0x0664, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a77dc */
    {  104, 0x0662, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a77e0 */
    {  105, 0x07b1, 0xc0  , 0x80  , 0x0       , 0, 2 },  /* 0x008a77e4 */
    {  106, 0x0665, 0x60  , 0x60  , 0x0       , 0, 2 },  /* 0x008a77e8 */
    {   96, 0x07b2, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a77c0 */
    {   97, 0x07d7, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a77c4 */
    {   98, 0x087a, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a77c8 */
    {  107, 0x08cb, 0x60  , 0x60  , 0x0       , 0, 2 },  /* 0x008a77ec */
    {  108, 0x0415, 0x40  , 0x40  , 0x1ffffff , 0, 0 },  /* 0x008a77f0 */
    {  109, 0x0417, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a77f4 */
    {  110, 0x0418, 0x40  , 0x80  , 0x0       , 0, 2 },  /* 0x008a77f8 */
    {  111, 0x0414, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a77fc */
    {  112, 0x0419, 0x80  , 0xa0  , 0x1ffffff , 0, 0 },  /* 0x008a7800 */
    {  113, 0x0416, 0xc0  , 0xa0  , 0x1ffffff , 1, 0 },  /* 0x008a7804 */
    {  114, 0x06dd, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7808 */
    {  115, 0x06de, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a780c */
    {  116, 0x06e0, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7810 */
    {  117, 0x07aa, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7814 */
    {  118, 0x041a, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7818 */
    {  119, 0x06df, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a781c */
    {  120, 0x06e3, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7820 */
    {  121, 0x07ab, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7824 */
    {  122, 0x057f, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7828 */
    {  123, 0x06e1, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a782c */
    {  124, 0x06e2, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7830 */
    {  125, 0x07a9, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7834 */
    {  338, 0x0481, 0x140 , 0x140 , 0x0       , 0, 0 },  /* 0x008a7b88 */
    {  339, 0x0482, 0x160 , 0x120 , 0x0       , 0, 0 },  /* 0x008a7b8c */
    {  340, 0x0483, 0xa0  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7b90 */
    {  341, 0x07a5, 0x140 , 0x140 , 0x0       , 0, 0 },  /* 0x008a7b94 */
    {  342, 0x0427, 0x50  , 0x50  , 0x0       , 0, 2 },  /* 0x008a7b98 */
    {  344, 0x047d, 0x60  , 0x60  , 0x0       , 0, 2 },  /* 0x008a7ba0 */
    {  345, 0x047c, 0x50  , 0x30  , 0x0       , 0, 2 },  /* 0x008a7ba4 */
    {  343, 0x0426, 0x30  , 0x30  , 0x0       , 0, 2 },  /* 0x008a7b9c */
    {  357, 0x0428, 0xc0  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7bd4 */
    {  358, 0x0429, 0x140 , 0x100 , 0x0       , 0, 2 },  /* 0x008a7bd8 */
    {  359, 0x042a, 0x80  , 0x120 , 0x0       , 0, 2 },  /* 0x008a7bdc */
    {  360, 0x03ec, 0x80  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7be0 */
    {  366, 0x0432, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7bf8 */
    {  351, 0x0403, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7bbc */
    {  352, 0x0404, 0x40  , 0x40  , 0x0       , 0, 2 },  /* 0x008a7bc0 */
    {  353, 0x0405, 0x50  , 0x50  , 0x0       , 0, 2 },  /* 0x008a7bc4 */
    {  354, 0x03ff, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7bc8 */
    {  355, 0x0400, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7bcc */
    {  356, 0x0401, 0xc0  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7bd0 */
    {  350, 0x0402, 0x28  , 0x28  , 0x0       , 0, 2 },  /* 0x008a7bb8 */
    {  361, 0x0436, 0x40  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7be4 */
    {  362, 0x08ba, 0x40  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7be8 */
    {  363, 0x08bb, 0xa0  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7bec */
    {  364, 0x08bc, 0x80  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7bf0 */
    {  365, 0x0437, 0x40  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7bf4 */
    {  367, 0x0431, 0x40  , 0x20  , 0x0       , 0, 2 },  /* 0x008a7bfc */
    {  368, 0x0430, 0x40  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c00 */
    {  369, 0x042f, 0x80  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c04 */
    {  384, 0x06a1, 0x50  , 0x60  , 0x0       , 0, 2 },  /* 0x008a7c40 */
    {  385, 0x06a3, 0x20  , 0x60  , 0x0       , 0, 2 },  /* 0x008a7c44 */
    {  386, 0x06a2, 0x100 , 0x100 , 0x0       , 0, 2 },  /* 0x008a7c48 */
    {  370, 0x0590, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7c08 */
    {  373, 0x058f, 0x40  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c14 */
    {  374, 0x058e, 0x80  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c18 */
    {  375, 0x0769, 0x40  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c1c */
    {  376, 0x076a, 0x80  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c20 */
    {  377, 0x076b, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7c24 */
    {  380, 0x06b6, 0x60  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7c30 */
    {  381, 0x078d, 0x40  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c34 */
    {  382, 0x078e, 0x40  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c38 */
    {  383, 0x0703, 0x40  , 0xa0  , 0x0       , 0, 2 },  /* 0x008a7c3c */
    {  387, 0x08b7, 0x40  , 0x80  , 0x0       , 0, 2 },  /* 0x008a7c4c */
    {  388, 0x08b8, 0x40  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7c50 */
    {  389, 0x08b9, 0xa0  , 0xc0  , 0x0       , 0, 2 },  /* 0x008a7c54 */
    {  390, 0x042b, 0x40  , 0xe0  , 0x0       , 0, 1 },  /* 0x008a7c58 */
    {  391, 0x042c, 0x40  , 0x40  , 0x0       , 0, 1 },  /* 0x008a7c5c */
    {  392, 0x042d, 0xc0  , 0xe0  , 0x0       , 0, 2 },  /* 0x008a7c60 */
    {  393, 0x042e, 0x80  , 0xe0  , 0x0       , 0, 2 },  /* 0x008a7c64 */
    {  278, 0x0442, 0x60  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7a98 */
    {  281, 0x0447, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7aa4 */
    {  275, 0x0440, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7a8c */
    {  284, 0x0441, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7ab0 */
    {  286, 0x0445, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7ab8 */
    {  288, 0x0797, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7ac0 */
    {  290, 0x0538, 0xf0  , 0xf0  , 0x0       , 1, 2 },  /* 0x008a7ac8 */
    {  296, 0x053b, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7ae0 */
    {  297, 0x0900, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7ae4 */
    {  305, 0x0871, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7b04 */
    {  291, 0x07c3, 0x90  , 0x90  , 0x0       , 1, 2 },  /* 0x008a7acc */
    {  292, 0x07c4, 0xf0  , 0xf0  , 0x0       , 1, 2 },  /* 0x008a7ad0 */
    {  293, 0x07c5, 0x90  , 0x90  , 0x0       , 0, 2 },  /* 0x008a7ad4 */
    {  294, 0x07c6, 0x60  , 0x60  , 0x0       , 0, 2 },  /* 0x008a7ad8 */
    {  295, 0x07c7, 0x170 , 0x170 , 0x0       , 0, 2 },  /* 0x008a7adc */
    {  308, 0x06b1, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7b10 */
    {  306, 0x06ad, 0x60  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7b08 */
    {  250, 0x0804, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7a28 */
    {  249, 0x0444, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7a24 */
    {  251, 0x06db, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7a2c */
    {  253, 0x0443, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7a34 */
    {  309, 0x0669, 0xa0  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7b14 */
    {  303, 0x07d4, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7afc */
    {  301, 0x0666, 0x60  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7af4 */
    {  270, 0x07b9, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7a78 */
    {  273, 0x08be, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7a84 */
    {  255, 0x0446, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7a3c */
    {  256, 0x06ac, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7a40 */
    {  266, 0x071d, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7a68 */
    {  269, 0x07ac, 0xe0  , 0x100 , 0x0       , 1, 2 },  /* 0x008a7a74 */
    {  313, 0x03fa, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7b24 */
    {  314, 0x03f4, 0x30  , 0x30  , 0x0       , 1, 2 },  /* 0x008a7b28 */
    {  315, 0x03fc, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7b2c */
    {  316, 0x03fb, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7b30 */
    {  319, 0x03f8, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7b3c */
    {  321, 0x03f3, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7b44 */
    {  324, 0x03f5, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7b50 */
    {  325, 0x03f6, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7b54 */
    {  326, 0x03f7, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7b58 */
    {  327, 0x06bf, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7b5c */
    {  328, 0x06be, 0x20  , 0x20  , 0x1ffffff , 1, 0 },  /* 0x008a7b60 */
    {  329, 0x03fe, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7b64 */
    {  334, 0x03fd, 0x40  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7b78 */
    {  335, 0x06d9, 0x80  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7b7c */
    {  336, 0x08bd, 0x60  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7b80 */
    {  337, 0x078f, 0xc0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7b84 */
    {  422, 0x047f, 0x28  , 0x28  , 0x0       , 1, 2 },  /* 0x008a7cd8 */
    {  423, 0x0480, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7cdc */
    {  346, 0x0667, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7ba8 */
    {  347, 0x06da, 0x80  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7bac */
    {  348, 0x06b5, 0x80  , 0xe0  , 0x0       , 1, 2 },  /* 0x008a7bb0 */
    {  349, 0x087b, 0x80  , 0xe0  , 0x0       , 1, 2 },  /* 0x008a7bb4 */
    {  332, 0x06b3, 0x80  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7b70 */
    {  333, 0x078c, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7b74 */
    {  330, 0x06d8, 0x100 , 0x40  , 0x0       , 1, 2 },  /* 0x008a7b68 */
    {  331, 0x0789, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7b6c */
    {  394, 0x043b, 0x60  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7c68 */
    {  395, 0x043c, 0x30  , 0x30  , 0x0       , 1, 2 },  /* 0x008a7c6c */
    {  396, 0x0875, 0x30  , 0x30  , 0x0       , 1, 2 },  /* 0x008a7c70 */
    {  397, 0x0878, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c74 */
    {  398, 0x043e, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c78 */
    {  400, 0x043f, 0xa0  , 0xc0  , 0x0       , 1, 2 },  /* 0x008a7c80 */
    {  399, 0x07bd, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c7c */
    {  401, 0x0806, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7c84 */
    {  402, 0x0874, 0xa0  , 0xa0  , 0x0       , 1, 2 },  /* 0x008a7c88 */
    {  403, 0x0704, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c8c */
    {  404, 0x0409, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c90 */
    {  405, 0x07e5, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c94 */
    {  406, 0x040a, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7c98 */
    {  407, 0x040b, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7c9c */
    {  408, 0x0539, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7ca0 */
    {  410, 0x040c, 0x80  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7ca8 */
    {  409, 0x08ff, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7ca4 */
    {  412, 0x040d, 0xb0  , 0xb0  , 0x0       , 1, 2 },  /* 0x008a7cb0 */
    {  411, 0x040e, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7cac */
    {  413, 0x0408, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7cb4 */
    {  414, 0x0408, 0x20  , 0x20  , 0x0       , 1, 2 },  /* 0x008a7cb8 */
    {  415, 0x0407, 0x60  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7cbc */
    {  416, 0x0591, 0x90  , 0x90  , 0x0       , 1, 2 },  /* 0x008a7cc0 */
    {  417, 0x0406, 0x40  , 0x40  , 0x0       , 1, 0 },  /* 0x008a7cc4 */
    {  418, 0x0410, 0x80  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7cc8 */
    {  419, 0x07d0, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7ccc */
    {  420, 0x07d3, 0xc0  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7cd0 */
    {  421, 0x047e, 0x40  , 0x40  , 0x0       , 1, 2 },  /* 0x008a7cd4 */
    {  240, 0x079d, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7a00 */
    {  235, 0x079f, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79ec */
    {  241, 0x079e, 0x50  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7a04 */
    {  134, 0x0459, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7858 */
    {  136, 0x045a, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7860 */
    {  138, 0x07a3, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7868 */
    {  140, 0x045b, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7870 */
    {  143, 0x045c, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a787c */
    {  145, 0x045d, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7884 */
    {  146, 0x045e, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7888 */
    {  147, 0x045f, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a788c */
    {  148, 0x0460, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7890 */
    {  149, 0x0786, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7894 */
    {  152, 0x0461, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78a0 */
    {  156, 0x0462, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78b0 */
    {  160, 0x0463, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78c0 */
    {  169, 0x08b5, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78e4 */
    {  171, 0x06bb, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78ec */
    {  174, 0x06bc, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78f8 */
    {  177, 0x06bd, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7904 */
    {  180, 0x06d7, 0x50  , 0x80  , 0x0       , 1, 2 },  /* 0x008a7910 */
    {  183, 0x0707, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a791c */
    {  185, 0x0708, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7924 */
    {  186, 0x0709, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7928 */
    {  188, 0x077a, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7930 */
    {  189, 0x077b, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7934 */
    {  190, 0x077c, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7938 */
    {  163, 0x0464, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78cc */
    {  166, 0x0465, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78d8 */
    {  167, 0x0466, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78dc */
    {  168, 0x0467, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a78e0 */
    {  191, 0x0468, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a793c */
    {  194, 0x0469, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7948 */
    {  195, 0x046a, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a794c */
    {  197, 0x06a5, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7954 */
    {  199, 0x046b, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a795c */
    {  201, 0x046c, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7964 */
    {  202, 0x046d, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7968 */
    {  204, 0x046e, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7970 */
    {  206, 0x046f, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a7978 */
    {  207, 0x0470, 0x50  , 0x50  , 0x0       , 1, 2 },  /* 0x008a797c */
    {  208, 0x0471, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7980 */
    {  212, 0x0472, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7990 */
    {  214, 0x0473, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a7998 */
    {  215, 0x0474, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a799c */
    {  216, 0x0475, 0x50  , 0x80  , 0x0       , 1, 2 },  /* 0x008a79a0 */
    {  219, 0x0476, 0x50  , 0x80  , 0x0       , 1, 2 },  /* 0x008a79ac */
    {  222, 0x0477, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79b8 */
    {  225, 0x0478, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79c4 */
    {  227, 0x0479, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79cc */
    {  229, 0x047a, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79d4 */
    {  232, 0x0784, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79e0 */
    {  236, 0x047b, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a79f0 */
    {  187, 0x070b, 0x50  , 0x60  , 0x0       , 1, 2 },  /* 0x008a792c */
};
#define GROUP3_SPRITES_COUNT  233

/* ─── FUN_0057ca40 — info-entry pool writes (4th pass) ────────────
 *
 * 443 info-entry events in retail issue order — the
 * `g_ar_info_table[i]` writes that follow each sprite slot in
 * FUN_0057ca40.  See `docs/findings/0057ca40-rabbit-hole.md` §2.
 *
 * Indexing: pool[i] lives at retail BSS `0x8a8440 + i*4`.
 *
 * Source order preserved (MARKER_COPY/FLAG_COPY/STRUCT_COPY events
 * depend on prior writes — walking in source order keeps the
 * copies' source pool entries already-written).
 *
 * DATA_SET payloads are retail PE rdata addresses (e.g.
 * 0x006752f8); the port stores them as opaque uintptr_t markers
 * — no consumer reads them as bytes yet.  When a consumer lands
 * (FUN_00586010-style palette draw with flag dispatch), these will
 * need extracted PE bytes; for now they're observability-only. */

enum ar_info_event_kind {
    AR_INFO_EVT_MARKER_SET = 0,
    AR_INFO_EVT_FLAG_SET,
    AR_INFO_EVT_DATA_SET,
    AR_INFO_EVT_MARKER_COPY,
    AR_INFO_EVT_FLAG_COPY,
    AR_INFO_EVT_STRUCT_COPY,
};

struct ar_info_event {
    uint8_t   kind;
    uint16_t  dst_idx;     /* destination pool index */
    uintptr_t payload;     /* SET: literal value | COPY/STRUCT_COPY: src pool index */
};

static const struct ar_info_event group3_info_events[] = {
    /* kind                       dst   payload */
    { AR_INFO_EVT_FLAG_SET        ,   92,         0x1u },  /* L40 */
    { AR_INFO_EVT_FLAG_SET        ,   93,         0x1u },  /* L67 */
    { AR_INFO_EVT_FLAG_SET        ,   98,         0x1u },  /* L94 */
    { AR_INFO_EVT_FLAG_SET        ,   99,         0x1u },  /* L121 */
    { AR_INFO_EVT_FLAG_SET        ,   94,         0x1u },  /* L148 */
    { AR_INFO_EVT_FLAG_SET        ,   95,         0x1u },  /* L175 */
    { AR_INFO_EVT_FLAG_SET        ,   96,         0x1u },  /* L202 */
    { AR_INFO_EVT_MARKER_SET      ,   97,         0x0u },  /* L229 */
    { AR_INFO_EVT_FLAG_SET        ,   97,         0x1u },  /* L230 */
    { AR_INFO_EVT_FLAG_SET        ,  100,         0x1u },  /* L257 */
    { AR_INFO_EVT_FLAG_SET        ,  101,         0x1u },  /* L284 */
    { AR_INFO_EVT_FLAG_SET        ,  102,         0x1u },  /* L311 */
    { AR_INFO_EVT_FLAG_SET        ,  103,         0x1u },  /* L338 */
    { AR_INFO_EVT_FLAG_SET        ,  105,         0x1u },  /* L365 */
    { AR_INFO_EVT_FLAG_SET        ,  104,         0x1u },  /* L392 */
    { AR_INFO_EVT_FLAG_SET        ,  106,         0x1u },  /* L419 */
    { AR_INFO_EVT_FLAG_SET        ,  112,         0x1u },  /* L446 */
    { AR_INFO_EVT_FLAG_SET        ,  113,         0x1u },  /* L473 */
    { AR_INFO_EVT_FLAG_SET        ,  114,         0x1u },  /* L552 */
    { AR_INFO_EVT_FLAG_SET        ,  115,         0x1u },  /* L579 */
    { AR_INFO_EVT_FLAG_SET        ,  116,         0x1u },  /* L606 */
    { AR_INFO_EVT_FLAG_SET        ,  117,         0x1u },  /* L633 */
    { AR_INFO_EVT_FLAG_SET        ,  118,         0x1u },  /* L660 */
    { AR_INFO_EVT_FLAG_SET        ,  119,         0x1u },  /* L687 */
    { AR_INFO_EVT_FLAG_SET        ,  120,         0x1u },  /* L792 */
    { AR_INFO_EVT_FLAG_SET        ,  121,         0x1u },  /* L819 */
    { AR_INFO_EVT_FLAG_SET        ,  122,         0x1u },  /* L846 */
    { AR_INFO_EVT_FLAG_SET        ,  123,         0x1u },  /* L873 */
    { AR_INFO_EVT_FLAG_SET        ,  124,         0x1u },  /* L900 */
    { AR_INFO_EVT_FLAG_SET        ,  125,         0x1u },  /* L927 */
    { AR_INFO_EVT_FLAG_SET        ,  126,         0x1u },  /* L954 */
    { AR_INFO_EVT_FLAG_SET        ,  131,         0x1u },  /* L1085 */
    { AR_INFO_EVT_FLAG_SET        ,  132,         0x1u },  /* L1112 */
    { AR_INFO_EVT_FLAG_SET        ,  133,         0x1u },  /* L1139 */
    { AR_INFO_EVT_FLAG_SET        ,  134,         0x1u },  /* L1166 */
    { AR_INFO_EVT_FLAG_SET        ,  135,         0x1u },  /* L1193 */
    { AR_INFO_EVT_FLAG_SET        ,  136,         0x1u },  /* L1220 */
    { AR_INFO_EVT_FLAG_SET        ,  137,         0x1u },  /* L1247 */
    { AR_INFO_EVT_FLAG_SET        ,  138,         0x1u },  /* L1274 */
    { AR_INFO_EVT_FLAG_SET        ,  351,         0x1u },  /* L1301 */
    { AR_INFO_EVT_FLAG_SET        ,  352,         0x1u },  /* L1328 */
    { AR_INFO_EVT_FLAG_SET        ,  353,         0x1u },  /* L1355 */
    { AR_INFO_EVT_FLAG_SET        ,  354,         0x1u },  /* L1382 */
    { AR_INFO_EVT_FLAG_SET        ,  355,         0x1u },  /* L1409 */
    { AR_INFO_EVT_FLAG_SET        ,  357,         0x1u },  /* L1436 */
    { AR_INFO_EVT_FLAG_SET        ,  358,         0x1u },  /* L1463 */
    { AR_INFO_EVT_FLAG_SET        ,  356,         0x1u },  /* L1490 */
    { AR_INFO_EVT_FLAG_SET        ,  370,         0x1u },  /* L1517 */
    { AR_INFO_EVT_FLAG_SET        ,  371,         0x1u },  /* L1544 */
    { AR_INFO_EVT_FLAG_SET        ,  372,         0x1u },  /* L1571 */
    { AR_INFO_EVT_FLAG_SET        ,  373,         0x1u },  /* L1598 */
    { AR_INFO_EVT_FLAG_SET        ,  379,         0x2u },  /* L1625 */
    { AR_INFO_EVT_FLAG_SET        ,  364,         0x2u },  /* L1652 */
    { AR_INFO_EVT_FLAG_SET        ,  365,         0x2u },  /* L1679 */
    { AR_INFO_EVT_FLAG_SET        ,  366,         0x2u },  /* L1706 */
    { AR_INFO_EVT_FLAG_SET        ,  367,         0x2u },  /* L1733 */
    { AR_INFO_EVT_FLAG_SET        ,  368,         0x2u },  /* L1760 */
    { AR_INFO_EVT_FLAG_SET        ,  369,         0x2u },  /* L1787 */
    { AR_INFO_EVT_FLAG_SET        ,  363,         0x2u },  /* L1814 */
    { AR_INFO_EVT_FLAG_SET        ,  374,         0x1u },  /* L1841 */
    { AR_INFO_EVT_FLAG_SET        ,  375,         0x1u },  /* L1868 */
    { AR_INFO_EVT_FLAG_SET        ,  376,         0x1u },  /* L1895 */
    { AR_INFO_EVT_FLAG_SET        ,  377,         0x1u },  /* L1922 */
    { AR_INFO_EVT_FLAG_SET        ,  378,         0x1u },  /* L1949 */
    { AR_INFO_EVT_FLAG_SET        ,  380,         0x2u },  /* L1976 */
    { AR_INFO_EVT_FLAG_SET        ,  381,         0x2u },  /* L2003 */
    { AR_INFO_EVT_FLAG_SET        ,  382,         0x2u },  /* L2030 */
    { AR_INFO_EVT_FLAG_SET        ,  397,         0x1u },  /* L2057 */
    { AR_INFO_EVT_FLAG_SET        ,  398,         0x1u },  /* L2084 */
    { AR_INFO_EVT_FLAG_SET        ,  399,         0x1u },  /* L2111 */
    { AR_INFO_EVT_MARKER_COPY     ,  384,         381u },  /* L2140 */
    { AR_INFO_EVT_FLAG_COPY       ,  384,         381u },  /* L2141 */
    { AR_INFO_EVT_DATA_SET        ,  384,  0x006752f8u },  /* L2142 */
    { AR_INFO_EVT_MARKER_COPY     ,  385,         382u },  /* L2145 */
    { AR_INFO_EVT_FLAG_COPY       ,  385,         382u },  /* L2146 */
    { AR_INFO_EVT_DATA_SET        ,  385,  0x006752f8u },  /* L2147 */
    { AR_INFO_EVT_FLAG_SET        ,  386,         0x2u },  /* L2174 */
    { AR_INFO_EVT_FLAG_SET        ,  387,         0x2u },  /* L2201 */
    { AR_INFO_EVT_FLAG_SET        ,  388,         0x2u },  /* L2228 */
    { AR_INFO_EVT_FLAG_SET        ,  389,         0x2u },  /* L2255 */
    { AR_INFO_EVT_MARKER_COPY     ,  391,         388u },  /* L2284 */
    { AR_INFO_EVT_FLAG_COPY       ,  391,         388u },  /* L2285 */
    { AR_INFO_EVT_DATA_SET        ,  391,  0x00675500u },  /* L2286 */
    { AR_INFO_EVT_MARKER_COPY     ,  392,         389u },  /* L2289 */
    { AR_INFO_EVT_FLAG_COPY       ,  392,         389u },  /* L2290 */
    { AR_INFO_EVT_DATA_SET        ,  392,  0x00675500u },  /* L2291 */
    { AR_INFO_EVT_FLAG_SET        ,  393,         0x1u },  /* L2318 */
    { AR_INFO_EVT_FLAG_SET        ,  394,         0x1u },  /* L2345 */
    { AR_INFO_EVT_FLAG_SET        ,  395,         0x1u },  /* L2372 */
    { AR_INFO_EVT_FLAG_SET        ,  396,         0x1u },  /* L2399 */
    { AR_INFO_EVT_FLAG_SET        ,  400,         0x1u },  /* L2426 */
    { AR_INFO_EVT_FLAG_SET        ,  401,         0x1u },  /* L2453 */
    { AR_INFO_EVT_FLAG_SET        ,  402,         0x1u },  /* L2480 */
    { AR_INFO_EVT_FLAG_SET        ,  403,         0x2u },  /* L2482 */
    { AR_INFO_EVT_FLAG_SET        ,  404,         0x2u },  /* L2484 */
    { AR_INFO_EVT_FLAG_SET        ,  405,         0x2u },  /* L2486 */
    { AR_INFO_EVT_FLAG_SET        ,  406,         0x2u },  /* L2488 */
    { AR_INFO_EVT_FLAG_SET        ,  291,         0x2u },  /* L2490 */
    { AR_INFO_EVT_MARKER_SET      ,  291,        0x1cu },  /* L2491 */
    { AR_INFO_EVT_DATA_SET        ,  292,  0x006748d0u },  /* L2493 */
    { AR_INFO_EVT_DATA_SET        ,  293,  0x00674ad8u },  /* L2495 */
    { AR_INFO_EVT_FLAG_SET        ,  294,         0x2u },  /* L2497 */
    { AR_INFO_EVT_MARKER_SET      ,  294,        0x18u },  /* L2498 */
    { AR_INFO_EVT_DATA_SET        ,  295,  0x00672e68u },  /* L2500 */
    { AR_INFO_EVT_DATA_SET        ,  296,  0x00673070u },  /* L2502 */
    { AR_INFO_EVT_FLAG_SET        ,  288,         0x2u },  /* L2504 */
    { AR_INFO_EVT_MARKER_SET      ,  288,         0x8u },  /* L2505 */
    { AR_INFO_EVT_DATA_SET        ,  289,  0x00672e68u },  /* L2507 */
    { AR_INFO_EVT_DATA_SET        ,  290,  0x00673070u },  /* L2509 */
    { AR_INFO_EVT_FLAG_SET        ,  297,         0x2u },  /* L2511 */
    { AR_INFO_EVT_MARKER_SET      ,  297,        0x10u },  /* L2512 */
    { AR_INFO_EVT_DATA_SET        ,  298,  0x006748d0u },  /* L2514 */
    { AR_INFO_EVT_FLAG_SET        ,  299,         0x2u },  /* L2516 */
    { AR_INFO_EVT_MARKER_SET      ,  299,        0x28u },  /* L2517 */
    { AR_INFO_EVT_DATA_SET        ,  300,  0x006740b0u },  /* L2519 */
    { AR_INFO_EVT_FLAG_SET        ,  301,         0x2u },  /* L2521 */
    { AR_INFO_EVT_MARKER_SET      ,  301,        0x28u },  /* L2522 */
    { AR_INFO_EVT_DATA_SET        ,  302,  0x006740b0u },  /* L2524 */
    { AR_INFO_EVT_FLAG_SET        ,  303,         0x2u },  /* L2526 */
    { AR_INFO_EVT_MARKER_SET      ,  303,        0x20u },  /* L2527 */
    { AR_INFO_EVT_FLAG_SET        ,  309,         0x2u },  /* L2529 */
    { AR_INFO_EVT_MARKER_SET      ,  309,        0x1cu },  /* L2530 */
    { AR_INFO_EVT_DATA_SET        ,  311,  0x006742b8u },  /* L2532 */
    { AR_INFO_EVT_DATA_SET        ,  312,  0x006744c0u },  /* L2534 */
    { AR_INFO_EVT_DATA_SET        ,  313,  0x006746c8u },  /* L2537 */
    { AR_INFO_EVT_FLAG_SET        ,  318,         0x2u },  /* L2539 */
    { AR_INFO_EVT_MARKER_SET      ,  318,        0x50u },  /* L2540 */
    { AR_INFO_EVT_FLAG_SET        ,  304,         0x2u },  /* L2542 */
    { AR_INFO_EVT_MARKER_SET      ,  304,         0x0u },  /* L2543 */
    { AR_INFO_EVT_FLAG_SET        ,  305,         0x2u },  /* L2545 */
    { AR_INFO_EVT_MARKER_SET      ,  305,         0x0u },  /* L2546 */
    { AR_INFO_EVT_FLAG_SET        ,  306,         0x2u },  /* L2548 */
    { AR_INFO_EVT_MARKER_SET      ,  306,         0x0u },  /* L2549 */
    { AR_INFO_EVT_FLAG_SET        ,  307,         0x2u },  /* L2551 */
    { AR_INFO_EVT_MARKER_SET      ,  307,         0x0u },  /* L2552 */
    { AR_INFO_EVT_FLAG_SET        ,  308,         0x2u },  /* L2554 */
    { AR_INFO_EVT_MARKER_SET      ,  308,         0x0u },  /* L2555 */
    { AR_INFO_EVT_FLAG_SET        ,  321,         0x2u },  /* L2557 */
    { AR_INFO_EVT_MARKER_SET      ,  321,         0x0u },  /* L2558 */
    { AR_INFO_EVT_FLAG_SET        ,  319,         0x2u },  /* L2560 */
    { AR_INFO_EVT_MARKER_SET      ,  319,        0x20u },  /* L2561 */
    { AR_INFO_EVT_DATA_SET        ,  320,  0x006748d0u },  /* L2563 */
    { AR_INFO_EVT_FLAG_SET        ,  263,         0x2u },  /* L2565 */
    { AR_INFO_EVT_MARKER_SET      ,  263,        0x20u },  /* L2566 */
    { AR_INFO_EVT_FLAG_SET        ,  262,         0x2u },  /* L2568 */
    { AR_INFO_EVT_MARKER_SET      ,  262,        0x20u },  /* L2569 */
    { AR_INFO_EVT_FLAG_SET        ,  264,         0x2u },  /* L2571 */
    { AR_INFO_EVT_MARKER_SET      ,  264,        0x1cu },  /* L2572 */
    { AR_INFO_EVT_DATA_SET        ,  265,  0x006748d0u },  /* L2574 */
    { AR_INFO_EVT_FLAG_SET        ,  266,         0x2u },  /* L2576 */
    { AR_INFO_EVT_MARKER_SET      ,  266,        0x20u },  /* L2577 */
    { AR_INFO_EVT_DATA_SET        ,  267,  0x006748d0u },  /* L2579 */
    { AR_INFO_EVT_FLAG_SET        ,  322,         0x2u },  /* L2581 */
    { AR_INFO_EVT_MARKER_SET      ,  322,        0x24u },  /* L2582 */
    { AR_INFO_EVT_DATA_SET        ,  323,  0x006748d0u },  /* L2584 */
    { AR_INFO_EVT_DATA_SET        ,  324,  0x00674ad8u },  /* L2586 */
    { AR_INFO_EVT_DATA_SET        ,  325,  0x00674ce0u },  /* L2588 */
    { AR_INFO_EVT_FLAG_SET        ,  316,         0x2u },  /* L2590 */
    { AR_INFO_EVT_MARKER_SET      ,  316,        0x20u },  /* L2591 */
    { AR_INFO_EVT_DATA_SET        ,  317,  0x00673278u },  /* L2593 */
    { AR_INFO_EVT_FLAG_SET        ,  314,         0x2u },  /* L2595 */
    { AR_INFO_EVT_MARKER_SET      ,  314,        0x28u },  /* L2596 */
    { AR_INFO_EVT_DATA_SET        ,  315,  0x00673480u },  /* L2598 */
    { AR_INFO_EVT_FLAG_SET        ,  283,         0x2u },  /* L2600 */
    { AR_INFO_EVT_MARKER_SET      ,  283,        0x18u },  /* L2601 */
    { AR_INFO_EVT_DATA_SET        ,  284,  0x00673a98u },  /* L2603 */
    { AR_INFO_EVT_DATA_SET        ,  285,  0x00673ca0u },  /* L2605 */
    { AR_INFO_EVT_FLAG_SET        ,  286,         0x2u },  /* L2607 */
    { AR_INFO_EVT_MARKER_SET      ,  286,        0x18u },  /* L2608 */
    { AR_INFO_EVT_DATA_SET        ,  287,  0x00673ea8u },  /* L2610 */
    { AR_INFO_EVT_FLAG_SET        ,  268,         0x2u },  /* L2612 */
    { AR_INFO_EVT_MARKER_SET      ,  268,        0x18u },  /* L2613 */
    { AR_INFO_EVT_FLAG_SET        ,  269,         0x2u },  /* L2615 */
    { AR_INFO_EVT_MARKER_SET      ,  269,         0x8u },  /* L2616 */
    { AR_INFO_EVT_DATA_SET        ,  270,  0x00672440u },  /* L2618 */
    { AR_INFO_EVT_DATA_SET        ,  271,  0x00672648u },  /* L2620 */
    { AR_INFO_EVT_DATA_SET        ,  272,  0x00672648u },  /* L2622 */
    { AR_INFO_EVT_DATA_SET        ,  273,  0x00672850u },  /* L2624 */
    { AR_INFO_EVT_DATA_SET        ,  274,  0x00672850u },  /* L2626 */
    { AR_INFO_EVT_DATA_SET        ,  275,  0x00672a58u },  /* L2628 */
    { AR_INFO_EVT_DATA_SET        ,  276,  0x00672a58u },  /* L2630 */
    { AR_INFO_EVT_DATA_SET        ,  277,  0x00672c60u },  /* L2632 */
    { AR_INFO_EVT_DATA_SET        ,  278,  0x00672c60u },  /* L2634 */
    { AR_INFO_EVT_MARKER_SET      ,  279,        0x20u },  /* L2636 */
    { AR_INFO_EVT_DATA_SET        ,  280,  0x00673688u },  /* L2638 */
    { AR_INFO_EVT_DATA_SET        ,  281,  0x00673890u },  /* L2640 */
    { AR_INFO_EVT_MARKER_SET      ,  282,         0xcu },  /* L2642 */
    { AR_INFO_EVT_FLAG_SET        ,  326,         0x2u },  /* L2644 */
    { AR_INFO_EVT_MARKER_SET      ,  326,        0x10u },  /* L2645 */
    { AR_INFO_EVT_FLAG_SET        ,  327,         0x2u },  /* L2647 */
    { AR_INFO_EVT_MARKER_SET      ,  327,         0x4u },  /* L2648 */
    { AR_INFO_EVT_FLAG_SET        ,  328,         0x2u },  /* L2650 */
    { AR_INFO_EVT_MARKER_SET      ,  328,         0x4u },  /* L2651 */
    { AR_INFO_EVT_FLAG_SET        ,  329,         0x2u },  /* L2653 */
    { AR_INFO_EVT_MARKER_SET      ,  329,         0x4u },  /* L2654 */
    { AR_INFO_EVT_DATA_SET        ,  330,  0x006748d0u },  /* L2656 */
    { AR_INFO_EVT_DATA_SET        ,  331,  0x00674ad8u },  /* L2658 */
    { AR_INFO_EVT_FLAG_SET        ,  332,         0x2u },  /* L2660 */
    { AR_INFO_EVT_MARKER_SET      ,  332,        0x10u },  /* L2661 */
    { AR_INFO_EVT_DATA_SET        ,  333,  0x006748d0u },  /* L2663 */
    { AR_INFO_EVT_FLAG_SET        ,  334,         0x2u },  /* L2665 */
    { AR_INFO_EVT_MARKER_SET      ,  334,         0x5u },  /* L2666 */
    { AR_INFO_EVT_DATA_SET        ,  335,  0x006748d0u },  /* L2668 */
    { AR_INFO_EVT_FLAG_SET        ,  342,         0x2u },  /* L2675 */
    { AR_INFO_EVT_MARKER_SET      ,  342,         0x0u },  /* L2676 */
    { AR_INFO_EVT_FLAG_SET        ,  347,         0x2u },  /* L2678 */
    { AR_INFO_EVT_MARKER_SET      ,  347,         0x0u },  /* L2679 */
    { AR_INFO_EVT_FLAG_SET        ,  348,         0x2u },  /* L2681 */
    { AR_INFO_EVT_MARKER_SET      ,  348,         0x0u },  /* L2682 */
    { AR_INFO_EVT_FLAG_SET        ,  349,         0x2u },  /* L2684 */
    { AR_INFO_EVT_MARKER_SET      ,  349,         0x0u },  /* L2685 */
    { AR_INFO_EVT_FLAG_SET        ,  350,         0x2u },  /* L2687 */
    { AR_INFO_EVT_MARKER_SET      ,  350,         0x0u },  /* L2688 */
    { AR_INFO_EVT_FLAG_SET        ,  435,         0x2u },  /* L2690 */
    { AR_INFO_EVT_MARKER_SET      ,  435,         0x0u },  /* L2691 */
    { AR_INFO_EVT_FLAG_SET        ,  436,         0x2u },  /* L2693 */
    { AR_INFO_EVT_MARKER_SET      ,  436,         0x0u },  /* L2694 */
    { AR_INFO_EVT_FLAG_SET        ,  437,         0x2u },  /* L2696 */
    { AR_INFO_EVT_MARKER_SET      ,  437,         0x0u },  /* L2697 */
    { AR_INFO_EVT_FLAG_SET        ,  359,         0x2u },  /* L2699 */
    { AR_INFO_EVT_MARKER_SET      ,  359,         0x0u },  /* L2700 */
    { AR_INFO_EVT_FLAG_SET        ,  360,         0x2u },  /* L2702 */
    { AR_INFO_EVT_MARKER_SET      ,  360,         0x0u },  /* L2703 */
    { AR_INFO_EVT_FLAG_SET        ,  361,         0x2u },  /* L2705 */
    { AR_INFO_EVT_MARKER_SET      ,  361,         0x0u },  /* L2706 */
    { AR_INFO_EVT_FLAG_SET        ,  362,         0x2u },  /* L2708 */
    { AR_INFO_EVT_MARKER_SET      ,  362,         0x0u },  /* L2709 */
    { AR_INFO_EVT_FLAG_SET        ,  345,         0x2u },  /* L2711 */
    { AR_INFO_EVT_MARKER_SET      ,  345,         0x0u },  /* L2712 */
    { AR_INFO_EVT_FLAG_SET        ,  346,         0x2u },  /* L2714 */
    { AR_INFO_EVT_MARKER_SET      ,  346,         0x0u },  /* L2715 */
    { AR_INFO_EVT_FLAG_SET        ,  343,         0x2u },  /* L2717 */
    { AR_INFO_EVT_MARKER_SET      ,  343,         0x0u },  /* L2718 */
    { AR_INFO_EVT_FLAG_SET        ,  344,         0x2u },  /* L2720 */
    { AR_INFO_EVT_MARKER_SET      ,  344,         0x0u },  /* L2721 */
    { AR_INFO_EVT_MARKER_SET      ,  407,         0x1u },  /* L2723 */
    { AR_INFO_EVT_MARKER_SET      ,  408,         0x8u },  /* L2725 */
    { AR_INFO_EVT_MARKER_SET      ,  409,         0x0u },  /* L2727 */
    { AR_INFO_EVT_MARKER_SET      ,  410,        0x18u },  /* L2729 */
    { AR_INFO_EVT_MARKER_SET      ,  411,        0x28u },  /* L2731 */
    { AR_INFO_EVT_MARKER_SET      ,  413,         0x0u },  /* L2733 */
    { AR_INFO_EVT_MARKER_SET      ,  412,         0x0u },  /* L2735 */
    { AR_INFO_EVT_MARKER_SET      ,  414,         0x0u },  /* L2737 */
    { AR_INFO_EVT_MARKER_SET      ,  415,         0xcu },  /* L2739 */
    { AR_INFO_EVT_MARKER_SET      ,  416,         0x0u },  /* L2741 */
    { AR_INFO_EVT_MARKER_SET      ,  417,         0x0u },  /* L2743 */
    { AR_INFO_EVT_MARKER_SET      ,  418,         0x0u },  /* L2745 */
    { AR_INFO_EVT_MARKER_SET      ,  419,         0x0u },  /* L2747 */
    { AR_INFO_EVT_MARKER_SET      ,  420,         0x0u },  /* L2749 */
    { AR_INFO_EVT_MARKER_SET      ,  421,         0x0u },  /* L2751 */
    { AR_INFO_EVT_MARKER_SET      ,  423,         0x0u },  /* L2753 */
    { AR_INFO_EVT_MARKER_SET      ,  422,         0x0u },  /* L2755 */
    { AR_INFO_EVT_MARKER_SET      ,  425,         0x0u },  /* L2757 */
    { AR_INFO_EVT_MARKER_SET      ,  424,         0x0u },  /* L2759 */
    { AR_INFO_EVT_MARKER_SET      ,  426,         0x0u },  /* L2761 */
    { AR_INFO_EVT_FLAG_SET        ,  426,         0x1u },  /* L2762 */
    { AR_INFO_EVT_MARKER_SET      ,  427,         0x0u },  /* L2764 */
    { AR_INFO_EVT_FLAG_SET        ,  427,         0x2u },  /* L2765 */
    { AR_INFO_EVT_MARKER_SET      ,  428,         0x0u },  /* L2767 */
    { AR_INFO_EVT_MARKER_SET      ,  429,         0x0u },  /* L2769 */
    { AR_INFO_EVT_MARKER_SET      ,  430,         0x0u },  /* L2771 */
    { AR_INFO_EVT_MARKER_SET      ,  431,         0xcu },  /* L2773 */
    { AR_INFO_EVT_MARKER_SET      ,  432,         0x0u },  /* L2775 */
    { AR_INFO_EVT_MARKER_SET      ,  433,         0x1u },  /* L2777 */
    { AR_INFO_EVT_FLAG_SET        ,  142,         0x2u },  /* L2788 */
    { AR_INFO_EVT_MARKER_SET      ,  142,        0x10u },  /* L2789 */
    { AR_INFO_EVT_FLAG_SET        ,  144,         0x2u },  /* L2794 */
    { AR_INFO_EVT_MARKER_SET      ,  144,        0x10u },  /* L2795 */
    { AR_INFO_EVT_FLAG_SET        ,  146,         0x2u },  /* L2800 */
    { AR_INFO_EVT_MARKER_SET      ,  146,        0x10u },  /* L2801 */
    { AR_INFO_EVT_FLAG_SET        ,  336,         0x2u },  /* L2803 */
    { AR_INFO_EVT_MARKER_SET      ,  336,        0x20u },  /* L2804 */
    { AR_INFO_EVT_FLAG_SET        ,  434,         0x2u },  /* L2806 */
    { AR_INFO_EVT_MARKER_SET      ,  434,         0x0u },  /* L2807 */
    { AR_INFO_EVT_FLAG_SET        ,  253,         0x2u },  /* L2809 */
    { AR_INFO_EVT_MARKER_SET      ,  253,         0x1u },  /* L2810 */
    { AR_INFO_EVT_FLAG_SET        ,  248,         0x2u },  /* L2812 */
    { AR_INFO_EVT_MARKER_SET      ,  248,         0x4u },  /* L2813 */
    { AR_INFO_EVT_FLAG_SET        ,  254,         0x2u },  /* L2815 */
    { AR_INFO_EVT_MARKER_SET      ,  254,         0x8u },  /* L2816 */
    { AR_INFO_EVT_DATA_SET        ,  255,  0x00674ee8u },  /* L2818 */
    { AR_INFO_EVT_DATA_SET        ,  256,  0x006750f0u },  /* L2820 */
    { AR_INFO_EVT_FLAG_SET        ,  147,         0x2u },  /* L2822 */
    { AR_INFO_EVT_MARKER_SET      ,  147,        0x10u },  /* L2823 */
    { AR_INFO_EVT_DATA_SET        ,  148,  0x006748d0u },  /* L2825 */
    { AR_INFO_EVT_FLAG_SET        ,  149,         0x2u },  /* L2827 */
    { AR_INFO_EVT_MARKER_SET      ,  149,        0x10u },  /* L2828 */
    { AR_INFO_EVT_DATA_SET        ,  150,  0x006748d0u },  /* L2830 */
    { AR_INFO_EVT_FLAG_SET        ,  151,         0x2u },  /* L2832 */
    { AR_INFO_EVT_MARKER_SET      ,  151,        0x10u },  /* L2833 */
    { AR_INFO_EVT_DATA_SET        ,  152,  0x006748d0u },  /* L2835 */
    { AR_INFO_EVT_FLAG_SET        ,  153,         0x2u },  /* L2837 */
    { AR_INFO_EVT_MARKER_SET      ,  153,        0x10u },  /* L2838 */
    { AR_INFO_EVT_DATA_SET        ,  154,  0x006748d0u },  /* L2840 */
    { AR_INFO_EVT_DATA_SET        ,  155,  0x00674ad8u },  /* L2842 */
    { AR_INFO_EVT_FLAG_SET        ,  156,         0x2u },  /* L2844 */
    { AR_INFO_EVT_MARKER_SET      ,  156,        0x10u },  /* L2845 */
    { AR_INFO_EVT_DATA_SET        ,  157,  0x006748d0u },  /* L2847 */
    { AR_INFO_EVT_FLAG_SET        ,  158,         0x2u },  /* L2849 */
    { AR_INFO_EVT_MARKER_SET      ,  158,        0x10u },  /* L2850 */
    { AR_INFO_EVT_FLAG_SET        ,  159,         0x2u },  /* L2852 */
    { AR_INFO_EVT_MARKER_SET      ,  159,        0x10u },  /* L2853 */
    { AR_INFO_EVT_FLAG_SET        ,  160,         0x2u },  /* L2855 */
    { AR_INFO_EVT_MARKER_SET      ,  160,        0x10u },  /* L2856 */
    { AR_INFO_EVT_FLAG_SET        ,  161,         0x2u },  /* L2858 */
    { AR_INFO_EVT_MARKER_SET      ,  161,        0x10u },  /* L2859 */
    { AR_INFO_EVT_FLAG_SET        ,  162,         0x2u },  /* L2861 */
    { AR_INFO_EVT_MARKER_SET      ,  162,        0x10u },  /* L2862 */
    { AR_INFO_EVT_DATA_SET        ,  163,  0x006748d0u },  /* L2864 */
    { AR_INFO_EVT_DATA_SET        ,  164,  0x00674ad8u },  /* L2866 */
    { AR_INFO_EVT_FLAG_SET        ,  165,         0x2u },  /* L2868 */
    { AR_INFO_EVT_MARKER_SET      ,  165,        0x10u },  /* L2869 */
    { AR_INFO_EVT_DATA_SET        ,  166,  0x006748d0u },  /* L2871 */
    { AR_INFO_EVT_DATA_SET        ,  167,  0x00674ad8u },  /* L2873 */
    { AR_INFO_EVT_DATA_SET        ,  168,  0x00674ce0u },  /* L2875 */
    { AR_INFO_EVT_FLAG_SET        ,  169,         0x2u },  /* L2877 */
    { AR_INFO_EVT_MARKER_SET      ,  169,        0x10u },  /* L2878 */
    { AR_INFO_EVT_DATA_SET        ,  170,  0x006748d0u },  /* L2880 */
    { AR_INFO_EVT_DATA_SET        ,  171,  0x00674ad8u },  /* L2882 */
    { AR_INFO_EVT_DATA_SET        ,  172,  0x00674ce0u },  /* L2884 */
    { AR_INFO_EVT_FLAG_SET        ,  173,         0x2u },  /* L2886 */
    { AR_INFO_EVT_MARKER_SET      ,  173,        0x10u },  /* L2887 */
    { AR_INFO_EVT_DATA_SET        ,  174,  0x006748d0u },  /* L2889 */
    { AR_INFO_EVT_DATA_SET        ,  175,  0x00674ad8u },  /* L2891 */
    { AR_INFO_EVT_FLAG_SET        ,  182,         0x2u },  /* L2893 */
    { AR_INFO_EVT_MARKER_SET      ,  182,         0x4u },  /* L2894 */
    { AR_INFO_EVT_DATA_SET        ,  183,  0x006748d0u },  /* L2896 */
    { AR_INFO_EVT_FLAG_SET        ,  184,         0x2u },  /* L2898 */
    { AR_INFO_EVT_MARKER_SET      ,  184,         0x5u },  /* L2899 */
    { AR_INFO_EVT_DATA_SET        ,  185,  0x006748d0u },  /* L2901 */
    { AR_INFO_EVT_DATA_SET        ,  186,  0x00674ad8u },  /* L2903 */
    { AR_INFO_EVT_FLAG_SET        ,  187,         0x2u },  /* L2905 */
    { AR_INFO_EVT_MARKER_SET      ,  187,         0x5u },  /* L2906 */
    { AR_INFO_EVT_DATA_SET        ,  188,  0x006748d0u },  /* L2908 */
    { AR_INFO_EVT_DATA_SET        ,  189,  0x00674ad8u },  /* L2910 */
    { AR_INFO_EVT_FLAG_SET        ,  190,         0x2u },  /* L2912 */
    { AR_INFO_EVT_MARKER_SET      ,  190,         0x5u },  /* L2913 */
    { AR_INFO_EVT_DATA_SET        ,  191,  0x006748d0u },  /* L2915 */
    { AR_INFO_EVT_DATA_SET        ,  192,  0x00674ad8u },  /* L2917 */
    { AR_INFO_EVT_FLAG_SET        ,  193,         0x2u },  /* L2919 */
    { AR_INFO_EVT_MARKER_SET      ,  193,         0x4u },  /* L2920 */
    { AR_INFO_EVT_DATA_SET        ,  194,  0x006748d0u },  /* L2922 */
    { AR_INFO_EVT_DATA_SET        ,  195,  0x00674ad8u },  /* L2924 */
    { AR_INFO_EVT_FLAG_SET        ,  196,         0x2u },  /* L2926 */
    { AR_INFO_EVT_MARKER_SET      ,  196,         0x4u },  /* L2927 */
    { AR_INFO_EVT_DATA_SET        ,  197,  0x006748d0u },  /* L2929 */
    { AR_INFO_EVT_FLAG_SET        ,  198,         0x2u },  /* L2931 */
    { AR_INFO_EVT_MARKER_SET      ,  198,         0x4u },  /* L2932 */
    { AR_INFO_EVT_FLAG_SET        ,  199,         0x2u },  /* L2934 */
    { AR_INFO_EVT_MARKER_SET      ,  199,         0x4u },  /* L2935 */
    { AR_INFO_EVT_FLAG_SET        ,  201,         0x2u },  /* L2937 */
    { AR_INFO_EVT_MARKER_SET      ,  201,         0x4u },  /* L2938 */
    { AR_INFO_EVT_FLAG_SET        ,  202,         0x2u },  /* L2940 */
    { AR_INFO_EVT_MARKER_SET      ,  202,         0x4u },  /* L2941 */
    { AR_INFO_EVT_FLAG_SET        ,  203,         0x2u },  /* L2943 */
    { AR_INFO_EVT_MARKER_SET      ,  203,         0x4u },  /* L2944 */
    { AR_INFO_EVT_FLAG_SET        ,  176,         0x2u },  /* L2946 */
    { AR_INFO_EVT_MARKER_SET      ,  176,         0x5u },  /* L2947 */
    { AR_INFO_EVT_DATA_SET        ,  177,  0x006748d0u },  /* L2949 */
    { AR_INFO_EVT_DATA_SET        ,  178,  0x00674ad8u },  /* L2951 */
    { AR_INFO_EVT_FLAG_SET        ,  179,         0x2u },  /* L2953 */
    { AR_INFO_EVT_MARKER_SET      ,  179,         0x4u },  /* L2954 */
    { AR_INFO_EVT_FLAG_SET        ,  180,         0x2u },  /* L2956 */
    { AR_INFO_EVT_MARKER_SET      ,  180,         0x4u },  /* L2957 */
    { AR_INFO_EVT_FLAG_SET        ,  181,         0x2u },  /* L2959 */
    { AR_INFO_EVT_MARKER_SET      ,  181,         0x4u },  /* L2960 */
    { AR_INFO_EVT_FLAG_SET        ,  204,         0x2u },  /* L2962 */
    { AR_INFO_EVT_MARKER_SET      ,  204,         0x5u },  /* L2963 */
    { AR_INFO_EVT_DATA_SET        ,  205,  0x006748d0u },  /* L2965 */
    { AR_INFO_EVT_DATA_SET        ,  206,  0x00674ad8u },  /* L2967 */
    { AR_INFO_EVT_FLAG_SET        ,  207,         0x2u },  /* L2969 */
    { AR_INFO_EVT_MARKER_SET      ,  207,         0x4u },  /* L2970 */
    { AR_INFO_EVT_FLAG_SET        ,  208,         0x2u },  /* L2972 */
    { AR_INFO_EVT_MARKER_SET      ,  208,         0x4u },  /* L2973 */
    { AR_INFO_EVT_DATA_SET        ,  209,  0x006748d0u },  /* L2975 */
    { AR_INFO_EVT_FLAG_SET        ,  210,         0x2u },  /* L2977 */
    { AR_INFO_EVT_MARKER_SET      ,  210,         0x4u },  /* L2978 */
    { AR_INFO_EVT_DATA_SET        ,  211,  0x006748d0u },  /* L2980 */
    { AR_INFO_EVT_FLAG_SET        ,  212,         0x2u },  /* L2982 */
    { AR_INFO_EVT_MARKER_SET      ,  212,        0x10u },  /* L2983 */
    { AR_INFO_EVT_DATA_SET        ,  213,  0x006748d0u },  /* L2985 */
    { AR_INFO_EVT_FLAG_SET        ,  214,         0x2u },  /* L2987 */
    { AR_INFO_EVT_MARKER_SET      ,  214,        0x10u },  /* L2988 */
    { AR_INFO_EVT_FLAG_SET        ,  215,         0x2u },  /* L2990 */
    { AR_INFO_EVT_MARKER_SET      ,  215,        0x10u },  /* L2991 */
    { AR_INFO_EVT_DATA_SET        ,  216,  0x006748d0u },  /* L2993 */
    { AR_INFO_EVT_FLAG_SET        ,  217,         0x2u },  /* L2995 */
    { AR_INFO_EVT_MARKER_SET      ,  217,        0x10u },  /* L2996 */
    { AR_INFO_EVT_DATA_SET        ,  218,  0x006748d0u },  /* L2998 */
    { AR_INFO_EVT_FLAG_SET        ,  219,         0x2u },  /* L3000 */
    { AR_INFO_EVT_MARKER_SET      ,  219,        0x10u },  /* L3001 */
    { AR_INFO_EVT_FLAG_SET        ,  220,         0x2u },  /* L3003 */
    { AR_INFO_EVT_MARKER_SET      ,  220,        0x10u },  /* L3004 */
    { AR_INFO_EVT_FLAG_SET        ,  221,         0x2u },  /* L3006 */
    { AR_INFO_EVT_MARKER_SET      ,  221,         0x4u },  /* L3007 */
    { AR_INFO_EVT_DATA_SET        ,  222,  0x006748d0u },  /* L3009 */
    { AR_INFO_EVT_DATA_SET        ,  223,  0x00674ad8u },  /* L3011 */
    { AR_INFO_EVT_DATA_SET        ,  224,  0x00674ce0u },  /* L3013 */
    { AR_INFO_EVT_FLAG_SET        ,  225,         0x2u },  /* L3015 */
    { AR_INFO_EVT_MARKER_SET      ,  225,         0x4u },  /* L3016 */
    { AR_INFO_EVT_DATA_SET        ,  226,  0x006748d0u },  /* L3018 */
    { AR_INFO_EVT_FLAG_SET        ,  227,         0x2u },  /* L3020 */
    { AR_INFO_EVT_MARKER_SET      ,  227,         0x4u },  /* L3021 */
    { AR_INFO_EVT_FLAG_SET        ,  228,         0x2u },  /* L3023 */
    { AR_INFO_EVT_MARKER_SET      ,  228,         0x4u },  /* L3024 */
    { AR_INFO_EVT_FLAG_SET        ,  229,         0x2u },  /* L3026 */
    { AR_INFO_EVT_MARKER_SET      ,  229,         0x4u },  /* L3027 */
    { AR_INFO_EVT_DATA_SET        ,  230,  0x006748d0u },  /* L3029 */
    { AR_INFO_EVT_DATA_SET        ,  231,  0x00674ad8u },  /* L3031 */
    { AR_INFO_EVT_FLAG_SET        ,  232,         0x2u },  /* L3033 */
    { AR_INFO_EVT_MARKER_SET      ,  232,         0x4u },  /* L3034 */
    { AR_INFO_EVT_DATA_SET        ,  233,  0x006748d0u },  /* L3036 */
    { AR_INFO_EVT_DATA_SET        ,  234,  0x00674ad8u },  /* L3038 */
    { AR_INFO_EVT_FLAG_SET        ,  235,         0x2u },  /* L3040 */
    { AR_INFO_EVT_MARKER_SET      ,  235,         0x4u },  /* L3041 */
    { AR_INFO_EVT_DATA_SET        ,  236,  0x006748d0u },  /* L3043 */
    { AR_INFO_EVT_DATA_SET        ,  237,  0x00674ad8u },  /* L3045 */
    { AR_INFO_EVT_FLAG_SET        ,  238,         0x2u },  /* L3047 */
    { AR_INFO_EVT_MARKER_SET      ,  238,         0x4u },  /* L3048 */
    { AR_INFO_EVT_DATA_SET        ,  239,  0x006748d0u },  /* L3050 */
    { AR_INFO_EVT_FLAG_SET        ,  240,         0x2u },  /* L3052 */
    { AR_INFO_EVT_MARKER_SET      ,  240,         0x4u },  /* L3053 */
    { AR_INFO_EVT_DATA_SET        ,  241,  0x006748d0u },  /* L3055 */
    { AR_INFO_EVT_FLAG_SET        ,  242,         0x2u },  /* L3057 */
    { AR_INFO_EVT_MARKER_SET      ,  242,         0x4u },  /* L3058 */
    { AR_INFO_EVT_DATA_SET        ,  243,  0x006748d0u },  /* L3060 */
    { AR_INFO_EVT_DATA_SET        ,  244,  0x00674ad8u },  /* L3062 */
    { AR_INFO_EVT_FLAG_SET        ,  245,         0x2u },  /* L3064 */
    { AR_INFO_EVT_MARKER_SET      ,  245,         0x4u },  /* L3065 */
    { AR_INFO_EVT_DATA_SET        ,  246,  0x006748d0u },  /* L3067 */
    { AR_INFO_EVT_DATA_SET        ,  247,  0x00674ad8u },  /* L3069 */
    { AR_INFO_EVT_FLAG_SET        ,  249,         0x2u },  /* L3071 */
    { AR_INFO_EVT_MARKER_SET      ,  249,         0x4u },  /* L3072 */
    { AR_INFO_EVT_DATA_SET        ,  250,  0x006748d0u },  /* L3074 */
    { AR_INFO_EVT_DATA_SET        ,  251,  0x00674ad8u },  /* L3076 */
    { AR_INFO_EVT_DATA_SET        ,  252,  0x00674ce0u },  /* L3078 */
    { AR_INFO_EVT_FLAG_SET        ,  200,         0x2u },  /* L3080 */
    { AR_INFO_EVT_MARKER_SET      ,  200,         0x4u },  /* L3081 */
    { AR_INFO_EVT_STRUCT_COPY     ,  257,         139u },  /* L3083 */
    { AR_INFO_EVT_STRUCT_COPY     ,  258,         140u },  /* L3091 */
    { AR_INFO_EVT_STRUCT_COPY     ,  259,         141u },  /* L3099 */
    { AR_INFO_EVT_STRUCT_COPY     ,  260,         143u },  /* L3107 */
    { AR_INFO_EVT_STRUCT_COPY     ,  261,         145u },  /* L3115 */
};
#define GROUP3_INFO_EVENTS_COUNT  443

void ar_apply_group3_info_events(void)
{
    for (size_t i = 0; i < GROUP3_INFO_EVENTS_COUNT; i++) {
        const struct ar_info_event *ev = &group3_info_events[i];
        ar_info_entry *dst = g_ar_info_table[ev->dst_idx];
        switch (ev->kind) {
        case AR_INFO_EVT_MARKER_SET:
            dst->marker = (uint16_t)ev->payload;
            break;
        case AR_INFO_EVT_FLAG_SET:
            dst->flag = (uint32_t)ev->payload;
            break;
        case AR_INFO_EVT_DATA_SET:
            dst->data = (const void *)ev->payload;
            break;
        case AR_INFO_EVT_MARKER_COPY:
            dst->marker = g_ar_info_table[ev->payload]->marker;
            break;
        case AR_INFO_EVT_FLAG_COPY:
            dst->flag = g_ar_info_table[ev->payload]->flag;
            break;
        case AR_INFO_EVT_STRUCT_COPY:
            *dst = *g_ar_info_table[ev->payload];
            break;
        }
    }
}

/* ─── FUN_0057ca40 — group-3 SS_MGR slot-clone calls ────────────
 *
 * 94 FUN_004179b0(dst_pool_idx, src_pool_idx) calls in retail
 * issue order — the SS_MGR singleton slot-clone subset that the
 * 2026-05-24 partial port left deferred.  See
 * docs/findings/0057ca40-rabbit-hole.md §3 for the call shape and
 * §7 for the SS_MGR == input_mgr (at 0x008a6b60) finding.
 *
 * Distinct sources: 54.  Distinct destinations: 94.
 *
 * Pool indices index the unified 909-entry pool (see
 * ar_pool_get_slot above).  Re-run tools/extract/57ca40_clone_table.py
 * after re-exporting the decomp to catch drift. */
struct ar_group3_clone {
    uint16_t  dst_idx;    /* pool index — destination slot */
    uint16_t  src_idx;    /* pool index — source slot */
};

static const struct ar_group3_clone group3_clones[] = {
    /*  dst,    src      (retail issue order; 57ca40.c line) */
    { 0x124, 0x123 },  /* L2492 */
    { 0x125, 0x123 },  /* L2494 */
    { 0x127, 0x126 },  /* L2499 */
    { 0x128, 0x126 },  /* L2501 */
    { 0x121, 0x120 },  /* L2506 */
    { 0x122, 0x120 },  /* L2508 */
    { 0x12a, 0x129 },  /* L2513 */
    { 0x12c, 0x12b },  /* L2518 */
    { 0x12e, 0x12d },  /* L2523 */
    { 0x137, 0x135 },  /* L2531 */
    { 0x138, 0x135 },  /* L2533 */
    { 0x139, 0x135 },  /* L2536 */
    { 0x140, 0x13f },  /* L2562 */
    { 0x109, 0x108 },  /* L2573 */
    { 0x10b, 0x10a },  /* L2578 */
    { 0x143, 0x142 },  /* L2583 */
    { 0x144, 0x142 },  /* L2585 */
    { 0x145, 0x142 },  /* L2587 */
    { 0x13d, 0x13c },  /* L2592 */
    { 0x13b, 0x13a },  /* L2597 */
    { 0x11c, 0x11b },  /* L2602 */
    { 0x11d, 0x11b },  /* L2604 */
    { 0x11f, 0x11e },  /* L2609 */
    { 0x10e, 0x10c },  /* L2617 */
    { 0x10f, 0x10c },  /* L2619 */
    { 0x110, 0x10d },  /* L2621 */
    { 0x111, 0x10c },  /* L2623 */
    { 0x112, 0x10d },  /* L2625 */
    { 0x113, 0x10c },  /* L2627 */
    { 0x114, 0x10d },  /* L2629 */
    { 0x115, 0x10c },  /* L2631 */
    { 0x116, 0x10d },  /* L2633 */
    { 0x118, 0x117 },  /* L2637 */
    { 0x119, 0x117 },  /* L2639 */
    { 0x14a, 0x149 },  /* L2655 */
    { 0x14b, 0x149 },  /* L2657 */
    { 0x14d, 0x14c },  /* L2662 */
    { 0x14f, 0x14e },  /* L2667 */
    { 0x0ff, 0x0fe },  /* L2817 */
    { 0x100, 0x0fe },  /* L2819 */
    { 0x094, 0x093 },  /* L2824 */
    { 0x096, 0x095 },  /* L2829 */
    { 0x098, 0x097 },  /* L2834 */
    { 0x09a, 0x099 },  /* L2839 */
    { 0x09b, 0x099 },  /* L2841 */
    { 0x09d, 0x09c },  /* L2846 */
    { 0x0a3, 0x0a2 },  /* L2863 */
    { 0x0a4, 0x0a2 },  /* L2865 */
    { 0x0a6, 0x0a5 },  /* L2870 */
    { 0x0a7, 0x0a5 },  /* L2872 */
    { 0x0a8, 0x0a5 },  /* L2874 */
    { 0x0aa, 0x0a9 },  /* L2879 */
    { 0x0ab, 0x0a9 },  /* L2881 */
    { 0x0ac, 0x0a9 },  /* L2883 */
    { 0x0ae, 0x0ad },  /* L2888 */
    { 0x0af, 0x0ad },  /* L2890 */
    { 0x0b7, 0x0b6 },  /* L2895 */
    { 0x0b9, 0x0b8 },  /* L2900 */
    { 0x0ba, 0x0b8 },  /* L2902 */
    { 0x0bc, 0x0bb },  /* L2907 */
    { 0x0bd, 0x0bb },  /* L2909 */
    { 0x0bf, 0x0be },  /* L2914 */
    { 0x0c0, 0x0be },  /* L2916 */
    { 0x0c2, 0x0c1 },  /* L2921 */
    { 0x0c3, 0x0c1 },  /* L2923 */
    { 0x0c5, 0x0c4 },  /* L2928 */
    { 0x0b1, 0x0b0 },  /* L2948 */
    { 0x0b2, 0x0b0 },  /* L2950 */
    { 0x0cd, 0x0cc },  /* L2964 */
    { 0x0ce, 0x0cc },  /* L2966 */
    { 0x0d1, 0x0d0 },  /* L2974 */
    { 0x0d3, 0x0d2 },  /* L2979 */
    { 0x0d5, 0x0d4 },  /* L2984 */
    { 0x0d8, 0x0d7 },  /* L2992 */
    { 0x0da, 0x0d9 },  /* L2997 */
    { 0x0de, 0x0dd },  /* L3008 */
    { 0x0df, 0x0dd },  /* L3010 */
    { 0x0e0, 0x0dd },  /* L3012 */
    { 0x0e2, 0x0e1 },  /* L3017 */
    { 0x0e6, 0x0e5 },  /* L3028 */
    { 0x0e7, 0x0e5 },  /* L3030 */
    { 0x0e9, 0x0e8 },  /* L3035 */
    { 0x0ea, 0x0e8 },  /* L3037 */
    { 0x0ec, 0x0eb },  /* L3042 */
    { 0x0ed, 0x0eb },  /* L3044 */
    { 0x0ef, 0x0ee },  /* L3049 */
    { 0x0f1, 0x0f0 },  /* L3054 */
    { 0x0f3, 0x0f2 },  /* L3059 */
    { 0x0f4, 0x0f2 },  /* L3061 */
    { 0x0f6, 0x0f5 },  /* L3066 */
    { 0x0f7, 0x0f5 },  /* L3068 */
    { 0x0fa, 0x0f9 },  /* L3073 */
    { 0x0fb, 0x0f9 },  /* L3075 */
    { 0x0fc, 0x0f9 },  /* L3077 */
};
#define GROUP3_CLONES_COUNT  94

void ar_apply_group3_clones(void)
{
    for (size_t i = 0; i < GROUP3_CLONES_COUNT; i++) {
        const struct ar_group3_clone *c = &group3_clones[i];
        ar_ss_mgr_clone_slot(c->dst_idx, c->src_idx);
    }
}

/* ─── FUN_0057ca40 — group-3 inline FUN_00582b80 slot-clones ─────
 *
 * 9 FUN_00582b80(target_slot) calls in retail issue order.  Each call
 * is a __thiscall on the source slot (ECX = the last `paVar1 = DAT_X`
 * assignment in the cluster); it clones source metadata + aux_buf
 * into the target slot via `ar_sprite_slot_clone`.
 *
 * Distinct sources: 3 (pool 383, 390, 402 — all themselves registered
 * by the slot-register pass above).  Distinct targets: 9.  src/dst
 * sets are disjoint (asserted by the extractor and by a unit test),
 * so apply order is independent of the SS_MGR clone pass and the
 * info-event pass.
 *
 * Info-entry side (zero + marker/flag-copy + data-ptr for the 4 early
 * clusters; 20-byte STRUCT_COPY for the 5 late ones) is NOT here —
 * those events live in group3_info_events[] and are replayed by
 * ar_apply_group3_info_events().  Together with this walker that
 * closes the last subsystem of FUN_0057ca40; see rabbit-hole §4.
 *
 * Re-run tools/extract/57ca40_inline_clone_table.py after re-exporting
 * the decomp to catch drift. */
struct ar_group3_inline_clone {
    uint16_t  dst_idx;
    uint16_t  src_idx;
};

static const struct ar_group3_inline_clone group3_inline_clones[] = {
    /*  dst,    src      (retail issue order; 57ca40.c line) */
    { 0x180, 0x17f },  /* L2138  0x008a7c0c <- 0x008a7c08 */
    { 0x181, 0x17f },  /* L2143  0x008a7c10 <- 0x008a7c08 */
    { 0x187, 0x186 },  /* L2282  0x008a7c28 <- 0x008a7c24 */
    { 0x188, 0x186 },  /* L2287  0x008a7c2c <- 0x008a7c24 */
    { 0x101, 0x192 },  /* L3082  0x008a7a10 <- 0x008a7c54 */
    { 0x102, 0x192 },  /* L3090  0x008a7a14 <- 0x008a7c54 */
    { 0x103, 0x192 },  /* L3098  0x008a7a18 <- 0x008a7c54 */
    { 0x104, 0x192 },  /* L3106  0x008a7a1c <- 0x008a7c54 */
    { 0x105, 0x192 },  /* L3114  0x008a7a20 <- 0x008a7c54 */
};
#define GROUP3_INLINE_CLONES_COUNT  9

void ar_apply_group3_inline_clones(void)
{
    for (size_t i = 0; i < GROUP3_INLINE_CLONES_COUNT; i++) {
        const struct ar_group3_inline_clone *c = &group3_inline_clones[i];
        ar_sprite_slot *dst = ar_pool_get_slot(c->dst_idx);
        ar_sprite_slot *src = ar_pool_get_slot(c->src_idx);
        ar_sprite_slot_clone(dst, src);
    }
}

void ar_register_group3_sprites(void *zdd, uint16_t group, void *settings)
{
    for (size_t i = 0; i < GROUP3_SPRITES_COUNT; i++) {
        const struct ar_group3_entry *e = &group3_sprites[i];
        ar_sprite_slot_register(&g_ar_sprite_slots[e->idx],
            zdd, settings, e->id,
            e->width, e->height, e->colorkey,
            e->scale_flag, e->type, group);
    }
    /* Retail issue order: in FUN_0057ca40 the 4th-pass info-entry
     * writes (FLAG/MARKER/DATA_SET etc.) are interleaved with the
     * slot-register calls, the SS_MGR clones, and the inline clones.
     * Order of those four passes is observably independent because:
     *
     *   - The info-event pass touches `g_ar_info_table[*]` only.
     *   - The slot-register pass touches `g_ar_sprite_slots[*]` only.
     *   - SS_MGR clones read src (slot + info) and write dst (slot +
     *     info) — and the clone table's source indices are ALL outside
     *     the destination set (see test), so source state is whatever
     *     the slot pass + info pass left.
     *   - Inline clones touch only the slot side (info-entry side of
     *     each cluster lives in group3_info_events[]); src/dst sets
     *     are disjoint, and the 3 sources (383/390/402) are
     *     themselves registered by the slot pass above.
     *
     * So we replay them sequentially: registers, info events, SS_MGR
     * clones, inline clones. */
    ar_apply_group3_info_events();
    ar_apply_group3_clones();
    ar_apply_group3_inline_clones();
}

/* ─── FUN_00562ea0:613-624 — boot-driver register wiring ─────────── */

void ar_boot_register_all(void *zdd, void *zds, void *settings,
                          void *sotesp_module,
                          const ar_locale_state *locale)
{
    ar_register_fonts         (zdd, 1, settings);
    ar_register_sounds        (zds, 1, settings);
    ar_register_palette_ramps (zdd, 2, settings, sotesp_module);
    ar_register_aux_sounds    (zds, 2, settings);
    ar_register_group3_sprites(zdd, 3, settings);
    ar_register_game_sounds   (zds, 3, settings);
    if (locale != NULL) {
        ar_register_locale_sounds(zds, 3, settings, locale);
    }
    ar_register_main_sprites  (zdd, 4, settings, sotesp_module);
    ar_register_game_sprites  (zdd, 5, settings);
}
