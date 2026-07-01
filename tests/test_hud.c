/*
 * test_hud.c — host tests for the freeroam status HUD geometry + formatting
 * (src/hud.c).  Ground truth: hud_probe.py on sword2.osr (errands freeroam),
 * the top-left leader panel at a fully-slid-in tick:
 *   HP bar rects (118,7,168,2),(117,9,..),(116,11,..),(115,13,..) src y 0,
 *     each + a depleted alpha companion at x 286,285,284,283 width 0 (HP full);
 *   MP bar rects (118,21,168,2),(117,23,..),(116,25,..) src y 2;
 *   HP text ' 100 / 100', MP text '  20 / 20' (font 2).
 * Derived from FUN_00494e60 (slide x-base) + FUN_00498680 (per-row geometry).
 */
#include "hud.h"
#include "t.h"

#include <string.h>

/* ---- the panel slide x-base (0x494e60:87) -------------------------------- */

int test_hud_panel_xbase(void)
{
    /* fully slid in (prog 1000) -> xbase 1 (the steady HUD). */
    T_ASSERT_EQ_I(hud_panel_xbase(1000), 1);
    /* fully out (prog 0): ((0 - 11000) * 0x20) / 1000 + 1 = -351. */
    T_ASSERT_EQ_I(hud_panel_xbase(0), -351);
    /* half (prog 500): ((5500 - 11000) * 32) / 1000 + 1 = -175. */
    T_ASSERT_EQ_I(hud_panel_xbase(500), -175);
    return 0;
}

/* ---- HP bar: 4 rows, full ratio, the exact ground-truth rects ------------- */

int test_hud_hp_bar_rows_full(void)
{
    const int x = 1 + HUD_BAR_DX;           /* xbase(1) + 0x75 = 118 */
    const int y = HUD_PANEL_YBASE + HUD_HP_BAR_DY;   /* 1 + 6 = 7 */
    const int exp_x[4] = {118, 117, 116, 115};
    const int exp_y[4] = {7, 9, 11, 13};
    const int exp_dep[4] = {286, 285, 284, 283};
    for (int r = 0; r < HUD_HP_ROWS; r++) {
        hud_bar_row g = hud_bar_row_geom(1000, 1000, x, y, HUD_BAR_WIDTH,
                                         HUD_HP_SRC_Y, r);
        T_ASSERT_EQ_I(g.dst_x, exp_x[r]);
        T_ASSERT_EQ_I(g.dst_y, exp_y[r]);
        T_ASSERT_EQ_I(g.dst_w, 168);
        T_ASSERT_EQ_I(g.dst_h, 2);
        T_ASSERT_EQ_I(g.src_x, 0);
        T_ASSERT_EQ_I(g.src_y, 0);
        T_ASSERT_EQ_I(g.src_w, 168);
        T_ASSERT_EQ_I(g.src_h, 2);
        T_ASSERT_EQ_I(g.dep_x, exp_dep[r]);   /* depleted companion x */
        T_ASSERT_EQ_I(g.dep_w, 0);            /* full HP -> 0-width depleted */
    }
    return 0;
}

/* ---- MP bar: 3 rows, src y 2, base y 21 ---------------------------------- */

int test_hud_mp_bar_rows_full(void)
{
    const int x = 1 + HUD_BAR_DX;            /* 118 */
    const int y = HUD_PANEL_YBASE + HUD_MP_BAR_DY;   /* 1 + 0x14 = 21 */
    const int exp_x[3] = {118, 117, 116};
    const int exp_y[3] = {21, 23, 25};
    for (int r = 0; r < HUD_MP_ROWS; r++) {
        hud_bar_row g = hud_bar_row_geom(1000, 1000, x, y, HUD_BAR_WIDTH,
                                         HUD_MP_SRC_Y, r);
        T_ASSERT_EQ_I(g.dst_x, exp_x[r]);
        T_ASSERT_EQ_I(g.dst_y, exp_y[r]);
        T_ASSERT_EQ_I(g.dst_w, 168);
        T_ASSERT_EQ_I(g.src_y, 2);
        T_ASSERT_EQ_I(g.dep_w, 0);
    }
    return 0;
}

/* ---- a partial gauge splits filled / depleted at cur*width/max ------------ */

int test_hud_bar_partial(void)
{
    const int x = 118, y = 7;
    /* ratio 500/1000 -> fill = 500*168/1000 = 84. */
    hud_bar_row g = hud_bar_row_geom(500, 1000, x, y, HUD_BAR_WIDTH,
                                     HUD_HP_SRC_Y, 0);
    T_ASSERT_EQ_I(g.dst_x, 118);
    T_ASSERT_EQ_I(g.dst_w, 84);            /* filled */
    T_ASSERT_EQ_I(g.src_w, 84);
    T_ASSERT_EQ_I(g.dep_x, 118 + 84);      /* depleted starts after the fill */
    T_ASSERT_EQ_I(g.dep_w, 168 - 84);      /* 84 */
    /* empty gauge -> 0 filled, full depleted. */
    g = hud_bar_row_geom(0, 1000, x, y, HUD_BAR_WIDTH, HUD_HP_SRC_Y, 0);
    T_ASSERT_EQ_I(g.dst_w, 0);
    T_ASSERT_EQ_I(g.dep_x, 118);
    T_ASSERT_EQ_I(g.dep_w, 168);
    return 0;
}

/* ---- the "%s / %d" gauge text (cur right-justified width 4) --------------- */

int test_hud_format_gauge(void)
{
    char buf[16];
    hud_format_gauge(100, 100, buf, sizeof buf);
    T_ASSERT(strcmp(buf, " 100 / 100") == 0);
    hud_format_gauge(20, 20, buf, sizeof buf);
    T_ASSERT(strcmp(buf, "  20 / 20") == 0);
    hud_format_gauge(5, 30, buf, sizeof buf);
    T_ASSERT(strcmp(buf, "   5 / 30") == 0);
    return 0;
}

/* ---- the slide-in ramps +50/tick (capped) + hits the exact retail xbase ---- */

int test_hud_slide_step(void)
{
    T_ASSERT_EQ_I(hud_slide_step(0), 50);
    T_ASSERT_EQ_I(hud_slide_step(50), 100);
    T_ASSERT_EQ_I(hud_slide_step(950), 1000);
    T_ASSERT_EQ_I(hud_slide_step(1000), 1000);     /* capped at full */
    /* the +50 ramp reproduces the recording's integer xbase sequence */
    T_ASSERT_EQ_I(hud_panel_xbase(50), -333);
    T_ASSERT_EQ_I(hud_panel_xbase(100), -315);
    T_ASSERT_EQ_I(hud_panel_xbase(150), -298);
    return 0;
}

/* ---- slice 1c-1: the level-digit atlas glyph select (FUN_00495e40) ------- */

int test_hud_glyph_frame(void)
{
    /* printable c -> frame c - 0x21; the level '1' -> frame 16 (ground truth
     * sword2.osr seq 526).  '0'..'9' map to 15..24. */
    T_ASSERT_EQ_I(hud_glyph_frame('1'), 16);
    T_ASSERT_EQ_I(hud_glyph_frame('0'), 15);
    T_ASSERT_EQ_I(hud_glyph_frame('9'), 24);
    T_ASSERT_EQ_I(hud_glyph_frame('!'), 0);        /* 0x21 - 0x21 */
    T_ASSERT_EQ_I(hud_glyph_frame('z'), 'z' - 0x21);
    /* space + out-of-range -> -1 (a gap, no cel). */
    T_ASSERT_EQ_I(hud_glyph_frame(' '), -1);
    T_ASSERT_EQ_I(hud_glyph_frame('{'), -1);       /* == 0x7b, excluded */
    T_ASSERT_EQ_I(hud_glyph_frame('\0'), -1);
    return 0;
}

/* ---- slice 1c-1: the star + level dst positions (fully slid in) ----------- */

int test_hud_star_level_positions(void)
{
    const int xb = hud_panel_xbase(1000);          /* = 1 */
    const int yb = HUD_PANEL_YBASE;                 /* = 1 */
    /* the 2 element stars at (187,30),(200,30) — +13 px steps (seq 496-497). */
    T_ASSERT_EQ_I(HUD_STAR_COUNT, 2);
    const int star_x[2] = {187, 200};
    for (int k = 0; k < HUD_STAR_COUNT; k++) {
        T_ASSERT_EQ_I(xb + HUD_STAR_DX + k * HUD_STAR_STEP, star_x[k]);
        T_ASSERT_EQ_I(yb + HUD_STAR_DY, 30);
    }
    /* the level digit base at (161,25) (seq 526). */
    T_ASSERT_EQ_I(xb + HUD_LEVEL_DX, 161);
    T_ASSERT_EQ_I(yb + HUD_LEVEL_DY, 25);
    return 0;
}

/* ---- slice 1c-2: the EXP gauge dst rect (fully slid in) -------------------- */

int test_hud_exp_gauge_position(void)
{
    const int xb = hud_panel_xbase(1000);          /* = 1 */
    const int yb = HUD_PANEL_YBASE;                 /* = 1 */
    /* the depleted span at (144,42,104,2), src (0,14) (seq493-494). */
    T_ASSERT_EQ_I(xb + HUD_EXP_DX, 144);
    T_ASSERT_EQ_I(yb + HUD_EXP_DY, 42);
    T_ASSERT_EQ_I(HUD_EXP_WIDTH, 104);
    T_ASSERT_EQ_I(HUD_EXP_HEIGHT, 2);
    T_ASSERT_EQ_I(HUD_EXP_SRC_Y, 14);
    return 0;
}

/* ---- slice 2: the item bar (0x4962a0 x6) position math -------------------- */

int test_hud_item_slot_position(void)
{
    /* fully slid in (hslide=0, vslide=1000): x = slot*32+440, y = 444 —
     * ground truth sword2.osr tick 2200 seq 535-552 (findings/freeroam-
     * hud.md sec8). */
    const int slot_x[HUD_ITEM_SLOT_COUNT] = {440, 472, 504, 536, 568, 600};
    for (int s = 0; s < HUD_ITEM_SLOT_COUNT; s++) {
        T_ASSERT_EQ_I(hud_item_slot_x(s, 0), slot_x[s]);
        T_ASSERT_EQ_I(hud_item_slot_y(1000), 444);
    }
    /* hslide shifts every slot left together (the door-glow x term). */
    T_ASSERT_EQ_I(hud_item_slot_x(0, 1000), 240);
    /* vslide=0 (not yet slid in) sits 128px below the resting y. */
    T_ASSERT_EQ_I(hud_item_slot_y(0), 572);
    return 0;
}

/* ---- slice 2: the item bar's own (slower) slide-in ramp ------------------- */

int test_hud_item_slide_step(void)
{
    T_ASSERT_EQ_I(hud_item_slide_step(0), 20);
    T_ASSERT_EQ_I(hud_item_slide_step(980), 1000);
    T_ASSERT_EQ_I(hud_item_slide_step(1000), 1000);    /* capped at full */
    return 0;
}

/* ---- slice 2: the 6 ground-truthed mode-icon frames ----------------------- */

int test_hud_item_icon_frames(void)
{
    /* dhash-matched against sword2.osr tick 2200 by a brute-force sweep of
     * pool 0x31's frames 0..89 (findings/freeroam-hud.md sec8) — none are 0
     * (the "no icon" skip is never observed in the errands). */
    const int expect[HUD_ITEM_SLOT_COUNT] = {44, 48, 40, 36, 59, 80};
    for (int s = 0; s < HUD_ITEM_SLOT_COUNT; s++) {
        T_ASSERT_EQ_I(HUD_ITEM_ICON_FRAME[s], expect[s]);
        T_ASSERT(HUD_ITEM_ICON_FRAME[s] != 0);
    }
    /* the label frame is slot+4 (F1..F6), independent of party state. */
    for (int s = 0; s < HUD_ITEM_SLOT_COUNT; s++)
        T_ASSERT_EQ_I(s + HUD_ITEM_LABEL_BASE, s + 4);
    return 0;
}

/* ---- slice 3: the DOOR INDICATOR (FUN_004969b0) --------------------------- *
 * Ground truth: hand-derived from the ckpt-174 disasm RE (docs/decompiled/
 * by-address/4969b0.c + the ECX-hiding fixes in hud.h's doc comment),
 * cross-checked independently in Python (tdiv/center helpers reimplemented
 * fresh, not by calling this C code) before being baked in here — see the
 * ckpt-174 session log.  No real off-screen exit is reachable in any
 * currently-captured session (PORT-DEBT(hud-door-actors), hud.h), so these
 * are synthetic fixtures exercising every branch the errands' 2 real
 * candidates never reach. */

static const hud_door_camera door_test_cam = { 0, 0, 0, 0, 64000, 48000, 0 };
static const hud_door_ref    door_test_ref = { { 32000, 24000, 0, 0, 0 }, 3 };

static hud_door_candidate door_cand(int32_t x, int32_t y, int32_t zone, const void *id)
{
    hud_door_candidate c;
    c.body.x = x; c.body.y = y; c.body.w = 0; c.body.h = 0; c.body.baseline = 0;
    c.zone = zone;
    c.active = 1; c.body_valid = 1; c.suppressed = 0; c.status = 0;
    c.id = id;
    return c;
}

int test_hud_door_edges(void)
{
    hud_door_dedup dd;
    hud_door_draw out;

    /* TOP: straight above the reference, well outside the viewport. */
    hud_door_dedup_reset(&dd);
    hud_door_candidate c = door_cand(32000, -20000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cx, 320); T_ASSERT_EQ_I(out.cy, 0);
    T_ASSERT_EQ_I(out.frame_index, 4);      /* edge TOP(0) + 4 */
    T_ASSERT_EQ_I(out.ramp_idx, 10);

    /* RIGHT: past the right edge, within reach. */
    hud_door_dedup_reset(&dd);
    c = door_cand(90000, 24000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cx, 640); T_ASSERT_EQ_I(out.cy, 240);
    T_ASSERT_EQ_I(out.frame_index, 5);      /* edge RIGHT(1) + 4 */
    T_ASSERT_EQ_I(out.ramp_idx, 8);

    /* BOTTOM: straight below. */
    hud_door_dedup_reset(&dd);
    c = door_cand(32000, 70000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cx, 320); T_ASSERT_EQ_I(out.cy, 480);
    T_ASSERT_EQ_I(out.frame_index, 6);      /* edge BOTTOM(2) + 4 */
    T_ASSERT_EQ_I(out.ramp_idx, 8);

    /* LEFT: past the left edge, within reach (72000 is an exclusive bound —
     * x=-40000 gives dx=72000 exactly and must be REJECTED; -30000 (62000)
     * passes). */
    hud_door_dedup_reset(&dd);
    c = door_cand(-40000, 24000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 0);
    c = door_cand(-30000, 24000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cx, 0); T_ASSERT_EQ_I(out.cy, 240);
    T_ASSERT_EQ_I(out.frame_index, 7);      /* edge LEFT(3) + 4 */
    T_ASSERT_EQ_I(out.ramp_idx, 6);
    return 0;
}

int test_hud_door_filters(void)
{
    hud_door_dedup dd;
    hud_door_draw out;
    hud_door_candidate c;

    /* the base TOP candidate passes every gate (baseline, from test_hud_door_edges). */
    hud_door_dedup_reset(&dd);
    c = door_cand(32000, -20000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);

    /* inactive / invalid body / suppressed / bad status each filter it out. */
    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 3, NULL); c.active = 0;
    hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);

    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 3, NULL); c.body_valid = 0;
    hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);

    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 3, NULL); c.suppressed = 1;
    hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);

    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 3, NULL); c.status = 1;
    hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);

    /* zone gate: both non-wildcard and EQUAL -> reject; equal-but-3 (either
     * side) or non-3-and-different -> pass. */
    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 1, NULL);
    hud_door_ref ref1 = door_test_ref; ref1.zone = 1;
    hud_door_process(&dd, &ref1, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);          /* 1 == 1, neither is 3 -> reject */

    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 2, NULL);
    hud_door_process(&dd, &ref1, &door_test_cam, &c, NULL, &out);   /* ref zone 1, cand zone 2 */
    T_ASSERT_EQ_I(out.visible, 1);          /* differ, neither is 3 -> pass */

    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 1, NULL);
    hud_door_process(&dd, &door_test_ref /* zone 3 */, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 1);          /* ref is wildcard 3 -> pass despite equal-looking */

    hud_door_dedup_reset(&dd); c = door_cand(32000, -20000, 0, NULL);
    hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);          /* cand zone 0 -> reject unconditionally */

    /* on-screen candidates never get an indicator (co-located with the ref,
     * comfortably inside the viewport). */
    hud_door_dedup_reset(&dd); c = door_cand(32000, 24000, 3, NULL);
    hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
    T_ASSERT_EQ_I(out.visible, 0);
    return 0;
}

int test_hud_door_dedup_stack(void)
{
    hud_door_dedup dd;
    hud_door_draw out;
    hud_door_dedup_reset(&dd);

    /* A and B clamp to TOP positions 3px apart (< 5) -> same cluster; B is
     * the SECOND arrival, so it carries the +12 perpendicular (into-screen,
     * i.e. +Y for a TOP edge) stack offset; A (first) gets none. */
    hud_door_candidate a = door_cand(32000, -20000, 3, NULL);
    hud_door_candidate b = door_cand(32300, -20000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &a, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cx, 320); T_ASSERT_EQ_I(out.cy, 0);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &b, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cx, 323); T_ASSERT_EQ_I(out.cy, 12);
    T_ASSERT_EQ_I(dd.n, 1);                 /* still ONE bucket — B joined A's */

    /* a 3rd, FAR-away TOP candidate (>=5px away) opens its own bucket. */
    hud_door_candidate far = door_cand(40000, -20000, 3, NULL);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &far, NULL, &out), 0);
    T_ASSERT_EQ_I(out.visible, 1);
    T_ASSERT_EQ_I(out.cy, 0);                /* first arrival at its own bucket -> no offset */
    T_ASSERT_EQ_I(dd.n, 2);

    /* stacking caps at 5 drawn (stack 0..4); the 6th duplicate at the SAME
     * cluster updates the bucket but does not draw. */
    hud_door_dedup_reset(&dd);
    int expect_visible[6] = {1,1,1,1,1,0};
    for (int i = 0; i < 6; i++) {
        hud_door_candidate d = door_cand(32000, -20000, 3, NULL);
        T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &d, NULL, &out), 0);
        T_ASSERT_EQ_I(out.visible, expect_visible[i]);
    }
    T_ASSERT_EQ_I(dd.n, 1);
    return 0;
}

int test_hud_door_highlight(void)
{
    hud_door_dedup dd;
    hud_door_draw out;
    hud_door_dedup_reset(&dd);

    int tag_a, tag_b;
    hud_door_candidate a = door_cand(32000, -20000, 3, &tag_a);
    hud_door_candidate b = door_cand(40000, -20000, 3, &tag_b);   /* a distinct bucket */

    /* highlighting b must not affect a's frame (still edge TOP, no +4). */
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &a, &tag_b, &out), 0);
    T_ASSERT_EQ_I(out.frame_index, 4);
    T_ASSERT_EQ_I(hud_door_process(&dd, &door_test_ref, &door_test_cam, &b, &tag_b, &out), 0);
    T_ASSERT_EQ_I(out.frame_index, 8);      /* edge TOP(0) + hi-lite(4) + 4 */
    return 0;
}

int test_hud_door_dedup_exhaustion(void)
{
    hud_door_dedup dd;
    hud_door_draw out;
    hud_door_dedup_reset(&dd);

    /* 20 distinct TOP-edge clusters (world x spaced 1000 apart -> 10px on
     * screen, comfortably >5px) fill the table exactly; the 21st must abort
     * the whole scan (-1), matching retail's bare `return;` on table
     * exhaustion. */
    int rc = 0;
    for (int i = 0; i < 21; i++) {
        hud_door_candidate c = door_cand(32000 + i * 1000, -20000, 3, NULL);
        rc = hud_door_process(&dd, &door_test_ref, &door_test_cam, &c, NULL, &out);
        if (i < 20) {
            T_ASSERT_EQ_I(rc, 0);
            T_ASSERT_EQ_I(out.visible, 1);
        }
    }
    T_ASSERT_EQ_I(rc, -1);                  /* the 21st candidate aborted the scan */
    T_ASSERT_EQ_I(dd.n, 20);
    return 0;
}
