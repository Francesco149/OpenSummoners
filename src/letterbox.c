/*
 * letterbox.c — the establishing-shot cinematic letterbox grid-fill.
 * See letterbox.h.  The decompiled arithmetic of 0x48c150:124-162 IS the
 * spec; this ports the two bar loops verbatim.
 */
#include "letterbox.h"

#include <stddef.h>

/* 0x48c150 literals. */
#define LB_SCREEN_H   0x1e0   /* 480 (the bottom bar's base = 0x1e0 - height) */
#define LB_CELL_W     0x40    /* res 0x583 width  (column pitch)              */
#define LB_CELL_H     4       /* res 0x583 height (row pitch / round-up unit) */
#define LB_COL_LIMIT  0x281   /* inner loop blits while (dx + 0x80) < 0x281   */

/* One 4px row of the bar: tile the cel left-to-right at 64px pitch
 * (0x48c150:133-138 / 153-158).
 *   dx = 0; do { blit(dx,dy); chk = dx+0x80; dx += 0x40; } while (chk < 0x281);
 * yields 10 columns dx in {0,0x40,…,0x240}. */
static void lb_row(int dy, letterbox_blit_fn blit, void *ctx)
{
    int dx = 0;
    do {
        blit(ctx, dx, dy);
        int chk = dx + 0x80;
        dx += LB_CELL_W;
        if (!(chk < LB_COL_LIMIT))
            break;
    } while (1);
}

void letterbox_render(int top_h, int bottom_h, letterbox_blit_fn blit, void *ctx)
{
    if (blit == NULL)
        return;

    /* BOTTOM bar — 0x48c150:124-142 (in_ECX[0x11] = +0x44), emitted FIRST.
     *   rounded = roundup4(bottom_h);  base = 480 - bottom_h;
     *   for local_1c in 0,4,…,<rounded:  dy = local_1c + base. */
    if (0 < bottom_h) {
        int rounded = ((bottom_h + 3 + ((bottom_h + 3) >> 31 & 3)) >> 2) * 4;
        int base    = LB_SCREEN_H - bottom_h;
        for (int local_1c = 0; local_1c < rounded; local_1c += LB_CELL_H)
            lb_row(local_1c + base, blit, ctx);
    }

    /* TOP bar — 0x48c150:143-162 (in_ECX[0x12] = +0x48), emitted SECOND.
     *   q = ceil(top_h/4);  rounded = q*4;  base = top_h - rounded;
     *   for local_1c in 0,4,…,<rounded:  dy = local_1c + base. */
    if (0 < top_h) {
        int q       = (top_h + 3 + ((top_h + 3) >> 31 & 3)) >> 2;
        int rounded = q * 4;
        int base    = top_h - rounded;
        for (int local_1c = 0; local_1c < rounded; local_1c += LB_CELL_H)
            lb_row(local_1c + base, blit, ctx);
    }
}
