/* script_io.c — the script engine's callees with outside effects: the GIFTS 0x408730 ("You obtained …": items /
 * money / a magic stone into the party), the voice pick 0x409520, the script commands 0x40a340 / 0x41a160 (BGM, the
 * scene wait), the party command 0x41d4a0, 0x40a2a0, the LOGGER 0x408b90 (the log file through retail's CRT) and the
 * BGM change 0x40a5d0.  See script_io.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish (the inlined logger copies run
 * as retail compiled them: retail's own fopen / fprintf / fclose).  Retail's own (RC_: PORT-DEBT(engine-call-slots)):
 * the music manager 0x5bbc20 / 0x5bbc60 / 0x5bbc90 / 0x5bbcd0 / 0x5bcb80 (DirectShow), 0x41e4c0. */
#include "script_io.h"

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

#include "actor_update.h"
#include "char_integrate.h"
#include "engine_rest.h"
#include "party_member.h"
#include "room_load.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "script_engine.h"
#include "sound_dev.h"
#include "sound_fx.h"
#include "title_menu.h"
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

static inline intptr_t W_actor_by_handle(intptr_t a0, ...) { return (intptr_t)actor_by_handle((int32_t)a0); }
static inline intptr_t W_area_level_row(intptr_t a0, ...) { return (intptr_t)area_level_row((int32_t)a0); }
static inline intptr_t W_body_action_start(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)body_action_start((wl_body *)a0, (int16_t)a1, (int32_t)a2); }
static inline intptr_t W_cast_cmd_clear(intptr_t a0, ...) { cast_cmd_clear((uint8_t *)a0); return 0; }
static inline intptr_t W_el_41e4c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)el_41e4c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_member_attach(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)member_attach((wl_char *)a0, (wl_pslot *)a1, (int32_t)a2); }
static inline intptr_t W_pslot_heal(intptr_t a0, intptr_t a1, ...) { pslot_heal((wl_pslot *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_formation(intptr_t a0, ...) { return (intptr_t)room_formation((wl_room *)a0); }
static inline intptr_t W_room_party_level(intptr_t a0, intptr_t a1, ...) { room_party_level((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_stats_refresh(intptr_t a0, intptr_t a1, ...) { room_stats_refresh((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_scene_device_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)scene_device_spawn((wl_event *)a0, (int32_t)a1, (uint32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_scene_party_gather(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)scene_party_gather((wl_event *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_scene_party_stance(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)scene_party_stance((wl_event *)a0, (int32_t)a1, (wl_room *)a2); }
static inline intptr_t W_script_begin(intptr_t a0, ...) { script_begin((wl_event *)a0); return 0; }
static inline intptr_t W_script_end(intptr_t a0, ...) { script_end((wl_event *)a0); return 0; }
static inline intptr_t W_script_leader(intptr_t a0, ...) { return (intptr_t)script_leader((const wl_event *)a0); }
static inline intptr_t W_script_party_cast(intptr_t a0, ...) { script_party_cast((wl_event *)a0); return 0; }
static inline intptr_t W_script_wait(intptr_t a0, ...) { return (intptr_t)script_wait((wl_event *)a0); }
static inline intptr_t W_sd_5bbc20(intptr_t a0, ...) { return (intptr_t)sd_5bbc20((wl_char *)a0); }
static inline intptr_t W_sd_5bbc60(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbc60((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_sd_5bbc90(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbc90((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_sd_5bbcd0(intptr_t a0, intptr_t a1, ...) { sd_5bbcd0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sd_5bcb80(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sd_5bcb80((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_tm_41d780(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)tm_41d780((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_voice_group_prio(intptr_t a0, intptr_t a1, ...) { return (intptr_t)voice_group_prio((const wl_voiceset *)a0, (uint16_t)a1); }
static inline intptr_t W_voice_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)voice_place((const wl_char *)a0, (int32_t *)a1, (int32_t *)a2, (int32_t *)a3); }
static inline intptr_t W_voice_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)voice_play((wl_voiceset *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }

typedef uint32_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLocalTime_t)(intptr_t);
#define API_GetLocalTime ((api_GetLocalTime_t)g_eng.api("GetLocalTime"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fprintf ((intptr_t (*)())g_eng.retail(0x5bf496u))   /* retail's CRT fprintf (cdecl) */
int32_t si_408730(wl_char *self, int32_t a2);
int32_t si_409520(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9, int32_t a10);
int32_t si_40a340(wl_char *self);
int32_t si_41a160(wl_char *self);
int32_t si_41d4a0(wl_char *self, int32_t a2, int32_t a3);
int32_t si_40a2a0(wl_char *self, int32_t a2);
void si_408b90(wl_char *self, char * msg, int32_t errno_decorate, char * trailer);
void si_40a5d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);

/* 0x408730: int __thiscall sub_408730(cls_401D40 *this, int a2) */
int32_t si_408730(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    char * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t result = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    int32_t * v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    int32_t * v25 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    uint16_t *const v29__f = (uint16_t *)(frame_ + 0xc);   /* ebp-0x4 */

  if ( a2 == 0 )
  {
    v10 = 31;
    (*v26__f) = 101;
    (*v29__f) = 400;
    (*v28__f) = 23;
    v11 = 3;
    goto LABEL_16;
  }
  v3 = 0;
  v4 = F32(W, 0x1024);
  while ( *(_DWORD *)&GAP(W, 0x0, 516 * v4 + 4) != 0 )
  {
    v4 = (v4 + 1) % 8;
    if ( (uint16_t)++v3 >= 8u )
      goto LABEL_7;
  }
  v5 = &GAP(W, 0x0, 516 * v4);
  F32(v5, 4 * 1) = 1;
  strcpy(v5 + 8, ((char *)D(0x854724)));
LABEL_7:
  v6 = F32(c, 0x8);
  ++F32(v6, 0x401c);
  W_room_party_level((intptr_t)(v6), (intptr_t)(0));
  v7 = 8;
  v8 = F32(c, 0x8) + 16432;
  do
  {
    v9 = F32(v8, 0);
    if ( F32(F32(v8, 0), 2500) == 1 )
    {
      if ( F32(v9, 2544) == 100000104 )
        goto LABEL_13;
LABEL_12:
      W_pslot_heal((intptr_t)(v9), (intptr_t)(1));
      goto LABEL_13;
    }
    if ( F32(F32(v8, 0), 2500) == 2 )
      goto LABEL_12;
LABEL_13:
    v8 += 4;
    --v7;
  }
  while ( v7 != 0 );
  v10 = 30;
  (*v26__f) = 100;
  (*v29__f) = 650;
  (*v28__f) = 24;
  v11 = 4;
LABEL_16:
  v12 = W_script_leader((intptr_t)(c));
  F32(c, 0x4c) = 0;
  F32(c, 0x5c) = v12;
  F32(c, 0x50) = 0;
  F32(c, 0x54) = 0;
  F32(c, 0x58) = 400;
  F32(c, 0x20) = 3;
  W_script_party_cast((intptr_t)(c));
  result = W_script_wait((intptr_t)(c));
  if ( result != 6 )
  {
    W_scene_party_gather((intptr_t)(c), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
    result = W_script_wait((intptr_t)(c));
    if ( result != 6 )
    {
      W_scene_party_stance((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x8)));
      result = W_script_wait((intptr_t)(c));
      if ( result != 6 )
      {
        if ( SYS32(0x2884) != 0
          && F32(CFG(), 372) != 0
          && (!W_sd_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(v10))
           || W_sd_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(v10)) != 0) )
        {
          F16(D(0x8a93e8), 0) = 0;
          W_sd_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
          if ( W_sd_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
          {
            if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
LABEL_27:
              g_eng.h408b90(D(0x8a9530), (SYS32(0x2884) + 0x2c), 0, D(0x8a9b6c));
          }
          else if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
          {
            goto LABEL_27;
          }
        }
        v14 = F32(c, 0x0);
        F32(v14, 216) = 0;
        F16(v14, 220) = v10;
        *(_DWORD *)&GAP(c, 0x300, 636) = 30;
        F32(c, 0x20) = 6;
        result = W_script_wait((intptr_t)(c));
        if ( result != 6 )
        {
          if ( a2 != 0 && W_script_leader((intptr_t)(c)) != 0 )
          {
            v15 = W_script_leader((intptr_t)(c));
            v16 = W_script_leader((intptr_t)(c));
            W_scene_device_spawn((intptr_t)(c), (intptr_t)(0), (intptr_t)(100145), (intptr_t)(0), (intptr_t)(F32(F32((v16 + 64), 0), 4) + F32(F32((v16 + 64), 0), 12) / 2), (intptr_t)(F32(F32((v15 + 64), 0), 20)
            + F32(F32((v15 + 64), 0), 8)
            + (F32(F32((v15 + 64), 0), 16) - F32(F32((v15 + 64), 0), 20)) / 2), (intptr_t)(0));
          }
          v17 = F32(F32(c, 0x0), 292);
          if ( F32(v17, 0x20) == 0 )
            si_40a2a0(v17, v11);
          v18 = 16432;
          for ( (*i__f) = 8; (*i__f) != 0; --(*i__f) )
          {
            v19 = F32((F32(c, 0x8) + v18), 0);
            if ( F32(v19, 2500) == 1 )
            {
              v20 = F32(v19, 2548);
              v21 = 0;
              while ( *(_DWORD *)&GAP(c, 0x300, 20 * v21 + 4) != v20 || *(&F32(c, 0x2fc) + 5 * v21) == 0 )
              {
                if ( ++v21 >= 0x20u )
                {
                  v21 = 0;
                  while ( *(&F32(c, 0x2fc) + 5 * v21) != 0 )
                  {
                    if ( ++v21 >= 0x20u )
                    {
                      v22 = 0;
                      goto LABEL_44;
                    }
                  }
                  break;
                }
              }
              v25 = &F32(c, 0x0) + 5 * v21;
              v22 = v25 + 191;
              v25[191] = 1;
              v25[192] = 1;
              v25[193] = v20;
              v25[194] = v20;
              *(_DWORD *)&GAP(c, 0x300, 20 * v21 + 12) = 0;
LABEL_44:
              v23 = v22[3];
              W_cast_cmd_clear((intptr_t)(v23));
              F32(v23, 88932) = 4;
              F32(v23, 88984) = (*v26__f);
              F32(v23, 88988) = 1;
              F32(v23, 88992) = 0;
              F32(v23, 88996) = 0;
              F32(v23, 88968) = 100;
            }
            v18 += 4;
          }
          si_409520(F32(c, 0x8), (*v28__f), 0, 0, 0, 20, 62, 0, (_DWORD *)1, 1);
          *(_DWORD *)&GAP(c, 0x300, 636) = 100;
          F32(c, 0x20) = 6;
          result = W_script_wait((intptr_t)(c));
          if ( result != 6 )
          {
            if ( a2 == 0 )
              F32(c, 0x2e4) = 1;
            F32(c, 0x20) = 6;
            *(_DWORD *)&GAP(c, 0x300, 636) = (*v29__f);
            v24 = W_script_wait((intptr_t)(c));
            if ( v24 != 6 && a2 != 0 )
              W_el_41e4c0((intptr_t)(c), (intptr_t)(100145), (intptr_t)(1), (intptr_t)(0));
            return v24;
          }
        }
      }
    }
  }
  return result;
}

/* 0x409520: int __thiscall sub_409520( cls_409520 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, _DWORD *a9, int a10) */
int32_t si_409520(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    uint16_t v24 = 0;
    intptr_t v25 = 0;
    uint16_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    uint16_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    uint16_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    int v38 = 0;
    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v42 = (intptr_t *)(frame_ + 0x8);   /* [8] at ebp-0x20 */
    uint16_t *const v40__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x28 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x28 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x24 */

  v10 = 0;
  (*v41__f) = 0;
  v11 = c + 4 * (4108);
  for ( (*i__f) = 8; (*i__f) != 0; --(*i__f) )
  {
    v12 = F32(v11, 0);
    if ( F32(F32(v11, 0), 2500) == 1 && (F32(*(_DWORD *)&v12, 2544) != 100000104 || a9 != 0) )
    {
      v14 = F32(*(_DWORD *)&v12, 2548);
      if ( F32(v14, 88916) != 0 && (F32(v14, 512) != 0 || a4 == 0) )
      {
        if ( F32(v14, 464) == 1
          && F8(F32(v14, 64), 0) == 1
          && F16(F32(v14, 1872), 1098) == 0 )
        {
          v15 = 0;
          while ( F16(F32(v14, 1872) + 2 * v15, 1096) == 0 || ((int32_t *)D(0x628764))[35 * v15] == 0 )
          {
            if ( ++v15 >= 0x12u )
              goto LABEL_21;
          }
        }
        if ( a8 == 0 )
        {
LABEL_21:
          v16 = v10++;
          v42[v16] = v12;
        }
      }
    }
    v11 += 4;
  }
  (*v40__f) = v10;
  if ( v10 == 0 )
    return 0;
  if ( a5 == 0 )
  {
    v30 = a6;
    v31 = a2;
    a8 = v10;
    v32 = a10;
    a9 = v42;
    while ( 1 )
    {
      v33 = F32(*a9, 2548);
      if ( v31 == 0 )
        goto LABEL_66;
      v34 = F32(v33, 88916);
      if ( v34 == 0
        || F32(v33, 88924) != 0
        || F32(v33, 88716) == 1 && F32(CFG(), 400) == 0 )
      {
        goto LABEL_66;
      }
      if ( v30 <= 0 )
      {
        if ( v32 != 0 )
        {
          v37 = F32(CFG(), 416) + F32(v33, 88712);
          a4 = 0;
          a2 = 0;
          a10 = v37;
        }
        else
        {
          if ( W_voice_place((intptr_t)(v33), (intptr_t)(&a10), (intptr_t)(&a4), (intptr_t)(&a2)) == 0 )
            goto LABEL_65;
          v37 = a10;
        }
        if ( W_voice_play((intptr_t)(F32(v33, 88916)), (intptr_t)(v31), (intptr_t)(a3), (intptr_t)(v37), (intptr_t)(a4), (intptr_t)(a2), (intptr_t)(v32)) == 0 )
        {
LABEL_65:
          v30 = a6;
          goto LABEL_66;
        }
      }
      else
      {
        if ( F32(v33, 51052) > 0 )
        {
          v35 = W_voice_group_prio((intptr_t)(v34), (intptr_t)(F16(v33, 51056)));
          if ( (uint16_t)W_voice_group_prio((intptr_t)(F32(v33, 88916)), (intptr_t)(v31)) < v35 )
            goto LABEL_65;
        }
        v30 = a6;
        v36 = a3;
        F32(v33, 51052) = a6;
        F16(v33, 51056) = v31;
        F32(v33, 51060) = v36;
        F32(v33, 51064) = v32;
      }
      (*v41__f) = 1;
      v30 += a7;
      a6 = v30;
LABEL_66:
      v38 = a8 == 1;
      ++a9;
      --a8;
      if ( v38 )
        return (*v41__f);
    }
  }
  v18 = v10;
  v19 = rnd();
  a8 = 0;
  v20 = 100 * v10 * v19 / 0x8000 % v10;
  v21 = a2;
  while ( 1 )
  {
    v22 = (uint16_t)v20;
    v23 = F32(v42[(uint16_t)v20], 2548);
    v24 = 0;
    if ( v21 == 0 )
      goto LABEL_45;
    v25 = F32(v23, 88916);
    if ( v25 == 0
      || F32(v23, 88924) != 0
      || F32(v23, 88716) == 1 && F32(CFG(), 400) == 0 )
    {
      goto LABEL_45;
    }
    if ( a6 > 0 )
      break;
    if ( a10 != 0 )
    {
      v27 = F32(CFG(), 416) + F32(v23, 88712);
      a4 = 0;
      a9 = 0;
      a2 = v27;
    }
    else
    {
      if ( W_voice_place((intptr_t)(v23), (intptr_t)(&a2), (intptr_t)(&a4), (intptr_t)(&a9)) == 0 )
        goto LABEL_45;
      v27 = a2;
    }
    if ( W_voice_play((intptr_t)(F32(v23, 88916)), (intptr_t)(v21), (intptr_t)(a3), (intptr_t)(v27), (intptr_t)(a4), (intptr_t)((int)a9), (intptr_t)(a10)) != 0 )
      return 1;
LABEL_45:
    v20 = (v22 + 1) % v18;
    if ( (uint16_t)++a8 >= (*v40__f) )
      return (*v41__f);
  }
  if ( F32(v23, 51052) > 0 )
  {
    if ( v21 < F16(v25, 4) )
      v24 = F16(1296 * v21 + F32(v25, 0), 2);
    v26 = F16(v23, 51056);
    if ( v26 != 0
      && v26 < F16(v25, 4)
      && v24 < F16(1296 * F16(v23, 51056) + F32(v25, 0), 2) )
    {
      goto LABEL_45;
    }
  }
  v28 = a3;
  v29 = a10;
  F32(v23, 51052) = a6;
  F16(v23, 51056) = v21;
  F32(v23, 51060) = v28;
  F32(v23, 51064) = v29;
  return 1;
}

/* 0x40a340: int __thiscall sub_40A340(cls_401D40 *this) */
int32_t si_40a340(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t i = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t result = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;

  W_script_begin((intptr_t)(c));
  si_40a5d0((int)((intptr_t)g_eng.sys), 0, 0, 0, 0, 1);
  v2 = F32(c, 0x0);
  F32(v2, 216) = 1;
  F16(v2, 220) = 0;
  if ( *(_DWORD *)&GAP(W, 10148 + 0x0, 16) != 2 )
  {
    v3 = 16432;
    for ( i = 8; i != 0; --i )
    {
      v5 = F32((v3 + F32(c, 0x8)), 0);
      if ( F32(v5, 2500) == 1 )
      {
        v6 = F32(v5, 2548);
        v7 = F32(v6, 64);
        if ( F16(v7, 72) == 111 )
          W_body_action_start((intptr_t)(v7), (intptr_t)(112), (intptr_t)(900));
        else
          W_body_action_start((intptr_t)(F32(v6, 64)), (intptr_t)(113), (intptr_t)(900));
      }
      v3 += 4;
    }
  }
  F32(c, 0x74) = 1;
  *(_DWORD *)&GAP(c, 0xb0e, 18) = 1;
  *(_DWORD *)&GAP(c, 0x300, 636) = 50;
  F32(c, 0x20) = 6;
  result = W_script_wait((intptr_t)(c));
  if ( result != 6 )
  {
    if ( SYS32(0x2884) != 0
      && F32(CFG(), 372) != 0
      && (!W_sd_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Bu))
       || W_sd_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Bu)) != 0) )
    {
      F16(D(0x8a93e8), 0) = 0;
      W_sd_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
      if ( W_sd_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
      {
        if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(27), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
          goto LABEL_17;
      }
      else if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(27), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
      {
LABEL_17:
        g_eng.h408b90(D(0x8a9530), (SYS32(0x2884) + 0x2c), 0, D(0x8a9b6c));
      }
    }
    v9 = F32(c, 0x0);
    F32(v9, 216) = 0;
    F16(v9, 220) = 27;
    *(_DWORD *)&GAP(c, 0x300, 636) = 200;
    F32(c, 0x20) = 6;
    result = W_script_wait((intptr_t)(c));
    if ( result != 6 )
    {
      F32(c, 0x74) = 0;
      *(_DWORD *)&GAP(c, 0x300, 636) = 50;
      F32(c, 0x20) = 6;
      result = W_script_wait((intptr_t)(c));
      if ( result != 6 )
      {
        F32(c, 0x30) = 1;
        F32(c, 0x24) = 1;
        F32(c, 0x34) = 3;
        F32(c, 0x28) = 2;
        F32(c, 0x20) = 2;
        F32(c, 0x3c) = 1000;
        F32(c, 0x38) = 0;
        F32(c, 0x2c) = 1000;
        v10 = W_script_wait((intptr_t)(c));
        if ( v10 != 6 )
        {
          F32(c, 0x1c) = 1;
          W_script_end((intptr_t)(c));
        }
        return v10;
      }
    }
  }
  return result;
}

/* 0x41a160: int __thiscall sub_41A160(cls_401D40 *this) */
int32_t si_41a160(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t result = 0;

  F32(c, 0x2e4) = 1;
  if ( SYS32(0x2884) != 0
    && F32(CFG(), 372) != 0
    && (!W_sd_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Au))
     || W_sd_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Au)) != 0) )
  {
    F16(D(0x8a93e8), 0) = 0;
    W_sd_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
    if ( W_sd_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
    {
      if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(26), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
        g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
    }
    else if ( W_sd_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(26), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
    {
      g_eng.h408b90(D(0x8a9530), (SYS32(0x2884) + 0x2c), 0, D(0x8a9b6c));
    }
  }
  v2 = F32(c, 0x0);
  F32(v2, 216) = 0;
  F16(v2, 220) = 26;
  *(_DWORD *)&GAP(c, 0x300, 636) = 50;
  F32(c, 0x20) = 6;
  result = W_script_wait((intptr_t)(c));
  if ( result != 6 )
  {
    F32(c, 0x3c) = 1000;
    F32(c, 0x2c) = 1000;
    F32(c, 0x30) = 1;
    F32(c, 0x34) = 3;
    F32(c, 0x38) = 0;
    F32(c, 0x24) = 1;
    F32(c, 0x28) = 2;
    F32(c, 0x20) = 2;
    result = W_script_wait((intptr_t)(c));
    if ( result != 6 )
      F32(c, 0x1c) = 1;
  }
  return result;
}

/* 0x41d4a0: int __thiscall sub_41D4A0(cls_401D40 *this, int a2, int a3) */
int32_t si_41d4a0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t result = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    uint16_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    char * v12 = 0;
    intptr_t v13 = 0;
    char * v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    char * v17 = 0;
    int32_t * v18 = 0;
    intptr_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v4 = W_actor_by_handle((intptr_t)(a2));
  result = a3;
  (*v21__f) = (int)v4;
  if ( a3 != 0 || v4 != 0 )
  {
    v6 = F32(c, 0x8);
    result = 0;
    while ( 1 )
    {
      v7 = *(_DWORD *)&GAP(v6, 0x4024, 4 * (uint16_t)result + 12);
      if ( F32(v7, 2544) == a2 )
        break;
      if ( (uint16_t)++result >= 8u )
        return result;
    }
    result = W_pslot_ok((intptr_t)(v7), (intptr_t)(1), (intptr_t)(1));
    if ( result != 0 && v7 != 0 )
    {
      v8 = 0;
      v23 = 0;
      if ( F16(v7, 2504) != 0 )
      {
        do
        {
          v9 = F16(v6, 0x4010);
          v10 = 0;
          v11 = F32(v7 + 4 * v8, 2508);
          if ( v9 != 0 )
          {
            while ( 1 )
            {
              if ( *(_DWORD *)&GAP(v6, 0x2010, 16 * v10) == v11 )
              {
                v12 = &GAP(v6, 0x0, 16 * v10);
                if ( F32(v12, 4 * 2053) == 0 && v12[8218] == 0 )
                  break;
              }
LABEL_19:
              if ( ++v10 >= v9 )
                goto LABEL_28;
            }
            v13 = F32(v12, 4 * 2055);
            switch ( F32(v7, 2544) )
            {
              case 0x5F5E165:
                switch ( F32(v13, 124) )
                {
                  case 1:
                  case 2:
                  case 4:
                  case 9:
                    goto LABEL_21;
                  default:
                    goto LABEL_19;
                }
              case 0x5F5E166:
                switch ( F32(v13, 124) )
                {
                  case 1:
                  case 3:
                  case 5:
                  case 9:
                    goto LABEL_21;
                  default:
                    goto LABEL_19;
                }
              case 0x5F5E167:
                switch ( F32(v13, 124) )
                {
                  case 1:
                  case 3:
                  case 6:
                  case 9:
                    goto LABEL_21;
                  default:
                    goto LABEL_19;
                }
              case 0x5F5E168:
                if ( F32(v13, 124) != 8 )
                  goto LABEL_19;
LABEL_21:
                v14 = &GAP(v6, 0x0, 16 * v10);
                v15 = F32(v14, 4 * 2055);
                if ( a2 != 0 )
                {
                  v16 = 0;
                  do
                  {
                    v17 = &GAP(v6, 0x0, 16 * v16);
                    if ( F32(v17, 4 * 2053) == a2
                      && F32(F32(v17, 4 * 2055), 120) == F32(v15, 120) )
                    {
                      F32(v17, 4 * 2053) = 0;
                    }
                    ++v16;
                  }
                  while ( v16 < F16(v6, 0x4010) );
                }
                F32(v14, 4 * 2053) = a2;
                break;
              default:
                goto LABEL_19;
            }
          }
LABEL_28:
          v8 = ++v23;
        }
        while ( v23 < F16(v7, 2504) );
        v4 = (*v21__f);
      }
      F16(v7, 2504) = 0;
      if ( v4 != 0 )
        W_member_attach((intptr_t)(v4), (intptr_t)((_DWORD *)v7), (intptr_t)(F32(v6, 0x4018)));
      F32(v7, 2500) = 1;
      F32(v7, 2548) = v4;
      v18 = W_area_level_row((intptr_t)(F32(v6, 0x401c)));
      v19 = (intptr_t *)&GAP(v6, 0x4024, 12);
      v22 = 8;
      F32(v6, 0x4020) = v18[3];
      do
      {
        v20 = *v19;
        if ( W_pslot_ok((intptr_t)(*v19), (intptr_t)(0), (intptr_t)(0)) )
        {
          W_tm_41d780((intptr_t)(v20), (intptr_t)(0), (intptr_t)(v18[1]), (intptr_t)(v18[2]));
        }
        else if ( W_pslot_ok((intptr_t)(v20), (intptr_t)(0), (intptr_t)(1)) )
        {
          W_tm_41d780((intptr_t)(v20), (intptr_t)(1), (intptr_t)(v18[1]), (intptr_t)(v18[2]));
        }
        ++v19;
        --v22;
      }
      while ( v22 != 0 );
      W_room_stats_refresh((intptr_t)(v6), (intptr_t)(1));
      result = (*v21__f);
      if ( (*v21__f) != 0 )
        return (int)W_room_formation((intptr_t)(v6));
    }
  }
  return result;
}

/* 0x40a2a0: int __thiscall sub_40A2A0(cls_40A2A0 *this, int a2) */
int32_t si_40a2a0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = a2;
  F32(c, 0x0) = a2;
  F32(c, 0x20) = 1;
  F16(((uint8_t *)(c) + 0x4), 0) = 0;
  *(_WORD *)&GAP(c, 0x4, 2) = 0;
  *(_DWORD *)&F16(c, 0x8) = 0;
  F32(c, 0x1c) = 0;
  if ( a2 >= 2 && a2 <= 4 )
  {
    F32(c, 0x14) = 120;
    F32(c, 0x18) = 96;
  }
  return result;
}

/* 0x408b90: void __thiscall sub_408B90(log_singleton *this, LPCSTR msg, int errno_decorate, LPCSTR trailer) */
void si_408b90(wl_char *self, char * msg, int32_t errno_decorate, char * trailer)
{
    intptr_t c = (intptr_t)self;
    uint32_t LastError = 0;
    FILE * v6 = 0;
    FILE * v7 = 0;
    _Alignas(8) uint8_t frame_[0x210 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x10);   /* [256] at ebp-0x200 */
    char *Buffer = (char *)(frame_ + 0x110);   /* [256] at ebp-0x100 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x210 */

  if ( errno_decorate != 0 )
  {
    LastError = API_GetLastError();
    if ( LastError != 0 )
    {
      API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
      g_eng.h5bf3ee(OutputString, "[%08d] %s", LastError, Buffer);
      API_OutputDebugStringA((intptr_t)(OutputString));
      API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
      v6 = CRT_fopen(((uint8_t *)c + 0x404), ((char *)D(0x85456c)));
      if ( v6 != 0 )
      {
        API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
        CRT_fprintf(
          v6,
          "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
          (*SystemTime__f).wYear,
          (*SystemTime__f).wMonth,
          (*SystemTime__f).wDay,
          (*SystemTime__f).wHour,
          (*SystemTime__f).wMinute,
          (*SystemTime__f).wSecond,
          OutputString,
          D(0x8a9b6c));
        CRT_fclose(v6);
      }
    }
  }
  API_OutputDebugStringA((intptr_t)(msg));
  API_OutputDebugStringA((intptr_t)(trailer));
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
  v7 = CRT_fopen(((uint8_t *)c + 0x404), ((char *)D(0x85456c)));
  if ( v7 != 0 )
  {
    API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
    CRT_fprintf(
      v7,
      "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
      (*SystemTime__f).wYear,
      (*SystemTime__f).wMonth,
      (*SystemTime__f).wDay,
      (*SystemTime__f).wHour,
      (*SystemTime__f).wMinute,
      (*SystemTime__f).wSecond,
      msg,
      trailer);
    CRT_fclose(v7);
  }
}

/* 0x40a5d0: void __thiscall sub_40A5D0(int this, int a2, int a3, int a4, int a5, int a6) */
void si_40a5d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    char * v10 = 0;
    FILE * v11 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x10 */

  v7 = F32(c, 10372);
  if ( v7 != 0
    && F32(F32(F32(c, 800), 0), 372) != 0
    && (!W_sd_5bbc60((intptr_t)(v7), (intptr_t)(a2)) || W_sd_5bbc90((intptr_t)(F32(c, 10372)), (intptr_t)(a2)) != 0) )
  {
    if ( a3 != 0 )
      F16(c, 10376) = a2;
    else
      F16(c, 10376) = 0;
    if ( a6 != 0 )
      W_sd_5bbcd0((intptr_t)(F32(c, 10372)), (intptr_t)(20));
    if ( a4 != 0 )
    {
      if ( W_sd_5bcb80((intptr_t)(F32(c, 10372)), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(a5), (intptr_t)(1)) == 0 )
        g_eng.log_fail_406440((const char *)D(0x8a9530), (char *)(F32(c, 10372) + 44), D(0x8a9b6c));
      return;
    }
    v8 = W_sd_5bbc20((intptr_t)(F32(c, 10372)));
    v9 = F32(c, 10372);
    if ( v8 != 0 )
    {
      if ( W_sd_5bcb80((intptr_t)(v9), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
      {
        v10 = (const CHAR *)(F32(c, 10372) + 44);
        API_OutputDebugStringA((intptr_t)(v10));
        API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
        API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
        v11 = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
        if ( v11 != 0 )
        {
LABEL_19:
          API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
          CRT_fprintf(
            v11,
            "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
            (*SystemTime__f).wYear,
            (*SystemTime__f).wMonth,
            (*SystemTime__f).wDay,
            (*SystemTime__f).wHour,
            (*SystemTime__f).wMinute,
            (*SystemTime__f).wSecond,
            v10,
            D(0x8a9b6c));
          CRT_fclose(v11);
        }
      }
    }
    else if ( W_sd_5bcb80((intptr_t)(v9), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
    {
      v10 = (const CHAR *)(F32(c, 10372) + 44);
      API_OutputDebugStringA((intptr_t)(v10));
      API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
      v11 = CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
      if ( v11 != 0 )
        goto LABEL_19;
    }
  }
}
/* unported callees: none */
