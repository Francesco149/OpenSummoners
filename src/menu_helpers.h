/*
 * menu_helpers.{c,h} — the UI dialogs (Yes / No, message + choice windows, party pick), the item use, the start gear,
 * the level-up stat roll.  Retail layout; rhost REPLACE-wired.  Names by VA.
 *
 *   FUN_00418ec0  mh_418ec0   FUN_004190d0  mh_4190d0   FUN_004199b0  mh_4199b0   FUN_00419c70  mh_419c70
 *   FUN_00419eb0  mh_419eb0   FUN_004e61d0  mh_4e61d0   FUN_004e6520  mh_4e6520   FUN_004e6870  mh_4e6870
 *   FUN_005b1f10  mh_5b1f10   FUN_005b35f0  mh_5b35f0   FUN_005b3850  mh_5b3850   FUN_00409150  mh_409150
 */
#ifndef OSS_MENU_HELPERS_H
#define OSS_MENU_HELPERS_H

#include <stdint.h>

#include "world_layout.h"

int32_t mh_418ec0(wl_char *self, int32_t a2);
int32_t mh_4190d0(wl_char *self, char * a2, uint16_t a3, int32_t * a4, int32_t a5);
int32_t mh_4199b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t mh_419c70(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7, int32_t * a8, int32_t * a9, char * a10, int32_t a11);
int32_t mh_419eb0(wl_char *self, char * a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7, int32_t a8, int32_t * a9, int32_t a10);
int32_t mh_4e61d0(wl_char *self, int32_t a2, int32_t a3);
int32_t mh_4e6520(wl_char *self, int32_t a2, int32_t a3);
int32_t mh_4e6870(wl_char *self, uint16_t a2, int32_t a3, int32_t a4);
int32_t mh_5b1f10(wl_char *self, int32_t a3, int32_t a4, char * Buffer, uint16_t * a6, uint32_t a7, int32_t a8);
int32_t mh_5b35f0(wl_char *self, int32_t a2);
int32_t mh_5b3850(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t mh_409150(wl_char *self, int32_t * a2);

#endif /* OSS_MENU_HELPERS_H */
