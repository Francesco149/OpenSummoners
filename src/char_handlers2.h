/*
 * char_handlers2.{c,h} — the integrator's per-type handlers the corpus does not reach yet + their helpers.  Retail
 * layout; rhost REPLACE-wired (UNVERIFIED until a recording meets them).  Names by VA.
 *
 *   FUN_00445e50  ih_445e50   FUN_00446900  ih_446900   FUN_004484b0  ih_4484b0   FUN_00448dc0  ih_448dc0
 *   FUN_00449810  ih_449810   FUN_0044a160  ih_44a160   FUN_0044a200  ih_44a200   FUN_0044a740  ih_44a740
 *   FUN_0044b160  ih_44b160   FUN_0044bc10  ih_44bc10   FUN_0044c250  ih_44c250   FUN_0044d8f0  ih_44d8f0
 *   FUN_0044e1c0  ih_44e1c0   FUN_0044e730  ih_44e730   FUN_0044e8d0  ih_44e8d0   FUN_0044f690  ih_44f690
 *   FUN_00450810  ih_450810   FUN_004516e0  ih_4516e0   FUN_00452020  ih_452020   FUN_00452730  ih_452730
 *   FUN_004539b0  ih_4539b0   FUN_00455c90  ih_455c90   FUN_00455cf0  ih_455cf0   FUN_00455d20  ih_455d20
 *   FUN_00456d30  ih_456d30   FUN_0045e260  ih_45e260   FUN_0045e3e0  ih_45e3e0   FUN_00461640  ih_461640
 *   FUN_00467330  ih_467330
 */
#ifndef OSS_CHAR_HANDLERS2_H
#define OSS_CHAR_HANDLERS2_H

#include <stdint.h>

#include "world_layout.h"

int32_t ih_445e50(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5);
int32_t ih_446900(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t * a4, int32_t i, int32_t a6, int32_t * a7, int32_t a8);
int32_t ih_4484b0(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ih_448dc0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
void ih_449810(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);
int32_t ih_44a160(int32_t a1, int32_t a2, int16_t a3, int32_t a4);
int32_t ih_44a200(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_44a740(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t a6, int32_t a7);
int32_t ih_44b160(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t a6, int32_t a7);
void ih_44bc10(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5);
int32_t ih_44c250(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5);
int32_t ih_44d8f0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);
int32_t ih_44e1c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ih_44e730(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6);
int32_t ih_44e8d0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_44f690(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9);
int32_t ih_450810(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_4516e0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ih_452020(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ih_452730(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6);
int32_t ih_4539b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, int32_t a7);
int32_t ih_455c90(wl_char *self, uint16_t a2, int32_t a3);
int32_t ih_455cf0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t ih_455d20(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4);
int32_t ih_456d30(wl_char *self, int32_t * a3, int32_t a4, uint16_t a5, int32_t a6, int32_t a7);
int32_t ih_45e260(wl_char *self, int32_t * a2, int32_t * a3, int32_t a4);
int32_t ih_45e3e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t ih_461640(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_467330(wl_char *self);

#endif /* OSS_CHAR_HANDLERS2_H */
