/* ai_skill.c — the character AI's held-attack SKILL PICK 0x479f90 (char_ai 0x478ba0 calls it while the attack key
 * is held): mode 1 (+0x14898; Arche without a weapon: 2) takes the skill bound to the held direction off the member's
 * skill slots, mode 2 the first usable shortcut skill (kind +0xbc: 1 held / 2 tap / 3 the HP-ratio gate); then the
 * pose check 0x446640 + usable 0x407320, the cast target 0x47a970, and the pending-skill fields +0x14868.. .  See
 * ai_skill.h.  A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish. */
#include "ai_skill.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))

#include "behave_helpers.h"
#include "char_integrate.h"
#include "party_member.h"
#include "behave_helpers.h"
#include "char_integrate.h"
#include "party_member.h"
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

static inline intptr_t W_beh_47a970(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)beh_47a970((wl_char *)a0, (int32_t)a1, (int32_t *)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_char_skill_usable(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_skill_usable((wl_char *)a0, (const uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_skill_pose_ok(intptr_t a0, intptr_t a1, ...) { return (intptr_t)skill_pose_ok((const wl_skill *)a0, (const wl_body *)a1); }

int32_t ai_479f90(wl_char *self, int32_t a2, int32_t a3);

/* 0x479f90: int __thiscall sub_479F90(cls_4022A0 *this, int a2, int a3) */
int32_t ai_479f90(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    uint16_t v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    char * v23 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *const v25__f = (int16_t *)(frame_ + 0x0);   /* ebp-0x8 */
    uint16_t *const v26__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x4 */

  v4 = F32(c, 0x14898);
  if ( F32(c, 0x1d4) == 50010 && F16(F32(c, 0x750), 1126) == 0 )
    v4 = 2;
  v5 = v4 - 1;
  if ( v5 == 0 )
  {
    v12 = F32(c, 0x750);
    switch ( F32(v12, 320) )
    {
      case 1:
        v13 = 0;
        break;
      case 2:
        v13 = 1;
        break;
      case 3:
        v13 = 2;
        break;
      default:
        v13 = a3;
        break;
    }
    v14 = 0;
    (*v25__f) = F16(v12 + 2 * v13, 312);
    (*v26__f) = F16(v12, 324);
    if ( (*v26__f) == 0 )
      goto LABEL_42;
    while ( 1 )
    {
      v15 = v12 + 12 * v14;
      if ( F32(v15, 332) == F32(v12, 320) && F16((v12 + 4 * (3 * v14 + 84)), 0) == (*v25__f) )
      {
        v16 = 0;
        v17 = F32(v15, 328);
        v18 = 10101;
        while ( v18 != v17 )
        {
          v18 = ((int32_t *)D(0x6826c0))[192 * ++v16];
          if ( v18 == 0 )
            goto LABEL_41;
        }
        v19 = &((int32_t *)D(0x6826c0))[192 * v16];
        if ( v19 != 0 && v19[42] != 0 )
          break;
      }
LABEL_41:
      if ( ++v14 >= (*v26__f) )
        goto LABEL_42;
    }
    if ( v12 + 12 * v14 == -328 )
LABEL_42:
      SETLO(v10, 0);
    else
      v10 = F32(v12 + 12 * v14, 328);
    v11 = 0;
    goto LABEL_44;
  }
  if ( v5 != 1 )
    return 0;
  v7 = 0;
  if ( F16(c, 0x1489c) == 0 )
    return 0;
  while ( 1 )
  {
    v8 = v7 << 6;
    if ( *(_DWORD *)&GAP(c, 0x1489e, v8 + 14) == 3 )
    {
      v9 = *(int **)&GAP(c, 0x1489e, v8 + 62);
      if ( v9 == 0 || W_char_skill_usable((intptr_t)(c), (intptr_t)((int)v9), (intptr_t)(0)) == 0 || W_skill_pose_ok((intptr_t)((int)v9), (intptr_t)((uint16_t *)a2)) == 0 )
        goto LABEL_21;
      if ( v9[47] != 1 )
        break;
      if ( a3 != 0 )
        goto LABEL_23;
    }
LABEL_21:
    if ( ++v7 >= F16(c, 0x1489c) )
      return 0;
  }
  if ( v9[47] == 2 )
  {
    if ( a3 == 0 )
      goto LABEL_23;
    goto LABEL_21;
  }
  if ( v9[47] == 3 && (a3 == 0 || F32(a2, 40) < 900 * F32(c, 0x15990) / 1000) )
    goto LABEL_21;
LABEL_23:
  v10 = *v9;
  v11 = 1;
  if ( *v9 == 0 )
    return 0;
LABEL_44:
  v20 = F16(c, 0xc754);
  v21 = 0;
  if ( v20 != 0 )
  {
    while ( *(_DWORD *)&GAP(c, 0x754, 768 * v21) != (uint16_t)v10 )
    {
      if ( ++v21 >= v20 )
        return 0;
    }
    v22 = 768 * v21;
    v23 = &GAP(c, 0x754, v22);
    if ( ((uint8_t *)c + v22) != -1876 )
    {
      if ( v11 == 0 && *(_DWORD *)&GAP(c, 0x754, v22 + 168) == 0 )
        return 0;
      if ( W_char_skill_usable((intptr_t)(c), (intptr_t)((int)&GAP(c, 0x754, v22)), (intptr_t)(0)) == 0 || W_skill_pose_ok((intptr_t)((int)v23), (intptr_t)((uint16_t *)a2)) == 0 )
        return 0;
      if ( F32(v23, 4 * 47) == 1 )
      {
        if ( a3 == 0 )
          return 0;
        goto LABEL_62;
      }
      if ( F32(v23, 4 * 47) == 2 )
      {
        if ( a3 != 0 )
          return 0;
LABEL_62:
        v24 = W_beh_47a970((intptr_t)(c), (intptr_t)((int)v23), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v24 == 0 )
          v24 = c;
        F16(c, 0x14868) = v10;
        F16(c, 0x14870) = 210;
        F16(c, 0x1486a) = 0;
        F32(c, 0x1486c) = (int)v24;
        return 1;
      }
      if ( F32(v23, 4 * 47) != 3 || a3 != 0 && F32(a2, 40) >= 900 * F32(c, 0x15990) / 1000 )
        goto LABEL_62;
      return 0;
    }
  }
  return 0;
}
/* unported callees: none */
