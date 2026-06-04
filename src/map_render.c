/*
 * map_render.c — see map_render.h for the full model.  Faithful port of the
 * pure geometry of FUN_00490f30 (the static tilemap render walk).
 */
#include "map_render.h"
#include "map_grid.h"

#include <string.h>

/* Region-A field reads (same layout map_grid_emit_tile writes). */
static uint16_t ra_u16(const uint8_t *g, size_t off)
{
    uint16_t v;
    memcpy(&v, g + off, 2);
    return v;
}
static int32_t ra_i32(const uint8_t *g, size_t off)
{
    int32_t v;
    memcpy(&v, g + off, 4);
    return v;
}

/*
 * The decomp's negative clamp idiom `uVar = ((int)uVar < 0) - 1 & uVar`:
 *   v <  0 -> (1) - 1 = 0,           0 & v = 0
 *   v >= 0 -> (0) - 1 = 0xffffffff,  -1 & v = v
 * i.e. max(v, 0).
 */
static int32_t clamp_neg(int32_t v)
{
    return v < 0 ? 0 : v;
}

void map_render_visible_window(const mr_camera *cam,
                               int32_t dim0, int32_t dim1,
                               mr_window *out)
{
    /* First visible column (490f30.c:40-46). */
    int32_t col0 = (cam->off60 + cam->off34) / (int32_t)MG_PX_PER_DIM - 1;
    col0 = clamp_neg(col0);
    int32_t ncols = dim0 - col0;
    int32_t cap_c = cam->off64 / (int32_t)MG_PX_PER_DIM + 2;
    if (cap_c <= ncols)
        ncols = cap_c;

    /* First visible row (490f30.c:47-54).  The row origin sums three view
     * components, one scaled x100, before the /0xc80. */
    int32_t row0 = (cam->off5c + cam->off74 * 100 + cam->off4c)
                       / (int32_t)MG_PX_PER_DIM - 1;
    row0 = clamp_neg(row0);
    int32_t nrows = dim1 - row0;
    int32_t cap_r = cam->off68 / (int32_t)MG_PX_PER_DIM + 2;
    if (cap_r <= nrows)
        nrows = cap_r;

    out->col0  = col0;
    out->row0  = row0;
    out->ncols = ncols;
    out->nrows = nrows;
}

uint32_t map_render_grid_index(int32_t col, int32_t row)
{
    /* 490f30.c:64 — iVar11 = col*0x80 + row (the inner loop advances the
     * region pointer by one column = +0x80 cells). */
    return (uint32_t)(col * (int32_t)MG_ROW_PITCH + row);
}

int map_render_tile(const uint8_t *grid, int32_t col, int32_t row, int slot,
                    mr_tile *out)
{
    uint32_t idx  = map_render_grid_index(col, row);
    size_t   base = MG_REGION_A + (size_t)slot * 0x10 + (size_t)idx * 0x40;

    uint16_t bank = ra_u16(grid, base + 0x0);
    if (bank == 0)
        return 0;   /* empty sub-slot — 490f30 `if (uVar1 != 0)` skips it */

    int32_t dx = ra_i32(grid, base + 0x8);
    int32_t dy = ra_i32(grid, base + 0xc);

    out->bank  = bank;
    out->frame = ra_u16(grid, base + 0x2);
    out->layer = ra_u16(grid, base + 0x4);
    /* Destination = the tile's world origin (490f30: local_838 / local_83c). */
    out->dst_x = col * (int32_t)MG_PX_PER_DIM;
    out->dst_y = row * (int32_t)MG_PX_PER_DIM;
    /* Source-rect offset = the (dx,dy)-th 32-px sub-tile (node +0x2c/+0x30). */
    out->src_x = (dx * (int32_t)MG_PX_PER_DIM) / 100;
    out->src_y = (dy * (int32_t)MG_PX_PER_DIM) / 100;
    out->w = 0x20;   /* node +0x34 */
    out->h = 0x20;   /* node +0x38 */
    return 1;
}
