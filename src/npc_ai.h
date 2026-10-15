/*
 * npc_ai.{c,h} — the NPC NAVIGATION AI FUN_0043f880 (5.5 KB) and its
 * helper cluster: walk a character toward a target x (townsfolk, party members,
 * monsters).  Faithful on the retail layout; rhost-verified inside retail.
 *
 * Each call builds up to 5 candidate DIRECTIONS (toward / walk / run / stop, ordered by
 * distance and speed) and an ordered list of ACTIONS (walk, crouch, look up, jump, the
 * chase / ranged skills), then tries them in order, SIMULATING each through the
 * integrator 0x442a70 (one step, 0x4412d0; or 31 steps on heap scratch bodies for the
 * jumps / look-ups) and rejecting any that ends in a hostile hitbox (0x440f40), against
 * another entity, or off a ledge.  The first acceptable command is copied to the
 * character's command block (char+0x14854); none -> an empty command.
 *
 *   FUN_0043f880  npc_navigate         FUN_004412d0  npc_try_cmd
 *   FUN_00441270  npc_probe_free       FUN_00441ae0  npc_floor_ahead
 *   FUN_0043ee30  grid_c_query         FUN_00441700  body_hitbox_hit
 *   FUN_004429c0  char_hit_fresh       FUN_00442a20  char_hit_kind_ok
 *   FUN_004418e0  npc_blocked          FUN_00440e40  entity_blocks
 *   FUN_004022a0  part_bottom          FUN_00426f70  part_left
 *   FUN_00426fa0  part_right           FUN_0044e640  part_center_x
 *   FUN_004842c0  part_facing
 *   FUN_0044e680  part_center_y        FUN_0047e790  part_width
 */
#ifndef OSS_NPC_AI_H
#define OSS_NPC_AI_H

#include <stdint.h>

#include "world_layout.h"

int npc_navigate(wl_char *self, int32_t tx, int32_t arrive, wl_body *b, int32_t grounded, int32_t calm,
                 int32_t mode, int32_t ty, int32_t no_run, int32_t want_jump);
int npc_try_cmd(wl_char *self, wl_cmd *cmd, int32_t steps, int32_t check_hits, int16_t switch_at,
                int32_t switch_lr, int32_t need_hit);
void npc_probe_free(wl_char *self, wl_body *block, uint16_t n);
int npc_floor_ahead(wl_char *self, const wl_body *b, int32_t x, int32_t strict, int32_t check_floor, int32_t depth);
int grid_c_query(const uint8_t *grid, int32_t x, int32_t y, int32_t *out);
const int32_t *body_hitbox_hit(const wl_char *e, const wl_body *b, int32_t part, int32_t use_frames, uint16_t nframes);
int char_hit_fresh(const wl_char *self, const int32_t *hit, uint16_t n);
int char_hit_kind_ok(const wl_char *self, int32_t gate, int32_t kind);
int npc_blocked(const wl_char *self, const wl_body *b, uint16_t part);
int entity_blocks(const wl_entity *e, const wl_entity *other, uint16_t part, int32_t dir, int16_t pass, int32_t nocollide);
int32_t part_bottom(const wl_entity *e, uint16_t part);
int32_t part_left(const wl_entity *e, uint16_t part);
int32_t part_right(const wl_entity *e, uint16_t part);
/* 0x4842c0 (thiscall entity): part `part`'s facing (out of range: part 0, logged) */
int32_t part_facing(const wl_entity *e, int32_t part);
int32_t part_center_x(const wl_entity *e, uint16_t part);
int32_t part_center_y(const wl_entity *e, uint16_t part);   /* the middle of the box below the margin */
int32_t part_width(const wl_entity *e, uint16_t part);

#endif /* OSS_NPC_AI_H */
