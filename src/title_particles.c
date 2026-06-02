/*
 * src/title_particles.c — phase-7 title sparkle-particle pool (see header).
 *
 *   spawn  = FUN_0056c070 @ 0x56c070
 *   (draw   = FUN_0056c180 == title_compositor_draw, title_render.c)
 *   (clear  = FUN_0056c2b0 @ 0x56c2b0 — modelled as pool_init's count reset)
 */

#include "title_particles.h"
#include "rng.h"

/* The shared scaling idiom retail spells out at every random field:
 *   (int)(r*mul + (r*mul >> 0x1f & 0x7fff)) >> 0xf
 * i.e. signed division of (r*mul) by 32768 (1<<15), truncating toward zero.
 * For the title call every (r, mul) is non-negative, so the >>31 bias term is
 * zero — but we keep the exact form so the port is bit-identical for any
 * argument, not just the ones the title screen happens to pass. */
static int32_t lcg_scale(int32_t r, int32_t mul)
{
    int32_t prod = r * mul;
    return (prod + ((prod >> 31) & 0x7fff)) >> 15;
}

void title_particle_pool_init(title_particle_pool *pool)
{
    pool->group.entries = pool->store;   /* operator_new(14000) base       */
    pool->group.cap     = TITLE_PARTICLE_CAP;   /* *(u16*)(hdr+4) = 500     */
    pool->group.count   = 0;             /* *(u16*)(hdr+6) = 0 (also 56c2b0)*/
}

void title_particle_spawn_raw(title_particle_pool *pool,
                              uint16_t p1, uint16_t p2, uint16_t p3,
                              int32_t p4, int32_t p5, int32_t p6,
                              int32_t p7, int32_t p8, int16_t p9, uint32_t p10)
{
    uint16_t cur = pool->group.count;        /* uVar1 = *(u16*)(ecx+6)      */
    if (cur >= pool->group.cap) {            /* if (uVar1 < *(u16*)(ecx+4)) */
        return;                              /* pool full → drop (retail no-op) */
    }
    pool->group.count = (uint16_t)(cur + 1);

    title_sprite_entry *e = &pool->group.entries[cur];   /* base + cur*0x1c */

    /* +0x00 x: sweep-edge x with [0,p7) jitter, biased by p5, centi-pixel. */
    e->x_num = (lcg_scale((int32_t)rng_rand(), p7) + p5) * 100;
    /* +0x04 y: subtitle-row y with [0,p8) jitter, biased by p6, centi-pixel. */
    e->y_num = (lcg_scale((int32_t)rng_rand(), p8) + p6) * 100;

    /* The constant fields (p2/p3/p1/p4 in retail's write order). */
    e->frame_base  = p2;                     /* +0x12 */
    e->frame_count = p3;                     /* +0x14 */
    e->bank_id     = p1;                     /* +0x10 */
    e->alpha_level = p4;                     /* +0x18 */

    /* +0x08 vel: initial upward velocity [0,200) centi-px/tick (the update
     * subtracts it from y_num each tick and grows it by 2 — accelerating up). */
    e->vel = lcg_scale((int32_t)rng_rand(), 200);

    /* +0x0e anim_div: [0,p10) jitter biased by p9; +0x0c anim_num copies it
     * (so the draw's (anim_num*frame_count)/anim_div clamps to frame 0). */
    int16_t phase = (int16_t)(lcg_scale((int32_t)rng_rand(),
                                        (int32_t)(p10 & 0xffffu)) + p9);
    e->anim_div = (uint16_t)phase;
    e->anim_num = e->anim_div;
}

void title_particle_pool_update(title_particle_pool *pool)
{
    /* FUN_0056aea0 @ 0x56ba69 — the per-frame particle update (runs every
     * update tick, unconditionally).  Iterates the live particles back-to-front
     * so the swap-remove cull (FUN_0056c030 @ 0x56baae) is index-safe: a culled
     * slot is overwritten by the current last entry, which this pass has
     * already updated.  Count is read once; the loop index is independent of
     * it (matching retail's `edi = count-1` latched before the loop). */
    int n = (int)pool->group.count;
    for (int i = n - 1; i >= 0; i--) {
        title_sprite_entry *e = &pool->group.entries[i];

        e->y_num -= e->vel;            /* rise (y decreases)         [0x56ba8e] */
        e->vel   += 2;                 /* accelerate upward          [0x56ba90] */

        if (e->anim_num != 0) {
            e->anim_num--;             /* age — draw frame 0→7       [0x56baa2] */
        } else {
            /* lifetime expired → cull (FUN_0056c030): count--, copy the last
             * live entry into this slot. */
            uint16_t last = (uint16_t)(pool->group.count - 1);
            pool->group.count = last;
            pool->group.entries[i] = pool->group.entries[last];
        }
    }
}

void title_particle_spawn_title(title_particle_pool *pool, int32_t intensity)
{
    /* FUN_0056c070(0x15,0,8,800, intensity, 0x1a0,0x10,0x18,0x14,0x14)
     * — the phase-7 call site at 0x56bcf7. */
    title_particle_spawn_raw(pool,
                             /*p1 bank   */ 0x15,
                             /*p2 fbase  */ 0,
                             /*p3 fcount */ 8,
                             /*p4 alpha  */ 800,
                             /*p5 x-bias */ intensity,
                             /*p6 y-bias */ 0x1a0,
                             /*p7 x-jit  */ 0x10,
                             /*p8 y-jit  */ 0x18,
                             /*p9 ph-bias*/ 0x14,
                             /*p10 ph-jit*/ 0x14);
}
