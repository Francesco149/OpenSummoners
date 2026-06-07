/*
 * test_actor_render.c — host tests for the town actor render path
 * (FUN_0044d160 descriptor builder + the FUN_00491ae0 default-arm emit,
 * src/actor_render.c).
 *
 * The decompiled arithmetic IS the spec: each describe test drives a synthetic
 * actor + render-state through actor_render_describe and asserts the exact
 * descriptor bytes (off_x/off_y/bank/frame/alpha) FUN_0044d160 produces for the
 * static / mirrored / animated / flag-abs / angle cases; the emit tests build a
 * draw_pool, run actor_render_static, and assert the node FUN_00492670 lays down
 * (world pos, layer, placement offset, cel, mode-0) plus the skip / layer-
 * override / describe-fail gates.
 */
#include "actor_render.h"
#include "draw_pool.h"
#include "anim_clip.h"
#include "t.h"

#include <string.h>

/* A resolver that packs (bank, frame) into a non-zero cel handle so a test can
 * verify the exact (bank, frame) actor_render_static resolved. */
static uint32_t resolve_pack(uint16_t bank, uint16_t frame, void *ud)
{
    (void)ud;
    return ((uint32_t)bank << 16) | frame;
}

/* ---- actor_render_describe (FUN_0044d160) --------------------------------- */

int test_actor_describe_static(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 2;
    a.sprite_table[2].bank       = 0x100;
    a.sprite_table[2].frame_base = 5;
    a.sprite_table[2].x_off      = 10;
    a.sprite_table[2].y_off      = 20;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;          /* clip NULL => static */

    actor_desc d;
    memset(&d, 0xee, sizeof(d));
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 1);
    T_ASSERT_EQ_I(d.off_x, 10);     /* 0 + row.x_off */
    T_ASSERT_EQ_I(d.off_y, 20);     /* 0 + row.y_off */
    T_ASSERT_EQ_U(d.bank, 0x100);
    T_ASSERT_EQ_I(d.frame, 5);      /* frame_base + 0 + 0 */
    T_ASSERT_EQ_I(d.alpha, 0);
    return 0;
}

/* Skip when the direction's sprite bank is 0, or the render-state is inactive. */
int test_actor_describe_skips(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 2;   /* sprite_table[2].bank stays 0 */

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 0);  /* bank 0 */

    a.sprite_table[2].bank = 0x100;
    rs.active = 0;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 0);  /* inactive */
    return 0;
}

/* facing==3 mirrors: off_x = row.mirror_x - off_x, frame += flip_table[bank]. */
int test_actor_describe_mirrored(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 3;
    a.sprite_table[3].bank       = 4;     /* small bank -> small flip table */
    a.sprite_table[3].frame_base = 5;
    a.sprite_table[3].mirror_x   = 0x40;
    a.sprite_table[3].x_off      = 10;    /* NOT used on the mirror path */
    a.sprite_table[3].y_off      = 20;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.facing = 3;

    int16_t flip[8] = { 0 };
    flip[4] = 3;   /* DAT_008a8440[bank 4] */

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, flip, &d), 1);
    T_ASSERT_EQ_I(d.off_x, 0x40);   /* mirror_x - 0 */
    T_ASSERT_EQ_I(d.off_y, 20);
    T_ASSERT_EQ_U(d.bank, 4);
    T_ASSERT_EQ_I(d.frame, 8);      /* frame_base 5 + flip 3 + 0 */
    return 0;
}

/* An animated clip: off = clip per-frame offset + row offset; frame =
 * frame_base + base_sprite + frame_delta[f]. */
int test_actor_describe_animated(void)
{
    anim_clip clip;
    memset(&clip, 0, sizeof(clip));
    clip.base_sprite     = 100;
    clip.frame_delta[1]  = 7;
    clip.off_x[1]        = 3;
    clip.off_y[1]        = 4;
    clip.flag_abs        = 0;

    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank       = 0x50;
    a.sprite_table[0].frame_base = 2;
    a.sprite_table[0].x_off      = 11;
    a.sprite_table[0].y_off      = 12;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.clip   = &clip;
    rs.frame  = 1;

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 1);
    T_ASSERT_EQ_I(d.off_x, 14);     /* clip.off_x[1] 3 + row.x_off 11 */
    T_ASSERT_EQ_I(d.off_y, 16);     /* clip.off_y[1] 4 + row.y_off 12 */
    T_ASSERT_EQ_U(d.bank, 0x50);
    T_ASSERT_EQ_I(d.frame, 109);    /* frame_base 2 + (base 100 + delta 7) */
    return 0;
}

/* The clip's -1 frame_delta terminator skips the actor. */
int test_actor_describe_clip_terminator(void)
{
    anim_clip clip;
    memset(&clip, 0, sizeof(clip));
    clip.frame_delta[2] = -1;

    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank = 0x50;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.clip   = &clip;
    rs.frame  = 2;

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 0);
    return 0;
}

/* clip.link overrides the direction used for the sprite-table row (but not the
 * original-dir skip check). */
int test_actor_describe_clip_link_overrides_dir(void)
{
    anim_clip clip;
    memset(&clip, 0, sizeof(clip));
    clip.base_sprite    = 0;
    clip.frame_delta[0] = 0;
    clip.link           = 3;     /* override dir 0 -> 3 */

    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank       = 0x50;   /* original-dir skip check uses this */
    a.sprite_table[3].bank       = 0x77;   /* the row actually rendered */
    a.sprite_table[3].frame_base = 9;
    a.sprite_table[3].x_off      = 1;
    a.sprite_table[3].y_off      = 2;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.clip   = &clip;
    rs.frame  = 0;

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 1);
    T_ASSERT_EQ_U(d.bank, 0x77);    /* sprite_table[3] */
    T_ASSERT_EQ_I(d.off_x, 1);      /* clip.off_x[0] 0 + row[3].x_off 1 */
    T_ASSERT_EQ_I(d.frame, 9);      /* row[3].frame_base 9 + 0 + 0 */
    return 0;
}

/* clip.flag_abs (+0x4c) => "absolute" offset: the row x_off is NOT added. */
int test_actor_describe_flag_abs(void)
{
    anim_clip clip;
    memset(&clip, 0, sizeof(clip));
    clip.base_sprite    = 0;
    clip.frame_delta[0] = 0;
    clip.off_x[0]       = 30;
    clip.off_y[0]       = 40;
    clip.flag_abs       = 1;       /* absolute */

    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank       = 0x50;
    a.sprite_table[0].frame_base = 0;
    a.sprite_table[0].x_off      = 11;    /* must NOT be added */
    a.sprite_table[0].y_off      = 12;    /* y_off IS still added (LAB_0044d294) */

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.clip   = &clip;
    rs.frame  = 0;

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 1);
    T_ASSERT_EQ_I(d.off_x, 30);     /* clip.off_x[0], no row x_off */
    T_ASSERT_EQ_I(d.off_y, 52);     /* clip.off_y[0] 40 + row.y_off 12 */
    return 0;
}

/* actor.angle_anim => frame_off = ((angle+360000)/(360000/div) & 0xffff) % div. */
int test_actor_describe_angle(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank       = 0x50;
    a.sprite_table[0].frame_base = 5;
    a.sprite_table[0].x_off      = 11;    /* skipped on the angle path */
    a.sprite_table[0].y_off      = 12;
    a.angle_anim = 1;
    a.angle_div  = 8;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.angle  = 45000;   /* (45000+360000)/(360000/8=45000)=9 ; 9 % 8 = 1 */

    actor_desc d;
    T_ASSERT_EQ_I(actor_render_describe(&a, &rs, NULL, &d), 1);
    T_ASSERT_EQ_I(d.off_x, 0);      /* static off_x, angle path skips row x_off */
    T_ASSERT_EQ_I(d.off_y, 12);     /* 0 + row.y_off */
    T_ASSERT_EQ_I(d.frame, 6);      /* frame_base 5 + frame_off 1 + 0 */
    return 0;
}

/* ---- actor_render_static (the FUN_00491ae0 default arm) ------------------- */

int test_actor_static_emits_node(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 2;
    a.sprite_table[2].bank       = 0x100;
    a.sprite_table[2].frame_base = 5;
    a.sprite_table[2].x_off      = 10;
    a.sprite_table[2].y_off      = 20;
    a.layer      = 9;
    a.node_alpha = 0;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active     = 1;
    rs.world_x    = 150 * 100;
    rs.world_y    = 80 * 100;
    rs.dst_base_x = 1000;
    rs.dst_base_y = 2000;

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    int emitted = actor_render_static(&a, &rs, NULL, &p, resolve_pack, NULL);
    T_ASSERT_EQ_I(emitted, 1);
    T_ASSERT_EQ_U(p.layers[9].count, 1);

    const draw_node *n = &p.layers[9].nodes[0];
    T_ASSERT_EQ_U(n->sprite, ((uint32_t)0x100 << 16) | 5);  /* resolve(bank,frame) */
    T_ASSERT_EQ_I(n->dst_x, 150 * 100);     /* world_x */
    T_ASSERT_EQ_I(n->dst_y, 80 * 100);      /* world_y */
    T_ASSERT_EQ_I((int32_t)n->param6, 1010);/* off_x 10 + dst_base_x 1000 */
    T_ASSERT_EQ_I((int32_t)n->param7, 2020);/* off_y 20 + dst_base_y 2000 */
    T_ASSERT_EQ_U(n->param8, 0);            /* alpha */
    T_ASSERT_EQ_U(n->mode, 0);             /* opaque -> mode 0 (keyed) */

    draw_pool_free(&p);
    return 0;
}

/* The actor +0x284 skip flag emits nothing. */
int test_actor_static_skip_flag(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank = 0x100;
    a.skip  = 1;
    a.layer = 9;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    T_ASSERT_EQ_I(actor_render_static(&a, &rs, NULL, &p, resolve_pack, NULL), 0);
    T_ASSERT_EQ_U(p.layers[9].count, 0);
    draw_pool_free(&p);
    return 0;
}

/* The render-state +0x284 sub-object's +0x100 field overrides the layer. */
int test_actor_static_layer_override(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank = 0x100;
    a.layer = 9;

    uint8_t subobj[0x108];
    memset(subobj, 0, sizeof(subobj));
    *(uint32_t *)(subobj + 0x100) = 12;   /* override layer */

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active         = 1;
    rs.layer_override = subobj;

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    T_ASSERT_EQ_I(actor_render_static(&a, &rs, NULL, &p, resolve_pack, NULL), 1);
    T_ASSERT_EQ_U(p.layers[12].count, 1);   /* emitted into the override layer */
    T_ASSERT_EQ_U(p.layers[9].count, 0);
    draw_pool_free(&p);
    return 0;
}

/* When describe returns 0 (e.g. bank 0), no node is emitted. */
int test_actor_static_describe_fail(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir   = 0;     /* sprite_table[0].bank stays 0 -> describe returns 0 */
    a.layer = 9;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    T_ASSERT_EQ_I(actor_render_static(&a, &rs, NULL, &p, resolve_pack, NULL), 0);
    T_ASSERT_EQ_U(p.layers[9].count, 0);
    draw_pool_free(&p);
    return 0;
}

/* ---- actor_render_protagonist (the FUN_00491ae0 case-0x1872d arm) --------- */

/* The protagonist emits THREE cels on its layer: two fixed bank-0x175 cels
 * (frame 0 @ x-256, frame 1 @ x-128) then the animated body, in that order. */
int test_actor_protagonist_three_cels(void)
{
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank       = 0x200;   /* the body's bank (the table row) */
    a.sprite_table[0].frame_base = 7;
    a.sprite_table[0].x_off      = 4;
    a.sprite_table[0].y_off      = 6;
    a.layer      = 9;
    a.node_alpha = 0;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active     = 1;          /* clip NULL => the body is static */
    rs.world_x    = 544 * 100;
    rs.world_y    = 320 * 100;
    rs.dst_base_x = 100;
    rs.dst_base_y = 200;

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    int emitted = actor_render_protagonist(&a, &rs, NULL, &p, resolve_pack, NULL);
    T_ASSERT_EQ_I(emitted, 3);
    T_ASSERT_EQ_U(p.layers[9].count, 3);

    /* Part 0: fixed left cel — bank 0x175, frame 0, x = -256 + dst_base. */
    const draw_node *n0 = &p.layers[9].nodes[0];
    T_ASSERT_EQ_U(n0->sprite, ((uint32_t)ACTOR_PROT_BANK << 16) | 0);
    T_ASSERT_EQ_I((int32_t)n0->param6, -256 + 100);
    T_ASSERT_EQ_I((int32_t)n0->param7, 0 + 200);
    T_ASSERT_EQ_I(n0->dst_x, 544 * 100);
    T_ASSERT_EQ_I(n0->dst_y, 320 * 100);

    /* Part 1: fixed middle cel — bank 0x175, frame 1, x = -128 + dst_base. */
    const draw_node *n1 = &p.layers[9].nodes[1];
    T_ASSERT_EQ_U(n1->sprite, ((uint32_t)ACTOR_PROT_BANK << 16) | 1);
    T_ASSERT_EQ_I((int32_t)n1->param6, -128 + 100);
    T_ASSERT_EQ_I((int32_t)n1->param7, 0 + 200);

    /* Part 2: the animated body — describe result (bank 0x200, frame_base 7,
     * off = row offset + dst base). */
    const draw_node *n2 = &p.layers[9].nodes[2];
    T_ASSERT_EQ_U(n2->sprite, ((uint32_t)0x200 << 16) | 7);
    T_ASSERT_EQ_I((int32_t)n2->param6, 4 + 100);   /* row.x_off 4 + dst_base 100 */
    T_ASSERT_EQ_I((int32_t)n2->param7, 6 + 200);   /* row.y_off 6 + dst_base 200 */
    draw_pool_free(&p);
    return 0;
}

/* The body part runs the full describe path — e.g. an animated clip with a
 * link-overridden direction picks a different bank/frame for cel #2, while the
 * two fixed cels stay bank 0x175. */
int test_actor_protagonist_body_is_describe(void)
{
    anim_clip clip;
    memset(&clip, 0, sizeof(clip));
    clip.base_sprite    = 10;
    clip.frame_delta[2] = 5;
    clip.off_x[2]       = 1;
    clip.off_y[2]       = 2;
    clip.link           = 4;     /* override dir 0 -> 4 for the body row */

    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank       = 0x200;   /* original-dir skip check uses this */
    a.sprite_table[4].bank       = 0x333;   /* the body row actually rendered */
    a.sprite_table[4].frame_base = 8;
    a.sprite_table[4].x_off      = 3;
    a.sprite_table[4].y_off      = 9;
    a.layer = 9;

    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    rs.clip   = &clip;
    rs.frame  = 2;

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    T_ASSERT_EQ_I(actor_render_protagonist(&a, &rs, NULL, &p, resolve_pack, NULL), 3);

    /* Fixed cels keep bank 0x175. */
    T_ASSERT_EQ_U(p.layers[9].nodes[0].sprite >> 16, ACTOR_PROT_BANK);
    T_ASSERT_EQ_U(p.layers[9].nodes[1].sprite >> 16, ACTOR_PROT_BANK);
    /* Body cel: link-overridden row 4 -> bank 0x333, frame = 8 + (10 + 5). */
    const draw_node *body = &p.layers[9].nodes[2];
    T_ASSERT_EQ_U(body->sprite, ((uint32_t)0x333 << 16) | (8 + 15));
    T_ASSERT_EQ_I((int32_t)body->param6, 1 + 3);   /* clip.off_x[2] 1 + row.x_off 3 */
    T_ASSERT_EQ_I((int32_t)body->param7, 2 + 9);   /* clip.off_y[2] 2 + row.y_off 9 */
    draw_pool_free(&p);
    return 0;
}

/* The +0x284 skip flag and the describe gates (zero dir bank / inactive /
 * clip terminator) all emit NOTHING — not even the two fixed cels. */
int test_actor_protagonist_skip_and_gate(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    /* skip flag set */
    actor a;
    memset(&a, 0, sizeof(a));
    a.dir = 0;
    a.sprite_table[0].bank = 0x200;
    a.layer = 9;
    a.skip  = 1;
    actor_render_state rs;
    memset(&rs, 0, sizeof(rs));
    rs.active = 1;
    T_ASSERT_EQ_I(actor_render_protagonist(&a, &rs, NULL, &p, resolve_pack, NULL), 0);
    T_ASSERT_EQ_U(p.layers[9].count, 0);

    /* describe gate: direction bank 0 -> whole actor (incl. fixed cels) skipped */
    a.skip = 0;
    a.sprite_table[0].bank = 0;
    T_ASSERT_EQ_I(actor_render_protagonist(&a, &rs, NULL, &p, resolve_pack, NULL), 0);
    T_ASSERT_EQ_U(p.layers[9].count, 0);

    /* describe gate: render-state inactive */
    a.sprite_table[0].bank = 0x200;
    rs.active = 0;
    T_ASSERT_EQ_I(actor_render_protagonist(&a, &rs, NULL, &p, resolve_pack, NULL), 0);
    T_ASSERT_EQ_U(p.layers[9].count, 0);
    draw_pool_free(&p);
    return 0;
}

/* ---- actor_anim_advance (0x54f980's frame stepper on a render-state) ------- */

/* The render-state anim sub-block (clip/timer/frame/done) IS an anim_state, so
 * actor_anim_advance must produce byte-identical trajectories to the single
 * ported stepper anim_clip_advance — two ports of the same 0x54f980 idiom. */
int test_actor_anim_advance_matches_stepper(void)
{
    /* a looping 4-frame clip, 18 ticks/frame, loop_to 0 (the WAGON_CLIP shape) */
    anim_clip c = (anim_clip){0};
    c.base_sprite = 2;
    c.frame_delta[0] = 0; c.frame_delta[1] = 1;
    c.frame_delta[2] = 2; c.frame_delta[3] = 3;
    c.frame_count = 4; c.frame_dur = 18; c.oneshot = 0; c.loop_to = 0;

    actor_render_state rs; memset(&rs, 0, sizeof rs);
    rs.active = 1; rs.clip = &c;
    anim_state ref = (anim_state){0};
    anim_state_set(&ref, &c);

    /* step both for two full cycles (4 frames * 18 ticks * 2) + a bit */
    for (int t = 0; t < 4 * 18 * 2 + 7; t++) {
        actor_anim_advance(&rs);
        anim_clip_advance(&ref);
        T_ASSERT_EQ_U(rs.frame, ref.frame);
        T_ASSERT_EQ_U(rs.timer, ref.timer);
        T_ASSERT_EQ_U(rs.done, ref.done);
    }
    /* it loops: frame stays in [0,4), never the dead [4,...] */
    T_ASSERT(rs.frame < 4);
    return 0;
}

/* A NULL clip (the 32 static town actors) is a no-op — the stepper short-
 * circuits on clip==0, so a static render-state never advances. */
int test_actor_anim_advance_null_is_noop(void)
{
    actor_render_state rs; memset(&rs, 0, sizeof rs);
    rs.active = 1; rs.clip = NULL; rs.frame = 0; rs.timer = 0;
    for (int t = 0; t < 50; t++)
        actor_anim_advance(&rs);
    T_ASSERT_EQ_U(rs.frame, 0u);
    T_ASSERT_EQ_U(rs.timer, 0u);
    T_ASSERT_EQ_U(rs.done, 0u);
    return 0;
}
