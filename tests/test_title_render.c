/*
 * tests/test_title_render.c — host tests for the title render bridge
 * (the sprite-display-list compositor FUN_0056c180).
 *
 * The resolution step (frame-index animation math, ramp-index clamp,
 * asset-pool/frame lookup, centi-pixel geometry) is tested exhaustively
 * against hand-computed expectations.  The draw loop is tested via the
 * blit hook (iteration bound + skip-invalid + arg forwarding) without
 * touching real locked surfaces.
 */
#include "t.h"
#include "title_render.h"
#include "asset_register.h"

#include <string.h>

/* ─── fixtures ───────────────────────────────────────────────────────
 *
 * Install a pre-decoded frame-surface array on a pool slot so
 * ar_sprite_slot_frame resolves without invoking the (absent) decoder. */

#define FR_MAX 16
static ar_sprite_entry  g_entries_a[1];
static ar_sprite_entry  g_entries_b[1];
static void            *g_frames_a[FR_MAX];
static void            *g_frames_b[FR_MAX];

static void bank_setup(uint16_t bank_id, ar_sprite_entry *entries,
                       void **frames, zdd_object *surf, int frame_slot)
{
    ar_sprite_slot *slot = ar_pool_get_slot(bank_id);
    memset(slot, 0, sizeof *slot);
    memset(frames, 0, sizeof(void *) * FR_MAX);
    frames[frame_slot] = surf;
    entries[0].frames = frames;
    entries[0].b = NULL;
    slot->entries = entries;
    slot->entry_count = 1;
}

static void bank_clear(uint16_t bank_id)
{
    ar_sprite_slot *slot = ar_pool_get_slot(bank_id);
    memset(slot, 0, sizeof *slot);
}

static zdd_object mk_surf(int32_t m0c, int32_t m10, int32_t m14,
                          int32_t m18, int32_t ck)
{
    zdd_object s;
    memset(&s, 0, sizeof s);
    s.metric_0c    = m0c;
    s.metric_10    = m10;
    s.metric_14    = m14;
    s.metric_18    = m18;
    s.colorkey_out = ck;
    return s;
}

/* ─── title_compositor_resolve ──────────────────────────────────────── */

int test_title_render_resolve_basic(void)
{
    static zdd_object surf;
    surf = mk_surf(100, 200, 64, 48, 0xABCD);
    bank_setup(13, g_entries_a, g_frames_a, &surf, 9);

    /* dummy ramp: 20 distinct descriptors */
    static zdd_blend_desc descs[TITLE_FADE_RAMP_LEN];
    const zdd_blend_desc *ramp[TITLE_FADE_RAMP_LEN];
    for (int i = 0; i < TITLE_FADE_RAMP_LEN; i++) ramp[i] = &descs[i];

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    e.anim_num = 2; e.anim_div = 4; e.frame_count = 10; e.frame_base = 5;
    e.alpha_level = 500; e.x_num = 1234; e.y_num = -250; e.bank_id = 13;

    /* p = (2*10)/4 = 5 → min(5,9)=5 → frame = 5-5+10-1 = 9 */
    title_blit_params p = title_compositor_resolve(&e, ramp);
    T_ASSERT_EQ_I(p.valid, 1);
    T_ASSERT_EQ_P(p.src, &surf);
    T_ASSERT_EQ_P(p.desc, &descs[10]);          /* idx = (500*20)/1000 = 10 */
    T_ASSERT_EQ_I(p.dst_x, 112);                /* 100 + 1234/100 = 100+12  */
    T_ASSERT_EQ_I(p.dst_y, 198);                /* 200 + (-250)/100 = 200-2 */
    T_ASSERT_EQ_I(p.width, 64);
    T_ASSERT_EQ_I(p.height, 48);
    T_ASSERT_EQ_I(p.colorkey, 0xABCD);
    bank_clear(13);
    return 0;
}

int test_title_render_resolve_frame_index_min_clamp(void)
{
    static zdd_object surf;
    surf = mk_surf(0, 0, 8, 8, 0);
    bank_setup(13, g_entries_a, g_frames_a, &surf, 3);

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    /* p = (100*4)/1 = 400 → clamped to frame_count-1 = 3.
     * frame = frame_base - 3 + 4 - 1 = frame_base = 3. */
    e.anim_num = 100; e.anim_div = 1; e.frame_count = 4; e.frame_base = 3;
    e.bank_id = 13;

    title_blit_params p = title_compositor_resolve(&e, NULL);
    T_ASSERT_EQ_I(p.valid, 1);
    T_ASSERT_EQ_P(p.src, &surf);
    bank_clear(13);
    return 0;
}

int test_title_render_resolve_frame_index_masks_to_u16(void)
{
    static zdd_object surf;
    surf = mk_surf(0, 0, 1, 1, 0);
    bank_setup(13, g_entries_a, g_frames_a, &surf, 3);

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    /* p = 0 → frame = 65530 - 0 + 10 - 1 = 65539 → & 0xffff = 3. */
    e.anim_num = 0; e.anim_div = 1; e.frame_count = 10; e.frame_base = 65530;
    e.bank_id = 13;

    title_blit_params p = title_compositor_resolve(&e, NULL);
    T_ASSERT_EQ_I(p.valid, 1);
    T_ASSERT_EQ_P(p.src, &surf);                /* frame index 3 resolved */
    bank_clear(13);
    return 0;
}

int test_title_render_resolve_ramp_clamp_low_high_mid(void)
{
    static zdd_object surf;
    surf = mk_surf(0, 0, 1, 1, 0);
    bank_setup(13, g_entries_a, g_frames_a, &surf, 0);

    static zdd_blend_desc descs[TITLE_FADE_RAMP_LEN];
    const zdd_blend_desc *ramp[TITLE_FADE_RAMP_LEN];
    for (int i = 0; i < TITLE_FADE_RAMP_LEN; i++) ramp[i] = &descs[i];

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    e.anim_num = 0; e.anim_div = 1; e.frame_count = 1; e.frame_base = 0;
    e.bank_id = 13;

    /* negative idx → ramp[0] */
    e.alpha_level = -50;
    T_ASSERT_EQ_P(title_compositor_resolve(&e, ramp).desc, &descs[0]);
    /* idx exactly 20 (1000*20/1000) → clamp to ramp[19] */
    e.alpha_level = 1000;
    T_ASSERT_EQ_P(title_compositor_resolve(&e, ramp).desc, &descs[19]);
    /* idx > 20 → clamp to ramp[19] */
    e.alpha_level = 5000;
    T_ASSERT_EQ_P(title_compositor_resolve(&e, ramp).desc, &descs[19]);
    /* mid: 350*20/1000 = 7 */
    e.alpha_level = 350;
    T_ASSERT_EQ_P(title_compositor_resolve(&e, ramp).desc, &descs[7]);
    bank_clear(13);
    return 0;
}

int test_title_render_resolve_div_zero_is_invalid(void)
{
    static zdd_object surf;
    surf = mk_surf(0, 0, 1, 1, 0);
    bank_setup(13, g_entries_a, g_frames_a, &surf, 0);

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    e.anim_num = 1; e.anim_div = 0; e.frame_count = 4; e.bank_id = 13;

    title_blit_params p = title_compositor_resolve(&e, NULL);
    T_ASSERT_EQ_I(p.valid, 0);
    bank_clear(13);
    return 0;
}

int test_title_render_resolve_null_sprite_is_invalid(void)
{
    /* bank with no entries installed → ar_sprite_slot_frame returns NULL. */
    bank_clear(20);

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    e.anim_num = 0; e.anim_div = 1; e.frame_count = 1; e.bank_id = 20;

    title_blit_params p = title_compositor_resolve(&e, NULL);
    T_ASSERT_EQ_I(p.valid, 0);
    return 0;
}

int test_title_render_resolve_null_entry_is_invalid(void)
{
    title_blit_params p = title_compositor_resolve(NULL, NULL);
    T_ASSERT_EQ_I(p.valid, 0);
    return 0;
}

int test_title_render_resolve_null_ramp_yields_null_desc(void)
{
    static zdd_object surf;
    surf = mk_surf(1, 2, 3, 4, 5);
    bank_setup(13, g_entries_a, g_frames_a, &surf, 0);

    title_sprite_entry e;
    memset(&e, 0, sizeof e);
    e.anim_num = 0; e.anim_div = 1; e.frame_count = 1; e.alpha_level = 500;
    e.bank_id = 13;

    title_blit_params p = title_compositor_resolve(&e, NULL);
    T_ASSERT_EQ_I(p.valid, 1);
    T_ASSERT_EQ_P(p.desc, NULL);
    bank_clear(13);
    return 0;
}

/* ─── title_compositor_draw (loop + hook) ───────────────────────────── */

static struct {
    const zdd_blend_desc *desc;
    zdd_object *dest, *src;
    int32_t dx, dy, w, h, sx, sy, ck;
    zdd_object *gdi;
} g_cap[8];
static int g_cap_n;

static void cap_hook(const zdd_blend_desc *desc, zdd_object *dest,
                     zdd_object *src, int32_t dx, int32_t dy,
                     int32_t w, int32_t h, int32_t sx, int32_t sy,
                     int32_t ck, zdd_object *gdi)
{
    if (g_cap_n < 8) {
        g_cap[g_cap_n].desc = desc; g_cap[g_cap_n].dest = dest;
        g_cap[g_cap_n].src = src;   g_cap[g_cap_n].dx = dx;
        g_cap[g_cap_n].dy = dy;     g_cap[g_cap_n].w = w;
        g_cap[g_cap_n].h = h;       g_cap[g_cap_n].sx = sx;
        g_cap[g_cap_n].sy = sy;     g_cap[g_cap_n].ck = ck;
        g_cap[g_cap_n].gdi = gdi;
    }
    g_cap_n++;
}

int test_title_render_draw_iterates_and_skips_invalid(void)
{
    static zdd_object surf_a, surf_b, dest;
    surf_a = mk_surf(10, 20, 30, 40, 0x111);
    surf_b = mk_surf(50, 60, 70, 80, 0x222);
    memset(&dest, 0, sizeof dest);
    bank_setup(13, g_entries_a, g_frames_a, &surf_a, 0);
    bank_setup(14, g_entries_b, g_frames_b, &surf_b, 0);

    title_sprite_entry entries[3];
    memset(entries, 0, sizeof entries);
    /* entry 0: valid, bank 13, frame 0, alpha 0 → ramp[0] */
    entries[0].anim_num = 0; entries[0].anim_div = 1; entries[0].frame_count = 1;
    entries[0].frame_base = 0; entries[0].bank_id = 13;
    entries[0].x_num = 500; entries[0].y_num = 700;   /* +5 / +7 */
    /* entry 1: invalid (div 0) → skipped */
    entries[1].anim_div = 0; entries[1].bank_id = 13;
    /* entry 2: valid, bank 14 */
    entries[2].anim_num = 0; entries[2].anim_div = 1; entries[2].frame_count = 1;
    entries[2].frame_base = 0; entries[2].bank_id = 14;

    title_sprite_group group;
    memset(&group, 0, sizeof group);
    group.entries = entries;
    group.count = 3;

    g_cap_n = 0;
    title_compositor_blit_hook = cap_hook;
    title_compositor_draw(&group, &dest, NULL);
    title_compositor_blit_hook = NULL;

    T_ASSERT_EQ_I(g_cap_n, 2);                  /* entry 1 skipped          */
    /* call 0 = entry 0 (surf_a) */
    T_ASSERT_EQ_P(g_cap[0].src, &surf_a);
    T_ASSERT_EQ_P(g_cap[0].dest, &dest);
    T_ASSERT_EQ_I(g_cap[0].dx, 15);             /* metric_0c 10 + 500/100=5 */
    T_ASSERT_EQ_I(g_cap[0].dy, 27);             /* metric_10 20 + 700/100=7 */
    T_ASSERT_EQ_I(g_cap[0].w, 30);
    T_ASSERT_EQ_I(g_cap[0].h, 40);
    T_ASSERT_EQ_I(g_cap[0].sx, 0);
    T_ASSERT_EQ_I(g_cap[0].sy, 0);
    T_ASSERT_EQ_I(g_cap[0].ck, 0x111);
    T_ASSERT_EQ_P(g_cap[0].gdi, NULL);
    /* call 1 = entry 2 (surf_b) */
    T_ASSERT_EQ_P(g_cap[1].src, &surf_b);
    T_ASSERT_EQ_I(g_cap[1].ck, 0x222);

    bank_clear(13);
    bank_clear(14);
    return 0;
}

int test_title_render_draw_null_and_empty_group_noop(void)
{
    g_cap_n = 0;
    title_compositor_blit_hook = cap_hook;

    title_compositor_draw(NULL, NULL, NULL);     /* NULL group */

    title_sprite_group empty;
    memset(&empty, 0, sizeof empty);
    title_sprite_entry e; memset(&e, 0, sizeof e);
    empty.entries = &e; empty.count = 0;         /* count 0 */
    title_compositor_draw(&empty, NULL, NULL);

    title_sprite_group noents;
    memset(&noents, 0, sizeof noents);
    noents.entries = NULL; noents.count = 5;     /* NULL entries */
    title_compositor_draw(&noents, NULL, NULL);

    title_compositor_blit_hook = NULL;
    T_ASSERT_EQ_I(g_cap_n, 0);
    return 0;
}
