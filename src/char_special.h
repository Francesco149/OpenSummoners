/*
 * char_special.{c,h} — the integrator's special poses (0x75..0x7a / 0x81: flying, butterflies, riding, ...), the
 * cases of FUN_00442a70's pose switch char_integrate hands over.  Retail layout (world_layout.h).
 *
 *   (no whole function: a region of FUN_00442a70 char_integrate, src/char_integrate.c)
 */
#ifndef OSS_CHAR_SPECIAL_H
#define OSS_CHAR_SPECIAL_H

#include <stdint.h>

#include "world_layout.h"

/* is `p` one of the special poses (the cases integrate_special runs) */
static inline int integrate_special_pose(uint16_t p) { return (p >= 0x75 && p <= 0x7a) || p == 0x81; }

/* the special pose `out->pose` of a char_integrate call, after its prologue; char_integrate continues at its tail */
void integrate_special(wl_char *self, wl_cmd *cmd, wl_body *out, int32_t probe, int32_t part);

#endif
