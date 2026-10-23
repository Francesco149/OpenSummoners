/*
 * actor_behave.{c,h} — the actor behaviours the corpus does not reach yet.  Retail layout;
 * rhost REPLACE-wired (UNVERIFIED).  Names by VA.
 *
 *   FUN_0054f140  ab_54f140   FUN_00552c70  ab_552c70   FUN_00552f60  ab_552f60   FUN_00553520  ab_553520
 *   FUN_00553ec0  ab_553ec0   FUN_00554570  ab_554570   FUN_00554c40  ab_554c40   FUN_005554e0  ab_5554e0
 *   FUN_005556d0  ab_5556d0
 */
#ifndef OSS_ACTOR_BEHAVE_H
#define OSS_ACTOR_BEHAVE_H

#include <stdint.h>
#include <stdio.h>

#include "world_layout.h"

int32_t ab_54f140(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ab_552c70(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t ab_552f60(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t ab_553520(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ab_553ec0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ab_554570(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ab_554c40(wl_char *self, int32_t a2, FILE * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ab_5554e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ab_5556d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

#endif /* OSS_ACTOR_BEHAVE_H */
