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

/* One sim-tick of the panel slide-in (prog += 50, capped at 1000). */
int hud_slide_step(int prog)
{
    prog += HUD_SLIDE_STEP;
    return prog > HUD_SLIDE_FULL ? HUD_SLIDE_FULL : prog;
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

/* 0x495e40 glyph select: a printable glyph (' ' < c < '{') maps to
 * atlas frame c - 0x21; space and out-of-range render no cel (a -1 gap). */
int hud_glyph_frame(char c)
{
    if (c > ' ' && c < '{')
        return (int)(unsigned char)c - 0x21;
    return -1;
}

/* "%s / %d", cur right-justified width 4 (0x495dc0 width=4), then max. */
void hud_format_gauge(int cur, int max, char *buf, int buflen)
{
    char cur_s[16];
    snprintf(cur_s, sizeof cur_s, "%4d", cur);
    snprintf(buf, (size_t)buflen, "%s / %d", cur_s, max);
}

/* PORT-DEBT(hud-party-context) — see hud.h; ground-truthed off sword2.osr
 * tick 2200 by brute-force dhash sweep (findings/freeroam-hud.md §8). */
const int HUD_ITEM_ICON_FRAME[HUD_ITEM_SLOT_COUNT] = { 44, 48, 40, 36, 59, 80 };

/* FUN_004962a0: x = slot*0x20 - (hslide*200)/1000 + 0x1b8. */
int hud_item_slot_x(int slot, int hslide)
{
    return slot * HUD_ITEM_SLOT_STEP - (hslide * 200) / 1000 + HUD_ITEM_SLOT_DX0;
}

/* FUN_004962a0: y = (1000-vslide)*0x80/1000 + 0x1bc. */
int hud_item_slot_y(int vslide)
{
    return (1000 - vslide) * 0x80 / 1000 + HUD_ITEM_SLOT_DY_BASE;
}

/* One sim-tick of the item-bar's own slide-in (prog += 20, capped at 1000). */
int hud_item_slide_step(int prog)
{
    prog += HUD_ITEM_SLIDE_STEP;
    return prog > HUD_SLIDE_FULL ? HUD_SLIDE_FULL : prog;
}

/* ── the DOOR INDICATOR (FUN_004969b0) — see hud.h ───────────────────── */

void hud_door_dedup_reset(hud_door_dedup *d)
{
    d->n = 0;
}

/* The "center" every distance/projection test in FUN_004969b0 uses for a
 * body — X is the plain geometric center; Y is baseline-relative (the
 * FUN_0044e680 shape: (h - baseline)/2 + y + baseline), NOT a plain center.
 * Both the reach-box gate and the screen projection read the SAME formula
 * (confirmed at the asm level — FUN_0044e640/_e680 are reused verbatim). */
static int32_t hud_door_center_x(const hud_door_body *b)
{
    return b->x + b->w / 2;
}
static int32_t hud_door_center_y(const hud_door_body *b)
{
    return b->y + (b->h - b->baseline) / 2 + b->baseline;
}

/* FUN_004766a0, margin (param_5) always 0 at this call site: does the
 * candidate's world rect (top-left x,y + w,h) intersect the camera's
 * viewport?  Y uses -cam74*100 (retail's literal sign here — the position
 * projection below uses +cam74*100; the two are independently read off the
 * disasm, not assumed symmetric — currently moot since cam74 is always 0,
 * single-floor rooms). */
static int hud_door_on_screen(const hud_door_camera *cam, const hud_door_body *b)
{
    int32_t rx = b->x - cam->cam60 - cam->cam34;
    int32_t ry = b->y - cam->cam74 * 100 - cam->cam5c - cam->cam4c;
    return (rx + b->w >= 0) && (ry + b->h >= 0) &&
           (rx < cam->cam64) && (ry < cam->cam68);
}

int hud_door_process(hud_door_dedup *dedup, const hud_door_ref *ref,
                     const hud_door_camera *cam, const hud_door_candidate *cand,
                     const void *highlight_id, hud_door_draw *out)
{
    out->visible = 0;

    /* body validity + status gate (0x4969b0 top-of-loop `continue`s). */
    if (!cand->active || !cand->body_valid || cand->suppressed || cand->status != 0)
        return 0;

    /* zone/room relation gate: both non-zero, and (either side is the
     * wildcard zone 3, or the zones differ). */
    if (ref->zone == 0 || cand->zone == 0)
        return 0;
    if (ref->zone != 3 && cand->zone != 3 && ref->zone == cand->zone)
        return 0;

    /* world-space "reach" pre-filter — a 72000x56000 box around the
     * reference (roughly one screen's worth + margin). */
    int32_t dx = hud_door_center_x(&ref->body) - hud_door_center_x(&cand->body);
    int32_t dy = hud_door_center_y(&ref->body) - hud_door_center_y(&cand->body);
    int32_t adx = dx < 0 ? -dx : dx;
    int32_t ady = dy < 0 ? -dy : dy;
    if (adx >= HUD_DOOR_REACH_X || ady >= HUD_DOOR_REACH_Y)
        return 0;

    /* already on-screen -> no arrow needed (the indicator is OFF-screen-only). */
    if (hud_door_on_screen(cam, &cand->body))
        return 0;

    /* fade depth: how close to the reach-box edge (small = far/faded, large
     * = near/opaque) — min of the two axes' ratios.  Always >=0 here (adx/
     * ady are already bounded above), so retail's "ramp_raw<0 -> 0" branch
     * is unreachable and omitted (both arms read table[0] at raw<=0 — moot
     * since raw is never negative once the reach-box gate above passed). */
    int32_t x_ratio = (HUD_DOOR_REACH_X - adx) * 1000 / HUD_DOOR_FADE_X_DENOM;
    int32_t y_ratio = (HUD_DOOR_REACH_Y - ady) * 1000 / HUD_DOOR_FADE_Y_DENOM;
    int32_t depth = x_ratio < y_ratio ? x_ratio : y_ratio;

    /* project the candidate's center through the camera, /100 EACH TERM
     * separately before subtracting (retail's exact division order —
     * matters for negative operands under truncating division). */
    int32_t cx = hud_door_center_x(&cand->body);
    int32_t cy = hud_door_center_y(&cand->body);
    int32_t proj_x = cx / 100 - (cam->cam60 + cam->cam34) / 100;
    int32_t proj_y = cy / 100 - (cam->cam5c + cam->cam74 * 100 + cam->cam4c) / 100;

    int32_t vw = cam->cam64 / 100, vh = cam->cam68 / 100;
    int32_t ax = proj_x < 0 ? 0 : (proj_x > vw ? vw : proj_x);
    int32_t ay = proj_y < 0 ? 0 : (proj_y > vh ? vh : proj_y);

    /* edge direction: sequential overrides (NOT else-if) — LEFT beats RIGHT
     * beats BOTTOM beats the TOP default, exactly as retail's 3 back-to-back
     * ifs. */
    int edge = HUD_DOOR_EDGE_TOP;
    if (ay == vh) edge = HUD_DOOR_EDGE_BOTTOM;
    if (ax == vw) edge = HUD_DOOR_EDGE_RIGHT;
    if (ax == 0)  edge = HUD_DOOR_EDGE_LEFT;

    /* dedup: find an existing bucket within [0,5)px on both axes; else
     * allocate a new one (abort the WHOLE scan if the table is already full
     * — retail's bare `return;`, not a per-candidate skip). */
    int bi = -1;
    for (int i = 0; i < dedup->n; i++) {
        int32_t bdx = dedup->buckets[i].x - ax; if (bdx < 0) bdx = -bdx;
        int32_t bdy = dedup->buckets[i].y - ay; if (bdy < 0) bdy = -bdy;
        if (bdx < HUD_DOOR_CLUSTER_RADIUS && bdy < HUD_DOOR_CLUSTER_RADIUS) { bi = i; break; }
    }
    int stack;
    if (bi < 0) {
        if (dedup->n >= HUD_DOOR_MAX_BUCKETS)
            return -1;                    /* table exhausted -> abort the scan */
        bi = dedup->n++;
        dedup->buckets[bi].count = 0;
        stack = 0;
    } else {
        dedup->buckets[bi].count++;
        stack = dedup->buckets[bi].count;
    }
    dedup->buckets[bi].x = ax;
    dedup->buckets[bi].y = ay;

    /* stack>=5 (retail: stack!=4 && stack>3, identical for stack>=0): the
     * bucket updated above, but this 6th+ duplicate doesn't draw. */
    if (stack != 4 && stack > 3)
        return 0;

    /* stacked duplicates spread PERPENDICULAR to their edge, INTO the
     * screen (not along the edge) — 12px per stack level. */
    switch (edge) {
    case HUD_DOOR_EDGE_TOP:    ay += stack * HUD_DOOR_STACK_OFFSET; break;
    case HUD_DOOR_EDGE_RIGHT:  ax -= stack * HUD_DOOR_STACK_OFFSET; break;
    case HUD_DOOR_EDGE_BOTTOM: ay -= stack * HUD_DOOR_STACK_OFFSET; break;
    case HUD_DOOR_EDGE_LEFT:   ax += stack * HUD_DOOR_STACK_OFFSET; break;
    }

    int dir = edge;
    if (highlight_id != NULL && highlight_id == cand->id)
        dir += 4;

    int32_t ramp_raw = depth * HUD_DOOR_RAMP_COUNT / 1000;
    out->ramp_idx = (ramp_raw < HUD_DOOR_RAMP_COUNT) ? (int)ramp_raw : -1;
    out->frame_index = dir + 4;
    out->cx = ax;
    out->cy = ay;
    out->visible = 1;
    return 0;
}
