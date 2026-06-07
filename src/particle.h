/*
 * particle.{c,h} — the engine's particle subsystem (DAT_008a9b50 + 0x13e0, the
 * DEVICE band): a 1024-slot pool of short-lived sprite particles allocated by
 * 0x557370 (round-robin / evict-oldest), configured per-code by 0x557550,
 * stepped per-sim-tick by 0x46e510 (gravity / integrate / clip / fade /
 * expire), and rendered by 0x493480's default arm.  See engine-quirk #87 +
 * findings/in-game-intro.md "The FOUNTAIN SPRAY".
 *
 * Two particle systems are ported here, both bank 0x1aa / res 0x408, both
 * ALPHA-blended (the +0x13e0 band always blits mode-1, 0x4917b0 -> the 0x5bd550
 * orchestrate):
 *   - the FOUNTAIN WATER (code 0x18708): emitter the fountain prop 0x112e5
 *     (0x54f980 case 0x112e5), one droplet/primary-tick launched UP + OUT (a
 *     3-way velocity cycle); 0x46e510 case 0x18708 applies gravity (+8000/tick)
 *     and a fade.  frame_base 6, layer 11, clip 0x6449c0 (2-frame loop).
 *   - the SKY-AMBIENT particles (code 0x18704): emitter the prop 0x112e2
 *     (0x54f980 case 0x112e2), one particle every 6th tick drifting UP + LEFT
 *     (0x453960 velocity scatter at config); 0x46e510 case 0x18704 accelerates
 *     vel_y toward -5000 and fades after lifetime 40.  frame_base 8, layer 6,
 *     clip 0x644b58 (6-frame ONESHOT — the particle expires when it finishes).
 *
 * The fade differs by system: the WATER reads ramp_a (&DAT_008a92e0[-sub_phase]
 * = g_ramp_a[10 - sub_phase]); the SKY reads ramp_b (&DAT_008a9308[idx],
 * idx = 18 - (life-40)/4 clamped [2,18]).  particle_pool_render emits mode-1
 * nodes whose param8 carries (ramp-selector << 8) | index; game_present_blit
 * PRESENT_ALPHA decodes that to g_ramp_a / g_ramp_b and orchestrates the blend
 * (mirrors title_render's alpha_blit; map_present mode-1).
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
 * The mode-1 (alpha) node param8 contract between particle_pool_render and the
 * present (game_present_blit PRESENT_ALPHA).  Retail emits the resolved blend
 * descriptor pointer; the port carries (ramp-selector << 8) | ramp-index, where
 * bit 8 picks ramp_b (0x8a9308) over ramp_a (0x8a92b8) and the low byte is the
 * 0..19 index into that 20-entry table.  The fountain water (ramp_a) leaves bit
 * 8 clear; the sky-ambient particles (ramp_b) set it.
 */
#define PARTICLE_PARAM8_RAMP_B 0x100u
#define PARTICLE_PARAM8_IDX_MASK 0xffu

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
 * 0x557370 alloc + 0x557550 case 0x18704 for a SKY-AMBIENT particle at world
 * (x, y): round-robin to a free slot, install bank 0x1aa frame_base 8 + the sky
 * clip (0x644b58, 6-frame ONESHOT), layer 6, and the config velocity scatter
 * 0x453960(-10000,5000,-1000,1000) -> vel_x in [-10000,-5000), vel_y in
 * [-1000,0) (drifts UP + LEFT).  Draws 2 LCG (the scatter).  Returns the slot,
 * or -1 if the pool is full.
 */
int particle_spawn_sky(particle_pool *pool, int32_t world_x, int32_t world_y);

/*
 * The sky emitter's one sim-tick — 0x54f980 case 0x112e2 (:150-172).  Advances
 * `*counter`; on every 6th tick it draws 2 LCG (a y then x spawn jitter:
 * y=(rand*800>>15)-800, x=(rand*1600>>15)-800), spawns one 0x18704 particle at
 * the emitter center + jitter (which draws 2 more LCG for the velocity scatter),
 * and resets the counter.  4 LCG draws on a spawn tick, 0 otherwise.  Unlike the
 * fountain this case is NOT gated on primary/paused — it free-runs.
 */
void particle_sky_emit(particle_pool *pool, int32_t emit_cx, int32_t emit_cy,
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
