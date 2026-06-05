/*
 * town_render.{c,h} — compose the in-game backdrop pipeline into one scene.
 *
 * Every stage of the static-tilemap render path is already a pure, host-tested
 * unit:  the map-data parse (map_data.c, FUN_00587970), the per-tile-id decode
 * into the runtime render grid (map_decode.c, the FUN_00587e00 town arms), the
 * visible-window draw-node walk (map_render.c map_render_walk, FUN_00490f30),
 * and the 27-layer present/projector flush (map_present.c, FUN_0048eac0 /
 * FUN_00490b90).  This module is the thin SCENE that owns the per-room state
 * those stages share and runs them in the engine's order, so main.c can wire
 * real town pixels with three Win32 callbacks (sprite resolve, bank dims, blit)
 * and no knowledge of the grid/pool internals.
 *
 * Engine correspondence:
 *   - town_render_load = the ONCE-per-room work: FUN_00587970 (parse the locked
 *     DATA resource) + FUN_00587e00 (decode it into the runtime grid).  The
 *     caller supplies the already-locked resource bytes (FindResource /
 *     LockResource stays Win32 in main.c) and a bank-dims callback for the
 *     bank-derived tile footprints (map_grid_emit_tile).
 *   - town_render_step = the PER-FRAME work the draw driver 0x48c150 does
 *     for the backdrop: draw_pool_reset (the "begin draw list" at 0x48c150:18),
 *     map_render_walk (FUN_00490f30(view,1) at :108 — emit the visible tiles),
 *     then map_present (FUN_0048eac0 at :109 — flush the 27 layers to blits).
 *     The camera/view object is passed in (the live-verified first-frame
 *     constant MAP_RENDER_CAM_TOWN_3F2 today; the spawn-snap + intro pan that
 *     produce it are PORT-DEBT ingame-camera-snap).
 *
 * SCOPE: the static backdrop tiles (mode-3 draw nodes) — the producers map_decode
 * + map_render_walk emit.  The actor/HUD modes (0/1/2) and the region-C
 * blend/overlay arms are deferred with their producers (PORT-DEBT
 * present-actor-modes); map_present still visits + counts them via out_deferred.
 *
 * Win32-free: the grid/pool/map are owned host allocations and the three engine
 * globals (the sprite-bank pool, the bank pixel sizes, the zdd blit) come in as
 * callbacks (the mr_sprite_fn / mg_bank_dims_fn / present_blit_fn pattern), so
 * the whole compose is pure and host-tests with synthetic data.
 */
#ifndef OSS_TOWN_RENDER_H
#define OSS_TOWN_RENDER_H

#include <stddef.h>
#include <stdint.h>

#include "map_data.h"
#include "map_grid.h"
#include "map_decode.h"
#include "map_render.h"
#include "map_present.h"
#include "draw_pool.h"
#include "parallax.h"

/* The town's 0x587e00-prologue parallax-selection params (room 210110, area
 * 0xd2): param_2 = room[0x44] = area-A = 4, param_3 = room[0x43] = area-C = 1.
 * Hardcoded for the only in-game scene today; PORT-DEBT ingame-nontile-layers
 * tracks deriving them from game_map/game_world. */
#define TOWN_RENDER_PARALLAX_P2  4
#define TOWN_RENDER_PARALLAX_P3  1

typedef struct town_render {
    map_data      map;       /* the parsed DATA resource (owned)               */
    uint8_t      *grid;      /* the decoded runtime render grid (map_grid_alloc)*/
    draw_pool     pool;      /* the 27-layer draw-node accumulator             */
    parallax_desc parallax;  /* the far-plane descriptor (grid front-header)   */
    int           loaded;    /* town_render_load succeeded                     */
} town_render;

/*
 * Build the room's static scene from `len` bytes of a locked map-data resource:
 * parse it (map_data_parse), allocate the runtime grid (map_grid_alloc), and
 * decode every cell into the grid (map_decode).  `dims`/`dims_ctx` resolve a
 * sprite bank's pixel size for the bank-derived tile footprints; NULL `dims`
 * yields degenerate 0x0 footprints (safe, but no tiles place).
 *
 * Returns 0 on success, -1 on a malformed resource / allocation failure (the
 * partial state is cleaned up — call town_render_free regardless to be safe).
 * Re-loading: call town_render_free first.
 */
int town_render_load(town_render *tr, const uint8_t *map_bytes, size_t len,
                     mg_bank_dims_fn dims, void *dims_ctx);

/*
 * Render one frame of the loaded scene through `cam`: reset the draw pool, walk
 * the visible cell window emitting one draw node per populated backdrop sub-slot
 * (`resolve` turns a tile's (bank, frame) into a cel handle; a tile is skipped
 * when it returns 0), then flush the 27 layers — handing each visible mode-3
 * node to `blit` in present order.  Returns the number of nodes blitted; if
 * `out_deferred` is non-NULL it receives the count of visited-but-deferred
 * actor/HUD nodes (modes 0/1/2 — none today, but never silently dropped).
 *
 * A no-op (returns 0) on an unloaded scene.  `blit` may be NULL (a dry walk that
 * only builds + counts the draw list).
 */
int town_render_step(town_render *tr, const mr_camera *cam,
                     mr_sprite_fn resolve, void *resolve_ctx,
                     present_blit_fn blit, void *blit_ctx,
                     int *out_deferred);

/*
 * Draw the PARALLAX far-plane (FUN_00490cd0) through `cam` — the sky/mountain
 * background BEHIND the tilemap.  Call this BEFORE town_render_step each frame
 * (it draws to the surface first; the tile present then blits over it), matching
 * 0x48c150's order (parallax at :47, tilemap walk/present at :108-109).  Reads
 * the descriptor parallax_select wrote into the scene at load.  `blit` selects a
 * bank/frame cel and blits it at (x,y) (the 0x417c40 -> FUN_005b9a40 pair).
 * Returns the number of tiles emitted; a no-op (0) on an unloaded scene.
 */
int town_render_parallax(const town_render *tr, const mr_camera *cam,
                         parallax_blit_fn blit, void *ctx);

/* Release the owned map / grid / pool.  Safe on a zeroed or never-loaded tr. */
void town_render_free(town_render *tr);

#endif /* OSS_TOWN_RENDER_H */
