/*
 * sana_skills.{c,h} — Sana's (code 50011) and 50012's SKILL handler on the retail layout (rhost-verified inside
 * retail).  The integrator 0x442a70 runs it every tick after the pose handler 0x457ff0 and the skill casts
 * 0x45a300, by the body's action pose +0x48:
 *
 *   10201  the staff swing (act_arg 4: the air swing, restarted as the ground one on landing): clips, the lunge,
 *          voice group 1 + the swing sound (pool sys+0x474) at phase 2, the hit box(es) at phase 3 (kind 11)
 *   10202  the magic shot 100001 (act_arg 7 air / 8 ground swap by the ground; 1 a held stream): voice cue 3 / 5
 *   10301  the spell bursts (act_arg 1 the polar ring, 3 / 5 the forward burst, else the rising column):
 *          particles 100057 / 100067 + hit boxes (char_attack_hits mode 8)
 *   10302  the spin swing (act_arg 4: two lunges, kind 1, an orbit of particles 100077 at the char +0x14834)
 *   10303  the back-flip (the clip set by code: 0x639b48 Sana, 0x63a668 50012): afterimages 100082
 *   10, 13..16 the plain poses (10 a two-part rest on the ground with voice 25 + the queued line 26)
 *   100 / 101  the idle animations (50011 / 50012 clip sets, 180 / 520 / 550 ticks)
 *
 * *ctl gets 2 (3 for the back-flip).  Retail returns AL (whatever it held): the integrator ignores it.
 *
 *   FUN_0045e830  sana_skills        FUN_00409970  voice_group_prio (sound_fx.c)
 */
#ifndef OSS_SANA_SKILLS_H
#define OSS_SANA_SKILLS_H

#include <stdint.h>

#include "world_layout.h"

void sana_skills(wl_char *c, wl_body *b, int32_t probe, int32_t *ctl, const wl_cmd *cmd, int32_t grounded); /* 0x45e830 */

#endif /* OSS_SANA_SKILLS_H */
