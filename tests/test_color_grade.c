/*
 * test_color_grade.c — host tests for the in-game palette color-grade LUT
 * (src/color_grade.c; FUN_00562ea0's builder + FUN_00417c40's palette remap).
 *
 * The LUT values are GROUND TRUTH: a live retail probe of DAT_008a9410 read
 * the exact bytes at the sampled indices (gate1=700, gate2=850).
 */
#include "color_grade.h"
#include "t.h"

#include <string.h>

/* The four live-probed samples + the endpoints (DAT_008a9410, town gates). */
int test_color_grade_town_lut_samples(void)
{
    uint8_t lut[256];
    color_grade_build_lut(lut, COLOR_GRADE_TOWN_GATE1, COLOR_GRADE_TOWN_GATE2);
    T_ASSERT_EQ_U(lut[0],   0);     /* shadows crushed to black            */
    T_ASSERT_EQ_U(lut[64],  35);    /* 0x23 — live probe                   */
    T_ASSERT_EQ_U(lut[128], 100);   /* 0x64 — live probe                   */
    T_ASSERT_EQ_U(lut[192], 175);   /* 0xaf — live probe                   */
    T_ASSERT_EQ_U(lut[255], 233);   /* highlights preserved (not full 255) */
    /* monotonic non-decreasing (a valid tone curve). */
    for (int i = 1; i < 256; i++)
        T_ASSERT(lut[i] >= lut[i - 1]);
    return 0;
}

/* gate1=0 && gate2=1000 is the identity (the boot default, before the config
 * gates arm it) — the title/new-game/prologue case. */
int test_color_grade_identity(void)
{
    uint8_t lut[256];
    color_grade_build_lut(lut, 0, 1000);
    for (int i = 0; i < 256; i++)
        T_ASSERT_EQ_U(lut[i], (uint8_t)i);
    return 0;
}

/* The arming gate mirrors the engine: (gate1 != 0 || gate2 != 1000). */
int test_color_grade_active_gate(void)
{
    T_ASSERT_EQ_I(color_grade_is_active(0, 1000), 0);   /* identity → off */
    T_ASSERT_EQ_I(color_grade_is_active(700, 850), 1);  /* town → on      */
    T_ASSERT_EQ_I(color_grade_is_active(0, 850), 1);
    T_ASSERT_EQ_I(color_grade_is_active(700, 1000), 1);
    return 0;
}

/* The per-channel palette remap touches R,G,B of each RGBQUAD, not the 4th
 * byte (FUN_00417c40's 3-of-4 loop). */
int test_color_grade_apply_palette_rgb(void)
{
    uint8_t lut[256];
    color_grade_build_lut(lut, COLOR_GRADE_TOWN_GATE1, COLOR_GRADE_TOWN_GATE2);

    /* two entries: {64,128,192,0xAA} and {255,0,128,0xBB}. */
    uint8_t pal[8] = { 64, 128, 192, 0xAA,  255, 0, 128, 0xBB };
    color_grade_apply_palette(pal, 2, lut);

    T_ASSERT_EQ_U(pal[0], 35);    /* lut[64]  */
    T_ASSERT_EQ_U(pal[1], 100);   /* lut[128] */
    T_ASSERT_EQ_U(pal[2], 175);   /* lut[192] */
    T_ASSERT_EQ_U(pal[3], 0xAA);  /* reserved byte untouched */
    T_ASSERT_EQ_U(pal[4], 233);   /* lut[255] */
    T_ASSERT_EQ_U(pal[5], 0);     /* lut[0]   */
    T_ASSERT_EQ_U(pal[6], 100);   /* lut[128] */
    T_ASSERT_EQ_U(pal[7], 0xBB);  /* reserved byte untouched */
    return 0;
}
