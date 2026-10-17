/*
 * ai_skill.{c,h} — the character AI's held-attack skill pick on the retail layout (verified inside retail by the
 * hybrid gate + a shadow: rh-combat's held attacks).
 *
 *   FUN_00479f90  ai_479f90
 */
#ifndef OSS_AI_SKILL_H
#define OSS_AI_SKILL_H

#include <stdint.h>

#include "world_layout.h"

int32_t ai_479f90(wl_char *self, int32_t a2, int32_t a3);

#endif
