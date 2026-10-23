/* ending.c — the ENDING: the credits roll 0x5a00c0 ("Produced by" ..., the staff pages, the pictures, the music; the scene
 * loop runs it when the game is beaten) + its helpers 0x5a3670 / 0x5a3870 (the demo scene blocks) / 0x5a4320 /
 * 0x5a4440 / 0x5a45c0, and the shared screen helpers it reaches: the text box 0x410090, the input re-poll 0x49bf90,
 * 0x5646c0, the fades 0x56c640 / 0x56c900 / 0x56dbb0.  See ending.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  UNVERIFIED: no recording beats
 * the game (rh-early-game reaches the screen helpers 0x56c640 0x56c900 0x56dbb0: REPLACE-verified); REPLACE-wired,
 * compile-checked. */
#include "ending.h"

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
#include "asset_register.h"
#include "frame_load.h"
#include "gamepad.h"
#include "input_dev.h"
#include "input_dik.h"
#include "render_draw.h"
#include "render_platform.h"
#include "scene_loop.h"
#include "sound_dev.h"
#include "sprite_slot.h"
#include "title_menu.h"
#include "win_app.h"
#include "world_alloc.h"
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

static inline intptr_t W_fl_4184a0(intptr_t a0, intptr_t a1, ...) { fl_4184a0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_gp_468b10(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)gp_468b10((wl_char *)a0, (int32_t *)a1, (int32_t)a2); }
static inline intptr_t W_id_5ba2d0(intptr_t a0, ...) { return (intptr_t)id_5ba2d0((wl_char *)a0); }
static inline intptr_t W_id_5ba4c0(intptr_t a0, ...) { return (intptr_t)id_5ba4c0((wl_char *)a0); }
static inline intptr_t W_id_5ba650(intptr_t a0, ...) { return (intptr_t)id_5ba650((wl_char *)a0); }
static inline intptr_t W_input_dik_produce(intptr_t a0, intptr_t a1, ...) { input_dik_produce((input_mgr *)a0, (const void *)a1); return 0; }
static inline intptr_t W_pool_obj_delete(intptr_t a0, intptr_t a1, ...) { pool_obj_delete((uint8_t *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_rd_491820(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)rd_491820((wl_char *)a0, (int32_t)a1, (char *)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_roomloop_pools_free(intptr_t a0, ...) { roomloop_pools_free((wl_roomloop *)a0); return 0; }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_rp_5b9b70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)rp_5b9b70((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_rp_5bd550(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)rp_5bd550((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_sd_5bb3a0(intptr_t a0, ...) { return (intptr_t)sd_5bb3a0((wl_char *)a0); }
static inline intptr_t W_sd_5bbc20(intptr_t a0, ...) { return (intptr_t)sd_5bbc20((wl_char *)a0); }
static inline intptr_t W_sd_5bbc60(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbc60((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_sd_5bbc90(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbc90((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_sd_5bbcd0(intptr_t a0, intptr_t a1, ...) { sd_5bbcd0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sd_5bc0c0(intptr_t a0, ...) { sd_5bc0c0((wl_char *)a0); return 0; }
static inline intptr_t W_sd_5bcb80(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sd_5bcb80((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_slot_frames_free(intptr_t a0, intptr_t a1, ...) { slot_frames_free((ar_sprite_slot *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_tm_49a6e0(intptr_t a0, ...) { tm_49a6e0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_562a10(intptr_t a0, ...) { tm_562a10((wl_char *)a0); return 0; }
static inline intptr_t W_tm_562af0(intptr_t a0, ...) { tm_562af0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_56d710(intptr_t a0, ...) { return (intptr_t)tm_56d710((wl_char *)a0); }
static inline intptr_t W_wm_5b1030(void) { return (intptr_t)wm_5b1030(); }
static inline intptr_t W_world_free(intptr_t a0, ...) { world_free((wl_world *)a0); return 0; }
static inline intptr_t W_zd_5b9410(intptr_t a0, ...) { return (intptr_t)zd_5b9410((wl_char *)a0); }
typedef intptr_t (__attribute__((thiscall)) *rt_5b8fc0)(intptr_t, intptr_t);   /* 0x5b8fc0 thiscall: unported, retail's */
#define RC_5b8fc0 ((rt_5b8fc0)g_eng.retail(0x5b8fc0u))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteObject_t)(intptr_t);
#define API_DeleteObject ((api_DeleteObject_t)g_eng.api("DeleteObject"))
typedef uint32_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLocalTime_t)(intptr_t);
#define API_GetLocalTime ((api_GetLocalTime_t)g_eng.api("GetLocalTime"))
typedef uint32_t (__attribute__((stdcall)) *api_GetTickCount_t)(void);
#define API_GetTickCount ((api_GetTickCount_t)g_eng.api("GetTickCount"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fprintf ((intptr_t (*)())g_eng.retail(0x5bf496u))   /* retail's CRT fprintf (cdecl) */
int32_t en_410090(int32_t a1, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8);
int32_t en_49bf90(wl_char *self, int32_t a2);
void en_5646c0(intptr_t * a1, int32_t a2);
void en_56c640(wl_char *self);
int32_t en_56c900(wl_char *self, char a2);
int32_t en_56dbb0(wl_char *self, char a2);
int32_t en_5a00c0(wl_char *self);
void en_5a3670(wl_char *self);
int32_t en_5a3870(wl_char *self, int32_t * a2);
int32_t en_5a4320(wl_char *self, char a2);
int32_t en_5a4440(wl_char *self, char a2);
int32_t en_5a45c0(wl_char *self, char a2);

/* 0x410090: unsigned int __stdcall sub_410090(int a1, int a2, int a3, int *a4, _DWORD *a5, _DWORD *a6, _DWORD *a7, _DWORD *a8) */
int32_t en_410090(int32_t a1, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8)
{

    intptr_t v8 = 0;
    uint32_t result = 0;

  *a5 = 0;
  *a6 = 0;
  *a7 = 0;
  *a8 = 0;
  if ( a1 != 0 )
  {
    v8 = 900 * a2 / 1000;
    if ( *a4 == -1 && a3 > -1000 * v8 / 1000 )
    {
      *a4 = 0;
      *a7 = 1;
    }
    if ( *a4 == 1 && a3 < 1000 * v8 / 1000 )
    {
      *a4 = 0;
      *a8 = 1;
    }
    if ( *a4 < 0 )
      goto LABEL_12;
    result = (unsigned int)((0xFFFFFFBFFFFFFFC8uLL * a2) >> 32) >> 31;
    if ( a3 <= -1000 * a2 / 1000 )
    {
      *a4 = -1;
      *a5 = 1;
    }
    if ( *a4 <= 0 )
    {
LABEL_12:
      result = (unsigned int)((uint64_t)(274877907000LL * a2) >> 32) >> 31;
      if ( a3 >= 1000 * a2 / 1000 )
      {
        *a4 = 1;
        *a6 = 1;
      }
      else
      {
        return -56 * a2;
      }
    }
  }
  else
  {
    result = a3;
    if ( a3 > -500 )
    {
      if ( a3 < 500 )
      {
        *a7 = 1;
        *a8 = 1;
      }
      else
      {
        *a6 = 1;
      }
    }
    else
    {
      *a5 = 1;
    }
  }
  return result;
}

/* 0x49bf90: BOOL __thiscall sub_49BF90(input_dev *this, int a2) */
int32_t en_49bf90(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v4 = 0;
    int32_t * v5 = 0;
    intptr_t v6 = 0;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const i__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x14 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x10 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xc */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x4 */

  result = W_id_5ba2d0((intptr_t)(c));
  if ( result )
  {
    v4 = 0;
    for ( (*i__f) = 0; (*i__f) < F32(c, 4 * 4); ++(*i__f) )
    {
      v5 = (int *)(v4 + F32(c, 4 * 5));
      v6 = *v5;
      v7 = *v5 - 8;
      if ( v7 <= 0x18 )
      {
        switch ( ((uint8_t *)D(0x49c150))[v7] )
        {
          case 0:
            v9 = (intptr_t)((uint8_t *)c + 104);
            goto LABEL_18;
          case 1:
            v9 = c + 0x6c;
            goto LABEL_18;
          case 2:
            v9 = (intptr_t)((uint8_t *)c + 112);
            goto LABEL_18;
          case 3:
            v9 = (intptr_t)((uint8_t *)c + 116);
LABEL_18:
            en_410090(1, a2, v5[1], (int *)v9, &(*v11__f), &(*v13__f), &(*v12__f), &(*v14__f));
            if ( (*v11__f) != 0 )
            {
              F32((v4 + F32(c, 4 * 5)), 0) = v6 + 248;
              F32(v4 + F32(c, 4 * 5), 4) = 129;
            }
            if ( (*v12__f) != 0 )
            {
              F32((v4 + F32(c, 4 * 5)), 0) = v6 + 248;
              F32(v4 + F32(c, 4 * 5), 4) = 0;
            }
            if ( (*v13__f) != 0 )
            {
              F32((v4 + F32(c, 4 * 5)), 0) = v6 + 148;
              F32(v4 + F32(c, 4 * 5), 4) = 129;
            }
            if ( (*v14__f) != 0 )
            {
              F32((v4 + F32(c, 4 * 5)), 0) = v6 + 148;
              F32(v4 + F32(c, 4 * 5), 4) = 0;
            }
            break;
          case 4:
            v8 = v5[1];
            if ( v8 > 18000 )
            {
              if ( v8 == 27000 )
              {
                *v5 = 554;
                F32(v4 + F32(c, 4 * 5), 4) = 129;
              }
            }
            else if ( v8 == 18000 )
            {
              *v5 = 552;
              F32(v4 + F32(c, 4 * 5), 4) = 129;
            }
            else if ( v8 != 0 )
            {
              if ( v8 == 9000 )
              {
                *v5 = 550;
                F32(v4 + F32(c, 4 * 5), 4) = 129;
              }
            }
            else
            {
              *v5 = 548;
              F32(v4 + F32(c, 4 * 5), 4) = 129;
            }
            break;
          case 5:
            break;
        }
      }
      v4 += 16;
    }
    return true;
  }
  return result;
}

/* 0x5646c0: void __stdcall sub_5646C0(cls_5BB870 **a1, int a2) */
void en_5646c0(intptr_t * a1, int32_t a2)
{

    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    uint32_t TickCount = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */

  v2 = *a1;
  if ( *a1 != 0 )
  {
    if ( a1[4] == 1 )
    {
      TickCount = API_GetTickCount();
      v6 = (int)a1[1];
      a1[3] = TickCount;
      (*v8__f) = v6;
      v7 = *a1;
      a1[4] = 2;
      g_eng.dsb_volume_5bb870(v7, (*v8__f));
      g_eng.dsb_play_5bb8c0(*a1);
    }
    else if ( a1[4] == 2 )
    {
      if ( a2 - (int)a1[3] > (unsigned int)a1[2] )
      {
        a1[4] = 3;
        a1[5] = 200;
      }
    }
    else if ( a1[4] == 3 )
    {
      v3 = (char *)a1[1] - (char *)a1[5];
      if ( v3 < -10000 )
        v3 = -10000;
      a1[1] = v3;
      g_eng.dsb_volume_5bb870(v2, v3);
      if ( (int)a1[1] <= -10000 )
      {
        v4 = *a1;
        if ( *a1 != 0 )
        {
          W_sd_5bb3a0((intptr_t)(*a1));
          g_eng.op_delete((void *)v4);
        }
        memset(a1, 0, 0x18u);
      }
    }
  }
}

/* 0x56c640: void __thiscall sub_56C640(cls_56C640 *this) */
void en_56c640(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    void ** v7 = 0;
    intptr_t * v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1c */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x18 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x14 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x10 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */

  v1 = c;
  v2 = 0;
  for ( (*i__f) = 0; (uint16_t)v2 < *(_WORD *)((uint8_t *)(v1 + (1) * 0x4) + 0x0); (*i__f) = v2 )
  {
    v3 = 4 * (uint16_t)v2;
    (*v23__f) = v3;
    v4 = F32((v3 + F32(v1, 0)), 0);
    (*v22__f) = v4;
    if ( v4 != 0 )
    {
      if ( F32(v4, 72) != 0 )
      {
        v5 = 0;
        (*v19__f) = 0;
        if ( F16(v4, 76) != 0 )
        {
          do
          {
            v6 = 4 * (uint16_t)v5;
            (*v21__f) = v6;
            v7 = *(void ***)(v6 + F32(v4, 72));
            if ( v7 != 0 )
            {
              v8 = (intptr_t *)v7[92];
              if ( v8 != 0 && *v8 != 0 )
              {
                en_56dbb0(*v8, 1);
                *(_DWORD *)v7[92] = 0;
              }
              if ( v7[92] != 0 )
              {
                g_eng.op_delete((void *)v7[92]);
                v7[92] = 0;
              }
              if ( v7[89] != 0 )
              {
                g_eng.op_delete((void *)v7[89]);
                v7[89] = 0;
              }
              if ( v7[94] != 0 )
              {
                g_eng.op_delete((void *)v7[94]);
                v7[94] = 0;
              }
              if ( v7[95] != 0 )
              {
                v9 = (int *)v7[93];
                (*v18__f) = 0;
                if ( v9[1] > 0 )
                {
                  v10 = 0;
                  do
                  {
                    v11 = 16 * v10;
                    if ( F32((char *)v7[95] + v11, 12) != 0 )
                    {
                      (*v17__f) = 0;
                      if ( v9[2] > 0 )
                      {
                        v12 = 0;
                        do
                        {
                          v13 = 24 * v12;
                          v14 = F32(F32((char *)v7[95] + v11, 12), 24 * v12);
                          if ( v14 != 0 )
                          {
                            en_56c900(v14, 1);
                            F32((F32((char *)v7[95] + v11, 12) + v13), 0) = 0;
                          }
                          v15 = F32((char *)v7[95] + v11, 12);
                          if ( F32(v15 + v13, 4) != 0 )
                          {
                            g_eng.op_delete((void *)F32(v15, v13 + 4));
                            F32(F32(((char *)v7[95] + v11 + 12), 0) + v13, 4) = 0;
                          }
                          v16 = F32((char *)v7[95] + v11, 12);
                          if ( F32(v16 + v13, 8) != 0 )
                          {
                            g_eng.op_delete((void *)F32(v16, v13 + 8));
                            F32(F32(((char *)v7[95] + v11 + 12), 0) + v13, 8) = 0;
                          }
                          v12 = (uint16_t)++(*v17__f);
                        }
                        while ( (uint16_t)(*v17__f) < *((int *)v7[93] + 2) );
                        v6 = (*v21__f);
                      }
                    }
                    if ( F32((char *)v7[95] + v11, 12) != 0 )
                    {
                      g_eng.op_delete((void *)F32((char *)v7[95], v11 + 12));
                      F32((char *)v7[95] + v11, 12) = 0;
                    }
                    v9 = (int *)v7[93];
                    v10 = (uint16_t)++(*v18__f);
                  }
                  while ( (uint16_t)(*v18__f) < v9[1] );
                  v4 = (*v22__f);
                }
                if ( v7[95] != 0 )
                {
                  g_eng.op_delete((void *)v7[95]);
                  v7[95] = 0;
                }
              }
              if ( v7[93] != 0 )
              {
                g_eng.op_delete((void *)v7[93]);
                v7[93] = 0;
              }
              g_eng.op_delete((void *)v7);
              v5 = (*v19__f);
              F32((v6 + F32(v4, 72)), 0) = 0;
            }
            (*v19__f) = ++v5;
          }
          while ( (uint16_t)v5 < F16(v4, 76) );
          v3 = (*v23__f);
          v1 = c;
        }
        if ( F32(v4, 72) != 0 )
        {
          g_eng.op_delete((void *)F32(v4, 72));
          F32(v4, 72) = 0;
        }
      }
      g_eng.op_delete((void *)(void *)v4);
      v2 = (*i__f);
      F32((v3 + F32(v1, 0)), 0) = 0;
    }
    ++v2;
  }
  if ( F32(v1, 0) != 0 )
  {
    g_eng.op_delete((void *)F32(v1, 0));
    F32(v1, 0) = 0;
  }
}

/* 0x56c900: void **__thiscall sub_56C900(cls_56C900 *this, char a2) */
int32_t en_56c900(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;

  g_eng.op_delete((void *)(void *)F32(c, 0x0));
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return (void **)c;
}

/* 0x56dbb0: void *__thiscall sub_56DBB0(cls_56DBB0 *this, char a2) */
int32_t en_56dbb0(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    void ** v3 = 0;

  v3 = *((void ***)c + 3);
  if ( v3 != 0 )
  {
    g_eng.op_delete((void *)*v3);
    g_eng.op_delete((void *)v3);
    F32(c, 4 * 3) = 0;
  }
  if ( F32(c, 0x0) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  if ( F32(c, 4 * 2) != 0 )
  {
    g_eng.op_delete((void *)*((void **)c + 2));
    F32(c, 4 * 2) = 0;
  }
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return c;
}

/* 0x5a00c0: int __thiscall sub_5A00C0(cls_41BBE0 *this) */
int32_t en_5a00c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    char * name = 0;
    intptr_t * v2 = 0;
    uint16_t v3 = 0;
    intptr_t v4 = 0;
    uint32_t v5 = 0;
    uint32_t v6 = 0;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    uint16_t v9 = 0;
    uint32_t i = 0;
    intptr_t * v11 = 0;
    intptr_t j = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    uint8_t * v18 = 0;
    char * v19 = 0;
    uint32_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
    intptr_t m = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    int32_t * v26 = 0;
    int32_t * v27 = 0;
    char * v28 = 0;
    FILE * v29 = 0;
    char * v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    uint32_t v33 = 0;
    int v34 = 0;
    intptr_t v35 = 0;
    int32_t * v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    int32_t * v43 = 0;
    int32_t * v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    int32_t * v47 = 0;
    int32_t ** v48 = 0;
    intptr_t n = 0;
    int32_t * v50 = 0;
    int32_t * v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
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
    uint32_t v72 = 0;
    char v73 = 0;
    intptr_t v74 = 0;
    intptr_t v75 = 0;
    uint16_t v76 = 0;
    int32_t ** v77 = 0;
    intptr_t v78 = 0;
    int v79 = 0;
    intptr_t v80 = 0;
    uint16_t v81 = 0;
    intptr_t v82 = 0;
    intptr_t v83 = 0;
    int32_t * v84 = 0;
    char * v85 = 0;
    FILE * v86 = 0;
    intptr_t v87 = 0;
    uint16_t ii = 0;
    intptr_t v89 = 0;
    void * v90 = 0;
    intptr_t v91 = 0;
    intptr_t v92 = 0;
    void ** v93 = 0;
    uint32_t kk = 0;
    void * v95 = 0;
    uint16_t * v96 = 0;
    intptr_t mm = 0;
    intptr_t v98 = 0;
    void * v99 = 0;
    intptr_t v100 = 0;
    uint16_t nn = 0;
    intptr_t v102 = 0;
    intptr_t v103 = 0;
    intptr_t v104 = 0;
    intptr_t v105 = 0;
    intptr_t v106 = 0;
    uint16_t i2 = 0;
    intptr_t v108 = 0;
    void * v109 = 0;
    void ** v110 = 0;
    void ** v111 = 0;
    intptr_t i3 = 0;
    _Alignas(8) uint8_t frame_[0x8b60 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v159 = (intptr_t *)(frame_ + 0x98);   /* [100] at ebp-0x8ac8 */
    uint8_t *v160 = (uint8_t *)(frame_ + 0x228);   /* [44] at ebp-0x8938 */
    uint16_t *v161 = (uint16_t *)(frame_ + 0x254);   /* [40] at ebp-0x890c */
    char *v162 = (char *)(frame_ + 0x2a4);   /* [256] at ebp-0x88bc */
    uint8_t *v165 = (uint8_t *)(frame_ + 0x3a9);   /* [255] at ebp-0x87b7 */
    uint8_t *v177 = (uint8_t *)(frame_ + 0x4cd);   /* [287] at ebp-0x8693 */
    uint8_t *v180 = (uint8_t *)(frame_ + 0x5f1);   /* [255] at ebp-0x856f */
    uint8_t *v192 = (uint8_t *)(frame_ + 0x715);   /* [287] at ebp-0x844b */
    char *v194 = (char *)(frame_ + 0x838);   /* [256] at ebp-0x8328 */
    uint8_t *v205 = (uint8_t *)(frame_ + 0x95d);   /* [287] at ebp-0x8203 */
    uint8_t *v208 = (uint8_t *)(frame_ + 0xa81);   /* [287] at ebp-0x80df */
    uint8_t *v211 = (uint8_t *)(frame_ + 0xba5);   /* [287] at ebp-0x7fbb */
    uint8_t *v214 = (uint8_t *)(frame_ + 0xcc9);   /* [287] at ebp-0x7e97 */
    uint8_t *v217 = (uint8_t *)(frame_ + 0xded);   /* [287] at ebp-0x7d73 */
    char *v219 = (char *)(frame_ + 0xf10);   /* [288] at ebp-0x7c50 */
    char *v221 = (char *)(frame_ + 0x1034);   /* [256] at ebp-0x7b2c */
    intptr_t *v223 = (intptr_t *)(frame_ + 0x1138);   /* [8] at ebp-0x7a28 */
    uint8_t *v225 = (uint8_t *)(frame_ + 0x1159);   /* [287] at ebp-0x7a07 */
    uint8_t *v228 = (uint8_t *)(frame_ + 0x127d);   /* [287] at ebp-0x78e3 */
    uint8_t *v231 = (uint8_t *)(frame_ + 0x13a1);   /* [287] at ebp-0x77bf */
    uint8_t *v234 = (uint8_t *)(frame_ + 0x14c5);   /* [287] at ebp-0x769b */
    char *v236 = (char *)(frame_ + 0x15e8);   /* [288] at ebp-0x7578 */
    char *v238 = (char *)(frame_ + 0x170c);   /* [256] at ebp-0x7454 */
    intptr_t *v240 = (intptr_t *)(frame_ + 0x1810);   /* [8] at ebp-0x7350 */
    char *v241 = (char *)(frame_ + 0x1830);   /* [254] at ebp-0x7330 */
    intptr_t *v244 = (intptr_t *)(frame_ + 0x1934);   /* [8] at ebp-0x722c */
    uint8_t *v246 = (uint8_t *)(frame_ + 0x1955);   /* [287] at ebp-0x720b */
    uint8_t *v249 = (uint8_t *)(frame_ + 0x1a79);   /* [287] at ebp-0x70e7 */
    uint8_t *v252 = (uint8_t *)(frame_ + 0x1b9d);   /* [287] at ebp-0x6fc3 */
    uint8_t *v255 = (uint8_t *)(frame_ + 0x1cc1);   /* [287] at ebp-0x6e9f */
    uint8_t *v258 = (uint8_t *)(frame_ + 0x1de5);   /* [287] at ebp-0x6d7b */
    char *v260 = (char *)(frame_ + 0x1f08);   /* [288] at ebp-0x6c58 */
    char *v262 = (char *)(frame_ + 0x202c);   /* [256] at ebp-0x6b34 */
    intptr_t *v264 = (intptr_t *)(frame_ + 0x2130);   /* [8] at ebp-0x6a30 */
    uint8_t *v266 = (uint8_t *)(frame_ + 0x2151);   /* [287] at ebp-0x6a0f */
    uint8_t *v269 = (uint8_t *)(frame_ + 0x2275);   /* [287] at ebp-0x68eb */
    uint8_t *v272 = (uint8_t *)(frame_ + 0x2399);   /* [287] at ebp-0x67c7 */
    uint8_t *v275 = (uint8_t *)(frame_ + 0x24bd);   /* [287] at ebp-0x66a3 */
    char *v277 = (char *)(frame_ + 0x25e0);   /* [253] at ebp-0x6580 */
    intptr_t *v280 = (intptr_t *)(frame_ + 0x26e0);   /* [9] at ebp-0x6480 */
    char *v281 = (char *)(frame_ + 0x2704);   /* [253] at ebp-0x645c */
    intptr_t *v285 = (intptr_t *)(frame_ + 0x2808);   /* [8] at ebp-0x6358 */
    uint8_t *v287 = (uint8_t *)(frame_ + 0x2829);   /* [287] at ebp-0x6337 */
    uint8_t *v290 = (uint8_t *)(frame_ + 0x294d);   /* [287] at ebp-0x6213 */
    uint8_t *v293 = (uint8_t *)(frame_ + 0x2a71);   /* [287] at ebp-0x60ef */
    uint8_t *v296 = (uint8_t *)(frame_ + 0x2b95);   /* [287] at ebp-0x5fcb */
    char *v298 = (char *)(frame_ + 0x2cb8);   /* [253] at ebp-0x5ea8 */
    intptr_t *v301 = (intptr_t *)(frame_ + 0x2db8);   /* [9] at ebp-0x5da8 */
    char *v302 = (char *)(frame_ + 0x2ddc);   /* [256] at ebp-0x5d84 */
    intptr_t *v304 = (intptr_t *)(frame_ + 0x2ee0);   /* [8] at ebp-0x5c80 */
    uint8_t *v306 = (uint8_t *)(frame_ + 0x2f01);   /* [287] at ebp-0x5c5f */
    uint8_t *v309 = (uint8_t *)(frame_ + 0x3025);   /* [287] at ebp-0x5b3b */
    uint8_t *v312 = (uint8_t *)(frame_ + 0x3149);   /* [287] at ebp-0x5a17 */
    uint8_t *v315 = (uint8_t *)(frame_ + 0x326d);   /* [287] at ebp-0x58f3 */
    char *v317 = (char *)(frame_ + 0x3390);   /* [288] at ebp-0x57d0 */
    char *v319 = (char *)(frame_ + 0x34b4);   /* [253] at ebp-0x56ac */
    intptr_t *v323 = (intptr_t *)(frame_ + 0x35b8);   /* [8] at ebp-0x55a8 */
    uint8_t *v325 = (uint8_t *)(frame_ + 0x35d9);   /* [287] at ebp-0x5587 */
    char *v327 = (char *)(frame_ + 0x36fc);   /* [256] at ebp-0x5464 */
    intptr_t *v330 = (intptr_t *)(frame_ + 0x3804);   /* [7] at ebp-0x535c */
    char *v331 = (char *)(frame_ + 0x3820);   /* [254] at ebp-0x5340 */
    intptr_t *v335 = (intptr_t *)(frame_ + 0x3928);   /* [7] at ebp-0x5238 */
    char *v336 = (char *)(frame_ + 0x3944);   /* [256] at ebp-0x521c */
    intptr_t *v339 = (intptr_t *)(frame_ + 0x3a4c);   /* [7] at ebp-0x5114 */
    uint8_t *v341 = (uint8_t *)(frame_ + 0x3a69);   /* [287] at ebp-0x50f7 */
    char *v343 = (char *)(frame_ + 0x3b8c);   /* [256] at ebp-0x4fd4 */
    intptr_t *v346 = (intptr_t *)(frame_ + 0x3c94);   /* [7] at ebp-0x4ecc */
    char *v347 = (char *)(frame_ + 0x3cb0);   /* [256] at ebp-0x4eb0 */
    intptr_t *v350 = (intptr_t *)(frame_ + 0x3db8);   /* [7] at ebp-0x4da8 */
    uint8_t *v352 = (uint8_t *)(frame_ + 0x3dd5);   /* [287] at ebp-0x4d8b */
    uint8_t *v355 = (uint8_t *)(frame_ + 0x3ef9);   /* [287] at ebp-0x4c67 */
    uint8_t *v358 = (uint8_t *)(frame_ + 0x401d);   /* [287] at ebp-0x4b43 */
    uint8_t *v361 = (uint8_t *)(frame_ + 0x4141);   /* [287] at ebp-0x4a1f */
    char *v363 = (char *)(frame_ + 0x4264);   /* [288] at ebp-0x48fc */
    char *v365 = (char *)(frame_ + 0x4388);   /* [254] at ebp-0x47d8 */
    intptr_t *v368 = (intptr_t *)(frame_ + 0x448c);   /* [8] at ebp-0x46d4 */
    char *v369 = (char *)(frame_ + 0x44ac);   /* [254] at ebp-0x46b4 */
    intptr_t *v373 = (intptr_t *)(frame_ + 0x45b4);   /* [7] at ebp-0x45ac */
    uint8_t *v375 = (uint8_t *)(frame_ + 0x45d1);   /* [287] at ebp-0x458f */
    uint8_t *v378 = (uint8_t *)(frame_ + 0x46f5);   /* [287] at ebp-0x446b */
    uint8_t *v381 = (uint8_t *)(frame_ + 0x4819);   /* [287] at ebp-0x4347 */
    uint8_t *v384 = (uint8_t *)(frame_ + 0x493d);   /* [287] at ebp-0x4223 */
    char *v386 = (char *)(frame_ + 0x4a60);   /* [253] at ebp-0x4100 */
    intptr_t *v389 = (intptr_t *)(frame_ + 0x4b60);   /* [9] at ebp-0x4000 */
    uint8_t *v391 = (uint8_t *)(frame_ + 0x4b85);   /* [287] at ebp-0x3fdb */
    char *v393 = (char *)(frame_ + 0x4ca8);   /* [254] at ebp-0x3eb8 */
    intptr_t *v396 = (intptr_t *)(frame_ + 0x4dac);   /* [8] at ebp-0x3db4 */
    char *v397 = (char *)(frame_ + 0x4dcc);   /* [253] at ebp-0x3d94 */
    intptr_t *v401 = (intptr_t *)(frame_ + 0x4ed0);   /* [8] at ebp-0x3c90 */
    uint8_t *v403 = (uint8_t *)(frame_ + 0x4ef1);   /* [287] at ebp-0x3c6f */
    char *v405 = (char *)(frame_ + 0x5014);   /* [253] at ebp-0x3b4c */
    intptr_t *v409 = (intptr_t *)(frame_ + 0x5118);   /* [8] at ebp-0x3a48 */
    char *v410 = (char *)(frame_ + 0x5138);   /* [254] at ebp-0x3a28 */
    intptr_t *v413 = (intptr_t *)(frame_ + 0x523c);   /* [8] at ebp-0x3924 */
    uint8_t *v415 = (uint8_t *)(frame_ + 0x525d);   /* [287] at ebp-0x3903 */
    char *v417 = (char *)(frame_ + 0x5380);   /* [256] at ebp-0x37e0 */
    intptr_t *v419 = (intptr_t *)(frame_ + 0x5484);   /* [8] at ebp-0x36dc */
    char *v420 = (char *)(frame_ + 0x54a4);   /* [256] at ebp-0x36bc */
    intptr_t *v422 = (intptr_t *)(frame_ + 0x55a8);   /* [8] at ebp-0x35b8 */
    uint8_t *v424 = (uint8_t *)(frame_ + 0x55c9);   /* [287] at ebp-0x3597 */
    char *v426 = (char *)(frame_ + 0x56ec);   /* [256] at ebp-0x3474 */
    intptr_t *v428 = (intptr_t *)(frame_ + 0x57f0);   /* [8] at ebp-0x3370 */
    char *v429 = (char *)(frame_ + 0x5810);   /* [256] at ebp-0x3350 */
    intptr_t *v431 = (intptr_t *)(frame_ + 0x5914);   /* [8] at ebp-0x324c */
    uint8_t *v433 = (uint8_t *)(frame_ + 0x5935);   /* [287] at ebp-0x322b */
    uint8_t *v436 = (uint8_t *)(frame_ + 0x5a59);   /* [287] at ebp-0x3107 */
    uint8_t *v439 = (uint8_t *)(frame_ + 0x5b7d);   /* [287] at ebp-0x2fe3 */
    uint8_t *v442 = (uint8_t *)(frame_ + 0x5ca1);   /* [287] at ebp-0x2ebf */
    char *v444 = (char *)(frame_ + 0x5dc4);   /* [254] at ebp-0x2d9c */
    intptr_t *v446 = (intptr_t *)(frame_ + 0x5ec4);   /* [9] at ebp-0x2c9c */
    char *v447 = (char *)(frame_ + 0x5ee8);   /* [253] at ebp-0x2c78 */
    intptr_t *v451 = (intptr_t *)(frame_ + 0x5fec);   /* [8] at ebp-0x2b74 */
    uint8_t *v453 = (uint8_t *)(frame_ + 0x600d);   /* [287] at ebp-0x2b53 */
    uint8_t *v456 = (uint8_t *)(frame_ + 0x6131);   /* [287] at ebp-0x2a2f */
    uint8_t *v459 = (uint8_t *)(frame_ + 0x6255);   /* [287] at ebp-0x290b */
    uint8_t *v462 = (uint8_t *)(frame_ + 0x6379);   /* [287] at ebp-0x27e7 */
    uint8_t *v464 = (uint8_t *)(frame_ + 0x649c);   /* [288] at ebp-0x26c4 */
    char *v466 = (char *)(frame_ + 0x65c0);   /* [256] at ebp-0x25a0 */
    intptr_t *v468 = (intptr_t *)(frame_ + 0x66c4);   /* [8] at ebp-0x249c */
    uint8_t *v470 = (uint8_t *)(frame_ + 0x66e5);   /* [287] at ebp-0x247b */
    uint8_t *v473 = (uint8_t *)(frame_ + 0x6809);   /* [287] at ebp-0x2357 */
    uint8_t *v476 = (uint8_t *)(frame_ + 0x692d);   /* [287] at ebp-0x2233 */
    uint8_t *v479 = (uint8_t *)(frame_ + 0x6a51);   /* [287] at ebp-0x210f */
    char *v481 = (char *)(frame_ + 0x6b74);   /* [254] at ebp-0x1fec */
    intptr_t *v483 = (intptr_t *)(frame_ + 0x6c74);   /* [9] at ebp-0x1eec */
    char *v484 = (char *)(frame_ + 0x6c98);   /* [254] at ebp-0x1ec8 */
    intptr_t *v487 = (intptr_t *)(frame_ + 0x6d9c);   /* [8] at ebp-0x1dc4 */
    char *v488 = (char *)(frame_ + 0x6dbc);   /* [256] at ebp-0x1da4 */
    intptr_t *v490 = (intptr_t *)(frame_ + 0x6ec0);   /* [8] at ebp-0x1ca0 */
    char *v491 = (char *)(frame_ + 0x6ee0);   /* [256] at ebp-0x1c80 */
    intptr_t *v493 = (intptr_t *)(frame_ + 0x6fe4);   /* [8] at ebp-0x1b7c */
    uint8_t *v495 = (uint8_t *)(frame_ + 0x7005);   /* [287] at ebp-0x1b5b */
    uint8_t *v498 = (uint8_t *)(frame_ + 0x7129);   /* [287] at ebp-0x1a37 */
    uint8_t *v501 = (uint8_t *)(frame_ + 0x724d);   /* [287] at ebp-0x1913 */
    uint8_t *v504 = (uint8_t *)(frame_ + 0x7371);   /* [287] at ebp-0x17ef */
    char *v506 = (char *)(frame_ + 0x7494);   /* [253] at ebp-0x16cc */
    intptr_t *v509 = (intptr_t *)(frame_ + 0x7594);   /* [9] at ebp-0x15cc */
    char *v510 = (char *)(frame_ + 0x75b8);   /* [253] at ebp-0x15a8 */
    intptr_t *v514 = (intptr_t *)(frame_ + 0x76bc);   /* [8] at ebp-0x14a4 */
    char *v515 = (char *)(frame_ + 0x76dc);   /* [256] at ebp-0x1484 */
    intptr_t *v517 = (intptr_t *)(frame_ + 0x77e0);   /* [8] at ebp-0x1380 */
    uint8_t *v519 = (uint8_t *)(frame_ + 0x7801);   /* [287] at ebp-0x135f */
    uint8_t *v522 = (uint8_t *)(frame_ + 0x7925);   /* [287] at ebp-0x123b */
    uint8_t *v525 = (uint8_t *)(frame_ + 0x7a49);   /* [287] at ebp-0x1117 */
    uint8_t *v528 = (uint8_t *)(frame_ + 0x7b6d);   /* [287] at ebp-0xff3 */
    char *v530 = (char *)(frame_ + 0x7c90);   /* [254] at ebp-0xed0 */
    intptr_t *v532 = (intptr_t *)(frame_ + 0x7d90);   /* [9] at ebp-0xdd0 */
    char *v533 = (char *)(frame_ + 0x7db4);   /* [256] at ebp-0xdac */
    intptr_t *v535 = (intptr_t *)(frame_ + 0x7eb8);   /* [8] at ebp-0xca8 */
    uint8_t *v537 = (uint8_t *)(frame_ + 0x7ed9);   /* [287] at ebp-0xc87 */
    uint8_t *v540 = (uint8_t *)(frame_ + 0x7ffd);   /* [287] at ebp-0xb63 */
    uint8_t *v543 = (uint8_t *)(frame_ + 0x8121);   /* [287] at ebp-0xa3f */
    uint8_t *v546 = (uint8_t *)(frame_ + 0x8245);   /* [287] at ebp-0x91b */
    uint8_t *v548 = (uint8_t *)(frame_ + 0x8368);   /* [288] at ebp-0x7f8 */
    char *v550 = (char *)(frame_ + 0x848c);   /* [256] at ebp-0x6d4 */
    intptr_t *v552 = (intptr_t *)(frame_ + 0x8590);   /* [8] at ebp-0x5d0 */
    uint8_t *v554 = (uint8_t *)(frame_ + 0x85b1);   /* [287] at ebp-0x5af */
    uint8_t *v557 = (uint8_t *)(frame_ + 0x86d5);   /* [287] at ebp-0x48b */
    uint8_t *v560 = (uint8_t *)(frame_ + 0x87f9);   /* [287] at ebp-0x367 */
    uint8_t *v563 = (uint8_t *)(frame_ + 0x891d);   /* [579] at ebp-0x243 */
    intptr_t *const v119__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8b60 */
    intptr_t *const v118__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8b60 */
    intptr_t *const v117__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8b60 */
    intptr_t *const v116__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8b60 */
    intptr_t *const v115__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8b60 */
    intptr_t *const k__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8b60 */
    intptr_t *const v123__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8b5c */
    intptr_t *const v122__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8b5c */
    intptr_t *const v120__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8b5c */
    intptr_t *const jj__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8b5c */
    intptr_t *const v125__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8b58 */
    intptr_t *const v124__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8b58 */
    intptr_t *const v128__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8b54 */
    intptr_t *const v127__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8b54 */
    intptr_t *const v126__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8b54 */
    intptr_t *const v129__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8b50 */
    intptr_t *const v131__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8b4c */
    intptr_t *const v130__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8b4c */
    intptr_t *const v133__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8b48 */
    intptr_t *const v132__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8b48 */
    intptr_t *const v134__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x8b44 */
    int32_t * *const v136__f = (int32_t * *)(frame_ + 0x24);   /* ebp-0x8b3c */
    uint16_t * *const v138__f = (uint16_t * *)(frame_ + 0x28);   /* ebp-0x8b38 */
    intptr_t *const v137__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8b38 */
    intptr_t *const v140__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x8b34 */
    intptr_t *const v139__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x8b34 */
    intptr_t *const v141__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8b30 */
    intptr_t *const i1__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8b30 */
    SYSTEMTIME *const v143__f = (SYSTEMTIME *)(frame_ + 0x34);   /* ebp-0x8b2c */
    intptr_t *const v144__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x8b1c */
    intptr_t *const v145__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x8b18 */
    intptr_t *const v146__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x8b14 */
    intptr_t *const v147__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x8b10 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x54);   /* ebp-0x8b0c */
    uint32_t *const v149__f = (uint32_t *)(frame_ + 0x64);   /* ebp-0x8afc */
    intptr_t *const v150__f = (intptr_t *)(frame_ + 0x68);   /* ebp-0x8af8 */
    uint32_t *const v151__f = (uint32_t *)(frame_ + 0x6c);   /* ebp-0x8af4 */
    intptr_t *const v152__f = (intptr_t *)(frame_ + 0x70);   /* ebp-0x8af0 */
    uint32_t *const TickCount__f = (uint32_t *)(frame_ + 0x78);   /* ebp-0x8ae8 */
    intptr_t *const v154__f = (intptr_t *)(frame_ + 0x7c);   /* ebp-0x8ae4 */
    uint32_t *const v155__f = (uint32_t *)(frame_ + 0x84);   /* ebp-0x8adc */
    intptr_t *const v156__f = (intptr_t *)(frame_ + 0x8c);   /* ebp-0x8ad4 */
    intptr_t *const v157__f = (intptr_t *)(frame_ + 0x90);   /* ebp-0x8ad0 */
    intptr_t *const v158__f = (intptr_t *)(frame_ + 0x94);   /* ebp-0x8acc */
    intptr_t *const v163__f = (intptr_t *)(frame_ + 0x3a4);   /* ebp-0x87bc */
    char *const v164__f = (char *)(frame_ + 0x3a8);   /* ebp-0x87b8 */
    intptr_t *const v166__f = (intptr_t *)(frame_ + 0x4a8);   /* ebp-0x86b8 */
    intptr_t *const v167__f = (intptr_t *)(frame_ + 0x4ac);   /* ebp-0x86b4 */
    intptr_t *const v168__f = (intptr_t *)(frame_ + 0x4b0);   /* ebp-0x86b0 */
    int16_t *const v169__f = (int16_t *)(frame_ + 0x4b4);   /* ebp-0x86ac */
    int16_t *const v170__f = (int16_t *)(frame_ + 0x4b6);   /* ebp-0x86aa */
    intptr_t *const v171__f = (intptr_t *)(frame_ + 0x4b8);   /* ebp-0x86a8 */
    intptr_t *const v172__f = (intptr_t *)(frame_ + 0x4bc);   /* ebp-0x86a4 */
    intptr_t *const v173__f = (intptr_t *)(frame_ + 0x4c0);   /* ebp-0x86a0 */
    intptr_t *const v174__f = (intptr_t *)(frame_ + 0x4c4);   /* ebp-0x869c */
    intptr_t *const v175__f = (intptr_t *)(frame_ + 0x4c8);   /* ebp-0x8698 */
    char *const v176__f = (char *)(frame_ + 0x4cc);   /* ebp-0x8694 */
    intptr_t *const v178__f = (intptr_t *)(frame_ + 0x5ec);   /* ebp-0x8574 */
    char *const v179__f = (char *)(frame_ + 0x5f0);   /* ebp-0x8570 */
    intptr_t *const v181__f = (intptr_t *)(frame_ + 0x6f0);   /* ebp-0x8470 */
    intptr_t *const v182__f = (intptr_t *)(frame_ + 0x6f4);   /* ebp-0x846c */
    intptr_t *const v183__f = (intptr_t *)(frame_ + 0x6f8);   /* ebp-0x8468 */
    int16_t *const v184__f = (int16_t *)(frame_ + 0x6fc);   /* ebp-0x8464 */
    int16_t *const v185__f = (int16_t *)(frame_ + 0x6fe);   /* ebp-0x8462 */
    intptr_t *const v186__f = (intptr_t *)(frame_ + 0x700);   /* ebp-0x8460 */
    intptr_t *const v187__f = (intptr_t *)(frame_ + 0x704);   /* ebp-0x845c */
    intptr_t *const v188__f = (intptr_t *)(frame_ + 0x708);   /* ebp-0x8458 */
    intptr_t *const v189__f = (intptr_t *)(frame_ + 0x70c);   /* ebp-0x8454 */
    intptr_t *const v190__f = (intptr_t *)(frame_ + 0x710);   /* ebp-0x8450 */
    char *const v191__f = (char *)(frame_ + 0x714);   /* ebp-0x844c */
    intptr_t *const v193__f = (intptr_t *)(frame_ + 0x834);   /* ebp-0x832c */
    intptr_t *const v195__f = (intptr_t *)(frame_ + 0x938);   /* ebp-0x8228 */
    intptr_t *const v196__f = (intptr_t *)(frame_ + 0x93c);   /* ebp-0x8224 */
    intptr_t *const v197__f = (intptr_t *)(frame_ + 0x940);   /* ebp-0x8220 */
    intptr_t *const v198__f = (intptr_t *)(frame_ + 0x944);   /* ebp-0x821c */
    intptr_t *const v199__f = (intptr_t *)(frame_ + 0x948);   /* ebp-0x8218 */
    intptr_t *const v200__f = (intptr_t *)(frame_ + 0x94c);   /* ebp-0x8214 */
    intptr_t *const v201__f = (intptr_t *)(frame_ + 0x950);   /* ebp-0x8210 */
    intptr_t *const v202__f = (intptr_t *)(frame_ + 0x954);   /* ebp-0x820c */
    intptr_t *const v203__f = (intptr_t *)(frame_ + 0x958);   /* ebp-0x8208 */
    char *const v204__f = (char *)(frame_ + 0x95c);   /* ebp-0x8204 */
    intptr_t *const v206__f = (intptr_t *)(frame_ + 0xa7c);   /* ebp-0x80e4 */
    char *const v207__f = (char *)(frame_ + 0xa80);   /* ebp-0x80e0 */
    intptr_t *const v209__f = (intptr_t *)(frame_ + 0xba0);   /* ebp-0x7fc0 */
    char *const v210__f = (char *)(frame_ + 0xba4);   /* ebp-0x7fbc */
    intptr_t *const v212__f = (intptr_t *)(frame_ + 0xcc4);   /* ebp-0x7e9c */
    char *const v213__f = (char *)(frame_ + 0xcc8);   /* ebp-0x7e98 */
    intptr_t *const v215__f = (intptr_t *)(frame_ + 0xde8);   /* ebp-0x7d78 */
    char *const v216__f = (char *)(frame_ + 0xdec);   /* ebp-0x7d74 */
    intptr_t *const v218__f = (intptr_t *)(frame_ + 0xf0c);   /* ebp-0x7c54 */
    intptr_t *const v220__f = (intptr_t *)(frame_ + 0x1030);   /* ebp-0x7b30 */
    intptr_t *const v222__f = (intptr_t *)(frame_ + 0x1134);   /* ebp-0x7a2c */
    char *const v224__f = (char *)(frame_ + 0x1158);   /* ebp-0x7a08 */
    intptr_t *const v226__f = (intptr_t *)(frame_ + 0x1278);   /* ebp-0x78e8 */
    char *const v227__f = (char *)(frame_ + 0x127c);   /* ebp-0x78e4 */
    intptr_t *const v229__f = (intptr_t *)(frame_ + 0x139c);   /* ebp-0x77c4 */
    char *const v230__f = (char *)(frame_ + 0x13a0);   /* ebp-0x77c0 */
    intptr_t *const v232__f = (intptr_t *)(frame_ + 0x14c0);   /* ebp-0x76a0 */
    char *const v233__f = (char *)(frame_ + 0x14c4);   /* ebp-0x769c */
    intptr_t *const v235__f = (intptr_t *)(frame_ + 0x15e4);   /* ebp-0x757c */
    intptr_t *const v237__f = (intptr_t *)(frame_ + 0x1708);   /* ebp-0x7458 */
    intptr_t *const v239__f = (intptr_t *)(frame_ + 0x180c);   /* ebp-0x7354 */
    int16_t *const v242__f = (int16_t *)(frame_ + 0x192e);   /* ebp-0x7232 */
    intptr_t *const v243__f = (intptr_t *)(frame_ + 0x1930);   /* ebp-0x7230 */
    char *const v245__f = (char *)(frame_ + 0x1954);   /* ebp-0x720c */
    intptr_t *const v247__f = (intptr_t *)(frame_ + 0x1a74);   /* ebp-0x70ec */
    char *const v248__f = (char *)(frame_ + 0x1a78);   /* ebp-0x70e8 */
    intptr_t *const v250__f = (intptr_t *)(frame_ + 0x1b98);   /* ebp-0x6fc8 */
    char *const v251__f = (char *)(frame_ + 0x1b9c);   /* ebp-0x6fc4 */
    intptr_t *const v253__f = (intptr_t *)(frame_ + 0x1cbc);   /* ebp-0x6ea4 */
    char *const v254__f = (char *)(frame_ + 0x1cc0);   /* ebp-0x6ea0 */
    intptr_t *const v256__f = (intptr_t *)(frame_ + 0x1de0);   /* ebp-0x6d80 */
    char *const v257__f = (char *)(frame_ + 0x1de4);   /* ebp-0x6d7c */
    intptr_t *const v259__f = (intptr_t *)(frame_ + 0x1f04);   /* ebp-0x6c5c */
    intptr_t *const v261__f = (intptr_t *)(frame_ + 0x2028);   /* ebp-0x6b38 */
    intptr_t *const v263__f = (intptr_t *)(frame_ + 0x212c);   /* ebp-0x6a34 */
    char *const v265__f = (char *)(frame_ + 0x2150);   /* ebp-0x6a10 */
    intptr_t *const v267__f = (intptr_t *)(frame_ + 0x2270);   /* ebp-0x68f0 */
    char *const v268__f = (char *)(frame_ + 0x2274);   /* ebp-0x68ec */
    intptr_t *const v270__f = (intptr_t *)(frame_ + 0x2394);   /* ebp-0x67cc */
    char *const v271__f = (char *)(frame_ + 0x2398);   /* ebp-0x67c8 */
    intptr_t *const v273__f = (intptr_t *)(frame_ + 0x24b8);   /* ebp-0x66a8 */
    char *const v274__f = (char *)(frame_ + 0x24bc);   /* ebp-0x66a4 */
    intptr_t *const v276__f = (intptr_t *)(frame_ + 0x25dc);   /* ebp-0x6584 */
    int16_t *const v278__f = (int16_t *)(frame_ + 0x26dd);   /* ebp-0x6483 */
    char *const v279__f = (char *)(frame_ + 0x26df);   /* ebp-0x6481 */
    int16_t *const v282__f = (int16_t *)(frame_ + 0x2801);   /* ebp-0x635f */
    char *const v283__f = (char *)(frame_ + 0x2803);   /* ebp-0x635d */
    intptr_t *const v284__f = (intptr_t *)(frame_ + 0x2804);   /* ebp-0x635c */
    char *const v286__f = (char *)(frame_ + 0x2828);   /* ebp-0x6338 */
    intptr_t *const v288__f = (intptr_t *)(frame_ + 0x2948);   /* ebp-0x6218 */
    char *const v289__f = (char *)(frame_ + 0x294c);   /* ebp-0x6214 */
    intptr_t *const v291__f = (intptr_t *)(frame_ + 0x2a6c);   /* ebp-0x60f4 */
    char *const v292__f = (char *)(frame_ + 0x2a70);   /* ebp-0x60f0 */
    intptr_t *const v294__f = (intptr_t *)(frame_ + 0x2b90);   /* ebp-0x5fd0 */
    char *const v295__f = (char *)(frame_ + 0x2b94);   /* ebp-0x5fcc */
    intptr_t *const v297__f = (intptr_t *)(frame_ + 0x2cb4);   /* ebp-0x5eac */
    int16_t *const v299__f = (int16_t *)(frame_ + 0x2db5);   /* ebp-0x5dab */
    char *const v300__f = (char *)(frame_ + 0x2db7);   /* ebp-0x5da9 */
    intptr_t *const v303__f = (intptr_t *)(frame_ + 0x2edc);   /* ebp-0x5c84 */
    char *const v305__f = (char *)(frame_ + 0x2f00);   /* ebp-0x5c60 */
    intptr_t *const v307__f = (intptr_t *)(frame_ + 0x3020);   /* ebp-0x5b40 */
    char *const v308__f = (char *)(frame_ + 0x3024);   /* ebp-0x5b3c */
    intptr_t *const v310__f = (intptr_t *)(frame_ + 0x3144);   /* ebp-0x5a1c */
    char *const v311__f = (char *)(frame_ + 0x3148);   /* ebp-0x5a18 */
    intptr_t *const v313__f = (intptr_t *)(frame_ + 0x3268);   /* ebp-0x58f8 */
    char *const v314__f = (char *)(frame_ + 0x326c);   /* ebp-0x58f4 */
    intptr_t *const v316__f = (intptr_t *)(frame_ + 0x338c);   /* ebp-0x57d4 */
    intptr_t *const v318__f = (intptr_t *)(frame_ + 0x34b0);   /* ebp-0x56b0 */
    int16_t *const v320__f = (int16_t *)(frame_ + 0x35b1);   /* ebp-0x55af */
    char *const v321__f = (char *)(frame_ + 0x35b3);   /* ebp-0x55ad */
    intptr_t *const v322__f = (intptr_t *)(frame_ + 0x35b4);   /* ebp-0x55ac */
    char *const v324__f = (char *)(frame_ + 0x35d8);   /* ebp-0x5588 */
    intptr_t *const v326__f = (intptr_t *)(frame_ + 0x36f8);   /* ebp-0x5468 */
    intptr_t *const v328__f = (intptr_t *)(frame_ + 0x37fc);   /* ebp-0x5364 */
    intptr_t *const v329__f = (intptr_t *)(frame_ + 0x3800);   /* ebp-0x5360 */
    int16_t *const v332__f = (int16_t *)(frame_ + 0x391e);   /* ebp-0x5242 */
    intptr_t *const v333__f = (intptr_t *)(frame_ + 0x3920);   /* ebp-0x5240 */
    intptr_t *const v334__f = (intptr_t *)(frame_ + 0x3924);   /* ebp-0x523c */
    intptr_t *const v337__f = (intptr_t *)(frame_ + 0x3a44);   /* ebp-0x511c */
    intptr_t *const v338__f = (intptr_t *)(frame_ + 0x3a48);   /* ebp-0x5118 */
    char *const v340__f = (char *)(frame_ + 0x3a68);   /* ebp-0x50f8 */
    intptr_t *const v342__f = (intptr_t *)(frame_ + 0x3b88);   /* ebp-0x4fd8 */
    intptr_t *const v344__f = (intptr_t *)(frame_ + 0x3c8c);   /* ebp-0x4ed4 */
    intptr_t *const v345__f = (intptr_t *)(frame_ + 0x3c90);   /* ebp-0x4ed0 */
    intptr_t *const v348__f = (intptr_t *)(frame_ + 0x3db0);   /* ebp-0x4db0 */
    intptr_t *const v349__f = (intptr_t *)(frame_ + 0x3db4);   /* ebp-0x4dac */
    char *const v351__f = (char *)(frame_ + 0x3dd4);   /* ebp-0x4d8c */
    intptr_t *const v353__f = (intptr_t *)(frame_ + 0x3ef4);   /* ebp-0x4c6c */
    char *const v354__f = (char *)(frame_ + 0x3ef8);   /* ebp-0x4c68 */
    intptr_t *const v356__f = (intptr_t *)(frame_ + 0x4018);   /* ebp-0x4b48 */
    char *const v357__f = (char *)(frame_ + 0x401c);   /* ebp-0x4b44 */
    intptr_t *const v359__f = (intptr_t *)(frame_ + 0x413c);   /* ebp-0x4a24 */
    char *const v360__f = (char *)(frame_ + 0x4140);   /* ebp-0x4a20 */
    intptr_t *const v362__f = (intptr_t *)(frame_ + 0x4260);   /* ebp-0x4900 */
    intptr_t *const v364__f = (intptr_t *)(frame_ + 0x4384);   /* ebp-0x47dc */
    int16_t *const v366__f = (int16_t *)(frame_ + 0x4486);   /* ebp-0x46da */
    intptr_t *const v367__f = (intptr_t *)(frame_ + 0x4488);   /* ebp-0x46d8 */
    int16_t *const v370__f = (int16_t *)(frame_ + 0x45aa);   /* ebp-0x45b6 */
    intptr_t *const v371__f = (intptr_t *)(frame_ + 0x45ac);   /* ebp-0x45b4 */
    intptr_t *const v372__f = (intptr_t *)(frame_ + 0x45b0);   /* ebp-0x45b0 */
    char *const v374__f = (char *)(frame_ + 0x45d0);   /* ebp-0x4590 */
    intptr_t *const v376__f = (intptr_t *)(frame_ + 0x46f0);   /* ebp-0x4470 */
    char *const v377__f = (char *)(frame_ + 0x46f4);   /* ebp-0x446c */
    intptr_t *const v379__f = (intptr_t *)(frame_ + 0x4814);   /* ebp-0x434c */
    char *const v380__f = (char *)(frame_ + 0x4818);   /* ebp-0x4348 */
    intptr_t *const v382__f = (intptr_t *)(frame_ + 0x4938);   /* ebp-0x4228 */
    char *const v383__f = (char *)(frame_ + 0x493c);   /* ebp-0x4224 */
    intptr_t *const v385__f = (intptr_t *)(frame_ + 0x4a5c);   /* ebp-0x4104 */
    int16_t *const v387__f = (int16_t *)(frame_ + 0x4b5d);   /* ebp-0x4003 */
    char *const v388__f = (char *)(frame_ + 0x4b5f);   /* ebp-0x4001 */
    char *const v390__f = (char *)(frame_ + 0x4b84);   /* ebp-0x3fdc */
    intptr_t *const v392__f = (intptr_t *)(frame_ + 0x4ca4);   /* ebp-0x3ebc */
    int16_t *const v394__f = (int16_t *)(frame_ + 0x4da6);   /* ebp-0x3dba */
    intptr_t *const v395__f = (intptr_t *)(frame_ + 0x4da8);   /* ebp-0x3db8 */
    int16_t *const v398__f = (int16_t *)(frame_ + 0x4ec9);   /* ebp-0x3c97 */
    char *const v399__f = (char *)(frame_ + 0x4ecb);   /* ebp-0x3c95 */
    intptr_t *const v400__f = (intptr_t *)(frame_ + 0x4ecc);   /* ebp-0x3c94 */
    char *const v402__f = (char *)(frame_ + 0x4ef0);   /* ebp-0x3c70 */
    intptr_t *const v404__f = (intptr_t *)(frame_ + 0x5010);   /* ebp-0x3b50 */
    int16_t *const v406__f = (int16_t *)(frame_ + 0x5111);   /* ebp-0x3a4f */
    char *const v407__f = (char *)(frame_ + 0x5113);   /* ebp-0x3a4d */
    intptr_t *const v408__f = (intptr_t *)(frame_ + 0x5114);   /* ebp-0x3a4c */
    int16_t *const v411__f = (int16_t *)(frame_ + 0x5236);   /* ebp-0x392a */
    intptr_t *const v412__f = (intptr_t *)(frame_ + 0x5238);   /* ebp-0x3928 */
    char *const v414__f = (char *)(frame_ + 0x525c);   /* ebp-0x3904 */
    intptr_t *const v416__f = (intptr_t *)(frame_ + 0x537c);   /* ebp-0x37e4 */
    intptr_t *const v418__f = (intptr_t *)(frame_ + 0x5480);   /* ebp-0x36e0 */
    intptr_t *const v421__f = (intptr_t *)(frame_ + 0x55a4);   /* ebp-0x35bc */
    char *const v423__f = (char *)(frame_ + 0x55c8);   /* ebp-0x3598 */
    intptr_t *const v425__f = (intptr_t *)(frame_ + 0x56e8);   /* ebp-0x3478 */
    intptr_t *const v427__f = (intptr_t *)(frame_ + 0x57ec);   /* ebp-0x3374 */
    intptr_t *const v430__f = (intptr_t *)(frame_ + 0x5910);   /* ebp-0x3250 */
    char *const v432__f = (char *)(frame_ + 0x5934);   /* ebp-0x322c */
    intptr_t *const v434__f = (intptr_t *)(frame_ + 0x5a54);   /* ebp-0x310c */
    char *const v435__f = (char *)(frame_ + 0x5a58);   /* ebp-0x3108 */
    intptr_t *const v437__f = (intptr_t *)(frame_ + 0x5b78);   /* ebp-0x2fe8 */
    char *const v438__f = (char *)(frame_ + 0x5b7c);   /* ebp-0x2fe4 */
    intptr_t *const v440__f = (intptr_t *)(frame_ + 0x5c9c);   /* ebp-0x2ec4 */
    char *const v441__f = (char *)(frame_ + 0x5ca0);   /* ebp-0x2ec0 */
    intptr_t *const v443__f = (intptr_t *)(frame_ + 0x5dc0);   /* ebp-0x2da0 */
    int16_t *const v445__f = (int16_t *)(frame_ + 0x5ec2);   /* ebp-0x2c9e */
    int16_t *const v448__f = (int16_t *)(frame_ + 0x5fe5);   /* ebp-0x2b7b */
    char *const v449__f = (char *)(frame_ + 0x5fe7);   /* ebp-0x2b79 */
    intptr_t *const v450__f = (intptr_t *)(frame_ + 0x5fe8);   /* ebp-0x2b78 */
    char *const v452__f = (char *)(frame_ + 0x600c);   /* ebp-0x2b54 */
    intptr_t *const v454__f = (intptr_t *)(frame_ + 0x612c);   /* ebp-0x2a34 */
    char *const v455__f = (char *)(frame_ + 0x6130);   /* ebp-0x2a30 */
    intptr_t *const v457__f = (intptr_t *)(frame_ + 0x6250);   /* ebp-0x2910 */
    char *const v458__f = (char *)(frame_ + 0x6254);   /* ebp-0x290c */
    intptr_t *const v460__f = (intptr_t *)(frame_ + 0x6374);   /* ebp-0x27ec */
    char *const v461__f = (char *)(frame_ + 0x6378);   /* ebp-0x27e8 */
    intptr_t *const v463__f = (intptr_t *)(frame_ + 0x6498);   /* ebp-0x26c8 */
    intptr_t *const v465__f = (intptr_t *)(frame_ + 0x65bc);   /* ebp-0x25a4 */
    intptr_t *const v467__f = (intptr_t *)(frame_ + 0x66c0);   /* ebp-0x24a0 */
    char *const v469__f = (char *)(frame_ + 0x66e4);   /* ebp-0x247c */
    intptr_t *const v471__f = (intptr_t *)(frame_ + 0x6804);   /* ebp-0x235c */
    char *const v472__f = (char *)(frame_ + 0x6808);   /* ebp-0x2358 */
    intptr_t *const v474__f = (intptr_t *)(frame_ + 0x6928);   /* ebp-0x2238 */
    char *const v475__f = (char *)(frame_ + 0x692c);   /* ebp-0x2234 */
    intptr_t *const v477__f = (intptr_t *)(frame_ + 0x6a4c);   /* ebp-0x2114 */
    char *const v478__f = (char *)(frame_ + 0x6a50);   /* ebp-0x2110 */
    intptr_t *const v480__f = (intptr_t *)(frame_ + 0x6b70);   /* ebp-0x1ff0 */
    int16_t *const v482__f = (int16_t *)(frame_ + 0x6c72);   /* ebp-0x1eee */
    int16_t *const v485__f = (int16_t *)(frame_ + 0x6d96);   /* ebp-0x1dca */
    intptr_t *const v486__f = (intptr_t *)(frame_ + 0x6d98);   /* ebp-0x1dc8 */
    intptr_t *const v489__f = (intptr_t *)(frame_ + 0x6ebc);   /* ebp-0x1ca4 */
    intptr_t *const v492__f = (intptr_t *)(frame_ + 0x6fe0);   /* ebp-0x1b80 */
    char *const v494__f = (char *)(frame_ + 0x7004);   /* ebp-0x1b5c */
    intptr_t *const v496__f = (intptr_t *)(frame_ + 0x7124);   /* ebp-0x1a3c */
    char *const v497__f = (char *)(frame_ + 0x7128);   /* ebp-0x1a38 */
    intptr_t *const v499__f = (intptr_t *)(frame_ + 0x7248);   /* ebp-0x1918 */
    char *const v500__f = (char *)(frame_ + 0x724c);   /* ebp-0x1914 */
    intptr_t *const v502__f = (intptr_t *)(frame_ + 0x736c);   /* ebp-0x17f4 */
    char *const v503__f = (char *)(frame_ + 0x7370);   /* ebp-0x17f0 */
    intptr_t *const v505__f = (intptr_t *)(frame_ + 0x7490);   /* ebp-0x16d0 */
    int16_t *const v507__f = (int16_t *)(frame_ + 0x7591);   /* ebp-0x15cf */
    char *const v508__f = (char *)(frame_ + 0x7593);   /* ebp-0x15cd */
    int16_t *const v511__f = (int16_t *)(frame_ + 0x76b5);   /* ebp-0x14ab */
    char *const v512__f = (char *)(frame_ + 0x76b7);   /* ebp-0x14a9 */
    intptr_t *const v513__f = (intptr_t *)(frame_ + 0x76b8);   /* ebp-0x14a8 */
    intptr_t *const v516__f = (intptr_t *)(frame_ + 0x77dc);   /* ebp-0x1384 */
    char *const v518__f = (char *)(frame_ + 0x7800);   /* ebp-0x1360 */
    intptr_t *const v520__f = (intptr_t *)(frame_ + 0x7920);   /* ebp-0x1240 */
    char *const v521__f = (char *)(frame_ + 0x7924);   /* ebp-0x123c */
    intptr_t *const v523__f = (intptr_t *)(frame_ + 0x7a44);   /* ebp-0x111c */
    char *const v524__f = (char *)(frame_ + 0x7a48);   /* ebp-0x1118 */
    intptr_t *const v526__f = (intptr_t *)(frame_ + 0x7b68);   /* ebp-0xff8 */
    char *const v527__f = (char *)(frame_ + 0x7b6c);   /* ebp-0xff4 */
    intptr_t *const v529__f = (intptr_t *)(frame_ + 0x7c8c);   /* ebp-0xed4 */
    int16_t *const v531__f = (int16_t *)(frame_ + 0x7d8e);   /* ebp-0xdd2 */
    intptr_t *const v534__f = (intptr_t *)(frame_ + 0x7eb4);   /* ebp-0xcac */
    char *const v536__f = (char *)(frame_ + 0x7ed8);   /* ebp-0xc88 */
    intptr_t *const v538__f = (intptr_t *)(frame_ + 0x7ff8);   /* ebp-0xb68 */
    char *const v539__f = (char *)(frame_ + 0x7ffc);   /* ebp-0xb64 */
    intptr_t *const v541__f = (intptr_t *)(frame_ + 0x811c);   /* ebp-0xa44 */
    char *const v542__f = (char *)(frame_ + 0x8120);   /* ebp-0xa40 */
    intptr_t *const v544__f = (intptr_t *)(frame_ + 0x8240);   /* ebp-0x920 */
    char *const v545__f = (char *)(frame_ + 0x8244);   /* ebp-0x91c */
    intptr_t *const v547__f = (intptr_t *)(frame_ + 0x8364);   /* ebp-0x7fc */
    intptr_t *const v549__f = (intptr_t *)(frame_ + 0x8488);   /* ebp-0x6d8 */
    intptr_t *const v551__f = (intptr_t *)(frame_ + 0x858c);   /* ebp-0x5d4 */
    char *const v553__f = (char *)(frame_ + 0x85b0);   /* ebp-0x5b0 */
    intptr_t *const v555__f = (intptr_t *)(frame_ + 0x86d0);   /* ebp-0x490 */
    char *const v556__f = (char *)(frame_ + 0x86d4);   /* ebp-0x48c */
    intptr_t *const v558__f = (intptr_t *)(frame_ + 0x87f4);   /* ebp-0x36c */
    char *const v559__f = (char *)(frame_ + 0x87f8);   /* ebp-0x368 */
    intptr_t *const v561__f = (intptr_t *)(frame_ + 0x8918);   /* ebp-0x248 */
    char *const v562__f = (char *)(frame_ + 0x891c);   /* ebp-0x244 */

  (*v146__f) = 0;
  (*v126__f) = 0;
  (*v141__f) = 0;
  (*v145__f) = 0;
  (*v129__f) = 0;
  W_tm_56d710((intptr_t)(c));
  if ( SYS32(0x2884) != 0
    && F32(CFG(), 372) != 0
    && (!W_sd_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0)) || W_sd_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0)) != 0) )
  {
    F16(D(0x8a93e8), 0) = 0;
    W_sd_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
    if ( W_sd_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
    {
      if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
        g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
    }
    else if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
    {
      name = (SYS32(0x2884) + 0x2c);
      g_eng.log_fail_406440(g_eng.log, (const char *)((SYS32(0x2884) + 0x2c)), (const char *)(D(0x8a9b6c)));
    }
  }
  (*v150__f) = 17;
  (*v149__f) = 0;
  (*v151__f) = 0;
  (*v152__f) = 0;
  (*v154__f) = 0;
  (*TickCount__f) = API_GetTickCount();
  (*v163__f) = 1;
  (*v164__f) = F8(D(0x8a9b6c), 0);
  memset(v165, 0, (255 * sizeof(uint8_t)));
  (*v166__f) = 0;
  (*v167__f) = -176;
  (*v168__f) = 1;
  (*v169__f) = 30;
  (*v170__f) = 0;
  (*v171__f) = 96;
  (*v172__f) = 432;
  (*v173__f) = 48;
  (*v174__f) = 0;
  (*v175__f) = 1;
  (*v176__f) = F8(D(0x8a9b6c), 0);
  memset(v177, 0, (287 * sizeof(uint8_t)));
  (*v178__f) = 1;
  (*v179__f) = F8(D(0x8a9b6c), 0);
  memset(v180, 0, (255 * sizeof(uint8_t)));
  (*v188__f) = 0;
  (*v181__f) = 0;
  (*v182__f) = 1;
  (*v183__f) = 31;
  (*v184__f) = 0;
  (*v185__f) = 96;
  (*v186__f) = 432;
  (*v187__f) = 48;
  (*v189__f) = 0;
  (*v190__f) = 1;
  (*v191__f) = F8(D(0x8a9b6c), 0);
  memset(v192, 0, (287 * sizeof(uint8_t)));
  strcpy(v194, "- Creative Staff -");
  (*v193__f) = 1;
  memset(&v194[19], 0, 237);
  (*v198__f) = 0;
  (*v199__f) = 0;
  (*v200__f) = 0;
  (*v195__f) = 0;
  (*v201__f) = 0;
  (*v196__f) = 0;
  (*v202__f) = 0;
  (*v197__f) = 1;
  (*v203__f) = 1;
  (*v204__f) = F8(D(0x8a9b6c), 0);
  memset(v205, 0, (287 * sizeof(uint8_t)));
  (*v206__f) = 1;
  (*v207__f) = F8(D(0x8a9b6c), 0);
  memset(v208, 0, (287 * sizeof(uint8_t)));
  (*v209__f) = 1;
  (*v210__f) = F8(D(0x8a9b6c), 0);
  memset(v211, 0, (287 * sizeof(uint8_t)));
  (*v212__f) = 1;
  (*v213__f) = F8(D(0x8a9b6c), 0);
  memset(v214, 0, (287 * sizeof(uint8_t)));
  (*v215__f) = 1;
  (*v216__f) = F8(D(0x8a9b6c), 0);
  memset(v217, 0, (287 * sizeof(uint8_t)));
  strcpy(v219, "Directed by");
  (*v218__f) = 1;
  memset(&v219[12], 0, 276);
  strcpy(v221, "Lizsoft");
  (*v220__f) = 1;
  memset(&v221[8], 0, 0xF8u);
  (*v222__f) = 24;
  memset(v223, 0, 0x1Cu);
  v223[7] = 1;
  (*v224__f) = F8(D(0x8a9b6c), 0);
  memset(v225, 0, (287 * sizeof(uint8_t)));
  (*v226__f) = 1;
  (*v227__f) = F8(D(0x8a9b6c), 0);
  memset(v228, 0, (287 * sizeof(uint8_t)));
  (*v229__f) = 1;
  (*v230__f) = F8(D(0x8a9b6c), 0);
  memset(v231, 0, (287 * sizeof(uint8_t)));
  (*v232__f) = 1;
  (*v233__f) = F8(D(0x8a9b6c), 0);
  memset(v234, 0, (287 * sizeof(uint8_t)));
  strcpy(v236, "Produced by");
  (*v235__f) = 1;
  memset(&v236[12], 0, 276);
  strcpy(v238, "K.Kitahara");
  (*v237__f) = 1;
  memset(&v238[11], 0, 245);
  (*v239__f) = 24;
  memset(v240, 0, 0x1Cu);
  strcpy(v241, "(Lizsoft MEL)");
  v240[7] = 1;
  memset(&v241[14], 0, 0xF0u);
  (*v242__f) = 0;
  (*v243__f) = 48;
  memset(v244, 0, 0x1Cu);
  v244[7] = 1;
  (*v245__f) = F8(D(0x8a9b6c), 0);
  memset(v246, 0, (287 * sizeof(uint8_t)));
  (*v247__f) = 1;
  (*v248__f) = F8(D(0x8a9b6c), 0);
  memset(v249, 0, (287 * sizeof(uint8_t)));
  (*v250__f) = 1;
  (*v251__f) = F8(D(0x8a9b6c), 0);
  memset(v252, 0, (287 * sizeof(uint8_t)));
  (*v253__f) = 1;
  (*v254__f) = F8(D(0x8a9b6c), 0);
  memset(v255, 0, (287 * sizeof(uint8_t)));
  (*v256__f) = 1;
  (*v257__f) = F8(D(0x8a9b6c), 0);
  memset(v258, 0, (287 * sizeof(uint8_t)));
  strcpy(v260, "Program");
  (*v259__f) = 1;
  memset(&v260[8], 0, 280);
  strcpy(v262, "K.Kitahara");
  (*v261__f) = 1;
  memset(&v262[11], 0, 245);
  (*v263__f) = 24;
  memset(v264, 0, 0x1Cu);
  v264[7] = 1;
  (*v265__f) = F8(D(0x8a9b6c), 0);
  memset(v266, 0, (287 * sizeof(uint8_t)));
  (*v267__f) = 1;
  (*v268__f) = F8(D(0x8a9b6c), 0);
  memset(v269, 0, (287 * sizeof(uint8_t)));
  (*v270__f) = 1;
  (*v271__f) = F8(D(0x8a9b6c), 0);
  memset(v272, 0, (287 * sizeof(uint8_t)));
  (*v273__f) = 1;
  (*v274__f) = F8(D(0x8a9b6c), 0);
  memset(v275, 0, (287 * sizeof(uint8_t)));
  strcpy(v277, "Game Graphic");
  (*v276__f) = 1;
  memset(&v277[13], 0, 0xF0u);
  (*v278__f) = 0;
  (*v279__f) = 0;
  memset(v280, 0, 0x20u);
  strcpy(v281, "  K.Kitahara");
  v280[8] = 1;
  memset(&v281[13], 0, 0xF0u);
  (*v282__f) = 0;
  (*v283__f) = 0;
  (*v284__f) = 24;
  memset(v285, 0, 0x1Cu);
  v285[7] = 1;
  (*v286__f) = F8(D(0x8a9b6c), 0);
  memset(v287, 0, (287 * sizeof(uint8_t)));
  (*v288__f) = 1;
  (*v289__f) = F8(D(0x8a9b6c), 0);
  memset(v290, 0, (287 * sizeof(uint8_t)));
  (*v291__f) = 1;
  (*v292__f) = F8(D(0x8a9b6c), 0);
  memset(v293, 0, (287 * sizeof(uint8_t)));
  (*v294__f) = 1;
  (*v295__f) = F8(D(0x8a9b6c), 0);
  memset(v296, 0, (287 * sizeof(uint8_t)));
  strcpy(v298, "Illustration");
  (*v297__f) = 1;
  memset(&v298[13], 0, 0xF0u);
  (*v299__f) = 0;
  (*v300__f) = 0;
  memset(v301, 0, 0x20u);
  strcpy(v302, "Takeda Mika");
  v301[8] = 1;
  memset(&v302[12], 0, 0xF4u);
  (*v303__f) = 24;
  memset(v304, 0, 0x1Cu);
  v304[7] = 1;
  (*v305__f) = F8(D(0x8a9b6c), 0);
  memset(v306, 0, (287 * sizeof(uint8_t)));
  (*v307__f) = 1;
  (*v308__f) = F8(D(0x8a9b6c), 0);
  memset(v309, 0, (287 * sizeof(uint8_t)));
  (*v310__f) = 1;
  (*v311__f) = F8(D(0x8a9b6c), 0);
  memset(v312, 0, (287 * sizeof(uint8_t)));
  (*v313__f) = 1;
  (*v314__f) = F8(D(0x8a9b6c), 0);
  memset(v315, 0, (287 * sizeof(uint8_t)));
  (*v316__f) = 1;
  strcpy(v317, "Translation");
  memset(&v317[12], 0, 276);
  strcpy(v319, "Carpe Fulgur LLC");
  (*v318__f) = 1;
  memset(&v319[17], 0, 0xECu);
  (*v320__f) = 0;
  (*v321__f) = 0;
  (*v322__f) = 24;
  memset(v323, 0, 0x1Cu);
  v323[7] = 1;
  (*v324__f) = F8(D(0x8a9b6c), 0);
  memset(v325, 0, (287 * sizeof(uint8_t)));
  strcpy(v327, "Lead Translator");
  (*v326__f) = 1;
  memset(&v327[16], 0, 0xF0u);
  (*v328__f) = 48;
  (*v329__f) = 10;
  memset(v330, 0, 0x18u);
  strcpy(v331, "Robin");
  v330[6] = 1;
  memset(&v331[6], 0, 0xF8u);
  (*v332__f) = 0;
  (*v333__f) = 68;
  (*v334__f) = 20;
  memset(v335, 0, 0x18u);
  v335[6] = 1;
  strcpy(v336, "Light-Williams");
  memset(&v336[15], 0, 241);
  (*v337__f) = 68;
  (*v338__f) = 10;
  memset(v339, 0, 0x18u);
  v339[6] = 1;
  (*v340__f) = F8(D(0x8a9b6c), 0);
  memset(v341, 0, (287 * sizeof(uint8_t)));
  strcpy(v343, "Lead Editor");
  (*v342__f) = 1;
  memset(&v343[12], 0, 0xF4u);
  (*v344__f) = 48;
  (*v345__f) = 10;
  memset(v346, 0, 0x18u);
  strcpy(v347, "Andrew Dice");
  v346[6] = 1;
  memset(&v347[12], 0, 0xF4u);
  (*v348__f) = 68;
  (*v349__f) = 10;
  memset(v350, 0, 0x18u);
  v350[6] = 1;
  (*v351__f) = F8(D(0x8a9b6c), 0);
  memset(v352, 0, (287 * sizeof(uint8_t)));
  (*v353__f) = 1;
  (*v354__f) = F8(D(0x8a9b6c), 0);
  memset(v355, 0, (287 * sizeof(uint8_t)));
  (*v356__f) = 1;
  (*v357__f) = F8(D(0x8a9b6c), 0);
  memset(v358, 0, (287 * sizeof(uint8_t)));
  (*v359__f) = 1;
  (*v360__f) = F8(D(0x8a9b6c), 0);
  memset(v361, 0, (287 * sizeof(uint8_t)));
  strcpy(v363, "Logotype Design");
  (*v362__f) = 1;
  memset(&v363[16], 0, 272);
  strcpy(v365, "Takashi Saito");
  (*v364__f) = 1;
  memset(&v365[14], 0, 0xF0u);
  (*v366__f) = 0;
  (*v367__f) = 24;
  memset(v368, 0, 0x1Cu);
  strcpy(v369, "(Jungle,Inc.)");
  v368[7] = 1;
  memset(&v369[14], 0, 0xF0u);
  (*v370__f) = 0;
  (*v371__f) = 48;
  (*v372__f) = -10;
  memset(v373, 0, 0x18u);
  v373[6] = 1;
  (*v374__f) = F8(D(0x8a9b6c), 0);
  memset(v375, 0, (287 * sizeof(uint8_t)));
  (*v376__f) = 1;
  (*v377__f) = F8(D(0x8a9b6c), 0);
  memset(v378, 0, (287 * sizeof(uint8_t)));
  (*v379__f) = 1;
  (*v380__f) = F8(D(0x8a9b6c), 0);
  memset(v381, 0, (287 * sizeof(uint8_t)));
  (*v382__f) = 1;
  (*v383__f) = F8(D(0x8a9b6c), 0);
  memset(v384, 0, (287 * sizeof(uint8_t)));
  strcpy(v386, "Voice Artist");
  (*v385__f) = 1;
  memset(&v386[13], 0, 0xF0u);
  (*v387__f) = 0;
  (*v388__f) = 0;
  memset(v389, 0, 0x20u);
  v389[8] = 1;
  (*v390__f) = F8(D(0x8a9b6c), 0);
  memset(v391, 0, (287 * sizeof(uint8_t)));
  (*v392__f) = 1;
  strcpy(v393, "Arche");
  memset(&v393[6], 0, 0xF8u);
  (*v394__f) = 0;
  (*v395__f) = 24;
  memset(v396, 0, 0x1Cu);
  strcpy(v397, "Aihara Kaori");
  v396[7] = 1;
  memset(&v397[13], 0, 0xF0u);
  (*v398__f) = 0;
  (*v399__f) = 0;
  (*v400__f) = 48;
  memset(v401, 0, 0x1Cu);
  v401[7] = 1;
  (*v402__f) = F8(D(0x8a9b6c), 0);
  memset(v403, 0, (287 * sizeof(uint8_t)));
  strcpy(v405, "Sana");
  (*v404__f) = 1;
  memset(&v405[5], 0, 0xF8u);
  (*v406__f) = 0;
  (*v407__f) = 0;
  (*v408__f) = 24;
  memset(v409, 0, 0x1Cu);
  strcpy(v410, "Souno Sachika");
  v409[7] = 1;
  memset(&v410[14], 0, 0xF0u);
  (*v411__f) = 0;
  (*v412__f) = 48;
  memset(v413, 0, 0x1Cu);
  v413[7] = 1;
  (*v414__f) = F8(D(0x8a9b6c), 0);
  memset(v415, 0, (287 * sizeof(uint8_t)));
  strcpy(v417, "Stella");
  (*v416__f) = 1;
  memset(&v417[7], 0, 249);
  (*v418__f) = 24;
  memset(v419, 0, 0x1Cu);
  strcpy(v420, "Ruou Ryouki");
  v419[7] = 1;
  memset(&v420[12], 0, 0xF4u);
  (*v421__f) = 48;
  memset(v422, 0, 0x1Cu);
  v422[7] = 1;
  (*v423__f) = F8(D(0x8a9b6c), 0);
  memset(v424, 0, (287 * sizeof(uint8_t)));
  strcpy(v426, "Chiffon,Selene");
  (*v425__f) = 1;
  memset(&v426[15], 0, 241);
  (*v427__f) = 24;
  memset(v428, 0, 0x1Cu);
  strcpy(v429, "Ouka Kasuga");
  v428[7] = 1;
  memset(&v429[12], 0, 0xF4u);
  (*v430__f) = 48;
  memset(v431, 0, 0x1Cu);
  v431[7] = 1;
  (*v432__f) = F8(D(0x8a9b6c), 0);
  memset(v433, 0, (287 * sizeof(uint8_t)));
  (*v434__f) = 1;
  (*v435__f) = F8(D(0x8a9b6c), 0);
  memset(v436, 0, (287 * sizeof(uint8_t)));
  (*v437__f) = 1;
  (*v438__f) = F8(D(0x8a9b6c), 0);
  memset(v439, 0, (287 * sizeof(uint8_t)));
  (*v440__f) = 1;
  (*v441__f) = F8(D(0x8a9b6c), 0);
  memset(v442, 0, (287 * sizeof(uint8_t)));
  strcpy(v444, "Test Play");
  (*v443__f) = 1;
  memset(&v444[10], 0, 0xF4u);
  (*v445__f) = 0;
  memset(v446, 0, 0x20u);
  strcpy(v447, "Mynk");
  v446[8] = 1;
  memset(&v447[5], 0, 0xF8u);
  (*v448__f) = 0;
  (*v449__f) = 0;
  (*v450__f) = 24;
  memset(v451, 0, 0x1Cu);
  v451[7] = 1;
  (*v452__f) = F8(D(0x8a9b6c), 0);
  memset(v453, 0, (287 * sizeof(uint8_t)));
  (*v454__f) = 1;
  (*v455__f) = F8(D(0x8a9b6c), 0);
  memset(v456, 0, (287 * sizeof(uint8_t)));
  (*v457__f) = 1;
  (*v458__f) = F8(D(0x8a9b6c), 0);
  memset(v459, 0, (287 * sizeof(uint8_t)));
  (*v460__f) = 1;
  (*v461__f) = F8(D(0x8a9b6c), 0);
  memset(v462, 0, (287 * sizeof(uint8_t)));
  strcpy(v464, "Background Picture");
  (*v463__f) = 1;
  memset(&v464[19], 0, 269);
  strcpy(v466, "BIHOU,Inc.");
  (*v465__f) = 1;
  memset(&v466[11], 0, 245);
  (*v467__f) = 24;
  memset(v468, 0, 0x1Cu);
  v468[7] = 1;
  (*v469__f) = F8(D(0x8a9b6c), 0);
  memset(v470, 0, (287 * sizeof(uint8_t)));
  (*v471__f) = 1;
  (*v472__f) = F8(D(0x8a9b6c), 0);
  memset(v473, 0, (287 * sizeof(uint8_t)));
  (*v474__f) = 1;
  (*v475__f) = F8(D(0x8a9b6c), 0);
  memset(v476, 0, (287 * sizeof(uint8_t)));
  (*v477__f) = 1;
  (*v478__f) = F8(D(0x8a9b6c), 0);
  memset(v479, 0, (287 * sizeof(uint8_t)));
  strcpy(v481, "Music");
  (*v480__f) = 1;
  memset(&v481[6], 0, 0xF8u);
  (*v482__f) = 0;
  memset(v483, 0, 0x20u);
  strcpy(v484, "Nash Studio inc.,");
  v483[8] = 1;
  memset(&v484[18], 0, 0xECu);
  (*v485__f) = 0;
  (*v486__f) = 24;
  memset(v487, 0, 0x1Cu);
  strcpy(v488, "BGM OnLine");
  v487[7] = 1;
  memset(&v488[11], 0, 245);
  (*v489__f) = 24;
  memset(v490, 0, 0x1Cu);
  strcpy(v491, "Sound Juke");
  v490[7] = 1;
  memset(&v491[11], 0, 245);
  (*v492__f) = 24;
  memset(v493, 0, 0x1Cu);
  v493[7] = 1;
  (*v494__f) = F8(D(0x8a9b6c), 0);
  memset(v495, 0, (287 * sizeof(uint8_t)));
  (*v496__f) = 1;
  (*v497__f) = F8(D(0x8a9b6c), 0);
  memset(v498, 0, (287 * sizeof(uint8_t)));
  (*v499__f) = 1;
  (*v500__f) = F8(D(0x8a9b6c), 0);
  memset(v501, 0, (287 * sizeof(uint8_t)));
  (*v502__f) = 1;
  (*v503__f) = F8(D(0x8a9b6c), 0);
  memset(v504, 0, (287 * sizeof(uint8_t)));
  strcpy(v506, "Effect Sound");
  (*v505__f) = 1;
  memset(&v506[13], 0, 0xF0u);
  (*v507__f) = 0;
  (*v508__f) = 0;
  memset(v509, 0, 0x20u);
  strcpy(v510, "Datacraft Co,Ltd");
  v509[8] = 1;
  memset(&v510[17], 0, 0xECu);
  (*v511__f) = 0;
  (*v512__f) = 0;
  (*v513__f) = 24;
  memset(v514, 0, 0x1Cu);
  strcpy(v515, "Sound Ideas");
  v514[7] = 1;
  memset(&v515[12], 0, 0xF4u);
  (*v516__f) = 24;
  memset(v517, 0, 0x1Cu);
  v517[7] = 1;
  (*v518__f) = F8(D(0x8a9b6c), 0);
  memset(v519, 0, (287 * sizeof(uint8_t)));
  (*v520__f) = 1;
  (*v521__f) = F8(D(0x8a9b6c), 0);
  memset(v522, 0, (287 * sizeof(uint8_t)));
  (*v523__f) = 1;
  (*v524__f) = F8(D(0x8a9b6c), 0);
  memset(v525, 0, (287 * sizeof(uint8_t)));
  (*v526__f) = 1;
  (*v527__f) = F8(D(0x8a9b6c), 0);
  memset(v528, 0, (287 * sizeof(uint8_t)));
  strcpy(v530, "Sound Coordinator");
  (*v529__f) = 1;
  memset(&v530[18], 0, 0xECu);
  (*v531__f) = 0;
  memset(v532, 0, 0x20u);
  strcpy(v533, "K.Kitahara");
  v532[8] = 1;
  memset(&v533[11], 0, 245);
  (*v534__f) = 24;
  memset(v535, 0, 0x1Cu);
  v535[7] = 1;
  (*v536__f) = F8(D(0x8a9b6c), 0);
  memset(v537, 0, (287 * sizeof(uint8_t)));
  (*v538__f) = 1;
  (*v539__f) = F8(D(0x8a9b6c), 0);
  memset(v540, 0, (287 * sizeof(uint8_t)));
  (*v541__f) = 1;
  (*v542__f) = F8(D(0x8a9b6c), 0);
  memset(v543, 0, (287 * sizeof(uint8_t)));
  (*v544__f) = 1;
  (*v545__f) = F8(D(0x8a9b6c), 0);
  memset(v546, 0, (287 * sizeof(uint8_t)));
  strcpy(v548, "Special Thanks");
  (*v547__f) = 1;
  memset(&v548[15], 0, 273);
  strcpy(v550, "All Users and You.");
  (*v549__f) = 1;
  memset(&v550[19], 0, 237);
  (*v551__f) = 24;
  memset(v552, 0, 0x1Cu);
  v552[7] = 1;
  (*v553__f) = F8(D(0x8a9b6c), 0);
  memset(v554, 0, (287 * sizeof(uint8_t)));
  (*v555__f) = 1;
  (*v556__f) = F8(D(0x8a9b6c), 0);
  memset(v557, 0, (287 * sizeof(uint8_t)));
  (*v558__f) = 1;
  (*v559__f) = F8(D(0x8a9b6c), 0);
  memset(v560, 0, (287 * sizeof(uint8_t)));
  (*v561__f) = 1;
  (*v562__f) = F8(D(0x8a9b6c), 0);
  memset(v563, 0, (579 * sizeof(uint8_t)));
  v3 = 0;
  do
    ++v3;
  while ( *(&(*v163__f) + 73 * v3) != 0 );
  v159[23] = 24;
  (*v144__f) = 3200;
  v159[0] = 1;
  v159[1] = 13000;
  v159[3] = 22;
  v159[10] = 1;
  v159[11] = 30500;
  v159[13] = 23;
  v159[20] = 1;
  v159[21] = 48500;
  v159[24] = 1;
  (*v158__f) = 4000 * v3 + 48800;
  v159[30] = 1;
  (*v157__f) = 16 * (*v158__f) / 158000;
  v159[2] = 16000;
  v159[12] = 16000;
  memset(&v159[25], 0, 20);
  memset(&v159[5], 0, 20);
  memset(&v159[35], 0, 20);
  memset(&v159[55], 0, 20);
  v159[4] = 2;
  memset(&v159[14], 0, 24);
  v159[22] = 17000;
  v159[31] = 67000;
  v159[32] = 17500;
  v159[33] = 25;
  v159[34] = 2;
  v159[40] = 1;
  v159[41] = 86500;
  v159[42] = 17000;
  v159[43] = 26;
  memset(&v159[44], 0, 24);
  v159[50] = 1;
  v159[51] = 105000;
  v159[52] = 16500;
  v159[53] = 27;
  v159[54] = 1;
  v159[64] = 2;
  memset(&v159[65], 0, 20);
  v159[62] = 17000;
  memset(&v159[75], 0, 20);
  memset(&v159[85], 0, 60);
  v159[60] = 1;
  v159[61] = 123000;
  v159[63] = 29;
  v159[70] = 1;
  v159[71] = 141500;
  v159[72] = 12000;
  v159[73] = 28;
  v159[74] = 1;
  v159[80] = 1;
  v159[81] = 155500;
  v159[82] = 15000;
  v159[83] = 32;
  v159[84] = 3;
LABEL_14:
  v4 = (int)(*v124__f);
  v5 = (*v150__f);
LABEL_15:
  while ( 2 )
  {
    v6 = API_GetTickCount();
    (*v155__f) = v6;
    if ( (*v152__f) == 0 )
    {
      (*v149__f) = v6;
      W_wm_5b1030();
      goto LABEL_27;
    }
    if ( (*v152__f) != 1 )
    {
      if ( (*v152__f) == 2 )
      {
        if ( v6 - (*v151__f) <= v5 )
        {
          v5 -= 16;
          (*v150__f) = v5;
          if ( v5 <= 0x10 )
            (*v152__f) = 1;
        }
        else
        {
          v5 = 0;
          (*v152__f) = 1;
          (*v150__f) = 0;
        }
      }
      goto LABEL_28;
    }
    v7 = v6 - (*v149__f) + v5;
    if ( v7 > 0x64 )
      v7 = 100;
    v5 = v7;
    (*v149__f) = v6;
    (*v150__f) = v7;
    W_wm_5b1030();
    if ( v5 > 0x10 )
LABEL_27:
      (*v152__f) = 2;
LABEL_28:
    if ( (*v152__f) == 1 )
    {
      if ( v6 - (*TickCount__f) <= 0x3E8 )
      {
        ++(*v154__f);
      }
      else
      {
        (*v154__f) = 0;
        (*TickCount__f) = v6;
      }
    }
    else if ( (*v152__f) == 2 )
    {
      (*v151__f) = v6;
    }
    if ( (*v152__f) == 1 )
    {
      W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
      (*v139__f) = 0;
      if ( v159[0] == 0 )
        goto LABEL_143;
      v51 = v159;
      while ( 1 )
      {
        (*v115__f) = *(intptr_t *)(SYS32(10348) + 364);
        v52 = v51[7];
        if ( v52 > 0 && v52 <= 2 )
          break;
LABEL_141:
        v51 = &v159[10 * (uint16_t)++(*v139__f)];
        if ( *v51 == 0 )
        {
          (*v124__f) = v4;
LABEL_143:
          (*v140__f) = 0;
          if ( (*v163__f) != 0 )
          {
            (*v136__f) = &(*v163__f);
            v61 = 0;
            (*v156__f) = (*v144__f) / 100;
            while ( 2 )
            {
              (*v134__f) = 0;
              (*v116__f) = *(intptr_t *)(SYS32(10348) + 364);
              v62 = (*v136__f)[69];
              (*v130__f) = (*v136__f)[70];
              (*v137__f) = (*v136__f)[71];
              if ( (*v130__f) == 0 )
                (*v130__f) = 448;
              if ( (*v136__f)[71] == 0 )
                (*v137__f) = 64;
              v63 = 8 * (5 * v61 + 60) - (*v156__f);
              (*v132__f) = v63;
              if ( v63 < v62 || v63 >= (*v130__f) )
                goto LABEL_202;
              strcpy(v162, (uint8_t *)(*v136__f) + 4);
              if ( (*v136__f)[67] != 0 )
                v64 = 320 - W_rd_491820((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(v162), (intptr_t)(0xDu), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0)) / 2;
              else
                v64 = (*v136__f)[65] + 392;
              v65 = (*v132__f);
              (*v147__f) = v64;
              if ( (*v132__f) < (*v137__f) + v62 )
              {
                if ( (*v137__f) <= 0 || (v66 = 20 * ((*v132__f) - v62) / (*v137__f)) < 0 )
                {
                  v66 = 0;
                }
                else if ( v66 >= 20 )
                {
                  (*v134__f) = 0;
                  goto LABEL_161;
                }
                (*v134__f) = SYS32(4 * v66 + 10152);
              }
LABEL_161:
              if ( (*v132__f) >= (*v130__f) - (*v137__f) )
              {
                if ( (*v137__f) <= 0 || (v67 = 20 * ((*v130__f) - (*v132__f)) / (*v137__f)) < 0 )
                {
                  v67 = 0;
                }
                else if ( v67 >= 20 )
                {
                  (*v134__f) = 0;
                  goto LABEL_168;
                }
                (*v134__f) = SYS32(4 * v67 + 10152);
              }
LABEL_168:
              v68 = (*v136__f)[72];
              if ( v68 > 0 && v68 > (*v132__f) )
                v65 = (*v136__f)[72];
              v69 = 0;
              v70 = 0;
              (*v133__f) = (*v136__f)[66] + v65;
              (*v131__f) = 0;
              v71 = 0;
              while ( 1 )
              {
                v72 = strlen(v162) <= 0x28 ? strlen(v162) : 40;
                if ( (uint16_t)v71 >= v72 )
                  break;
                v73 = v162[(uint16_t)v71];
                if ( v73 == 32 )
                {
                  v74 = (uint16_t)v70++;
                  ++v71;
                  v161[v74] = 255;
                }
                else
                {
                  if ( v73 >= 33 && v73 <= 122 )
                  {
                    v75 = (uint16_t)v70++;
                    v161[v75] = v73 - 33;
                  }
                  ++v71;
                }
              }
              if ( (uint16_t)v70 != 0 )
              {
                (*v138__f) = v161;
                (*v120__f) = (uint16_t)v70;
                while ( 1 )
                {
                  v76 = *(*v138__f);
                  if ( *(*v138__f) == 255 )
                    goto LABEL_193;
                  v77 = *(_DWORD ***)&SYSB(2784);
                  if ( ***(_DWORD ***)&SYSB(2784) == 0 )
                    W_fl_4184a0((intptr_t)(SYS32(2784)), (intptr_t)(0));
                  v78 = F32((**v77 + 4 * v76), 0);
                  if ( v78 == 0 )
                  {
LABEL_193:
                    v69 += 8;
                    goto LABEL_194;
                  }
                  if ( (*v116__f) == 0 )
                    goto LABEL_192;
                  if ( (*v134__f) == 0 )
                    break;
                  W_rp_5bd550((intptr_t)((*v134__f)), (intptr_t)((*v116__f)), (intptr_t)(v78), (intptr_t)(F32(v78, 12) + (*v147__f) + (*v131__f) - F32(v78, 12)), (intptr_t)((*v133__f) + F32(v78, 16)), (intptr_t)(F32(v78, 20)), (intptr_t)(F32(v78, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v78, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
                  v69 = F32(v78, 20) + (*v131__f);
LABEL_194:
                  v79 = (*v120__f) == 1;
                  (*v131__f) = v69;
                  ++(*v138__f);
                  --(*v120__f);
                  if ( v79 )
                    goto LABEL_195;
                }
                W_rp_5b9b70((intptr_t)(v78), (intptr_t)((int)(*v116__f)), (intptr_t)((*v147__f) + v69 - F32(v78, 12)), (intptr_t)((*v133__f)));
LABEL_192:
                v69 += F32(v78, 20);
                goto LABEL_194;
              }
LABEL_195:
              if ( F16((*v136__f), 2 * 136) != 0 )
              {
                v80 = SYS32(4 * *((uint16_t *)(*v136__f) + 136) + 2732);
                v81 = F16((*v136__f), 2 * 137);
                if ( **(_DWORD **)((uint8_t *)(v80) + 0x0) == 0 )
                  W_fl_4184a0((intptr_t)(v80), (intptr_t)(0));
                v82 = F32((**(_DWORD **)((uint8_t *)(v80) + 0x0) + 4 * v81), 0);
                v83 = 320 - F32(v82, 28) / 2;
                if ( v82 != 0 )
                {
                  if ( (*v134__f) != 0 )
                    W_rp_5bd550((intptr_t)((*v134__f)), (intptr_t)((*v116__f)), (intptr_t)(v82), (intptr_t)(v83 + F32(v82, 12)), (intptr_t)((*v133__f) + F32(v82, 16)), (intptr_t)(F32(v82, 20)), (intptr_t)(F32(v82, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v82, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
                  else
                    W_rp_5b9b70((intptr_t)(v82), (intptr_t)((int)(*v116__f)), (intptr_t)(v83), (intptr_t)((*v133__f)));
                }
              }
LABEL_202:
              v61 = (uint16_t)++(*v140__f);
              (*v136__f) = &(*v163__f) + 73 * (uint16_t)(*v140__f);
              if ( *(*v136__f) == 0 )
                break;
              continue;
            }
          }
          RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
          v5 = (*v150__f);
          v4 = (int)(*v124__f);
          goto LABEL_15;
        }
      }
      v53 = 20 * v51[8] / 1000;
      if ( v53 >= 0 )
      {
        v54 = 20 * v51[8] / 1000;
        if ( v53 >= 20 )
        {
          (*v125__f) = 0;
LABEL_126:
          v55 = v51[3];
          if ( v55 != 0 )
          {
            v56 = SYS32(4 * v55 + 2732);
            if ( **(_DWORD **)((uint8_t *)(v56) + 0x0) == 0 )
              W_fl_4184a0((intptr_t)(v56), (intptr_t)(0));
            v57 = ***(_DWORD ***)((uint8_t *)(v56) + 0x0);
            if ( v57 != 0 )
            {
              v58 = 12;
              switch ( v51[4] )
              {
                case 0:
                  v4 = 12;
                  break;
                case 1:
                  goto LABEL_136;
                case 2:
                  v59 = F32(v57, 32);
                  v60 = 468;
                  goto LABEL_137;
                case 3:
                  v58 = 320 - F32(v57, 28) / 2;
LABEL_136:
                  v60 = 240;
                  v59 = F32(v57, 32) / 2;
LABEL_137:
                  v4 = v60 - v59;
                  break;
                default:
                  break;
              }
              v4 += v51[5] + v51[6] * v51[9] / 1000;
              if ( (*v125__f) != 0 )
                W_rp_5bd550((intptr_t)((*v125__f)), (intptr_t)((*v115__f)), (intptr_t)(v57), (intptr_t)(v58 + F32(v57, 12)), (intptr_t)(v4 + F32(v57, 16)), (intptr_t)(F32(v57, 20)), (intptr_t)(F32(v57, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v57, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
              else
                W_rp_5b9b70((intptr_t)(v57), (intptr_t)((int)(*v115__f)), (intptr_t)(v58), (intptr_t)(v4));
            }
          }
          goto LABEL_141;
        }
      }
      else
      {
        v54 = 0;
      }
      (*v125__f) = SYS32(4 * v54 + 10152);
      goto LABEL_126;
    }
    if ( (*v152__f) != 2 )
      continue;
    break;
  }
  if ( SYS32(0x2884) != 0 )
    W_sd_5bc0c0((intptr_t)(SYS32(0x2884)));
  v8 = SYS32(2728);
  if ( SYS32(2728) != 0 && F32(SYS32(2728), 8) != 0 )
  {
    v9 = 0;
    for ( i = API_GetTickCount(); v9 < F16(v8, 4); ++v9 )
      en_5646c0((intptr_t *)(F32(v8, 8) + 24 * v9), i);
  }
  W_id_5ba4c0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  W_id_5ba2d0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  v11 = (intptr_t *)&SYSB(10364);
  for ( j = 2; j != 0; --j )
  {
    if ( *v11 != 0 )
    {
      W_id_5ba650((intptr_t)(*v11));
      en_49bf90(*v11, 1000 * F32(CFG(), 1340) / 1000);
    }
    ++v11;
  }
  v13 = 11;
  v14 = F32(c, 0x4);
  v15 = (int *)v160;
  v16 = &F32(v14, 0x114);
  do
  {
    v17 = *v16;
    *v16 = 0;
    *v15++ = v17;
    ++v16;
    --v13;
  }
  while ( v13 != 0 );
  if ( F32(v14, 0x0) != 0 )
    W_input_dik_produce((intptr_t)(v14), (intptr_t)(SYS32(10360)));
  v18 = &SYSB(10364);
  v19 = ((uint8_t *)(v14) + 0x4);
  for ( (*k__f) = 2; (*k__f) != 0; --(*k__f) )
  {
    if ( F32(v19, 0) != 0 && F32(v18, 0) != 0 )
      W_gp_468b10((intptr_t)(v14), (intptr_t)(*(_DWORD **)v18), (intptr_t)(0));
    v19 += 4;
    v18 += 4;
  }
  v20 = API_GetTickCount();
  v21 = v14 + 320 * 1;
  v22 = v160;
  for ( m = 11; m != 0; --m )
  {
    if ( F32(v21 + (-1) * 0x140, 0x114) != 0 )
    {
      if ( *v22 == 0 )
        F32(v21, 0x0) = v20;
    }
    else
    {
      F32(v21, 0x0) = 0;
    }
    ++v22;
    v21 = ((uint8_t *)v21 + 4);
  }
  v24 = 63;
  v25 = F32(c, 0x4);
  v26 = (_DWORD *)(v25 + 264);
  while ( 2 )
  {
    v27 = (_DWORD *)*v26;
    if ( *(_DWORD *)*v26 != 34 || v27[2] != 1 || (*v155__f) - v27[1] > 0x64 )
    {
      --v24;
      --v26;
      if ( v24 >= 0 )
        continue;
      switch ( (*v145__f) )
      {
        case 0:
          if ( (*v129__f) >= 100 )
          {
            (*v129__f) = 0;
            (*v145__f) = 1;
          }
          else
          {
            ++(*v129__f);
          }
          goto LABEL_107;
        case 1:
          if ( (*v129__f) < 10 )
          {
            ++(*v129__f);
            goto LABEL_84;
          }
          (*v129__f) = 0;
          (*v145__f) = 2;
          if ( SYS32(0x2884) != 0
            && F32(CFG(), 372) != 0
            && (!W_sd_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0xBu))
             || W_sd_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0xBu)) != 0) )
          {
            F16(D(0x8a93e8), 0) = 0;
            W_sd_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
            if ( W_sd_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
            {
              if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
              {
                v28 = (SYS32(0x2884) + 0x2c);
                API_OutputDebugStringA((intptr_t)((SYS32(0x2884) + 0x2c)));
                API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
                API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
                v29 = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
                if ( v29 != 0 )
                {
                  API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
                  CRT_fprintf(
                    v29,
                    "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
                    (*SystemTime__f).wYear,
                    (*SystemTime__f).wMonth,
                    (*SystemTime__f).wDay,
                    (*SystemTime__f).wHour,
                    (*SystemTime__f).wMinute,
                    (*SystemTime__f).wSecond,
                    v28,
                    D(0x8a9b6c));
LABEL_83:
                  CRT_fclose(v29);
                }
              }
            }
            else if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
            {
              v30 = (SYS32(0x2884) + 0x2c);
              API_OutputDebugStringA((intptr_t)((SYS32(0x2884) + 0x2c)));
              API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
              API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
              v29 = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
              if ( v29 != 0 )
              {
                API_GetLocalTime((intptr_t)(&(*v143__f)));
                CRT_fprintf(
                  v29,
                  "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
                  (*v143__f).wYear,
                  (*v143__f).wMonth,
                  (*v143__f).wDay,
                  (*v143__f).wHour,
                  (*v143__f).wMinute,
                  (*v143__f).wSecond,
                  v30,
                  D(0x8a9b6c));
                goto LABEL_83;
              }
            }
          }
LABEL_84:
          v31 = (*v157__f) + (*v144__f);
          v32 = 0;
          v33 = (*v141__f) + 16;
          v34 = (*v158__f) < (*v157__f) + (*v144__f);
          (*v141__f) += 16;
          (*v144__f) = (*v158__f);
          if ( !v34 )
            (*v144__f) = v31;
          v35 = 1;
          if ( v159[0] != 0 )
          {
            v36 = v159;
            while ( 2 )
            {
              switch ( v36[7] )
              {
                case 0:
                  if ( v33 > v36[1] )
                    v36[7] = 1;
                  goto LABEL_100;
                case 1:
                  v37 = v36[8];
                  if ( v37 < 1000 )
                  {
                    v38 = v37 + 10;
                    if ( v38 > 1000 )
                      v38 = 1000;
                    v36[8] = v38;
                  }
                  if ( v33 > v36[1] + v36[2] )
                    v36[7] = 2;
                  goto LABEL_100;
                case 2:
                  v39 = v36[8];
                  if ( v39 <= 0 )
                    v36[7] = 3;
                  else
                    v36[8] = v39 - 10 < 0 ? 0 : v39 - 10;
                  goto LABEL_100;
                case 3:
                  goto LABEL_104;
                default:
LABEL_100:
                  v40 = v36[7];
                  if ( v40 > 0 && v40 <= 2 )
                    v36[9] = (1000 * v33 - 1000 * v36[1]) / (v36[2] + 1600);
                  v35 = 0;
LABEL_104:
                  v36 = &v159[10 * (uint16_t)++v32];
                  if ( *v36 != 0 )
                    continue;
                  if ( v35 != 0 )
                    goto LABEL_106;
                  goto LABEL_107;
              }
            }
          }
LABEL_106:
          (*v145__f) = 3;
LABEL_107:
          v41 = F32(c, 0x4);
          v42 = 63;
          v43 = (_DWORD *)(v41 + 264);
          while ( 1 )
          {
            v44 = (_DWORD *)*v43;
            if ( *(_DWORD *)*v43 != 0 && v44[2] == 1 && (*v155__f) - v44[1] <= 0x64 )
              break;
            --v42;
            --v43;
            if ( v42 < 0 )
              goto LABEL_14;
          }
          F32(F32(v41 + 4 * v42, 12), 0) = 0;
          v45 = F32(c, 0x4);
          v46 = 11;
          F16(v45, 364) = 0;
          v47 = (_DWORD *)(v45 + 276);
          do
          {
            v47[11] = 0;
            *v47++ = 0;
            --v46;
          }
          while ( v46 != 0 );
          v48 = (_DWORD **)(v45 + 12);
          for ( n = 64; n != 0; --n )
          {
            v50 = *v48++;
            *v50 = 0;
          }
          F32(v45, 268) = 0;
          F32(v45, 272) = 0;
          if ( (uint16_t)++(*v126__f) >= 5u )
            goto LABEL_205;
          break;
        case 2:
          goto LABEL_84;
        case 3:
          goto LABEL_205;
        default:
          goto LABEL_107;
      }
      goto LABEL_14;
    }
    break;
  }
  v84 = F32(v25, 4 * v24 + 12);
  (*v146__f) = 6;
  *v84 = 0;
LABEL_205:
  if ( SYS32(0x2884) != 0
    && F32(CFG(), 372) != 0
    && (!W_sd_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0)) || W_sd_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0)) != 0) )
  {
    F16(D(0x8a93e8), 0) = 0;
    W_sd_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
    if ( W_sd_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
    {
      if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
      {
        v85 = (SYS32(0x2884) + 0x2c);
        API_OutputDebugStringA((intptr_t)((SYS32(0x2884) + 0x2c)));
        API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
        API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
        v86 = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
        if ( v86 != 0 )
        {
LABEL_215:
          API_GetLocalTime((intptr_t)(&(*v143__f)));
          CRT_fprintf(
            v86,
            "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
            (*v143__f).wYear,
            (*v143__f).wMonth,
            (*v143__f).wDay,
            (*v143__f).wHour,
            (*v143__f).wMinute,
            (*v143__f).wSecond,
            v85,
            D(0x8a9b6c));
          CRT_fclose(v86);
        }
      }
    }
    else if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
    {
      v85 = (SYS32(0x2884) + 0x2c);
      API_OutputDebugStringA((intptr_t)((SYS32(0x2884) + 0x2c)));
      API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
      v86 = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
      if ( v86 != 0 )
        goto LABEL_215;
    }
  }
  W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  v87 = SYS32(10068);
  for ( ii = 0; ii < F16(v87, 4); F32(v89 + F32(v87, 0), 20) = 0 )
  {
    v89 = 28 * ii;
    v90 = F32(v89 + F32(v87, 0), 24);
    if ( v90 != 0 )
    {
      W_rp_5b9390((intptr_t)(F32(v89 + F32(v87, 0), 24)));
      g_eng.op_delete((void *)v90);
      F32(v89 + F32(v87, 0), 24) = 0;
    }
    ++ii;
    F32((v89 + F32(v87, 0)), 0) = 0;
    F32(v89 + F32(v87, 0), 4) = 0;
    F32(v89 + F32(v87, 0), 8) = 0;
    F32(v89 + F32(v87, 0), 12) = 0;
    F32(v89 + F32(v87, 0), 16) = 0;
  }
  F16(v87, 6) = 0;
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854780))));
  (*v117__f) = (intptr_t)&SYSB(2732);
  (*v127__f) = 909;
  do
  {
    v91 = F32(((uint8_t *)((*v117__f)) + 0x000), 0);
    if ( F32(((uint8_t *)((*v117__f)) + 0x000), 0) != 0 && F16(v91, 66) >= 4u )
    {
      v92 = 0;
      for ( (*jj__f) = 0; (uint16_t)v92 < F16(v91, 4); (*jj__f) = v92 )
      {
        if ( F32(v91, 0) != 0 )
        {
          v93 = (void **)(F32(v91, 0) + 8 * (uint16_t)v92);
          if ( *v93 != 0 )
          {
            for ( kk = 0; kk < F32(v91, 56); *((_DWORD *)*v93 + kk - 1) = 0 )
            {
              v95 = *((void **)*v93 + kk);
              if ( v95 != 0 )
              {
                W_rp_5b9390((intptr_t)(*((intptr_t *)*v93 + kk)));
                g_eng.op_delete((void *)v95);
              }
              ++kk;
            }
            g_eng.op_delete((void *)*v93);
            v92 = (*jj__f);
            *v93 = 0;
          }
        }
        ++v92;
      }
    }
    v79 = (*v127__f) == 1;
    (*v117__f) = (intptr_t)((uint8_t *)(*v117__f) + 4);
    --(*v127__f);
  }
  while ( !v79 );
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854770))));
  (*v118__f) = (intptr_t)&SYSB(868);
  (*v122__f) = 465;
  do
  {
    v96 = *(uint16_t **)((uint8_t *)((*v118__f)) + 0x000);
    if ( F32(((uint8_t *)((*v118__f)) + 0x000), 0) != 0 && v96[10] >= 4u && F32(v96, 4 * 1) != 0 )
    {
      for ( mm = *v96 - 1; mm >= 0; --mm )
      {
        v98 = F32(v96, 4 * 1);
        v99 = F32(v98, 8 * mm);
        if ( v99 != 0 )
        {
          W_sd_5bb3a0((intptr_t)(F32(v98, 8 * mm)));
          g_eng.op_delete((void *)v99);
          F32((F32(v96, 4 * 1) + 8 * mm), 0) = 0;
        }
      }
      g_eng.op_delete((void *)*((void **)v96 + 1));
      F32(v96, 4 * 1) = 0;
    }
    v79 = (*v122__f) == 1;
    (*v118__f) = (intptr_t)((uint8_t *)(*v118__f) + 4);
    --(*v122__f);
  }
  while ( !v79 );
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854760))));
  (*v119__f) = (intptr_t)&SYSB(10004);
  (*v123__f) = 16;
  do
  {
    v100 = F32(((uint8_t *)((*v119__f)) + 0x000), 0);
    if ( F32(((uint8_t *)((*v119__f)) + 0x000), 0) != 0 && F16(v100, 8) >= 4u && F32(v100, 0) != 0 )
    {
      for ( nn = 0; nn < F16(v100, 6); ++nn )
      {
        if ( F32((F32(v100, 0) + 4 * nn), 0) != 0 )
        {
          API_DeleteObject((intptr_t)(*(intptr_t *)(F32(v100, 0) + 4 * nn)));
          F32((F32(v100, 0) + 4 * nn), 0) = 0;
        }
      }
      g_eng.op_delete((void *)F32(v100, 0));
      F16(v100, 6) = 0;
      F16(v100, 4) = 0;
      F32(v100, 0) = 0;
    }
    v79 = (*v123__f) == 1;
    (*v119__f) = (intptr_t)((uint8_t *)(*v119__f) + 4);
    --(*v123__f);
  }
  while ( !v79 );
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854744))));
  v102 = c;
  v103 = F32(c, 0x0);
  (*v128__f) = F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    v104 = 0;
    for ( (*i1__f) = 0; (uint16_t)v104 < F16(v103, 4); (*i1__f) = v104 )
    {
      v105 = 4 * (uint16_t)v104;
      v106 = F32((F32(v103, 0) + v105), 0);
      if ( v106 != 0 )
      {
        if ( F32(v106, 72) != 0 )
        {
          for ( i2 = 0; i2 < F16(v106, 76); ++i2 )
          {
            v108 = F32(v106, 72);
            v109 = F32(v108, 4 * i2);
            if ( v109 != 0 )
            {
              en_5a3670(F32(v108, 4 * i2));
              g_eng.op_delete((void *)v109);
              F32((F32(v106, 72) + 4 * i2), 0) = 0;
            }
          }
          if ( F32(v106, 72) != 0 )
          {
            g_eng.op_delete((void *)F32(v106, 72));
            F32(v106, 72) = 0;
          }
          v103 = (*v128__f);
        }
        g_eng.op_delete((void *)(void *)v106);
        v104 = (*i1__f);
        v102 = c;
        F32((F32(v103, 0) + v105), 0) = 0;
      }
      ++v104;
    }
    if ( F32(v103, 0) != 0 )
    {
      g_eng.op_delete((void *)F32(v103, 0));
      F32(v103, 0) = 0;
    }
    g_eng.op_delete((void *)(void *)v103);
    F32(v102, 0x0) = 0;
  }
  v110 = (void **)F32(v102, 0x4);
  if ( v110 != 0 )
  {
    v111 = v110 + 3;
    for ( i3 = 64; i3 != 0; --i3 )
    {
      if ( *v111 != 0 )
      {
        g_eng.op_delete((void *)*v111);
        *v111 = 0;
      }
      ++v111;
    }
    g_eng.op_delete((void *)v110);
    F32(v102, 0x4) = 0;
  }
  return (*v146__f);
}

/* 0x5a3670: void __thiscall sub_5A3670(cls_5A3670 *this) */
void en_5a3670(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t * v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    void ** v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  v2 = (intptr_t *)F32(c, 0x170);
  if ( v2 != 0 )
  {
    v3 = *v2;
    if ( *v2 != 0 )
    {
      W_tm_49a6e0((intptr_t)(*v2));
      g_eng.op_delete((void *)v3);
      *(_DWORD *)F32(c, 0x170) = 0;
    }
  }
  if ( F32(c, 0x170) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x170));
    F32(c, 0x170) = 0;
  }
  if ( F32(c, 0x164) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x164));
    F32(c, 0x164) = 0;
  }
  if ( F32(c, 0x178) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x178));
    F32(c, 0x178) = 0;
  }
  if ( F32(c, 0x17c) != 0 )
  {
    v4 = F32(c, 0x174);
    (*v14__f) = 0;
    if ( F32(v4, 4) > 0 )
    {
      v5 = 0;
      do
      {
        v6 = 16 * v5;
        if ( F32(v6 + F32(c, 0x17c), 12) != 0 )
        {
          (*v13__f) = 0;
          if ( F32(v4, 8) > 0 )
          {
            v7 = 0;
            do
            {
              v8 = 24 * v7;
              v9 = F32(F32(v6 + F32(c, 0x17c), 12), 24 * v7);
              if ( v9 != 0 )
              {
                g_eng.op_delete((void *)*v9);
                g_eng.op_delete((void *)v9);
                F32((F32(v6 + F32(c, 0x17c), 12) + v8), 0) = 0;
              }
              v10 = F32(v6 + F32(c, 0x17c), 12);
              if ( F32(v10 + v8, 4) != 0 )
              {
                g_eng.op_delete((void *)F32(v10, v8 + 4));
                F32(F32((v6 + F32(c, 0x17c) + 12), 0) + v8, 4) = 0;
              }
              v11 = F32(c, 0x17c);
              if ( F32(F32((v6 + v11 + 12), 0) + v8, 8) != 0 )
              {
                g_eng.op_delete((void *)F32(F32(v6 + v11, 12), v8 + 8));
                F32(F32((v6 + F32(c, 0x17c) + 12), 0) + v8, 8) = 0;
              }
              v7 = (uint16_t)++(*v13__f);
            }
            while ( (uint16_t)(*v13__f) < F32(F32(c, 0x174), 8) );
          }
        }
        v12 = F32(c, 0x17c);
        if ( F32(v6 + v12, 12) != 0 )
        {
          g_eng.op_delete((void *)F32(v6, v12 + 12));
          F32(v6 + F32(c, 0x17c), 12) = 0;
        }
        v4 = F32(c, 0x174);
        v5 = (uint16_t)++(*v14__f);
      }
      while ( (uint16_t)(*v14__f) < F32(v4, 4) );
    }
    if ( F32(c, 0x17c) != 0 )
    {
      g_eng.op_delete((void *)(void *)F32(c, 0x17c));
      F32(c, 0x17c) = 0;
    }
  }
  if ( F32(c, 0x174) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x174));
    F32(c, 0x174) = 0;
  }
}

/* 0x5a3870: int __thiscall sub_5A3870(cls_5A3870 *this, _DWORD *a2) */
int32_t en_5a3870(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    char * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    uint32_t v9 = 0;
    int32_t * v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    uint32_t v18 = 0;
    uint32_t LastError = 0;
    uint32_t v20 = 0;
    intptr_t * v21 = 0;
    uint32_t v22 = 0;
    char * v23 = 0;
    char * v24 = 0;
    char * v25 = 0;
    char v26 = 0;
    uint32_t v27 = 0;
    intptr_t * v28 = 0;
    _Alignas(8) uint8_t frame_[0x220 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x20);   /* [256] at ebp-0x200 */
    char *Buffer = (char *)(frame_ + 0x120);   /* [256] at ebp-0x100 */
    intptr_t *const SystemTime__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x220 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x210 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x20c */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x208 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x204 */

  result = F32(c, 0x1800);
  (*v31__f) = c;
  (*v30__f) = 0;
  if ( result != 0 )
  {
    result = 0;
    do
    {
      v3 = 3 * result;
      v4 = *(_DWORD *)&GAP(c, 0x0, 4 * v3 + 8);
      v5 = &GAP(c, 0x0, 4 * v3);
      (*v32__f) = v4;
      if ( v4 != 0 )
      {
        v6 = F32(v5, 4 * 1);
        v7 = F32(v5, 0);
        v8 = a2[1536];
        v9 = 0;
        (*v33__f) = v6;
        if ( v8 == 0 )
          goto LABEL_10;
        v10 = a2;
        while ( *v10 != v7 )
        {
          ++v9;
          v10 += 3;
          if ( v9 >= v8 )
            goto LABEL_10;
        }
        v11 = &a2[3 * v9];
        if ( v11 == 0 )
        {
LABEL_10:
          v11 = 0;
          v12 = 0;
          if ( v8 == 0 )
            goto LABEL_16;
          v13 = 0;
          do
          {
            v14 = 3 * v13;
            v15 = a2[v14] == 0;
            v16 = &a2[v14];
            if ( v15 )
              v11 = v16;
            v13 = (uint16_t)++v12;
          }
          while ( (uint16_t)v12 < v8 );
          if ( v11 == 0 )
          {
LABEL_16:
            if ( v8 == 512 )
            {
              LastError = API_GetLastError();
              v20 = LastError;
              if ( LastError != 0 )
              {
                API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
                g_eng.h5bf3ee(OutputString, "[%08d] %s", v20, Buffer);
                g_eng.log_fail_406440(g_eng.log, (const char *)(OutputString), (const char *)(D(0x8a9b6c)));
              }
              F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
              if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
              {
                strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
                v22 = strlen(((char *)D(0x855040))) + 1;
                v23 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
                qmemcpy(v23, ((char *)D(0x855040)), 4 * (v22 >> 2));
                v25 = ((char *)D(0x855040) + (4 * (v22 >> 2)));
                v24 = &v23[4 * (v22 >> 2)];
                v26 = v22;
              }
              else
              {
                v27 = strlen(((char *)D(0x855040))) + 1;
                qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x855040)), 4 * (v27 >> 2));
                v25 = ((char *)D(0x855040) + (4 * (v27 >> 2)));
                v24 = &((char *)D(0x8a9530) + 0x404)[4 * (v27 >> 2) - 1024];
                v26 = v27;
              }
              qmemcpy(v24, v25, v26 & 3);
              strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
              g_eng.log_fail_406440(g_eng.log, (const char *)(((char *)D(0x855040))), (const char *)(D(0x8a9b6c)));
              g_eng.h5bf5db(0);
            }
            a2[1536] = v8 + 1;
            v11 = &a2[3 * v8];
          }
          v6 = (*v33__f);
          *v11 = v7;
          v11[2] = 0;
          v11[1] = 0;
        }
        v17 = (*v32__f);
        if ( (*v32__f) != 0 || v6 != 0 )
        {
          v11[1] = v6;
          v11[2] = v17;
        }
        else
        {
          *v11 = 0;
        }
        c = (*v31__f);
      }
      v18 = F32(c, 0x1800);
      result = (uint16_t)++(*v30__f);
    }
    while ( (uint16_t)(*v30__f) < v18 );
  }
  return result;
}

/* 0x5a4320: void *__thiscall sub_5A4320(cls_5A4320 *this, char a2) */
int32_t en_5a4320(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    void * v4 = 0;
    int32_t * v5 = 0;
    void ** v6 = 0;
    int32_t * v7 = 0;
    void ** v8 = 0;
    int32_t * v9 = 0;
    void ** v10 = 0;

  v2 = W;
  if ( W != 0 )
  {
    W_world_free((intptr_t)(W));
    g_eng.op_delete((void *)v2);
    *(intptr_t *)g_eng.world = 0;
  }
  v4 = *((void **)c + 58);
  if ( v4 != 0 )
  {
    en_56c640(F32(c, 4 * (58)));
    g_eng.op_delete((void *)v4);
    F32(c, 4 * 58) = 0;
  }
  v5 = F32(c, 4 * (71));
  if ( v5 != 0 )
  {
    v6 = (void **)v5[3];
    if ( v6 != 0 )
    {
      g_eng.op_delete((void *)*v6);
      g_eng.op_delete((void *)v6);
      v5[3] = 0;
    }
    g_eng.op_delete((void *)v5);
    F32(c, 4 * 71) = 0;
  }
  v7 = F32(c, 4 * (72));
  if ( v7 != 0 )
  {
    v8 = (void **)v7[3];
    if ( v8 != 0 )
    {
      g_eng.op_delete((void *)*v8);
      g_eng.op_delete((void *)v8);
      v7[3] = 0;
    }
    g_eng.op_delete((void *)v7);
    F32(c, 4 * 72) = 0;
  }
  v9 = F32(c, 4 * (73));
  if ( v9 != 0 )
  {
    v10 = (void **)v9[3];
    if ( v10 != 0 )
    {
      g_eng.op_delete((void *)*v10);
      g_eng.op_delete((void *)v10);
      v9[3] = 0;
    }
    g_eng.op_delete((void *)v9);
    F32(c, 4 * 73) = 0;
  }
  if ( F32(c, 4 * 56) != 0 )
  {
    g_eng.op_delete((void *)*((void **)c + 56));
    F32(c, 4 * 56) = 0;
  }
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return c;
}

/* 0x5a4440: void *__thiscall sub_5A4440(cls_5A4440 *this, char a2) */
int32_t en_5a4440(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;

  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return c;
}

/* 0x5a45c0: void **__thiscall sub_5A45C0(cls_5A45C0 *this, char a2) */
int32_t en_5a45c0(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    char * v7 = 0;
    intptr_t i = 0;
    uint8_t * v9 = 0;
    intptr_t j = 0;
    intptr_t v11 = 0;
    uint16_t k = 0;
    uint8_t * v13 = 0;
    intptr_t m = 0;
    intptr_t v15 = 0;
    ar_gdi_slot ** v16 = 0;
    intptr_t n = 0;
    ar_gdi_slot * v18 = 0;

  v3 = F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    W_roomloop_pools_free((intptr_t)(F32(c, 0x0)));
    v4 = F32(v3, 284);
    if ( v4 != 0 )
    {
      W_pool_obj_delete((intptr_t)(v4), (intptr_t)(1));
      F32(v3, 284) = 0;
    }
    v5 = F32(v3, 288);
    if ( v5 != 0 )
    {
      W_pool_obj_delete((intptr_t)(v5), (intptr_t)(1));
      F32(v3, 288) = 0;
    }
    v6 = F32(v3, 292);
    if ( v6 != 0 )
    {
      W_pool_obj_delete((intptr_t)(v6), (intptr_t)(1));
      F32(v3, 292) = 0;
    }
    if ( F32(v3, 224) != 0 )
    {
      g_eng.op_delete((void *)F32(v3, 224));
      F32(v3, 224) = 0;
    }
    g_eng.op_delete((void *)(void *)v3);
    F32(c, 0x0) = 0;
  }
  v7 = ((uint8_t *)(c) + 0x4);
  for ( i = 3; i != 0; --i )
  {
    if ( F32(v7, 0) != 0 )
    {
      g_eng.op_delete((void *)F32(v7, 0));
      F32(v7, 0) = 0;
    }
    v7 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x8a0bfc))));
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854780))));
  v9 = &SYSB(2732);
  for ( j = 909; j != 0; --j )
  {
    v11 = F32(v9, 0);
    if ( F32(v9, 0) != 0 && F16(v11, 66) >= 2u )
    {
      for ( k = 0; k < F16(v11, 4); ++k )
        W_slot_frames_free((intptr_t)(v11), (intptr_t)(k));
    }
    v9 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854770))));
  v13 = &SYSB(868);
  for ( m = 465; m != 0; --m )
  {
    v15 = F32(v13, 0);
    if ( F32(v13, 0) != 0 && F16(v15, 20) >= 2u )
      W_tm_562af0((intptr_t)(v15));
    v13 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854760))));
  v16 = (ar_gdi_slot **)&SYSB(10004);
  for ( n = 16; n != 0; --n )
  {
    v18 = *v16;
    if ( *v16 != 0 && v18->group >= 2u )
      W_tm_562a10((intptr_t)(v18));
    ++v16;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854744))));
  if ( F32(c, 0x10) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x10));
    F32(c, 0x10) = 0;
  }
  if ( F32(c, 0x14) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x14));
    F32(c, 0x14) = 0;
  }
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return (void **)c;
}
/* unported callees: 0x5b8fc0 */
