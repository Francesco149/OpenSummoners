/*
 * scene_box.{c,h} — the cutscene message box on the retail layout (rhost-verified inside retail): its
 * placement over the speaker (a speech balloon with its tail) and its fade in / out.  The box records are
 * the room loop's two 0x3c-byte slots at +0x54 (the current box) / +0x90 (the previous one, fading out),
 * filled by the cutscene runner 0x439690.
 *
 *   FUN_0049c640  box_place           FUN_0049c910  box_fade_tick
 *
 * The box record B (0x3c B): +0 its window, +0x10 / +0x14 the two tail controls (each a 1 x 1 grid whose cell
 * icon is a tail piece of the sys+0xba8 sheet), u16 +0x2e the fade state (1 in, 2 / 3 out, 0 none), +0x30 the
 * fade clock 0..1000, +0x34 its step.  The alpha ramp is the system's 20-step table sys+0x27a8.
 */
#ifndef OSS_SCENE_BOX_H
#define OSS_SCENE_BOX_H

#include <stdint.h>

#include "world_layout.h"

/* place window `win` over the speaker `who` (above it, below when there is no room above: under `high` 64 /
 * 24 px), clamped to x 32..608 - w; the tail pieces on `ta` / `tb` (`single`: `tb` only; frames 9..14) under
 * the speaker's centre, kept 32 px from the box's ends.  No speaker: centred at y 80.  Returns retail's eax. */
int32_t box_place(int32_t a1, uint8_t *win, int32_t a3, uint8_t *ta, uint8_t *tb, const wl_entity *who,
                  int32_t single, int32_t high);                                       /* 0x49c640 */
/* one frame of the box's fade (only while its window is live and has the focus +0x54 == 1000) */
int32_t box_fade_tick(uint8_t *bx);                                                    /* 0x49c910 */

#endif /* OSS_SCENE_BOX_H */
