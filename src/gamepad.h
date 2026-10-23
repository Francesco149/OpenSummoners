/*
 * gamepad.{c,h} — the gamepad input producer.  Retail layout; rhost
 * REPLACE-wired (UNVERIFIED: no joystick under rhost).  Names by VA.
 *
 *   FUN_00468b10  gp_468b10   FUN_0046cb40  gp_46cb40   FUN_0046cd10  gp_46cd10
 */
#ifndef OSS_GAMEPAD_H
#define OSS_GAMEPAD_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t gp_468b10(wl_char *self, int32_t * a2, int32_t a3);
int32_t gp_46cb40(wl_char *self, uint32_t a2, int32_t a3);
int32_t gp_46cd10(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

#endif /* OSS_GAMEPAD_H */
