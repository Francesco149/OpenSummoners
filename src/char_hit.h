/*
 * char_hit.{c,h} — a struck character TAKES its pending hit, on the retail layout (rhost-verified inside
 * retail): the hit combo, the damage text, the record damage, a crate's knock.
 *
 *   FUN_004895c0  char_take_hit       FUN_005b4d00  char_die            FUN_005b5020  foe_drops
 *   FUN_005b5140  exp_share           FUN_005b34d0  float_text          FUN_00556990  pickup_spawn
 *   FUN_00525ab0  char_part_release
 *   FUN_005b3f00  hit_combo           FUN_005b4130  dmg_party_scale     FUN_005b4170  dmg_record
 *   FUN_0048b850  dmg_glyphs          FUN_0048b8e0  crate_knock
 *
 * TAKING A HIT (0x4895c0, the apply pass when +0x15ab0 = 1): the hit sound by kind +0x15ad0 (pool sys+0x364 + 4
 * id, its gain / frequency rolled) and the element; the hit point held inside the struck part's box (its frame
 * record; the part's damage multiplier rects +16, flag +20); the status roll (+0x15ac4 -> status, +0x15ac8 the
 * power less the resist / defence, rand x 10000 / 0x8000 % 1000 below it); the damage (+0x15ad8 less the defence
 * jitter / 4, at least 1, x 1.5 with party +0x452; the combo; the part; the element resist; a block (look-up
 * state 5) takes off +0x15a14 per mille; the guard meter (party +0x464) takes half; the slot scale); the damage
 * number (particle 100042, "%d" / "%d Max" on a room record); the stagger meter (+0xc784 += 1500 a hit, over
 * +0x15a0c (x 1.4 / 1.2 for the party on levels 10 / 20) the break pose +0x15a08 and every hit-worn status
 * ends); else the flinch (+0x159ec, not when armoured) with its knock-back (+0x159f8 strength, +0x15ab8 the rule)
 * and the hit-worn statuses' timers (status rows 0x628708 + 140 k, +0x70); the inflicted status (its length
 * from the row, two rand draws; the messages); the HP (death: 0x5b4d00); a crate pushed back; the guard / block
 * effects or the hit sparks 100073.
 *
 * The HIT COMBO (the attacker's party room, +0x40dc..+0x4100): +0x40e0 the count, +0x40e4 its clock (1000 at
 * a hit), +0x40e8 the bonus (per mille, at most 2000: +30 a hit, +40 within 400 ticks of the clock (> 600 left),
 * + 10 / 30 / 50 for 1 / 2 / 3 kinds among the last four not seen before them), +0x40ec the bonus shown (from 5
 * hits), +0x40f0..+0x40fc the last four kinds (255 none), +0x40dc the best count (+0x4100 = 1 when beaten).
 * From 5 hits: the combo sound (pool sys+0x408, gain + 800, frequency min(4000, 400 n - 1600)) and the damage
 * + damage x bonus / 1000.
 */
#ifndef OSS_CHAR_HIT_H
#define OSS_CHAR_HIT_H

#include <stdint.h>

#include "world_layout.h"

/* the pending hit taken (see above); returns the HP change */
int32_t char_take_hit(wl_char *c);                                                      /* 0x4895c0 */
/* the death: a vanishing foe (+0x15904, `release`) lets go of its holds, parts, voice set and is released; else
 * status 1 (fallen, its length rolled) replaces every other, the knock-down pose +0x15a0a (unless `release`),
 * and with `reward` (not in a two-room world) a foe's drops + the EXP to the other side's rooms ("Exp +%d") */
void char_die(wl_char *c, int32_t release, int32_t reward, int32_t dir);               /* 0x5b4d00 */
/* the drops: the item `item` (chance per mille, rand x 10000 / 0x8000 % 1000) + `gold` as 100 / 10 / 1 coins
 * (fewer than 3 coins: one bigger coin broken into ten smaller) */
int32_t foe_drops(wl_char *c, int32_t gold, int32_t item, int32_t chance);              /* 0x5b5020 */
/* a pickup (coins 100159 / 100160 / 100161 worth 1 / 10 / 100, item 100162, 100163) from the 96-band at `at`'s
 * centre, tossed up; NULL when the band is full */
wl_entity *pickup_spawn(int32_t code, const wl_body *at, int32_t item);                 /* 0x556990 */
/* the room loop's EXP split (the difficulty +0x2780 scales it -8 % .. +10 %): every live member of the rooms
 * not on `faction`'s side gains it (less 50 % / 25 % / 0 % by level), levelling up; the rooms' kill / EXP
 * totals; returns the last such room leader's gain */
int32_t exp_share(void *loop, int32_t faction, int32_t exp);                            /* 0x5b5140 */
/* a floating number (particle 100042, glyphs of fmt % v) */
void float_text(wl_char *c, const char *fmt, int32_t v, wl_body *b, int32_t a5, int32_t x, int32_t y, int32_t facing,
                uint16_t color, int32_t kind, uint16_t life, int32_t vy);              /* 0x5b34d0 */
/* char +0xc7fc + 4 i's entity released */
void char_part_release(wl_char *c, uint16_t i);                                         /* 0x525ab0 */
/* the attacker `atk`'s room combo after a hit of kind `kind` on `struck`: *dmg grows by the bonus, *count = the
 * count (1 without a party slot).  Retail returns a division's sign bit: unused */
void hit_combo(wl_char *atk, wl_body *b, int32_t *dmg, int32_t *count, wl_char *struck, int32_t kind); /* 0x5b3f00 */
/* the damage scaled by the party slot's +0xa08 per mille (at least 1; 0 when none) */
int32_t dmg_party_scale(const wl_char *c, int32_t dmg);                                 /* 0x5b4130 */
/* the room's record damage (magic +0x40c8 / +0x40cc, else +0x40c0 / +0x40c4 = the member id): 1 = a new record
 * above 6 that the room logs (char_room_logs) */
int32_t dmg_record(const wl_char *c, int32_t dmg, int32_t magic);                       /* 0x5b4170 */
/* the damage text's glyph codes (' ' 255, '!'..'z' 0..89, the rest dropped; 40 characters at most) */
int32_t dmg_glyphs(const char *s, uint16_t *out, uint16_t *n);                          /* 0x48b850 */
/* a crate knocked: speed dir x (20000 + rand x 4000 / 0x8000), by its facing */
int32_t crate_knock(wl_entity *e, int32_t dir);                                         /* 0x48b8e0 */

#endif /* OSS_CHAR_HIT_H */
