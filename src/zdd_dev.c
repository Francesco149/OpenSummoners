/* zdd_dev.c — the DirectDraw DEVICE layer ("zdd"): the device object 0x5b7ee0 / 0x5b7f80 (DirectDrawCreateEx
 * 0x5b88c0), the cooperative level / display mode 0x5b89d0 / 0x5b8900 / 0x5b8950, the PRIMARY surface + back buffers
 * 0x5b8480 (windowed blit / exclusive flip / the stretched modes 3 / 4 with their attached surface 0x5b9740), the
 * 8-bit palette 0x5b8e00, the 16-bit format masks 0x5b8a20, the cursor 0x5b8da0 / 0x5b8dd0, the FLIP 0x5b8fc0 (+ the
 * 2x upscale 0x5b8ea0), WM_PAINT's blit 0x5b9130, lost-surface checks 0x5b91d0 / 0x5b9240, the frees 0x5b7fe0 /
 * 0x5b8040, the DDERR text log 0x5b80d0, the surface clear 0x5b9410; and the alpha-blend LUT set-up 0x5bd4d0 / 0x5bd3b0
 * / 0x5bd3d0 → 0x5bd040 (+ 0x5bcff0 / 0x5bd020 / 0x5bd380 / 0x5bd520).  See zdd_dev.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  The Flip runs under rhost's
 * own flip stub (the virtual clock + the render gate's back-buffer hash). */
#include "zdd_dev.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))


#include "render_platform.h"
#include "render_platform.h"
#include "render_platform.h"
#include "win_main.h"
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

static inline intptr_t W_rp_5b9350(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b9350((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_rp_5b9490(intptr_t a0, ...) { return (intptr_t)rp_5b9490((wl_char *)a0); }
static inline intptr_t W_rp_5b94d0(intptr_t a0, ...) { return (intptr_t)rp_5b94d0((wl_char *)a0); }
static inline intptr_t W_rp_5b94e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b94e0((wl_char *)a0, (intptr_t *)a1); }
static inline intptr_t W_rp_5b9500(intptr_t a0, intptr_t a1, ...) { rp_5b9500((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_rp_5b9520(intptr_t a0, ...) { return (intptr_t)rp_5b9520((wl_char *)a0); }
static inline intptr_t W_rp_5b95c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)rp_5b95c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_rp_5b97e0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rp_5b97e0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rp_5b9830(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b9830((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_rp_5b98c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)rp_5b98c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_rp_5b9a40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)rp_5b9a40((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_wn_5b9ab0(intptr_t a0, ...) { return (intptr_t)wn_5b9ab0((wl_char *)a0); }
static inline intptr_t W_wn_5b9ac0(intptr_t a0, ...) { return (intptr_t)wn_5b9ac0((wl_char *)a0); }
typedef intptr_t (*rt_5c0907)(intptr_t, intptr_t, intptr_t);   /* 0x5c0907 cdecl: unported, retail's */
#define RC_5c0907 ((rt_5c0907)g_eng.retail(0x5c0907u))
typedef intptr_t (__attribute__((stdcall)) *api_BeginPaint_t)(intptr_t, intptr_t);
#define API_BeginPaint ((api_BeginPaint_t)g_eng.api("BeginPaint"))
typedef intptr_t (__attribute__((stdcall)) *api_BitBlt_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_BitBlt ((api_BitBlt_t)g_eng.api("BitBlt"))
typedef intptr_t (__attribute__((stdcall)) *api_DirectDrawCreateEx_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_DirectDrawCreateEx ((api_DirectDrawCreateEx_t)g_eng.api("DirectDrawCreateEx"))
typedef intptr_t (__attribute__((stdcall)) *api_EndPaint_t)(intptr_t, intptr_t);
#define API_EndPaint ((api_EndPaint_t)g_eng.api("EndPaint"))
typedef intptr_t (__attribute__((stdcall)) *api_GetDC_t)(intptr_t);
#define API_GetDC ((api_GetDC_t)g_eng.api("GetDC"))
typedef uint32_t (__attribute__((stdcall)) *api_GetSystemPaletteEntries_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_GetSystemPaletteEntries ((api_GetSystemPaletteEntries_t)g_eng.api("GetSystemPaletteEntries"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
typedef intptr_t (__attribute__((stdcall)) *api_ReleaseDC_t)(intptr_t, intptr_t);
#define API_ReleaseDC ((api_ReleaseDC_t)g_eng.api("ReleaseDC"))
typedef intptr_t (__attribute__((stdcall)) *api_ShowCursor_t)(intptr_t);
#define API_ShowCursor ((api_ShowCursor_t)g_eng.api("ShowCursor"))
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

/* 0x5b7ee0: int __stdcall sub_5B7EE0(cls_5B7FE0 **a1) */
int32_t zd_5b7ee0(intptr_t * a1)
{

    intptr_t v1 = 0;
    intptr_t v2 = 0;

  v1 = (intptr_t)g_eng.op_new(0x170u);
  if ( v1 != 0 )
    v2 = zd_5b7f80(v1);
  else
    v2 = 0;
  if ( zd_5b88c0(v2) != 0 )
  {
    *a1 = v2;
    return 1;
  }
  else
  {
    if ( v2 != 0 )
    {
      zd_5b7fe0(v2);
      g_eng.op_delete((void *)v2);
    }
    return 0;
  }
}

/* 0x5b7f80: int __thiscall zdd_ctor_5B7F80(cls_5B7F80 *this) */
int32_t zd_5b7f80(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x130) = 1;
  F32(c, 0x124) = 0;
  F32(c, 0x128) = 0;
  F32(c, 0x16c) = 0;
  F32(c, 0x18) = 0;
  F32(c, 0x1c) = 0;
  F32(c, 0x12c) = 0;
  F32(c, 0x120) = 0;
  strcpy(((uint8_t *)(c) + 0x20), D(0x8a9b6c));
  return (int)((uint8_t *)(c) + 0x0);
}

/* 0x5b7fe0: void __thiscall sub_5B7FE0(cls_5B7FE0 *this) */
void zd_5b7fe0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  zd_5b8da0(c);
  zd_5b8040(c);
  v2 = F32(c, 0x124);
  if ( v2 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v2, 0) + 8))(F32(c, 0x124));
    F32(c, 0x124) = 0;
  }
  if ( F32(c, 0x120) != 0 )
    API_OutputDebugStringA((intptr_t)(((char *)D(0x8a4914))));
  if ( strlen(((uint8_t *)(c) + 0x20)) != 0 )
    API_OutputDebugStringA((intptr_t)(((uint8_t *)(c) + 0x20)));
}

/* 0x5b8040: int __thiscall sub_5B8040(cls_5B7FE0 *this) */
int32_t zd_5b8040(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    void * v2 = 0;
    void * v3 = 0;
    void * v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;

  v2 = (void *)F32(c, 0x16c);
  if ( v2 != 0 )
  {
    W_rp_5b9390((intptr_t)(F32(c, 0x16c)));
    g_eng.op_delete((void *)v2);
    F32(c, 0x16c) = 0;
  }
  v3 = (void *)F32(c, 0x18);
  if ( v3 != 0 )
  {
    W_rp_5b9390((intptr_t)(F32(c, 0x18)));
    g_eng.op_delete((void *)v3);
    F32(c, 0x18) = 0;
  }
  v4 = (void *)F32(c, 0x1c);
  if ( v4 != 0 )
  {
    W_rp_5b9390((intptr_t)(F32(c, 0x1c)));
    g_eng.op_delete((void *)v4);
    F32(c, 0x1c) = 0;
  }
  v5 = F32(c, 0x128);
  if ( v5 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v5, 0) + 8))(F32(c, 0x128));
    F32(c, 0x128) = 0;
  }
  result = F32(c, 0x12c);
  if ( result != 0 )
  {
    result = (*(int (__stdcall **)(int))(F32(result, 0) + 8))(F32(c, 0x12c));
    F32(c, 0x12c) = 0;
  }
  return result;
}

/* 0x5b80d0: void __thiscall sub_5B80D0(cls_5B7FE0 *this, const char *a2, const char *a3, int Value) */
void zd_5b80d0(wl_char *self, char * a2, char * a3, int32_t Value)
{
    intptr_t c = (intptr_t)self;
    char * v5 = 0;
    _Alignas(8) uint8_t frame_[0x54 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x14);   /* [64] at ebp-0x40 */
    char *const v6__f = (char *)(frame_ + 0x0);   /* ebp-0x54 */
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x1);   /* ebp-0x53 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x5);   /* ebp-0x4f */
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x9);   /* ebp-0x4b */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0xd);   /* ebp-0x47 */
    int16_t *const v11__f = (int16_t *)(frame_ + 0x11);   /* ebp-0x43 */
    char *const v12__f = (char *)(frame_ + 0x13);   /* ebp-0x41 */

  (*v7__f) = 0;
  (*v8__f) = 0;
  (*v9__f) = 0;
  (*v10__f) = 0;
  (*v6__f) = F8(D(0x8a9b6c), 0);
  (*v11__f) = 0;
  (*v12__f) = 0;
  if ( Value > -2005532442 )
  {
    if ( Value > -2005532108 )
    {
      switch ( Value )
      {
        case -2005532107:
          v5 = ((char *)D(0x8a4968));
          goto LABEL_39;
        case -2005532082:
          v5 = ((char *)D(0x8a497c));
          goto LABEL_39;
        case -2005532081:
          v5 = ((char *)D(0x8a4994));
          goto LABEL_39;
        default:
          break;
      }
    }
    else
    {
      if ( Value == -2005532108 )
      {
        v5 = ((char *)D(0x8a49a8));
        goto LABEL_39;
      }
      if ( Value > -2005532292 )
      {
        if ( Value == -2005532109 )
        {
          v5 = ((char *)D(0x8a49cc));
          goto LABEL_39;
        }
      }
      else
      {
        switch ( Value )
        {
          case -2005532292:
            v5 = ((char *)D(0x8a49e4));
            goto LABEL_39;
          case -2005532412:
            v5 = ((char *)D(0x8a49fc));
            goto LABEL_39;
          case -2005532332:
            v5 = ((char *)D(0x8a4a10));
            goto LABEL_39;
          default:
            break;
        }
      }
    }
  }
  else
  {
    if ( Value == -2005532442 )
    {
      v5 = ((char *)D(0x8a4a24));
      goto LABEL_39;
    }
    if ( Value > -2005532542 )
    {
      switch ( Value )
      {
        case -2005532527:
          v5 = ((char *)D(0x8a4a78));
          goto LABEL_39;
        case -2005532492:
          v5 = ((char *)D(0x8a4a68));
          goto LABEL_39;
        case -2005532460:
          v5 = ((char *)D(0x8a4a4c));
          goto LABEL_39;
        case -2005532447:
          v5 = ((char *)D(0x8a4a34));
          goto LABEL_39;
        default:
          break;
      }
    }
    else
    {
      if ( Value == -2005532542 )
      {
        v5 = ((char *)D(0x8a4a94));
        goto LABEL_39;
      }
      if ( Value > -2005532577 )
      {
        if ( Value == -2005532572 )
        {
          v5 = ((char *)D(0x8a4aa8));
          goto LABEL_39;
        }
      }
      else
      {
        switch ( Value )
        {
          case -2005532577:
            v5 = ((char *)D(0x8a4abc));
            goto LABEL_39;
          case -2147024882:
            v5 = ((char *)D(0x8a4ad8));
            goto LABEL_39;
          case -2147024809:
            v5 = ((char *)D(0x8a4aec));
LABEL_39:
            strcpy(&(*v6__f), v5);
            break;
          default:
            break;
        }
      }
    }
  }
  strcpy(((uint8_t *)(c) + 0x20), ((char *)D(0x8a494c)));
  strcat(((uint8_t *)(c) + 0x20), a2);
  strcat(((uint8_t *)(c) + 0x20), D(0x85486c));
  strcat(((uint8_t *)(c) + 0x20), a3);
  strcat(((uint8_t *)(c) + 0x20), ((char *)D(0x8a4938)));
  RC_5c0907((intptr_t)(Value), (intptr_t)(Buffer), (intptr_t)(16));
  strcat(((uint8_t *)(c) + 0x20), Buffer);
  strcat(((uint8_t *)(c) + 0x20), &(*v6__f));
  strcat(((uint8_t *)(c) + 0x20), D(0x8a4934));
  API_OutputDebugStringA((intptr_t)(((uint8_t *)(c) + 0x20)));
}

/* 0x5b8480: int __thiscall zdd_create_primary_surface_5B8480( cls_5B7FE0 *this, int a2, int a3, int a4, int a5, int a6, const void *a7) */
int32_t zd_5b8480(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, void * a7)
{
    intptr_t c = (intptr_t)self;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    void * v25 = 0;
    void * v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    void * v30 = 0;
    intptr_t v31 = 0;
    _Alignas(8) uint8_t frame_[0xac + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v35 = (intptr_t *)(frame_ + 0x24);   /* [31] at ebp-0x88 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xac */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xa8 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xa4 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xa8);   /* ebp-0x4 */

  zd_5b8040(c);
  F32(c, 0x134) = a6;
  F32(c, 0x144) = a3;
  F32(c, 0x164) = a5;
  F32(c, 0x138) = 0;
  F32(c, 0x13c) = 0;
  F32(c, 0x140) = a2;
  v8 = &F32(c, 0x148);
  if ( a7 != 0 )
    qmemcpy(v8, a7, 0x1Cu);
  else
    memset(v8, 0, 0x1Cu);
  if ( F32(c, 0x164) == 2 )
  {
    v9 = F32(c, 0x128);
    v10 = &F32(c, 0x128);
    if ( v9 != 0 )
    {
      (*(void (__stdcall **)(int))(F32(v9, 0) + 8))(F32(c, 0x128));
      *v10 = 0;
    }
    goto LABEL_7;
  }
  memset(v35, 0, (31 * sizeof(intptr_t)));
  v35[0] = 124;
  if ( a5 != 0 )
  {
    if ( a5 <= 2 || a5 > 4 )
    {
      v35[1] = 1;
      v35[26] = 512;
      goto LABEL_23;
    }
    v35[1] = 33;
    v35[26] = 536;
  }
  else
  {
    v16 = F32(c, 0x134);
    v35[1] = 33;
    v35[26] = 536;
    if ( v16 != 0 )
      v35[26] = 2584;
  }
  v35[5] = 1;
LABEL_23:
  v10 = &F32(c, 0x128);
  v17 = (*(int (__stdcall **)(int, _DWORD *, int *, _DWORD))(*(_DWORD *)F32(c, 0x124) + 24))(
          F32(c, 0x124),
          v35,
          &F32(c, 0x128),
          0);
  if ( v17 != 0 )
  {
    zd_5b80d0(c, ((char *)D(0x8a4b00)), ((char *)D(0x8a4b0c)), v17);
    return 0;
  }
LABEL_7:
  F32(c, 0x168) = a4;
  if ( a4 == 8 )
    zd_5b8e00(c);
  v11 = (intptr_t)g_eng.op_new(0xD8u);
  v12 = 0;
  (*v36__f) = 0;
  if ( v11 != 0 )
    v12 = W_rp_5b9350((intptr_t)(v11), (intptr_t)((int)c));
  v13 = F32(c, 0x164);
  (*v36__f) = -1;
  F32(c, 0x16c) = (int)v12;
  if ( v13 == 0 )
  {
    if ( zd_5b9740(v12, *v10, a2, a3, F32(c, 0x134)) == 0 )
    {
      v31 = F32(c, 0x16c);
      if ( v31 != 0 )
      {
LABEL_53:
        W_rp_5b9390((intptr_t)(v31));
        g_eng.op_delete((void *)v31);
      }
LABEL_54:
      F32(c, 0x16c) = 0;
      return 0;
    }
    goto LABEL_55;
  }
  v14 = v13 - 3;
  if ( v14 == 0 )
  {
    v27 = (intptr_t)g_eng.op_new(0xD8u);
    (*v36__f) = 3;
    if ( v27 != 0 )
      v28 = W_rp_5b9350((intptr_t)(v27), (intptr_t)((int)c));
    else
      v28 = 0;
    v29 = *v10;
    (*v36__f) = -1;
    F32(c, 0x18) = (int)v28;
    if ( zd_5b9740(v28, v29, a2, a3, 0) != 0 )
    {
      v15 = W_rp_5b95c0((intptr_t)(F32(c, 0x16c)), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0x1FFFFFF), (intptr_t)(F32(c, 0x134)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(a3));
LABEL_48:
      if ( v15 == 0 )
      {
        v31 = F32(c, 0x16c);
        if ( v31 != 0 )
          goto LABEL_53;
        goto LABEL_54;
      }
      goto LABEL_55;
    }
    goto LABEL_44;
  }
  if ( v14 != 1 )
  {
    v15 = W_rp_5b95c0((intptr_t)(v12), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0x1FFFFFF), (intptr_t)(F32(c, 0x134)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(a3));
    goto LABEL_48;
  }
  v19 = (intptr_t)g_eng.op_new(0xD8u);
  (*v36__f) = 1;
  if ( v19 != 0 )
    v20 = W_rp_5b9350((intptr_t)(v19), (intptr_t)((int)c));
  else
    v20 = 0;
  (*v34__f) = F32(c, 0x14c);
  (*v33__f) = F32(c, 0x148);
  (*v32__f) = *v10;
  (*v36__f) = -1;
  F32(c, 0x18) = (int)v20;
  if ( zd_5b9740(v20, (*v32__f), (*v33__f), (*v34__f), 0) == 0 )
  {
LABEL_44:
    v30 = (void *)F32(c, 0x18);
    if ( v30 != 0 )
    {
      W_rp_5b9390((intptr_t)(F32(c, 0x18)));
      g_eng.op_delete((void *)v30);
    }
    F32(c, 0x18) = 0;
    return 0;
  }
  v21 = (intptr_t)g_eng.op_new(0xD8u);
  (*v36__f) = 2;
  if ( v21 != 0 )
    v22 = W_rp_5b9350((intptr_t)(v21), (intptr_t)((int)c));
  else
    v22 = 0;
  v23 = F32(c, 0x160);
  v24 = F32(c, 0x15c);
  (*v36__f) = -1;
  F32(c, 0x1c) = (int)v22;
  if ( W_rp_5b95c0((intptr_t)(v22), (intptr_t)(v24), (intptr_t)(v23), (intptr_t)(0), (intptr_t)(0x1FFFFFF), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v24), (intptr_t)(v23)) != 0 )
  {
    if ( W_rp_5b95c0((intptr_t)(F32(c, 0x16c)), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0x1FFFFFF), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(a3)) != 0 )
    {
LABEL_55:
      W_rp_5b9520((intptr_t)(F32(c, 0x16c)));
      if ( F32(c, 0x168) == 16 )
        zd_5b8a20(c, F32(F32(c, 0x16c), 44));
      return 1;
    }
    v26 = (void *)F32(c, 0x16c);
    if ( v26 != 0 )
    {
      W_rp_5b9390((intptr_t)(F32(c, 0x16c)));
      g_eng.op_delete((void *)v26);
    }
    F32(c, 0x16c) = 0;
    return 0;
  }
  else
  {
    v25 = (void *)F32(c, 0x1c);
    if ( v25 != 0 )
    {
      W_rp_5b9390((intptr_t)(F32(c, 0x1c)));
      g_eng.op_delete((void *)v25);
    }
    F32(c, 0x1c) = 0;
    return 0;
  }
}

/* 0x5b88c0: int __thiscall zdd_directdraw_create_ex_5B88C0(cls_5B7FE0 *this) */
int32_t zd_5b88c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  v2 = API_DirectDrawCreateEx((intptr_t)(0), (intptr_t)((intptr_t *)&F32(c, 0x124)), (intptr_t)(D(0x850eb0)), (intptr_t)(0));
  if ( v2 == 0 )
    return 1;
  zd_5b80d0(c, D(0x8a9b6c), ((char *)D(0x8a4b1c)), v2);
  return 0;
}

/* 0x5b8900: int __thiscall zdd_set_display_mode_5B8900(cls_5B7FE0 *this, int a2, int a3, int a4, int a5) */
int32_t zd_5b8900(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;

  v6 = (*(int (__stdcall **)(int, int, int, int, int, _DWORD))(*(_DWORD *)F32(c, 0x124) + 84))(
         F32(c, 0x124),
         a2,
         a3,
         a4,
         a5,
         0);
  if ( v6 == 0 )
    return 1;
  zd_5b80d0(c, ((char *)D(0x8a4b00)), ((char *)D(0x8a4b30)), v6);
  return 0;
}

/* 0x5b8950: int __thiscall zdd_get_display_mode_5B8950(cls_5B8950 *this, _DWORD *a2, _DWORD *a3, _DWORD *a4) */
int32_t zd_5b8950(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    _Alignas(8) uint8_t frame_[0x7c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v6 = (intptr_t *)(frame_ + 0x0);   /* [31] at ebp-0x7c */

  v4 = F32(c, 0x124);
  v6[0] = 124;
  v6[1] = 266246;
  v6[18] = 32;
  v6[19] = 64;
  if ( (*(int (__stdcall **)(int, _DWORD *))(F32(v4, 0) + 48))(v4, v6) != 0 )
    return 0;
  if ( a2 != 0 )
    *a2 = v6[3];
  if ( a3 != 0 )
    *a3 = v6[2];
  if ( a4 != 0 )
    *a4 = v6[6];
  return 1;
}

/* 0x5b89d0: int __thiscall zdd_set_coop_level_5B89D0(cls_5B7FE0 *this, int a2, int a3) */
int32_t zd_5b89d0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v6__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */

  if ( a3 != 0 )
    (*v6__f) = 19;
  else
    (*v6__f) = 8;
  v4 = (*(int (__stdcall **)(int, int, int))(*(_DWORD *)F32(c, 0x124) + 80))(F32(c, 0x124), a2, (*v6__f));
  if ( v4 == 0 )
    return 1;
  zd_5b80d0(c, ((char *)D(0x8a4b00)), ((char *)D(0x8a4b40)), v4);
  return 0;
}

/* 0x5b8a20: int __thiscall sub_5B8A20(cls_5B8A20 *this, int *a2) */
int32_t zd_5b8a20(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint32_t v5 = 0;
    uint8_t v6 = 0;
    int v7 = 0;
    uint32_t v8 = 0;
    char v9 = 0;
    int v10 = 0;
    uint32_t v11 = 0;
    char v12 = 0;
    _Alignas(8) uint8_t frame_[0x7c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v13 = (intptr_t *)(frame_ + 0x0);   /* [22] at ebp-0x7c */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x24 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x20 */
    uint32_t *const v16__f = (uint32_t *)(frame_ + 0x60);   /* ebp-0x1c */

  v3 = *a2;
  v13[0] = 124;
  v13[1] = 4096;
  if ( (*(int (__stdcall **)(int *, _DWORD *))(v3 + 88))(a2, v13) != 0 )
    return 0;
  v5 = (*v14__f);
  v6 = 0;
  v7 = ((*v14__f) & 1) == 0;
  F32(c, 0x4) = (*v14__f);
  if ( v7 )
  {
    do
    {
      v5 >>= 1;
      ++v6;
    }
    while ( (v5 & 1) == 0 );
  }
  GAP(c, 0x10, 3) = v5;
  F8(c, 0x0) = v6;
  GAP(c, 0x10, 0) = (v5 == 31) + 2;
  v8 = (*v15__f);
  v9 = 0;
  for ( F32(c, 0x8) = (*v15__f); (v8 & 1) == 0; ++v9 )
    v8 >>= 1;
  GAP(c, 0x1, 0) = v9;
  v10 = v8 == 31;
  GAP(c, 0x10, 4) = v8;
  v11 = (*v16__f);
  GAP(c, 0x10, 1) = v10 + 2;
  v12 = 0;
  for ( F32(c, 0xc) = v11; (v11 & 1) == 0; ++v12 )
    v11 >>= 1;
  GAP(c, 0x10, 5) = v11;
  GAP(c, 0x1, 1) = v12;
  GAP(c, 0x10, 2) = (v11 == 31) + 2;
  return 1;
}

/* 0x5b8ae0: int __thiscall pd_format_get_masks_5B8AE0(cls_5B8AE0 *this, _DWORD *a2, _DWORD *a3, _DWORD *a4) */
int32_t zd_5b8ae0(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  *a2 = F32(c, 0x4);
  *a3 = F32(c, 0x8);
  result = F32(c, 0xc);
  *a4 = result;
  return result;
}

/* 0x5b8da0: int __thiscall sub_5B8DA0(cls_5B8DA0 *this) */
int32_t zd_5b8da0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = F32(c, 0x130);
  if ( result == 0 )
  {
    result = API_ShowCursor((intptr_t)(true));
    F32(c, 0x130) = 1;
  }
  return result;
}

/* 0x5b8dd0: int __thiscall sub_5B8DD0(cls_5B8DD0 *this) */
int32_t zd_5b8dd0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = F32(c, 0x130);
  if ( result != 0 )
  {
    result = API_ShowCursor((intptr_t)(false));
    F32(c, 0x130) = 0;
  }
  return result;
}

/* 0x5b8e00: int __thiscall sub_5B8E00(cls_5B7FE0 *this) */
int32_t zd_5b8e00(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    _Alignas(8) uint8_t frame_[0x400 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const pPalEntries__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x400 */

  API_GetSystemPaletteEntries((intptr_t)(0), (intptr_t)(0), (intptr_t)(0x100u), (intptr_t)(&(*pPalEntries__f)));
  v2 = (*(int (__stdcall **)(int, int, struct tagPALETTEENTRY *, int *, _DWORD))(*(_DWORD *)F32(c, 0x124) + 20))(
         F32(c, 0x124),
         4,
         &(*pPalEntries__f),
         &F32(c, 0x12c),
         0);
  if ( v2 != 0 )
  {
    zd_5b80d0(c, ((char *)D(0x8a4b00)), ((char *)D(0x8a4b74)), v2);
    return 0;
  }
  else
  {
    v4 = F32(c, 0x128);
    if ( v4 != 0 && (v5 = (*(int (__stdcall **)(int, int))(F32(v4, 0) + 124))(v4, F32(c, 0x12c))) != 0 )
    {
      zd_5b80d0(c, ((char *)D(0x8a4b54)), ((char *)D(0x8a4b68)), v5);
      return 0;
    }
    else
    {
      return 1;
    }
  }
}

/* 0x5b8ea0: int __stdcall zdd_object_upscale_16bpp_5B8EA0(cls_5B9410 *a1, cls_5B9410 *a2, int a3) */
int32_t zd_5b8ea0(int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t v3 = 0;
    intptr_t result = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    uint16_t * v9 = 0;
    uint16_t * v10 = 0;
    uint16_t * v11 = 0;
    uint16_t * v12 = 0;
    uint16_t * v13 = 0;
    intptr_t j = 0;
    intptr_t k = 0;
    int v16 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    uint16_t * *const v19__f = (uint16_t * *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  v3 = *(_DWORD *)&GAP(a2, 0xc, 16);
  (*v17__f) = v3;
  result = W_rp_5b9490((intptr_t)(a1));
  if ( result != 0 )
  {
    result = W_rp_5b9490((intptr_t)(a2));
    if ( result != 0 )
    {
      v5 = F32(F32(a1, 0x8), 0);
      v6 = a3;
      v7 = F32(F32(a1, 0x4), 0) / 2;
      (*v21__f) = v7;
      if ( v3 >= v7 / a3 )
      {
        v3 = v7 / a3;
        (*v17__f) = v7 / a3;
      }
      v8 = v5 / a3;
      if ( v5 < v5 / a3 )
        v8 = F32(F32(a1, 0x8), 0);
      v9 = *(_WORD **)F32(a1, 0x0);
      v10 = *(_WORD **)F32(a2, 0x0);
      if ( v8 > 0 )
      {
        (*v19__f) = *(_WORD **)F32(a1, 0x0);
        (*v22__f) = 2 * (F32(F32(a2, 0x4), 0) / 2);
        (*v20__f) = v8;
        do
        {
          if ( v6 > 0 )
          {
            v11 = v9;
            for ( (*i__f) = a3; (*i__f) != 0; --(*i__f) )
            {
              v12 = v10;
              v13 = v11;
              if ( v3 > 0 )
              {
                for ( j = v3; j != 0; --j )
                {
                  for ( k = a3; k != 0; --k )
                    *v13++ = *v12;
                  ++v12;
                }
                v3 = (*v17__f);
                v7 = (*v21__f);
              }
              v11 += v7;
            }
            v6 = a3;
          }
          v10 = (_WORD *)((uint8_t *)v10 + (*v22__f));
          v9 = &(*v19__f)[2 * v7];
          v16 = (*v20__f) == 1;
          (*v19__f) = v9;
          --(*v20__f);
        }
        while ( !v16 );
      }
      W_rp_5b94d0((intptr_t)(a2));
      return W_rp_5b94d0((intptr_t)(a1));
    }
  }
  return result;
}

/* 0x5b8fc0: void __thiscall flip_5B8FC0(cls_5B7FE0 *this, HWND hWnd) */
void zd_5b8fc0(wl_char *self, int32_t hWnd)
{
    intptr_t c = (intptr_t)self;
    intptr_t DC = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const hdcSrc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  (*hdcSrc__f) = 0;
  switch ( F32(c, 0x164) )
  {
    case 0:
      (*(void (__stdcall **)(int, _DWORD, int))(*(_DWORD *)F32(c, 0x128) + 44))(
        F32(c, 0x128),
        F32(F32(c, 0x16c), 44),
        1);
      break;
    case 1:
      (*(void (__stdcall **)(int, int *, _DWORD, int *, int, _DWORD))(*(_DWORD *)F32(c, 0x128) + 20))(
        F32(c, 0x128),
        &F32(c, 0x138),
        F32(F32(c, 0x16c), 44),
        &F32(c, 0x138),
        0x1000000,
        0);
      break;
    case 2:
      W_rp_5b94e0((intptr_t)((intptr_t)F32(c, 0x16c)), (intptr_t)((intptr_t *)&(*hdcSrc__f)));
      DC = API_GetDC((intptr_t)(hWnd));
      API_BitBlt((intptr_t)(DC), (intptr_t)(F32(c, 0x138)), (intptr_t)(F32(c, 0x13c)), (intptr_t)(F32(c, 0x140)), (intptr_t)(F32(c, 0x144)), (intptr_t)((*hdcSrc__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xCC0020u));
      API_ReleaseDC((intptr_t)(hWnd), (intptr_t)(DC));
      W_rp_5b9500((intptr_t)((intptr_t)F32(c, 0x16c)), (intptr_t)((intptr_t)(*hdcSrc__f)));
      break;
    case 3:
      (*(void (__stdcall **)(_DWORD, int *, _DWORD, int *, int, _DWORD))(F32(F32(F32(c, 0x18), 44), 0) + 20))(
        F32(F32(c, 0x18), 44),
        &F32(c, 0x138),
        F32(F32(c, 0x16c), 44),
        &F32(c, 0x138),
        0x1000000,
        0);
      goto LABEL_4;
    case 4:
      zd_5b8ea0(F32(c, 0x1c), F32(c, 0x16c), F32(c, 0x150));
      W_rp_5b9a40((intptr_t)(F32(c, 0x1c)), (intptr_t)(F32(c, 0x18)), (intptr_t)(F32(c, 0x154)), (intptr_t)(F32(c, 0x158)));
LABEL_4:
      (*(void (__stdcall **)(int, _DWORD, int))(*(_DWORD *)F32(c, 0x128) + 44))(
        F32(c, 0x128),
        F32(F32(c, 0x18), 44),
        1);
      break;
    default:
      return;
  }
}

/* 0x5b9130: int __thiscall sub_5B9130(paint_ctx *this, HWND target) */
int32_t zd_5b9130(wl_char *self, int32_t target)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    _Alignas(8) uint8_t frame_[0x40 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const Paint__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x40 */

  if ( F32(c, 0x164) != 2 )
    return 0;
  v3 = target;
  v4 = API_BeginPaint((intptr_t)(target), (intptr_t)(&(*Paint__f)));
  W_rp_5b94e0((intptr_t)(F32(c, 0x16c)), (intptr_t)(&target));
  API_BitBlt((intptr_t)(v4), (intptr_t)(F32(c, 0x138)), (intptr_t)(F32(c, 0x13c)), (intptr_t)(F32(c, 0x140)), (intptr_t)(F32(c, 0x144)), (intptr_t)((intptr_t)target), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xCC0020u));
  W_rp_5b9500((intptr_t)(F32(c, 0x16c)), (intptr_t)(target));
  API_EndPaint((intptr_t)(v3), (intptr_t)(&(*Paint__f)));
  return 1;
}

/* 0x5b91d0: BOOL __thiscall sub_5B91D0(cls_5B91D0 *this) */
int32_t zd_5b91d0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;

  v2 = F32(c, 0x128);
  if ( v2 != 0 && (*(int (__stdcall **)(int))(F32(v2, 0) + 96))(v2) == -2005532222 )
    return true;
  v4 = F32(c, 0x16c);
  if ( v4 != 0 && W_wn_5b9ac0((intptr_t)(v4)) )
    return true;
  v5 = F32(c, 0x18);
  if ( v5 != 0 && W_wn_5b9ac0((intptr_t)(v5)) )
    return true;
  v6 = F32(c, 0x1c);
  return v6 != 0 && W_wn_5b9ac0((intptr_t)(v6));
}

/* 0x5b9240: int __thiscall sub_5B9240(cls_5B9240 *this) */
int32_t zd_5b9240(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;

  result = F32(c, 0x128);
  if ( result != 0 )
    result = (*(int (__stdcall **)(int))(F32(result, 0) + 108))(result);
  v3 = F32(c, 0x16c);
  if ( v3 != 0 )
    result = W_wn_5b9ab0((intptr_t)(v3));
  v4 = F32(c, 0x18);
  if ( v4 != 0 )
    result = W_wn_5b9ab0((intptr_t)(v4));
  v5 = F32(c, 0x1c);
  if ( v5 != 0 )
    return W_wn_5b9ab0((intptr_t)(v5));
  return result;
}

/* 0x5b9410: int __thiscall zdd_object_clear_5B9410(cls_5B9410 *this) */
int32_t zd_5b9410(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = W_rp_5b9490((intptr_t)(c));
  if ( result != 0 )
  {
    memset((void *)F32(c, 0x54), 0, F32(c, 0x38) * F32(c, 0x40));
    return W_rp_5b94d0((intptr_t)(c));
  }
  return result;
}

/* 0x5b9740: int __thiscall zdd_get_attached_surface_5B9740(cls_5B9740 *this, int a2, int a3, int a4, int a5) */
int32_t zd_5b9740(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v8 = (intptr_t *)(frame_ + 0x0);   /* [4] at ebp-0x10 */

  W_rp_5b97e0((intptr_t)(c), (intptr_t)(0), (intptr_t)(0));
  v8[0] = 4;
  memset(&v8[1], 0, 12);
  if ( a5 != 0 )
    v8[0] = 2052;
  v6 = (*(int (__stdcall **)(int, _DWORD *, char *))(F32(a2, 0) + 48))(a2, v8, &GAP(c, 0x0, 44));
  if ( v6 != 0 )
  {
    zd_5b80d0(F32(c, 0xc0), ((char *)D(0x8a4b54)), ((char *)D(0x8a4b8c)), v6);
    return 0;
  }
  else
  {
    W_rp_5b98c0((intptr_t)(c), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(a4));
    return W_rp_5b9830((intptr_t)(c), (intptr_t)(0x1FFFFFF));
  }
}

/* 0x5bcff0: void __stdcall sub_5BCFF0(int a1) */
void zd_5bcff0(int32_t a1)
{


  if ( F32(a1, 16) != 0 )
  {
    g_eng.op_delete((void *)F32(a1, 8));
    F32(a1, 8) = 0;
    F32(a1, 16) = 0;
  }
}

/* 0x5bd020: int __stdcall pd_channel_init_5BD020(int a1) */
int32_t zd_5bd020(int32_t a1)
{


  F32(a1, 0) = 0;
  F32(a1, 4) = 0;
  F32(a1, 8) = 0;
  F32(a1, 16) = 0;
  F16(a1, 12) = 1000;
  return a1;
}

/* 0x5bd040: unsigned int __thiscall build_lut_small_5BD040(cls_5BD040 *this, int a2, int a3) */
int32_t zd_5bd040(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint32_t result = 0;
    intptr_t v5 = 0;
    uint32_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    uint32_t v9 = 0;
    uint32_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  if ( a3 != 0 && F16(a2, 12) == F16(a3, 12) )
  {
    result = F32(a3, 8);
    F32(a2, 8) = result;
  }
  else
  {
    if ( F32(c, 0x0) == 0 )
      goto LABEL_32;
    if ( F32(c, 0x0) == 1 )
    {
      v5 = 0;
      F32(a2, 8) = (intptr_t)g_eng.op_new(0x400u);
      F32(a2, 16) = 1;
LABEL_9:
      result = 32;
      v24 = 0;
      (*v23__f) = 0;
      (*v22__f) = 32;
      while ( 2 )
      {
        v6 = v24;
        if ( F32(c, 0x48) != 0 )
          v6 = result;
        switch ( F32(c, 0x44) )
        {
          case 1:
            v7 = (int)((v5 + v6) * F16(c, 0x40)) / 1000 + v5 * (1000 - F16(c, 0x40)) / 1000;
            if ( v5 > v7 )
              v7 = v5;
            goto LABEL_23;
          case 2:
            v7 = (int)((v5 - v6) * F16(c, 0x40)) / 1000 + v5 * (1000 - F16(c, 0x40)) / 1000;
            if ( v5 < v7 )
              v7 = v5;
            goto LABEL_23;
          case 3:
            v8 = F16(c, 0x40);
            v9 = (int)((uint64_t)(274877907LL * (int)(v8 * (v6 - v5))) >> 32) >> 6;
            v10 = (unsigned int)((uint64_t)(274877907LL * v5 * (1000 - v8)) >> 32) >> 31;
            v11 = (v9 >> 31) + v9 + ((int)((uint64_t)(274877907LL * v5 * (1000 - v8)) >> 32) >> 6);
            goto LABEL_22;
          case 4:
            v12 = F16(c, 0x40);
            v10 = (unsigned int)((uint64_t)(274877907LL * v5 * (1000 - v12)) >> 32) >> 31;
            v11 = v12
                * ((int)(v5 * (32 - v6)) / 32 + (int)(v6 * (32 * F16(a2, 12) / 1000)) / 32)
                / 1000
                + ((int)((uint64_t)(274877907LL * v5 * (1000 - v12)) >> 32) >> 6);
            goto LABEL_22;
          default:
            if ( v6 == v5 )
            {
              v7 = v5;
            }
            else
            {
              v13 = F16(c, 0x40);
              v14 = 1000 - v13;
              v15 = v6 * v13;
              v10 = (unsigned int)((uint64_t)(274877907LL * v15) >> 32) >> 31;
              v11 = v5 * v14 / 1000 + ((int)((uint64_t)(274877907LL * v15) >> 32) >> 6);
LABEL_22:
              v7 = v11 + v10;
            }
LABEL_23:
            if ( F32(c, 0x44) != 4 )
              v7 = v7 * F16(a2, 12) / 1000;
            if ( v7 >= 0 )
            {
              if ( v7 >= 32 )
                SETLB(v7, 31);
            }
            else
            {
              SETLB(v7, 0);
            }
            v16 = (*v23__f);
            (*v23__f) += 32;
            F8((F32(a2, 8) + v16 + v5), 0) = v7;
            result = (*v22__f) - 1;
            ++v24;
            if ( --(*v22__f) > 0 )
              continue;
            if ( ++v5 >= 32 )
              return result;
            goto LABEL_9;
        }
      }
    }
    result = F32(c, 0x0) - 2;
    if ( F32(c, 0x0) == 2 )
    {
LABEL_32:
      v17 = 0;
      v18 = 32;
      F32(a2, 8) = (intptr_t)g_eng.op_new(0x20u);
      F32(a2, 16) = 1;
      do
      {
        v19 = v17;
        if ( F32(c, 0x48) != 0 )
          v19 = v18;
        v20 = v19 * F16(a2, 12);
        result = (unsigned int)((uint64_t)(274877907LL * v20) >> 32) >> 31;
        v21 = v20 / 1000;
        if ( v20 / 1000 >= 0 )
        {
          if ( v21 >= 32 )
            SETLB(v21, 31);
        }
        else
        {
          SETLB(v21, 0);
        }
        ++v17;
        --v18;
        F8((F32(a2, 8) + v17 - 1), 0) = v21;
      }
      while ( v18 > 0 );
    }
  }
  return result;
}

/* 0x5bd380: int __stdcall pd_channel_mask_to_shift_5BD380(unsigned int a1) */
int32_t zd_5bd380(uint32_t a1)
{

    intptr_t i = 0;

  for ( i = 0; i < 16; ++i )
  {
    if ( ((a1 >> (15 - i)) & 1) != 0 )
      break;
  }
  return 11 - i;
}

/* 0x5bd3b0: __int16 __thiscall pd_blend_set_color_5BD3B0(cls_5BD3B0 *this, __int16 a2, __int16 a3, __int16 a4) */
int32_t zd_5bd3b0(wl_char *self, int16_t a2, int16_t a3, int16_t a4)
{
    intptr_t c = (intptr_t)self;

  F16(c, 0x10) = a2;
  F16(c, 0x24) = a3;
  F16(c, 0x38) = a4;
  return a4;
}

/* 0x5bd3d0: unsigned int __thiscall pd_blend_commit_5BD3D0(cls_5BD040 *this, _DWORD *a2) */
int32_t zd_5bd3d0(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    char * v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint32_t v7 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const v9__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x20 */
    uint32_t *const v10__f = (uint32_t *)(frame_ + 0x14);   /* ebp-0xc */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x4 */

  v3 = ((uint8_t *)(c) + 0x4);
  zd_5bcff0((int)((uint8_t *)(c) + 0x4));
  zd_5bcff0((int)&F32(c, 0x18));
  zd_5bcff0((int)&F32(c, 0x2c));
  if ( a2 != 0 )
  {
    zd_5b8ae0(a2, &(*v10__f), &(*v11__f), &(*v12__f));
  }
  else
  {
    (*v10__f) = 63488;
    (*v11__f) = 2016;
    (*v12__f) = 31;
  }
  v4 = zd_5bd380((*v10__f));
  v5 = (*v10__f);
  F32(v3, 0) = v4;
  (*v9__f) = (*v11__f);
  F32(c, 0x8) = v5;
  v6 = zd_5bd380((*v9__f));
  v7 = (*v12__f);
  F32(c, 0x1c) = (*v11__f);
  F32(c, 0x18) = v6;
  F32(c, 0x2c) = zd_5bd380(v7);
  F32(c, 0x30) = (*v12__f);
  if ( F32(c, 0x4c) == 1 )
    F32(c, 0x0) = 2;
  else
    F32(c, 0x0) = F16(c, 0x40) != 1000 || F32(c, 0x44) != 0;
  zd_5bd040(c, (int)v3, 0);
  zd_5bd040(c, (int)&F32(c, 0x18), (int)v3);
  return zd_5bd040(c, (int)&F32(c, 0x2c), (int)v3);
}

/* 0x5bd4d0: _DWORD *__thiscall pd_blend_init_5BD4D0(cls_5BD4D0 *this) */
int32_t zd_5bd4d0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x48) = 0;
  F32(c, 0x4c) = 0;
  *(_WORD *)&GAP(c, 0x4, 60) = 1000;
  F32(c, 0x44) = 0;
  zd_5bd020((int)((uint8_t *)(c) + 0x4));
  zd_5bd020((int)&GAP(c, 0x4, 20));
  zd_5bd020((int)&GAP(c, 0x4, 40));
  F32(c, 0x0) = 0;
  return &F32(c, 0x0);
}

/* 0x5bd520: void __thiscall sub_5BD520(cls_5BD520 *this) */
void zd_5bd520(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  zd_5bcff0((int)(c + 4 * (1)));
  zd_5bcff0((int)(c + 4 * (6)));
  zd_5bcff0((int)(c + 4 * (11)));
}
/* unported callees: 0x5c0907 */
