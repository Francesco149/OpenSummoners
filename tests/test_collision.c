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
static phys_box body_col1(int32_t y) { return (phys_box){ 4800, y, 1000, 1000 }; }

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
