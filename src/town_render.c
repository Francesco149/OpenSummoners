/*
 * town_render.c — see town_render.h.  Composes the already-ported, individually
 * host-tested backdrop stages; holds no engine logic of its own beyond running
 * them in the 0x48c150 per-frame order.
 */
#include "town_render.h"

#include <stdlib.h>
#include <string.h>

int town_render_load(town_render *tr, const uint8_t *map_bytes, size_t len,
                     mg_bank_dims_fn dims, void *dims_ctx,
                     int parallax_p2, int parallax_p3)
{
    if (tr == NULL) return -1;
    memset(tr, 0, sizeof *tr);

    /* FUN_00587970 — parse the locked DATA resource. */
    if (map_data_parse(&tr->map, map_bytes, len) != 0)
        return -1;

    /* The runtime render grid (0x587e00's in_ECX) + the 27-layer draw pool the
     * walk emits into (0x586010:510-650's view+0x54 table). */
    tr->grid = map_grid_alloc();
    if (tr->grid == NULL) {
        map_data_free(&tr->map);
        return -1;
    }
    if (draw_pool_init(&tr->pool) != 0) {
        map_grid_free(tr->grid);
        tr->grid = NULL;
        map_data_free(&tr->map);
        return -1;
    }

    /* FUN_00587e00 — decode the parsed cells into the runtime grid (once). */
    map_decode(&tr->map, tr->grid, dims, dims_ctx);

    /* The 0x587e00 prologue's parallax-bank selection for this room (the
     * front-header slice map_decode otherwise defers).  param_2 = room[0x44],
     * param_3 = room[0x43] — now passed in from the active room's registry
     * record (game_world_room_render_cfg) rather than hardcoded to the town's
     * (4,1).  The town/house resolve to (4,1) (identical to the old constant),
     * the errands room to (9,4).  (PORT-DEBT ingame-nontile-layers narrows: the
     * real param_3 is the scene field local_918, not room[0x43] — but room[0x43]
     * normalizes the same for the town/house/errands; revisit with the capture.)
     * Write the descriptor into the grid front-header so its bytes match retail. */
    parallax_select(parallax_p2, parallax_p3, &tr->parallax);
    parallax_to_grid(tr->grid, &tr->parallax);

    tr->loaded = 1;
    return 0;
}

int town_render_parallax(const town_render *tr, const mr_camera *cam,
                         parallax_blit_fn blit, void *ctx)
{
    if (tr == NULL || !tr->loaded) return 0;
    /* FUN_00490cd0 — drawn first in 0x48c150 (behind the tilemap). */
    return parallax_render(&tr->parallax, cam, blit, ctx);
}

int town_render_step(town_render *tr, const mr_camera *cam,
                     mr_sprite_fn resolve, void *resolve_ctx,
                     present_blit_fn blit, void *blit_ctx,
                     int *out_deferred)
{
    /* Tile-only: no actor seam, no cel-dims callback (any mode-0/1/2 node
     * visited is deferred + counted via out_deferred). */
    return town_render_step_ex(tr, cam, resolve, resolve_ctx, blit, blit_ctx,
                               NULL, NULL, NULL, NULL, out_deferred);
}

int town_render_step_ex(town_render *tr, const mr_camera *cam,
                        mr_sprite_fn resolve, void *resolve_ctx,
                        present_blit_fn blit, void *blit_ctx,
                        present_dims_fn dims, void *dims_ctx,
                        town_actor_walk_fn actors, void *actors_ctx,
                        int *out_deferred)
{
    if (out_deferred != NULL) *out_deferred = 0;
    if (tr == NULL || !tr->loaded) return 0;

    /* The per-frame draw driver 0x48c150:
     *   :18   draw_pool_reset    — begin the frame's draw list
     *   :108  map_render_walk    — FUN_00490f30(view, 1): emit visible tiles
     *   (actor emitters run HERE, between the tile walk and the present, so
     *    actor + tile nodes interleave by layer in the single present pass)
     *   :109  map_present        — FUN_0048eac0: flush the 27 layers to blits
     */
    draw_pool_reset(&tr->pool);
    map_render_walk(tr->grid, cam,
                    (int32_t)tr->map.dim0, (int32_t)tr->map.dim1,
                    &tr->pool, resolve, resolve_ctx);
    if (actors != NULL)
        actors(&tr->pool, cam, actors_ctx);
    return map_present(&tr->pool, cam, blit, blit_ctx, dims, dims_ctx,
                       out_deferred);
}

void town_render_free(town_render *tr)
{
    if (tr == NULL) return;
    if (tr->grid != NULL) {
        map_grid_free(tr->grid);
        tr->grid = NULL;
    }
    draw_pool_free(&tr->pool);
    map_data_free(&tr->map);
    tr->loaded = 0;
}
