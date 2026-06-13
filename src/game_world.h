/*
 * game_world.{c,h} — the in-game world's static-table layer (the data half of
 * the engine FUN_0059f2c0's fresh-entry world construction).
 *
 * On map entry FUN_0059f2c0:122-144 builds the world's `scene[4]` object from
 * two compiled-in .rdata tables:
 *   - the AREA registry  (&DAT_00693848) is pointed at by scene[4][0];
 *   - every ROOM registry (&DAT_006940c8) entry is COPIED, zero-terminated,
 *     into scene[4][1..], and FUN_00585000 is run per copied room to
 *     cross-reference it against the AREA table (fill per-room defaults) and
 *     against the other rooms (build the reciprocal room-transition exits).
 * The active room is then fetched by id via FUN_00561c90.
 *
 * This module ports exactly that table layer — verbatim field semantics, the
 * 585000 cross-reference, and the 561c90 lookup — over the generated table
 * bytes in world_tables_data.{c,h}.  It is pure (no Win32) and host-tested.
 * The map object (0x4120), the actor slots, the sim (0x586010) and the render
 * (0x5a00c0) are separate, later units; this is the data foundation they read.
 *
 * Layout mirrors the engine: a room record is 0x54 dwords (0x150 bytes); an
 * area record is 0x10 dwords (0x40 bytes).  scene[4]'s room region starts with
 * the header-sentinel entry [0] (dword0 == 0xf423f) followed by the real rooms;
 * the engine's room count (scene[4]+0x54004) and FUN_00561c90's search both
 * include that header entry, so we do too.
 */
#ifndef OSS_GAME_WORLD_H
#define OSS_GAME_WORLD_H

#include <stdint.h>

#define GW_ROOM_DWORDS 0x54   /* 0x150-byte room record */
#define GW_AREA_DWORDS 0x10   /* 0x40-byte  area record */

/* Room record dword fields used by name (the rest is sim scratch). */
#define GW_ROOM_ID    0x00    /* packed room id (e.g. 110110); 0 terminates  */
#define GW_ROOM_AREA  0x01    /* area key -> AREA.id (English name)          */
#define GW_ROOM_SCENE 0x03    /* sequential scene/record index (1002, ...)   */

/* Area record dword fields. */
#define GW_AREA_ID    0x00    /* area key (matched by ROOM[GW_ROOM_AREA])    */

typedef struct game_world {
    uint32_t *rooms;       /* room_count * 0x54 dwords — the working scene[4]
                              room region (FUN_00585000 mutates this).        */
    uint32_t *orig;        /* room_count * 0x54 dwords — the pristine registry
                              copy 585000's reciprocal-exit scan reads from
                              (the engine's param_1 = &DAT_006940c8).         */
    uint32_t *areas;       /* area_count * 0x10 dwords — read-only AREA table */
    uint16_t  room_count;  /* incl. the [0] header sentinel = scene[4]+0x54004 */
    uint16_t  area_count;  /* excl. the zero terminator                       */
} game_world;

/* Build the world's room registry from the compiled-in tables: count the
 * zero-terminated rooms, copy them, and run the FUN_00585000 cross-reference
 * per room (area defaults + reciprocal exits).  Returns 0 on success, -1 on
 * allocation failure.  game_world_free releases it. */
int  game_world_build(game_world *w);
void game_world_free(game_world *w);

/* FUN_00561c90: linear search of the room region for the entry whose dword0
 * (id) == `id`.  Returns a pointer to that 0x54-dword record, or NULL. */
uint32_t *game_world_find_room(game_world *w, uint32_t id);

/* Resolve a room key's RENDER CONFIG from the registry — what main.c needs to
 * load + decode the room's backdrop the way the engine does.  The map-load call
 * site FUN_00586010:697 is FUN_00587e00(map, room[0x44], local_918, room[0x43]),
 * so the 0x587e00 prologue's parallax/tileset selection reads param_2=room[0x44]
 * and param_4=room[0x43]; the EXE keys the DATA resource on room[GW_ROOM_SCENE].
 *   *scene  <- room[GW_ROOM_SCENE]   (the FindResourceA DATA id)
 *   *px_p2  <- room[0x44]            (parallax_select param_2)
 *   *px_p3  <- room[0x43]            (parallax_select param_3 — the town's
 *                                     existing room[0x43] approximation of the
 *                                     real local_918; see town_render.h)
 * Returns 0 (room found, outputs filled) or -1 (unknown key, outputs untouched).
 * Any out pointer may be NULL. */
int game_world_room_render_cfg(game_world *w, uint32_t room_key,
                               uint16_t *scene, int *px_p2, int *px_p3);

/* Convenience read accessor for room record `idx` dword `k`. */
static inline uint32_t gw_room_dw(const game_world *w, unsigned idx, unsigned k)
{
    return w->rooms[idx * GW_ROOM_DWORDS + k];
}

#endif /* OSS_GAME_WORLD_H */
