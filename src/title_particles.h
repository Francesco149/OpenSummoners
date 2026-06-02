/*
 * src/title_particles.h — the phase-7 title sparkle-particle pool.
 *
 * The title intro's phase 7 runs TWO sparkle systems (engine-quirk #57):
 *
 *   1. the subtitle-reveal *sweep* (0x56bcf7 → TITLE_DRAW_SPARKLE) — wired and
 *      bit-exact at ckpt 30; it uncovers the "Secret of the Elemental Stone"
 *      subtitle column-by-column.
 *   2. this *particle* system — a scatter of additive white twinkles spawned
 *      along the leading edge of that reveal, drawn over the lower art.
 *
 * The particle system is three retail functions over one global pool
 * (DAT_008a92b4, allocated cap-500 at title-scene setup @0x56b35x):
 *
 *   spawn   FUN_0056c070  — append one particle at the sweep edge (random x/y
 *                           jitter, random per-particle anim phase).  Called
 *                           once per phase-7 update tick while the reveal level
 *                           uVar15 < 0x352, with the title-screen constants.
 *   draw    FUN_0056c180  — the per-flip compositor that blits every live
 *                           particle (== title_compositor_draw in title_render.c;
 *                           the pool *is* the compositor's display list).
 *   clear   FUN_0056c2b0  — scene-exit cleanup; zeroes the pool (count→0).
 *
 * The pool is *accumulate-only* within a title run: count only grows (spawn),
 * particles never expire, and the draw reads frame 0 of every one (their anim
 * fields are constant after spawn) — so the twinkles build up along the
 * subtitle and persist, glittering, until the player picks a menu item.
 *
 * Determinism: every random field comes from the engine LCG (rng.h).  Pin the
 * seed on both sides (port boots a fixed seed; the parity harness pins retail's
 * DAT_008a4f94 to match) and the spawned stream is bit-reproducible.
 *
 * Pure C / Win32-free — host-testable.
 */
#ifndef OPENSUMMONERS_TITLE_PARTICLES_H
#define OPENSUMMONERS_TITLE_PARTICLES_H

#include <stdint.h>
#include "title_render.h"   /* title_sprite_entry / title_sprite_group */

/* Retail pool capacity: operator_new(8) header + operator_new(14000) array,
 * 14000 / 0x1c = 500 entries (0x56b358..0x56b36e). */
#define TITLE_PARTICLE_CAP 500

/* The pool: the compositor display-list header (entries/cap/count) plus its
 * backing storage, owned together so the caller holds one object.  `group`
 * is what feeds title_compositor_draw via the sink's compose_group. */
typedef struct title_particle_pool {
    title_sprite_group group;                       /* {entries, cap, count} */
    title_sprite_entry store[TITLE_PARTICLE_CAP];   /* operator_new(14000)   */
} title_particle_pool;

/* Initialise / reset the pool: point group.entries at store, cap = 500,
 * count = 0.  Mirrors the retail allocation (count starts 0) and the
 * scene-exit clear (FUN_0056c2b0: count→0). */
void title_particle_pool_init(title_particle_pool *pool);

/* FUN_0056c070 — append one particle (faithful, full arg list).  No-op when
 * the pool is full (count == cap), exactly like retail's `if (count < cap)`
 * guard.  Consumes four rng_rand() values, in this order: x-jitter, y-jitter,
 * the +0x08 spare, the +0x0e anim phase.
 *
 *   x(+0x00) = ((rand*p7)/32768 + p5) * 100      // sweep-edge x, centi-px
 *   y(+0x04) = ((rand*p8)/32768 + p6) * 100      // subtitle row y, centi-px
 *   (+0x08)  =  (rand*200)/32768                 // spare (unread by the draw)
 *   bank(+0x10)=p1 ; frame_base(+0x12)=p2 ; frame_count(+0x14)=p3
 *   alpha_level(+0x18)=p4
 *   anim_div(+0x0e) = (rand*p10)/32768 + p9 ; anim_num(+0x0c) = anim_div
 */
void title_particle_spawn_raw(title_particle_pool *pool,
                              uint16_t p1, uint16_t p2, uint16_t p3,
                              int32_t p4, int32_t p5, int32_t p6,
                              int32_t p7, int32_t p8, int16_t p9, uint32_t p10);

/* The per-frame particle update (FUN_0056aea0 @ 0x56ba69 + cull 0x56c030),
 * run once per scene update tick.  Each live particle:
 *   y_num -= vel ; vel += 2          // rises, accelerating upward
 *   anim_num != 0 ? anim_num--       // ages (the draw frame animates 0→7)
 *                 : cull (swap-remove, count--)
 * Iterates back-to-front so the swap-remove is index-safe.  No-op on an empty
 * pool.  This is what makes the twinkles "evaporate upwards" rather than pile
 * up frozen at the spawn row. */
void title_particle_pool_update(title_particle_pool *pool);

/* The phase-7 title call: bakes FUN_0056c070's call-site constants
 * (0x56bcf7) and takes only the per-frame sparkle intensity (the spawn's
 * 5th arg, (uVar15*0xe0)/900 + 0xc0, surfaced by title_fade_step as
 * sparkle_intensity).  This is what the spawn_sparkle hook calls. */
void title_particle_spawn_title(title_particle_pool *pool, int32_t intensity);

#endif /* OPENSUMMONERS_TITLE_PARTICLES_H */
