/*
 * sound_dev.{c,h} — the SOUND layer (DirectSound: the device, the sound buffers from WAVE resources / files) and the
 * MUSIC manager (DirectShow tracks: the BGM table, load / play / stop, fades, loop points, volumes).  Retail layout;
 * rhost-verified inside retail (REPLACE: rhost's fake DirectSound + its virtual-clock FilterGraph answer the same COM
 * calls).  Names by VA.
 *
 *   FUN_005ba740  sd_5ba740   FUN_005ba7e0  sd_5ba7e0   FUN_005ba820  sd_5ba820   FUN_005ba850  sd_5ba850
 *   FUN_005ba860  sd_5ba860   FUN_005ba9a0  sd_5ba9a0   FUN_005baa60  sd_5baa60   FUN_005baa80  sd_5baa80
 *   FUN_005baaa0  sd_5baaa0   FUN_005bab10  sd_5bab10   FUN_005bad20  sd_5bad20   FUN_005bad40  sd_5bad40
 *   FUN_005bad60  sd_5bad60   FUN_005bad80  sd_5bad80   FUN_005badd0  sd_5badd0   FUN_005bae00  sd_5bae00
 *   FUN_005bae30  sd_5bae30   FUN_005bae70  sd_5bae70   FUN_005bae90  sd_5bae90   FUN_005baec0  sd_5baec0
 *   FUN_005baed0  sd_5baed0   FUN_005bafa0  sd_5bafa0   FUN_005bafe0  sd_5bafe0   FUN_005bb040  sd_5bb040
 *   FUN_005bb180  sd_5bb180   FUN_005bb250  sd_5bb250   FUN_005bb2f0  sd_5bb2f0   FUN_005bb380  sd_5bb380
 *   FUN_005bb3a0  sd_5bb3a0   FUN_005bb3d0  sd_5bb3d0   FUN_005bb5c0  sd_5bb5c0   FUN_005bb610  sd_5bb610
 *   FUN_005bb740  sd_5bb740   FUN_005bb870  sd_5bb870   FUN_005bb880  sd_5bb880   FUN_005bb890  sd_5bb890
 *   FUN_005bb8c0  sd_5bb8c0   FUN_005bb8e0  sd_5bb8e0   FUN_005bb8f0  sd_5bb8f0   FUN_005bb930  sd_5bb930
 *   FUN_005bb9b0  sd_5bb9b0   FUN_005bbb10  sd_5bbb10   FUN_005bbb50  sd_5bbb50   FUN_005bbc20  sd_5bbc20
 *   FUN_005bbc60  sd_5bbc60   FUN_005bbc90  sd_5bbc90   FUN_005bbcd0  sd_5bbcd0   FUN_005bbd20  sd_5bbd20
 *   FUN_005bbdb0  sd_5bbdb0   FUN_005bbe20  sd_5bbe20   FUN_005bbeb0  sd_5bbeb0   FUN_005bc0c0  sd_5bc0c0
 *   FUN_005bc150  sd_5bc150   FUN_005bc7c0  sd_5bc7c0   FUN_005bcb30  sd_5bcb30   FUN_005bcb80  sd_5bcb80
 *   FUN_005bcdf0  sd_5bcdf0   FUN_005bce40  sd_5bce40   FUN_005bce80  sd_5bce80   FUN_005bceb0  sd_5bceb0
 *   FUN_005bcf30  sd_5bcf30   FUN_005bcfa0  sd_5bcfa0   FUN_005bcfb0  sd_5bcfb0
 */
#ifndef OSS_SOUND_DEV_H
#define OSS_SOUND_DEV_H

#include <stdint.h>

#include "world_layout.h"

int32_t sd_5ba740(char ** a1);
int32_t sd_5ba7e0(wl_char *self);
void sd_5ba820(wl_char *self);
int32_t sd_5ba850(void);
void sd_5ba860(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t sd_5ba9a0(wl_char *self, int32_t * a2, char * Source);
int32_t sd_5baa60(wl_char *self, int32_t * a2);
int32_t sd_5baa80(wl_char *self);
int32_t sd_5baaa0(wl_char *self, char * Source);
int32_t sd_5bab10(wl_char *self);
int32_t sd_5bad20(wl_char *self);
int32_t sd_5bad40(wl_char *self);
int32_t sd_5bad60(wl_char *self);
int32_t sd_5bad80(wl_char *self);
int32_t sd_5badd0(wl_char *self, int32_t a2);
int32_t sd_5bae00(wl_char *self, int32_t a2);
int32_t sd_5bae30(wl_char *self, int32_t a2, int32_t a3);
int64_t sd_5bae70(wl_char *self);
int64_t sd_5bae90(wl_char *self);
int32_t sd_5baec0(wl_char *self);
int32_t sd_5baed0(wl_char *self, char ** a2, int32_t a3);
int32_t sd_5bafa0(wl_char *self);
void sd_5bafe0(wl_char *self);
void sd_5bb040(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t sd_5bb180(wl_char *self, int32_t a2);
int32_t sd_5bb250(wl_char *self, int32_t * a2, int32_t hModule, int32_t a4, int32_t a5, int32_t a6);
int32_t sd_5bb2f0(wl_char *self, int32_t * a2, int32_t a3);
int32_t sd_5bb380(wl_char *self, int32_t a2);
int32_t sd_5bb3a0(wl_char *self);
int32_t sd_5bb3d0(int32_t * a1, char * pszFileName, int32_t hModule, uint16_t a5, int32_t a6, char ** a8, int32_t a9);
int32_t sd_5bb5c0(wl_char *self, int32_t * a2, int32_t a3);
int32_t sd_5bb610(char * pszFileName, char * pch, int32_t * a3, intptr_t * a4);
int32_t sd_5bb740(int32_t hModule, uint16_t a2, int32_t * a3, int32_t * a4, uint32_t * a5);
int32_t sd_5bb870(wl_char *self, int32_t a2);
int32_t sd_5bb880(wl_char *self, int32_t a2);
int32_t sd_5bb890(wl_char *self, int32_t a2);
int32_t sd_5bb8c0(wl_char *self);
int32_t sd_5bb8e0(wl_char *self);
int32_t sd_5bb8f0(wl_char *self, int32_t a2);
int32_t sd_5bb930(wl_char *self, int32_t a2);
int32_t sd_5bb9b0(wl_char *self, int32_t a2, int32_t a3);
int32_t sd_5bbb10(wl_char *self);
void sd_5bbb50(wl_char *self);
int32_t sd_5bbc20(wl_char *self);
int32_t sd_5bbc60(wl_char *self, uint16_t a2);
int32_t sd_5bbc90(wl_char *self, uint16_t a2);
void sd_5bbcd0(wl_char *self, int32_t a2);
void sd_5bbd20(wl_char *self, int32_t active);
int32_t sd_5bbdb0(wl_char *self, int32_t a2, int32_t a3);
int32_t sd_5bbe20(wl_char *self, int32_t a2);
int32_t sd_5bbeb0(wl_char *self, int16_t a2, int16_t a3);
void sd_5bc0c0(wl_char *self);
int32_t sd_5bc150(wl_char *self, int32_t hModule, uint32_t NumberOfBytesWritten, char * lpFileName, char * lpType);
int32_t sd_5bc7c0(wl_char *self, uint16_t a2, int32_t hModule, uint32_t NumberOfBytesWritten, char * lpFileName, char * a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13);
int32_t sd_5bcb30(wl_char *self);
int32_t sd_5bcb80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t sd_5bcdf0(wl_char *self, uint16_t a2);
int32_t sd_5bce40(wl_char *self, int32_t a2);
int32_t sd_5bce80(wl_char *self, uint16_t a2);
int32_t sd_5bceb0(wl_char *self, int32_t a2);
int32_t sd_5bcf30(wl_char *self, uint16_t a2);
int32_t sd_5bcfa0(wl_char *self);
int32_t sd_5bcfb0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

#endif /* OSS_SOUND_DEV_H */
