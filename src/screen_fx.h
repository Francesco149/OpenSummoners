/*
 * screen_fx.{c,h} — the per-frame SCREEN EFFECTS on the retail layout (rhost-verified inside retail): the
 * cell dissolve (the room-entry reveal / cover), the fader level, the overlay pictures' fades.
 *
 *   FUN_0049a890  fx_rows_center_out  FUN_0049a740  fx_rows_edges_in   FUN_0049aa00  fx_rows_sweep_down
 *   FUN_0049aae0  fx_rows_sweep_up    FUN_0049abd0  overlay_fade        FUN_0049ac80  overlay_tick
 *   FUN_0049c2f0  notice_tick         FUN_00499ab0  screen_step
 *
 * The screen object FX (world+0x1040): +0 the cells (12 B {u16 state 0 / 1 fading / 2 done, u16 its clock
 * 0..1000, +4 col, +8 row}), u16 +4 / +6 the grid's width / height in cells, u16 +8 the count, u16 +0xa, +0xc the
 * dissolve kind (1 reveal, 2 cover, 0 / 3 none), +0x10 the pattern (0 centre-out, 1 edges-in, 2 a sweep), u16
 * +0x14 the rows reached, +0x18 done, +0x1c the cell speed (step 100 x it / 1000), +0x20 the fader state (1
 * reset, 2 up, 3 down), +0x24 its speed (step 20 x it / 1000), +0x28 the level 0..1000.  A pattern marks the
 * next `rows` rows' cells fading, each staggered by (rows - i - 1) x (step / rows) on its clock.
 *
 * An overlay O (room loop +0x118 / +0x11c / +0x120): +0 the kind, u16 +4 the phase (0 in, 1 hold, 2 out), u16 +6
 * the hold clock, +8 the alpha 0..1000, u16 +0x10 the hold length (kind 1), +0x20 live.  Its frames (sys+0xbb4
 * kind 1, else sys+0xbb8) are freed when it ends.
 */
#ifndef OSS_SCREEN_FX_H
#define OSS_SCREEN_FX_H

#include <stdint.h>

#include "world_layout.h"

void fx_rows_center_out(uint8_t *fx, uint16_t step, uint16_t rows);                   /* 0x49a890 */
void fx_rows_edges_in(uint8_t *fx, uint16_t step, uint16_t rows);                     /* 0x49a740 */
void fx_rows_sweep_down(uint8_t *fx, uint16_t step, uint16_t rows);                   /* 0x49aa00 */
void fx_rows_sweep_up(uint8_t *fx, uint16_t step, uint16_t rows);                     /* 0x49aae0 */
/* an overlay's in (+in a frame) / hold (`hold` frames) / out (-out); 1 the frame it ends */
int32_t overlay_fade(uint8_t *o, int32_t in, int32_t out, int32_t hold);              /* 0x49abd0 */
/* the third overlay (room loop +0x120): kind 1 in 20 / hold +0x10 / out 20; 2: 40 / 200 / 40; 3: 20 / 300 / 20;
 * 4: 20 / 600 / 20 */
void overlay_tick(uint8_t *o);                                                         /* 0x49ac80 */

/* the notice popup (room loop +0xec): the world's next pending log line (8 x 516 B from world+4, round robin
 * from world+0x1024) in a (160, 384) 320 x 80 window sliding in from the right, its text shown 120 frames (50
 * when another waits), closed when its control is done */
void notice_tick(wl_roomloop *rl);                                                     /* 0x49c2f0 */
/* the room loop's per-frame screen step: the letterbox bars (room loop +0x40 on, +0x44 / +0x48 to 64, +0x4c to
 * +0x50 — the view's +0x74 follows it), every window's animation, the fader, the dissolve, both views' HUD, the
 * overlays, the notice popup */
void screen_step(wl_roomloop *rl);                                                     /* 0x499ab0 */

#endif /* OSS_SCREEN_FX_H */
