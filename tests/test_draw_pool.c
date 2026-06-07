/*
 * test_draw_pool.c — host tests for the per-layer draw-node accumulator
 * (FUN_004917b0 + the FUN_00586010 layer table, src/draw_pool.c).
 *
 * The decompiled stores ARE the spec: each test drives draw_pool_emit with
 * known args and asserts the exact node bytes / per-layer bookkeeping
 * FUN_004917b0 produces, plus the fill/overflow behaviour of the table the
 * sim builds.
 */
#include "draw_pool.h"
#include "t.h"

#include <stddef.h>

/* The draw node must be exactly 0x3c bytes to match retail's `cap * 0x3c`
 * array sizing and the +0x2c/+0x30/+0x34/+0x38 src-rect offsets. */
int test_draw_pool_node_size(void)
{
    T_ASSERT_EQ_U(sizeof(draw_node), 0x3c);
    /* Spot-check the offsets FUN_004917b0 / FUN_00490f30 write. */
    T_ASSERT_EQ_U(offsetof(draw_node, sprite), 0x00);
    T_ASSERT_EQ_U(offsetof(draw_node, dst_x),  0x04);
    T_ASSERT_EQ_U(offsetof(draw_node, dst_y),  0x08);
    T_ASSERT_EQ_U(offsetof(draw_node, mode),   0x18);
    T_ASSERT_EQ_U(offsetof(draw_node, src_x),  0x2c);
    T_ASSERT_EQ_U(offsetof(draw_node, src_y),  0x30);
    T_ASSERT_EQ_U(offsetof(draw_node, w),      0x34);
    T_ASSERT_EQ_U(offsetof(draw_node, h),      0x38);
    return 0;
}

/* The 27 caps match the literals stamped at FUN_00586010, and each layer's
 * array sizing is consistent (cap * 0x3c == the operator_new arg). */
int test_draw_pool_caps_table(void)
{
    T_ASSERT_EQ_U(DRAW_POOL_LAYERS, 27);
    /* Slot 0 has no array; a few representative caps. */
    T_ASSERT_EQ_U(draw_pool_default_caps[0],  0x000);
    T_ASSERT_EQ_U(draw_pool_default_caps[1],  0x080);
    T_ASSERT_EQ_U(draw_pool_default_caps[2],  0x1b8);
    T_ASSERT_EQ_U(draw_pool_default_caps[6],  0x400);
    T_ASSERT_EQ_U(draw_pool_default_caps[9],  0x0e0);
    T_ASSERT_EQ_U(draw_pool_default_caps[10], 0x094);
    T_ASSERT_EQ_U(draw_pool_default_caps[20], 0x0c8);
    T_ASSERT_EQ_U(draw_pool_default_caps[26], 0x400);

    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    for (unsigned i = 0; i < DRAW_POOL_LAYERS; i++) {
        T_ASSERT_EQ_U(p.layers[i].cap, draw_pool_default_caps[i]);
        T_ASSERT_EQ_U(p.layers[i].count, 0);
        if (draw_pool_default_caps[i] == 0)
            T_ASSERT(p.layers[i].nodes == NULL);
        else
            T_ASSERT(p.layers[i].nodes != NULL);
    }
    draw_pool_free(&p);
    return 0;
}

/* draw_pool_emit lays the six caller dwords down exactly where FUN_004917b0
 * stores them, leaves the src rect for the caller, and bumps the layer count. */
int test_draw_pool_emit_fields(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    draw_node *n = draw_pool_emit(&p, 5, 3, 0xdead, 0x1234, 0x5678,
                                  0xaa, 0xbb, 0xcc);
    T_ASSERT(n != NULL);
    T_ASSERT_EQ_U(n->sprite, 0xdead);
    T_ASSERT_EQ_I(n->dst_x, 0x1234);
    T_ASSERT_EQ_I(n->dst_y, 0x5678);
    T_ASSERT_EQ_U(n->param6, 0xaa);
    T_ASSERT_EQ_U(n->param7, 0xbb);
    T_ASSERT_EQ_U(n->param8, 0xcc);
    T_ASSERT_EQ_U(n->mode, 3);
    /* src rect untouched by emit (calloc-zeroed). */
    T_ASSERT_EQ_I(n->src_x, 0);
    T_ASSERT_EQ_I(n->src_y, 0);

    T_ASSERT_EQ_U(p.layers[5].count, 1);
    /* The node lives in layer 5's array, slot 0. */
    T_ASSERT_EQ_P(n, &p.layers[5].nodes[0]);
    draw_pool_free(&p);
    return 0;
}

/* The layer index is `layer_key & 0xffff` — the high word is discarded, as in
 * retail (FUN_004917b0 masks param_1; FUN_00490f30 packs a sort key there). */
int test_draw_pool_emit_layer_mask(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    draw_node *n = draw_pool_emit(&p, 0x51eb0007, 3, 1, 0, 0, 0, 0, 0);
    T_ASSERT(n != NULL);
    T_ASSERT_EQ_U(p.layers[7].count, 1);
    T_ASSERT_EQ_U(p.layers[0].count, 0);
    draw_pool_free(&p);
    return 0;
}

/* A layer fills to exactly its cap, then emit returns NULL (retail
 * `if (cap <= count) return 0`).  Layer 1 caps at 0x80. */
int test_draw_pool_emit_overflow(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    uint16_t cap = draw_pool_default_caps[1];
    for (uint16_t i = 0; i < cap; i++) {
        draw_node *n = draw_pool_emit(&p, 1, 3, i + 1, 0, 0, 0, 0, 0);
        T_ASSERT(n != NULL);
    }
    T_ASSERT_EQ_U(p.layers[1].count, cap);
    /* One past cap -> NULL, count unchanged. */
    T_ASSERT(draw_pool_emit(&p, 1, 3, 0xffff, 0, 0, 0, 0, 0) == NULL);
    T_ASSERT_EQ_U(p.layers[1].count, cap);
    draw_pool_free(&p);
    return 0;
}

/* Layer 0 has cap 0 (no array) -> every emit fails, matching the sim leaving
 * slot 0 zero-initialised. */
int test_draw_pool_layer0_always_full(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    T_ASSERT(draw_pool_emit(&p, 0, 3, 1, 0, 0, 0, 0, 0) == NULL);
    T_ASSERT_EQ_U(p.layers[0].count, 0);
    /* Out-of-range layer key (>= 27) also fails without touching memory. */
    T_ASSERT(draw_pool_emit(&p, 100, 3, 1, 0, 0, 0, 0, 0) == NULL);
    draw_pool_free(&p);
    return 0;
}

/* draw_pool_emit_actor (FUN_00492670): the node bytes match the retail stores,
 * and the MODE is derived as bool(alpha != 0) — opaque actor -> mode 0, alpha
 * actor -> mode 1.  off_x/off_y land in param6/param7; alpha in param8. */
int test_draw_pool_emit_actor_opaque(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    /* layer 9, opaque (alpha 0): cel/world/off exactly placed, mode 0. */
    draw_node *n = draw_pool_emit_actor(&p, 9, 0xCE1, 0x4444, 0x8888,
                                        -0x10, 0x20, 0);
    T_ASSERT(n != NULL);
    T_ASSERT_EQ_U(n->sprite, 0xCE1);     /* node[0] = cel        */
    T_ASSERT_EQ_I(n->dst_x, 0x4444);     /* node[1] = world_x    */
    T_ASSERT_EQ_I(n->dst_y, 0x8888);     /* node[2] = world_y    */
    T_ASSERT_EQ_I((int32_t)n->param6, -0x10);  /* node[3] = off_x */
    T_ASSERT_EQ_I((int32_t)n->param7, 0x20);   /* node[4] = off_y */
    T_ASSERT_EQ_U(n->param8, 0);         /* node[5] = alpha      */
    T_ASSERT_EQ_U(n->mode, 0);           /* node[6] = bool(alpha)=0 -> keyed */
    T_ASSERT_EQ_U(p.layers[9].count, 1);
    draw_pool_free(&p);
    return 0;
}

int test_draw_pool_emit_actor_alpha(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");

    /* Non-zero alpha -> mode 1 (the present alpha path), alpha kept in param8. */
    draw_node *n = draw_pool_emit_actor(&p, 10, 0xBEEF, 1, 2, 3, 4, 0x80);
    T_ASSERT(n != NULL);
    T_ASSERT_EQ_U(n->mode, 1);
    T_ASSERT_EQ_U(n->param8, 0x80);
    draw_pool_free(&p);
    return 0;
}

/* A NULL cel emits nothing (492670.c:12 `if (param_2 != 0)`) — the count stays
 * put and the call returns NULL. */
int test_draw_pool_emit_actor_null_cel(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    T_ASSERT(draw_pool_emit_actor(&p, 9, 0, 1, 2, 3, 4, 0) == NULL);
    T_ASSERT_EQ_U(p.layers[9].count, 0);
    draw_pool_free(&p);
    return 0;
}

/* draw_pool_reset zeroes counts but keeps the arrays — the per-frame begin. */
int test_draw_pool_reset(void)
{
    draw_pool p;
    if (draw_pool_init(&p) != 0) T_SKIP("pool alloc failed");
    draw_node *a = p.layers[3].nodes;
    draw_pool_emit(&p, 3, 3, 1, 0, 0, 0, 0, 0);
    draw_pool_emit(&p, 3, 3, 2, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_U(p.layers[3].count, 2);
    draw_pool_reset(&p);
    T_ASSERT_EQ_U(p.layers[3].count, 0);
    T_ASSERT_EQ_U(p.layers[3].cap, draw_pool_default_caps[3]);
    T_ASSERT_EQ_P(p.layers[3].nodes, a);   /* array preserved */
    draw_pool_free(&p);
    return 0;
}
