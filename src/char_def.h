/*
 * char_def.{c,h} — the character DEFINITION helpers (retail layout; rhost-verified inside
 * retail): the setters the character init 0x41f200 and its per-type inits (0x428c00..0x431930)
 * fill a wl_char's tuning blocks with, the skill / AI / combo record builders, the voice set,
 * the base stats.
 *
 * Skills (+0x754, 768-byte records, u16 count +0xc754, cap 64): 0x4282f0 copies one from the
 * skill table 0x6826c0 (768-byte rows keyed by +0; 0 ends it) — the name at +4 = the empty
 * string 0x8a9b6c, the display name +0x54, the fields +0xa4..+0x2f4, mode +0x2f8 = 1;
 * 0x427fb0 learns every skill the code's rows in 0x6907c0 (16 bytes: code, skill) list;
 * 0x4285f0 defines one from its arguments.  AI skill records (+0xca28, 320 bytes, u16 count
 * +0x14228, no cap): 0x4287d0 binds a learnt skill with its range box (+0x14 first, count
 * +0x10, up to 8) and a second box (+0x98, count +0x94, up to 8); 0x428950 adds both boxes.
 * Approach records (+0xc8a4, 48 bytes, u16 count +0xca24, no cap): 0x427f00.  Combos
 * (+0x148a0, 64 bytes, u16 count +0x1489c, cap 64): 0x427430, only for a learnt skill (or
 * a direct one, a2 == 0).  The voice set (+0x15b54): 0x4289f0 builds 74 groups of the
 * 0x690ca0 table and the code's lines of 0x691018 (pool sys+0x364 + 4 x slot; the volume
 * only when the line's flag is set, sys+0x310 != 0 and **(sys+0x320)+0x1c8 == 0).  The base
 * stats (char+0x274, 0x426fd0) come from the stat table 0x67ac58 (220-byte rows: code, level
 * (0 = the first row of the code), HP, MP, four stats, EXP max, ...; the name +0x3a, else the
 * level-1 row's), +0xf4 = row[11] x (750 + rand x 500 / 0x8000) / 1000.
 *
 *   FUN_004072c0  char_skill_find      FUN_0041e280  char_ground_probe
 *   FUN_00426380  voice_set_free       FUN_0041e440  voice_set_stop
 *   FUN_00426420  tod_off              FUN_00426480  grid_f1c
 *   FUN_00426e00  stat_jitter          FUN_00426fd0  char_stats_init
 *   FUN_004090e0  stat_row_find        FUN_004c6110  char_profile_find
 *   FUN_00427360  char_idle_init       FUN_004273c0  char_idle_add
 *   FUN_004273f0  char_member_skill_add FUN_00427430 char_combo_add
 *   FUN_00427580  char_clips_set       FUN_004275f0  char_clips2_set
 *   FUN_00427610  char_carry_set       FUN_00427650  char_step_set
 *   FUN_00427670  char_pace_set        FUN_00427b70  char_pace_raw
 *   FUN_00427c00  char_c880_set        FUN_00427c30  char_move_set
 *   FUN_00427c90  char_15a5c_set       FUN_00427cc0  char_turn_set
 *   FUN_00427ce0  char_walk_set        FUN_00427d30  char_jump_set
 *   FUN_00427dc0  char_run_set         FUN_00427df0  char_hit_set
 *   FUN_00427e40  char_member_pose_set FUN_00427e70  char_crouch_set
 *   FUN_00427eb0  char_nav_skills_set  FUN_00427f00  char_approach_add
 *   FUN_00427fb0  char_skills_by_code  FUN_004282f0  char_skill_learn
 *   FUN_004285f0  char_skill_def       FUN_00428780  char_nav_set
 *   FUN_004287b0  char_retarget_set    FUN_004287d0  char_ai_skill_add
 *   FUN_00428950  ai_skill_boxes_add   FUN_004289f0  char_voice_init
 *   FUN_00431930  char_npc_default     FUN_00431b90  char_form0_set
 *   FUN_00431ba0  char_party_mode_set  FUN_00431cb0  char_idle_voice_set
 *   FUN_0042eb20  char_party_ai_skills
 */
#ifndef OSS_CHAR_DEF_H
#define OSS_CHAR_DEF_H

#include <stdint.h>

#include "world_layout.h"

/* 0x4072c0 (thiscall): the learnt skill `id` (compared as u16), or NULL */
uint8_t *char_skill_find(wl_char *c, uint16_t id);
/* 0x41e280 (thiscall): the support probe of body 0 (+0x14238 = what it stands on) */
int char_ground_probe(wl_char *c);
/* 0x426380 (thiscall): stop every line's voice, free the groups, count 0 */
void voice_set_free(wl_voiceset *v);
/* 0x41e440 (fastcall): stop every line's voice (the groups stay) */
void voice_set_stop(wl_voiceset *v);
/* 0x426420 (thiscall sys): the time of day (sys+0x289c) is not 1..4 */
int tod_off(const void *sys);
/* 0x426480 (thiscall grid): +0x1c */
int32_t grid_f1c(const void *grid);
/* 0x426e00 (stdcall): q = base x pm / 1000; d = min(50% q, max(20% q, 2), 100);
 * q + r x d / 1000 - d (r per mille) */
int32_t stat_jitter(int32_t base, int32_t pm, int32_t r);
/* 0x426fd0 (stdcall): the stat block `st` (char+0x274) for (code, level) */
void char_stats_init(uint8_t *st, int32_t code, int32_t level);
/* 0x4090e0 (stdcall): its stat table row (the first of `code` at `level`, any for 0; else row 0) */
const uint8_t *stat_row_find(int32_t code, int32_t level);
/* 0x4c6110 (stdcall): the code's profile row (0x682660, 24 B), or NULL */
const uint8_t *char_profile_find(int32_t code);
/* 0x427360 / 0x4273c0: the idle animation timing / one idle pose (up to 4) */
void char_idle_init(wl_char *c, int32_t first, int32_t next, int32_t jitter);
void char_idle_add(wl_char *c, int32_t pose);
/* 0x4273f0: a member-skill key pair (up to 4) */
void char_member_skill_add(wl_char *c, int32_t key, int32_t val);
/* 0x427430: a combo (14 args; ids are u16) */
uint8_t *char_combo_add(wl_char *c, int32_t direct, int32_t skill, int32_t arg, int32_t ts_mode, int32_t release,
                        int32_t lo, int32_t hi, int32_t lo2, int32_t hi2, uint16_t id0, uint16_t id1, uint16_t id2,
                        uint16_t id3, uint16_t id4);
/* 0x427580: the stand / air clips (+0x15914..), the collision (a6), the carry block cleared
 * (a5 is unused) */
void char_clips_set(wl_char *c, const void *stand, const void *air0, const void *air1, int32_t unused, int32_t collide);
void char_clips2_set(wl_char *c, const void *a, const void *b);                     /* 0x4275f0 */
void char_carry_set(wl_char *c, int32_t on, const void *c0, const void *c1, int32_t a5, int32_t a6,
                    int32_t a7);                                                    /* 0x427610 */
void char_step_set(wl_char *c, int32_t mode, uint16_t sfx);                         /* 0x427650 */
/* 0x427670: the AI pacing by kind 0..7 (2..5 random; 6 / 7 the flat 0x427b70) */
void char_pace_set(wl_char *c, int32_t kind, int32_t reach, int32_t c838, int32_t c844);
void char_pace_raw(wl_char *c, int32_t reach, int32_t c838, uint16_t c840, uint16_t wait, uint16_t c876,
                   uint16_t c878, uint16_t pause_chance, uint16_t pause_len, int32_t c844); /* 0x427b70 */
void char_c880_set(wl_char *c, int32_t a, int32_t b, int32_t d);                    /* 0x427c00 */
void char_move_set(wl_char *c, int32_t mode, int32_t fall_cap, int32_t grav, int32_t slide_cap, int32_t slide_grav,
                   int32_t f1f4, int32_t low_bridge, int32_t float_h);              /* 0x427c30 */
void char_15a5c_set(wl_char *c, int32_t a, int32_t b, int32_t d, int32_t e);         /* 0x427c90 */
void char_turn_set(wl_char *c, int32_t on, int32_t air);                            /* 0x427cc0 */
void char_walk_set(wl_char *c, int32_t max, int32_t accel, int32_t run_accel, int32_t brake, int32_t f1597c,
                   const void *clip);                                               /* 0x427ce0 */
void char_jump_set(wl_char *c, int32_t impulse, int32_t g_held, int32_t g_free, int32_t air_max, int32_t fast,
                   int32_t slow, const void *c0, const void *c1, const void *c2, uint16_t sfx); /* 0x427d30 */
void char_run_set(wl_char *c, int32_t max, const void *clip);                       /* 0x427dc0 */
void char_hit_set(wl_char *c, uint16_t pose, const void *clip, int32_t f159f8, int32_t f158fc,
                  int32_t unused);                                                  /* 0x427df0 */
void char_member_pose_set(wl_char *c, uint16_t a, uint16_t pose, int32_t f15a0c);    /* 0x427e40 */
void char_crouch_set(wl_char *c, int32_t margin, const void *c0, const void *c1, const void *c2); /* 0x427e70 */
void char_nav_skills_set(wl_char *c, int32_t sk, int32_t arg, int32_t sk2, int32_t arg2, int32_t lo, int32_t hi,
                         int32_t chase);                                            /* 0x427eb0 */
/* 0x427f00: an approach record (12 args, the last u16) */
void char_approach_add(wl_char *c, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7,
                       int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, uint16_t a13);
void char_skills_by_code(wl_char *c, int32_t code);                                 /* 0x427fb0 */
uint8_t *char_skill_learn(wl_char *c, int32_t id);                                  /* 0x4282f0 */
/* 0x4285f0: define a skill (22 args) */
uint8_t *char_skill_def(wl_char *c, int32_t id, uint16_t fae, int32_t f0b0, int32_t f0b4, int32_t f0b8,
                        int32_t f0bc, int32_t f0c0, int32_t f2c4, int32_t f2c8, int32_t f2cc, int32_t f2d0,
                        int32_t f2d4, uint16_t f2d8, int32_t f2dc, int32_t f2e0, int32_t f2e4, int32_t f2e8,
                        int32_t f2ec, uint16_t f2f0, uint16_t f2f2, int32_t f2f4, const char *name);
void char_nav_set(wl_char *c, int32_t drop, int32_t rise, int32_t fall);            /* 0x428780 */
void char_retarget_set(wl_char *c, uint16_t t, uint16_t b);                         /* 0x4287b0 */
/* 0x4287d0: bind learnt skill `id` (u16 match) as the next AI skill record (16 args) */
uint8_t *char_ai_skill_add(wl_char *c, int32_t id, int32_t a3, int32_t a4, int32_t x0, int32_t x1, int32_t y0,
                           int32_t y1, uint16_t f11a, uint16_t f120, uint16_t f124, uint16_t f126, int32_t bx0,
                           int32_t bx1, int32_t by0, int32_t by1, int32_t unused);
/* 0x428950 (stdcall): one box to each list of an AI skill record */
uint8_t *ai_skill_boxes_add(uint8_t *rec, int32_t x0, int32_t x1, int32_t y0, int32_t y1, int32_t bx0, int32_t bx1,
                            int32_t by0, int32_t by1);
void char_voice_init(wl_char *c, int32_t code, int32_t mode, int32_t bias);          /* 0x4289f0 */
void char_npc_default(wl_char *c, int32_t reach, const void *stand);                /* 0x431930 */
void char_form0_set(wl_char *c, uint16_t v);                                        /* 0x431b90 */
void char_party_mode_set(wl_char *c, int32_t mode);                                 /* 0x431ba0 */
void char_idle_voice_set(wl_char *c, int32_t on, uint16_t span);                    /* 0x431cb0 */
/* 0x42eb20: an AI skill record for each known party skill (47 / 48 / 134..140 / 10113.. / 20101..20136: ranges
 * by skill; the plain ones take a recharge f2 / f3 / f4 (f4 0 = f3)), and the skills still of class 0xf00000
 * take `cls` */
void char_party_ai_skills(wl_char *c, uint16_t f2, uint16_t f3, uint16_t f4, int32_t cls);

#endif /* OSS_CHAR_DEF_H */
