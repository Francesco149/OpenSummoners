/*
 * story_scripts.{c,h} — the LATER chapters' story scripts (0x4e1d10..0x546670) + their helpers, on the retail
 * layout.  The script engine (src/script_engine.c) runs `story_find(va)` when the script is ported, else retail's
 * by VA.  The corpus (chapter 1) does not reach them: UNVERIFIED first passes (they compile + link; rhost wires
 * them REPLACE-only — nested frame loops — as the chapter scripts).  Names by VA.
 *
 *   FUN_004e1d10  chs_4e1d10   FUN_004e23d0  chs_4e23d0   FUN_004e5950  chs_4e5950   FUN_004e68d0  chs_4e68d0
 *   FUN_004e6a60  chs_4e6a60   FUN_004ea6f0  chs_4ea6f0   FUN_004ea710  chs_4ea710   FUN_004ea7b0  chs_4ea7b0
 *   FUN_004ee490  chs_4ee490   FUN_004ee690  chs_4ee690   FUN_004ef4e0  chs_4ef4e0   FUN_004f19e0  chs_4f19e0
 *   FUN_004f69c0  chs_4f69c0   FUN_004f69e0  chs_4f69e0   FUN_004f6a10  chs_4f6a10   FUN_004fa2a0  chs_4fa2a0
 *   FUN_004fea60  chs_4fea60   FUN_004fec40  chs_4fec40   FUN_005034b0  chs_5034b0   FUN_00507e70  chs_507e70
 *   FUN_0050ffe0  chs_50ffe0   FUN_005100e0  chs_5100e0   FUN_00510100  chs_510100   FUN_00515a00  chs_515a00
 *   FUN_0051a1d0  chs_51a1d0   FUN_00522310  chs_522310   FUN_00523010  chs_523010   FUN_00523300  chs_523300
 *   FUN_00525ae0  chs_525ae0   FUN_00525e70  chs_525e70   FUN_0052fd80  chs_52fd80   FUN_00530080  chs_530080
 *   FUN_00533570  chs_533570   FUN_00533580  chs_533580   FUN_00535520  chs_535520   FUN_00536480  chs_536480
 *   FUN_00537440  chs_537440   FUN_00539e80  chs_539e80   FUN_0053ebe0  chs_53ebe0   FUN_0053ec10  chs_53ec10
 *   FUN_0053ec20  chs_53ec20   FUN_0053ec40  chs_53ec40   FUN_005402e0  chs_5402e0   FUN_00546580  chs_546580
 *   FUN_00546670  chs_546670   FUN_0054be50  chs_54be50   FUN_0054be90  chs_54be90   FUN_0054bed0  chs_54bed0
 */
#ifndef OSS_STORY_SCRIPTS_H
#define OSS_STORY_SCRIPTS_H

#include <stdint.h>
#include <stdio.h>

#include "chapter_scripts.h"
#include "world_layout.h"

/* the ported story script at retail VA `va` (thiscall runner = the event), NULL when not one */
chs_fn story_find(uint32_t va);

int32_t chs_4e1d10(wl_char *self);
int32_t chs_4e23d0(wl_char *self);
int32_t chs_4e5950(wl_char *self);
int32_t chs_4e68d0(wl_char *self);
int32_t chs_4e6a60(wl_char *self);
int32_t chs_4ea6f0(wl_char *self, int32_t a2);
int32_t chs_4ea710(wl_char *self, int32_t a2);
int32_t chs_4ea7b0(wl_char *self);
int32_t chs_4ee490(wl_char *self, int32_t a2, int32_t a3);
int32_t chs_4ee690(wl_char *self);
int32_t chs_4ef4e0(wl_char *self);
int32_t chs_4f19e0(wl_char *self);
void chs_4f69c0(wl_char *self, int32_t a1);
int32_t chs_4f69e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t chs_4f6a10(wl_char *self);
int32_t chs_4fa2a0(wl_char *self);
int32_t chs_4fea60(wl_char *self, int32_t a2);
int32_t chs_4fec40(wl_char *self);
int32_t chs_5034b0(wl_char *self);
int32_t chs_507e70(wl_char *self);
int32_t chs_50ffe0(wl_char *self, uint16_t a2);
int32_t chs_5100e0(wl_char *self);
int32_t chs_510100(wl_char *self);
int32_t chs_515a00(wl_char *self);
int32_t chs_51a1d0(wl_char *self);
int32_t chs_522310(wl_char *self);
int32_t chs_523010(wl_char *self);
int32_t chs_523300(wl_char *self);
int32_t chs_525ae0(wl_char *self);
int32_t chs_525e70(wl_char *self);
int32_t chs_52fd80(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t chs_530080(wl_char *self);
int32_t chs_533570(wl_char *self);
int32_t chs_533580(wl_char *self);
int32_t chs_535520(wl_char *self);
int32_t chs_536480(wl_char *self);
int32_t chs_537440(wl_char *self);
int32_t chs_539e80(wl_char *self);
int32_t chs_53ebe0(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3);
int32_t chs_53ec10(wl_char *self, int32_t a2);
int32_t chs_53ec20(wl_char *self, int32_t a2, int32_t a3);
int32_t chs_53ec40(wl_char *self);
int32_t chs_5402e0(wl_char *self);
int32_t chs_546580(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t chs_546670(wl_char *self);
void chs_54be50(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t chs_54be90(wl_char *self, int32_t a2);
int32_t chs_54bed0(wl_char *self, int32_t a2, int16_t a3);

#endif /* OSS_STORY_SCRIPTS_H */
