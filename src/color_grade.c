/*
 * color_grade.c — see color_grade.h.  Port of FUN_00562ea0's in-game tone-curve
 * LUT builder (0x5639fd-0x563a70) + FUN_00417c40's per-channel palette remap.
 */
#include "color_grade.h"

#include <math.h>

/* The builder's .rdata double constants (read out of the binary):
 *   0x5cc288 = PI, 0x850dd0 = 1/255, 0x850dc8 = 1.0, 0x850dc0 = 127.5,
 *   0x850d98 = 0.001, 0x850db8 = 255.0 (clamp ceiling). */
#define CG_PI       3.14159265358979
#define CG_INV255   0.00392156862745098   /* 1.0/255.0 */

void color_grade_build_lut(uint8_t lut[256], int gate1, int gate2)
{
    for (int i = 0; i < 256; i++) {
        /* q = (i * gate2) / 1000  — integer divide (matches the imul/magic
         * div-by-1000 at 0x5639ff-0x563a21). */
        int q = (i * gate2) / 1000;

        /* accum = ( (1000-gate1)*q + (1 - cos(q*PI/255))*127.5*gate1 ) * 0.001
         * (the faddp/fmul chain at 0x563a2b-0x563a4e). */
        double accum = ((double)((1000 - gate1) * q)
                        + (1.0 - cos((double)q * (CG_PI * CG_INV255))) * 127.5 * (double)gate1)
                       * 0.001;

        /* fcomp vs 255.0 then ftol — clamp to the ceiling, truncate toward 0. */
        if (accum > 255.0)
            accum = 255.0;
        lut[i] = (uint8_t)(int)accum;
    }
}

int color_grade_is_active(int gate1, int gate2)
{
    /* The engine gate: DAT_008a9510 != 0 || DAT_008a9514 != 1000. */
    return (gate1 != 0) || (gate2 != 1000);
}

void color_grade_apply_palette(uint8_t *palette, int n_entries,
                               const uint8_t lut[256])
{
    /* FUN_00417c40 (417c40.c:259-273): remap the first three bytes (R,G,B) of
     * each 4-byte RGBQUAD entry; leave the 4th (reserved) untouched. */
    for (int e = 0; e < n_entries; e++) {
        uint8_t *p = &palette[e * 4];
        p[0] = lut[p[0]];
        p[1] = lut[p[1]];
        p[2] = lut[p[2]];
    }
}
