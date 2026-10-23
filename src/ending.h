/*
 * ending.{c,h} — the ending credits + the screen helpers it reaches.  Retail layout; rhost
 * REPLACE-wired (UNVERIFIED but 0x56c640 0x56c900 0x56dbb0).  Names by VA.
 *
 *   FUN_00410090  en_410090   FUN_0049bf90  en_49bf90   FUN_005646c0  en_5646c0   FUN_0056c640  en_56c640
 *   FUN_0056c900  en_56c900   FUN_0056dbb0  en_56dbb0   FUN_005a00c0  en_5a00c0   FUN_005a3670  en_5a3670
 *   FUN_005a3870  en_5a3870   FUN_005a4320  en_5a4320   FUN_005a4440  en_5a4440   FUN_005a45c0  en_5a45c0
 */
#ifndef OSS_ENDING_H
#define OSS_ENDING_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t en_410090(int32_t a1, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8);
int32_t en_49bf90(wl_char *self, int32_t a2);
void en_5646c0(intptr_t * a1, int32_t a2);
void en_56c640(wl_char *self);
int32_t en_56c900(wl_char *self, char a2);
int32_t en_56dbb0(wl_char *self, char a2);
int32_t en_5a00c0(wl_char *self);
void en_5a3670(wl_char *self);
int32_t en_5a3870(wl_char *self, int32_t * a2);
int32_t en_5a4320(wl_char *self, char a2);
int32_t en_5a4440(wl_char *self, char a2);
int32_t en_5a45c0(wl_char *self, char a2);

#endif /* OSS_ENDING_H */
