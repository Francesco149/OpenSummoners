/*
 * test_scene_fade.c — host tests for the scene-transition fade-grid / establishing
 * REVEAL (src/scene_fade.c; FUN_0048e920 render + the FUN_00499ab0:125-177 update
 * + the FUN_0049a8xx iris pattern setters; armed by FUN_00439690:555-583).
 *
 * Ground truth: runs/reveal-grid (the town reveal) — W=10, H=120, count=1200,
 * mode=1, speed=1000, variant=0 (center-out, RNG this run).
 */
#include "scene_fade.h"
#include "t.h"

#include <string.h>

/* ---- a recording render sink (counts opaque/alpha, marks per-row opacity) -- */

typedef struct {
    int opaque, alpha;
    int row_black[SCENE_FADE_H];   /* count of opaque cells per row */
    int row_fade[SCENE_FADE_H];    /* count of alpha cells per row  */
    int last_alpha_level;
} sf_log;

static int sf_row_of(int y) { return y >> 2; }

static void rec_opaque(void *ctx, int x, int y)
{
    sf_log *L = (sf_log *)ctx;
    (void)x;
    L->opaque++;
    if (sf_row_of(y) >= 0 && sf_row_of(y) < SCENE_FADE_H) L->row_black[sf_row_of(y)]++;
}
static void rec_alpha(void *ctx, int x, int y, int level)
{
    sf_log *L = (sf_log *)ctx;
    (void)x;
    L->alpha++;
    L->last_alpha_level = level;
    if (sf_row_of(y) >= 0 && sf_row_of(y) < SCENE_FADE_H) L->row_fade[sf_row_of(y)]++;
}

static void sf_render_log(const scene_fade_grid *g, sf_log *L)
{
    memset(L, 0, sizeof *L);
    scene_fade_render(g, rec_opaque, rec_alpha, L);
}

/* ---- arm: a full-screen black grid, mapped col/row -> screen ---------------- */

int test_scene_fade_arm_fills_screen(void)
{
    static scene_fade_grid g;
    memset(&g, 0, sizeof g);

    /* unarmed: render is a no-op (count==0 gate). */
    sf_log L;
    sf_render_log(&g, &L);
    T_ASSERT_EQ_I(L.opaque, 0);
    T_ASSERT_EQ_I(scene_fade_active(&g), 0);

    scene_fade_arm(&g, SCENE_FADE_MODE_OUT, /*variant=*/0, /*speed=*/1000);

    /* every cell opaque black -> 1200 opaque tiles, the whole 640x480. */
    sf_render_log(&g, &L);
    T_ASSERT_EQ_I(L.opaque, SCENE_FADE_COUNT);
    T_ASSERT_EQ_I(L.alpha, 0);
    T_ASSERT_EQ_I(scene_fade_active(&g), 1);

    /* every row has all 10 columns; cell (col,row) maps to (col<<6, row<<2). */
    for (int r = 0; r < SCENE_FADE_H; r++) T_ASSERT_EQ_I(L.row_black[r], SCENE_FADE_W);
    T_ASSERT_EQ_I(g.cells[0].col, 0);
    T_ASSERT_EQ_I(g.cells[0].row, 0);
    T_ASSERT_EQ_I(g.cells[1].col, 1);            /* row-major: idx 1 = (col 1, row 0) */
    T_ASSERT_EQ_I(g.cells[SCENE_FADE_W].row, 1); /* idx W = (col 0, row 1) */
    return 0;
}

/* ---- variant 0: the center-out vertical iris ------------------------------- */

int test_scene_fade_center_out(void)
{
    static scene_fade_grid g;
    memset(&g, 0, sizeof g);
    scene_fade_arm(&g, SCENE_FADE_MODE_OUT, /*variant=*/0, /*speed=*/1000);

    /* one tick: mode 1 advances 2 rows/tick from the center (H/2 = 60).  k=0
     * marks row 60 (radius 0), k=1 marks rows 59 & 61 (radius 1); radius -> 2. */
    scene_fade_step(&g);
    T_ASSERT_EQ_I(g.radius, 2);
    T_ASSERT_EQ_I(g.cells[60 * SCENE_FADE_W].state, 1);   /* row 60 fading */
    T_ASSERT_EQ_I(g.cells[59 * SCENE_FADE_W].state, 1);
    T_ASSERT_EQ_I(g.cells[61 * SCENE_FADE_W].state, 1);
    T_ASSERT_EQ_I(g.cells[58 * SCENE_FADE_W].state, 0);   /* not yet reached */
    T_ASSERT_EQ_I(g.cells[0].state, 0);                   /* edges still black */

    sf_log L;
    sf_render_log(&g, &L);
    /* 3 center rows fading (alpha), the other 117 opaque. */
    T_ASSERT_EQ_I(L.alpha, 3 * SCENE_FADE_W);
    T_ASSERT_EQ_I(L.opaque, (SCENE_FADE_H - 3) * SCENE_FADE_W);
    T_ASSERT_EQ_I(L.row_fade[60], SCENE_FADE_W);
    T_ASSERT_EQ_I(L.row_fade[59], SCENE_FADE_W);
    T_ASSERT_EQ_I(L.row_fade[61], SCENE_FADE_W);
    /* the iris is symmetric about the center, and the black recedes from there. */
    T_ASSERT_EQ_I(L.row_black[0], SCENE_FADE_W);
    T_ASSERT_EQ_I(L.row_black[119], SCENE_FADE_W);

    /* the center fades over ~1000/100 = 10 ticks; after 12 total it has cleared
     * (state 2 -> skipped, neither opaque nor alpha) and the band has widened. */
    for (int i = 0; i < 11; i++) scene_fade_step(&g);
    sf_render_log(&g, &L);
    int center_drawn = L.row_black[60] + L.row_fade[60];
    T_ASSERT_EQ_I(center_drawn, 0);
    /* symmetry: row (60-d) and (60+d) are always in the same draw state. */
    for (int d = 0; d <= 40; d++) {
        T_ASSERT_EQ_I(L.row_black[60 - d], L.row_black[60 + d]);
        T_ASSERT_EQ_I(L.row_fade[60 - d],  L.row_fade[60 + d]);
    }
    return 0;
}

/* ---- variant 1: edges-in (the first rows cleared are the screen edges) ----- */

int test_scene_fade_edges_in(void)
{
    static scene_fade_grid g;
    memset(&g, 0, sizeof g);
    scene_fade_arm(&g, SCENE_FADE_MODE_OUT, /*variant=*/1, /*speed=*/1000);

    scene_fade_step(&g);
    /* edges-in marks row=radius (top) and row=H-radius (bottom).  k=0 radius 0:
     * top row 0, bottom row 120 (out of range, skipped).  k=1 radius 1: top row
     * 1, bottom row 119.  radius -> 2. */
    T_ASSERT_EQ_I(g.radius, 2);
    T_ASSERT_EQ_I(g.cells[0 * SCENE_FADE_W].state, 1);     /* top edge fading */
    T_ASSERT_EQ_I(g.cells[1 * SCENE_FADE_W].state, 1);
    T_ASSERT_EQ_I(g.cells[119 * SCENE_FADE_W].state, 1);   /* bottom edge fading */
    T_ASSERT_EQ_I(g.cells[60 * SCENE_FADE_W].state, 0);    /* center still black */
    return 0;
}

/* ---- the reveal completes (every cell clears, done latches) ----------------- */

int test_scene_fade_completes(void)
{
    static scene_fade_grid g;
    memset(&g, 0, sizeof g);
    scene_fade_arm(&g, SCENE_FADE_MODE_OUT, /*variant=*/0, /*speed=*/1000);

    /* radius reaches H/2 in H/2 / 2 = 30 ticks; each cell then fades over
     * ~1000/100 = 10 ticks.  done latches the tick AFTER the last cell clears
     * (0x499ab0:154-174 counts a state-1 cell active even as it -> state 2), so
     * loop on done, not active.  Bounded run must terminate. */
    int ticks = 0;
    while (!g.done && ticks < 200) { scene_fade_step(&g); ticks++; }
    T_ASSERT(ticks < 200);
    T_ASSERT_EQ_I(g.done, 1);
    T_ASSERT_EQ_I(scene_fade_active(&g), 0);

    sf_log L;
    sf_render_log(&g, &L);
    T_ASSERT_EQ_I(L.opaque, 0);
    T_ASSERT_EQ_I(L.alpha, 0);

    /* stepping a settled grid is a no-op. */
    scene_fade_step(&g);
    T_ASSERT_EQ_I(g.done, 1);
    return 0;
}

/* ---- the alpha level falls 31 -> 0 as a cell fades ------------------------- */

int test_scene_fade_alpha_ramp(void)
{
    static scene_fade_grid g;
    memset(&g, 0, sizeof g);
    scene_fade_arm(&g, SCENE_FADE_MODE_OUT, /*variant=*/0, /*speed=*/1000);

    /* watch row 60 (the first marked): its alpha = 0x1f - (timer<<5)/1000 falls
     * as the timer rises (+100/tick), and the level stays in (0, 32). */
    int prev = 999;
    for (int i = 0; i < 12; i++) {
        scene_fade_step(&g);
        const scene_fade_cell *c = &g.cells[60 * SCENE_FADE_W];
        if (c->state != 1) break;
        int a = 0x1f - (((int)c->timer << 5) / 1000);
        T_ASSERT(a >= 0 && a < 0x20);
        T_ASSERT(a <= prev);              /* monotonically fading out */
        prev = a;
    }
    /* eventually it clears (state 2). */
    T_ASSERT_EQ_I(g.cells[60 * SCENE_FADE_W].state, 2);
    return 0;
}
