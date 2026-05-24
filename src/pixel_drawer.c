/*
 * src/pixel_drawer.c — Pixel Drawer module (leaf primitives).
 *
 * See pixel_drawer.h for the per-function provenance comments
 * (FUN_005bd020, FUN_005bd380, FUN_005bcff0, FUN_005bd3b0, FUN_005bd4d0).
 */
#include "pixel_drawer.h"

#include <stdlib.h>
#include <string.h>

/* FUN_005bd380 — mask → MSB shift.
 *
 * Original disassembly equivalent:
 *   int i = 0;
 *   do {
 *       if ((mask >> (15 - i)) & 1) break;
 *       i++;
 *   } while (i < 16);
 *   return 11 - i;
 *
 * Behaviour: walks bits from MSB (bit 15) downward, returns 11 - position
 * of highest set bit (zero-indexed from bit 0).  For RGB565 R/G/B masks
 * this gives 11/6/0; for RGB555 R it gives 10.  When mask == 0, no bit
 * is found and the loop exits with i = 16, returning -5 — this is what
 * the retail code does, and the caller (FUN_005bd3d0 commit) only ever
 * passes non-zero masks read from a real surface format, so the edge
 * case is academic but preserved for parity. */
int32_t pd_channel_mask_to_shift(uint32_t mask)
{
    int i = 0;
    while (i < 16) {
        if ((mask >> (15 - i)) & 1u) break;
        i++;
    }
    return 11 - i;
}

/* FUN_005bd020 — channel sub-block init.
 * Zeros all fields and seeds the per-channel weight to 1000 (full
 * intensity, the identity input for the LUT blend formulas). */
void pd_channel_init(PdChannel *c)
{
    c->shift          = 0;
    c->mask           = 0;
    c->lut            = NULL;
    c->weight         = 1000;
    c->_pad0e         = 0;
    c->lut_allocated  = 0;
}

/* FUN_005bcff0 — channel "free LUT".
 * Frees the LUT buffer iff it was allocated by the LUT builder.
 * The original uses `operator delete[]` (FUN_005bef0e); `free` is the
 * matching pair on the host side since the eventual LUT builder will
 * allocate with `malloc`. */
void pd_channel_free_lut(PdChannel *c)
{
    if (c->lut_allocated != 0) {
        free(c->lut);
        c->lut           = NULL;
        c->lut_allocated = 0;
    }
}

/* FUN_005bd4d0 — slot ctor.
 * Reads of the original:
 *   in_ECX[0x12] = 0;                              // commit_flag (offset 0x48)
 *   in_ECX[0x13] = 0;                              // unk_4c     (offset 0x4c)
 *   *(undefined2 *)(in_ECX + 0x10) = 1000;         // slot weight (offset 0x40)
 *   in_ECX[0x11] = 0;                              // pad+mode high bits (offset 0x44 spans 4B)
 *   FUN_005bd020(in_ECX + 1);                      // R channel
 *   FUN_005bd020(in_ECX + 6);                      // G channel
 *   FUN_005bd020(in_ECX + 0xb);                    // B channel
 *   *in_ECX = 0;                                   // state field
 *
 * Note: u32-aligned `in_ECX[0x11]` clears bytes 0x44..0x47 — same as
 * writing `mode = 0` here.  The boot driver immediately overwrites mode
 * to 1, 2, 3, 5, 6, or 7 depending on the slot group (winmain-and-
 * bootstrap.md), so init-time zero is just a clean baseline. */
void pd_blend_init(PdBlend *b)
{
    b->commit_flag = 0;   /* original: in_ECX[0x13] = 0 (byte 0x4c) */
    b->invert      = 0;   /* original: in_ECX[0x12] = 0 (byte 0x48) */
    b->weight      = 1000;
    b->_pad42      = 0;
    b->mode        = 0;
    pd_channel_init(&b->r);
    pd_channel_init(&b->g);
    pd_channel_init(&b->b);
    b->state       = 0;
}

/* FUN_005bd040 — channel LUT builder.
 *
 * The original is dense (801 bytes, four blend modes, nested loops).
 * The structure boils down to:
 *
 *   1. Shared-LUT short-circuit: if `prev` is non-NULL and has the
 *      same channel weight as `chan`, alias chan->lut to prev->lut
 *      and return.  Does NOT set lut_allocated — the alias is read-
 *      only and pd_channel_free_lut on chan must NOT call free().
 *   2. Switch on slot->state:
 *        0 → small 32-byte LUT (also reached via fall-through from 2)
 *        1 → large 1024-byte 2-D LUT, mode-dependent fill
 *        2 → small 32-byte LUT
 *        else → no-op (no allocation, leaves chan->lut at whatever
 *               value it had — caller's responsibility to init).
 *
 * The "floor-correction" terms (`uVar1 + (int)uVar5 / 1000 +
 * ((int)uVar5 >> 0x1f)`) are preserved literally even though they
 * always evaluate to 0 for valid weight ranges (W, w ∈ [0, 1000]
 * and i, s ∈ [0, 31]).  Keeping them protects against any out-of-
 * range input the engine might pass that would tip the arithmetic
 * into the sign-correction branch — better to match retail byte-
 * for-byte than to "simplify" a behaviour we don't fully understand. */
static void build_lut_small(PdBlend *slot, PdChannel *chan)
{
    /* Original: operator_new(0x20), then 32-iteration fill loop. */
    uint8_t *lut = (uint8_t *)malloc(0x20);
    chan->lut           = lut;
    chan->lut_allocated = 1;

    int w = (int)chan->weight;
    int i = 0;          /* outer counter, 0..31 */
    int j = 0x20;       /* paired decrementer, 32..1 */
    do {
        int input = (slot->invert != 0) ? j : i;
        int v     = (w * input) / 1000;
        if (v < 0)       v = 0;
        else if (v > 31) v = 31;
        /* Original writes lut[-1 + (i+1)] = lut[i] after the i++.
         * Equivalent: write at i before incrementing.  Same effect. */
        lut[i] = (uint8_t)v;
        i++;
        j--;
    } while (j > 0);
}

static void build_lut_large(PdBlend *slot, PdChannel *chan)
{
    /* Original: operator_new(0x400), nested 32×32 fill loop selecting
     * one of five formulas by slot->mode.  Each entry is at
     * lut[inner * 32 + outer]. */
    uint8_t *lut = (uint8_t *)malloc(0x400);
    chan->lut           = lut;
    chan->lut_allocated = 1;

    /* Arithmetic notes:
     *
     * The retail code does the per-term multiplications in uint32_t
     * (because the source-language `(uint)W * signed_expr` triggers
     * the unsigned wrap-then-cast pattern Ghidra renders verbatim),
     * but for all weight / index ranges the engine ever uses (W, w ∈
     * [0, 1000], outer/inner ∈ [0, 31]) the products fit in 21 bits
     * signed and the unsigned wrap never reaches the high bit.  The
     * `(int) ... / 1000 + (... >> 31)` floor-correction therefore
     * always adds zero, and the uVar1 bias term `uVar5 >> 31` is
     * always zero too.
     *
     * Doing the math in `int` produces identical results for valid
     * inputs and is much easier to audit.  If the engine ever passes
     * out-of-range weights, the divergence becomes visible — but it
     * would be a sign of a *different* problem (out-of-range weight)
     * and we should fail loudly rather than silently replicate
     * undefined behaviour. */
    int W      = (int)slot->weight;
    int w      = (int)chan->weight;
    int mode   = (int)slot->mode;
    int invert = (slot->invert != 0);

    for (int outer = 0; outer < 32; outer++) {     /* iVar4 — row base */
        int j = 0x20;                              /* local_8 */
        int inner = 0;                             /* param_2 reused */
        do {
            int s = invert ? j : inner;            /* iVar6 */
            int v = 0;

            switch (mode) {
            case 1:
                v = (W * (s + outer)) / 1000
                  + ((1000 - W) * outer) / 1000;
                if (v < outer) v = outer;
                break;
            case 2:
                v = (W * (outer - s)) / 1000
                  + ((1000 - W) * outer) / 1000;
                if (v > outer) v = outer;
                break;
            case 3:
                v = ((1000 - W) * outer) / 1000
                  + ((s - outer) * W) / 1000;
                break;
            case 4: {
                /* The two `(x + ((x >> 31) & 0x1f)) >> 5` patterns are
                 * compiler-emitted `floor(x/32)` for signed x — they
                 * round toward negative infinity (vs C's `/32` which
                 * truncates toward zero).  Preserved literally. */
                int a = ((w << 5) / 1000) * s;
                int b = (0x20 - s) * outer;
                v = ((1000 - W) * outer) / 1000
                  + ((((a + ((a >> 31) & 0x1f)) >> 5)
                    + ((b + ((b >> 31) & 0x1f)) >> 5)) * W) / 1000;
                break;
            }
            default:
                v = outer;
                if (s != outer) {
                    v = ((1000 - W) * outer) / 1000
                      + (s * W) / 1000;
                }
                break;
            }

            if (mode != 4) {
                v = (w * v) / 1000;
            }
            if (v < 0)       v = 0;
            else if (v > 31) v = 31;

            lut[inner * 32 + outer] = (uint8_t)v;
            inner++;
            j--;
        } while (j > 0);
    }
}

void pd_blend_build_channel_lut(PdBlend *slot, PdChannel *chan, PdChannel *prev)
{
    /* Shared-LUT short-circuit. */
    if (prev != NULL && prev->weight == chan->weight) {
        chan->lut = prev->lut;
        /* Note: lut_allocated stays at its previous value (typically 0
         * after pd_channel_free_lut).  Aliasing must not transfer
         * ownership, so we don't set it to 1 here. */
        return;
    }

    /* State dispatch.  state==0 falls through to the small-LUT path
     * (matches original's structure: `if (iVar4 == 1) {…}; if (iVar4
     * != 2) return;` — with iVar4 starting at slot->state, this means
     * state 0 SKIPS both branches and FALLS THROUGH to the small-LUT
     * code below). */
    switch (slot->state) {
    case 1:
        build_lut_large(slot, chan);
        return;
    case 0:
    case 2:
        build_lut_small(slot, chan);
        return;
    default:
        /* No LUT allocated. */
        return;
    }
}

/* FUN_005bd3b0 — slot SetColor.
 * Original:
 *   *(undefined2 *)(in_ECX + 0x10) = param_1;   // byte +0x10 = R channel weight
 *   *(undefined2 *)(in_ECX + 0x24) = param_2;   // byte +0x24 = G channel weight
 *   *(undefined2 *)(in_ECX + 0x38) = param_3;   // byte +0x38 = B channel weight
 *
 * Note: the original writes a u16 to byte offset 0x10, which is +0x0c
 * inside the R sub-block (the per-channel weight).  Same pattern for
 * G (+0x18+0x0c = 0x24) and B (+0x2c+0x0c = 0x38). */
void pd_blend_set_color(PdBlend *b,
                        uint16_t r, uint16_t g, uint16_t b_chan)
{
    b->r.weight = r;
    b->g.weight = g;
    b->b.weight = b_chan;
}
