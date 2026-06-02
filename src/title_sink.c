/*
 * src/title_sink.c — the title-scene render sink (see title_sink.h).
 *
 * Translates the title scene's TITLE_DRAW_* command stream into the retail
 * render-half's actual ZDD calls.  Pure C, Win32-free: every leaf it calls
 * (ar_pool_get_slot, ar_sprite_slot_frame, title_draw_sprite[_level],
 * title_compositor_draw, zdd_object_clear) is already ported; the present
 * + log + deferred alpha draws go through ctx callbacks.
 */

#include <stddef.h>
#ifdef SINK_RESOLVE_DEBUG
#include <stdio.h>
#endif

#include "title_sink.h"
#include "asset_register.h"  /* ar_pool_get_slot / ar_sprite_slot_frame */
#include "zdd.h"             /* zdd_object_clear */

/* Bound runtime context.  g_bound==0 ⇒ unbound ⇒ every command no-ops.
 * Stored by value so the caller's stack ctx need not outlive the bind. */
static title_sink_ctx g_ctx;
static int            g_bound;

void title_sink_bind(const title_sink_ctx *ctx)
{
    if (ctx == NULL) {
        g_bound = 0;
        return;
    }
    g_ctx   = *ctx;
    g_bound = 1;
}

/* Resolve one frame surface out of a title sprite bank (pool slot).
 * Returns NULL when the pool slot is empty (unregistered) or the frame is
 * not yet decoded (no ar_frame_build_hook) — the faithful no-op path. */
static zdd_object *resolve_frame(uint16_t bank_idx, int32_t frame_id)
{
    ar_sprite_slot *slot = ar_pool_get_slot(bank_idx);
    if (slot == NULL) return NULL;
    zdd_object *r = (zdd_object *)ar_sprite_slot_frame(slot, (uint16_t)frame_id);
#ifdef SINK_RESOLVE_DEBUG
    {
        static int seen[64];
        int key = (bank_idx & 31) | ((r != NULL) << 5);
        if (key < 64 && !seen[key]) {
            seen[key] = 1;
            fprintf(stderr, "[sink] resolve_frame bank=%u id=%d -> %p "
                    "(slot.resid=0x%x frames=%p)\n", bank_idx, frame_id,
                    (void *)r, slot->resource_id,
                    (void *)(slot->entries ? slot->entries[0].frames : NULL));
        }
    }
#endif
    return r;
}

void title_render_sink(const title_draw_cmd *cmd)
{
    zdd_object *primary;
    zdd_object *frame;

    if (!g_bound || cmd == NULL) return;
    primary = g_ctx.primary;
    if (primary == NULL) return;   /* no surfaces yet ⇒ faithful no-op */

    switch (cmd->op) {
    case TITLE_DRAW_SURFACE_RESET:
        /* 0x5b9410 — clear the primary surface to its key/zero. */
        zdd_object_clear(primary);
        break;

    case TITLE_DRAW_SURFACE_CLEAR:
        /* Retail 0x5b9b70 direct: a plain color-keyed Blt of the source
         * frame onto the primary.  `asset` is the frame index (0 = the
         * phase-2..3 background, 1/2 = the logo handler's alpha-0 frame).
         * Routed through title_draw_sprite — the identical 0x5b9b70 leaf,
         * via the 0x56c610 wrapper — so the keyed-blit hook observes it. */
        frame = resolve_frame(AR_SPR_TITLE_MAIN, cmd->asset);
        if (frame != NULL)
            title_draw_sprite(primary, frame, 0, 0);
        break;

    case TITLE_DRAW_SPRITE:
        /* 0x56c610(obj, primary, frame, 0, 0) — plain keyed blit. */
        frame = resolve_frame(AR_SPR_TITLE_MAIN, cmd->asset);
        if (frame != NULL)
            title_draw_sprite(primary, frame, 0, 0);
        break;

    case TITLE_DRAW_SPRITE_LEVEL:
        /* 0x56c4e0(obj, primary, frame, level, 1000, 0, 0) — fade-levelled
         * via the 0x8a9308 ramp (ramp_b).  level numerator = cmd->level,
         * divisor 1000, x=y=0 (retail pushes). */
        frame = resolve_frame(AR_SPR_TITLE_MAIN, cmd->asset);
        if (frame != NULL)
            title_draw_sprite_level(primary, frame, cmd->level, 1000,
                                    0, 0, g_ctx.ramp_b);
        break;

    case TITLE_DRAW_FRAME_END:
        /* 0x56c180 — compose the scene's sprite-group display list onto the
         * primary, blending through the 0x8a92b8 ramp (ramp_a).  An absent
         * (NULL) group composes nothing — faithful for an unpopulated list. */
        if (g_ctx.compose_group != NULL)
            title_compositor_draw(g_ctx.compose_group, primary, g_ctx.ramp_a);
        break;

    case TITLE_DRAW_FLIP:
        /* 0x5b8fc0 — present (zdd_present), via the ctx thunk. */
        if (g_ctx.present != NULL)
            g_ctx.present(g_ctx.user);
        break;

    case TITLE_DRAW_LOG_FLIPPING:
        if (g_ctx.log_flip != NULL)
            g_ctx.log_flip(g_ctx.user);
        break;

    /* ── deferred alpha-ramp draws (see title_sink.h) ── */
    case TITLE_DRAW_LOGO:
        if (g_ctx.draw_logo != NULL)
            g_ctx.draw_logo(cmd, g_ctx.user);
        break;
    case TITLE_DRAW_SPARKLE:
        if (g_ctx.draw_sparkle != NULL)
            g_ctx.draw_sparkle(cmd, g_ctx.user);
        break;
    case TITLE_DRAW_MENU_CURSOR:
        if (g_ctx.draw_cursor != NULL)
            g_ctx.draw_cursor(cmd, g_ctx.user);
        break;

    default:
        break;
    }
}
