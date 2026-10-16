/*
 * arche_sword.{c,h} — Arche's (code 50010) SWORD SKILLS on the retail layout (rhost-verified inside retail): the
 * integrator 0x442a70 runs it after the pose handler 0x457ff0 and the skill casts 0x45a300; 0 = the physics step
 * runs.  The skills by the body's action pose +0x48: 10101 the combo slash (chaining to 10115), 10102..10106,
 * 10107, 10108..10122, the common poses 3 / 4 / 11 / 12 / 100 / 101 / 114.
 *
 *   FUN_00461790  arche_sword        FUN_00466750  arche_combo_skill
 */
#ifndef OSS_ARCHE_SWORD_H
#define OSS_ARCHE_SWORD_H

#include <stdint.h>

#include "world_layout.h"

/* the learnt combo skill 10115 when usable (party +0x466 set; its class 0xf00000 / 0xf00002, 0xf00003 for Arche,
 * or the member id; mode 1..3, 255 with +0x1f0), else NULL */
uint8_t *arche_combo_skill(wl_char *c);                                                /* 0x466750 */
int32_t arche_sword(wl_char *self, wl_body *body, int32_t probe, int32_t *ctl, int32_t grounded, int32_t part,
                    const wl_cmd *cmd);                                               /* 0x461790 */

#endif /* OSS_ARCHE_SWORD_H */
