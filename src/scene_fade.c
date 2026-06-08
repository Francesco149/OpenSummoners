/*
 * scene_fade.c — the scene-transition fade-grid (establishing REVEAL).  See
 * scene_fade.h for the RE provenance.  Faithful port of FUN_0048e920 (render),
 * 0x439690:555-583 (arm), and the 0x499ab0:125-177 update block + the
 * iris pattern setters FUN_0049a890 / FUN_0049a740 / FUN_0049aae0 / FUN_0049aa00.
 */
#include "scene_fade.h"

/* The per-cell fade timer freshly-marked rows get, staggered within one step
 * call by row index k (the row marked first fades first): the 0x49a8xx setters'
 * `(step/rows) * ((rows - k) - 1)`. */
static uint16_t sf_mark_timer(int step, int rows, int k)
{
    int per = (rows != 0) ? step / rows : 0;
    return (uint16_t)(per * ((rows - k) - 1));
}

/* Mark cell `idx` as fading (state 0 -> 1) with timer `t`, honoring the bounds
 * guard the setters apply (`-1 < idx && idx < count && state == 0`). */
static void sf_mark(scene_fade_grid *g, int idx, uint16_t t)
{
    if (idx >= 0 && idx < (int)SCENE_FADE_COUNT && g->cells[idx].state == 0) {
        g->cells[idx].state = 1;
        g->cells[idx].timer = t;
    }
}

/* variant 0 — FUN_0049a890: center-out vertical iris.  Each of `rows` passes
 * marks the rows H/2-radius (above) and H/2+radius (below), then grows radius
 * toward H/2. */
static void sf_pattern_center_out(scene_fade_grid *g, int step, int rows)
{
    int half = SCENE_FADE_H >> 1;
    for (int k = 0; k < rows; k++) {
        uint16_t t = sf_mark_timer(step, rows, k);
        for (int col = 0; col < SCENE_FADE_W; col++) {
            sf_mark(g, (half - g->radius) * SCENE_FADE_W + col, t);
            sf_mark(g, (half + g->radius) * SCENE_FADE_W + col, t);
        }
        if (g->radius < half) g->radius++;
    }
}

/* variant 1 — FUN_0049a740: edges-in vertical wipe.  Marks the rows radius
 * (from the top) and H-radius (from the bottom), radius -> H/2. */
static void sf_pattern_edges_in(scene_fade_grid *g, int step, int rows)
{
    int half = SCENE_FADE_H >> 1;
    for (int k = 0; k < rows; k++) {
        uint16_t t = sf_mark_timer(step, rows, k);
        for (int col = 0; col < SCENE_FADE_W; col++) {
            sf_mark(g, g->radius * SCENE_FADE_W + col, t);
            sf_mark(g, (SCENE_FADE_H - g->radius) * SCENE_FADE_W + col, t);
        }
        if (g->radius < half) g->radius++;
    }
}

/* variant 2, mode 1 — FUN_0049aae0: single bottom-up sweep.  Marks row
 * (H-radius)-1, radius -> H. */
static void sf_pattern_sweep_up(scene_fade_grid *g, int step, int rows)
{
    for (int k = 0; k < rows; k++) {
        uint16_t t = sf_mark_timer(step, rows, k);
        for (int col = 0; col < SCENE_FADE_W; col++)
            sf_mark(g, ((SCENE_FADE_H - g->radius) - 1) * SCENE_FADE_W + col, t);
        if (g->radius < SCENE_FADE_H) g->radius++;
    }
}

/* variant 2, mode 2 — FUN_0049aa00: single top-down sweep.  Marks row radius,
 * radius -> H. */
static void sf_pattern_sweep_down(scene_fade_grid *g, int step, int rows)
{
    for (int k = 0; k < rows; k++) {
        uint16_t t = sf_mark_timer(step, rows, k);
        for (int col = 0; col < SCENE_FADE_W; col++)
            sf_mark(g, g->radius * SCENE_FADE_W + col, t);
        if (g->radius < SCENE_FADE_H) g->radius++;
    }
}

void scene_fade_arm(scene_fade_grid *g, int mode, int variant, int speed)
{
    g->mode    = mode;
    g->variant = variant;
    g->speed   = speed;
    g->radius  = 0;
    g->done    = 0;
    g->armed   = 1;
    /* 0x439690:566-583 — row-major fill: cell[row*W+col] = {0,0,col,row}. */
    for (int col = 0; col < SCENE_FADE_W; col++) {
        for (int row = 0; row < SCENE_FADE_H; row++) {
            scene_fade_cell *c = &g->cells[row * SCENE_FADE_W + col];
            c->state = 0;
            c->timer = 0;
            c->col   = col;
            c->row   = row;
        }
    }
}

void scene_fade_step(scene_fade_grid *g)
{
    if (!g->armed || g->done) return;

    int step = (g->speed * 100) / 1000;   /* 0x499ab0:127 iVar7 */
    int rows;                              /* iVar9 — rows advanced per tick */
    switch (g->mode) {
    case SCENE_FADE_MODE_OUT: rows = 2; break;
    case SCENE_FADE_MODE_IN:  rows = 4; break;
    default:                  g->done = 1; return;   /* mode 0/3: instant */
    }

    /* iris pattern setter (marks `rows` new rows fading this tick). */
    switch (g->variant) {
    case 0: sf_pattern_center_out(g, step, rows); break;
    case 1: sf_pattern_edges_in(g, step, rows);   break;
    case 2:
        if (g->mode == SCENE_FADE_MODE_OUT)      sf_pattern_sweep_up(g, step, rows * 2);
        else if (g->mode == SCENE_FADE_MODE_IN)  sf_pattern_sweep_down(g, step, rows * 2);
        break;
    default: break;
    }

    /* age each fading cell's timer; done when every cell is clear (0x499ab0:
     * 154-176 — state 0 + state 1 both count as still-active). */
    int active = 0;
    for (uint32_t i = 0; i < SCENE_FADE_COUNT; i++) {
        scene_fade_cell *c = &g->cells[i];
        if (c->state == 0) {
            active++;
        } else if (c->state == 1) {
            c->timer = (uint16_t)(c->timer + step);
            if (c->timer > 999) { c->timer = 1000; c->state = 2; }
            active++;
        }
    }
    if (active == 0) g->done = 1;
}

void scene_fade_render(const scene_fade_grid *g,
                       scene_fade_opaque_fn opaque,
                       scene_fade_alpha_fn alpha, void *ctx)
{
    if (!g->armed) return;   /* 0x48e920:17 — count==0 (no transition) -> skip */

    for (uint32_t i = 0; i < SCENE_FADE_COUNT; i++) {
        const scene_fade_cell *c = &g->cells[i];
        int x = c->col << 6;   /* 0x48e920:24 — col << 6 */
        int y = c->row << 2;   /* 0x48e920:25 — row << 2 */

        if (g->mode == SCENE_FADE_MODE_IN) {
            /* mode 2: state 0 clear, state 1 rising alpha, state 2 opaque. */
            if (c->state == 1) {
                int a = ((int)c->timer << 5) / 1000;
                if (a != 0) { if (a < 0x20) { if (alpha) alpha(ctx, x, y, a); }
                              else if (opaque) opaque(ctx, x, y); }
            } else if (c->state == 2) {
                if (opaque) opaque(ctx, x, y);
            }
        } else {
            /* mode 1/3: state 0 opaque, state 1 falling alpha, state 2 clear. */
            if (c->state == 0) {
                if (opaque) opaque(ctx, x, y);
            } else if (c->state == 1) {
                int a = 0x1f - (((int)c->timer << 5) / 1000);
                if (a != 0) { if (a < 0x20) { if (alpha) alpha(ctx, x, y, a); }
                              else if (opaque) opaque(ctx, x, y); }
            }
        }
    }
}

int scene_fade_active(const scene_fade_grid *g)
{
    if (!g->armed) return 0;
    for (uint32_t i = 0; i < SCENE_FADE_COUNT; i++)
        if (g->cells[i].state != 2) return 1;
    return 0;
}
