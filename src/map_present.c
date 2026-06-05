/*
 * map_present.c — see map_present.h for the full model.  Faithful port of the
 * pure walk + projection of FUN_0048eac0 (the in-game present pass) and its
 * shared projector FUN_00490b90.
 */
#include "map_present.h"

/* The engine's sub-pixel fixed-point scale: world coords are px * 100, so a
 * `/100` converts a world/camera component to screen pixels (FUN_00490b90 and
 * the case-0/3 culls all divide camera + node coords by this). */
#define MP_FP 100

int map_present_project(const mr_camera *cam,
                        int32_t world_x, int32_t world_y,
                        int32_t offx, int32_t offy,
                        int32_t cw, int32_t ch,
                        int32_t *sx, int32_t *sy)
{
    /* FUN_00490b90, param_9 == 0 arm (the only arm the present pass uses):
     *   sx = world_x/100 - (off60 + off34)/100 + offx
     *   sy = world_y/100 - (off5c + off74*100 + off4c)/100 + offy           */
    int32_t x = (world_x / MP_FP - (cam->off60 + cam->off34) / MP_FP) + offx;
    int32_t iv = cam->off5c + cam->off74 * MP_FP + cam->off4c;
    int32_t y = (world_y / MP_FP - iv / MP_FP) + offy;

    *sx = x;
    *sy = y;

    /* Four-corner viewport cull (FUN_00490b90:21-24): the node's right/bottom
     * edge must be on-screen and its origin within the viewport extent. */
    if (cw + x >= 0 && y + ch >= 0 &&
        x < cam->off64 / MP_FP && y < cam->off68 / MP_FP)
        return 1;
    return 0;
}

int map_present(const draw_pool *pool, const mr_camera *cam,
                present_blit_fn blit, void *ud, int *out_deferred)
{
    int presented = 0;
    int deferred  = 0;

    /* Outer loop over the 27 layers in order (FUN_0048eac0: local_1c <
     * view+0x58).  The layer index is the draw-order key. */
    for (uint32_t li = 0; li < DRAW_POOL_LAYERS; li++) {
        const draw_layer *layer = &pool->layers[li];

        /* Inner loop over this layer's populated nodes (local_20 <
         * layer.count).  cap-0 layers (e.g. layer 0) have count 0 and a NULL
         * array, so the loop body never runs. */
        for (uint32_t ni = 0; ni < layer->count; ni++) {
            const draw_node *n = &layer->nodes[ni];

            switch (n->mode) {
            case 3: {
                /* The static-backdrop TILE path map_render_walk emits.
                 * Project with the node's placement adjust (+0x0c/+0x10) and
                 * its own w/h as the cull box (FUN_0048eac0 case 3). */
                int32_t dx, dy;
                if (!map_present_project(cam, n->dst_x, n->dst_y,
                                         (int32_t)n->param6, (int32_t)n->param7,
                                         n->w, n->h, &dx, &dy))
                    break;   /* culled — retail blits nothing */

                if (blit) {
                    present_op op;
                    /* node +0x14 selects the blit: 0 -> clipped color-key
                     * (FUN_005b9bf0), else -> alpha orchestrate (FUN_005bd550). */
                    op.kind   = (n->param8 != 0) ? PRESENT_ALPHA : PRESENT_CLIPPED;
                    op.layer  = li;
                    op.sprite = n->sprite;
                    op.dst_x  = dx;
                    op.dst_y  = dy;
                    op.src_x  = n->src_x;
                    op.src_y  = n->src_y;
                    op.w      = n->w;
                    op.h      = n->h;
                    op.node   = n;
                    blit(&op, ud);
                }
                presented++;
                break;
            }
            default:
                /* Modes 0/1/2 — the actor/sprite/scaled draws.  No ported
                 * producer emits them yet and their geometry reads engine
                 * sprite internals; visit them (faithful order) but defer.
                 * PORT-DEBT(present-actor-modes, docs/port-debt.md). */
                deferred++;
                break;
            }
        }
    }

    if (out_deferred)
        *out_deferred = deferred;
    return presented;
}
