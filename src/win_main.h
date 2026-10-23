/*
 * win_main.{c,h} — the program's top: _WinMain (the session loop), WinMain's init, the app object's teardown and a
 * few platform leftovers.  Retail layout; rhost-verified inside retail (REPLACE: the whole program runs from the
 * port's _WinMain).  Names by VA.
 *
 *   FUN_005a4770  wn_5a4770   FUN_00562210  wn_562210   FUN_005624c0  wn_5624c0   FUN_00562b50  wn_562b50
 *   FUN_00562b70  wn_562b70   FUN_00560900  wn_560900   FUN_0058ffa0  wn_58ffa0   FUN_005b9ab0  wn_5b9ab0
 *   FUN_005b9ac0  wn_5b9ac0
 */
#ifndef OSS_WIN_MAIN_H
#define OSS_WIN_MAIN_H

#include <stdint.h>

#include "world_layout.h"

int32_t wn_5a4770(wl_char *self, int32_t hInstance);
int32_t wn_562210(int32_t hInstance, int32_t hPrevInstance, char * lpCmdLine, int32_t nShowCmd);
void wn_5624c0(wl_char *self);
int32_t wn_562b50(wl_char *self, char a2);
int32_t wn_562b70(intptr_t * a1);
int32_t wn_560900(wl_char *self);
void wn_58ffa0(wl_char *self, int32_t active);
int32_t wn_5b9ab0(wl_char *self);
int32_t wn_5b9ac0(wl_char *self);

#endif /* OSS_WIN_MAIN_H */
