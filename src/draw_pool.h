/*
 * draw_pool.{c,h} — the per-layer draw-node accumulator (FUN_004917b0) over
 * the 27-layer draw-node table the in-game sim builds at 0x586010.
 *
 * After map_render_walk (FUN_00490f30) decides a tile must be drawn, it
 * appends a 0x3c-byte DRAW NODE to one of the render context's 27 LAYERS.
 * Each layer is an independent bump array: a {count, cap, node[]} triple.
 * A later pass walks the layers in order and blits each node — so the layer
 * index doubles as the draw-order key (this is how the tilemap, sprites and
 * HUD interleave into a single sorted frame).
 *
 *   - FUN_004917b0 (106 B)  draw_pool_emit   bump-allocate one node in a layer,
 *                                            or NULL when that layer is full.
 *
 * THE LAYER TABLE (0x586010:510-650).  The sim allocates `operator_new(0xd8)`
 * for the table (== `view + 0x54`, where `view = *(room_state + 0x104c)`), then
 * fills 27 (`0x1b`) 8-byte slots — `{u16 count, u16 cap, ptr node[cap]}` — each
 * node array sized `cap * 0x3c` via its own `operator_new`.  The per-layer caps
 * are literal-stamped in the sim and reproduced verbatim in
 * `draw_pool_default_caps[]`.  Slot 0 is zero-initialised and never given an
 * array (cap 0) — emits to layer 0 always fail, matching retail.
 *
 * THE DRAW NODE (0x3c B).  FUN_004917b0 stores six caller dwords; the caller
 * (FUN_00490f30) then stamps the source rect.  Byte layout, pinned from the
 * two functions' stores:
 *   +0x00 sprite   (4917b0 param_3) — retail: the resolved cel pointer
 *                                     0x418470(frame) returns
 *   +0x04 dst_x    (param_4)        — world x = col * 0xc80
 *   +0x08 dst_y    (param_5)        — world y = row * 0xc80
 *   +0x0c param_6  +0x10 param_7  +0x14 param_8   (0 for plain backdrop tiles)
 *   +0x18 mode     (param_2)        — 3 for the backdrop tile arm
 *   +0x1c..+0x28   untouched by these two (filled by the blit consumer)
 *   +0x2c src_x    +0x30 src_y      (490f30) — source-rect offset into the atlas
 *   +0x34 w        +0x38 h          (490f30) — source-rect size (0x20 x 0x20)
 *
 * HOST-PORT NOTE.  The node carries a 4-byte `sprite` handle where retail keeps
 * the cel pointer; map_render_walk resolves it through a callback (see
 * map_render.h's mr_sprite_fn) so the engine sprite manager (0x418470 /
 * &DAT_008a760c) stays out of the pure port.  The layer's `nodes` is a real
 * host pointer (8 B on a 64-bit host) rather than packed into the 8-byte slot;
 * the per-frame logic is identical.
 */
#ifndef OSS_DRAW_POOL_H
#define OSS_DRAW_POOL_H

#include <stdint.h>

/* The render context owns this many draw layers (0x586010: +0x58 = 0x1b). */
#define DRAW_POOL_LAYERS 27u

/* One draw node — exactly 0x3c bytes (15 dwords), matching retail. */
typedef struct draw_node {
    uint32_t sprite;   /* +0x00 */
    int32_t  dst_x;    /* +0x04 */
    int32_t  dst_y;    /* +0x08 */
    uint32_t param6;   /* +0x0c */
    uint32_t param7;   /* +0x10 */
    uint32_t param8;   /* +0x14 */
    uint32_t mode;     /* +0x18 */
    uint32_t _pad1c;   /* +0x1c */
    uint32_t _pad20;   /* +0x20 */
    uint32_t _pad24;   /* +0x24 */
    uint32_t _pad28;   /* +0x28 */
    int32_t  src_x;    /* +0x2c */
    int32_t  src_y;    /* +0x30 */
    int32_t  w;        /* +0x34 */
    int32_t  h;        /* +0x38 */
} draw_node;

/* One layer = a bump array of draw nodes. */
typedef struct draw_layer {
    uint16_t   count;  /* slot +0x00 — nodes used this frame */
    uint16_t   cap;    /* slot +0x02 — capacity (per-layer literal) */
    draw_node *nodes;  /* slot +0x04 — array of `cap` nodes */
} draw_layer;

typedef struct draw_pool {
    draw_layer layers[DRAW_POOL_LAYERS];
} draw_pool;

/* The 27 per-layer capacities the sim stamps (0x586010:510-650).  Indexed
 * by layer (slot offset / 8).  Slot 0 = 0 (no array). */
extern const uint16_t draw_pool_default_caps[DRAW_POOL_LAYERS];

/*
 * Allocate the 27 node arrays per draw_pool_default_caps (caps copied in,
 * counts zeroed).  Returns 0 on success, -1 if any allocation failed (in which
 * case the pool is freed and left zeroed).
 */
int  draw_pool_init(draw_pool *p);

/* Zero every layer's count (the per-frame "begin draw list" — the walk
 * repopulates the layers each frame). */
void draw_pool_reset(draw_pool *p);

/* Release the node arrays and zero the pool. */
void draw_pool_free(draw_pool *p);

/*
 * Port of FUN_004917b0: append one node to layer `layer_key & 0xffff`.
 * Returns the node (with sprite/dst/param/mode set; the src rect left for the
 * caller to stamp) or NULL when that layer is full (`count >= cap`) — exactly
 * retail's `if (cap <= count) return 0`.  Layer keys >= DRAW_POOL_LAYERS return
 * NULL (a port bound; retail's keys are always < 27 by construction).
 *
 * Argument order mirrors the retail call FUN_004917b0(layer, mode, sprite,
 * dst_x, dst_y, p6, p7, p8).
 */
draw_node *draw_pool_emit(draw_pool *p, uint32_t layer_key, uint32_t mode,
                          uint32_t sprite, int32_t dst_x, int32_t dst_y,
                          uint32_t p6, uint32_t p7, uint32_t p8);

/*
 * Port of FUN_00492670 — the ACTOR analog of draw_pool_emit (FUN_004917b0).
 * The town's actor renderer (0x491ae0 / FUN_0044d160) emits sprite nodes
 * through this rather than 0x4917b0.  It writes the SAME 0x3c-byte node, but
 * with two retail-specific twists pinned from 492670.c:
 *   - the node MODE is derived: `mode = (alpha != 0)` — so an opaque actor
 *     (alpha 0) lands as a mode-0 keyed blit and a translucent one as mode-1
 *     alpha (the present-pass actor modes, map_present.c).
 *   - `alpha` is stored in the param8 slot (node +0x14); param6/param7 carry
 *     the per-frame placement offset (off_x/off_y), and dst_x/dst_y carry the
 *     actor's WORLD position (the projector subtracts the camera at present).
 *   - a NULL cel (`param_2 == 0`) emits NOTHING (492670.c:12 `if (param_2 != 0)`).
 *
 * Argument order mirrors the retail call FUN_00492670(layer, cel, world_x,
 * world_y, off_x, off_y, alpha).  Returns the node, or NULL on a NULL cel /
 * full or out-of-range layer (same as draw_pool_emit).
 */
draw_node *draw_pool_emit_actor(draw_pool *p, uint32_t layer_key, uint32_t cel,
                                int32_t world_x, int32_t world_y,
                                int32_t off_x, int32_t off_y, uint32_t alpha);

#endif /* OSS_DRAW_POOL_H */
