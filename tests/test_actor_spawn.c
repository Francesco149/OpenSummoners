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
    layer_set(&layers[1], 0x00ec55u, 100, 100);   /* 60501 STRUCTURE -> skipped  */
    layer_set(&layers[2], 0x112e5u, 1760, 416);   /* prop C -> bank 0x16c f36, layer 10 */
    layer_set(&layers[3], 0x112e6u,  624, 288);   /* invisible CHARACTER volume  */
    layer_set(&layers[4], 0x1129fu, 1184, 448);   /* prop B -> bank 0x16c f2 */

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

/* ---- a spawned prop drives the ckpt-77 render path end-to-end ----------- */

int test_actor_spawn_prop_renders(void)
{
    map_layer L;
    layer_set(&L, 0x1129eu, 864, 448);   /* prop A */
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
    T_ASSERT_EQ_I(actor_spawn_effect_from_map(&pool, &md), 3);  /* 2 townsfolk + 1 butterfly */

    /* slot 0 = 0xc3e6: bank 0xe5, fb 0, layer 13, dst (-30,-32), idle clip set,
     * RNG start phase (frame in [0,20), timer in [0,14)).
     * world = (map - dst) * 100 = ((208,384) - (-30,-32)) * 100 = (23800,41600)
     * — matches the live census rs_x/rs_y exactly. */
    T_ASSERT_EQ_U(pool.actors[0].sprite_table[0].bank, 0xe5u);
    T_ASSERT_EQ_I(pool.actors[0].sprite_table[0].frame_base, 0);
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
    T_ASSERT_EQ_I(fl, 4);                 /* res 0x3fa frames-per-direction      */
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
    T_ASSERT_EQ_I(actor_spawn_effect_from_map(NULL, &md), -1);
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
