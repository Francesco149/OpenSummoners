/*
 * sword_trail.{c,h} — the freeroam UP-attack sword-tip TRAIL: the res 0x40b
 * sparkle stream that follows Arche's overhead thrust (chip 2c-2).  A small
 * captured-emitter particle pool mirroring particle.c (the +0x13e0 DEVICE band):
 *
 *   - 2 sparkles/sim-tick are emitted on the captured sword-tip ARC during the
 *     up-attack's swing-ticks 9-17 (9 ticks, 18 sparkles), then NO more;
 *   - each sparkle ages frames 24->31 (res 0x40b, dur-2, 8-frame oneshot = 16t),
 *     SHRINKING 22x22 -> 6x6 about a fixed centre (the cel's own metric_0c origin
 *     keeps it centred), and expires when the clip finishes (lifetime ~16t);
 *   - the blend is a CONSTANT additive ramp_b (engine-quirk #87, the sky-particle
 *     blend) — sword2.osr shows every sparkle at one blend descriptor, NOT a
 *     per-age ramp index, so the visual fade is the shrinking frame alone.
 *
 * GROUND TRUTH: the USER's real-play recording sword2.osr (the first up-attack,
 * ticks 3880-3914), extracted by tools/trace_studio2/trail_extract.py (the arc) +
 * the cel offsets/constant blend recorded in findings/freeroam-sword-attack.md;
 * the port is checked by tools/trace_studio2/trail_verify.py.  Provenance:
 * spawned by the 0x283f up-action handler 0x45e830 case 2 via
 * 0x557370(this, 0x186f2, ...) (pool DAT_008a9b50+0x13e0); config 0x55d140
 * (bank 0x1ad, frame_base 0x18); step 0x46e510 case 0x186f2 (ramp_b fade);
 * render 0x493480 default arm.  res 0x40b is registered in the port at sprite
 * slot 407 = bank 0x1a4 (asset_register.c game_sprites, 32x32 type 2).  The
 * constant blend is LUT-pinned to the port's ramp_A[19] (= retail ramp_b[18];
 * quirk #117) — see SWORD_TRAIL_RAMP_IDX in the .c.
 *
 * The exact retail emitter GEOMETRY (0x4505c0, the per-tick tip position)
 * is NOT ported — the captured tip-arc table is the faithful stand-in for the
 * un-ported emitter, exactly like butterfly_flap_ctrl: PORT-DEBT(sword-attack
 * -trail) covers deriving the arc from the emitter rather than the capture.
 * findings/freeroam-sword-attack.md "## chip 2c ... The TRAIL".
 */
#ifndef OSS_SWORD_TRAIL_H
#define OSS_SWORD_TRAIL_H

#include <stdint.h>

#include "actor_render.h"   /* actor, actor_render_state                       */
#include "anim_clip.h"      /* anim_clip                                       */
#include "draw_pool.h"      /* draw_pool (render)                              */
#include "map_render.h"     /* mr_sprite_fn (the bank,frame -> cel resolver)   */

/* res 0x40b = sprite slot 407 -> bank 0x1a4 (ar_pool_get_slot: slot + 0xd). */
#define SWORD_TRAIL_BANK 0x1a4u
/* The up-attack tip-trail never has more than ~18 alive at once; 32 is slack. */
#define SWORD_TRAIL_SLOTS 32

/* The emit window, in swing-ticks (attack_timer): the recording's first sparkle
 * lands at attack_timer 9 (body frame 2), the last at 17 — 9 ticks, 2 each. */
#define SWORD_TRAIL_EMIT_START 9
#define SWORD_TRAIL_EMIT_TICKS 9

/*
 * The trail pool — parallel {actor, render-state} arrays like particle_pool,
 * sized to the live trail.  A slot is active when its render-state's `active`
 * byte is set.  cursor is the round-robin alloc cursor (0x557370).
 */
typedef struct sword_trail {
    uint16_t           cursor;
    actor              actors[SWORD_TRAIL_SLOTS];
    actor_render_state states[SWORD_TRAIL_SLOTS];
} sword_trail;

/* Zero the pool (all slots inactive, cursor 0). */
void sword_trail_reset(sword_trail *t);

/*
 * Emit the two sparkles for one up-attack swing tick.  `swing_tick` is the
 * char's attack_timer (0-based, advanced by character_resolve_attack BEFORE
 * this is called); the emitter fires ONLY for swing_tick in
 * [SWORD_TRAIL_EMIT_START, START+TICKS-1].  (anchor_wx, anchor_wy) = Arche's
 * world pos (she is stationary through the thrust); (base_dx, base_dy) = her
 * freeroam render anchor (g_freeroam_rs.dst_base); facing = CHAR_FACE_* — the
 * RIGHT-facing tip-arc mirrors its dx about the anchor when facing LEFT.
 */
void sword_trail_emit(sword_trail *t, int swing_tick,
                      int32_t anchor_wx, int32_t anchor_wy,
                      int32_t base_dx, int32_t base_dy, int facing);

/*
 * Age every live sparkle one sim-tick: advance the 24->31 clip and expire the
 * slot when the one-shot finishes (the `done` flag).  RNG-free.  Mirrors the
 * particle band stepping BEFORE the emitters (engine-quirk #95) so a sparkle
 * spawned this tick renders UNSTEPPED (frame 24) until the next.
 */
void sword_trail_step(sword_trail *t);

/*
 * Render every live sparkle through 0x493480's default arm — describe the cel
 * then emit a MODE-1 (alpha) node via draw_pool_emit with param8 = the constant
 * additive ramp_b descriptor index.  The present (PRESENT_ALPHA) blends it.
 * Returns the number of sparkles emitted.
 */
int sword_trail_render(const sword_trail *t, draw_pool *pool,
                       mr_sprite_fn resolve, void *resolve_ctx);

#endif /* OSS_SWORD_TRAIL_H */
