/*
 * test_map_present.c — host tests for the in-game present pass
 * (FUN_0048eac0, src/map_present.c) and its projector (FUN_00490b90).
 *
 * The decompiled arithmetic IS the spec: each projector test drives the pure
 * transform with known camera + node coords and asserts the exact screen
 * coords + cull decision.  The walk tests build a draw_pool (the same node the
 * producer map_render_walk emits), run map_present with a recording sink, and
 * assert the present order (layer-then-node), the projected geometry, the
 * CLIPPED/ALPHA kind selection (node +0x14), and that culled / non-mode-3
 * nodes are handled per retail (skipped / deferred, never silently dropped).
 */
#include "map_present.h"
#include "draw_pool.h"
#include "t.h"

#include <assert.h>
#include <string.h>

/* ---- a recording blit sink ------------------------------------------------ */

typedef struct rec {
    int        n;
    present_op ops[64];
} rec;

static void rec_blit(const present_op *op, void *ud)
{
    rec *r = (rec *)ud;
    if (r->n < (int)(sizeof(r->ops) / sizeof(r->ops[0])))
        r->ops[r->n] = *op;
    r->n++;
}

/* A camera with no offset: world px*100 maps straight to screen px, viewport
 * 0x280 x 0x1e0 px (the retail 640x480 primary). */
static mr_camera cam_zero(void)
{
    mr_camera c;
    memset(&c, 0, sizeof(c));
    c.off64 = 0x280 * 100;   /* 640 px viewport width  -> off64/100 = 640 */
    c.off68 = 0x1e0 * 100;   /* 480 px viewport height -> off68/100 = 480 */
    return c;
}

/* ---- projector (FUN_00490b90) --------------------------------------------- */

int test_map_present_project_basic(void)
{
    mr_camera c = cam_zero();
    /* world (320*100, 240*100), no camera offset, no placement adjust. */
    int32_t sx = 0, sy = 0;
    int vis = map_present_project(&c, 320 * 100, 240 * 100, 0, 0, 0x20, 0x20, &sx, &sy);
    T_ASSERT_EQ_I(sx, 320);
    T_ASSERT_EQ_I(sy, 240);
    T_ASSERT_EQ_I(vis, 1);
    return 0;
}

int test_map_present_project_camera_and_offset(void)
{
    mr_camera c = cam_zero();
    c.off60 = 100 * 100;   /* (off60+off34)/100 = 100 px scroll-x            */
    c.off34 = 5 * 100;     /* -> total 105 px subtracted from world x        */
    c.off5c = 40 * 100;    /* off5c/100 = 40                                 */
    c.off74 = 2;           /* off74*100 = 200 units = 2 px                   */
    c.off4c = 8 * 100;     /* off4c/100 = 8  -> (4000+200+800)/100 = 50 px y */
    int32_t sx = 0, sy = 0;
    /* sx = 300 - 105 + 7 (offx) = 202 ; sy = 200 - 50 + 3 (offy) = 153 */
    int vis = map_present_project(&c, 300 * 100, 200 * 100, 7, 3, 0x20, 0x20, &sx, &sy);
    T_ASSERT_EQ_I(sx, 202);
    T_ASSERT_EQ_I(sy, 153);
    T_ASSERT_EQ_I(vis, 1);
    return 0;
}

int test_map_present_project_cull_left(void)
{
    mr_camera c = cam_zero();
    int32_t sx = 0, sy = 0;
    /* sx = -40, cull box w = 0x20 (32): cw + sx = 32 + -40 = -8 < 0 -> culled.
     * (sx/sy still written.) */
    int vis = map_present_project(&c, -40 * 100, 100 * 100, 0, 0, 0x20, 0x20, &sx, &sy);
    T_ASSERT_EQ_I(sx, -40);
    T_ASSERT_EQ_I(sy, 100);
    T_ASSERT_EQ_I(vis, 0);
    /* sx = -32 exactly: cw + sx = 0 >= 0 -> on-screen (boundary inclusive). */
    vis = map_present_project(&c, -32 * 100, 100 * 100, 0, 0, 0x20, 0x20, &sx, &sy);
    T_ASSERT_EQ_I(sx, -32);
    T_ASSERT_EQ_I(vis, 1);
    return 0;
}

int test_map_present_project_cull_right_bottom(void)
{
    mr_camera c = cam_zero();   /* off64/100 = 640, off68/100 = 480 */
    int32_t sx = 0, sy = 0;
    /* sx = 640: not < 640 -> culled on the right edge (exclusive). */
    int vis = map_present_project(&c, 640 * 100, 100 * 100, 0, 0, 0x20, 0x20, &sx, &sy);
    T_ASSERT_EQ_I(sx, 640);
    T_ASSERT_EQ_I(vis, 0);
    /* sx = 639 -> visible; sy = 480 -> culled on the bottom edge. */
    vis = map_present_project(&c, 639 * 100, 480 * 100, 0, 0, 0x20, 0x20, &sx, &sy);
    T_ASSERT_EQ_I(sx, 639);
    T_ASSERT_EQ_I(sy, 480);
    T_ASSERT_EQ_I(vis, 0);
    return 0;
}

/* ---- the walk (FUN_0048eac0) ---------------------------------------------- */

/* Emit a mode-3 backdrop node into `layer` and stamp its source rect the way
 * map_render_walk / FUN_00490f30 does. */
static draw_node *emit_tile(draw_pool *p, uint32_t layer, uint32_t sprite,
                            int32_t dst_x, int32_t dst_y, uint32_t param8,
                            int32_t src_x, int32_t src_y)
{
    draw_node *n = draw_pool_emit(p, layer, 3, sprite, dst_x, dst_y, 0, 0, param8);
    assert(n != NULL);
    n->src_x = src_x;
    n->src_y = src_y;
    n->w = 0x20;
    n->h = 0x20;
    return n;
}

int test_map_present_walk_order_and_geometry(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();

    /* Two nodes on layer 5, one on layer 2 — present order is layer-ascending
     * (2 before 5), node-order within a layer. */
    emit_tile(&p, 5, 0xAAAA, 100 * 100, 50 * 100, 0, 0x40, 0x60);   /* L5 #0 */
    emit_tile(&p, 5, 0xBBBB, 200 * 100, 60 * 100, 0, 0, 0);         /* L5 #1 */
    emit_tile(&p, 2, 0xCCCC, 10 * 100, 20 * 100, 0, 0x20, 0x20);    /* L2 #0 */

    rec r = { 0 };
    int deferred = -1;
    int presented = map_present(&p, &c, rec_blit, &r, NULL, NULL, &deferred);

    T_ASSERT_EQ_I(presented, 3);
    T_ASSERT_EQ_I(deferred, 0);
    T_ASSERT_EQ_I(r.n, 3);

    /* layer 2 first */
    T_ASSERT_EQ_U(r.ops[0].layer, 2);
    T_ASSERT_EQ_U(r.ops[0].sprite, 0xCCCC);
    T_ASSERT_EQ_I(r.ops[0].dst_x, 10);
    T_ASSERT_EQ_I(r.ops[0].dst_y, 20);
    T_ASSERT_EQ_I(r.ops[0].src_x, 0x20);
    T_ASSERT_EQ_I(r.ops[0].src_y, 0x20);
    T_ASSERT_EQ_I(r.ops[0].w, 0x20);
    T_ASSERT_EQ_I(r.ops[0].kind, PRESENT_CLIPPED);

    /* then layer 5, in emit order */
    T_ASSERT_EQ_U(r.ops[1].layer, 5);
    T_ASSERT_EQ_U(r.ops[1].sprite, 0xAAAA);
    T_ASSERT_EQ_I(r.ops[1].dst_x, 100);
    T_ASSERT_EQ_I(r.ops[1].dst_y, 50);
    T_ASSERT_EQ_I(r.ops[1].src_x, 0x40);
    T_ASSERT_EQ_I(r.ops[1].src_y, 0x60);
    T_ASSERT_EQ_U(r.ops[2].sprite, 0xBBBB);
    T_ASSERT_EQ_I(r.ops[2].dst_x, 200);

    draw_pool_free(&p);
    return 0;
}

int test_map_present_walk_alpha_kind(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();

    emit_tile(&p, 3, 0x1111, 0, 0, 0, 0, 0);        /* param8 = 0 -> CLIPPED */
    emit_tile(&p, 3, 0x2222, 0, 0, 1, 0, 0);        /* param8 = 1 -> ALPHA   */

    rec r = { 0 };
    int presented = map_present(&p, &c, rec_blit, &r, NULL, NULL, NULL);
    T_ASSERT_EQ_I(presented, 2);
    T_ASSERT_EQ_I(r.ops[0].kind, PRESENT_CLIPPED);
    T_ASSERT_EQ_I(r.ops[1].kind, PRESENT_ALPHA);
    /* the alpha op carries the raw node for the orchestrator's extra fields */
    T_ASSERT(r.ops[1].node != NULL);
    T_ASSERT_EQ_U(r.ops[1].node->param8, 1);

    draw_pool_free(&p);
    return 0;
}

int test_map_present_walk_culls_offscreen(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();

    emit_tile(&p, 1, 0xF00D, 100 * 100, 100 * 100, 0, 0, 0);   /* on-screen  */
    emit_tile(&p, 1, 0xDEAD, 5000 * 100, 100 * 100, 0, 0, 0);  /* x=5000 > 640 */

    rec r = { 0 };
    int presented = map_present(&p, &c, rec_blit, &r, NULL, NULL, NULL);
    /* Only the on-screen node is presented; the culled one calls no sink. */
    T_ASSERT_EQ_I(presented, 1);
    T_ASSERT_EQ_I(r.n, 1);
    T_ASSERT_EQ_U(r.ops[0].sprite, 0xF00D);

    draw_pool_free(&p);
    return 0;
}

int test_map_present_walk_defers_other_modes(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();

    /* Modes 0/1/2 are visited but deferred (no ported producer emits them). */
    draw_pool_emit(&p, 4, 0, 0x10, 0, 0, 0, 0, 0);   /* mode 0 */
    draw_pool_emit(&p, 4, 1, 0x11, 0, 0, 0, 0, 0);   /* mode 1 */
    draw_pool_emit(&p, 4, 2, 0x12, 0, 0, 0, 0, 0);   /* mode 2 */
    emit_tile(&p, 4, 0x13, 0, 0, 0, 0, 0);           /* mode 3 */

    rec r = { 0 };
    int deferred = -1;
    int presented = map_present(&p, &c, rec_blit, &r, NULL, NULL, &deferred);
    T_ASSERT_EQ_I(presented, 1);     /* only the mode-3 node */
    T_ASSERT_EQ_I(deferred, 3);      /* the three other modes, counted */
    T_ASSERT_EQ_I(r.n, 1);
    T_ASSERT_EQ_U(r.ops[0].sprite, 0x13);

    draw_pool_free(&p);
    return 0;
}

/* ---- mode 0 (the opaque ACTOR keyed path, FUN_00492670 emits) ------------ */

/* A cel-dims callback: every cel is 0x20 x 0x18 px (the cull box retail reads
 * from cel +0x1c/+0x20). */
static void dims_2018(uint32_t cel, int32_t *w, int32_t *h, void *ud)
{
    (void)cel; (void)ud;
    *w = 0x20;
    *h = 0x18;
}

/* A mode-0 actor node projects like a tile but blits KEYED (whole sprite, no
 * src rect), and its cull box comes from the cel dims callback. */
int test_map_present_walk_mode0_keyed(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();

    /* world (150,80) px, placement off (+5,+7), opaque (alpha 0 -> mode 0). */
    draw_node *n = draw_pool_emit_actor(&p, 9, 0xACE, 150 * 100, 80 * 100, 5, 7, 0);
    T_ASSERT(n != NULL);
    T_ASSERT_EQ_U(n->mode, 0);

    rec r = { 0 };
    int deferred = -1;
    int presented = map_present(&p, &c, rec_blit, &r, dims_2018, NULL, &deferred);

    T_ASSERT_EQ_I(presented, 1);
    T_ASSERT_EQ_I(deferred, 0);
    T_ASSERT_EQ_I(r.n, 1);
    T_ASSERT_EQ_I(r.ops[0].kind, PRESENT_KEYED);
    T_ASSERT_EQ_U(r.ops[0].layer, 9);
    T_ASSERT_EQ_U(r.ops[0].sprite, 0xACE);
    /* sx = 150 - 0 + 5 = 155 ; sy = 80 - 0 + 7 = 87 */
    T_ASSERT_EQ_I(r.ops[0].dst_x, 155);
    T_ASSERT_EQ_I(r.ops[0].dst_y, 87);
    /* keyed blit draws the whole cel: no src rect; w/h carry the cull dims. */
    T_ASSERT_EQ_I(r.ops[0].src_x, 0);
    T_ASSERT_EQ_I(r.ops[0].w, 0x20);
    T_ASSERT_EQ_I(r.ops[0].h, 0x18);
    draw_pool_free(&p);
    return 0;
}

/* Mode 0 culls using the CEL's dims (not the node's): a node whose right edge
 * is off the left of screen (cel_w + sx < 0) is not presented. */
int test_map_present_walk_mode0_cull(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();

    /* sx = -40, cel_w 0x20 (32): 32 + -40 = -8 < 0 -> culled. */
    draw_pool_emit_actor(&p, 9, 0xACE, -40 * 100, 80 * 100, 0, 0, 0);
    /* sx = -32, cel_w 32: 0 >= 0 -> visible (boundary inclusive). */
    draw_pool_emit_actor(&p, 9, 0xBEE, -32 * 100, 80 * 100, 0, 0, 0);

    rec r = { 0 };
    int presented = map_present(&p, &c, rec_blit, &r, dims_2018, NULL, NULL);
    T_ASSERT_EQ_I(presented, 1);
    T_ASSERT_EQ_U(r.ops[0].sprite, 0xBEE);
    draw_pool_free(&p);
    return 0;
}

/* With NO dims callback, a mode-0 node is DEFERRED (counted), never presented
 * — the tile-only caller contract (town_render_step today). */
int test_map_present_walk_mode0_deferred_without_dims(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();
    draw_pool_emit_actor(&p, 9, 0xACE, 0, 0, 0, 0, 0);

    rec r = { 0 };
    int deferred = -1;
    int presented = map_present(&p, &c, rec_blit, &r, NULL, NULL, &deferred);
    T_ASSERT_EQ_I(presented, 0);
    T_ASSERT_EQ_I(deferred, 1);
    T_ASSERT_EQ_I(r.n, 0);
    draw_pool_free(&p);
    return 0;
}

int test_map_present_walk_dry_count(void)
{
    draw_pool p;
    T_ASSERT_EQ_I(draw_pool_init(&p), 0);
    mr_camera c = cam_zero();
    emit_tile(&p, 6, 0x1, 0, 0, 0, 0, 0);
    emit_tile(&p, 6, 0x2, 0, 0, 0, 0, 0);
    /* NULL sink + NULL out_deferred: a pure count, no crash. */
    int presented = map_present(&p, &c, NULL, NULL, NULL, NULL, NULL);
    T_ASSERT_EQ_I(presented, 2);
    draw_pool_free(&p);
    return 0;
}
