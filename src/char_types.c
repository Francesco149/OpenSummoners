/* char_types.c — the CHARACTER INIT's per-type inits the corpus does not meet yet (the character init 0x41f200 calls
 * them by code: monsters, bosses, party allies): 0x428c00 0x429060 0x429db0 0x42a500 0x42a9a0 0x42b9a0 0x42c130 0x42c510
 * 0x42cce0 0x42d3c0 0x42da50 0x42f840 0x42ff00 0x430400 (skill sets, AI records, approach / combo tables, part boxes,
 * stats) + their leaves 0x429d50 0x42b550 0x42b580 0x42b5f0 (a data-file table read) 0x42b840 0x42b890.  See
 * char_types.h.  (The three rh-combat reaches — slimes 0x42e290, skill-51 / skill-29 types — are in char_monster.c.)
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  UNVERIFIED: no recording meets
 * these types yet; REPLACE-wired, compile-checked. */
#include "char_types.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "char_def.h"
#include "char_monster.h"
#include "char_state.h"
#include "entity_life.h"
#include "file_stream.h"
#include "party_member.h"
#include "room_load.h"
#include "char_def.h"
#include "char_monster.h"
#include "char_state.h"
#include "entity_life.h"
#include "file_stream.h"
#include "party_member.h"
#include "room_load.h"
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

static inline intptr_t W_ai_skill_boxes_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)ai_skill_boxes_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8); }
static inline intptr_t W_char_15a5c_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_15a5c_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_char_ai_skill_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, ...) { return (intptr_t)char_ai_skill_add((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (uint16_t)a8, (uint16_t)a9, (uint16_t)a10, (uint16_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16); }
static inline intptr_t W_char_approach_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { char_approach_add((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (uint16_t)a12); return 0; }
static inline intptr_t W_char_c880_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { char_c880_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_carry_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { char_carry_set((wl_char *)a0, (int32_t)a1, (const void *)a2, (const void *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); return 0; }
static inline intptr_t W_char_combo_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, ...) { return (intptr_t)char_combo_add((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (uint16_t)a10, (uint16_t)a11, (uint16_t)a12, (uint16_t)a13, (uint16_t)a14); }
static inline intptr_t W_char_follow_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_follow_set((wl_char *)a0, (int32_t)a1, (wl_char *)a2, (int32_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_char_jump_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { char_jump_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (const void *)a7, (const void *)a8, (const void *)a9, (uint16_t)a10); return 0; }
static inline intptr_t W_char_member_pose_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { char_member_pose_set((wl_char *)a0, (uint16_t)a1, (uint16_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_move_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { char_move_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8); return 0; }
static inline intptr_t W_char_nav_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { char_nav_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_nav_skills_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { char_nav_skills_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); return 0; }
static inline intptr_t W_char_pace_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_pace_set((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_char_party_ai_skills(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_party_ai_skills((wl_char *)a0, (uint16_t)a1, (uint16_t)a2, (uint16_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_char_skill_def(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, intptr_t a20, intptr_t a21, intptr_t a22, ...) { return (intptr_t)char_skill_def((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (uint16_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (uint16_t)a19, (uint16_t)a20, (int32_t)a21, (const char *)a22); }
static inline intptr_t W_char_skill_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skill_find((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_char_skill_learn(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skill_learn((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_char_skill_list_add(intptr_t a0, intptr_t a1, ...) { char_skill_list_add((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_char_voice_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { char_voice_init((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_entity_set_collide(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_set_collide((wl_entity *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_entity_set_track(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_set_track((wl_entity *)a0, (uint16_t)a1, (int16_t)a2, (int16_t)a3, (uint8_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_fs_5b6060(intptr_t a0, ...) { return (intptr_t)fs_5b6060((wl_char *)a0); }
static inline intptr_t W_fs_5b60d0(intptr_t a0, ...) { return (intptr_t)fs_5b60d0((wl_char *)a0); }
static inline intptr_t W_fs_5b62a0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b62a0((wl_char *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_fs_5b6340(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b6340((wl_char *)a0, (int32_t)a1, (uint32_t)a2); }
static inline intptr_t W_fs_5b64b0(intptr_t a0, ...) { return (intptr_t)fs_5b64b0((wl_char *)a0); }
static inline intptr_t W_skill_row_find(intptr_t a0, ...) { return (intptr_t)skill_row_find((int32_t)a0); }

int32_t ci_428c00(wl_char *self, int32_t a2, int32_t a3, int16_t a4);
int32_t ci_429060(wl_char *self);
void ci_429d50(wl_char *self, int32_t a2, int16_t a3, int16_t a4);
int32_t ci_429db0(wl_char *self, int16_t a2);
int32_t ci_42a500(wl_char *self, int16_t a2);
int32_t ci_42a9a0(wl_char *self);
int32_t ci_42b550(wl_char *self, int32_t a2, int32_t a3);
int32_t ci_42b580(wl_char *self, int32_t a2, int32_t a3, int16_t a4);
int32_t ci_42b5f0(wl_char *self);
int32_t ci_42b840(wl_char *self);
int32_t ci_42b890(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ci_42b9a0(wl_char *self, int32_t a2);
int32_t ci_42c130(wl_char *self, int16_t a2);
int32_t ci_42c510(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ci_42cce0(wl_char *self, int16_t a2);
int32_t ci_42d3c0(wl_char *self);
int32_t ci_42da50(wl_char *self, uint32_t a2, int16_t a3);
int32_t ci_42f840(wl_char *self, int32_t a2);
int32_t ci_42ff00(wl_char *self, uint32_t a2);
int32_t ci_430400(wl_char *self, int32_t a2);

/* 0x428c00: __int16 __thiscall sub_428C00(cls_4022A0 *this, int a2, int a3, __int16 a4) */
int32_t ci_428c00(wl_char *self, int32_t a2, int32_t a3, int16_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x658530);
  F32(c, 0x15918) = (int)D(0x658d28);
  F32(c, 0x1591c) = (int)D(0x6589f8);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(2), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x6586c8);
  F32(c, 0x1598c) = 1;
  F32(c, 0x15990) = 48000;
  F32(c, 0x15994) = (int)D(0x6586c8);
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x658860);
  F32(c, 0x15a1c) = (int)D(0x658860);
  F16(c, 0x15a14) = 700;
  F16(c, 0x15a20) = 75;
  F32(c, 0xc848) = 500;
  F32(c, 0x15a24) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x658530);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x658b90);
  F32(c, 0x159b8) = (int)D(0x658d28);
  F32(c, 0x159bc) = (int)D(0x658d28);
  F16(c, 0x159c0) = 14;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F32(c, 0x15944) = 1;
  F32(c, 0x15924) = (int)D(0x62a400);
  F16(c, 0x15948) = 0;
  F32(c, 0x15928) = (int)D(0x62a400);
  W_char_skill_def((intptr_t)(c), (intptr_t)(63), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(64), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(1500), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v5 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x3Fu));
  v6 = &GAP(c, 0xca26, 320 * F16(c, 0x14228) + 2);
  if ( v5 != 0 )
  {
    F32(v6, 0) = v5;
    F32(v6, 4 * 1) = 63;
    F32(v6, 4 * 2) = 0;
    F32(v6, 4 * 3) = 1;
    F16(v6, 2 * 141) = 5;
    F16(v6, 2 * 140) = 0;
    F16(v6, 2 * 142) = 0;
    F32(v6, 4 * 72) = 1000;
    F16(v6, 2 * 146) = 10;
    F16(v6, 2 * 147) = 10;
    F16(v6, 2 * 8) = 0;
    F16(v6, 2 * 74) = 0;
    F32(v6, 4 * 74) = 0;
    F32(v6, 4 * 75) = 0;
    F32(v6, 4 * 76) = 0;
    F32(v6, 4 * 77) = 0;
    F32(v6, 4 * 78) = 1000;
    F16(v6, 2 * 158) = 0;
    F16(v6, 2 * 159) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)v6), (intptr_t)(0), (intptr_t)(11000), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(64), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-3200), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(12800), (intptr_t)(100), (intptr_t)(0x320u), (intptr_t)(4), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_approach_add((intptr_t)(c), (intptr_t)(13600), (intptr_t)(9600), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(15000), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
  W_char_approach_add((intptr_t)(c), (intptr_t)(6400), (intptr_t)(8000), (intptr_t)(-8000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
  W_char_approach_add((intptr_t)(c), (intptr_t)(3200), (intptr_t)(8000), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(1600), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x12Cu));
  F32(c, 0xc880) = 1;
  F32(c, 0xc884) = 12800;
  F32(c, 0xc888) = 800;
  F32(c, 0xc88c) = 12800;
  F32(c, 0x15a5c) = 4;
  F32(c, 0x15a60) = 1;
  F32(c, 0x15a64) = 0;
  F32(c, 0x15a68) = -4800;
  F16(c, 0xc89e) = 10;
  F16(c, 0xc8a0) = 100;
  F32(c, 0x1e8) = -2000;
  if ( a3 != 0 )
  {
    W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_pace_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(2000), (intptr_t)(2000), (intptr_t)(200));
  }
  SETLO(v7, F16(c, 0x158d0));
  if ( (uint16_t)v7 < 8u )
  {
    v7 = F16(c, 0x158d0);
    *(_DWORD *)&GAP(c, 0x158d2, 4 * v7 + 2) = 2;
    ++F16(c, 0x158d0);
  }
  return v7;
}

/* 0x429060: _DWORD *__thiscall sub_429060(cls_4022A0 *this) */
int32_t ci_429060(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    void * v3 = 0;
    uint16_t v4 = 0;
    char * v5 = 0;
    char * v6 = 0;
    char * v7 = 0;
    char * v8 = 0;
    int32_t * v9 = 0;
    int32_t * v10 = 0;
    char * v11 = 0;
    uint16_t v12 = 0;
    int v13 = 0;
    intptr_t v14 = 0;
    char * v15 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */

  v1 = *(_DWORD *)&GAP(W, 0x2774, 12);
  (*v17__f) = 7;
  (*v18__f) = 1;
  (*v19__f) = false;
  if ( v1 == 10 )
  {
    (*v17__f) = 4;
    (*v18__f) = 0;
  }
  else if ( v1 == 20 )
  {
    (*v17__f) = 6;
  }
  else
  {
    (*v19__f) = v1 == 50;
  }
  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(318), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x63d8f8);
  F32(c, 0x15918) = (int)D(0x63dc28);
  F32(c, 0x1591c) = (int)D(0x63df58);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15944) = 1;
  F16(c, 0x15948) = 0;
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x63e0f0);
  F32(c, 0x1598c) = 1;
  F32(c, 0x15990) = 48000;
  F32(c, 0x15994) = (int)D(0x63e0f0);
  F32(c, 0x15984) = 1;
  v3 = D(0x63e8e8);
  F32(c, 0x15988) = 1;
  F16(c, 0x15a08) = 0;
  F16(c, 0x15a0a) = 93;
  F32(c, 0x15924) = (int)D(0x63ea80);
  F32(c, 0x15928) = (int)D(0x63e288);
  if ( D(0x63e8e8) == 0 )
    v3 = D(0x63e750);
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x63e750);
  F32(c, 0x15a1c) = (int)v3;
  F16(c, 0x15a14) = 600;
  F16(c, 0x15a20) = 78;
  F32(c, 0xc848) = 600;
  v4 = F16(c, 0xc754);
  F32(c, 0x15a24) = 1;
  F32(c, 0x15a28) = 2;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x63e288);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 106;
  F32(c, 0x159f4) = (int)D(0x63e420);
  F32(c, 0x15a04) = -400;
  if ( v4 < 0x40u )
  {
    v5 = (uint8_t *)c + 768 * v4;
    strcpy(v5 + 1880, D(0x8a9b6c));
    strcpy(v5 + 1960, D(0x8a9b6c));
    F32(v5, 4 * 469) = 89;
    F16(v5, 2 * 1025) = 1;
    F32(v5, 4 * 513) = 0;
    F32(v5, 4 * 514) = 0;
    F32(v5, 4 * 515) = 0;
    F32(v5, 4 * 516) = 0;
    F32(v5, 4 * 517) = 15728640;
    F32(v5, 4 * 646) = 1000;
    F32(v5, 4 * 647) = 0;
    F32(v5, 4 * 648) = 0;
    F32(v5, 4 * 649) = 0;
    F32(v5, 4 * 650) = 0;
    F16(v5, 2 * 1302) = 0;
    F32(v5, 4 * 659) = 1;
    F32(v5, 4 * 652) = 0;
    F32(v5, 4 * 653) = 0;
    F32(v5, 4 * 654) = 0;
    F32(v5, 4 * 655) = 0;
    F16(v5, 2 * 1020) = 3;
    F32(v5, 4 * 511) = 0;
    F16(v5, 2 * 1024) = 0;
    F32(v5, 4 * 656) = 0;
    F16(v5, 2 * 1314) = 0;
    F16(v5, 2 * 1315) = 0;
    F32(v5, 4 * 658) = 3;
    F32(v5, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v6 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v6 + 1880, D(0x8a9b6c));
    strcpy(v6 + 1960, D(0x8a9b6c));
    F32(v6, 4 * 469) = 91;
    F16(v6, 2 * 1025) = 1;
    F32(v6, 4 * 513) = 0;
    F32(v6, 4 * 514) = 0;
    F32(v6, 4 * 515) = 0;
    F32(v6, 4 * 516) = 0;
    F32(v6, 4 * 517) = 15728640;
    F32(v6, 4 * 646) = 1000;
    F32(v6, 4 * 647) = 0;
    F32(v6, 4 * 648) = 0;
    F32(v6, 4 * 649) = 0;
    F32(v6, 4 * 650) = 0;
    F16(v6, 2 * 1302) = 0;
    F32(v6, 4 * 659) = 1;
    F32(v6, 4 * 652) = 1;
    F32(v6, 4 * 653) = 1;
    F32(v6, 4 * 654) = 0;
    F32(v6, 4 * 655) = 0;
    F16(v6, 2 * 1020) = 3;
    F32(v6, 4 * 511) = 0;
    F16(v6, 2 * 1024) = 0;
    F32(v6, 4 * 656) = 0;
    F16(v6, 2 * 1314) = 0;
    F16(v6, 2 * 1315) = 0;
    F32(v6, 4 * 658) = 3;
    F32(v6, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v7 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v7 + 1880, D(0x8a9b6c));
    strcpy(v7 + 1960, D(0x8a9b6c));
    F32(v7, 4 * 469) = 90;
    F16(v7, 2 * 1025) = 1;
    F32(v7, 4 * 513) = 0;
    F32(v7, 4 * 514) = 0;
    F32(v7, 4 * 515) = 0;
    F32(v7, 4 * 516) = 0;
    F32(v7, 4 * 517) = 15728640;
    F32(v7, 4 * 646) = 2000;
    F32(v7, 4 * 647) = 0;
    F32(v7, 4 * 648) = 0;
    F32(v7, 4 * 649) = 0;
    F32(v7, 4 * 650) = 0;
    F16(v7, 2 * 1302) = 0;
    F32(v7, 4 * 659) = 1;
    F32(v7, 4 * 652) = 0;
    F32(v7, 4 * 653) = 0;
    F32(v7, 4 * 654) = 1;
    F32(v7, 4 * 655) = 0;
    F16(v7, 2 * 1020) = 3;
    F32(v7, 4 * 511) = 0;
    F16(v7, 2 * 1024) = 0;
    F32(v7, 4 * 656) = 0;
    F16(v7, 2 * 1314) = 0;
    F16(v7, 2 * 1315) = 0;
    F32(v7, 4 * 658) = 3;
    F32(v7, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v8 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v8 + 1880, D(0x8a9b6c));
    strcpy(v8 + 1960, D(0x8a9b6c));
    F32(v8, 4 * 469) = 127;
    F16(v8, 2 * 1025) = 4;
    F32(v8, 4 * 513) = 0;
    F32(v8, 4 * 514) = 0;
    F32(v8, 4 * 515) = 0;
    F32(v8, 4 * 516) = 0;
    F32(v8, 4 * 517) = 15728640;
    F32(v8, 4 * 646) = 0;
    F32(v8, 4 * 647) = 0;
    F32(v8, 4 * 648) = 0;
    F32(v8, 4 * 649) = 0;
    F32(v8, 4 * 650) = 0;
    F16(v8, 2 * 1302) = 20;
    F32(v8, 4 * 659) = 1;
    F32(v8, 4 * 652) = 0;
    F32(v8, 4 * 653) = 0;
    F32(v8, 4 * 654) = 0;
    F32(v8, 4 * 655) = 0;
    F16(v8, 2 * 1020) = 3;
    F32(v8, 4 * 511) = 0;
    F16(v8, 2 * 1024) = 0;
    F32(v8, 4 * 656) = 0;
    F16(v8, 2 * 1314) = 0;
    F16(v8, 2 * 1315) = 0;
    F32(v8, 4 * 658) = 3;
    F32(v8, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(128), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(92), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v9 = W_skill_row_find((intptr_t)(47));
  v10 = v9;
  if ( v9 != 0 )
  {
    v11 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v9), (intptr_t)(F16(v9, 2 * 87)), (intptr_t)(v9[44]), (intptr_t)(v9[45]), (intptr_t)(v9[46]), (intptr_t)(v9[47]), (intptr_t)(v9[48]), (intptr_t)(v9[177]), (intptr_t)(v9[178]), (intptr_t)(v9[179]), (intptr_t)(v9[180]), (intptr_t)(v9[181]), (intptr_t)(F16(v9, 2 * 364)), (intptr_t)(v9[183]), (intptr_t)(v9[184]), (intptr_t)(v9[185]), (intptr_t)(v9[186]), (intptr_t)(v9[187]), (intptr_t)(F16(v9, 2 * 376)), (intptr_t)(F16(v9, 2 * 377)), (intptr_t)(v9[189]), (intptr_t)((uint8_t *)v9 + 84));
    if ( v11 != 0 )
    {
      F16(v11, 2 * 82) = F16(v10, 2 * 82);
      F32(v11, 4 * 42) = v10[42];
      F16(v11, 2 * 86) = F16(v10, 2 * 86);
    }
  }
  W_char_skill_learn((intptr_t)(c), (intptr_t)(20123));
  W_char_skill_learn((intptr_t)(c), (intptr_t)(20103));
  W_char_skill_learn((intptr_t)(c), (intptr_t)(136));
  W_char_skill_learn((intptr_t)(c), (intptr_t)(137));
  W_char_skill_learn((intptr_t)(c), (intptr_t)(20133));
  W_char_skill_learn((intptr_t)(c), (intptr_t)(48));
  if ( (*v18__f) != 0 )
    W_char_skill_learn((intptr_t)(c), (intptr_t)(20132));
  if ( (*v19__f) )
    W_char_skill_learn((intptr_t)(c), (intptr_t)(138));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(90), (intptr_t)(0), (intptr_t)(2), (intptr_t)(4000), (intptr_t)(9600), (intptr_t)(-3200), (intptr_t)(16000), (intptr_t)(120), (intptr_t)(0x3E8u), (intptr_t)(6), (intptr_t)(6), (intptr_t)(4000), (intptr_t)(9600), (intptr_t)(-3200), (intptr_t)(16000), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(89), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(-1600), (intptr_t)(9600), (intptr_t)(60), (intptr_t)(0x3E8u), (intptr_t)(6), (intptr_t)(6), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(91), (intptr_t)(0), (intptr_t)(2), (intptr_t)(8000), (intptr_t)(30000), (intptr_t)(-800), (intptr_t)(8000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(8000), (intptr_t)(30000), (intptr_t)(-800), (intptr_t)(8000), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(91), (intptr_t)(1), (intptr_t)(2), (intptr_t)(4000), (intptr_t)(16000), (intptr_t)(-16000), (intptr_t)(16000), (intptr_t)(100), (intptr_t)(0x384u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(4000), (intptr_t)(16000), (intptr_t)(-16000), (intptr_t)(16000), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(91), (intptr_t)(3), (intptr_t)(2), (intptr_t)(-4000), (intptr_t)(24000), (intptr_t)(8000), (intptr_t)(16000), (intptr_t)(100), (intptr_t)(0x384u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(-4000), (intptr_t)(24000), (intptr_t)(8000), (intptr_t)(16000), (intptr_t)(0));
  W_char_party_ai_skills((intptr_t)(c), (intptr_t)(2000), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(15728640));
  v12 = 0;
  v13 = F16(c, 0x14228) == 0;
  F32(c, 0xc854) = 92;
  F32(c, 0xc858) = 0;
  F32(c, 0xc85c) = 92;
  F32(c, 0xc860) = 2;
  F32(c, 0xc864) = -8000;
  F32(c, 0xc868) = 800;
  F32(c, 0xc86c) = 0;
  if ( !v13 )
  {
    do
    {
      v14 = 320 * v12;
      v13 = *(_DWORD *)&GAP(c, 0xca26, v14 + 6) == 48;
      v15 = &GAP(c, 0xca26, v14 + 2);
      if ( v13 )
      {
        F16(v15, 2 * 158) = 0;
        F16(v15, 2 * 159) = 800;
      }
      ++v12;
    }
    while ( v12 < F16(c, 0x14228) );
  }
  ci_429d50(c, 20133, 0, 500);
  ci_429d50(c, 20103, 0, 500);
  ci_429d50(c, 20123, 0, 500);
  W_char_15a5c_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(1), (intptr_t)(1600), (intptr_t)(-3200));
  W_char_jump_set((intptr_t)(c), (intptr_t)(-64000), (intptr_t)(1000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)((int)D(0x63df58)), (intptr_t)((int)D(0x63ddc0)), (intptr_t)((int)D(0x63ddc0)), (intptr_t)(134));
  W_char_move_set((intptr_t)(c), (intptr_t)(1), (intptr_t)(0), (intptr_t)(2000), (intptr_t)(40000), (intptr_t)(2000), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(12800));
  W_char_pace_set((intptr_t)(c), (intptr_t)((*v17__f)), (intptr_t)(10000), (intptr_t)(2000), (intptr_t)(0));
  W_char_c880_set((intptr_t)(c), (intptr_t)(16000), (intptr_t)(800), (intptr_t)(16000));
  W_char_nav_set((intptr_t)(c), (intptr_t)(F32(c, 0x1f4)), (intptr_t)(3200), (intptr_t)(F32(c, 0x15964)));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(90), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(200), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(92), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(6u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(92), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(5u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(91), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(91), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(91), (intptr_t)(3), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(89), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x14898) = 2;
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(20133), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15a44) = 0;
  F32(c, 0x15a48) = 0;
  F32(c, 0x15a50) = 0;
  F32(c, 0x15a54) = 0;
  F32(c, 0x15a4c) = 0;
  F32(c, 0x1e8) = 1800;
  F32(c, 0x15a94) = 1;
  F32(c, 0x15a6c) = 2;
  F16(c, 0x15a84) = 500;
  F16(c, 0x15a86) = 1000;
  if ( *(_DWORD *)&GAP(W, 10148 + 0x0, 16) == 2 )
    F32(c, 0x15a6c) = -1;
  return W_char_voice_init((intptr_t)(c), (intptr_t)(100174), (intptr_t)(1), (intptr_t)(1400));
}

/* 0x429d50: void __thiscall sub_429D50(cls_429D50 *this, int a2, __int16 a3, __int16 a4) */
void ci_429d50(wl_char *self, int32_t a2, int16_t a3, int16_t a4)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    char * v7 = 0;

  for ( i = 0; i < F16(c, 0x14228); ++i )
  {
    v5 = 320 * i;
    v6 = *(_DWORD *)&GAP(c, 0x0, v5 + 51756);
    v7 = &GAP(c, 0x0, v5 + 51752);
    if ( v6 == a2 )
    {
      F16(v7, 2 * 158) = a3;
      F16(v7, 2 * 159) = a4;
    }
  }
}

/* 0x429db0: _DWORD *__thiscall sub_429DB0(cls_4022A0 *this, __int16 a2) */
int32_t ci_429db0(wl_char *self, int16_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    char * v4 = 0;
    char * v5 = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x66a410);
  F32(c, 0x15918) = (int)D(0x66a410);
  F32(c, 0x1591c) = (int)D(0x66a410);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 3000;
  F32(c, 0x15974) = 4800;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -2400;
  F32(c, 0x15980) = (int)D(0x66a410);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x66a410);
  F32(c, 0x159b8) = (int)D(0x66a410);
  F32(c, 0x159bc) = (int)D(0x66a410);
  F16(c, 0x159c0) = 14;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x66a5a8);
  F32(c, 0x159f8) = 1;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x15924) = (int)D(0x66a740);
  F32(c, 0x15928) = (int)D(0x66a5a8);
  F32(c, 0x159f4) = 0;
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x66aa70);
  F32(c, 0x15a1c) = (int)D(0x66aa70);
  F16(c, 0x15a14) = 500;
  F16(c, 0x15a20) = 77;
  F32(c, 0xc848) = 300;
  F32(c, 0x15a24) = 1;
  F32(c, 0x1594c) = 0;
  F32(c, 0x15950) = 20000;
  F32(c, 0x15954) = 1000;
  F32(c, 0x15958) = 64000;
  v3 = F16(c, 0xc754);
  F32(c, 0x1595c) = 4000;
  F32(c, 0x15960) = 0;
  F32(c, 0x15964) = 0;
  F32(c, 0x1f4) = 0;
  if ( v3 < 0x40u )
  {
    v4 = (uint8_t *)c + 768 * v3;
    strcpy(v4 + 1880, D(0x8a9b6c));
    strcpy(v4 + 1960, D(0x8a9b6c));
    F32(v4, 4 * 469) = 86;
    F16(v4, 2 * 1025) = 1;
    F32(v4, 4 * 513) = 0;
    F32(v4, 4 * 514) = 0;
    F32(v4, 4 * 515) = 0;
    F32(v4, 4 * 516) = 0;
    F32(v4, 4 * 517) = 15728640;
    F32(v4, 4 * 646) = 1000;
    F32(v4, 4 * 647) = 0;
    F32(v4, 4 * 648) = 0;
    F32(v4, 4 * 649) = 0;
    F32(v4, 4 * 650) = 0;
    F16(v4, 2 * 1302) = 0;
    F32(v4, 4 * 659) = 1;
    F32(v4, 4 * 652) = 0;
    F32(v4, 4 * 653) = 0;
    F32(v4, 4 * 654) = 0;
    F32(v4, 4 * 655) = 0;
    F16(v4, 2 * 1020) = 3;
    F32(v4, 4 * 511) = 0;
    F16(v4, 2 * 1024) = 0;
    F32(v4, 4 * 656) = 0;
    F16(v4, 2 * 1314) = 0;
    F16(v4, 2 * 1315) = 0;
    F32(v4, 4 * 658) = 3;
    F32(v4, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v5 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v5 + 1880, D(0x8a9b6c));
    strcpy(v5 + 1960, D(0x8a9b6c));
    F32(v5, 4 * 469) = 87;
    F16(v5, 2 * 1025) = 4;
    F32(v5, 4 * 513) = 0;
    F32(v5, 4 * 514) = 0;
    F32(v5, 4 * 515) = 0;
    F32(v5, 4 * 516) = 0;
    F32(v5, 4 * 517) = 15728640;
    F32(v5, 4 * 646) = 0;
    F32(v5, 4 * 647) = 0;
    F32(v5, 4 * 648) = 0;
    F32(v5, 4 * 649) = 0;
    F32(v5, 4 * 650) = 0;
    F16(v5, 2 * 1302) = 0;
    F32(v5, 4 * 659) = 1;
    F32(v5, 4 * 652) = 0;
    F32(v5, 4 * 653) = 0;
    F32(v5, 4 * 654) = 0;
    F32(v5, 4 * 655) = 0;
    F16(v5, 2 * 1020) = 3;
    F32(v5, 4 * 511) = 0;
    F16(v5, 2 * 1024) = 0;
    F32(v5, 4 * 656) = 0;
    F16(v5, 2 * 1314) = 0;
    F16(v5, 2 * 1315) = 0;
    F32(v5, 4 * 658) = 3;
    F32(v5, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(88), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  F32(c, 0xc854) = 87;
  F32(c, 0xc858) = 0;
  F32(c, 0xc85c) = 87;
  F32(c, 0xc860) = 2;
  F32(c, 0xc864) = -8000;
  F32(c, 0xc868) = 800;
  F32(c, 0xc86c) = 0;
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(88), (intptr_t)(0), (intptr_t)(2), (intptr_t)(12000), (intptr_t)(12800), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(160), (intptr_t)(0x2EEu), (intptr_t)(20), (intptr_t)(10), (intptr_t)(12000), (intptr_t)(6400), (intptr_t)(800), (intptr_t)(3200), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(88), (intptr_t)(2), (intptr_t)(2), (intptr_t)(22000), (intptr_t)(12800), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(240), (intptr_t)(0x258u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(22000), (intptr_t)(8000), (intptr_t)(800), (intptr_t)(3200), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(86), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(4800), (intptr_t)(1600), (intptr_t)(4800), (intptr_t)(60), (intptr_t)(0x3E8u), (intptr_t)(6), (intptr_t)(6), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  switch ( F32(c, 0x1d4) )
  {
    case 0xC829:
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20132));
      goto LABEL_11;
    case 0xC82A:
      W_char_skill_learn((intptr_t)(c), (intptr_t)(139));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20114));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(140));
      W_char_15a5c_set((intptr_t)(c), (intptr_t)(6), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
LABEL_12:
      F16(c, 0x15a84) = 0;
      break;
    case 0x1874A:
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20106));
LABEL_11:
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20103));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20123));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20101));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20119));
      F32(c, 0x15a5c) = 5;
      F32(c, 0x15a60) = 1;
      F32(c, 0x15a64) = 0;
      F32(c, 0x15a68) = 0;
      goto LABEL_12;
    default:
      break;
  }
  W_char_party_ai_skills((intptr_t)(c), (intptr_t)(1000), (intptr_t)(2000), (intptr_t)(0), (intptr_t)(15728640));
  F32(c, 0x14898) = 2;
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(87), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(6u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(87), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(5u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(88), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(88), (intptr_t)(2), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(86), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(20101), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(12800));
  W_char_pace_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(16000), (intptr_t)(2000), (intptr_t)(200));
  return W_char_voice_init((intptr_t)(c), (intptr_t)(51241), (intptr_t)(1), (intptr_t)(-400));
}

/* 0x42a500: _DWORD *__thiscall sub_42A500(cls_4022A0 *this, __int16 a2) */
int32_t ci_42a500(wl_char *self, int16_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    char * v4 = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x652fa8);
  F32(c, 0x15918) = (int)D(0x653140);
  F32(c, 0x1591c) = (int)D(0x6532d8);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(2), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 3000;
  F32(c, 0x15974) = 4800;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -2400;
  F32(c, 0x15980) = (int)D(0x653938);
  F32(c, 0x1598c) = 1;
  F32(c, 0x15990) = 56000;
  F32(c, 0x15994) = (int)D(0x653938);
  F32(c, 0x15998) = 1;
  v3 = F16(c, 0xc754);
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x653470);
  F32(c, 0x159b8) = (int)D(0x653608);
  F32(c, 0x159bc) = (int)D(0x6537a0);
  F32(c, 0x15924) = (int)D(0x655420);
  F16(c, 0x159c0) = 134;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F32(c, 0x15984) = 1;
  F32(c, 0x15988) = 1;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x6555b8);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 0;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15944) = 2;
  F16(c, 0x15948) = 0;
  F32(c, 0x15928) = (int)D(0x62a400);
  if ( v3 < 0x40u )
  {
    v4 = (uint8_t *)c + 768 * v3;
    strcpy(v4 + 1880, D(0x8a9b6c));
    strcpy(v4 + 1960, D(0x8a9b6c));
    F32(v4, 4 * 469) = 66;
    F16(v4, 2 * 1025) = 1;
    F32(v4, 4 * 513) = 0;
    F32(v4, 4 * 514) = 0;
    F32(v4, 4 * 515) = 0;
    F32(v4, 4 * 516) = 0;
    F32(v4, 4 * 517) = 15728640;
    F32(v4, 4 * 646) = 1000;
    F32(v4, 4 * 647) = 0;
    F32(v4, 4 * 648) = 0;
    F32(v4, 4 * 649) = 0;
    F32(v4, 4 * 650) = 0;
    F16(v4, 2 * 1302) = 0;
    F32(v4, 4 * 659) = 1;
    F32(v4, 4 * 652) = 0;
    F32(v4, 4 * 653) = 0;
    F32(v4, 4 * 654) = 0;
    F32(v4, 4 * 655) = 0;
    F16(v4, 2 * 1020) = 3;
    F32(v4, 4 * 511) = 0;
    F16(v4, 2 * 1024) = 0;
    F32(v4, 4 * 656) = 0;
    F16(v4, 2 * 1314) = 0;
    F16(v4, 2 * 1315) = 0;
    F32(v4, 4 * 658) = 3;
    F32(v4, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(67), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(15728640), (intptr_t)(1200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(68), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(15728640), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(66), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(0), (intptr_t)(4800), (intptr_t)(5), (intptr_t)(0x3E8u), (intptr_t)(5), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(67), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(8000), (intptr_t)(-4000), (intptr_t)(8000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0xc854) = 68;
  F32(c, 0xc85c) = 68;
  F32(c, 0xc858) = 0;
  F32(c, 0xc860) = 2;
  F32(c, 0xc864) = -8000;
  F32(c, 0xc868) = 800;
  F32(c, 0xc86c) = 0;
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(68), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(6u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(68), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(5u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(67), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(66), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0xc880) = 1;
  F32(c, 0xc884) = 12800;
  F32(c, 0xc888) = 800;
  F32(c, 0xc88c) = 12800;
  F32(c, 0x158f4) = 16;
  return W_char_voice_init((intptr_t)(c), (intptr_t)(51211), (intptr_t)(0), (intptr_t)(-400));
}

/* 0x42a9a0: _DWORD *__thiscall sub_42A9A0(cls_4022A0 *this) */
int32_t ci_42a9a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    char * v3 = 0;
    char * v4 = 0;
    char * v5 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;
    char * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    int32_t * result = 0;

  F32(c, 0x15914) = (int)D(0x6672e8);
  F32(c, 0x15918) = (int)D(0x6672e8);
  F32(c, 0x1591c) = (int)D(0x6672e8);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x6672e8);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -60000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x6672e8);
  F32(c, 0x159b8) = (int)D(0x6672e8);
  F32(c, 0x159bc) = (int)D(0x6672e8);
  F16(c, 0x159c0) = 145;
  F32(c, 0x15984) = 1;
  F32(c, 0x15988) = 1;
  F32(c, 0x15924) = (int)D(0x6677b0);
  F32(c, 0x15928) = (int)D(0x6677b0);
  v2 = F16(c, 0xc754);
  F32(c, 0x159c4) = 400;
  F32(c, 0x159c8) = -500;
  F32(c, 0x159cc) = 1000;
  F16(c, 0x159ec) = 105;
  F32(c, 0x159f0) = (int)D(0x667618);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 0;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F16(c, 0x15a08) = 0;
  F16(c, 0x15a0a) = 18;
  F32(c, 0x159d0) = -3000;
  F32(c, 0x1594c) = 1;
  F32(c, 0x15950) = 12000;
  F32(c, 0x15954) = 2000;
  F32(c, 0x15958) = 64000;
  F32(c, 0x1595c) = 8000;
  F32(c, 0x15960) = 1;
  F32(c, 0x15964) = 0;
  F32(c, 0x1f4) = 16000;
  if ( v2 < 0x40u )
  {
    v3 = (uint8_t *)c + 768 * v2;
    strcpy(v3 + 1880, D(0x8a9b6c));
    strcpy(v3 + 1960, D(0x8a9b6c));
    F32(v3, 4 * 469) = 79;
    F16(v3, 2 * 1025) = 1;
    F32(v3, 4 * 513) = 0;
    F32(v3, 4 * 514) = 0;
    F32(v3, 4 * 515) = 0;
    F32(v3, 4 * 516) = 0;
    F32(v3, 4 * 517) = 15728640;
    F32(v3, 4 * 646) = 1000;
    F32(v3, 4 * 647) = 0;
    F32(v3, 4 * 648) = 0;
    F32(v3, 4 * 649) = 0;
    F32(v3, 4 * 650) = 0;
    F16(v3, 2 * 1302) = 0;
    F32(v3, 4 * 659) = 1;
    F32(v3, 4 * 652) = 0;
    F32(v3, 4 * 653) = 0;
    F32(v3, 4 * 654) = 0;
    F32(v3, 4 * 655) = 0;
    F16(v3, 2 * 1020) = 3;
    F32(v3, 4 * 511) = 0;
    F16(v3, 2 * 1024) = 0;
    F32(v3, 4 * 656) = 0;
    F16(v3, 2 * 1314) = 0;
    F16(v3, 2 * 1315) = 0;
    F32(v3, 4 * 658) = 3;
    F32(v3, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v4 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v4 + 1880, D(0x8a9b6c));
    strcpy(v4 + 1960, D(0x8a9b6c));
    F32(v4, 4 * 469) = 80;
    F16(v4, 2 * 1025) = 1;
    F32(v4, 4 * 513) = 0;
    F32(v4, 4 * 514) = 0;
    F32(v4, 4 * 515) = 0;
    F32(v4, 4 * 516) = 2;
    F32(v4, 4 * 517) = 15728640;
    F32(v4, 4 * 646) = 1000;
    F32(v4, 4 * 647) = 0;
    F32(v4, 4 * 648) = 0;
    F32(v4, 4 * 649) = 0;
    F32(v4, 4 * 650) = 0;
    F16(v4, 2 * 1302) = 0;
    F32(v4, 4 * 659) = 1;
    F32(v4, 4 * 652) = 0;
    F32(v4, 4 * 653) = 0;
    F32(v4, 4 * 654) = 0;
    F32(v4, 4 * 655) = 0;
    F16(v4, 2 * 1020) = 3;
    F32(v4, 4 * 511) = 0;
    F16(v4, 2 * 1024) = 0;
    F32(v4, 4 * 656) = 0;
    F16(v4, 2 * 1314) = 0;
    F16(v4, 2 * 1315) = 0;
    F32(v4, 4 * 658) = 3;
    F32(v4, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v5 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v5 + 1880, D(0x8a9b6c));
    strcpy(v5 + 1960, D(0x8a9b6c));
    F32(v5, 4 * 469) = 81;
    F16(v5, 2 * 1025) = 1;
    F32(v5, 4 * 513) = 0;
    F32(v5, 4 * 514) = 0;
    F32(v5, 4 * 515) = 0;
    F32(v5, 4 * 516) = 2;
    F32(v5, 4 * 517) = 15728640;
    F32(v5, 4 * 646) = 1400;
    F32(v5, 4 * 647) = 0;
    F32(v5, 4 * 648) = 0;
    F32(v5, 4 * 649) = 0;
    F32(v5, 4 * 650) = 0;
    F16(v5, 2 * 1302) = 0;
    F32(v5, 4 * 659) = 1;
    F32(v5, 4 * 652) = 0;
    F32(v5, 4 * 653) = 0;
    F32(v5, 4 * 654) = 1;
    F32(v5, 4 * 655) = 0;
    F16(v5, 2 * 1020) = 3;
    F32(v5, 4 * 511) = 0;
    F16(v5, 2 * 1024) = 0;
    F32(v5, 4 * 656) = 0;
    F16(v5, 2 * 1314) = 0;
    F16(v5, 2 * 1315) = 0;
    F32(v5, 4 * 658) = 3;
    F32(v5, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v6 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v6 + 1880, D(0x8a9b6c));
    strcpy(v6 + 1960, D(0x8a9b6c));
    F32(v6, 4 * 469) = 82;
    F16(v6, 2 * 1025) = 1;
    F32(v6, 4 * 513) = 0;
    F32(v6, 4 * 514) = 0;
    F32(v6, 4 * 515) = 0;
    F32(v6, 4 * 516) = 2;
    F32(v6, 4 * 517) = 15728640;
    F32(v6, 4 * 646) = 1100;
    F32(v6, 4 * 647) = 0;
    F32(v6, 4 * 648) = 0;
    F32(v6, 4 * 649) = 0;
    F32(v6, 4 * 650) = 0;
    F16(v6, 2 * 1302) = 0;
    F32(v6, 4 * 659) = 1;
    F32(v6, 4 * 652) = 0;
    F32(v6, 4 * 653) = 0;
    F32(v6, 4 * 654) = 1;
    F32(v6, 4 * 655) = 0;
    F16(v6, 2 * 1020) = 3;
    F32(v6, 4 * 511) = 0;
    F16(v6, 2 * 1024) = 0;
    F32(v6, 4 * 656) = 0;
    F16(v6, 2 * 1314) = 0;
    F16(v6, 2 * 1315) = 0;
    F32(v6, 4 * 658) = 3;
    F32(v6, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(83), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1200), (intptr_t)(30), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(84), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v7 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x4Fu));
  v8 = &GAP(c, 0xca26, 320 * F16(c, 0x14228) + 2);
  if ( v7 != 0 )
  {
    F32(v8, 0) = v7;
    F32(v8, 4 * 1) = 79;
    F32(v8, 4 * 2) = 0;
    F32(v8, 4 * 3) = 1;
    F16(v8, 2 * 141) = 10;
    F16(v8, 2 * 140) = 0;
    F16(v8, 2 * 142) = 0;
    F32(v8, 4 * 72) = 1000;
    F16(v8, 2 * 146) = 5;
    F16(v8, 2 * 147) = 10;
    F16(v8, 2 * 8) = 0;
    F16(v8, 2 * 74) = 0;
    F32(v8, 4 * 74) = 0;
    F32(v8, 4 * 75) = 0;
    F32(v8, 4 * 76) = 0;
    F32(v8, 4 * 77) = 0;
    F32(v8, 4 * 78) = 1000;
    F16(v8, 2 * 158) = 0;
    F16(v8, 2 * 159) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)v8), (intptr_t)(0), (intptr_t)(12000), (intptr_t)(3000), (intptr_t)(12000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  v9 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(80), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-4000), (intptr_t)(12000), (intptr_t)(6000), (intptr_t)(12000), (intptr_t)(10), (intptr_t)(0x3E8u), (intptr_t)(5), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  if ( v9 != 0 )
    W_ai_skill_boxes_add((intptr_t)(v9), (intptr_t)(-12000), (intptr_t)(12000), (intptr_t)(12000), (intptr_t)(12000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(81), (intptr_t)(0), (intptr_t)(1), (intptr_t)(4000), (intptr_t)(20000), (intptr_t)(8000), (intptr_t)(20000), (intptr_t)(60), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(82), (intptr_t)(0), (intptr_t)(2), (intptr_t)(8000), (intptr_t)(36000), (intptr_t)(6000), (intptr_t)(20000), (intptr_t)(30), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(8000), (intptr_t)(32000), (intptr_t)(6000), (intptr_t)(18000), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(83), (intptr_t)(0), (intptr_t)(1), (intptr_t)(12000), (intptr_t)(28000), (intptr_t)(6000), (intptr_t)(18000), (intptr_t)(80), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(83), (intptr_t)(1), (intptr_t)(1), (intptr_t)(12000), (intptr_t)(20000), (intptr_t)(-10000), (intptr_t)(18000), (intptr_t)(70), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(83), (intptr_t)(2), (intptr_t)(2), (intptr_t)(28000), (intptr_t)(24000), (intptr_t)(4000), (intptr_t)(14000), (intptr_t)(75), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(28000), (intptr_t)(24000), (intptr_t)(4000), (intptr_t)(14000), (intptr_t)(0));
  W_char_nav_skills_set((intptr_t)(c), (intptr_t)(84), (intptr_t)(0), (intptr_t)(82), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(84), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(6u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(83), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(200), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(83), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(200), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(83), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(200), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(82), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(81), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(80), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(79), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v10 = (intptr_t)g_eng.op_new(0x20u);
  if ( v10 != 0 )
  {
    F32(v10, 0x0) = 0;
    F32(v10 + (1) * 0x4, 0x0) = 0;
  }
  else
  {
    v10 = 0;
  }
  F32(c, 0x8) = (int)v10;
  ci_42b580(v10, 10, SYS32(788), 1986);
  ci_42b550(F32(c, 0x8), 1001, 304);
  ci_42b550(F32(c, 0x8), 1002, 305);
  ci_42b550(F32(c, 0x8), 1003, 306);
  ci_42b550(F32(c, 0x8), 1004, 307);
  ci_42b550(F32(c, 0x8), 1005, 308);
  ci_42b5f0(F32(c, 0x8));
  ci_42b840(c);
  ci_42b890(c, 0, 0, 6400, 4800, 1500, 1, 1001);
  ci_42b890(c, 0, 0, 6400, 6400, 500, 2, 1006);
  F32(c, 0x158f4) = -128;
  F32(c, 0x158f8) = 16;
  W_char_follow_set((intptr_t)(c), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_pace_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(16000), (intptr_t)(8000), (intptr_t)(200));
  W_char_c880_set((intptr_t)(c), (intptr_t)(20000), (intptr_t)(800), (intptr_t)(20000));
  W_char_nav_set((intptr_t)(c), (intptr_t)(F32(c, 0x1f4)), (intptr_t)(8000), (intptr_t)(20800));
  result = W_char_voice_init((intptr_t)(c), (intptr_t)(100181), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15a44) = 0;
  F32(c, 0x15a48) = 0;
  F32(c, 0x15a94) = 1;
  F32(c, 0x15a4c) = 0;
  F32(c, 0x15a50) = 0;
  F32(c, 0x15a54) = 500;
  return result;
}

/* 0x42b550: unsigned __int16 __thiscall sub_42B550(cls_42B550 *this, int a2, int a3) */
int32_t ci_42b550(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t result = 0;

  F32((F32(c, 0x4) + 8 * F16(c, 0x8)), 0) = a2;
  result = F16(c, 0x8);
  F32(F32(c, 0x4) + 8 * result, 4) = a3;
  ++F16(c, 0x8);
  return result;
}

/* 0x42b580: __int16 __thiscall sub_42B580(cls_42B580 *this, int a2, int a3, __int16 a4) */
int32_t ci_42b580(wl_char *self, int32_t a2, int32_t a3, int16_t a4)
{
    intptr_t c = (intptr_t)self;
    void * v5 = 0;

  v5 = (void *)F32(c, 0x0);
  F32(c, 4 * 5) = 0;
  F32(c, 4 * 6) = 0;
  F32(c, 4 * 7) = 0;
  if ( v5 != 0 )
  {
    g_eng.op_delete((void *)v5);
    F32(c, 0x0) = 0;
  }
  if ( F32(c, 4 * 1) != 0 )
  {
    g_eng.op_delete((void *)*((void **)c + 1));
    F32(c, 4 * 1) = 0;
  }
  F16(c, 2 * 4) = 0;
  F32(c, 4 * 1) = (intptr_t)g_eng.op_new(8 * a2);
  F32(c, 4 * 3) = a3;
  F16(c, 2 * 8) = a4;
  return a4;
}

/* 0x42b5f0: int __thiscall sub_42B5F0(cls_42B5F0 *this) */
int32_t ci_42b5f0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x48c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v17 = (intptr_t *)(frame_ + 0x1c);   /* [13] at ebp-0x470 */
    intptr_t *v18 = (intptr_t *)(frame_ + 0x50);   /* [7] at ebp-0x43c */
    intptr_t *v19 = (intptr_t *)(frame_ + 0x6c);   /* [52] at ebp-0x420 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x48c */
    uint16_t *const v12__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x488 */
    uint32_t *const v13__f = (uint32_t *)(frame_ + 0xc);   /* ebp-0x480 */
    uint32_t *const v14__f = (uint32_t *)(frame_ + 0x10);   /* ebp-0x47c */
    uint32_t *const nNumberOfBytesToRead__f = (uint32_t *)(frame_ + 0x14);   /* ebp-0x478 */
    uint32_t *const Buffer__f = (uint32_t *)(frame_ + 0x18);   /* ebp-0x474 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x488);   /* ebp-0x4 */

  W_fs_5b6060((intptr_t)(v19));
  (*v12__f) = F16(c, 2 * 8);
  (*v11__f) = *((intptr_t *)c + 3);
  (*v20__f) = 0;
  if ( W_fs_5b62a0((intptr_t)(v19), (intptr_t)((*v11__f)), (intptr_t)((*v12__f))) != 0 )
  {
    W_fs_5b6340((intptr_t)(v19), (intptr_t)(&(*Buffer__f)), (intptr_t)(4u));
    (*v14__f) = 0;
    (*nNumberOfBytesToRead__f) = 0;
    W_fs_5b6340((intptr_t)(v19), (intptr_t)(&(*v14__f)), (intptr_t)((*Buffer__f)));
    memset(v18, 0, (7 * sizeof(intptr_t)));
    W_fs_5b6340((intptr_t)(v19), (intptr_t)(v18), (intptr_t)((*nNumberOfBytesToRead__f)));
    v3 = v18[0];
    v4 = v18[4];
    F32(c, 4 * (7)) = v18[0];
    v5 = v18[1];
    F32(c, 4 * 5) = v4;
    F32(c, 4 * 6) = v5;
    F32(c, 0x0) = (int)(intptr_t)g_eng.op_new(52 * v4 * v3);
    (*v13__f) = 0;
    if ( v18[4] * F32(c, 4 * 7) != 0 )
    {
      v6 = 0;
      do
      {
        memset(v17, 0, (13 * sizeof(intptr_t)));
        W_fs_5b6340((intptr_t)(v19), (intptr_t)(v17), (intptr_t)((*v14__f)));
        qmemcpy((void *)(v6 + F32(c, 0x0)), v17, 24);
        v7 = 0;
        v8 = F16(c, 2 * 4);
        if ( v8 != 0 )
        {
          v9 = F32(c, 4 * 1);
          while ( v17[6] != F32((v9 + 8 * v7), 0) )
          {
            if ( ++v7 >= v8 )
              goto LABEL_9;
          }
          v10 = F32(v9 + 8 * v7, 4);
        }
        else
        {
LABEL_9:
          v10 = 0;
        }
        F32(v6 + F32(c, 0x0), 24) = v10;
        F32(v6 + F32(c, 0x0), 28) = v17[7];
        F32(v6 + F32(c, 0x0), 32) = v17[8];
        F32(v6 + F32(c, 0x0), 36) = v17[9];
        F32(v6 + F32(c, 0x0), 40) = v17[10];
        F32(v6 + F32(c, 0x0), 44) = v17[11];
        F32(v6 + F32(c, 0x0), 48) = v17[12];
        if ( F32(v6 + F32(c, 0x0), 24) == 0 )
          F32(v6 + F32(c, 0x0), 20) = 1;
        v6 += 52;
        ++(*v13__f);
      }
      while ( (*v13__f) < v18[4] * F32(c, 4 * 7) );
    }
    W_fs_5b64b0((intptr_t)(v19));
    (*v20__f) = -1;
    W_fs_5b60d0((intptr_t)(v19));
    return 1;
  }
  else
  {
    (*v20__f) = -1;
    W_fs_5b60d0((intptr_t)(v19));
    return 0;
  }
}

/* 0x42b840: void *__thiscall sub_42B840(cls_4022A0 *this) */
int32_t ci_42b840(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    void * result = 0;
    intptr_t v3 = 0;
    int v4 = 0;
    intptr_t v5 = 0;

  result = (intptr_t)g_eng.op_new(0x80u);
  v3 = 0;
  v4 = F16(c, 0x44) == 0;
  F32(c, 0x0) = (int)result;
  F16(c, 0x4) = 0;
  if ( !v4 )
  {
    do
    {
      result = (intptr_t)g_eng.op_new(0x50u);
      v5 = 33 * (uint16_t)v3++;
      F32(F32(c, 0x40) + 20 * v5, 656) = result;
    }
    while ( (uint16_t)v3 < F16(c, 0x44) );
  }
  return result;
}

/* 0x42b890: int __thiscall sub_42B890(cls_42B890 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t ci_42b890(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;

  if ( F16(c, 0x4) >= 4u )
    return 0;
  v9 = F16(c, 0x4);
  v10 = 32 * v9;
  v11 = 0;
  F32((v10 + F32(c, 0x0)), 0) = a2;
  F32(v10 + F32(c, 0x0), 4) = a3;
  F32(v10 + F32(c, 0x0), 8) = a4;
  F32(v10 + F32(c, 0x0), 12) = a5;
  F32(v10 + F32(c, 0x0), 16) = a6;
  F32(v10 + F32(c, 0x0), 20) = a7;
  F32(v10 + F32(c, 0x0), 24) = a8;
  F16(v10 + F32(c, 0x0), 28) = 0;
  if ( F16(c, 0x44) != 0 )
  {
    v12 = 20 * v9;
    do
    {
      v13 = 660 * (uint16_t)v11++;
      F32((v12 + F32(v13 + *(_DWORD *)&GAP(c, 0x24, 28), 656)), 0) = F32((v10 + F32(c, 0x0)), 0);
      F32(v12 + F32((v13 + *(_DWORD *)&GAP(c, 0x24, 28) + 656), 0), 4) = F32(v10 + F32(c, 0x0), 4);
      F32(v12 + F32((v13 + *(_DWORD *)&GAP(c, 0x24, 28) + 656), 0), 8) = F32(v10 + F32(c, 0x0), 8);
      F32(v12 + F32((v13 + *(_DWORD *)&GAP(c, 0x24, 28) + 656), 0), 12) = F32(v10 + F32(c, 0x0), 12);
      F32(F32((*(_DWORD *)&GAP(c, 0x24, 28) + v13 + 656), 0) + v12, 16) = 0;
    }
    while ( (uint16_t)v11 < F16(c, 0x44) );
  }
  ++F16(c, 0x4);
  return 1;
}

/* 0x42b9a0: unsigned int __thiscall sub_42B9A0(cls_4022A0 *this, int a2) */
int32_t ci_42b9a0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    char * v4 = 0;
    char * v5 = 0;
    uint32_t v6 = 0;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint32_t result = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x65e120);
  F32(c, 0x15918) = (int)D(0x65e2b8);
  F32(c, 0x1591c) = (int)D(0x65e2b8);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 4000;
  F32(c, 0x15970) = 400;
  F32(c, 0x15974) = 200;
  F32(c, 0x15978) = -1600;
  F32(c, 0x1597c) = -3200;
  F32(c, 0x15980) = (int)D(0x65e120);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -56000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 2000;
  F32(c, 0x159b0) = 4000;
  F32(c, 0x159a8) = 40000;
  F32(c, 0x159b4) = (int)D(0x65e120);
  F32(c, 0x159b8) = (int)D(0x65e120);
  F32(c, 0x159bc) = (int)D(0x65e120);
  F16(c, 0x159c0) = 208;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x65e450);
  F32(c, 0x159f8) = 1;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15924) = (int)D(0x65e5e8);
  F32(c, 0x15928) = (int)D(0x65e450);
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x65e780);
  F32(c, 0x15a1c) = (int)D(0x65e780);
  F16(c, 0x15a14) = 700;
  F16(c, 0x15a20) = 78;
  F32(c, 0xc848) = 800;
  F32(c, 0x15a24) = 1;
  F32(c, 0x15954) = 4000;
  F32(c, 0x1595c) = 4000;
  F32(c, 0x1f4) = 4000;
  v3 = F16(c, 0xc754);
  F32(c, 0x1594c) = 1;
  F32(c, 0x15950) = 8000;
  F32(c, 0x15958) = 48000;
  F32(c, 0x15960) = 1;
  F32(c, 0x15964) = 0;
  if ( v3 < 0x40u )
  {
    v4 = (uint8_t *)c + 768 * v3;
    strcpy(v4 + 1880, D(0x8a9b6c));
    a2 = (int)(v4 + 1960);
    strcpy(v4 + 1960, D(0x8a9b6c));
    F32(v4, 4 * 469) = 96;
    F16(v4, 2 * 1025) = 4;
    F32(v4, 4 * 513) = 0;
    F32(v4, 4 * 514) = 0;
    F32(v4, 4 * 515) = 0;
    F32(v4, 4 * 516) = 0;
    F32(v4, 4 * 517) = 15728640;
    F32(v4, 4 * 646) = 0;
    F32(v4, 4 * 647) = 0;
    F32(v4, 4 * 648) = 0;
    F32(v4, 4 * 649) = 0;
    F32(v4, 4 * 650) = 0;
    F16(v4, 2 * 1302) = 0;
    F32(v4, 4 * 659) = 1;
    F32(v4, 4 * 652) = 0;
    F32(v4, 4 * 653) = 0;
    F32(v4, 4 * 654) = 0;
    F32(v4, 4 * 655) = 0;
    F16(v4, 2 * 1020) = 3;
    F32(v4, 4 * 511) = 0;
    F16(v4, 2 * 1024) = 0;
    F32(v4, 4 * 656) = 0;
    F16(v4, 2 * 1314) = 0;
    F16(v4, 2 * 1315) = 0;
    F32(v4, 4 * 658) = 3;
    F32(v4, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  if ( F16(c, 0xc754) < 0x40u )
  {
    v5 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v5 + 1880, D(0x8a9b6c));
    a2 = (int)(v5 + 1960);
    strcpy(v5 + 1960, D(0x8a9b6c));
    F32(v5, 4 * 469) = 97;
    F16(v5, 2 * 1025) = 1;
    F32(v5, 4 * 513) = 0;
    F32(v5, 4 * 514) = 0;
    F32(v5, 4 * 515) = 0;
    F32(v5, 4 * 516) = 0;
    F32(v5, 4 * 517) = 15728640;
    F32(v5, 4 * 646) = 1000;
    F32(v5, 4 * 647) = 0;
    F32(v5, 4 * 648) = 0;
    F32(v5, 4 * 649) = 0;
    F32(v5, 4 * 650) = 0;
    F16(v5, 2 * 1302) = 0;
    F32(v5, 4 * 659) = 1;
    F32(v5, 4 * 652) = 0;
    F32(v5, 4 * 653) = 0;
    F32(v5, 4 * 654) = 0;
    F32(v5, 4 * 655) = 0;
    F16(v5, 2 * 1020) = 3;
    F32(v5, 4 * 511) = 0;
    F16(v5, 2 * 1024) = 0;
    F32(v5, 4 * 656) = 0;
    F16(v5, 2 * 1314) = 0;
    F16(v5, 2 * 1315) = 0;
    F32(v5, 4 * 658) = 3;
    F32(v5, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  v6 = F32(c, 0x1d4);
  if ( v6 > 0x1875C )
  {
    if ( v6 != 100189 )
      goto LABEL_9;
  }
  else
  {
    if ( v6 == 100188 || (v7 = v6 - 51261) == 0 )
    {
      W_char_skill_def((intptr_t)(c), (intptr_t)(94), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1200), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
      v8 = 94;
      F32(c, 0x15a30) = 500;
      F32(c, 0x15a38) = 1500;
      goto LABEL_10;
    }
    if ( v7 != 1 )
    {
LABEL_9:
      v8 = a2;
      goto LABEL_10;
    }
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(95), (intptr_t)(1), (intptr_t)(4), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1200), (intptr_t)(20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v8 = 95;
  F32(c, 0x15a30) = 1500;
  F32(c, 0x15a38) = 500;
LABEL_10:
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v8), (intptr_t)(2), (intptr_t)(2), (intptr_t)(12000), (intptr_t)(24000), (intptr_t)(-1000), (intptr_t)(10000), (intptr_t)(160), (intptr_t)(0x320u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(12000), (intptr_t)(20000), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(1), (intptr_t)(4000), (intptr_t)(14000), (intptr_t)(5000), (intptr_t)(10000), (intptr_t)(160), (intptr_t)(0x320u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v8), (intptr_t)(1), (intptr_t)(1), (intptr_t)(4000), (intptr_t)(14000), (intptr_t)(-2000), (intptr_t)(10000), (intptr_t)(160), (intptr_t)(0x320u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(97), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-3200), (intptr_t)(8000), (intptr_t)(1600), (intptr_t)(5600), (intptr_t)(40), (intptr_t)(0x3E8u), (intptr_t)(5), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0xc858) = 0;
  F32(c, 0xc854) = 96;
  F32(c, 0xc85c) = 96;
  F32(c, 0xc860) = 2;
  F32(c, 0xc864) = -8000;
  F32(c, 0xc868) = 800;
  F32(c, 0xc86c) = 0;
  F32(c, 0x14240) = 8;
  v9 = F32(c, 0x40);
  F32(c, 0x1424c) = 0;
  F32(c, 0x14250) = 0;
  F32(c, 0x14254) = 0;
  F32(c, 0x14244) = F32(v9, 44);
  F16(c, 0x14248) = 0;
  F32(c, 0x14258) = 0;
  W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(6400), (intptr_t)(12800));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(96), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(6u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(96), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10), (intptr_t)(200), (intptr_t)(40), (intptr_t)(200), (intptr_t)(5u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(v8), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(v8), (intptr_t)(2), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(97), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_voice_init((intptr_t)(c), (intptr_t)(51261), (intptr_t)(0), (intptr_t)(0));
  result = F32(c, 0x1d4);
  if ( result >= 0x1875C && result <= 0x1875D )
  {
    F32(c, 0x15a44) = 0;
    F32(c, 0x15a48) = 0;
    F32(c, 0x15a50) = 500;
    F32(c, 0x15a54) = 500;
    F32(c, 0x15a4c) = 0;
    F32(c, 0x15a94) = 1;
    return W_char_member_pose_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(18), (intptr_t)(0));
  }
  return result;
}

/* 0x42c130: _DWORD *__thiscall sub_42C130(cls_4022A0 *this, __int16 a2) */
int32_t ci_42c130(wl_char *self, int16_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    char * v4 = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x65ff40);
  F32(c, 0x15918) = (int)D(0x6600d8);
  F32(c, 0x1591c) = (int)D(0x6600d8);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 4000;
  F32(c, 0x15970) = 400;
  F32(c, 0x15974) = 200;
  F32(c, 0x15978) = -1600;
  F32(c, 0x1597c) = -3200;
  F32(c, 0x15980) = (int)D(0x65ff40);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -56000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 2000;
  F32(c, 0x159b0) = 4000;
  F32(c, 0x159a8) = 40000;
  F32(c, 0x159b4) = (int)D(0x65ff40);
  F32(c, 0x159b8) = (int)D(0x65ff40);
  F32(c, 0x159bc) = (int)D(0x65ff40);
  F16(c, 0x159c0) = 143;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x660270);
  F32(c, 0x159f8) = 1;
  F32(c, 0x15924) = (int)D(0x660408);
  F32(c, 0x15928) = (int)D(0x660270);
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F16(c, 0x15a08) = 0;
  F16(c, 0x15a0a) = 39;
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x6605a0);
  F32(c, 0x15a1c) = (int)D(0x6605a0);
  F16(c, 0x15a14) = 700;
  F16(c, 0x15a20) = 78;
  F32(c, 0x1f4) = 4000;
  F32(c, 0xc848) = 800;
  F32(c, 0x15a24) = 1;
  F32(c, 0x1594c) = 1;
  F32(c, 0x15950) = 16000;
  F32(c, 0x15954) = 2000;
  F32(c, 0x15958) = 64000;
  F32(c, 0x1595c) = 8000;
  F32(c, 0x15960) = 1;
  F32(c, 0x15964) = 0;
  W_char_skill_def((intptr_t)(c), (intptr_t)(36), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(37), (intptr_t)(1), (intptr_t)(8), (intptr_t)(600), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v3 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x24u));
  v4 = &GAP(c, 0xca26, 320 * F16(c, 0x14228) + 2);
  if ( v3 != 0 )
  {
    *(_DWORD *)&GAP(c, 0xca26, 320 * F16(c, 0x14228) + 6) = 36;
    F32(v4, 4 * 2) = 0;
    F32(v4, 0) = v3;
    F32(v4, 4 * 3) = 1;
    F16(v4, 2 * 141) = 80;
    F16(v4, 2 * 140) = 0;
    F16(v4, 2 * 142) = 0;
    F32(v4, 4 * 72) = 1000;
    F16(v4, 2 * 146) = 5;
    F16(v4, 2 * 147) = 10;
    F16(v4, 2 * 8) = 0;
    F16(v4, 2 * 74) = 0;
    F32(v4, 4 * 74) = 0;
    F32(v4, 4 * 75) = 0;
    F32(v4, 4 * 76) = 0;
    F32(v4, 4 * 77) = 0;
    F32(v4, 4 * 78) = 1000;
    F16(v4, 2 * 158) = 0;
    F16(v4, 2 * 159) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)v4), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(5600), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(37), (intptr_t)(1), (intptr_t)(1), (intptr_t)(6400), (intptr_t)(9600), (intptr_t)(-8000), (intptr_t)(12800), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(37), (intptr_t)(0), (intptr_t)(1), (intptr_t)(6400), (intptr_t)(9600), (intptr_t)(-1600), (intptr_t)(12800), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(37), (intptr_t)(3), (intptr_t)(1), (intptr_t)(0), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(12800), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(36), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0xc880) = 1;
  F32(c, 0xc884) = 12800;
  F32(c, 0xc888) = 6400;
  F32(c, 0xc88c) = 12800;
  F16(c, 0xc89e) = 10;
  F16(c, 0xc8a0) = 100;
  return W_char_voice_init((intptr_t)(c), (intptr_t)(51090), (intptr_t)(0), (intptr_t)(0));
}

/* 0x42c510: int __thiscall sub_42C510(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t ci_42c510(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    uint16_t v7 = 0;
    char * v8 = 0;
    intptr_t result = 0;

  F32(c, 0x15914) = (int)D(0x661bc8);
  F32(c, 0x15918) = (int)D(0x661d60);
  F32(c, 0x1591c) = (int)D(0x661d60);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(1), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 4000;
  F32(c, 0x15970) = 400;
  F32(c, 0x15974) = 200;
  F32(c, 0x15978) = -1600;
  F32(c, 0x1597c) = -3200;
  F32(c, 0x15980) = (int)D(0x661bc8);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -56000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159ac) = 2000;
  F32(c, 0x159b4) = (int)D(0x661bc8);
  F32(c, 0x159b8) = (int)D(0x661bc8);
  F32(c, 0x159bc) = (int)D(0x661bc8);
  F32(c, 0x159f0) = (int)D(0x661bc8);
  F32(c, 0x15954) = 2000;
  v7 = F16(c, 0xc754);
  F32(c, 0x15924) = (int)D(0x62a598);
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159b0) = 4000;
  F32(c, 0x159a8) = 40000;
  F16(c, 0x159c0) = 157;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f8) = 1;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15928) = (int)D(0x62a598);
  F32(c, 0x1594c) = 1;
  F32(c, 0x15950) = 24000;
  F32(c, 0x15958) = 64000;
  F32(c, 0x1595c) = 8000;
  F32(c, 0x15960) = 1;
  F32(c, 0x15964) = 0;
  F32(c, 0x1f4) = 4000;
  if ( v7 < 0x40u )
  {
    v8 = (uint8_t *)c + 768 * v7;
    strcpy(v8 + 1880, D(0x8a9b6c));
    strcpy(v8 + 1960, D(0x8a9b6c));
    F32(v8, 4 * 469) = 33;
    F16(v8, 2 * 1025) = 1;
    F32(v8, 4 * 513) = a2;
    F32(v8, 4 * 514) = a3;
    F32(v8, 4 * 515) = 0;
    F32(v8, 4 * 516) = 0;
    F32(v8, 4 * 517) = 15728640;
    F32(v8, 4 * 646) = 1000;
    F32(v8, 4 * 647) = 0;
    F32(v8, 4 * 648) = 0;
    F32(v8, 4 * 649) = 0;
    F32(v8, 4 * 650) = 0;
    F16(v8, 2 * 1302) = 0;
    F32(v8, 4 * 659) = 1;
    F32(v8, 4 * 652) = 0;
    F32(v8, 4 * 653) = 0;
    F32(v8, 4 * 654) = 0;
    F32(v8, 4 * 655) = 0;
    F16(v8, 2 * 1020) = 3;
    F32(v8, 4 * 511) = 0;
    F16(v8, 2 * 1024) = 0;
    F32(v8, 4 * 656) = 0;
    F16(v8, 2 * 1314) = 0;
    F16(v8, 2 * 1315) = 0;
    F32(v8, 4 * 658) = 3;
    F32(v8, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(34), (intptr_t)(1), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(33), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-1600), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(4800), (intptr_t)(80), (intptr_t)(0x3E8u), (intptr_t)(5), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(34), (intptr_t)(0), (intptr_t)(2), (intptr_t)(8000), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(6400), (intptr_t)(150), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(8000), (intptr_t)(9600), (intptr_t)(1600), (intptr_t)(3200), (intptr_t)(0));
  if ( a6 > 0 || a4 != 0 )
  {
    W_char_skill_def((intptr_t)(c), (intptr_t)(35), (intptr_t)(1), (intptr_t)(a4), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(a6), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(35), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(28000), (intptr_t)(8000), (intptr_t)(8000), (intptr_t)(500), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(0), (intptr_t)(24000), (intptr_t)(8000), (intptr_t)(6000), (intptr_t)(0));
  }
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(35), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_pace_set((intptr_t)(c), (intptr_t)(2), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(0));
  W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(6400), (intptr_t)(12800));
  result = W_char_nav_set((intptr_t)(c), (intptr_t)(F32(c, 0x1f4)), (intptr_t)(-800), (intptr_t)(12800));
  F32(c, 0x15a30) = 1500;
  return result;
}

/* 0x42cce0: _DWORD *__thiscall sub_42CCE0(cls_4022A0 *this, __int16 a2) */
int32_t ci_42cce0(wl_char *self, int16_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    char * v4 = 0;
    intptr_t v5 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x6504f0);
  F32(c, 0x15918) = (int)D(0x650688);
  F32(c, 0x1591c) = (int)D(0x650820);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(2), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x650e80);
  F32(c, 0x1598c) = 1;
  F32(c, 0x15990) = 40000;
  F32(c, 0x15994) = (int)D(0x650e80);
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x650b50);
  F32(c, 0x15a1c) = (int)D(0x650b50);
  F16(c, 0x15a14) = 750;
  F16(c, 0x15a20) = 75;
  F32(c, 0xc848) = 600;
  F32(c, 0x15a24) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x650ce8);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x650820);
  F32(c, 0x159b8) = (int)D(0x6509b8);
  F32(c, 0x159bc) = (int)D(0x6509b8);
  F16(c, 0x159c0) = 14;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F32(c, 0x15944) = 1;
  F32(c, 0x15924) = (int)D(0x62a400);
  F32(c, 0x15928) = (int)D(0x62a400);
  v3 = F16(c, 0xc754);
  F16(c, 0x15948) = 0;
  if ( v3 < 0x40u )
  {
    v4 = (uint8_t *)c + 768 * v3;
    strcpy(v4 + 1880, D(0x8a9b6c));
    strcpy(v4 + 1960, D(0x8a9b6c));
    F32(v4, 4 * 469) = 57;
    F16(v4, 2 * 1025) = 1;
    F32(v4, 4 * 513) = 0;
    F32(v4, 4 * 514) = 0;
    F32(v4, 4 * 515) = 0;
    F32(v4, 4 * 516) = 0;
    F32(v4, 4 * 517) = 15728640;
    F32(v4, 4 * 646) = 1000;
    F32(v4, 4 * 647) = 0;
    F32(v4, 4 * 648) = 0;
    F32(v4, 4 * 649) = 0;
    F32(v4, 4 * 650) = 0;
    F16(v4, 2 * 1302) = 0;
    F32(v4, 4 * 659) = 1;
    F32(v4, 4 * 652) = 0;
    F32(v4, 4 * 653) = 0;
    F32(v4, 4 * 654) = 0;
    F32(v4, 4 * 655) = 0;
    F16(v4, 2 * 1020) = 3;
    F32(v4, 4 * 511) = 0;
    F16(v4, 2 * 1024) = 0;
    F32(v4, 4 * 656) = 0;
    F16(v4, 2 * 1314) = 0;
    F16(v4, 2 * 1315) = 0;
    F32(v4, 4 * 658) = 3;
    F32(v4, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(58), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(15728640), (intptr_t)(1200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(59), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(60), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(15728640), (intptr_t)(1300), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v5 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x39u));
  v6 = &GAP(c, 0xca26, 320 * F16(c, 0x14228) + 2);
  if ( v5 != 0 )
  {
    F32(v6, 0) = v5;
    F32(v6, 4 * 1) = 57;
    F32(v6, 4 * 2) = 0;
    F32(v6, 4 * 3) = 1;
    F16(v6, 2 * 141) = 10;
    F16(v6, 2 * 140) = 0;
    F16(v6, 2 * 142) = 0;
    F32(v6, 4 * 72) = 1000;
    F16(v6, 2 * 146) = 10;
    F16(v6, 2 * 147) = 10;
    F16(v6, 2 * 8) = 0;
    F16(v6, 2 * 74) = 0;
    F32(v6, 4 * 74) = 0;
    F32(v6, 4 * 75) = 0;
    F32(v6, 4 * 76) = 0;
    F32(v6, 4 * 77) = 0;
    F32(v6, 4 * 78) = 1000;
    F16(v6, 2 * 158) = 0;
    F16(v6, 2 * 159) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)v6), (intptr_t)(0), (intptr_t)(5600), (intptr_t)(0), (intptr_t)(4800), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  v7 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x39u));
  v8 = 320 * F16(c, 0x14228);
  if ( v7 != 0 )
  {
    *(_DWORD *)&GAP(c, 0xca26, v8 + 2) = v7;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 6) = 57;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 10) = 6;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 14) = 1;
    *(_WORD *)&GAP(c, 0xca26, v8 + 284) = 5;
    *(_WORD *)&GAP(c, 0xca26, v8 + 282) = 0;
    *(_WORD *)&GAP(c, 0xca26, v8 + 286) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 290) = 1000;
    *(_WORD *)&GAP(c, 0xca26, v8 + 294) = 10;
    *(_WORD *)&GAP(c, 0xca26, v8 + 296) = 10;
    *(_WORD *)&GAP(c, 0xca26, v8 + 18) = 0;
    *(_WORD *)&GAP(c, 0xca26, v8 + 150) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 298) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 302) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 306) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 310) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v8 + 314) = 1000;
    *(_WORD *)&GAP(c, 0xca26, v8 + 318) = 0;
    *(_WORD *)&GAP(c, 0xca26, v8 + 320) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)&GAP(c, 0xca26, v8 + 2)), (intptr_t)(0), (intptr_t)(7200), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(58), (intptr_t)(0), (intptr_t)(1), (intptr_t)(3200), (intptr_t)(5600), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(30), (intptr_t)(0x2BCu), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(59), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-3200), (intptr_t)(9600), (intptr_t)(-11200), (intptr_t)(12800), (intptr_t)(5), (intptr_t)(0x384u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(60), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-3200), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(12800), (intptr_t)(5), (intptr_t)(0x320u), (intptr_t)(4), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_approach_add((intptr_t)(c), (intptr_t)(13600), (intptr_t)(9600), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(15000), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
  W_char_approach_add((intptr_t)(c), (intptr_t)(6400), (intptr_t)(8000), (intptr_t)(-8000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
  W_char_approach_add((intptr_t)(c), (intptr_t)(3200), (intptr_t)(8000), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(1600), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x12Cu));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(60), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(59), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(58), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(57), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(12800));
  return W_char_voice_init((intptr_t)(c), (intptr_t)(51030), (intptr_t)(0), (intptr_t)(0));
}

/* 0x42d3c0: int __thiscall sub_42D3C0(cls_4022A0 *this) */
int32_t ci_42d3c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;
    int32_t * v3 = 0;
    char * v4 = 0;
    int32_t * v5 = 0;
    int32_t * v6 = 0;
    char * v7 = 0;
    int32_t * v8 = 0;
    int32_t * v9 = 0;
    char * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t result = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(282), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x647ff0);
  F32(c, 0x15918) = (int)D(0x647ff0);
  F32(c, 0x1591c) = (int)D(0x647ff0);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -56000;
  F32(c, 0x159a0) = 1000;
  F32(c, 0x159a4) = 4000;
  F32(c, 0x159ac) = 0;
  F32(c, 0x159b0) = 0;
  F32(c, 0x159a8) = 0;
  F32(c, 0x159b4) = (int)D(0x647ff0);
  F32(c, 0x159b8) = (int)D(0x647ff0);
  F32(c, 0x159bc) = (int)D(0x647ff0);
  F16(c, 0x159c0) = 0;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 28000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x647ff0);
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x648188);
  F32(c, 0x15924) = (int)D(0x62a400);
  F32(c, 0x15928) = (int)D(0x648188);
  F32(c, 0x159f8) = 2;
  F32(c, 0x158fc) = 0;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x1594c) = 1;
  F32(c, 0x15950) = 0;
  F32(c, 0x15954) = 2000;
  F32(c, 0x15958) = 40000;
  F32(c, 0x1595c) = 2000;
  F32(c, 0x15960) = 0;
  F32(c, 0x15964) = 0;
  F32(c, 0x1f4) = 0;
  F32(c, 0x15a5c) = 4;
  F32(c, 0x15a60) = 0;
  F32(c, 0x15a64) = 0;
  F32(c, 0x15a68) = 0;
  v2 = W_skill_row_find((intptr_t)(134));
  v3 = v2;
  if ( v2 != 0 )
  {
    v4 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v2), (intptr_t)(F16(v2, 2 * 87)), (intptr_t)(v2[44]), (intptr_t)(v2[45]), (intptr_t)(v2[46]), (intptr_t)(v2[47]), (intptr_t)(v2[48]), (intptr_t)(v2[177]), (intptr_t)(v2[178]), (intptr_t)(v2[179]), (intptr_t)(v2[180]), (intptr_t)(v2[181]), (intptr_t)(F16(v2, 2 * 364)), (intptr_t)(v2[183]), (intptr_t)(v2[184]), (intptr_t)(v2[185]), (intptr_t)(v2[186]), (intptr_t)(v2[187]), (intptr_t)(F16(v2, 2 * 376)), (intptr_t)(F16(v2, 2 * 377)), (intptr_t)(v2[189]), (intptr_t)((uint8_t *)v2 + 84));
    if ( v4 != 0 )
    {
      F16(v4, 2 * 82) = F16(v3, 2 * 82);
      F32(v4, 4 * 42) = v3[42];
      F16(v4, 2 * 86) = F16(v3, 2 * 86);
    }
  }
  v5 = W_skill_row_find((intptr_t)(135));
  v6 = v5;
  if ( v5 != 0 )
  {
    v7 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v5), (intptr_t)(F16(v5, 2 * 87)), (intptr_t)(v5[44]), (intptr_t)(v5[45]), (intptr_t)(v5[46]), (intptr_t)(v5[47]), (intptr_t)(v5[48]), (intptr_t)(v5[177]), (intptr_t)(v5[178]), (intptr_t)(v5[179]), (intptr_t)(v5[180]), (intptr_t)(v5[181]), (intptr_t)(F16(v5, 2 * 364)), (intptr_t)(v5[183]), (intptr_t)(v5[184]), (intptr_t)(v5[185]), (intptr_t)(v5[186]), (intptr_t)(v5[187]), (intptr_t)(F16(v5, 2 * 376)), (intptr_t)(F16(v5, 2 * 377)), (intptr_t)(v5[189]), (intptr_t)((uint8_t *)v5 + 84));
    if ( v7 != 0 )
    {
      F16(v7, 2 * 82) = F16(v6, 2 * 82);
      F32(v7, 4 * 42) = v6[42];
      F16(v7, 2 * 86) = F16(v6, 2 * 86);
    }
  }
  v8 = W_skill_row_find((intptr_t)(47));
  v9 = v8;
  if ( v8 != 0 )
  {
    v10 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v8), (intptr_t)(F16(v8, 2 * 87)), (intptr_t)(v8[44]), (intptr_t)(v8[45]), (intptr_t)(v8[46]), (intptr_t)(v8[47]), (intptr_t)(v8[48]), (intptr_t)(v8[177]), (intptr_t)(v8[178]), (intptr_t)(v8[179]), (intptr_t)(v8[180]), (intptr_t)(v8[181]), (intptr_t)(F16(v8, 2 * 364)), (intptr_t)(v8[183]), (intptr_t)(v8[184]), (intptr_t)(v8[185]), (intptr_t)(v8[186]), (intptr_t)(v8[187]), (intptr_t)(F16(v8, 2 * 376)), (intptr_t)(F16(v8, 2 * 377)), (intptr_t)(v8[189]), (intptr_t)((uint8_t *)v8 + 84));
    if ( v10 != 0 )
    {
      F16(v10, 2 * 82) = F16(v9, 2 * 82);
      F32(v10, 4 * 42) = v9[42];
      F16(v10, 2 * 86) = F16(v9, 2 * 86);
    }
  }
  W_char_skill_learn((intptr_t)(c), (intptr_t)(20115));
  W_char_party_ai_skills((intptr_t)(c), (intptr_t)(2000), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(1));
  W_char_skill_def((intptr_t)(c), (intptr_t)(49), (intptr_t)(1), (intptr_t)(8), (intptr_t)(750), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(50), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(49), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(28000), (intptr_t)(3200), (intptr_t)(19200), (intptr_t)(50), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v11 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(50), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(20000), (intptr_t)(-8000), (intptr_t)(24000), (intptr_t)(60), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v12 = v11;
  if ( v11 != 0 )
  {
    W_ai_skill_boxes_add((intptr_t)(v11), (intptr_t)(-30000), (intptr_t)(20000), (intptr_t)(-8000), (intptr_t)(24000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_ai_skill_boxes_add((intptr_t)(v12), (intptr_t)(-10000), (intptr_t)(20000), (intptr_t)(8000), (intptr_t)(20000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_ai_skill_boxes_add((intptr_t)(v12), (intptr_t)(-10000), (intptr_t)(20000), (intptr_t)(-28000), (intptr_t)(20000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  }
  F32(c, 0x14898) = 2;
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(47), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0xc890) = F32(c, 0x1f4);
  F32(c, 0xc898) = 6400;
  F32(c, 0xc894) = 30000;
  W_char_c880_set((intptr_t)(c), (intptr_t)(64000), (intptr_t)(6400), (intptr_t)(12800));
  W_char_pace_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(20000), (intptr_t)(800), (intptr_t)(300));
  F32(c, 0x15a90) = 0;
  F32(c, 0x1590c) = 1;
  F32(c, 0x15a94) = 1;
  W_char_member_pose_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(18), (intptr_t)(0));
  W_char_voice_init((intptr_t)(c), (intptr_t)(100185), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15a44) = 0;
  F32(c, 0x15a48) = 0;
  F32(c, 0x15a54) = 500;
  F32(c, 0x15a4c) = 0;
  F32(c, 0x15a50) = 0;
  SETLO(result, W_char_skill_list_add((intptr_t)(c), (intptr_t)(2)));
  return result;
}

/* 0x42da50: __int16 __thiscall sub_42DA50(cls_4022A0 *this, unsigned int a2, __int16 a3) */
int32_t ci_42da50(wl_char *self, uint32_t a2, int16_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v4 = 0;
    char * v5 = 0;
    uint32_t v6 = 0;
    char * v7 = 0;
    intptr_t v8 = 0;
    char * v9 = 0;
    intptr_t v10 = 0;
    uint32_t v11 = 0;

  W_entity_set_track((intptr_t)(c), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15914) = (int)D(0x6497b0);
  F32(c, 0x15918) = (int)D(0x6497b0);
  F32(c, 0x1591c) = (int)D(0x6497b0);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -40000;
  F32(c, 0x159a0) = 1000;
  F32(c, 0x159a4) = 4000;
  F32(c, 0x159ac) = 0;
  F32(c, 0x159b0) = 0;
  F32(c, 0x159a8) = 0;
  F32(c, 0x159b4) = (int)D(0x6497b0);
  F32(c, 0x159b8) = (int)D(0x6497b0);
  F32(c, 0x159bc) = (int)D(0x6497b0);
  F16(c, 0x159c0) = 0;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 28000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x6497b0);
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x649948);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x15924) = (int)D(0x62a400);
  F32(c, 0x15928) = (int)D(0x649948);
  F32(c, 0x15954) = 2000;
  F32(c, 0x1595c) = 2000;
  v4 = F16(c, 0xc754);
  F32(c, 0x159f4) = 0;
  F16(c, 0x15a08) = 0;
  F16(c, 0x15a0a) = 45;
  F32(c, 0x1594c) = 3;
  F32(c, 0x15950) = 0;
  F32(c, 0x15958) = 40000;
  F32(c, 0x15960) = 0;
  F32(c, 0x15964) = 0;
  F32(c, 0x1f4) = 0;
  if ( v4 < 0x40u )
  {
    v5 = (uint8_t *)c + 768 * v4;
    strcpy(v5 + 1880, D(0x8a9b6c));
    strcpy(v5 + 1960, D(0x8a9b6c));
    F32(v5, 4 * 469) = 44;
    F16(v5, 2 * 1025) = 4;
    F32(v5, 4 * 513) = 0;
    F32(v5, 4 * 514) = 0;
    F32(v5, 4 * 515) = 0;
    F32(v5, 4 * 516) = 0;
    F32(v5, 4 * 517) = 1;
    F32(v5, 4 * 646) = 0;
    F32(v5, 4 * 647) = 0;
    F32(v5, 4 * 648) = 0;
    F32(v5, 4 * 649) = 0;
    F32(v5, 4 * 650) = 0;
    F16(v5, 2 * 1302) = 0;
    F32(v5, 4 * 659) = 1;
    F32(v5, 4 * 652) = 0;
    F32(v5, 4 * 653) = 0;
    F32(v5, 4 * 654) = 0;
    F32(v5, 4 * 655) = 0;
    F16(v5, 2 * 1020) = 3;
    F32(v5, 4 * 511) = 0;
    F16(v5, 2 * 1024) = 0;
    F32(v5, 4 * 656) = 0;
    F16(v5, 2 * 1314) = 0;
    F16(v5, 2 * 1315) = 0;
    F32(v5, 4 * 658) = 3;
    F32(v5, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  v6 = a2;
  F32(c, 0xc854) = 44;
  F32(c, 0xc858) = 2;
  F32(c, 0xc85c) = 44;
  F32(c, 0xc860) = 0;
  F32(c, 0xc864) = 0;
  F32(c, 0xc868) = 0;
  F32(c, 0xc86c) = 0;
  if ( a2 == 51081 )
  {
    if ( F16(c, 0xc754) < 0x40u )
    {
      v7 = (uint8_t *)c + 768 * F16(c, 0xc754);
      strcpy(v7 + 1880, D(0x8a9b6c));
      strcpy(v7 + 1960, D(0x8a9b6c));
      F32(v7, 4 * 469) = 43;
      F16(v7, 2 * 1025) = 1;
      F32(v7, 4 * 513) = 0;
      F32(v7, 4 * 514) = 0;
      F32(v7, 4 * 515) = 0;
      F32(v7, 4 * 516) = 0;
      F32(v7, 4 * 517) = 1;
      F32(v7, 4 * 646) = 1000;
      F32(v7, 4 * 647) = 0;
      F32(v7, 4 * 648) = 0;
      F32(v7, 4 * 649) = 0;
      F32(v7, 4 * 650) = 0;
      F16(v7, 2 * 1302) = 0;
      F32(v7, 4 * 659) = 1;
      F32(v7, 4 * 652) = 0;
      F32(v7, 4 * 653) = 0;
      F32(v7, 4 * 654) = 0;
      F32(v7, 4 * 655) = 0;
      F16(v7, 2 * 1020) = 3;
      F32(v7, 4 * 511) = 0;
      F16(v7, 2 * 1024) = 0;
      F32(v7, 4 * 656) = 0;
      F16(v7, 2 * 1314) = 0;
      F16(v7, 2 * 1315) = 0;
      F32(v7, 4 * 658) = 3;
      F32(v7, 4 * 660) = 0;
      ++F16(c, 0xc754);
    }
    v8 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x2Bu));
    v9 = &GAP(c, 0xca26, 320 * F16(c, 0x14228) + 2);
    if ( v8 != 0 )
    {
      F32(v9, 0) = v8;
      F32(v9, 4 * 1) = 43;
      F32(v9, 4 * 2) = 0;
      F32(v9, 4 * 3) = 1;
      F16(v9, 2 * 141) = 20;
      F16(v9, 2 * 140) = 0;
      F16(v9, 2 * 142) = 0;
      F32(v9, 4 * 72) = 1000;
      F16(v9, 2 * 146) = 5;
      F16(v9, 2 * 147) = 10;
      F16(v9, 2 * 8) = 0;
      F16(v9, 2 * 74) = 0;
      F32(v9, 4 * 74) = 0;
      F32(v9, 4 * 75) = 0;
      F32(v9, 4 * 76) = 0;
      F32(v9, 4 * 77) = 0;
      F32(v9, 4 * 78) = 1000;
      F16(v9, 2 * 158) = 0;
      F16(v9, 2 * 159) = 0;
      W_ai_skill_boxes_add((intptr_t)((int)v9), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      ++F16(c, 0x14228);
    }
    W_char_pace_set((intptr_t)(c), (intptr_t)(2), (intptr_t)(1600), (intptr_t)(800), (intptr_t)(300));
    v10 = F32(c, 0x1f4);
    F32(c, 0xc898) = -2000;
    F32(c, 0xc890) = v10;
    F32(c, 0xc894) = 16000;
    goto LABEL_12;
  }
  if ( a2 > 0xC789 && a2 <= 0xC78B )
  {
    W_char_skill_def((intptr_t)(c), (intptr_t)(46), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(46), (intptr_t)(1), (intptr_t)(2), (intptr_t)(4000), (intptr_t)(12000), (intptr_t)(-12000), (intptr_t)(10000), (intptr_t)(60), (intptr_t)(0x384u), (intptr_t)(25), (intptr_t)(5), (intptr_t)(6400), (intptr_t)(8000), (intptr_t)(-11200), (intptr_t)(6000), (intptr_t)(0));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(46), (intptr_t)(0), (intptr_t)(2), (intptr_t)(12000), (intptr_t)(12800), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(160), (intptr_t)(0x2EEu), (intptr_t)(20), (intptr_t)(10), (intptr_t)(12000), (intptr_t)(6400), (intptr_t)(800), (intptr_t)(3200), (intptr_t)(0));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(46), (intptr_t)(3), (intptr_t)(2), (intptr_t)(9600), (intptr_t)(12800), (intptr_t)(4800), (intptr_t)(12800), (intptr_t)(100), (intptr_t)(0x320u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(9600), (intptr_t)(6400), (intptr_t)(4800), (intptr_t)(6400), (intptr_t)(0));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(46), (intptr_t)(2), (intptr_t)(2), (intptr_t)(22000), (intptr_t)(12800), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(240), (intptr_t)(0x258u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(22000), (intptr_t)(8000), (intptr_t)(800), (intptr_t)(3200), (intptr_t)(0));
    W_char_pace_set((intptr_t)(c), (intptr_t)(2), (intptr_t)(20000), (intptr_t)(800), (intptr_t)(300));
    W_char_nav_set((intptr_t)(c), (intptr_t)(F32(c, 0x1f4)), (intptr_t)(0), (intptr_t)(16000));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(46), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(46), (intptr_t)(3), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(46), (intptr_t)(2), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(46), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    F16(c, 0x15a84) = 0;
LABEL_12:
    v6 = a2;
  }
  v11 = v6 - 51082;
  if ( v11 != 0 )
  {
    if ( v11 == 1 )
    {
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20104));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(140));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(47));
      W_char_skill_learn((intptr_t)(c), (intptr_t)(20132));
    }
  }
  else
  {
    W_char_skill_learn((intptr_t)(c), (intptr_t)(47));
    W_char_skill_learn((intptr_t)(c), (intptr_t)(20106));
    W_char_skill_learn((intptr_t)(c), (intptr_t)(20123));
    W_char_skill_learn((intptr_t)(c), (intptr_t)(20103));
  }
  W_char_party_ai_skills((intptr_t)(c), (intptr_t)(2000), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x14898) = 2;
  W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(47), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_15a5c_set((intptr_t)(c), (intptr_t)(7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_follow_set((intptr_t)(c), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_c880_set((intptr_t)(c), (intptr_t)(64000), (intptr_t)(6400), (intptr_t)(12800));
  F32(c, 0x15a90) = 0;
  F32(c, 0x1590c) = 1;
  W_char_voice_init((intptr_t)(c), (intptr_t)(51081), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15a44) = 0;
  F32(c, 0x15a48) = 0;
  F32(c, 0x15a4c) = 0;
  return W_char_skill_list_add((intptr_t)(c), (intptr_t)(2));
}

/* 0x42f840: int __thiscall sub_42F840(cls_4022A0 *this, int a2) */
int32_t ci_42f840(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    void * v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t result = 0;

  F32(c, 0x15914) = (int)D(0x665340);
  F32(c, 0x15918) = (int)D(0x665808);
  F32(c, 0x1591c) = (int)D(0x6654d8);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(2), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -1600;
  F32(c, 0x1597c) = -3200;
  F32(c, 0x15980) = (int)D(0x62a730);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x6654d8);
  F32(c, 0x159b8) = (int)D(0x665670);
  F32(c, 0x159bc) = (int)D(0x665670);
  F16(c, 0x159c0) = 14;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x15924) = (int)D(0x62a400);
  F32(c, 0x15928) = (int)D(0x62a400);
  v3 = D(0x665b38);
  F32(c, 0x159f0) = (int)D(0x665340);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15944) = 3;
  F16(c, 0x15948) = 147;
  if ( D(0x665b38) == 0 )
    v3 = D(0x6659a0);
  F32(c, 0x15a1c) = (int)v3;
  F32(c, 0x15a10) = 1;
  F32(c, 0x15a18) = (int)D(0x6659a0);
  F16(c, 0x15a14) = 600;
  F16(c, 0x15a20) = 78;
  F32(c, 0xc848) = 600;
  F32(c, 0x15a24) = 0;
  W_char_skill_def((intptr_t)(c), (intptr_t)(25), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_skill_def((intptr_t)(c), (intptr_t)(26), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(15728640), (intptr_t)(1500), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  v4 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x19u));
  v5 = 320 * F16(c, 0x14228);
  if ( v4 != 0 )
  {
    *(_DWORD *)&GAP(c, 0xca26, v5 + 2) = v4;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 6) = 25;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 10) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 14) = 1;
    *(_WORD *)&GAP(c, 0xca26, v5 + 284) = 80;
    *(_WORD *)&GAP(c, 0xca26, v5 + 282) = 0;
    *(_WORD *)&GAP(c, 0xca26, v5 + 286) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 290) = 1000;
    *(_WORD *)&GAP(c, 0xca26, v5 + 294) = 5;
    *(_WORD *)&GAP(c, 0xca26, v5 + 296) = 10;
    *(_WORD *)&GAP(c, 0xca26, v5 + 18) = 0;
    *(_WORD *)&GAP(c, 0xca26, v5 + 150) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 298) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 302) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 306) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 310) = 0;
    *(_DWORD *)&GAP(c, 0xca26, v5 + 314) = 1000;
    *(_WORD *)&GAP(c, 0xca26, v5 + 318) = 0;
    *(_WORD *)&GAP(c, 0xca26, v5 + 320) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)&GAP(c, 0xca26, v5 + 2)), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(4800), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(26), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-3200), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(12800), (intptr_t)(100), (intptr_t)(0x320u), (intptr_t)(4), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_approach_add((intptr_t)(c), (intptr_t)(0), (intptr_t)(12800), (intptr_t)(-3200), (intptr_t)(16000), (intptr_t)(0), (intptr_t)(3200), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(0), (intptr_t)(9600), (intptr_t)(0x12Cu));
  v6 = F32(c, 0x40);
  F32(c, 0x14240) = 5;
  F32(c, 0x1424c) = 0;
  F32(c, 0x14250) = 0;
  F32(c, 0x14254) = 0;
  F32(c, 0x14244) = F32(v6, 44);
  F16(c, 0x14248) = 0;
  F32(c, 0x14258) = 0;
  result = W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(6400), (intptr_t)(12800));
  if ( a2 != 0 )
  {
    F32(c, 0x15a44) = 0;
    F32(c, 0x15a48) = 0;
    F32(c, 0x15a50) = 500;
    F32(c, 0x15a54) = 500;
    F32(c, 0x15a4c) = 0;
    return 500;
  }
  return result;
}

/* 0x42ff00: __int16 __thiscall sub_42FF00(cls_4022A0 *this, unsigned int a2) */
int32_t ci_42ff00(wl_char *self, uint32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    char * v4 = 0;
    int16_t result = 0;

  F32(c, 0x15914) = (int)D(0x64b758);
  F32(c, 0x15918) = (int)D(0x64b8f0);
  F32(c, 0x1591c) = (int)D(0x64ba88);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(2), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 24000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -1600;
  F32(c, 0x1597c) = -3200;
  F32(c, 0x15980) = (int)D(0x64bf50);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x64bc20);
  F32(c, 0x159b8) = (int)D(0x64bdb8);
  F32(c, 0x159bc) = (int)D(0x64bdb8);
  F16(c, 0x159c0) = 14;
  F32(c, 0x159c4) = 0;
  F32(c, 0x15924) = (int)D(0x62a400);
  F32(c, 0x15928) = (int)D(0x62a400);
  F32(c, 0x15a18) = (int)D(0x64c0e8);
  F32(c, 0x15a1c) = (int)D(0x64c0e8);
  v3 = F16(c, 0xc754);
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x64c280);
  F32(c, 0x159f8) = 0;
  F32(c, 0x158fc) = 1;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15944) = 3;
  F16(c, 0x15948) = 147;
  F32(c, 0x15a10) = 1;
  F16(c, 0x15a14) = 700;
  F16(c, 0x15a20) = 77;
  F32(c, 0xc848) = 600;
  F32(c, 0x15a24) = 0;
  F32(c, 0x15a90) = 0;
  F32(c, 0x1590c) = 0;
  if ( v3 < 0x40u )
  {
    v4 = (uint8_t *)c + 768 * v3;
    strcpy(v4 + 1880, D(0x8a9b6c));
    strcpy(v4 + 1960, D(0x8a9b6c));
    F32(v4, 4 * 469) = 28;
    F16(v4, 2 * 1025) = 1;
    F32(v4, 4 * 513) = 0;
    F32(v4, 4 * 514) = 0;
    F32(v4, 4 * 515) = 0;
    F32(v4, 4 * 516) = 0;
    F32(v4, 4 * 517) = 15728640;
    F32(v4, 4 * 646) = 700;
    F32(v4, 4 * 647) = 0;
    F32(v4, 4 * 648) = 0;
    F32(v4, 4 * 649) = 0;
    F32(v4, 4 * 650) = 0;
    F16(v4, 2 * 1302) = 0;
    F32(v4, 4 * 659) = 1;
    F32(v4, 4 * 652) = 0;
    F32(v4, 4 * 653) = 0;
    F32(v4, 4 * 654) = 0;
    F32(v4, 4 * 655) = 0;
    F16(v4, 2 * 1020) = 3;
    F32(v4, 4 * 511) = 0;
    F16(v4, 2 * 1024) = 0;
    F32(v4, 4 * 656) = 0;
    F16(v4, 2 * 1314) = 0;
    F16(v4, 2 * 1315) = 0;
    F32(v4, 4 * 658) = 3;
    F32(v4, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(28), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(2000), (intptr_t)(4800), (intptr_t)(20), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  if ( a2 == 51043 )
  {
    F32(c, 0x15a30) = 500;
    F32(c, 0x15a38) = 1500;
  }
  else
  {
    F32(c, 0x15a30) = 1500;
    F32(c, 0x15a38) = 500;
  }
  W_char_voice_init((intptr_t)(c), (intptr_t)(51040), (intptr_t)(0), (intptr_t)(0));
  result = 12800;
  F32(c, 0xc880) = 1;
  F32(c, 0xc884) = 12800;
  F32(c, 0xc888) = 6400;
  F32(c, 0xc88c) = 12800;
  if ( a2 <= 0xC763 )
  {
    if ( a2 < 0xC762 && a2 != 51040 )
    {
      F32(c, 0x1e8) = 0;
      return result;
    }
  }
  else if ( a2 != 100169 )
  {
LABEL_12:
    F32(c, 0x1e8) = 0;
    return result;
  }
  W_char_skill_def((intptr_t)(c), (intptr_t)(27), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(27), (intptr_t)(0), (intptr_t)(1), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(2000), (intptr_t)(4800), (intptr_t)(10), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(27), (intptr_t)(6), (intptr_t)(1), (intptr_t)(4800), (intptr_t)(6400), (intptr_t)(4800), (intptr_t)(4800), (intptr_t)(10), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_ai_skill_add((intptr_t)(c), (intptr_t)(27), (intptr_t)(1), (intptr_t)(1), (intptr_t)(3200), (intptr_t)(6400), (intptr_t)(-8000), (intptr_t)(6400), (intptr_t)(10), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_char_approach_add((intptr_t)(c), (intptr_t)(13600), (intptr_t)(9600), (intptr_t)(-5200), (intptr_t)(13600), (intptr_t)(15000), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(3200), (intptr_t)(9600), (intptr_t)(0x3E8u));
  result = W_char_approach_add((intptr_t)(c), (intptr_t)(6400), (intptr_t)(8000), (intptr_t)(-4800), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(3200), (intptr_t)(9600), (intptr_t)(0x3E8u));
  goto LABEL_12;
}

/* 0x430400: int __thiscall sub_430400(cls_4022A0 *this, int a2) */
int32_t ci_430400(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint32_t v3 = 0;
    uint32_t v4 = 0;
    char * v5 = 0;
    intptr_t v6 = 0;
    char * v7 = 0;
    int32_t * v8 = 0;
    int32_t * v9 = 0;
    char * v10 = 0;
    int32_t * v11 = 0;
    int32_t * v12 = 0;
    char * v13 = 0;
    int32_t * v14 = 0;
    int32_t * v15 = 0;
    char * v16 = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    uint32_t v27 = 0;
    uint32_t v28 = 0;
    intptr_t v29 = 0;
    uint32_t v30 = 0;
    intptr_t result = 0;
    uint32_t v32 = 0;
    uint32_t v33 = 0;
    uint16_t v34 = 0;

  F32(c, 0x15914) = (int)D(0x66c3b8);
  F32(c, 0x15918) = (int)D(0x66c880);
  F32(c, 0x1591c) = (int)D(0x66c550);
  F32(c, 0x15920) = 0;
  W_entity_set_collide((intptr_t)(c), (intptr_t)(a2), (intptr_t)(1));
  F32(c, 0x15924) = 0;
  F32(c, 0x15928) = 0;
  W_char_carry_set((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  F32(c, 0x15968) = 1;
  F32(c, 0x1596c) = 20000;
  F32(c, 0x15970) = 1600;
  F32(c, 0x15974) = 3200;
  F32(c, 0x15978) = -800;
  F32(c, 0x1597c) = -1600;
  F32(c, 0x15980) = (int)D(0x62a730);
  F32(c, 0x15998) = 1;
  F32(c, 0x1599c) = -80000;
  F32(c, 0x159a0) = 2000;
  F32(c, 0x159a4) = 8000;
  F32(c, 0x159ac) = 1600;
  F32(c, 0x159b0) = 3200;
  F32(c, 0x159a8) = 48000;
  F32(c, 0x159b4) = (int)D(0x66c6e8);
  F32(c, 0x159b8) = (int)D(0x66c880);
  F32(c, 0x159bc) = (int)D(0x66c880);
  F16(c, 0x159c0) = 14;
  F32(c, 0x159c4) = 0;
  F32(c, 0x159c8) = 0;
  F32(c, 0x159cc) = 0;
  F32(c, 0x159d0) = 0;
  F16(c, 0x159ec) = 104;
  F32(c, 0x159f0) = (int)D(0x66cbb0);
  F32(c, 0x159f8) = 0;
  F32(c, 0x15924) = (int)D(0x62a400);
  F32(c, 0x15928) = (int)D(0x62a400);
  v3 = F32(c, 0x1d4);
  F32(c, 0x158fc) = 0;
  F32(c, 0x159fc) = 0;
  F16(c, 0x15a00) = 0;
  F16(c, 0x15a02) = 0;
  F32(c, 0x15a04) = 0;
  F32(c, 0x159f4) = 0;
  F32(c, 0x15944) = 1;
  F16(c, 0x15948) = 0;
  if ( v3 > 0xC7B2 )
  {
    v4 = v3 - 100172;
    if ( v4 != 0 )
    {
      if ( v4 != 1 )
        goto LABEL_11;
LABEL_8:
      F32(c, 0x15a10) = 1;
      F32(c, 0x15a18) = (int)D(0x66ca18);
      F32(c, 0x15a1c) = (int)D(0x66ca18);
      F16(c, 0x15a14) = 500;
      F16(c, 0x15a20) = 77;
      F32(c, 0xc848) = 400;
    }
    else
    {
LABEL_9:
      F32(c, 0x15a10) = 1;
      F32(c, 0x15a18) = (int)D(0x66ca18);
      F32(c, 0x15a1c) = (int)D(0x66ca18);
      F16(c, 0x15a14) = 750;
      F16(c, 0x15a20) = 75;
      F32(c, 0xc848) = 600;
    }
LABEL_10:
    F32(c, 0x15a24) = 0;
  }
  else if ( v3 == 51122 )
  {
    F32(c, 0x15a10) = 1;
    F32(c, 0x15a18) = (int)D(0x66ca18);
    F32(c, 0x15a1c) = (int)D(0x66ca18);
    F16(c, 0x15a14) = 800;
    F16(c, 0x15a20) = 79;
    F32(c, 0xc848) = 800;
    F32(c, 0x15a24) = 0;
    F32(c, 0x15a28) = 1;
  }
  else
  {
    switch ( v3 )
    {
      case 0xC79Cu:
        goto LABEL_9;
      case 0xC7A6u:
      case 0xC7A7u:
        goto LABEL_8;
      case 0xC7B0u:
      case 0xC7B1u:
        F32(c, 0x15a10) = 1;
        F32(c, 0x15a18) = (int)D(0x66ca18);
        F32(c, 0x15a1c) = (int)D(0x66ca18);
        F16(c, 0x15a14) = 700;
        F16(c, 0x15a20) = 77;
        F32(c, 0xc848) = 500;
        goto LABEL_10;
      default:
        break;
    }
  }
LABEL_11:
  if ( F16(c, 0xc754) < 0x40u )
  {
    v5 = (uint8_t *)c + 768 * F16(c, 0xc754);
    strcpy(v5 + 1880, D(0x8a9b6c));
    strcpy(v5 + 1960, D(0x8a9b6c));
    F32(v5, 4 * 469) = 55;
    F16(v5, 2 * 1025) = 1;
    F32(v5, 4 * 513) = 0;
    F32(v5, 4 * 514) = 0;
    F32(v5, 4 * 515) = 0;
    F32(v5, 4 * 516) = 1;
    F32(v5, 4 * 517) = 15728640;
    F32(v5, 4 * 646) = 800;
    F32(v5, 4 * 647) = 0;
    F32(v5, 4 * 648) = 0;
    F32(v5, 4 * 649) = 0;
    F32(v5, 4 * 650) = 0;
    F16(v5, 2 * 1302) = 0;
    F32(v5, 4 * 659) = 1;
    F32(v5, 4 * 652) = 0;
    F32(v5, 4 * 653) = 0;
    F32(v5, 4 * 654) = 0;
    F32(v5, 4 * 655) = 0;
    F16(v5, 2 * 1020) = 3;
    F32(v5, 4 * 511) = 0;
    F16(v5, 2 * 1024) = 0;
    F32(v5, 4 * 656) = 0;
    F16(v5, 2 * 1314) = 0;
    F16(v5, 2 * 1315) = 0;
    F32(v5, 4 * 658) = 3;
    F32(v5, 4 * 660) = 0;
    ++F16(c, 0xc754);
  }
  v6 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x37u));
  v7 = &GAP(c, 0xca26, 320 * F16(c, 0x14228) + 2);
  if ( v6 != 0 )
  {
    F32(v7, 0) = v6;
    F32(v7, 4 * 1) = 55;
    F32(v7, 4 * 2) = 0;
    F32(v7, 4 * 3) = 1;
    F16(v7, 2 * 141) = 300;
    F16(v7, 2 * 140) = 0;
    F16(v7, 2 * 142) = 0;
    F32(v7, 4 * 72) = 1000;
    F16(v7, 2 * 146) = 10;
    F16(v7, 2 * 147) = 10;
    F16(v7, 2 * 8) = 0;
    F16(v7, 2 * 74) = 0;
    F32(v7, 4 * 74) = 0;
    F32(v7, 4 * 75) = 0;
    F32(v7, 4 * 76) = 0;
    F32(v7, 4 * 77) = 0;
    F32(v7, 4 * 78) = 1000;
    F16(v7, 2 * 158) = 0;
    F16(v7, 2 * 159) = 0;
    W_ai_skill_boxes_add((intptr_t)((int)v7), (intptr_t)(-2000), (intptr_t)(10000), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    ++F16(c, 0x14228);
  }
  W_char_voice_init((intptr_t)(c), (intptr_t)(51100), (intptr_t)(0), (intptr_t)(-600));
  if ( F16(c, 0x158d0) < 8u )
    *(_DWORD *)&GAP(c, 0x158d2, 4 * F16(c, 0x158d0)++ + 2) = 1;
  if ( F32(c, 0x1d4) == 51121 )
  {
    v8 = W_skill_row_find((intptr_t)(20102));
    v9 = v8;
    if ( v8 != 0 )
    {
      v10 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v8), (intptr_t)(F16(v8, 2 * 87)), (intptr_t)(v8[44]), (intptr_t)(v8[45]), (intptr_t)(v8[46]), (intptr_t)(v8[47]), (intptr_t)(v8[48]), (intptr_t)(v8[177]), (intptr_t)(v8[178]), (intptr_t)(v8[179]), (intptr_t)(v8[180]), (intptr_t)(v8[181]), (intptr_t)(F16(v8, 2 * 364)), (intptr_t)(v8[183]), (intptr_t)(v8[184]), (intptr_t)(v8[185]), (intptr_t)(v8[186]), (intptr_t)(v8[187]), (intptr_t)(F16(v8, 2 * 376)), (intptr_t)(F16(v8, 2 * 377)), (intptr_t)(v8[189]), (intptr_t)((uint8_t *)v8 + 84));
      if ( v10 != 0 )
      {
        F16(v10, 2 * 82) = F16(v9, 2 * 82);
        F32(v10, 4 * 42) = v9[42];
        F16(v10, 2 * 86) = F16(v9, 2 * 86);
      }
    }
    v11 = W_skill_row_find((intptr_t)(20110));
    v12 = v11;
    if ( v11 != 0 )
    {
      v13 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v11), (intptr_t)(F16(v11, 2 * 87)), (intptr_t)(v11[44]), (intptr_t)(v11[45]), (intptr_t)(v11[46]), (intptr_t)(v11[47]), (intptr_t)(v11[48]), (intptr_t)(v11[177]), (intptr_t)(v11[178]), (intptr_t)(v11[179]), (intptr_t)(v11[180]), (intptr_t)(v11[181]), (intptr_t)(F16(v11, 2 * 364)), (intptr_t)(v11[183]), (intptr_t)(v11[184]), (intptr_t)(v11[185]), (intptr_t)(v11[186]), (intptr_t)(v11[187]), (intptr_t)(F16(v11, 2 * 376)), (intptr_t)(F16(v11, 2 * 377)), (intptr_t)(v11[189]), (intptr_t)((uint8_t *)v11 + 84));
      if ( v13 != 0 )
      {
        F16(v13, 2 * 82) = F16(v12, 2 * 82);
        F32(v13, 4 * 42) = v12[42];
        F16(v13, 2 * 86) = F16(v12, 2 * 86);
      }
    }
    v14 = W_skill_row_find((intptr_t)(20103));
    v15 = v14;
    if ( v14 != 0 )
    {
      v16 = W_char_skill_def((intptr_t)(c), (intptr_t)(*v14), (intptr_t)(F16(v14, 2 * 87)), (intptr_t)(v14[44]), (intptr_t)(v14[45]), (intptr_t)(v14[46]), (intptr_t)(v14[47]), (intptr_t)(v14[48]), (intptr_t)(v14[177]), (intptr_t)(v14[178]), (intptr_t)(v14[179]), (intptr_t)(v14[180]), (intptr_t)(v14[181]), (intptr_t)(F16(v14, 2 * 364)), (intptr_t)(v14[183]), (intptr_t)(v14[184]), (intptr_t)(v14[185]), (intptr_t)(v14[186]), (intptr_t)(v14[187]), (intptr_t)(F16(v14, 2 * 376)), (intptr_t)(F16(v14, 2 * 377)), (intptr_t)(v14[189]), (intptr_t)((uint8_t *)v14 + 84));
      if ( v16 != 0 )
      {
        F16(v16, 2 * 82) = F16(v15, 2 * 82);
        F32(v16, 4 * 42) = v15[42];
        F16(v16, 2 * 86) = F16(v15, 2 * 86);
      }
    }
    v17 = 0;
    v34 = 0;
    if ( F16(c, 0xc754) != 0 )
    {
      while ( 1 )
      {
        v18 = *(_DWORD *)&GAP(c, 0x754, 768 * v17);
        if ( v18 > 20116 )
          break;
        if ( v18 == 20116 )
        {
LABEL_49:
          v19 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(4000), (intptr_t)(24000), (intptr_t)(-3200), (intptr_t)(9600), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(4000), (intptr_t)(20000), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(0));
LABEL_50:
          if ( v19 != 0 )
            F32(v19, 312) = 2000;
          goto LABEL_69;
        }
        if ( v18 > 20102 )
        {
          switch ( v18 )
          {
            case 20103:
            case 20104:
              W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(-28000), (intptr_t)(56000), (intptr_t)(-3200), (intptr_t)(6400), (intptr_t)(500), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(-24000), (intptr_t)(48000), (intptr_t)(0), (intptr_t)(5600), (intptr_t)(0));
              break;
            case 20105:
            case 20106:
            case 20110:
            case 20111:
            case 20113:
            case 20115:
              goto LABEL_68;
            case 20108:
            case 20109:
              goto LABEL_46;
            case 20112:
              goto LABEL_65;
            case 20114:
              goto LABEL_60;
            default:
              goto LABEL_69;
          }
          goto LABEL_69;
        }
        if ( v18 == 20102 )
          goto LABEL_68;
        if ( v18 <= 140 )
        {
          if ( v18 != 140 )
          {
            switch ( v18 )
            {
              case 47:
                v19 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(6000), (intptr_t)(48000), (intptr_t)(-9600), (intptr_t)(20000), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(25), (intptr_t)(15), (intptr_t)(6000), (intptr_t)(48000), (intptr_t)(-9600), (intptr_t)(20000), (intptr_t)(0));
                goto LABEL_50;
              case 48:
                W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(48000), (intptr_t)(-10000), (intptr_t)(30000), (intptr_t)(1000), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(4000), (intptr_t)(40000), (intptr_t)(-10000), (intptr_t)(28000), (intptr_t)(0));
                break;
              case 134:
              case 135:
              case 136:
              case 137:
              case 138:
                goto LABEL_68;
              case 139:
                goto LABEL_49;
              default:
                goto LABEL_69;
            }
            goto LABEL_69;
          }
LABEL_68:
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1000), (intptr_t)(0x3E8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          goto LABEL_69;
        }
        if ( v18 > 10121 )
        {
          if ( v18 != 20101 )
            goto LABEL_69;
LABEL_46:
          v21 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(6400), (intptr_t)(14000), (intptr_t)(-4800), (intptr_t)(14000), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(6400), (intptr_t)(14000), (intptr_t)(-4800), (intptr_t)(14000), (intptr_t)(0));
          if ( v21 != 0 )
          {
            W_ai_skill_boxes_add((intptr_t)(v21), (intptr_t)(20800), (intptr_t)(14000), (intptr_t)(6000), (intptr_t)(14000), (intptr_t)(20800), (intptr_t)(14000), (intptr_t)(6000), (intptr_t)(14000));
LABEL_67:
            F32(v21, 312) = 1500;
          }
        }
        else
        {
          switch ( v18 )
          {
            case 10121:
              goto LABEL_41;
            case 10113:
              goto LABEL_68;
            case 10114:
LABEL_41:
              v20 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(4800), (intptr_t)(20000), (intptr_t)(-8000), (intptr_t)(14400), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(4800), (intptr_t)(18000), (intptr_t)(-6400), (intptr_t)(12800), (intptr_t)(0));
              if ( v20 != 0 )
                F32(v20, 312) = 1500;
              break;
            default:
              break;
          }
        }
LABEL_69:
        v17 = ++v34;
        if ( v34 >= F16(c, 0xc754) )
          goto LABEL_70;
      }
      switch ( v18 )
      {
        case 20117:
          v26 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(6000), (intptr_t)(32000), (intptr_t)(-16000), (intptr_t)(32000), (intptr_t)(500), (intptr_t)(0x3E8u), (intptr_t)(25), (intptr_t)(15), (intptr_t)(8000), (intptr_t)(28000), (intptr_t)(-14000), (intptr_t)(28000), (intptr_t)(0));
          if ( v26 != 0 )
            F32(v26, 312) = 2500;
          goto LABEL_69;
        case 20118:
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(-28000), (intptr_t)(56000), (intptr_t)(-24000), (intptr_t)(48000), (intptr_t)(1000), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(-24000), (intptr_t)(48000), (intptr_t)(-22000), (intptr_t)(44000), (intptr_t)(0));
          goto LABEL_69;
        case 20119:
        case 20120:
        case 20124:
        case 20125:
        case 20132:
        case 20133:
        case 20134:
        case 20135:
        case 20136:
          goto LABEL_68;
        case 20122:
        case 20130:
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(8000), (intptr_t)(40000), (intptr_t)(-6000), (intptr_t)(12000), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(8000), (intptr_t)(32000), (intptr_t)(-4800), (intptr_t)(8000), (intptr_t)(0));
          goto LABEL_69;
        case 20123:
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(40000), (intptr_t)(-6400), (intptr_t)(12800), (intptr_t)(500), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(4000), (intptr_t)(20000), (intptr_t)(-4800), (intptr_t)(9600), (intptr_t)(0));
          goto LABEL_69;
        case 20126:
          v22 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(-20000), (intptr_t)(40000), (intptr_t)(-14000), (intptr_t)(40000), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(-20000), (intptr_t)(40000), (intptr_t)(-14000), (intptr_t)(40000), (intptr_t)(0));
          if ( v22 != 0 )
            F32(v22, 312) = 500;
          goto LABEL_69;
        case 20127:
          v23 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(51200), (intptr_t)(-8000), (intptr_t)(19200), (intptr_t)(500), (intptr_t)(0x3E8u), (intptr_t)(25), (intptr_t)(15), (intptr_t)(6000), (intptr_t)(51200), (intptr_t)(-6400), (intptr_t)(16000), (intptr_t)(0));
          if ( v23 != 0 )
            F32(v23, 312) = 3000;
          goto LABEL_69;
        case 20128:
LABEL_65:
          v21 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(4000), (intptr_t)(18000), (intptr_t)(-3200), (intptr_t)(9600), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(4000), (intptr_t)(12800), (intptr_t)(0), (intptr_t)(6400), (intptr_t)(0));
          if ( v21 == 0 )
            goto LABEL_69;
          W_ai_skill_boxes_add((intptr_t)(v21), (intptr_t)(-22000), (intptr_t)(18000), (intptr_t)(-3200), (intptr_t)(9600), (intptr_t)(-16800), (intptr_t)(12800), (intptr_t)(0), (intptr_t)(6400));
          break;
        case 20129:
LABEL_60:
          v19 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(12800), (intptr_t)(48000), (intptr_t)(-6400), (intptr_t)(16000), (intptr_t)(500), (intptr_t)(0x3E8u), (intptr_t)(25), (intptr_t)(15), (intptr_t)(16000), (intptr_t)(40000), (intptr_t)(-3200), (intptr_t)(9600), (intptr_t)(0));
          goto LABEL_50;
        case 20131:
          v24 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(3), (intptr_t)(4000), (intptr_t)(22000), (intptr_t)(-16000), (intptr_t)(20000), (intptr_t)(200), (intptr_t)(0x3E8u), (intptr_t)(5), (intptr_t)(10), (intptr_t)(4000), (intptr_t)(16000), (intptr_t)(-10800), (intptr_t)(12800), (intptr_t)(0));
          v25 = v24;
          if ( v24 != 0 )
          {
            W_ai_skill_boxes_add((intptr_t)(v24), (intptr_t)(20000), (intptr_t)(16000), (intptr_t)(-17000), (intptr_t)(16000), (intptr_t)(20000), (intptr_t)(12800), (intptr_t)(-16000), (intptr_t)(12800));
            F32(v25, 312) = 3000;
          }
          goto LABEL_69;
        default:
          goto LABEL_69;
      }
      goto LABEL_67;
    }
LABEL_70:
    F32(c, 0x15a5c) = 7;
    F32(c, 0x15a60) = 1;
    F32(c, 0x15a64) = -1600;
    F32(c, 0x15a68) = -4000;
    F16(c, 0x15a84) = 500;
    F16(c, 0x15a86) = 900;
  }
  v27 = F32(c, 0x1d4);
  if ( v27 <= 0xC7B2 )
  {
    if ( v27 != 51122 )
    {
      switch ( v27 )
      {
        case 0xC79Cu:
          goto LABEL_80;
        case 0xC7A6u:
        case 0xC7A7u:
          goto LABEL_77;
        case 0xC7B0u:
        case 0xC7B1u:
          W_char_skill_def((intptr_t)(c), (intptr_t)(53), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(53), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(11000), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(53), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(11000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          W_char_ai_skill_add((intptr_t)(c), (intptr_t)(53), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(12000), (intptr_t)(-6400), (intptr_t)(9600), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          W_char_approach_add((intptr_t)(c), (intptr_t)(13600), (intptr_t)(9600), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(15000), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
          W_char_approach_add((intptr_t)(c), (intptr_t)(6400), (intptr_t)(8000), (intptr_t)(-8000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
          W_char_approach_add((intptr_t)(c), (intptr_t)(3200), (intptr_t)(8000), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(1600), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x12Cu));
          F32(c, 0x14898) = 1;
          W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(53), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(55), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(53), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          break;
        default:
          goto LABEL_81;
      }
      goto LABEL_81;
    }
LABEL_80:
    W_char_skill_def((intptr_t)(c), (intptr_t)(52), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
    W_char_skill_def((intptr_t)(c), (intptr_t)(56), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(15728640), (intptr_t)(1300), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(52), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(11000), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(52), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(11000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(52), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(12000), (intptr_t)(-6400), (intptr_t)(9600), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(56), (intptr_t)(0), (intptr_t)(1), (intptr_t)(-3200), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(12800), (intptr_t)(5), (intptr_t)(0x320u), (intptr_t)(4), (intptr_t)(10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_approach_add((intptr_t)(c), (intptr_t)(13600), (intptr_t)(9600), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(15000), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
    W_char_approach_add((intptr_t)(c), (intptr_t)(6400), (intptr_t)(8000), (intptr_t)(-8000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x3E8u));
    W_char_approach_add((intptr_t)(c), (intptr_t)(3200), (intptr_t)(8000), (intptr_t)(-2000), (intptr_t)(13600), (intptr_t)(0), (intptr_t)(1600), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(8000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(0x12Cu));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(56), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(52), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(55), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(10), (intptr_t)(300), (intptr_t)(0), (intptr_t)(210), (intptr_t)(8u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_combo_add((intptr_t)(c), (intptr_t)(1), (intptr_t)(52), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(400), (intptr_t)(0), (intptr_t)(0), (intptr_t)(8u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    goto LABEL_81;
  }
  v28 = v27 - 100172;
  if ( v28 == 0 )
    goto LABEL_80;
  if ( v28 == 1 )
  {
LABEL_77:
    W_char_skill_def((intptr_t)(c), (intptr_t)(54), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(15728640), (intptr_t)(1000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a9b6c)));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(54), (intptr_t)(0), (intptr_t)(4), (intptr_t)(6000), (intptr_t)(26400), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(100), (intptr_t)(0x3E8u), (intptr_t)(20), (intptr_t)(10), (intptr_t)(6000), (intptr_t)(22400), (intptr_t)(1600), (intptr_t)(12800), (intptr_t)(0));
    v29 = W_char_ai_skill_add((intptr_t)(c), (intptr_t)(54), (intptr_t)(1), (intptr_t)(4), (intptr_t)(4000), (intptr_t)(16000), (intptr_t)(-12000), (intptr_t)(10000), (intptr_t)(60), (intptr_t)(0x384u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(6400), (intptr_t)(12000), (intptr_t)(-11200), (intptr_t)(6000), (intptr_t)(0));
    if ( v29 != 0 )
      W_ai_skill_boxes_add((intptr_t)(v29), (intptr_t)(40000), (intptr_t)(12800), (intptr_t)(-1600), (intptr_t)(16000), (intptr_t)(40800), (intptr_t)(4800), (intptr_t)(0), (intptr_t)(12800));
    W_char_ai_skill_add((intptr_t)(c), (intptr_t)(54), (intptr_t)(7), (intptr_t)(2), (intptr_t)(9600), (intptr_t)(12800), (intptr_t)(4800), (intptr_t)(12800), (intptr_t)(60), (intptr_t)(0x320u), (intptr_t)(10), (intptr_t)(10), (intptr_t)(9600), (intptr_t)(8000), (intptr_t)(6400), (intptr_t)(9600), (intptr_t)(0));
    W_char_approach_add((intptr_t)(c), (intptr_t)(16000), (intptr_t)(12800), (intptr_t)(-3200), (intptr_t)(16000), (intptr_t)(0), (intptr_t)(16000), (intptr_t)(6000), (intptr_t)(12000), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(9600), (intptr_t)(0x3E8u));
  }
LABEL_81:
  v30 = F32(c, 0x1d4);
  if ( v30 > 0xC7B1 )
  {
    v32 = v30 - 51122;
    if ( v32 != 0 )
    {
      v33 = v32 - 49050;
      if ( v33 != 0 )
      {
        result = v33 - 1;
        if ( result != 0 )
          return result;
        W_char_follow_set((intptr_t)(c), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
LABEL_92:
        W_char_pace_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(20000), (intptr_t)(6000), (intptr_t)(500));
        return W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(12800));
      }
      W_char_follow_set((intptr_t)(c), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
    else
    {
LABEL_94:
      W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
    W_char_pace_set((intptr_t)(c), (intptr_t)(4), (intptr_t)(2000), (intptr_t)(2000), (intptr_t)(200));
    return W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(12800));
  }
  if ( v30 == 51121 )
  {
    W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_char_pace_set((intptr_t)(c), (intptr_t)(2), (intptr_t)(10000), (intptr_t)(8000), (intptr_t)(200));
    return W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(12800));
  }
  result = v30 - 51100;
  switch ( result )
  {
    case 0:
      goto LABEL_94;
    case 10:
      W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      W_char_pace_set((intptr_t)(c), (intptr_t)(2), (intptr_t)(20000), (intptr_t)(6000), (intptr_t)(500));
      F32(c, 0xc880) = 1;
      F32(c, 0xc884) = 12800;
      F32(c, 0xc888) = 800;
      F32(c, 0xc88c) = 12800;
      result = 12800;
      break;
    case 11:
      W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      goto LABEL_92;
    case 20:
      W_char_follow_set((intptr_t)(c), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      W_char_pace_set((intptr_t)(c), (intptr_t)(2), (intptr_t)(2000), (intptr_t)(2000), (intptr_t)(200));
      return W_char_c880_set((intptr_t)(c), (intptr_t)(12800), (intptr_t)(800), (intptr_t)(12800));
    default:
      return result;
  }
  return result;
}
/* unported callees: none */
