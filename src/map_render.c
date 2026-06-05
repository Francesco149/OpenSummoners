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

void map_render_camera_init(mr_camera *cam,
                            int32_t map_pixel_w, int32_t map_pixel_h)
{
    /* 586010:854-861 — the view-object viewport/origin init, mr_camera subset.
     * view[0]=map_pixel_w (:856) and view[1]=map_pixel_h (:862) are written on
     * the full 0x78-byte object but are not part of this projection. */
    (void)map_pixel_w;
    (void)map_pixel_h;
    cam->off34 = 0;        /* 587d30 zeroes the +0x24 sub-block (incl +0x34)   */
    cam->off4c = 0;        /* 587d30 zeroes the +0x3c sub-block (incl +0x4c)   */
    cam->off5c = 0;        /* 586010:858  puVar17[0x17] = 0                    */
    cam->off60 = 0;        /* 586010:857  puVar17[0x18] = 0                    */
    cam->off64 = 64000;    /* 586010:860  puVar17[0x19] = 64000 (640*100)      */
    cam->off68 = 48000;    /* 586010:861  puVar17[0x1a] = 48000 (480*100)      */
    cam->off74 = 0;        /* 586010:859  puVar17[0x1d] = 0                    */
}

/* Live-probed opening-town first-frame camera (see header + in-game-intro.md). */
const mr_camera MAP_RENDER_CAM_TOWN_3F2 = {
    .off34 = 0,
    .off4c = 0,
    .off5c = 12800,    /* 4 cells down  (4  * 0xc80)  — spawn snap            */
    .off60 = 128000,   /* 40 cells right (40 * 0xc80) — spawn snap            */
    .off64 = 64000,    /* 640*100 viewport (== map_render_camera_init)        */
    .off68 = 48000,    /* 480*100 viewport                                    */
    .off74 = 0,
};

/* Live-probed SETTLED opening-town camera (pan end state; see header). */
const mr_camera MAP_RENDER_CAM_TOWN_3F2_SETTLED = {
    .off34 = 0,
    .off4c = 0,
    .off5c = 12800,    /* maph-vph clamp (unchanged from the spawn snap)      */
    .off60 = 12800,    /* 4 cells right — town's left edge (pan target)       */
    .off64 = 64000,    /* 640*100 viewport                                    */
    .off68 = 48000,    /* 480*100 viewport                                    */
    .off74 = 0,
};

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

int map_render_walk(const uint8_t *grid, const mr_camera *cam,
                    int32_t dim0, int32_t dim1,
                    draw_pool *pool, mr_sprite_fn resolve, void *ud)
{
    mr_window w;
    map_render_visible_window(cam, dim0, dim1, &w);

    int emitted = 0;
    /* Outer = rows (490f30 local_814 -> uVar6+counter), inner = cols
     * (local_830 -> uVar9+counter).  ncols/nrows may be <= 0, in which case
     * the retail `if (0 < ...)` guards skip the loop entirely. */
    for (int32_t r = 0; r < w.nrows; r++) {
        int32_t row = w.row0 + r;
        for (int32_t c = 0; c < w.ncols; c++) {
            int32_t col = w.col0 + c;
            for (int slot = 0; slot < 4; slot++) {
                mr_tile t;
                if (!map_render_tile(grid, col, row, slot, &t))
                    continue;   /* empty sub-slot (bank == 0) */

                /* 490f30.c:213-216 — resolve the cel; emit only if non-NULL. */
                uint32_t sprite = resolve ? resolve(t.bank, t.frame, ud) : 0;
                if (sprite == 0)
                    continue;

                /* 490f30.c:217 — FUN_004917b0(layer, 3, sprite, dst_x, dst_y,
                 * 0, 0, 0); then the src-rect stamp at :221-224. */
                draw_node *n = draw_pool_emit(pool, t.layer, 3, sprite,
                                              t.dst_x, t.dst_y, 0, 0, 0);
                if (n) {
                    n->src_x = t.src_x;
                    n->src_y = t.src_y;
                    n->w = t.w;
                    n->h = t.h;
                    emitted++;
                }
            }
        }
    }
    return emitted;
}
