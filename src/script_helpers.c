/* script_helpers.c — the chapter scripts' callees whose closure stays clear of file / sound / DirectX calls.  See
 * script_helpers.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (IDA's control flow and types, real class layouts, stack locals at
 * their ebp offsets); no hand finish (the fresh export shows every call's ecx). */
#include "script_helpers.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }



#include "actor_update.h"
#include "char_state.h"
#include "entity_life.h"
#include "entity_move.h"
#include "gimmick.h"
#include "npc_ai.h"
#include "party_member.h"
#include "room_load.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "actor_update.h"
#include "char_state.h"
#include "entity_life.h"
#include "entity_move.h"
#include "gimmick.h"
#include "npc_ai.h"
#include "party_member.h"
#include "room_load.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
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
static inline intptr_t W_actor_release(intptr_t a0, ...) { actor_release((wl_actor *)a0); return 0; }
static inline intptr_t W_char_ai_reset(intptr_t a0, intptr_t a1, ...) { char_ai_reset((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_char_set_look_up(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)char_set_look_up((wl_char *)a0, (int16_t)a1, (const void *)a2, (const void *)a3, (int16_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_char_skills_refresh(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skills_refresh((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_entity_anchor(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { entity_anchor((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4, (int32_t *)a5); return 0; }
static inline intptr_t W_entity_ground_probe(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)entity_ground_probe((wl_entity *)a0, (wl_body *)a1, (uint16_t)a2); }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_entity_release(intptr_t a0, intptr_t a1, ...) { entity_release((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_set_collide(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_set_collide((wl_entity *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_entity_set_handle(intptr_t a0, intptr_t a1, ...) { entity_set_handle((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_unlist(intptr_t a0, ...) { entity_unlist((wl_entity *)a0); return 0; }
static inline intptr_t W_gate_actor(intptr_t a0, ...) { return (intptr_t)gate_actor((int32_t)a0); }
static inline intptr_t W_item_give(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)item_give((wl_event *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4); }
static inline intptr_t W_member_palette(intptr_t a0, ...) { member_palette((wl_char *)a0); return 0; }
static inline intptr_t W_part_bottom(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_bottom((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_pslot_input_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)pslot_input_set((wl_pslot *)a0, (void *)a1); }
static inline intptr_t W_room_formation(intptr_t a0, ...) { return (intptr_t)room_formation((wl_room *)a0); }
static inline intptr_t W_scene_face(intptr_t a0, intptr_t a1, intptr_t a2, ...) { scene_face((wl_event *)a0, (void *)a1, (int32_t)a2); return 0; }
static inline intptr_t W_scene_msg(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { scene_msg((wl_event *)a0, (uint32_t)a1, (void *)a2, (const char *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (const char *)a7, (int32_t)a8, (int32_t)a9, (uint16_t)a10); return 0; }
static inline intptr_t W_scene_party_stance(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)scene_party_stance((wl_event *)a0, (int32_t)a1, (wl_room *)a2); }
static inline intptr_t W_scene_spawn_at_door(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)scene_spawn_at_door((wl_event *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8); }
static inline intptr_t W_scene_talk_face(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)scene_talk_face((wl_event *)a0, (int32_t)a1, (void *)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_scene_yes_no(intptr_t a0, ...) { return (intptr_t)scene_yes_no((wl_event *)a0); }
static inline intptr_t W_script_begin(intptr_t a0, ...) { script_begin((wl_event *)a0); return 0; }
static inline intptr_t W_script_end(intptr_t a0, ...) { script_end((wl_event *)a0); return 0; }
static inline intptr_t W_script_flag(intptr_t a0, intptr_t a1, ...) { return (intptr_t)script_flag((const wl_event *)a0, (int32_t)a1); }
static inline intptr_t W_script_flag_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { script_flag_set((wl_event *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_script_leader(intptr_t a0, ...) { return (intptr_t)script_leader((const wl_event *)a0); }
static inline intptr_t W_script_party_cast(intptr_t a0, ...) { script_party_cast((wl_event *)a0); return 0; }
static inline intptr_t W_script_wait(intptr_t a0, ...) { return (intptr_t)script_wait((wl_event *)a0); }
static inline intptr_t W_stats_recompute(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { stats_recompute((wl_room *)a0, (int32_t)a1, (uint8_t *)a2, (const uint8_t *)a3); return 0; }
static inline intptr_t W_switch_glow(intptr_t a0, intptr_t a1, intptr_t a2, ...) { switch_glow((wl_actor *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_wl_list_remove(intptr_t a0, intptr_t a1, ...) { wl_list_remove((wl_list *)a0, (const wl_entity *)a1); return 0; }

int32_t sc_401d80(int32_t a1, int32_t a2, int32_t a3);
int32_t sc_402030(wl_char *self);
void sc_4020a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
void sc_4024e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
void sc_402610(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t sc_402f40(wl_char *self);
int32_t sc_406500(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_406ab0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t sc_406c70(int32_t a1, int32_t a2, int32_t a3, uint16_t a4, int32_t a5);
int32_t sc_41a3a0(wl_char *self, int32_t a2);
int32_t sc_41e2b0(wl_char *self, int32_t a2);
int32_t sc_41e340(int32_t a1, int32_t a2);
void sc_41eb30(int32_t ecx0, int32_t a2, int32_t a1);
int32_t sc_41ece0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t sc_41efe0(int32_t a1, int32_t a2);
int32_t sc_41f050(int32_t a1, int32_t a2);
int32_t sc_439510(wl_char *self, int32_t a2);
int32_t sc_439560(wl_char *self, int32_t a2);
int32_t sc_49cf60(int32_t a1);
void sc_49cf80(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t sc_49e250(wl_char *self);
int32_t sc_49e6c0(wl_char *self);
int32_t sc_4a5e40(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3);
int32_t sc_4a5e70(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t sc_4a5ea0(wl_char *self);
int32_t sc_4a5ec0(wl_char *self);
int32_t sc_4a5f80(wl_char *self, int32_t a2);
int32_t sc_4a63d0(wl_char *self, uint32_t a2);
int32_t sc_4ac610(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t sc_4ac660(wl_char *self, int32_t a2, char * a3);
int32_t sc_4b3b10(void);
int32_t sc_4b3f30(wl_char *self);
int32_t sc_402f60(wl_char *self, char * a2, char * a3);
int32_t sc_4b4570(wl_char *self);
void sc_4bbe00(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
void sc_4bbe40(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
int32_t sc_4bbe80(int32_t * a1, int32_t * a2);
void sc_4c5300(int32_t a1);
int32_t sc_4cbfb0(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t sc_4cbfd0(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t sc_41f000(int32_t a1, int32_t a2);
int32_t sc_4cbff0(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cc010(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cc030(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t sc_4cc250(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cc470(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t sc_4cca20(wl_char *self, int32_t a2, int32_t a3);
int32_t sc_4cca50(wl_char *self, int32_t a2, int16_t a3);
int32_t sc_4d6490(intptr_t ecx_, int32_t a1, int32_t a2);
int32_t sc_5b4330(int32_t a1, int32_t a2, int32_t a3, int32_t a4);

/* 0x401d80: int __stdcall sub_401D80(int a1, int a2, int a3) */
int32_t sc_401d80(int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t result;

  if ( a1 == 0 || a2 == 0 )
    return 1;
  result = F32(F32((a1 + 64), 0), 4) + F32(F32((a1 + 64), 0), 12) / 2 >= F32(F32((a2 + 64), 0), 4)
                                                                                               + F32(F32((a2 + 64), 0), 12)
                                                                                               / 2
         ? 3
         : 1;
  if ( a3 != 0 )
    return result != 3 ? 3 : 1;
  return result;
}

/* 0x402030: int __thiscall sub_402030(cls_401D40 *this) */
int32_t sc_402030(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int32_t * v1;
    int32_t * v2;
    intptr_t v3;
    intptr_t v4;
    intptr_t result;

  v1 = (_DWORD *)F32(c, 0x0);
  v2 = (_DWORD *)F32(c, 0x8);
  v3 = v1[578];
  v4 = v1[577];
  result = v1[576];
  v2[4106] = v4;
  v2[4105] = result;
  v2[4107] = v3;
  return result;
}

/* 0x4020a0: void __thiscall sub_4020A0(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) */
void sc_4020a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10;
    intptr_t v11;
    int32_t * v12;
    intptr_t v13;
    intptr_t v14;
    uint16_t v15;
    int32_t * v16;
    intptr_t v17;
    int32_t * v18;
    int v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;

  if ( F32(c, 0x0) != 0 )
  {
    if ( a3 != 0 )
    {
      v10 = W_gate_actor(a3);
      if ( v10 == 0 )
        return;
      v11 = a4 + F32(F32(v10, 0x40), 4);
      v22 = W_part_bottom(v10, 0) + a5;
    }
    else
    {
      if ( a2 == 0 )
        return;
      v12 = F32(a2, 64);
      v11 = a4 + v12[1] + v12[3] / 2;
      v22 = v12[2] + v12[4] + a5 - 1;
    }
    v21 = 16432;
    v20 = 8;
    do
    {
      v13 = F32((v21 + F32(c, 0x8)), 0);
      if ( F32(v13, 2500) == 1 )
      {
        v14 = F32(v13, 2548);
        v15 = 0;
        while ( *(_DWORD *)&GAP(c, 0x300, 20 * v15 + 4) != v14 || *(&F32(c, 0x2fc) + 5 * v15) == 0 )
        {
          if ( ++v15 >= 0x20u )
          {
            v15 = 0;
            while ( *(&F32(c, 0x2fc) + 5 * v15) != 0 )
            {
              if ( ++v15 >= 0x20u )
              {
                v16 = 0;
                goto LABEL_17;
              }
            }
            break;
          }
        }
        v18 = &F32(c, 0x0) + 5 * v15;
        v16 = v18 + 191;
        v18[191] = 1;
        v18[192] = 1;
        v18[193] = v14;
        v18[194] = v14;
        *(_DWORD *)&GAP(c, 0x300, 20 * v15 + 12) = 0;
LABEL_17:
        v17 = v16[3];
        F32(v17, 88944) = v22;
        F32(v17, 88964) = v22;
        F32(v17, 88972) = 0;
        F16(v17, 88936) = 0;
        F16(v17, 88938) = 0;
        F32(v17, 88976) = 0;
        F32(v17, 89000) = 0;
        F16(v17, 82588) = 0;
        F16(v17, 83872) = 0;
        F32(v17, 88932) = 1;
        F32(v17, 88940) = v11;
        F32(v17, 88948) = a6;
        F32(v17, 88952) = a8;
        F32(v17, 88956) = 0;
        F32(v17, 88960) = v11;
        F32(v17, 88968) = a7;
        if ( a6 == 1 )
          v11 -= a9;
        else
          v11 += a9;
      }
      v19 = v20 == 1;
      v21 += 4;
      --v20;
    }
    while ( !v19 );
    F32(c, 0x20) = 4;
  }
}

/* 0x4024e0: void __thiscall sub_4024E0(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
void sc_4024e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    uint16_t v7;
    int32_t * v8;
    intptr_t v9;
    int32_t * v10;

  if ( a2 != 0 )
  {
    v7 = 0;
    while ( *(_DWORD *)&GAP(c, 0x300, 20 * v7 + 4) != a2 || *(&F32(c, 0x2fc) + 5 * v7) == 0 )
    {
      if ( ++v7 >= 0x20u )
      {
        v7 = 0;
        while ( *(&F32(c, 0x2fc) + 5 * v7) != 0 )
        {
          if ( ++v7 >= 0x20u )
          {
            v8 = 0;
            goto LABEL_10;
          }
        }
        break;
      }
    }
    v10 = &F32(c, 0x0) + 5 * v7;
    v8 = v10 + 191;
    v10[191] = 1;
    v10[192] = 1;
    v10[193] = a2;
    v10[194] = a2;
    *(_DWORD *)&GAP(c, 0x300, 20 * v7 + 12) = 0;
LABEL_10:
    v9 = v8[3];
    F32(v9, 88984) = a3;
    F32(v9, 88988) = a6;
    F32(v9, 88992) = a5;
    F32(v9, 88996) = a4;
    F32(v9, 88972) = 0;
    F16(v9, 88936) = 0;
    F16(v9, 88938) = 0;
    F32(v9, 88976) = 0;
    F32(v9, 89000) = 0;
    F16(v9, 82588) = 0;
    F16(v9, 83872) = 0;
    F32(v9, 88932) = 5;
    F32(v9, 88968) = 100;
    F32(v9, 88980) = a7;
    F32(c, 0x20) = 4;
  }
}

/* 0x402610: void __thiscall sub_402610(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6) */
void sc_402610(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    uint16_t v6;
    int32_t * v7;
    intptr_t v8;
    int32_t * v9;

  if ( a2 != 0 )
  {
    v6 = 0;
    while ( *(_DWORD *)&GAP(c, 0x300, 20 * v6 + 4) != a2 || *(&F32(c, 0x2fc) + 5 * v6) == 0 )
    {
      if ( ++v6 >= 0x20u )
      {
        v6 = 0;
        while ( *(&F32(c, 0x2fc) + 5 * v6) != 0 )
        {
          if ( ++v6 >= 0x20u )
          {
            v7 = 0;
            goto LABEL_10;
          }
        }
        break;
      }
    }
    v9 = &F32(c, 0x0) + 5 * v6;
    v7 = v9 + 191;
    v9[191] = 1;
    v9[192] = 1;
    v9[193] = a2;
    v9[194] = a2;
    *(_DWORD *)&GAP(c, 0x300, 20 * v6 + 12) = 0;
LABEL_10:
    v8 = v7[3];
    F32(v8, 88984) = a3;
    F32(v8, 88988) = a6;
    F32(v8, 88992) = a5;
    F32(v8, 88972) = 0;
    F16(v8, 88936) = 0;
    F16(v8, 88938) = 0;
    F32(v8, 88976) = 0;
    F32(v8, 89000) = 0;
    F16(v8, 82588) = 0;
    F16(v8, 83872) = 0;
    F32(v8, 88932) = 4;
    F32(v8, 88996) = a4;
    F32(v8, 88968) = 100;
    F32(c, 0x20) = 4;
  }
}

/* 0x402f40: int __thiscall sub_402F40(cls_401D40 *this) */
int32_t sc_402f40(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;

  result = F32(F32(c, 0x8), 8204);
  if ( result != 0 )
    return F32(result, 2544);
  return result;
}

/* 0x406500: int __thiscall sub_406500(cls_401D40 *this, int a2, int a3) */
int32_t sc_406500(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    uint16_t * v13;
    intptr_t v14;
    intptr_t i;
    intptr_t v16;
    int32_t * v17;
    int32_t * v18;
    intptr_t v19;
    intptr_t j;
    intptr_t v23;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */

  switch ( a2 )
  {
    case 1:
      v3 = a3;
      break;
    case 2:
      v3 = 10 * a3;
      break;
    case 3:
      v3 = 100 * a3;
      break;
    default:
      v9 = F32(c, 0x8);
      v10 = 0;
      v11 = -1;
      v12 = 0;
      v23 = 0;
      if ( F16(v9, 16400) != 0 )
      {
        v13 = (uint16_t *)(v9 + 8216);
        v14 = F16(v9, 16400);
        do
        {
          if ( *((uint8_t *)v13 + 2) == 0 && *((int32_t *)v13 - 2) == a2 )
          {
            if ( &(*v21__f) != (int *)-24 && (v11 == -1 || v12 > *v13) )
            {
              v11 = v10;
              v12 = *v13;
            }
            ++v23;
          }
          ++v10;
          v13 += 8;
          --v14;
        }
        while ( v14 != 0 );
      }
      if ( &(*v22__f) == (int *)-20 || (i = v11, v11 < 0) )
        i = v23;
      if ( (uint16_t)v23 != 0 && (uint16_t)i < F16(v9, 16400) )
      {
        v16 = i;
        for ( i = (uint16_t)i;
              (uint16_t)v16 < F16(v9, 16400) - 1;
              i = (uint16_t)v16 )
        {
          v17 = (_DWORD *)(v9 + 16 * (i + 514));
          v18 = (_DWORD *)(v9 + 16 * (i + 513));
          ++v16;
          *v18 = *v17;
          v18[1] = v17[1];
          v18[2] = v17[2];
          v18[3] = v17[3];
        }
        --F16(v9, 16400);
        F16(v9, 16496) = 0;
        v19 = v9 + 16432;
        for ( j = 8; j != 0; --j )
        {
          if ( F32(F32(v19, 0), 2500) == 1 || F32(F32(v19, 0), 2500) == 2 )
            W_stats_recompute(v9, F32(F32(v19, 0), 2544), F32(v19, 0) + 2576, 0);
          if ( F32(F32(v19, 0), 2500) == 1 && F32(F32(v19, 0), 2548) != 0 )
            W_member_palette(F32(F32(v19, 0), 2548));
          v19 += 4;
        }
      }
      return 0;
  }
  v4 = F32(c, 0x8);
  v5 = -v3;
  if ( v5 > 0 && F32(v4, 16416) <= F32(v4, 16404) )
    return 0;
  v6 = v5 + F32(v4, 16404);
  F32(v4, 16404) = v6;
  if ( v5 <= 0 )
    return 0;
  v7 = F32(v4, 16416);
  if ( v7 >= v6 )
    v7 = v6;
  F32(v4, 16404) = v7;
  return 0;
}

/* 0x406ab0: int __thiscall sub_406AB0(cls_401D40 *this, int a2, int a3, int a4, int a5) */
int32_t sc_406ab0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    uint16_t v5;
    intptr_t v6;
    uint16_t v7;
    intptr_t v8;
    uint16_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    uint16_t v15;
    int32_t * v16;
    uint16_t i;
    intptr_t v18;
    intptr_t v19;
    _Alignas(8) uint8_t frame_[0x208 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x8);   /* [512] at ebp-0x200 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x208 */

  v5 = 0;
  v6 = F32(c, 0x8);
  (*v20__f) = 0;
  v7 = 0;
  while ( 1 )
  {
    v8 = F32(v6 + 4 * v7, 16432);
    if ( F32(v8, 2544) == a2 )
      break;
    if ( ++v7 >= 8u )
      return 0;
  }
  if ( (F32(v8, 2500) == 1 || F32(v8, 2500) == 2 && a5 != 0) && v8 != 0 )
  {
    v10 = F16(v8, 2900);
    if ( v10 != 0 )
    {
      while ( F32(v8 + 2576 + 12 * v5, 328) != a3 )
      {
        if ( ++v5 >= v10 )
          goto LABEL_12;
      }
    }
    else
    {
LABEL_12:
      v11 = sc_406c70(v8 + 2576, a3, 255, 0, 1);
      v12 = F32(v8, 2548);
      v13 = v11;
      if ( v12 != 0 )
        W_char_skills_refresh(v12, 0);
      if ( v13 != 0 && a4 != 0 )
      {
        v14 = 10101;
        v15 = 0;
        while ( v14 != a3 )
        {
          v14 = ((int32_t *)D(0x6826c0))[192 * ++v15];
          if ( v14 == 0 )
          {
            v16 = 0;
            goto LABEL_20;
          }
        }
        v16 = &((int32_t *)D(0x6826c0))[192 * v15];
LABEL_20:
        for ( i = 0; ; ++i )
        {
          v18 = ((int32_t *)D(0x6b6ea8))[13 * i];
          if ( v18 == 0 )
            break;
          if ( v18 == a2 )
          {
            v19 = 13 * i;
            if ( 4 * v19 != -7040680 && v16 != 0 )
            {
              g_eng.h5bf3ee(
                Buffer,
                "%s learned %s!",
                (const char *)(4 * v19 + 7040688),
                (uint8_t *)v16 + 4);
              W_scene_msg(c, 3u, 0, Buffer, 0, 1, 0, 0, 0, 0, 0);
              return W_script_wait(c);
            }
            return (*v20__f);
          }
        }
      }
    }
  }
  return (*v20__f);
}

/* 0x406c70: int __stdcall sub_406C70(int a1, int a2, int a3, unsigned __int16 a4, int a5) */
int32_t sc_406c70(int32_t a1, int32_t a2, int32_t a3, uint16_t a4, int32_t a5)
{

    uint16_t v6;
    intptr_t v7;
    int32_t * v8;
    uint16_t i;
    uint16_t v10;
    intptr_t v11;
    uint16_t v12;
    intptr_t v13;
    intptr_t v14;
    int32_t * v15;
    intptr_t v16;
    uint16_t v17;
    uint16_t v18;
    intptr_t v19;
    uint16_t v20;
    int32_t * v21;
    intptr_t v22;
    uint16_t v24;

  v6 = 0;
  v24 = F16(a1, 324);
  v7 = 10101;
  while ( v7 != a2 )
  {
    v7 = ((int32_t *)D(0x6826c0))[192 * ++v6];
    if ( v7 == 0 )
    {
      v8 = 0;
      goto LABEL_5;
    }
  }
  v8 = &((int32_t *)D(0x6826c0))[192 * v6];
LABEL_5:
  if ( a5 != 0 && v8 != 0 )
  {
    if ( v8[42] != 0 )
    {
      for ( i = 0; i < 0xAu; ++i )
      {
        v10 = 0;
        if ( v24 == 0 )
          goto LABEL_24;
        while ( 1 )
        {
          v11 = a1 + 12 * v10;
          if ( F32(v11, 332) == 1 && F16((a1 + 4 * (3 * v10 + 84)), 0) == i )
          {
            v12 = 0;
            v13 = F32(v11, 328);
            v14 = 10101;
            while ( v14 != v13 )
            {
              v14 = ((int32_t *)D(0x6826c0))[192 * ++v12];
              if ( v14 == 0 )
                goto LABEL_19;
            }
            v15 = &((int32_t *)D(0x6826c0))[192 * v12];
            if ( v15 != 0 && v15[42] != 0 )
              break;
          }
LABEL_19:
          if ( ++v10 >= v24 )
            goto LABEL_24;
        }
        if ( a1 + 12 * v10 == -328 )
        {
LABEL_24:
          v16 = 1;
          a4 = i;
          goto LABEL_44;
        }
      }
      v17 = 0;
      v16 = 2;
      while ( 2 )
      {
        v18 = 0;
        while ( 1 )
        {
          if ( F32(a1 + 12 * v18, 332) == 2 && F16((a1 + 4 * (3 * v18 + 84)), 0) == v17 )
          {
            v19 = 10101;
            v20 = 0;
            while ( v19 != F32(a1 + 12 * v18, 328) )
            {
              v19 = ((int32_t *)D(0x6826c0))[192 * ++v20];
              if ( v19 == 0 )
                goto LABEL_35;
            }
            v21 = &((int32_t *)D(0x6826c0))[192 * v20];
            if ( v21 != 0 && v21[42] != 0 )
              break;
          }
LABEL_35:
          if ( ++v18 >= v24 )
            goto LABEL_40;
        }
        if ( a1 + 12 * v18 == -328 )
        {
LABEL_40:
          a4 = v17;
          goto LABEL_44;
        }
        if ( ++v17 < 0xAu )
          continue;
        break;
      }
      v16 = 255;
    }
    else
    {
      v16 = 1;
    }
  }
  else
  {
    v16 = a3;
  }
LABEL_44:
  v22 = a1 + 12 * v24;
  F32(v22, 328) = a2;
  F32(v22, 332) = v16;
  F16((a1 + 4 * (3 * v24 + 84)), 0) = a4;
  ++F16(a1, 324);
  return v22 + 328;
}

/* 0x41a3a0: int __thiscall sub_41A3A0(cls_401D40 *this, int a2) */
int32_t sc_41a3a0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t v4;
    intptr_t v5;
    uint16_t v6;

  result = 0;
  v4 = CFG();
  if ( a2 > F32(CFG(), 2780) )
  {
    v5 = F32(v4, 2788);
    F32(v4, 2780) = a2;
    if ( v5 != 0 )
    {
      v6 = F16(v4, 2794);
      if ( v6 != 0 )
      {
        while ( F32((v5 + 8 * (uint16_t)result), 0) != a2 )
        {
          SETLO(result, result + 1);
          if ( (uint16_t)result >= v6 )
            goto LABEL_8;
        }
        F32(v4, 2796) = F32(v5 + 8 * (uint16_t)result, 4);
      }
    }
LABEL_8:
    W_scene_msg(c, 3u, 0, D(0x854ddc), 0, 1, 0, 0, 0, 0, 0);
    return W_script_wait(c);
  }
  return result;
}

/* 0x41e2b0: _DWORD *__thiscall sub_41E2B0(cls_401D40 *this, int a2) */
int32_t sc_41e2b0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    int32_t * result;
    intptr_t v3;
    intptr_t v4;

  result = (_DWORD *)F32(c, 0x8);
  if ( a2 != 0 )
  {
    result[4148] = 0;
  }
  else
  {
    v3 = result[4106];
    result[4148] = result[4105];
    v4 = result[4107];
    result[4149] = v3;
    result[4150] = v4;
  }
  return result;
}

/* 0x41e340: int __stdcall sub_41E340(int a1, int a2) */
int32_t sc_41e340(int32_t a1, int32_t a2)
{

    intptr_t result;

  result = a1;
  if ( a1 != 0 )
    F32(a1, 88924) = a2;
  return result;
}

/* 0x41eb30: void __fastcall sub_41EB30(int ecx0, int a2, cls_4022A0 *a1) */
void sc_41eb30(int32_t ecx0, int32_t a2, int32_t a1)
{

    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t i;
    intptr_t v8;

  if ( a1 != 0 )
  {
    if ( F32(a1, 0x1b8) == 2 )
    {
      v3 = F32(a1, 0x1f8);
      if ( v3 != 0 )
      {
        W_actor_release(v3);
        F32(a1, 0x1f8) = 0;
      }
      v4 = *(_DWORD *)&GAP(W, 0x2788, 16);
      v5 = 0;
      SETLO(v3, F16(v4, 8));
      v6 = v3;
      if ( (uint16_t)v3 != 0 )
      {
        while ( F32((F32(v4, 4) + 4 * (uint16_t)v5), 0) != a1 )
        {
          v5 = ((uint8_t *)v5 + 1);
          if ( (uint16_t)v5 >= (uint16_t)v3 )
            goto LABEL_12;
        }
        v6 = v5;
      }
LABEL_12:
      if ( (uint16_t)v6 < (uint16_t)v3 )
      {
        for ( i = (uint16_t)v6; (uint16_t)v6 < F16(v4, 8) - 1; i = (uint16_t)v6 )
        {
          v6 = ((uint8_t *)v6 + 1);
          F32((F32(v4, 4) + 4 * i), 0) = F32(F32((v4 + 4), 0) + 4 * i, 4);
        }
        --F16(v4, 8);
      }
      W_entity_unlist(a1 /*edx*/);
      W_entity_release((int)a1, /*edx*/ 1);
      if ( F32(a1, 0x2a8) != 0 )
      {
        g_eng.op_delete((void *)(void *)F32(a1, 0x2a8));
        F32(a1, 0x2a8) = 0;
      }
    }
    else if ( F32(a1, 0x1b8) == 5 )
    {
      W_entity_release((int)a1, /*edx*/ 0);
    }
  }
}

/* 0x41ece0: int __thiscall sub_41ECE0(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t sc_41ece0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    int v16;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  result = F32(c, 0x0);
  (*v20__f) = c;
  (*v17__f) = 0;
  if ( F32(c, 0x0) != 0 )
  {
    (*v18__f) = 16432;
    (*v19__f) = 8;
    while ( 1 )
    {
      v8 = F32((F32(c, 0x8) + (*v18__f)), 0);
      if ( F32(v8, 2500) == 1 )
      {
        v9 = F32(v8, 2548);
        if ( a2 != 0 )
        {
          v10 = W_gate_actor(a2);
          if ( v10 != 0 )
          {
            v11 = F32(F32(v10, 0x40), 4);
            v12 = W_part_bottom(v10, 0);
            W_entity_anchor(v9, a3 + (*v17__f) + v11, a4 + v12 + 1, 3, (_DWORD *)(F32(v9, 0x40) + 4), (int *)(F32(v9, 0x40) + 8));
          }
        }
        if ( a5 != 99 )
          F32(F32(v9, 0x40), 44) = a5;
        if ( a5 == 1 )
          (*v17__f) -= a6;
        else
          (*v17__f) += a6;
        if ( a7 != 0 )
        {
          v13 = F32(v8, 2548);
          v14 = F32(v13, 0x40);
          F32(F32(v13, 0x750), 212) = 0;
          if ( F32(v13, 0x1d4) == 50010 )
          {
            F32(v13, 0x158a0) = 0;
            W_char_set_look_up(v13, 500, (int)D(0x62e3c0), (int)D(0x62e558), 78, 200, 0);
            F16(v13, 0xe8) = 0;
            F32(v13, 0x15914) = (int)D(0x62a8c8);
            F32(v13, 0x15a80) = 1;
          }
          v15 = F32(v13, 0x15914);
          if ( F32(v14, 108) != v15 )
          {
            F32(v14, 108) = v15;
            F8(v14, 116) = 0;
            F16(v14, 114) = 0;
            F16(v14, 112) = 0;
          }
        }
      }
      result = (*v19__f) - 1;
      v16 = (*v19__f) == 1;
      (*v18__f) += 4;
      --(*v19__f);
      if ( v16 )
        break;
      c = (*v20__f);
    }
  }
  return result;
}

/* 0x41efe0: int __stdcall sub_41EFE0(int a1, int a2) */
int32_t sc_41efe0(int32_t a1, int32_t a2)
{

    intptr_t result;

  result = a1;
  if ( a1 != 0 )
  {
    F32(a1, 252) = a2;
    F32(a1, 256) = a2;
  }
  return result;
}

/* 0x41f050: int __stdcall sub_41F050(int a1, int a2) */
int32_t sc_41f050(int32_t a1, int32_t a2)
{

    intptr_t result;
    intptr_t v3;

  result = a1;
  if ( a1 != 0 )
  {
    v3 = F32(a1, 64);
    result = 0;
    if ( F16(v3, 86) / 0x64u <= 9 || F16(v3, 72) == 0 )
    {
      result = a2;
      if ( (uint16_t)a2 != F16(v3, 72) )
      {
        F16(v3, 72) = a2;
        F16(v3, 86) = 900;
        F16(v3, 88) = 0;
        F32(v3, 92) = 0;
        F32(v3, 120) = 0;
        F16(v3, 84) = 0;
        F32(v3, 76) = 0;
        F32(v3, 80) = 0;
        F16(v3, 90) = 0;
        F16(v3, 100) = 0;
      }
    }
  }
  return result;
}

/* 0x439510: int __thiscall sub_439510(cls_401D40 *this, int a2) */
int32_t sc_439510(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2;
    intptr_t v3;
    intptr_t i;
    intptr_t v5;
    intptr_t result;

  if ( F32(c, 0x0) != 0 )
  {
    v3 = 9568;
    for ( i = 128; i != 0; --i )
    {
      v5 = *(_DWORD *)&GAP(W, 0x0, v3);
      result = F32(v5, 464);
      if ( result != 0 && a2 == F32(v5, 472) )
        W_entity_release(v5, /*edx*/ 0);
      v3 += 4;
    }
  }
  return result;
}

/* 0x439560: int __thiscall sub_439560(cls_401D40 *this, int a2) */
int32_t sc_439560(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t i;
    intptr_t v9;
    int v10;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  result = F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    (*v11__f) = 4576;
    (*v12__f) = 128;
    do
    {
      v3 = *(_DWORD *)&GAP(W, 0x0, (*v11__f));
      if ( F32(v3, 464) != 0 && a2 == F32(v3, 472) )
      {
        v4 = F32(v3, 504);
        if ( v4 != 0 )
        {
          W_actor_release(v4);
          F32(v3, 504) = 0;
        }
        v5 = *(_DWORD *)&GAP(W, 0x2788, 16);
        v6 = 0;
        SETLO(v4, F16(v5, 8));
        v7 = v4;
        if ( (uint16_t)v4 != 0 )
        {
          while ( F32((F32(v5, 4) + 4 * (uint16_t)v6), 0) != v3 )
          {
            v6 = ((uint8_t *)v6 + 1);
            if ( (uint16_t)v6 >= (uint16_t)v4 )
              goto LABEL_12;
          }
          v7 = v6;
        }
LABEL_12:
        if ( (uint16_t)v7 < (uint16_t)v4 )
        {
          for ( i = (uint16_t)v7; (uint16_t)v7 < F16(v5, 8) - 1; i = (uint16_t)v7 )
          {
            v7 = ((uint8_t *)v7 + 1);
            F32((F32(v5, 4) + 4 * i), 0) = F32(F32((v5 + 4), 0) + 4 * i, 4);
          }
          --F16(v5, 8);
        }
        W_entity_unlist(v3 /*edx*/);
        W_entity_release(v3, /*edx*/ 1);
        if ( F32(v3, 680) != 0 )
        {
          g_eng.op_delete((void *)F32(v3, 680));
          F32(v3, 680) = 0;
        }
      }
      result = (*v12__f) - 1;
      v10 = (*v12__f) == 1;
      (*v11__f) += 4;
      --(*v12__f);
    }
    while ( !v10 );
  }
  return result;
}

/* 0x49cf60: int __stdcall sub_49CF60(int a1) */
int32_t sc_49cf60(int32_t a1)
{

    intptr_t result;

  result = a1;
  if ( a1 != 0 )
    return F32(a1, 12);
  return result;
}

/* 0x49cf80: void __fastcall sub_49CF80(int a1, int a2, cls_4022A0 *a3, int a4) */
void sc_49cf80(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v4;
    uint16_t v5;
    int v6;
    uint32_t v7;
    intptr_t v8;
    intptr_t v9;
    uint32_t v10;
    uint32_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    uint32_t v15;
    intptr_t v16;

  if ( a3 == 0 )
    return;
  v4 = F32(a3, 0x40);
  v5 = 0;
  v6 = F32(a3, 0x268) == 0;
  F32(a3, 0xc) = a4;
  if ( !v6 )
  {
    do
    {
      a2 = *(&F32(a3, 0x208) + 3 * v5);
      if ( a2 == a4 )
      {
        a2 = *(&F32(a3, 0x20c) + 3 * v5);
        if ( a2 != 0 )
        {
          if ( F32(v4, 108) != a2 )
          {
            F32(v4, 108) = a2;
            F8(v4, 116) = 0;
            F16(v4, 114) = 0;
            F16(v4, 112) = 0;
          }
          a2 = *(&F32(a3, 0x20c) + 3 * v5);
          F16(v4, 114) = F16(a2, 66) - 1;
        }
      }
      ++v5;
    }
    while ( v5 < F32(a3, 0x268) );
  }
  v7 = F32(a3, 0x1d4);
  if ( v7 <= 0x112E3 )
  {
    if ( v7 == 70371 )
    {
LABEL_26:
      W_switch_glow(a3, F32(a3, 0xc), 0);
    }
    else
    {
      switch ( v7 )
      {
        case 0x111D4u:
          F32(a3, 0x284) = F32(a3, 0xc) == 1;
          break;
        case 0x111E8u:
          v9 = F32(v4, 16);
          if ( F32(a3, 0xc) == 1 )
          {
            F32(v4, 8) += v9 - F32(a3, 0x270);
            F32(v4, 16) = F32(a3, 0x270);
            F32(*(_DWORD *)&GAP(W, 0x1030, 24), 40) = 1;
          }
          else
          {
            F32(v4, 8) += v9 - *(_DWORD *)&F32(a3, 0x26c);
            F32(v4, 16) = *(_DWORD *)&F32(a3, 0x26c);
          }
          break;
        case 0x11256u:
          v8 = F32(a3, 0xc);
          if ( v8 != 0 )
          {
            if ( v8 == 1 )
              F32(v4, 16) = 3200;
          }
          else
          {
            F32(v4, 16) = 12800;
          }
          F32(v4, 68) = (F32(v4, 16) - F32(a3, 0x2c)) / 100;
          break;
        case 0x1126Bu:
        case 0x1126Cu:
          goto LABEL_26;
        case 0x11289u:
        case 0x11292u:
          if ( F32(a3, 0xc) == 1 )
          {
            F32(a3, 0x284) = 1;
            W_entity_set_collide(a3, 0, 0);
          }
          else
          {
            F32(a3, 0x284) = 0;
            W_entity_set_collide(a3, 3, 2);
          }
          break;
        default:
          return;
      }
    }
    return;
  }
  if ( v7 > 0x113DC )
  {
    v15 = v7 - 70810;
    if ( v15 != 0 )
    {
      if ( v15 == 10 )
      {
        if ( a4 != 0 )
        {
          if ( a4 == 1 )
            W_entity_set_collide(a3, 0, 0);
        }
        else
        {
          W_entity_set_collide(a3, 3, 3);
        }
      }
    }
    else
    {
      v16 = F32(a3, 0xc);
      if ( v16 == 0 )
        goto LABEL_51;
      if ( v16 == 1 )
      {
        F32(a3, 0x284) = 1;
        F32(v4, 92) = 0;
      }
    }
  }
  else
  {
    if ( v7 == 70620 )
      goto LABEL_39;
    v10 = v7 - 70400;
    if ( v10 != 0 )
    {
      v11 = v10 - 40;
      if ( v11 != 0 )
      {
        if ( v11 != 170 )
          return;
LABEL_39:
        v14 = F32(a3, 0xc);
        if ( v14 != 0 )
        {
          if ( v14 == 1 )
          {
            F32(a3, 0x284) = 1;
            F32(v4, 92) = 0;
          }
          return;
        }
LABEL_51:
        F32(a3, 0x284) = 0;
        F32(v4, 92) = 1000;
        return;
      }
      v12 = F32(a3, 0xc);
      if ( v12 != 0 )
      {
        if ( v12 == 1 )
          F32(v4, 8) = F32(a3, 0x2bc) + 19200;
      }
      else
      {
        F32(v4, 8) = F32(a3, 0x2bc);
      }
    }
    else if ( F32(a3, 0xc) == 1 )
    {
      F32(a3, 0x284) = 1;
      W_wl_list_remove(*(_DWORD *)&GAP(W, 0x2788, 16), /*edx*/ (int)a3);
      W_entity_unlist(a3 /*edx*/);
      F32(a3, 0x290) = 0;
    }
  }
}

/* 0x49e250: int __thiscall sub_49E250(cls_401D40 *this) */
int32_t sc_49e250(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2;
    intptr_t result;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v11 = (intptr_t *)(frame_ + 0x0);   /* [5] at ebp-0x14 */

  v11[0] = 901;
  v11[1] = 902;
  v11[2] = 903;
  v11[3] = 904;
  v11[4] = 905;
  v2 = 0;
  result = W_script_flag(c, 100100102);
  if ( result != 50 && result != 60 )
  {
    if ( W_script_flag(c, 100100213) == 20 && 1000 * rnd() / 0x8000 < 600 )
    {
      v4 = rnd();
      W_scene_spawn_at_door(c, 210140913, 50150, 901, 0, 0, 1000 * v4 / 0x8000 >= 500 ? 3 : 1, 0, 0);
      v2 = 1;
    }
    if ( W_script_flag(c, 100100211) == 20 && 1000 * rnd() / 0x8000 < 600 )
    {
      v5 = rnd();
      W_scene_spawn_at_door(c, 210140911, 50300, v11[v2++], 0, 0, 1000 * v5 / 0x8000 >= 500 ? 3 : 1, 0, 0);
    }
    if ( W_script_flag(c, 100100209) == 20 && 1000 * rnd() / 0x8000 < 600 && v2 < 5u )
    {
      v6 = rnd();
      W_scene_spawn_at_door(c, 210140909, 50164, v11[v2++], 0, 0, 1000 * v6 / 0x8000 >= 500 ? 3 : 1, 0, 0);
    }
    if ( W_script_flag(c, 100100206) == 20 && 1000 * rnd() / 0x8000 < 600 && v2 < 5u )
    {
      v7 = rnd();
      W_scene_spawn_at_door(c, 210140906, 50243, v11[v2++], 0, 0, 1000 * v7 / 0x8000 >= 500 ? 3 : 1, 0, 177);
    }
    if ( W_script_flag(c, 100100204) == 20 && 1000 * rnd() / 0x8000 < 600 && v2 < 5u )
    {
      v8 = rnd();
      W_scene_spawn_at_door(c, 210140904, 50260, v11[v2++], 0, 0, 1000 * v8 / 0x8000 >= 500 ? 3 : 1, 0, 0);
    }
    if ( W_script_flag(c, 100100203) == 20 && 1000 * rnd() / 0x8000 < 600 && v2 < 5u )
    {
      v9 = rnd();
      W_scene_spawn_at_door(c, 210140903, 50210, v11[v2++], 0, 0, 1000 * v9 / 0x8000 >= 500 ? 3 : 1, 1, 213);
    }
    result = W_script_flag(c, 100100202);
    if ( result == 20 )
    {
      result = 1000 * rnd() / 0x8000;
      if ( result < 600 && v2 < 5u )
      {
        v10 = rnd();
        return W_scene_spawn_at_door(c, 210140902, 50110, v11[v2], 0, 0, 1000 * v10 / 0x8000 >= 500 ? 3 : 1, 1, 213);
      }
    }
  }
  return result;
}

/* 0x49e6c0: int __thiscall sub_49E6C0(cls_401D40 *this) */
int32_t sc_49e6c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1;
    intptr_t v2;
    int32_t * v3;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t result;
    intptr_t v9;
    uint16_t v10;
    int32_t * v11;
    intptr_t v12;
    intptr_t v13;
    int16_t v14;
    intptr_t i;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    uint16_t v23;
    intptr_t v24;
    int32_t * v25;
    intptr_t v26;
    char * v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    intptr_t v31;
    intptr_t v32;
    intptr_t v33;
    intptr_t v34;
    intptr_t v35;
    intptr_t v36;
    intptr_t v37;
    intptr_t v38;
    intptr_t v39;
    intptr_t v40;
    _Alignas(8) uint8_t frame_[0x238 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x38);   /* [512] at ebp-0x200 */
    char * *const v41__f = (char * *)(frame_ + 0x0);   /* ebp-0x238 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x208 */
    int32_t * *const v43__f = (int32_t * *)(frame_ + 0x34);   /* ebp-0x204 */

  v1 = 0;
  v2 = 0;
  v3 = (int32_t *)D(0x6e7a38);
  (*v43__f) = (int32_t *)D(0x6e7a38);
  while ( 1 )
  {
    v5 = W_script_flag(c, v3[2]);
    if ( (v5 == 0 || v5 == 10) && W_script_flag(c, 100100101) >= v3[3] )
      break;
    (*v43__f) = (int *)(20 * (uint16_t)++v2 + 7240248);
    if ( ((int32_t *)D(0x6e7a38))[5 * (uint16_t)v2] == 0 )
      goto LABEL_7;
    v3 = (int *)(20 * (uint16_t)v2 + 7240248);
  }
  v9 = 110010;
  v10 = 0;
  (*v42__f) = v2;
  while ( *v3 != v9 )
  {
    v9 = ((int32_t *)D(0x6c0cd0))[184 * ++v10];
    if ( v9 == 0 )
    {
      v11 = 0;
      goto LABEL_15;
    }
  }
  v11 = &((int32_t *)D(0x6c0cd0))[184 * v10];
LABEL_15:
  if ( v3 == 0 || v11 == 0 )
  {
LABEL_7:
    v6 = W_actor_by_handle(100000101);
    W_scene_msg(c, 0, v6, D(0x855ad4), 0, 1, 1, 0, 2, 0, 0);
    v1 = W_script_wait(c);
    if ( v1 == 6 )
      return v1;
    v7 = W_actor_by_handle(100000211);
    W_scene_msg(c, 0, v7, D(0x855a88), 0, 1, 1, 0, 30, 0, 0);
    v1 = W_script_wait(c);
    if ( v1 == 6 )
      return v1;
    goto LABEL_9;
  }
  v12 = W_script_flag(c, v3[2]);
  if ( v12 == 0 )
  {
    v37 = W_actor_by_handle(100000101);
    W_scene_msg(c, 0, v37, D(0x855ad4), 0, 1, 1, 0, 2, 0, 0);
    v1 = W_script_wait(c);
    if ( v1 != 6 )
    {
      g_eng.h5bf3ee(
        Buffer,
        "Hmm... well, would you be willing to find a %s for me?",
        (uint8_t *)v11 + 4);
      v38 = W_actor_by_handle(100000211);
      W_scene_msg(c, 0, v38, Buffer, 0, 1, 1, 0, 30, 0, 0);
      v1 = W_script_wait(c);
      if ( v1 != 6 )
      {
        v39 = W_actor_by_handle(100000211);
        W_scene_msg(c, 0, v39, D(0x855750), 0, 1, 1, 0, 30, 0, 0);
        v1 = W_script_wait(c);
        if ( v1 != 6 )
        {
          v40 = W_actor_by_handle(100000101);
          W_scene_msg(c, 0, v40, D(0x855748), 0, 1, 1, 0, 3, 0, 0);
          v1 = W_script_wait(c);
          if ( v1 != 6 )
          {
            W_script_flag_set(c, v3[2], 10, 0);
            W_script_end(c);
            return v1;
          }
        }
      }
    }
    return v1;
  }
  if ( v12 != 10 )
  {
LABEL_9:
    W_script_end(c);
    return v1;
  }
  v13 = F32(c, 0x8);
  v14 = 0;
  SETLO(i, F16(v13, 16400));
  if ( (uint16_t)i != 0 )
  {
    v16 = v13 + 8208;
    for ( i = (uint16_t)i; i != 0; --i )
    {
      if ( F8(v16, 10) == 0 && F32(v16, 0) == *v3 )
        ++v14;
      v16 += 16;
    }
    if ( v14 != 0 )
    {
      g_eng.h5bf3ee(Buffer, "Ah, it looks like you found a %s, Arche.", (uint8_t *)v11 + 4);
      v17 = W_actor_by_handle(100000211);
      W_scene_msg(c, 0, v17, Buffer, 0, 1, 1, 0, 30, 0, 0);
      v1 = W_script_wait(c);
      if ( v1 == 6 )
        return v1;
      v18 = W_actor_by_handle(100000211);
      W_scene_msg(c, 0, v18, D(0x855a18), 0, 1, 1, 0, 30, 0, 0);
      v1 = W_script_wait(c);
      if ( v1 == 6 )
        return v1;
      v19 = W_scene_yes_no(c);
      v1 = 6;
      if ( v19 == 6 )
        return v1;
      *(_DWORD *)&GAP(c, 0x300, 636) = 20;
      F32(c, 0x20) = 6;
      v1 = W_script_wait(c);
      if ( v1 == 6 )
        return v1;
      if ( v19 != 14 )
      {
        (*v41__f) = D(0x855930);
        goto LABEL_64;
      }
      result = sc_406500(c, *v3, 0);
      if ( result != 6 )
      {
        v20 = W_actor_by_handle(100000211);
        W_scene_msg(c, 0, v20, D(0x855a08), 0, 1, 1, 0, 30, 0, 0);
        v1 = W_script_wait(c);
        if ( v1 != 6 )
        {
          v21 = v3[1];
          if ( v21 == 0 )
            goto LABEL_39;
          v22 = 110010;
          v23 = 0;
          while ( v21 != v22 )
          {
            v22 = ((int32_t *)D(0x6c0cd0))[184 * ++v23];
            if ( v22 == 0 )
              goto LABEL_39;
          }
          v25 = &((int32_t *)D(0x6c0cd0))[184 * v23];
          if ( v25 != 0 )
          {
            g_eng.h5bf3ee(Buffer, "All right then! We now have %s in stock!", (uint8_t *)v25 + 4);
            v26 = W_actor_by_handle(100000211);
            W_scene_msg(c, 0, v26, Buffer, 0, 1, 1, 0, 30, 0, 0);
            v1 = W_script_wait(c);
            if ( v1 != 6 )
              goto LABEL_40;
          }
          else
          {
LABEL_39:
            v24 = W_actor_by_handle(100000211);
            W_scene_msg(c, 0, v24, D(0x8559bc), 0, 1, 1, 0, 30, 0, 0);
            v1 = W_script_wait(c);
            if ( v1 != 6 )
            {
LABEL_40:
              W_script_flag_set(c, v3[2], 20, 0);
              W_script_end(c);
              return v1;
            }
          }
        }
        return v1;
      }
      return result;
    }
  }
  if ( v3[4] == 0 || v3[4] > W_script_flag(c, 100100101) )
  {
    g_eng.h5bf3ee(Buffer, "Looks like you haven't found %s yet, Arche.", (uint8_t *)v11 + 4);
    v35 = W_actor_by_handle(100000211);
    W_scene_msg(c, 0, v35, Buffer, 0, 1, 1, 0, 30, 0, 0);
    v1 = W_script_wait(c);
    if ( v1 == 6 )
      return v1;
    (*v41__f) = D(0x8557bc);
    goto LABEL_64;
  }
  v27 = (const char *)(v11 + 1);
  g_eng.h5bf3ee(
    Buffer,
    "Looks like you still haven't found %s yet, Arche. Do you think you'll be able to find it?",
    v27);
  v28 = W_actor_by_handle(100000211);
  W_scene_msg(c, 0, v28, Buffer, 0, 1, 1, 0, 30, 0, 0);
  v1 = W_script_wait(c);
  if ( v1 == 6 )
    return v1;
  v29 = W_scene_yes_no(c);
  v1 = 6;
  if ( v29 == 6 )
    return v1;
  *(_DWORD *)&GAP(c, 0x300, 636) = 20;
  F32(c, 0x20) = 6;
  v1 = W_script_wait(c);
  if ( v1 == 6 )
    return v1;
  if ( v29 != 14 )
  {
    g_eng.h5bf3ee(Buffer, "Hmm, well then. Are you going to give up on finding %s?", v27);
    v30 = W_actor_by_handle(100000211);
    W_scene_msg(c, 0, v30, Buffer, 0, 1, 1, 0, 30, 0, 0);
    v1 = W_script_wait(c);
    if ( v1 == 6 )
      return v1;
    result = W_scene_yes_no(c);
    v31 = result;
    if ( result == 6 )
      return result;
    *(_DWORD *)&GAP(c, 0x300, 636) = 20;
    F32(c, 0x20) = 6;
    v1 = W_script_wait(c);
    if ( v1 == 6 )
      return v1;
    if ( v31 == 14 )
    {
      v32 = W_actor_by_handle(100000211);
      W_scene_msg(c, 0, v32, D(0x855834), 0, 1, 1, 0, 30, 0, 0);
      v1 = W_script_wait(c);
      if ( v1 == 6 )
        return v1;
      W_script_flag_set(c, (*v43__f)[2], 15, 0);
      if ( (*v43__f)[1] == 0 )
      {
        v33 = (*v42__f) + 1;
        v34 = 20 * (uint16_t)((*v42__f) + 1) + 7240248;
        if ( ((int32_t *)D(0x6e7a38))[5 * (uint16_t)((*v42__f) + 1)] != 0 )
        {
          while ( 1 )
          {
            W_script_flag_set(c, F32(v34, 8), 15, 0);
            if ( F32(v34, 4) != 0 )
              break;
            v34 = 20 * (uint16_t)++v33 + 7240248;
            if ( ((int32_t *)D(0x6e7a38))[5 * (uint16_t)v33] == 0 )
              goto LABEL_61;
          }
        }
      }
      goto LABEL_9;
    }
  }
  (*v41__f) = D(0x8558b4);
LABEL_64:
  v36 = W_actor_by_handle(100000211);
  W_scene_msg(c, 0, v36, (*v41__f), 0, 1, 1, 0, 30, 0, 0);
  v1 = W_script_wait(c);
  if ( v1 != 6 )
  {
LABEL_61:
    W_script_end(c);
    return v1;
  }
  return v1;
}

/* 0x4a5e40: int __stdcall sub_4A5E40(int a1, int a2, int a3) */
int32_t sc_4a5e40(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t v3;

  v3 = W_actor_by_handle(a1);
  return sc_401d80(v3, a2, a3);
}

/* 0x4a5e70: int __thiscall sub_4A5E70(cls_401D40 *this, int a2, int a3, int a4, int a5) */
int32_t sc_4a5e70(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;

    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */

  (*v7__f) = W_actor_by_handle(a3);
  return W_scene_talk_face(c, a2, (*v7__f), a4, a5);
}

/* 0x4a5ea0: BOOL __thiscall sub_4A5EA0(cls_401D40 *this) */
int32_t sc_4a5ea0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1;

  v1 = W_script_flag(c, 100100102);
  return v1 == 50 || v1 == 60;
}

/* 0x4a5ec0: BOOL __thiscall sub_4A5EC0(cls_401D40 *this) */
int32_t sc_4a5ec0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return W_script_flag(c, 100100102) == 40;
}

/* 0x4a5f80: int __thiscall sub_4A5F80(cls_401D40 *this, int a2) */
int32_t sc_4a5f80(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x20) = a2;
  return a2;
}

/* 0x4a63d0: unsigned int *__thiscall sub_4A63D0(cls_4A63D0 *this, unsigned int a2) */
int32_t sc_4a63d0(wl_char *self, uint32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint32_t * result;
    uint32_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    uint32_t * v8;
    uint32_t LastError;
    intptr_t * v10;
    uint32_t v11;
    char * v12;
    char * v13;
    char * v14;
    char v15;
    uint32_t v16;
    intptr_t * v17;
    _Alignas(8) uint8_t frame_[0x210 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x10);   /* [256] at ebp-0x200 */
    char *Buffer = (char *)(frame_ + 0x110);   /* [256] at ebp-0x100 */
    intptr_t *const SystemTime__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x210 */

  result = 0;
  v3 = F32(c, 0x7804);
  v4 = 0;
  if ( v3 == 0 )
    goto LABEL_7;
  v5 = 0;
  do
  {
    v6 = 3 * v5;
    v7 = *(_DWORD *)&GAP(c, 0x0, 4 * v6 + 6148);
    v8 = (unsigned int *)&GAP(c, 0x0, 4 * v6 + 6148);
    if ( v7 == 0 )
      result = v8;
    v5 = (uint16_t)++v4;
  }
  while ( (uint16_t)v4 < v3 );
  if ( result == 0 )
  {
LABEL_7:
    if ( v3 == 2048 )
    {
      g_eng.h426110(g_eng.log, (intptr_t)D(0x859388), 1, (intptr_t)D(0x8a9b6c));   /* the error box 0x426110, inlined */
      g_eng.h5bf5db(0);
    }
    F32(c, 0x7804) = v3 + 1;
    result = (unsigned int *)&GAP(c, 0x0, 12 * v3 + 6148);
  }
  result[1] = 0;
  result[2] = 0;
  *result = a2;
  return result;
}

/* 0x4ac610: int __thiscall sub_4AC610(cls_4AC610 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t sc_4ac610(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x588) = a2;
  F32(c, 0x590) = a3;
  F32(c, 0x58c) = a4;
  F32(c, 0x594) = a5;
  F16(c, 0x584) = 0;
  F32(c, 0x598) = a6;
  F32(c, 0x59c) = a7;
  F32(c, 0x20) = 5;
  return a6;
}

/* 0x4ac660: unsigned int __thiscall sub_4AC660(cls_4AC660 *this, int a2, const char *a3) */
int32_t sc_4ac660(wl_char *self, int32_t a2, char * a3)
{
    intptr_t c = (intptr_t)self;
    char * v4;
    char * v5;
    uint32_t result;

  v4 = &GAP(c, 0x0, 88 * F16(c, 0x584));
  F32(v4, 4 * 381) = a2;
  F32(v4, 4 * 380) = 0;
  v5 = v4 + 1440;
  result = strlen(a3) + 1;
  qmemcpy(v5, a3, result);
  ++F16(c, 0x584);
  return result;
}

/* 0x4b3b10: int sub_4B3B10() */
int32_t sc_4b3b10(void)
{


  return 1;
}

/* 0x4b3f30: int __thiscall sub_4B3F30(cls_401D40 *this) */
int32_t sc_4b3f30(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2;
    intptr_t v4;
    intptr_t v5;
    _Alignas(8) uint8_t frame_[0x22c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v8 = (char *)(frame_ + 0x4);   /* [40] at ebp-0x228 */
    char *Buffer = (char *)(frame_ + 0x2c);   /* [512] at ebp-0x200 */
    uint16_t *const v6__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x22c */
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x22c */

  Buffer[0] = F8(D(0x8a9b6c), 0);
  memset(&Buffer[1], 0, 511);
  (*v6__f) = sc_402f60(c, D(0x6c0838), v8);
  v2 = W_scene_yes_no(c);
  if ( v2 == 6 )
    return 6;
  *(_DWORD *)&GAP(c, 0x300, 636) = 20;
  F32(c, 0x20) = 6;
  v4 = W_script_wait(c);
  if ( v4 == 6 )
    return v4;
  if ( v2 != 14 )
  {
    W_scene_msg(c, 0, F32(c, 0x4), D(0x85df90), 0, 1, 1, 0, 0, 0, 0);
    v4 = W_script_wait(c);
    if ( v4 != 6 )
      W_script_end(c);
    return v4;
  }
  W_scene_msg(c, 0, F32(c, 0x4), D(0x85e334), 0, 1, 1, 0, 0, 0, 0);
  v4 = W_script_wait(c);
  if ( v4 != 6 )
  {
    F32(c, 0x2e4) = 1;
    *(_DWORD *)&GAP(c, 0x300, 636) = 50;
    F32(c, 0x20) = 6;
    v4 = W_script_wait(c);
    if ( v4 != 6 )
    {
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e328), 0, 1, 1, 0, 0, 0, 0);
      v4 = W_script_wait(c);
      if ( v4 != 6 )
      {
        F32(c, 0x2e4) = 1;
        *(_DWORD *)&GAP(c, 0x300, 636) = 50;
        F32(c, 0x20) = 6;
        v4 = W_script_wait(c);
        if ( v4 != 6 )
        {
          W_scene_msg(c, 0, F32(c, 0x4), D(0x85e2f0), 0, 1, 1, 0, 0, 0, 0);
          v4 = W_script_wait(c);
          if ( v4 != 6 )
          {
            F32(c, 0x2e4) = 1;
            *(_DWORD *)&GAP(c, 0x300, 636) = 50;
            F32(c, 0x20) = 6;
            v4 = W_script_wait(c);
            if ( v4 != 6 )
            {
              W_scene_msg(c, 0, F32(c, 0x4), D(0x85e2dc), 0, 1, 1, 0, 0, 0, 0);
              v4 = W_script_wait(c);
              if ( v4 != 6 )
              {
                if ( (*v6__f) != 0 )
                {
                  g_eng.h5bf3ee(
                    Buffer,
                    "Hmm... the Fates tell me you've missed %d Marks of Heroism...",
                    (*v6__f));
                  W_scene_msg(c, 0, F32(c, 0x4), Buffer, 0, 1, 1, 0, 0, 0, 0);
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                  W_scene_msg(c, 0, F32(c, 0x4), D(0x85e1e8), 0, 1, 1, 0, 0, 0, 0);
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                  v5 = 100 * (F32(F32(c, 0x8), 16412) + 1);
                  g_eng.h5bf3ee(
                    Buffer,
                    "Okay, then! I'll tell you where the Marks are for %d copper!",
                    v5);
                  W_scene_msg(c, 0, F32(c, 0x4), Buffer, 0, 1, 1, 0, 0, 0, 0);
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                  g_eng.h5bf3ee(
                    Buffer,
                    "You currently have %d copper.%%nWould you like to spend %d to learn where the Marks are?",
                    F32(F32(c, 0x8), 16404),
                    v5);
                  W_scene_msg(c, 3u, 0, Buffer, 0, 1, 0, 0, 0, 0, 0);
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                  (*v7__f) = W_scene_yes_no(c);
                  if ( (*v7__f) == 6 )
                    return 6;
                  *(_DWORD *)&GAP(c, 0x300, 636) = 20;
                  F32(c, 0x20) = 6;
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                  if ( (*v7__f) != 14 )
                  {
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85dfe0), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 != 6 )
                      goto LABEL_39;
                    return v4;
                  }
                  if ( v5 <= F32(F32(c, 0x8), 16404) )
                  {
                    if ( sc_406500(c, 1, v5) == 6 )
                      return 6;
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e10c), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    F32(c, 0x2e4) = 1;
                    *(_DWORD *)&GAP(c, 0x300, 636) = 50;
                    F32(c, 0x20) = 6;
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e328), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    F32(c, 0x2e4) = 1;
                    *(_DWORD *)&GAP(c, 0x300, 636) = 50;
                    F32(c, 0x20) = 6;
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e0d4), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    F32(c, 0x2e4) = 1;
                    *(_DWORD *)&GAP(c, 0x300, 636) = 50;
                    F32(c, 0x20) = 6;
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e2dc), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    g_eng.h5bf3ee(Buffer, "You missed the mark in the %s.", v8);
                    W_scene_msg(c, 0, F32(c, 0x4), Buffer, 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e088), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                  }
                  else
                  {
                    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e118), 0, 1, 1, 0, 0, 0, 0);
                    v4 = W_script_wait(c);
                    if ( v4 == 6 )
                      return v4;
                  }
                }
                else
                {
                  W_scene_msg(c, 0, F32(c, 0x4), D(0x85e2ac), 0, 1, 1, 0, 0, 0, 0);
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                  W_scene_msg(c, 0, F32(c, 0x4), D(0x85e284), 0, 1, 1, 0, 0, 0, 0);
                  v4 = W_script_wait(c);
                  if ( v4 == 6 )
                    return v4;
                }
                W_scene_msg(c, 0, F32(c, 0x4), D(0x85e038), 0, 1, 1, 0, 0, 0, 0);
                v4 = W_script_wait(c);
                if ( v4 != 6 )
                {
LABEL_39:
                  W_script_end(c);
                  return v4;
                }
              }
            }
          }
        }
      }
    }
  }
  return v4;
}

/* 0x402f60: __int16 __thiscall sub_402F60(cls_401D40 *this, const char *a2, char *a3) */
int32_t sc_402f60(wl_char *self, char * a2, char * a3)
{
    intptr_t c = (intptr_t)self;
    char * v3;
    intptr_t v4;
    uint32_t v5;
    uint32_t v6;
    int32_t * v7;
    intptr_t v8;
    uint32_t v9;
    char * v10;
    char * v11;
    char * v12;
    char v13;
    uint16_t v14;
    int32_t * v15;
    intptr_t v16;
    intptr_t v17;
    uint32_t v18;
    char * v20;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *const v21__f = (int16_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  (*v23__f) = 0;
  (*v21__f) = 0;
  strcpy(a3, D(0x8a9b6c));
  v3 = a2;
  if ( F32(a2, 0) != 0 )
  {
    while ( 1 )
    {
      if ( F32(v3, 4 * 13) > W_script_flag(c, F32(v3, 4 * 12)) )
        goto LABEL_21;
      v4 = F32(F32(c, 0x8), 16556);
      v5 = 0;
      v6 = F32(v4, 30724);
      if ( v6 != 0 )
      {
        v7 = (_DWORD *)(v4 + 6148);
        while ( *v7 != F32(v3, 0) )
        {
          ++v5;
          v7 += 3;
          if ( v5 >= v6 )
            goto LABEL_10;
        }
        v8 = v4 + 12 * v5 + 6148;
        if ( v8 != 0 && F32(v8, 4) != 0 )
          goto LABEL_21;
      }
LABEL_10:
      if ( (*v21__f) == 0 )
        break;
LABEL_20:
      ++(*v21__f);
LABEL_21:
      v3 = &a2[56 * (uint16_t)++(*v23__f)];
      if ( F32(v3, 0) == 0 )
        return (*v21__f);
    }
    if ( strlen(v3 + 8) != 0 )
    {
      v9 = strlen(v3 + 8) + 1;
      v10 = &v3[v9 + 8];
    }
    else
    {
      v14 = 0;
      v15 = **(int ***)&GAP(W, 0x1030, 12);
      v16 = *v15;
      if ( *v15 == 0 )
        goto LABEL_18;
      v17 = F32(v3, 4 * 1);
      while ( v16 != v17 )
      {
        v16 = v15[16 * ++v14];
        if ( v16 == 0 )
          goto LABEL_18;
      }
      v20 = (const char *)&v15[16 * v14];
      if ( v20 == 0 )
      {
LABEL_18:
        v18 = strlen(D(0x8a9b6c)) + 1;
        qmemcpy(a3, D(0x8a9b6c), 4 * (v18 >> 2));
        v12 = D(0x8a9b6c) + 4 * (v18 >> 2);
        v11 = &a3[4 * (v18 >> 2)];
        v13 = v18;
        goto LABEL_19;
      }
      v9 = strlen(v20 + 4) + 1;
      v10 = &v20[v9 + 4];
    }
    qmemcpy(a3, &v10[-v9], 4 * (v9 >> 2));
    v12 = (CHAR *)&v10[4 * (v9 >> 2) - v9];
    v11 = &a3[4 * (v9 >> 2)];
    v13 = v9;
LABEL_19:
    qmemcpy(v11, v12, v13 & 3);
    goto LABEL_20;
  }
  return 0;
}

/* 0x4b4570: int __thiscall sub_4B4570(cls_401D40 *this) */
int32_t sc_4b4570(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    char * v7;
    char * v8;
    char * v9;
    char * v10;
    char * v11;
    intptr_t v12;
    int v13;
    _Alignas(8) uint8_t frame_[0x204 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x4);   /* [512] at ebp-0x200 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x204 */

  W_script_begin(c);
  v2 = W_script_leader(c);
  F32(c, 0x4c) = 0;
  F32(c, 0x5c) = v2;
  F32(c, 0x50) = 0;
  F32(c, 0x54) = 0;
  F32(c, 0x58) = 800;
  F32(c, 0x20) = 3;
  W_script_party_cast(c);
  v3 = W_script_wait(c);
  if ( v3 == 6 )
    return v3;
  if ( W_scene_party_stance(c, 0, F32(c, 0x8)) != 0 )
  {
    v3 = W_script_wait(c);
    if ( v3 == 6 )
      return v3;
  }
  v4 = W_script_leader(c);
  F32(c, 0x4c) = 0;
  F32(c, 0x5c) = v4;
  F32(c, 0x50) = 0;
  F32(c, 0x54) = 0;
  F32(c, 0x58) = 800;
  F32(c, 0x20) = 3;
  v3 = W_script_wait(c);
  if ( v3 == 6 )
    return v3;
  W_script_party_cast(c);
  v3 = W_script_wait(c);
  if ( v3 == 6 )
    return v3;
  v5 = W_actor_by_handle(300290101);
  sc_4020a0(c, v5, 0, 4800, 0, 3, 150, 0, 4000);
  v6 = W_actor_by_handle(300290101);
  W_scene_face(c, v6, 1);
  v3 = W_script_wait(c);
  if ( v3 == 6 )
    return v3;
  W_scene_msg(c, 0, F32(c, 0x4), D(0x85e860), 0, 1, 1, 0, 0, 0, 0);
  v3 = W_script_wait(c);
  if ( v3 == 6 )
    return v3;
  while ( 1 )
  {
    g_eng.h5bf3ee(
      Buffer,
      "What an odd nun... still, would you like to donate to her? You have %d copper (with a %d copper max) right now.",
      F32(F32(c, 0x8), 16404),
      F32(F32(c, 0x8), 16416));
    W_scene_msg(c, 3u, 0, Buffer, 0, 1, 0, 0, 0, 0, 0);
    v3 = W_script_wait(c);
    if ( v3 == 6 )
      return v3;
    F16(c, 0x584) = 0;
    F32(c, 0x598) = 240;
    F32(c, 0x59c) = 240;
    *(_DWORD *)&F16(c, 0x588) = 0;
    F32(c, 0x590) = 0;
    F32(c, 0x58c) = 0;
    F32(c, 0x594) = 200;
    F32(c, 0x20) = 5;
    F32(c, 0x5f4) = 0;
    *(_DWORD *)&GAP(c, 0x5e6, 10) = 0;
    strcpy((char *)&F32(c, 0x5a0), D(0x85e7e8));
    v7 = (uint8_t *)c + 88 * (uint16_t)++F16(c, 0x584);
    F32(v7, 4 * 381) = 100;
    F32(v7, 4 * 380) = 0;
    strcpy(v7 + 1440, D(0x85e7d8));
    v8 = (uint8_t *)c + 88 * (uint16_t)++F16(c, 0x584);
    F32(v8, 4 * 381) = 1000;
    F32(v8, 4 * 380) = 0;
    strcpy(v8 + 1440, D(0x85e7c8));
    v9 = (uint8_t *)c + 88 * (uint16_t)++F16(c, 0x584);
    F32(v9, 4 * 381) = 10000;
    F32(v9, 4 * 380) = 0;
    strcpy(v9 + 1440, D(0x85e7b8));
    v10 = (uint8_t *)c + 88 * (uint16_t)++F16(c, 0x584);
    F32(v10, 4 * 381) = 50000;
    F32(v10, 4 * 380) = 0;
    strcpy(v10 + 1440, D(0x85e7a8));
    v11 = (uint8_t *)c + 88 * (uint16_t)++F16(c, 0x584);
    F32(v11, 4 * 381) = 100000;
    F32(v11, 4 * 380) = 0;
    strcpy(v11 + 1440, D(0x85e798));
    ++F16(c, 0x584);
    v3 = W_script_wait(c);
    if ( v3 == 6 )
      return v3;
    v12 = *(_DWORD *)&F16(c, 0x588);
    v13 = v12 == 0;
    if ( v12 > 0 )
    {
      if ( v12 > F32(F32(c, 0x8), 16404) )
      {
        W_scene_msg(c, 0, F32(c, 0x4), D(0x85e718), 0, 1, 1, 0, 0, 0, 0);
        goto LABEL_27;
      }
      W_scene_msg(c, 3u, 0, D(0x85e6f8), 0, 1, 0, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
      v3 = 6;
      (*v15__f) = W_scene_yes_no(c);
      if ( (*v15__f) == 6 )
        return v3;
      *(_DWORD *)&GAP(c, 0x300, 636) = 20;
      F32(c, 0x20) = 6;
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
      if ( (*v15__f) != 14 )
      {
LABEL_29:
        W_scene_msg(c, 0, F32(c, 0x4), D(0x85e640), 0, 1, 1, 0, 0, 0, 0);
        v3 = W_script_wait(c);
        if ( v3 == 6 )
          return v3;
        goto LABEL_46;
      }
      v13 = v12 == 0;
    }
    if ( v13 )
      goto LABEL_29;
    if ( sc_406500(c, 1, v12) == 6 )
      return 6;
    if ( v12 >= 100000 )
      break;
    if ( v12 >= 50000 )
    {
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e4f4), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e4a8), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e444), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
      goto LABEL_46;
    }
    if ( v12 >= 10000 )
    {
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e414), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e3bc), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 == 6 )
        return v3;
LABEL_46:
      W_script_end(c);
      return v3;
    }
    if ( v12 >= 1000 )
    {
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e3ac), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 != 6 )
      {
        W_scene_msg(c, 0, F32(c, 0x4), D(0x85e344), 0, 1, 1, 0, 0, 0, 0);
        v3 = W_script_wait(c);
        if ( v3 != 6 )
          goto LABEL_49;
      }
      return v3;
    }
    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e6e0), 0, 1, 1, 0, 0, 0, 0);
    v3 = W_script_wait(c);
    if ( v3 == 6 )
      return v3;
    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e680), 0, 1, 1, 0, 0, 0, 0);
    v3 = W_script_wait(c);
    if ( v3 == 6 )
      return v3;
    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e658), 0, 1, 1, 0, 0, 0, 0);
LABEL_27:
    v3 = W_script_wait(c);
    if ( v3 == 6 )
      return v3;
  }
  W_scene_msg(c, 0, F32(c, 0x4), D(0x85e5fc), 0, 1, 1, 0, 0, 0, 0);
  v3 = W_script_wait(c);
  if ( v3 != 6 )
  {
    W_scene_msg(c, 0, F32(c, 0x4), D(0x85e5c4), 0, 1, 1, 0, 0, 0, 0);
    v3 = W_script_wait(c);
    if ( v3 != 6 )
    {
      W_scene_msg(c, 0, F32(c, 0x4), D(0x85e574), 0, 1, 1, 0, 0, 0, 0);
      v3 = W_script_wait(c);
      if ( v3 != 6 )
      {
        if ( W_script_flag(c, 100300107) == 0 )
        {
          W_scene_msg(c, 0, F32(c, 0x4), D(0x85e530), 0, 1, 1, 0, 0, 0, 0);
          v3 = W_script_wait(c);
          if ( v3 == 6 )
            return v3;
          if ( W_item_give(c, 164060, 0, 1, &(*v15__f)) == 6 )
            return 6;
          if ( (*v15__f) != 0 )
          {
            W_script_flag_set(c, 100300107, 1, 0);
            W_script_end(c);
            return v3;
          }
        }
LABEL_49:
        W_script_end(c);
      }
    }
  }
  return v3;
}

/* 0x4bbe00: void __thiscall sub_4BBE00(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6) */
void sc_4bbe00(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */

  (*v8__f) = W_actor_by_handle(a5);
  v7 = W_actor_by_handle(a2);
  sc_402610(c, v7, a3, a4, (*v8__f), a6);
}

/* 0x4bbe40: void __thiscall sub_4BBE40(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) */
void sc_4bbe40(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10;

  v10 = W_actor_by_handle(a2);
  sc_4020a0(c, v10, a3, a4, a5, a6, a7, a8, a9);
}

/* 0x4bbe80: int __stdcall sub_4BBE80(_DWORD *a1, _DWORD *a2) */
int32_t sc_4bbe80(int32_t * a1, int32_t * a2)
{

    int64_t v2;
    uint16_t v3;
    intptr_t v4;
    intptr_t v5;

  v2 = (unsigned int)a1;
  v3 = 0;
  if ( *a1 != 0 )
  {
    do
      ++v3;
    while ( a1[v3] != 0 );
  }
  if ( *a2 != 0 )
  {
    do
    {
      if ( v3 >= 0xFFu )
        break;
      v4 = WORD2(v2);
      v5 = v3++;
      ((v2) += 0x100000000ll);
      F32((v2 + 4 * v5), 0) = a2[v4];
    }
    while ( a2[WORD2(v2)] != 0 );
  }
  a1[v3] = 0;
  return v2;
}

/* 0x4c5300: void __stdcall sub_4C5300(int a1) */
void sc_4c5300(int32_t a1)
{

    intptr_t v1;
    uint16_t v2;
    uint16_t v3;

  v1 = F32(a1, 16);
  if ( v1 != 0 )
  {
    v2 = F16(a1, 22);
    v3 = 0;
    if ( v2 != 0 )
    {
      while ( F32((v1 + 8 * v3), 0) != F32(a1, 8) )
      {
        if ( ++v3 >= v2 )
          return;
      }
      F32(a1, 24) = F32(v1 + 8 * v3, 4);
    }
  }
}

/* 0x4cbfb0: int __stdcall sub_4CBFB0(int a1, int a2) */
int32_t sc_4cbfb0(intptr_t ecx_, int32_t a1, int32_t a2)
{

    intptr_t v2;

  v2 = W_actor_by_handle(a1);
  return sc_41f050(v2, a2);
}

/* 0x4cbfd0: int __stdcall sub_4CBFD0(int a1, int a2) */
int32_t sc_4cbfd0(intptr_t ecx_, int32_t a1, int32_t a2)
{

    intptr_t v2;

  v2 = W_actor_by_handle(a1);
  return sc_41f000(v2, a2);
}

/* 0x41f000: int __stdcall sub_41F000(int a1, int a2) */
int32_t sc_41f000(int32_t a1, int32_t a2)
{

    intptr_t result;

  if ( a1 != 0 )
  {
    result = F32(a1, 64);
    F16(result, 72) = 0;
    F16(result, 86) = 210;
    F16(result, 88) = 0;
    F32(result, 92) = 0;
    F32(result, 120) = 0;
    F16(result, 84) = 0;
    F32(result, 76) = 0;
    F32(result, 80) = 0;
    F16(result, 90) = 0;
    F16(result, 100) = 0;
    if ( a2 != 0 )
    {
      result = F32(a1, 64);
      F32(result, 24) = 0;
      F32(F32((a1 + 64), 0), 40) = 0;
    }
  }
  return result;
}

/* 0x4cbff0: int __thiscall sub_4CBFF0(cls_401D40 *this, int a2, int a3) */
int32_t sc_4cbff0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x6c) = a2;
  F32(c, 0x70) = a3;
  F32(c, 0x20) = 7;
  return a2;
}

/* 0x4cc010: int __thiscall sub_4CC010(cls_401D40 *this, int a2, int a3) */
int32_t sc_4cc010(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x40) = 1;
  F32(c, 0x5c) = 0;
  F32(c, 0x44) = a2;
  F32(c, 0x48) = a3;
  return a2;
}

/* 0x4cc030: int __thiscall sub_4CC030(cls_401D40 *this, int a2, int a3, int a4) */
int32_t sc_4cc030(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x50) = a2;
  F32(c, 0x4c) = 1;
  F32(c, 0x5c) = 0;
  F32(c, 0x54) = a3;
  F32(c, 0x58) = a4;
  F32(c, 0x20) = 3;
  return a4;
}

/* 0x4cc250: int __thiscall sub_4CC250(cls_413810 *this, int a2, int a3) */
int32_t sc_4cc250(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    int32_t * v3;
    intptr_t v4;
    uint16_t v5;
    uint16_t v6;
    intptr_t v7;
    int32_t * v8;
    intptr_t v9;
    intptr_t v11;
    intptr_t v12;
    int32_t * v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;

  v3 = (int *)a3;
  v4 = 0;
  if ( a3 != 0 )
    v5 = F16(a3, 2572) + 1;
  else
    v5 = 0;
  v6 = 0;
  v7 = v5;
  while ( 1 )
  {
    v8 = *(_DWORD **)&GAP(c, 0x4024, 4 * (uint16_t)((v7 + v6) % 8) + 12);
    if ( v8[625] == 1 && v8[636] != 100000104 && *v8 == 0 )
    {
      v9 = v8[637];
      if ( F32(v9, 464) == 1 && F8(F32(v9, 64), 0) == 1 && F16(F32(v9, 1872), 1098) == 0 )
        break;
    }
    if ( ++v6 >= 8u )
      goto LABEL_14;
  }
  v4 = *(intptr_t *)&GAP(c, 0x4024, 4 * (uint16_t)((v7 + v6) % 8) + 12);
LABEL_14:
  if ( a2 != 0 )
  {
    if ( v4 == 0 || a3 == v4 )
      return 0;
    goto LABEL_34;
  }
  if ( v4 == 0 || a3 == v4 )
    return 1;
  if ( a3 != 0 )
  {
    v11 = F32(a3, 2548);
    if ( v11 != 0 )
    {
      v12 = F32(v11, 0x1594c);
      if ( v12 != 0 )
      {
        if ( v12 > 0 )
        {
          if ( v12 > 3 )
            return 0;
          goto LABEL_28;
        }
      }
      else
      {
        v13 = &a3;
        SETLO(v13, F8(F32(v11, 0x40), 32));
        v14 = W_entity_move_vertical(v11, (_DWORD *)F32(v11, 0x40), 1, (int)v13, 0, (int)&a3, 0);
        a3 = v14 == 0;
        if ( v14 == 0 )
          goto LABEL_28;
      }
      return 0;
    }
  }
LABEL_28:
  v15 = F32(v4, 0x9f4);
  v16 = F32(v15, 88396);
  if ( v16 == 0 )
  {
    if ( W_entity_ground_probe(v15, F32(v15, 64), 0) )
      goto LABEL_34;
    return 0;
  }
  if ( v16 <= 0 )
    return 0;
  if ( v16 > 3 )
    return 0;
LABEL_34:
  v17 = F32(c, 0x200c);
  if ( v3 != (int *)v17 )
  {
    v19 = *v3;
    v20 = v3[625] - 1;
    *v3 = 0;
    if ( v20 == 0 && v3[636] != 100000104 )
    {
      v21 = v3[637];
      if ( v21 != 0 )
        sc_4cc470(v21, 1, 0, 0);
    }
    v22 = F32(v4, 0x9c4);
    F32(v4, 0x0) = v19;
    if ( v22 == 1 && F32(v4, 0x9f0) != 100000104 )
    {
      v23 = F32(v4, 0x9f4);
      if ( v23 != 0 )
      {
        if ( v19 != 0 )
          sc_4cc470(v23, 0, v19, 0);
        else
          sc_4cc470(v23, 1, 0, 0);
      }
    }
    W_room_formation(c);
    return 1;
  }
  v18 = 0;
  if ( v17 != 0 )
  {
    v18 = F32(v17, 0x0);
    W_pslot_input_set(v17, 0);
  }
  W_pslot_input_set(v4, v18);
  F32(c, 0x200c) = (int)v4;
  W_room_formation(c);
  return 1;
}

/* 0x4cc470: int __thiscall sub_4CC470(cls_4CC470 *this, int a2, int a3, int a4) */
int32_t sc_4cc470(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t v5;
    int v6;
    int32_t * v7;
    intptr_t v8;

  result = a3;
  F32(c, 0x200) = a2;
  v5 = 0;
  v6 = F16(c, 0x14228) == 0;
  F32(c, 0x158a4) = result;
  F16(c, 0x1429c) = 0;
  F16(c, 0x147a0) = 0;
  F16(c, 0x1422c) = 0;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  F16(c, 0x14278) = 0;
  F16(c, 0x147ac) = 0;
  F16(c, 0x147f4) = 0;
  F16(c, 0x14232) = 0;
  F16(c, 0x14802) = 0;
  F32(c, 0x14270) = 0;
  F32(c, 0x1427c) = 0;
  F16(c, 0x14234) = 0;
  F16(c, 0x14236) = 0;
  F16(c, 0x14804) = 0;
  F16(c, 0x14288) = 0;
  F32(c, 0x1428c) = 0;
  F16(c, 0x14248) = 0;
  F16(c, 0x14290) = 0;
  F32(c, 0x14294) = 0;
  F32(c, 0x1423c) = 0;
  F32(c, 0x14810) = 0;
  F32(c, 0x14814) = 0;
  F16(c, 0x14830) = 0;
  F16(c, 0x1482e) = 0;
  F32(c, 0x14274) = 0;
  F16(c, 0x1428a) = 0;
  if ( !v6 )
  {
    do
    {
      result = (int)&GAP(c, 0x0, 320 * (uint16_t)v5++);
      F16(result, 52032) = F16(result, 52036);
    }
    while ( (uint16_t)v5 < F16(c, 0x14228) );
  }
  if ( a4 == 0 )
  {
    v7 = (_DWORD *)F32(c, 0x40);
    v8 = v7[2];
    F32(c, 0x14238) = 0;
    a3 = v8;
    return W_entity_move_vertical(c, v7, 1, 0, 0, (int)&a3, &F32(c, 0x14238));
  }
  return result;
}

/* 0x4cca20: int __thiscall sub_4CCA20(cls_4CCA20 *this, int a2, int a3) */
int32_t sc_4cca20(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;

  F32(W, 0x2764) = a2;
  F32(W, 0x2768) = a3;
  F16(c, 0x22) = 300;
  return a3;
}

/* 0x4cca50: int __thiscall sub_4CCA50(cls_4CCA50 *this, int a2, __int16 a3) */
int32_t sc_4cca50(wl_char *self, int32_t a2, int16_t a3)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0xd8) = a2;
  F16(c, 0xdc) = a3;
  return a2;
}

/* 0x4d6490: int __stdcall sub_4D6490(int a1, int a2) */
int32_t sc_4d6490(intptr_t ecx_, int32_t a1, int32_t a2)
{

    intptr_t v2;

  v2 = W_actor_by_handle(a1);
  return sc_41efe0(v2, a2);
}

/* 0x5b4330: int __fastcall sub_5B4330(cls_4022A0 *a1, int a2, int a3, int a4) */
int32_t sc_5b4330(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v5;
    int32_t * v6;
    intptr_t v7;
    intptr_t result;
    intptr_t v9;
    int v10;

  v5 = a4;
  F32(a1, 0xc794) = 0;
  F32(a1, 0x750) = (int)&F32(a1, 0x274);
  F32(a1, 0x1dc) = 0;
  W_entity_set_handle((int)a1, /*edx*/ v5);
  F32(a1, 0xc84c) = 1;
  F32(a1, 0x158a4) = 0;
  F32(a1, 0x200) = 1;
  W_char_ai_reset(a1, 1);
  v6 = (_DWORD *)F32(a1, 0x40);
  v7 = v6[2];
  F32(a1, 0x14238) = 0;
  a4 = v7;
  W_entity_move_vertical(a1, v6, 1, 0, 0, (int)&a4, &F32(a1, 0x14238));
  result = F32(a1, 0x40);
  F32(a1, 0x14240) = 0;
  F32(a1, 0x1424c) = 0;
  F32(a1, 0x14250) = 0;
  F32(a1, 0x14254) = 0;
  F32(a1, 0x14244) = F32(result, 44);
  F32(a1, 0x14258) = 0;
  F32(a1, 0x14858) = 0;
  F32(a1, 0x14854) = 0;
  F32(a1, 0x1485c) = 0;
  F32(a1, 0x14860) = 0;
  F32(a1, 0x14868) = 0;
  F32(a1, 0x14864) = 0;
  F32(a1, 0x1486a) = 0;
  F32(a1, 0x1486c) = 0;
  F32(a1, 0x14870) = 0;
  F32(a1, 0x1429c) = 0;
  v9 = 0;
  v10 = F32(a1, 0x14228) == 0;
  F32(a1, 0x147a0) = 0;
  F32(a1, 0x1422c) = 0;
  F32(a1, 0x1422e) = 0;
  F32(a1, 0x14230) = 0;
  F32(a1, 0x14278) = 0;
  F32(a1, 0x147ac) = 0;
  F32(a1, 0x147f4) = 0;
  F32(a1, 0x14232) = 0;
  F32(a1, 0x14802) = 0;
  F32(a1, 0x14270) = 0;
  F32(a1, 0x1427c) = 0;
  F32(a1, 0x14234) = 0;
  F32(a1, 0x14236) = 0;
  F32(a1, 0x14804) = 0;
  F32(a1, 0x14288) = 0;
  F32(a1, 0x1428c) = 0;
  F32(a1, 0x14248) = 0;
  F32(a1, 0x14290) = 0;
  F32(a1, 0x14294) = 0;
  F32(a1, 0x1423c) = 0;
  F32(a1, 0x14810) = 0;
  F32(a1, 0x14814) = 0;
  F32(a1, 0x14830) = 0;
  F32(a1, 0x1482e) = 0;
  F32(a1, 0x14274) = 0;
  F32(a1, 0x1428a) = 0;
  if ( !v10 )
  {
    do
    {
      result = (int)a1 + 320 * (uint16_t)v9++;
      F16(result, 52032) = F16(result, 52036);
    }
    while ( (uint16_t)v9 < F32(a1, 0x14228) );
  }
  F32(a1, 0x1bc) = 0;
  F32(a1, 0xc758) = 0;
  F32(a1, 0x1e4) = 2;
  return result;
}
/* unported callees: none */
