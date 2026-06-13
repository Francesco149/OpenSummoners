/* game_world.c — see game_world.h. */
#include "game_world.h"
#include "world_tables_data.h"

#include <stdlib.h>
#include <string.h>

/* Low/high 16-bit halves of a dword in the (4-aligned) record buffers. */
static inline uint16_t lo16(const uint32_t *p, unsigned dw) { return ((const uint16_t *)(p + dw))[0]; }
static inline uint16_t hi16(const uint32_t *p, unsigned dw) { return ((const uint16_t *)(p + dw))[1]; }
static inline void set_lo16(uint32_t *p, unsigned dw, uint16_t v) { ((uint16_t *)(p + dw))[0] = v; }
static inline void set_hi16(uint32_t *p, unsigned dw, uint16_t v) { ((uint16_t *)(p + dw))[1] = v; }

/* FUN_00585000: cross-reference one freshly-copied room against the world.
 *   areas  = scene[4][0] (the AREA table)         [the engine's *in_ECX]
 *   orig   = &DAT_006940c8 (pristine room base)    [param_1]
 *   count  = scene[4]+0x54004 (room count)
 *   room   = the room being processed (in scene[4])[param_2]
 *
 * Part 1: find the AREA whose id == room.area and fill any still-zero per-room
 *         default fields (0x43/0x44/0x45/0x50/0x51-lo/0x51-hi) from it.
 * Part 2: scan every OTHER room's 0x14 exit slots for one whose target id ==
 *         this room's id, and add the reciprocal exit slot into this room. */
static void gw_cross_reference(const uint32_t *areas, uint16_t area_count,
                               const uint32_t *orig, uint16_t count,
                               uint32_t *room)
{
    const uint32_t room_id   = room[GW_ROOM_ID];
    const uint32_t room_area = room[GW_ROOM_AREA];

    /* Part 1 — AREA default fill (only the area-keyed defaults; matches
     * FUN_00585000's first arm).  Find area by id; bail if none. */
    const uint32_t *area = NULL;
    for (uint16_t i = 0; i < area_count; i++) {
        const uint32_t *a = areas + (unsigned)i * GW_AREA_DWORDS;
        if (a[GW_AREA_ID] == 0)      /* terminator: no matching area */
            break;
        if (a[GW_AREA_ID] == room_area) { area = a; break; }
    }
    if (area) {
        if (room[0x44] == 0)  room[0x44] = area[0xb];          /* A */
        if (room[0x43] == 0)  room[0x43] = area[0xd];          /* C */
        if (room[0x45] == 0)  room[0x45] = area[0xc];          /* B */
        if (room[0x50] == 0)  room[0x50] = area[0xe];          /* D */
        if (lo16(room, 0x51) == 0)  set_lo16(room, 0x51, lo16(area, 0xf)); /* E */
        if (hi16(room, 0x51) == 0)  set_hi16(room, 0x51, hi16(area, 0xf)); /* F */
    }

    /* Part 2 — reciprocal room-transition exits.  Each room carries 0x14 exit
     * slots starting at dword 7, stride 3 dwords: {key@dw7 (lo16), target
     * room id@dw8, return field@dw9 (lo16), 0@dw9 (hi16)}.  For every other
     * room with an exit pointing back at us, mirror it into our table. */
    for (uint16_t r = 0; r < count; r++) {
        const uint32_t *other = orig + (unsigned)r * GW_ROOM_DWORDS;
        if (other[GW_ROOM_ID] == room_id)
            continue;                            /* skip ourselves */
        for (unsigned k = 0; k < 0x14; k++) {
            const uint32_t *slot = other + 7 + k * 3;   /* dw7+3k of `other`  */
            if (slot[1] != room_id)              /* exit doesn't target us    */
                continue;
            const uint32_t other_id  = other[GW_ROOM_ID]; /* iVar4 */
            const uint16_t other_dw7 = lo16(other, 7 + k * 3); /* (short)*piVar6 */
            const uint16_t key       = lo16(other, 9 + k * 3); /* (short)piVar6[2] */

            /* already have an exit with this key?  scan our 0x14 slots. */
            int have = 0;
            for (unsigned j = 0; j < 0x14; j++)
                if (lo16(room, 7 + j * 3) == key) { have = 1; break; }
            if (have)
                continue;
            /* else fill the first empty slot (key lo16 == 0). */
            for (unsigned j = 0; j < 0x14; j++) {
                if (lo16(room, 7 + j * 3) == 0) {
                    set_lo16(room, 7 + j * 3, key);
                    room[8 + j * 3]      = other_id;
                    set_lo16(room, 9 + j * 3, other_dw7);
                    set_hi16(room, 9 + j * 3, 0);
                    break;
                }
            }
        }
    }
}

int game_world_build(game_world *w)
{
    memset(w, 0, sizeof *w);

    unsigned long alen = 0, rlen = 0;
    const unsigned char *araw = world_tables_area(&alen);
    const unsigned char *rraw = world_tables_room(&rlen);

    /* Count zero-terminated entries (FUN_0059f2c0:122-130).  room_count
     * includes the [0] header sentinel; area_count excludes the terminator. */
    uint16_t rc = 0;
    while ((unsigned)(rc + 1) * GW_ROOM_DWORDS * 4u <= rlen) {
        uint32_t d0;
        memcpy(&d0, rraw + (unsigned)rc * GW_ROOM_DWORDS * 4u, 4);
        if (d0 == 0) break;
        rc++;
    }
    uint16_t ac = 0;
    while ((unsigned)(ac + 1) * GW_AREA_DWORDS * 4u <= alen) {
        uint32_t d0;
        memcpy(&d0, araw + (unsigned)ac * GW_AREA_DWORDS * 4u, 4);
        if (d0 == 0) break;
        ac++;
    }

    w->room_count = rc;
    w->area_count = ac;

    const size_t room_dwords = (size_t)rc * GW_ROOM_DWORDS;
    const size_t area_dwords = (size_t)ac * GW_AREA_DWORDS;

    w->rooms = malloc(room_dwords * sizeof(uint32_t));
    w->orig  = malloc(room_dwords * sizeof(uint32_t));
    w->areas = malloc(area_dwords * sizeof(uint32_t));
    if (!w->rooms || !w->orig || !w->areas) {
        game_world_free(w);
        return -1;
    }

    /* Aligned copies of the .rdata blobs (the raw arrays are byte-aligned). */
    memcpy(w->rooms, rraw, room_dwords * sizeof(uint32_t));
    memcpy(w->orig,  rraw, room_dwords * sizeof(uint32_t));
    memcpy(w->areas, araw, area_dwords * sizeof(uint32_t));

    /* Run the per-room cross-reference, as the engine's copy loop does. */
    for (uint16_t i = 0; i < rc; i++)
        gw_cross_reference(w->areas, ac, w->orig, rc,
                           w->rooms + (size_t)i * GW_ROOM_DWORDS);
    return 0;
}

void game_world_free(game_world *w)
{
    if (!w) return;
    free(w->rooms);
    free(w->orig);
    free(w->areas);
    w->rooms = w->orig = w->areas = NULL;
    w->room_count = w->area_count = 0;
}

uint32_t *game_world_find_room(game_world *w, uint32_t id)
{
    for (uint16_t i = 0; i < w->room_count; i++) {
        uint32_t *r = w->rooms + (size_t)i * GW_ROOM_DWORDS;
        if (r[GW_ROOM_ID] == id)
            return r;
    }
    return NULL;
}

int game_world_room_render_cfg(game_world *w, uint32_t room_key,
                               uint16_t *scene, int *px_p2, int *px_p3)
{
    uint32_t *r = game_world_find_room(w, room_key);
    if (r == NULL)
        return -1;
    if (scene)  *scene  = (uint16_t)r[GW_ROOM_SCENE];
    if (px_p2)  *px_p2  = (int)r[0x44];
    if (px_p3)  *px_p3  = (int)r[0x43];
    return 0;
}
