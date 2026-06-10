/*
 * butterfly.{c,h} — the town BUTTERFLIES' per-sim-tick behaviour: the EFFECT
 * band's only per-tick LCG consumer (the RNG draws) AND, as of chip 1, their
 * open-air PATROL MOTION (the left/right drift).  See engine-quirk #95 +
 * findings/in-game-intro.md "The per-tick RNG stream" + the movement arc
 * docs/plans/movement-system.md (Phase 4, chip 1).
 *
 * Of the 19 EFFECT-band actors in the town (15 map + 4 cutscene cast), ONLY the
 * 4 butterflies (code 0xe29a) take the mode-1 AI 0x47b990 (the 11 townsfolk +
 * the cast take the RNG-free 0x478ba0).  So this module IS the EFFECT-band
 * per-tick stream — porting it (same draw count/order) keeps the shared LCG
 * aligned for the downstream CHARACTER-band emitters (the ckpt-99 settled-town
 * stream is bit-exact and MUST NOT regress: the heading FSM here moves the two
 * RNG draws/tick into their real use, it does not add or reorder draws).
 *
 * ── The two clocks (capture-verified, runs/butterfly-fsm; movement-system.md) ──
 *   - The AI (0x47b990, the 0xe29a case 47b990.c:769-801) runs on WORK ticks only
 *     (the every-other gate 0x14232): it draws the wander range + the 10% flutter
 *     flag, decrements the flip cooldown, FLIPS the heading toward the far patrol
 *     bound when the cooldown is 0 and (within ~1 tile of the bound OR the 10%
 *     roll), and writes the move command (0x43f880) toward the near bound.
 *   - The APPLY (the band's 2nd EFFECT pass 0x485fc0 -> the integrator 0x442a70)
 *     integrates EVERY tick: it carries a horizontal velocity that ramps toward
 *     the commanded direction, so the butterfly keeps GLIDING on the gated-out
 *     ticks.  Decision is gated; motion glides.
 *
 * ── Chip-1 OPEN-AIR REDUCTION (PORT-DEBT(butterfly-wander) motion half) ──
 * The real 0x442a70 is a 12 KB shared integrator (X + Y + gravity + the vertical
 * flutter sawtooth + entity/tile collision sweeps) built for Arche; chip 2/3
 * generalise it.  Here we port only the OPEN-AIR HORIZONTAL law, fit bit-exact to
 * the capture's per-tick worldX on the non-reversal stretches:
 *   - hvel ramps +-10/tick toward +-100 (the commanded direction), worldX += hvel.
 * DEFERRED, tagged below: the VERTICAL flutter (body+0x18 vel sawtooth + the
 * cmd_2 flap sub-FSM -> worldY bob) and the flap/heading-reversal COUPLING (a
 * +-100 worldX lurch when a flap coincides with a turn).  See PORT-DEBT rows
 * butterfly-flutter + butterfly-bounds-writer in docs/port-debt.md.
 *
 * Win32-free + pure (advances the shared LCG + its own body state); host-tested.
 */
#ifndef OSS_BUTTERFLY_H
#define OSS_BUTTERFLY_H

#include <stdint.h>

/* The town has 4 butterflies; keep a little headroom. */
#define BUTTERFLY_MAX 8

/* 0x47b990:540 — the wander-pick timer reload (work-ticks between flit picks). */
#define BUTTERFLY_WANDER_PERIOD 0x50
/* 0x427670 case 2 — the move-frequency base added to (rand*100>>15). */
#define BUTTERFLY_FREQ_BASE 0x28a

/* The patrol bounds, set at register-time from the spawn worldX (ckpt-109 capture:
 * dead-constant per butterfly; PORT-DEBT(butterfly-bounds-writer) for the un-RE'd
 * spawn-time derivation).  center = spawn_wx + 1600, half = 9600 = 3 tiles. */
#define BUTTERFLY_BOUND1_OFF  11200   /* 0x14264 (heading 1 -> right) = spawn_wx + 11200 */
#define BUTTERFLY_BOUND3_OFF  (-8000) /* 0x14268 (heading 3 -> left)  = spawn_wx -  8000 */

/* 0x47b990:782 — flip when |worldX - target bound| < 0xc81 (~one tile). */
#define BUTTERFLY_BOUND_NEAR  0xc81
/* 0x47b990:786/498 — the flip cooldown reload (0x3c work/skip-weighted ticks). */
#define BUTTERFLY_FLIP_COOLDOWN 0x3c
/* The open-air horizontal patrol speed cap + per-tick ramp (fit to the capture). */
#define BUTTERFLY_HSPEED_CAP  100
#define BUTTERFLY_HSPEED_RAMP 10

typedef struct {
    /* --- AI / RNG state (the per-tick draw model, ckpt 95/98) --- */
    uint16_t wander_freq;   /* 0xc874 — the (rand*1000>>15) < freq move test     */
    int16_t  gate;          /* 0x14232 — 0 => work this sim-tick, 1 => skip next */
    int16_t  wander_timer;  /* 0x14236 — work-ticks until the next flit pick     */
    /* --- motion state (chip-1 open-air reduction) --- */
    int32_t  world_x;       /* body+4  — patrol position (mirrors the render-state) */
    int32_t  bound1;        /* 0x14264 — right target (spawn_wx + 11200)           */
    int32_t  bound3;        /* 0x14268 — left  target (spawn_wx -  8000)           */
    int32_t  heading;       /* 0x14244 — INTENT: 0(init)/1(right)/3(left)          */
    int32_t  hvel;          /* the open-air horizontal velocity (-100..+100)       */
    int16_t  facing;        /* body+0x2c — travel dir: 1 right / 3 left            */
    int16_t  cooldown;      /* 0x14248 — flip cooldown                             */
    int      cmd_dir;       /* the commanded move dir the apply integrates (+1/-1/0)*/
    int      effect_slot;   /* index of this butterfly's actor in the EFFECT pool  */
} butterfly;

typedef struct butterfly_pool {
    butterfly b[BUTTERFLY_MAX];
    int       count;
} butterfly_pool;

/* Clear the pool (call at scene entry, before the spawn registers the town's). */
void butterfly_pool_reset(butterfly_pool *p);

/* Register one butterfly with its spawn-derived move frequency (0xc874), its
 * spawn worldX (-> the patrol bounds), and the EFFECT-pool slot of its rendered
 * actor (so butterfly_apply can drive that actor's worldX/facing).  Order
 * matters: register in EFFECT-band (map-layer) order so the per-tick draw order
 * matches retail's 0x46cd70 walk.  Returns the slot index, or -1 if full/NULL. */
int butterfly_register(butterfly_pool *p, uint16_t wander_freq,
                       int32_t spawn_world_x, int effect_slot);

/* One sim-tick of the butterfly behaviour for the whole pool: advances the shared
 * LCG by each working butterfly's draws (heading + flag, + the wander pick when
 * its timer fires) in registration order, runs the gated heading FSM, and
 * integrates the open-air horizontal patrol motion (EVERY tick, both gate phases,
 * mirroring the 0x485fc0 apply pass).  MUST run in the 0x46cd70 slot — i.e. the
 * EFFECT band BEFORE the CHARACTER-band particle emitters — so the stream stays
 * aligned.  Returns the number of LCG draws consumed this tick. */
int butterfly_step(butterfly_pool *p);

#endif /* OSS_BUTTERFLY_H */
