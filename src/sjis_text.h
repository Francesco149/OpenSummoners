/*
 * sjis_text.{c,h} — the UI text layer's SJIS word wrap + ruby layout.  Retail
 * layout; rhost REPLACE-wired (UNVERIFIED: EN corpus).  Names by VA.
 *
 *   FUN_004034f0  sj_4034f0   FUN_004051d0  sj_4051d0
 */
#ifndef OSS_SJIS_TEXT_H
#define OSS_SJIS_TEXT_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t sj_4034f0(char * a1, int32_t a2, int32_t a3, int32_t a4);
int32_t sj_4051d0(int32_t a1, char * a2);

#endif /* OSS_SJIS_TEXT_H */
