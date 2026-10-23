/* actor_behave.c — the ACTOR behaviours the per-tick actor update 0x54f980 dispatches by code that the corpus does not reach
 * (doors / lifts / NPC-like props: 0x553520 (a scripted mover reading its route from a data file), 0x554c40, 0x553ec0,
 * 0x554570 (a sound-emitting prop), 0x552c70, 0x552f60, 0x54f140, 0x5556d0 (+ 0x5554e0)).  See actor_behave.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  UNVERIFIED: no recording reaches
 * these codes yet; REPLACE-wired, compile-checked. */
#include "actor_behave.h"

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

#include "actor_ops.h"
#include "actor_update.h"
#include "char_attack.h"
#include "char_physics.h"
#include "entity_fx.h"
#include "entity_life.h"
#include "entity_move.h"
#include "gimmick.h"
#include "npc_ai.h"
#include "particle_spawn.h"
#include "sound_dev.h"
#include "sound_fx.h"
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

static inline intptr_t W_actor_by_handle(intptr_t a0, ...) { return (intptr_t)actor_by_handle((int32_t)a0); }
static inline intptr_t W_body_hit_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, intptr_t a20, intptr_t a21, intptr_t a22, intptr_t a23, ...) { return (intptr_t)body_hit_spawn((wl_char *)a0, (wl_char *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19, (int32_t)a20, (int32_t)a21, (int32_t)a22, (int32_t)a23); }
static inline intptr_t W_char_in_rect(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_in_rect((uint16_t)a0, (const int32_t *)a1, (int32_t)a2); }
static inline intptr_t W_char_part_top(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_part_top((const wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_char_riding(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_riding((const wl_actor *)a0, (int32_t)a1, (uint16_t *)a2); }
static inline intptr_t W_entity_carry_riders(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { entity_carry_riders((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3); return 0; }
static inline intptr_t W_entity_count_riders(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)entity_count_riders((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (uint16_t *)a4, (int32_t)a5); }
static inline intptr_t W_entity_move_horizontal(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_move_horizontal((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (uint16_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_entity_snd_keep(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_snd_keep((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2); return 0; }
static inline intptr_t W_entity_snd_loop(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { entity_snd_loop((wl_entity *)a0, (uint16_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8); return 0; }
static inline intptr_t W_entity_snd_start(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_snd_start((wl_entity *)a0, (uint16_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_part_bottom(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_bottom((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_sd_5bb250(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sd_5bb250((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_sd_5bb2f0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sd_5bb2f0((wl_char *)a0, (int32_t *)a1, (int32_t)a2); }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_speed_ramp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)speed_ramp((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_vpool_load(intptr_t a0, ...) { return (intptr_t)vpool_load((wl_vpool *)a0); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }

typedef intptr_t (__attribute__((stdcall)) *api_GetLocalTime_t)(intptr_t);
#define API_GetLocalTime ((api_GetLocalTime_t)g_eng.api("GetLocalTime"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fprintf ((intptr_t (*)())g_eng.retail(0x5bf496u))   /* retail's CRT fprintf (cdecl) */
int32_t ab_54f140(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ab_552c70(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t ab_552f60(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t ab_553520(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ab_553ec0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ab_554570(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ab_554c40(wl_char *self, int32_t a2, FILE * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ab_5554e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ab_5556d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

/* 0x54f140: int __thiscall sub_54F140(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t ab_54f140(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    intptr_t v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    uint16_t v24 = 0;
    uint16_t v25 = 0;
    uint16_t v26 = 0;
    uint16_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    int32_t * v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    FILE * v55 = 0;
    int32_t * v56 = 0;
    intptr_t ** v57 = 0;
    intptr_t * v58 = 0;
    intptr_t v59 = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    _Alignas(8) uint8_t frame_[0x50 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x50 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x34 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x1c */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x18 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x14 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x40);   /* ebp-0x10 */

  (*v62__f) = 19200;
  if ( a5 == 0 )
  {
    F16(a2, 88) = 0;
    F32(a2, 92) = 0;
    return a2;
  }
  v8 = F32(c, 0x2cc);
  if ( v8 != 0 )
    v69 = (v8 << 6) / 1000;
  else
    v69 = 128;
  v9 = F32(c, 0x2d0);
  if ( v9 != 0 )
    v10 = (v9 << 6) / 1000;
  else
    v10 = 128;
  v11 = F32(c, 0x2d8);
  if ( v11 != 0 )
    (*v64__f) = (v11 << 6) / 1000;
  else
    (*v64__f) = 64;
  result = a2;
  v12 = F16(a2, 88);
  if ( (uint16_t)v12 != 0 )
  {
    if ( v12 != 1 )
      goto LABEL_21;
    v13 = F32(a2, 92);
    if ( v13 < v10 && v9 != 1 )
    {
      F32(a2, 92) = v13 + 1;
      goto LABEL_21;
    }
  }
  else
  {
    v14 = F32(a2, 92);
    if ( v14 < v69 && v8 != 1 )
    {
      F32(a2, 92) = v14 + 1;
      goto LABEL_21;
    }
  }
  F32(a2, 92) = 0;
  F16(a2, 88) = 2;
LABEL_21:
  if ( F16(a2, 88) == 2 )
  {
    v15 = F32(a2, 92);
    if ( F32(c, 0x2d8) == 1 )
    {
      if ( v15 < 128 )
        F32(a2, 92) = v15 + 1;
    }
    else if ( v15 >= (*v64__f) )
    {
      F32(a2, 92) = 0;
      F16(a2, 88) = 1;
    }
    else
    {
      F32(a2, 92) = v15 + 1;
    }
    if ( a3 == 0 )
    {
      v16 = F16(c, 0x2f4);
      if ( v16 != 0 )
      {
        F16(c, 0x2f4) = v16 - 1;
      }
      else
      {
        (*v64__f) = 0;
        do
        {
          v17 = *(_DWORD *)&GAP(W, 0x1030, 4 * (uint16_t)(*v64__f) + 432);
          v69 = v17;
          if ( F32(v17, 464) != 0 && F32(v17, 264) == 3 )
          {
            v18 = a4;
            if ( (uint16_t)a4 >= F16(c, 0x44) )
              v18 = 0;
            v19 = F32(c, 0x40);
            v20 = 165 * v18;
            v21 = a4;
            v22 = F32(v19 + 4 * v20, 4);
            if ( (uint16_t)a4 >= F16(v69, 68) )
              v21 = 0;
            v23 = F32(v69, 64);
            if ( v22 <= F32(v23 + 660 * v21, 4) + F32(v23 + 660 * v21, 12) )
            {
              v24 = a4;
              if ( (uint16_t)a4 >= F16(c, 0x44) )
                v24 = 0;
              v25 = a4;
              if ( (uint16_t)a4 >= F16(v69, 68) )
                v25 = 0;
              if ( F32(v19 + 660 * v24, 4) + F32(v19 + 660 * v24, 12) > F32(v23 + 660 * v25, 4) )
              {
                v26 = a4;
                if ( (uint16_t)a4 >= F16(c, 0x44) )
                  v26 = 0;
                v27 = a4;
                if ( (uint16_t)a4 >= F16(v69, 68) )
                  v27 = 0;
                if ( F32(v19 + 660 * v26, 8) + F32(v19 + 660 * v26, 20) <= F32(v23 + 660 * v27, 16)
                                                                                           + F32(v23 + 660 * v27, 8)
                                                                                           - 1 )
                {
                  v28 = W_char_part_top((intptr_t)(v69), (intptr_t)(a4));
                  if ( W_part_bottom((intptr_t)(c), (intptr_t)(a4)) > v28 )
                  {
                    F16(a2, 88) = 0;
                    F32(a2, 92) = 0;
                    F32(c, 0x2f0) = 1;
                    goto LABEL_54;
                  }
                }
              }
            }
          }
          ++(*v64__f);
        }
        while ( (uint16_t)(*v64__f) < 0x80u );
        F32(c, 0x2f0) = 0;
LABEL_54:
        result = a2;
        F16(c, 0x2f4) = 20;
      }
    }
    if ( F32(c, 0x2f0) == 0 )
    {
      switch ( F32(c, 0x2e4) )
      {
        case 0:
          v29 = F32(c, 0x2bc);
          v30 = F32(c, 0x2c0);
          if ( v29 == v30 )
          {
            v31 = 19200;
          }
          else
          {
            v31 = v30 - v29 + 4800;
            (*v62__f) = v31;
          }
          v32 = 800 * F32(result, 92);
          if ( v31 < v32 )
            v32 = v31;
          v33 = 0;
          a4 = 3200;
          v69 = v32;
          (*v64__f) = 0;
          v34 = 3200 - v32;
          (*v63__f) = 9;
          break;
        case 1:
          v35 = F32(c, 0x2bc);
          v36 = F32(c, 0x2c0);
          if ( v35 == v36 )
          {
            v37 = 19200;
          }
          else
          {
            v37 = v36 - v35;
            (*v62__f) = v37;
          }
          v69 = v37;
          v38 = 800 * F32(result, 92);
          if ( v37 >= v38 )
            v69 = v38;
          v34 = 0;
          a4 = 3200;
          (*v64__f) = 0;
          v33 = 180;
          (*v63__f) = 10;
          break;
        case 2:
          v39 = F32(c, 0x2b4);
          v40 = F32(c, 0x2b8);
          if ( v39 == v40 )
          {
            v41 = 19200;
          }
          else
          {
            v41 = v40 - v39;
            (*v62__f) = v41;
          }
          v42 = 800 * F32(result, 92);
          if ( v41 < v42 )
            v42 = v41;
          a4 = v42;
          v69 = 3200;
          (*v64__f) = v42 - 4800;
          v34 = 0;
          v33 = 90;
          (*v63__f) = 7;
          break;
        case 3:
          v43 = F32(c, 0x2b4);
          v44 = F32(c, 0x2b8);
          if ( v43 == v44 )
          {
            v45 = 19200;
          }
          else
          {
            v45 = v44 - v43 + 4800;
            (*v62__f) = v45;
          }
          a4 = v45;
          v46 = 800 * F32(result, 92);
          if ( v45 >= v46 )
            a4 = v46;
          v34 = 0;
          v69 = 3200;
          (*v64__f) = 0;
          v33 = 270;
          (*v63__f) = 7;
          break;
        default:
          v34 = a2;
          v33 = a2;
          break;
      }
      F32(a2, 52) = 1000 * v33;
      W_body_hit_spawn((intptr_t)(c), (intptr_t)(0), (intptr_t)(a2), (intptr_t)((*v64__f)), (intptr_t)(v34), (intptr_t)(a4), (intptr_t)(v69), (intptr_t)((*v63__f)), (intptr_t)(a3), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(9), (intptr_t)(a6 * F32(F32(c, 0x2e0), 4) / 1000), (intptr_t)(1), (intptr_t)(1), (intptr_t)(F32(c, 0x2a4)), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      result = a3;
      if ( a3 == 0 )
      {
        v66 = *(intptr_t *)&GAP(W, 0x1030, 28);
        v48 = F32(a2, 4) - *(_DWORD *)((uint8_t *)(v66 + (4) * 0x14) + 0x10) - F32(v66 + (2) * 0x14, 0xc);
        result = F32(a2, 8) - 100 * *(_DWORD *)((uint8_t *)(v66 + (5) * 0x14) + 0x10) - F32(v66 + (4) * 0x14, 0xc) - *(_DWORD *)((uint8_t *)(v66 + (3) * 0x14) + 0x10);
        if ( (*v62__f) + v48 + F32(a2, 12) >= 0
          && (*v62__f) + result + F32(a2, 16) >= 0
          && *(_DWORD *)((uint8_t *)(v66 + (5) * 0x14) + 0x0) > v48 - (*v62__f) )
        {
          result -= (*v62__f);
          if ( F32(v66 + (5) * 0x14, 0x4) > result )
          {
            v49 = ((*v62__f) - 3200) / 800 < 0 ? 0 : ((*v62__f) - 3200) / 800;
            (*v61__f) = v33 - 17 * rnd() / 0x8000 + 8;
            (*v60__f) = 800 * rnd() / 0x8000 - 400;
            v50 = rnd();
            v51 = (_DWORD *)W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100102), (intptr_t)((_DWORD *)a2), (intptr_t)(800 * v50 / 0x8000 - 400), (intptr_t)((*v60__f)), (intptr_t)(99), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)((*v61__f)), (intptr_t)(10));
            if ( v51 != 0 )
            {
              v52 = v51[117];
              v51[151] = v49;
              if ( v52 == 100044 )
              {
                v53 = SYS32(4 * v49 + 10072);
                v51[61] = v53;
                v51[62] = v53;
              }
            }
            v54 = 1000 * rnd();
            result = F32(c, 0x1b0);
            if ( result != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
              {
                v56 = (_DWORD *)(result + 32);
                v68 = result + 32;
                if ( F32(result, 32) == 0
                  && W_sfx_place((intptr_t)(c), (intptr_t)(0), (intptr_t)(-5000), (intptr_t)(0), (intptr_t)((_DWORD *)F32(c, 0x40)), (intptr_t)(0), (intptr_t)(0)) != 0 )
                {
                  v57 = (intptr_t **)F32(c, 0x1b0);
                  v67 = SYS32(1788);
                  if ( *v57 != 0 && v57 != 0 )
                  {
                    if ( F16(c, 0x1b4) != 0 )
                    {
                      if ( *v57 != 0 )
                      {
                        (*v57)[1] = 0;
                        *v57 = 0;
                        v57[1] = 0;
                        v57[8] = 0;
                      }
                    }
                    else
                    {
                      g_eng.h408b90(D(0x8a9530), ((char *)D(0x85525c)), 0, D(0x8a9b6c));
                    }
                  }
                  v58 = (intptr_t *)W_vpool_take((intptr_t)(v67), (intptr_t)(1));
                  if ( v58 != 0 )
                  {
                    *v57 = v58;
                    v57[1] = (intptr_t *)v67;
                    v57[2] = 0;
                    v57[3] = (intptr_t *)(v54 / 0x8000 + 500);
                    v57[4] = (intptr_t *)-5000;
                    v57[5] = (intptr_t *)1;
                    v57[6] = 0;
                    v57[7] = 0;
                    g_eng.dsb_volume_5bb870(*v58, -10000);
                    g_eng.dsb_play_5bb8c0(*v58);
                  }
                  v56 = (_DWORD *)v68;
                }
                v59 = F32(c, 0x1b0);
                if ( v59 != 0 )
                {
                  if ( F16(c, 0x1b4) != 0 )
                    F32(v59, 20) = 1;
                  else
                    g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
                }
                result = *v56 + 1;
                *v56 = result;
                if ( result >= 20 )
                  *v56 = 0;
              }
              else
              {
                API_OutputDebugStringA((intptr_t)(((char *)D(0x85525c))));
                API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
                API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
                result = (int)CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
                v55 = result;
                if ( result != 0 )
                {
                  API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
                  CRT_fprintf(
                    v55,
                    "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
                    (*SystemTime__f).wYear,
                    (*SystemTime__f).wMonth,
                    (*SystemTime__f).wDay,
                    (*SystemTime__f).wHour,
                    (*SystemTime__f).wMinute,
                    (*SystemTime__f).wSecond,
                    ((char *)D(0x85525c)),
                    D(0x8a9b6c));
                  return CRT_fclose(v55);
                }
              }
            }
          }
        }
      }
    }
  }
  return result;
}

/* 0x552c70: int __thiscall sub_552C70(cls_4022A0 *this, _DWORD *a2, int a3, int a4) */
int32_t ab_552c70(wl_char *self, int32_t * a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    int32_t * v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t i = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t * v16 = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t * v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t result = 0;
    intptr_t v27 = 0;
    intptr_t v33 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    int32_t * *const v30__f = (int32_t * *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  v4 = a2;
  v6 = a2[2] - F32(c, 0x2bc);
  (*v28__f) = 0;
  if ( F32(c, 0xc) != 0 )
  {
    if ( F32(c, 0xc) == 1 )
    {
      v7 = a4;
      v8 = 19200;
      (*v28__f) = -19200;
    }
    else
    {
      v8 = (int)a2;
      v7 = (int)a2;
    }
  }
  else
  {
    v8 = 0;
    v7 = -a4;
  }
  if ( v6 == v8 )
  {
    a2[6] = 0;
    goto LABEL_48;
  }
  v9 = a2[6];
  if ( v7 < 0 )
  {
    if ( v7 < v9 )
    {
      v9 -= 200;
      if ( v7 > v9 )
        v9 = v7;
      goto LABEL_19;
    }
    if ( v7 > v9 )
    {
      v10 = v9 + 200;
      v11 = v10 > 0;
      goto LABEL_18;
    }
  }
  else
  {
    if ( v7 > v9 )
    {
      v9 += 200;
      if ( v7 < v9 )
        v9 = v7;
      goto LABEL_19;
    }
    if ( v7 < v9 )
    {
      v10 = v9 - 200;
      v11 = v10 < 0;
LABEL_18:
      v9 = v11 ? 0 : v10;
    }
  }
LABEL_19:
  a2[6] = v9;
  v12 = v6 + v9 / 100;
  if ( v12 > 19200 )
  {
    v33 = 19200;
  }
  else if ( v12 >= 0 )
  {
    v33 = v6 + v9 / 100;
  }
  else
  {
    v33 = 0;
  }
  if ( v33 == v8 )
  {
    if ( a3 == 0 )
    {
      for ( i = 20; i != 0; --i )
      {
        v14 = a2[3] + 1600;
        v15 = rnd();
        W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)(a2), (intptr_t)(v14 * v15 / 0x8000 - v14 / 2), (intptr_t)((*v28__f)), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
      }
    }
    (*v30__f) = (_DWORD *)F32(c, 0x40);
    if ( a3 == 0 )
    {
      v16 = SYS32(1016);
      if ( F32(SYS32(1016), 8) != 0
        && (F32(SYS32(1016), 4) != 0
         || W_vpool_load((intptr_t)(SYS32(1016))) != 0) )
      {
        v17 = 0;
        (*v29__f) = 0;
        if ( *v16 != 0 )
        {
          v18 = F32(v16, 4 * 1);
          SETHI(v19, 0);
          while ( 1 )
          {
            SETLO(v19, v16[1]);
            v20 = (v19 + v17) % *v16;
            if ( F32(v18 + 8 * (uint16_t)v20, 4) == 0 )
              break;
            v17 = ++(*v29__f);
            if ( (uint16_t)(*v29__f) >= *v16 )
              goto LABEL_41;
          }
          v21 = v20 + 1;
          v22 = 8 * (uint16_t)v20;
          v16[1] = v21;
          F32(v22 + v18, 4) = 0;
          v23 = (intptr_t *)(v22 + F32(v16, 4 * 1));
          if ( v23 != 0 )
          {
            v24 = *v23;
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) + 2000), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v30__f)), (intptr_t)(&(*v31__f)), (intptr_t)(&(*v32__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v24, (*v31__f));
              g_eng.dsb_pan_5bb880(v24, (*v32__f));
              g_eng.dsb_freq_5bb890(v24, 0);
              g_eng.dsb_play_5bb8c0(v24);
            }
          }
LABEL_41:
          v4 = a2;
        }
      }
    }
    v4[6] = 0;
  }
  if ( a3 == 0 )
  {
    v25 = F32(c, 0x1b0);
    if ( v25 != 0 )
    {
      if ( F16(c, 0x1b4) != 0 )
      {
        if ( F32(v25, 32) == 0 )
          W_entity_snd_start((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xECu), (intptr_t)(-1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        W_entity_snd_keep((intptr_t)(c), (intptr_t)(0), (intptr_t)(0));
        v27 = F32(v25, 32) + 1;
        F32(v25, 32) = v27;
        if ( v27 >= 16 )
          F32(v25, 32) = 0;
      }
      else
      {
        g_eng.h408b90(D(0x8a9530), ((char *)D(0x85525c)), 0, D(0x8a9b6c));
      }
    }
  }
  v6 = v33;
LABEL_48:
  result = F32(c, 0x2bc);
  v4[2] = result + v6;
  return result;
}

/* 0x552f60: int __thiscall sub_552F60(cls_4022A0 *this, int a2, int a3, int a4, int a5, _DWORD *a6) */
int32_t ab_552f60(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t result = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t * v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t * v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v6 = a2;
  result = (uint16_t)a6;
  v9 = F32(a2, 16);
  (*v26__f) = (uint16_t)a6;
  if ( v9 == (uint16_t)a6 )
    return result;
  v10 = F32(a2, 24);
  if ( a4 < 0 )
  {
    if ( a4 < v10 )
    {
      v10 -= 800;
      if ( a4 > v10 )
        v10 = a4;
      goto LABEL_14;
    }
    if ( a4 > v10 )
    {
      v11 = v10 + 800;
      v12 = v11 > 0;
      goto LABEL_13;
    }
  }
  else
  {
    if ( a4 > v10 )
    {
      v10 += 800;
      if ( a4 < v10 )
        v10 = a4;
      goto LABEL_14;
    }
    if ( a4 < v10 )
    {
      v11 = v10 - 800;
      v12 = v11 < 0;
LABEL_13:
      v10 = v12 ? 0 : v11;
    }
  }
LABEL_14:
  F32(a2, 24) = v10;
  if ( v9 == (uint16_t)a5 )
  {
    a6 = (_DWORD *)F32(c, 0x40);
    if ( a3 == 0 )
    {
      v13 = SYS32(1748);
      if ( F32(SYS32(1748), 8) != 0
        && (F32(SYS32(1748), 4) != 0
         || W_vpool_load((intptr_t)(SYS32(1748))) != 0) )
      {
        v14 = 0;
        if ( *v13 != 0 )
        {
          v15 = F32(v13, 4 * 1);
          SETHI(v16, 0);
          while ( 1 )
          {
            SETLO(v16, v13[1]);
            v17 = (v16 + v14) % *v13;
            if ( F32(v15 + 8 * (uint16_t)v17, 4) == 0 )
              break;
            if ( ++v14 >= *v13 )
              goto LABEL_27;
          }
          v18 = v17 + 1;
          v19 = 8 * (uint16_t)v17;
          v13[1] = v18;
          F32(v19 + v15, 4) = 0;
          v20 = (intptr_t *)(F32(v13, 4 * 1) + v19);
          if ( v20 != 0 )
          {
            v21 = *v20;
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(&a4), (intptr_t)(&a5)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v21, a4);
              g_eng.dsb_pan_5bb880(v21, a5);
              g_eng.dsb_freq_5bb890(v21, 0);
              g_eng.dsb_play_5bb8c0(v21);
            }
          }
LABEL_27:
          v6 = a2;
        }
      }
    }
  }
  v22 = F32(v6, 16) + F32(v6, 24) / 100;
  if ( v22 > 12800 )
  {
    v22 = 12800;
  }
  else if ( v22 < 3200 )
  {
    v22 = 3200;
  }
  result = (*v26__f);
  F32(v6, 16) = v22;
  if ( v22 == (*v26__f) )
  {
    result = a3;
    v23 = (_DWORD *)F32(c, 0x40);
    if ( a3 == 0 )
    {
      result = W_vpool_take((intptr_t)(SYS32(1752)), (intptr_t)(0));
      if ( result != 0 )
      {
        v24 = F32(result, 0);
        result = W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v23), (intptr_t)(&a2), (intptr_t)(&a3));
        if ( result != 0 )
        {
          g_eng.dsb_volume_5bb870(v24, a2);
          g_eng.dsb_pan_5bb880(v24, a3);
          g_eng.dsb_freq_5bb890(v24, 0);
          result = g_eng.dsb_play_5bb8c0(v24);
        }
      }
    }
    F32(v6, 24) = 0;
  }
  return result;
}

/* 0x553520: int __thiscall sub_553520(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t ab_553520(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint32_t v11 = 0;
    intptr_t v12 = 0;
    int v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    uint16_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    uint16_t * v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    uint16_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t * v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t i = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    _Alignas(8) uint8_t frame_[0x160 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x60);   /* [256] at ebp-0x100 */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x160 */
    intptr_t *const Stream__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x12c */
    FILE * *const Streamc__f = (FILE * *)(frame_ + 0x34);   /* ebp-0x12c */
    FILE * *const Streamb__f = (FILE * *)(frame_ + 0x34);   /* ebp-0x12c */
    FILE * *const Streama__f = (FILE * *)(frame_ + 0x34);   /* ebp-0x12c */
    intptr_t *const v72__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x128 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x128 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x124 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x120 */
    int32_t * *const v75__f = (int32_t * *)(frame_ + 0x44);   /* ebp-0x11c */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x118 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x114 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x50);   /* ebp-0x110 */

  (*Stream__f) = 0;
  (*v71__f) = 0;
  v8 = F32(a2, 12);
  v9 = F32(a2, 4);
  (*v74__f) = F32(a2, 44);
  v10 = *(_DWORD *)&GAP(W, 0x1030, 24);
  v11 = (int)((uint64_t)(1374389535LL * (v9 + v8 / 2)) >> 32) >> 10;
  v12 = F32(a2, 8) + 2400;
  v13 = (int)((v11 >> 31) + v11) < 0;
  v14 = (v11 >> 31) + v11;
  if ( F32(v10, 2887728) - 1 >= (v13 ? 0 : v14) )
    v15 = v14 < 0 ? 0 : v14;
  else
    v15 = F32(v10, 2887728) - 1;
  (*v73__f) = v12 / 3200;
  v16 = F32(v10, 2887732) - 1;
  if ( v16 >= (v12 / 3200 < 0 ? 0 : v12 / 3200) )
    v16 = (*v73__f) < 0 ? 0 : (*v73__f);
  v17 = F32((v10 + 12 * ((v15 << 7) + v16 + 138244)), 0);
  if ( v17 == 1 )
    v17 = v12 % 3200 > 1600;
  v18 = 0;
  F32(a2, 92) = (F32(a2, 92) + 1) % 100;
  if ( v17 == 0 )
  {
    if ( a7 != 0 )
    {
      if ( a3 == 0 )
      {
        if ( F32(c, 0x3c) != 0 )
        {
          if ( F32(c, 0x2c4) == 0 )
            F32(c, 0x2c4) = 60;
          if ( F32(c, 0x2c4) == 50 )
          {
            v52 = SYS32(1780);
            (*v75__f) = (_DWORD *)F32(c, 0x40);
            if ( F32(SYS32(1780), 8) != 0 )
            {
              if ( F32(SYS32(1780), 4) == 0 )
              {
                if ( W_vpool_load((intptr_t)(SYS32(1780))) == 0 )
                  goto LABEL_124;
                v18 = 0;
              }
              (*Streamc__f) = 0;
              if ( *v52 != 0 )
              {
                v53 = F32(v52, 4 * 1);
                v54 = v52[1];
                v55 = *v52;
                (*v74__f) = v55;
                (*v72__f) = v53;
                while ( 1 )
                {
                  v56 = (v54 + v18) % v55;
                  if ( F32((*v72__f) + 8 * (uint16_t)v56, 4) == 0 )
                    break;
                  v18 = (uint16_t)(*Streamc__f) + 1;
                  (*Streamc__f) = ((uint8_t *)(*Streamc__f) + 1);
                  if ( (uint16_t)(*Streamc__f) >= *v52 )
                    goto LABEL_124;
                  v55 = (*v74__f);
                }
                v57 = v56 + 1;
                v58 = 8 * (uint16_t)v56;
                v52[1] = v57;
                F32(v58 + (*v72__f), 4) = 0;
                v59 = (intptr_t *)(v58 + F32(v52, 4 * 1));
                if ( v59 != 0 )
                {
                  v60 = *v59;
                  if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v75__f)), (intptr_t)(&(*v77__f)), (intptr_t)(&(*v76__f))) != 0 )
                  {
                    g_eng.dsb_volume_5bb870(v60, (*v77__f));
                    g_eng.dsb_pan_5bb880(v60, (*v76__f));
                    g_eng.dsb_freq_5bb890(v60, 0);
                    g_eng.dsb_play_5bb8c0(v60);
                  }
                }
              }
            }
          }
          else
          {
            F32(c, 0x3c) = 0;
          }
        }
        else
        {
          F32(c, 0x2c4) = 0;
        }
      }
    }
    else
    {
      F32(a2, 40) = 0;
      v61 = F32(a2, 24) + 2000;
      if ( v61 > 80000 )
        v61 = 80000;
      F32(a2, 24) = v61;
      if ( W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)a2), (intptr_t)(v61 / 100), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(a2 + 8), (intptr_t)(0)) != 0 )
        goto LABEL_125;
      if ( a3 == 0 && F32(a2, 24) >= 20000 )
      {
        for ( i = 20; i != 0; --i )
        {
          v63 = F32(a2, 12) + 1600;
          v64 = rnd();
          W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)a2), (intptr_t)(v63 * v64 / 0x8000 - v63 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
        }
        W_sfx_play_at((intptr_t)(c), (intptr_t)(0), (intptr_t)(37), (intptr_t)(2000), (intptr_t)(0), (intptr_t)(0));
      }
    }
LABEL_124:
    F32(a2, 24) = 0;
LABEL_125:
    F32(c, 0x108) = 3;
    return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(a2, 4) - a5), (intptr_t)(F32(a2, 8) - a6), (intptr_t)(a4));
  }
  F32(c, 0x108) = 2;
  v19 = F32(a2, 24);
  if ( v19 > 5000 )
    F32(a2, 24) = v19 - 1000;
  v20 = F32(c, 0xc);
  if ( v20 != 0 )
  {
    if ( v20 == 1 )
    {
      (*v71__f) = 1;
      v21 = F32(a2, 24) + 500;
      if ( v21 > 10000 )
        v21 = 10000;
      F32(a2, 24) = v21;
    }
  }
  else if ( v17 == 2 )
  {
    if ( F32(a2, 92) % 4 == 0 )
    {
      (*v66__f) = 800 * rnd() / 0x8000 - 800;
      v22 = rnd();
      W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100105), (intptr_t)((_DWORD *)a2), (intptr_t)(24000 * v22 / 0x8000 - 12000), (intptr_t)((*v66__f)), (intptr_t)(99), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
    v23 = F32(a2, 24) - 500;
    if ( v23 < -20000 )
      v23 = -20000;
    F32(a2, 24) = v23;
    (*v71__f) = 1;
  }
  else if ( F16(a2, 88) != 0 )
  {
    if ( F16(a2, 88) == 1 )
    {
      v24 = F32(a2, 24);
      if ( v24 <= -5000 )
      {
        F16(a2, 88) = 0;
      }
      else
      {
        v25 = v24 - 500;
        if ( v25 < -5000 )
          v25 = -5000;
        F32(a2, 24) = v25;
      }
    }
  }
  else
  {
    v26 = F32(a2, 24);
    if ( v26 >= 5000 || a7 != 0 )
    {
      v28 = F32(a2, 8) + 1600;
      v29 = *(_DWORD *)&GAP(W, 0x1030, 24);
      v30 = (F32(a2, 4) + F32(a2, 12) / 2) / 3200;
      if ( F32(v29, 2887728) - 1 >= (v30 < 0 ? 0 : v30) )
        (*v73__f) = v30 < 0 ? 0 : v30;
      else
        (*v73__f) = F32(v29, 2887728) - 1;
      v31 = F32(v29, 2887732) - 1;
      if ( v31 >= (v28 / 3200 < 0 ? 0 : v28 / 3200) )
        v31 = v28 / 3200 < 0 ? 0 : v28 / 3200;
      v32 = F32((v29 + 12 * (((*v73__f) << 7) + v31 + 138244)), 0);
      if ( v32 == 1 )
      {
        if ( v28 % 3200 <= 1600 )
          goto LABEL_44;
      }
      else if ( v32 == 0 )
      {
        goto LABEL_44;
      }
      F16(a2, 88) = 1;
      goto LABEL_44;
    }
    v27 = v26 + 500;
    if ( v26 + 500 > 5000 )
      v27 = 5000;
    F32(a2, 24) = v27;
  }
LABEL_44:
  if ( W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)a2), (intptr_t)(F32(a2, 24) / 100), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(a2 + 8), (intptr_t)(0)) == 0 )
    F32(a2, 24) = 0;
  v33 = W_char_riding((intptr_t)(c), (intptr_t)(1), (intptr_t)(&(*v73__f)));
  if ( F32(c, 0x28c) == 3 && W_entity_count_riders((intptr_t)(c), (intptr_t)(1), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0)) > 0 || v33 == 0 )
  {
    v35 = (*v74__f);
  }
  else
  {
    v34 = (uint16_t)a4;
    if ( (uint16_t)a4 >= (int)F16(v33, 68) )
    {
      v34 = 0;
      g_eng.h5bf3ee(OutputString, "%d", F32(v33, 468));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x85451c))));
      API_OutputDebugStringA((intptr_t)(OutputString));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
      (*Streama__f) = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
      if ( (*Streama__f) != 0 )
      {
        API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
        CRT_fprintf(
          (*Streama__f),
          "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
          (*SystemTime__f).wYear,
          (*SystemTime__f).wMonth,
          (*SystemTime__f).wDay,
          (*SystemTime__f).wHour,
          (*SystemTime__f).wMinute,
          (*SystemTime__f).wSecond,
          ((char *)D(0x85451c)),
          OutputString);
        CRT_fclose((*Streama__f));
      }
    }
    (*Stream__f) = 1;
    v35 = F32(F32((v33 + 64), 0) + 660 * v34, 44);
    if ( a3 == 0 )
      F32(c, 0x2dc) = 1;
  }
  v36 = F32(c, 0x28c);
  v37 = 0;
  if ( v36 <= 0 )
    goto LABEL_66;
  if ( v36 <= 2 )
    goto LABEL_59;
  if ( v36 == 3 )
  {
    v37 = 1;
LABEL_59:
    if ( W_entity_count_riders((intptr_t)(c), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 && (F32(c, 0x2dc) != 0 || v37 != 0) )
    {
      v38 = F32(a2, 4);
      if ( (int)abs32(F32(c, 0x27c) - v38) <= 1600 )
      {
        if ( a3 == 0 )
          F32(c, 0x2dc) = 0;
      }
      else
      {
        (*Stream__f) = 1;
        v35 = F32(c, 0x27c) >= v38 ? 1 : 3;
      }
    }
  }
LABEL_66:
  if ( F32(a2, 44) != v35 )
  {
    v39 = F32(a2, 40);
    F32(a2, 44) = v35;
    F32(a2, 40) = -v39;
  }
  if ( (*v71__f) != 0 || (*Stream__f) == 0 )
  {
    v40 = F32(a2, 40) - 1000 < 0 ? 0 : F32(a2, 40) - 1000;
  }
  else
  {
    v40 = F32(a2, 40) + 500;
    if ( v40 > 20000 )
      v40 = 20000;
  }
  F32(a2, 40) = v40;
  v41 = F32(a2, 40);
  if ( F32(a2, 44) == 1 )
    v42 = v41 / 100;
  else
    v42 = v41 / -100;
  if ( W_entity_move_horizontal((intptr_t)(c), (intptr_t)((_DWORD *)a2), (intptr_t)(v42), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(0)) != 0 )
  {
    if ( a3 == 0 )
    {
      v43 = (100 * rnd() / 0x8000 >= 50) + 64;
      v44 = 50 * rnd() / 0x8000 + 150;
      v45 = rnd();
      v46 = F32(c, 0x1b0);
      v47 = 500 - 1000 * v45 / 0x8000;
      if ( v46 != 0 )
      {
        if ( F16(c, 0x1b4) != 0 )
        {
          (*Streamb__f) = (v46 + 32);
          if ( F32(v46, 32) == 0 )
            W_entity_snd_start((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v43), (intptr_t)(-1000), (intptr_t)(v47), (intptr_t)(-5000), (intptr_t)(0));
          v48 = F32(c, 0x1b0);
          if ( v48 != 0 )
          {
            if ( F16(c, 0x1b4) != 0 )
              F32(v48, 20) = 1;
            else
              g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
          }
          v49 = (int)((*Streamb__f)->_ptr + 1);
          (*Streamb__f)->_ptr = (char *)v49;
          if ( v49 >= v44 )
            (*Streamb__f)->_ptr = 0;
        }
        else
        {
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
        }
      }
    }
  }
  else
  {
    F32(a2, 40) /= -2;
  }
  v50 = F32(c, 0x2b4);
  if ( v50 != F32(c, 0x2b8) )
  {
    if ( F32(a2, 4) < v50 )
    {
      F32(a2, 4) = v50;
      F32(a2, 40) = 0;
    }
    v51 = F32(c, 0x2b8);
    if ( F32(a2, 4) > v51 )
    {
      F32(a2, 4) = v51;
      F32(a2, 40) = 0;
    }
  }
  return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(a2, 4) - a5), (intptr_t)(F32(a2, 8) - a6), (intptr_t)(a4));
}

/* 0x553ec0: int __thiscall sub_553EC0(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t ab_553ec0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    intptr_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    intptr_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t i = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    int32_t * v26 = 0;
    intptr_t * v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    uint16_t v41 = 0;
    int v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    int v47 = 0;
    int16_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    uint16_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    int16_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t * v61 = 0;
    intptr_t v62 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v69 = (intptr_t *)(frame_ + 0x14);   /* [4] at ebp-0x10 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x20 */
    int32_t * *const v66__f = (int32_t * *)(frame_ + 0x8);   /* ebp-0x1c */
    int32_t * *const v67__f = (int32_t * *)(frame_ + 0xc);   /* ebp-0x18 */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x14 */

  v9 = a2;
  (*v64__f) = 1000;
  if ( F32(c, 0x28c) == 0 )
  {
    if ( a8 == 0 )
      return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v9, 4) - a5), (intptr_t)(F32(v9, 8) - a6), (intptr_t)(a4));
    if ( F32(c, 0x2bc) == F32(c, 0x2c0) )
    {
LABEL_80:
      W_body_hit_spawn((intptr_t)(c), (intptr_t)(0), (intptr_t)(v9), (intptr_t)(0), (intptr_t)(1600), (intptr_t)(F32(v9, 12)), (intptr_t)(3200), (intptr_t)(13), (intptr_t)(a3), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v64__f) * F32(F32(c, 0x2e0), 4) / 1000), (intptr_t)(1), (intptr_t)(0), (intptr_t)(F32(c, 0x2a4)), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v9, 4) - a5), (intptr_t)(F32(v9, 8) - a6), (intptr_t)(a4));
    }
    v32 = F32(a2, 24) + 1000;
    if ( v32 > 40000 )
      v32 = 40000;
    F32(a2, 24) = v32;
    v33 = v32 / 100;
    if ( F32(v9, 48) == 2 )
    {
      v34 = F32(v9, 8);
      v35 = F32(c, 0x2c0);
      if ( v34 < v35 )
      {
        v36 = v33 + v34;
        if ( v35 >= v36 )
        {
          F32(v9, 8) = v36;
          goto LABEL_79;
        }
        goto LABEL_50;
      }
      F32(v9, 48) = 0;
    }
    else
    {
      v37 = F32(v9, 8);
      v38 = F32(c, 0x2bc);
      if ( v37 > v38 )
      {
        v35 = v37 - v33;
        if ( v38 > v35 )
          v35 = F32(c, 0x2bc);
LABEL_50:
        F32(v9, 8) = v35;
        goto LABEL_79;
      }
      F32(v9, 48) = 2;
    }
    (*v66__f) = (_DWORD *)F32(c, 0x40);
    if ( a3 == 0 )
    {
      v39 = SYS32(1748);
      if ( F32(SYS32(1748), 8) != 0
        && (F32(SYS32(1748), 4) != 0
         || W_vpool_load((intptr_t)(SYS32(1748))) != 0) )
      {
        SETLO(v40, F16(v39, 0));
        v41 = 0;
        v42 = F16(v39, 0) == 0;
        a8 = 0;
        if ( !v42 )
        {
          v43 = F32(v39, 4);
          v44 = F16(v39, 2);
          v40 = (uint16_t)v40;
          a2 = (uint16_t)v40;
          (*v65__f) = v43;
          while ( 1 )
          {
            v45 = (v44 + v41) % v40;
            v46 = (*v65__f);
            if ( F32((*v65__f) + 8 * (uint16_t)v45, 4) == 0 )
              break;
            v41 = a8 + 1;
            v47 = (uint16_t)++a8 < F16(v39, 0);
            if ( !v47 )
              goto LABEL_65;
            v40 = a2;
          }
          v48 = v45 + 1;
          v49 = 8 * (uint16_t)v45;
          F16(v39, 2) = v48;
          F32(v49 + v46, 4) = 0;
          v50 = (intptr_t *)(F32(v39, 4) + v49);
          if ( v50 != 0 )
          {
            v51 = *v50;
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v66__f)), (intptr_t)((int *)&(*v67__f)), (intptr_t)(&(*v68__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v51, (int)(*v67__f));
              g_eng.dsb_pan_5bb880(v51, (*v68__f));
              g_eng.dsb_freq_5bb890(v51, 0);
              g_eng.dsb_play_5bb8c0(v51);
            }
          }
        }
      }
    }
LABEL_65:
    (*v67__f) = (_DWORD *)F32(c, 0x40);
    if ( a3 == 0 )
    {
      v52 = SYS32(1752);
      if ( F32(SYS32(1752), 8) != 0
        && (F32(SYS32(1752), 4) != 0
         || W_vpool_load((intptr_t)(SYS32(1752))) != 0) )
      {
        SETLO(v53, F16(v52, 0));
        v54 = 0;
        v42 = F16(v52, 0) == 0;
        a8 = 0;
        if ( !v42 )
        {
          v55 = F32(v52, 4);
          v56 = F16(v52, 2);
          v53 = (uint16_t)v53;
          a2 = (uint16_t)v53;
          (*v68__f) = v55;
          while ( 1 )
          {
            v57 = (v56 + v54) % v53;
            v58 = (*v68__f);
            if ( F32((*v68__f) + 8 * (uint16_t)v57, 4) == 0 )
              break;
            v54 = a8 + 1;
            v47 = (uint16_t)++a8 < F16(v52, 0);
            if ( !v47 )
              goto LABEL_78;
            v53 = a2;
          }
          v59 = v57 + 1;
          v60 = 8 * (uint16_t)v57;
          F16(v52, 2) = v59;
          F32(v60 + v58, 4) = 0;
          v61 = (intptr_t *)(F32(v52, 4) + v60);
          if ( v61 != 0 )
          {
            v62 = *v61;
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v67__f)), (intptr_t)((int *)&(*v66__f)), (intptr_t)(&(*v65__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v62, (int)(*v66__f));
              g_eng.dsb_pan_5bb880(v62, (*v65__f));
              g_eng.dsb_freq_5bb890(v62, 0);
              g_eng.dsb_play_5bb8c0(v62);
            }
          }
        }
      }
    }
LABEL_78:
    F32(v9, 24) = 0;
LABEL_79:
    if ( F32(v9, 48) != 2 )
      return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v9, 4) - a5), (intptr_t)(F32(v9, 8) - a6), (intptr_t)(a4));
    goto LABEL_80;
  }
  if ( F32(c, 0x28c) == 1 )
  {
    v10 = F32(c, 0x2b8);
    v11 = F32(c, 0x2c0) - 6400;
    v69[0] = F32(c, 0x2b4);
    v69[1] = v11;
    (*v64__f) = 1800;
    v69[2] = v10 - v69[0];
    v69[3] = 12800;
    v12 = W_char_in_rect((intptr_t)(a4), (intptr_t)(v69), (intptr_t)(1));
    switch ( F16(v9, 88) )
    {
      case 0:
        if ( a8 != 0 && v12 != 0 )
          ++F16(v9, 88);
        break;
      case 1:
        if ( F32(c, 0x2c0) <= F32(v9, 8) )
        {
          v18 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v19 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1752)), (intptr_t)(0));
            if ( v19 != 0 )
            {
              v20 = *v19;
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) - 500), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v18), (intptr_t)(&a8), (intptr_t)(&a2)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v20, a8);
                g_eng.dsb_pan_5bb880(v20, a2);
                g_eng.dsb_freq_5bb890(v20, 0);
                g_eng.dsb_play_5bb8c0(v20);
              }
            }
            for ( i = 20; i != 0; --i )
            {
              v22 = F32(v9, 12) + 1600;
              v23 = rnd();
              W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)v9), (intptr_t)(v22 * v23 / 0x8000 - v22 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
            }
          }
          v24 = F32(c, 0x2c0);
          ++F16(v9, 88);
          F32(v9, 8) = v24;
          F32(v9, 24) = 0;
          F32(v9, 92) = 0;
        }
        else
        {
          if ( F32(v9, 24) == 0 )
          {
            v13 = (_DWORD *)F32(c, 0x40);
            if ( a3 == 0 )
            {
              v14 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1748)), (intptr_t)(0));
              if ( v14 != 0 )
              {
                v15 = *v14;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v13), (intptr_t)(&a8), (intptr_t)(&a2)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v15, a8);
                  g_eng.dsb_pan_5bb880(v15, a2);
                  g_eng.dsb_freq_5bb890(v15, 0);
                  g_eng.dsb_play_5bb8c0(v15);
                }
              }
            }
          }
          v16 = F32(v9, 24) + 2000;
          if ( v16 > 80000 )
            v16 = 80000;
          F32(v9, 24) = v16;
          v17 = F32(c, 0x2c0);
          if ( v17 >= F32(v9, 8) + v16 / 100 )
            v17 = F32(v9, 8) + v16 / 100;
          F32(v9, 8) = v17;
        }
        goto LABEL_80;
      case 2:
        v25 = F32(v9, 92) + 1;
        F32(v9, 92) = v25;
        if ( v25 > 20 )
        {
          v26 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v27 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1748)), (intptr_t)(0));
            if ( v27 != 0 )
            {
              v28 = *v27;
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v26), (intptr_t)(&a3), (intptr_t)(&a8)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v28, a3);
                g_eng.dsb_pan_5bb880(v28, a8);
                g_eng.dsb_freq_5bb890(v28, 0);
                g_eng.dsb_play_5bb8c0(v28);
              }
            }
          }
          ++F16(v9, 88);
        }
        break;
      case 3:
        v29 = F32(v9, 8);
        if ( F32(c, 0x2bc) >= v29 )
        {
          F16(v9, 88) = 0;
          F32(v9, 24) = 0;
          F32(v9, 8) = F32(c, 0x2bc);
        }
        else
        {
          v30 = F32(v9, 24) - 500;
          if ( v30 < -20000 )
            v30 = -20000;
          F32(v9, 24) = v30;
          v31 = F32(c, 0x2bc);
          if ( v31 <= v29 + v30 / 100 )
            v31 = v29 + v30 / 100;
          F32(v9, 8) = v31;
        }
        break;
      default:
        return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v9, 4) - a5), (intptr_t)(F32(v9, 8) - a6), (intptr_t)(a4));
    }
  }
  return W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v9, 4) - a5), (intptr_t)(F32(v9, 8) - a6), (intptr_t)(a4));
}

/* 0x554570: int __thiscall sub_554570(cls_554570 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t ab_554570(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t * v19 = 0;
    void * v20 = 0;
    uint32_t v21 = 0;
    uint16_t v22 = 0;
    uint16_t v23 = 0;
    uint16_t v24 = 0;
    int v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    uint16_t v31 = 0;
    uint16_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t * v34 = 0;
    int32_t * v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x118 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x18);   /* [256] at ebp-0x100 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x118 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x118 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x114 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x110 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10c */
    intptr_t *const v55__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x108 */
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x104 */

  v6 = c;
  v7 = F32(c, 0x2cc);
  v8 = 0;
  (*v53__f) = c;
  v9 = 96;
  if ( v7 != 0 )
    (*v50__f) = 48 * v7 / 1000;
  else
    (*v50__f) = 96;
  v10 = F32(c, 0x2d0);
  if ( v10 != 0 )
    v9 = 48 * v10 / 1000;
  v11 = F32(c, 0x2d8);
  if ( v11 != 0 )
    v12 = 48 * v11 / 1000;
  else
    v12 = 48;
  if ( F32(c, 0x28c) != 1 && a5 != 0 )
    F16(a2, 88) = 3;
  v13 = F16(a2, 88);
  if ( (uint16_t)v13 == 0 )
  {
    if ( a5 == 0 )
      goto LABEL_23;
    v15 = F32(a2, 92);
    if ( v15 < (*v50__f) && F32(c, 0x2cc) != 1 )
    {
      F32(a2, 92) = v15 + 1;
      goto LABEL_23;
    }
    goto LABEL_22;
  }
  if ( v13 == 1 && a5 != 0 )
  {
    v14 = F32(a2, 92);
    if ( v14 < v9 && F32(c, 0x2d0) != 1 )
    {
      F32(a2, 92) = v14 + 1;
      goto LABEL_23;
    }
LABEL_22:
    F32(a2, 92) = 0;
    F16(a2, 88) = 2;
  }
LABEL_23:
  switch ( F16(a2, 88) )
  {
    case 2:
      v18 = F32(a2, 92);
      (*v52__f) = 1;
      (*v51__f) = 1000 * v18 / 10;
      if ( v18 != 1 )
        goto LABEL_67;
      (*v55__f) = F32(v6, 0x40);
      if ( a3 != 0 )
        goto LABEL_67;
      v19 = SYS32(1820);
      if ( F32(SYS32(1820), 8) == 0 )
        goto LABEL_67;
      if ( F32(SYS32(1820), 4) == 0 )
      {
        v20 = (intptr_t)g_eng.op_new(8 * (uint16_t)(*(uint16_t *)SYS32(1820)));
        v21 = 8 * *v19;
        F32(v19, 4 * 1) = v20;
        memset(v20, 0, v21);
        if ( W_sd_5bb250((intptr_t)(F32(v19, 4 * (2))), (intptr_t)(F32(v19, 4 * (1))), (intptr_t)(*((intptr_t *)v19 + 4)), (intptr_t)(v19[6]), (intptr_t)(0), (intptr_t)(0)) == 0 )
        {
LABEL_47:
          g_eng.op_delete((void *)*((void **)v19 + 1));
          v31 = v19[6];
          F32(v19, 4 * 1) = 0;
          g_eng.h5bf3ee(Buffer, "%d", v31);
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x854620)), Buffer);
LABEL_67:
          v48 = F32(a2, 92);
          if ( v48 >= 10 )
          {
            F32(a2, 92) = 0;
            F16(a2, 88) = 3;
          }
          else
          {
            F32(a2, 92) = v48 + 1;
          }
          break;
        }
        v22 = 1;
        if ( *v19 > 1u )
        {
          while ( W_sd_5bb2f0((intptr_t)(F32(v19, 4 * (2))), (intptr_t)((_DWORD *)(F32(v19, 4 * 1) + 8 * v22)), (intptr_t)(F32(F32(v19, 4 * 1), 0))) != 0 )
          {
            if ( ++v22 >= *v19 )
              goto LABEL_42;
          }
          goto LABEL_47;
        }
      }
LABEL_42:
      v23 = *v19;
      v24 = 0;
      v25 = *v19 == 0;
      (*v54__f) = 0;
      if ( !v25 )
      {
        v26 = F32(v19, 4 * 1);
        v27 = v19[1];
        v28 = v23;
        while ( 1 )
        {
          v29 = (v27 + v24) % v28;
          if ( F32(v26 + 8 * (uint16_t)v29, 4) == 0 )
            break;
          v24 = (*v54__f) + 1;
          if ( (uint16_t)++(*v54__f) >= *v19 )
            goto LABEL_66;
        }
        v32 = v29 + 1;
        v33 = 8 * (uint16_t)v29;
        v19[1] = v32;
        F32(v33 + v26, 4) = 0;
        v34 = (intptr_t *)(F32(v19, 4 * 1) + v33);
        if ( v34 != 0 )
        {
          (*v56__f) = *v34;
          v35 = *(_DWORD **)&GAP(W, 0x1030, 28);
          v36 = F32(CFG(), 360);
          if ( v35[6] != 0 )
          {
            v37 = v35[4] + v35[24] + v35[25] / 2;
            v38 = v35[5] + v35[23] + v35[26] / 2;
          }
          else
          {
            v37 = v35[4];
            v38 = v35[5];
          }
          (*v54__f) = v38;
          v39 = F32((*v55__f), 4);
          v40 = v39 + F32((*v55__f), 12);
          if ( v37 <= v39 || v37 >= v40 )
          {
            v42 = v39 - v37;
            v41 = v40 - v37;
            if ( (int)abs32(v42) < (int)abs32(v40 - v37) )
              v41 = v42;
          }
          else
          {
            v41 = 0;
          }
          v43 = abs32(v41);
          if ( v43 <= 128000 )
          {
            v44 = abs32(*(_DWORD *)&GAP((*v53__f), 0x0, 36) + F32((*v55__f), 8) - (*v54__f));
            if ( v44 <= 64000 )
            {
              (*v55__f) = 5000 * (v43 - 8000 < 0 ? 0 : v43 - 8000) / 120000;
              v45 = 10000 * v41 / 128000;
              v46 = v36
                  + 3000 * (F32(*(_DWORD *)&GAP(W, 0x1030, 16), 40) - 1000) / 1000
                  + -5000 * ((v44 - 16000) & ((v44 - 16000 < 0) - 1)) / 48000
                  - (*v55__f);
              if ( v46 < -10000 )
              {
                v46 = -10000;
              }
              else if ( v46 > 0 )
              {
                v46 = 0;
              }
              if ( &(*v52__f) == 0 )
              {
                v46 = (*v52__f);
                v45 = (*v52__f);
              }
              v47 = (*v56__f);
              g_eng.dsb_volume_5bb870((*v56__f), v46);
              g_eng.dsb_pan_5bb880(v47, v45);
              g_eng.dsb_freq_5bb890(v47, 0);
              g_eng.dsb_play_5bb8c0(v47);
            }
          }
        }
LABEL_66:
        v6 = (*v53__f);
      }
      goto LABEL_67;
    case 3:
      (*v52__f) = 1;
      (*v51__f) = 1000;
      if ( a5 != 0 )
      {
        if ( F32(c, 0x28c) != 1 )
          break;
        v17 = F32(a2, 92);
        if ( v17 < v12 )
        {
          F32(a2, 92) = v17 + 1;
          break;
        }
      }
      F32(a2, 92) = 0;
      F16(a2, 88) = 4;
      break;
    case 4:
      v16 = F32(a2, 92);
      (*v51__f) = 1000 - 1000 * v16 / 20;
      if ( v16 >= 20 )
      {
        F32(a2, 92) = 0;
        F16(a2, 88) = 1;
      }
      else
      {
        F32(a2, 92) = v16 + 1;
      }
      break;
    default:
      goto LABEL_71;
  }
  v8 = (*v51__f);
LABEL_71:
  switch ( F32(v6, 0x1d4) )
  {
    case 0x11265:
      result = 9;
      F32(a2, 8) = F32(v6, 0x2bc) + 4000 * (1000 - v8) / 1000;
      break;
    case 0x11266:
      result = 10;
      F32(a2, 8) = F32(v6, 0x2bc) + 800 * (5 * v8 - 5000) / 1000;
      break;
    case 0x11267:
      result = 12;
      F32(a2, 4) = F32(v6, 0x2b4) + 800 * (5 * v8 - 5000) / 1000;
      break;
    case 0x11268:
      result = 11;
      F32(a2, 4) = F32(v6, 0x2b4) + 4000 * (1000 - v8) / 1000;
      break;
    default:
      result = (int)(*v56__f);
      break;
  }
  if ( a5 == 0 && v8 == 0 )
  {
    F32(a2, 92) = 0;
    F16(a2, 88) = 0;
  }
  if ( (*v52__f) == 1 && v8 > 0 )
    return W_body_hit_spawn((intptr_t)(v6), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(a2, 12)), (intptr_t)(F32(a2, 16)), (intptr_t)(result), (intptr_t)(a3), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(F32(F32(v6, 0x2e0), 4)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(F32(v6, 0x2a4)), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  return result;
}

/* 0x554c40: _DWORD *__thiscall sub_554C40(cls_4022A0 *this, int a2, FILE *a3, int a4, int a5, int a6, int a7) */
int32_t ab_554c40(wl_char *self, int32_t a2, FILE * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    int32_t * result = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    uint32_t v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    int v27 = 0;
    int16_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    int32_t * v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    int32_t * v37 = 0;
    intptr_t * v38 = 0;
    intptr_t v39 = 0;
    intptr_t * v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    uint16_t v48 = 0;
    intptr_t v49 = 0;
    uint16_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    int32_t * v55 = 0;
    intptr_t * v56 = 0;
    intptr_t v57 = 0;
    int32_t * v58 = 0;
    intptr_t * v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    int32_t * v63 = 0;
    intptr_t * v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    int32_t * *const v72__f = (int32_t * *)(frame_ + 0x14);   /* ebp-0x4 */

  (*v70__f) = 40000;
  (*v68__f) = 2000;
  result = (_DWORD *)F32(c, 0x2bc);
  v9 = (_DWORD *)F32(c, 0x2c0);
  v10 = F32(c, 0xc);
  v11 = result;
  v12 = (int)v9;
  (*v72__f) = result;
  (*v71__f) = (int)v9;
  if ( v10 != 1 )
  {
    v13 = a2;
    switch ( F32(c, 0x28c) )
    {
      case 0:
        if ( F32(a2, 48) == 2 )
        {
          v52 = F32(a2, 8);
          if ( v52 < (int)v9 )
          {
            result = (_DWORD *)(v52 + 100);
            if ( (int)v9 < (int)result )
              result = v9;
            goto LABEL_87;
          }
          F32(a2, 48) = 0;
          goto LABEL_93;
        }
        v53 = F32(a2, 8);
        if ( v53 <= (int)result )
        {
          F32(a2, 48) = 2;
LABEL_93:
          v55 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v56 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1748)), (intptr_t)(0));
            if ( v56 != 0 )
            {
              v57 = *v56;
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v55), (intptr_t)(&a2), (intptr_t)((int *)&(*v72__f))) != 0 )
              {
                g_eng.dsb_volume_5bb870(v57, a2);
                g_eng.dsb_pan_5bb880(v57, (int)(*v72__f));
                g_eng.dsb_freq_5bb890(v57, 0);
                g_eng.dsb_play_5bb8c0(v57);
              }
            }
          }
          v58 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v59 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1752)), (intptr_t)(0));
            if ( v59 != 0 )
            {
              v60 = *v59;
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) - 500), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v58), (intptr_t)((int *)&a3), (intptr_t)(&a2)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v60, (int)a3);
                g_eng.dsb_pan_5bb880(v60, a2);
                g_eng.dsb_freq_5bb890(v60, 0);
                g_eng.dsb_play_5bb8c0(v60);
              }
            }
          }
          return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
        }
        v54 = v53 - 100;
        if ( (int)result > v54 )
        {
LABEL_87:
          F32(a2, 8) = result;
          return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
        }
        F32(a2, 8) = v54;
        return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
      case 1:
        if ( W_entity_count_riders((intptr_t)(c), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) > 0 )
          F16(v13, 88) = 1;
        v61 = F16(v13, 88);
        if ( (uint16_t)v61 != 0 )
        {
          if ( v61 == 1 )
          {
            v62 = v12;
            if ( F32(v13, 8) == v12 )
              F16(v13, 88) = 0;
          }
          else
          {
            v62 = a2;
          }
        }
        else
        {
          (*v70__f) = 20000;
          (*v68__f) = 1000;
          v62 = (int)v11;
        }
        goto LABEL_112;
      case 2:
        v27 = W_entity_count_riders((intptr_t)(c), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) <= 0;
        v62 = (int)v11;
        if ( v27 )
          v62 = v12;
        F32(v13, 92) = 0;
LABEL_112:
        if ( ab_5554e0(v13, v62, (*v70__f), (*v68__f), (int)v11, v12) == 2 )
        {
          v63 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v64 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1752)), (intptr_t)(0));
            if ( v64 != 0 )
            {
              v65 = *v64;
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) - 500), (intptr_t)(-3000), (intptr_t)(0), (intptr_t)(v63), (intptr_t)(&a2), (intptr_t)((int *)&(*v72__f))) != 0 )
              {
                g_eng.dsb_volume_5bb870(v65, a2);
                g_eng.dsb_pan_5bb880(v65, (int)(*v72__f));
                g_eng.dsb_freq_5bb890(v65, 0);
                g_eng.dsb_play_5bb8c0(v65);
              }
            }
          }
        }
        if ( (int)abs32(F32(v13, 24)) < 1000 )
        {
          if ( a3 == 0 )
          {
            v66 = F32(c, 0x1b0);
            if ( v66 != 0 && F16(c, 0x1b4) != 0 )
              F32(v66, 32) = 0;
          }
        }
        else
        {
          W_entity_snd_loop((intptr_t)(c), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(0xDCu), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-3000), (intptr_t)(0), (intptr_t)(100));
        }
        return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
      case 3:
        v14 = W_actor_by_handle((intptr_t)(F32(c, 0x298)));
        if ( v14 == 0 )
          return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
        W_entity_count_riders((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(&(*v67__f)), (intptr_t)(0));
        SETLO(v15, F16(c, 0x1ec) + F16(c, 0x2cc));
        (*v67__f) += v15;
        W_entity_count_riders((intptr_t)(v14), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(&(*v69__f)), (intptr_t)(0));
        v16 = F32(v14, 716);
        SETLO(v16, F16(v14, 492) + v16);
        v17 = F32(c, 0x1d8);
        v18 = v16 + (*v69__f);
        (*v69__f) += v16;
        if ( v17 >= F32(v14, 472) )
        {
          v48 = a4;
          if ( (uint16_t)a4 >= F16(v14, 68) )
            v48 = 0;
          v49 = F32(v14, 64) + 660 * v48;
          v50 = a4;
          F32(v13, 8) = F32(c, 0x2c0)
                               - (F32(c, 0x2c0) - F32(c, 0x2bc))
                               * (F32(v14, 700) - F32(v49, 20) - F32(v49, 8))
                               / (F32(v14, 700) - F32(v14, 704));
          if ( v50 >= F16(v14, 68) )
            v50 = 0;
          if ( F32(F32((v14 + 64), 0) + 660 * v50, 24) != 0 )
          {
            if ( a3 != 0 )
              return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
            v51 = F32(c, 0x1b0);
            if ( v51 != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
                F32(v51, 20) = 1;
              else
                g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
              F32(c, 0x2f8) = F32(v14, 760);
              return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
            }
          }
          else if ( a3 != 0 )
          {
            return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
          }
          F32(c, 0x2f8) = F32(v14, 760);
          return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
        }
        if ( (uint16_t)(*v67__f) == v18 )
        {
          v19 = F32(c, 0x2c0);
          v20 = F32(c, 0x2bc);
          v21 = F32(v13, 8);
          a2 = (v19 + v20) / 2;
          v22 = 1000 * ((int)(80 * abs32(a2 - v21)) / (v19 - v20));
          if ( v22 > 40000 )
          {
            v23 = 40000;
          }
          else if ( v22 >= 10000 )
          {
            v23 = v22;
          }
          else
          {
            v23 = 10000;
          }
          v24 = v23 / 10;
        }
        else
        {
          if ( (uint16_t)(*v67__f) <= v18 )
            a2 = F32(c, 0x2bc);
          else
            a2 = F32(c, 0x2c0);
          v23 = 40000;
          v24 = 2000;
        }
        v25 = F32(v13, 8);
        v26 = a2;
        v27 = a2 <= v25;
        (*v68__f) = 0;
        if ( a2 == v25 )
        {
          v28 = 0;
LABEL_35:
          if ( v28 == 2 )
          {
            (*v72__f) = (_DWORD *)F32(c, 0x40);
            if ( a3 == 0 )
            {
              v38 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1752)), (intptr_t)(0));
              if ( v38 != 0 )
              {
                v39 = *v38;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) - 500), (intptr_t)(-3000), (intptr_t)(0), (intptr_t)((*v72__f)), (intptr_t)(&(*v71__f)), (intptr_t)(&(*v70__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v39, (*v71__f));
                  g_eng.dsb_pan_5bb880(v39, (*v70__f));
                  g_eng.dsb_freq_5bb890(v39, 0);
                  g_eng.dsb_play_5bb8c0(v39);
                }
              }
            }
            (*v72__f) = F32(v14, 64);
            if ( a3 == 0 )
            {
              v40 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1752)), (intptr_t)(0));
              if ( v40 != 0 )
              {
                v41 = *v40;
                if ( W_sfx_place((intptr_t)(v14), (intptr_t)(F32(CFG(), 360) - 500), (intptr_t)(-3000), (intptr_t)(0), (intptr_t)((*v72__f)), (intptr_t)(&(*v71__f)), (intptr_t)(&(*v70__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v41, (*v71__f));
                  g_eng.dsb_pan_5bb880(v41, (*v70__f));
                  g_eng.dsb_freq_5bb890(v41, 0);
                  g_eng.dsb_play_5bb8c0(v41);
                }
              }
            }
          }
          if ( (int)abs32(F32(v13, 24)) < 1000 )
          {
            if ( a3 != 0 )
              return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
            v46 = F32(c, 0x1b0);
            if ( v46 != 0 && F16(c, 0x1b4) != 0 )
              F32(v46, 32) = 0;
            v47 = F32(v14, 432);
            if ( v47 != 0 && F16(v14, 436) != 0 )
              F32(v47, 32) = 0;
          }
          else
          {
            if ( a3 != 0 )
              return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
            v42 = F32(c, 0x1b0);
            if ( v42 != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
              {
                if ( F32(v42, 32) == 0 )
                  W_entity_snd_start((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xDCu), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-3000), (intptr_t)(0));
                W_entity_snd_keep((intptr_t)(c), (intptr_t)(0), (intptr_t)(0));
                v43 = F32(v42, 32) + 1;
                F32(v42, 32) = v43;
                if ( v43 >= 100 )
                  F32(v42, 32) = 0;
              }
              else
              {
                g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
              }
            }
            v44 = F32(v14, 432);
            if ( v44 != 0 )
            {
              if ( F16(v14, 436) != 0 )
              {
                if ( F32(v44, 32) == 0 )
                  W_entity_snd_start((intptr_t)(v14), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xDCu), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-3000), (intptr_t)(0));
                W_entity_snd_keep((intptr_t)(v14), (intptr_t)(0), (intptr_t)(0));
                v45 = F32(v44, 32) + 1;
                F32(v44, 32) = v45;
                if ( v45 >= 100 )
                  F32(v44, 32) = 0;
              }
              else
              {
                g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
              }
            }
          }
          F32(c, 0x2f8) = a2 != F32(v13, 8);
          return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
        }
        v29 = F32(v13, 24);
        if ( v27 )
        {
          (*v68__f) = v29 == 0;
          v32 = W_speed_ramp((intptr_t)(F32(v13, 24)), (intptr_t)(v24), (intptr_t)(-v24), (intptr_t)(-v23));
          F32(v13, 24) = v32;
          v33 = v32 / 100 + F32(v13, 8);
          F32(v13, 8) = v33;
          if ( v33 < v26 )
LABEL_22:
            F32(v13, 8) = v26;
        }
        else
        {
          (*v68__f) = v29 == 0;
          v30 = W_speed_ramp((intptr_t)(F32(v13, 24)), (intptr_t)(v24), (intptr_t)(-v24), (intptr_t)(v23));
          F32(v13, 24) = v30;
          v31 = v30 / 100 + F32(v13, 8);
          F32(v13, 8) = v31;
          if ( v31 > v26 )
            goto LABEL_22;
        }
        v34 = F32(v13, 8);
        if ( (_DWORD *)v26 == v34 )
          F32(v13, 24) = 0;
        v35 = (*v71__f);
        v36 = (*v71__f);
        if ( (*v71__f) >= (int)v34 )
          v36 = (int)v34;
        v37 = (*v72__f);
        if ( (int)(*v72__f) <= v36 )
        {
          if ( (*v71__f) < (int)v34 )
            v34 = (_DWORD *)(*v71__f);
        }
        else
        {
          v34 = (*v72__f);
        }
        F32(v13, 8) = v34;
        if ( v34 == v37 || v34 == (_DWORD *)v35 )
        {
          F32(v13, 24) = 0;
          v28 = 2;
        }
        else
        {
          v28 = (*v68__f);
        }
        goto LABEL_35;
      default:
        return (_DWORD *)W_entity_carry_riders((intptr_t)(c), (intptr_t)(F32(v13, 4) - a5), (intptr_t)(F32(v13, 8) - a6), (intptr_t)(a4));
    }
  }
  return result;
}

/* 0x5554e0: __int16 __stdcall sub_5554E0(int a1, int a2, int a3, int a4, int a5, int a6) */
int32_t ab_5554e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{

    intptr_t v6 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int v10 = 0;
    int v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int v15 = 0;
    int v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v22__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v6 = F32(a1, 8);
  if ( a2 == v6 )
    return 0;
  v8 = F32(a1, 24);
  if ( a2 > v6 )
  {
    (*v22__f) = v8 == 0;
    v9 = a4;
    if ( a3 < 0 )
    {
      if ( a4 > 0 || v8 > 0 )
      {
        if ( a3 >= v8 )
        {
          if ( a3 > v8 )
            v8 = v8 + a4 > 0 ? 0 : v8 + a4;
        }
        else
        {
          v8 -= a4;
          if ( a3 > v8 )
            v8 = a3;
        }
      }
      else if ( a4 < 0 )
      {
        v8 = v8 - a4 > 0 ? 0 : v8 - a4;
      }
LABEL_26:
      F32(a1, 24) = v8;
      v12 = v8 / 100 + F32(a1, 8);
      F32(a1, 8) = v12;
      if ( v12 > a2 )
        goto LABEL_52;
      goto LABEL_53;
    }
    if ( v8 < 0 )
    {
      v10 = a4 < 0;
      v11 = a4 <= 0;
      if ( a4 >= 0 )
        goto LABEL_9;
      v9 = -a4;
    }
    v10 = v9 < 0;
    v11 = v9 <= 0;
LABEL_9:
    if ( v11 )
    {
      if ( v10 )
        v8 = v8 + v9 < 0 ? 0 : v8 + v9;
    }
    else if ( a3 <= v8 )
    {
      if ( a3 < v8 )
        v8 = (v8 - a4) & ((v8 - a4 < 0) - 1);
    }
    else
    {
      v8 += v9;
      if ( a3 < v8 )
        v8 = a3;
    }
    goto LABEL_26;
  }
  (*v22__f) = v8 == 0;
  v13 = a4;
  v14 = -a3;
  if ( a3 <= 0 )
  {
    if ( v8 < 0 )
    {
      v15 = a4 < 0;
      v16 = a4 <= 0;
      if ( a4 >= 0 )
        goto LABEL_33;
      v13 = -a4;
    }
    v15 = v13 < 0;
    v16 = v13 <= 0;
LABEL_33:
    if ( v16 )
    {
      if ( !v15 )
        goto LABEL_51;
      v17 = v8 + v13;
      v18 = (v8 + v13 < 0) - 1;
    }
    else
    {
      if ( v14 > v8 )
      {
        v8 += v13;
        if ( v14 < v8 )
          v8 = -a3;
        goto LABEL_51;
      }
      if ( v14 >= v8 )
        goto LABEL_51;
      v17 = v8 - a4;
      v18 = (v8 - a4 < 0) - 1;
    }
LABEL_50:
    v8 = v17 & v18;
    goto LABEL_51;
  }
  if ( a4 > 0 || v8 > 0 )
  {
    if ( v14 >= v8 )
    {
      if ( v14 <= v8 )
        goto LABEL_51;
      v18 = v8 + a4;
      v17 = (v18 > 0) - 1;
      goto LABEL_50;
    }
    v8 -= a4;
    if ( v14 > v8 )
      v8 = -a3;
  }
  else if ( a4 < 0 )
  {
    v8 = v8 - a4 > 0 ? 0 : v8 - a4;
  }
LABEL_51:
  F32(a1, 24) = v8;
  v19 = v8 / 100 + F32(a1, 8);
  F32(a1, 8) = v19;
  if ( v19 < a2 )
LABEL_52:
    F32(a1, 8) = a2;
LABEL_53:
  v20 = F32(a1, 8);
  if ( a2 == v20 )
    F32(a1, 24) = 0;
  v21 = a6;
  if ( a6 >= v20 )
    v21 = v20;
  if ( a5 <= v21 )
  {
    if ( a6 < v20 )
      v20 = a6;
  }
  else
  {
    v20 = a5;
  }
  F32(a1, 8) = v20;
  if ( v20 != a5 && v20 != a6 )
    return (*v22__f);
  F32(a1, 24) = 0;
  return 2;
}

/* 0x5556d0: int __thiscall sub_5556D0(cls_5556D0 *this, int a2, int a3, int a4) */
int32_t ab_5556d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;

  result = a3;
  if ( a3 == 0 )
  {
    v5 = F32(c, 0xc);
    if ( v5 != 0 )
    {
      if ( v5 != 1 || (v6 = F32(a2, 92)) <= 0 )
      {
LABEL_10:
        result = F32(a2, 92);
        if ( result == 0 )
        {
          F32(c, 0x284) = 1;
          return result;
        }
        v9 = 20 * result / 1000;
        if ( v9 >= 0 )
        {
          if ( v9 >= 20 )
          {
            result = 0;
            goto LABEL_15;
          }
        }
        else
        {
          v9 = 0;
        }
        result = SYS32(4 * v9 + 10152);
LABEL_15:
        F32(c, 0xf4) = result;
        F32(c, 0xf8) = result;
        F32(c, 0x284) = 0;
        return result;
      }
      v7 = v6 - a4 < 0 ? 0 : v6 - a4;
    }
    else
    {
      v8 = F32(a2, 92);
      if ( v8 >= 1000 )
        goto LABEL_10;
      v7 = a4 + v8;
      if ( v7 > 1000 )
        v7 = 1000;
    }
    F32(a2, 92) = v7;
    goto LABEL_10;
  }
  return result;
}
/* unported callees: none */
