/*
 * map_render.{c,h} — the static tilemap RENDER-WALK geometry (FUN_00490f30).
 *
 * After map_decode (0x587e00) places the room's tiles into the runtime
 * render grid (map_grid.{c,h}), the engine draws the static backdrop by walking
 * the *visible* cell window and emitting one draw node per populated region-A
 * sub-slot.  That walk is **FUN_00490f30** (2002 B) — NOT 0x5a00c0.
 *
 * MODEL CORRECTION (ckpt 60).  Earlier handoffs called 0x5a00c0 "the
 * in-game RENDER dispatch" that "reads the decoded grid and blits the town".
 * It does not: 0x5a00c0 references none of the grid regions — it is the
 * scripted-scene OVERLAY player (a 3-state GetTickCount pace machine + a sprite
 * draw-list on its own stack + a 0x124-stride caption-text array drawn through
 * the font bank DAT_008a7640).  The backdrop tilemap is rendered by
 * FUN_00490f30, called as `FUN_00490f30(view, 1)` with the render grid in ECX
 * from the per-frame draw walk (0x48c150:108 / 0x499100:185, both
 * passing the view object `*(room_state + 0x104c)`).
 *
 * WHAT FUN_00490f30 DOES (490f30.c):
 *   1. From the VIEW/camera object (param_1) + the grid dims it computes the
 *      visible cell window {col0,row0} x {ncols,nrows} (490f30.c:40-54).
 *   2. It scans that window (rows outer, cols inner); per cell it reads the
 *      grid index `col*0x80 + row` and walks region A's 4 sub-slots
 *      (490f30.c:66-229).  For each populated sub-slot (bank != 0) it resolves
 *      the sprite + applies the difficulty/time palette tint (DAT_008a93fc, the
 *      `0x4182d0` ramp), then enqueues a draw node via 0x4917b0 with
 *      destination = the tile's world position and a 0x20x0x20 source rect at
 *      offset (dx*0xc80/100, dy*0xc80/100) — i.e. the (dx,dy)-th 32-px sub-tile
 *      of the bank's atlas.
 *   3. It then consults region C (`+0x195038`, the per-cell object/blend record
 *      map_grid_clear_cell writes) for the `0x1b58d` blend + `0x1b5ab` overlay
 *      objects, emitting more nodes via 0x417c40 / 0x48c6b0.
 *
 * THE DRAW NODE (0x4917b0, 106 B).  A per-LAYER bump allocator: the layer
 * table is at `render_ctx + 0x54` (8 B/layer: u16 count, u16 cap, ptr array of
 * 0x3c-byte nodes).  490f30 fills node `+0x00` sprite, `+0x04/+0x08` dest world
 * x/y, `+0x18` mode, `+0x2c/+0x30` source-rect offset, `+0x34/+0x38` source
 * size (0x20).  The layer key is region-A `+0x4` (the tile's "flag").
 *
 * WHAT THIS MODULE PORTS (pure, host-tested): the GEOMETRY — the part of
 * FUN_00490f30 that is pure arithmetic over the view object and the grid:
 *   - map_render_visible_window  (490f30.c:40-54)
 *   - map_render_grid_index      (the col*0x80 + row read-side linearization)
 *   - map_render_tile            (one region-A sub-slot -> draw-node geometry)
 *
 * DEFERRED (engine-coupled, the rest of the rock): the sprite resolve
 * (0x418470/0x417c40), the palette tint (DAT_008a93fc / 0x4182d0),
 * the draw-node pool enqueue (0x4917b0, needs render_ctx+0x54) and the zdd
 * blit/present, plus the region-C blend/overlay object arms.  Those land with
 * the full draw-pipeline port; this is the read-the-grid + place-the-tile core
 * those build on.
 */
#ifndef OSS_MAP_RENDER_H
#define OSS_MAP_RENDER_H

#include <stdint.h>

#include "draw_pool.h"

/*
 * The VIEW / camera object (FUN_00490f30's param_1; the room-state's
 * `+0x104c`).  Only the fields the visible-window computation reads are
 * modelled here, at their retail byte offsets:
 *   +0x34 cam-x component        (added to +0x60 for the first visible column)
 *   +0x4c cam-y component        (one of three summed for the first row)
 *   +0x5c cam-y component
 *   +0x60 cam-x component
 *   +0x64 viewport extent along the col (dim0) axis
 *   +0x68 viewport extent along the row (dim1) axis
 *   +0x74 cam-y component        (scaled x100 in the row sum)
 * Coordinates are in the engine's 1/0xc80 (=1/3200) sub-tile fixed point; one
 * cell spans 0xc80 units (= 32 px * 100).
 */
typedef struct mr_camera {
    int32_t off34;   /* +0x34 */
    int32_t off4c;   /* +0x4c */
    int32_t off5c;   /* +0x5c */
    int32_t off60;   /* +0x60 */
    int32_t off64;   /* +0x64 viewport cols extent */
    int32_t off68;   /* +0x68 viewport rows extent */
    int32_t off74;   /* +0x74 */
} mr_camera;

/* The visible cell window: cells [col0, col0+ncols) x [row0, row0+nrows). */
typedef struct mr_window {
    int32_t col0;
    int32_t row0;
    int32_t ncols;
    int32_t nrows;
} mr_window;

/* Per-sub-slot draw-node geometry (the populated region-A tile at one cell). */
typedef struct mr_tile {
    uint16_t bank;     /* region A +0x0 — sprite bank id (pool DAT_008a760c)     */
    uint16_t frame;    /* region A +0x2 — sprite frame (490f30: 0x418470)    */
    uint16_t layer;    /* region A +0x4 — the 0x4917b0 layer key            */
    int32_t  dst_x;    /* node +0x04 — destination world x = col * 0xc80         */
    int32_t  dst_y;    /* node +0x08 — destination world y = row * 0xc80         */
    int32_t  src_x;    /* node +0x2c — source-rect x = (dx * 0xc80) / 100        */
    int32_t  src_y;    /* node +0x30 — source-rect y = (dy * 0xc80) / 100        */
    int32_t  w;        /* node +0x34 — source-rect width  (always 0x20)          */
    int32_t  h;        /* node +0x38 — source-rect height (always 0x20)          */
} mr_tile;

/*
 * Compute the visible cell window from the view object + the grid dims
 * (490f30.c:40-54).  dim0 = cols (grid MG_DIM0), dim1 = rows (MG_DIM1).
 * The first visible col/row is clamped to >= 0; the count is capped at
 * viewport_extent/0xc80 + 2 (the 2-cell render margin).  ncols/nrows may be
 * <= 0, in which case the retail walk renders nothing.
 */
void map_render_visible_window(const mr_camera *cam,
                               int32_t dim0, int32_t dim1,
                               mr_window *out);

/* The runtime-grid read-side linearization: idx = col * 0x80 + row
 * (490f30.c:64, pitch MG_ROW_PITCH — the same addressing map_grid writes). */
uint32_t map_render_grid_index(int32_t col, int32_t row);

/*
 * Read region-A sub-slot `slot` (0..3) of cell (col,row) from the runtime grid
 * and fill `out` with the draw-node geometry FUN_00490f30 would emit.  Returns
 * 1 when the sub-slot is populated (bank != 0) — `out` is filled — or 0 for an
 * empty sub-slot, in which case retail emits no node and `out` is left
 * untouched.  `grid` is a map_grid buffer (dims must already be set so the
 * caller's window is valid); this routine itself reads only region A.
 */
int map_render_tile(const uint8_t *grid, int32_t col, int32_t row, int slot,
                    mr_tile *out);

/*
 * Sprite-resolver callback.  FUN_00490f30 turns a tile's region-A frame id
 * (`+0x2`) into a cel pointer via 0x418470(frame) and only emits a draw
 * node when that pointer is non-NULL.  The engine sprite manager
 * (&DAT_008a760c / 0x418470) is an engine global, so the pure walk takes
 * it as a callback (the mg_bank_dims_fn pattern).  Return the sprite handle
 * stored in the draw node's +0x00, or 0 to skip the tile (retail's `iVar4 != 0`
 * gate).  `bank` (region-A +0x0) is supplied too, since the engine uses it to
 * pick the atlas/palette slot.
 */
typedef uint32_t (*mr_sprite_fn)(uint16_t bank, uint16_t frame, void *ud);

/*
 * Walk the visible cell window (map_render_visible_window) and emit one draw
 * node per populated region-A sub-slot into `pool` — the pure backdrop-tile
 * core of FUN_00490f30's main loop (490f30.c:55-229).  Rows are the outer
 * axis, columns the inner, matching retail.  For each populated sub-slot:
 * resolve the sprite (skip when the resolver returns 0), append a node via
 * draw_pool_emit with layer = the sub-slot's region-A `+0x4`, mode 3, dst =
 * tile world origin, then stamp the 0x20x0x20 source rect at the sub-tile
 * offset (map_render_tile).  Returns the number of nodes emitted.
 *
 * DEFERRED (the engine-coupled rest of 490f30): the difficulty/time palette
 * tint (DAT_008a93fc / 0x4182d0) — it recolors the sprite's pixels, not the
 * geometry; and the per-cell region-C blend/overlay arms (the `0x1b58d` /
 * `0x1b5ab` objects, 490f30.c:230-282) drawn through 0x417c40 / 0x48c6b0.
 */
int map_render_walk(const uint8_t *grid, const mr_camera *cam,
                    int32_t dim0, int32_t dim1,
                    draw_pool *pool, mr_sprite_fn resolve, void *ud);

#endif /* OSS_MAP_RENDER_H */
