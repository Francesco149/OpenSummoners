/*
 * behave_ops.{c,h} — the behaviour AI's helpers (the layer under FUN_0047b990,
 * char_behave.c): the step-ahead check, the target / ally pick, and the small
 * per-state movers (walk to a point, pace between two posts, hold, go home, follow
 * the leader).  Faithful on the retail layout; rhost-verified inside retail.
 *
 *   FUN_0047dbb0  behave_step_ok        FUN_0047e330  behave_pick_char
 *   FUN_0047b770  faction_rel           FUN_004852c0  char_engaged_target
 *   FUN_00485f30  char_status_threat    FUN_004852f0  behave_set_target
 *   FUN_0047d760  behave_c_check        FUN_00484bf0  behave_set_state
 *   FUN_00484c10  behave_walk_to        FUN_004857f0  behave_pace
 *   FUN_004856d0  behave_go_home        FUN_00485670  behave_hold
 *   FUN_00485740  behave_follow
 */
#ifndef OSS_BEHAVE_OPS_H
#define OSS_BEHAVE_OPS_H

#include <stdint.h>

#include "world_layout.h"

/* Can `b` step `dist` ahead (dir 1 right / 3 left): no wall (region D == 1) at the head row
 * of that column and — unless the movement mode skips it — a floor within 2 (5 when `deep`)
 * rows under the feet, or the platform it rides still under that x.  `strict`: off-map is
 * a no (else the column clamps).  `need_floor` only matters in move mode 3 (0 => yes). */
int behave_step_ok(wl_char *self, const wl_body *b, int32_t dist, int32_t dir, int32_t strict, int32_t deep,
                   int32_t need_floor);

/* The character to engage: every live character (not self, body[0] active and not +0x7e,
 * party status[1] clear) whose relation to `fac` is `rel` (0 none, 1 hostile, 2 allied),
 * within 48000 x 32000 of the reference point (96000 x 64000 while engaged), scored by
 * |reach - dx| + 2 dy + a 0..6399 rand jitter (+32000 for a hostile faction when self is
 * coll_off, else + the status threat when `threat`), -16000 for the least-crowded one when
 * `crowd` == 1.  The reference is self's part 0 centre, or (with a leader and `near_leader`)
 * the spot beside the leader's part `part` (side_dist to the kept side).  Lowest score wins. */
wl_char *behave_pick_char(wl_char *self, int32_t fac, int32_t rel, uint32_t part, int32_t reach, int32_t crowd,
                          int32_t near_leader, int32_t threat);

int faction_rel(const wl_char *self, int32_t a, int32_t b);   /* 0 none, 1 hostile, 2 allied */
wl_char *char_engaged_target(const wl_char *c);               /* its target if engaged / player-driven */
int32_t char_status_threat(const wl_char *c);                 /* max table weight of its live statuses */
intptr_t behave_set_target(wl_char *self, wl_char *t);         /* records where it was; eax = t's body */
int behave_c_check(const wl_char *self);
int behave_set_state(wl_char *self, int16_t s);                /* returns 0 */

int behave_walk_to(wl_char *self, wl_body *b, int32_t gr);
int behave_pace(wl_char *self, wl_body *b, int32_t gr, int16_t hold, int32_t chance);
intptr_t behave_go_home(wl_char *self, wl_body *b, int32_t gr);
int behave_hold(wl_char *self, wl_body *b, int32_t gr, int32_t dec);
void behave_follow(wl_char *self, wl_body *b, int32_t gr, uint16_t part);

#endif /* OSS_BEHAVE_OPS_H */
