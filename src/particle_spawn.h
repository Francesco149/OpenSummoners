/*
 * particle_spawn.{c,h} — the PARTICLE SPAWNER FUN_00557370 (retail layout;
 * rhost-verified inside retail).
 *
 * Takes the next dead slot of the 1024 world particles round-robin (cursor u16 at
 * (world+0x1044)+0xce); with none free it evicts the live particle with the lowest
 * +0x280 priority below `prio` (released first), or gives up.  The position is
 * (dx, dy) from an anchor on body `b` (1 centre, 2 top-left, 3 top centre, 4 bottom
 * centre, else absolute), dx mirrored when `mirror` and the body faces left; the
 * per-code setup is the particle init 0x557550 (particle_init.c).
 *
 *   FUN_00557370  particle_spawn
 */
#ifndef OSS_PARTICLE_SPAWN_H
#define OSS_PARTICLE_SPAWN_H

#include <stdint.h>

#include "world_layout.h"

/* `self`'s faction is handed to the init; a9 != 0 spawns nothing.  Returns the particle. */
wl_entity *particle_spawn(wl_entity *self, wl_entity *owner, int32_t code, const wl_body *b, int32_t dx, int32_t dy,
                          int32_t a7, int32_t anchor, int32_t a9, int32_t a10, int32_t a11, int32_t mirror, int32_t a13,
                          int32_t prio);

#endif /* OSS_PARTICLE_SPAWN_H */
