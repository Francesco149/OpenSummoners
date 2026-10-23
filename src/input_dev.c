/* input_dev.c — the DirectInput DEVICE layer: the DirectInput7 object 0x5b9cf0 / 0x5b9d90 / 0x5b9f80 (+ its error
 * log 0x5b9e30 → OutputDebugString), the keyboard 0x5b9fc0 / 0x5ba410 (CreateDeviceEx, c_dfDIKeyboard, cooperative
 * level, the 32-event buffer 0x5ba330) and the joysticks 0x5ba120 / 0x5ba070 / 0x5ba540 (EnumDevices + 0x5ba200, the
 * ±1000 axis ranges 0x5ba6a0), acquire / unacquire 0x5ba290 / 0x5ba2b0, the polls (buffered events 0x5ba2d0, the
 * key state 0x5ba4c0, the joystick state 0x5ba650) and the getters (event id / data / key down 0x5ba3a0..0x5ba520).
 * See input_dev.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish. */
#include "input_dev.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))


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
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define qmemcpy memcpy   /* IDA's inline memcpy */
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


typedef intptr_t (*rt_5c0907)(intptr_t, intptr_t, intptr_t);   /* 0x5c0907 cdecl: unported, retail's */
#define RC_5c0907 ((rt_5c0907)g_eng.retail(0x5c0907u))
typedef intptr_t (__attribute__((stdcall)) *api_DirectInputCreateEx_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_DirectInputCreateEx ((api_DirectInputCreateEx_t)g_eng.api("DirectInputCreateEx"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
int32_t id_5b9cf0(intptr_t * a1, int32_t a2);
int32_t id_5b9d90(wl_char *self);
void id_5b9de0(wl_char *self);
void id_5b9e30(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t id_5b9f80(wl_char *self, int32_t a2);
int32_t id_5b9fc0(wl_char *self, int32_t * a2, int32_t a3);
int32_t id_5ba070(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t id_5ba120(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t id_5ba1c0(wl_char *self, void * a2);
int32_t id_5ba200(void * a1, int32_t a2);
int32_t id_5ba220(wl_char *self, int32_t a2);
int32_t id_5ba240(wl_char *self);
int32_t id_5ba290(wl_char *self);
int32_t id_5ba2b0(wl_char *self);
int32_t id_5ba2d0(wl_char *self);
int32_t id_5ba330(wl_char *self, int32_t a2);
int32_t id_5ba3a0(wl_char *self, int32_t a2);
int32_t id_5ba3b0(wl_char *self, int32_t a2);
int32_t id_5ba3d0(wl_char *self, int32_t a2);
int32_t id_5ba3f0(wl_char *self, int32_t a2);
int32_t id_5ba410(wl_char *self, int32_t a2);
int32_t id_5ba4c0(wl_char *self);
int32_t id_5ba520(wl_char *self, uint8_t a2);
int32_t id_5ba540(wl_char *self, int32_t a2, int32_t a3);
int32_t id_5ba650(wl_char *self);
int32_t id_5ba6a0(wl_char *self, int32_t a2);
static intptr_t __attribute__((stdcall)) cb_id_5ba200(intptr_t a0, intptr_t a1) { return (intptr_t)id_5ba200(a0, a1); }   /* 0x5ba200 by address: retail's stdcall */

/* 0x5b9cf0: int __stdcall sub_5B9CF0(cls_5B9DE0 **a1, int a2) */
int32_t id_5b9cf0(intptr_t * a1, int32_t a2)
{

    intptr_t v2 = 0;
    intptr_t v3 = 0;

  v2 = (intptr_t)g_eng.op_new(0x498Cu);
  if ( v2 != 0 )
    v3 = id_5b9d90(v2);
  else
    v3 = 0;
  if ( id_5b9f80(v3, a2) == 1 )
  {
    *a1 = v3;
    return 1;
  }
  else
  {
    if ( v3 != 0 )
    {
      id_5b9de0(v3);
      g_eng.op_delete((void *)v3);
    }
    return 0;
  }
}

/* 0x5b9d90: void *__thiscall sub_5B9D90(cls_5B9D90 *this) */
int32_t id_5b9d90(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = 0;
  *(_DWORD *)&GAP(c, 0x4, 18820) = 0;
  strcpy(&GAP(c, 0x4, 18564), D(0x8a9b6c));
  return c;
}

/* 0x5b9de0: void __thiscall sub_5B9DE0(cls_5B9DE0 *this) */
void id_5b9de0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x0) != 0 )
  {
    (*(void (__stdcall **)(int))(*(_DWORD *)F32(c, 0x0) + 8))(F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  if ( F32(c, 4 * 4706) != 0 )
    API_OutputDebugStringA((intptr_t)(((char *)D(0x8a4bac))));
  if ( strlen((uint8_t *)c + 18568) != 0 )
    API_OutputDebugStringA((intptr_t)((char *)c + 18568));
}

/* 0x5b9e30: void __thiscall sub_5B9E30(cls_5B9E30 *this, const char *a2, const char *a3, int Value) */
void id_5b9e30(wl_char *self, char * a2, char * a3, int32_t Value)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    _Alignas(8) uint8_t frame_[0x40 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [64] at ebp-0x40 */

  v4 = c + 4 * (4642);
  strcpy((uint8_t *)c + 18568, ((char *)D(0x8a4bcc)));
  strcat((uint8_t *)c + 18568, a2);
  strcat((uint8_t *)c + 18568, D(0x85486c));
  strcat((uint8_t *)c + 18568, a3);
  strcat((uint8_t *)c + 18568, ((char *)D(0x8a4938)));
  RC_5c0907((intptr_t)(Value), (intptr_t)(Buffer), (intptr_t)(16));
  strcat((char *)v4, Buffer);
  strcat((char *)v4, D(0x8a4934));
  API_OutputDebugStringA((intptr_t)((char *)v4));
}

/* 0x5b9f80: int __thiscall sub_5B9F80(cls_5B9E30 *this, int a2) */
int32_t id_5b9f80(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;

  v3 = API_DirectInputCreateEx((intptr_t)(a2), (intptr_t)(1792), (intptr_t)(D(0x850ea0)), (intptr_t)(c), (intptr_t)(0));
  if ( v3 == 0 )
    return 1;
  id_5b9e30(c, D(0x8a9b6c), ((char *)D(0x8a4be8)), v3);
  return 0;
}

/* 0x5b9fc0: int __thiscall sub_5B9FC0(cls_5B9FC0 *this, _DWORD *a2, int a3) */
int32_t id_5b9fc0(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;

  v4 = (intptr_t)g_eng.op_new(0x118u);
  v5 = v4;
  if ( v4 != 0 )
    id_5ba220(v4, (int)c);
  else
    v5 = 0;
  if ( id_5ba410(v5, a3) )
  {
    *a2 = v5;
    return 1;
  }
  else
  {
    if ( v5 != 0 )
    {
      id_5ba240(v5);
      g_eng.op_delete((void *)v5);
    }
    return 0;
  }
}

/* 0x5ba070: int __thiscall sub_5BA070(cls_5B9FC0 *this, _DWORD *a2, int a3, int a4) */
int32_t id_5ba070(wl_char *self, int32_t * a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;

  v5 = (intptr_t)g_eng.op_new(0x78u);
  v6 = v5;
  if ( v5 != 0 )
    id_5ba220(v5, (int)c);
  else
    v6 = 0;
  if ( id_5ba540(v6, a3, a4) != 0 )
  {
    *a2 = v6;
    return 1;
  }
  else
  {
    if ( v6 != 0 )
    {
      F32(v6, 0x4) = 0;
      id_5ba240(v6);
      g_eng.op_delete((void *)v6);
    }
    return 0;
  }
}

/* 0x5ba120: int __thiscall sub_5BA120(cls_5B9FC0 *this, _DWORD *a2, int a3, int a4) */
int32_t id_5ba120(wl_char *self, int32_t * a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    int32_t * i = 0;

  v5 = 0;
  v6 = F32(c, 0x0);
  F32(c, 0x4884) = 0;
  (*(void (__stdcall **)(int, int, int (__stdcall *)(const void *, _DWORD *), void *, int))(F32(v6, 0) + 16))(
    v6,
    4,
    ((intptr_t)cb_id_5ba200),
    c,
    1);
  if ( a4 == -1 )
  {
    if ( F32(c, 0x4884) > 0 )
    {
      for ( i = &F32(c, 0x4); id_5ba070(c, a2, a3, (int)i) == 0; i += 145 )
      {
        if ( ++v5 >= F32(c, 0x4884) )
          return 0;
      }
      return 1;
    }
  }
  else if ( F32(c, 0x4884) > a4 && id_5ba070(c, a2, a3, (int)(&F32(c, 0x4) + 145 * a4)) != 0 )
  {
    return 1;
  }
  return 0;
}

/* 0x5ba1c0: int __thiscall sub_5BA1C0(cls_5BA1C0 *this, const void *a2) */
int32_t id_5ba1c0(wl_char *self, void * a2)
{
    intptr_t c = (intptr_t)self;

  qmemcpy(&GAP(c, 0x0, 580 * F32(c, 0x4884)++ + 4), a2, 0x244u);
  return 1;
}

/* 0x5ba200: int __stdcall sub_5BA200(void *a1, cls_5BA1C0 *a2) */
int32_t id_5ba200(void * a1, int32_t a2)
{


  return id_5ba1c0(a2, a1);
}

/* 0x5ba220: int __thiscall sub_5BA220(cls_5B9FC0 *this, int a2) */
int32_t id_5ba220(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = a2;
  F32(c, 0x4) = 0;
  F32(c, 0x8) = 0;
  ++F32(a2, 18824);
  return a2;
}

/* 0x5ba240: int __thiscall sub_5BA240(cls_5BA240 *this) */
int32_t id_5ba240(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t result = 0;

  if ( F32(c, 0x8) == 1 && F32(c, 0x4) != 0 )
    id_5ba2b0(c);
  g_eng.op_delete((void *)(void *)F32(c, 0x14));
  v2 = F32(c, 0x4);
  if ( v2 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v2, 0) + 8))(F32(c, 0x4));
    F32(c, 0x4) = 0;
  }
  v3 = F32(c, 0x0);
  result = F32(v3, 18824) - 1;
  F32(v3, 18824) = result;
  return result;
}

/* 0x5ba290: int __thiscall sub_5BA290(input_dev *this) */
int32_t id_5ba290(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( (*(int (__stdcall **)(void *))(*(_DWORD *)F32(c, 0x4) + 28))(F32(c, 0x4)) != 0 )
    return 0;
  F32(c, 0x8) = 1;
  return 1;
}

/* 0x5ba2b0: int __thiscall sub_5BA2B0(cls_5BA2B0 *this) */
int32_t id_5ba2b0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = (*(int (__stdcall **)(int))(*(_DWORD *)F32(c, 0x4) + 32))(F32(c, 0x4));
  F32(c, 0x8) = 0;
  return result;
}

/* 0x5ba2d0: BOOL __thiscall sub_5BA2D0(input_dev *this) */
int32_t id_5ba2d0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    char * v2 = 0;
    intptr_t result = 0;

  v2 = (uint8_t *)c + 16;
  F32(c, 4 * 4) = F32(c, 4 * 3);
  (*(void (__stdcall **)(void *))(*(_DWORD *)F32(c, 0x4) + 100))(F32(c, 0x4));
  result = true;
  if ( (*(int (__stdcall **)(void *, int, _DWORD, char *, _DWORD))(*(_DWORD *)F32(c, 0x4) + 40))(
         F32(c, 0x4),
         16,
         F32(c, 4 * 5),
         v2,
         0) == -2147024866
    && F32(c, 0x8) != 0 )
  {
    id_5ba290(c);
    if ( (*(int (__stdcall **)(void *, int, _DWORD, char *, _DWORD))(*(_DWORD *)F32(c, 0x4) + 40))(
           F32(c, 0x4),
           16,
           F32(c, 4 * 5),
           v2,
           0) != 0 )
      return false;
  }
  return result;
}

/* 0x5ba330: int __thiscall sub_5BA330(cls_5BA330 *this, int a2) */
int32_t id_5ba330(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v5 = (intptr_t *)(frame_ + 0x0);   /* [5] at ebp-0x14 */

  F32(c, 0xc) = a2;
  F32(c, 0x14) = (int)(intptr_t)g_eng.op_new(16 * a2);
  F32(c, 0x10) = 0;
  v3 = F32(c, 0x4);
  v5[0] = 20;
  v5[1] = 16;
  v5[2] = 0;
  v5[3] = 0;
  v5[4] = a2;
  return (*(int (__stdcall **)(int, int, _DWORD *))(F32(v3, 0) + 24))(v3, 1, v5);
}

/* 0x5ba3a0: int __thiscall sub_5BA3A0(cls_5BA3A0 *this, int a2) */
int32_t id_5ba3a0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return F32((16 * a2 + F32(c, 0x14)), 0);
}

/* 0x5ba3b0: int __thiscall sub_5BA3B0(cls_5BA3B0 *this, int a2) */
int32_t id_5ba3b0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return F32(16 * a2 + F32(c, 0x14), 4);
}

/* 0x5ba3d0: int __thiscall sub_5BA3D0(cls_5BA3D0 *this, int a2) */
int32_t id_5ba3d0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return F32(16 * a2 + F32(c, 0x14), 4) & 0x80;
}

/* 0x5ba3f0: int __thiscall sub_5BA3F0(cls_5BA3F0 *this, int a2) */
int32_t id_5ba3f0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return F32(16 * a2 + F32(c, 0x14), 8);
}

/* 0x5ba410: BOOL __thiscall sub_5BA410(cls_5BA330 *this, int a2) */
int32_t id_5ba410(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    int32_t * v3 = 0;
    intptr_t v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;

  memset(((uint8_t *)(c) + 0x18), 0, 0x100u);
  v3 = &F32(c, 0x4);
  v4 = (*(int (__stdcall **)(_DWORD, void *, void *, int *, _DWORD))(**(_DWORD **)F32(c, 0x0) + 36))(
         *(_DWORD *)F32(c, 0x0),
         D(0x850df0),
         D(0x850e90),
         &F32(c, 0x4),
         0);
  if ( v4 != 0 )
  {
    id_5b9e30(F32(c, 0x0), ((char *)D(0x8a4c20)), ((char *)D(0x8a4c2c)), v4);
    return false;
  }
  else
  {
    v6 = (*(int (__stdcall **)(int, void *))(*(_DWORD *)*v3 + 44))(*v3, D(0x850ec0));
    if ( v6 != 0 )
    {
      id_5b9e30(F32(c, 0x0), ((char *)D(0x8a4bfc)), ((char *)D(0x8a4c10)), v6);
      return false;
    }
    else
    {
      v7 = (*(int (__stdcall **)(int, int, int))(*(_DWORD *)*v3 + 52))(*v3, a2, 6);
      if ( v7 != 0 )
      {
        id_5b9e30(F32(c, 0x0), ((char *)D(0x8a4bfc)), ((char *)D(0x8a4b40)), v7);
        return false;
      }
      else
      {
        return id_5ba330(c, 32) == 0;
      }
    }
  }
}

/* 0x5ba4c0: BOOL __thiscall sub_5BA4C0(input_dev *this) */
int32_t id_5ba4c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  (*(void (__stdcall **)(void *))(*(_DWORD *)F32(c, 0x4) + 100))(F32(c, 0x4));
  v2 = (*(int (__stdcall **)(void *, int, void *))(*(_DWORD *)F32(c, 0x4) + 36))(F32(c, 0x4), 256, c + 0x18);
  if ( v2 == -2147024866 )
  {
    if ( F32(c, 0x8) == 0 )
      return false;
    id_5ba290(c);
    v2 = (*(int (__stdcall **)(void *, int, void *))(*(_DWORD *)F32(c, 0x4) + 36))(F32(c, 0x4), 256, c + 0x18);
  }
  return v2 == 0;
}

/* 0x5ba520: int __thiscall sub_5BA520(cls_5BA520 *this, unsigned __int8 a2) */
int32_t id_5ba520(wl_char *self, uint8_t a2)
{
    intptr_t c = (intptr_t)self;

  return *((uint8_t *)c + a2 + 24) & 0x80;
}

/* 0x5ba540: int __thiscall sub_5BA540(cls_5BA330 *this, int a2, int a3) */
int32_t id_5ba540(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  memset(((uint8_t *)(c) + 0x18), 0, 80);
  v4 = &F32(c, 0x4);
  v5 = (*(int (__stdcall **)(_DWORD, int, void *, int *, _DWORD))(**(_DWORD **)F32(c, 0x0) + 36))(
         *(_DWORD *)F32(c, 0x0),
         a3 + 4,
         D(0x850e90),
         &F32(c, 0x4),
         0);
  if ( v5 != 0 )
  {
    id_5b9e30(F32(c, 0x0), ((char *)D(0x8a4c20)), ((char *)D(0x8a4c44)), v5);
    return 0;
  }
  else
  {
    v7 = (*(int (__stdcall **)(int, void *))(*(_DWORD *)*v4 + 44))(*v4, D(0x850ef0));
    if ( v7 != 0 )
    {
      id_5b9e30(F32(c, 0x0), ((char *)D(0x8a4bfc)), ((char *)D(0x8a4c10)), v7);
      return 0;
    }
    else
    {
      v8 = (*(int (__stdcall **)(int, int, int))(*(_DWORD *)*v4 + 52))(*v4, a2, 6);
      if ( v8 != 0 )
      {
        id_5b9e30(F32(c, 0x0), ((char *)D(0x8a4bfc)), ((char *)D(0x8a4b40)), v8);
        return 0;
      }
      else
      {
        result = id_5ba6a0(c, 0);
        if ( result != 0 )
        {
          result = id_5ba6a0(c, 4);
          if ( result != 0 )
          {
            id_5ba6a0(c, 8);
            id_5ba6a0(c, 12);
            id_5ba6a0(c, 16);
            id_5ba6a0(c, 20);
            F32(c, 0x68) = 0;
            F32(c, 0x6c) = 0;
            F32(c, 0x70) = 0;
            F32(c, 0x74) = 0;
            id_5ba330(c, 32);
            return 1;
          }
        }
      }
    }
  }
  return result;
}

/* 0x5ba650: BOOL __thiscall sub_5BA650(input_dev *this) */
int32_t id_5ba650(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  (*(void (__stdcall **)(void *))(*(_DWORD *)F32(c, 0x4) + 100))(F32(c, 0x4));
  v2 = (*(int (__stdcall **)(void *, int, void *))(*(_DWORD *)F32(c, 0x4) + 36))(F32(c, 0x4), 80, c + 0x18);
  if ( v2 == -2147024866 )
  {
    if ( F32(c, 0x8) == 0 )
      return false;
    id_5ba290(c);
    v2 = (*(int (__stdcall **)(void *, int, void *))(*(_DWORD *)F32(c, 0x4) + 36))(F32(c, 0x4), 80, c + 0x18);
  }
  return v2 == 0;
}

/* 0x5ba6a0: BOOL __thiscall sub_5BA6A0(cls_5BA6A0 *this, int a2) */
int32_t id_5ba6a0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v5 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v6 = (intptr_t *)(frame_ + 0x0);   /* [5] at ebp-0x2c */
    intptr_t *v7 = (intptr_t *)(frame_ + 0x14);   /* [6] at ebp-0x18 */

  v3 = F32(c, 0x4);
  v7[0] = 24;
  v7[1] = 16;
  v7[3] = 1;
  v7[4] = -1000;
  v7[5] = 1000;
  v7[2] = a2;
  if ( (*(int (__stdcall **)(int, int, _DWORD *))(F32(v3, 0) + 24))(v3, 4, v7) != 0 )
    return false;
  v5 = F32(c, 0x4);
  v6[0] = 20;
  v6[1] = 16;
  v6[3] = 1;
  v6[4] = 1000;
  v6[2] = a2;
  return (*(int (__stdcall **)(int, int, _DWORD *))(F32(v5, 0) + 24))(v5, 5, v6) == 0;
}
/* unported callees: 0x5c0907 */
