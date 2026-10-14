/*
 * world_tick.{c,h} — the per-tick ENTITY driver FUN_0046cd70 (actor_update_all): one
 * world tick of every entity band.  Faithful on the retail layout; rhost-verified.
 *
 *   hitstop (the controller's +0x38/+0x3a/+0x3c: freeze every other tick while armed),
 *   world tick counter (world+0x276c), then per band in retail order:
 *   - the 32 characters: the support probe (0x54e5c0, +1) then the player AI char_ai
 *     (char+0x200 == 0, unless `cut`) or the NPC behaviour 0x47b990 (+0x200 counting to 1);
 *     then for each character the apply pass char_apply (movement included);
 *   - the 64 shots (0x4710c0 per part, 0x46d4a0 per sub-effect), the 1024 particles
 *     (0x46e510), the 96-band (0x46da00), the 128 actors (cooldown, the 16-tick faction
 *     box check 0x5567a0, the actor update 0x54f980 per part, 0x46d7f0, 0x54d090, 0x46d180);
 *   - the map's tile-animation phase in the grid header (+0x26 counts 0..5, then +0x24
 *     steps 0..5), after a pending 0x54c640 refresh.
 *
 *   FUN_0046cd70  world_tick
 */
#ifndef OSS_WORLD_TICK_H
#define OSS_WORLD_TICK_H

#include <stdint.h>

#include "world_layout.h"

/* `ctl` = the scene controller (retail `this`, its +0x38/+0x3a/+0x3c hitstop words);
 * `cut` != 0 = a cutscene tick (no player AI). Returns retail's eax. */
int world_tick(void *ctl, int32_t cut);

#endif /* OSS_WORLD_TICK_H */
