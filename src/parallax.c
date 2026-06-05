/*
 * parallax.c — the in-game parallax far-plane producer.  See parallax.h for the
 * engine correspondence (FUN_00490cd0 / 0x499100+FUN_00499560) and the
 * descriptor layout.
 */
#include "parallax.h"

#include <stdint.h>

/* The signed magic constant the engine uses in the vertical-parallax term
 * (587e00 / 490cd0:34): iVar3 = (int)((longlong)cam_y * -0x51eb851f >> 32).
 * Transcribed verbatim from the decompile — do NOT "simplify" to cam_y/100; the
 * negative magic yields -(cam_y/100)-ish, which is exactly the engine's value. */
#define PX_VMAGIC  ((int64_t)-0x51eb851f)

/* ── 0x587e00 prologue parallax selection (587e00.c:64-196) ──────────────── */

void parallax_select(int32_t param_2, int32_t param_3, parallax_desc *out)
{
    /* clear all parallax fields (587e00.c:104-109 default) */
    out->a_bank = 0;
    out->c.bank = out->c.base_y = out->c.wrap = 0; out->c.para_y = 0;
    out->b.bank = out->b.base_y = out->b.wrap = 0; out->b.para_y = 0;

    /* param_3 normalisation (587e00.c:64-80) */
    int32_t p3;
    switch (param_3) {
    case 10: case 0x28: p3 = 1; break;
    case 0x32:          p3 = 2; break;
    case 0x3c:          p3 = 4; break;
    case 0x3d:          p3 = 5; break;
    default:            p3 = 0; break;
    }

    switch (param_2) {
    case 1: case 5: case 6: case 8: case 10: case 0xd: case 0xe: case 0x12:
        /* A by p3; C bank 0x57, no B (587e00.c:119-133) */
        if (p3 < 2)       out->a_bank = 0x4e;
        else if (p3 < 5)  out->a_bank = 0x4f;
        else              out->a_bank = (p3 == 5) ? 0x50 : 0x4e;
        out->c.bank = 0x57; out->c.wrap = 8; out->c.base_y = 0xa0;
        break;
    case 4: case 9:
        /* the TOWN arm (587e00.c:143-159) */
        out->a_bank = (p3 < 2 || 4 < p3) ? 0x55 : 0x56;
        out->c.bank = 0x58; out->c.wrap = 8; out->c.base_y = 0xf8; out->c.para_y = 0xfa;
        out->b.bank = 0x59; out->b.wrap = 8; out->b.base_y = 0xe0;
        break;
    case 0xb: case 0xc: case 0xf: case 0x10:
        /* 587e00.c:160-178 */
        out->a_bank = (p3 < 2 || 4 < p3) ? 0x55 : 0x56;
        out->c.bank = 0x5a; out->c.wrap = 8; out->c.base_y = 0xc8; out->c.para_y = 0xfa;
        out->b.bank = 0x5b; out->b.wrap = 8; out->b.base_y = 0xd0;
        break;
    case 0x11:
        /* A only (587e00.c:179-186) */
        out->a_bank = (p3 < 2 || 4 < p3) ? 0x51 : 0x52;
        break;
    case 0x13:
        /* A 0x53 + C 0x54 (587e00.c:187-193; the in_ECX[0xe]=1 HUD write is
         * out of scope here) */
        out->a_bank = 0x53;
        out->c.bank = 0x54; out->c.wrap = 8; out->c.base_y = 0xa0;
        break;
    default:
        /* case 3 / default (587e00.c:134-142): special-render room, *in_ECX=0 —
         * no parallax (the all-zero descriptor set above). */
        break;
    }
}

/* ── grid front-header <-> descriptor ────────────────────────────────────── */

static void wr16(uint8_t *p, uint32_t off, uint16_t v)
{
    p[off]   = (uint8_t)(v & 0xff);
    p[off+1] = (uint8_t)((v >> 8) & 0xff);
}
static void wr32(uint8_t *p, uint32_t off, int32_t v)
{
    uint32_t u = (uint32_t)v;
    p[off]   = (uint8_t)(u & 0xff);
    p[off+1] = (uint8_t)((u >> 8) & 0xff);
    p[off+2] = (uint8_t)((u >> 16) & 0xff);
    p[off+3] = (uint8_t)((u >> 24) & 0xff);
}
static uint16_t rd16(const uint8_t *p, uint32_t off)
{
    return (uint16_t)(p[off] | ((uint16_t)p[off+1] << 8));
}
static int32_t rd32(const uint8_t *p, uint32_t off)
{
    return (int32_t)((uint32_t)p[off] | ((uint32_t)p[off+1] << 8) |
                     ((uint32_t)p[off+2] << 16) | ((uint32_t)p[off+3] << 24));
}

void parallax_to_grid(uint8_t *grid, const parallax_desc *d)
{
    wr16(grid, PX_A_BANK,  d->a_bank);
    wr16(grid, PX_C_BANK,  d->c.bank);
    wr16(grid, PX_C_BASEY, d->c.base_y);
    wr16(grid, PX_C_WRAP,  d->c.wrap);
    wr32(grid, PX_C_PARAY, d->c.para_y);
    wr16(grid, PX_B_BANK,  d->b.bank);
    wr16(grid, PX_B_BASEY, d->b.base_y);
    wr16(grid, PX_B_WRAP,  d->b.wrap);
    wr32(grid, PX_B_PARAY, d->b.para_y);
}

void parallax_from_grid(const uint8_t *grid, parallax_desc *out)
{
    out->a_bank   = rd16(grid, PX_A_BANK);
    out->c.bank   = rd16(grid, PX_C_BANK);
    out->c.base_y = rd16(grid, PX_C_BASEY);
    out->c.wrap   = rd16(grid, PX_C_WRAP);
    out->c.para_y = rd32(grid, PX_C_PARAY);
    out->b.bank   = rd16(grid, PX_B_BANK);
    out->b.base_y = rd16(grid, PX_B_BASEY);
    out->b.wrap   = rd16(grid, PX_B_WRAP);
    out->b.para_y = rd32(grid, PX_B_PARAY);
}

/* ── the renderer ────────────────────────────────────────────────────────── */

/* FUN_00499560 — one scrolling strip (9 tiles) at horizontal parallax
 * `factor_x` (out of 1000).  Verbatim from the decompile: the horizontal
 * column/offset from cam scroll-x, the clamped vertical-parallax offset, then 9
 * tiles laid left->right with wrapping frames. */
static int parallax_strip(const parallax_layer *L, const mr_camera *cam,
                          int32_t factor_x, parallax_blit_fn blit, void *ctx)
{
    if (L->bank == 0) return 0;       /* 499560:15 — *param_3 == 0 */

    int32_t sx   = (((cam->off60 + cam->off34) / 100) * factor_x) / 1000;
    int32_t col0 = sx / 0x50;
    int32_t xoff = -(sx % 0x50);

    int32_t cam_y = cam->off5c + cam->off74 * 100 + cam->off4c;
    int32_t iv    = (int32_t)(((int64_t)cam_y * PX_VMAGIC) >> 32);
    int32_t yoff  = (((iv >> 5) - (iv >> 31)) * L->para_y) / 1000;
    /* clamp to [-0x1c, 0] (499560:26-33) */
    if (-0x1d < yoff) { if (0 < yoff) yoff = 0; }
    else              { yoff = -0x1c; }

    for (int i = 0; i < 9; i++) {
        int32_t frame = (L->wrap != 0)
                            ? (int32_t)(col0 + i) % (int32_t)(uint32_t)L->wrap
                            : 0;   /* retail divides unconditionally; wrap is
                                    * always set with the bank, so this guard is
                                    * inert (avoids a host div-by-0 only). */
        int32_t y = (int32_t)L->base_y + yoff;
        if (blit) blit(ctx, L->bank, frame, xoff + i * 0x50, y);
    }
    return 9;
}

int parallax_render(const parallax_desc *d, const mr_camera *cam,
                    parallax_blit_fn blit, void *ctx)
{
    int n = 0;

    /* layer A — 8 fixed tiles, frame 0..7 at x=i*0x50, y=0 (490cd0:17-29) */
    if (d->a_bank != 0) {
        for (int i = 0; i < 8; i++) {
            if (blit) blit(ctx, d->a_bank, i, i * 0x50, 0);
            n++;
        }
    }

    /* layers B then C (the engine order: A, B, C — C drawn last/nearest)      */
    n += parallax_strip(&d->b, cam, 0xfa, blit, ctx);   /* 499100:47, factor 0xfa */
    n += parallax_strip(&d->c, cam, 500,  blit, ctx);   /* 499100:48, factor 500  */
    return n;
}
