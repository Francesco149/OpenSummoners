/*
 * world_alloc.{c,h} — making and freeing the WORLD (retail layout; rhost-verified inside retail).
 *
 * The room setup 0x586010 deletes the world and makes a new one every room.  0x4017d0 (the
 * constructor) allocates, in this order: the effects-volume fader (0x2c B, +0x1040), the 64
 * shots (0x258 B), the 32 characters (0x15bb0 B; their +0x210 block initialized by 0x401c70 and
 * eight areas zeroed), the 128 actors (0x2fc), the 1024 particles (0x284), the 96 band-96
 * entities (0x20c), the 128 at +0x2560 (0x1f8, the bare entity), the grid (0x2cb044 B, +0x1048),
 * the view (0x78 B, +0x104c), +0x1050 (0xa52c B), the event runner (0xb2c B, +0x1054), the five
 * entity lists +0x2788..+0x2798 (12 B each, then 128 slots of 0x200 B); every entity starts with
 * no bodies / sound channels / rects / part table.  0x54bfb0 frees it all (the entities through
 * 0x401cb0, the characters through 0x54c3c0 — their voice set's playing lines are stopped — the
 * actors through 0x54c2e0), then the lists, the grid, the view with its draw pools, +0x1050,
 * the runner, +0x1058, the fader.
 *
 *   FUN_004017d0  world_new            FUN_00401c70  char_block_init
 *   FUN_0054bfb0  world_free           FUN_00401cb0  entity_free
 *   FUN_0054c3c0  char_free            FUN_0054c2e0  actor_free
 */
#ifndef OSS_WORLD_ALLOC_H
#define OSS_WORLD_ALLOC_H

#include <stdint.h>

#include "world_layout.h"

/* 0x4017d0 (thiscall memory): construct the world in `mem` (0x27b8 bytes); returns it */
wl_world *world_new(void *mem);
/* 0x54bfb0 (fastcall world): free everything the world owns (not the world itself) */
void world_free(wl_world *w);
/* 0x401c70 (thiscall): the 0x30-byte block at character +0x210 */
void char_block_init(uint8_t *b);
/* 0x401cb0 (thiscall entity): free its bodies, sound channels, rects and part table */
void entity_free(wl_entity *e);
/* 0x54c2e0 (thiscall actor): + its +0x2a8 buffer */
void actor_free(wl_actor *a);
/* 0x54c3c0 (thiscall character): + its voice set (playing lines stopped) */
void char_free(wl_char *c);

#endif /* OSS_WORLD_ALLOC_H */
