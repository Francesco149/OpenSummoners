/*
 * zdd_dev.{c,h} — the DirectDraw DEVICE layer (device, display mode, primary surface + back buffers, palette, flip,
 * lost-surface checks, frees, the DDERR log) + the alpha-blend LUT set-up.  Retail layout; rhost-verified inside
 * retail (REPLACE: the render gate's back-buffer hash on every Flip).  Names by VA.
 *
 *   FUN_005b7ee0  zd_5b7ee0   FUN_005b7f80  zd_5b7f80   FUN_005b7fe0  zd_5b7fe0   FUN_005b8040  zd_5b8040
 *   FUN_005b80d0  zd_5b80d0   FUN_005b8480  zd_5b8480   FUN_005b88c0  zd_5b88c0   FUN_005b8900  zd_5b8900
 *   FUN_005b8950  zd_5b8950   FUN_005b89d0  zd_5b89d0   FUN_005b8a20  zd_5b8a20   FUN_005b8ae0  zd_5b8ae0
 *   FUN_005b8da0  zd_5b8da0   FUN_005b8dd0  zd_5b8dd0   FUN_005b8e00  zd_5b8e00   FUN_005b8ea0  zd_5b8ea0
 *   FUN_005b8fc0  zd_5b8fc0   FUN_005b9130  zd_5b9130   FUN_005b91d0  zd_5b91d0   FUN_005b9240  zd_5b9240
 *   FUN_005b9410  zd_5b9410   FUN_005b9740  zd_5b9740   FUN_005bcff0  zd_5bcff0   FUN_005bd020  zd_5bd020
 *   FUN_005bd040  zd_5bd040   FUN_005bd380  zd_5bd380   FUN_005bd3b0  zd_5bd3b0   FUN_005bd3d0  zd_5bd3d0
 *   FUN_005bd4d0  zd_5bd4d0   FUN_005bd520  zd_5bd520
 */
#ifndef OSS_ZDD_DEV_H
#define OSS_ZDD_DEV_H

#include <stdint.h>

#include "world_layout.h"

int32_t zd_5b7ee0(intptr_t * a1);
int32_t zd_5b7f80(wl_char *self);
void zd_5b7fe0(wl_char *self);
int32_t zd_5b8040(wl_char *self);
void zd_5b80d0(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t zd_5b8480(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, void * a7);
int32_t zd_5b88c0(wl_char *self);
int32_t zd_5b8900(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t zd_5b8950(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4);
int32_t zd_5b89d0(wl_char *self, int32_t a2, int32_t a3);
int32_t zd_5b8a20(wl_char *self, int32_t * a2);
int32_t zd_5b8ae0(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4);
int32_t zd_5b8da0(wl_char *self);
int32_t zd_5b8dd0(wl_char *self);
int32_t zd_5b8e00(wl_char *self);
int32_t zd_5b8ea0(int32_t a1, int32_t a2, int32_t a3);
void zd_5b8fc0(wl_char *self, int32_t hWnd);
int32_t zd_5b9130(wl_char *self, int32_t target);
int32_t zd_5b91d0(wl_char *self);
int32_t zd_5b9240(wl_char *self);
int32_t zd_5b9410(wl_char *self);
int32_t zd_5b9740(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
void zd_5bcff0(int32_t a1);
int32_t zd_5bd020(int32_t a1);
int32_t zd_5bd040(wl_char *self, int32_t a2, int32_t a3);
int32_t zd_5bd380(uint32_t a1);
int32_t zd_5bd3b0(wl_char *self, int16_t a2, int16_t a3, int16_t a4);
int32_t zd_5bd3d0(wl_char *self, int32_t * a2);
int32_t zd_5bd4d0(wl_char *self);
void zd_5bd520(wl_char *self);

#endif /* OSS_ZDD_DEV_H */
