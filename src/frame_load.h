/*
 * frame_load.{c,h} — the sprite-frame LOADERS (a slot's palette read, the frame load + cut into one zdd surface per
 * cell) and the BITMAP SESSION under them (the DIB in a LocalAlloc block: resource / file loads, the depth
 * converters, the BMP writer, the getters), the screenshot.  Retail layout; rhost-verified inside retail (REPLACE,
 * the render gate).  Names by VA.
 *
 *   FUN_004178e0  fl_4178e0   FUN_004184a0  fl_4184a0   FUN_004188b0  fl_4188b0   FUN_00499850  fl_499850
 *   FUN_005b6e40  bs_5b6e40   FUN_005b6e70  bs_5b6e70   FUN_005b6e80  bs_5b6e80   FUN_005b6e90  bs_5b6e90
 *   FUN_005b6eb0  bs_5b6eb0   FUN_005b6ec0  bs_5b6ec0   FUN_005b6ee0  bs_5b6ee0   FUN_005b6ef0  bs_5b6ef0
 *   FUN_005b6f00  bs_5b6f00   FUN_005b6f10  bs_5b6f10   FUN_005b6f30  bs_5b6f30   FUN_005b6f80  bs_5b6f80
 *   FUN_005b71f0  bs_5b71f0   FUN_005b7270  bs_5b7270   FUN_005b7310  bs_5b7310   FUN_005b74f0  bs_5b74f0
 *   FUN_005b7620  bs_5b7620   FUN_005b7800  bs_5b7800   FUN_005b7970  bs_5b7970   FUN_005b7b90  bs_5b7b90
 *   FUN_005b7bd0  bs_5b7bd0   FUN_005b7c10  bs_5b7c10   FUN_005b7cd0  bs_5b7cd0   FUN_005b9280  bs_5b9280
 *   FUN_005b9630  bs_5b9630   FUN_005b9910  bs_5b9910   FUN_005ba390  bs_5ba390
 */
#ifndef OSS_FRAME_LOAD_H
#define OSS_FRAME_LOAD_H

#include <stdint.h>

#include "bitmap_session.h"
#include "world_layout.h"

int32_t fl_4178e0(wl_char *self, void * out_palette);
void fl_4184a0(wl_char *self, int32_t a2);
int32_t fl_4188b0(wl_char *self, uint16_t a2, bitmap_session * a3, int32_t a4, int32_t a5, int32_t a6);
void fl_499850(int32_t a1);
int32_t bs_5b6e40(int32_t a1, uint32_t a2);
void bs_5b6e70(wl_char *self);
void bs_5b6e80(wl_char *self);
void bs_5b6e90(wl_char *self);
int32_t bs_5b6eb0(wl_char *self);
int32_t bs_5b6ec0(wl_char *self);
int32_t bs_5b6ee0(wl_char *self);
int32_t bs_5b6ef0(wl_char *self);
int32_t bs_5b6f00(wl_char *self);
void bs_5b6f10(wl_char *self, uint16_t bit_count);
int32_t bs_5b6f30(wl_char *self, int32_t hdc, int32_t xDest, int32_t yDest, int32_t DestWidth, int32_t DestHeight, int32_t xSrc, int32_t a8, int32_t SrcWidth, int32_t SrcHeight);
int32_t bs_5b6f80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t * a7);
int32_t bs_5b71f0(wl_char *self, int32_t width, int32_t height, uint16_t bit_count);
int32_t bs_5b7270(wl_char *self);
int32_t bs_5b7310(wl_char *self, int32_t a2, uint8_t * a3, int32_t a4, int32_t a5);
int32_t bs_5b74f0(wl_char *self, int32_t a2, uint32_t a3);
int32_t bs_5b7620(wl_char *self, char * lpFileName, int32_t a3, int32_t a4);
int32_t bs_5b7800(wl_char *self, void * hModule, uint32_t resource_id, void * resource_type, int32_t compressed_flag);
int32_t bs_5b7970(wl_char *self, char * lpFileName, int32_t a3, int32_t a4);
void bs_5b7b90(wl_char *self, void * dest_palette);
int32_t bs_5b7bd0(wl_char *self, int32_t a2);
int32_t bs_5b7c10(int32_t a1, int32_t a2, uint32_t * a3);
int32_t bs_5b7cd0(int32_t a1, int32_t * a2, int32_t * a3);
int32_t bs_5b9280(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
int32_t bs_5b9630(wl_char *self, bitmap_session * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t * a11);
int32_t bs_5b9910(wl_char *self, bitmap_session * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t bs_5ba390(wl_char *self);

#endif /* OSS_FRAME_LOAD_H */
