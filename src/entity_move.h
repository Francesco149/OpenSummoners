/*
 * entity_move.{c,h} — the ENTITY movers: the shared horizontal / vertical move entry
 * points every world entity goes through (characters, townsfolk, platforms, …).
 * Each first scans the world's collidable-entity list (g_world->collide, world+0x278c)
 * for a blocking entity, then hands the remaining move to the tile sweep
 * (collision.c).  Faithful ports on the retail layout (world_layout.h), verified
 * call-for-call inside retail by rhost (`--hook shadow:ent_move_h,shadow:ent_move_v`).
 *
 *   FUN_0054db10  entity_move_horizontal  (946 B, 21 callers)
 *   FUN_0054e5c0  entity_move_vertical    (945 B)
 *   FUN_004416c0  faction_relation        (52 B)
 */
#ifndef OSS_ENTITY_MOVE_H
#define OSS_ENTITY_MOVE_H

#include <stdint.h>

#include "world_layout.h"

/* FUN_004416c0: 0 if either side is 0, 1 if either is 3, else 2 if equal / 1 if not. */
int faction_relation(int32_t a, int32_t b);

/* FUN_0054db10 (thiscall self): move `body` (part `part` of `self`) by `delta` world
 * units along X.  An entity in g_world->collide (not self, part `part` present, its
 * coll_kind 1 or 3, mode gate passed, not attached to/from self) whose box the move
 * would enter returns 0 at once (no move).  Otherwise the tile sweep runs and its
 * result is returned; it writes body->x / body->y (step-up/down), or — `probe` != 0 —
 * only throwaway locals (a pure "could I move?" test).  `step_down`/`step_up`/`pass`
 * are the sweep's flags (collision_sweep_horizontal). */
int entity_move_horizontal(wl_entity *self, wl_body *body, int32_t delta,
                           int32_t step_down, int32_t step_up, int16_t pass,
                           uint16_t part, int32_t probe);

/* FUN_0054e5c0 (thiscall self): move `body` by `delta` along Y.  Moving up into a
 * kind-3 entity returns 0 with nothing written.  Moving down, the nearest kind-3 /
 * kind-2 (one-way; skipped while `pass` != 0) entity top within reach shortens the
 * delta to land on it (the result is then 0) and is reported through *support
 * (NULL when none; `support` may be NULL).  The tile mover then runs with the
 * (possibly shortened) delta, writing *out_y (collision_move_vertical). */
int entity_move_vertical(wl_entity *self, wl_body *body, int32_t delta,
                         int32_t pass, uint16_t part, int32_t *out_y,
                         wl_entity **support);

#endif /* OSS_ENTITY_MOVE_H */
