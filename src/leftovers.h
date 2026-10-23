/*
 * leftovers.{c,h} — the AI's input-ring helpers, the
 * logger, the launcher, the last three small leaves.  Retail layout; rhost REPLACE-wired.  Names by VA.
 *
 *   FUN_00479960  lo_479960   FUN_00479a10  lo_479a10   FUN_00479de0  lo_479de0   FUN_00406440  lo_406440
 *   FUN_00401140  lo_401140   FUN_004011b0  lo_4011b0   FUN_00401210  lo_401210   FUN_004013a0  lo_4013a0
 *   FUN_004013c0  lo_4013c0   FUN_00401730  lo_401730   FUN_005a4760  lo_5a4760   FUN_0058c8d0  lo_58c8d0
 *   FUN_0054e4f0  lo_54e4f0
 */
#ifndef OSS_LEFTOVERS_H
#define OSS_LEFTOVERS_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t lo_479960(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9, int32_t a10);
int32_t lo_479a10(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t lo_479de0(int32_t a1, int16_t a2, int16_t a3);
int32_t lo_406440(wl_char *self, char * lpOutputString, char * a3);
int32_t lo_401140(void);
int32_t lo_4011b0(void);
int32_t lo_401210(int32_t hWnd, uint32_t Msg, uint32_t wParam, int32_t lParam);
int32_t lo_4013a0(void);
int32_t lo_4013c0(int32_t hDlg, uint32_t a2, uint32_t a3, int32_t a4);
int32_t lo_401730(int32_t hDlg);
int32_t lo_5a4760(wl_char *self);
int32_t lo_58c8d0(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4);
int32_t lo_54e4f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

#endif /* OSS_LEFTOVERS_H */
