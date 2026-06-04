/*
 * test_map_render.c — host tests for the static tilemap render-walk geometry
 * (FUN_00490f30, src/map_render.c).
 *
 * The decompiled arithmetic IS the spec: each test drives the pure geometry
 * with known inputs and asserts the exact visible window / grid index /
 * draw-node geometry FUN_00490f30 computes.  The tile-read tests populate a
 * real map_grid buffer through the ported write primitive (map_grid_emit_tile)
 * and assert map_render reads the same bytes back — i.e. the writer (0x58c910)
 * and the reader (0x490f30) agree on region A's layout.
 */
#include "map_render.h"
#include "map_grid.h"
#include "t.h"

/* ---- visible-window arithmetic (490f30.c:40-54) ---------------------------- */

int test_map_render_window_basic(void)
{
    /* off60=5 tiles, off34=0 -> col0 = 16000/3200 - 1 = 4.
     * off64=10 tiles -> cap_c = 10 + 2 = 12 (<= 88-4) -> ncols = 12.
     * off5c=3 tiles -> row0 = 9600/3200 - 1 = 2.
     * off68=6 tiles -> cap_r = 6 + 2 = 8 (<= 19-2) -> nrows = 8. */
    mr_camera cam = { 0 };
    cam.off60 = 3200 * 5;
    cam.off64 = 3200 * 10;
    cam.off5c = 3200 * 3;
    cam.off68 = 3200 * 6;

    mr_window w;
    map_render_visible_window(&cam, 88, 19, &w);
    T_ASSERT_EQ_I(w.col0, 4);
    T_ASSERT_EQ_I(w.ncols, 12);
    T_ASSERT_EQ_I(w.row0, 2);
    T_ASSERT_EQ_I(w.nrows, 8);
    return 0;
}

int test_map_render_window_clamps_origin(void)
{
    /* Zero camera -> origin = 0/3200 - 1 = -1, clamped to 0 on both axes.
     * Small viewport caps the counts: off64/off68 = 1 tile -> cap = 3. */
    mr_camera cam = { 0 };
    cam.off64 = 3200 * 1;
    cam.off68 = 3200 * 1;

    mr_window w;
    map_render_visible_window(&cam, 88, 19, &w);
    T_ASSERT_EQ_I(w.col0, 0);
    T_ASSERT_EQ_I(w.row0, 0);
    T_ASSERT_EQ_I(w.ncols, 3);   /* min(88-0, 1+2) */
    T_ASSERT_EQ_I(w.nrows, 3);   /* min(19-0, 1+2) */
    return 0;
}

int test_map_render_window_no_cap_when_near_edge(void)
{
    /* col0 close to dim0 so (dim0 - col0) < cap -> the cap branch is NOT taken
     * and ncols stays the smaller remaining-cells value.
     * off60=9 tiles -> col0 = 28800/3200 - 1 = 8; dim0=10 -> ncols = 2 (< 12). */
    mr_camera cam = { 0 };
    cam.off60 = 3200 * 9;
    cam.off64 = 3200 * 10;       /* cap_c would be 12, but 12 <= 2 is false */
    cam.off5c = 3200 * 7;        /* row0 = 22400/3200 - 1 = 6; dim1=8 -> nrows 2 */
    cam.off68 = 3200 * 10;

    mr_window w;
    map_render_visible_window(&cam, 10, 8, &w);
    T_ASSERT_EQ_I(w.col0, 8);
    T_ASSERT_EQ_I(w.ncols, 2);
    T_ASSERT_EQ_I(w.row0, 6);
    T_ASSERT_EQ_I(w.nrows, 2);
    return 0;
}

int test_map_render_window_row_sum_components(void)
{
    /* Exercise all three row-origin components, incl. the x100 scale on off74:
     * off5c=1280, off74=20 (-> 2000), off4c=2720; sum = 1280+2000+2720 = 6000?
     * Use round numbers: off5c=3200, off74=32 (->3200), off4c=3200 -> sum 9600
     * -> row0 = 9600/3200 - 1 = 2. */
    mr_camera cam = { 0 };
    cam.off5c = 3200;
    cam.off74 = 32;       /* * 100 = 3200 */
    cam.off4c = 3200;
    cam.off68 = 3200 * 100;   /* large cap so nrows = dim1 - row0 */

    mr_window w;
    map_render_visible_window(&cam, 88, 50, &w);
    T_ASSERT_EQ_I(w.row0, 2);
    T_ASSERT_EQ_I(w.nrows, 48);   /* 50 - 2 */
    return 0;
}

/* ---- grid index (490f30.c:64) --------------------------------------------- */

int test_map_render_grid_index(void)
{
    T_ASSERT_EQ_U(map_render_grid_index(4, 2), 0x202u);   /* 4*128 + 2 */
    T_ASSERT_EQ_U(map_render_grid_index(0, 0), 0u);
    T_ASSERT_EQ_U(map_render_grid_index(88, 18), 11282u); /* 88*128 + 18 */
    return 0;
}

/* ---- tile read: writer (emit_tile) <-> reader (map_render_tile) agree ------ */

int test_map_render_tile_basic(void)
{
    uint8_t *g = map_grid_alloc();
    if (!g) T_SKIP("grid alloc failed");
    map_grid_set_dims(g, 88, 19);

    /* Emit a single 1x1 tile at cell (col=4, row=2), sub-slot 1, bank 0x62,
     * frame 0x07, flag defaulted (slot 1 -> 2). */
    map_grid_emit_tile(g, 4, 2, 1, 0x62, 0x07, 0, 1, 1, NULL, NULL);

    mr_tile t;
    int rc = map_render_tile(g, 4, 2, 1, &t);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_U(t.bank, 0x62u);
    T_ASSERT_EQ_U(t.frame, 0x07u);
    T_ASSERT_EQ_U(t.layer, 0x02u);    /* slot-1 default flag */
    T_ASSERT_EQ_I(t.dst_x, 4 * 3200);
    T_ASSERT_EQ_I(t.dst_y, 2 * 3200);
    T_ASSERT_EQ_I(t.src_x, 0);        /* dx=0 -> (0*3200)/100 */
    T_ASSERT_EQ_I(t.src_y, 0);
    T_ASSERT_EQ_I(t.w, 0x20);
    T_ASSERT_EQ_I(t.h, 0x20);

    /* A different sub-slot of the same cell is empty -> no node. */
    T_ASSERT_EQ_I(map_render_tile(g, 4, 2, 0, &t), 0);

    map_grid_free(g);
    return 0;
}

int test_map_render_tile_empty_cell(void)
{
    uint8_t *g = map_grid_alloc();
    if (!g) T_SKIP("grid alloc failed");
    map_grid_set_dims(g, 88, 19);

    mr_tile t;
    /* Untouched cell -> bank 0 -> returns 0, out left untouched. */
    T_ASSERT_EQ_I(map_render_tile(g, 10, 5, 0, &t), 0);
    map_grid_free(g);
    return 0;
}

int test_map_render_tile_footprint_suboffset(void)
{
    uint8_t *g = map_grid_alloc();
    if (!g) T_SKIP("grid alloc failed");
    map_grid_set_dims(g, 88, 19);

    /* A 2x2 footprint at (p1=4, p2=2): emit_tile writes the (dx,dy) sub-tile
     * offsets into region A +0x8/+0xc per cell.  The far corner cell (5,3)
     * carries dx=1,dy=1 -> source offset (32,32). */
    map_grid_emit_tile(g, 4, 2, 2, 0x174, 0, 0, 2, 2, NULL, NULL);

    mr_tile t;
    int rc = map_render_tile(g, 5, 3, 2, &t);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_U(t.bank, 0x174u);
    T_ASSERT_EQ_I(t.dst_x, 5 * 3200);
    T_ASSERT_EQ_I(t.dst_y, 3 * 3200);
    T_ASSERT_EQ_I(t.src_x, 32);    /* (1 * 3200) / 100 */
    T_ASSERT_EQ_I(t.src_y, 32);

    /* The origin cell (4,2) of the same footprint has dx=dy=0 -> source (0,0). */
    rc = map_render_tile(g, 4, 2, 2, &t);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(t.src_x, 0);
    T_ASSERT_EQ_I(t.src_y, 0);

    map_grid_free(g);
    return 0;
}
