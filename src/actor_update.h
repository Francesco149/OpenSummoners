/*
 * actor_update.{c,h} — the per-tick ACTOR update FUN_0054f980 (11.6 KB): one body of a
 * world actor (world+0x11e0, wl_actor) — `in` -> `out`, in == out for the primary,
 * then each extra part — dispatched on the actor's code (+0x1d4).  Faithful on the
 * retail layout; rhost-verified inside retail (`--hook shadow:actor_update`, full state).
 *
 * One call: copy the body (the same in -> out stamp as the integrator), the ground probe
 * (the full mover 0x54e5c0 when +0x2c8, else 0x442460), then the code's behaviour:
 * swaying / bobbing props, fountains and drips (particles), growing platforms (they set
 * the grid's refresh flag), the ambient sound emitters (looping channels through
 * 0x450ef0 / 0x450e00, time-of-day and scene-lock gated, LCG-timed), the earthquake
 * emitter (camera shake + falling debris), the switch-off step records, a fade-in /
 * fade-out / fly-to actor, a follower, the bird take-off; the larger behaviours
 * (doors, lifts, NPC-like props: 0x555790, 0x554c40, 0x553ec0, 0x553520, 0x552c70,
 * 0x552f60, 0x554570, 0x54f140, 0x5556d0, 0x5531b0) are engine_calls slots.  Every path
 * ends in the region-C flag pass 0x426be0, whose eax retail returns.
 *
 *   FUN_0054f980  actor_update         FUN_00445db0  speed_ramp
 *   FUN_00442460  entity_ground_probe  FUN_00556eb0  actor_by_handle
 *   FUN_0047d740  world_scene_running
 */
#ifndef OSS_ACTOR_UPDATE_H
#define OSS_ACTOR_UPDATE_H

#include <stdint.h>

#include "world_layout.h"

int actor_update(wl_actor *a, wl_body *in, wl_body *out, int32_t probe, int32_t part);

/* Step `cur` toward `target` by `step` (signs by target / direction), braking by `brake`
 * past it; never crosses zero the wrong way. */
int32_t speed_ramp(int32_t cur, int32_t step, int32_t brake, int32_t target);
/* The one-tick ground probe (0x54e5c0, delta 1): 1 when supported. */
int entity_ground_probe(wl_entity *e, wl_body *b, uint16_t part);
wl_entity *actor_by_handle(int32_t handle);
int world_scene_running(void);

#endif /* OSS_ACTOR_UPDATE_H */
