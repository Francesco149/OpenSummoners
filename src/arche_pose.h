/*
 * arche_pose.{c,h} — the party characters' POSE handler 0x457ff0 (Arche, Sana, 50012 every
 * integrator tick, before the action handler 0x45a300) + its helpers (retail layout;
 * rhost-verified inside retail).
 *
 * By the body's pose (+0x48): 2 the turn-around (clip 0x62c8a8, the facing flips at frame 1),
 * 98 / 99 the hit reactions (a 20% voice line 17 / 18 by the hit kind +0x15ad0, 19 for 13 / 16,
 * else line 9; the burn mark 100156 for kind 8), 102 / 103 the stagger (margin 3200), 107 / 111 /
 * 115 the knockdowns (0x459990: knocked up -40000 / -40000, falls in stages with dust and the
 * landing sound 132; 107 then inflicts status 10 (fallen) on landing, 115 shakes the screen,
 * 111 lies 100 ticks), 108 / 109 down, 110 / 112 the blown-back falls (the stage drift 0x4678f0),
 * 113 the get-up (clip by the body state: walk / run / crouch / jump / air / look-up / stand),
 * 116 the revive, 125 / 126 the look-up / revive clips.  A status (0x4593e0): its length from the
 * 0x628708 row (+0x64 base, +0x68 the AI extra, +0x66 jitter: the jitter draw is taken TWICE when
 * the first sum is >= 0), the party log line ("%s has fallen!" ...) when the room logs.
 *
 *   FUN_00457ff0  arche_pose           FUN_004593e0  char_status_inflict
 *   FUN_00459990  char_knockdown       FUN_00459ed0  body_switch_touch
 *   FUN_004678f0  char_drift           FUN_00467ae0  char_drift_move
 *   FUN_00459370  particle_live        FUN_0054d060  room_hitstop_set
 *   FUN_005b1aa0  switch_touch         FUN_005b4510  char_room_logs
 */
#ifndef OSS_ARCHE_POSE_H
#define OSS_ARCHE_POSE_H

#include <stdint.h>

#include "world_layout.h"

/* 0x457ff0 (thiscall char): 1 = the pose handled the motion (no physics step) */
int arche_pose(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t *ctl);
/* 0x4593e0 (thiscall char): inflict status `st` (1..18) with value `v` */
void char_status_inflict(wl_char *c, uint16_t st, int32_t v);
/* 0x459990 (thiscall char): the knockdown stages (`burn` = the kind-8 mark / the party aura) */
int char_knockdown(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t aura);
/* 0x459ed0 (stdcall): with no probe, the switch contact below the body */
int body_switch_touch(const wl_body *b, int32_t probe, uint16_t part);
/* 0x4678f0 (thiscall char): ramp vx / vy to their targets, then move (0x467ae0) */
int char_drift(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t xt, int32_t xs, int32_t xb, int32_t yt,
               int32_t ys, int32_t yb, int32_t stop_x, int32_t a12, int32_t a13);
/* 0x467ae0 (thiscall char): move the body by its velocity (vertical, then horizontal) */
int char_drift_move(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t stop_x, int32_t a6, int32_t a7);
/* 0x459370 (stdcall): a live particle of `code` exists */
int particle_live(int32_t code);
/* 0x54d060 (stdcall): the room loop's hitstop (+0x3c on, +0x3a ticks, phase 0) */
void room_hitstop_set(int32_t on, uint16_t n);
/* 0x5b1aa0 (stdcall): the first live material entity whose part overlaps rect {x, y, w, h} and is
 * not yet touched gets touched by its kind (+0x290: 1 / 2 any (returns 1); 3 on: state 1 / off;
 * 4 off-only) */
int switch_touch(const int32_t *rect, uint16_t part, int32_t mode);
/* 0x5b4510 (thiscall char): its party room's +0x40a4 (0 without a slot) */
int32_t char_room_logs(const wl_char *c);

#endif /* OSS_ARCHE_POSE_H */
