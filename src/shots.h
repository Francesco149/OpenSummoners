/*
 * shots.{c,h} — the SHOTS (band-64 entities: projectiles, spell bodies) on the retail layout (rhost-verified inside
 * retail): the per-tick update 0x4710c0 (the tick driver 0x46cd70 runs it per live shot part), the shot init
 * 0x55d140 (shot_spawn's), the 96-band update 0x46da00 (pickups) and their helpers.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (IDA's control flow and local types kept), finished by hand; the
 * helper names are their VAs.  Reached by the corpus (rh-combat): shots_tick, shot_init, band96_tick and the
 * helpers 0x4504c0 0x46e190 0x476550 0x476ba0 0x476ef0 0x477070 0x477310; the rest are unexercised.
 *
 *   FUN_004710c0  shots_tick         FUN_0055d140  shot_init          FUN_0046da00  band96_tick
 *   FUN_00590230  room_band64_tick (the room loop's: the parts' shots_tick + the looping voices' fade / place)
 *   FUN_00471080  ftol_round_ (inline: rounded half away from zero)
 *   FUN_00445ce0  shot_445ce0        FUN_00449700  shot_449700
 *   FUN_004497b0  shot_4497b0        FUN_004504c0  shot_4504c0        FUN_004538f0  shot_4538f0
 *   FUN_0046e110  shot_46e110        FUN_0046e150  shot_46e150        FUN_0046e190  shot_46e190
 *   FUN_00476500  shot_476500        FUN_00476550  shot_476550        FUN_00476740  shot_476740
 *   FUN_00476a80  shot_476a80        FUN_00476ab0  shot_476ab0        FUN_00476ba0  shot_476ba0
 *   FUN_00476d30  shot_476d30        FUN_00476e20  shot_476e20        FUN_00476ef0  shot_476ef0
 *   FUN_00477070  shot_477070        FUN_004771d0  shot_4771d0        FUN_00477310  shot_477310
 *   FUN_00477560  shot_477560        FUN_00477650  shot_477650        FUN_00477b70  shot_477b70
 *   FUN_00477f10  shot_477f10        FUN_004781f0  shot_4781f0        FUN_00478550  shot_478550
 *   FUN_004785a0  shot_4785a0        FUN_00478820  shot_478820        FUN_0054cab0  shot_54cab0
 *   FUN_0054cca0  shot_54cca0        FUN_0055f150  shot_55f150        FUN_0054cfc0  shot_54cfc0
 */
#ifndef OSS_SHOTS_H
#define OSS_SHOTS_H

#include <stdint.h>

#include "world_layout.h"

int32_t shots_tick(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t shot_init(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int16_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15, int32_t a16, int32_t a17, int32_t a18);
int32_t band96_tick(wl_char *self);
int32_t shot_445ce0(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t shot_449700(wl_char *self, int32_t a2, int32_t a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t shot_4497b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t shot_4504c0(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14);
int32_t shot_4538f0(int32_t * a1, int32_t * a2, int32_t a3);
int32_t shot_46e110(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t shot_46e150(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t shot_46e190(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t shot_476500(int32_t a1, int32_t a2, int32_t a3);
int32_t shot_476550(int32_t a1, int32_t a2, int32_t * a3, double * a4);
int32_t shot_476740(wl_char *self, int32_t a2);
int32_t shot_476a80(int32_t * a1);
int32_t shot_476ab0(wl_char *self, int32_t * a1, int32_t a2, uint16_t a3, int32_t a4);
int64_t shot_476ba0(int32_t * a1, int32_t a2);
int32_t shot_476d30(wl_char *self, int32_t a2, uint16_t a3, uint16_t a4, int32_t a5);
int32_t shot_476e20(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_476ef0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13);
int32_t shot_477070(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_4771d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, uint16_t a5, int32_t * a6, int32_t * a7, int32_t a8, int32_t a9, int32_t a10);
int32_t shot_477310(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t shot_477560(wl_char *self, int32_t * a1, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t shot_477650(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t shot_477b70(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_477f10(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_4781f0(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_478550(int32_t a1, int32_t a2);
int32_t shot_4785a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);
int32_t shot_478820(int32_t * a1, uint16_t a2, int32_t a3, int32_t * a4, int32_t a5);
int32_t shot_54cab0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint16_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12);
int32_t shot_54cca0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t shot_55f150(int32_t a1, int32_t a2);
double shot_54cfc0(int32_t a1, int32_t a2);
int32_t room_band64_tick(wl_char *self);

#endif /* OSS_SHOTS_H */
