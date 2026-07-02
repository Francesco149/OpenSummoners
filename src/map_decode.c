/* map_decode.c — FUN_00587e00 per-tile-id placement dispatch.  See map_decode.h.
 *
 * Each arm below is a faithful transcription of the matching arm in
 * docs/decompiled/by-address/587e00.c.  The decompiler's variable names are
 * noted where helpful: iVar17 = x, param_2 = y, local_28 = z; uVar23 = cell+0x0c
 * (the emit_tile a2 index); the shared tail LAB_0058c3b9 issues the final
 * map_grid_emit_tile.  FUN_0058ca80 -> map_grid_emit_obj and FUN_0058c910 ->
 * map_grid_emit_tile both keep the decomp argument order (emit_tile drops the
 * two unused params 9/10; see map_grid.c).
 *
 * COVERAGE (ckpt 130): the town (DATA 1022) + the house (1023) + errands (1025)
 * rooms — the cutscene room chain (arrival 0x334be -> house 0x334c8 -> errands
 * 0x334dc), all area 0xd2.  The decode arms + their exact (bank, slot, flag) are
 * ground-truthed against a retail emit-sequence capture (runs/room-render-gt,
 * hooks on 0x58c910/0x58ca80 across the chain) cross-referenced with the cell
 * (tile id, shape) histograms (tools/extract/map_data.py --cells).
 *
 * The 113xxx auto-footprint floor/wall tiles (0x1b97c/72/77) are written by
 * retail as an INLINED grid-rectangle loop (587e00.c:2039.. / :1527.. /
 * :1577..) sized by the bank's pixel dims; that is exactly what
 * map_grid_emit_tile already does (span 0/0 -> dims footprint, writing dx/dy per
 * sub-tile), so the port emits them as one emit_tile each (verified equivalent
 * by the region-A byte offsets: 0x1b97c writes sub-slot 2 at 0x30+2*0x10).
 * Their shape-1/2 blocks (LAB_00589520 / 0x1b97c case 1 — region B class-10 +
 * region D=1, no visible tile) are ported: they are invisible COLLISION WALLS
 * (the errands left wall; ckpt 175 retired PORT-DEBT(decode-occlusion-mark),
 * which had misread them as culling marks).
 */
#include "map_decode.h"

#include <string.h>

void map_decode_cfg_init(map_decode_cfg *cfg, int param_3, int param_4)
{
    /* 587e00.c:64-80 param_3 normalization -> the 113xxx tile frame (0 for the
     * town-area rooms; MAP_DECODE_SCENE_PARAM3=0x14 normalizes to 0). */
    int sf;
    switch (param_3) {
    case 10: case 0x28: sf = 1; break;
    case 0x32:          sf = 2; break;
    case 0x3c:          sf = 4; break;
    case 0x3d:          sf = 5; break;
    default:            sf = 0; break;
    }
    cfg->scene_frame = (int16_t)sf;

    /* 587e00.c:49-52 defaults (param_4 not in {2,3,4,5..8}, e.g. 1 = town/house). */
    cfg->bank_24 = 0x17e;
    cfg->bank_1c = 0x17d;
    cfg->bank_20 = 0x185;
    cfg->bank_18 = 0x184;
    /* the param_4 = room[0x43] switch (587e00.c:197-214). */
    switch (param_4) {
    case 2:  cfg->bank_24 = 0x183; cfg->bank_1c = 0x182;
             cfg->bank_20 = 0x185; cfg->bank_18 = 0x184; break;
    case 3:  cfg->bank_24 = 0x181; cfg->bank_1c = 0x180;
             cfg->bank_20 = 0x188; cfg->bank_18 = 0x187; break;
    case 4:  cfg->bank_24 = 0x17e; cfg->bank_1c = 0x17d;
             cfg->bank_20 = 0x188; cfg->bank_18 = 0x187; break;
    default: break;   /* 1 (town/house) -> the defaults above; 5-8 unmodeled */
    }
}

/* 0x1b58f / 0x29ff4 / 0x29ffe / 0x29c02 / 0x29c0c share one arm: an optional
 * foreground tile (bank 0x17a, frame chosen by the shape) plus a base tile whose
 * bank differs per id.  587e00.c:2255-2311 (base 0x176), :2632-2671 (base 0x177),
 * :2589-2628 (0x178), :2674-2714 (0x191), :2716-2752 (0x190). */
static void decode_dir6_tile(uint8_t *grid, uint32_t x, uint32_t y, uint32_t z,
                             const map_cell *c, uint32_t base_bank,
                             mg_bank_dims_fn dims, void *ctx)
{
    int16_t flag;       /* uVar14 */
    int32_t slot;       /* cVar5  */
    if (z == 0) { flag = 2; slot = 1; }
    else        { flag = 3; slot = 2; }

    int32_t ox = (int32_t)x, oy = (int32_t)y; /* iVar6 / iVar15 */
    int32_t frame;                            /* uVar20 */
    int have_fg = 1;
    switch (c->shape) {
    case 10:   frame = 0; break;
    case 0xb:  frame = 1; break;
    case 0xc:  frame = 2; break;
    case 0xd:  frame = 3; ox = (int32_t)x + 4; oy = (int32_t)y + 2; break;
    case 0xe:  frame = 4; oy = (int32_t)y + 1; break;
    case 0xf:  frame = 5; break;
    default:   frame = 0; have_fg = 0; break;
    }
    if (have_fg)
        map_grid_emit_tile(grid, ox, oy, 0, 0x17a, (uint16_t)frame, flag,
                           0, 0, dims, ctx);

    map_grid_emit_tile(grid, (int32_t)x, (int32_t)y, slot, base_bank,
                       (uint16_t)c->arg_0c, flag, 0, 0, dims, ctx);
}

/* 587e00.c switchD_005887cc_caseD_2724 (0x2724 / 0x2738, :3206-3280): a
 * shape-switch object block (the obj geometry varies per shape) + a base tile
 * (bank 0x5d, slot 3).  emit_obj(iVar15, iVar6, uVar14, uVar20, 10, 1, 0, 1, 0). */
static void decode_block_2724(uint8_t *grid, int32_t ix, int32_t iy,
                              const map_cell *c, mg_bank_dims_fn dims, void *ctx)
{
    int32_t bx = ix, by = iy;     /* iVar15, iVar6 */
    int32_t p3 = 0, p4 = 0;       /* uVar14, uVar20 */
    int do_block = 1;
    switch (c->shape) {
    case 0:   p4 = 2; p3 = 2; break;
    case 1:   p4 = 1; p3 = 1; bx = ix + 1; by = iy + 1; break;
    case 2:   p4 = 1; by = iy + 1; p3 = 2; break;
    case 3:   p4 = 1; by = iy + 1; p3 = 1; break;
    case 4:   map_grid_emit_obj(grid, ix, iy, 1, 1, 10, 1, 0, 1, 0);
              p4 = 1; by = iy + 1; p3 = 2; break;
    case 5:   p4 = 2; p3 = 1; break;
    case 6:   map_grid_emit_obj(grid, ix, iy + 1, 1, 1, 10, 1, 0, 1, 0);
              p4 = 1; p3 = 2; break;
    case 7:   p4 = 1; p3 = 1; break;
    case 8:   p4 = 1; p3 = 2; break;
    case 9:   p4 = 1; p3 = 1; bx = ix + 1; break;
    case 10:  map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, 0, 1, 0);
              p4 = 1; p3 = 2; break;
    case 0xb: p4 = 2; p3 = 1; bx = ix + 1; break;
    case 0xc: map_grid_emit_obj(grid, ix + 1, iy, 1, 1, 10, 1, 0, 1, 0);
              p4 = 1; by = iy + 1; p3 = 2; break;
    default:  do_block = 0; break;
    }
    if (do_block)
        map_grid_emit_obj(grid, bx, by, p3, p4, 10, 1, 0, 1, 0);
    map_grid_emit_tile(grid, ix, iy, 3, 0x5d, (uint16_t)c->arg_0c, 0, 0, 0, dims, ctx);
}

/* 587e00.c 0x272e arm (:2207-2251): a shape-switch object cluster (with the
 * &DAT_005ccXXX blend pointers) + a base tile (bank 0x60, slot 3). */
static void decode_block_272e(uint8_t *grid, int32_t ix, int32_t iy,
                              const map_cell *c, mg_bank_dims_fn dims, void *ctx)
{
    switch (c->shape) {
    case 0xb:
        map_grid_emit_obj(grid, ix, iy + 1, 2, 2, 10, 1, 0, 1, 0);
        break;
    case 0xc:
        map_grid_emit_obj(grid, ix, iy, 2, 3, 10, 1, 0, 1, 0);
        break;
    case 1:
        map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 1, MD_BLEND_5cc3d0, 1, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 3, MD_BLEND_5cc3f0, 1, 0);
        map_grid_emit_obj(grid, ix,     iy + 2, 2, 1, 10, 1, 0,              1, 0);
        break;
    case 2:
        map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 2, MD_BLEND_5cc390, 1, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, MD_BLEND_5cc3b0, 1, 0);
        map_grid_emit_obj(grid, ix,     iy + 2, 2, 1, 10, 1, 0,              1, 0);
        break;
    case 3:
        map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 3, MD_BLEND_5cc410, 1, 0);
        map_grid_emit_obj(grid, ix,     iy + 2, 1, 1, 10, 1, 0,              1, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 3, MD_BLEND_5cc410, 1, 0);
        map_grid_emit_obj(grid, ix,     iy + 3, 2, 1, 10, 1, 0,              1, 0);
        break;
    case 4:
        map_grid_emit_obj(grid, ix,     iy + 2, 1, 1, 10, 2, MD_BLEND_5cc430, 1, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 1, 0,              1, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 2, MD_BLEND_5cc430, 1, 0);
        map_grid_emit_obj(grid, ix,     iy + 3, 2, 1, 10, 1, 0,              1, 0);
        break;
    default: break;
    }
    map_grid_emit_tile(grid, ix, iy, 3, 0x60, (uint16_t)c->arg_0c, 0, 0, 0, dims, ctx);
}

void map_decode_cell(const map_data *m, uint8_t *grid,
                     uint32_t x, uint32_t y, uint32_t z,
                     const map_decode_cfg *cfg,
                     mg_bank_dims_fn dims, void *ctx)
{
    map_cell c;
    if (map_data_cell(m, x, y, z, &c) != 0 || c.tile_id == 0)
        return;

    /* Town default cfg if none given (the town arms ignore it; the house/errands
     * arms need the per-room banks + scene frame). */
    map_decode_cfg def;
    if (cfg == NULL) {
        map_decode_cfg_init(&def, MAP_DECODE_SCENE_PARAM3, 1);
        cfg = &def;
    }

    const int32_t ix = (int32_t)x, iy = (int32_t)y;
    const uint16_t a2 = (uint16_t)c.arg_0c;  /* uVar23 */

    switch (c.tile_id) {

    /* 0x1b58b / 0x1b58c (587e00.c:2365-2439): an object block sized by the shape
     * (some shapes add an extra 1x1 obj) + a base tile (bank 0x62, slot 3). */
    case 0x1b58b:
    case 0x1b58c: {
        int32_t ox = ix, oy = iy, rows = 0, cols = 0;
        int do_block = 1;
        switch (c.shape) {
        case 0:  rows = 2; cols = 2; break;
        case 1:  ox = ix + 1; oy = iy + 1; rows = 1; cols = 1; break;
        case 2:  oy = iy + 1; rows = 2; cols = 1; break;
        case 3:  oy = iy + 1; rows = 1; cols = 1; break;
        case 4:  map_grid_emit_obj(grid, ix, iy, 1, 1, 10, 1, 0, 7, 0);
                 oy = iy + 1; rows = 2; cols = 1; break;
        case 5:  rows = 1; cols = 2; break;
        case 6:  map_grid_emit_obj(grid, ix, iy + 1, 1, 1, 10, 1, 0, 7, 0);
                 rows = 2; cols = 1; break;
        case 7:  rows = 1; cols = 1; break;
        case 8:  rows = 2; cols = 1; break;
        case 9:  ox = ix + 1; rows = 1; cols = 1; break;
        case 10: map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, 0, 7, 0);
                 rows = 2; cols = 1; break;
        case 0xb: ox = ix + 1; rows = 1; cols = 2; break;
        case 0xc: map_grid_emit_obj(grid, ix + 1, iy, 1, 1, 10, 1, 0, 7, 0);
                 oy = iy + 1; rows = 2; cols = 1; break;
        default: do_block = 0; break;
        }
        if (do_block)
            map_grid_emit_obj(grid, ox, oy, rows, cols, 10, 1, 0, 7, 0);
        map_grid_emit_tile(grid, ix, iy, 3, 0x62, a2, 0, 0, 0, dims, ctx);
        break;
    }

    /* 0x1b58d (587e00.c:2313-2339): a 6-call object cluster (shape 1/2 only,
     * with a &DAT_005ccXXX blend pointer in two of them) + base tile (bank 0x63,
     * slot 3). */
    case 0x1b58d: {
        if (c.shape == 1) {
            map_grid_emit_obj(grid, ix,     iy + 1, 1, 3, 10, 1, 0,              7, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 3, MD_BLEND_5cc410, 5, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 1, 0,              5, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 2, 1, 1, 10, 3, MD_BLEND_5cc410, 5, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 3, 2, 1, 10, 1, 0,              5, 0);
            map_grid_emit_obj(grid, ix + 3, iy + 3, 1, 1, 10, 1, 0,              7, 0);
        } else if (c.shape == 2) {
            map_grid_emit_obj(grid, ix,     iy + 3, 1, 1, 10, 1, 0,              7, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 2, MD_BLEND_5cc430, 5, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 2, 1, 1, 10, 1, 0,              5, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 1, 1, 1, 10, 2, MD_BLEND_5cc430, 5, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 3, 2, 1, 10, 1, 0,              5, 0);
            map_grid_emit_obj(grid, ix + 3, iy + 1, 1, 3, 10, 1, 0,              7, 0);
        }
        map_grid_emit_tile(grid, ix, iy, 3, 0x63, a2, 0, 0, 0, dims, ctx);
        break;
    }

    /* 0x1b58f / 0x29ff4 / 0x29ffe / 0x29c02 / 0x29c0c: dir6 fg tile + base tile.
     * Base banks ground-truthed off the emit capture (ckpt 130). */
    case 0x1b58f: decode_dir6_tile(grid, x, y, z, &c, 0x176, dims, ctx); break;
    case 0x29ff4: decode_dir6_tile(grid, x, y, z, &c, 0x177, dims, ctx); break;
    case 0x29ffe: decode_dir6_tile(grid, x, y, z, &c, 0x178, dims, ctx); break;
    case 0x29c02: decode_dir6_tile(grid, x, y, z, &c, 0x190, dims, ctx); break;
    case 0x29c0c: decode_dir6_tile(grid, x, y, z, &c, 0x191, dims, ctx); break;

    /* 0x1b5a0 (587e00.c:2263-2271): one base tile, bank 0x17b, slot 2, flag 0xa. */
    case 0x1b5a0:
        map_grid_emit_tile(grid, ix, iy, 2, 0x17b, a2, 10, 0, 0, dims, ctx);
        break;

    /* 0x1b59f (587e00.c:2256-2262): one base tile, bank 0x17b, slot 2, flag 0x14. */
    case 0x1b59f:
        map_grid_emit_tile(grid, ix, iy, 2, 0x17b, a2, 0x14, 0, 0, dims, ctx);
        break;

    /* 0x1b5a9 (587e00.c:2137-2143): one base tile, bank 0x172, slot 1. */
    case 0x1b5a9:
        map_grid_emit_tile(grid, ix, iy, 1, 0x172, a2, 0, 0, 0, dims, ctx);
        break;

    /* 0x1b5aa (587e00.c:1463-1473): one base tile, bank 0x173, z-dependent
     * slot/flag. */
    case 0x1b5aa: {
        int32_t slot;  int16_t flag;
        if (z == 0) { slot = 1; flag = 2; }
        else        { slot = 2; flag = 3; }
        map_grid_emit_tile(grid, ix, iy, slot, 0x173, a2, flag, 0, 0, dims, ctx);
        break;
    }

    /* 0x1b5ab (587e00.c:1474-1480 -> LAB_005897d6): a tile (bank 0x174) + a
     * 2x9 object block; no base tile (falls straight to the next cell). */
    case 0x1b5ab:
        map_grid_emit_tile(grid, ix, iy, (z != 0) + 1, 0x174, a2, 0x14,
                           0, 0, dims, ctx);
        map_grid_emit_obj(grid, ix + 1, iy, 2, 9, 10, 1, 0, 7, 0);
        break;

    /* 0x1b5b3 (587e00.c:1483-1495): one base tile, bank 0x18e, shape-1 ->
     * slot 2 flag 0x14, else slot 1 flag 2. */
    case 0x1b5b3:
        if (c.shape == 1)
            map_grid_emit_tile(grid, ix, iy, 2, 0x18e, a2, 0x14, 0, 0, dims, ctx);
        else
            map_grid_emit_tile(grid, ix, iy, 1, 0x18e, a2, 2,    0, 0, dims, ctx);
        break;

    /* 0x2724 / 0x2738 (587e00.c:2205/:2162 -> caseD_2724): shape-switch obj
     * block + base tile bank 0x5d slot 3. */
    case 0x2724:
    case 0x2738:
        decode_block_2724(grid, ix, iy, &c, dims, ctx);
        break;

    /* 0x272e (587e00.c:2207-2251): shape-switch obj cluster (blends) + base
     * tile bank 0x60 slot 3. */
    case 0x272e:
        decode_block_272e(grid, ix, iy, &c, dims, ctx);
        break;

    /* 0x1b986 (587e00.c:2118-2122 -> LAB_0058c3b4): base tile bank local_24,
     * slot 1, flag 2. */
    case 0x1b986:
        map_grid_emit_tile(grid, ix, iy, 1, cfg->bank_24, a2, 2, 0, 0, dims, ctx);
        break;

    /* 0x1b98b (587e00.c:2123-2130 -> LAB_0058c3b4): base tile bank local_20,
     * slot 1, flag 2. */
    case 0x1b98b:
        map_grid_emit_tile(grid, ix, iy, 1, cfg->bank_20, a2, 2, 0, 0, dims, ctx);
        break;

    /* 0x1b990 (587e00.c:2131-2133 -> LAB_0058988d): base tile bank local_24,
     * slot 2, flag 0x15, + a shape-switch obj cluster (cases 0-3, blends). */
    case 0x1b990:
        map_grid_emit_tile(grid, ix, iy, 2, cfg->bank_24, a2, 0x15, 0, 0, dims, ctx);
        switch (c.shape) {
        case 0:
            map_grid_emit_obj(grid, ix,     iy,     2, 4, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 2, iy,     1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 1, 1, 1, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 3, iy + 1, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 2, 2, 4, 10, 1, 0,              6, 0);
            break;
        case 1:
            map_grid_emit_obj(grid, ix,     iy + 2, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
            map_grid_emit_obj(grid, ix,     iy + 3, 1, 1, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 3, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
            map_grid_emit_obj(grid, ix,     iy + 4, 2, 2, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 4, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
            break;
        case 2:
            map_grid_emit_obj(grid, ix + 1, iy + 4, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 3, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
            map_grid_emit_obj(grid, ix + 3, iy + 3, 1, 1, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 3, iy + 2, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
            map_grid_emit_obj(grid, ix + 2, iy + 4, 2, 2, 10, 1, 0,              6, 0);
            break;
        case 3:
            map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
            map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 1, iy,     1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
            map_grid_emit_obj(grid, ix,     iy + 2, 2, 4, 10, 1, 0,              6, 0);
            map_grid_emit_obj(grid, ix + 2, iy,     2, 4, 10, 1, 0,              6, 0);
            break;
        default: break;
        }
        break;

    /* 0x1b97c (587e00.c:2038-2117): the auto-footprint FLOOR tile (bank 0x17c,
     * slot 2, flag 0x14, frame = the CELL's arg_0c) + a shape-switch obj.  The
     * retail inlined grid-rectangle write == emit_tile with span 0/0.  FRAME FIX
     * (errands): the per-cell tile VARIANT is c.arg_0c (the cell +0xc), NOT the
     * scene param_3 — proven off the errands cells (arg_0c per column) == retail's
     * res frames exactly (the town/house don't use these auto-footprint tiles, so
     * the prior scene_frame=0 read was untested; the errands is the first interior
     * with autotiled walls/floor). */
    case 0x1b97c:
        map_grid_emit_tile(grid, ix, iy, 2, 0x17c, (uint16_t)c.arg_0c,
                           0x14, 0, 0, dims, ctx);
        switch (c.shape) {
        /* shape 1 (587e00.c:2072-2100): a 1x1 inline region-B class-10 +
         * region-D=1 write at (ix,iy) — the same record the FUN_0058ca80 arms
         * deposit (a=10, b=1, d4=6).  Previously misread as an occlusion mark
         * (ckpt 175: it is COLLISION — retail's live grid probe,
         * runs/arche-box/gridcells). */
        case 1:  map_grid_emit_obj(grid, ix, iy, 1, 1, 10, 1, 0, 6, 0); break;
        case 2:  map_grid_emit_obj(grid, ix + 1, iy, 1, 1, 10, 1, 0, 6, 0); break;
        case 3:  map_grid_emit_obj(grid, ix, iy, 2, 1, 1, 4, 0, 6, 0); break;
        case 4:  break;
        default: map_grid_emit_obj(grid, ix, iy, 2, 1, 10, 1, 0, 6, 0); break;
        }
        break;

    /* 0x1b972 (587e00.c:1510-1548): auto-footprint wall span, bank local_1c,
     * slot 1, flag 2, frame = the CELL's arg_0c (the per-cell wall variant; see
     * 0x1b97c above).  + shape-1/2 occlusion (deferred). */
    case 0x1b972:
        map_grid_emit_tile(grid, ix, iy, 1, cfg->bank_1c, (uint16_t)c.arg_0c,
                           2, 0, 0, dims, ctx);
        /* LAB_00589520 (587e00.c:1602-1660): shape 1 = a 1-col x 5-row
         * region-B class-10 + region-D=1 WALL COLUMN at (ix, iy..iy+4);
         * shape 2 = the same column one cell right.  Previously deferred as
         * an "occlusion mark" — it is COLLISION (the errands left wall,
         * ckpt 175: retail grid probe runs/arche-box/gridcells col 1 rows
         * 13-17 = class 10, emitted by the 113015 cell at (1,13,shape 1)). */
        if (c.shape == 1)      map_grid_emit_obj(grid, ix,     iy, 1, 5, 10, 1, 0, 6, 0);
        else if (c.shape == 2) map_grid_emit_obj(grid, ix + 1, iy, 1, 5, 10, 1, 0, 6, 0);
        break;

    /* 0x1b977 (587e00.c:1557-1601): auto-footprint wall span, bank local_18,
     * slot 1, flag 2, frame = the CELL's arg_0c.  PROVEN: the 8 errands 0x1b977
     * cells' arg_0c (4,5,5,8,5,5,6,7 by column) == retail's res 1897 frames
     * exactly (draw_probe).  + shape-1/2 occlusion (deferred). */
    case 0x1b977:
        map_grid_emit_tile(grid, ix, iy, 1, cfg->bank_18, (uint16_t)c.arg_0c,
                           2, 0, 0, dims, ctx);
        /* LAB_00589520 shape-1/2 wall column — same block as 0x1b972 above. */
        if (c.shape == 1)      map_grid_emit_obj(grid, ix,     iy, 1, 5, 10, 1, 0, 6, 0);
        else if (c.shape == 2) map_grid_emit_obj(grid, ix + 1, iy, 1, 5, 10, 1, 0, 6, 0);
        break;

    default:
        /* unhandled id: retail falls through to switchD_005887cc_caseD_271d */
        break;
    }
}

void map_decode(const map_data *m, uint8_t *grid, const map_decode_cfg *cfg,
                mg_bank_dims_fn dims, void *ctx)
{
    map_decode_cfg def;
    if (cfg == NULL) {
        map_decode_cfg_init(&def, MAP_DECODE_SCENE_PARAM3, 1);
        cfg = &def;
    }

    /* 587e00.c:34-56 (the four dim-header writes only). */
    map_grid_set_dims(grid, (int32_t)m->dim0, (int32_t)m->dim1);

    /* 587e00.c:572-584 — pre-clear region C over dim0 x dim1. */
    for (uint32_t y = 0; y < m->dim1; y++)
        for (uint32_t x = 0; x < m->dim0; x++)
            map_grid_clear_cell(grid, (int32_t)x, (int32_t)y, 0, 0, 0);

    /* 587e00.c:585-3183 — per-cell dispatch (z-major), then zero the cell's
     * region-E co-id slot (587e00.c:3175). */
    for (uint32_t z = 0; z < m->dim2; z++) {
        for (uint32_t y = 0; y < m->dim1; y++) {
            for (uint32_t x = 0; x < m->dim0; x++) {
                map_decode_cell(m, grid, x, y, z, cfg, dims, ctx);
                size_t idx = (size_t)x * MG_ROW_PITCH + y;
                size_t off = MG_REGION_E + idx * MG_REGION_E_STRIDE;
                memset(grid + off, 0, 2);
            }
        }
    }
}
