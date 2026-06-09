/*
 * test_banner.c — host tests for the area-title banner (src/banner.c).
 *
 * Ground truth: engine-quirk #96 + runs/banner-state (the live 0x494a60
 * field-spec): mode 1, alpha += 0x14/sim-tick to 1000, hold 400, then fade out.
 * Layout from 0x494a60 case-1 + the live textout (Courier New, the town name
 * "Town of Tonkiness" len 17 -> advance 10, y_off 4, x_base 75).
 */
#include "banner.h"
#include "t.h"

#include <string.h>

/* ---- arm sets the area card live in mode 1, alpha 0, phase 0 -------------- */

int test_banner_arm(void)
{
    area_banner b;
    banner_arm(&b, "Town of Tonkiness", BANNER_HOLD_DUR);
    T_ASSERT_EQ_I(b.mode, BANNER_MODE_TEXT);
    T_ASSERT_EQ_I(b.enable, 1);
    T_ASSERT_EQ_I(b.phase, 0);
    T_ASSERT_EQ_I(b.alpha, 0);
    T_ASSERT_EQ_I(b.hold_ctr, 0);
    T_ASSERT_EQ_I(b.hold_dur, BANNER_HOLD_DUR);
    T_ASSERT_EQ_I(b.composed, 0);
    T_ASSERT(strcmp(b.text, "Town of Tonkiness") == 0);
    T_ASSERT(banner_active(&b));
    return 0;
}

/* ---- phase 0 falls through to the phase-1 fade-in on the first tick -------- */

int test_banner_fade_in(void)
{
    area_banner b;
    banner_arm(&b, "Town of Tonkiness", BANNER_HOLD_DUR);

    /* first step: phase 0 -> 1, alpha 0 -> 0x14 (no break between case 0/1). */
    banner_step(&b);
    T_ASSERT_EQ_I(b.phase, 1);
    T_ASSERT_EQ_I(b.alpha, BANNER_ALPHA_STEP);          /* 20 */

    /* alpha climbs +20/tick; reaches 1000 at tick 50, then -> phase 2. */
    for (int i = 1; i < 50; i++) {
        banner_step(&b);
    }
    T_ASSERT_EQ_I(b.alpha, BANNER_ALPHA_MAX);           /* 1000 */
    T_ASSERT_EQ_I(b.phase, 2);                          /* fade-in done */
    return 0;
}

/* ---- phase 2 holds for hold_dur ticks, then fades out to enable=0 --------- */

int test_banner_hold_then_fade(void)
{
    area_banner b;
    banner_arm(&b, "Town of Tonkiness", BANNER_HOLD_DUR);

    /* run the 50 fade-in ticks -> phase 2, alpha 1000. */
    for (int i = 0; i < 50; i++) banner_step(&b);
    T_ASSERT_EQ_I(b.phase, 2);

    /* HOLD: hold_ctr climbs to hold_dur over `hold_dur` ticks; the (hold_dur+1)th
     * tick (hold_ctr == hold_dur, not < ) advances to phase 3. */
    for (int i = 0; i < BANNER_HOLD_DUR; i++) {
        banner_step(&b);
        T_ASSERT_EQ_I(b.phase, 2);                      /* still holding */
    }
    T_ASSERT_EQ_I(b.hold_ctr, BANNER_HOLD_DUR);
    banner_step(&b);                                    /* the transition tick */
    T_ASSERT_EQ_I(b.phase, 3);

    /* FADE OUT: alpha -20/tick from 1000; at alpha<1 enable clears. */
    for (int i = 0; i < 50; i++) {
        T_ASSERT_EQ_I(b.enable, 1);
        banner_step(&b);
    }
    T_ASSERT_EQ_I(b.alpha, 0);
    T_ASSERT_EQ_I(b.enable, 1);                         /* one more tick to clear */
    banner_step(&b);
    T_ASSERT_EQ_I(b.enable, 0);
    T_ASSERT(!banner_active(&b));
    return 0;
}

/* ---- the length->advance/x_base layout (0x494a60 case 1) -------------- */

int test_banner_layout(void)
{
    /* the town name: len 17 (15-22 band) -> advance 10, y_off 4, x_base 75
     * (matches the live textout x 73-76 = x_base-2..+1). */
    banner_layout L = banner_text_layout("Town of Tonkiness");
    T_ASSERT_EQ_I(L.len, 17);
    T_ASSERT_EQ_I(L.advance, 10);
    T_ASSERT_EQ_I(L.y_off, 4);
    T_ASSERT_EQ_I(L.x_base, 0xa0 - (17 * 10) / 2);      /* 75 */

    /* the length ladder boundaries. */
    T_ASSERT_EQ_I(banner_text_layout("0123456789").advance, 0xe);          /* len 10 */
    T_ASSERT_EQ_I(banner_text_layout("0123456789a").advance, 0xc);         /* len 11 */
    T_ASSERT_EQ_I(banner_text_layout("0123456789abcd").advance, 0xc);      /* len 14 */
    T_ASSERT_EQ_I(banner_text_layout("0123456789abcde").advance, 10);      /* len 15 */
    {
        banner_layout l22 = banner_text_layout("0123456789012345678901");  /* len 22 */
        T_ASSERT_EQ_I(l22.advance, 10);
    }
    {
        banner_layout l23 = banner_text_layout("01234567890123456789012"); /* len 23 */
        T_ASSERT_EQ_I(l23.advance, 9);
        T_ASSERT_EQ_I(l23.y_off, 6);
    }
    {
        banner_layout l29 = banner_text_layout(
            "01234567890123456789012345678");                              /* len 29 */
        T_ASSERT_EQ_I(l29.advance, 8);
        T_ASSERT_EQ_I(l29.y_off, 6);
    }
    return 0;
}

/* ---- alpha -> ramp_b index / keyed threshold (0x494a60 0x494acc) ------ */

int test_banner_alpha_ramp(void)
{
    T_ASSERT_EQ_I(banner_alpha_ramp_index(1000), -1);   /* idx 20 > 0x13 -> keyed */
    T_ASSERT_EQ_I(banner_alpha_ramp_index(950), 19);
    T_ASSERT_EQ_I(banner_alpha_ramp_index(500), 10);
    T_ASSERT_EQ_I(banner_alpha_ramp_index(BANNER_ALPHA_STEP), 0); /* 20*20/1000 = 0 */
    T_ASSERT_EQ_I(banner_alpha_ramp_index(0), 0);
    return 0;
}
