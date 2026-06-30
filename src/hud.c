/* hud.c — the freeroam STATUS HUD geometry + formatting (pure C).  See hud.h.
 * Logic ported from FUN_00494e60 (orchestrator) + FUN_00498680 (HP/MP bar). */

#include "hud.h"
#include <stdio.h>

/* 0x494e60:87 — xbase = ((prog*0xb - 11000)*0x20)/1000 + 1.  Integer math
 * exactly as retail (the *0x20 before /1000). */
int hud_panel_xbase(int slide_progress)
{
    return ((slide_progress * 0xb - 11000) * 0x20) / 1000 + 1;
}

/* FUN_00498680 per-row geometry.  fill = (cur*width)/max (max clamped >=1);
 * row r: filled dst_x = x - r (param_4 decremented per row), dst_y = y + 2r;
 * the filled rect is a 1:1 copy [dst .. dst+fill] from src (0, src_y); the
 * depleted span starts at x - r + fill, width = width - fill, src_y = 14. */
hud_bar_row hud_bar_row_geom(int cur, int max, int x, int y, int width,
                             int src_y, int row)
{
    if (max < 1)
        max = 1;
    int fill = (cur * width) / max;
    if (fill < 0)
        fill = 0;
    if (fill > width)
        fill = width;

    hud_bar_row r;
    r.dst_x = x - row;
    r.dst_y = y + 2 * row;
    r.dst_w = fill;
    r.dst_h = 2;
    r.src_x = 0;
    r.src_y = src_y;
    r.src_w = fill;
    r.src_h = 2;
    r.dep_x = x - row + fill;
    r.dep_w = width - fill;
    return r;
}

/* "%s / %d", cur right-justified width 4 (0x495dc0 width=4), then max. */
void hud_format_gauge(int cur, int max, char *buf, int buflen)
{
    char cur_s[16];
    snprintf(cur_s, sizeof cur_s, "%4d", cur);
    snprintf(buf, (size_t)buflen, "%s / %d", cur_s, max);
}
