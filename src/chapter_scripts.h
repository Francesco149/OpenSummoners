/*
 * chapter_scripts.{c,h} — the CHAPTER SCRIPTS the corpus reaches, on the retail layout (verified inside retail
 * by the hybrid gate: they run nested frame loops, so REPLACE only).  The script engine (src/script_engine.c)
 * runs `chs_find(va)` when the script is ported, else retail's by VA.
 *
 *   FUN_004d7d80  chs_4d7d80    FUN_004dc510  chs_4dc510    FUN_004d64f0  chs_4d64f0
 *   FUN_004cd6d0  chs_4cd6d0    FUN_004d13a0  chs_4d13a0    FUN_004ccc90  chs_4ccc90
 *   FUN_004c6b40  chs_4c6b40    FUN_004e19e0  chs_4e19e0    FUN_0049d930  chs_49d930
 *   FUN_004a0c90  chs_4a0c90    FUN_0049f7c0  chs_49f7c0    FUN_004a6b80  chs_4a6b80
 *   FUN_004ac6d0  chs_4ac6d0    FUN_004a66a0  chs_4a66a0    FUN_004a5fa0  chs_4a5fa0
 *   FUN_004ae150  chs_4ae150    FUN_004ae420  chs_4ae420    FUN_004afdb0  chs_4afdb0
 *   FUN_004b3b20  chs_4b3b20    FUN_004b3c70  chs_4b3c70    FUN_004b4d40  chs_4b4d40
 *   FUN_004bbef0  chs_4bbef0    FUN_004bd2e0  chs_4bd2e0    FUN_004bd6b0  chs_4bd6b0
 *   FUN_004bd820  chs_4bd820    FUN_004bdf90  chs_4bdf90    FUN_004bdbe0  chs_4bdbe0
 *   FUN_004be220  chs_4be220    FUN_004c1a30  chs_4c1a30    FUN_004c1c20  chs_4c1c20
 *   FUN_004c2310  chs_4c2310    FUN_004c29c0  chs_4c29c0    FUN_004c2dc0  chs_4c2dc0
 *   FUN_004c3ee0  chs_4c3ee0    FUN_004c45a0  chs_4c45a0
 */
#ifndef OSS_CHAPTER_SCRIPTS_H
#define OSS_CHAPTER_SCRIPTS_H

#include <stdint.h>

#include "world_layout.h"

typedef int32_t (*chs_fn)(wl_char *runner);
/* the ported script at retail VA `va` (thiscall runner = the event), NULL when not ported */
chs_fn chs_find(uint32_t va);

int32_t chs_4d7d80(wl_char *self);
int32_t chs_4dc510(wl_char *self);
int32_t chs_4d64f0(wl_char *self);
int32_t chs_4cd6d0(wl_char *self);
int32_t chs_4d13a0(wl_char *self);
int32_t chs_4ccc90(wl_char *self);
int32_t chs_4c6b40(wl_char *self);
int32_t chs_4e19e0(wl_char *self);
int32_t chs_49d930(wl_char *self);
int32_t chs_4a0c90(wl_char *self);
int32_t chs_49f7c0(wl_char *self);
int32_t chs_4a6b80(wl_char *self);
int32_t chs_4ac6d0(wl_char *self);
int32_t chs_4a66a0(wl_char *self);
int32_t chs_4a5fa0(wl_char *self);
int32_t chs_4ae150(wl_char *self);
int32_t chs_4ae420(wl_char *self);
int32_t chs_4afdb0(wl_char *self);
int32_t chs_4b3b20(wl_char *self);
int32_t chs_4b3c70(wl_char *self);
int32_t chs_4b4d40(wl_char *self);
int32_t chs_4bbef0(wl_char *self);
int32_t chs_4bd2e0(wl_char *self);
int32_t chs_4bd6b0(wl_char *self);
int32_t chs_4bd820(wl_char *self);
int32_t chs_4bdf90(wl_char *self);
int32_t chs_4bdbe0(wl_char *self);
int32_t chs_4be220(wl_char *self);
int32_t chs_4c1a30(wl_char *self);
int32_t chs_4c1c20(wl_char *self);
int32_t chs_4c2310(wl_char *self);
int32_t chs_4c29c0(wl_char *self);
int32_t chs_4c2dc0(wl_char *self);
int32_t chs_4c3ee0(wl_char *self);
int32_t chs_4c45a0(wl_char *self);

#endif /* OSS_CHAPTER_SCRIPTS_H */
