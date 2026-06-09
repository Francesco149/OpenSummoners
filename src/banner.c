/*
 * src/banner.c — the area-title banner phase machine + text layout (pure C).
 *
 * Direct translation of the retail slice (engine-quirk #96):
 *   - banner_step       = 0x499ab0's mode-1 sub-machine (the cinematic
 *                         step, once/sim-tick).
 *   - banner_text_layout/banner_alpha_ramp_index = the 0x494a60 case-1
 *                         length->advance/x math + the alpha->ramp gate.
 *
 * No Win32/ddraw: the cel resolve, the GDI text compose, and the blit are the
 * caller's sink (main.c game_render_banner), so this file is host-testable.
 */
#include "banner.h"

#include <string.h>

void banner_arm(area_banner *b, const char *text, int32_t hold_dur)
{
    memset(b, 0, sizeof *b);
    b->mode     = BANNER_MODE_TEXT;
    b->phase    = 0;
    b->hold_ctr = 0;
    b->alpha    = 0;
    b->hold_dur = hold_dur;
    b->enable   = 1;
    b->composed = 0;
    if (text != NULL) {
        strncpy(b->text, text, sizeof b->text - 1);
        b->text[sizeof b->text - 1] = '\0';
    }
}

void banner_step(area_banner *b)
{
    if (b->enable == 0 || b->mode != BANNER_MODE_TEXT)
        return;

    /* 0x499ab0 case-1 sub-machine on *(short*)(obj+1) (the phase).
     * case 0 has NO break — it falls through into the phase-1 fade-in. */
    switch (b->phase) {
    case 0:
        /* retail: compose-readiness check (0x49a050) + old-text free
         * (0x4118b0); the port composes lazily in the render, so this is
         * just the phase advance. */
        b->phase = 1;
        /* fallthrough */
    case 1:                                            /* FADE IN */
        if (b->alpha < BANNER_ALPHA_MAX) {
            b->alpha += BANNER_ALPHA_STEP;
            if (b->alpha > BANNER_ALPHA_MAX)
                b->alpha = BANNER_ALPHA_MAX;
        }
        if (b->alpha > 999)                            /* 999 < alpha */
            b->phase = 2;
        break;
    case 2:                                            /* HOLD */
        if (b->hold_ctr < (uint16_t)b->hold_dur)
            b->hold_ctr = (uint16_t)(b->hold_ctr + 1);
        else
            b->phase = 3;
        break;
    case 3:                                            /* FADE OUT */
        if (b->alpha < 1) {
            b->enable = 0;                             /* done */
        } else {
            b->alpha -= BANNER_ALPHA_STEP;
            if (b->alpha < 0)
                b->alpha = 0;
        }
        break;
    default:
        break;
    }
}

int banner_active(const area_banner *b)
{
    return b->enable != 0 && b->mode == BANNER_MODE_TEXT;
}

banner_layout banner_text_layout(const char *text)
{
    banner_layout L;
    int len = (text != NULL) ? (int)strlen(text) : 0;
    int advance = 0xe, y_off = 0;

    /* 0x494a60 0x494b76-0x494be1: the length->advance/y_off ladder.
     * (The retail font index also changes — 6 for len<23, 8 for >=23 — but the
     * town name uses font 6 = Courier New h20 w10, the port's banner font.) */
    if (len < 0x1d) {
        if (len < 0x17) {
            if (len < 0xf) {
                if (len > 10)
                    advance = 0xc;                     /* 11-14 */
                /* else (<=10): advance stays 0xe */
            } else {
                advance = 10; y_off = 4;               /* 15-22 (the town: 17) */
            }
        } else {
            advance = 9; y_off = 6;                     /* 23-28 */
        }
    } else {
        advance = 8; y_off = 6;                         /* >=29 */
    }

    L.len     = len;
    L.advance = advance;
    L.y_off   = y_off;
    L.x_base  = 0xa0 - (len * advance) / 2;             /* iVar7 = -((len*adv)/2)+0xa0 */
    return L;
}

int banner_alpha_ramp_index(int32_t alpha)
{
    int idx = (int)((alpha * 0x14) / 1000);            /* 0x494a60 0x494acc */
    if (idx < 0)
        idx = 0;
    if (idx > 0x13)
        return -1;                                     /* fully opaque -> keyed */
    return idx;
}
