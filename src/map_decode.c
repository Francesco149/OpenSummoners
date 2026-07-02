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
 * COVERAGE (ckpt 130 + the ckpt-178 all-maps sweep): FULL — every tile id that
 * occurs in any of the exe's 376 MSD_SOTES_MAPDATA resources has an arm
 * (tools/extract/map_sweep.py; 87 distinct ids).  The cutscene-chain arms
 * (town 1022 / house 1023 / errands 1025) are ground-truthed against a retail
 * emit-sequence capture (runs/room-render-gt, hooks on 0x58c910/0x58ca80)
 * cross-referenced with the cell (tile id, shape) histograms
 * (tools/extract/map_data.py --cells); the remaining arms are faithful
 * transcriptions of 587e00.c (same emits, same order), to be behaviorally
 * verified when their areas enter the parity loop.
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

    /* 587e00.c:49-55 defaults (param_4 not in the switch, e.g. 1 = town/house). */
    cfg->bank_24 = 0x17e;
    cfg->bank_1c = 0x17d;
    cfg->bank_20 = 0x185;
    cfg->bank_18 = 0x184;
    cfg->bank_14 = 0x83;
    /* the param_4 = room[0x43] switch (587e00.c:197-252).  Cases 5-13 keep the
     * default tile banks; 5/6/8 swap only local_14 (their palette-remap table
     * installs are PORT-DEBT(decode-prologue-header)). */
    switch (param_4) {
    case 2:  cfg->bank_24 = 0x183; cfg->bank_1c = 0x182;
             cfg->bank_20 = 0x185; cfg->bank_18 = 0x184; break;
    case 3:  cfg->bank_24 = 0x181; cfg->bank_1c = 0x180;
             cfg->bank_20 = 0x188; cfg->bank_18 = 0x187; break;
    case 4:  cfg->bank_24 = 0x17e; cfg->bank_1c = 0x17d;
             cfg->bank_20 = 0x188; cfg->bank_18 = 0x187; break;
    case 5:  cfg->bank_14 = 0x84; break;
    case 6:  cfg->bank_14 = 0x85; break;
    case 8:  cfg->bank_14 = 0x86; break;
    default: break;
    }
}

/* 0x1b58f / 0x29ff4 / 0x29ffe / 0x29c02 / 0x29c0c / 0x29c16 / 0x2a008 share one
 * arm: an optional foreground tile (bank 0x17a, frame chosen by the shape) plus
 * a base tile whose bank differs per id.  587e00.c:2255-2311 (base 0x176),
 * :2632-2671 (0x177), :2800-2841 (0x178), :2674-2714 (0x191), :2716-2756
 * (0x190), :2758-2798 (0x192), :2590-2628 (0x179). */
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

/* 587e00.c switchD_005887cc_caseD_2724 (0x2724 / 0x2738, :3206-3280) and its
 * d4=2 twin switchD_0058b42a_caseD_1ffc3 (0x1ffc3 / 0x1ffc5, :3096-3169): a
 * shape-switch object block (the obj geometry varies per shape) + a base tile
 * (slot 3).  emit_obj(iVar15, iVar6, uVar14, uVar20, 10, 1, 0, d4, 0).  The two
 * arms are call-for-call identical apart from the obj d4 (1 vs 2) and the base
 * bank (0x5d vs 0x7a). */
static void decode_block_2724(uint8_t *grid, int32_t ix, int32_t iy,
                              const map_cell *c, uint32_t d4, uint32_t base_bank,
                              mg_bank_dims_fn dims, void *ctx)
{
    int32_t bx = ix, by = iy;     /* iVar15, iVar6 */
    int32_t p3 = 0, p4 = 0;       /* uVar14, uVar20 */
    int do_block = 1;
    switch (c->shape) {
    case 0:   p4 = 2; p3 = 2; break;
    case 1:   p4 = 1; p3 = 1; bx = ix + 1; by = iy + 1; break;
    case 2:   p4 = 1; by = iy + 1; p3 = 2; break;
    case 3:   p4 = 1; by = iy + 1; p3 = 1; break;
    case 4:   map_grid_emit_obj(grid, ix, iy, 1, 1, 10, 1, 0, d4, 0);
              p4 = 1; by = iy + 1; p3 = 2; break;
    case 5:   p4 = 2; p3 = 1; break;
    case 6:   map_grid_emit_obj(grid, ix, iy + 1, 1, 1, 10, 1, 0, d4, 0);
              p4 = 1; p3 = 2; break;
    case 7:   p4 = 1; p3 = 1; break;
    case 8:   p4 = 1; p3 = 2; break;
    case 9:   p4 = 1; p3 = 1; bx = ix + 1; break;
    case 10:  map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, 0, d4, 0);
              p4 = 1; p3 = 2; break;
    case 0xb: p4 = 2; p3 = 1; bx = ix + 1; break;
    case 0xc: map_grid_emit_obj(grid, ix + 1, iy, 1, 1, 10, 1, 0, d4, 0);
              p4 = 1; by = iy + 1; p3 = 2; break;
    default:  do_block = 0; break;
    }
    if (do_block)
        map_grid_emit_obj(grid, bx, by, p3, p4, 10, 1, 0, d4, 0);
    map_grid_emit_tile(grid, ix, iy, 3, base_bank, (uint16_t)c->arg_0c,
                       0, 0, 0, dims, ctx);
}

/* 587e00.c 0x272e arm (:2207-2251) and its 0x1ffeb twin (:2874-2903): a
 * shape-switch object cluster (with the &DAT_005ccXXX blend pointers) + a base
 * tile (slot 3).  Same geometry; 0x272e has d4=1, base 0x60 and two extra
 * shapes (0xb/0xc); 0x1ffeb has d4=2, base 0x7b, shapes 1-4 only. */
static void decode_block_272e(uint8_t *grid, int32_t ix, int32_t iy,
                              const map_cell *c, uint32_t d4, uint32_t base_bank,
                              int has_bc_shapes, mg_bank_dims_fn dims, void *ctx)
{
    switch (c->shape) {
    case 0xb:
        if (has_bc_shapes)
            map_grid_emit_obj(grid, ix, iy + 1, 2, 2, 10, 1, 0, d4, 0);
        break;
    case 0xc:
        if (has_bc_shapes)
            map_grid_emit_obj(grid, ix, iy, 2, 3, 10, 1, 0, d4, 0);
        break;
    case 1:
        map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 1, MD_BLEND_5cc3d0, d4, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 3, MD_BLEND_5cc3f0, d4, 0);
        map_grid_emit_obj(grid, ix,     iy + 2, 2, 1, 10, 1, 0,              d4, 0);
        break;
    case 2:
        map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 2, MD_BLEND_5cc390, d4, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, MD_BLEND_5cc3b0, d4, 0);
        map_grid_emit_obj(grid, ix,     iy + 2, 2, 1, 10, 1, 0,              d4, 0);
        break;
    case 3:
        map_grid_emit_obj(grid, ix,     iy + 1, 1, 1, 10, 3, MD_BLEND_5cc410, d4, 0);
        map_grid_emit_obj(grid, ix,     iy + 2, 1, 1, 10, 1, 0,              d4, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 3, MD_BLEND_5cc410, d4, 0);
        map_grid_emit_obj(grid, ix,     iy + 3, 2, 1, 10, 1, 0,              d4, 0);
        break;
    case 4:
        map_grid_emit_obj(grid, ix,     iy + 2, 1, 1, 10, 2, MD_BLEND_5cc430, d4, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 1, 0,              d4, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 2, MD_BLEND_5cc430, d4, 0);
        map_grid_emit_obj(grid, ix,     iy + 3, 2, 1, 10, 1, 0,              d4, 0);
        break;
    default: break;
    }
    map_grid_emit_tile(grid, ix, iy, 3, base_bank, (uint16_t)c->arg_0c,
                       0, 0, 0, dims, ctx);
}

/* 587e00.c 0x1d8aa (:2957-2991) / 0x22ab2 (:2486-2520) shared shape arm: one
 * 1x1 blended obj per shape 0-5 + a base tile (slot 3, flag 0).  Identical
 * shape -> (blend, region-D) table; only d4 (2 vs 5) and the base bank (0x68 vs
 * 0x72) differ. */
static void decode_blend_pair(uint8_t *grid, int32_t ix, int32_t iy,
                              const map_cell *c, uint32_t d4, uint32_t base_bank,
                              mg_bank_dims_fn dims, void *ctx)
{
    static const struct { uint32_t blend; uint16_t rd; } SH[6] = {
        { MD_BLEND_5cc390, 2 }, { MD_BLEND_5cc3b0, 1 }, { MD_BLEND_5cc3d0, 1 },
        { MD_BLEND_5cc3f0, 3 }, { MD_BLEND_5cc430, 2 }, { MD_BLEND_5cc410, 3 },
    };
    if (c->shape < 6)
        map_grid_emit_obj(grid, ix, iy, 1, 1, 10, SH[c->shape].rd,
                          SH[c->shape].blend, d4, 0);
    map_grid_emit_tile(grid, ix, iy, 3, base_bank, (uint16_t)c->arg_0c,
                       0, 0, 0, dims, ctx);
}

/* 587e00.c 0x1d8ab (:2992-3025) / 0x22b16 (:2533-2563) shared shape arm: a
 * column obj for shapes 1/2/3 + a base tile (slot 3, flag 0) whose footprint is
 * an explicit 1x2 span for shape 3 (uVar22=1/uVar24=2) and bank-derived
 * otherwise.  d4 = 2 vs 5; base bank = 0x69 vs 0x74. */
static void decode_block_1d8ab(uint8_t *grid, int32_t ix, int32_t iy,
                               const map_cell *c, uint32_t d4, uint32_t base_bank,
                               mg_bank_dims_fn dims, void *ctx)
{
    switch (c->shape) {
    case 1: map_grid_emit_obj(grid, ix, iy + 1, 2, 1, 10, 1, 0, d4, 0); break;
    case 2: map_grid_emit_obj(grid, ix, iy,     2, 1, 10, 1, 0, d4, 0); break;
    case 3: map_grid_emit_obj(grid, ix, iy,     1, 1, 10, 1, 0, d4, 0); break;
    default: break;
    }
    if (c->shape == 3)
        map_grid_emit_tile(grid, ix, iy, 3, base_bank, (uint16_t)c->arg_0c,
                           0, 1, 2, dims, ctx);
    else
        map_grid_emit_tile(grid, ix, iy, 3, base_bank, (uint16_t)c->arg_0c,
                           0, 0, 0, dims, ctx);
}

/* 587e00.c LAB_0058988d (0x1b990 :2131-2133 / 0x1b995 :2001-2004 -> :2004-2033):
 * base tile (slot 2, flag 0x15, bank = local_24 for 0x1b990 / local_20 for
 * 0x1b995) + a shape-switch obj cluster (cases 0-3, blends, d4=6). */
static void decode_block_1b990(uint8_t *grid, int32_t ix, int32_t iy,
                               const map_cell *c, uint32_t bank,
                               mg_bank_dims_fn dims, void *ctx)
{
    map_grid_emit_tile(grid, ix, iy, 2, bank, (uint16_t)c->arg_0c, 0x15,
                       0, 0, dims, ctx);
    switch (c->shape) {
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
}

/* 587e00.c 0x1bd82 (:605-1451): the 113xxx-family STAIRCASE autotile — a slot-3
 * bank-0x195 footprint tile + a large shape-0/1 collision cluster (class-10
 * walls with slope blends + one class-1 region-D-4 slope marker, all d4=6).
 * Emit calls transcribed in retail order (two are duplicate writes in retail —
 * kept for structural parity; the grid bytes are idempotent). */
static void decode_block_1bd82(uint8_t *grid, int32_t ix, int32_t iy,
                               const map_cell *c, mg_bank_dims_fn dims, void *ctx)
{
    map_grid_emit_tile(grid, ix, iy, 3, 0x195, (uint16_t)c->arg_0c, 0x14,
                       0, 0, dims, ctx);
    if (c->shape == 0) {
        map_grid_emit_obj(grid, ix + 1, iy + 6, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 2, iy + 6, 1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 2, iy + 5, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 3, iy + 4, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 4, iy + 4, 1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 4, iy + 3, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 3, iy + 5, 2, 2, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 5, iy + 3, 1, 4, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 3, iy + 2, 1, 1, 1,  4, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 1, iy,     1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 1, 1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 2, iy + 1, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix,     iy - 1, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix,     iy,     1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 1, iy,     1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
    } else if (c->shape == 1) {
        map_grid_emit_obj(grid, ix + 3, iy + 5, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix + 3, iy + 6, 1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 4, iy + 6, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 3, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 4, 1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 2, iy + 4, 1, 1, 10, 3, MD_BLEND_5cc410, 6, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 5, 2, 2, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix,     iy + 3, 1, 4, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 2, iy + 2, 1, 1, 1,  4, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 3, iy + 1, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 4, iy + 1, 1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 4, iy,     1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 4, iy,     1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
        map_grid_emit_obj(grid, ix + 5, iy,     1, 1, 10, 1, 0,              6, 0);
        map_grid_emit_obj(grid, ix + 5, iy - 1, 1, 1, 10, 2, MD_BLEND_5cc430, 6, 0);
    }
}

/* 587e00.c 0x1bd6e (:1698-1933): slot-1 bank-0x194 footprint tile (flag 0x14)
 * + a shape-0..4 collision block (d4=5; shapes 3/4 are blended slope pairs). */
static void decode_block_1bd6e(uint8_t *grid, int32_t ix, int32_t iy,
                               const map_cell *c, mg_bank_dims_fn dims, void *ctx)
{
    map_grid_emit_tile(grid, ix, iy, 1, 0x194, (uint16_t)c->arg_0c, 0x14,
                       0, 0, dims, ctx);
    switch (c->shape) {
    case 0: map_grid_emit_obj(grid, ix, iy,     2, 2, 10, 1, 0, 5, 0); break;
    case 1: map_grid_emit_obj(grid, ix, iy + 1, 2, 1, 10, 1, 0, 5, 0); break;
    case 2: map_grid_emit_obj(grid, ix, iy,     2, 1, 10, 1, 0, 5, 0); break;
    case 3:
        map_grid_emit_obj(grid, ix,     iy, 1, 1, 10, 2, MD_BLEND_5cc390, 5, 0);
        map_grid_emit_obj(grid, ix + 1, iy, 1, 1, 10, 1, MD_BLEND_5cc3b0, 5, 0);
        break;
    case 4:
        map_grid_emit_obj(grid, ix,     iy, 1, 1, 10, 1, MD_BLEND_5cc3d0, 5, 0);
        map_grid_emit_obj(grid, ix + 1, iy, 1, 1, 10, 3, MD_BLEND_5cc3f0, 5, 0);
        break;
    default: break;
    }
}

/* 587e00.c 0x1ffc4 (:3048-3092): shape-1..4 rail objs (one blended end cap) +
 * a base tile (slot 3, bank 0x7c, flag 0), all obj d4=2. */
static void decode_block_1ffc4(uint8_t *grid, int32_t ix, int32_t iy,
                               const map_cell *c, mg_bank_dims_fn dims, void *ctx)
{
    switch (c->shape) {
    case 1:
        map_grid_emit_obj(grid, ix,     iy + 3, 4, 1, 10, 1, 0, 2, 0);
        map_grid_emit_obj(grid, ix + 3, iy,     1, 4, 10, 1, 0, 2, 0);
        map_grid_emit_obj(grid, ix + 2, iy + 2, 1, 1, 10, 2, MD_BLEND_5cc430, 2, 0);
        break;
    case 2:
        map_grid_emit_obj(grid, ix, iy, 4, 1, 10, 1, 0, 2, 0);
        map_grid_emit_obj(grid, ix, iy, 1, 4, 10, 1, 0, 2, 0);
        break;
    case 3:
        map_grid_emit_obj(grid, ix,     iy, 4, 1, 10, 1, 0, 2, 0);
        map_grid_emit_obj(grid, ix + 3, iy, 1, 4, 10, 1, 0, 2, 0);
        break;
    case 4:
        map_grid_emit_obj(grid, ix,     iy + 3, 4, 1, 10, 1, 0, 2, 0);
        map_grid_emit_obj(grid, ix,     iy,     1, 4, 10, 1, 0, 2, 0);
        map_grid_emit_obj(grid, ix + 1, iy + 2, 1, 1, 10, 3, MD_BLEND_5cc410, 2, 0);
        break;
    default: break;
    }
    map_grid_emit_tile(grid, ix, iy, 3, 0x7c, (uint16_t)c->arg_0c, 0, 0, 0, dims, ctx);
}

/* The plain one-base-tile arms: id -> (slot, bank, flag); frame = the cell's
 * arg_0c, bank-derived footprint, no objs.  Transcribed from the 587e00.c
 * LAB_0058c3b4/LAB_0058c3b9 setters (bank = uVar20, slot = cVar5, flag =
 * uVar14; flag 0 resolves per slot inside emit_tile, 58c910.c:15-27).  The
 * z-slot arms (0x1b5bc/bd/be) and cfg-bank arms stay in the switch. */
static const struct md_simple_arm {
    uint32_t id;
    uint8_t  slot;
    uint16_t bank;
    int16_t  flag;
} MD_SIMPLE_ARMS[] = {
    /* 10xxx doors/frames (587e00.c:2190-2202) */
    { 0x271a, 1, 0x5c, 0 }, { 0x271b, 1, 0x5c, 0 }, { 0x271c, 1, 0x5c, 0 },
    { 0x271f, 1, 0x5c, 0 }, { 0x2720, 1, 0x5c, 0 }, { 0x2721, 1, 0x5c, 0 },
    { 0x2722, 1, 0x5c, 0 },
    /* 11xxx (:2145-2180, :2341-2360) */
    { 0x2af8, 1, 0x64, 0 }, { 0x2af9, 1, 0x64, 0 },
    { 0x2b02, 1, 0x65, 0 }, { 0x2b03, 1, 0x65, 0 }, { 0x2b04, 1, 0x65, 0 },
    { 0x2b34, 1, 0x66, 0 }, { 0x2c2e, 1, 0x67, 0 }, { 0x2c38, 1, 0x6a, 0 },
    /* 112070 (:1503-1507) */
    { 0x1b5c6, 1, 0x18c, 0 },
    /* 121010 (:3026-3030) */
    { 0x1d8b2, 1, 0x69, 0 },
    /* 131xxx (:2941-2948, :3039-3045, :2860-2872) */
    { 0x1ffb9, 1, 0x79, 0 }, { 0x1ffba, 1, 0x79, 0 }, { 0x1ffbb, 1, 0x79, 0 },
    { 0x1ffbd, 1, 0x7c, 0 }, { 0x1ffe1, 1, 0x7d, 0 }, { 0x1ffe5, 1, 0x7e, 0 },
    /* 141xxx (:2850-2919, :2922-2926, :2843-2847) */
    { 0x226c9, 1, 0x72, 0 }, { 0x226ca, 1, 0x72, 0 },
    { 0x22791, 1, 0x74, 0 }, { 0x227f5, 1, 0x75, 0 }, { 0x227f6, 1, 0x76, 0 },
    /* 142301 (:2468-2474) */
    { 0x22bdd, 1, 0x73, 0 },
    /* 151xxx (:2445-2457) */
    { 0x24de2, 1, 0x5f, 0 }, { 0x24dec, 1, 0x5e, 0 },
    /* 161001 (:2459-2466), 162001 (:2567-2571) */
    { 0x274e9, 2, 0x70, 3 }, { 0x278d1, 1, 0x71, 0 },
};

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

    /* The plain one-base-tile arms (table above). */
    for (size_t i = 0; i < sizeof MD_SIMPLE_ARMS / sizeof MD_SIMPLE_ARMS[0]; i++) {
        if (MD_SIMPLE_ARMS[i].id == c.tile_id) {
            map_grid_emit_tile(grid, ix, iy, MD_SIMPLE_ARMS[i].slot,
                               MD_SIMPLE_ARMS[i].bank, a2,
                               MD_SIMPLE_ARMS[i].flag, 0, 0, dims, ctx);
            return;
        }
    }

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

    /* 0x1b58f / 0x29ff4 / 0x29ffe / 0x29c02 / 0x29c0c / 0x29c16 / 0x2a008: dir6
     * fg tile + base tile.  The town-chain base banks ground-truthed off the
     * emit capture (ckpt 130); 0x29c16/0x2a008 transcribed (:2758.. / :2590..). */
    case 0x1b58f: decode_dir6_tile(grid, x, y, z, &c, 0x176, dims, ctx); break;
    case 0x29ff4: decode_dir6_tile(grid, x, y, z, &c, 0x177, dims, ctx); break;
    case 0x29ffe: decode_dir6_tile(grid, x, y, z, &c, 0x178, dims, ctx); break;
    case 0x29c02: decode_dir6_tile(grid, x, y, z, &c, 0x190, dims, ctx); break;
    case 0x29c0c: decode_dir6_tile(grid, x, y, z, &c, 0x191, dims, ctx); break;
    case 0x29c16: decode_dir6_tile(grid, x, y, z, &c, 0x192, dims, ctx); break;
    case 0x2a008: decode_dir6_tile(grid, x, y, z, &c, 0x179, dims, ctx); break;

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
     * block + base tile bank 0x5d slot 3; 0x1ffc3 / 0x1ffc5 (:3046/:2940 ->
     * caseD_1ffc3) are the same block with obj d4=2 + base bank 0x7a. */
    case 0x2724:
    case 0x2738:
        decode_block_2724(grid, ix, iy, &c, 1, 0x5d, dims, ctx);
        break;
    case 0x1ffc3:
    case 0x1ffc5:
        decode_block_2724(grid, ix, iy, &c, 2, 0x7a, dims, ctx);
        break;

    /* 0x272e (587e00.c:2207-2251): shape-switch obj cluster (blends) + base
     * tile bank 0x60 slot 3; 0x1ffeb (:2874-2903) = same minus shapes 0xb/0xc,
     * obj d4=2, base bank 0x7b. */
    case 0x272e:
        decode_block_272e(grid, ix, iy, &c, 1, 0x60, 1, dims, ctx);
        break;
    case 0x1ffeb:
        decode_block_272e(grid, ix, iy, &c, 2, 0x7b, 0, dims, ctx);
        break;

    /* 0x272f / 0x2730 (587e00.c:2184-2187): base tile (slot 1, bank 0x5c) + a
     * 1x1 class-1 region-D-4 slope marker (d4=3). */
    case 0x272f:
    case 0x2730:
        map_grid_emit_tile(grid, ix, iy, 1, 0x5c, a2, 0, 0, 0, dims, ctx);
        map_grid_emit_obj(grid, ix, iy, 1, 1, 1, 4, 0, 3, 0);
        break;

    /* 0x2b2a (587e00.c:2172-2180): slot 2, bank 0x69; flag 3 for shape 0 else
     * the slot default. */
    case 0x2b2a:
        map_grid_emit_tile(grid, ix, iy, 2, 0x69, a2,
                           (c.shape == 0) ? 3 : 0, 0, 0, dims, ctx);
        break;

    /* 0x2bca / 0x2bcb (587e00.c:2351-2354 -> LAB_0058c283): base tile (slot 1,
     * bank 0x64) + a 1x1 class-1 region-D-4 marker (d4=4). */
    case 0x2bca:
    case 0x2bcb:
        map_grid_emit_tile(grid, ix, iy, 1, 0x64, a2, 0, 0, 0, dims, ctx);
        map_grid_emit_obj(grid, ix, iy, 1, 1, 1, 4, 0, 4, 0);
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

    /* 0x1b990 (587e00.c:2131-2133) / 0x1b995 (:2001-2004) -> LAB_0058988d:
     * base tile (slot 2, flag 0x15, bank local_24 / local_20) + the shared
     * shape cluster (decode_block_1b990). */
    case 0x1b990:
        decode_block_1b990(grid, ix, iy, &c, cfg->bank_24, dims, ctx);
        break;
    case 0x1b995:
        decode_block_1b990(grid, ix, iy, &c, cfg->bank_20, dims, ctx);
        break;

    /* 0x1b5bc / 0x1b5bd / 0x1b5be (587e00.c:1458-1459/:1496-1497/:1550 ->
     * :1552-1556): one base tile, banks 0x189/0x18a/0x18b, z-dependent slot,
     * flag 2. */
    case 0x1b5bc:
        map_grid_emit_tile(grid, ix, iy, (z != 0) + 1, 0x189, a2, 2, 0, 0, dims, ctx);
        break;
    case 0x1b5bd:
        map_grid_emit_tile(grid, ix, iy, (z != 0) + 1, 0x18a, a2, 2, 0, 0, dims, ctx);
        break;
    case 0x1b5be:
        map_grid_emit_tile(grid, ix, iy, (z != 0) + 1, 0x18b, a2, 2, 0, 0, dims, ctx);
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

    /* 0x1bd64 (587e00.c:1664-1696): slot-1 bank-0x193 footprint tile, flag 2. */
    case 0x1bd64:
        map_grid_emit_tile(grid, ix, iy, 1, 0x193, a2, 2, 0, 0, dims, ctx);
        break;

    /* 0x1bd6e (587e00.c:1698-1933): decode_block_1bd6e above. */
    case 0x1bd6e:
        decode_block_1bd6e(grid, ix, iy, &c, dims, ctx);
        break;

    /* 0x1bd78 (587e00.c:1937-1997): slot-2 bank-0x193 footprint tile (flag
     * 0x14) + a shape-1 2x7 class-10 block (d4=5). */
    case 0x1bd78:
        map_grid_emit_tile(grid, ix, iy, 2, 0x193, a2, 0x14, 0, 0, dims, ctx);
        if (c.shape == 1)
            map_grid_emit_obj(grid, ix, iy, 2, 7, 10, 1, 0, 5, 0);
        break;

    /* 0x1bd82 (587e00.c:605-1451): decode_block_1bd82 above. */
    case 0x1bd82:
        decode_block_1bd82(grid, ix, iy, &c, dims, ctx);
        break;

    /* 0x1d8a9 (587e00.c:2951-2956): base tile (slot 3, bank 0x68) + a shape-0
     * 1x1 class-10 obj (d4=2). */
    case 0x1d8a9:
        map_grid_emit_tile(grid, ix, iy, 3, 0x68, a2, 0, 0, 0, dims, ctx);
        if (c.shape == 0)
            map_grid_emit_obj(grid, ix, iy, 1, 1, 10, 1, 0, 2, 0);
        break;

    /* 0x1d8aa (:2957-2991) / 0x22ab2 (:2486-2520): decode_blend_pair above. */
    case 0x1d8aa:
        decode_blend_pair(grid, ix, iy, &c, 2, 0x68, dims, ctx);
        break;
    case 0x22ab2:
        decode_blend_pair(grid, ix, iy, &c, 5, 0x72, dims, ctx);
        break;

    /* 0x1d8ab (:2992-3025) / 0x22b16 (:2533-2563): decode_block_1d8ab above. */
    case 0x1d8ab:
        decode_block_1d8ab(grid, ix, iy, &c, 2, 0x69, dims, ctx);
        break;
    case 0x22b16:
        decode_block_1d8ab(grid, ix, iy, &c, 5, 0x74, dims, ctx);
        break;

    /* 0x1ffbc (587e00.c:3035-3038): base tile (slot 1, bank 0x79) + a 2x1
     * class-1 region-D-4 marker at (x, y+1), d4=3. */
    case 0x1ffbc:
        map_grid_emit_tile(grid, ix, iy, 1, 0x79, a2, 0, 0, 0, dims, ctx);
        map_grid_emit_obj(grid, ix, iy + 1, 2, 1, 1, 4, 0, 3, 0);
        break;

    /* 0x1ffc4 (587e00.c:3048-3092): decode_block_1ffc4 above. */
    case 0x1ffc4:
        decode_block_1ffc4(grid, ix, iy, &c, dims, ctx);
        break;

    /* 0x1ffcd (587e00.c:2853-2857): slot 2, bank local_14 (the param_4-swapped
     * 0x83 family), flag 3. */
    case 0x1ffcd:
        map_grid_emit_tile(grid, ix, iy, 2, cfg->bank_14, a2, 3, 0, 0, dims, ctx);
        break;

    /* 0x22791 is in the simple table; 0x22792 (587e00.c:2930-2937): slot 2,
     * bank 0x74; flag 3 for shape 0 else the slot default. */
    case 0x22792:
        map_grid_emit_tile(grid, ix, iy, 2, 0x74, a2,
                           (c.shape == 0) ? 3 : 0, 0, 0, dims, ctx);
        break;

    /* 0x22ab1 (587e00.c:2477-2485): base tile (slot 3, bank 0x72) + a shape-0
     * 1x1 class-10 obj (d4=5, LAB_0058c2a9 with uVar25=10/uVar21=1/uVar23=5). */
    case 0x22ab1:
        map_grid_emit_tile(grid, ix, iy, 3, 0x72, a2, 0, 0, 0, dims, ctx);
        if (c.shape == 0)
            map_grid_emit_obj(grid, ix, iy, 1, 1, 10, 1, 0, 5, 0);
        break;

    /* 0x22b15 (587e00.c:2521-2532): base tile bank 0x74, z==1 -> slot 2 flag
     * 0x14 else slot 3 flag 0x15, + a 2x2 class-10 block (d4=5). */
    case 0x22b15:
        if (z == 1)
            map_grid_emit_tile(grid, ix, iy, 2, 0x74, a2, 0x14, 0, 0, dims, ctx);
        else
            map_grid_emit_tile(grid, ix, iy, 3, 0x74, a2, 0x15, 0, 0, dims, ctx);
        map_grid_emit_obj(grid, ix, iy, 2, 2, 10, 1, 0, 5, 0);
        break;

    /* 0xf3e62 / 0xf3e6c (587e00.c:2576-2587): no tile — a single 1x1 class-1
     * region-D-4 marker; d4 = 3 (999010) / 5 (999020). */
    case 0xf3e62:
        map_grid_emit_obj(grid, ix, iy, 1, 1, 1, 4, 0, 3, 0);
        break;
    case 0xf3e6c:
        map_grid_emit_obj(grid, ix, iy, 1, 1, 1, 4, 0, 5, 0);
        break;

    default:
        /* unhandled id: retail falls through to switchD_005887cc_caseD_271d */
        break;
    }
}

/* FUN_0058cb30 — one 0x15f9a/0x15f9b placeholder layer -> the anchor cell's
 * region-E record (layout in map_decode.h).  `all_links` = 1 for 0x15f9a (link
 * every sub-D entry), 0 for 0x15f9b (only entries whose first dword == 1).
 * Retail fatals past 4 links ("The maximum number exceeded by t..."); the port
 * stops linking instead (host-safety; no shipped map exceeds it). */
static uint32_t layer_hdr_u32(const map_layer *l, int off)
{
    return (uint32_t)l->hdr[off] | ((uint32_t)l->hdr[off + 1] << 8) |
           ((uint32_t)l->hdr[off + 2] << 16) | ((uint32_t)l->hdr[off + 3] << 24);
}

static void decode_placeholder_layer(const map_data *m, uint8_t *grid,
                                     const map_layer *l, int all_links)
{
    int32_t hx = (int32_t)layer_hdr_u32(l, 0x04);
    int32_t hy = (int32_t)layer_hdr_u32(l, 0x08);
    int32_t cx = (hx * 100) / 0xc80;             /* 58cb30.c:26-27 */
    int32_t cy = (hy * 100) / 0xc80;
    size_t rec = MG_REGION_E +
                 (size_t)(cx * (int32_t)MG_ROW_PITCH + cy) * MG_REGION_E_STRIDE;

    /* :28-39 — flag = (sub-A first dword == 1), anchor world pos. */
    uint32_t flag = (l->n_a > 0 && l->a != NULL &&
                     (uint32_t)(l->a[0] | (l->a[1] << 8) | (l->a[2] << 16) |
                                (l->a[3] << 24)) == 1) ? 1u : 0u;
    memcpy(grid + rec + 0x04, &flag, 4);
    int32_t wx = hx * 100, wy = hy * 100;
    memcpy(grid + rec + 0x28, &wx, 4);
    memcpy(grid + rec + 0x2c, &wy, 4);

    /* :40-93 — for each sub-D (flag, instance-id) pair, resolve the target
     * layer by hdr+0x00 id and append its cell coords to the record. */
    for (uint32_t k = 0; k < l->n_d && l->d != NULL; k++) {
        const uint8_t *ent = l->d + (size_t)k * 8;
        uint32_t ef = (uint32_t)(ent[0] | (ent[1] << 8) | (ent[2] << 16) |
                                 (ent[3] << 24));
        if (ef != 1 && !all_links)
            continue;
        uint32_t target = (uint32_t)(ent[4] | (ent[5] << 8) | (ent[6] << 16) |
                                     (ent[7] << 24));
        for (uint32_t j = 0; j < m->count; j++) {
            const map_layer *t = &m->layers[j];
            if (layer_hdr_u32(t, 0x00) != target)
                continue;
            uint16_t n;
            memcpy(&n, grid + rec + 0x00, 2);
            if (n > 3)                     /* retail: fatal; port: stop */
                return;
            int32_t tx = ((int32_t)layer_hdr_u32(t, 0x04) * 100) / 0xc80;
            int32_t ty = ((int32_t)layer_hdr_u32(t, 0x08) * 100) / 0xc80;
            memcpy(grid + rec + 0x08 + (size_t)n * 8 + 0, &tx, 4);
            memcpy(grid + rec + 0x08 + (size_t)n * 8 + 4, &ty, 4);
            n = (uint16_t)(n + 1);
            memcpy(grid + rec + 0x00, &n, 2);
            break;
        }
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

    /* 587e00.c:3185-3204 — the trailing PLACEHOLDER pass: walk the object
     * layers (0x58c8c0 count / 0x58c8d0 accessor) and fill region-E for the
     * 0x15f9a (all-links) / 0x15f9b (flagged-links) anchors via FUN_0058cb30. */
    for (uint32_t i = 0; i < m->count && m->layers != NULL; i++) {
        uint32_t code = layer_hdr_u32(&m->layers[i], 0x10);
        if (code == 0x15f9a)
            decode_placeholder_layer(m, grid, &m->layers[i], 1);
        else if (code == 0x15f9b)
            decode_placeholder_layer(m, grid, &m->layers[i], 0);
    }
}
