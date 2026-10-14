/*
 * char_state.{c,h} — the small per-character state helpers the character apply pass
 * 0x485fc0 (char_apply.c) and the behaviour layer call: HP / MP changes, the status
 * gates, the per-tick modifier reset, the AI reset, the respawn mark, the pit timer,
 * the look-up setup, a forced action, and two grid cell reads.  Retail layout;
 * rhost-verified inside retail.
 *
 *   FUN_005b54d0  char_add_hp            FUN_005b5580  char_add_mp
 *   FUN_00485f70  char_status_stuck      FUN_0040d960  char_targetable
 *   FUN_00487530  char_mods_reset        FUN_0041e180  char_ai_reset
 *   FUN_00487570  char_respawn_mark      FUN_00487730  char_pit_check
 *   FUN_0041ef80  char_set_look_up       FUN_004874b0  char_force_action
 *   FUN_004593c0  body_slowed            FUN_00487250  grid_b_word
 *   FUN_004872d0  grid_c_at_feet         FUN_00453960  entity_scatter
 *   FUN_00426440  sys_by_mode            FUN_0041ee60  entity_anchor
 *   FUN_00426db0  entity_set_track
 */
#ifndef OSS_CHAR_STATE_H
#define OSS_CHAR_STATE_H

#include <stdint.h>

#include "world_layout.h"

/* Add `d` HP (clamped to 0 .. max = hp_max + the two bonuses); `dry` != 0 only reports.
 * *out = the applied change (party status[1] set: *out = 0, no change — retail writes it
 * without a NULL check).  Returns hp <= 0. */
int char_add_hp(wl_char *c, int32_t d, int32_t *out, int32_t dry);
/* The same for MP; returns the applied change (or `d` when status[1] is set). */
int32_t char_add_mp(wl_char *c, int32_t d, int32_t *out, int32_t dry);
/* 1 when a live status whose table entry (0x628764 + 0x8c k) is set holds it */
int char_status_stuck(const wl_char *c);
/* alive, body[0] active, (`any_7e` or not +0x7e), (`any_busy` or status[1] clear) */
int char_targetable(const wl_char *c, int32_t any_busy, int32_t any_7e);
int32_t char_mods_reset(wl_char *c);
/* Clear the behaviour state; `full` also the hold budget, engage / calm timers and every
 * cooldown back to its reset value. */
void char_ai_reset(wl_char *c, int32_t full);
/* Standing on solid ground with both feet over floor cells that carry no object: that
 * spot becomes the respawn point. */
void char_respawn_mark(wl_char *c, wl_body *b);
/* Over a pit cell (region C 1 / 3) for more than 20 ticks: 1 and *kind = the cell. */
int char_pit_check(wl_char *c, const wl_body *b, int32_t *kind);
int32_t char_set_look_up(wl_char *c, int16_t pose, const void *clip0, const void *clip1, int16_t len,
                         int32_t chance, int32_t a7);
int char_force_action(wl_char *c, int16_t pose, int32_t len);
int body_slowed(const wl_body *b);
int32_t grid_b_word(const uint8_t *grid, int32_t x, int32_t y);
int32_t grid_c_at_feet(int32_t a1, const wl_body *b);
/* body[0] velocity: vvel = vy0 + vyr * rand / 0x8000, then vel_x = vx0 + vxr * rand / 0x8000 */
int32_t entity_scatter(wl_entity *e, int32_t vx0, int32_t vxr, int32_t vy0, int32_t vyr);
/* by the system's mode word (sys +0x289c): 2/3 -> a, 4 -> b, 5 -> c, else 0 */
int32_t sys_by_mode(const void *sys, int32_t a, int32_t b, int32_t c);
/* The body position for an anchor point (x, y): 0 top-left, 1 minus 100 x the entity's
 * +0x30/+0x34 offset, 2 centre, 3 feet (minus +0x1f4 and the height), 4 top centre,
 * 5 feet centre; other anchors write nothing. */
void entity_anchor(const wl_entity *e, int32_t x, int32_t y, int32_t anchor, int32_t *ox, int32_t *oy);
/* fill the 20-byte record i at entity +0x48 (no bounds check, as retail) */
int32_t entity_set_track(wl_entity *e, uint16_t i, int16_t a, int16_t b, uint8_t on, int32_t f0c, int32_t f08, int32_t f10);

#endif /* OSS_CHAR_STATE_H */
