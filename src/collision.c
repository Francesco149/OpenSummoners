/* collision.c — the in-game tile-collision read-side (chip 2).  See collision.h.
 *
 * Faithful port of FUN_0054e990 (54e990.c).  The decompiled arithmetic IS the
 * spec; the sign-clamp bit-idioms ( ((int)x<0)-1 & x  ==  max(x,0) ) and the
 * slope-contact predicate are preserved verbatim so the behaviour matches
 * retail exactly. */
#include "collision.h"
#include "map_grid.h"

#include <string.h>

static inline int32_t cg_i32(const uint8_t *g, size_t off)
{ int32_t v; memcpy(&v, g + off, 4); return v; }

/* ((int)x < 0) - 1 & x  ==  max(x, 0)  (54e990.c:25 etc.). */
static inline int32_t clamp_lo0(int32_t x) { return x < 0 ? 0 : x; }

/* 54e990.c:83-86 slope-contact predicate, verbatim:
 *   d = suby - surf;  s = d >> 31;  a = d ^ s;  (a == s || (int)(a - s) < 0)
 * (it reduces to d == 0, but the bit-idiom is kept as the literal spec). */
static inline int coll_slope_contact(int32_t d)
{
    uint32_t s = (uint32_t)(d >> 31);
    uint32_t a = (uint32_t)d ^ s;
    return a == s || (int32_t)(a - s) < 0;
}

/* Does the cell at world-x `wx`, grid row `row` block the leading edge at
 * sub-tile-y `suby`?  Mirrors the per-cell body shared by 54e990's inner scan
 * (:64-92) and far-edge test (:96-120). */
static int coll_cell_blocks(const uint8_t *grid, int32_t wx, int32_t row,
                            int32_t suby, int edge_flag,
                            coll_slope_fn slope, void *ctx)
{
    int32_t col = wx / 0xc80;
    const uint8_t *recB = map_grid_obj_record(grid, col, row);
    uint16_t cls;  memcpy(&cls,  recB + 0x0, 2);
    uint32_t sref; memcpy(&sref, recB + 0x8, 4);

    int16_t klass;
    int32_t surf;
    if (sref == 0) {
        klass = (int16_t)cls;
        surf  = 0;
    } else {
        int h = slope ? slope(ctx, sref, (wx % 0xc80) / 100) : 0;
        surf = -h;
        if (surf + 0x20 < suby) { klass = (int16_t)cls; surf += 0x21; }
        else                    { klass = 0;            surf += 0x21; }
    }

    if (klass == 1)
        return coll_slope_contact(suby - surf) && edge_flag == 0;
    if (klass == 10)
        return 1;
    return 0;
}

int collision_move_vertical(const uint8_t *grid, const phys_box *box,
                            int32_t delta, int pass_slopes,
                            coll_slope_fn slope, void *ctx,
                            int32_t *out_y)
{
    const int32_t dim0_px = cg_i32(grid, MG_DIM0_PX);
    const int32_t dim1_px = cg_i32(grid, MG_DIM1_PX);

    *out_y = box->y;                       /* always-defined (upward-block case) */

    int32_t mag = delta < 0 ? -delta : delta;          /* iVar12 = abs(delta) */

    /* X-extent [near, far] clamped to [0, DIM0_PX-1]. */
    int32_t near = clamp_lo0(box->x);
    if (near > dim0_px - 1) near = dim0_px - 1;
    int32_t far = clamp_lo0(box->x - 1 + box->width);
    if (far > dim0_px - 1) far = dim0_px - 1;

    int32_t step_dir = 1, edge_off = 0, edge_flag = 0;
    if (delta < 0) { step_dir = -1; edge_off = 0;                edge_flag = 1; }
    if (delta > 0) { step_dir =  1; edge_off = box->height - 1;  edge_flag = pass_slopes != 0; }

    int32_t y = box->y;
    for (;;) {
        if (mag < 1)
            return 1;                      /* whole delta cleared */
        int32_t step = mag < 100 ? mag : 100;
        mag -= step;
        y += step * step_dir;

        int32_t edge_y = clamp_lo0(edge_off + y);
        if (edge_y > dim1_px - 1) edge_y = dim1_px - 1;
        int32_t row  = edge_y / 0xc80;
        int32_t suby = (edge_y % 0xc80) / 100;

        int blocked = 0;
        for (int32_t wx = near; wx < far; wx += 0xc80) {
            if (coll_cell_blocks(grid, wx, row, suby, edge_flag, slope, ctx)) {
                blocked = 1;
                break;
            }
        }

        int far_block = coll_cell_blocks(grid, far, row, suby, edge_flag, slope, ctx);
        if (far_block || blocked) {
            if (delta > 0)
                *out_y = (edge_y / 100) * 100 - box->height;
            return 0;
        }

        /* clear step — *out_y = y clamped to [-16000-h, DIM1_PX+16000]
         * (54e990.c:128-137, the comma-operator clamp written out). */
        int32_t lo = -16000 - box->height;
        int32_t mx = lo <= y ? y : lo;
        int32_t hi = dim1_px + 16000;
        int32_t res = hi;
        if (mx <= hi) {
            res = lo;
            if (lo <= y) res = y;
        }
        *out_y = res;
    }
}
