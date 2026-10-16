/*
 * char_handlers.{c,h} — the integrator's per-type HANDLERS the corpus reaches (monsters' skill bodies 0x455d80 /
 * 0x44d2e0 / 0x4512d0 + their helpers) and the ITEM / SKILL EFFECT 0x5b3b00 (the skill casts' and the room's
 * item use) with its value 0x5b28a0, on the retail layout (rhost-verified inside retail).
 *
 * A FIRST PASS through tools/ida/decomp2c.py, finished by hand; the names are their VAs.
 *
 *   FUN_00455d80  ih_455d80           FUN_0044d2e0  ih_44d2e0           FUN_004512d0  ih_4512d0
 *   FUN_004466b0  ih_4466b0           FUN_00467260  ih_467260           FUN_005b3b00  item_effect
 *   FUN_005b28a0  ih_5b28a0           FUN_00456c60  ih_456c60
 */
#ifndef OSS_CHAR_HANDLERS_H
#define OSS_CHAR_HANDLERS_H

#include <stdint.h>

#include "world_layout.h"

int32_t ih_455d80(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t * a7, int32_t a8);
int32_t ih_44d2e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t ih_4512d0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
void ih_4466b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t ih_467260(wl_char *self, int32_t a3, int32_t a4, int32_t a5);
int32_t item_effect(wl_char *self, int32_t a2, int32_t a3, uint16_t * a4, int32_t a5, int32_t a6, int16_t a7);
int32_t ih_5b28a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, char * Buffer);
int32_t ih_456c60(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);

#endif /* OSS_CHAR_HANDLERS_H */
