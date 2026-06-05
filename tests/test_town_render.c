/*
 * test_town_render.c — host tests for src/town_render.c, the module that
 * composes the backdrop pipeline (parse -> decode -> walk -> present).
 *
 * The sub-stages are exhaustively tested elsewhere (test_map_data /
 * test_map_decode / test_map_render / test_draw_pool / test_map_present); these
 * tests verify the COMPOSITION: that town_render_load feeds a locked resource
 * blob end-to-end and town_render_step turns a populated cell into the exact
 * present op the stack produces, that the sprite-resolver gate is honoured, and
 * that the empty / dry / reload paths behave.
 *
 * The fixture builds a minimal-but-valid map-data resource in memory (the same
 * byte layout map_data_parse reads) with one town tile, so the test exercises
 * the real parse + decode rather than a hand-built map_data.
 */
#include "town_render.h"
#include "t.h"

#include <stdlib.h>
#include <string.h>

/* every bank rounds to a 1x1-tile footprint (so a base tile lands on one cell) */
static void dims_1x1(void *ctx, uint16_t bank, int32_t *w, int32_t *h)
{ (void)ctx; (void)bank; *w = 1; *h = 1; }

/* a stub sprite resolver: return a fixed non-NULL cel handle for every bank. */
#define STUB_CEL 0xdeadbeefu
static uint32_t resolve_all(uint16_t bank, uint16_t frame, void *ud)
{ (void)bank; (void)frame; (void)ud; return STUB_CEL; }
/* a resolver that always fails (skip every tile). */
static uint32_t resolve_none(uint16_t bank, uint16_t frame, void *ud)
{ (void)bank; (void)frame; (void)ud; return 0; }

/* a recording blit sink. */
typedef struct { present_op ops[64]; int n; } sink;
static void rec_blit(const present_op *op, void *ud)
{ sink *s = (sink *)ud; if (s->n < 64) s->ops[s->n] = *op; s->n++; }

/* Build a minimal valid map-data resource: magic + 0x30 header + 0x34 maphdr
 * (dims d0xd1xd2, count 0 layers) + d0*d1*d2 cells of 0x1c bytes each.  Returns
 * an owned buffer; *out_len gets its length. */
static uint8_t *build_blob(uint32_t d0, uint32_t d1, uint32_t d2, size_t *out_len)
{
    size_t cells = (size_t)d0 * d1 * d2 * MD_CELL_SIZE;
    size_t len   = 0x68 + cells;          /* count == 0 -> no layer entries */
    uint8_t *b = (uint8_t *)calloc(len, 1);
    uint32_t magic = 0x30;
    memcpy(b + 0x00, &magic, 4);          /* [0x00:0x04] magic                */
    /* [0x04:0x34] header stays zero.  maphdr at [0x34:0x68]: */
    memcpy(b + 0x54, &d0, 4);             /* maphdr +0x20 dim0                */
    memcpy(b + 0x58, &d1, 4);             /* maphdr +0x24 dim1                */
    memcpy(b + 0x5c, &d2, 4);             /* maphdr +0x28 dim2                */
    /* maphdr +0x2c count == 0 (already zero) */
    *out_len = len;
    return b;
}

/* Set the cell at (x,y,z) of a blob to (tile_id, shape, arg_0c). */
static void blob_set_cell(uint8_t *b, uint32_t d0, uint32_t d1, uint32_t d2,
                          uint32_t x, uint32_t y, uint32_t z,
                          uint32_t tile_id, uint32_t shape, uint32_t arg_0c)
{
    (void)d2;
    size_t idx = (size_t)(d1 * z + y) * d0 + x;   /* map_data z-major */
    uint8_t *p = b + 0x68 + idx * MD_CELL_SIZE;
    memcpy(p + 0x04, &tile_id, 4);
    memcpy(p + 0x0c, &arg_0c, 4);
    memcpy(p + 0x10, &shape, 4);
}

/* The origin-0 camera (full-viewport window over a small map). */
static const mr_camera CAM0 = { 0, 0, 0, 0, 64000, 48000, 0 };

/* ---- load parses + decodes; step blits the populated cell -------------- */

int test_town_render_one_tile(void)
{
    size_t len; uint8_t *blob = build_blob(8, 8, 1, &len);
    /* a 0x1b58b shape-0 cell at (2,3): base tile bank 0x62, a2 0x18, slot 3,
     * flag 0x15, sub-tile offset (0,0) — matches test_map_decode. */
    blob_set_cell(blob, 8, 8, 1, 2, 3, 0, 0x1b58b, 0, 0x18);

    town_render tr;
    T_ASSERT_EQ_I(town_render_load(&tr, blob, len, dims_1x1, NULL), 0);
    T_ASSERT(tr.loaded);
    /* parse consumed the whole resource (well-formed invariant). */
    T_ASSERT_EQ_U(tr.map.consumed, len);
    T_ASSERT_EQ_U(tr.map.dim0, 8);
    T_ASSERT_EQ_U(tr.map.dim1, 8);

    sink s = { .n = 0 };
    int deferred = -1;
    int n = town_render_step(&tr, &CAM0, resolve_all, NULL, rec_blit, &s, &deferred);

    T_ASSERT_EQ_I(n, 1);            /* one visible backdrop tile */
    T_ASSERT_EQ_I(s.n, 1);
    T_ASSERT_EQ_I(deferred, 0);     /* no actor/HUD modes */

    const present_op *op = &s.ops[0];
    T_ASSERT_EQ_I((int)op->kind, (int)PRESENT_CLIPPED);  /* node +0x14 == 0 */
    T_ASSERT_EQ_U(op->sprite, STUB_CEL);
    T_ASSERT_EQ_U(op->layer, 0x15);                      /* region-A flag for slot 3 */
    /* projection through CAM0: dst = cell_world/100.  col 2 -> 2*0xc80/100 = 64,
     * row 3 -> 3*0xc80/100 = 96. */
    T_ASSERT_EQ_I(op->dst_x, 64);
    T_ASSERT_EQ_I(op->dst_y, 96);
    /* the tile's own 0x20x0x20 sub-tile at offset (0,0). */
    T_ASSERT_EQ_I(op->src_x, 0);
    T_ASSERT_EQ_I(op->src_y, 0);
    T_ASSERT_EQ_I(op->w, 0x20);
    T_ASSERT_EQ_I(op->h, 0x20);

    town_render_free(&tr);
    T_ASSERT(!tr.loaded);
    free(blob);
    return 0;
}

/* ---- the resolver gate: a NULL cel skips the tile (no op) -------------- */

int test_town_render_resolver_gate(void)
{
    size_t len; uint8_t *blob = build_blob(8, 8, 1, &len);
    blob_set_cell(blob, 8, 8, 1, 2, 3, 0, 0x1b58b, 0, 0x18);

    town_render tr;
    T_ASSERT_EQ_I(town_render_load(&tr, blob, len, dims_1x1, NULL), 0);

    sink s = { .n = 0 };
    int n = town_render_step(&tr, &CAM0, resolve_none, NULL, rec_blit, &s, NULL);
    T_ASSERT_EQ_I(n, 0);
    T_ASSERT_EQ_I(s.n, 0);

    town_render_free(&tr);
    free(blob);
    return 0;
}

/* ---- an empty map decodes to zero draw nodes -------------------------- */

int test_town_render_empty(void)
{
    size_t len; uint8_t *blob = build_blob(8, 8, 1, &len);   /* no cells set */

    town_render tr;
    T_ASSERT_EQ_I(town_render_load(&tr, blob, len, dims_1x1, NULL), 0);

    sink s = { .n = 0 };
    int n = town_render_step(&tr, &CAM0, resolve_all, NULL, rec_blit, &s, NULL);
    T_ASSERT_EQ_I(n, 0);
    T_ASSERT_EQ_I(s.n, 0);

    town_render_free(&tr);
    free(blob);
    return 0;
}

/* ---- a dry step (NULL blit) still walks + counts ---------------------- */

int test_town_render_dry_step(void)
{
    size_t len; uint8_t *blob = build_blob(8, 8, 1, &len);
    blob_set_cell(blob, 8, 8, 1, 2, 3, 0, 0x1b58b, 0, 0x18);

    town_render tr;
    T_ASSERT_EQ_I(town_render_load(&tr, blob, len, dims_1x1, NULL), 0);

    int n = town_render_step(&tr, &CAM0, resolve_all, NULL, /*blit=*/NULL, NULL, NULL);
    T_ASSERT_EQ_I(n, 1);            /* counted even without a sink */

    town_render_free(&tr);
    free(blob);
    return 0;
}

/* ---- a step on an unloaded scene is a no-op --------------------------- */

int test_town_render_unloaded_noop(void)
{
    town_render tr;
    memset(&tr, 0, sizeof tr);
    sink s = { .n = 0 };
    int deferred = -1;
    int n = town_render_step(&tr, &CAM0, resolve_all, NULL, rec_blit, &s, &deferred);
    T_ASSERT_EQ_I(n, 0);
    T_ASSERT_EQ_I(s.n, 0);
    T_ASSERT_EQ_I(deferred, 0);
    town_render_free(&tr);          /* safe on a zeroed tr */
    return 0;
}

/* ---- a malformed (truncated) resource fails the load cleanly ---------- */

int test_town_render_malformed(void)
{
    /* a blob whose declared cell array overruns its length. */
    size_t len; uint8_t *blob = build_blob(8, 8, 1, &len);
    uint32_t big = 1000;
    memcpy(blob + 0x54, &big, 4);   /* dim0 = 1000 -> cells far exceed len */

    town_render tr;
    T_ASSERT_EQ_I(town_render_load(&tr, blob, len, dims_1x1, NULL), -1);
    T_ASSERT(!tr.loaded);
    town_render_free(&tr);          /* safe after a failed load */
    free(blob);
    return 0;
}
