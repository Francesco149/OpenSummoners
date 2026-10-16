/*
 * char_attack.{c,h} — the ATTACK pipeline on the retail layout (rhost-verified inside retail): the power
 * rolls, the lunge, a skill's hit boxes (+ the twin side hits) and the target search.
 *
 *   FUN_004478e0  facing_back         FUN_004483a0  char_mag_sum        FUN_004481f0  char_atk_roll
 *   FUN_00447f30  char_skill_roll     FUN_00447ed0  char_lunge          FUN_00447900  char_attack_hits
 *   FUN_005b15e0  body_hit_spawn      FUN_005b1c50  char_hit_target
 *   FUN_00460de0  angle_wrap          FUN_004610e0  char_particle       FUN_004665d0  entity_slide_step
 *   FUN_00466700  orbit_init          FUN_00460ed0  orbit_step          FUN_00467500  body_pose_set
 *   FUN_00461240  slash_step          FUN_0044ce20  slash_trail         FUN_00466e80  crate_push
 *   FUN_005b3890  hit_breaks_shots    FUN_00478a50  shot_end            FUN_004483c0  char_mag_roll
 *   FUN_004611c0  clip_seq_step       FUN_00461410  body_pose_swap      FUN_00447c10  char_shot_fire
 *   FUN_004505c0  entity_sprite_descs FUN_00450590  particle_tint_set
 *
 * A struck foe goes on the HUD's foe gauge (foe_gauge_set 0x5b5600, hud_view.c).
 *
 * A body's hit records: u8 +0x7f live, u16 +0x80 count (8 at most), +0x84 + 64 k {+4 direction, +8 kind (99
 * none), +0xc, +0x10, +0x14 x, +0x18 y, +0x1c w, +0x20 h, +0x24..+0x3c the damage fields}.  A struck char
 * takes them at +0x15aa8.. (+0x26e = 1: hit this tick), its recent hits at +0x15aec (8 x {id, u16 time}).
 */
#ifndef OSS_CHAR_ATTACK_H
#define OSS_CHAR_ATTACK_H

#include <stdint.h>

#include "char_status.h"   /* es_desc */
#include "world_layout.h"

/* the other facing (3 left / 1 right) */
int32_t facing_back(int32_t f);                                                        /* 0x4478e0 */
/* the magic stat sum: party +0x6c + +0x7c + +0x94 + char +0xc7e4 */
int32_t char_mag_sum(const wl_char *c);                                                /* 0x4483a0 */
/* a physical power roll: w x (power + stats / 2) / 1000 less its spread (min(100, min(50 %, max(2, 20 %))))
 * plus r (0..999) per mille of it; `alt` (element 6): the stats without +0x74 / +0xc7d8 */
int32_t char_atk_roll(const wl_char *c, int32_t w, int32_t r, int32_t alt, int32_t power);  /* 0x4481f0 */
/* a magic power roll: as char_atk_roll on the magic sum (char_mag_sum) */
int32_t char_mag_roll(const wl_char *c, int32_t w, int32_t r, int32_t power);         /* 0x4483c0 */
/* a skill record's roll (+0x2dc magic: the magic sum; else as char_atk_roll by element +0xb0 == 6):
 * weight a3 x +0x2c4 / 1000, power +0x2c8 */
int32_t char_skill_roll(const wl_char *c, const uint8_t *skill, int32_t a3, int32_t r);    /* 0x447f30 */
/* the attack's lunge: body `b` moves d forward (the mover 0x54db10); `push` also takes it off +0x78;
 * unless `keep`, +0x14834 -= d.  0 = blocked */
int char_lunge(wl_char *c, wl_body *b, int32_t d, int32_t keep, int32_t push);          /* 0x447ed0 */
/* a skill's hit box(es) on `b` at (dx, dy) w x h (`mode` the knock-back direction rule, `nohit` record only);
 * the power rolled from `atk`; with +0xc7f4 two more at ±+0xc7f8.  The first box's result */
int char_attack_hits(wl_char *c, wl_body *b, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t mode,
                     int32_t nohit, const uint8_t *skill, int32_t atk, int32_t a11);    /* 0x447900 */
/* one hit box on `b` for `self` (attacker `atk`): its record, then the struck char (the damage fields, 0x5b5600)
 * or, none, a switch (0x5b1aa0); 1 = something hit */
int body_hit_spawn(wl_char *self, wl_char *atk, wl_body *b, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t mode,
                   int32_t nohit, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15,
                   int32_t a16, int32_t a17, int32_t a18, int32_t a19, int32_t a20, int32_t a21, int32_t a22,
                   int32_t a23, int32_t a24);                                          /* 0x5b15e0 */
/* the char a hit record's box strikes (alive, a body, not invulnerable / hit this tick / skip, faction
 * relation `rel`, not hit by this attack id within part + 30): the first, or the nearest when `nearest` */
wl_char *char_hit_target(const wl_char *self, const wl_char *skip, uint16_t part, const uint8_t *rec, int32_t rel,
                         int32_t nearest, int32_t *part_out);                          /* 0x5b1c50 */

/* a hit box `r` {x, y, w, h} breaking the hostile shots it overlaps (band64 entities with +0x230 set): a clink
 * (sound pool sys+0x568, the frequency jittered), 8 sparks 100074 when on screen, the shot ended; 1 = any */
int hit_breaks_shots(wl_char *self, const int32_t *r);                                 /* 0x5b3890 */
/* a shot's end: `keep` only zeroes the byte at `flag`; else its looping sounds let go (when +0x254) and the
 * entity released */
void shot_end(wl_entity *s, uint8_t *flag, int32_t keep);                              /* 0x478a50 */

/* the body's clip sequence on the phase +0x58 from `base`: at `base` clip `clips`, then (the clip done) the
 * next of `n` (404 B apart; past them none); the phase + 1.  0 = waiting for the clip */
int clip_seq_step(wl_body *b, intptr_t clips, uint16_t n, uint16_t base);             /* 0x4611c0 */
/* the action swaps between (pa, sa) and (pb, sb) by `flag` (with `gate`: 1 in a region-C body or below the
 * +0xc7e8 count): (pa, sa / any) and 0 -> (pb, sb); (pb, sb) and 1 -> (pa, sa); restarted with the skill
 * record + a new serial.  1 = swapped */
int body_pose_swap(wl_char *c, wl_body *b, int32_t flag, uint16_t pa, uint16_t sa, uint16_t pb, uint16_t sb,
                   int32_t gate);                                                      /* 0x461410 */
/* a skill's shot `code` from `b` (+0x7f set): the power rolled (the magic sum when +0x2dc, else the attack
 * roll) by weight `w`, the action serial (a new one when `fresh`), element 7's magic roll; shot_spawn */
void *char_shot_fire(wl_char *c, int32_t code, wl_body *b, int32_t dx, int32_t dy, int32_t anchor, int32_t a6,
                     int16_t a7, int32_t a8, const uint8_t *skill, int32_t w, int32_t a11, int32_t fresh,
                     int32_t a13);                                                     /* 0x447c10 */
/* a particle's tint index +0x25c (code 100044 also takes the tint table 0x8a92b8 entry) */
void particle_tint_set(wl_entity *e, int32_t i);                                       /* 0x450590 */
/* the sprite descriptors of `e` (*n): the drawn bow (codes 100165..100168, 51027, 51252 / 51253 at pose 3 phase
 * 2) adds the string over the sprite (its alpha by +0x5c); codes 100179 / 100180 the multi-part frame's live
 * parts; else entity_sprite_desc */
int entity_sprite_descs(const wl_entity *e, const wl_body *b, es_desc *out, uint16_t *n); /* 0x4505c0 */

/* (a + d) mod 360 n, floored (an exact negative multiple gives 360 n) */
int32_t angle_wrap(int32_t a, int32_t d, int32_t n);                                   /* 0x460de0 */
/* a particle at `b` + (dx, dy) for `c` (+ the twin copies at ±+0xc7f8 when `twins`); the last one */
wl_entity *char_particle(wl_char *c, int32_t code, const wl_body *b, int32_t dx, int32_t dy, int32_t a6, int32_t a7,
                         int32_t a8, int32_t unused, int32_t a10, int32_t a11, int32_t twins, int32_t a13); /* 0x4610e0 */
/* a pushed / sliding entity's step on part `part`: fall 1, then `dir` x its speed +0x1c4 inside [+0x1c8, +0x1cc]
 * (no range when equal), carrying its riders; *dx_out the x moved; 1 when it moved */
int entity_slide_step(wl_entity *e, int32_t dir, uint16_t part, int32_t *dx_out);     /* 0x4665d0 */
/* an orbit record {x, y, r, ang (milli-degrees), dang, u16 +0x18 clock, +0x1a delay, +0x1c limit} (`skip`: as is) */
int32_t *orbit_init(int32_t *o, int32_t x, int32_t y, int32_t r, int32_t ang, int32_t dang, int16_t delay, int16_t limit,
                    int32_t skip);                                                     /* 0x466700 */
/* `n` orbit steps (the angle + dang, dang + accel; the x87 polar step) each dropping particle `code` (+ twins);
 * past the limit only the particles, when `after` */
int32_t orbit_step(wl_char *c, const wl_body *b, int32_t *o, int32_t accel, int32_t n, int32_t hold, int32_t code,
                   int32_t after);                                                     /* 0x460ed0 */
/* the body's action `pose` at priority `prio` (kept while a running one's prio / 100 is higher), a new world serial */
void body_pose_set(wl_body *b, uint16_t pose, uint16_t prio, uint16_t a4, int32_t a5, int32_t a6); /* 0x467500 */
/* the slash's stages on +0x58 (the clips at clips + 0 / 404 / 808 / 1212; the two lunges d1 / d2): 1 = advanced */
int slash_step(wl_char *c, wl_body *b, int32_t keep, intptr_t clips, int32_t d1, int32_t d2, int32_t a7, int32_t a8); /* 0x461240 */
/* the swing's afterimage: particle 100082 per sprite descriptor (two for a drawn bow); the last particle */
intptr_t slash_trail(wl_entity *e, wl_body *b, int32_t quiet, int32_t a4, int32_t a5);  /* 0x44ce20 */
/* the push: the crate ahead (commands 1 / 5 left, 2 / 6 right) slides by its speed and the pusher follows;
 * `probe`: only whether one is there */
int crate_push(wl_entity *e, const int32_t *cmd, wl_body *b, int32_t armed, uint16_t part, int32_t dir_in,
               int32_t probe);                                                         /* 0x466e80 */

#endif /* OSS_CHAR_ATTACK_H */
