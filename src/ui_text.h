/*
 * ui_text.{c,h} — the UI toolkit's text layer on the retail layout (rhost-verified inside retail): a grid
 * cell's glyph records, a text control's %-code parse + word wrap, its key-glyph table (the "@@ZK"-style
 * tokens → a key icon) and the message box's cursor.  Plan + object model: docs/plans/ui-toolkit.md.
 *
 *   FUN_0040fa00  grid_cell_text      FUN_004031c0  glyph_recs_set      FUN_0040f040  text_parse
 *   FUN_0040e5e0  text_wrap           FUN_00413ed0  text_layout_set     FUN_0040e360  ctrl_text_set
 *   FUN_0043cf30  ctrl_keys_alloc     FUN_00411230  ctrl_key_add        FUN_0043cf90  ctrl_keys_expand
 *   FUN_00410610  sys_msg_keys        FUN_00410560  sys_box_cursor
 *
 * Objects (the text control's node at +0x170 = T, see ctrl_text_init 0x40df40):
 *   T {+0 W, u16 +4, u16 +6, u16 +8 the wrap width (bytes), u16 +0xa, +0xc.. metrics, +0x1c}
 *   W {+0 the positions (12 B each: u16 +0 column, u16 +2 line, +4 the %m value, u8 +8 forced break (%n),
 *      u8 +9 the break class: 1 '・', 2 ',' / '、', 3 sentence end, 4 space / closing bracket, 5 %w),
 *      u16 +4 cap, +8 the plain text, +0xc G}
 *   G {+0 the glyph records (0x24 B: the 1- or 2-byte glyph + NUL, u16 +0x1c ruby-consumed, +0x20 ruby
 *      offset), +4 cap, u16 +8 count, u16 +0xa byte length} — a grid cell's own G has cap 80.
 * The ruby (furigana) pass of 0x4031c0 / 0x40fa00 walks the 2,267-word table at 0x5cd978; its context
 * test 0x4034f0 and reading writer 0x4051d0 stay slots (Japanese text only: EN text never matches).
 */
#ifndef OSS_UI_TEXT_H
#define OSS_UI_TEXT_H

#include <stdint.h>

/* G: glyph records from `text` (from the end of the current ones when `append`), then the ruby pass */
void glyph_recs_set(uint8_t *g, const char *text, int32_t append);                      /* 0x4031c0 */
/* a grid cell's text: the cell's G (made on first use), its records + the ruby pass */
void grid_cell_text(uint8_t *c, int32_t row, int32_t col, const char *text);            /* 0x40fa00 */
/* W: strip the %-codes (%mN% value, %n break, %w class 5, any other %x dropped) into W+8 */
char *text_parse(uint8_t *w, const char *text);                                         /* 0x40f040 */
/* W: column / line / break class per glyph, wrapping words at `width` bytes; returns G */
uint8_t *text_wrap(uint8_t *w, uint16_t width);                                         /* 0x40e5e0 */
/* W: parse + records + wrap (nothing when the text does not fit W's cap: returns the cap) */
intptr_t text_layout_set(uint8_t *w, const char *text, uint16_t width);                 /* 0x413ed0 */
/* a text control: expand its key tokens, then lay the text out at the node's width; rewinds the node */
intptr_t ctrl_text_set(uint8_t *c, const char *text);                                   /* 0x40e360 */
/* the key-glyph table: 100-byte entries {+0 token, +0x28 replacement, +0x50 id 10000+i (0: plain),
 * +0x54 sprite, u16 +0x58 frame, +0x5c dx, +0x60 dy} */
void *ctrl_keys_alloc(uint8_t *c, int32_t n);                                           /* 0x43cf30 */
int32_t ctrl_key_add(uint8_t *c, const char *tok, const char *rep, int32_t sprite, int16_t frame,
                     int32_t dx, int32_t dy);                                           /* 0x411230 */
char *ctrl_keys_expand(uint8_t *c, const char *text, char *out);                        /* 0x43cf90 */
/* the system's (0x8a6b60) message-text set-up of a text control: margins, line pad, the key table */
int32_t sys_msg_keys(void *sys, int32_t unused, uint8_t *c);                            /* 0x410610 */
/* the message box's 4-frame "more" cursor at the window's bottom-right corner (+dx, +dy) */
uint8_t *sys_box_cursor(void *sys, const uint8_t *win, uint8_t *c, int32_t dx, int32_t dy); /* 0x410560 */

#endif /* OSS_UI_TEXT_H */
