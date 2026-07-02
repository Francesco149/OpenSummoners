/*
 * test_collision.c — host tests for the tile-collision read-side (src/collision.c,
 * the FUN_0054e990 vertical mover).  Synthetic grids (map_grid_emit_obj places
 * class-10 solid cells) exercise the clamp/clear paths; the expected stop
 * positions are hand-derived from the decompiled <=100-unit sweep.
 *
 * Grid addressing recap: idx = col*0x80 + row, col = worldX/0xc80 (the DIM0 /
 * row-pitch axis), row = worldY/0xc80; cell = 0xc80 = 3200 world units.
 * map_grid_emit_obj(grid, p1=col, p2=row, p3=col_span, p4=row_span, a, b, ...).
 */
#include "collision.h"
#include "map_grid.h"
#include "t.h"

#include <stdlib.h>

/* a 10x10-cell grid (DIM0_PX = DIM1_PX = 32000). */
static uint8_t *grid10(void)
{
    uint8_t *g = map_grid_alloc();
    map_grid_set_dims(g, 10, 10);
    return g;
}

/* place a solid (class-10) wall: col span [col, col+ncol), row span [row, row+nrow). */
static void wall(uint8_t *g, int32_t col, int32_t row, int32_t ncol, int32_t nrow)
{
    map_grid_emit_obj(g, col, row, ncol, nrow, /*a*/ 10, /*b*/ 1, /*d8*/ 0, 0, 0);
}

/* A body in col 1 (x=4800, w=1000); the X-extent stays within col 1. */
static phys_box body_col1(int32_t y) { return (phys_box){ 4800, y, 1000, 1000, 0 }; }

/* drop onto a floor at row 5 (worldY 16000): bottom edge clamps so the body top
 * rests at 16000-1000 = 15000. */
int test_collision_drop_clamps_on_floor(void)
{
    uint8_t *g = grid10();
    wall(g, /*col*/ 0, /*row*/ 5, /*cols*/ 10, /*rows*/ 1);   /* full horizontal floor */
    phys_box b = body_col1(8000);
    int32_t out = -1;
    int r = collision_move_vertical(g, &b, +12000, 0, NULL, NULL, &out);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(out, 15000);
    map_grid_free(g);
    return 0;
}

/* no floor: the full downward delta clears, out = y + delta = 20000. */
int test_collision_open_air_clears(void)
{
    uint8_t *g = grid10();
    phys_box b = body_col1(8000);
    int32_t out = -1;
    int r = collision_move_vertical(g, &b, +12000, 0, NULL, NULL, &out);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(out, 20000);
    map_grid_free(g);
    return 0;
}

/* a wall NOT under the body (col 5) must not block the body in col 1. */
int test_collision_wall_off_axis_no_block(void)
{
    uint8_t *g = grid10();
    wall(g, /*col*/ 5, /*row*/ 5, 1, 1);
    phys_box b = body_col1(8000);
    int32_t out = -1;
    int r = collision_move_vertical(g, &b, +12000, 0, NULL, NULL, &out);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(out, 20000);
    map_grid_free(g);
    return 0;
}

/* moving up into a ceiling at row 2: top edge clamps, body top rests at the
 * ceiling bottom (row 3 top = 9600); the upward-block leaves out at the last
 * clear y. */
int test_collision_ceiling_blocks_upward(void)
{
    uint8_t *g = grid10();
    wall(g, /*col*/ 1, /*row*/ 2, 1, 1);
    phys_box b = body_col1(20000);
    int32_t out = -1;
    int r = collision_move_vertical(g, &b, -12000, 0, NULL, NULL, &out);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(out, 9600);
    map_grid_free(g);
    return 0;
}

/* delta 0 is an immediate clear with out unchanged from y. */
int test_collision_zero_delta_noop(void)
{
    uint8_t *g = grid10();
    wall(g, 0, 5, 10, 1);
    phys_box b = body_col1(8000);
    int32_t out = -1;
    int r = collision_move_vertical(g, &b, 0, 0, NULL, NULL, &out);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(out, 8000);
    map_grid_free(g);
    return 0;
}

/* slope branch: a cell with a nonzero region-B +0x8 slope ref invokes the
 * resolver callback (proves the slope path is wired; the live profile resolver
 * is PORT-DEBT(collision-slopes)). */
static int g_slope_calls;
static uint32_t g_slope_ref_seen;
static int slope_cb(void *ctx, uint32_t slope_ref, int subtile_x)
{
    (void)ctx; (void)subtile_x;
    g_slope_calls++;
    g_slope_ref_seen = slope_ref;
    return 0;   /* flat profile */
}

int test_collision_slope_callback_invoked(void)
{
    uint8_t *g = grid10();
    /* a sloped solid cell under the body: a=10, slope ref = the engine VA. */
    map_grid_emit_obj(g, 1, 5, 1, 1, /*a*/ 10, /*b*/ 1, /*d8*/ 0x005cc410u, 0, 0);
    phys_box b = body_col1(8000);
    int32_t out = -1;
    g_slope_calls = 0; g_slope_ref_seen = 0;
    (void)collision_move_vertical(g, &b, +12000, 0, slope_cb, NULL, &out);
    T_ASSERT(g_slope_calls > 0);
    T_ASSERT_EQ_U(g_slope_ref_seen, 0x005cc410u);
    map_grid_free(g);
    return 0;
}

/* ══ the HORIZONTAL mover (FUN_0054ded0 sweep + the 54db10 tile-half) ═══════
 * The ramp resolver mirrors the retail .rdata profiles byte-for-byte in SHAPE
 * (0x5cc430 = h(subx) = subx+1 ascending; 0x5cc410 = 32-subx descending) —
 * the live build reads the actual bytes off the user's exe (exe_data_bytes). */
static int ramp_cb(void *ctx, uint32_t slope_ref, int subtile_x)
{
    (void)ctx;
    if (slope_ref == 0x005cc430u) return subtile_x + 1;    /* rises rightward */
    if (slope_ref == 0x005cc410u) return 32 - subtile_x;   /* rises leftward  */
    return 0;
}

/* walking right into a solid col-2 wall: the sweep commits the cleared steps
 * (write-through to &body.x) and stops flush — right edge 6399 against the
 * wall at 6400 — returning blocked. */
int test_collision_hwalk_wall_stops_flush(void)
{
    uint8_t *g = grid10();
    wall(g, /*col*/ 2, /*row*/ 2, 1, 1);
    phys_box b = { 4800, 8000, 1000, 1000, 0 };
    int32_t x = -1, y = b.y;
    int r = collision_move_horizontal(g, &b, +2000, 0, 1, 0, NULL, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(x, 5400);          /* last clear step; 5400+999 == 6399     */
    T_ASSERT_EQ_I(y, 8000);          /* no y shift on a blocked sweep         */
    map_grid_free(g);
    return 0;
}

/* open ground, no flags: the full delta clears. */
int test_collision_hwalk_open_clears(void)
{
    uint8_t *g = grid10();
    phys_box b = { 4800, 8000, 1000, 1000, 0 };
    int32_t x = -1, y = b.y;
    int r = collision_move_horizontal(g, &b, +2000, 0, 0, 0, NULL, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(x, 6800);
    map_grid_free(g);
    return 0;
}

/* delta 0 short-circuits (54db10.c:32-34) with *inout_x defined. */
int test_collision_hwalk_zero_delta_noop(void)
{
    uint8_t *g = grid10();
    wall(g, 2, 2, 1, 1);
    phys_box b = { 4800, 8000, 1000, 1000, 0 };
    int32_t x = -1, y = b.y;
    int r = collision_move_horizontal(g, &b, 0, 1, 1, 0, NULL, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(x, 4800);
    map_grid_free(g);
    return 0;
}

/* CLIMB a 45-degree ramp (the stairs): floor across row 5, an ascending ramp
 * cell (class 10 + slope ref 0x5cc430) at col 2 row 4.  Walking right +800
 * from x=5300 (resting on the floor, bottom edge 15999): 2 flat steps, then 6
 * step-ups — one 100-unit rise per 100-unit run (hand-derived from the
 * per-sub-column solid predicate suby > 0x20 - h). */
int test_collision_hwalk_climbs_ramp(void)
{
    uint8_t *g = grid10();
    wall(g, 0, 5, 10, 1);                                     /* the floor    */
    map_grid_emit_obj(g, 2, 4, 1, 1, 10, 1, 0x005cc430u, 0, 0);  /* the ramp  */
    phys_box b = { 5300, 15000, 1000, 1000, 0 };
    int32_t x = -1, y = b.y;
    int r = collision_move_horizontal(g, &b, +800, 1, 1, 0, ramp_cb, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(x, 6100);
    T_ASSERT_EQ_I(y, 14400);         /* 6 x 100-unit risers climbed           */
    map_grid_free(g);
    return 0;
}

/* DESCEND the same ramp walking left (the H3 end state reversed): the floor
 * hug steps down 100 units per step while ground stays within 2 sub-rows,
 * landing back on the row-5 floor — the exact inverse of the climb. */
int test_collision_hwalk_descends_ramp(void)
{
    uint8_t *g = grid10();
    wall(g, 0, 5, 10, 1);
    map_grid_emit_obj(g, 2, 4, 1, 1, 10, 1, 0x005cc430u, 0, 0);
    phys_box b = { 6100, 14400, 1000, 1000, 0 };
    int32_t x = -1, y = b.y;
    int r = collision_move_horizontal(g, &b, -800, 1, 1, 0, ramp_cb, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(x, 5300);
    T_ASSERT_EQ_I(y, 15000);         /* hugged back down onto the floor       */
    map_grid_free(g);
    return 0;
}

/* the +0x14 top margin: a head-height wall (row 4) does not block a body
 * whose margin starts the scan below it (54db10.c:27 iVar14 = y+0x14), but
 * blocks the same body with margin 0. */
int test_collision_hwalk_margin_skips_head_row(void)
{
    uint8_t *g = grid10();
    wall(g, 2, 4, 1, 1);                 /* rows 12800..15999 (head height)   */
    phys_box tall = { 4800, 15000, 1000, 3000, 1300 };   /* scan 16300..17999 */
    int32_t x = -1, y = tall.y;
    int r = collision_move_horizontal(g, &tall, +2000, 0, 0, 0, NULL, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(x, 6800);

    phys_box nomargin = { 4800, 15000, 1000, 3000, 0 };  /* scan 15000..17999 */
    x = -1; y = nomargin.y;
    r = collision_move_horizontal(g, &nomargin, +2000, 0, 0, 0, NULL, NULL, &x, &y);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(x, 5400);
    map_grid_free(g);
    return 0;
}
