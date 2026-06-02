/*
 * src/glyph_render.h — the GDI text renderer (the *render* half of the
 * cell-grid dynamic-text system).
 *
 * The build half (glyph_text.{c,h}) turns a string into a cell's obj0
 * glyph_buf of per-glyph records; this module walks the menu object's
 * rows × columns and draws each cell's glyph records with Win32 GDI
 * (SelectObject + SetTextColor + TextOutA).  See
 * docs/findings/text-glyph-pipeline.md for the whole-subsystem map and
 * src/menu_list.h for the container model (menu_node / menu_ctrl, the
 * descriptor at +0x174, the per-column `entries` at +0x178, the `rows` at
 * +0x17c, and each cell's obj0 glyph buffer).
 *
 * Ported functions (one per address):
 *   - FUN_0048e860   glyph_row_draw    (per-glyph TextOutA loop, 7px advance)
 *   - FUN_0048e6d0   glyph_ruby_draw   (furigana/ruby annotation loop)
 *   - FUN_0048e200   glyph_grid_render (the row×col walk + colour selection +
 *                                       drop-shadow pre-pass; GDI mode)
 *
 * GDI is reached through a tiny injected `glyph_gdi_ops` vtable so the walk
 * and colour selection are pure and host-testable with a recording stub
 * (no Win32 surface); the real back-buffer adapter is `glyph_gdi_ops_win32`
 * (Win32 builds only).  The renderer's sprite-cell mode (0x48e200's mode==0
 * branch, which blits font-texture frames instead of TextOutA) is a separate
 * path that needs ZDD sprite calls + the cell sub-widgets; it is deferred —
 * see the .c.
 */
#ifndef OPENSUMMONERS_GLYPH_RENDER_H
#define OPENSUMMONERS_GLYPH_RENDER_H

#include <stdint.h>

#include "glyph_text.h"   /* glyph_buf / glyph_record */
#include "menu_list.h"    /* menu_node / menu_list_hdr / menu_row / menu_cell */

/* ─── glyph_gdi_ops — the injected GDI primitives ────────────────────────
 *
 * The three GDI calls 0x48e200/0x48e860/0x48e6d0 make, abstracted so the
 * pure walk can be host-tested against a recording stub.  `user` is an
 * opaque cookie threaded into every call (the back-buffer HDC in the real
 * adapter; a recorder in tests).  Fonts are opaque HGDIOBJ handles. */
typedef struct glyph_gdi_ops {
    void (*select_font)(void *user, void *font);             /* SelectObject  */
    void (*set_text_color)(void *user, uint32_t color);      /* SetTextColor  */
    void (*text_out)(void *user, int x, int y,
                     const char *str, int len);              /* TextOutA      */
    void  *user;
} glyph_gdi_ops;

/* ─── FUN_0048e860 — draw one cell's glyph records in a row ───────────────
 *
 * For `count` records starting at index `start` (count==0 or an over-long
 * count means "to the end of the buffer"), TextOutA each record's `ch`
 * string at (x + accumulated, y), advancing the pen by `advance` px per
 * source byte of the record (so a 2-byte SJIS record advances 2·advance).
 * The currently-selected font/colour are used; this only emits text_out.
 * Faithful to 0x48e860 (the inner strlen-then-TextOutA loop). */
void glyph_row_draw(const glyph_buf *buf, const glyph_gdi_ops *gdi,
                    int x, int y, int advance, uint16_t start, uint16_t count);

/* ─── FUN_0048e6d0 — draw a cell's ruby (furigana) annotations ────────────
 *
 * The second, ruby-only TextOutA pass 0x48e200 runs when node->field_14 is
 * set.  For each main glyph record carrying ruby data (record->flag1c != 0,
 * the ruby-char count), it draws the packed ruby substrings above the glyph
 * (two TextOutA's per ruby char, offset by advance/2).  The ruby substrings
 * are packed into the glyph_record scratch area by the (still-unported)
 * escape expander, so this pass is a no-op for raw text (flag1c == 0) and is
 * unexercised by the current scenes; it is a faithful translation of
 * 0x48e6d0 against that ground truth, ready for when ruby text lands. */
void glyph_ruby_draw(const glyph_buf *buf, const glyph_gdi_ops *gdi,
                     int x, int y, int advance, uint16_t start, uint16_t count);

/* ─── FUN_0048e200 (GDI branch) — render the menu object's text grid ──────
 *
 * Walk `node`'s rows [sel2 .. min(sel2+stride, count)) × columns
 * [0 .. alloc_b), and for each cell with a built glyph buffer:
 *   1. position it at (entry.pos + x + node.field_c,
 *                      entry.field4 + node.field_10 + lineH·dispRow + y),
 *      where lineH = node->field_1ac;
 *   2. pick text/shadow COLORREFs from the node's display config (+0x180..)
 *      by selection state — disabled row (flag8==0), focused row
 *      (hdr->cursor==row), normal, or a per-entry / per-cell override;
 *   3. when `entry.field_c == 1`, right-align within the column (shift by
 *      max(0, entry.extent − 7·len));
 *   4. draw a 2-copy drop shadow (offsets (0,+1) and (+1,0)) in the shadow
 *      colour with `font_main`, then the glyphs in the text colour;
 *   5. when node->field_14 != 0, draw the ruby pass in `font_shadow`.
 *
 * `x`/`y` are the parent node's base position (0x48c820 passes the parent's
 * field_c/field_10).  GDI mode only — the caller selects TRANSPARENT bk mode
 * and passes the registered HFONTs.  Faithful to 0x48e200's param_1 != 0
 * branch. */
void glyph_grid_render(const menu_node *node, const glyph_gdi_ops *gdi,
                       int x, int y, void *font_main, void *font_shadow);

/* Real back-buffer adapter: bind `glyph_gdi_ops` to GDI on `hdc` (an HDC,
 * passed opaquely so this header stays Win32-free for the host harness).
 * The caller is responsible for SetBkMode(hdc, TRANSPARENT).  DEFINED in
 * glyph_render_win32.c (real build); the host test harness provides its own
 * recording ops and does NOT link that file. */
glyph_gdi_ops glyph_gdi_ops_win32(void *hdc);

#endif /* OPENSUMMONERS_GLYPH_RENDER_H */
