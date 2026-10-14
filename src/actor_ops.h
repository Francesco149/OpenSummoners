/*
 * actor_ops.{c,h} — the per-code actor behaviours behind the actor update 0x54f980's
 * slots (actor_update.c), retail layout; rhost-verified inside retail.
 *
 * The CRATE (0x555790): a block that rests (kind 1: falls once something stands on it,
 * kind 4: falls), drops with gravity (+2000 per tick to 80000), on landing breaks
 * (velocity >= the break threshold: material sound + 20 dust, then released) or thuds
 * (>= 20000: dust + sound 37); kind 2 (fell) is pushable and plays its sound at the push
 * cooldown's 50.  Optionally it slides back and forth between area x0..x1 / y0..y1
 * (sound 221 at each turn) and carries its riders along.
 *
 *   FUN_00555790  actor_crate            FUN_00555bd0  actor_break
 *   FUN_00555e90  actor_slide_v          FUN_005564e0  entity_count_riders
 *   FUN_0040ab80  entity_dust
 */
#ifndef OSS_ACTOR_OPS_H
#define OSS_ACTOR_OPS_H

#include <stdint.h>

#include "world_layout.h"

int actor_crate(wl_actor *a, wl_body *b, int32_t probe, uint16_t part, int32_t x0, int32_t y0, int32_t hold,
                int32_t pushed, int32_t speed, int32_t break_v, int32_t slide, int32_t slide_sfx, uint16_t push_sfx);
void actor_break(wl_actor *a, wl_body *b);
/* one step of the vertical slide between area y0..y1 (body +0x30 the direction); 1 = it turned */
int actor_slide_v(wl_actor *a, wl_body *b, int32_t speed);
/* Entities riding `e` (their body[0] +0x284), filtered by +0x1b8 `kind` and faction `fac`
 * (0 = any), within `reach` of its centre (0 = any), `heavy` != 0 counting only +0x1bc ones;
 * *weight (if given) = the sum of their +0x1ec plus their own riders' (recursively). */
int entity_count_riders(wl_entity *e, int32_t kind, int32_t heavy, int32_t fac, uint16_t *weight, int32_t reach);
/* n (u16) dust particles (100080) along body `b` (a3 != 0: none) */
void entity_dust(wl_entity *e, wl_body *b, int32_t a3, uint32_t n, int32_t dx, int32_t dy);

#endif /* OSS_ACTOR_OPS_H */
