/*
 * field_menu.{c,h} — the room loop's menus on the retail layout (verified inside retail by the hybrid gate + the
 * render gate: the menu / Messages / System flows of rh-combat): the main menu 0x591120, Messages 0x597aa0 /
 * 0x5985f0 / 0x5984c0 / 0x598460, System 0x596d60, the party / item hotkey menus 0x590480 / 0x590f20 / 0x590ca0, the
 * menu context 0x59d540, the scene runner 0x59ad60.  Names by VA.
 *
 *   FUN_00591120  fm_591120   FUN_00591590  fm_591590   FUN_00596d60  fm_596d60   FUN_00597aa0  fm_597aa0
 *   FUN_00598460  fm_598460   FUN_005984c0  fm_5984c0   FUN_005985f0  fm_5985f0   FUN_0056c430  fm_56c430
 *   FUN_00590480  fm_590480   FUN_00590f20  fm_590f20   FUN_00590ca0  fm_590ca0   FUN_0059d540  fm_59d540
 *   FUN_0059ad60  fm_59ad60
 */
#ifndef OSS_FIELD_MENU_H
#define OSS_FIELD_MENU_H

#include <stdint.h>

#include "world_layout.h"

int32_t fm_591120(wl_char *self);
int32_t fm_591590(wl_char *self, int32_t * a2, int32_t * a3);
int32_t fm_596d60(wl_char *self);
int32_t fm_597aa0(wl_char *self, int32_t * a2);
void fm_598460(wl_char *self, int32_t a2);
int32_t fm_5984c0(char * a1, char * a2, int32_t a3, char * a4);
int32_t fm_5985f0(wl_char *self, int32_t a2, int32_t a3);
int32_t fm_56c430(wl_char *self, char a2);
int32_t fm_590480(wl_char *self);
int32_t fm_590f20(wl_char *self, int32_t a2);
int32_t fm_590ca0(wl_char *self);
int32_t fm_59d540(wl_char *self, int32_t a2, int32_t a3);
int32_t fm_59ad60(wl_char *self, int32_t a2);

#endif
