/*
 * tests/test_game_world.c — the in-game world's static-table layer
 * (src/game_world.c: the data half of the engine FUN_0059f2c0's fresh-entry
 * world construction).
 *
 * These exercise the port against the ground truth extracted by
 * tools/extract/game_world_tables.py (and the raw room/area dumps in
 * docs/findings/in-game-intro.md): the table counts, the opening-town first
 * room (110110, area 0x6e "Town of Tolkien", scene 1002), the FUN_00585000
 * area-default fill, the FUN_00561c90 id lookup, the header sentinel, and the
 * FUN_00585000 reciprocal room-exit builder.
 */
#include "t.h"
#include "game_world.h"

#include <stdint.h>

/* low/high 16-bit halves of a record dword (records are 4-aligned). */
static uint16_t dw_lo16(const uint32_t *r, unsigned dw) { return ((const uint16_t *)(r + dw))[0]; }

/* The world builds, with the expected zero-terminated counts:
 * 33 areas, and 417 room records (the [0] header sentinel + 416 real rooms). */
int test_game_world_build_counts(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);
    T_ASSERT_EQ_U(w.area_count, 33);
    T_ASSERT_EQ_U(w.room_count, 417);
    game_world_free(&w);
    return 0;
}

/* Room record [1] is the opening town's first district: id 110110, area key
 * 0x6e (110, "Town of Tolkien"), scene index 1002. */
int test_game_world_first_town_room(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);
    T_ASSERT_EQ_U(gw_room_dw(&w, 1, GW_ROOM_ID),    110110);
    T_ASSERT_EQ_U(gw_room_dw(&w, 1, GW_ROOM_AREA),  0x6e);
    T_ASSERT_EQ_U(gw_room_dw(&w, 1, GW_ROOM_SCENE), 1002);
    game_world_free(&w);
    return 0;
}

/* FUN_00585000 part 1: room 110110's still-zero default fields are filled from
 * its area (0x6e: A=4 -> room[0x44], B=2 -> room[0x45], C=1 -> room[0x43]).
 * The raw entry has these three dwords zero, so the area defaults apply. */
int test_game_world_area_defaults_filled(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);
    uint32_t *r = game_world_find_room(&w, 110110);
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r[0x43], 1);   /* C */
    T_ASSERT_EQ_U(r[0x44], 4);   /* A */
    T_ASSERT_EQ_U(r[0x45], 2);   /* B */
    game_world_free(&w);
    return 0;
}

/* FUN_00561c90: lookup by id finds the right record; a bogus id returns NULL. */
int test_game_world_find_room_by_id(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);

    uint32_t *r = game_world_find_room(&w, 110110);
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r[GW_ROOM_SCENE], 1002);

    uint32_t *silver = game_world_find_room(&w, 130110);
    T_ASSERT(silver != NULL);
    T_ASSERT_EQ_U(silver[GW_ROOM_AREA], 130);   /* Silver Dungeon */

    T_ASSERT_EQ_P(game_world_find_room(&w, 0xdeadbeef), NULL);
    game_world_free(&w);
    return 0;
}

/* The map-0x3f2 opening-room resolution.  FUN_004c5350's `map==0x3f2` arm sets
 * the map object's room-lookup key (+0x4024) to 0x334be; the room loop passes
 * it to FUN_00561c90.  0x334be == 210110: room id 210110, area key 0xd2
 * ("Town of Tonkiness"), scene 1022 — the actual opening room (NOT 110110
 * "Town of Tolkien", which the engine survey had assumed). */
int test_game_world_map_3f2_opening_room(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);

    T_ASSERT_EQ_U(0x334be, 210110);             /* the lookup key, decoded */
    uint32_t *r = game_world_find_room(&w, 0x334be);
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r[GW_ROOM_ID],    210110);
    T_ASSERT_EQ_U(r[GW_ROOM_AREA],  0xd2);      /* "Town of Tonkiness" */
    T_ASSERT_EQ_U(r[GW_ROOM_SCENE], 1022);
    game_world_free(&w);
    return 0;
}

/* game_world_room_render_cfg: a room key resolves to its DATA scene + the
 * 0x587e00 prologue parallax params (param_2 = room[0x44], param_3 = room[0x43]),
 * the values main.c's load_room passes to town_render_load.  The town-intro chain
 * keys (verified against the registry, tools/extract/game_world_tables.py):
 *   arrival 0x334be -> scene 1022, (p2=4, p3=1)   [the town; identical to the old
 *                                                   hardcoded TOWN_RENDER_PARALLAX]
 *   house   0x334c8 -> scene 1023, (p2=4, p3=1)
 *   errands 0x334dc -> scene 1025, (p2=9, p3=4)   [a DIFFERENT parallax param set]
 * An unknown key returns -1 (outputs untouched). */
int test_game_world_room_render_cfg(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);

    uint16_t scene; int p2, p3;

    T_ASSERT_EQ_I(game_world_room_render_cfg(&w, 0x334be, &scene, &p2, &p3), 0);
    T_ASSERT_EQ_U(scene, 1022); T_ASSERT_EQ_I(p2, 4); T_ASSERT_EQ_I(p3, 1);

    T_ASSERT_EQ_I(game_world_room_render_cfg(&w, 0x334c8, &scene, &p2, &p3), 0);
    T_ASSERT_EQ_U(scene, 1023); T_ASSERT_EQ_I(p2, 4); T_ASSERT_EQ_I(p3, 1);

    T_ASSERT_EQ_I(game_world_room_render_cfg(&w, 0x334dc, &scene, &p2, &p3), 0);
    T_ASSERT_EQ_U(scene, 1025); T_ASSERT_EQ_I(p2, 9); T_ASSERT_EQ_I(p3, 4);

    /* unknown key: -1, outputs untouched */
    scene = 0xffff; p2 = p3 = -99;
    T_ASSERT_EQ_I(game_world_room_render_cfg(&w, 0xdeadbeef, &scene, &p2, &p3), -1);
    T_ASSERT_EQ_U(scene, 0xffff); T_ASSERT_EQ_I(p2, -99); T_ASSERT_EQ_I(p3, -99);

    /* NULL outputs are tolerated */
    T_ASSERT_EQ_I(game_world_room_render_cfg(&w, 0x334be, NULL, NULL, NULL), 0);

    game_world_free(&w);
    return 0;
}

/* Room record [0] is the header sentinel (dword0 == 0xf423f). */
int test_game_world_header_sentinel(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);
    T_ASSERT_EQ_U(gw_room_dw(&w, 0, GW_ROOM_ID), 0xf423f);
    game_world_free(&w);
    return 0;
}

/* FUN_00585000 part 2: the reciprocal-exit builder.  Room 110110 has a static
 * exit (slot 0) targeting 120110 with return-key 1.  Room 120110's static
 * exits are keyed {2,3} (no key-1 slot), so the builder must ADD a reciprocal
 * exit to 120110: {key=1, target=110110}.  Verify it appears in the built
 * room but not in the pristine source. */
int test_game_world_reciprocal_exits(void)
{
    game_world w;
    T_ASSERT_EQ_I(game_world_build(&w), 0);

    /* locate 120110's record index so we can read the pristine `orig` too. */
    int idx = -1;
    for (uint16_t i = 0; i < w.room_count; i++)
        if (w.rooms[(unsigned)i * GW_ROOM_DWORDS] == 120110) { idx = i; break; }
    T_ASSERT(idx >= 0);

    const uint32_t *built = w.rooms + (unsigned)idx * GW_ROOM_DWORDS;
    const uint32_t *orig  = w.orig  + (unsigned)idx * GW_ROOM_DWORDS;

    int built_has = 0, orig_has = 0;
    for (unsigned k = 0; k < 0x14; k++) {
        if (dw_lo16(built, 7 + k * 3) == 1 && built[8 + k * 3] == 110110) built_has = 1;
        if (dw_lo16(orig,  7 + k * 3) == 1 && orig[8 + k * 3]  == 110110) orig_has  = 1;
    }
    T_ASSERT_EQ_I(orig_has, 0);    /* pristine 120110 has no key-1 -> 110110 exit */
    T_ASSERT_EQ_I(built_has, 1);   /* the reciprocal builder added it             */
    game_world_free(&w);
    return 0;
}
