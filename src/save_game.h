/*
 * save_game.{c,h} — saving and loading on the retail layout (verified inside retail by the hybrid gate + the render
 * gate: the room-start snapshots of every scenario, rh-combat's statue save, game-over retry and the title's
 * Continue): the statue dialog 0x41a430, the slot list 0x41bd20 / 0x41bf60 / 0x561560, the writer 0x41a890, the
 * reader 0x41b670, the snapshot prep 0x55f620, the apply 0x561d20 / 0x560530, the file name 0x41cfd0.  Names by VA.
 *
 *   FUN_0041a430  sv_41a430   FUN_0041a890  sv_41a890   FUN_0041b670  sv_41b670   FUN_0041bd20  sv_41bd20
 *   FUN_0041bf60  sv_41bf60   FUN_0055f620  sv_55f620   FUN_00561560  sv_561560   FUN_00561bf0  sv_561bf0
 *   FUN_00561cf0  sv_561cf0   FUN_00561d20  sv_561d20   FUN_00560530  sv_560530   FUN_0041cfd0  sv_41cfd0
 */
#ifndef OSS_SAVE_GAME_H
#define OSS_SAVE_GAME_H

#include <stdint.h>

#include "world_layout.h"

int32_t sv_41a430(wl_char *self, int32_t a2, int32_t a3);
int32_t sv_41a890(wl_char *self, int32_t a2, uint16_t a3, int32_t a4, int32_t a5);
int32_t sv_41b670(wl_char *self, int32_t a2, uint16_t a3, int32_t * a4);
int32_t sv_41bd20(intptr_t ecx_, int32_t a1, int32_t a2);
void sv_41bf60(int32_t * a1, int32_t a2);
int32_t sv_55f620(wl_char *self, int32_t a2, int32_t a3);
void sv_561560(int32_t a1, int32_t a2, uint16_t a3, int32_t a4, char * a5, int32_t a6);
int32_t sv_561bf0(wl_char *self, int32_t a2);
int32_t sv_561cf0(wl_char *self, int32_t a2);
int32_t sv_561d20(wl_char *self, int32_t a2);
int32_t sv_560530(wl_char *self, int32_t a2);
int32_t sv_41cfd0(int32_t a1, uint16_t a2, char * a3, int32_t * a4);

#endif
