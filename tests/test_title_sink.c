/*
 * tests/test_title_sink.c — host tests for the title render sink
 * (title_sink.c): the command-stream → ZDD-call mapping.
 *
 * The sink resolves sprites out of the two title banks (asset-pool slots
 * 19 main / 20 cursor) and forwards each TITLE_DRAW_* command to its retail
 * render-half call against a bound primary surface.  We observe the keyed
 * blits via title_keyed_blit_hook, the compositor's alpha blits via
 * title_compositor_blit_hook, and present / log / deferred draws via the
 * bound ctx callbacks — so nothing here touches a real locked surface.
 */
#include "t.h"
#include "title_sink.h"
#include "asset_register.h"

#include <string.h>

/* ─── fixtures ───────────────────────────────────────────────────────
 *
 * Pre-decoded frame arrays on the two banks so ar_sprite_slot_frame
 * resolves without invoking the (absent) decoder.  Each bank's frames[i]
 * points at a distinct surface so a test can prove the right frame id was
 * selected. */
#define FR_MAX 16
static ar_sprite_entry g_entries_main[1];
static ar_sprite_entry g_entries_cur[1];
static void           *g_frames_main[FR_MAX];
static void           *g_frames_cur[FR_MAX];
static zdd_object      g_surf_main[FR_MAX];
static zdd_object      g_surf_cur[FR_MAX];
static zdd_object      g_primary;

static void bank_fill(uint16_t bank, ar_sprite_entry *entries, void **frames,
                      zdd_object *surfs)
{
    ar_sprite_slot *slot = ar_pool_get_slot(bank);
    memset(slot, 0, sizeof *slot);
    memset(frames, 0, sizeof(void *) * FR_MAX);
    for (int i = 0; i < FR_MAX; i++) {
        memset(&surfs[i], 0, sizeof surfs[i]);
        surfs[i].metric_0c    = 1000 + i;   /* distinct, observable metrics */
        surfs[i].metric_10    = 2000 + i;
        surfs[i].metric_14    = 64;
        surfs[i].metric_18    = 48;
        surfs[i].colorkey_out = 0xFF00FF;
        frames[i] = &surfs[i];
    }
    entries[0].frames = frames;
    entries[0].b      = NULL;
    slot->entries     = entries;
    slot->entry_count = 1;
}

static void bank_clear(uint16_t bank)
{
    ar_sprite_slot *slot = ar_pool_get_slot(bank);
    memset(slot, 0, sizeof *slot);
}

/* ── capture state ── */
static struct {
    int          keyed_n;
    zdd_object  *keyed_self, *keyed_dest;
    int32_t      keyed_x, keyed_y;

    int          comp_n;
    const zdd_blend_desc *comp_desc;
    zdd_object  *comp_dest, *comp_src;
    int32_t      comp_dx, comp_dy, comp_ck, comp_w, comp_h, comp_sx, comp_sy;

    int          clip_n;
    zdd_object  *clip_src, *clip_dest;
    int32_t      clip_dx, clip_dy, clip_w, clip_h, clip_sx, clip_sy;

    int          present_n, logflip_n;
    int          logo_n, sparkle_n, cursor_n;
    title_draw_cmd last_logo, last_sparkle, last_cursor;
    void        *user_seen;
} cap;

static int keyed_cap(zdd_object *self, zdd_object *dest, int32_t x, int32_t y)
{
    cap.keyed_n++; cap.keyed_self = self; cap.keyed_dest = dest;
    cap.keyed_x = x; cap.keyed_y = y;
    return 0;
}
static void comp_cap(const zdd_blend_desc *desc, zdd_object *dest, zdd_object *src,
                     int32_t dx, int32_t dy, int32_t w, int32_t h,
                     int32_t sx, int32_t sy, int32_t ck, zdd_object *gdi)
{
    cap.comp_n++; cap.comp_desc = desc; cap.comp_dest = dest; cap.comp_src = src;
    cap.comp_dx = dx; cap.comp_dy = dy; cap.comp_ck = ck;
    cap.comp_w = w; cap.comp_h = h; cap.comp_sx = sx; cap.comp_sy = sy;
}
static int clip_cap(zdd_object *src, zdd_object *dest, int32_t dx, int32_t dy,
                    int32_t w, int32_t h, int32_t sx, int32_t sy)
{
    cap.clip_n++; cap.clip_src = src; cap.clip_dest = dest;
    cap.clip_dx = dx; cap.clip_dy = dy; cap.clip_w = w; cap.clip_h = h;
    cap.clip_sx = sx; cap.clip_sy = sy;
    return 0;
}
static void present_cb(void *u) { cap.present_n++; cap.user_seen = u; }
static void logflip_cb(void *u) { cap.logflip_n++; }
static void logo_cb(const title_draw_cmd *c, void *u)   { cap.logo_n++;   cap.last_logo = *c; }
static void sparkle_cb(const title_draw_cmd *c, void *u){ cap.sparkle_n++; cap.last_sparkle = *c; }
static void cursor_cb(const title_draw_cmd *c, void *u) { cap.cursor_n++;  cap.last_cursor = *c; }

static int g_user_tag = 0x1234;

/* A fake 20-entry blend ramp for the alpha-path tests (sparkle / cursor). */
static zdd_blend_desc g_fake_descs[20];
static const zdd_blend_desc *g_fake_ramp[20];

/* Install both banks, a primary, all hooks/callbacks, and bind the sink. */
static void sink_setup(title_sink_ctx *ctx)
{
    memset(&cap, 0, sizeof cap);
    memset(&g_primary, 0, sizeof g_primary);
    bank_fill(AR_SPR_TITLE_MAIN,   g_entries_main, g_frames_main, g_surf_main);
    bank_fill(AR_SPR_TITLE_CURSOR, g_entries_cur,  g_frames_cur,  g_surf_cur);

    title_keyed_blit_hook      = keyed_cap;
    title_compositor_blit_hook = comp_cap;
    title_clipped_blit_hook    = clip_cap;

    memset(ctx, 0, sizeof *ctx);
    ctx->primary      = &g_primary;
    ctx->present      = present_cb;
    ctx->log_flip     = logflip_cb;
    ctx->draw_logo    = logo_cb;
    ctx->draw_sparkle = sparkle_cb;
    ctx->draw_cursor  = cursor_cb;
    ctx->user         = &g_user_tag;
    title_sink_bind(ctx);
}

static void sink_teardown(void)
{
    title_sink_bind(NULL);
    title_keyed_blit_hook = NULL;
    title_compositor_blit_hook = NULL;
    title_clipped_blit_hook = NULL;
    bank_clear(AR_SPR_TITLE_MAIN);
    bank_clear(AR_SPR_TITLE_CURSOR);
}

static void emit(title_draw_op op, int32_t asset, int32_t level,
                 int32_t alpha, int32_t x, int32_t y)
{
    title_draw_cmd cmd = { op, asset, level, alpha, x, y };
    title_render_sink(&cmd);
}

/* ─── unbound / null-primary safety ─────────────────────────────────── */

int test_sink_unbound_is_noop(void)
{
    memset(&cap, 0, sizeof cap);
    title_keyed_blit_hook = keyed_cap;
    title_sink_bind(NULL);            /* explicitly unbound */
    emit(TITLE_DRAW_SPRITE, 2, 0, 0, 0, 0);
    emit(TITLE_DRAW_FLIP, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.keyed_n, 0);
    T_ASSERT_EQ_I(cap.present_n, 0);
    title_keyed_blit_hook = NULL;
    return 0;
}

int test_sink_null_primary_is_noop(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    ctx.primary = NULL;
    title_sink_bind(&ctx);
    emit(TITLE_DRAW_SPRITE, 2, 0, 0, 0, 0);
    emit(TITLE_DRAW_SURFACE_RESET, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.keyed_n, 0);
    sink_teardown();
    return 0;
}

/* ─── SURFACE_RESET ─────────────────────────────────────────────────── */

int test_sink_surface_reset_no_blit(void)
{
    /* RESET maps to zdd_object_clear (no keyed/compositor blit).  On a host
     * primary with no COM surface zdd_object_clear no-ops; we assert the
     * sink does NOT mistake it for a blit and does not crash. */
    title_sink_ctx ctx;
    sink_setup(&ctx);
    emit(TITLE_DRAW_SURFACE_RESET, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.keyed_n, 0);
    T_ASSERT_EQ_I(cap.comp_n, 0);
    sink_teardown();
    return 0;
}

/* ─── SURFACE_CLEAR + SPRITE: keyed blit of frame(main, asset) → primary ── */

int test_sink_surface_clear_blits_frame(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    /* prologue background = frame 0; logo alpha-0 = frame 1/2 */
    emit(TITLE_DRAW_SURFACE_CLEAR, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.keyed_n, 1);
    T_ASSERT_EQ_P(cap.keyed_self, &g_surf_main[0]);   /* frame 0 */
    T_ASSERT_EQ_P(cap.keyed_dest, &g_primary);
    T_ASSERT_EQ_I(cap.keyed_x, 0);
    T_ASSERT_EQ_I(cap.keyed_y, 0);

    emit(TITLE_DRAW_SURFACE_CLEAR, 2, 0, 0, 0, 0);    /* title logo frame 2 */
    T_ASSERT_EQ_I(cap.keyed_n, 2);
    T_ASSERT_EQ_P(cap.keyed_self, &g_surf_main[2]);
    sink_teardown();
    return 0;
}

int test_sink_sprite_resolves_frame_id(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    emit(TITLE_DRAW_SPRITE, 5, 0, 0, 0, 0);           /* asset = frame id 5 */
    T_ASSERT_EQ_I(cap.keyed_n, 1);
    T_ASSERT_EQ_P(cap.keyed_self, &g_surf_main[5]);
    T_ASSERT_EQ_P(cap.keyed_dest, &g_primary);
    sink_teardown();
    return 0;
}

/* A bank that is not registered (NULL pool slot body / NULL entries) makes
 * ar_sprite_slot_frame return NULL → the sprite op is a faithful no-op. */
int test_sink_sprite_unregistered_bank_noop(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    bank_clear(AR_SPR_TITLE_MAIN);                    /* unregister main */
    emit(TITLE_DRAW_SPRITE, 5, 0, 0, 0, 0);
    emit(TITLE_DRAW_SURFACE_CLEAR, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.keyed_n, 0);
    sink_teardown();
    return 0;
}

/* ─── SPRITE_LEVEL: plain path (NULL ramp_b) vs alpha path ───────────── */

int test_sink_sprite_level_plain_path(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    ctx.ramp_b = NULL;                                /* ramp 0 ⇒ plain keyed */
    title_sink_bind(&ctx);
    emit(TITLE_DRAW_SPRITE_LEVEL, 6, 1000, 0, 0, 0);  /* asset 6, fade 1000 */
    T_ASSERT_EQ_I(cap.keyed_n, 1);
    T_ASSERT_EQ_P(cap.keyed_self, &g_surf_main[6]);
    T_ASSERT_EQ_P(cap.keyed_dest, &g_primary);
    sink_teardown();
    return 0;
}

int test_sink_sprite_level_alpha_path(void)
{
    static zdd_blend_desc descs[TITLE_FADE_RAMP_LEN];
    static const zdd_blend_desc *ramp[TITLE_FADE_RAMP_LEN];
    for (int i = 0; i < TITLE_FADE_RAMP_LEN; i++) ramp[i] = &descs[i];

    title_sink_ctx ctx;
    sink_setup(&ctx);
    ctx.ramp_b = ramp;                                /* every entry non-NULL */
    title_sink_bind(&ctx);
    /* level 1000 / div 1000 → idx = 20 → diverts to PLAIN (retail never
     * loads ramp[20]); use level 500 → idx 10 → alpha via ramp[10]. */
    emit(TITLE_DRAW_SPRITE_LEVEL, 6, 500, 0, 0, 0);
    T_ASSERT_EQ_I(cap.keyed_n, 0);                    /* not the plain path */
    T_ASSERT_EQ_I(cap.comp_n, 1);
    T_ASSERT_EQ_P(cap.comp_desc, &descs[10]);
    T_ASSERT_EQ_P(cap.comp_src, &g_surf_main[6]);
    T_ASSERT_EQ_P(cap.comp_dest, &g_primary);
    /* alpha_blit offsets the dest by the sprite's placement metric. */
    T_ASSERT_EQ_I(cap.comp_dx, g_surf_main[6].metric_0c + 0);
    T_ASSERT_EQ_I(cap.comp_dy, g_surf_main[6].metric_10 + 0);
    T_ASSERT_EQ_I(cap.comp_ck, (int32_t)0xFF00FF);
    sink_teardown();
    return 0;
}

/* ─── FRAME_END: compose the bound display-list group ───────────────── */

int test_sink_frame_end_composes_group(void)
{
    /* one display-list entry resolving to a registered bank (slot 13). */
    static ar_sprite_entry g_e13[1];
    static void           *g_f13[FR_MAX];
    static zdd_object       g_s13;
    ar_sprite_slot *slot13 = ar_pool_get_slot(13);
    memset(slot13, 0, sizeof *slot13);
    memset(g_f13, 0, sizeof g_f13);
    memset(&g_s13, 0, sizeof g_s13);
    g_s13.metric_14 = 32; g_s13.metric_18 = 24; g_s13.colorkey_out = 0x123456;
    g_f13[0] = &g_s13;
    g_e13[0].frames = g_f13; g_e13[0].b = NULL;
    slot13->entries = g_e13; slot13->entry_count = 1;

    static title_sprite_entry entry;
    memset(&entry, 0, sizeof entry);
    entry.bank_id = 13; entry.frame_count = 1; entry.anim_div = 1;
    static title_sprite_group group;
    memset(&group, 0, sizeof group);
    group.entries = &entry; group.count = 1;

    title_sink_ctx ctx;
    sink_setup(&ctx);
    ctx.compose_group = &group;
    title_sink_bind(&ctx);
    emit(TITLE_DRAW_FRAME_END, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.comp_n, 1);
    T_ASSERT_EQ_P(cap.comp_src, &g_s13);
    T_ASSERT_EQ_P(cap.comp_dest, &g_primary);

    /* no group bound ⇒ frame-end composes nothing. */
    cap.comp_n = 0;
    ctx.compose_group = NULL;
    title_sink_bind(&ctx);
    emit(TITLE_DRAW_FRAME_END, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.comp_n, 0);

    memset(slot13, 0, sizeof *slot13);
    sink_teardown();
    return 0;
}

/* ─── FLIP / LOG_FLIPPING / deferred draws via ctx callbacks ─────────── */

int test_sink_flip_and_log_callbacks(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    emit(TITLE_DRAW_FLIP, 0, 0, 0, 0, 0);
    emit(TITLE_DRAW_LOG_FLIPPING, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(cap.present_n, 1);
    T_ASSERT_EQ_I(cap.logflip_n, 1);
    T_ASSERT_EQ_P(cap.user_seen, &g_user_tag);
    sink_teardown();
    return 0;
}

int test_sink_logo_op_routes_to_callback(void)
{
    /* TITLE_DRAW_LOGO is no longer emitted by the scene (the logos fold into
     * TITLE_DRAW_SPRITE_LEVEL, ckpt 30) but its sink case + draw_logo
     * callback remain as an extension point — exercise it. */
    title_sink_ctx ctx;
    sink_setup(&ctx);
    emit(TITLE_DRAW_LOGO, 8, 0, 77, 0, 0);
    T_ASSERT_EQ_I(cap.logo_n, 1);
    T_ASSERT_EQ_I(cap.last_logo.asset, 8);
    T_ASSERT_EQ_I(cap.last_logo.alpha, 77);
    T_ASSERT_EQ_I(cap.keyed_n, 0);
    T_ASSERT_EQ_I(cap.comp_n, 0);
    sink_teardown();
    return 0;
}

/* SPARKLE is wired: with ramp_b NULL the per-sparkle level resolves to no
 * descriptor → the opaque clipped path (0x5b9bf0).  Frame 5 of the MAIN bank;
 * dst raw x/y (NOT metric-offset on the clipped path), 4×48 sliver, src
 * sub-rect (x, 416). */
int test_sink_sparkle_opaque_clipped_when_no_ramp(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);                         /* ramp_b stays NULL */
    emit(TITLE_DRAW_SPARKLE, 5, 700, 0, 196, 0x1a0);  /* level 700, x=196 */
    T_ASSERT_EQ_I(cap.sparkle_n, 0);          /* not the deferred callback path */
    T_ASSERT_EQ_I(cap.clip_n, 1);             /* opaque clipped copy            */
    T_ASSERT_EQ_P(cap.clip_src,  &g_surf_main[5]);
    T_ASSERT_EQ_P(cap.clip_dest, &g_primary);
    T_ASSERT_EQ_I(cap.clip_dx, 196);          /* raw dst x (no metric offset)   */
    T_ASSERT_EQ_I(cap.clip_dy, 0x1a0);        /* raw dst y = 416                */
    T_ASSERT_EQ_I(cap.clip_w, 4);
    T_ASSERT_EQ_I(cap.clip_h, 48);
    T_ASSERT_EQ_I(cap.clip_sx, 196);          /* src x = column                 */
    T_ASSERT_EQ_I(cap.clip_sy, 0x1a0);        /* src y = 416                    */
    T_ASSERT_EQ_I(cap.comp_n, 0);
    sink_teardown();
    return 0;
}

/* SPARKLE alpha path: with a populated ramp_b a mid-range level resolves to
 * ramp_b[idx] (idx = level·20/1000) → the compositor alpha blit, dest origin
 * offset by the sprite metric (metric_0c + x, metric_10 + 416). */
int test_sink_sparkle_alpha_via_ramp(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    for (int i = 0; i < 20; i++) g_fake_ramp[i] = &g_fake_descs[i];
    ctx.ramp_b = g_fake_ramp;
    title_sink_bind(&ctx);

    /* level 500 → idx = (500*20)/1000 = 10 → ramp_b[10]. */
    emit(TITLE_DRAW_SPARKLE, 5, 500, 0, 200, 0x1a0);
    T_ASSERT_EQ_I(cap.clip_n, 0);                       /* not the opaque path */
    T_ASSERT_EQ_I(cap.comp_n, 1);
    T_ASSERT_EQ_P(cap.comp_src,  &g_surf_main[5]);
    T_ASSERT_EQ_P(cap.comp_dest, &g_primary);
    T_ASSERT_EQ_P((void *)cap.comp_desc, (void *)g_fake_ramp[10]);
    T_ASSERT_EQ_I(cap.comp_dx, (1000 + 5) + 200);       /* metric_0c + x       */
    T_ASSERT_EQ_I(cap.comp_dy, (2000 + 5) + 0x1a0);     /* metric_10 + 416     */
    T_ASSERT_EQ_I(cap.comp_w, 4);
    T_ASSERT_EQ_I(cap.comp_h, 48);
    T_ASSERT_EQ_I(cap.comp_sx, 200);                    /* src x = column      */
    T_ASSERT_EQ_I(cap.comp_sy, 0x1a0);                  /* src y = 416         */
    sink_teardown();
    return 0;
}

/* MENU_CURSOR is wired: it resolves the CURSOR bank (pool 20) frame at the
 * row index and alpha-blends it via ramp_a (the cursor's draw wrapper
 * 0x56c470).  With a non-NULL ramp the blend routes through the compositor
 * blit hook; the sprite metric offsets the dest origin. */
int test_sink_menu_cursor_draws_via_ramp(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    for (int i = 0; i < 20; i++) g_fake_ramp[i] = &g_fake_descs[i];
    ctx.ramp_a = g_fake_ramp;
    title_sink_bind(&ctx);

    /* row 3 cursor at y=80; level_num = the pulse peak 1000, level_div =
     * 0x4b0 (in the alpha field) ⇒ idx = (1000*20)/1200 = 16 (the real peak,
     * NOT a clamped 19). */
    emit(TITLE_DRAW_MENU_CURSOR, 3, 1000, 0x4b0, 0, 80);

    T_ASSERT_EQ_I(cap.cursor_n, 0);                  /* not the deferred path  */
    T_ASSERT_EQ_I(cap.comp_n, 1);                    /* alpha blend happened   */
    T_ASSERT_EQ_P(cap.comp_src,  &g_surf_cur[3]);    /* CURSOR bank, frame 3   */
    T_ASSERT_EQ_P(cap.comp_dest, &g_primary);
    T_ASSERT_EQ_P((void *)cap.comp_desc, (void *)g_fake_ramp[16]);  /* idx 16 */
    T_ASSERT_EQ_I(cap.comp_dx, 1000 + 3 + 0);        /* metric_0c + x          */
    T_ASSERT_EQ_I(cap.comp_dy, 2000 + 3 + 80);       /* metric_10 + y          */
    sink_teardown();
    return 0;
}

/* Deferred ops with NULL callbacks are silent no-ops (no crash). */
int test_sink_deferred_null_callbacks_safe(void)
{
    title_sink_ctx ctx;
    sink_setup(&ctx);
    ctx.draw_logo = NULL; ctx.draw_sparkle = NULL; ctx.draw_cursor = NULL;
    title_sink_bind(&ctx);
    emit(TITLE_DRAW_LOGO, 8, 0, 77, 0, 0);
    emit(TITLE_DRAW_SPARKLE, 5, 0, 99, 196, 0);
    emit(TITLE_DRAW_MENU_CURSOR, 3, 0, 0x4b0, 0, 80);   /* num 0 ⇒ no draw */
    T_ASSERT_EQ_I(cap.logo_n, 0);
    sink_teardown();
    return 0;
}
