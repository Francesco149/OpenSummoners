/*
 * tests/test_game_map.c — the in-game MAP OBJECT (src/game_map.c: the engine
 * FUN_0059f2c0's fresh-entry arm + FUN_004c5350's room-key resolution).
 *
 * Ground truth from the disassembly (docs/findings/in-game-intro.md
 * "ROOM lookup", FUN_0059f2c0:160-218, FUN_004c5350:72-109): on a fresh
 * new-game entry with map 0x3f2 the engine builds the 0x4120 map object + 8
 * actor slots, sets the header fields, and FUN_004c5350 writes the room-lookup
 * key map+0x4024 = 0x334be (room 210110 "Town of Tonkiness").
 */
#include "t.h"
#include "game_map.h"
#include "game_world.h"

#include <stdint.h>

/* A fresh map 0x3f2 build resolves the room-lookup key to 0x334be and the
 * entry spawn params to 0x65 / 1 (FUN_004c5350's map==0x3f2 arm). */
int test_game_map_3f2_room_key(void)
{
    game_map m;
    T_ASSERT_EQ_I(game_map_build(&m, 0x3f2, 0), 0);
    T_ASSERT_EQ_U(gm_dw(&m, GM_MAPID),   0x3f2);
    T_ASSERT_EQ_U(game_map_room_key(&m), 0x334be);
    T_ASSERT_EQ_U(gm_dw(&m, GM_SPAWN_A), 0x65);
    T_ASSERT_EQ_U(gm_dw(&m, GM_SPAWN_B), 1);
    game_map_free(&m);
    return 0;
}

/* The fresh-entry header field writes (FUN_0059f2c0:171-214): the verified
 * scalar fields land as the decomp specifies. */
int test_game_map_header_fields(void)
{
    game_map m;
    T_ASSERT_EQ_I(game_map_build(&m, 0x3f2, 0x12345678), 0);
    T_ASSERT_EQ_U(gm_dw(&m, 0x40a4),    1);
    T_ASSERT_EQ_U(gm_dw(&m, 0x4018),    1);
    T_ASSERT_EQ_U(gm_dw(&m, GM_STATE54), 3);     /* +0x4054 = 3 */
    T_ASSERT_EQ_U(gm_dw(&m, GM_TICK),   0x12345678);
    T_ASSERT_EQ_U(gm_dw(&m, 0x406c),    0);
    /* +0x4064 dword write last, overwriting the u16 (=1) at +0x4064. */
    T_ASSERT_EQ_U(gm_dw(&m, 0x4064),    0);
    /* +0x4062 stays the u16 =1 (in the dword at +0x4060, high half). */
    T_ASSERT_EQ_U(((const uint16_t *)(m.buf + 0x4062))[0], 1);
    game_map_free(&m);
    return 0;
}

/* The 8 actor slots: each is stamped with its slot index at +0xa0c and its
 * per-slot active flag at map+0x4084+4*i is set to 1. */
int test_game_map_actor_slots(void)
{
    game_map m;
    T_ASSERT_EQ_I(game_map_build(&m, 0x3f2, 0), 0);
    for (int i = 0; i < GM_ACTOR_N; i++) {
        T_ASSERT(m.actors[i] != NULL);
        T_ASSERT_EQ_U(((const uint16_t *)(m.actors[i] + 0xa0c))[0], (unsigned)i);
        T_ASSERT_EQ_U(gm_dw(&m, GM_ACTIVE + 4u * (unsigned)i), 1);
    }
    game_map_free(&m);
    return 0;
}

/* The 3 {1,0} dword pairs at +0x4108 fill the object's tail exactly to +0x4120. */
int test_game_map_tail_pairs(void)
{
    game_map m;
    T_ASSERT_EQ_I(game_map_build(&m, 0x3f2, 0), 0);
    T_ASSERT_EQ_U(gm_dw(&m, 0x4108), 1);  T_ASSERT_EQ_U(gm_dw(&m, 0x410c), 0);
    T_ASSERT_EQ_U(gm_dw(&m, 0x4110), 1);  T_ASSERT_EQ_U(gm_dw(&m, 0x4114), 0);
    T_ASSERT_EQ_U(gm_dw(&m, 0x4118), 1);  T_ASSERT_EQ_U(gm_dw(&m, 0x411c), 0);
    /* last dword ends at 0x411c+4 == GM_OBJ_SIZE. */
    T_ASSERT_EQ_U(0x411c + 4, GM_OBJ_SIZE);
    game_map_free(&m);
    return 0;
}

/* End-to-end: the map's room key resolves, via game_world, to room 210110
 * "Town of Tonkiness" (area 0xd2, scene 1022) — the opening room. */
int test_game_map_resolves_opening_room(void)
{
    game_world w;
    game_map m;
    T_ASSERT_EQ_I(game_world_build(&w), 0);
    T_ASSERT_EQ_I(game_map_build(&m, 0x3f2, 0), 0);

    uint32_t *r = game_map_active_room(&m, &w);
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r[GW_ROOM_ID],    210110);
    T_ASSERT_EQ_U(r[GW_ROOM_AREA],  0xd2);
    T_ASSERT_EQ_U(r[GW_ROOM_SCENE], 1022);

    game_map_free(&m);
    game_world_free(&w);
    return 0;
}

/* A 0 map id defaults to 0x3f2 (FUN_0059f2c0:378-381), but — matching retail's
 * ordering (the default is applied AFTER FUN_004c5350) — the room key is NOT
 * resolved in that case, so the engine would rely on a later resolution. */
int test_game_map_zero_defaults_to_3f2(void)
{
    game_map m;
    T_ASSERT_EQ_I(game_map_build(&m, 0, 0), 0);
    T_ASSERT_EQ_U(gm_dw(&m, GM_MAPID), 0x3f2);
    T_ASSERT_EQ_U(game_map_room_key(&m), 0);   /* key not set on the 0 path */
    game_map_free(&m);
    return 0;
}
