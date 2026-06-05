/*
 * test_parallax.c — host tests for the in-game parallax far-plane
 * (src/parallax.c; FUN_00490cd0 / FUN_00499560 + the 0x587e00 prologue
 * bank-selection switch).
 *
 * The decompiled arithmetic IS the spec: each test drives the pure selector /
 * renderer with known inputs and asserts the exact descriptor / emitted tiles.
 */
#include "parallax.h"
#include "map_grid.h"
#include "t.h"

#include <string.h>

/* ---- a recording blit sink ------------------------------------------------ */

typedef struct { uint16_t bank; int32_t frame, x, y; } px_rec;
typedef struct { px_rec r[64]; int n; } px_log;

static void rec_blit(void *ctx, uint16_t bank, int32_t frame, int32_t x, int32_t y)
{
    px_log *L = (px_log *)ctx;
    if (L->n < (int)(sizeof L->r / sizeof L->r[0])) {
        L->r[L->n].bank = bank; L->r[L->n].frame = frame;
        L->r[L->n].x = x; L->r[L->n].y = y;
    }
    L->n++;
}

/* ---- parallax_select (the 0x587e00 prologue switch) ----------------------- */

int test_parallax_select_town(void)
{
    /* room 210110 / area 0xd2 -> param_2 = 4 (room[0x44]=A), param_3 = 1
     * (room[0x43]=C); p3 normalises to 0 -> A-variant 0x55. */
    parallax_desc d;
    parallax_select(4, 1, &d);
    T_ASSERT_EQ_U(d.a_bank, 0x55);
    T_ASSERT_EQ_U(d.c.bank, 0x58);
    T_ASSERT_EQ_U(d.c.base_y, 0xf8);
    T_ASSERT_EQ_U(d.c.wrap, 8);
    T_ASSERT_EQ_I(d.c.para_y, 0xfa);
    T_ASSERT_EQ_U(d.b.bank, 0x59);
    T_ASSERT_EQ_U(d.b.base_y, 0xe0);
    T_ASSERT_EQ_U(d.b.wrap, 8);
    T_ASSERT_EQ_I(d.b.para_y, 0);
    return 0;
}

int test_parallax_select_param3_normalize(void)
{
    /* case 4 with param_3 = 0x3c -> p3 = 4; (p3<2 || 4<p3) is false -> A=0x56. */
    parallax_desc d;
    parallax_select(4, 0x3c, &d);
    T_ASSERT_EQ_U(d.a_bank, 0x56);
    /* case 4 with param_3 = 0x3d -> p3 = 5; (5<2 || 4<5) true -> A=0x55. */
    parallax_select(4, 0x3d, &d);
    T_ASSERT_EQ_U(d.a_bank, 0x55);
    return 0;
}

int test_parallax_select_case1_no_b(void)
{
    /* case 1: A by p3 (p3=0 -> 0x4e), C bank 0x57 baseY 0xa0, NO B layer. */
    parallax_desc d;
    parallax_select(1, 0, &d);
    T_ASSERT_EQ_U(d.a_bank, 0x4e);
    T_ASSERT_EQ_U(d.c.bank, 0x57);
    T_ASSERT_EQ_U(d.c.base_y, 0xa0);
    T_ASSERT_EQ_U(d.c.wrap, 8);
    T_ASSERT_EQ_I(d.c.para_y, 0);     /* case 1 sets no C paraY */
    T_ASSERT_EQ_U(d.b.bank, 0);       /* no B layer */
    /* p3=2 -> 0x4f, p3=5 -> 0x50 */
    parallax_select(1, 0x32, &d); T_ASSERT_EQ_U(d.a_bank, 0x4f);
    parallax_select(1, 0x3d, &d); T_ASSERT_EQ_U(d.a_bank, 0x50);
    return 0;
}

int test_parallax_select_a_only_and_default(void)
{
    parallax_desc d;
    /* case 0x11: A only (p3=0 -> 0x51) */
    parallax_select(0x11, 0, &d);
    T_ASSERT_EQ_U(d.a_bank, 0x51);
    T_ASSERT_EQ_U(d.c.bank, 0);
    T_ASSERT_EQ_U(d.b.bank, 0);
    /* default / special-render room (e.g. param_2 = 3): no parallax at all */
    parallax_select(3, 0, &d);
    T_ASSERT_EQ_U(d.a_bank, 0);
    T_ASSERT_EQ_U(d.c.bank, 0);
    T_ASSERT_EQ_U(d.b.bank, 0);
    return 0;
}

/* ---- grid front-header round-trip + exact offsets ------------------------- */

int test_parallax_grid_roundtrip(void)
{
    uint8_t *grid = map_grid_alloc();
    if (!grid) T_SKIP("map_grid_alloc failed");

    parallax_desc d;
    parallax_select(4, 1, &d);      /* the town descriptor */
    parallax_to_grid(grid, &d);

    /* exact bytes at the offsets the engine producers read */
    T_ASSERT_EQ_U(grid[PX_A_BANK], 0x55);
    T_ASSERT_EQ_U(grid[PX_C_BANK], 0x58);
    T_ASSERT_EQ_U(grid[PX_C_BASEY], 0xf8);
    T_ASSERT_EQ_U(grid[PX_C_WRAP], 8);
    T_ASSERT_EQ_U(grid[PX_C_PARAY], 0xfa);   /* low byte of 0xfa */
    T_ASSERT_EQ_U(grid[PX_B_BANK], 0x59);
    T_ASSERT_EQ_U(grid[PX_B_BASEY], 0xe0);
    T_ASSERT_EQ_U(grid[PX_B_WRAP], 8);
    T_ASSERT_EQ_U(grid[PX_B_PARAY], 0);

    parallax_desc back;
    parallax_from_grid(grid, &back);
    T_ASSERT_EQ_U(back.a_bank, d.a_bank);
    T_ASSERT_EQ_U(back.c.bank, d.c.bank);
    T_ASSERT_EQ_I(back.c.para_y, d.c.para_y);
    T_ASSERT_EQ_U(back.b.bank, d.b.bank);
    T_ASSERT_EQ_I(back.b.para_y, d.b.para_y);

    map_grid_free(grid);
    return 0;
}

/* ---- renderer: zero camera (no scroll, no vertical parallax) --------------- */

int test_parallax_render_zero_camera(void)
{
    parallax_desc d;
    parallax_select(4, 1, &d);
    mr_camera cam = { 0 };
    px_log L = { 0 };

    int n = parallax_render(&d, &cam, rec_blit, &L);
    T_ASSERT_EQ_I(n, 8 + 9 + 9);   /* A(8) + B(9) + C(9) */
    T_ASSERT_EQ_I(L.n, 26);

    /* layer A: 8 tiles, bank 0x55, frame i, x=i*0x50, y=0 */
    for (int i = 0; i < 8; i++) {
        T_ASSERT_EQ_U(L.r[i].bank, 0x55);
        T_ASSERT_EQ_I(L.r[i].frame, i);
        T_ASSERT_EQ_I(L.r[i].x, i * 0x50);
        T_ASSERT_EQ_I(L.r[i].y, 0);
    }
    /* layer B: 9 tiles, bank 0x59, frame (0+i)%8, x=i*0x50, y=baseY(0xe0) */
    for (int i = 0; i < 9; i++) {
        px_rec *r = &L.r[8 + i];
        T_ASSERT_EQ_U(r->bank, 0x59);
        T_ASSERT_EQ_I(r->frame, i % 8);
        T_ASSERT_EQ_I(r->x, i * 0x50);
        T_ASSERT_EQ_I(r->y, 0xe0);
    }
    /* layer C: 9 tiles, bank 0x58, frame i%8, x=i*0x50, y=baseY(0xf8) */
    for (int i = 0; i < 9; i++) {
        px_rec *r = &L.r[17 + i];
        T_ASSERT_EQ_U(r->bank, 0x58);
        T_ASSERT_EQ_I(r->frame, i % 8);
        T_ASSERT_EQ_I(r->x, i * 0x50);
        T_ASSERT_EQ_I(r->y, 0xf8);
    }
    return 0;
}

/* ---- renderer: the live-verified town first-frame camera ------------------ */

int test_parallax_render_town_first_frame(void)
{
    /* MAP_RENDER_CAM_TOWN_3F2: off60=128000, off5c=12800, off64/off68 set,
     * scroll subs 0.  Layer B (factor 0xfa): sx=320 -> col0=4, xoff=0;
     * paraY=0 -> yoff=0; y=0xe0.  Layer C (factor 500): sx=640 -> col0=8,
     * xoff=0; cam_y=12800 -> the vertical term = -128, *0xfa/1000 = -32,
     * clamped to -0x1c (-28); y = 0xf8 - 28 = 220. */
    parallax_desc d;
    parallax_select(4, 1, &d);
    mr_camera cam = { 0 };
    cam.off60 = 128000;
    cam.off5c = 12800;
    cam.off64 = 64000;
    cam.off68 = 48000;
    px_log L = { 0 };

    int n = parallax_render(&d, &cam, rec_blit, &L);
    T_ASSERT_EQ_I(n, 26);

    /* layer B[0]: col0=4 -> frame=(4+0)%8=4, x=0, y=224 */
    T_ASSERT_EQ_I(L.r[8].frame, 4);
    T_ASSERT_EQ_I(L.r[8].x, 0);
    T_ASSERT_EQ_I(L.r[8].y, 0xe0);
    /* layer B[5]: frame=(4+5)%8=1, x=5*0x50 */
    T_ASSERT_EQ_I(L.r[13].frame, 1);
    T_ASSERT_EQ_I(L.r[13].x, 5 * 0x50);

    /* layer C[0]: col0=8 -> frame=(8+0)%8=0, x=0, y=220 (clamped paraY) */
    T_ASSERT_EQ_U(L.r[17].bank, 0x58);
    T_ASSERT_EQ_I(L.r[17].frame, 0);
    T_ASSERT_EQ_I(L.r[17].x, 0);
    T_ASSERT_EQ_I(L.r[17].y, 220);
    return 0;
}

/* ---- renderer: edge cases ------------------------------------------------- */

int test_parallax_render_dry_and_empty(void)
{
    parallax_desc d;
    parallax_select(4, 1, &d);
    mr_camera cam = { 0 };

    /* dry walk (NULL blit) still counts every tile */
    T_ASSERT_EQ_I(parallax_render(&d, &cam, NULL, NULL), 26);

    /* a_bank == 0 -> layer A skipped (9+9 = 18) */
    d.a_bank = 0;
    px_log L = { 0 };
    int n = parallax_render(&d, &cam, rec_blit, &L);
    T_ASSERT_EQ_I(n, 18);
    T_ASSERT_EQ_U(L.r[0].bank, 0x59);   /* first emit is now layer B */

    /* a fully-empty descriptor (special-render room) emits nothing */
    parallax_select(3, 0, &d);
    L.n = 0;
    T_ASSERT_EQ_I(parallax_render(&d, &cam, rec_blit, &L), 0);
    T_ASSERT_EQ_I(L.n, 0);
    return 0;
}
