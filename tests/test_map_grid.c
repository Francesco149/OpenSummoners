/*
 * test_map_grid.c — host tests for the runtime render-grid write primitives
 * (FUN_0054c970 / FUN_0058ca80 / FUN_0058c910, src/map_grid.c).
 *
 * The decompiled arithmetic IS the spec here: each test drives a primitive with
 * known inputs and asserts the exact bytes it deposits at the computed region
 * offsets (and that out-of-range coords write nothing).  No retail capture is
 * needed — these are pure memory transforms; the in-game render that *reads*
 * the grid (FUN_005a00c0) is a separate, later unit.
 */
#include "map_grid.h"
#include "t.h"

static uint32_t rd_u32(const uint8_t *g, size_t off)
{
    uint32_t v;
    memcpy(&v, g + off, 4);
    return v;
}
static int32_t rd_i32(const uint8_t *g, size_t off)
{
    int32_t v;
    memcpy(&v, g + off, 4);
    return v;
}
static uint16_t rd_u16(const uint8_t *g, size_t off)
{
    uint16_t v;
    memcpy(&v, g + off, 2);
    return v;
}

static size_t cell_idx(int32_t p1, int32_t p2)
{
    return (size_t)p1 * MG_ROW_PITCH + (size_t)p2;
}

int test_map_grid_set_dims(void)
{
    uint8_t *g = map_grid_alloc();
    T_ASSERT(g != NULL);
    map_grid_set_dims(g, 88, 19);
    T_ASSERT_EQ_I(rd_i32(g, MG_DIM0), 88);
    T_ASSERT_EQ_I(rd_i32(g, MG_DIM1), 19);
    T_ASSERT_EQ_I(rd_i32(g, MG_DIM0_PX), 88 * 0xc80);
    T_ASSERT_EQ_I(rd_i32(g, MG_DIM1_PX), 19 * 0xc80);
    map_grid_free(g);
    return 0;
}

int test_map_grid_clear_cell(void)
{
    uint8_t *g = map_grid_alloc();
    T_ASSERT(g != NULL);
    map_grid_set_dims(g, 88, 19);

    /* (p1,p2) = (3,5) -> idx = 3*0x80 + 5 = 0x185; base = REGION_C + idx*0xc */
    map_grid_clear_cell(g, 3, 5, 0xdeadbeef, 0x12345678, 0xabcd);
    size_t base = MG_REGION_C + cell_idx(3, 5) * 0xc;
    T_ASSERT_EQ_U(rd_u32(g, base + 0x0), 0xdeadbeef);
    T_ASSERT_EQ_U(rd_u32(g, base + 0x4), 0x12345678);
    T_ASSERT_EQ_U(rd_u16(g, base + 0x8), 0xabcd);

    /* the clear is guarded by the PIXEL dims (dim*0xc80) — coords well inside
     * the pixel extent but beyond the tile dims still write (matches retail). */
    map_grid_clear_cell(g, 87, 18, 1, 2, 3);
    size_t b2 = MG_REGION_C + cell_idx(87, 18) * 0xc;
    T_ASSERT_EQ_U(rd_u32(g, b2 + 0x0), 1);

    /* negative coord: no-op (region stays zero) */
    map_grid_clear_cell(g, -1, 0, 0xff, 0xff, 0xff);
    T_ASSERT_EQ_U(rd_u32(g, MG_REGION_C + 0), 0);
    map_grid_free(g);
    return 0;
}

int test_map_grid_emit_obj_block(void)
{
    uint8_t *g = map_grid_alloc();
    T_ASSERT(g != NULL);
    map_grid_set_dims(g, 88, 19);

    /* 2 rows x 3 cols starting at (10, 4).  Args mirror FUN_0058ca80:
     * a(=p5,u16) b(=p6,u16) d8(=p7) d4(=p8) dc(=p9). */
    map_grid_emit_obj(g, 10, 4, /*rows*/2, /*cols*/3,
                      /*a*/0x0010, /*b*/0x0c0c,
                      /*d8*/0x88888888, /*d4*/0x44444444, /*dc*/0xcccccccc);

    for (int32_t r = 10; r < 12; r++) {
        for (int32_t c = 4; c < 7; c++) {
            size_t idx  = cell_idx(r, c);
            size_t recB = MG_REGION_B + idx * 0x10;
            T_ASSERT_EQ_U(rd_u16(g, recB + 0x0), 0x0010);
            T_ASSERT_EQ_U(rd_u32(g, recB + 0x4), 0x44444444); /* d4 = p8 */
            T_ASSERT_EQ_U(rd_u32(g, recB + 0x8), 0x88888888); /* d8 = p7 */
            T_ASSERT_EQ_U(rd_u32(g, recB + 0xc), 0xcccccccc); /* dc = p9 */
            T_ASSERT_EQ_U(rd_u16(g, MG_REGION_D + idx * 2), 0x0c0c);
        }
    }
    /* a cell just outside the block is untouched */
    T_ASSERT_EQ_U(rd_u16(g, MG_REGION_B + cell_idx(12, 4) * 0x10 + 0x0), 0);
    map_grid_free(g);
    return 0;
}

int test_map_grid_emit_obj_clamps_dims(void)
{
    uint8_t *g = map_grid_alloc();
    T_ASSERT(g != NULL);
    map_grid_set_dims(g, 88, 19); /* col bound = dim1 = 19 */

    /* request 1x4 cols starting at col 17 -> cols 17,18 valid; 19,20 clamped. */
    map_grid_emit_obj(g, 0, 17, 1, 4, 0x55, 0x66, 0, 0, 0);
    T_ASSERT_EQ_U(rd_u16(g, MG_REGION_B + cell_idx(0, 17) * 0x10), 0x55);
    T_ASSERT_EQ_U(rd_u16(g, MG_REGION_B + cell_idx(0, 18) * 0x10), 0x55);
    T_ASSERT_EQ_U(rd_u16(g, MG_REGION_B + cell_idx(0, 19) * 0x10), 0); /* clamped */
    map_grid_free(g);
    return 0;
}

int test_map_grid_emit_tile_explicit_span(void)
{
    uint8_t *g = map_grid_alloc();
    T_ASSERT(g != NULL);
    map_grid_set_dims(g, 88, 19);

    /* slot 2, explicit span 2 rows x 2 cols at (5,3), flag defaulted (=0). */
    map_grid_emit_tile(g, 5, 3, /*slot*/2, /*bank*/0x1b58b, /*a2*/0x2222,
                       /*flag*/0, /*span_rows*/2, /*span_cols*/2, NULL, NULL);

    for (int32_t dx = 0; dx < 2; dx++) {
        for (int32_t dy = 0; dy < 2; dy++) {
            size_t idx  = cell_idx(5 + dx, 3 + dy);
            size_t base = MG_REGION_A + (size_t)2 * 0x10 + idx * 0x40;
            T_ASSERT_EQ_U(rd_u16(g, base + 0x0), (uint16_t)0x1b58b); /* bank */
            T_ASSERT_EQ_U(rd_u16(g, base + 0x2), 0x2222);            /* a2   */
            T_ASSERT_EQ_U(rd_u16(g, base + 0x4), 0x14);              /* slot 2 default */
            T_ASSERT_EQ_I(rd_i32(g, base + 0x8), dx);
            T_ASSERT_EQ_I(rd_i32(g, base + 0xc), dy);
        }
    }
    map_grid_free(g);
    return 0;
}

/* A bank whose pixel size rounds to 2 tiles wide x 3 tiles tall. */
static void bank_dims_2x3(void *ctx, uint16_t bank_id, int32_t *w, int32_t *h)
{
    (void)ctx;
    (void)bank_id;
    *w = 64; /* ceil(64/32) = 2 */
    *h = 96; /* ceil(96/32) = 3 */
}

int test_map_grid_emit_tile_bank_span(void)
{
    uint8_t *g = map_grid_alloc();
    T_ASSERT(g != NULL);
    map_grid_set_dims(g, 88, 19);

    /* slot 0, no explicit span -> derive 2x3 from the bank dims. */
    map_grid_emit_tile(g, 0, 0, /*slot*/0, /*bank*/7, /*a2*/0x9, /*flag*/0,
                       /*span_rows*/0, /*span_cols*/0, bank_dims_2x3, NULL);

    int written = 0;
    for (int32_t dx = 0; dx < 2; dx++) {
        for (int32_t dy = 0; dy < 3; dy++) {
            size_t idx  = cell_idx(dx, dy);
            size_t base = MG_REGION_A + idx * 0x40; /* slot 0 */
            T_ASSERT_EQ_U(rd_u16(g, base + 0x0), 7);
            T_ASSERT_EQ_U(rd_u16(g, base + 0x4), 2); /* slot 0/1 default flag */
            T_ASSERT_EQ_I(rd_i32(g, base + 0x8), dx);
            T_ASSERT_EQ_I(rd_i32(g, base + 0xc), dy);
            written++;
        }
    }
    T_ASSERT_EQ_I(written, 6);
    /* one cell beyond the footprint is untouched */
    T_ASSERT_EQ_U(rd_u16(g, MG_REGION_A + cell_idx(2, 0) * 0x40), 0);

    /* bank == 0 -> 1x1 footprint regardless of dims */
    map_grid_set_dims(g, 88, 19);
    map_grid_emit_tile(g, 20, 1, 1, /*bank*/0, 0x1, 0, 0, 0, bank_dims_2x3, NULL);
    size_t b0 = MG_REGION_A + (size_t)1 * 0x10 + cell_idx(20, 1) * 0x40;
    T_ASSERT_EQ_U(rd_u16(g, b0 + 0x4), 2); /* slot 1 default flag */
    T_ASSERT_EQ_U(rd_u16(g, MG_REGION_A + (size_t)1 * 0x10 + cell_idx(21, 1) * 0x40 + 0x4), 0);
    map_grid_free(g);
    return 0;
}
