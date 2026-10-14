/*
 * char_apply.{c,h} — the per-character APPLY pass FUN_00485fc0 (4.7 KB): everything a
 * character does each tick around its movement.  The tick driver 0x46cd70 runs it for
 * the 32 characters after their AIs.  Faithful on the retail layout; rhost-verified
 * (`--hook shadow:char_apply`, full state).
 *
 * Order: regen (char+0x158c0, every 1600 ticks), falling out of the map / pit tiles ->
 * damage + checkpoint respawn / leash warp, timers (invulnerability, rect cooldowns,
 * recent-hit list), the queued voice line + positional voice volume, ally proximity,
 * idle animations, then — unless stunned — the INTEGRATOR on the live body (and on
 * each extra part), water splashes, the water dust, fast-run effects, footsteps by
 * ground surface, the party-member pose watch; the tail (leash timeout, idle yawn for
 * Arche's forms, linked-effect release).  Unported callees via engine_calls slots.
 *
 *   FUN_00485fc0  char_apply
 */
#ifndef OSS_CHAR_APPLY_H
#define OSS_CHAR_APPLY_H

#include "world_layout.h"

void char_apply(wl_char *self);

#endif /* OSS_CHAR_APPLY_H */
