/*
 * particle_tick.{c,h} — the per-tick PARTICLE update FUN_0046e510 (10.7 KB): one world
 * particle (world+0x13e0, wl_particle), dispatched on its code (+0x1d4, 100042..100137).
 * Faithful on the retail layout; rhost-verified inside retail (`--hook
 * shadow:particle_tick`, full state).
 *
 * One call: the delayed start (pose 1 counts +0x5c down, then the body goes live), the
 * ride on a parent entity (+0x10, at +0x14/+0x18 from its top centre), then the code's
 * motion: gravity / rise / drift with caps, direction-vector drift (+0x278/+0x27c),
 * fades through the tint ramps at sys+0x2758 / +0x27a8 (+0xf4/+0xf8), clip changes,
 * splash drops that die in water, a falling rock with its own rumble channel, a
 * bouncing ball with impact sounds, the orbit (x87 fsin / fcos), spawners; a finished
 * clip or an expired count kills it (0x556180).  The common tail: the lifetime
 * (+0x258 / +0x26c, u16) kills and returns; the looping-sound channels (0x46d180
 * inlined, when +0x270); the clip step; the effect records (0x54d090); the parent
 * offsets re-derived from the body.
 *
 *   FUN_0046e510  particle_tick        FUN_00471080  round_half_away (x87)
 */
#ifndef OSS_PARTICLE_TICK_H
#define OSS_PARTICLE_TICK_H

#include <stdint.h>

#include "world_layout.h"

void particle_tick(wl_particle *p);

#endif /* OSS_PARTICLE_TICK_H */
