/*
 * test_actor_spawn.c — host tests for src/actor_spawn.c (the town CHARACTER-band
 * spawn: the FUN_0058d460 -> FUN_00431e30 slice + the visible-code sprite-table
 * stand-in).
 *
 * The map's object layers are the spec input (docs/proofs/map-object-layer-
 * format.md): a 0x3c-byte header per object, type code @+0x10, x @+0x04, y
 * @+0x08.  The expected spawn output is the live census (findings/in-game-intro
 * "The town actor RENDER CENSUS"): CHARACTER codes (70000..79999) activate a
 * slot at (x,y)*100; only 0x1129e/0x1129f/0x112e5 draw (bank 0x16c), every other
 * code is an invisible volume (bank 0).  The render integration test then drives
 * a spawned villager through actor_render_static and asserts the emitted node.
 */
#include "actor_spawn.h"
#include "actor_render.h"
#include "draw_pool.h"
#include "t.h"

#include <string.h>

/* Write a little-endian u32 into a layer header. */
static void hdr_set(uint8_t *h, int off, uint32_t v)
{
    h[off] = v & 0xff; h[off + 1] = (v >> 8) & 0xff;
    h[off + 2] = (v >> 16) & 0xff; h[off + 3] = (v >> 24) & 0xff;
}

/* Fill one map_layer header with (code, x, y); zero the rest. */
static void layer_set(map_layer *L, uint32_t code, int32_t x, int32_t y)
{
    memset(L, 0, sizeof *L);
    hdr_set(L->hdr, 0x10, code);
    hdr_set(L->hdr, 0x04, (uint32_t)x);
    hdr_set(L->hdr, 0x08, (uint32_t)y);
}

/* A resolver that packs (bank, frame) into a non-zero cel handle. */
static uint32_t resolve_pack(uint16_t bank, uint16_t frame, void *ud)
{
    (void)ud;
    return ((uint32_t)bank << 16) | frame;
}

/* ---- the census: char objects spawn, others skip, positions scale ---------- */

int test_actor_spawn_census(void)
{
    /* DATA-1022-shaped sample: 3 visible villagers + 1 invisible volume + a
     * non-character (STRUCTURE) object that must be skipped. */
    map_layer layers[5];
    layer_set(&layers[0], 0x1129eu,  864, 448);   /* villager A -> bank 0x16c f1 */
    layer_set(&layers[1], 0x00ec55u, 100, 100);   /* 60501 STRUCTURE -> skipped  */
    layer_set(&layers[2], 0x112e5u, 1760, 416);   /* villager C -> bank 0x16c f36, layer 10 */
    layer_set(&layers[3], 0x112e6u,  624, 288);   /* invisible CHARACTER volume  */
    layer_set(&layers[4], 0x1129fu, 1184, 448);   /* villager B -> bank 0x16c f2 */

    map_data md;
    memset(&md, 0, sizeof md);
    md.count  = 5;
    md.layers = layers;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 4);  /* 4 CHARACTER, 1 skipped */

    /* slot 0 = villager A (0x1129e): bank 0x16c frame 1 layer 9, world *100. */
    T_ASSERT_EQ_U(pool.actors[0].code, 0x1129eu);
    T_ASSERT_EQ_U(pool.actors[0].sprite_table[0].bank, 0x16cu);
    T_ASSERT_EQ_I(pool.actors[0].sprite_table[0].frame_base, 1);
    T_ASSERT_EQ_U(pool.actors[0].layer, 9u);
    T_ASSERT_EQ_U(pool.actors[0].dir, 0u);
    T_ASSERT_EQ_I(pool.states[0].active, 1);
    T_ASSERT_EQ_I(pool.states[0].world_x, 86400);
    T_ASSERT_EQ_I(pool.states[0].world_y, 44800);
    T_ASSERT_EQ_P(pool.states[0].clip, NULL);

    /* slot 1 = villager C (0x112e5): bank 0x16c frame 36, DRAW LAYER 10. */
    T_ASSERT_EQ_U(pool.actors[1].code, 0x112e5u);
    T_ASSERT_EQ_U(pool.actors[1].sprite_table[0].bank, 0x16cu);
    T_ASSERT_EQ_I(pool.actors[1].sprite_table[0].frame_base, 36);
    T_ASSERT_EQ_U(pool.actors[1].layer, 10u);
    T_ASSERT_EQ_I(pool.states[1].world_x, 176000);
    T_ASSERT_EQ_I(pool.states[1].world_y, 41600);

    /* slot 2 = the invisible volume (0x112e6): spawned + active but bank 0. */
    T_ASSERT_EQ_U(pool.actors[2].code, 0x112e6u);
    T_ASSERT_EQ_U(pool.actors[2].sprite_table[0].bank, 0u);
    T_ASSERT_EQ_I(pool.states[2].active, 1);
    T_ASSERT_EQ_I(pool.states[2].world_x, 62400);

    /* slot 3 = villager B (0x1129f): bank 0x16c frame 2 layer 9. */
    T_ASSERT_EQ_U(pool.actors[3].code, 0x1129fu);
    T_ASSERT_EQ_I(pool.actors[3].sprite_table[0].frame_base, 2);
    T_ASSERT_EQ_U(pool.actors[3].layer, 9u);
    return 0;
}

/* ---- the visible-code sprite map ------------------------------------------- */

int test_actor_spawn_sprite_lookup(void)
{
    uint16_t bank; int16_t fb; uint32_t layer;
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x1129eu, &bank, &fb, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_I(fb, 1); T_ASSERT_EQ_U(layer, 9u);
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112e5u, &bank, &fb, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_I(fb, 36); T_ASSERT_EQ_U(layer, 10u);
    /* an invisible CHARACTER code + a non-character code both miss. */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112e6u, &bank, &fb, &layer), 0);
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x00ec55u, &bank, &fb, &layer), 0);
    return 0;
}

/* ---- the invisible volume renders nothing (self-skips at the renderer) ------ */

int test_actor_spawn_invisible_self_skips(void)
{
    map_layer L;
    layer_set(&L, 0x112e6u, 624, 288);   /* invisible CHARACTER volume */
    map_data md; memset(&md, 0, sizeof md); md.count = 1; md.layers = &L;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 1);

    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    /* bank 0 => FUN_0044d160 returns 0 => no node emitted. */
    T_ASSERT_EQ_I(actor_render_static(&pool.actors[0], &pool.states[0], NULL,
                                      &dp, resolve_pack, NULL), 0);
    draw_pool_free(&dp);
    return 0;
}

/* ---- a spawned villager drives the ckpt-77 render path end-to-end ----------- */

int test_actor_spawn_villager_renders(void)
{
    map_layer L;
    layer_set(&L, 0x1129eu, 864, 448);   /* villager A */
    map_data md; memset(&md, 0, sizeof md); md.count = 1; md.layers = &L;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 1);

    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    T_ASSERT_EQ_I(actor_render_static(&pool.actors[0], &pool.states[0], NULL,
                                      &dp, resolve_pack, NULL), 1);

    /* The node landed in layer 9 with the villager's cel (bank 0x16c, frame 1)
     * at its WORLD position; placement offset 0 (static row, zero offsets);
     * opaque => mode 0 (the keyed actor blit). */
    T_ASSERT_EQ_U(dp.layers[9].count, 1u);
    const draw_node *n = &dp.layers[9].nodes[0];
    T_ASSERT_EQ_U(n->sprite, resolve_pack(0x16cu, 1u, NULL));
    T_ASSERT_EQ_I(n->dst_x, 86400);
    T_ASSERT_EQ_I(n->dst_y, 44800);
    T_ASSERT_EQ_U(n->mode, 0u);
    draw_pool_free(&dp);
    return 0;
}

/* ---- guards: NULL args, empty map, band overflow --------------------------- */

int test_actor_spawn_guards(void)
{
    actor_spawn_pool pool;
    map_data md; memset(&md, 0, sizeof md);

    T_ASSERT_EQ_I(actor_spawn_from_map(NULL, &md), -1);
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, NULL), -1);

    /* no layers => 0 spawned. */
    md.count = 0; md.layers = NULL;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 0);
    T_ASSERT_EQ_I(pool.count, 0);

    /* > ACTOR_BAND_SLOTS character objects => -1 (the "Count Over" abort). */
    static map_layer many[ACTOR_BAND_SLOTS + 4];
    for (int i = 0; i < ACTOR_BAND_SLOTS + 4; i++)
        layer_set(&many[i], 0x112e6u, i, 0);   /* all CHARACTER */
    md.count = ACTOR_BAND_SLOTS + 4; md.layers = many;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), -1);
    return 0;
}
