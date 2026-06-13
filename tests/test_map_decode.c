/*
 * test_map_decode.c — host tests for the FUN_00587e00 per-tile-id dispatch
 * (src/map_decode.c).  The decompiled arms ARE the spec: each test drives one
 * town tile id/shape through map_decode_cell and asserts the exact region bytes
 * its recipe deposits.  The (id, shape) set covered here is exactly the one the
 * opening town exercises (tools/extract/map_data.py --cells, DATA 1022).
 *
 * A stub bank-dims callback reports a 1x1-tile footprint for every bank so the
 * base-tile placements land on a single deterministic cell.
 */
#include "map_decode.h"
#include "t.h"

static uint32_t rd_u32(const uint8_t *g, size_t off)
{ uint32_t v; memcpy(&v, g + off, 4); return v; }
static int32_t rd_i32(const uint8_t *g, size_t off)
{ int32_t v; memcpy(&v, g + off, 4); return v; }
static uint16_t rd_u16(const uint8_t *g, size_t off)
{ uint16_t v; memcpy(&v, g + off, 2); return v; }

static size_t cidx(int32_t p1, int32_t p2)
{ return (size_t)p1 * MG_ROW_PITCH + (size_t)p2; }

/* every bank rounds to a 1x1-tile footprint */
static void dims_1x1(void *ctx, uint16_t bank, int32_t *w, int32_t *h)
{ (void)ctx; (void)bank; *w = 1; *h = 1; }

/* a tiny in-memory map: dim0 x dim1 x dim2, owned zeroed cells. */
typedef struct { map_data m; uint8_t *cells; } fixture;

static void fx_init(fixture *f, uint32_t d0, uint32_t d1, uint32_t d2)
{
    memset(&f->m, 0, sizeof f->m);
    f->m.dim0 = d0; f->m.dim1 = d1; f->m.dim2 = d2;
    f->m.cells_len = (size_t)d0 * d1 * d2 * MD_CELL_SIZE;
    f->cells = (uint8_t *)calloc(f->m.cells_len, 1);
    f->m.cells = f->cells;
}
static void fx_free(fixture *f) { free(f->cells); }

static void fx_set(fixture *f, uint32_t x, uint32_t y, uint32_t z,
                   uint32_t tile_id, uint32_t shape, uint32_t arg_0c)
{
    uint8_t *p = f->cells + map_data_cell_index(&f->m, x, y, z) * MD_CELL_SIZE;
    memcpy(p + 0x04, &tile_id, 4);
    memcpy(p + 0x0c, &arg_0c, 4);
    memcpy(p + 0x10, &shape, 4);
}

/* region helpers */
static size_t recA(int32_t x, int32_t y, int slot)
{ return MG_REGION_A + (size_t)slot * 0x10 + cidx(x, y) * 0x40; }
static size_t recB(int32_t x, int32_t y) { return MG_REGION_B + cidx(x, y) * 0x10; }
static size_t recD(int32_t x, int32_t y) { return MG_REGION_D + cidx(x, y) * 2; }

/* assert region-A sub-slot s at (x,y) holds (bank, a2, flag) with dx/dy. */
static int chk_tile(const uint8_t *g, int32_t x, int32_t y, int s,
                    uint16_t bank, uint16_t a2, uint16_t flag,
                    int32_t dx, int32_t dy)
{
    size_t b = recA(x, y, s);
    T_ASSERT_EQ_U(rd_u16(g, b + 0x0), bank);
    T_ASSERT_EQ_U(rd_u16(g, b + 0x2), a2);
    T_ASSERT_EQ_U(rd_u16(g, b + 0x4), flag);
    T_ASSERT_EQ_I(rd_i32(g, b + 0x8), dx);
    T_ASSERT_EQ_I(rd_i32(g, b + 0xc), dy);
    return 0;
}

/* assert region-B record + region-D value at (x,y). */
static int chk_obj(const uint8_t *g, int32_t x, int32_t y,
                   uint16_t a, uint32_t d4, uint32_t d8, uint32_t dc, uint16_t dval)
{
    size_t b = recB(x, y);
    T_ASSERT_EQ_U(rd_u16(g, b + 0x0), a);
    T_ASSERT_EQ_U(rd_u32(g, b + 0x4), d4);
    T_ASSERT_EQ_U(rd_u32(g, b + 0x8), d8);
    T_ASSERT_EQ_U(rd_u32(g, b + 0xc), dc);
    T_ASSERT_EQ_U(rd_u16(g, recD(x, y)), dval);
    return 0;
}

/* ---- 0x1b58b / 0x1b58c : object block + base tile (bank 0x62, slot 3) ---- */

int test_map_decode_1b58b_shape0(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 2, 3, 0, 0x1b58b, 0, 0x18);
    map_decode_cell(&f.m, g, 2, 3, 0, NULL, dims_1x1, NULL);

    /* shape 0 -> 2 rows x 2 cols block at (2,3): rows r in {2,3}, cols in {3,4} */
    for (int32_t r = 2; r <= 3; r++)
        for (int32_t cc = 3; cc <= 4; cc++)
            if (chk_obj(g, r, cc, 10, 7, 0, 0, 1)) return 1;
    /* base tile: slot 3, bank 0x62, a2 0x18, flag default for slot 3 = 0x15 */
    if (chk_tile(g, 2, 3, 3, 0x62, 0x18, 0x15, 0, 0)) return 1;
    /* a cell outside the block is untouched */
    T_ASSERT_EQ_U(rd_u16(g, recB(4, 3) + 0x0), 0);

    map_grid_free(g); fx_free(&f);
    return 0;
}

int test_map_decode_1b58b_shape2(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 5, 5, 0, 0x1b58c, 2, 0x2); /* 0x1b58c shares the arm */
    map_decode_cell(&f.m, g, 5, 5, 0, NULL, dims_1x1, NULL);

    /* shape 2 -> 2 rows x 1 col at (5, 6): r in {5,6}, col 6 */
    if (chk_obj(g, 5, 6, 10, 7, 0, 0, 1)) return 1;
    if (chk_obj(g, 6, 6, 10, 7, 0, 0, 1)) return 1;
    T_ASSERT_EQ_U(rd_u16(g, recB(5, 5) + 0x0), 0); /* (5,5) not in block */
    if (chk_tile(g, 5, 5, 3, 0x62, 0x2, 0x15, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- 0x1b58d shape 2 : blend-pointer cluster + base tile (bank 0x63) ---- */

int test_map_decode_1b58d_shape2_blend(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 1, 1, 0, 0x1b58d, 2, 0x1);
    map_decode_cell(&f.m, g, 1, 1, 0, NULL, dims_1x1, NULL);

    /* the two blended cells carry b=2 in region D and the &DAT_005cc430 VA in
     * region B +0x8; the plain cells carry b=1 and 0. */
    if (chk_obj(g, 2, 3, 10, 5, MD_BLEND_5cc430, 0, 2)) return 1; /* x+1,y+2 */
    if (chk_obj(g, 3, 2, 10, 5, MD_BLEND_5cc430, 0, 2)) return 1; /* x+2,y+1 */
    if (chk_obj(g, 1, 4, 10, 7, 0, 0, 1)) return 1;              /* x,  y+3 */
    if (chk_obj(g, 3, 3, 10, 5, 0, 0, 1)) return 1;              /* x+2,y+2 */
    /* base tile bank 0x63 slot 3 */
    if (chk_tile(g, 1, 1, 3, 0x63, 0x1, 0x15, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- 0x1b58f / 0x29ff4 : optional fg tile (0x17a) + base ---- */

int test_map_decode_1b58f_shape0_base_only(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 3, 3, 0, 0x1b58f, 0, 0x0);
    map_decode_cell(&f.m, g, 3, 3, 0, NULL, dims_1x1, NULL);

    /* z==0 -> slot 1, flag 2; shape 0 -> no fg tile, base bank 0x176 */
    if (chk_tile(g, 3, 3, 1, 0x176, 0x0, 2, 0, 0)) return 1;
    T_ASSERT_EQ_U(rd_u16(g, recA(3, 3, 0) + 0x0), 0); /* no fg in slot 0 */

    map_grid_free(g); fx_free(&f);
    return 0;
}

int test_map_decode_1b58f_shape12_fg(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 4, 4, 0, 0x1b58f, 0xc, 0x9);
    map_decode_cell(&f.m, g, 4, 4, 0, NULL, dims_1x1, NULL);

    /* shape 0xc -> fg frame 2 at (4,4) slot 0; base 0x176 at (4,4) slot 1 */
    if (chk_tile(g, 4, 4, 0, 0x17a, 2, 2, 0, 0)) return 1;
    if (chk_tile(g, 4, 4, 1, 0x176, 0x9, 2, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

int test_map_decode_29ff4_shape14_z1(void)
{
    fixture f; fx_init(&f, 20, 20, 2);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 6, 6, 1, 0x29ff4, 0xe, 0x3);
    map_decode_cell(&f.m, g, 6, 6, 1, NULL, dims_1x1, NULL);

    /* z!=0 -> slot 2, flag 3; shape 0xe -> fg frame 4 at (6, y+1=7) slot 0;
     * base bank 0x177 at (6,6) slot 2 */
    if (chk_tile(g, 6, 7, 0, 0x17a, 4, 3, 0, 0)) return 1;
    if (chk_tile(g, 6, 6, 2, 0x177, 0x3, 3, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- the single-tile arms 0x1b5a0 / 0x1b5a9 / 0x1b5aa ---- */

int test_map_decode_single_tile_arms(void)
{
    fixture f; fx_init(&f, 20, 20, 2);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    /* 0x1b5a0: slot 2, bank 0x17b, flag 0xa */
    fx_set(&f, 1, 1, 0, 0x1b5a0, 0, 0xa);
    map_decode_cell(&f.m, g, 1, 1, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 1, 1, 2, 0x17b, 0xa, 0xa, 0, 0)) return 1;

    /* 0x1b5a9: slot 1, bank 0x172, flag default for slot 1 = 2 */
    fx_set(&f, 2, 2, 0, 0x1b5a9, 0, 0x0);
    map_decode_cell(&f.m, g, 2, 2, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 2, 2, 1, 0x172, 0x0, 2, 0, 0)) return 1;

    /* 0x1b5aa z==0: slot 1, flag 2, bank 0x173 */
    fx_set(&f, 3, 3, 0, 0x1b5aa, 0, 0x0);
    map_decode_cell(&f.m, g, 3, 3, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 3, 3, 1, 0x173, 0x0, 2, 0, 0)) return 1;

    /* 0x1b5aa z!=0: slot 2, flag 3 */
    fx_set(&f, 4, 4, 1, 0x1b5aa, 0, 0x0);
    map_decode_cell(&f.m, g, 4, 4, 1, NULL, dims_1x1, NULL);
    if (chk_tile(g, 4, 4, 2, 0x173, 0x0, 3, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- 0x1b5ab : tile (0x174) + 2x9 object block, no base tile ---- */

int test_map_decode_1b5ab(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    fx_set(&f, 2, 2, 0, 0x1b5ab, 0, 0x0);
    map_decode_cell(&f.m, g, 2, 2, 0, NULL, dims_1x1, NULL);

    /* tile: slot (z!=0)+1 = 1, bank 0x174, flag 0x14 */
    if (chk_tile(g, 2, 2, 1, 0x174, 0x0, 0x14, 0, 0)) return 1;
    /* 2 rows x 9 cols block at (3, 2): r in {3,4}, col in {2..10} */
    if (chk_obj(g, 3, 2, 10, 7, 0, 0, 1)) return 1;
    if (chk_obj(g, 4, 10, 10, 7, 0, 0, 1)) return 1;
    T_ASSERT_EQ_U(rd_u16(g, recB(5, 2) + 0x0), 0);  /* row 5 outside block */
    T_ASSERT_EQ_U(rd_u16(g, recB(3, 11) + 0x0), 0); /* col 11 outside block */

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- map_decode orchestration: dims, region-C pre-clear, region-E zero ---- */

int test_map_decode_full_small(void)
{
    fixture f; fx_init(&f, 4, 4, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);

    /* pre-fill region C and region E so we can see the clears happen. */
    for (int32_t y = 0; y < 4; y++)
        for (int32_t x = 0; x < 4; x++) {
            size_t e = MG_REGION_E + cidx(x, y) * MG_REGION_E_STRIDE;
            g[e] = 0xff; g[e + 1] = 0xff;
            size_t c = MG_REGION_C + cidx(x, y) * 0xc;
            g[c] = 0xff;
        }

    fx_set(&f, 1, 1, 0, 0x1b5a9, 0, 0x0); /* one easy cell */
    map_decode(&f.m, g, NULL, dims_1x1, NULL);

    /* dim header written */
    T_ASSERT_EQ_I(rd_i32(g, MG_DIM0), 4);
    T_ASSERT_EQ_I(rd_i32(g, MG_DIM1), 4);
    /* region C pre-cleared everywhere */
    for (int32_t y = 0; y < 4; y++)
        for (int32_t x = 0; x < 4; x++)
            T_ASSERT_EQ_U(rd_u32(g, MG_REGION_C + cidx(x, y) * 0xc), 0);
    /* region E co-id zeroed everywhere */
    for (int32_t y = 0; y < 4; y++)
        for (int32_t x = 0; x < 4; x++)
            T_ASSERT_EQ_U(rd_u16(g, MG_REGION_E + cidx(x, y) * MG_REGION_E_STRIDE), 0);
    /* the dispatched cell landed */
    if (chk_tile(g, 1, 1, 1, 0x172, 0x0, 2, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- the prologue cfg: param_4 tileset banks + param_3 scene-frame ---- */
int test_map_decode_cfg_init(void)
{
    map_decode_cfg c;
    /* param_4 = 1 (town/house) -> the 587e00.c:49-52 defaults */
    map_decode_cfg_init(&c, MAP_DECODE_SCENE_PARAM3, 1);
    T_ASSERT_EQ_U(c.bank_24, 0x17e); T_ASSERT_EQ_U(c.bank_1c, 0x17d);
    T_ASSERT_EQ_U(c.bank_20, 0x185); T_ASSERT_EQ_U(c.bank_18, 0x184);
    T_ASSERT_EQ_I(c.scene_frame, 0);   /* 0x14 normalizes to 0 */
    /* param_4 = 4 (errands) -> case 4: local_20/18 become 0x188/0x187 */
    map_decode_cfg_init(&c, MAP_DECODE_SCENE_PARAM3, 4);
    T_ASSERT_EQ_U(c.bank_24, 0x17e); T_ASSERT_EQ_U(c.bank_1c, 0x17d);
    T_ASSERT_EQ_U(c.bank_20, 0x188); T_ASSERT_EQ_U(c.bank_18, 0x187);
    /* the param_3 normalization ladder (587e00.c:64-80) */
    map_decode_cfg_init(&c, 10,   1); T_ASSERT_EQ_I(c.scene_frame, 1);
    map_decode_cfg_init(&c, 0x32, 1); T_ASSERT_EQ_I(c.scene_frame, 2);
    map_decode_cfg_init(&c, 0x3c, 1); T_ASSERT_EQ_I(c.scene_frame, 4);
    map_decode_cfg_init(&c, 0x3d, 1); T_ASSERT_EQ_I(c.scene_frame, 5);
    return 0;
}

/* ---- house arms (DATA 1023): 0x1b59f, 0x1b5b3, dir6 new banks ---- */
int test_map_decode_house_arms(void)
{
    fixture f; fx_init(&f, 20, 20, 2);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    /* 0x1b59f: slot 2, bank 0x17b, flag 0x14 (vs 0x1b5a0's flag 0xa) */
    fx_set(&f, 1, 1, 0, 0x1b59f, 0, 0x5);
    map_decode_cell(&f.m, g, 1, 1, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 1, 1, 2, 0x17b, 0x5, 0x14, 0, 0)) return 1;

    /* 0x1b5b3 shape!=1: slot 1, bank 0x18e, flag 2 */
    fx_set(&f, 2, 2, 0, 0x1b5b3, 0, 0x3);
    map_decode_cell(&f.m, g, 2, 2, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 2, 2, 1, 0x18e, 0x3, 2, 0, 0)) return 1;
    /* 0x1b5b3 shape==1: slot 2, flag 0x14 */
    fx_set(&f, 3, 3, 0, 0x1b5b3, 1, 0x3);
    map_decode_cell(&f.m, g, 3, 3, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 3, 3, 2, 0x18e, 0x3, 0x14, 0, 0)) return 1;

    /* dir6 new base banks: 0x29c0c->0x191 (shape 0xb fg frame 1), 0x29c02->0x190,
     * 0x29ffe->0x178 (shape 0 -> no fg). */
    fx_set(&f, 4, 4, 0, 0x29c0c, 0xb, 0x2);
    map_decode_cell(&f.m, g, 4, 4, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 4, 4, 0, 0x17a, 1, 2, 0, 0)) return 1;
    if (chk_tile(g, 4, 4, 1, 0x191, 0x2, 2, 0, 0)) return 1;
    fx_set(&f, 5, 5, 0, 0x29c02, 0, 0x1);
    map_decode_cell(&f.m, g, 5, 5, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 5, 5, 1, 0x190, 0x1, 2, 0, 0)) return 1;
    fx_set(&f, 6, 6, 0, 0x29ffe, 0, 0x1);
    map_decode_cell(&f.m, g, 6, 6, 0, NULL, dims_1x1, NULL);
    if (chk_tile(g, 6, 6, 1, 0x178, 0x1, 2, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- 0x2724 / 0x272e shape-switch blocks (house + errands) ---- */
int test_map_decode_block_arms(void)
{
    fixture f; fx_init(&f, 20, 20, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 20, 20);

    /* 0x2724 shape 7: emit_obj(x,y,1,1,10,1,0,1,0) + base tile 0x5d slot 3
     * (flag 0 -> slot-3 default 0x15). */
    fx_set(&f, 2, 2, 0, 0x2724, 7, 0x9);
    map_decode_cell(&f.m, g, 2, 2, 0, NULL, dims_1x1, NULL);
    if (chk_obj(g, 2, 2, 10, 1, 0, 0, 1)) return 1;
    if (chk_tile(g, 2, 2, 3, 0x5d, 0x9, 0x15, 0, 0)) return 1;

    /* 0x272e shape 2: 3-obj cluster (two blended) + base tile 0x60 slot 3. */
    fx_set(&f, 8, 8, 0, 0x272e, 2, 0x4);
    map_decode_cell(&f.m, g, 8, 8, 0, NULL, dims_1x1, NULL);
    if (chk_obj(g, 8, 9, 10, 1, MD_BLEND_5cc390, 0, 2)) return 1; /* x,y+1 b=2 */
    if (chk_obj(g, 9, 9, 10, 1, MD_BLEND_5cc3b0, 0, 1)) return 1; /* x+1,y+1 b=1 */
    if (chk_tile(g, 8, 8, 3, 0x60, 0x4, 0x15, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* ---- errands arms (DATA 1025, param_4=4): the 113xxx tiles + local banks ---- */
int test_map_decode_errands_arms(void)
{
    fixture f; fx_init(&f, 40, 40, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 40, 40);
    map_decode_cfg cfg; map_decode_cfg_init(&cfg, MAP_DECODE_SCENE_PARAM3, 4);

    /* 0x1b97c floor: slot 2, bank 0x17c, flag 0x14, frame = scene_frame (0,
     * NOT arg_0c); shape 0 -> default obj emit_obj(x,y,2,1,10,1,0,6,0). */
    fx_set(&f, 1, 1, 0, 0x1b97c, 0, 0x9);
    map_decode_cell(&f.m, g, 1, 1, 0, &cfg, dims_1x1, NULL);
    if (chk_tile(g, 1, 1, 2, 0x17c, 0, 0x14, 0, 0)) return 1;
    if (chk_obj(g, 1, 1, 10, 6, 0, 0, 1)) return 1;

    /* 0x1b972 wall: slot 1, bank local_1c=0x17d, flag 2 */
    fx_set(&f, 5, 5, 0, 0x1b972, 0, 0x0);
    map_decode_cell(&f.m, g, 5, 5, 0, &cfg, dims_1x1, NULL);
    if (chk_tile(g, 5, 5, 1, 0x17d, 0, 2, 0, 0)) return 1;
    /* 0x1b977: bank local_18=0x187 */
    fx_set(&f, 7, 7, 0, 0x1b977, 0, 0x0);
    map_decode_cell(&f.m, g, 7, 7, 0, &cfg, dims_1x1, NULL);
    if (chk_tile(g, 7, 7, 1, 0x187, 0, 2, 0, 0)) return 1;

    /* 0x1b986: bank local_24=0x17e slot 1 flag 2 (frame = arg_0c here) */
    fx_set(&f, 9, 9, 0, 0x1b986, 0, 0x4);
    map_decode_cell(&f.m, g, 9, 9, 0, &cfg, dims_1x1, NULL);
    if (chk_tile(g, 9, 9, 1, 0x17e, 0x4, 2, 0, 0)) return 1;
    /* 0x1b98b: bank local_20=0x188 slot 1 flag 2 */
    fx_set(&f, 11, 11, 0, 0x1b98b, 0, 0x4);
    map_decode_cell(&f.m, g, 11, 11, 0, &cfg, dims_1x1, NULL);
    if (chk_tile(g, 11, 11, 1, 0x188, 0x4, 2, 0, 0)) return 1;
    /* 0x1b990: bank local_24=0x17e slot 2 flag 0x15 */
    fx_set(&f, 13, 13, 0, 0x1b990, 0, 0x4);
    map_decode_cell(&f.m, g, 13, 13, 0, &cfg, dims_1x1, NULL);
    if (chk_tile(g, 13, 13, 2, 0x17e, 0x4, 0x15, 0, 0)) return 1;

    map_grid_free(g); fx_free(&f);
    return 0;
}

/* empty / unhandled cells are a no-op */
int test_map_decode_empty_cell_noop(void)
{
    fixture f; fx_init(&f, 8, 8, 1);
    uint8_t *g = map_grid_alloc(); T_ASSERT(g != NULL);
    map_grid_set_dims(g, 8, 8);

    /* empty cell (tile_id 0) */
    map_decode_cell(&f.m, g, 2, 2, 0, NULL, dims_1x1, NULL);
    /* an id not in the town set */
    fx_set(&f, 3, 3, 0, 0x1bd00, 0, 0);
    map_decode_cell(&f.m, g, 3, 3, 0, NULL, dims_1x1, NULL);

    /* neither cell touched any per-cell region */
    for (int dxy = 2; dxy <= 3; dxy++) {
        for (int s = 0; s < 4; s++)
            T_ASSERT_EQ_U(rd_u16(g, recA(dxy, dxy, s) + 0x0), 0);
        T_ASSERT_EQ_U(rd_u16(g, recB(dxy, dxy) + 0x0), 0);
        T_ASSERT_EQ_U(rd_u16(g, recD(dxy, dxy)), 0);
    }
    map_grid_free(g); fx_free(&f);
    return 0;
}
