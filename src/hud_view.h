/*
 * hud_view.{c,h} — the HUD's per-frame animation on the retail layout (rhost-verified inside retail): one call
 * per room-loop view record (room loop +0x128 + 0x3ec x i) eases what the HUD draws — the leader panel's
 * bars and status icons, the leader swap cross-fade, the party bars, the gold counter roll, the foe gauge, the
 * hit-combo counter.
 *
 *   FUN_0049af40  hud_view_tick       FUN_0049bc40  hud_status_icon     FUN_0049bd50  hud_status_drop
 *   FUN_0049bdf0  hud_ease            FUN_00496970  stats_mp_ratio
 *
 * The view record V (0x3ec B): +0 shown, +4 the room, +8 live, +0x14 the room +0x4070 flag's fade, +0x18 / u16
 * +0x1c a leader-change flash (the new leader for 100 frames when its +0x200 is 0), the leader panel P at +0x20
 * and the previous leader's at +0x1b0 (fading out), +0x340 + 8 x k the party bars (HP / MP per mille), +0x380 /
 * +0x384 a bounce (position, speed: 100 at the floor, -5 a frame), the foe gauge (+0x388 alpha, +0x38c hold,
 * +0x390 its HP per mille, +0x394 the foe kind, +0x398 the foe, u8 +0x39c, u16 +0x3c4 off-screen clock), +0x3c8 the
 * HUD alpha, u16 +0x3cc a 3-step clock, +0x3d0 a byte counter, +0x3d4 the gold shown, +0x3d8.. the hit combo
 * (alpha, the shown value up to 2000, the shown count, the last value / count).
 *
 * A panel P (0x190 B): +0 alpha, +4 the character, +8 / +0xc its HP / MP per mille, u16 +0x10 / +0x12 the quick
 * slot / class shown, u16 +0x14 their change clock (200), +0x18 the mood (1 calm, 2 a scene, 3 danger: HP under
 * 30 % or a lasting bad status), +0x1c the previous mood, +0x20 its cross-fade, 18 status icons from +0x24 (20 B:
 * u16 id, u16 value, u16 max, +8 alpha, +0xc dying, +0x10 pulse), +0x18c their count.
 */
#ifndef OSS_HUD_VIEW_H
#define OSS_HUD_VIEW_H

#include <stdint.h>

/* move *v toward t by a tenth of the gap (at least 5), then cap it at `max`; returns `max` */
int32_t hud_ease(int32_t *v, int32_t t, int32_t max);                                   /* 0x49bdf0 */
/* the MP per mille of a party stat block (MP +0x5c over +0x60 + +0x88 + +0xa0) */
int32_t stats_mp_ratio(const uint8_t *st);                                              /* 0x496970 */
/* status k of the panel's character: its icon updated (dying at 0) or added (up to 18) */
int32_t hud_status_icon(uint8_t *p, uint16_t k);                                        /* 0x49bc40 */
/* icon i removed, the rest shifted down (each pulsing +1000) */
int32_t hud_status_drop(uint8_t *p, uint16_t i);                                        /* 0x49bd50 */
/* one frame of a view record's HUD animation; returns retail's eax */
int32_t hud_view_tick(uint8_t *v);                                                      /* 0x49af40 */

#endif /* OSS_HUD_VIEW_H */
