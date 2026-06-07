/*
 * particle.{c,h} — the engine's particle subsystem (DAT_008a9b50 + 0x13e0, the
 * DEVICE band): a 1024-slot pool of short-lived sprite particles allocated by
 * 0x557370 (round-robin / evict-oldest), configured per-code by 0x557550,
 * stepped per-sim-tick by 0x46e510 (gravity / integrate / clip / fade /
 * expire), and rendered by 0x493480's default arm.  See engine-quirk #87 +
 * findings/in-game-intro.md "The FOUNTAIN SPRAY".
 *
 * This first chip ports the FOUNTAIN WATER particle (code 0x18708, bank 0x1aa /
 * res 0x408): the emitter is the fountain prop 0x112e5 (0x54f980 case
 * 0x112e5), which spawns one droplet each primary sim-tick and launches it UP +
 * OUT (a 3-way velocity cycle); 0x46e510 case 0x18708 then applies gravity
 * and a fade.  The droplets ALPHA-blend (the +0x13e0 band always blits mode-1,
 * 0x4917b0 -> the 0x5bd550 orchestrate): particle_pool_render emits mode-1 nodes
 * whose param8 carries the brightness ramp index g_ramp_a[10 - sub_phase] (the
 * faithful &DAT_008a92e0[-sub_phase] descriptor), and the present orchestrates
 * the blend (game_present_blit PRESENT_ALPHA + map_present mode-1).
 *
 * Particles draw RNG (the launch velocity + spawn jitter) via the shared LCG
 * (rng.h).  Under the ckpt-86 spawn re-pin this is deterministic per sim-tick,
 * but frame-exact alignment with retail is entangled with the other per-tick
 * consumers (0x47b990 wander etc.) — PORT-DEBT(fountain-rng-phase), the broader
 * Phase 2.  The physics/spawn here are faithful to the decompile.
 */
#ifndef OSS_PARTICLE_H
#define OSS_PARTICLE_H

#include <stdint.h>

#include "actor_render.h"   /* actor, actor_render_state                       */
#include "anim_clip.h"      /* anim_clip                                       */
#include "draw_pool.h"      /* draw_pool (render)                              */
#include "map_render.h"     /* mr_sprite_fn (the bank,frame -> cel resolver)   */

/* The +0x13e0 DEVICE band is 0x400 slots in retail. */
#define PARTICLE_POOL_SLOTS 1024

/*
 * The particle pool — parallel {actor, render-state} arrays like
 * actor_spawn_pool, but sized to the retail band and carrying the round-robin
 * alloc cursor (retail's *(mapctl+0xce)).  A slot is active when its
 * render-state's `active` byte is set.
 */
typedef struct particle_pool {
    uint16_t           cursor;     /* 0x557370 round-robin cursor (& 0x3ff) */
    actor              actors[PARTICLE_POOL_SLOTS];
    actor_render_state states[PARTICLE_POOL_SLOTS];
} particle_pool;

/* Zero the pool (all slots inactive, cursor 0). */
void particle_pool_reset(particle_pool *pool);

/*
 * 0x557370 alloc + 0x557550 config for a fountain WATER particle
 * (code 0x18708) at world (x, y): round-robin to a free slot, install bank
 * 0x1aa frame_base 6 + the water clip (0x6449c0, 2-frame loop), velocity 0
 * (the emitter sets it next).  Returns the slot index, or -1 if the pool is
 * full (retail evicts the oldest; with ~60 alive in a 1024 pool that never
 * fires — PORT-DEBT(particle-evict)).
 */
int particle_spawn_water(particle_pool *pool, int32_t world_x, int32_t world_y);

/*
 * The fountain emitter's one primary-tick step — 0x54f980 case 0x112e5
 * (:218-283).  Spawns one 0x18708 droplet at the emitter prop's center +
 * (jitter), draws the shared LCG for the jitter + the launch velocity, and sets
 * the new droplet's velocity from the 3-way cycle keyed on `*counter % 3`
 * (left-strong / right / left-weak, all upward).  `*counter` is the emitter's
 * +0x5c tick counter (mod 30 in retail); advanced by 1 here.
 *
 * (emit_cx, emit_cy) is the emitter's anchor-mode-1 CENTER in world units
 * (fountain prop world pos + body center); the per-particle jitter is added on
 * top.  Draws exactly 6 LCG values (2 jitter + 2 sound + 2 velocity), matching
 * the retail consumption count for one fountain tick.
 */
void particle_fountain_emit(particle_pool *pool, int32_t emit_cx, int32_t emit_cy,
                            int *counter);

/*
 * 0x46e510 — advance every active particle one sim-tick (the 0x18708 arm:
 * gravity vel_y += 8000 (cap 80000), integrate x/y, age the fade counter, expire
 * at sub_phase 8).  RNG-free.
 */
void particle_pool_step(particle_pool *pool);

/*
 * Render every active particle through 0x493480's default arm — describe the cel
 * then emit a MODE-1 (alpha) node via draw_pool_emit (0x4917b0), param8 = the
 * ramp index 10 - sub_phase.  The present (PRESENT_ALPHA) orchestrates the blend.
 * Returns the number of particles emitted.
 */
int particle_pool_render(const particle_pool *pool, draw_pool *pool_out,
                         mr_sprite_fn resolve, void *resolve_ctx);

#endif /* OSS_PARTICLE_H */
