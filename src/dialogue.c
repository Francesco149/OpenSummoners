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
                /* move the partial word down a row */
                char tail[DIALOGUE_ROW_CHARS + 1];
                int  tail_len = col - (brk + 1);
                memcpy(tail, &rows[r][brk + 1], (size_t)tail_len);
                rows[r][brk] = '\0';
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

void dialogue_arm(dialogue_box *d, const char *name, const char *text)
{
    memset(d, 0, sizeof(*d));
    d->active        = 1;
    d->portrait_slot = -1;   /* none until the caller resolves one (portrait.c) */
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

int dialogue_active(const dialogue_box *d)          { return d->active; }
int dialogue_content_visible(const dialogue_box *d) { return d->active && d->scale >= 1000; }

int dialogue_awaiting_advance(const dialogue_box *d)
{
    return dialogue_content_visible(d) && d->reveal >= d->total;
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
    /* portrait cross-fade (0x49c910, gated on scale==1000) */
    if (d->portrait_fade < 1000) {
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

void dialogue_scaled_rect(const dialogue_box *d,
                          int *x, int *y, int *w, int *h)
{
    /* 0x48c820 mode +0x1c==1: sw = w*scale/1000 (integer), centered. */
    int sw = (DIALOGUE_BOX_W * d->scale) / 1000;
    int sh = (DIALOGUE_BOX_H * d->scale) / 1000;
    *x = DIALOGUE_BOX_X + (DIALOGUE_BOX_W / 2 - sw / 2);
    *y = DIALOGUE_BOX_Y + (DIALOGUE_BOX_H / 2 - sh / 2);
    *w = sw;
    *h = sh;
}

int dialogue_portrait_ramp_index(const dialogue_box *d)
{
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
