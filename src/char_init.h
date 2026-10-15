/*
 * char_init.{c,h} — the CHARACTER INIT 0x41f200 (retail layout; rhost-verified inside retail).
 *
 * The room population 0x58d460 (and the party / event code) calls it on a free character
 * slot with a map object's record.  The DRAMATIST table 0x6b6ea8 (52-byte rows: handle, code,
 * name +8, u16 track +0x30) keyed by the handle overrides the code (for the party, a5 == 1, or
 * a code of 0) and the track; a11 picks the tint (0: 13, 1: 7, else the code itself, retail).
 * Every per-character block is reset (+0x158d0..+0x15aa7, the AI, the skills, the links, the
 * sound channels (5, 36 bytes each), the modifiers, the movement defaults); the base stats
 * (0x426fd0 at +0x274, the HP / MP jittered by 0x426e00 and the difficulty world+0x2780: 10 /
 * 20 = -40% / -25% HP (at least 8 / 4, at most half) and -4 / -2 to stats +0x64 / +0x6c, 40 /
 * 50 = +10% / +15% HP (at least 4 / 8) and +4 / +8); the pacing (5 draws).  Then by CODE
 * (50010..100189; any other is fatal: "Unknown Object Type Character."): Arche (50010) is
 * built in place (her bodies, tracks, clips, skills, 6 hand-built AI records, combos, voice);
 * the townsfolk get a box, a track (by the variant a12 unless the dramatist row has one),
 * clips, an optional walk/run, the AI mode 12 / 13, the +0x1e8 and a name; the rest go to
 * the per-type inits (0x428c00..0x430400, slots here).  Three codes are gated: 58010 only
 * when the time of day is not 1..4; 51022 / 51061 / 51062 only with sys_by_mode or the grid
 * +0x1c or world+0x27b4.  The epilogue files it in the world lists (+0x2788 unless it is a
 * townsperson, then +0x15908 = 1; +0x2794), resets the AI, probes the ground, sets the handle
 * and the party mode, the home posts (part 0 left/right -/+ 32000 / 16000 / 20000 for the AI
 * modes 5 / 6 / 12), the dramatist name, the stand clip (jittered) and the idle pick.
 *
 *   FUN_0041f200  char_init
 */
#ifndef OSS_CHAR_INIT_H
#define OSS_CHAR_INIT_H

#include <stdint.h>

#include "world_layout.h"

/* 0x41f200 (thiscall char, 14 args) */
void char_init(wl_char *c, int32_t x, int32_t y, int32_t anchor, int32_t f1f8, int32_t f1fc, int32_t code,
               int32_t a8, int32_t facing, int32_t handle, int32_t a11, int32_t variant, int32_t a13, int32_t a14,
               int32_t a15);

#endif /* OSS_CHAR_INIT_H */
