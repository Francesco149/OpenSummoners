/*
 * menu_screens.{c,h} — the main menu's other screens (level-up, Status, Items, Abilities, Party Commands).  Retail
 * layout; REPLACE-wired in rhost (UNVERIFIED until a recording opens them).  Names by VA.
 *
 *   FUN_005915c0  mm_5915c0   FUN_00592020  mm_592020   FUN_00592190  mm_592190   FUN_00592850  mm_592850
 *   FUN_00593d90  mm_593d90   FUN_00593e40  mm_593e40   FUN_00593f60  mm_593f60   FUN_005943e0  mm_5943e0
 *   FUN_00594a20  mm_594a20   FUN_00594c80  mm_594c80   FUN_00594f40  mm_594f40   FUN_00595320  mm_595320
 *   FUN_00595350  mm_595350   FUN_005956c0  mm_5956c0   FUN_00595dd0  mm_595dd0   FUN_005962a0  mm_5962a0
 *   FUN_005963a0  mm_5963a0   FUN_00596620  mm_596620   FUN_00596ac0  mm_596ac0   FUN_00596b50  mm_596b50
 *   FUN_00597480  mm_597480   FUN_005974b0  mm_5974b0   FUN_00597670  mm_597670   FUN_00598d00  mm_598d00
 *   FUN_005990b0  mm_5990b0   FUN_00599140  mm_599140   FUN_005994e0  mm_5994e0   FUN_00599a60  mm_599a60
 *   FUN_00599ac0  mm_599ac0   FUN_00599e60  mm_599e60
 */
#ifndef OSS_MENU_SCREENS_H
#define OSS_MENU_SCREENS_H

#include <stdint.h>

#include "world_layout.h"

int32_t mm_5915c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void mm_592020(char * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int16_t a6);
int32_t mm_592190(wl_char *self, int32_t a2);
int32_t mm_592850(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5);
void mm_593d90(wl_char *self, int32_t a2);
void mm_593e40(int32_t a1, uint16_t a2, char * a3, int32_t a4, int32_t a5, int16_t a6);
int32_t mm_593f60(wl_char *self, int32_t a2);
int32_t mm_5943e0(wl_char *self, int32_t a2, int32_t a3);
int32_t mm_594a20(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8, char * a9, int32_t * a10, int32_t a11, int32_t a12, int32_t a13);
int32_t mm_594c80(wl_char *self, int32_t * a2, int32_t a3);
int32_t mm_594f40(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t mm_595320(wl_char *self, int16_t a2, int32_t a3);
int32_t mm_595350(wl_char *self, int32_t a2, int32_t a3);
int32_t mm_5956c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t mm_595dd0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t mm_5962a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
int32_t mm_5963a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t mm_596620(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t mm_596ac0(int32_t a1, char * a2);
int32_t mm_596b50(wl_char *self);
int32_t mm_597480(void);
int32_t mm_5974b0(wl_char *self, int32_t a2);
int32_t mm_597670(wl_char *self, int32_t a2);
int32_t mm_598d00(wl_char *self);
int32_t mm_5990b0(int32_t a1, char * a2);
int32_t mm_599140(wl_char *self, int32_t * a2, int32_t a3);
int32_t mm_5994e0(wl_char *self, char * a2);
int32_t mm_599a60(wl_char *self, uint16_t * a2, uint16_t * a3, int32_t * a4, int32_t a5, uint16_t * a6);
int32_t mm_599ac0(wl_char *self, int32_t a2);
int32_t mm_599e60(wl_char *self, int32_t * a2, int32_t a3);

#endif /* OSS_MENU_SCREENS_H */
