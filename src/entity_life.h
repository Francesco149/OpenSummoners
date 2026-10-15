/*
 * entity_life.{c,h} — the world entities' LIST membership and RELEASE (retail layout;
 * rhost-verified inside retail).
 *
 * An entity sits in up to four world lists (+0x278c the movers' collide list, +0x2788,
 * +0x2790 the actors by handle, +0x2794 the riders); actors also in +0x2798.  Release
 * frees what the entity allocated (collision rects, per-body frame records, the
 * multi-part table, the looping-sound channels — each stopped and given back) and
 * clears its alive flag; a character also drops what it carries / is carried by, its
 * linked entities and its voice set.  A platform that moved carries its riders (bodies
 * whose +0x284 is it) the same distance, recursively.
 *
 *   FUN_00555fc0  entity_unlist          FUN_00556180  entity_release
 *   FUN_0041ea70  actor_release          FUN_004467d0  char_release           FUN_0041e360  char_unload
 *   FUN_00556380  entity_set_collide     FUN_005563d0  entity_carry_riders
 *   FUN_0040bd90  wl_list_remove         FUN_00555f00  entity_set_handle
 */
#ifndef OSS_ENTITY_LIFE_H
#define OSS_ENTITY_LIFE_H

#include <stdint.h>

#include "world_layout.h"

/* take `e` out of list `l` (the first slot holding it; the rest shift down) */
void wl_list_remove(wl_list *l, const wl_entity *e);
void entity_unlist(wl_entity *e);
/* `listed` == 0 also takes it out of the four lists first. */
void entity_release(wl_entity *e, int32_t listed);
void actor_release(wl_actor *a);
void char_release(wl_char *c);
/* 0x41e360 (stdcall): a story script's character removal — char_release with the voice lines stopped
 * through 0x41e440 (NULL: nothing) */
void char_unload(wl_char *c);
/* Set the collision mode / kind; a solid one (mode != 0, kind 1..3) joins the collide
 * list when it has room. */
void entity_set_collide(wl_entity *e, int32_t mode, int32_t kind);
void entity_carry_riders(wl_entity *e, int32_t dx, int32_t dy, uint16_t part);
/* 0x555f00 (thiscall entity): file it under handle `h` (+0x1d8) in the world's +0x2790 list: out
 * of it when it had one, back in (while there is room) when h != 0 */
void entity_set_handle(wl_entity *e, int32_t h);

#endif /* OSS_ENTITY_LIFE_H */
