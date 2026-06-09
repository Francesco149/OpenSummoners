/*
 * butterfly.{c,h} — the town BUTTERFLIES' per-sim-tick RNG behaviour (the EFFECT
 * band's only per-tick LCG consumer).  See engine-quirk #95 + findings/
 * in-game-intro.md "The per-tick RNG stream".
 *
 * Of the 19 EFFECT-band actors in the town (15 map + 4 cutscene cast), ONLY the
 * 4 butterflies (code 0xe29a) draw RNG per sim-tick: the per-tick driver 0x46cd70
 * calls the behaviour 0x47b990 only for actors whose update-mode (+0x200) is 1,
 * and the butterflies are the only such EFFECT actors (the 11 standing townsfolk
 * + the cast take the RNG-free arm 0x478ba0).  So this module IS the EFFECT-band
 * per-tick stream — porting it (consume-to-advance) keeps the shared LCG aligned
 * for the downstream CHARACTER-band emitters (the fountain / sky particles), whose
 * per-tick positions otherwise drift (PORT-DEBT(fountain-rng-phase)).
 *
 * The draw model, read off 0x47b990 (the 0xe29a case at :768-801 + the generic
 * wander block :509-545) and validated bit-exact against the seed-pinned per-tick
 * census (runs/rng-census-repin):
 *   - The behaviour runs every OTHER sim-tick: 0x14232 is a 1-bit gate (work on
 *     the tick it is 0, set it 1; next tick decrement to 0 and return).  Fresh-
 *     spawned butterflies share a phase, so all 4 work on the EVEN ticks.
 *   - On a work tick the wander-pick timer 0x14236 (a work-tick countdown from
 *     0x50) gates the "choose a new flit" draws: when it hits 0 it draws once
 *     ((rand*1000)>>15 < 0xc874 ? — the move-frequency test), and if that passes,
 *     a second draw (the flit offset), then reloads 0x50.  Fresh-spawned it is 0,
 *     so the pick fires on the spawn tick and then every 0x50 = 80 work-ticks =
 *     160 sim-ticks.
 *   - The 0xe29a case then ALWAYS draws twice: the heading 0xc890 =
 *     (rand*0xc80>>15)+0x640 and the flutter flag (rand*1000>>15 < 100).
 * Per work tick a butterfly draws 2 (heading+flag), or 3-4 when the wander pick
 * fires.  The drawn values feed the flit MOTION (0x43f880, the 5.5 KB movement /
 * collision FSM) + the facing/bounds — DEFERRED (PORT-DEBT(butterfly-wander) is
 * now blocked on that FSM, not on RNG), so the values are consumed-to-advance:
 * the butterflies hold their map-spawn positions but the stream stays aligned.
 *
 * Each butterfly's 0xc874 (the move-frequency threshold, ~650-749) is set at
 * spawn by 0x427670 case 2 (= (rand*100>>15)+0x28a, the 5th of its 5 draws);
 * actor_spawn_effect_from_map captures it from the spawn replay.
 *
 * Win32-free + pure (it only advances the shared LCG); host-tested.
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

typedef struct {
    uint16_t wander_freq;   /* 0xc874 — the (rand*1000>>15) < freq move test     */
    int16_t  gate;          /* 0x14232 — 0 => work this sim-tick, 1 => skip next */
    int16_t  wander_timer;  /* 0x14236 — work-ticks until the next flit pick     */
} butterfly;

typedef struct butterfly_pool {
    butterfly b[BUTTERFLY_MAX];
    int       count;
} butterfly_pool;

/* Clear the pool (call at scene entry, before the spawn registers the town's). */
void butterfly_pool_reset(butterfly_pool *p);

/* Register one butterfly with its spawn-derived move frequency (0xc874).  Order
 * matters: register in EFFECT-band (map-layer) order so the per-tick draw order
 * matches retail's 0x46cd70 walk.  Returns the slot index, or -1 if full/NULL. */
int butterfly_register(butterfly_pool *p, uint16_t wander_freq);

/* One sim-tick of the butterfly behaviour for the whole pool: advances the shared
 * LCG by each working butterfly's draws (heading + flag, + the wander pick when
 * its timer fires), in registration order.  MUST run in the 0x46cd70 slot — i.e.
 * the EFFECT band BEFORE the CHARACTER-band particle emitters — so the stream
 * stays aligned.  Returns the number of LCG draws consumed this tick. */
int butterfly_step(butterfly_pool *p);

#endif /* OSS_BUTTERFLY_H */
