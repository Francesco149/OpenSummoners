/*
 * dialogue.c — the in-game dialogue box state machine (see dialogue.h).
 *
 * Pure C, host-testable; the render half lives in the caller's sinks
 * (main.c game_render_dialogue).
 */
#include "dialogue.h"

#include <string.h>

/* ── text expand: %n breaks + word wrap (0x43cf90's subset) ─────────────── */

int dialogue_expand_text(const char *src,
                         char rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1])
{
    int r = 0, col = 0;
    memset(rows, 0, (size_t)DIALOGUE_MAX_ROWS * (DIALOGUE_ROW_CHARS + 1));
    if (src == NULL)
        return 0;

    while (*src != '\0' && r < DIALOGUE_MAX_ROWS) {
        if (src[0] == '%' && src[1] == 'n') {       /* forced break */
            src += 2;
            rows[r][col] = '\0';
            r++; col = 0;
            continue;
        }
        if (col >= DIALOGUE_ROW_CHARS) {            /* word wrap */
            /* back up to the last space in this row; the wrap space is
             * consumed (it becomes the break). */
            int brk = col - 1;
            while (brk > 0 && rows[r][brk] != ' ')
                brk--;
            if (brk > 0) {
                /* move the partial word down a row, KEEPING the wrap space at the
                 * end of this row — retail's word-wrap renders the trailing space
                 * (retail.osr: row "Look, Arche. This is our new " ends in a
                 * space, then "hometown." starts the next row), so the body is
                 * byte-identical.  The space's reveal grade is 1 and it sits past
                 * every line's skip point, so the typewriter cadence is unchanged
                 * (plans/intro-cutscene-1to1.md THEME 1). */
                char tail[DIALOGUE_ROW_CHARS + 1];
                int  tail_len = col - (brk + 1);
                memcpy(tail, &rows[r][brk + 1], (size_t)tail_len);
                rows[r][brk + 1] = '\0';   /* keep the space at brk, terminate after it */
                r++; col = 0;
                if (r >= DIALOGUE_MAX_ROWS)
                    break;
                memcpy(rows[r], tail, (size_t)tail_len);
                col = tail_len;
            } else {
                rows[r][col] = '\0';
                r++; col = 0;
                if (r >= DIALOGUE_MAX_ROWS)
                    break;
            }
            continue;                               /* re-test *src */
        }
        rows[r][col++] = *src++;
    }
    if (r < DIALOGUE_MAX_ROWS) {
        rows[r][col] = '\0';
        if (col > 0 || r == 0)
            r++;
    }
    return r;
}

/* ── arm / step ──────────────────────────────────────────────────────────── */

/* The run-off box-close slide curve (header doc): the box-corner screen offset
 * (dx,dy) from its frozen open position, per tick over the 21-tick slide-off,
 * measured off retail.osr (the "Cool!" empty frame sliding off lower-right). */
const int16_t DIALOGUE_RUNOFF_SLIDE[DIALOGUE_RUNOFF_SLIDE_N][2] = {
    {9,3},{17,5},{25,7},{33,9},{41,12},{49,14},{58,16},{66,18},{74,21},{82,23},
    {90,24},{98,24},{107,25},{115,25},{123,24},{131,24},{139,25},{147,25},
    {156,24},{164,24},{172,25},
};

void dialogue_arm(dialogue_box *d, const char *name, const char *text)
{
    memset(d, 0, sizeof(*d));
    d->active        = 1;
    d->portrait_slot = -1;   /* none until the caller resolves one (portrait.c) */
    d->slide_idx     = -1;   /* not sliding (memset 0 would mean slide tick 0)   */
    d->row_count = dialogue_expand_text(text, d->rows);
    for (int i = 0; i < d->row_count; i++)
        d->total += (int32_t)strlen(d->rows[i]);
    if (name != NULL) {
        strncpy(d->name, name, sizeof(d->name) - 1);
        d->name[sizeof(d->name) - 1] = '\0';
    }
    /* The first char reveals one update after the content gate opens (the
     * probe: pop done ~2775, name 2776, 'A' 2777). */
    d->type_timer = 1;
}

void dialogue_set_text(dialogue_box *d, const char *text)
{
    if (!d->active)
        return;
    /* Reset ONLY the typewriter — the box stays open (scale, name, portrait,
     * portrait_fade, arrow, anchor all persist).  Retail's same-speaker page
     * advance: the body clears and re-types from the next tick, no pop-in. */
    memset(d->rows, 0, sizeof d->rows);
    d->row_count = dialogue_expand_text(text, d->rows);
    d->total = 0;
    for (int i = 0; i < d->row_count; i++)
        d->total += (int32_t)strlen(d->rows[i]);
    d->reveal     = 0;
    d->type_timer = 1;   /* the first char reveals one update later (as dialogue_arm) */
}

void dialogue_close_step(dialogue_box *d)
{
    if (!d->active)
        return;
    /* Pop-OUT: shrink the centered-scale frame by 40/update (MEASURED, vs the
     * +50 pop-in).  Content (text/portrait) stops drawing the instant scale
     * drops below 1000 (the dialogue_content_visible gate), so only the
     * shrinking frame remains — retail's disappearing box.  Removed once it
     * drops below DIALOGUE_CLOSE_MIN (retail's last-drawn ~160). */
    d->scale -= DIALOGUE_CLOSE_STEP;
    if (d->scale < DIALOGUE_CLOSE_MIN)
        d->active = 0;
}

void dialogue_arm_fadeout(dialogue_box *d)
{
    if (!d->active)
        return;
    /* The reverse cross-fade (0x49c910 state 2/3): the dissolving bust starts at
     * idx 18 (DIALOGUE_FADEOUT_START) and walks down via dialogue_fadeout_step.
     * The box keeps its name/text/scale (still full) — only the portrait fades;
     * the frame pop-OUT is the caller's separate dialogue_close_step. */
    d->fade_out      = 1;
    d->portrait_fade = DIALOGUE_FADEOUT_START;
}

void dialogue_fadeout_step(dialogue_box *d)
{
    if (!d->active || !d->fade_out)
        return;
    d->portrait_fade -= DIALOGUE_FADE_STEP;     /* reverse ramp 450 -> 0 */
    if (d->portrait_fade < 0)
        d->portrait_fade = 0;
}

void dialogue_reopen(dialogue_box *d, const char *name, const char *text)
{
    /* A fresh line opening like the first box — from DIALOGUE_OPEN_SCALE0,
     * +50/update to 1000 (~16 updates).  The OLD box closes separately (the
     * cutscene's `closing` box), drawn BEHIND this one (retail's z-order). */
    dialogue_arm(d, name, text);
    d->scale = DIALOGUE_OPEN_SCALE0;
}

int dialogue_active(const dialogue_box *d)          { return d->active; }
int dialogue_content_visible(const dialogue_box *d) { return d->active && d->scale >= 1000; }

int dialogue_awaiting_advance(const dialogue_box *d)
{
    return dialogue_content_visible(d) && d->reveal >= d->total;
}

int dialogue_typing(const dialogue_box *d)
{
    return dialogue_content_visible(d) && d->reveal < d->total;
}

void dialogue_skip_reveal(dialogue_box *d)
{
    if (!dialogue_typing(d))
        return;
    /* FUN_0043ca40(9): jump the reveal to the end (the text machine's display
     * extent snaps to the full line).  reveal>=total then gates the typewriter
     * block off in dialogue_step and flips dialogue_awaiting_advance true. */
    d->reveal = d->total;
}

int dialogue_char_grade(char c)
{
    if (c == ' ')
        return 1;
    if (c == ',' || c == '.' || c == '!' || c == '?')
        return 3 * DIALOGUE_TYPE_INTERVAL;
    return DIALOGUE_TYPE_INTERVAL;
}

/* The row index + in-row offset of flat reveal cursor `n` (0-based: the n-th
 * revealable char). */
static int reveal_locate(const dialogue_box *d, int n, int *row, int *off)
{
    for (int r = 0; r < d->row_count; r++) {
        int len = (int)strlen(d->rows[r]);
        if (n < len) {
            *row = r;
            *off = n;
            return 1;
        }
        n -= len;
    }
    return 0;
}

void dialogue_step(dialogue_box *d)
{
    if (!d->active)
        return;
    if (d->scale < 1000) {                          /* pop-in (+0x54 += 50) */
        d->scale += DIALOGUE_SCALE_STEP;
        if (d->scale > 1000)
            d->scale = 1000;
        return;                                     /* content waits */
    }
    /* portrait cross-fade (0x49c910, gated on scale==1000).  The cross-fade
     * state (retail uVar1 at +0x2e) arms to 1 only on the FIRST fully-open tick:
     * while still 0 the 0x49c910 update returns early WITHOUT advancing the
     * fade counter, so the portrait renders one extra tick at idx 0 (the dimmest
     * step) before the ramp starts.  DRAWCALL-EXACT vs retail.osr (L0 ticks
     * 660-671): retail holds idx 0 for two opening ticks (660,661) then 2,4,..,18
     * (662-670), opaque at 671; without this arm the port advanced a tick early
     * (idx 2 at 661 = USER's "slightly less dim", maxd 17).  See engine-quirk. */
    if (!d->fade_armed) {
        d->fade_armed = 1;          /* this tick renders idx 0; fade starts next */
    } else if (d->portrait_fade < 1000) {
        d->portrait_fade += DIALOGUE_FADE_STEP;
        if (d->portrait_fade > 1000)
            d->portrait_fade = 1000;
    }
    /* arrow anim (0x410560: interval 10, table {0,1,2,3}) */
    d->arrow_timer++;
    if (d->arrow_timer >= DIALOGUE_ARROW_PERIOD) {
        d->arrow_timer = 0;
        d->arrow_idx   = (d->arrow_idx + 1) & 3;
    }
    /* typewriter reveal */
    if (d->reveal < d->total) {
        d->type_timer--;
        if (d->type_timer <= 0) {
            int row = 0, off = 0;
            (void)reveal_locate(d, d->reveal, &row, &off);
            char c = d->rows[row][off];
            d->reveal++;
            d->type_timer = dialogue_char_grade(c);
            /* a row close adds one extra interval (fitted: '!' -> next row
             * ~20 updates = grade 15 + 5) */
            if (off + 1 >= (int)strlen(d->rows[row]) && row + 1 < d->row_count)
                d->type_timer += DIALOGUE_TYPE_INTERVAL;
        }
    }
}

/* ── render queries ──────────────────────────────────────────────────────── */

void dialogue_scaled_rect(const dialogue_box *d, int box_x, int box_y,
                          int *x, int *y, int *w, int *h)
{
    /* 0x48c820 mode +0x1c==1: sw = w*scale/1000 (integer), centered in the box's
     * full rect at its live anchor (box_x/box_y = the speaker-anchored pos the
     * caller resolved via dialogue_box_position). */
    int sw = (DIALOGUE_BOX_W * d->scale) / 1000;
    int sh = (DIALOGUE_BOX_H * d->scale) / 1000;
    *x = box_x + (DIALOGUE_BOX_W / 2 - sw / 2);
    *y = box_y + (DIALOGUE_BOX_H / 2 - sh / 2);
    *w = sw;
    *h = sh;
}

/* FUN_0049c640 (the box-anchor) over FUN_00490b90 (the world->screen projection,
 * param_9==1 arm).  HARNESS-VERIFIED bit-exact against all 18 town-intro lines
 * (runs/box-pos-inputs); see engine-quirks #106. */
void dialogue_box_position(const dialogue_box *d,
                           int32_t box_w, int32_t box_h,
                           int32_t cam_x, int32_t cam_y, int32_t cam_scroll,
                           int *out_x, int *out_y, int *out_tail_x)
{
    if (!d->anchored) {
        /* FUN_0049c640 param_6==0 — the centered box at a fixed y (no speaker). */
        *out_x = (0x280 - box_w) / 2;
        *out_y = 0x50;
        if (out_tail_x != NULL) *out_tail_x = box_w / 2 - 0x10;   /* centered tail */
        return;
    }
    const dialogue_speaker_body *b = &d->spk_body;

    /* FUN_00490b90 param_9==1 — project the speaker world pos to screen.  TWO
     * separate /100 truncating divisions (NOT (wx-cam_x)/100), faithful to the
     * decompile (param_1/100 - *(ECX+0x60)/100; param_2/100 - (off5c+off74*100)/100). */
    int32_t scr_x = d->spk_wx / 100 - cam_x / 100;
    int32_t scr_y = d->spk_wy / 100 - (cam_y + cam_scroll * 100) / 100;

    /* box_x — center the box on the speaker, clamp to [0x20, 0x260-W].  iVar6 =
     * sprite_w/200 is the sprite-center half-offset; iVar7 = the pre-clamp left. */
    int32_t iVar7  = (b->sprite_w / 200 - box_w / 2) + scr_x;
    int32_t right  = 0x260 - box_w;
    int32_t bx     = right;
    if (iVar7 <= right) bx = iVar7;
    if (bx < 0x20)            bx = 0x20;     /* below the left margin -> clamp     */
    else if (iVar7 <= right)  bx = iVar7;    /* within range -> the candidate      */
    else                      bx = right;    /* past the right margin -> clamp     */

    /* box_y — above the speaker's head (metric_14/100 + off_1c, minus the box
     * height + the 0x30 gap).  The flip-below branch (param_8 != 0 -> threshold
     * 0x40) is provably DEAD for the town intro (by >= 148 >> 0x40); ported for
     * fidelity.  All captured town lines pass param_8 == 1. */
    int32_t by = (b->metric_14 / 100 + b->off_1c) - box_h - 0x30 + scr_y;
    if (by < 0x18 + 0x28)
        by = b->metric_10 / 100 + b->off_20 + scr_y;

    *out_x = bx;
    *out_y = by;

    /* the TAIL/arrow x relative to box_x (0x49c640: local_c = speaker_center - box_x,
     * then clamp(local_c, 0x20, box_w-0x20) - 0x10).  speaker_center = scr_x +
     * sprite_w/200 = iVar7 + box_w/2 (the un-clamped box left + half).  When the box
     * is NOT clamped, box_x == iVar7 so local_c == box_w/2 and the tail centers; when
     * the box clamps to an edge, the tail stays on the speaker. */
    if (out_tail_x != NULL) {
        int32_t local_c = (iVar7 + box_w / 2) - bx;     /* speaker_center - box_x */
        int32_t tail    = local_c;
        if (tail < 0x20)            tail = 0x20;         /* clamp lo               */
        else if (tail > box_w - 0x20) tail = box_w - 0x20; /* clamp hi             */
        *out_tail_x = tail - 0x10;                       /* cel pivot (-0x10)      */
    }
}

int dialogue_portrait_ramp_index(const dialogue_box *d)
{
    if (d->fade_out) {
        /* 0x49c910 state 2/3, the speaker-change OLD bust dissolving: the SAME
         * ramp LUTs played BACKWARDS (idx 18→2 as fade falls 450→50), then GONE
         * (draw nothing) once below idx 2.  Drawcall+LUT-exact off retail.osr —
         * idx 18,16,..,2 over the 9-tick window, the bust gone the next tick (as
         * the frame starts shrinking).  quirk #108. */
        if (d->portrait_fade < 50)
            return DIALOGUE_PORTRAIT_GONE;
        return (d->portrait_fade * 0x14) / 500;
    }
    /* 0x49c910 state 1, the fade<0x1f5 arm: idx = (fade*0x14)/500; once idx
     * exceeds 0x13 the NEW cel's desc is cleared to 0 = the plain keyed blit
     * (the 0x43d470 desc setter, args (0,0,0)) — the incoming portrait is FULLY OPAQUE from
     * fade 500 (update 10); the second fade half only fades the OLD cel out
     * (absent on a first line).  Live-verified: retail's bust luminance is
     * final from mid-fade while a hold-at-19 model lagged it. */
    if (d->portrait_fade >= 500)
        return -1;
    return (d->portrait_fade * 0x14) / 500;
}

int dialogue_arrow_frame(const dialogue_box *d)
{
    static const int tbl[4] = { 0, 1, 2, 3 };       /* 0x410560 +0x2e..+0x34 */
    return DIALOGUE_ARROW_FRAME_BASE + tbl[d->arrow_idx & 3];
}

int dialogue_row_revealed(const dialogue_box *d, int r)
{
    int n = d->reveal;
    for (int i = 0; i < d->row_count && i < r; i++)
        n -= (int)strlen(d->rows[i]);
    if (n < 0)
        n = 0;
    int len = (r < d->row_count) ? (int)strlen(d->rows[r]) : 0;
    return n > len ? len : n;
}

/* The body-grid row-distribution gap — the EXACT logic of FUN_0048da70 @ 0x48da70
 * (the grid text renderer), :57-88, ported faithfully:
 *
 *   local_2c = records[+0x1c]              // = DIALOGUE_GRID_MAX_GAP (20)
 *   rows     = min(max_row_index+1, max_rows)   // the line's TOTAL row count
 *   if local_2c != 0 && rows != 0:
 *       cand     = ((max_rows - rows) * pitch) / (rows + 1)   // signed int div
 *       local_2c = min(local_2c, cand)
 *   else local_2c = 0
 *
 * `rows` is counted over ALL grid records (every glyph of the line), not the
 * typewriter-revealed subset, so the layout is fixed once the text is set. */
int dialogue_body_gap(const dialogue_box *d)
{
    int rows = d->row_count;
    if (rows <= 0 || DIALOGUE_GRID_MAX_GAP == 0)
        return 0;
    if (rows > DIALOGUE_MAX_ROWS)
        rows = DIALOGUE_MAX_ROWS;
    int cand = ((DIALOGUE_MAX_ROWS - rows) * DIALOGUE_LINE_H) / (rows + 1);
    return cand < DIALOGUE_GRID_MAX_GAP ? cand : DIALOGUE_GRID_MAX_GAP;
}

/* The box-relative Y of body row `r` (0x48da70:101):
 *   iVar11 = (r+1)*gap + pitch*r + base_y   (then the caller adds box_y = param_4). */
int dialogue_body_row_dy(const dialogue_box *d, int r)
{
    int gap = dialogue_body_gap(d);
    return DIALOGUE_TEXT_DY + (r + 1) * gap + DIALOGUE_LINE_H * r;
}
