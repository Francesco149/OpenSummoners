/*
 * src/newgame_box.h — the new-game config scene's 9-slice box panel render
 * (the chrome behind the menu + tooltip text).
 *
 * Port of FUN_0048cf80 (the tiled 9-slice box renderer; its fade-scaled twin
 * is FUN_0048cb90).  Retail composes the bordered cream panel from a 32×32-cell
 * sprite bank (PE resource 0x457 = AR_SPR_FONT_TEX_457, already registered by
 * ar_register_fonts) as nine slices — corners, edges, and a tiled center fill:
 *
 *     ┌─tl─┬──top──┬─tr─┐      frame ids in the bank (quirk #67):
 *     │    │       │    │        tl 0  top 1  tr 2
 *     left center  right         left 3  center 4  right 5
 *     │    │       │    │        bl 6  bottom 7  br 8
 *     └─bl─┴─bottom┴─br─┘      each cell 32×32; center (4) is the cream
 *                              RGB(239,227,214) fill, edges 1/3/5/7 the bevel,
 *                              corners 0/2/6/8 the ornate gold filigree.
 *
 * Edges + center are TILED at the 32px cell pitch (with a partial last tile for
 * non-multiple extents), NOT stretched — matching FUN_0048cf80's
 * width/cell + width%cell loop structure.  This module ports the OPAQUE path
 * (FUN_0048cf80's param_6==0 arm → the keyed clipped blit FUN_005b9bf0 =
 * zdd_object_blt_clipped); the fade-in alpha arm (param_6!=0 → FUN_005bd550
 * with a fade desc) is deferred — the steady-state interactive box is opaque.
 *
 * The blit + frame-resolve are injected through a `newgame_box_ops` vtable so
 * the 9-slice walk is pure and host-testable with a recording stub; the real
 * adapter (ar_sprite_slot_frame + zdd_object_blt_clipped on the primary
 * surface) is wired in main.c.
 *
 * Geometry ground truth (live --box-probe, goldens/retail-newgame-box-cells.jsonl):
 *   - menu box:    rect (32,32) size 400×124
 *   - tooltip box: rect (32,392) size 576×80
 */
#ifndef OPENSUMMONERS_NEWGAME_BOX_H
#define OPENSUMMONERS_NEWGAME_BOX_H

/* The 9 slice frame ids within the box-art bank (quirk #67), in the order
 * newgame_box_render expects its `frames[9]`. */
enum {
    NEWGAME_BOX_TL = 0, NEWGAME_BOX_TOP    = 1, NEWGAME_BOX_TR = 2,
    NEWGAME_BOX_L  = 3, NEWGAME_BOX_CENTER = 4, NEWGAME_BOX_R  = 5,
    NEWGAME_BOX_BL = 6, NEWGAME_BOX_BOTTOM = 7, NEWGAME_BOX_BR = 8,
};

/* The bank's PE resource id (sotesd.dll) + the port slot it lands in. */
#define NEWGAME_BOX_BANK_RESOURCE  0x457
#define NEWGAME_BOX_CELL           32      /* corner/cell size (node+0x74/+0x78) */

/* ─── newgame_box_ops — the injected blit primitives ─────────────────────
 *
 * `frame(user, id)` resolves slice `id` of the box-art bank to an opaque
 * sprite handle (NULL → that slice is skipped, as FUN_005b9bf0 degenerates on
 * a NULL surface).  `blt(user, frame, x, y, w, h)` is the keyed clipped blit
 * (FUN_005b9bf0 / zdd_object_blt_clipped): draw `frame` at dest (x,y) with dest
 * size w×h (src sub-origin 0,0 — the frame's own placement metrics are applied
 * inside the blit).  `user` is threaded through unchanged (the ZDD primary in
 * the real adapter; a recorder in tests). */
typedef struct newgame_box_ops {
    void *(*frame)(void *user, int id);
    void  (*blt)(void *user, void *frame, int x, int y, int w, int h);
    void  *user;
} newgame_box_ops;

/* Render one 9-slice box: outer rect (x0,y0) size w×h, `frames[9]` the slice
 * ids (NEWGAME_BOX_TL..BR order), `cell` the corner/cell size (32).  Mirrors
 * FUN_0048cf80's opaque arm: the W<2·cell / H<2·cell centering, then the
 * top / full-middle / partial-middle / bottom rows, each tiled left-corner →
 * center tiles → remainder → right-corner. */
void newgame_box_render(const newgame_box_ops *ops,
                        int x0, int y0, int w, int h,
                        const int frames[9], int cell);

#endif /* OPENSUMMONERS_NEWGAME_BOX_H */
