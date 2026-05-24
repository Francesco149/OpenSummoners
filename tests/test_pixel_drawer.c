/*
 * tests/test_pixel_drawer.c — unit tests for src/pixel_drawer.c.
 *
 * Covers the five leaf primitives ported in the first Pixel-Drawer pass:
 *   pd_channel_mask_to_shift    (FUN_005bd380)
 *   pd_channel_init             (FUN_005bd020)
 *   pd_channel_free_lut         (FUN_005bcff0)
 *   pd_blend_init               (FUN_005bd4d0)
 *   pd_blend_set_color          (FUN_005bd3b0)
 *
 * The LUT builder (FUN_005bd040) and the commit (FUN_005bd3d0) are not
 * yet ported and hence not yet tested.
 */
#include "t.h"
#include "pixel_drawer.h"

#include <stdlib.h>
#include <string.h>


/* ─── pd_channel_mask_to_shift ──────────────────────────────────────── */

int test_pd_channel_mask_to_shift_rgb565_R(void)
{
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0xF800), 11);
    return 0;
}

int test_pd_channel_mask_to_shift_rgb565_G(void)
{
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x07E0), 6);
    return 0;
}

int test_pd_channel_mask_to_shift_rgb565_B(void)
{
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x001F), 0);
    return 0;
}

int test_pd_channel_mask_to_shift_rgb555(void)
{
    /* RGB555: R=0x7C00 (MSB at bit 14), G=0x03E0 (MSB at bit 9),
     * B=0x001F (MSB at bit 4 → shift 0). */
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x7C00), 10);
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x03E0), 5);
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x001F), 0);
    return 0;
}

int test_pd_channel_mask_to_shift_zero_mask(void)
{
    /* Edge: zero mask falls through the 16-iteration loop with i=16,
     * returning 11 - 16 = -5.  Matches the original disassembly. */
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x0000), -5);
    return 0;
}

int test_pd_channel_mask_to_shift_single_bit_at_0(void)
{
    /* Mask = 0x0001: MSB at bit 0 → i = 15 → 11 - 15 = -4. */
    T_ASSERT_EQ_I(pd_channel_mask_to_shift(0x0001), -4);
    return 0;
}


/* ─── pd_channel_init ──────────────────────────────────────────────── */

int test_pd_channel_init_zeroes_and_seeds_weight(void)
{
    /* Fill with junk first to make sure the init clears everything. */
    PdChannel c;
    memset(&c, 0xAA, sizeof(c));

    pd_channel_init(&c);

    T_ASSERT_EQ_I(c.shift,         0);
    T_ASSERT_EQ_U(c.mask,          0);
    T_ASSERT_EQ_P(c.lut,           NULL);
    T_ASSERT_EQ_U(c.weight,        1000);
    T_ASSERT_EQ_U(c._pad0e,        0);
    T_ASSERT_EQ_U(c.lut_allocated, 0);
    return 0;
}


/* ─── pd_channel_free_lut ──────────────────────────────────────────── */

int test_pd_channel_free_lut_when_allocated(void)
{
    PdChannel c;
    pd_channel_init(&c);
    c.lut           = malloc(64);  /* arbitrary size; the buffer will be free()'d */
    c.lut_allocated = 1;
    T_ASSERT(c.lut != NULL);

    pd_channel_free_lut(&c);

    T_ASSERT_EQ_P(c.lut,           NULL);
    T_ASSERT_EQ_U(c.lut_allocated, 0);
    /* ASan will catch a leak if the free didn't happen. */
    return 0;
}

int test_pd_channel_free_lut_when_not_allocated(void)
{
    /* Even if `lut` is a non-NULL pointer the engine handed us (e.g.
     * pointing into a shared LUT held by a sibling channel — see
     * the FUN_005bd040 same-weight short-circuit), we must NOT free
     * when `lut_allocated == 0`.  Verify by passing a stack address
     * — if we accidentally free()'d it, ASan would fire. */
    char fake_lut[64] = {0};
    PdChannel c;
    pd_channel_init(&c);
    c.lut           = (uint8_t *)fake_lut;
    c.lut_allocated = 0;

    pd_channel_free_lut(&c);

    /* free was NOT called → lut is unchanged. */
    T_ASSERT_EQ_P(c.lut,           (uint8_t *)fake_lut);
    T_ASSERT_EQ_U(c.lut_allocated, 0);
    return 0;
}


/* ─── pd_blend_init ────────────────────────────────────────────────── */

int test_pd_blend_init_zeroes_state_and_inits_channels(void)
{
    PdBlend b;
    memset(&b, 0xAA, sizeof(b));

    pd_blend_init(&b);

    T_ASSERT_EQ_U(b.state,       0);
    T_ASSERT_EQ_U(b.weight,      1000);
    T_ASSERT_EQ_U(b._pad42,      0);
    T_ASSERT_EQ_U(b.mode,        0);
    T_ASSERT_EQ_U(b.invert,      0);
    T_ASSERT_EQ_U(b.commit_flag, 0);

    /* Each channel sub-block is set as pd_channel_init would leave it. */
    for (int i = 0; i < 3; i++) {
        PdChannel *c = (i == 0) ? &b.r : (i == 1) ? &b.g : &b.b;
        T_ASSERT_EQ_I(c->shift,         0);
        T_ASSERT_EQ_U(c->mask,          0);
        T_ASSERT_EQ_P(c->lut,           NULL);
        T_ASSERT_EQ_U(c->weight,        1000);
        T_ASSERT_EQ_U(c->lut_allocated, 0);
    }
    return 0;
}


/* ─── pd_blend_set_color ───────────────────────────────────────────── */

int test_pd_blend_set_color_writes_per_channel_weights(void)
{
    PdBlend b;
    pd_blend_init(&b);

    pd_blend_set_color(&b, 600, 700, 800);

    T_ASSERT_EQ_U(b.r.weight, 600);
    T_ASSERT_EQ_U(b.g.weight, 700);
    T_ASSERT_EQ_U(b.b.weight, 800);
    return 0;
}

int test_pd_blend_set_color_does_not_touch_other_fields(void)
{
    PdBlend b;
    pd_blend_init(&b);
    /* Manually set non-default values everywhere outside the three
     * per-channel weight fields, then verify SetColor leaves them alone. */
    b.state          = 0x11223344;
    b.weight         = 555;
    b.mode           = 3;
    b.invert         = 1;
    b.commit_flag    = 1;
    b.r.shift        = 11;
    b.r.mask         = 0xF800;
    b.r.lut_allocated= 0;
    b.g.shift        = 6;
    b.g.mask         = 0x07E0;
    b.b.shift        = 0;
    b.b.mask         = 0x001F;

    pd_blend_set_color(&b, 100, 200, 300);

    T_ASSERT_EQ_U(b.r.weight, 100);
    T_ASSERT_EQ_U(b.g.weight, 200);
    T_ASSERT_EQ_U(b.b.weight, 300);

    /* Everything else preserved. */
    T_ASSERT_EQ_U(b.state,        0x11223344);
    T_ASSERT_EQ_U(b.weight,       555);
    T_ASSERT_EQ_U(b.mode,         3);
    T_ASSERT_EQ_U(b.invert,       1);
    T_ASSERT_EQ_U(b.commit_flag,  1);
    T_ASSERT_EQ_I(b.r.shift,      11);
    T_ASSERT_EQ_U(b.r.mask,       0xF800);
    T_ASSERT_EQ_I(b.g.shift,      6);
    T_ASSERT_EQ_U(b.g.mask,       0x07E0);
    T_ASSERT_EQ_I(b.b.shift,      0);
    T_ASSERT_EQ_U(b.b.mask,       0x001F);
    return 0;
}


/* ─── struct layout parity ─────────────────────────────────────────── */

int test_pd_blend_layout_matches_retail_offsets(void)
{
#if UINTPTR_MAX != 0xFFFFFFFFu
    /* Host x86_64 has 8-byte `lut` pointers in PdChannel; the retail
     * layout (and the i686-mingw drop-in) has 4-byte pointers, so the
     * raw-offset poke can only be checked on 32-bit builds.  The
     * compile-time _Static_assert block in pixel_drawer.h guards the
     * cross-build; here we just skip on 64-bit. */
    T_SKIP("64-bit host: retail layout only verified at compile time on i686");
#else
    PdBlend b;
    pd_blend_init(&b);

    uint8_t *raw = (uint8_t *)&b;
    *(uint16_t *)(raw + 0x10) = 0xCAFE;
    *(uint16_t *)(raw + 0x24) = 0xBABE;
    *(uint16_t *)(raw + 0x38) = 0xDEAD;
    T_ASSERT_EQ_U(b.r.weight, 0xCAFE);
    T_ASSERT_EQ_U(b.g.weight, 0xBABE);
    T_ASSERT_EQ_U(b.b.weight, 0xDEAD);

    *(uint16_t *)(raw + 0x40) = 0xABCD;
    T_ASSERT_EQ_U(b.weight,   0xABCD);

    *(uint32_t *)(raw + 0x44) = 7;
    *(uint32_t *)(raw + 0x48) = 8;
    *(uint32_t *)(raw + 0x4c) = 9;
    T_ASSERT_EQ_U(b.mode,        7);
    T_ASSERT_EQ_U(b.invert,      8);
    T_ASSERT_EQ_U(b.commit_flag, 9);
    return 0;
#endif
}
