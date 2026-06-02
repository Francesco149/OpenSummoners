/*
 * src/newgame_box.c — the 9-slice box panel render (see newgame_box.h).
 *
 * Faithful port of FUN_0048cf80's opaque (param_6==0) arm.  Retail's loop
 * structure, per row:
 *   - left corner/edge at x0
 *   - (W/cell - 2) full center/edge tiles, stepping +cell from x0+cell
 *   - one partial tile of width (W % cell) at x0 + (W/cell - 1)*cell  [if > 0]
 *   - right corner/edge at (x0 - cell) + W
 * and vertically: top row (cell high), (H/cell - 2) full middle rows (cell
 * high), one partial middle row of height (H % cell), then the bottom row.
 * The corner/edge frame ids per row come from FUN_0048cf80's +0x60..+0x72
 * field reads (here the caller-supplied frames[9]).
 */

#include <stddef.h>

#include "newgame_box.h"

/* One horizontal run: left frame at x0, mid frame tiled across the interior
 * (full tiles + a partial remainder), right frame at the far edge.  `rowh` is
 * the row height (cell for full rows, H%cell for the partial middle row).
 * Mirrors FUN_0048d670 (edges) / FUN_0048d3d0 (bottom), keyed/opaque arm. */
static void box_row(const newgame_box_ops *ops,
                    void *f_left, void *f_mid, void *f_right,
                    int x0, int y, int w, int cell, int rowh)
{
    int ntiles = w / cell;          /* e.g. 400/32 = 12          */
    int last   = ntiles - 1;        /* index of the right corner */

    /* left corner/edge */
    if (f_left != NULL)
        ops->blt(ops->user, f_left, x0, y, cell, rowh);

    /* (ntiles - 2) full center/edge tiles, from x0+cell stepping +cell */
    if (last > 1 && f_mid != NULL) {
        int x = x0;
        for (int k = ntiles - 2; k > 0; k--) {
            x += cell;
            ops->blt(ops->user, f_mid, x, y, cell, rowh);
        }
    }

    /* partial last center/edge tile (width w % cell), if any */
    int rem = w % cell;
    if (rem > 0 && f_mid != NULL)
        ops->blt(ops->user, f_mid, last * cell + x0, y, rem, rowh);

    /* right corner/edge at (x0 - cell) + w */
    if (f_right != NULL)
        ops->blt(ops->user, f_right, (x0 - cell) + w, y, cell, rowh);
}

void newgame_box_render(const newgame_box_ops *ops,
                        int x0, int y0, int w, int h,
                        const int frames[9], int cell)
{
    if (ops == NULL || ops->frame == NULL || ops->blt == NULL)
        return;

    /* FUN_0048cf80 guard: render only if the box is at least one full corner
     * pair wide OR tall (and the bank is present — checked via frame()). */
    if (!(cell * 2 <= w || cell * 2 <= h))
        return;

    /* Centering when an extent is below a corner pair (W<2·cell / H<2·cell):
     * shift the origin back by half the deficit and clamp to the pair size.
     * Faithful to FUN_0048cf80; the config boxes are both larger, so this is a
     * no-op for them. */
    if (w < cell * 2) { x0 -= (cell * 2 - w) / 2; w = cell * 2; }
    if (h < cell * 2) { y0 -= (cell * 2 - h) / 2; h = cell * 2; }

    /* Resolve the nine slice frames once. */
    void *f[9];
    for (int i = 0; i < 9; i++)
        f[i] = ops->frame(ops->user, frames[i]);

    /* Top row: TL / top / TR, cell high. */
    box_row(ops, f[NEWGAME_BOX_TL], f[NEWGAME_BOX_TOP], f[NEWGAME_BOX_TR],
            x0, y0, w, cell, cell);

    /* Full middle rows: (H/cell - 2) of them when (H/cell - 1) > 1. */
    int vtiles = h / cell;
    if (vtiles - 1 > 1) {
        int y = y0;
        for (int k = vtiles - 2; k > 0; k--) {
            y += cell;
            box_row(ops, f[NEWGAME_BOX_L], f[NEWGAME_BOX_CENTER], f[NEWGAME_BOX_R],
                    x0, y, w, cell, cell);
        }
    }

    /* Partial middle row of height (H % cell) at (H/cell - 1)*cell + y0. */
    int remh = h % cell;
    if (remh > 0) {
        int y = (vtiles - 1) * cell + y0;
        box_row(ops, f[NEWGAME_BOX_L], f[NEWGAME_BOX_CENTER], f[NEWGAME_BOX_R],
                x0, y, w, cell, remh);
    }

    /* Bottom row: BL / bottom / BR at (y0 - cell) + h, cell high. */
    box_row(ops, f[NEWGAME_BOX_BL], f[NEWGAME_BOX_BOTTOM], f[NEWGAME_BOX_BR],
            x0, (y0 - cell) + h, w, cell, cell);
}
