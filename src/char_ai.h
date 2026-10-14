/*
 * char_ai.{c,h} — the controllable character's per-tick INPUT -> COMMAND layer,
 * FUN_00478ba0 (3.5 KB), ported faithfully on the retail layout (world_layout.h)
 * and verified call-for-call inside retail by rhost (`--hook shadow:char_ai`, a
 * full-state shadow: every retail .data/heap dword equal after the call).
 *
 * Each tick it rebuilds the command block (character+0x14854, wl_cmd) the
 * integrator 0x442a70 then executes: walk / run (held arrows, the dash
 * double-tap 0x479e70, the run-key toggle), crouch / look-up, jump / drop-through,
 * attack, the use key, the combo skills, the skill-slot keys, target cycling.
 * Unported callees go through engine_calls.h slots (the integrator 0x442a70 for
 * the jump-assist probe, 0x440f40, the skill helpers, the sound player).
 *
 *   FUN_00478ba0  char_ai
 *   FUN_00479e70  char_ring_double     (the double-tap query, on the char's input)
 *   FUN_00479d30  party_slot_get       FUN_00479da0  party_slot_set
 *   FUN_00411530  cmd_clear            FUN_00479de0 / 0x479e10 / 0x479e40  cmd_set_*
 *   FUN_00440f40  char_hazard_probe    FUN_004411e0  body_frame_box
 * The ring queries 0x479960 / 0x479ca0 / 0x479a10 live in input.c.
 */
#ifndef OSS_CHAR_AI_H
#define OSS_CHAR_AI_H

#include <stdint.h>

#include "world_layout.h"

/* FUN_00478ba0 (thiscall self): one AI tick for `self`, whose controlled body is
 * `b` (b+0x2c facing selects forward/back); `a3` is forwarded to the skill
 * checks.  Returns retail's eax (no caller uses it). */
int char_ai(wl_char *self, wl_body *b, int32_t a3);

/* FUN_00479e70 (thiscall self): both `id1` then `id2` pressed — the first inside
 * [lo, hi] ms, the second (a different record) inside [0, hi2]; `consume` clears
 * both ids from the ring.  1/0. */
int char_ring_double(wl_char *self, uint32_t now, uint32_t lo, uint32_t hi, uint32_t hi2,
                     int32_t id1, int32_t id2, int32_t consume);

/* FUN_00440f40 (thiscall self): would part `n` of the (simulated) body `b` touch a
 * hostile entity's attack hitbox?  Skips hitbox ids taken recently (char+0x15aec),
 * honours the gate modes (char+0x15a28); while looking up (state 5 + up_assist) a hit
 * only sets *out and keeps scanning.  1 = hit. */
int  char_hazard_probe(wl_char *self, uint16_t n, const wl_body *b, int32_t *out);
/* FUN_004411e0: frame rect `r` of `b` (b->frames, 20 B each) in world space -> box[4]. */
int32_t body_frame_box(uint16_t r, const wl_body *b, int32_t box[4]);

void     cmd_clear(wl_cmd *c);                                                    /* 0x411530 */
void     cmd_set_skill(wl_cmd *c, int16_t skill, int16_t arg, int32_t ptr);        /* 0x479e10 */
void     cmd_set_skill_kind(wl_cmd *c, int16_t skill, int16_t kind, int16_t arg,
                            int32_t ptr);                                          /* 0x479e40 */
void     cmd_set_member_skill(wl_cmd *c, int16_t skill, int16_t kind);             /* 0x479de0 */
uint16_t party_slot_get(wl_party *p);                                              /* 0x479d30 */
uint16_t party_slot_set(wl_party *p, int16_t slot);                                /* 0x479da0 */

#endif /* OSS_CHAR_AI_H */
