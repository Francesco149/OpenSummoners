/*
 * src/newgame_cursor.h — the new-game config menu's selection cursor (the
 * drooping gold vine/tendril that hangs from the box's top-left corner toward
 * the focused row).
 *
 * Faithful port of FUN_0048d940's type-1 arm — the single-cell *animated*
 * sprite render the box widget invokes after the menu text (the tail of
 * FUN_0048c820:124, `FUN_0048d940(ctx, node+0xc, node+0x10, param)`).  Retail's
 * cursor is a child sprite-cell node whose:
 *
 *   - frame is `base + frames[idx % count]` — base = node+0x2c (16), the frame
 *     list node+0x2e[] = {0,1,2,3}, count = node+0x6e (4), the animated index
 *     idx = node+0x72 → sprite frames 16/17/18/19;
 *   - blit base is
 *         x = node+0x7c + node+0xc + parent_x
 *         y = (cursor - sel2) * pitch + node+0x80 + node+0x10 + parent_y
 *     (pitch = node+0x1ac, the same 0x1c=28 row pitch the text renderer walks);
 *   - the keyed/plain branch (node+0x78==0 → FUN_005b9b70 = zdd_object_blt_keyed)
 *     then adds the frame's own metric_0c/_10 placement offset to land the vine
 *     on screen.  The fade-in alpha branch (node+0x78!=0 → FUN_005bd550) is the
 *     box fade-in polish, deferred with the box's own fade arm.
 *
 * The port does NOT build retail's box-widget sub-node tree (the box panel is
 * drawn directly in main.c, the text via glyph_grid_render on the scene node),
 * so the +0x7c/+0x80 child-offset fields have no port home.  Instead the blit
 * base is calibrated from the live --box-probe ground truth
 * (goldens/retail-newgame-box-cells.jsonl): for the menu box at (32,32) the
 * row-0 cursor base resolves to (40,26) — i.e. box origin + (8,-6) — and the
 * per-row delta is the menu pitch.  (Retail's other-row positions can't be
 * captured under the harness — Flip freezes at 422 in the modal pump before any
 * post-entry cursor move injects — so the row→Y math is ported faithfully from
 * the type-1 formula and trusted; see HANDOFF Next move #1a'.)
 *
 * The frame-resolve + blit are injected through a `newgame_cursor_ops` vtable so
 * the geometry is pure and host-testable with a recording stub; the real
 * adapter (ar_sprite_slot_frame on slot 65 + zdd_object_blt_keyed on the
 * primary) is wired in main.c.
 */
#ifndef OPENSUMMONERS_NEWGAME_CURSOR_H
#define OPENSUMMONERS_NEWGAME_CURSOR_H

#include <stdint.h>

/* The cursor sprite bank is the menu/box ornament atlas PE resource 0x455
 * (sotesd.dll) — the sibling font-texture of the box-art bank 0x457, already
 * registered into port slot 43 (AR_SPR_FONT_TEX_455) by ar_register_fonts.  It
 * is a 32×48-cell, 4×6 = 24-frame atlas; the selection cursor uses frames
 * 16..19 (node+0x2c base 16, frame list {0,1,2,3}). */
#define NEWGAME_CURSOR_BANK_SLOT      43      /* AR_SPR_FONT_TEX_455 */

/* The animation: base frame + a 4-entry frame list (node+0x2c / +0x2e[]). */
#define NEWGAME_CURSOR_BASE_FRAME     16
#define NEWGAME_CURSOR_FRAME_COUNT    4       /* frames {0,1,2,3} → 16..19 */

/* Blit base calibration from the --box-probe golden (menu box at (32,32),
 * row 0 → base (40,26)).  Expressed relative to the box origin so it tracks the
 * box rect; the per-row delta is the menu pitch (node+0x1ac = 28). */
#define NEWGAME_CURSOR_OFFSET_X       8       /* golden 40 - box 32 */
#define NEWGAME_CURSOR_OFFSET_Y       (-6)    /* golden 26 - box 32 */

/* ─── newgame_cursor_ops — the injected sprite primitives ─────────────────
 *
 * `frame(user, frame_id)` resolves frame `frame_id` of the cursor bank to an
 * opaque sprite handle (NULL → nothing drawn, as the keyed blit degenerates on
 * a NULL surface).  `blt(user, frame, x, y)` is the plain keyed blit
 * (FUN_005b9b70 / zdd_object_blt_keyed): draw `frame` at base (x,y) — the
 * frame's own metric_0c/_10 placement offset is applied inside the blit.
 * `user` is threaded through unchanged (the ZDD primary in the real adapter; a
 * recorder in tests). */
typedef struct newgame_cursor_ops {
    void *(*frame)(void *user, int frame_id);
    void  (*blt)(void *user, void *frame, int x, int y);
    void  *user;
} newgame_cursor_ops;

/* The animated frame id for animation index `anim_idx`:
 * base + frames[anim_idx % count] with the canonical {0,1,2,3} list →
 * 16 + (anim_idx & 3).  Pure; exposed for testing. */
int newgame_cursor_frame_id(int anim_idx);

/* The blit base (before the frame's metric_0c/_10 offset) for the focused row.
 * `box_x`/`box_y` are the menu box origin; `cursor`/`sel2` the list header's
 * selection + page-top rows; `pitch` the row pitch (node+0x1ac).  Pure. */
void newgame_cursor_base(int box_x, int box_y,
                         int cursor, int sel2, int pitch,
                         int *out_x, int *out_y);

/* Draw the cursor for the focused row: resolve frame (base + anim) then keyed-
 * blit at the row's base.  No-op if `ops`/`frame`/`blt` is NULL or the frame
 * resolves NULL.  Mirrors FUN_0048d940's type-1 plain-branch composition. */
void newgame_cursor_render(const newgame_cursor_ops *ops,
                           int box_x, int box_y,
                           int cursor, int sel2, int pitch,
                           int anim_idx);

#endif /* OPENSUMMONERS_NEWGAME_CURSOR_H */
