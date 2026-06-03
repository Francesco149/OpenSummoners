/* game_map.c — see game_map.h. */
#include "game_map.h"

#include <stdlib.h>
#include <string.h>

/* Field-write helpers (byte-offset addressed; the map object is a flat blob). */
static inline void gm_set_dw(game_map *m, unsigned off, uint32_t v)
{
    memcpy(m->buf + off, &v, 4);
}
static inline void gm_set_w(game_map *m, unsigned off, uint16_t v)
{
    memcpy(m->buf + off, &v, 2);
}
static inline void actor_set_dw(uint8_t *slot, unsigned off, uint32_t v)
{
    memcpy(slot + off, &v, 4);
}
static inline void actor_set_w(uint8_t *slot, unsigned off, uint16_t v)
{
    memcpy(slot + off, &v, 2);
}

/* FUN_00560e60 (42 B): zero an actor sub-object's bookkeeping fields.  The
 * decomp indexes in dwords from the slot base: [0],[1],[2],[0x271],[0x27c],
 * [0x27d],[0x27e] dwords + a u16 at [0x272].  On our calloc'd slots these are
 * already zero, but port it verbatim for fidelity. */
static void actor_slot_init(uint8_t *slot)
{
    actor_set_dw(slot, 0x000, 0);          /* in_ECX[0]    */
    actor_set_dw(slot, 0x004, 0);          /* in_ECX[1]    */
    actor_set_dw(slot, 0x008, 0);          /* in_ECX[2]    */
    actor_set_dw(slot, 0x271 * 4, 0);      /* in_ECX[0x271]*/
    actor_set_w (slot, 0x272 * 4, 0);      /* (u16)in_ECX[0x272] */
    actor_set_dw(slot, 0x27c * 4, 0);      /* in_ECX[0x27c]*/
    actor_set_dw(slot, 0x27d * 4, 0);      /* in_ECX[0x27d]*/
    actor_set_dw(slot, 0x27e * 4, 0);      /* in_ECX[0x27e]*/
}

/* FUN_004c5350: the per-map room-key resolver — a jump table on map+0x4104.
 * We port the pure state writes of the reachable arms; the opaque sprite/anim
 * sub-calls (0x408dc0/0x413b20/0x4c5e00/0x412db0/…) are skipped
 * (they touch the sprite registry, not these map fields).  Runs the *in_ECX==0
 * (fresh-entry) branch, the only one on our path. */
static void map_resolve_room_key(game_map *m)
{
    uint32_t map_id = gm_dw(m, GM_MAPID);

    if (map_id == 1) {
        /* map==1 arm (4c5350:15-71): no +0x4024 write — that room key comes
         * from a different mechanism; only the pure field writes here. */
        gm_set_dw(m, 0x401c, 1);
        gm_set_dw(m, 0x40d0, 0);
    } else if (map_id == 0x3f2) {
        /* map==0x3f2 arm (4c5350:72-109) — the opening town. */
        gm_set_dw(m, 0x401c, 0);
        /* +0x4014 ramps toward the ceiling +0x4020 by +5 (clamped).  Under our
         * zero-init both are 0, so this is inert (see game_map.h port note). */
        int32_t ceil = (int32_t)gm_dw(m, 0x4020);
        int32_t cur  = (int32_t)gm_dw(m, 0x4014);
        if (cur < ceil) {
            int32_t next = cur + 5;
            gm_set_dw(m, 0x4014, (uint32_t)next);
            if (next > ceil) next = ceil;
            gm_set_dw(m, 0x4014, (uint32_t)next);
        }
        gm_set_dw(m, GM_ROOMKEY, 0x334be);   /* room 210110 "Town of Tonkiness" */
        gm_set_dw(m, GM_SPAWN_A, 0x65);
        gm_set_dw(m, GM_SPAWN_B, 1);
        gm_set_dw(m, 0x40d0, 0);
    }
    /* map==0x3fc arm (4c5350:111-160) writes +0x4024=0x30db3 but is gated by
     * unported global save-flag state (0x4c57f0 lookups), so it is not
     * reproduced here.  Other map ids fall through with no key write. */
}

int game_map_build(game_map *m, uint32_t map_id, uint32_t tick)
{
    memset(m, 0, sizeof *m);

    m->buf = calloc(1, GM_OBJ_SIZE);
    if (!m->buf) { game_map_free(m); return -1; }
    for (int i = 0; i < GM_ACTOR_N; i++) {
        m->actors[i] = calloc(1, GM_ACTOR_SIZE);
        if (!m->actors[i]) { game_map_free(m); return -1; }
    }

    /* ---- the fresh-entry arm (FUN_0059f2c0:160-218, in_stack_0000eb2c==0) ---- */

    /* 8 actor slots: stamp slot index at +0xa0c, init bookkeeping, mark active.
     * (Engine: slot ptr = *(map+0x4030+4*i); here we keep them in actors[].) */
    for (int i = 0; i < GM_ACTOR_N; i++) {
        actor_set_w(m->actors[i], 0xa0c, (uint16_t)i);
        actor_slot_init(m->actors[i]);
        gm_set_dw(m, GM_ACTIVE + 4u * (unsigned)i, 1);   /* map+0x4084+4*i = 1 */
    }

    gm_set_dw(m, 0x40a4, 1);
    gm_set_w (m, 0x40a8, 0);
    gm_set_dw(m, 0x4018, 1);
    gm_set_dw(m, 0x200c, 0);
    gm_set_w (m, 0x4010, 0);
    gm_set_dw(m, 0x4014, 0);
    gm_set_dw(m, 0x401c, 0);
    gm_set_dw(m, 0x4050, 0);
    gm_set_dw(m, GM_STATE54, 3);      /* map+0x4054 = 3 */
    gm_set_dw(m, 0x4058, 0);
    /* map+0x40ac = scene[5] (the 0x7808 world buffer) — not modeled here; 0. */
    gm_set_dw(m, 0x40ac, 0);
    gm_set_w (m, 0x2008, 0);
    gm_set_dw(m, 0x40b0, 0);
    gm_set_dw(m, 0x40b4, 0);
    gm_set_dw(m, 0x40b8, 0);
    gm_set_dw(m, 0x40bc, 0);
    gm_set_dw(m, 0x40c0, 0);
    gm_set_dw(m, 0x40c4, 0);
    gm_set_dw(m, 0x40c8, 0);
    gm_set_dw(m, 0x40cc, 0);
    gm_set_dw(m, 0x40dc, 0);
    gm_set_dw(m, 0x40e4, 0);
    /* 0x5612b0() — opaque sub-init; skipped. */
    gm_set_dw(m, GM_MAPID, 0);
    /* 0x5611d0() — opaque sub-init; skipped. */
    /* The +0x405c..+0x4064 header: a dword zero then a run of u16 writes, the
     * last dword write at +0x4064 overwriting the +0x4064/+0x4066 u16s. */
    gm_set_dw(m, 0x405c, 0);
    gm_set_w (m, 0x4060, 0);
    gm_set_w (m, 0x405c, 0);
    gm_set_w (m, 0x4064, 1);
    gm_set_w (m, 0x405e, 0);
    gm_set_w (m, 0x4060, 0);
    gm_set_w (m, 0x4062, 1);
    gm_set_dw(m, 0x4064, 0);
    /* 3 {1,0} dword pairs at +0x4108 (fills the object's tail to +0x4120). */
    for (unsigned k = 0; k < 3; k++) {
        gm_set_dw(m, 0x4108 + k * 8u + 0, 1);
        gm_set_dw(m, 0x4108 + k * 8u + 4, 0);
    }
    gm_set_dw(m, GM_TICK, tick);      /* retail: GetTickCount() */
    gm_set_dw(m, 0x406c, 0);

    /* map+0x4104 = the map id (FUN_0059f2c0:215), THEN FUN_004c5350 reads it. */
    gm_set_dw(m, GM_MAPID, map_id);
    map_resolve_room_key(m);
    /* 0x4e59a0() — opaque sub-init; skipped. */

    /* LAB_0059fb8a default (FUN_0059f2c0:378-381): a 0 map id defaults to 0x3f2.
     * (Applied after FUN_004c5350 in retail; our fresh path always passes 0x3f2,
     * so this only matters if a caller passes 0 — in which case the key is not
     * set by the resolver above.  Kept for faithful default behaviour.) */
    if (map_id == 0)
        gm_set_dw(m, GM_MAPID, 0x3f2);

    return 0;
}

void game_map_free(game_map *m)
{
    if (!m) return;
    free(m->buf);
    m->buf = NULL;
    for (int i = 0; i < GM_ACTOR_N; i++) {
        free(m->actors[i]);
        m->actors[i] = NULL;
    }
}

uint32_t *game_map_active_room(const game_map *m, game_world *w)
{
    return game_world_find_room(w, game_map_room_key(m));
}
