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
 * a spawned prop through actor_render_static and asserts the emitted node.
 */
#include "actor_spawn.h"
#include "actor_render.h"
#include "character.h"
#include "butterfly.h"
#include "ambient.h"
#include "draw_pool.h"
#include "rng.h"
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
    /* DATA-1022-shaped sample: 3 visible props + 1 invisible volume + a
     * non-character (STRUCTURE) object that must be skipped. */
    map_layer layers[5];
    layer_set(&layers[0], 0x1129eu,  864, 448);   /* prop A -> bank 0x16c f1 */
    hdr_set(layers[0].hdr, 0x18, 1);              /* variant 1 -> frame_base 1 */
    layer_set(&layers[1], 0x00ec55u, 100, 100);   /* 60501 STRUCTURE -> skipped  */
    layer_set(&layers[2], 0x112e5u, 1760, 416);   /* prop C -> bank 0x16c f36, layer 10 */
    hdr_set(layers[2].hdr, 0x18, 36);             /* variant 36 -> frame_base 36 */
    layer_set(&layers[3], 0x112e6u,  624, 288);   /* invisible CHARACTER volume  */
    layer_set(&layers[4], 0x1129fu, 1184, 448);   /* prop B -> bank 0x16c f2 */
    hdr_set(layers[4].hdr, 0x18, 2);              /* variant 2 -> frame_base 2 (frame_base now = map variant, ckpt 183) */

    map_data md;
    memset(&md, 0, sizeof md);
    md.count  = 5;
    md.layers = layers;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 4);  /* 4 CHARACTER, 1 skipped */

    /* slot 0 = prop A (0x1129e): bank 0x16c frame 1 layer 9, world *100. */
    T_ASSERT_EQ_U(pool.actors[0].code, 0x1129eu);
    T_ASSERT_EQ_U(pool.actors[0].sprite_table[0].bank, 0x16cu);
    T_ASSERT_EQ_I(pool.actors[0].sprite_table[0].frame_base, 1);
    T_ASSERT_EQ_U(pool.actors[0].layer, 9u);
    T_ASSERT_EQ_U(pool.actors[0].dir, 0u);
    T_ASSERT_EQ_I(pool.states[0].active, 1);
    T_ASSERT_EQ_I(pool.states[0].world_x, 86400);
    T_ASSERT_EQ_I(pool.states[0].world_y, 44800);
    T_ASSERT_EQ_P(pool.states[0].clip, NULL);

    /* slot 1 = prop C (0x112e5): bank 0x16c frame 36, DRAW LAYER 10. */
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

    /* slot 3 = prop B (0x1129f): bank 0x16c frame 2 layer 9. */
    T_ASSERT_EQ_U(pool.actors[3].code, 0x1129fu);
    T_ASSERT_EQ_I(pool.actors[3].sprite_table[0].frame_base, 2);
    T_ASSERT_EQ_U(pool.actors[3].layer, 9u);
    return 0;
}

/* ---- the visible-code sprite map ------------------------------------------- */

int test_actor_spawn_sprite_lookup(void)
{
    uint16_t bank; uint32_t layer;
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x1129eu, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_U(layer, 9u);
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112e5u, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_U(layer, 10u);
    /* errands map-driven CHARACTER furniture (ckpt 183): bank + layer RE'd from 0x431e30. */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112dcu, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_U(layer, 5u);   /* shelf-back unit res1027 -> L5 */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112cfu, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16fu); T_ASSERT_EQ_U(layer, 9u);   /* wall shelf res1023 -> L9 */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x1124cu, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x156u); T_ASSERT_EQ_U(layer, 9u);   /* res1022 prop -> L9 */
    /* the ANIM props clock/pot (ckpt 184): res1026 bank 0x16b -> L9 (clip added separately). */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112d9u, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16bu); T_ASSERT_EQ_U(layer, 9u);   /* pendulum clock */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112dau, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16bu); T_ASSERT_EQ_U(layer, 9u);   /* cooking pot     */
    /* the additive fireplace FIRE (ckpt 185): res1034 bank 0x1a3 -> LAYER 6 (0x438610(6)). */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112e4u, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x1a3u); T_ASSERT_EQ_U(layer, 6u);   /* fire (additive, node_alpha 14) */
    /* an invisible volume + a non-character both miss. */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112e6u, &bank, &layer), 0);
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x00ec55u, &bank, &layer), 0);
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

/* ---- a spawned prop drives the ckpt-77 render path end-to-end ----------- */

int test_actor_spawn_prop_renders(void)
{
    map_layer L;
    layer_set(&L, 0x1129eu, 864, 448);   /* prop A */
    hdr_set(L.hdr, 0x18, 1);             /* variant 1 -> frame_base 1 (ckpt 183) */
    map_data md; memset(&md, 0, sizeof md); md.count = 1; md.layers = &L;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 1);

    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    T_ASSERT_EQ_I(actor_render_static(&pool.actors[0], &pool.states[0], NULL,
                                      &dp, resolve_pack, NULL), 1);

    /* The node landed in layer 9 with the prop's cel (bank 0x16c, frame 1)
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

/* ---- the protagonist (the cutscene spawn 0x431e30 case-0x1872d) ----------- */

int test_actor_spawn_protagonist(void)
{
    actor_spawn_pool pool;
    memset(&pool, 0, sizeof pool);

    int slot = actor_spawn_protagonist(&pool, 54400, 32000);
    T_ASSERT_EQ_I(slot, 0);
    T_ASSERT_EQ_I(pool.count, 1);

    const actor *a = &pool.actors[0];
    T_ASSERT_EQ_U(a->code, ACTOR_CODE_PROTAGONIST);
    T_ASSERT_EQ_U(a->dir, 0u);
    T_ASSERT_EQ_U(a->layer, 9u);
    T_ASSERT_EQ_U(a->sprite_table[0].bank, (uint16_t)ACTOR_PROT_SPRITE_BANK);
    T_ASSERT_EQ_I(a->sprite_table[0].frame_base, 0);

    const actor_render_state *rs = &pool.states[0];
    T_ASSERT_EQ_U(rs->active, 1u);
    T_ASSERT_EQ_I(rs->world_x, 54400);
    T_ASSERT_EQ_I(rs->world_y, 32000);
    T_ASSERT_EQ_I(rs->facing, ACTOR_PROT_FACING);
    /* the wagon's horses clip (0x671c48: base 2, 4 frames, looping). */
    T_ASSERT(rs->clip != NULL);
    T_ASSERT_EQ_I(rs->clip->base_sprite, 2);
    T_ASSERT_EQ_U(rs->clip->frame_count, 4u);

    /* It renders through the case-0x1872d arm: 3 cels on layer 9, all bank
     * 0x175 — the two fixed wagon cels (frames 0/1) + the HORSES body cel
     * (frame_base 0 + base_sprite 2 + delta[0] 0 = sprite 2). */
    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    T_ASSERT_EQ_I(actor_render_protagonist(a, rs, NULL, &dp, resolve_pack, NULL), 3);
    T_ASSERT_EQ_U(dp.layers[9].count, 3u);
    T_ASSERT_EQ_U(dp.layers[9].nodes[0].sprite, resolve_pack(ACTOR_PROT_SPRITE_BANK, 0u, NULL));
    T_ASSERT_EQ_U(dp.layers[9].nodes[1].sprite, resolve_pack(ACTOR_PROT_SPRITE_BANK, 1u, NULL));
    T_ASSERT_EQ_U(dp.layers[9].nodes[2].sprite, resolve_pack(ACTOR_PROT_SPRITE_BANK, 2u, NULL));
    T_ASSERT_EQ_I(dp.layers[9].nodes[2].dst_x, 54400);
    draw_pool_free(&dp);

    /* NULL pool guard. */
    T_ASSERT_EQ_I(actor_spawn_protagonist(NULL, 0, 0), -1);
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

/* ---- actor_pool_update: the per-sim-tick trot (0x46cd70 -> 0x54f980 stepper) */

/* Render the protagonist and return its animated body cel (node[2]) sprite. */
static uint32_t prot_body_cel(const actor_spawn_pool *pool, int slot)
{
    draw_pool dp; draw_pool_init(&dp);
    actor_render_protagonist(&pool->actors[slot], &pool->states[slot], NULL, &dp,
                             resolve_pack, NULL);
    uint32_t cel = dp.layers[9].nodes[2].sprite;
    draw_pool_free(&dp);
    return cel;
}

/* The protagonist's looping WAGON_CLIP (base 2, 4 frames, dur 18) trots when the
 * pool is updated once per sim-tick: rs.frame cycles 0->1->2->3->0 every 18
 * ticks, so the rendered body cel cycles sprite 2->3->4->5.  A co-spawned static
 * actor (clip NULL) is left frozen, and actor_pool_update reports only the 1
 * animated actor advanced. */
int test_actor_pool_update_trots(void)
{
    actor_spawn_pool pool;
    memset(&pool, 0, sizeof pool);

    int pslot = actor_spawn_protagonist(&pool, 54400, 32000);
    T_ASSERT_EQ_I(pslot, 0);

    /* a co-spawned STATIC actor (clip NULL, active) — must not advance. */
    int sslot = pool.count++;
    pool.actors[sslot].code = 0x1129eu;
    pool.actors[sslot].dir  = 0;
    pool.actors[sslot].layer = 9;
    pool.actors[sslot].sprite_table[0].bank = 0x16cu;
    pool.states[sslot].active = 1;
    pool.states[sslot].clip   = NULL;

    actor_render_state *prs = &pool.states[pslot];
    T_ASSERT_EQ_U(prs->frame, 0u);

    /* frame 0 -> body sprite 2 (base 2 + delta[0] 0). */
    T_ASSERT_EQ_U(prot_body_cel(&pool, pslot),
                  resolve_pack(ACTOR_PROT_SPRITE_BANK, 2u, NULL));

    /* 17 ticks: still frame 0 (timer climbing), only the protagonist advanced. */
    for (int t = 0; t < 17; t++)
        T_ASSERT_EQ_I(actor_pool_update(&pool), 1);
    T_ASSERT_EQ_U(prs->frame, 0u);
    T_ASSERT_EQ_U(prs->timer, 17u);

    /* tick 18 -> frame 1 -> body sprite 3. */
    actor_pool_update(&pool);
    T_ASSERT_EQ_U(prs->frame, 1u);
    T_ASSERT_EQ_U(prs->timer, 0u);
    T_ASSERT_EQ_U(prot_body_cel(&pool, pslot),
                  resolve_pack(ACTOR_PROT_SPRITE_BANK, 3u, NULL));

    /* the static actor never moved. */
    T_ASSERT_EQ_U(pool.states[sslot].frame, 0u);
    T_ASSERT_EQ_U(pool.states[sslot].timer, 0u);

    /* frames 2, 3 (ticks 36, 54) -> body sprites 4, 5. */
    for (int t = 0; t < 18; t++) actor_pool_update(&pool);
    T_ASSERT_EQ_U(prs->frame, 2u);
    T_ASSERT_EQ_U(prot_body_cel(&pool, pslot),
                  resolve_pack(ACTOR_PROT_SPRITE_BANK, 4u, NULL));
    for (int t = 0; t < 18; t++) actor_pool_update(&pool);
    T_ASSERT_EQ_U(prs->frame, 3u);
    T_ASSERT_EQ_U(prot_body_cel(&pool, pslot),
                  resolve_pack(ACTOR_PROT_SPRITE_BANK, 5u, NULL));

    /* tick 72 -> loops back to frame 0 (loop_to 0) -> body sprite 2 again. */
    for (int t = 0; t < 18; t++) actor_pool_update(&pool);
    T_ASSERT_EQ_U(prs->frame, 0u);
    T_ASSERT_EQ_U(prot_body_cel(&pool, pslot),
                  resolve_pack(ACTOR_PROT_SPRITE_BANK, 2u, NULL));

    /* NULL guard. */
    T_ASSERT_EQ_I(actor_pool_update(NULL), 0);
    return 0;
}

/* ---- the STRUCTURE band: map-driven scenery (tree/hedge/deco), quirk #84 ---- */

/* Fill a STRUCTURE layer header: (code, x, y) + variant@+0x18 + fgflag@+0x30. */
static void struct_layer_set(map_layer *L, uint32_t code, int32_t x, int32_t y,
                             uint16_t variant, int32_t fgflag)
{
    layer_set(L, code, x, y);
    L->hdr[0x18] = (uint8_t)(variant & 0xff);
    L->hdr[0x19] = (uint8_t)((variant >> 8) & 0xff);
    hdr_set(L->hdr, 0x30, (uint32_t)fgflag);
}

int test_actor_spawn_struct(void)
{
    /* DATA-1022-shaped sample: the tree + a fg hedge + a bg deco, plus a
     * CHARACTER object (skipped) and an unknown structure code (skipped). */
    map_layer layers[5];
    struct_layer_set(&layers[0], 0xec55u, 1776, 192,  0, 0);  /* tree  -> bank 0x15f, fb 0,  layer 8  */
    struct_layer_set(&layers[1], 0x112e5u, 100, 100,  0, 0);  /* CHARACTER -> skipped               */
    struct_layer_set(&layers[2], 0xec60u, 1696, 476,  5, 1);  /* hedge -> bank 0x164, fb 5, layer 15 */
    struct_layer_set(&layers[3], 0xec6au, 1288, 400, 16, 0);  /* deco  -> bank 0x16c, fb 16, layer 8 */
    struct_layer_set(&layers[4], 0x6f00fu, 50, 50,    0, 0);  /* unknown structure code -> skipped   */

    map_data md;
    memset(&md, 0, sizeof md);
    md.count  = 5;
    md.layers = layers;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_struct_from_map(&pool, &md), 3);  /* 3 structures */

    /* slot 0 = tree (0xec55): bank 0x15f, fb 0, layer 8, world (177600,19200). */
    T_ASSERT_EQ_U(pool.actors[0].sprite_table[0].bank, 0x15fu);
    T_ASSERT_EQ_I(pool.actors[0].sprite_table[0].frame_base, 0);
    T_ASSERT_EQ_U(pool.actors[0].layer, 8u);
    T_ASSERT_EQ_I(pool.states[0].world_x, 177600);
    T_ASSERT_EQ_I(pool.states[0].world_y, 19200);
    T_ASSERT(pool.states[0].clip == NULL);

    /* slot 1 = hedge (0xec60): bank 0x164, fb = variant 5, layer 15 (fgflag 1). */
    T_ASSERT_EQ_U(pool.actors[1].sprite_table[0].bank, 0x164u);
    T_ASSERT_EQ_I(pool.actors[1].sprite_table[0].frame_base, 5);
    T_ASSERT_EQ_U(pool.actors[1].layer, 15u);

    /* slot 2 = deco (0xec6a): bank 0x16c, fb = variant 16, layer 8 (fgflag 0). */
    T_ASSERT_EQ_U(pool.actors[2].sprite_table[0].bank, 0x16cu);
    T_ASSERT_EQ_I(pool.actors[2].sprite_table[0].frame_base, 16);
    T_ASSERT_EQ_U(pool.actors[2].layer, 8u);

    /* the bank lookup itself. */
    uint16_t b = 0;
    T_ASSERT_EQ_I(actor_spawn_struct_bank_for_code(0xec55u, &b), 1);
    T_ASSERT_EQ_U(b, 0x15fu);
    T_ASSERT_EQ_I(actor_spawn_struct_bank_for_code(0x6f00fu, &b), 0);

    /* a structure renders through actor_render_static into its layer (the hedge
     * at layer 15) — cel = (bank 0x164, frame 5), world from the render-state. */
    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    int e = actor_render_static(&pool.actors[1], &pool.states[1], NULL, &dp,
                                resolve_pack, NULL);
    T_ASSERT_EQ_I(e, 1);
    T_ASSERT_EQ_U(dp.layers[15].count, 1u);
    T_ASSERT_EQ_U(dp.layers[15].nodes[0].sprite, resolve_pack(0x164u, 5u, NULL));
    T_ASSERT_EQ_I(dp.layers[15].nodes[0].dst_x, 169600);
    draw_pool_free(&dp);

    /* NULL guard. */
    T_ASSERT_EQ_I(actor_spawn_struct_from_map(NULL, &md), -1);
    return 0;
}

/* ---- the EFFECT band: map-driven townsfolk (0x41f200 / 0x493ba0), quirk #84 ---- */

int test_actor_spawn_effect(void)
{
    /* DATA-1022-shaped sample: two standing townsfolk + 0xe29a (the BUTTERFLY,
     * now spawned) + a STRUCTURE object (out of range -> skipped) + an unknown
     * EFFECT code (not in the def table -> skipped). */
    map_layer layers[5];
    layer_set(&layers[0], 0xc3e6u,  208, 384);   /* townsperson -> bank 0xe5, dst (-30,-32), layer 13 */
    layer_set(&layers[1], 0xe29au, 1056, 448);   /* BUTTERFLY -> bank 0x146, dst (0,0), layer 12       */
    layer_set(&layers[2], 0xc404u, 1808, 416);   /* townsperson -> bank 0xf9, dst (-30,-20)           */
    layer_set(&layers[3], 0xec55u,  100, 100);   /* STRUCTURE   -> out of EFFECT range -> skipped      */
    layer_set(&layers[4], 0xc999u,  50,  50);    /* unknown EFFECT code -> not in def table -> skipped */
    /* The map VARIANT field (+0x18): the BUTTERFLY (0xe29a) takes it as its sprite
     * frame_base (the per-instance base direction; 0x426d70(0,0x146,param_7)); the
     * standing townsfolk IGNORE it (their install hardcodes frame_base 0).  Set both
     * to non-zero to prove the conditional (ckpt 139, findings/butterfly-direction-sprite.md). */
    layers[0].hdr[0x18] = 5;                      /* townsperson variant -> MUST be ignored (fb 0)     */
    layers[1].hdr[0x18] = 8;                      /* butterfly  variant  -> becomes frame_base (base 2)*/

    map_data md;
    memset(&md, 0, sizeof md);
    md.count  = 5;
    md.layers = layers;

    /* Replicate the engine LCG (the same draw model the spawn replays) from a
     * pinned seed, so the idle-PHASE assertions lock in the per-object draw count
     * + order (engine-quirk #86): 0xc3e6 draws 8 prefix then 2 phase; 0xe29a
     * draws 8+5(0x427670) then 2; 0xc404 draws 8 then 2; 0xc999 (unknown effect,
     * not spawned) still draws 8+2.  Slot 0's phase is the 1st pair; slot 1's is
     * after 0xc3e6 (10) + 0xe29a (15) = 25 draws.  (The structure 0xec55 draws
     * nothing — it is out of the EFFECT range.) */
    uint32_t s = 0x4f5347u;
#define RREF() ((s = s * 0x343fdu + 0x269ec3u), (uint32_t)((s >> 16) & 0x7fffu))
    for (int k = 0; k < 8; k++) (void)RREF();          /* 0xc3e6 prefix */
    uint16_t ef0 = (uint16_t)((RREF() * 20u) >> 15);   /* slot 0 frame (idle clip 20)  */
    uint16_t et0 = (uint16_t)((RREF() * 14u) >> 15);   /* slot 0 timer (idle clip 14)  */
    for (int k = 0; k < 13; k++) (void)RREF();          /* 0xe29a 8+5 prefix           */
    uint16_t efb = (uint16_t)((RREF() * 3u)  >> 15);   /* slot 1 butterfly frame (clip 3) */
    uint16_t etb = (uint16_t)((RREF() * 4u)  >> 15);   /* slot 1 butterfly timer (clip 4) */
    for (int k = 0; k < 8; k++) (void)RREF();          /* 0xc404 prefix */
    uint16_t ef1 = (uint16_t)((RREF() * 20u) >> 15);   /* slot 2 frame  */
    uint16_t et1 = (uint16_t)((RREF() * 14u) >> 15);   /* slot 2 timer  */
#undef RREF

    rng_srand(0x4f5347u);   /* same pinned seed -> the spawn replay aligns */
    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_effect_from_map(&pool, &md, NULL), 3);  /* 2 townsfolk + 1 butterfly */

    /* slot 0 = 0xc3e6: bank 0xe5, fb 0, layer 13, dst (-30,-32), idle clip set,
     * RNG start phase (frame in [0,20), timer in [0,14)).
     * world = (map - dst) * 100 = ((208,384) - (-30,-32)) * 100 = (23800,41600)
     * — matches the live census rs_x/rs_y exactly. */
    T_ASSERT_EQ_U(pool.actors[0].sprite_table[0].bank, 0xe5u);
    T_ASSERT_EQ_I(pool.actors[0].sprite_table[0].frame_base, 0);  /* ignores variant 5 (not a butterfly) */
    T_ASSERT_EQ_U(pool.actors[0].layer, 13u);
    T_ASSERT_EQ_I(pool.states[0].world_x, 23800);
    T_ASSERT_EQ_I(pool.states[0].world_y, 41600);
    T_ASSERT_EQ_I(pool.states[0].dst_base_x, -30);
    T_ASSERT_EQ_I(pool.states[0].dst_base_y, -32);
    T_ASSERT(pool.states[0].clip != NULL);
    T_ASSERT_EQ_U(pool.states[0].clip->frame_count, 20u);  /* the idle clip 0x6290e0 */
    T_ASSERT_EQ_U(pool.states[0].clip->frame_dur, 14u);
    T_ASSERT_EQ_U(pool.states[0].frame, ef0);              /* 0x426ec0 start frame */
    T_ASSERT_EQ_U(pool.states[0].timer, et0);              /* 0x426ec0 start timer */
    T_ASSERT(pool.states[0].frame < 20u);
    T_ASSERT(pool.states[0].timer < 14u);

    /* slot 1 = 0xe29a the BUTTERFLY: bank 0x146, dst (0,0), layer 12, the 3-frame
     * flap clip, world = (1056,448)*100 = (105600,44800).  Start phase scaled by
     * the butterfly clip's count/dur (frame in [0,3), timer in [0,4)). */
    T_ASSERT_EQ_U(pool.actors[1].code, 0xe29au);
    T_ASSERT_EQ_U(pool.actors[1].sprite_table[0].bank, 0x146u);
    /* the butterfly takes frame_base from the map VARIANT (+0x18=8); the cel then
     * renders frame_base + 16*(facing==3) + flap (live-read ckpt 139). */
    T_ASSERT_EQ_I(pool.actors[1].sprite_table[0].frame_base, 8);
    T_ASSERT_EQ_U(pool.actors[1].layer, 12u);
    T_ASSERT_EQ_I(pool.states[1].world_x, 105600);
    T_ASSERT_EQ_I(pool.states[1].world_y, 44800);
    T_ASSERT(pool.states[1].clip != NULL);
    T_ASSERT_EQ_U(pool.states[1].clip->frame_count, 3u);   /* butterfly flap 0x65ddf0 */
    T_ASSERT_EQ_U(pool.states[1].clip->frame_dur, 4u);
    T_ASSERT_EQ_U(pool.states[1].frame, efb);
    T_ASSERT_EQ_U(pool.states[1].timer, etb);
    T_ASSERT(pool.states[1].frame < 3u);
    T_ASSERT(pool.states[1].timer < 4u);

    /* slot 2 = 0xc404: bank 0xf9, dst (-30,-20), world ((1808,416)-(-30,-20))*100
     * = (183800,43600) — the live census value.  Its idle phase reflects the
     * 0xe29a butterfly's 15 draws consumed in between. */
    T_ASSERT_EQ_U(pool.actors[2].sprite_table[0].bank, 0xf9u);
    T_ASSERT_EQ_I(pool.states[2].world_x, 183800);
    T_ASSERT_EQ_I(pool.states[2].world_y, 43600);
    T_ASSERT_EQ_I(pool.states[2].dst_base_x, -30);
    T_ASSERT_EQ_I(pool.states[2].dst_base_y, -20);
    T_ASSERT(pool.states[2].clip != NULL);
    T_ASSERT_EQ_U(pool.states[2].frame, ef1);
    T_ASSERT_EQ_U(pool.states[2].timer, et1);

    /* the def lookup itself. */
    uint16_t b = 0; int16_t dx = 0, dy = 0; uint32_t ly = 0; int16_t fc = 0, fl = 0;
    T_ASSERT_EQ_I(actor_spawn_effect_def_for_code(0xe2a5u, &b, &dx, &dy, &ly, &fc, &fl), 1);
    T_ASSERT_EQ_U(b, 0x14cu);
    T_ASSERT_EQ_I(dx, -16);
    T_ASSERT_EQ_I(dy, -32);
    T_ASSERT_EQ_U(ly, 13u);
    T_ASSERT_EQ_I(fc, 1);                 /* 0xe2a5 faces normal (not mirrored) */
    /* a mirrored townsperson: facing 3 + the captured flip (frames/dir). */
    T_ASSERT_EQ_I(actor_spawn_effect_def_for_code(0xc3beu, &b, &dx, &dy, &ly, &fc, &fl), 1);
    T_ASSERT_EQ_I(fc, 3);
    T_ASSERT_EQ_I(fl, 16);
    /* 0xe29a is the BUTTERFLY (bank 0x146, dst 0/0, layer 12) — corrected ckpt 96
     * from the "wanderer" mis-ID; it now resolves + spawns. */
    T_ASSERT_EQ_I(actor_spawn_effect_def_for_code(0xe29au, &b, &dx, &dy, &ly, &fc, &fl), 1);
    T_ASSERT_EQ_U(b, 0x146u);
    T_ASSERT_EQ_I(dx, 0);
    T_ASSERT_EQ_I(dy, 0);
    T_ASSERT_EQ_U(ly, 12u);
    T_ASSERT_EQ_I(fl, 16);                /* DAT_008a8440[0x146]=16: the facing==3
                                           * mirror-cel offset (live-read ckpt 139) */
    T_ASSERT_EQ_I(actor_spawn_effect_def_for_code(0xc999u, &b, &dx, &dy, &ly, NULL, NULL), 0); /* unknown  */

    /* the flip table fills the mirrored villager banks (stand-in for DAT_008a8440). */
    int16_t flip_tbl[1024] = {0};
    int filled = actor_spawn_effect_fill_flip_table(flip_tbl, 1024);
    T_ASSERT_EQ_I(filled, 12);            /* 11 villager banks + butterfly 0x146 */
    T_ASSERT_EQ_I(flip_tbl[0xd4], 16);   /* 0xc3be bank 0xd4 -> 16 frames/dir  */
    T_ASSERT_EQ_I(flip_tbl[0xe5], 4);    /* 0xc3e6 bank 0xe5 -> 4              */

    /* a townsperson renders through actor_render_static into layer 13.  actor[0]
     * is 0xc3e6 (bank 0xe5) — a MIRRORED townsperson (facing 3).  With a NULL
     * flip table the mirror reads flip 0; the world pos projects to 23800 and the
     * render adds the dst anchor (-30) back.  Pin the clip frame to 0 here so this
     * sub-test isolates the FACING-mirror cel math from the RNG idle phase
     * (verified above): at frame 0 the idle clip's sprite delta is 0
     * (frame_delta[0] == 0), so the cel reduces to frame_base + frame_off. */
    pool.states[0].frame = 0;
    draw_pool dp;
    T_ASSERT_EQ_I(draw_pool_init(&dp), 0);
    int e = actor_render_static(&pool.actors[0], &pool.states[0], NULL, &dp,
                                resolve_pack, NULL);
    T_ASSERT_EQ_I(e, 1);
    T_ASSERT_EQ_U(dp.layers[13].count, 1u);
    T_ASSERT_EQ_U(dp.layers[13].nodes[0].sprite, resolve_pack(0xe5u, 0u, NULL));
    T_ASSERT_EQ_I(dp.layers[13].nodes[0].dst_x, 23800);
    T_ASSERT_EQ_I((int32_t)dp.layers[13].nodes[0].param6, -30); /* the render dst anchor */
    draw_pool_free(&dp);

    /* WITH the flip table, the facing==3 arm picks the mirrored cel: frame =
     * frame_base(0) + flip(4) = 4 (the 0xc3e6 bank 0xe5 mirror block). */
    T_ASSERT_EQ_I(pool.states[0].facing, 3);
    draw_pool dpm;
    T_ASSERT_EQ_I(draw_pool_init(&dpm), 0);
    e = actor_render_static(&pool.actors[0], &pool.states[0], flip_tbl, &dpm,
                            resolve_pack, NULL);
    T_ASSERT_EQ_I(e, 1);
    T_ASSERT_EQ_U(dpm.layers[13].nodes[0].sprite, resolve_pack(0xe5u, 4u, NULL));
    draw_pool_free(&dpm);

    /* NULL guard. */
    T_ASSERT_EQ_I(actor_spawn_effect_from_map(NULL, &md, NULL), -1);
    return 0;
}

/* The cutscene arrival cast advances the shared LCG by exactly the 4 SCRIPT
 * EFFECT objects' spawn-draw burst (engine-quirk #94: 19-object room-load burst =
 * 15 map + 4 cutscene), so the establishing REVEAL's iris-variant draw lands on
 * retail's town value.  Locks the 42-draw contract + the post-spawn variant. */
int test_actor_spawn_cutscene_iris(void)
{
    /* (A) cutscene_cast consumes exactly 42 draws (Arche 12 + Barnard/Father/
     * Mother 10 each).  Compare the live seed word against a reference LCG. */
    rng_srand(0x4f5347u);
    actor_spawn_pool pool;
    memset(&pool, 0, sizeof pool);
    T_ASSERT_EQ_I(actor_spawn_cutscene_cast(&pool, NULL, 0), 4);
    uint32_t got = rng_peek_state();
    uint32_t ref = 0x4f5347u;
    for (int k = 0; k < 42; k++) ref = ref * 0x343fdu + 0x269ec3u;
    T_ASSERT_EQ_U(got, ref);   /* exactly 42 LCG steps (party_resolve is RNG-free) */

    /* (B) the full room-load chain: 15 map EFFECT objects (171 draws) + the 4
     * cutscene cast (42) = 213, then the iris-variant draw (rand*3)>>15 == 0
     * (center-out, the live town value — proven offline from the pinned seed). */
    rng_srand(0x4f5347u);
    for (int k = 0; k < 171; k++) (void)rng_rand();   /* the 15 map EFFECT burst */
    memset(&pool, 0, sizeof pool);
    T_ASSERT_EQ_I(actor_spawn_cutscene_cast(&pool, NULL, 0), 4);   /* +42 */
    uint32_t v = rng_rand();                           /* the 214th draw = iris  */
    T_ASSERT_EQ_U((v * 3u) >> 15, 0u);                 /* variant 0 = center-out */

    /* the wrong (pre-fix) phase — 171 draws, NO cutscene — would pick variant 2. */
    rng_srand(0x4f5347u);
    for (int k = 0; k < 171; k++) (void)rng_rand();
    uint32_t vbad = rng_rand();
    T_ASSERT_EQ_U((vbad * 3u) >> 15, 2u);              /* the bug this fixes */

    /* NULL guard. */
    T_ASSERT_EQ_I(actor_spawn_cutscene_cast(NULL, NULL, 0), -1);
    return 0;
}

/* The butterfly per-tick draw model (0x47b990 0xe29a, engine-quirk #95): the
 * every-other-tick gate, the flit-pick timer, and the heading+flag draws. */
int test_butterfly_pertick(void)
{
    butterfly_pool p;
    butterfly_pool_reset(&p);
    T_ASSERT_EQ_I(p.count, 0);

    /* A butterfly with freq 0: the (rand*1000>>15) < freq test NEVER passes, so
     * the flit pick draws ONLY the 523 test (no 534).  Deterministic vs the seed. */
    rng_srand(0x4f5347u);
    T_ASSERT_EQ_I(butterfly_register(&p, 0, 100000, 44800, 0), 0);
    T_ASSERT_EQ_I(butterfly_step(&p), 3);   /* tick 0: 523 test(1) + heading+flag(2) */
    T_ASSERT_EQ_I(butterfly_step(&p), 0);   /* tick 1: gate -> skip                  */
    T_ASSERT_EQ_I(butterfly_step(&p), 2);   /* tick 2: heading+flag (timer != 0)     */
    T_ASSERT_EQ_I(butterfly_step(&p), 0);   /* tick 3: gate                          */
    T_ASSERT_EQ_I(butterfly_step(&p), 2);   /* tick 4                                */

    /* A butterfly with freq 0x8000: the test ALWAYS passes (rand*1000>>15 < 32768),
     * so the flit pick draws the 523 test + the 534 offset. */
    butterfly_pool_reset(&p);
    rng_srand(0x4f5347u);
    T_ASSERT_EQ_I(butterfly_register(&p, 0x8000u, 100000, 44800, 0), 0);
    T_ASSERT_EQ_I(butterfly_step(&p), 4);   /* tick 0: 523(1) + 534(1) + heading+flag(2) */
    T_ASSERT_EQ_I(butterfly_step(&p), 0);   /* gate */
    T_ASSERT_EQ_I(butterfly_step(&p), 2);   /* tick 2: just heading+flag */

    /* The flit-pick timer reloads 0x50 work-ticks, so after the first pick (tick 0)
     * the next fires 0x50 work-ticks (= 0x50*2 sim-ticks) later.  Walk the even work
     * ticks: each draws only heading+flag (2) until the timer hits 0 again. */
    butterfly_pool_reset(&p);
    rng_srand(0x4f5347u);
    butterfly_register(&p, 0x8000u, 100000, 44800, 0);
    butterfly_step(&p);                     /* tick 0 work-tick 0: pick fires, reload 0x50 */
    /* The timer reloads 0x50 AFTER firing, so it decrements over the next 0x50 work
     * ticks and re-fires on work-tick 0x51 (= sim-tick 162, matching the census). */
    for (int wt = 1; wt <= BUTTERFLY_WANDER_PERIOD; wt++) {
        T_ASSERT_EQ_I(butterfly_step(&p), 0);   /* odd tick: gate */
        T_ASSERT_EQ_I(butterfly_step(&p), 2);   /* even work-tick: no pick (timer>0) */
    }
    T_ASSERT_EQ_I(butterfly_step(&p), 0);            /* gate */
    T_ASSERT_EQ_I(butterfly_step(&p), 4);            /* work-tick 0x51: the pick re-fires */

    /* End-to-end: the 4 town butterflies, registered with their seed-pinned move
     * frequencies (engine-quirk #95: captured 653/686/735/698 from the spawn), draw
     * exactly 14 on the spawn tick (2 of the 4 pass the move test) and 8 thereafter
     * on even ticks — matching the seed-pinned per-tick census bit-exact. */
    butterfly_pool_reset(&p);
    rng_srand(0x9c2b551du);                  /* the post-spawn LCG state (tick-0 onEnter) */
    butterfly_register(&p, 653, 105600, 44800, 0); /* the 4 town spawn (wx,wy)@t0 capture */
    butterfly_register(&p, 686,  99200, 43200, 1);
    butterfly_register(&p, 735, 181200, 44800, 2);
    butterfly_register(&p, 698, 176400, 44800, 3);
    T_ASSERT_EQ_I(butterfly_step(&p), 14);   /* spawn tick: 4 picks, 2 pass -> 14 */
    T_ASSERT_EQ_I(butterfly_step(&p), 0);    /* odd: gate */
    T_ASSERT_EQ_I(butterfly_step(&p), 8);    /* even: 4 x heading+flag */

    /* NULL guards. */
    T_ASSERT_EQ_I(butterfly_register(NULL, 0, 0, 0, 0), -1);
    T_ASSERT_EQ_I(butterfly_step(NULL), 0);
    return 0;
}

/* The VERTICAL flutter (the case-3 jump FSM with the butterfly's RE'd constants,
 * driven by the captured PORT-DEBT(butterfly-flutter-trigger) control).  Asserts
 * the ported PHYSICS reproduces the captured lane-0 (spawn worldX 105600) vvel +
 * worldY sawtooth bit-exact (impulse -32000 -> -30000 after the windup, the held
 * vs free rise grav, the +2000/16000 fall), and that a butterfly with no control
 * lane simply glides to the fall cap. */
int test_butterfly_flutter(void)
{
    butterfly_pool p;
    butterfly_pool_reset(&p);
    rng_srand(0x4f5347u);    /* the horizontal AI draws — irrelevant to the vertical */
    butterfly_register(&p, 653, 105600, 44800, 0);   /* lane 0 (BUTTERFLY_FLAP_CTRL_WX[0]) */
    T_ASSERT_EQ_I(p.b[0].ctrl_lane, 0);
    T_ASSERT_EQ_I(p.b[0].world_y, 44800);
    T_ASSERT_EQ_I(p.b[0].vvel, 0);
    /* reference (tick, vvel, worldY) read off runs/butterfly-flutter lane 0 */
    struct { int tick; int32_t vvel; int32_t wy; } ref[] = {
        {1, 2000, 44800}, {8, 16000, 45360}, {17, -30000, 46320},
        {18, -29000, 46020}, {21, -23000, 45180}, {25, -7000, 44500},
    };
    int ri = 0, n = (int)(sizeof ref / sizeof ref[0]);
    for (int t = 1; t <= 30 && ri < n; t++) {
        butterfly_step(&p);               /* one sim-tick -> life_tick == t */
        if (ref[ri].tick == t) {
            T_ASSERT_EQ_I(p.b[0].life_tick, t);
            T_ASSERT_EQ_I(p.b[0].vvel, ref[ri].vvel);
            T_ASSERT_EQ_I(p.b[0].world_y, ref[ri].wy);
            ri++;
        }
    }
    T_ASSERT_EQ_I(ri, n);

    /* No matching control lane -> never flaps: glides, vvel ramps +2000 to the cap. */
    butterfly_pool_reset(&p);
    rng_srand(0x4f5347u);
    butterfly_register(&p, 0, 123456, 40000, 0);
    T_ASSERT_EQ_I(p.b[0].ctrl_lane, -1);
    for (int t = 1; t <= 8; t++) butterfly_step(&p);
    T_ASSERT_EQ_I(p.b[0].vvel, 16000);    /* the fall terminal cap */
    T_ASSERT(p.b[0].world_y > 40000);     /* descended (no flap up) */
    butterfly_step(&p);
    T_ASSERT_EQ_I(p.b[0].vvel, 16000);    /* stays capped */
    return 0;
}

/* Chip-1 open-air PATROL MOTION (movement-system.md): the bounds are set from the
 * spawn worldX, the heading FSM aims the move command, and the apply integrates a
 * +-10/tick velocity ramp (cap +-100) into worldX EVERY tick (both gate phases). */
int test_butterfly_motion(void)
{
    butterfly_pool p;
    butterfly_pool_reset(&p);
    rng_srand(0x4f5347u);
    /* freq 0 -> the move test never passes + (with this seed) no early bound/roll
     * flip, so the butterfly just drifts toward its LEFT bound (heading 0 arm). */
    T_ASSERT_EQ_I(butterfly_register(&p, 0, 100000, 44800, 0), 0);
    /* Bounds = spawn_wx + 11200 / - 8000 (the capture's dead constants). */
    T_ASSERT_EQ_I(p.b[0].bound1, 100000 + 11200);
    T_ASSERT_EQ_I(p.b[0].bound3, 100000 -  8000);
    T_ASSERT_EQ_I(p.b[0].world_x, 100000);
    T_ASSERT_EQ_I(p.b[0].cmd_dir, -1);          /* heading 0 -> aim LEFT first */

    /* Ticks 0 (work) + 1 (skip) are GUARANTEED cmd_dir == -1 (the AI reads the
     * bound for the CURRENT heading BEFORE any flip, so a tick-0 roll-flip cannot
     * redirect the move until the next work tick).  The apply steps worldX by the
     * CURRENT velocity THEN ramps it -10/tick toward -100 (the capture's
     * "dwx=hv_before" form): tick 0 has hvel 0 so worldX holds; the glide builds
     * from tick 1.  The apply runs on BOTH gate phases. */
    butterfly_step(&p);  T_ASSERT_EQ_I(p.b[0].hvel, -10); T_ASSERT_EQ_I(p.b[0].world_x, 100000);
    butterfly_step(&p);  T_ASSERT_EQ_I(p.b[0].hvel, -20); T_ASSERT_EQ_I(p.b[0].world_x, 100000 - 10);
    T_ASSERT_EQ_I(p.b[0].facing, 3);            /* moving left -> facing 3 */

    /* Over a long run the heading FSM keeps it PATROLLING between its bounds: the
     * velocity never exceeds the cap and worldX never runs away past the bounds
     * (the within-one-tile bound flip turns it around; ~one tile of decel slack). */
    for (int k = 0; k < 600; k++) {
        butterfly_step(&p);
        T_ASSERT(p.b[0].hvel <=  BUTTERFLY_HSPEED_CAP);
        T_ASSERT(p.b[0].hvel >= -BUTTERFLY_HSPEED_CAP);
        T_ASSERT(p.b[0].world_x >= p.b[0].bound3 - 3 * 0xc80);
        T_ASSERT(p.b[0].world_x <= p.b[0].bound1 + 3 * 0xc80);
    }
    return 0;
}

/* The town's four IRREGULAR per-tick ambient/event timers (engine-quirk #95;
 * ambient.c) — the residual that desynced the settled-town stream past the
 * REVEAL.  Each fires once in the window; the draw counts + fire ticks are the
 * seed-pinned ground truth (runs/ambient-timer). */
int test_ambient_pertick(void)
{
    ambient_pool a;

    /* The 0x467380 (0xe2a5) event timer: spawn-set to 184, it decrements
     * silently for 183 ticks, then fires 4 draws at tick 183 (the cd==1 arm:
     * fire + FUN_004099a0 x2 + reload). */
    ambient_reset(&a);
    T_ASSERT_EQ_I(a.event.cd, 184);
    T_ASSERT_EQ_I(a.event.armed, 1);
    for (int t = 0; t < 183; t++)
        T_ASSERT_EQ_I(ambient_effect_step(&a), 0);   /* ticks 0..182: decrement */
    T_ASSERT_EQ_I(ambient_effect_step(&a), 4);        /* tick 183: FIRE          */

    /* The CHARACTER-band timers (0x1136f, 0x11370, the wagon's idle-wander) init
     * at tick 0 with one (rand*300)>>15 draw each.  Seeded at the post-spawn LCG
     * state + the 20 regular draws (14 butterfly + 6 fountain) that precede them
     * in the 0x46cd70 walk, the three countdowns are 189 / 33 / 134 — so they
     * fire at ticks 189 / 33 / 134 respectively (bit-exact vs the census). */
    ambient_reset(&a);
    rng_srand(0x9c2b551du);
    for (int i = 0; i < 20; i++) (void)rng_rand();
    T_ASSERT_EQ_I(ambient_character_step(&a), 3);   /* tick 0: three init draws */
    T_ASSERT_EQ_I(a.sound_a.cd, 188);   /* 0x1136f: 189 - 1 (the tick-0 decrement) */
    T_ASSERT_EQ_I(a.sound_b.cd, 32);    /* 0x11370: 33 - 1                         */
    T_ASSERT_EQ_I(a.wagon.cd,  133);    /* wagon:   134 - 1                        */

    /* Step ticks 1..32 (all silent), then tick 33 the 0x11370 emitter fires +3
     * (reload + pick + sound-param); the other two only decrement. */
    int total = 0;
    for (int t = 1; t <= 33; t++) total += ambient_character_step(&a);
    T_ASSERT_EQ_I(total, 3);
    T_ASSERT_EQ_I(a.sound_b.cd >= 300, 1);   /* 0x11370 reloaded to (rand*300>>15)+300 */
    T_ASSERT_EQ_I(a.sound_a.cd, 188 - 33);   /* 0x1136f still counting down (-> tick 189) */
    T_ASSERT_EQ_I(a.wagon.cd,  133 - 33);    /* wagon still counting down (-> tick 134)   */

    /* NULL guards. */
    ambient_reset(NULL);
    T_ASSERT_EQ_I(ambient_effect_step(NULL), 0);
    T_ASSERT_EQ_I(ambient_character_step(NULL), 0);
    return 0;
}

/* THEME 2: Arche's run-off (the L7->L8 "runs to the house" beat, cutscene-party-
 * chars).  Verifies the RUN clip's faithful cel sequence (16,16,17,18,19,19,20,21
 * @dur 5, RE'd off retail.osr) + the motion: the ported two-phase run accel up to
 * the cap, the decel-approach, and the stop AT the house door (ARCHE_RUNOFF_TARGET_X).
 */
int test_arche_runoff(void)
{
    arche_runoff st;

    /* NULL / inactive guards. */
    arche_runoff_begin(NULL, 0, 0);
    T_ASSERT_EQ_I(arche_runoff_step(NULL) == NULL, 1);
    memset(&st, 0, sizeof st);
    T_ASSERT_EQ_I(arche_runoff_step(&st) == NULL, 1);   /* inactive */

    /* Begin: from Arche's anchor (41600) to the house door (73104).  The MOTION runs
     * (phase RUN) from the first step — the run-off fires on the camera beat — but the
     * run CELS lag by the windup (retail plays her lean cels first). */
    arche_runoff_begin(&st, 41600, ARCHE_RUNOFF_TARGET_X);
    T_ASSERT_EQ_I(st.active, 1);
    T_ASSERT_EQ_I(st.phase, ARCHE_RUNOFF_RUN);
    T_ASSERT_EQ_I(st.world_x, 41600);
    T_ASSERT_EQ_I(st.vel, 0);

    /* During the windup the run CEL is held (idle = NULL clip) but the two-phase accel
     * ADVANCES — she slides forward while retail plays her lean cels, so her position
     * (and the camera onset) stay matched; only the run-cycle cel onset lags to ~980. */
    const anim_clip *c0 = arche_runoff_step(&st);
    T_ASSERT_EQ_I(c0 == NULL, 1);                 /* run cel held during the windup */
    T_ASSERT_EQ_I(st.vel, 3200);                  /* but the accel began (0 -> 3200) */
    T_ASSERT_EQ_I(st.world_x, 41600 + 3200 / 100);
    for (int t = 1; t < ARCHE_RUNOFF_WINDUP_TICKS; t++)
        T_ASSERT_EQ_I(arche_runoff_step(&st) == NULL, 1);   /* cel still held */
    T_ASSERT_EQ_I(st.vel > 3200, 1);              /* kept accelerating through the windup */

    /* First post-windup step: the run clip appears (she keeps accelerating). */
    const anim_clip *run = arche_runoff_step(&st);
    T_ASSERT_EQ_I(run != NULL, 1);
    T_ASSERT_EQ_I(st.phase, ARCHE_RUNOFF_RUN);

    /* The RUN clip's faithful cel sequence: step an anim_state through it and read
     * cels 16,16,17,18,19,19,20,21, then it wraps back to 16 (a 40-tick loop). */
    T_ASSERT_EQ_I(run->base_sprite, 16);
    T_ASSERT_EQ_I(run->frame_count, 8);
    T_ASSERT_EQ_I(run->frame_dur, 5);
    T_ASSERT_EQ_I(run->oneshot, 0);       /* loops */
    {
        anim_state as = { .clip = run, .timer = 0, .frame = 0, .done = 0 };
        int16_t want[8] = { 16, 16, 17, 18, 19, 19, 20, 21 };
        for (int f = 0; f < 8; f++) {
            T_ASSERT_EQ_I(anim_clip_sprite(&as), want[f]);
            for (int t = 0; t < run->frame_dur; t++) anim_clip_advance(&as);
        }
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 16);   /* wrapped back to cel 16 */
    }

    /* Accel reaches the cap (48000), then cruises +480/tick. */
    int guard = 0;
    while (st.vel < 48000 && guard++ < 100)
        arche_runoff_step(&st);
    T_ASSERT_EQ_I(st.vel, 48000);          /* CHAR_RUN_CAP */
    int32_t wx_before = st.world_x;
    arche_runoff_step(&st);
    T_ASSERT_EQ_I(st.world_x - wx_before, 480);   /* cruise: world_x += cap/100 */

    /* Run to completion: she decelerates near the target (the decel clip, cels
     * 8-11, one-shot) then stops EXACTLY at the door. */
    int saw_decel = 0;
    guard = 0;
    while (st.phase != ARCHE_RUNOFF_ARRIVED && guard++ < 4000) {
        const anim_clip *c = arche_runoff_step(&st);
        if (st.phase == ARCHE_RUNOFF_DECEL) {
            saw_decel = 1;
            T_ASSERT_EQ_I(c->base_sprite, 8);   /* the decel clip */
            T_ASSERT_EQ_I(c->oneshot, 1);
        }
    }
    T_ASSERT_EQ_I(saw_decel, 1);
    T_ASSERT_EQ_I(st.phase, ARCHE_RUNOFF_ARRIVED);
    T_ASSERT_EQ_I(st.world_x, ARCHE_RUNOFF_TARGET_X);   /* planted at the door */

    /* Arrived: the arrival-idle clip (cels 152-154), position held. */
    const anim_clip *idle = arche_runoff_step(&st);
    T_ASSERT_EQ_I(idle != NULL, 1);
    T_ASSERT_EQ_I(idle->base_sprite, 152);
    T_ASSERT_EQ_I(st.world_x, ARCHE_RUNOFF_TARGET_X);

    return 0;
}

/* ── USER notes #3-5: the house Arche TURN clip cels (RE'd off retail.osr res 0x570
 *    ticks 1579-1587): the one-shot turn 158(4t)→7(4t), then the post-turn base-0
 *    standing idle 0/1/2 (14t).  Steps an anim_state through both clips. ── */
int test_arche_house_turn_clip(void)
{
    const anim_clip *turn = arche_house_turn_clip();
    T_ASSERT(turn != NULL);
    T_ASSERT_EQ_I(turn->frame_count, 2);
    T_ASSERT_EQ_I(turn->frame_dur, 4);
    T_ASSERT_EQ_I(turn->oneshot, 1);          /* play once, hold on the last cel */

    /* Stepped: cel 158 held 4 ticks, then cel 7 held 4 ticks, then `done` is raised
     * and it freezes on cel 7 (the caller swaps to the standing idle on done). */
    {
        anim_state as = { .clip = turn, .timer = 0, .frame = 0, .done = 0 };
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 158);
        for (int t = 0; t < turn->frame_dur; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 7);
        T_ASSERT_EQ_I(as.done, 0);
        for (int t = 0; t < turn->frame_dur; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(as.done, 1);            /* one-shot finished after cel 7's dur */
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 7);   /* frozen on the last cel */
    }

    /* The post-turn standing idle: a base-0 breathe 0,1,2,1 at 14t/cel, looping. */
    const anim_clip *si = arche_house_turn_idle_clip();
    T_ASSERT(si != NULL);
    T_ASSERT_EQ_I(si->oneshot, 0);            /* loops */
    T_ASSERT_EQ_I(si->frame_count, 4);
    T_ASSERT_EQ_I(si->frame_dur, 14);
    {
        anim_state as = { .clip = si, .timer = 0, .frame = 0, .done = 0 };
        int16_t want[4] = { 0, 1, 2, 1 };
        for (int f = 0; f < 4; f++) {
            T_ASSERT_EQ_I(anim_clip_sprite(&as), want[f]);
            for (int t = 0; t < si->frame_dur; t++) anim_clip_advance(&as);
        }
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 0);   /* wrapped back to cel 0 */
    }
    return 0;
}

/* ── The U/D-POSE clip FSM (crouch / up-defensive), RE'd off retail-pose.osr (res
 *    0x570, ckpt 153b; engine-quirk #114): a transition cel on enter AND exit,
 *    holding a steady cel between.  CROUCH enter/exit 31, hold 32; UP enter/exit
 *    34, hold 35 (RIGHT-facing).  The LEFT-facing pose emerges from the bank-0x8b
 *    +152 renderer flip (ARCHE_FREEROAM_FLIP) — verified off retail-poseL.osr
 *    (left crouch 183/184, up 186/187), not unit-tested here. ── */
int test_arche_pose_clip(void)
{
    arche_pose_anim st;
    memset(&st, 0, sizeof st);

    /* CROUCH engage: the enter->hold one-shot {31, 32}. */
    const anim_clip *c = arche_pose_clip(&st, CHAR_POSE_DOWN, 0, 0, 0);
    T_ASSERT(c != NULL);
    T_ASSERT_EQ_I(c->frame_delta[0], 31);
    T_ASSERT_EQ_I(c->frame_delta[1], 32);
    T_ASSERT_EQ_I(c->frame_count, 2);
    T_ASSERT_EQ_I(c->frame_dur, 5);   /* dur 5 -> 4 rendered ticks (advance-before-render) */
    T_ASSERT_EQ_I(c->oneshot, 1);
    {   /* step it: cel 31 for the enter dur, then freeze on the crouch-hold cel 32 */
        anim_state as = { .clip = c, .timer = 0, .frame = 0, .done = 0 };
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 31);
        for (int t = 0; t < 5; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 32);
        for (int t = 0; t < 20; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 32);   /* one-shot freezes on the hold */
    }
    /* Sustained crouch keeps returning the crouch clip. */
    c = arche_pose_clip(&st, CHAR_POSE_DOWN, 0, 0, 0);
    T_ASSERT_EQ_I(c->frame_delta[0], 31);

    /* RELEASE: the exit transition cel (31) for exactly ARCHE_POSE_EXIT_TICKS, then
     * the walk/idle clip resumes. */
    for (int t = 0; t < ARCHE_POSE_EXIT_TICKS; t++) {
        c = arche_pose_clip(&st, 0, /*moving=*/0, 0, 0);
        T_ASSERT_EQ_I(c->frame_delta[0], 31);   /* the crouch-exit cel */
        T_ASSERT_EQ_I(c->frame_count, 1);
    }
    c = arche_pose_clip(&st, 0, /*moving=*/0, 0, 0);
    T_ASSERT_EQ_I(c == arche_freeroam_clip(0, 0, 0), 1);   /* back to idle */

    /* UP-pose engage: the enter->hold one-shot {34, 35}. */
    memset(&st, 0, sizeof st);
    c = arche_pose_clip(&st, CHAR_POSE_UP, 0, 0, 0);
    T_ASSERT_EQ_I(c->frame_delta[0], 34);
    T_ASSERT_EQ_I(c->frame_delta[1], 35);
    {
        anim_state as = { .clip = c, .timer = 0, .frame = 0, .done = 0 };
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 34);
        for (int t = 0; t < 5; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 35);
    }
    /* RELEASE up: the up-exit cel (34). */
    c = arche_pose_clip(&st, 0, 0, 0, 0);
    T_ASSERT_EQ_I(c->frame_delta[0], 34);
    T_ASSERT_EQ_I(c->frame_count, 1);

    /* While crouching, the MOVING/run args are ignored (the pose wins over walk). */
    memset(&st, 0, sizeof st);
    c = arche_pose_clip(&st, CHAR_POSE_DOWN, /*moving=*/1, 0, /*run=*/1);
    T_ASSERT_EQ_I(c->frame_delta[0], 31);

    return 0;
}

/* ── The SWORD unsheathe/sheathe clip FSM (ckpt 155-156; RE-DONE ckpt 159 — the
 *    sword-OUT form is a SEPARATE bank, USER ckpt 158).  DRAW = res 0x571 cels 96→103
 *    forward (8 frames dur 7, ends held on 103); SHEATHE = res 0x570 cels 96→103
 *    (16 frames dur 3); the sword-out idle/walk DELEGATE to the pose/freeroam clip
 *    (same cel indices, rendered on bank 0x8c by the freeroam bank swap). ── */
int test_arche_sword_clip(void)
{
    arche_sword_anim st;  memset(&st, 0, sizeof st);
    arche_pose_anim  pst; memset(&pst, 0, sizeof pst);

    /* DRAW: the sword_out 0->1 edge plays the FORWARD clip 96→103 (swing out). */
    const anim_clip *c = arche_sword_clip(&st, /*sword_out=*/1, /*attacking=*/0,
                                          /*kind=*/0, &pst, 0, 0, 0, 0);
    T_ASSERT(c != NULL);
    T_ASSERT_EQ_I(c->base_sprite, 96);
    T_ASSERT_EQ_I(c->frame_delta[0], 0);    /* cel 96 first (sword leaving the sheath) */
    T_ASSERT_EQ_I(c->frame_count, 8);
    T_ASSERT_EQ_I(c->frame_dur, 7);
    T_ASSERT_EQ_I(c->oneshot, 1);
    {   /* the clip steps 96 -> ... -> 103 (sword out front), freezing on 103 (one-shot) */
        anim_state as = { .clip = c, .timer = 0, .frame = 0, .done = 0 };
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 96);
        for (int t = 0; t < 8 * 7 + 10; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 103);
    }

    /* The draw clip is returned for ARCHE_SWORD_DRAW_TICKS calls, then the FSM hands
     * off to the sword-OUT idle = the normal freeroam idle clip (rendered on res 0x571
     * via the bank swap — the cel indices are stance-independent). */
    memset(&st, 0, sizeof st);
    memset(&pst, 0, sizeof pst);
    int draw_calls = 0;
    for (int t = 0; t < ARCHE_SWORD_DRAW_TICKS + 4; t++) {
        c = arche_sword_clip(&st, 1, /*attacking=*/0, /*kind=*/0, &pst,
                             /*cmd_pose=*/0, /*moving=*/0, 0, 0);
        if (c->frame_delta[0] == 0 && c->frame_count == 8 && c->frame_dur == 7)
            draw_calls++;
    }
    T_ASSERT_EQ_I(draw_calls, ARCHE_SWORD_DRAW_TICKS);
    /* sword-OUT idle = the dur-8 sword-out idle (cels 0-2, faster than the dur-14
     * sword-in idle) — a distinct clip, NOT the base freeroam idle. */
    T_ASSERT_EQ_I(c != arche_freeroam_clip(/*moving=*/0, 0, 0), 1);
    T_ASSERT_EQ_I(c->base_sprite, 0);
    T_ASSERT_EQ_I(c->frame_count, 3);
    T_ASSERT_EQ_I(c->frame_dur, 8);

    /* SHEATHE: the 1->0 edge plays res 0x570 96→103 (onto the back), 16 frames dur 3. */
    c = arche_sword_clip(&st, /*sword_out=*/0, /*attacking=*/0, /*kind=*/0,
                         &pst, 0, 0, 0, 0);
    T_ASSERT_EQ_I(c->base_sprite, 96);
    T_ASSERT_EQ_I(c->frame_delta[0], 0);    /* cel 96 first (forward) */
    T_ASSERT_EQ_I(c->frame_count, 16);
    T_ASSERT_EQ_I(c->frame_dur, 3);

    /* A moving sword-out Arche (no transient) delegates to the base WALK clip — the
     * blade comes from the bank swap (res 0x571 cels 8-15), not a distinct clip. */
    memset(&st, 0, sizeof st);
    memset(&pst, 0, sizeof pst);
    st.prev_out = 1;   /* already drawn, no transient */
    c = arche_sword_clip(&st, /*sword_out=*/1, /*attacking=*/0, /*kind=*/0,
                         &pst, /*cmd_pose=*/0, /*moving=*/1, 0, 0);
    T_ASSERT_EQ_I(c == arche_freeroam_clip(/*moving=*/1, 0, 0), 1);

    /* ── chip 2a: the NEUTRAL ATTACK swing (X, no direction) — res 0x571 cels
     *    104-109, dur-6, 36t, then back to idle.  attacking=1 + kind NEUTRAL wins
     *    over the walk/idle (but not the draw/sheathe transient). ── */
    memset(&st, 0, sizeof st);
    memset(&pst, 0, sizeof pst);
    st.prev_out = 1;   /* already drawn, no transient pending */
    c = arche_sword_clip(&st, /*sword_out=*/1, /*attacking=*/1,
                         /*kind=*/CHAR_ATTACK_NEUTRAL, &pst, 0, /*moving=*/0, 0, 0);
    T_ASSERT_EQ_I(c->base_sprite, 104);
    T_ASSERT_EQ_I(c->frame_delta[0], 0);     /* cel 104 first */
    T_ASSERT_EQ_I(c->frame_count, 6);        /* 104..109 */
    T_ASSERT_EQ_I(c->frame_dur, 6);
    T_ASSERT_EQ_I(c->oneshot, 1);
    {   /* steps 104 -> ... -> 109, freezing on 109 (one-shot) */
        anim_state as = { .clip = c, .timer = 0, .frame = 0, .done = 0 };
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 104);
        for (int t = 0; t < 6 * 6 + 8; t++) anim_clip_advance(&as);
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 109);
    }
    /* the swing wins over a moving body (no walk mid-swing) */
    c = arche_sword_clip(&st, 1, /*attacking=*/1, /*kind=*/CHAR_ATTACK_NEUTRAL,
                         &pst, 0, /*moving=*/1, 0, 0);
    T_ASSERT_EQ_I(c->base_sprite, 104);
    /* but the DRAW transient still wins over the attack (finish drawing first) */
    memset(&st, 0, sizeof st);
    c = arche_sword_clip(&st, /*sword_out=*/1, /*attacking=*/1,
                         /*kind=*/CHAR_ATTACK_NEUTRAL, &pst, 0, 0, 0, 0);
    T_ASSERT_EQ_I(c->base_sprite, 96);       /* the draw (96-103), not the swing */
    return 0;
}

/* ── USER "props missing on shelf" (ckpt 182/183): the errands shelf PILE (structure
 *    band, layer 8) was OCCLUDED by the shelf-BACK units, which ERRANDS_CAST spawned at
 *    the cast layer 13 (over the L8 pile).  ckpt 183 made the shop furniture/shelf/props
 *    MAP-DRIVEN (the CHARACTER band via CHAR_BANK_DEFS, RE'd from 0x431e30) — the shelf-
 *    backs now resolve to LAYER 5 (behind the pile), matching retail.  Guard that (a)
 *    the migrated furniture is GONE from ERRANDS_CAST, and (b) the def table gives the
 *    shelf-backs / bookshelf layer 5. ── */
int test_errands_cast_zorder(void)
{
    actor_spawn_pool pool;
    int n = actor_spawn_room_cast(&pool, 0x334dcu);   /* CUTSCENE_ROOM_ERRANDS */
    T_ASSERT(n > 0);
    int saw_family = 0, saw_bookshelf = 0, saw_shelfunit = 0;
    for (int i = 0; i < pool.count; i++) {
        uint16_t bank = pool.actors[i].sprite_table[0].bank;
        int16_t  fb   = pool.actors[i].sprite_table[0].frame_base;
        if (bank == 0xe3u)             saw_family = 1;      /* Father still cast    */
        if (bank == 0x16fu && fb == 3) saw_bookshelf = 1;  /* migrated -> must be GONE */
        if (bank == 0x16cu && fb == 9) saw_shelfunit = 1;  /* migrated -> must be GONE */
    }
    T_ASSERT_EQ_I(saw_family, 1);
    T_ASSERT_EQ_I(saw_bookshelf, 0);   /* now map-driven (CHARACTER band) */
    T_ASSERT_EQ_I(saw_shelfunit, 0);   /* now map-driven (CHARACTER band) */

    /* the shelf-BACK / bookshelf z (the "props missing on shelf" fix) now lives in the
     * CHARACTER def table: res1027 shelf-backs + res1023 bookshelf -> LAYER 5 (behind
     * the L8 structure pile). */
    uint16_t bank; uint32_t layer;
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112dcu, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_U(layer, 5u);   /* shelf-back var8/9/64 */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112dbu, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16cu); T_ASSERT_EQ_U(layer, 5u);   /* shelf-back var14/11 */
    T_ASSERT_EQ_I(actor_spawn_sprite_for_code(0x112d1u, &bank, &layer), 1);
    T_ASSERT_EQ_U(bank, 0x16fu); T_ASSERT_EQ_U(layer, 5u);   /* bookshelf/cabinet/hutch */

    /* the HOUSE cast stays at the default cast layer 13 (no z-order overrides). */
    int hn = actor_spawn_room_cast(&pool, 0x334c8u);   /* CUTSCENE_ROOM_HOUSE */
    T_ASSERT(hn > 0);
    for (int i = 0; i < pool.count; i++)
        T_ASSERT_EQ_I(pool.actors[i].layer, 13);
    return 0;
}

/* ── ckpt 185: the errands fireplace FIRE (0x112e4, res 1034) is now MAP-DRIVEN — the
 *    CHARACTER band spawns it ADDITIVE, retiring its ERRANDS_CAST capture.  RE'd off the
 *    0x431e30 fire case: install bank 0x1a3 (0x426d70), the FIRE_CLIP (0x407b80 clip
 *    DAT_00647e58), the additive blend ramp_a[14] (0x4385c0 DAT_008a92f0), and LAYER 6
 *    (0x438610(6) — NOT the ex-capture's default L13, and the additive z genuinely
 *    matters since the blit ADDS to what's behind).  The map world (32000,32000) +
 *    dst_base 0 nets to the SAME screen pos as the ex-fit (32900,33800)+dst(-9,-18) =
 *    (320,160)+pivot.  Guard the map-spawn params + GONE from ERRANDS_CAST. ── */
int test_errands_fire(void)
{
    map_layer L;
    layer_set(&L, 0x112e4u, 320, 320);   /* fireplace fire @map(320,320) var 0 */
    /* variant 0 (fire), left at 0 by layer_set */
    map_data md; memset(&md, 0, sizeof md); md.count = 1; md.layers = &L;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 1);
    const actor              *a  = &pool.actors[0];
    const actor_render_state *rs = &pool.states[0];
    T_ASSERT_EQ_U(a->code, 0x112e4u);
    T_ASSERT_EQ_U(a->sprite_table[0].bank, 0x1a3u);
    T_ASSERT_EQ_I(a->sprite_table[0].frame_base, 0);   /* variant 0 */
    T_ASSERT_EQ_I(a->node_alpha, 14);     /* ramp_a[14] additive (bmode 1) */
    T_ASSERT_EQ_I(a->layer, 6);           /* 0x438610(6) — behind the furniture (additive) */
    T_ASSERT_EQ_I(rs->world_x, 32000);    /* map 320*100 */
    T_ASSERT_EQ_I(rs->world_y, 32000);    /* map 320*100 */
    /* the FIRE_CLIP — 6 frames, uniform dur 6, LOOPING (cel f held 6 ticks). */
    const anim_clip *c = rs->clip;
    T_ASSERT(c != NULL);
    T_ASSERT_EQ_I(c->frame_count, 6);
    T_ASSERT_EQ_I(c->frame_dur, 6);
    T_ASSERT_EQ_I(c->oneshot, 0);
    {
        anim_state as = { .clip = c, .timer = 0, .frame = 0, .done = 0 };
        for (int f = 0; f < 6; f++) {
            T_ASSERT_EQ_I(anim_clip_sprite(&as), f);
            for (int t = 0; t < c->frame_dur; t++) anim_clip_advance(&as);
        }
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 0);   /* wrapped back to cel 0 */
    }

    /* GONE from ERRANDS_CAST: no room-cast member carries bank 0x1a3 now. */
    actor_spawn_pool rc;
    int n = actor_spawn_room_cast(&rc, 0x334dcu);   /* CUTSCENE_ROOM_ERRANDS */
    T_ASSERT(n > 0);
    for (int i = 0; i < rc.count; i++)
        T_ASSERT(rc.actors[i].sprite_table[0].bank != 0x1a3u);
    return 0;
}

/* ── ckpt 184: the errands ANIM props (pendulum clock 0x112d9, cooking pot 0x112da)
 *    are now MAP-DRIVEN — the CHARACTER band (g_actors) spawns them with their anim
 *    clip via actor_spawn_clip_for_code, retiring their ERRANDS_CAST capture.  Guard
 *    (a) the map-spawn assigns bank 0x16b + the variant frame_base + the swing/steam
 *    clip, and (b) they are GONE from ERRANDS_CAST (no bank-0x16b room-cast member ->
 *    no double-draw).  Map positions are ground truth off DATA-1025 (clock 528,248
 *    var43 -> 52800,24800; pot 676,296 var56 -> 67600,29600). ── */
int test_errands_clock_pot_mapdriven(void)
{
    map_layer layers[2];
    layer_set(&layers[0], 0x112d9u, 528, 248);   /* pendulum clock */
    hdr_set(layers[0].hdr, 0x18, 43);            /* variant 43 -> frame_base 43 */
    layer_set(&layers[1], 0x112dau, 676, 296);   /* cooking pot */
    hdr_set(layers[1].hdr, 0x18, 56);            /* variant 56 -> frame_base 56 */
    map_data md; memset(&md, 0, sizeof md); md.count = 2; md.layers = layers;

    actor_spawn_pool pool;
    T_ASSERT_EQ_I(actor_spawn_from_map(&pool, &md), 2);

    /* slot 0 = the CLOCK: bank 0x16b, frame_base 43, layer 9, world *100, the SWING
     * clip (delta {0,1,2,1}, dur 25) -> rendered cels 43,44,45,44. */
    T_ASSERT_EQ_U(pool.actors[0].code, 0x112d9u);
    T_ASSERT_EQ_U(pool.actors[0].sprite_table[0].bank, 0x16bu);
    T_ASSERT_EQ_I(pool.actors[0].sprite_table[0].frame_base, 43);
    T_ASSERT_EQ_U(pool.actors[0].layer, 9u);
    T_ASSERT_EQ_I(pool.states[0].world_x, 52800);
    T_ASSERT_EQ_I(pool.states[0].world_y, 24800);
    const anim_clip *cl = pool.states[0].clip;
    T_ASSERT(cl != NULL);
    T_ASSERT_EQ_I(cl->frame_count, 4);
    T_ASSERT_EQ_I(cl->frame_dur, 25);
    T_ASSERT_EQ_I(cl->oneshot, 0);
    {   /* the clip sprite delta = base_sprite + frame_delta[f] = {0,1,2,1} (frame_base
         * 43 is added at render time -> cels 43,44,45,44), looping. */
        int want[4] = { 0, 1, 2, 1 };
        anim_state as = { .clip = cl, .timer = 0, .frame = 0, .done = 0 };
        for (int f = 0; f < 4; f++) {
            T_ASSERT_EQ_I(anim_clip_sprite(&as), want[f]);
            for (int t = 0; t < cl->frame_dur; t++) anim_clip_advance(&as);
        }
        T_ASSERT_EQ_I(anim_clip_sprite(&as), 0);   /* wraps back to delta 0 */
    }

    /* slot 1 = the POT: bank 0x16b, frame_base 56, layer 9, the STEAM clip (delta
     * {1,2,3,4}, dur 6) -> rendered cels 57,58,59,60. */
    T_ASSERT_EQ_U(pool.actors[1].code, 0x112dau);
    T_ASSERT_EQ_U(pool.actors[1].sprite_table[0].bank, 0x16bu);
    T_ASSERT_EQ_I(pool.actors[1].sprite_table[0].frame_base, 56);
    T_ASSERT_EQ_U(pool.actors[1].layer, 9u);
    T_ASSERT_EQ_I(pool.states[1].world_x, 67600);
    T_ASSERT_EQ_I(pool.states[1].world_y, 29600);
    const anim_clip *cp = pool.states[1].clip;
    T_ASSERT(cp != NULL);
    T_ASSERT_EQ_I(cp->frame_count, 4);
    T_ASSERT_EQ_I(cp->frame_dur, 6);
    {
        int want[4] = { 1, 2, 3, 4 };
        anim_state as = { .clip = cp, .timer = 0, .frame = 0, .done = 0 };
        for (int f = 0; f < 4; f++) {
            T_ASSERT_EQ_I(anim_clip_sprite(&as), want[f]);
            for (int t = 0; t < cp->frame_dur; t++) anim_clip_advance(&as);
        }
    }

    /* a NON-anim CHARACTER code gets no clip (static prop) — the wall shelf 0x112cf. */
    {
        map_layer s; layer_set(&s, 0x112cfu, 532, 256);
        map_data smd; memset(&smd, 0, sizeof smd); smd.count = 1; smd.layers = &s;
        actor_spawn_pool sp;
        T_ASSERT_EQ_I(actor_spawn_from_map(&sp, &smd), 1);
        T_ASSERT_EQ_P(sp.states[0].clip, NULL);
    }

    /* GONE from ERRANDS_CAST: no room-cast member carries bank 0x16b now (the clock/pot
     * were the only 0x16b entries; keeping them here too would double-draw). */
    actor_spawn_pool rc;
    int n = actor_spawn_room_cast(&rc, 0x334dcu);   /* CUTSCENE_ROOM_ERRANDS */
    T_ASSERT(n > 0);
    for (int i = 0; i < rc.count; i++)
        T_ASSERT(rc.actors[i].sprite_table[0].bank != 0x16bu);
    return 0;
}
