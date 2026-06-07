/*
 * actor_spawn.c — see actor_spawn.h.  The town CHARACTER-band spawn: the
 * 0x58d460 -> 0x431e30 slice that turns the map's 70000-range object
 * layers into renderable {actor, render-state} pairs, plus the PORT-DEBT
 * visible-code sprite-table stand-in (the lazy 0x40afe0/0x41e600 fill).
 */
#include "actor_spawn.h"

#include <string.h>

/* Layer-header field reads (little-endian, off the 0x3c-byte map_layer.hdr).
 * Offsets from docs/proofs/map-object-layer-format.md (0x58d460's reads). */
static uint32_t hdr_u32(const uint8_t *h, int off)
{
    return (uint32_t)h[off] | ((uint32_t)h[off + 1] << 8) |
           ((uint32_t)h[off + 2] << 16) | ((uint32_t)h[off + 3] << 24);
}
static int32_t hdr_i32(const uint8_t *h, int off) { return (int32_t)hdr_u32(h, off); }

#define HDR_OFF_X    0x04   /* i32 tile-px x (spawn -> world x = x*100) */
#define HDR_OFF_Y    0x08   /* i32 tile-px y (spawn -> world y = y*100) */
#define HDR_OFF_CODE 0x10   /* u32 type code (= actor+0x1d4 behaviour)  */

/* PORT-DEBT(actor-sprite-table): the captured visible-code sprite map (retail
 * town hold, flip 1500 — live +0x48 capture via 0x491ae0).  Stands in for the
 * lazy def-table fill keyed by type+state.  Only codes that DRAW appear here;
 * any other CHARACTER code is an invisible volume (bank 0). */
static const struct {
    uint32_t code;
    uint16_t bank;
    int16_t  frame_base;
    uint32_t layer;
} TOWN_SPRITE_DEFS[] = {
    /* bank 0x16c (res 0x403) is the town-OBJECTS sheet — these are static PROPS
     * (e.g. a barrel, a fountain), NOT people-NPCs (USER-confirmed on the feed:
     * the fountain @0x112e5/176000 + a barrel @0x1129e).  The only person in the
     * CHARACTER band is the animated protagonist 0x1872d (bank 0x175, separate). */
    {0x1129eu, 0x16cu,  1u, 9u},   /* prop, frame 1  (x3 in DATA 1022)   */
    {0x1129fu, 0x16cu,  2u, 9u},   /* prop, frame 2  (x1)                */
    {0x112e5u, 0x16cu, 36u, 10u},  /* prop (the fountain), frame 36, layer 10 (x1) */
};

int actor_spawn_sprite_for_code(uint32_t code, uint16_t *bank,
                                int16_t *frame_base, uint32_t *layer)
{
    for (size_t i = 0; i < sizeof TOWN_SPRITE_DEFS / sizeof TOWN_SPRITE_DEFS[0]; i++) {
        if (TOWN_SPRITE_DEFS[i].code == code) {
            if (bank)       *bank       = TOWN_SPRITE_DEFS[i].bank;
            if (frame_base) *frame_base = TOWN_SPRITE_DEFS[i].frame_base;
            if (layer)      *layer      = TOWN_SPRITE_DEFS[i].layer;
            return 1;
        }
    }
    return 0;
}

int actor_spawn_from_map(actor_spawn_pool *pool, const map_data *md)
{
    if (pool == NULL || md == NULL) return -1;
    memset(pool, 0, sizeof *pool);
    if (md->layers == NULL) return 0;

    for (uint32_t li = 0; li < md->count; li++) {
        const uint8_t *h = md->layers[li].hdr;
        uint32_t code = hdr_u32(h, HDR_OFF_CODE);
        if (code < ACTOR_CODE_CHARACTER_LO || code > ACTOR_CODE_CHARACTER_HI)
            continue;                         /* not a CHARACTER object */

        if (pool->count >= ACTOR_BAND_SLOTS)  /* "Character Object Count Over" */
            return -1;

        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];

        /* 0x431e30 defaults: active, behaviour code, draw layer 9, dir 0,
         * the +0x48 table zeroed (already, via memset).  World = (x,y)*100. */
        a->code  = code;
        a->dir   = 0;
        a->layer = 9;        /* actor+0xfc */

        rs->active  = 1;     /* render-state +0x00 (the *param_1!='\0' gate) */
        rs->world_x = hdr_i32(h, HDR_OFF_X) * 100;   /* +0x04 */
        rs->world_y = hdr_i32(h, HDR_OFF_Y) * 100;   /* +0x08 */
        rs->clip    = NULL;  /* +0x6c — static (32/32 char actors, clip 0)  */

        /* The visible codes get their dir-0 sprite row (+ the draw layer the
         * lazy fill set); every other code stays an invisible volume (bank 0,
         * FUN_0044d160 returns 0). */
        uint16_t bank; int16_t frame_base; uint32_t layer;
        if (actor_spawn_sprite_for_code(code, &bank, &frame_base, &layer)) {
            a->sprite_table[0].bank       = bank;
            a->sprite_table[0].frame_base = frame_base;
            a->layer                      = layer;
        }
    }
    return pool->count;
}
