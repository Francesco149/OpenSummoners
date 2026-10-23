/*
 * char_types.{c,h} — the character init's per-type inits the corpus does not meet yet (monsters, bosses, allies).
 * Retail layout; rhost REPLACE-wired (UNVERIFIED until a recording meets them).  Names by VA.
 *
 *   FUN_00428c00  ci_428c00   FUN_00429060  ci_429060   FUN_00429d50  ci_429d50   FUN_00429db0  ci_429db0
 *   FUN_0042a500  ci_42a500   FUN_0042a9a0  ci_42a9a0   FUN_0042b550  ci_42b550   FUN_0042b580  ci_42b580
 *   FUN_0042b5f0  ci_42b5f0   FUN_0042b840  ci_42b840   FUN_0042b890  ci_42b890   FUN_0042b9a0  ci_42b9a0
 *   FUN_0042c130  ci_42c130   FUN_0042c510  ci_42c510   FUN_0042cce0  ci_42cce0   FUN_0042d3c0  ci_42d3c0
 *   FUN_0042da50  ci_42da50   FUN_0042f840  ci_42f840   FUN_0042ff00  ci_42ff00   FUN_00430400  ci_430400
 */
#ifndef OSS_CHAR_TYPES_H
#define OSS_CHAR_TYPES_H

#include <stdint.h>

#include "world_layout.h"

int32_t ci_428c00(wl_char *self, int32_t a2, int32_t a3, int16_t a4);
int32_t ci_429060(wl_char *self);
void ci_429d50(wl_char *self, int32_t a2, int16_t a3, int16_t a4);
int32_t ci_429db0(wl_char *self, int16_t a2);
int32_t ci_42a500(wl_char *self, int16_t a2);
int32_t ci_42a9a0(wl_char *self);
int32_t ci_42b550(wl_char *self, int32_t a2, int32_t a3);
int32_t ci_42b580(wl_char *self, int32_t a2, int32_t a3, int16_t a4);
int32_t ci_42b5f0(wl_char *self);
int32_t ci_42b840(wl_char *self);
int32_t ci_42b890(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ci_42b9a0(wl_char *self, int32_t a2);
int32_t ci_42c130(wl_char *self, int16_t a2);
int32_t ci_42c510(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ci_42cce0(wl_char *self, int16_t a2);
int32_t ci_42d3c0(wl_char *self);
int32_t ci_42da50(wl_char *self, uint32_t a2, int16_t a3);
int32_t ci_42f840(wl_char *self, int32_t a2);
int32_t ci_42ff00(wl_char *self, uint32_t a2);
int32_t ci_430400(wl_char *self, int32_t a2);

#endif /* OSS_CHAR_TYPES_H */
