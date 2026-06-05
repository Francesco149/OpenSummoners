/*
 * map_present.{c,h} — the in-game PRESENT PASS (FUN_0048eac0): walk the
 * 27-layer draw-node pool the per-frame draw walk built and blit each node.
 *
 * This is the CONSUMER side of the draw pipeline.  The producers
 * (map_render_walk / FUN_00490f30 for the tilemap, the actor renderers for
 * sprites) append 0x3c-byte draw nodes into the render context's 27 layers
 * (draw_pool.{c,h}).  Once the whole frame's draw list is built,
 * FUN_0048eac0 walks the layers in order (layer index = draw-order key) and,
 * for each node, projects its world position to screen coords through the
 * camera, culls off-screen nodes, and blits the survivor through one of four
 * zdd primitives selected by the node's MODE (`+0x18`):
 *
 *   mode 0  -> FUN_005b9b70  (zdd_object_blt_keyed-onto)   — sprite, screen-fixed
 *   mode 1  -> FUN_005bd550  (zdd_blit_orchestrate, alpha) — paint_ctx sprite
 *   mode 2  -> the scaled/palette path (DAT_008a9274 + paint_ctx scaling)
 *   mode 3  -> FUN_005b9bf0  (zdd_object_blt_clipped)   when node `+0x14` == 0
 *           -> FUN_005bd550  (zdd_blit_orchestrate, alpha)   when `+0x14` != 0
 *
 * The shared projector + cull is FUN_00490b90 (used verbatim by modes 1 and 3,
 * and inlined by mode 0): screen-space placement plus a four-corner viewport
 * cull, computed from the camera object (the same `view` map_render reads).
 *
 * WHAT THIS MODULE PORTS (pure, host-tested):
 *   - map_present_project  — FUN_00490b90 exactly (the projector + cull).
 *   - map_present          — the 27-layer walk + mode dispatch (FUN_0048eac0).
 *     MODE 3 (the static-backdrop TILE path map_render_walk emits) is ported
 *     in full: project with the node's own w/h as the cull box, then select
 *     CLIPPED (`+0x14`==0) or ALPHA (`+0x14`!=0).  A visible node is handed to
 *     the `present_blit_fn` sink, which the Win32 layer wires to the matching
 *     ported zdd blit (the engine sprite handle in `node +0x00` becomes the
 *     blit's `this`).  Keeping the actual blit behind a callback keeps the walk
 *     pure (the mr_sprite_fn / mg_bank_dims_fn pattern).
 *
 * DEFERRED (PORT-DEBT present-actor-modes): modes 0/1/2.  No ported producer
 * emits them yet — the only wired producer is map_render_walk (mode 3) — and
 * their geometry reads the engine sprite/paint_ctx internals (mode 0/1 cull
 * dims from sprite `+0x1c/+0x20`; mode 2 the DAT_008a9274 palette table + a
 * paint_ctx scaling clone), which are engine globals outside the pure port.
 * map_present still VISITS those nodes (faithful walk order) and reports them
 * through `out_deferred` so they are never silently dropped; it lands with the
 * actor renderers (0x491ae0 et al.).  See docs/port-debt.md.
 *
 * The per-frame "begin draw list" (the layer-count reset at 0x48c150:18-26)
 * is draw_pool_reset — it belongs to the per-frame DRIVER (0x48c150), not the
 * present pass, and is already ported in draw_pool.{c,h}.
 */
#ifndef OSS_MAP_PRESENT_H
#define OSS_MAP_PRESENT_H

#include <stdint.h>

#include "draw_pool.h"
#include "map_render.h"   /* mr_camera — the same view object map_render reads */

/*
 * Which retail blit primitive a VISIBLE node funnels into — faithful to the
 * FUN_0048eac0 mode dispatch.  Handed to the blit sink so the Win32 layer can
 * call the matching ported zdd function.
 */
typedef enum present_kind {
    PRESENT_KEYED   = 0,  /* mode 0      -> FUN_005b9b70 zdd_object_blt_keyed-onto */
    PRESENT_ALPHA   = 1,  /* mode 1, or mode 3 w/ +0x14!=0 -> FUN_005bd550        */
    PRESENT_SCALED  = 2,  /* mode 2      -> the scaled/palette path (deferred)    */
    PRESENT_CLIPPED = 3,  /* mode 3 w/ +0x14==0 -> FUN_005b9bf0 zdd_object_blt_clipped */
} present_kind;

/*
 * One resolved, on-screen draw — passed to the blit sink for each VISIBLE
 * node, in layer-then-node order (the retail present order).  `sprite` is the
 * engine cel handle stored in node `+0x00` (the blit's __thiscall `this`);
 * dst_x/dst_y are the projected screen coords; the source rect is the node's
 * `+0x2c/+0x30/+0x34/+0x38`.
 */
typedef struct present_op {
    present_kind     kind;
    uint32_t         layer;   /* the layer index this node lived in (draw order) */
    uint32_t         sprite;  /* node +0x00 — the cel handle (blit `this`)       */
    int32_t          dst_x;   /* projected screen x (FUN_00490b90 out)           */
    int32_t          dst_y;   /* projected screen y                              */
    int32_t          src_x;   /* node +0x2c                                      */
    int32_t          src_y;   /* node +0x30                                      */
    int32_t          w;       /* node +0x34                                      */
    int32_t          h;       /* node +0x38                                      */
    const draw_node *node;    /* the raw node (for the alpha path's extra fields) */
} present_op;

/* The blit sink — invoked once per VISIBLE node, in present order. */
typedef void (*present_blit_fn)(const present_op *op, void *ud);

/*
 * Port of FUN_00490b90 — project a world position to screen space and cull.
 *
 *   *sx = world_x/100 - (cam->off60 + cam->off34)/100 + offx
 *   *sy = world_y/100 - (cam->off5c + cam->off74*100 + cam->off4c)/100 + offy
 *
 * Returns 1 (visible) when all four hold, else 0 (culled):
 *   cw + *sx >= 0,  *sy + ch >= 0,  *sx < cam->off64/100,  *sy < cam->off68/100
 *
 * `offx`/`offy` are the node's `+0x0c`/`+0x10` placement adjust (0 for backdrop
 * tiles); `cw`/`ch` the cull box (the node's w/h for mode 3).  Integer division
 * truncates toward zero, matching the MSVC codegen.  (`*sx`/`*sy` are written
 * even when culled — retail computes them unconditionally.)
 */
int map_present_project(const mr_camera *cam,
                        int32_t world_x, int32_t world_y,
                        int32_t offx, int32_t offy,
                        int32_t cw, int32_t ch,
                        int32_t *sx, int32_t *sy);

/*
 * Port of FUN_0048eac0 — walk the draw pool's 27 layers in order and, for each
 * node, dispatch on its mode.  MODE 3 (the static-backdrop tile path) is fully
 * implemented: project the node (offx/offy = node `+0x0c/+0x10`, cull box =
 * node w/h), and on a visible node call `blit` with kind CLIPPED (node `+0x14`
 * == 0) or ALPHA (`+0x14` != 0).  Returns the number of nodes handed to the
 * sink (visible mode-3 nodes).
 *
 * Modes 0/1/2 are VISITED in order but DEFERRED (no ported producer emits them;
 * their geometry needs engine sprite internals — see the header banner).  If
 * `out_deferred` is non-NULL it receives the count of such nodes skipped, so
 * they are never silently dropped.
 *
 * `blit` may be NULL (a dry walk that only counts).  Layers with no node array
 * (cap 0, e.g. layer 0) are skipped naturally (count is always 0).
 */
int map_present(const draw_pool *pool, const mr_camera *cam,
                present_blit_fn blit, void *ud, int *out_deferred);

#endif /* OSS_MAP_PRESENT_H */
