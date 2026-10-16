/*
 * behave_helpers.{c,h} — the BEHAVIOUR HELPERS (monsters' and NPCs' AI: target picks, skill choice by range,
 * approach / retreat moves, the attack windows) on the retail layout (rhost-verified inside retail): every slot of
 * the AI tick 0x47b990 (src/char_behave.c) and their callees.
 *
 * A FIRST PASS through tools/ida/decomp2c.py, finished by hand; the names are their VAs.
 *
 *   FUN_00441f50  beh_441f50          FUN_0047adb0  beh_47adb0          FUN_0047d790  beh_47d790
 *   FUN_0047da60  beh_47da60          FUN_0047de50  beh_47de50          FUN_0047e010  beh_47e010
 *   FUN_0047e7c0  beh_47e7c0          FUN_0047eae0  beh_47eae0          FUN_0047ecd0  beh_47ecd0
 *   FUN_0047f670  beh_47f670          FUN_0047f9d0  beh_47f9d0          FUN_00481310  beh_481310
 *   FUN_00481ac0  beh_481ac0          FUN_00482240  beh_482240          FUN_00482380  beh_482380
 *   FUN_00482460  beh_482460          FUN_00482880  beh_482880          FUN_00482ac0  beh_482ac0
 *   FUN_00482e10  beh_482e10          FUN_004834f0  beh_4834f0          FUN_00483b50  beh_483b50
 *   FUN_00483de0  beh_483de0          FUN_00483fb0  beh_483fb0          FUN_00484530  beh_484530
 *   FUN_00484ea0  beh_484ea0          FUN_00485350  beh_485350          FUN_004854e0  beh_4854e0
 *   FUN_00485990  beh_485990          FUN_00442490  beh_442490          FUN_0047ae00  beh_47ae00
 *   FUN_0047f610  beh_47f610          FUN_0047fd40  beh_47fd40          FUN_00480b30  beh_480b30
 *   FUN_0048bdd0  beh_48bdd0          FUN_0048be60  beh_48be60          FUN_00479e40  beh_479e40
 *   FUN_00481790  beh_481790          FUN_00481a20  beh_481a20          FUN_00479e10  beh_479e10
 *   FUN_0047a970  beh_47a970          FUN_004826b0  beh_4826b0          FUN_00482700  beh_482700
 *   FUN_00482760  beh_482760          FUN_00483040  beh_483040          FUN_00484830  beh_484830
 *   FUN_00484d60  beh_484d60          FUN_00485330  beh_485330          FUN_00483920  beh_483920
 *   FUN_004848a0  beh_4848a0          FUN_00442640  beh_442640          FUN_004843c0  beh_4843c0
 *   FUN_00442710  beh_442710          FUN_00485cb0  beh_485cb0          FUN_0047b870  beh_47b870
 *   FUN_0048b920  beh_48b920          FUN_005b4200  beh_5b4200          FUN_00480260  beh_480260
 *   FUN_00480580  beh_480580          FUN_004832e0  beh_4832e0          FUN_00484bc0  beh_484bc0
 */
#ifndef OSS_BEHAVE_HELPERS_H
#define OSS_BEHAVE_HELPERS_H

#include <stdint.h>

#include "world_layout.h"

int32_t beh_441f50(wl_char *self, int32_t * a2, int32_t a3);
int32_t beh_47adb0(intptr_t ecx_, int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t beh_47d790(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t beh_47da60(wl_char *self, int32_t * a2);
int32_t beh_47de50(int32_t * a1, int32_t a2);
int32_t beh_47e010(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t beh_47e7c0(int32_t a1, uint16_t a2, int32_t * a3, int32_t a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8);
int32_t beh_47eae0(wl_char *self, int32_t * a2, int32_t * a3, int16_t * a4, int32_t a5);
int32_t beh_47ecd0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint16_t a8, int32_t * a9, int32_t * a10);
int32_t beh_47f670(wl_char *self, uint16_t a2, int32_t a3);
int32_t beh_47f9d0(wl_char *self, uint16_t a2, int32_t a3);
int32_t beh_481310(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t beh_481ac0(wl_char *self, int32_t * a3, int32_t a4);
int32_t beh_482240(wl_char *self, int32_t a2);
int32_t beh_482380(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, uint16_t * a6, int32_t a7);
int32_t beh_482460(wl_char *self, int32_t a2);
int32_t beh_482880(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t beh_482ac0(wl_char *self, int32_t * a2);
void beh_482e10(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_4834f0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_483b50(wl_char *self, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6, uint16_t * a7, int32_t a8);
int32_t beh_483de0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_483fb0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5);
int32_t beh_484530(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t beh_484ea0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_485350(wl_char *self, int16_t a2, int16_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_4854e0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_485990(wl_char *self, int32_t * a2, int32_t a3);
int32_t beh_442490(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t * a10);
int32_t beh_47ae00(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8, int32_t * a9, int32_t a10, int32_t a11, int32_t a12, int32_t * a13);
int32_t beh_47f610(int32_t a1, uint16_t a2, int32_t a3);
int32_t beh_47fd40(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t beh_480b30(wl_char *self, int32_t a2, int32_t a3, uint16_t a4, int32_t a5, int32_t a6, int32_t * a7);
int32_t beh_48bdd0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_48be60(wl_char *self, uint16_t a2, int32_t a3);
int32_t beh_479e40(int32_t a1, int16_t a2, int16_t a3, int16_t a4, int32_t a5);
int32_t beh_481790(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8);
int32_t beh_481a20(wl_char *self, uint16_t a2, uint16_t a3);
int32_t beh_479e10(int32_t a1, int16_t a2, int16_t a3, int32_t a4);
int32_t beh_47a970(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t i, int32_t a8);
int32_t beh_4826b0(wl_char *self, int32_t * a2);
int32_t beh_482700(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t beh_482760(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_483040(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t beh_484830(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_484d60(wl_char *self, int32_t a2);
int32_t beh_485330(wl_char *self);
int32_t beh_483920(int32_t a1, int32_t * a2, int32_t * a3, uint16_t a4);
int32_t beh_4848a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t beh_442640(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_4843c0(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4, uint16_t a5, int32_t a6);
int32_t beh_442710(wl_char *self, uint16_t a2);
int32_t beh_485cb0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t beh_47b870(int32_t a1, int32_t a2);
int32_t beh_48b920(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t beh_5b4200(wl_char *self);
int32_t beh_480260(wl_char *self, int32_t * a2, uint16_t a3, int32_t a4, int32_t * a5);
int32_t beh_480580(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, uint16_t a6, int32_t a7, int32_t a8);
int32_t beh_4832e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_484bc0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5);

#endif /* OSS_BEHAVE_HELPERS_H */
