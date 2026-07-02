/* collision.c — the in-game tile-collision read-side (chips 2+3).  See collision.h.
 *
 * Faithful ports of FUN_0054e990 (54e990.c, the vertical tile mover) and
 * FUN_0054ded0 (54ded0.c, the horizontal tile sweep with the stair step-up /
 * floor-hug step-down) + the FUN_0054db10 tile-half wrapper.  The decompiled
 * arithmetic IS the spec; the sign-clamp bit-idioms ( ((int)x<0)-1 & x  ==
 * max(x,0) ) and the slope-contact predicate are preserved verbatim so the
 * behaviour matches retail exactly. */
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

/* The 54ded0 COLUMN-SCAN cell class (54ded0.c:171-201): the leading-edge
 * vertical scan resolves a sloped cell to its raw class only BELOW the surface
 * (0x20 - h < suby), else 0 — and the caller blocks ONLY on class 10 (class 1
 * slope surfaces never block the column scan; the bottom-corner test handles
 * them).  The slope byte is indexed by the LEADING EDGE's sub-X (wx), the
 * sub-Y by the row iterator (wy). */
static int16_t coll_cell_class_col(const uint8_t *grid, int32_t wx, int32_t wy,
                                   coll_slope_fn slope, void *ctx)
{
    const uint8_t *recB = map_grid_obj_record(grid, wx / 0xc80, wy / 0xc80);
    uint16_t cls;  memcpy(&cls,  recB + 0x0, 2);
    uint32_t sref; memcpy(&sref, recB + 0x8, 4);
    if (sref == 0) return (int16_t)cls;
    int h = slope ? slope(ctx, sref, (wx % 0xc80) / 100) : 0;
    return (0x20 - h < (wy % 0xc80) / 100) ? (int16_t)cls : 0;
}

int collision_sweep_horizontal(const uint8_t *grid,
                               int32_t x, int32_t width,
                               int32_t y_top, int32_t y_bot,
                               int32_t delta, int step_down, int step_up,
                               int pass_slopes,
                               coll_slope_fn slope, void *ctx,
                               int32_t *out_x, int32_t *inout_y)
{
    const int32_t dim0_px = cg_i32(grid, MG_DIM0_PX);
    const int32_t dim1_px = cg_i32(grid, MG_DIM1_PX);

    int32_t mag = delta < 0 ? -delta : delta;          /* iVar6 = abs(delta) */

    /* Y-extent [top, bot] clamped ONCE to [0, DIM1_PX-1] (54ded0.c:29-38);
     * the step-up/down shifts then mutate it raw, as retail does. */
    int32_t top = clamp_lo0(y_top);
    if (top > dim1_px - 1) top = dim1_px - 1;
    int32_t bot = clamp_lo0(y_bot);
    if (bot > dim1_px - 1) bot = dim1_px - 1;

    int ok = 1;                                        /* bVar15 */
    int32_t yshift = 0;                                /* param_4 accumulator */
    int32_t step_dir = 0, lead_off = 0;
    if (delta < 0) { step_dir = -1; lead_off = 0; }
    if (delta > 0) { step_dir =  1; lead_off = width - 1; }
    int32_t x_run = x;                                 /* _param_10 */

    while (mag > 0) {
        int32_t step = mag < 100 ? mag : 100;
        mag -= step;
        x_run += step * step_dir;

        int32_t lead = clamp_lo0(x_run + lead_off);    /* uVar11: leading edge */
        if (lead > dim0_px - 1) lead = dim0_px - 1;

        /* ── STEP-UP (54ded0.c:65,110-165): leading bottom corner blocked +
         * the cell one sub-row up clear -> raise the whole window 100 units
         * (climbing a stair riser mid-stride).  Both blocked -> still try the
         * step-down check (a slope-contact bottom can slide down). ── */
        if (step_up != 0 && bot - 100 >= 0) {
            if (coll_cell_blocks(grid, lead, bot / 0xc80, (bot % 0xc80) / 100,
                                 pass_slopes, slope, ctx)) {
                int32_t up = bot - 100;
                if (!coll_cell_blocks(grid, lead, up / 0xc80,
                                      (up % 0xc80) / 100,
                                      pass_slopes, slope, ctx)) {
                    top -= 100; yshift -= 100; bot = up;
                    goto column_scan;
                }
                goto step_down_check;
            }
            /* corner clear -> fall through to the step-down check */
        }

step_down_check:
        /* ── STEP-DOWN / floor hug (54ded0.c:66-108): step the window down
         * 100 units only when ground exists within 2 sub-rows below (some
         * cell blocked at bot+200, or bot+200 out of bounds) AND the row one
         * below is entirely clear across the full X footprint. ── */
        if (step_down != 0) {
            int32_t left = clamp_lo0(x_run);
            if (left > dim0_px - 1) left = dim0_px - 1;
            int32_t right = clamp_lo0(x_run - 1 + width);
            if (right > dim0_px - 1) right = dim0_px - 1;

            int32_t below2 = bot + 200;
            if (below2 < dim1_px) {
                int32_t wx;
                int any = 0;
                for (wx = left; wx < right; wx += 0xc80) {
                    if (coll_cell_blocks(grid, wx, below2 / 0xc80,
                                         (below2 % 0xc80) / 100,
                                         pass_slopes, slope, ctx)) {
                        any = 1;
                        break;
                    }
                }
                if (!any &&
                    !coll_cell_blocks(grid, right, below2 / 0xc80,
                                      (below2 % 0xc80) / 100,
                                      pass_slopes, slope, ctx))
                    goto column_scan;      /* nothing below -> gravity's job */
            }

            int32_t below1 = bot + 100;
            if (below1 < dim1_px) {
                int32_t wx;
                int blocked1 = 0;
                for (wx = left; wx < right; wx += 0xc80) {
                    if (coll_cell_blocks(grid, wx, below1 / 0xc80,
                                         (below1 % 0xc80) / 100,
                                         pass_slopes, slope, ctx)) {
                        blocked1 = 1;
                        break;
                    }
                }
                if (!blocked1 &&
                    !coll_cell_blocks(grid, right, below1 / 0xc80,
                                      (below1 % 0xc80) / 100,
                                      pass_slopes, slope, ctx)) {
                    top += 100; yshift += 100; bot = below1;
                }
            }
        }

column_scan:
        /* ── The leading-edge COLUMN scan (54ded0.c:166-204): class 10
         * anywhere in [top, bot] (bot tested separately) blocks the sweep and
         * returns 0 IMMEDIATELY — prior steps' *out_x writes stand (partial
         * movement commits) and the accumulated yshift is DISCARDED, exactly
         * as retail's per-step write-through to &body.x does. ── */
        ok = 1;
        if (top < bot) {
            int32_t wy;
            for (wy = top; wy < bot; wy += 0xc80) {
                if (coll_cell_class_col(grid, lead, wy, slope, ctx) == 10) {
                    ok = 0;
                    break;
                }
            }
        }
        if (coll_cell_class_col(grid, lead, bot, slope, ctx) == 10 || !ok)
            return 0;

        /* clear step — clamp x_run to [0, DIM0_PX - width] and write through
         * (54ded0.c:205-215); leaving bounds makes the sweep report blocked. */
        ok = x_run >= 0;
        if (!ok) x_run = 0;
        {
            int32_t maxx = dim0_px - width;
            int over = maxx < x_run;
            if (over) x_run = maxx;
            ok = !over && ok;
        }
        *out_x = x_run;
    }

    if (yshift != 0) *inout_y += yshift;
    return ok;
}

int collision_move_horizontal(const uint8_t *grid, const phys_box *box,
                              int32_t delta, int step_down, int step_up,
                              int pass_slopes, coll_slope_fn slope, void *ctx,
                              int32_t *inout_x, int32_t *inout_y)
{
    if (inout_x != NULL) *inout_x = box->x;   /* always-defined (retail's out
                                                 ptr IS &body.x, unwritten on a
                                                 first-step block) */
    if (delta == 0) return 1;                 /* 54db10.c:32-34 */

    /* PORT-DEBT(mover-actor-scan): 54db10.c:35-143 — the actor-vs-actor
     * pre-scan over the world actor list (world+0x278c; frame-box overlap +
     * the +0x104/+0x108 collision-mode gates + the 0x4416c0 faction check).
     * The port has no collidable-actor registry yet; with an empty list the
     * loop is a no-op, so only the tile half runs here. */

    return collision_sweep_horizontal(grid, box->x, box->width,
                                      box->y + box->margin,
                                      box->y + box->height - 1,
                                      delta, step_down, step_up, pass_slopes,
                                      slope, ctx, inout_x, inout_y);
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
