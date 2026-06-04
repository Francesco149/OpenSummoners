/* map_decode.c — FUN_00587e00 per-tile-id placement dispatch.  See map_decode.h.
 *
 * Each arm below is a faithful transcription of the matching arm in
 * docs/decompiled/by-address/587e00.c.  The decompiler's variable names are
 * noted where helpful: iVar17 = x, param_2 = y, local_28 = z; uVar23 = cell+0x0c
 * (the emit_tile a2 index); the shared tail LAB_0058c3b9 issues the final
 * map_grid_emit_tile.  FUN_0058ca80 -> map_grid_emit_obj and FUN_0058c910 ->
 * map_grid_emit_tile both keep the decomp argument order (emit_tile drops the
 * two unused params 7/8; see map_grid.c).
 */
#include "map_decode.h"

#include <string.h>

/* 0x1b58f / 0x29ff4 share one arm: an optional foreground tile (bank 0x17a,
 * frame chosen by the shape) plus a base tile whose bank differs per id.
 * 587e00.c:2255-2311 (base 0x176) and :2632-2671 (base 0x177). */
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

void map_decode_cell(const map_data *m, uint8_t *grid,
                     uint32_t x, uint32_t y, uint32_t z,
                     mg_bank_dims_fn dims, void *ctx)
{
    map_cell c;
    if (map_data_cell(m, x, y, z, &c) != 0 || c.tile_id == 0)
        return;

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

    /* 0x1b58f / 0x29ff4: directional foreground tile + base tile. */
    case 0x1b58f:
        decode_dir6_tile(grid, x, y, z, &c, 0x176, dims, ctx);
        break;
    case 0x29ff4:
        decode_dir6_tile(grid, x, y, z, &c, 0x177, dims, ctx);
        break;

    /* 0x1b5a0 (587e00.c:2263-2271): one base tile, bank 0x17b, slot 2, flag 0xa. */
    case 0x1b5a0:
        map_grid_emit_tile(grid, ix, iy, 2, 0x17b, a2, 10, 0, 0, dims, ctx);
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

    default:
        /* unhandled id: retail falls through to switchD_005887cc_caseD_271d */
        break;
    }
}

void map_decode(const map_data *m, uint8_t *grid,
                mg_bank_dims_fn dims, void *ctx)
{
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
                map_decode_cell(m, grid, x, y, z, dims, ctx);
                size_t idx = (size_t)x * MG_ROW_PITCH + y;
                size_t off = MG_REGION_E + idx * MG_REGION_E_STRIDE;
                memset(grid + off, 0, 2);
            }
        }
    }
}
