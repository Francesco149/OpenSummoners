/*
 * render_platform.{c,h} — the frame RENDER on the retail layout (rhost-verified inside retail by the render gate:
 * every presented frame's pixels, pure vs hybrid): the frame render 0x48c150 (room / scene / menu), its bands
 * (characters, actors, effects, glyphs), the message box, the HUD, the sprite-frame resolver, the palette grade.
 * Names by VA.  Retail's callers reach 11 of them (hooked, REPLACE only); the rest are called from these.
 *
 *   FUN_005b5eb0  rp_5b5eb0   FUN_005b8b40  rp_5b8b40   FUN_005b9350  rp_5b9350   FUN_005b9390  rp_5b9390
 *   FUN_005b93e0  rp_5b93e0   FUN_005b95c0  rp_5b95c0   FUN_005b8c00  rp_5b8c00   FUN_005b97e0  rp_5b97e0
 *   FUN_005b9830  rp_5b9830   FUN_005b8b00  rp_5b8b00   FUN_005b98c0  rp_5b98c0   FUN_005b9450  rp_5b9450
 *   FUN_005b9490  rp_5b9490   FUN_005b94d0  rp_5b94d0   FUN_005b94e0  rp_5b94e0   FUN_005b9500  rp_5b9500
 *   FUN_005b9520  rp_5b9520   FUN_005b9a40  rp_5b9a40   FUN_005b9ae0  rp_5b9ae0   FUN_005b9b70  rp_5b9b70
 *   FUN_005b9bf0  rp_5b9bf0   FUN_005bd550  rp_5bd550   FUN_005bd680  rp_5bd680
 */
#ifndef OSS_RENDER_PLATFORM_H
#define OSS_RENDER_PLATFORM_H

#include <stdint.h>

#include "world_layout.h"

int32_t rp_5b5eb0(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rp_5b8b40(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t rp_5b9350(wl_char *self, int32_t a2);
int32_t rp_5b9390(wl_char *self);
int32_t rp_5b93e0(wl_char *self);
int32_t rp_5b95c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10);
int32_t rp_5b8c00(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t rp_5b97e0(wl_char *self, int32_t a2, int32_t a3);
int32_t rp_5b9830(wl_char *self, int32_t a2);
int32_t rp_5b8b00(wl_char *self, int32_t a2);
int32_t rp_5b98c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rp_5b9450(wl_char *self, int32_t a2);
int32_t rp_5b9490(wl_char *self);
int32_t rp_5b94d0(wl_char *self);
int32_t rp_5b94e0(wl_char *self, intptr_t * out_src_hdc);
void rp_5b9500(wl_char *self, int32_t src_hdc);
int32_t rp_5b9520(wl_char *self);
int32_t rp_5b9a40(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t rp_5b9ae0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10);
int32_t rp_5b9b70(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t rp_5b9bf0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rp_5bd550(wl_char *self, int32_t a2, int32_t a3, int32_t x1, int32_t y1, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t hdcSrc);
int32_t rp_5bd680(wl_char *self, int32_t ** a2, int32_t ** a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10);

#endif /* OSS_RENDER_PLATFORM_H */
