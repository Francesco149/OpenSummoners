/*
 * char_monster.{c,h} — the MONSTERS' per-type character inits (the slots of the character init 0x41f200), on the
 * retail layout (rhost-verified inside retail).  Each sets the type's clips, walk / jump / hit / move tuning, its
 * skills (char_skill_def) and AI skill records (char_ai_skill_add, or the record built in place), the pacing.
 *
 *   FUN_0042e290  mon_slime           FUN_0042c970  mon_skill51         FUN_0042fbf0  mon_skill29
 *   FUN_0042ea50  ai_rec_hold_set     FUN_0042da20  char_skill_list_add
 *
 * The record built in place (retail inlines 0x4287d0's head): the learnt skill `id` (none: nothing) into AI
 * record +0xca28 + 320 x the count +0x14228, then one range box (ai_skill_boxes_add) and the count + 1.
 */
#ifndef OSS_CHAR_MONSTER_H
#define OSS_CHAR_MONSTER_H

#include <stdint.h>

#include "world_layout.h"

/* the slimes (100164 / 100186, 51012..51015): track row `row` (+ the second track `trk1` when set), the bounce
 * skills 19 / 20 (+ 21 / 22 for the jumpers with `trk1`; 23 for the splitting ones) with the power `pw` / `pw2` */
void mon_slime(wl_char *c, int16_t row, int16_t trk1, int32_t pw, int32_t pw2);       /* 0x42e290 */
/* codes 0xc74e / 0xc775 / .. (track row `row`): skill 51, move mode 1 */
void mon_skill51(wl_char *c, int16_t row);                                            /* 0x42c970 */
/* code 0xc74f and its kin: skills 29 / 30 (power a3 / a4) */
void mon_skill29(wl_char *c, int32_t code, int32_t a3, int32_t a4);                   /* 0x42fbf0 */
/* the AI records of `rec`'s skill: their hold +0x11c = v x (300 rand / 0x8000) / 1000 + 700 v / 1000 */
void ai_rec_hold_set(wl_char *c, const uint8_t *rec, uint16_t v);                     /* 0x42ea50 */
/* `v` onto the list char +0x158d4 (u16 count +0x158d0, 8 at most) */
void char_skill_list_add(wl_char *c, int32_t v);                                       /* 0x42da20 */

#endif /* OSS_CHAR_MONSTER_H */
