/*
 * actor_render.c — see actor_render.h.  Faithful port of FUN_0044d160 (the
 * static-actor descriptor builder) and the 0x491ae0 default arm (the
 * per-actor emit) the town's 32 static main-band actors render through.
 */
#include "actor_render.h"

int actor_render_describe(const actor *a, const actor_render_state *rs,
                          const int16_t *flip_table, actor_desc *out)
{
    /* dir = actor+0xe8 — the ORIGINAL direction; the skip check below uses it
     * (an animated clip may override `dir` afterward, but not the skip test). */
    uint32_t dir = a->dir;
    int32_t  flag_abs     = 0;   /* iVar8 — clip+0x4c (0 for static)         */
    int16_t  sprite_delta = 0;   /* sVar5 — base + frame_delta[f] (0 static) */

    /* FUN_0044d160:19 — skip when this direction has no sprite bank, or the
     * render-state is inactive (*param_1 == '\0'). */
    if (a->sprite_table[dir].bank == 0 || rs->active == 0)
        return 0;

    const anim_clip *clip = rs->clip;   /* render-state +0x6c */
    if (clip == NULL) {
        /* STATIC: no per-frame placement offset, no sprite delta. */
        out->off_x = 0;
        out->off_y = 0;
    } else {
        /* ANIMATED: per-frame offset + sprite delta from the clip. */
        uint32_t f = rs->frame;                  /* +0x72 */
        if (clip->frame_delta[f] == -1)
            return 0;                            /* end-of-sequence terminator */
        flag_abs     = clip->flag_abs;           /* +0x4c                      */
        out->off_x   = clip->off_x[f];           /* +0x50                      */
        out->off_y   = clip->off_y[f];           /* +0xd0                      */
        sprite_delta = (int16_t)(clip->frame_delta[f] + clip->base_sprite);
        uint16_t link = clip->link;              /* +0x150 — overrides the dir */
        if (link != 0)
            dir = (link != 0xff) ? link : 0u;
    }

    const actor_sprite_row *row = &a->sprite_table[dir];
    uint16_t bank = row->bank;
    /* sVar3 — the mirror frame offset for this bank (DAT_008a8440[bank]).
     * Read unconditionally in retail; only USED on the mirror/angle paths, so a
     * NULL table (no mirrored actor in play) reads as 0 without dereferencing. */
    int16_t flip = (flip_table != NULL) ? flip_table[bank] : 0;
    int16_t frame_off = 0;   /* sVar6 — added to the frame */

    if (flag_abs == 0) {
        if (a->angle_anim != 0) {
            /* Angle-driven frame select (FUN_0044d160's 64-bit division idiom):
             *   frame_off = ((angle + 360000) / (360000 / div) & 0xffff) % div */
            uint32_t d     = a->angle_div;
            int32_t  denom = (int32_t)(360000 / d);
            int32_t  q     = (rs->angle + 360000) / denom;
            frame_off = (int16_t)((uint32_t)(q & 0xffff) % d);
            if (rs->facing == 3)
                frame_off = (int16_t)(frame_off + flip);
        } else if (rs->facing == 3) {
            /* MIRRORED: reflect off_x about the row's mirror reference; pick the
             * mirrored frame via the flip table. */
            out->off_x = row->mirror_x - out->off_x;
            frame_off  = flip;
        } else {
            /* NORMAL: add the row's x placement offset; frame_off stays 0. */
            out->off_x = out->off_x + row->x_off;
        }
    }
    /* else flag_abs != 0: "absolute" — no row x_off, frame_off stays 0. */

    out->off_y = out->off_y + row->y_off;   /* LAB_0044d294 */
    out->bank  = bank;
    out->alpha = 0;
    out->frame = (int16_t)(row->frame_base + frame_off + sprite_delta);
    return 1;
}

int actor_render_static(const actor *a, const actor_render_state *rs,
                        const int16_t *flip_table, draw_pool *pool,
                        mr_sprite_fn resolve, void *resolve_ctx)
{
    /* 0x491ae0:59 — actor+0x284 set => render nothing. */
    if (a->skip != 0)
        return 0;

    /* Layer = actor+0xfc, overridden by the render-state's +0x284 sub-object's
     * +0x100 field (0x491ae0:62-64).  NULL override => the actor's own layer. */
    uint32_t layer = a->layer;
    if (rs->layer_override != NULL)
        layer = *(const uint32_t *)(rs->layer_override + 0x100);

    actor_desc desc;
    if (!actor_render_describe(a, rs, flip_table, &desc))
        return 0;

    /* The default-arm emit (LAB_004923eb, one descriptor element): world pos
     * from the render-state, placement = desc offset + the render-state's dst
     * base, alpha from the actor (falling back to the descriptor's alpha). */
    int32_t node_alpha = a->node_alpha;            /* actor+0xf4 */
    int32_t off_x = desc.off_x + rs->dst_base_x;   /* + render-state +0x40 */
    int32_t off_y = desc.off_y + rs->dst_base_y;   /* + render-state +0x44 */
    /* The tile-occlusion mark (0x4927c0) is deferred — PORT-DEBT(actor-
     * occlusion); it culls backdrop tiles behind the actor, not the actor blit. */
    if (node_alpha == 0)
        node_alpha = desc.alpha;

    uint32_t cel = resolve ? resolve(desc.bank, (uint16_t)desc.frame, resolve_ctx)
                           : 0u;
    draw_node *n = draw_pool_emit_actor(pool, layer, cel,
                                        rs->world_x, rs->world_y,
                                        off_x, off_y, (uint32_t)node_alpha);
    return (n != NULL) ? 1 : 0;
}
