/*
 * game_map.{c,h} — the in-game MAP OBJECT (the engine FUN_0059f2c0's fresh-entry
 * arm: the operator_new(0x4120) world object + its 8 actor slots + the
 * FUN_004c5350 room-key resolution).
 *
 * On a fresh new-game entry (the in_stack_0000eb2c==0 arm of FUN_0059f2c0, lines
 * 160-218) the engine:
 *   - operator_new(0x4120)        — the map object (this struct's `buf`);
 *   - operator_new(0xeec) x8      — the 8 per-actor sub-objects, stored into
 *                                   map+0x4030..+0x404c (here: `actors[8]`);
 *   - inits each actor slot via FUN_00560e60 + stamps its slot index at +0xa0c;
 *   - writes the map-object header fields (map+0x4010..+0x40e4, +0x4108..);
 *   - sets map+0x4104 = the map id (0x3f2 for the opening town);
 *   - calls FUN_004c5350, whose `map==0x3f2` arm writes the ROOM-LOOKUP KEY
 *     map+0x4024 = 0x334be (= room 210110, "Town of Tonkiness"), plus the entry
 *     spawn params +0x4028=0x65 / +0x402c=1.
 * The per-room loop (LAB_0059fd85) then fetches the active room with
 * FUN_00561c90(scene[4], map+0x4024) — see game_world_find_room.
 *
 * This module ports exactly that construction — verbatim field writes by byte
 * offset, the actor-slot init, and the FUN_004c5350 0x3f2/1 arms' pure state
 * writes — over a zero-initialised map buffer.  It is pure (no Win32) and
 * host-tested.  The sim (0x586010) and render (0x5a00c0) are separate, later
 * units; this is the world-runtime object they read, sitting on the
 * game_world table layer.
 *
 * PORT NOTES / fidelity boundaries (documented, not hidden):
 *   - The map buffer is ZERO-INITIALISED here, whereas retail's operator_new
 *     returns raw memory and relies on the explicit field writes (+ a few
 *     unported sub-inits: 0x5612b0/0x5611d0/0x4e59a0) to set the
 *     rest.  Every field the verified path READS is either written explicitly
 *     below or is one of those sub-inits' outputs; zero-init is the clean
 *     baseline.  In particular map+0x4020 (the +0x4014 ramp ceiling read by
 *     FUN_004c5350) is set by an unported sub-init, so under zero-init the
 *     0x4014 ramp is inert — it does not affect the room-key resolution.
 *   - The 8 actor-slot pointers live in `actors[8]` (host pointers don't fit the
 *     4-byte map+0x4030 slots on a 64-bit host); the engine's slot-deref loop is
 *     ported against that array.
 *   - The opaque sprite/animation sub-calls inside FUN_004c5350 (0x408dc0,
 *     0x413b20, 0x4c5e00, 0x412db0, …) manage the sprite registry,
 *     not the map fields we verify, and are skipped.
 */
#ifndef OSS_GAME_MAP_H
#define OSS_GAME_MAP_H

#include <stdint.h>
#include "game_world.h"

#define GM_OBJ_SIZE   0x4120u   /* operator_new(0x4120) — the map object        */
#define GM_ACTOR_SIZE 0xeecu    /* operator_new(0xeec)  — one actor sub-object  */
#define GM_ACTOR_N    8         /* 8 actor slots at map+0x4030..+0x404c          */

/* Named map-object field offsets (bytes) used/verified by the port. */
#define GM_MAPID    0x4104      /* the map id (0x3f2 = opening town)             */
#define GM_ROOMKEY  0x4024      /* FUN_00561c90 lookup key (0x334be for 0x3f2)   */
#define GM_SPAWN_A  0x4028      /* entry spawn param (0x65 for 0x3f2)            */
#define GM_SPAWN_B  0x402c      /* entry spawn param (1 for 0x3f2)              */
#define GM_TICK     0x4068      /* GetTickCount stamp (the in-game pace clock)   */
#define GM_STATE54  0x4054      /* header field set to 3 on fresh entry          */
#define GM_ACTORS   0x4030      /* base of the 8 actor-slot pointers             */
#define GM_ACTIVE   0x4084      /* base of the 8 per-slot active flags (=1)      */

typedef struct game_map {
    uint8_t *buf;                       /* GM_OBJ_SIZE bytes — the map object    */
    uint8_t *actors[GM_ACTOR_N];        /* GM_ACTOR_SIZE bytes each              */
} game_map;

/* Build the fresh-entry map object for `map_id` (pass 0x3f2 for the opening
 * town; 0 defaults to 0x3f2, matching FUN_0059f2c0:378-381).  `tick` is stored
 * at +0x4068 (retail uses GetTickCount(); pass 0 for a deterministic build).
 * Returns 0 on success, -1 on allocation failure.  game_map_free releases it. */
int  game_map_build(game_map *m, uint32_t map_id, uint32_t tick);
void game_map_free(game_map *m);

/* Read/write a dword field of the map object at byte offset `off`. */
static inline uint32_t gm_dw(const game_map *m, unsigned off)
{
    uint32_t v;
    __builtin_memcpy(&v, m->buf + off, 4);
    return v;
}

/* The room-lookup key the room loop passes to FUN_00561c90 (map+0x4024). */
static inline uint32_t game_map_room_key(const game_map *m)
{
    return gm_dw(m, GM_ROOMKEY);
}

/* Resolve the active room record in `w` via the map's +0x4024 key
 * (FUN_00561c90).  Returns the 0x54-dword record, or NULL. */
uint32_t *game_map_active_room(const game_map *m, game_world *w);

#endif /* OSS_GAME_MAP_H */
