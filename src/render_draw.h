/*
 * render_draw.{c,h} — the frame RENDER on the retail layout (rhost-verified inside retail by the render gate:
 * every presented frame's pixels, pure vs hybrid): the frame render 0x48c150 (room / scene / menu), its bands
 * (characters, actors, effects, glyphs), the message box, the HUD, the sprite-frame resolver, the palette grade.
 * Names by VA.  Retail's callers reach 11 of them (hooked, REPLACE only); the rest are called from these.
 *
 *   FUN_0048c150  rd_48c150   FUN_00418470  rd_418470   FUN_0048c6b0  rd_48c6b0   FUN_0048c820  rd_48c820
 *   FUN_0048cb90  rd_48cb90   FUN_0048d3d0  rd_48d3d0   FUN_0048cf80  rd_48cf80   FUN_0048d670  rd_48d670
 *   FUN_0048d710  rd_48d710   FUN_0048d940  rd_48d940   FUN_0048da70  rd_48da70   FUN_0048e100  rd_48e100
 *   FUN_0048e200  rd_48e200   FUN_0048e6d0  rd_48e6d0   FUN_0048e860  rd_48e860   FUN_0048e920  rd_48e920
 *   FUN_0048eac0  rd_48eac0   FUN_0048ef40  rd_48ef40   FUN_00490090  rd_490090   FUN_004900d0  rd_4900d0
 *   FUN_00490160  rd_490160   FUN_004903f0  rd_4903f0   FUN_00490860  rd_490860   FUN_00490cd0  rd_490cd0
 *   FUN_00417c40  rd_417c40   FUN_00490f30  rd_490f30   FUN_004917b0  rd_4917b0   FUN_00491820  rd_491820
 *   FUN_00491ae0  rd_491ae0   FUN_004925e0  rd_4925e0   FUN_00492670  rd_492670   FUN_004926f0  rd_4926f0
 *   FUN_004927c0  rd_4927c0   FUN_004928f0  rd_4928f0   FUN_00492a90  rd_492a90   FUN_00492c40  rd_492c40
 *   FUN_00492f30  rd_492f30   FUN_00492fc0  rd_492fc0   FUN_00493230  rd_493230   FUN_00493480  rd_493480
 *   FUN_004937c0  rd_4937c0   FUN_00493a50  rd_493a50   FUN_00493ba0  rd_493ba0   FUN_00494790  rd_494790
 *   FUN_00494a60  rd_494a60   FUN_00494e10  rd_494e10   FUN_00494e60  rd_494e60   FUN_0043e250  rd_43e250
 *   FUN_00495dc0  rd_495dc0   FUN_00495e40  rd_495e40   FUN_00495fe0  rd_495fe0   FUN_00496170  rd_496170
 *   FUN_004961a0  rd_4961a0   FUN_004961e0  rd_4961e0   FUN_00496240  rd_496240   FUN_004962a0  rd_4962a0
 *   FUN_004963a0  rd_4963a0   FUN_00496560  rd_496560   FUN_00498f10  rd_498f10   FUN_004969b0  rd_4969b0
 *   FUN_00496ec0  rd_496ec0   FUN_004975a0  rd_4975a0   FUN_004975e0  rd_4975e0   FUN_00497a00  rd_497a00
 *   FUN_00497b40  rd_497b40   FUN_00497bb0  rd_497bb0   FUN_00497c20  rd_497c20   FUN_00497f40  rd_497f40
 *   FUN_00498620  rd_498620   FUN_00498680  rd_498680   FUN_00498820  rd_498820   FUN_00498960  rd_498960
 *   FUN_00499070  rd_499070   FUN_00499100  rd_499100   FUN_00499560  rd_499560   FUN_00499670  rd_499670
 *   FUN_004997b0  rd_4997b0   FUN_0049c170  rd_49c170
 */
#ifndef OSS_RENDER_DRAW_H
#define OSS_RENDER_DRAW_H

#include <stdint.h>

#include "world_layout.h"

int32_t rd_48c150(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t rd_418470(wl_char *self, uint16_t a2);
int32_t rd_48c6b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t * a13);
int32_t rd_48c820(wl_char *self, int32_t a2, int32_t hdcSrc);
int32_t rd_48cb90(wl_char *self, int32_t a2, int32_t hdcSrc);
int32_t rd_48d3d0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int16_t a8, int16_t a9, int16_t a10, int32_t a11, int32_t hdcSrc);
int32_t rd_48cf80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t hdcSrc);
int32_t rd_48d670(int32_t a1, int32_t a2, int16_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t hdcSrc);
int32_t rd_48d710(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t hdcSrc);
int32_t rd_48d940(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t hdcSrc);
int32_t rd_48da70(wl_char *self, int32_t a2, int32_t hdc, int32_t a4, int32_t a5, int32_t h, int32_t a7, int32_t a8);
int32_t rd_48e100(wl_char *self, int32_t hdc, int32_t a3, int32_t a4, int32_t h);
int32_t rd_48e200(wl_char *self, int32_t a2, int32_t a3, int32_t hdc, int32_t a5, int32_t a6, int32_t h, int32_t a8, int32_t hdcSrc);
int32_t rd_48e6d0(wl_char *self, int32_t hdc, int32_t a3, int32_t y, int32_t a5, uint16_t a6, uint16_t a7);
int32_t rd_48e860(wl_char *self, int32_t hdc, int32_t a3, int32_t y, int16_t a5, uint16_t a6, uint16_t a7);
int32_t rd_48e920(wl_char *self, int32_t a2);
void rd_48eac0(wl_char *self, int32_t a2);
int32_t rd_48ef40(wl_char *self, int32_t hdc, int32_t a3, int32_t a4);
double rd_490090(double a1, double a2);
int32_t rd_4900d0(int32_t a1, double a2, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6);
int32_t rd_490160(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_4903f0(int32_t a1, int32_t hdc, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
void rd_490860(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint8_t * a8);
int32_t rd_490cd0(wl_char *self, int32_t a2, int32_t * a3);
int32_t rd_417c40(wl_char *self, uint16_t a2, uint16_t a3);
int32_t rd_490f30(wl_char *self, int32_t * a2, int32_t a3);
int32_t rd_4917b0(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
int32_t rd_491820(wl_char *self, int32_t a2, char * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
int32_t rd_491ae0(wl_char *self, uint16_t a2, int32_t a3);
int32_t rd_4925e0(wl_char *self, uint16_t a2, int32_t * a3, int32_t * a4, int32_t * a5, int32_t * a6);
int32_t rd_492670(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_4926f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_4927c0(uint16_t a1, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t rd_4928f0(int32_t a1, uint16_t a2, uint16_t a3, int32_t a4, int32_t a5, uint16_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14);
int32_t rd_492a90(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5);
void rd_492c40(wl_char *self, int32_t * a2, int32_t ** a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14);
void rd_492f30(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12);
int32_t rd_492fc0(int32_t a1, int32_t * a2, int32_t a3, int32_t a4);
int32_t rd_493230(int32_t a1, int32_t a2, int32_t a3);
int32_t rd_493480(wl_char *self, int32_t a2, int32_t * a3);
int32_t rd_4937c0(wl_char *self, int32_t a2, int32_t a3);
int32_t rd_493a50(int32_t * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_493ba0(wl_char *self, uint16_t a2, int32_t * a3, int32_t a4);
int32_t rd_494790(wl_char *self, int32_t a2, int32_t * a3);
int32_t rd_494a60(wl_char *self, int32_t a2);
int32_t rd_494e10(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_494e60(wl_char *self, int32_t a2);
void rd_43e250(uint16_t a1, int32_t a2, int32_t hdc, int32_t y, int32_t a5, int32_t a6);
int32_t rd_495dc0(int32_t a1, uint16_t a2, char * Buffer);
int32_t rd_495e40(wl_char *self, int32_t a2, char * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_495fe0(wl_char *self, int32_t a2, uint16_t a3, int32_t * a4, int32_t a5, uint16_t a6);
int32_t rd_496170(wl_char *self);
int32_t rd_4961a0(wl_char *self);
int32_t rd_4961e0(wl_char *self, int32_t a2, int32_t a3);
int32_t rd_496240(wl_char *self, int32_t a2, int32_t a3);
int32_t rd_4962a0(int32_t a1, uint16_t a2, uint16_t a3, int32_t a4, int32_t a5);
void rd_4963a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t rd_496560(wl_char *self, int32_t a2, int32_t * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int16_t a12, int32_t * a13);
int32_t rd_498f10(int32_t a1, int32_t a2, int32_t y1, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, uint16_t a10, uint16_t a11, int32_t a12);
int32_t rd_4969b0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t rd_496ec0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rd_4975a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t x1, int32_t y1, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
void rd_4975e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rd_497a00(int32_t a1, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_497b40(int32_t * a1, int32_t a2);
int32_t rd_497bb0(int32_t * a1, int32_t a2);
void rd_497c20(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t rd_497f40(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5);
int32_t rd_498620(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rd_498680(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t y1, int32_t a7, int32_t a8, int32_t a9, int32_t a10, uint16_t a11, int32_t a12);
int32_t rd_498820(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t y1, int32_t a6, int32_t a7, int32_t a8, uint16_t a9);
int32_t rd_498960(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_499070(uint16_t a1);
void rd_499100(wl_char *self, int32_t a2);
void rd_499560(int32_t a1, int32_t * a2, int32_t a3, int32_t a4);
int32_t rd_499670(wl_char *self, int32_t a2, uint8_t * a3, int32_t a4, uint32_t a5);
int32_t rd_4997b0(int32_t a1, int32_t a2);
int32_t rd_49c170(int32_t a1);

#endif /* OSS_RENDER_DRAW_H */
