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
