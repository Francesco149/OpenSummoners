/*
 * src/title_sink.h — the title-scene render sink.
 *
 * The runtime bridge that turns the title scene's abstract draw-command
 * stream (title_render_step → title_render_sink_hook, see title_scene.h)
 * into real ZDD blits.  Install with
 *
 *     title_sink_bind(&ctx);
 *     title_render_sink_hook = title_render_sink;
 *
 * and every TITLE_DRAW_* command title_render_step emits becomes the
 * corresponding engine call from the retail render half (FUN_0056aea0's
 * 0x56bb04..0x56bf1a), targeting the bound primary surface.
 *
 * ─── how the retail render half resolves sprites ─────────────────────
 *
 * Decoded from the render-half disasm (r2, this checkpoint): every
 * per-phase draw resolves its source frame out of ONE of two fixed sprite
 * banks, then blits it onto the primary surface DAT_008a93cc->[0x16c]:
 *
 *   - the MAIN title bank   = the ar_sprite_slot* at retail 0x8a7658
 *     = pool slot 19 (0x8a7640 main base + 6*4) → ar_pool_get_slot(19).
 *     Carries: the studio/title logos (frames 1/2), the press-button
 *     sprites (frames 2/3/4), the sparkle (frames 4/5), and the menu
 *     background + menu sprite (frames 5/6).  The TITLE_DRAW_*'s `asset`
 *     field is the frame id passed to ar_sprite_slot_frame (FUN_00418470).
 *   - the CURSOR bank       = the ar_sprite_slot* at retail 0x8a765c
 *     = pool slot 20 → ar_pool_get_slot(20).  Carries the menu-selection
 *     highlight sprites; the frame id is the selected row index.
 *
 * Both banks self-decode on first frame access via ar_sprite_slot_frame
 * (the ar_sprite_decode_hook chain, ckpt 20) — but only once the slot is
 * registered with a real "DATA" resource AND the per-cell surface builder
 * (8d, ar_frame_build_hook) is wired.  Until then ar_sprite_slot_frame
 * returns NULL and every sprite op here is a faithful no-op — exactly the
 * "still-undecoded" path, so the drive renders a cleared+flipped window
 * with no sprites yet (move B in HANDOFF.md).
 *
 * ─── what this sink handles vs. defers ───────────────────────────────
 *
 * Faithfully + host-tested: TITLE_DRAW_SURFACE_RESET / SURFACE_CLEAR /
 * SPRITE / SPRITE_LEVEL / FRAME_END / FLIP / LOG_FLIPPING — the whole
 * intro path, the menu background + menu sprite, and the fade-out.
 *
 * Deferred to the drive checkpoint (routed through optional ctx callbacks,
 * no-op by default): TITLE_DRAW_LOGO, TITLE_DRAW_SPARKLE.  The two encode a
 * blend-descriptor POINTER in the 32-bit `alpha` field (retail 0x448c80's
 * ramp result), which cannot round-trip on a 64-bit host.  TITLE_DRAW_MENU_
 * CURSOR is now wired directly (cmd->level = level_num = the pulsing
 * menu_fade, cmd->alpha = level_div = 0x4b0); the draw_cursor callback
 * remains a fallback for when the CURSOR bank is unresolved.  These are
 * alpha-ramp draws
 * that only fire once the ramp tables are populated at run time (never at a
 * cold headless boot), so deferring them costs no intro/menu-background
 * fidelity — they get wired + validated against live goldens in the drive
 * checkpoint, where the command stream can be enriched if needed.
 */
#ifndef OPENSUMMONERS_TITLE_SINK_H
#define OPENSUMMONERS_TITLE_SINK_H

#include "title_scene.h"   /* title_draw_cmd / title_render_sink_fn */
#include "title_render.h"  /* title_sprite_group / the wrappers / zdd types */

/* The two title sprite banks, as asset-pool indices (ar_pool_get_slot).
 * Retail addresses 0x8a7658 / 0x8a765c → main pool base 0x8a7640 + 6*4 / +7*4. */
#define AR_SPR_TITLE_MAIN   19   /* 0x8a7658 — logos / press-btn / sparkle / menu */
#define AR_SPR_TITLE_CURSOR 20   /* 0x8a765c — menu-selection highlight           */

/* The runtime context the sink reads.  Bind once after DDraw init; pass
 * NULL to title_sink_bind to unbind (every command becomes a no-op). */
typedef struct title_sink_ctx {
    /* The blit dest — retail DAT_008a93cc->primary_obj (the ZDD's +0x16c).
     * NULL ⇒ every draw is a no-op (faithful pre-DDraw-init behaviour). */
    zdd_object *primary;

    /* The compositor's blend-descriptor ramp (retail 0x8a92b8) and the
     * level/logo ramp (retail 0x8a9308), each a 20-entry pointer table.
     * NULL is tolerated (all alpha resolves to NULL ⇒ plain/no-op blits),
     * which is the static-image reality until DDraw/asset init fills them. */
    const zdd_blend_desc *const *ramp_a;   /* 0x8a92b8 — compositor + cursor */
    const zdd_blend_desc *const *ramp_b;   /* 0x8a9308 — sprite-level + logo */

    /* The scene's sprite-group display list, blitted by TITLE_DRAW_FRAME_END
     * (the compositor FUN_0056c180).  NULL ⇒ the compose step is skipped
     * (an unpopulated display list composes nothing — faithful). */
    const title_sprite_group *compose_group;

    /* TITLE_DRAW_FLIP → present the frame (retail FUN_005b8fc0 = zdd_present).
     * Supplied as a callback so the sink stays Win32-free and host-testable;
     * the drive passes a thunk that calls zdd_present(g_zdd). */
    void (*present)(void *user);

    /* TITLE_DRAW_LOG_FLIPPING → the "Title Menu - Flipping" log marker.
     * NULL ⇒ skipped. */
    void (*log_flip)(void *user);

    /* Deferred alpha-ramp draws (see the header note).  Each receives the
     * raw command + the bound ctx user pointer; NULL ⇒ that op is a no-op. */
    void (*draw_logo)(const title_draw_cmd *cmd, void *user);    /* 0x494e10 */
    void (*draw_sparkle)(const title_draw_cmd *cmd, void *user); /* 0x56c580 */
    void (*draw_cursor)(const title_draw_cmd *cmd, void *user);  /* 0x56c470 */

    void *user;   /* opaque, passed to every callback above */
} title_sink_ctx;

/* Bind the runtime context (copied by value into module state).  Pass NULL
 * to unbind — after which title_render_sink is an all-no-op.  Not
 * thread-safe (the title scene is single-threaded, as in retail). */
void title_sink_bind(const title_sink_ctx *ctx);

/* The render sink.  Install into title_render_sink_hook.  Maps one
 * title_draw_cmd to its retail render-half call against the bound ctx.
 * A no-op when unbound, when ctx.primary is NULL, or (for sprite ops) when
 * the resolved frame surface is NULL (bank not yet decoded). */
void title_render_sink(const title_draw_cmd *cmd);

#endif /* OPENSUMMONERS_TITLE_SINK_H */
