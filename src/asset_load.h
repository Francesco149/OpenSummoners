/*
 * asset_load.{c,h} — the BOOT asset-group loaders (sprite / sound / UI banks, palettes, the display set-up, colour
 * tables, the BGM table, data files, the input devices).  Retail layout; rhost-verified inside retail (REPLACE: the
 * state hash + the render gate over every scenario's boot).  Names by VA.
 *
 *   FUN_0056e190  al_56e190   FUN_0057ca40  al_57ca40   FUN_005752e0  al_5752e0   FUN_0057b280  al_57b280
 *   FUN_0057a330  al_57a330   FUN_00582e90  al_582e90   FUN_005749b0  al_5749b0   FUN_005748c0  al_5748c0
 *   FUN_00579680  al_579680   FUN_00579740  al_579740   FUN_00579860  al_579860   FUN_005798f0  al_5798f0
 *   FUN_00579a00  al_579a00   FUN_00579bd0  al_579bd0   FUN_00579ec0  al_579ec0   FUN_00579f40  al_579f40
 *   FUN_0057a030  al_57a030   FUN_0057a1a0  al_57a1a0   FUN_0057a260  al_57a260   FUN_00582b80  al_582b80
 *   FUN_00582d00  al_582d00   FUN_00582d10  al_582d10   FUN_00583c90  al_583c90   FUN_00583ee0  al_583ee0
 *   FUN_00563ef0  al_563ef0
 */
#ifndef OSS_ASSET_LOAD_H
#define OSS_ASSET_LOAD_H

#include <stdint.h>

#include "asset_register.h"
#include "world_layout.h"

int32_t al_56e190(void * zdd, uint16_t group, void * settings);
int32_t al_57ca40(void * zdd, uint16_t group, void * settings);
void al_5752e0(void);
int32_t al_57b280(int32_t a1, uint16_t group, void * settings);
void al_57a330(void * zdd, uint16_t group, void * settings);
int32_t al_582e90(wl_char *self);
int32_t al_5749b0(void * zdd, uint16_t group, void * settings);
int32_t al_5748c0(wl_char *self, void * zdd, void * settings, uint16_t resource_id, uint32_t width, uint32_t height, uint32_t colorkey, uint32_t scale_flag, uint32_t type, uint16_t group);
int32_t al_579680(char * a1);
int32_t al_579740(int32_t hModule, uint16_t a2, int32_t a3, uint32_t a4, int32_t a5, uint32_t nNumberOfBytesToRead);
int32_t al_579860(wl_char *self, char * a2, char * a3);
int32_t al_5798f0(wl_char *self, char * a2, char * a3);
int32_t al_579a00(int32_t a1, uint16_t group, void * settings);
int32_t al_579bd0(intptr_t ecx_, int32_t a1, uint16_t group, int32_t a3);
void al_579ec0(wl_char *self, uint16_t capacity);
int32_t al_579f40(int32_t a1, int32_t a2, int32_t a3);
void al_57a030(wl_char *self, int32_t width, int32_t height, uint32_t family, uint16_t group);
void al_57a1a0(wl_char *self, int32_t width, uint32_t color, uint16_t group, uint16_t capacity);
void al_57a260(wl_char *self, uint32_t color, uint16_t group, uint16_t capacity);
int32_t al_582b80(wl_char *self, ar_sprite_slot * dst);
void al_582d00(wl_char *self);
int32_t al_582d10(uint16_t a1, uint16_t a2, uint16_t a3, uint32_t color, int32_t a5, uint32_t a6, uint16_t a7, uint16_t a8, uint16_t a9);
int32_t al_583c90(wl_char *self, int32_t a2);
int32_t al_583ee0(void);
int32_t al_563ef0(wl_char *self, void * zds, void * settings, uint16_t resource_id, uint16_t count, uint16_t group, int32_t load_flag);

#endif /* OSS_ASSET_LOAD_H */
