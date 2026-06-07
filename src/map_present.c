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
                present_blit_fn blit, void *ud,
                present_dims_fn dims, void *dims_ud, int *out_deferred)
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
            case 0: {
                /* The opaque ACTOR/sprite path (FUN_0048eac0 case 0).  Same
                 * projection as mode 3 but the cull box is the CEL's pixel size
                 * (retail reads cel +0x1c/+0x20); offx/offy are node
                 * param6/param7 and the world pos is node dst_x/dst_y.  Without
                 * a dims callback we can't form the cull box, so defer. */
                if (!dims) { deferred++; break; }

                int32_t cw = 0, ch = 0;
                dims(n->sprite, &cw, &ch, dims_ud);

                int32_t dx, dy;
                if (!map_present_project(cam, n->dst_x, n->dst_y,
                                         (int32_t)n->param6, (int32_t)n->param7,
                                         cw, ch, &dx, &dy))
                    break;   /* culled — retail blits nothing */

                if (blit) {
                    present_op op;
                    /* Mode 0 -> FUN_005b9b70 (whole-sprite color-keyed blit at
                     * the projected pos; the keyed primitive uses the cel's own
                     * metric_b8/bc, so no src rect is carried). */
                    op.kind   = PRESENT_KEYED;
                    op.layer  = li;
                    op.sprite = n->sprite;
                    op.dst_x  = dx;
                    op.dst_y  = dy;
                    op.src_x  = 0;
                    op.src_y  = 0;
                    op.w      = cw;
                    op.h      = ch;
                    op.node   = n;
                    blit(&op, ud);
                }
                presented++;
                break;
            }
            case 1: {
                /* The ALPHA actor/particle path (FUN_0048eac0 case 1) — the
                 * +0x13e0 fountain particles (engine-quirk #87).  Same cull/
                 * projection as mode 0 (the cel's pixel size), but the blit is
                 * alpha-blended: the node's param8 carries the brightness ramp
                 * index (g_ramp_a[10 - sub_phase]) for the present to orchestrate.
                 * Partially retires PORT-DEBT(present-actor-modes). */
                if (!dims) { deferred++; break; }

                int32_t cw = 0, ch = 0;
                dims(n->sprite, &cw, &ch, dims_ud);

                int32_t dx, dy;
                if (!map_present_project(cam, n->dst_x, n->dst_y,
                                         (int32_t)n->param6, (int32_t)n->param7,
                                         cw, ch, &dx, &dy))
                    break;   /* culled */

                if (blit) {
                    present_op op;
                    op.kind   = PRESENT_ALPHA;
                    op.layer  = li;
                    op.sprite = n->sprite;
                    op.dst_x  = dx;
                    op.dst_y  = dy;
                    op.src_x  = 0;
                    op.src_y  = 0;
                    op.w      = cw;
                    op.h      = ch;
                    op.node   = n;
                    blit(&op, ud);
                }
                presented++;
                break;
            }
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
                /* Modes 1 (alpha) / 2 (scaled) — the translucent/scaled draws.
                 * Their geometry reads paint_ctx/palette internals beyond the
                 * cull (sprite +0xc..+0x28, the DAT_008a9274 table); visit them
                 * in faithful order but defer.  (Mode 0 is handled above.)
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
