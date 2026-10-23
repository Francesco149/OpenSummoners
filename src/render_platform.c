/* render_platform.c — the frame RENDER: the draw layer above the platform blitters.  See render_platform.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (IDA's control flow and types, real class layouts, stack locals at
 * their ebp offsets, Win32 calls through retail's import slots, the knowledge pack's structs).  Retail's own
 * (RC_: PORT-DEBT(engine-call-slots)): the platform layer under it (the zdd surface / bitmap session / blitters
 * 0x5b6xxx..0x5bdxxx) and the sprite-frame LOADERS 0x4184a0 / 0x4188b0 / 0x4178e0 / 0x499850 (resource / file
 * reads, the error log). */
#include "render_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "zdd_dev.h"
#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
#define SYSB(o) F8(g_eng.sys, (o))   /* the system object's byte at +o (an lvalue: &SYSB(o) is its address) */
static inline int ofsub32_(int32_t a, int32_t b) { int32_t r; return __builtin_sub_overflow(a, b, &r); }
static inline int ofadd32_(int32_t a, int32_t b) { int32_t r; return __builtin_add_overflow(a, b, &r); }
#define __OFSUB__(a, b) ofsub32_((int32_t)(a), (int32_t)(b))
#define __OFADD__(a, b) ofadd32_((int32_t)(a), (int32_t)(b))
#define __CFADD__(a, b) ((uint32_t)(a) + (uint32_t)(b) < (uint32_t)(a))
#define __CFSUB__(a, b) ((uint32_t)(a) < (uint32_t)(b))
#define __SETS__(x) ((int32_t)(x) < 0)
#define LODWORD(x) ((uint32_t)(uint64_t)(x))
#define HIDWORD(x) ((uint32_t)((uint64_t)(x) >> 32))
#define WORD1(x) ((uint16_t)((uint64_t)(x) >> 16))
#define WORD2(x) ((uint16_t)((uint64_t)(x) >> 32))
#define BYTE1(x) ((uint8_t)((uint64_t)(x) >> 8))
#define BYTE2(x) ((uint8_t)((uint64_t)(x) >> 16))
#define HIBYTE(x) ((uint8_t)((uint32_t)(x) >> 24))
#define HIWORD(x) ((uint16_t)((uint32_t)(x) >> 16))
#define SHIDWORD(x) ((int32_t)((uint64_t)(x) >> 32))
#define SLODWORD(x) ((int32_t)(x))
#include <math.h>
/* 0x471080: rounded half away from zero (fcomp 0.0, then fsub / fadd 0.5 and __ftol's truncation) */
static inline int64_t ftol_round_(double a) { return a < 0.0 ? (int64_t)(a - 0.5) : (int64_t)(a + 0.5); }
static inline int32_t abs32(int32_t x) { return x < 0 ? (int32_t)(0u - (uint32_t)x) : x; }
#ifndef __cplusplus
#include <stdbool.h>
#endif
typedef int32_t _DWORD;
typedef uint8_t _UNKNOWN;   /* IDA's untyped byte (casts: `(_UNKNOWN *)p`) */
typedef uint8_t _BOOL1;
typedef uint16_t _BOOL2;
typedef int32_t _BOOL4;
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define qmemcpy memcpy   /* IDA's inline memcpy */
static inline void *memset32(void *d, uint32_t v, size_t n) { uint32_t *p = d; while (n--) *p++ = v; return d; }   /* IDA's rep stosd */
static inline void eh_vec_ctor(intptr_t p, uint32_t size, int32_t n, void *ctor, void *dtor)   /* MSVC's `eh vector */
{                                                                                             /* constructor iterator' */
    typedef void (__attribute__((thiscall)) *c_t)(void *);
    (void)dtor;
    for (int32_t i = 0; i < n; i++) ((c_t)ctor)((void *)(p + (intptr_t)size * i));
}
static inline void eh_vec_dtor(intptr_t p, uint32_t size, int32_t n, void *dtor)   /* MSVC's `eh vector destructor iterator': */
{                                                                                  /* the elements' dtors, last first */
    typedef void (__attribute__((thiscall)) *d_t)(void *);
    p += (intptr_t)size * n;
    while (--n >= 0) { p -= size; ((d_t)dtor)((void *)p); }
}
typedef char CHAR;
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETB1(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff00ffu) | ((uint32_t)(uint8_t)(v) << 8)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define FU32(p, o) (*(uint32_t *)((uint8_t *)(intptr_t)(p) + (o)))   /* an `unsigned int` load: >> / < / / stay unsigned */
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F64(p, o) (*(int64_t *)((uint8_t *)(intptr_t)(p) + (o)))

static inline intptr_t W_zd_5b80d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { zd_5b80d0((wl_char *)a0, (char *)a1, (char *)a2, (int32_t)a3); return 0; }

typedef intptr_t (__attribute__((stdcall)) *api_BitBlt_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_BitBlt ((api_BitBlt_t)g_eng.api("BitBlt"))
typedef intptr_t (__attribute__((stdcall)) *api_LocalFree_t)(intptr_t);
#define API_LocalFree ((api_LocalFree_t)g_eng.api("LocalFree"))
typedef intptr_t (__attribute__((stdcall)) *api_Polyline_t)(intptr_t, intptr_t, intptr_t);
#define API_Polyline ((api_Polyline_t)g_eng.api("Polyline"))
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

/* 0x5b5eb0: BOOL __stdcall sub_5B5EB0(HDC hdc, LONG a2, LONG a3, int a4, int a5) */
int32_t rp_5b5eb0(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{


    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    POINT *const apt__f = (POINT *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  (*apt__f).y = a3;
  (*v7__f) = a4;
  (*apt__f).x = a2;
  (*v8__f) = a5;
  return API_Polyline((intptr_t)(hdc), (intptr_t)(&(*apt__f)), (intptr_t)(2));
}

/* 0x5b8b40: int __thiscall sub_5B8B40(cls_5B7FE0 *this, _DWORD *a2, int a3, int a4, int a5, int a6) */
int32_t rp_5b8b40(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  v7 = (intptr_t)g_eng.op_new(0xD8u);
  if ( v7 != 0 )
    v8 = rp_5b9350(v7, (int)c);
  else
    v8 = 0;
  if ( rp_5b95c0(v8, a3, a4, 0, a5, a6, 0, 0, a3, a4) != 0 )
  {
    *a2 = v8;
    return 1;
  }
  else
  {
    if ( v8 != 0 )
    {
      rp_5b9390(v8);
      g_eng.op_delete((void *)v8);
    }
    return 0;
  }
}

/* 0x5b9350: _DWORD *__thiscall sub_5B9350(cls_5B7FE0 *this, int a2) */
int32_t rp_5b9350(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x2c) = 0;
  F32(c, 0xac) = 0;
  F32(c, 0xc0) = a2;
  F32(c, 0xd4) = 0;
  F32(c, 0xc4) = 0;
  F32(c, 0xc8) = 0;
  ++F32(a2, 288);
  return ((uint8_t *)(c) + 0x0);
}

/* 0x5b9390: int __thiscall title_frame_free_5B9390(cls_5B9390 *this) */
int32_t rp_5b9390(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t result = 0;

  rp_5b93e0(c);
  v2 = F32(c, 0xac);
  if ( v2 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v2, 0) + 8))(F32(c, 0xac));
    F32(c, 0xac) = 0;
  }
  v3 = F32(c, 0x2c);
  if ( v3 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v3, 0) + 8))(F32(c, 0x2c));
    F32(c, 0x2c) = 0;
  }
  v4 = F32(c, 0xc0);
  result = F32(v4, 288) - 1;
  F32(v4, 288) = result;
  return result;
}

/* 0x5b93e0: HLOCAL __thiscall zdd_object_local_free_5B93E0(cls_5B93E0 *this) */
int32_t rp_5b93e0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = (intptr_t)F32(c, 0xc4);
  if ( result != 0 )
  {
    result = API_LocalFree((intptr_t)((intptr_t)F32(c, 0xc4)));
    F32(c, 0xc4) = 0;
    F32(c, 0xc8) = 0;
  }
  return result;
}

/* 0x5b95c0: int __thiscall sub_5B95C0(cls_5B95C0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) */
int32_t rp_5b95c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  rp_5b97e0(c, a4, a6);
  result = rp_5b8c00(F32(c, 0xc0), &GAP(c, 0x0, 44), a9, a10, F32(c, 0xcc), a6);
  if ( result != 0 )
  {
    rp_5b98c0(c, a2, a3, a7, a8, a9, a10);
    return rp_5b9830(c, a5);
  }
  return result;
}

/* 0x5b8c00: int __thiscall zdd_create_surface_5B8C00(cls_5B7FE0 *this, _DWORD *a2, int a3, int a4, int a5, int a6) */
int32_t rp_5b8c00(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x7c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v11 = (intptr_t *)(frame_ + 0x0);   /* [31] at ebp-0x7c */

  memset(v11, 0, (31 * sizeof(intptr_t)));
  v7 = a5;
  v11[3] = a3;
  SETLB(v7, a5 | 0x40);
  v11[0] = 124;
  v11[1] = 7;
  v11[26] = v7;
  v11[2] = a4;
  if ( a6 != 0 || F32(c, 0x134) != 0 )
    v11[26] = v7 + 2048;
  if ( F32(c, 0x164) != 2 )
  {
    v8 = (*(int (__stdcall **)(int, _DWORD *, _DWORD *, _DWORD))(*(_DWORD *)F32(c, 0x124) + 24))(
           F32(c, 0x124),
           v11,
           a2,
           0);
    goto LABEL_6;
  }
  v10 = F32(c, 0x168);
  v11[1] = 4103;
  v11[21] = v10;
  v11[18] = 32;
  v11[19] = 64;
  switch ( v10 )
  {
    case 8:
      v11[19] = 96;
      break;
    case 16:
      v11[22] = 63488;
      v11[23] = 2016;
      v11[24] = 31;
      break;
    case 24:
    case 32:
      v11[22] = 16711680;
      v11[23] = 65280;
      v11[24] = 255;
      break;
    default:
      break;
  }
  v8 = (*(int (__stdcall **)(int, _DWORD *, _DWORD *, _DWORD))(*(_DWORD *)F32(c, 0x124) + 24))(
         F32(c, 0x124),
         v11,
         a2,
         0);
  if ( F32(c, 0x12c) == 0 )
  {
LABEL_6:
    if ( v8 == 0 )
      return 1;
    goto LABEL_15;
  }
  if ( v8 == 0 )
  {
    (*(void (__stdcall **)(_DWORD, int))(*(_DWORD *)*a2 + 124))(*a2, F32(c, 0x12c));
    return 1;
  }
LABEL_15:
  W_zd_5b80d0((intptr_t)(c), (intptr_t)(((char *)D(0x8a4b00))), (intptr_t)(((char *)D(0x8a4b0c))), (intptr_t)(v8));
  return 0;
}

/* 0x5b97e0: _DWORD *__thiscall sub_5B97E0(cls_5B97E0 *this, int a2, int a3) */
int32_t rp_5b97e0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0xcc) = a2;
  F32(c, 0xd0) = a3;
  memset(&F32(c, 0x30), 0, 0x7Cu);
  F32(c, 0x0) = (int)&GAP(c, 0x34, 32);
  F32(c, 0x30) = 124;
  F32(c, 0x4) = (int)&GAP(c, 0x34, 12);
  F32(c, 0x8) = (int)&GAP(c, 0x34, 4);
  return &GAP(c, 0x34, 4);
}

/* 0x5b9830: int __thiscall zdd_surface_set_color_key_5B9830(cls_5B9830 *this, int a2) */
int32_t rp_5b9830(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v7 = (intptr_t *)(frame_ + 0x0);   /* [2] at ebp-0x8 */

  v2 = a2;
  F32(c, 0x24) = a2;
  if ( a2 == 0x1FFFFFF )
  {
    F32(c, 0x28) = 0x1FFFFFF;
    F32(c, 0xd4) = 0;
    return 1;
  }
  v5 = F32(c, 0xc0);
  F32(c, 0xd4) = 0x8000;
  if ( F32(v5, 360) == 16 )
    v2 = rp_5b8b00(v5, a2);
  v7[1] = v2;
  F32(c, 0x28) = v2;
  v7[0] = v2;
  v6 = (*(int (__stdcall **)(int, int, _DWORD *))(*(_DWORD *)F32(c, 0x2c) + 116))(F32(c, 0x2c), 8, v7);
  if ( v6 == 0 )
    return 1;
  W_zd_5b80d0((intptr_t)(F32(c, 0xc0)), (intptr_t)(((char *)D(0x8a4b54))), (intptr_t)(((char *)D(0x8a4ba0))), (intptr_t)(v6));
  return 0;
}

/* 0x5b8b00: int __thiscall zdd_color_convert_5B8B00(cls_5B8B00 *this, int a2) */
int32_t rp_5b8b00(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return ((int)(uint8_t)a2 >> F8(c, 0x12) << F8(c, 0x2))
       | (BYTE1(a2) >> F8(c, 0x11) << F8(c, 0x1))
       | (BYTE2(a2) >> F8(c, 0x10) << F8(c, 0x0));
}

/* 0x5b98c0: int __thiscall sub_5B98C0(cls_5B98C0 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t rp_5b98c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0xb0) = 0;
  F32(c, 0xb4) = 0;
  F32(c, 0xc) = a4;
  F32(c, 0xb8) = a6;
  F32(c, 0xbc) = a7;
  F32(c, 0x14) = a6;
  F32(c, 0x18) = a7;
  F32(c, 0x10) = a5;
  F32(c, 0x1c) = a2;
  F32(c, 0x20) = a3;
  return a2;
}

/* 0x5b9450: int __thiscall sub_5B9450(cls_5B9410 *this, int a2) */
int32_t rp_5b9450(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = rp_5b9490(c);
  if ( result != 0 )
  {
    memset((void *)F32(c, 0x54), 0, a2 * F32(c, 0x40));
    return rp_5b94d0(c);
  }
  return result;
}

/* 0x5b9490: int __thiscall sub_5B9490(cls_5B9410 *this) */
int32_t rp_5b9490(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;

  v2 = F32(c, 0x2c);
  if ( v2 != 0 )
  {
    v3 = (*(int (__stdcall **)(int, _DWORD, char *, int, _DWORD))(F32(v2, 0) + 100))(v2, 0, ((uint8_t *)(c) + 0x30), 1, 0);
    if ( v3 == 0 )
      return 1;
    W_zd_5b80d0((intptr_t)(F32(c, 0xc0)), (intptr_t)(((char *)D(0x8a4b54))), (intptr_t)(((char *)D(0x8a4b84))), (intptr_t)(v3));
  }
  return 0;
}

/* 0x5b94d0: int __thiscall sub_5B94D0(cls_5B9410 *this) */
int32_t rp_5b94d0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return (*(int (__stdcall **)(int, _DWORD))(*(_DWORD *)F32(c, 0x2c) + 128))(F32(c, 0x2c), 0);
}

/* 0x5b94e0: int __thiscall sub_5B94E0(paint_ctx *this, HWND *out_src_hdc) */
int32_t rp_5b94e0(wl_char *self, intptr_t * out_src_hdc)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = (int)F32(c, 0x2c);
  if ( result != 0 )
  {
    (*(void (__stdcall **)(int, HWND *))(F32(result, 0) + 68))(result, out_src_hdc);
    return 1;
  }
  return result;
}

/* 0x5b9500: void __thiscall sub_5B9500(paint_ctx *this, HWND src_hdc) */
void rp_5b9500(wl_char *self, int32_t src_hdc)
{
    intptr_t c = (intptr_t)self;

  (*(void (__stdcall **)(void *, HWND))(*(_DWORD *)F32(c, 0x2c) + 104))(F32(c, 0x2c), src_hdc);
}

/* 0x5b9520: int __thiscall sub_5B9520(cls_5B9520 *this) */
int32_t rp_5b9520(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    int32_t * v3 = 0;
    intptr_t v4 = 0;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v6 = (intptr_t *)(frame_ + 0x0);   /* [6] at ebp-0x30 */
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x18 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x14 */
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x10 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0xc */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x4 */

  v6[1] = 1;
  v2 = F32(c, 0xbc);
  v6[2] = 1;
  v3 = &F32(c, 0xac);
  (*v7__f) = F32(c, 0xb8);
  (*v11__f) = (*v7__f);
  v4 = F32(c, 0xac);
  v6[0] = 32;
  v6[3] = 16;
  v6[4] = 0;
  v6[5] = 0;
  (*v8__f) = v2;
  (*v9__f) = 0;
  (*v10__f) = 0;
  (*v12__f) = v2;
  if ( v4 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v4, 0) + 8))(v4);
    *v3 = 0;
  }
  (*(void (__stdcall **)(_DWORD, _DWORD, int *, _DWORD))(F32(F32(F32(c, 0xc0), 292), 0) + 16))(
    F32(F32(c, 0xc0), 292),
    0,
    &F32(c, 0xac),
    0);
  (*(void (__stdcall **)(int, _DWORD *, _DWORD))(*(_DWORD *)*v3 + 28))(*v3, v6, 0);
  return (*(int (__stdcall **)(int, int))(*(_DWORD *)F32(c, 0x2c) + 112))(F32(c, 0x2c), *v3);
}

/* 0x5b9a40: int __thiscall blt_onto_5B9A40(cls_5B9A40 *this, int a2, int a3, int a4) */
int32_t rp_5b9a40(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v10 = (intptr_t *)(frame_ + 0x0);   /* [4] at ebp-0x10 */

  v4 = F32(c, 0x2c);
  if ( v4 == 0 )
    return 1;
  v6 = F32(c, 0xb8);
  v10[0] = a3;
  v10[2] = a3 + v6;
  v7 = F32(c, 0xbc);
  v8 = F32(c, 0xd4);
  v10[1] = a4;
  v9 = F32(a2, 44);
  v10[3] = a4 + v7;
  return (*(int (__stdcall **)(int, _DWORD *, int, char *, int, _DWORD))(F32(v9, 0) + 20))(
           v9,
           v10,
           v4,
           &GAP(c, 0x30, 128),
           v8 | 0x1000000,
           0);
}

/* 0x5b9ae0: int __thiscall blt_rects_5B9AE0( cls_5B9AE0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) */
int32_t rp_5b9ae0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v13 = (intptr_t *)(frame_ + 0x0);   /* [4] at ebp-0x20 */
    intptr_t *v14 = (intptr_t *)(frame_ + 0x10);   /* [4] at ebp-0x10 */

  v10 = F32(c, 0x2c);
  if ( v10 == 0 )
    return 1;
  v13[0] = a7;
  v13[2] = a9 + a7;
  v13[1] = a8;
  v13[3] = a10 + a8;
  v12 = F32(c, 0xd4);
  v14[0] = a3;
  v14[2] = a5 + a3;
  v14[1] = a4;
  v14[3] = a6 + a4;
  return (*(int (__stdcall **)(_DWORD, _DWORD *, int, _DWORD *, int, _DWORD))(F32(F32(a2, 44), 0) + 20))(
           F32(a2, 44),
           v14,
           v10,
           v13,
           v12 | 0x1000000,
           0);
}

/* 0x5b9b70: int __thiscall blt_keyed_5B9B70(cls_5B9B70 *this, int a2, int a3, int a4) */
int32_t rp_5b9b70(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v12 = (intptr_t *)(frame_ + 0x0);   /* [4] at ebp-0x10 */

  v4 = F32(c, 0x2c);
  if ( v4 == 0 )
    return 1;
  v6 = F32(c, 0xd4);
  v7 = a3 + F32(c, 0xc) + F32(c, 0xb8);
  v12[0] = a3 + F32(c, 0xc);
  v8 = F32(c, 0x10);
  v12[2] = v7;
  v9 = F32(c, 0xbc);
  v12[1] = a4 + v8;
  v10 = a4 + v8 + v9;
  v11 = F32(a2, 44);
  v12[3] = v10;
  return (*(int (__stdcall **)(int, _DWORD *, int, char *, int, _DWORD))(F32(v11, 0) + 20))(
           v11,
           v12,
           v4,
           &GAP(c, 0x30, 128),
           v6 | 0x1000000,
           0);
}

/* 0x5b9bf0: int __thiscall blt_clipped_5B9BF0(cls_5B9BF0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t rp_5b9bf0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v19 = (intptr_t *)(frame_ + 0x4);   /* [4] at ebp-0x20 */
    intptr_t *v20 = (intptr_t *)(frame_ + 0x14);   /* [4] at ebp-0x10 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */

  (*v18__f) = F32(c, 0x2c);
  if ( (*v18__f) == 0 )
    return 1;
  v9 = F32(c, 0xc);
  if ( a7 >= v9 )
  {
    v10 = a5;
    v11 = a7 - v9;
  }
  else
  {
    v10 = a7 - v9 + a5;
    a3 += v9 - a7;
    v11 = 0;
  }
  if ( F32(c, 0x14) - v11 < v10 )
    v10 = F32(c, 0x14) - v11;
  v12 = F32(c, 0x10);
  if ( a8 >= v12 )
  {
    v13 = a6;
    v14 = a4;
    v15 = a8 - v12;
  }
  else
  {
    v13 = a8 - v12 + a6;
    v14 = v12 - a8 + a4;
    v15 = 0;
  }
  if ( F32(c, 0x18) - v15 < v13 )
    v13 = F32(c, 0x18) - v15;
  if ( v10 <= 0 || v13 <= 0 )
    return 0;
  v16 = F32(c, 0xd4);
  v19[1] = v15;
  v19[0] = v11;
  v19[3] = v13 + v15;
  v19[2] = v10 + v11;
  v20[0] = a3;
  v20[2] = v10 + a3;
  v17 = F32(a2, 44);
  v20[1] = v14;
  v20[3] = v13 + v14;
  return (*(int (__stdcall **)(int, _DWORD *, int, _DWORD *, int, _DWORD))(F32(v17, 0) + 20))(
           v17,
           v20,
           (*v18__f),
           v19,
           v16 | 0x1000000,
           0);
}

/* 0x5bd550: int __thiscall blt_alpha_5BD550( cls_5BD550 *this, paint_ctx *a2, int a3, int x1, int y1, int a6, int a7, int a8, int a9, int a10, HDC hdcSrc) */
int32_t rp_5bd550(wl_char *self, int32_t a2, int32_t a3, int32_t x1, int32_t y1, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    intptr_t v11 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  v11 = (intptr_t)hdcSrc;
  (*v17__f) = c;
  if ( hdcSrc != 0 )
  {
    rp_5b94e0(a2, (intptr_t *)&hdcSrc);
    rp_5b94e0(v11, (intptr_t *)&(*hdc__f));
    v14 = 16;
    if ( a7 >= 16 )
      v14 = a7;
    v15 = 16;
    if ( a6 >= 16 )
      v15 = a6;
    API_BitBlt((intptr_t)((*hdc__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v15), (intptr_t)(v14), (intptr_t)(hdcSrc), (intptr_t)(x1), (intptr_t)(y1), (intptr_t)(0xCC0020u));
    rp_5b9500(a2, (intptr_t)hdcSrc);
    rp_5b9500(v11, (intptr_t)(*hdc__f));
    rp_5b9490(v11);
    rp_5bd680((*v17__f), (int **)v11, (_DWORD **)a3, 0, 0, a6, a7, a8, a9, a10);
    rp_5b94d0(v11);
    return rp_5b9ae0(v11, (int)a2, x1, y1, a6, a7, 0, 0, a6, a7);
  }
  else
  {
    rp_5b9490(a2);
    rp_5bd680(c, (int **)a2, (_DWORD **)a3, x1, y1, a6, a7, a8, a9, a10);
    return rp_5b94d0(a2);
  }
}

/* 0x5bd680: int __thiscall sub_5BD680( cls_5BD680 *this, int **a2, _DWORD **a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) */
int32_t rp_5bd680(wl_char *self, int32_t ** a2, int32_t ** a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t result = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint16_t * v22 = 0;
    uint32_t v23 = 0;
    uint32_t v24 = 0;
    int v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    uint16_t * v28 = 0;
    intptr_t v29 = 0;
    uint32_t v30 = 0;
    uint16_t v31 = 0;
    intptr_t v32 = 0;
    uint32_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    uint16_t * v42 = 0;
    uint32_t v43 = 0;
    intptr_t v44 = 0;
    uint32_t v45 = 0;
    uint32_t v46 = 0;
    uint32_t v47 = 0;
    uint32_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    int32_t ** v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    intptr_t v73 = 0;
    intptr_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    intptr_t v78 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v55__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */
    intptr_t *const v57__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  v11 = a7;
  v12 = *a2[1];
  v13 = *a2[2];
  (*v57__f) = **a2;
  v60 = (int **)a6;
  v14 = v12 / 2;
  (*v53__f) = a8;
  (*v51__f) = a9;
  result = rp_5b9490(a3);
  if ( result != 0 )
  {
    v16 = *a3[1] / 2;
    if ( a4 + a6 > v14 )
      v60 = (int **)(v14 - a4);
    v17 = a5;
    if ( a5 + a7 > v13 )
      v11 = v13 - a5;
    v18 = a4;
    if ( a4 < 0 )
    {
      v60 = (int **)((uint8_t *)v60 + a4);
      v18 = 0;
      (*v53__f) = a8 - a4;
    }
    if ( a5 < 0 )
    {
      v11 += a5;
      v17 = 0;
      (*v51__f) = a9 - a5;
    }
    if ( (int)v60 > 0 && v11 > 0 )
    {
      v19 = (*v57__f) + 2 * (v18 + v17 * v14);
      v20 = **a3 + 2 * ((*v53__f) + (*v51__f) * v16);
      if ( F32(c, 0x0) != 0 )
      {
        if ( F32(c, 0x0) == 1 )
        {
          v77 = **a3 + 2 * ((*v53__f) + (*v51__f) * v16);
          (*v52__f) = 2 * v16;
          v26 = 2 * v14;
          v27 = v19;
          (*v55__f) = v26;
          v74 = v19;
          v65 = v11;
          do
          {
            v28 = (uint16_t *)v20;
            v68 = v20;
            v29 = v27 - v20;
            v62 = (int)v60;
            v71 = v29;
            do
            {
              v30 = *v28;
              if ( a10 != v30 )
              {
                v31 = F16(((uint8_t *)v28 + v29), 0);
                v32 = F32(c, 0x4);
                v33 = (F32(c, 0x8) & (unsigned int)v31) >> v32;
                SETLO(v33, *(uint8_t *)(v33 + 32 * ((v30 & F32(c, 0x8)) >> v32) + F32(c, 0xc)));
                v34 = F32(c, 0x1c);
                SETLB(v32, F32(c, 0x18));
                v35 = (uint16_t)(v34 & v31) >> v32;
                SETLO(v35, F8((F32(c, 0x20) + v35 + 32 * ((v30 & v34) >> v32)), 0));
                v36 = F32(c, 0x2c);
                v37 = (v35 << F32(c, 0x18)) | (v33 << F32(c, 0x4));
                v38 = F32(c, 0x30);
                v39 = (uint16_t)(v38 & v31) >> v36;
                v29 = v71;
                SETLO(v39, F8((F32(c, 0x34) + v39 + 32 * ((v30 & v38) >> v36)), 0));
                v40 = v39 << v36;
                v28 = (uint16_t *)v68;
                F16((v71 + v68), 0) = v40 | v37;
              }
              ++v28;
              v25 = v62 == 1;
              v68 = (int)v28;
              --v62;
            }
            while ( !v25 );
            v27 = (*v55__f) + v74;
            v20 = (*v52__f) + v77;
            v25 = v65 == 1;
            v74 += (*v55__f);
            v77 += (*v52__f);
            --v65;
          }
          while ( !v25 );
        }
        else if ( F32(c, 0x0) == 2 )
        {
          v76 = **a3 + 2 * ((*v53__f) + (*v51__f) * v16);
          (*v54__f) = 2 * v16;
          (*v58__f) = 2 * v14;
          v21 = v19;
          v73 = v19;
          v64 = v11;
          do
          {
            v22 = (_WORD *)v20;
            v67 = v20;
            v61 = (int)v60;
            v70 = v21 - v20;
            do
            {
              SETHI(v23, 0);
              if ( a10 != (uint16_t)*v22 )
              {
                SETLO(v23, *v22);
                v24 = (((v23 & F32(c, 0x30)) >> F32(c, 0x2c))
                     + ((v23 & F32(c, 0x1c)) >> F32(c, 0x18))
                     + ((v23 & F32(c, 0x8)) >> F32(c, 0x4)))
                    / 3;
                v22 = (_WORD *)v67;
                F16((v70 + v67), 0) = (F8((F32(c, 0x34) + v24), 0) << F32(c, 0x2c))
                                      | (F8((F32(c, 0x20) + v24), 0) << F32(c, 0x18))
                                      | (F8((F32(c, 0xc) + v24), 0) << F32(c, 0x4));
              }
              ++v22;
              v25 = v61 == 1;
              v67 = (int)v22;
              --v61;
            }
            while ( !v25 );
            v21 = (*v58__f) + v73;
            v20 = (*v54__f) + v76;
            v25 = v64 == 1;
            v73 += (*v58__f);
            v76 += (*v54__f);
            --v64;
          }
          while ( !v25 );
        }
      }
      else
      {
        v78 = **a3 + 2 * ((*v53__f) + (*v51__f) * v16);
        (*v56__f) = 2 * v16;
        (*v59__f) = 2 * v14;
        v41 = v19;
        v75 = v19;
        v66 = v11;
        do
        {
          v42 = (uint16_t *)v20;
          v69 = v20;
          v63 = (int)v60;
          v72 = v41 - v20;
          do
          {
            v43 = *v42;
            if ( a10 != v43 )
            {
              v44 = F32(c, 0x4);
              v45 = (v43 & F32(c, 0x8)) >> v44;
              SETLO(v45, F8((v45 + F32(c, 0xc)), 0));
              v46 = v45 << v44;
              v47 = (v43 & F32(c, 0x1c)) >> F32(c, 0x18);
              v48 = v43 & F32(c, 0x30);
              v49 = F32(c, 0x34);
              SETLB(v44, F32(c, 0x2c));
              SETLO(v49, F8(((v48 >> v44) + v49), 0));
              v50 = v49 << v44;
              v42 = (uint16_t *)v69;
              F16((v72 + v69), 0) = v50 | (F8((F32(c, 0x20) + v47), 0) << F32(c, 0x18)) | v46;
            }
            ++v42;
            v25 = v63 == 1;
            v69 = (int)v42;
            --v63;
          }
          while ( !v25 );
          v41 = (*v59__f) + v75;
          v20 = (*v56__f) + v78;
          v25 = v66 == 1;
          v75 += (*v59__f);
          v78 += (*v56__f);
          --v66;
        }
        while ( !v25 );
      }
    }
    return rp_5b94d0(a3);
  }
  return result;
}
/* unported callees: none */
