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
