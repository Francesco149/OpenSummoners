/*
 * title_menu.{c,h} — the title and its menus on the retail layout (verified inside retail by the hybrid gate +
 * the render gate: the title / options / new-game / load flows of the corpus): the title drive 0x562ea0, the
 * splash + title menu 0x56aea0, New Game options 0x564780 / 0x566a80 .., Options 0x568de0, the load screen, the
 * sparkles / compositor.  Names by VA.  Retail's callers reach 30 of them (hooked, REPLACE only).
 *
 *   FUN_0041bbe0  tm_41bbe0   FUN_00562ea0  tm_562ea0   FUN_00564160  tm_564160   FUN_005642e0  tm_5642e0
 *   FUN_005644a0  tm_5644a0   FUN_00564690  tm_564690   FUN_00564780  tm_564780   FUN_00419900  tm_419900
 *   FUN_005657f0  tm_5657f0   FUN_00565810  tm_565810   FUN_00565840  tm_565840   FUN_005658c0  tm_5658c0
 *   FUN_00565d10  tm_565d10   FUN_00566250  tm_566250   FUN_00566350  tm_566350   FUN_0056d910  tm_56d910
 *   FUN_00566570  tm_566570   FUN_00566850  tm_566850   FUN_00566a80  tm_566a80   FUN_00567ba0  tm_567ba0
 *   FUN_00568320  tm_568320   FUN_0056da40  tm_56da40   FUN_0056daa0  tm_56daa0   FUN_00568b40  tm_568b40
 *   FUN_0056ded0  tm_56ded0   FUN_0056dfe0  tm_56dfe0   FUN_0056c930  tm_56c930   FUN_0056cc10  tm_56cc10
 *   FUN_0056dc20  tm_56dc20   FUN_00568de0  tm_568de0   FUN_0056a220  tm_56a220   FUN_0041c150  tm_41c150
 *   FUN_0041c240  tm_41c240   FUN_00560a40  tm_560a40   FUN_0041c2f0  tm_41c2f0   FUN_0041c320  tm_41c320
 *   FUN_00560a00  tm_560a00   FUN_00562a10  tm_562a10   FUN_0056aea0  tm_56aea0   FUN_0041bb80  tm_41bb80
 *   FUN_0056bfd0  tm_56bfd0   FUN_0056c030  tm_56c030   FUN_0056c070  tm_56c070   FUN_0056c180  tm_56c180
 *   FUN_0056c2b0  tm_56c2b0   FUN_00562af0  tm_562af0   FUN_0056c470  tm_56c470   FUN_0056c4e0  tm_56c4e0
 *   FUN_0056c580  tm_56c580   FUN_0056c610  tm_56c610   FUN_0056cb90  tm_56cb90   FUN_0056cd20  tm_56cd20
 *   FUN_0040bb70  tm_40bb70   FUN_0056d710  tm_56d710   FUN_0056de80  tm_56de80   FUN_0059ec30  tm_59ec30
 *   FUN_0059ee50  tm_59ee50   FUN_0041d780  tm_41d780   FUN_0049a6e0  tm_49a6e0
 */
#ifndef OSS_TITLE_MENU_H
#define OSS_TITLE_MENU_H

#include <stdint.h>

#include "world_layout.h"

void tm_41bbe0(wl_char *self);
int32_t tm_562ea0(wl_char *self, int32_t a2);
int32_t tm_564160(wl_char *self);
int32_t tm_5642e0(wl_char *self, int32_t a2, int32_t a3);
int32_t tm_5644a0(wl_char *self);
int32_t tm_564690(wl_char *self);
int32_t tm_564780(wl_char *self, int32_t a2);
int32_t tm_419900(wl_char *self, int32_t a2, int32_t a3);
int32_t tm_5657f0(wl_char *self, uint16_t a2, int32_t a3);
int32_t tm_565810(wl_char *self);
void tm_565840(wl_char *self);
int32_t tm_5658c0(wl_char *self);
int32_t tm_565d10(wl_char *self, int32_t * a2, int16_t a3, int32_t * a4, int32_t a5, uint32_t a6);
int32_t tm_566250(wl_char *self, int32_t a2);
int32_t tm_566350(wl_char *self, int32_t * a2);
int32_t tm_56d910(wl_char *self);
int32_t tm_566570(int16_t a1, char * a2);
int32_t tm_566850(int16_t a1, char * a2);
int32_t tm_566a80(int16_t a1, int32_t a2, char * Buffer);
int32_t tm_567ba0(wl_char *self, int32_t a2);
int32_t tm_568320(int16_t a1, int32_t * a2);
int32_t tm_56da40(wl_char *self, char * a2, int32_t a3, int32_t a4);
int32_t tm_56daa0(wl_char *self, int32_t a2, int16_t a3, int32_t a4, int32_t a5, int16_t a6, int32_t a7, int32_t a8, int16_t * a9, uint16_t a10, int16_t a11);
int32_t tm_568b40(wl_char *self, int32_t * a2);
void tm_56ded0(wl_char *self);
int32_t tm_56dfe0(wl_char *self);
int32_t tm_56c930(wl_char *self);
void tm_56cc10(wl_char *self);
void tm_56dc20(int32_t a1, int32_t a2, int32_t a3);
int32_t tm_568de0(wl_char *self, int32_t a2);
int32_t tm_56a220(wl_char *self);
int32_t tm_41c150(int32_t * a1, int32_t * a2, int32_t a3);
int32_t tm_41c240(wl_char *self, int32_t a2);
int32_t tm_560a40(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_41c2f0(wl_char *self, int32_t * a2, uint16_t * a3);
int32_t tm_41c320(wl_char *self, int32_t * a2, uint16_t a3);
int32_t tm_560a00(wl_char *self, int32_t a2);
void tm_562a10(wl_char *self);
int32_t tm_56aea0(wl_char *self, int32_t a2);
int32_t tm_41bb80(wl_char *self, uint16_t a2, int32_t a3);
int32_t tm_56bfd0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_56c030(wl_char *self, uint16_t a2);
int32_t tm_56c070(wl_char *self, int16_t a2, int16_t a3, int16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int16_t a10, uint16_t a11);
void tm_56c180(wl_char *self, int32_t a2);
void tm_56c2b0(wl_char *self);
void tm_562af0(wl_char *self);
int32_t tm_56c470(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t tm_56c4e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t tm_56c580(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
int32_t tm_56c610(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_56cb90(wl_char *self, uint16_t a2, int32_t a3, int32_t a4);
int32_t tm_56cd20(wl_char *self);
int32_t tm_40bb70(wl_char *self, char a2);
int32_t tm_56d710(wl_char *self);
int32_t tm_56de80(wl_char *self);
void tm_59ec30(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_59ee50(wl_char *self, int32_t a2, int32_t a3);
int32_t tm_41d780(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void tm_49a6e0(wl_char *self);

#endif /* OSS_TITLE_MENU_H */
