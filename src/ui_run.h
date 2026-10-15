/*
 * ui_run.{c,h} — the UI toolkit's runtime on the retail layout (rhost-verified inside retail): the controls'
 * key handling (list cursor + key repeat, the message advance), their per-frame tick (the typewriter, the
 * icon / arrow / cursor animations), the message window's update (confirm / cancel / skip, the typing blip,
 * the auto-advance), the list input handler, and the menu frame loop every menu runs on (pace, input, window
 * ticks, the world, render + Flip).  Plan: docs/plans/ui-toolkit.md (chip 3b).
 *
 *   FUN_0043c610  icon_tick           FUN_0043c920  text_key            FUN_0043ca40  grid_nav
 *   FUN_0043ce50  ctrl_key            FUN_0043c650  ctrl_tick           FUN_0043c2e0  win_tick
 *   FUN_0043b980  msg_update          FUN_0043bca0  ctrl_input          FUN_0043c170  jobs_busy
 *   FUN_0055f550  menu_pace           FUN_0055f190  menu_run
 *
 * Keys (the control-level codes): 0 / 1 up / down, 2 / 3 page back / on, 4 / 6 held down / up (repeat after
 * 300 ms, then every 100), 5 / 7 their release, 8 skip, 9 confirm, 10 cancel.  Results: grid_nav 1 moved,
 * 2 page turned, 3 confirmed, 4 cancelled; the text control 5 glyph shown, 6 page / all revealed, 7 waiting
 * to auto-close, 8 done.
 */
#ifndef OSS_UI_RUN_H
#define OSS_UI_RUN_H

#include <stdint.h>

#include "input.h"

/* an icon / cursor record's frame clock (u16 +0x46 frames, +0x48 rate, +0x4a frame, +0x4c timer) */
uint8_t *icon_tick(uint8_t *ic);                                                       /* 0x43c610 */
/* a text control's advance key (8..10) */
int32_t text_key(uint8_t *c, int32_t key);                                             /* 0x43c920 */
/* a grid's cursor key (the grid info: +0 mode 0 page-wrap / 2 paged / 3 scrolling / else wrap, +0xc page,
 * +0x10 rows, +0x14 cursor, +0x18 page top, +0x1c / +0x20 the held-key repeat deadlines) */
int32_t grid_nav(uint8_t *c, int32_t key);                                             /* 0x43ca40 */
/* a control's key when its window has the focus (+0x54 == 1000) */
int32_t ctrl_key(uint8_t *c, int32_t key);                                             /* 0x43ce50 */
/* a control's frame: the typewriter / the cell icons, then its arrows and cursor */
int32_t ctrl_tick(uint8_t *c);                                                         /* 0x43c650 */
/* every control of a shown window; the last one's result */
int32_t win_tick(uint8_t *win);                                                        /* 0x43c2e0 */
/* the message window's frame (`autoc`: the auto-advance countdown, NULL = wait for a key); 1 = done */
int32_t msg_update(input_mgr *in, uint32_t now, uint8_t *win, int32_t sound, int32_t freq, int32_t skip,
                   int32_t *autoc);                                                    /* 0x43b980 */
/* a list control's input (`nocancel`, `ud_ends`: up / down end it with 1, `horiz`: left / right move) */
int32_t ctrl_input(input_mgr *in, uint8_t *c, uint32_t now, int32_t cue, int32_t nocancel, int32_t ud_ends,
                   int32_t horiz);                                                     /* 0x43bca0 */
/* any job of the list with state 2 */
int32_t jobs_busy(const uint8_t *j);                                                   /* 0x43c170 */
/* the 16 ms pacing machine (0x28 B: +0 last, +4 owed ms, +8 last update, +0xc state 0 / 1 render / 2 update,
 * +0x10 step, +0x14 / +0x18 / +0x24 the fps count, +0x1c elapsed, +0x20 now) */
int32_t menu_pace(uint8_t *p);                                                         /* 0x55f550 */
/* the menu frame loop on the menu context {+4 the room loop, +8 the input manager} */
int32_t menu_run(uint8_t *m, uint8_t *c, uint8_t *msgwin, int32_t cue, int32_t lr, int32_t hot17,
                 int32_t world, int32_t anykey, int32_t ud_ends, int32_t ud);          /* 0x55f190 */

#endif /* OSS_UI_RUN_H */
