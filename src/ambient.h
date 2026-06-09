/*
 * ambient.{c,h} — the town's IRREGULAR per-sim-tick RNG timers (the residual
 * after the regular per-tick stream of engine-quirk #95).  See findings/
 * in-game-intro.md "The per-tick RNG stream" + engine-quirk #95; closes
 * PORT-DEBT(fountain-rng-phase).
 *
 * The regular per-tick consumers (butterflies + fountain + sky, src/butterfly.c
 * + src/particle.c) are bit-exact through the establishing-REVEAL window, but
 * the SETTLED-town stream desynced beyond ~tick 33 because of four IRREGULAR,
 * self-clocked ambient timers.  Each is a per-object countdown that decrements
 * ONCE per sim-tick and FIRES (drawing the LCG, then reloading) when it expires
 * — modelled here consume-to-advance: the drawn values feed sounds / the wagon's
 * idle-wander / an 0xe2a5 sub-effect (none ported), but the COUNTS + TIMING keep
 * the shared LCG aligned so the fountain/sky particle positions stay bit-exact.
 *
 * Ground truth = the seed-pinned timer-state capture (runs/ambient-timer, the
 * 0x5531b0/0x467380/0x54f980 field-spec reads), which pinned each one's cadence
 * directly (all are clean unit-decrement):
 *
 *   timer            mechanism                  init          fires    draws
 *   ----------------------------------------------------------------------------
 *   0x1136f sound    0x5531b0 (CHARACTER band)  (rand*300)>>15 = 189   tick 189  +3
 *   0x11370 sound    0x5531b0 (CHARACTER band)  (rand*300)>>15 = 33    tick 33   +3
 *   wagon 0x1872d    0x54f980 idle-wander       (rand*300)>>15 = 134   tick 134  +3
 *   0x467380 0xe2a5  event timer (EFFECT band)  cd=184 (spawn-set)     tick 183  +4
 *
 * ORDER in the 0x46cd70 band walk (proven by the capture's per-tick seq order):
 *   EFFECT band:    butterfly_step (0xe29a) -> ambient_effect_step (0x467380)
 *   CHARACTER band: fountain emit -> sky emit -> ambient_character_step
 *                   (0x1136f, then 0x11370, then wagon — slot order).
 * The 3 CHARACTER inits at tick 0 must run in that order so each timer gets the
 * right init value (189 / 33 / 134) off the aligned stream and fires on cue.
 *
 * Win32-free + pure (it only advances the shared LCG); host-tested.
 */
#ifndef OSS_AMBIENT_H
#define OSS_AMBIENT_H

#include <stdint.h>

typedef struct {
    int32_t cd;       /* countdown (sim-ticks to fire) */
    int     armed;    /* 1 once the init draw has run (or the cd is pre-set) */
} ambient_timer;

typedef struct ambient_pool {
    ambient_timer sound_a;   /* 0x1136f  (0x5531b0) -> fires tick 189 */
    ambient_timer sound_b;   /* 0x11370  (0x5531b0) -> fires tick 33  */
    ambient_timer wagon;     /* 0x1872d  idle-wander -> fires tick 134 */
    ambient_timer event;     /* 0x467380 (0xe2a5)   -> fires tick 183 */
} ambient_pool;

/* Clear + arm the pool at scene entry (sets the event timer's spawn-set cd). */
void ambient_reset(ambient_pool *p);

/* EFFECT-band slot: the 0x467380 (0xe2a5) event timer.  Call once per sim-tick
 * AFTER butterfly_step and BEFORE the CHARACTER-band emitters.  Returns the
 * number of LCG draws consumed this tick. */
int  ambient_effect_step(ambient_pool *p);

/* CHARACTER-band slot: the 0x1136f / 0x11370 sound timers + the wagon's
 * idle-wander, in that order.  Call once per sim-tick AFTER the fountain/sky
 * emitters.  Returns the number of LCG draws consumed this tick. */
int  ambient_character_step(ambient_pool *p);

#endif /* OSS_AMBIENT_H */
