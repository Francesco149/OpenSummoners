/* char_special.c — the integrator's SPECIAL POSES (FUN_00442a70 char_integrate, cases 0x75..0x7a / 0x81 of its pose
 * switch).  See char_special.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals) of 0x442a70, then CARVED by hand: the decompile's
 * pose switch at LABEL_51 keeps these seven cases (+ the state switch IDA laid out after it: case 0x77's `break`
 * lands there) as one closed region — entered only at its case labels, left only through `goto LABEL_838` (the
 * shared tail, char_integrate's `tail:`).  It reads the prologue's results only through the body (v7 = out), the
 * command (a2), the probe / part arguments and the stack slot v220 (0 at the switch); a3 / a4 are written before
 * any read in every case (the argument slots reused as scratch); it never touches dt (v221) or held (v224).
 * Hand fixes: the three 0x445ce0 calls take ecx = edi = this (0x442f89 / 0x4439dc / 0x444473).
 * Not a whole function: tools/ida/decomp2c_regen.py cannot re-splice it — re-carve by hand. */
#include "char_special.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }

#include "actor_update.h"
#include "arche_pose.h"
#include "arche_sword.h"
#include "char_handlers.h"
#include "char_handlers2.h"
#include "char_integrate.h"
#include "char_physics.h"
#include "char_state.h"
#include "engine_rest.h"
#include "entity_life.h"
#include "entity_move.h"
#include "particle_spawn.h"
#include "party_member.h"
#include "sana_skills.h"
#include "shots.h"
#include "skill_cast.h"
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

static inline intptr_t W_arche_pose(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)arche_pose((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4); }
static inline intptr_t W_arche_sword(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)arche_sword((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (const wl_cmd *)a6); }
static inline intptr_t W_body_action_reset(intptr_t a0, ...) { body_action_reset((wl_body *)a0); return 0; }
static inline intptr_t W_body_action_start(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)body_action_start((wl_body *)a0, (int16_t)a1, (int32_t)a2); }
static inline intptr_t W_body_can_act(intptr_t a0, ...) { return (intptr_t)body_can_act((const wl_body *)a0); }
static inline intptr_t W_body_clip_step(intptr_t a0, ...) { body_clip_step((wl_body *)a0); return 0; }
static inline intptr_t W_body_facing_sign(intptr_t a0, intptr_t a1, ...) { return (intptr_t)body_facing_sign((const wl_body *)a0, (int32_t)a1); }
static inline intptr_t W_body_region_flags(intptr_t a0, ...) { return (intptr_t)body_region_flags((wl_body *)a0); }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1, ...) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_body_set_state(intptr_t a0, intptr_t a1, ...) { body_set_state((wl_body *)a0, (int16_t)a1); return 0; }
static inline intptr_t W_body_special_clip(intptr_t a0, intptr_t a1, ...) { return (intptr_t)body_special_clip((wl_body *)a0, (int32_t)a1); }
static inline intptr_t W_char_anim_select(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { char_anim_select((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_carry(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_carry((wl_char *)a0, (wl_char *)a1); }
static inline intptr_t W_char_copy_rects(intptr_t a0, intptr_t a1, ...) { char_copy_rects((const wl_char *)a0, (wl_body *)a1); return 0; }
static inline intptr_t W_char_float_cap(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_float_cap((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_char_idle_voice(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_idle_voice((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (uint16_t)a4); return 0; }
static inline intptr_t W_char_physics_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)char_physics_step((wl_char *)a0, (const wl_cmd *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_char_release(intptr_t a0, ...) { char_release((wl_char *)a0); return 0; }
static inline intptr_t W_char_skill_usable(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_skill_usable((wl_char *)a0, (const uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_el_44bf60(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { el_44bf60((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_el_4675c0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)el_4675c0((wl_char *)a0, (int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_el_467750(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)el_467750((wl_char *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_entity_move_horizontal(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_move_horizontal((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (uint16_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_grid_floor_below(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_floor_below((const wl_body *)a0, (int32_t *)a1); }
static inline intptr_t W_ih_445e50(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ih_445e50((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t *)a4); }
static inline intptr_t W_ih_4466b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { ih_4466b0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_ih_446900(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_446900((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t *)a6, (int32_t)a7); }
static inline intptr_t W_ih_4484b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_4484b0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ih_448dc0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_448dc0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ih_449810(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { ih_449810((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t *)a7); return 0; }
static inline intptr_t W_ih_44a200(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_44a200((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_44a740(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_44a740((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t *)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_44b160(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_44b160((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t *)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_44bc10(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { ih_44bc10((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4); return 0; }
static inline intptr_t W_ih_44c250(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ih_44c250((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4); }
static inline intptr_t W_ih_44d2e0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)ih_44d2e0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t *)a5); }
static inline intptr_t W_ih_44d8f0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_44d8f0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t *)a7); }
static inline intptr_t W_ih_44e8d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_44e8d0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_44f690(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)ih_44f690((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t *)a8); }
static inline intptr_t W_ih_450810(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_450810((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_4512d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_4512d0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_4516e0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_4516e0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ih_452020(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_452020((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ih_452730(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)ih_452730((wl_char *)a0, (int32_t)a1, (FILE *)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_ih_4539b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ih_4539b0((wl_char *)a0, (int32_t)a1, (FILE *)a2, (int32_t)a3, (int32_t *)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ih_455d80(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ih_455d80((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t)a4, (int32_t)a5, (int32_t *)a6, (int32_t)a7); }
static inline intptr_t W_ih_456d30(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)ih_456d30((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_ih_467260(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)ih_467260((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_sana_skills(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { sana_skills((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t *)a3, (const wl_cmd *)a4, (int32_t)a5); return 0; }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_shot_445ce0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)shot_445ce0((wl_char *)a0, (int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_skill_cast(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)skill_cast((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t *)a6, (uint16_t *)a7, (int32_t)a8); }
static inline intptr_t W_skill_pose_ok(intptr_t a0, intptr_t a1, ...) { return (intptr_t)skill_pose_ok((const wl_skill *)a0, (const wl_body *)a1); }
static inline intptr_t W_speed_ramp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)speed_ramp((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_sys_by_mode(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sys_by_mode((const void *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_voice_cue(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)voice_cue((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_voice_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)voice_place((const wl_char *)a0, (int32_t *)a1, (int32_t *)a2, (int32_t *)a3); }
static inline intptr_t W_voice_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)voice_play((wl_voiceset *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }

/* the pose switch's special cases on `out` (char_integrate's prologue already ran); returns where retail's
 * LABEL_838 continues: char_integrate's `tail:` */
void integrate_special(wl_char *self, wl_cmd *cmd, wl_body *out, int32_t probe, int32_t part)
{
    intptr_t c = (intptr_t)self;
    int32_t a2 = (int32_t)(intptr_t)cmd;
    int32_t a3 = 0;
    int32_t a4 = (int32_t)(intptr_t)out;
    int32_t errno_decorate = probe;
    int32_t a6 = part;
    intptr_t v7 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    int16_t v31 = 0;
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
    char v47 = 0;
    intptr_t v48 = 0;
    uint16_t v49 = 0;
    intptr_t v50 = 0;
    int v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    int v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    uint16_t v57 = 0;
    intptr_t v58 = 0;
    uint16_t v59 = 0;
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
    int v70 = 0;
    uint16_t v71 = 0;
    uint16_t v72 = 0;
    intptr_t v73 = 0;
    int16_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    uint16_t v78 = 0;
    uint16_t v79 = 0;
    intptr_t v80 = 0;
    intptr_t v81 = 0;
    uint16_t v82 = 0;
    int16_t v83 = 0;
    intptr_t v84 = 0;
    intptr_t v85 = 0;
    intptr_t v86 = 0;
    intptr_t v87 = 0;
    intptr_t v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    intptr_t v91 = 0;
    intptr_t v92 = 0;
    intptr_t v93 = 0;
    intptr_t v94 = 0;
    intptr_t v95 = 0;
    intptr_t v96 = 0;
    intptr_t v97 = 0;
    intptr_t v98 = 0;
    char v99 = 0;
    intptr_t v100 = 0;
    uint16_t v101 = 0;
    intptr_t v102 = 0;
    int v103 = 0;
    intptr_t v104 = 0;
    intptr_t v105 = 0;
    int v106 = 0;
    intptr_t v107 = 0;
    intptr_t v108 = 0;
    uint16_t v109 = 0;
    intptr_t v110 = 0;
    uint16_t v111 = 0;
    intptr_t v112 = 0;
    intptr_t v113 = 0;
    intptr_t v114 = 0;
    intptr_t v115 = 0;
    intptr_t v116 = 0;
    intptr_t v117 = 0;
    intptr_t v118 = 0;
    intptr_t v119 = 0;
    intptr_t v120 = 0;
    intptr_t v121 = 0;
    int v122 = 0;
    uint16_t v123 = 0;
    uint16_t v124 = 0;
    intptr_t v125 = 0;
    intptr_t v126 = 0;
    intptr_t v127 = 0;
    intptr_t v128 = 0;
    intptr_t v132 = 0;
    intptr_t v133 = 0;
    intptr_t v134 = 0;
    intptr_t v135 = 0;
    intptr_t v136 = 0;
    intptr_t v137 = 0;
    intptr_t v138 = 0;
    intptr_t v139 = 0;
    intptr_t v140 = 0;
    intptr_t v141 = 0;
    intptr_t v142 = 0;
    intptr_t v143 = 0;
    intptr_t v144 = 0;
    intptr_t v145 = 0;
    intptr_t v146 = 0;
    intptr_t v147 = 0;
    intptr_t v148 = 0;
    intptr_t v149 = 0;
    intptr_t v150 = 0;
    char v151 = 0;
    intptr_t v152 = 0;
    uint16_t v153 = 0;
    intptr_t v154 = 0;
    int v155 = 0;
    intptr_t v156 = 0;
    intptr_t v157 = 0;
    int v158 = 0;
    intptr_t v159 = 0;
    intptr_t v160 = 0;
    uint16_t v161 = 0;
    intptr_t v162 = 0;
    uint16_t v163 = 0;
    intptr_t v164 = 0;
    intptr_t v165 = 0;
    intptr_t v166 = 0;
    intptr_t v167 = 0;
    intptr_t v168 = 0;
    intptr_t v169 = 0;
    intptr_t v170 = 0;
    intptr_t v171 = 0;
    intptr_t v172 = 0;
    intptr_t v173 = 0;
    int v174 = 0;
    uint16_t v175 = 0;
    uint16_t v176 = 0;
    intptr_t v177 = 0;
    intptr_t v178 = 0;
    intptr_t v179 = 0;
    intptr_t v180 = 0;
    intptr_t v181 = 0;
    uint16_t v182 = 0;
    int16_t v183 = 0;
    intptr_t v184 = 0;
    _Alignas(8) uint8_t frame_[0x60 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v210__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x60 */
    intptr_t *const v214__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x50 */
    intptr_t *const v213__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x50 */
    intptr_t *const v216__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x48 */
    intptr_t *const v215__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x48 */
    uint16_t *const v218__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x44 */
    uint16_t *const v217__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x44 */
    intptr_t *const v220__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x2c */
    intptr_t *const v222__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x24 */
    intptr_t *const v223__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x20 */
    intptr_t *const v225__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x18 */
    intptr_t *const v226__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x14 */
    intptr_t *const v227__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x10 */
    intptr_t *const v228__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0xc */
    intptr_t *const v229__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x8 */
    intptr_t *const v230__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x4 */

  v7 = a4;
  v29 = errno_decorate;
  (*v220__f) = 0;
  switch ( F16(v7, 72) )
  {
    case 0x75:
      (*v220__f) = 3;
      v132 = F16(v7, 88);
      F8(v7, 124) = 1;
      switch ( v132 )
      {
        case 0:
          if ( v29 == 0
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place((intptr_t)(c), (intptr_t)(&(*v230__f)), (intptr_t)(&a4), (intptr_t)(&a3)) != 0 )
          {
            W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x10u), (intptr_t)(0), (intptr_t)((*v230__f)), (intptr_t)(a4), (intptr_t)(a3), (intptr_t)(0));
          }
          v133 = F32(c, 0x15924);
          if ( v133 != 0 && F32(v7, 108) != v133 )
          {
            F32(v7, 108) = v133;
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          ++F16(v7, 88);
          goto LABEL_533;
        case 1:
LABEL_533:
          ++F32(v7, 92);
          if ( (F32(c, 0x204) & 0x1F0000) == 0 )
          {
            (*v210__f) = 800 * rnd() / 0x8000 - 1600;
            v134 = rnd();
            W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100111), (intptr_t)((_DWORD *)v7), (intptr_t)(1600 * v134 / 0x8000 - 800), (intptr_t)((*v210__f)), (intptr_t)(99), (intptr_t)(3), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          }
          goto LABEL_545;
        case 10:
          v135 = F32(c, 0x15924);
          if ( v135 != 0 && F16(v135, 66) > 1u )
            goto LABEL_545;
          v136 = (F32(v7, 92) + 1) % 8;
          F32(v7, 92) = v136;
          v51 = v136 < 4;
          v137 = F32(v7, 64);
          if ( !v51 )
            goto LABEL_543;
          v138 = v137 - 2;
          break;
        case 20:
          v139 = F32(c, 0x15924);
          if ( v139 != 0 && F16(v139, 66) > 1u )
            goto LABEL_545;
          v140 = (F32(v7, 92) + 1) % 4;
          F32(v7, 92) = v140;
          v51 = v140 < 2;
          v137 = F32(v7, 64);
          if ( v51 )
            v138 = v137 - 2;
          else
LABEL_543:
            v138 = v137 + 2;
          break;
        default:
          goto LABEL_545;
      }
      F32(v7, 64) = v138;
LABEL_545:
      v141 = F32(c, 0x1594c);
      if ( v141 <= 0 )
        goto LABEL_550;
      if ( v141 > 2 )
      {
        if ( v141 == 3 )
        {
          if ( W_shot_445ce0((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a6)) )
          {
            F32(v7, 40) = 0;
            F32(v7, 24) = 0;
            goto LABEL_554;
          }
        }
        else
        {
LABEL_550:
          if ( F32(c, 0x15950) > 56000 )
            goto LABEL_554;
        }
      }
      v142 = F32(c, 0x15950);
      if ( v142 <= 64000 )
        v142 = 64000;
      F32(v7, 28) = v142;
LABEL_554:
      v143 = F32(v7, 36);
      v144 = F32(c, 0x15950);
      v145 = F32(c, 0x15954);
      (*v229__f) = (*v220__f);
      v146 = F32(v7, 648);
      a3 = v143;
      (*v223__f) = v144;
      a4 = v145;
      (*v230__f) = 0;
      (*v228__f) = 0;
      if ( v146 != 0 )
        (*v223__f) = v144 / 3;
      v147 = F32(c, 0x1594c);
      if ( v147 != 0 )
      {
        if ( v147 <= 0 || v147 > 3 )
        {
          v148 = a2;
          v149 = a2;
        }
        else
        {
          v148 = W_body_can_act((intptr_t)(v7));
          (*v227__f) = 0;
          v149 = v148;
          (*v222__f) = v148;
          F8(v7, 32) = 1;
          (*v230__f) = 1;
        }
      }
      else
      {
        v150 = W_body_can_act((intptr_t)(v7));
        v148 = v150;
        if ( v150 == 0 || ((*v222__f) = 1, v143 == 0) )
          (*v222__f) = 0;
        v149 = v150 != 0 && v143 != 0;
        if ( errno_decorate == 0 && a3 != 0 )
          F16(c, 0x158bc) = 0;
        if ( F16(c, 0x158bc) < F16(c, 0xc7e8) )
          (*v222__f) = 1;
        (*v227__f) = 1;
        if ( F32(a2, 8) == 9 )
        {
          F8(v7, 32) = 8;
        }
        else
        {
          v151 = F8(v7, 32);
          if ( v151 != 0 )
            F8(v7, 32) = v151 - 1;
        }
      }
      if ( F32(c, 0x15964) > 0 && F32(c, 0x15964) <= W_grid_floor_below((intptr_t)((_DWORD *)v7), (intptr_t)(0)) )
      {
        if ( (*v223__f) < 50000 )
          (*v223__f) = 50000;
        if ( a4 < 2000 )
          a4 = 2000;
        (*v228__f) = 1;
      }
      v152 = F32(v7, 28);
      if ( v152 != 0 )
      {
        (*v223__f) = F32(v7, 28);
        a4 = v152 / 10;
      }
      switch ( (*v229__f) )
      {
        case 0:
          goto LABEL_591;
        case 1:
          if ( F16(v7, 56) != 3 )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          goto LABEL_589;
        case 2:
        case 3:
          v149 = 0;
          W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          goto LABEL_590;
        default:
LABEL_589:
          v149 = 0;
LABEL_590:
          v148 = 0;
          (*v222__f) = 0;
LABEL_591:
          if ( F32(a2, 12) == 10 )
          {
            if ( v149 != 0 && F32(c, 0x159d4) != 0 )
            {
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(2));
            }
            else if ( (*v230__f) != 0 )
            {
              if ( a3 != 0 )
                goto LABEL_602;
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(6));
              goto LABEL_600;
            }
          }
          if ( a3 == 0 )
          {
LABEL_600:
            if ( F32(c, 0x15a24) == 0 )
              v148 = 0;
          }
LABEL_602:
          if ( F32(a2, 12) == 11 && v148 != 0 && F32(c, 0x15a10) != 0 )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(5));
          if ( F32(c, 0x15a24) != 0 && F16(v7, 56) == 5 )
            (*v227__f) = 0;
          if ( F32(a2, 8) == 7
            && ((*v222__f) != 0 || F16(v7, 56) == 5 && a3 != 0)
            && F32(c, 0x15998) != 0
            && (*v228__f) == 0 )
          {
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(3));
            if ( errno_decorate == 0 )
            {
              v153 = F16(c, 0x158bc);
              if ( v153 < F16(c, 0xc7e8) && a3 == 0 )
                F16(c, 0x158bc) = v153 + 1;
            }
          }
          break;
      }
      switch ( F16(v7, 56) )
      {
        case 2:
          v159 = F16(v7, 58);
          if ( (uint16_t)v159 == 0 )
          {
            v158 = ++F16(v7, 60) <= 4u;
            goto LABEL_647;
          }
          v156 = v159 - 1;
          if ( v156 != 0 )
            goto LABEL_639;
          ++F16(v7, 60);
          v160 = a2;
          v161 = F16(v7, 60);
          F32(v7, 20) = F32(c, 0x159d8);
          if ( F32(v160, 12) != 10 && v161 > 4u )
          {
            F16(v7, 58) = 2;
            F16(v7, 60) = 0;
          }
          goto LABEL_656;
        case 3:
          switch ( F16(v7, 58) )
          {
            case 0:
              if ( ++F16(v7, 60) > 4u )
              {
                v154 = F32(c, 0x1599c);
                F16(v7, 58) = 1;
                F32(v7, 24) = v154;
                F16(v7, 60) = 0;
              }
              goto LABEL_656;
            case 1:
              F16(v7, 58) = 2;
              goto LABEL_627;
            case 2:
              if ( F32(v7, 24) > -20000 )
                F16(v7, 58) = 3;
              goto LABEL_627;
            case 3:
              ++F16(v7, 60);
LABEL_627:
              if ( F32(v7, 24) >= 0 )
              {
                v155 = F16(v7, 60) <= 6u;
                goto LABEL_641;
              }
              if ( F32(a2, 8) != 0 )
                a4 = F32(c, 0x159a0);
              else
                a4 = F32(c, 0x159a4);
              if ( (*v228__f) == 1 )
                a4 = F32(c, 0x159a4);
              break;
            default:
              goto LABEL_656;
          }
          goto LABEL_656;
        case 4:
          v164 = F16(v7, 58);
          if ( (uint16_t)v164 != 0 )
          {
            v156 = v164 - 1;
            if ( v156 != 0 )
            {
LABEL_639:
              if ( v156 == 1 )
              {
                v155 = ++F16(v7, 60) <= 4u;
LABEL_641:
                if ( !v155 )
LABEL_642:
                  W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
              }
            }
            else
            {
              F16(v7, 58) = 2;
            }
          }
          else
          {
            if ( a3 == 0 )
              goto LABEL_659;
            F16(v7, 58) = 1;
          }
LABEL_656:
          if ( a3 != 0 && F32(v7, 24) >= 0 )
          {
            F32(v7, 24) = 0;
            goto LABEL_674;
          }
LABEL_659:
          v165 = F32(c, 0x1594c);
          if ( v165 >= 0 )
          {
            if ( v165 <= 2 )
            {
              v166 = v7 + 8;
              SETLO(v166, F8(v7, 32));
              if ( W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(F32(v7, 24) / 100), (intptr_t)(v166), (intptr_t)(a6), (intptr_t)(v7 + 8), (intptr_t)((_DWORD *)(v7 + 644))) == 0 )
              {
                if ( F32(v7, 24) > 0 )
                  a3 = 1;
                F32(v7, 24) = 0;
              }
            }
            else if ( v165 == 3 )
            {
              W_el_467750((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a6));
            }
          }
          if ( (*v227__f) != 0 && (F16(v7, 56) < 3u || F16(v7, 56) > 4u) )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(4));
          v167 = F32(v7, 24) + a4;
          if ( (*v223__f) < v167 )
            v167 = (*v223__f);
          F32(v7, 24) = v167;
LABEL_674:
          v168 = F32(c, 0x1596c);
          v169 = F32(c, 0x15978);
          v170 = F16(v7, 56);
          (*v226__f) = 0;
          (*v222__f) = 0;
          (*v227__f) = 0;
          v171 = 1;
          a4 = v168;
          v172 = F32(c, 0x15970);
          (*v223__f) = v169;
          (*v225__f) = v169;
          (*v228__f) = 1;
          if ( (uint16_t)v170 == 2 || (uint16_t)v170 == 5 )
            v171 = 0;
          switch ( (*v229__f) )
          {
            case 1:
              goto LABEL_712;
            case 2:
              if ( a3 != 0 )
                goto LABEL_712;
              (*v228__f) = 0;
              break;
            case 3:
              goto LABEL_712;
            default:
              break;
          }
          if ( v171 != 0 )
          {
            if ( a3 != 0 || F32(c, 0x159a8) == 0 )
            {
              v173 = F32(a2, 0);
              if ( F32(a2, 0) >= 5 && v173 <= 6 && F32(c, 0x1598c) != 0 )
              {
                if ( F32(v7, 40) < a4 )
                  v172 = F32(c, 0x15974);
                a4 = F32(c, 0x15990);
              }
            }
            else
            {
              v173 = F32(a2, 0);
              switch ( F32(a2, 0) )
              {
                case 1:
                case 2:
                case 5:
                case 6:
                  if ( F32(c, 0x15998) != 0 )
                  {
                    if ( F32(v7, 40) >= a4 )
                      v172 = F32(c, 0x159ac);
                    else
                      v172 = F32(c, 0x159b0);
                    a4 = F32(c, 0x159a8);
                  }
                  break;
                default:
                  break;
              }
            }
            if ( F32(c, 0x14810) != 0 )
              a4 += 24000;
            if ( F32(c, 0x14814) != 0 )
            {
              v172 += 2000;
              (*v225__f) -= 800;
            }
            switch ( v173 )
            {
              case 1:
              case 5:
                v174 = F32(v7, 44) == 3;
                goto LABEL_704;
              case 2:
              case 6:
                v174 = F32(v7, 44) == 1;
LABEL_704:
                if ( v174 )
                {
                  v169 = v172;
                  (*v222__f) = 1;
                }
                else if ( F32(v7, 40) != 0 )
                {
                  v169 = -v172;
                  (*v227__f) = 1;
                }
                else
                {
                  if ( (*v228__f) != 0 )
                    (*v226__f) = 1;
LABEL_711:
                  v169 = (*v223__f);
                }
                break;
              default:
                goto LABEL_711;
            }
          }
LABEL_712:
          if ( v170 == 0 )
          {
            if ( (*v222__f) != 0 && F16(v7, 72) == 0 )
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(1));
            goto LABEL_735;
          }
          if ( v170 != 1 )
            goto LABEL_735;
          if ( F16(v7, 72) != 0 )
            goto LABEL_715;
          if ( F16(v7, 58) != 0 )
          {
            if ( F16(v7, 58) != 1 )
            {
LABEL_735:
              if ( (*v226__f) != 0 )
              {
                if ( F32(c, 0x15984) != 0 && (a3 != 0 || F32(c, 0x15988) != 0) )
                {
                  if ( F16(v7, 72) == 0 )
                    W_body_action_start((intptr_t)(v7), (intptr_t)(2), (intptr_t)(200));
                }
                else
                {
                  F32(v7, 44) = F32(v7, 44) != 3 ? 3 : 1;
                }
              }
              v177 = W_speed_ramp((intptr_t)(F32(v7, 40)), (intptr_t)(v169), (intptr_t)((*v225__f)), (intptr_t)(a4));
              F32(v7, 40) = v177;
              if ( F32(c, 0x1594c) == 3 )
              {
                W_el_4675c0((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(a6));
              }
              else
              {
                v178 = errno_decorate == 0 || F32(c, 0x200) == 0;
                SETHI(v179, HIWORD(a6));
                (*v218__f) = a6;
                SETLO(v179, F8(v7, 32));
                (*v216__f) = v179;
                (*v214__f) = v178;
                v180 = W_body_facing_sign((intptr_t)(v7), (intptr_t)(v177 / 100));
                if ( W_entity_move_horizontal((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(v180), (intptr_t)((*v214__f)), (intptr_t)(1), (intptr_t)((*v216__f)), (intptr_t)((*v218__f)), (intptr_t)(0)) == 0 )
                  F32(v7, 40) = 0;
              }
              v181 = F32(v7, 108);
              if ( v181 != 0 && ++F16(v7, 112) >= F16(v181, 68) )
              {
                v182 = ++F16(v7, 114);
                F16(v7, 112) = 0;
                if ( v182 >= F16(v181, 66) )
                {
                  if ( F32(v181, 72) != 0 )
                  {
                    F16(v7, 112) = 1;
                    v183 = F16(v181, 66) - 1;
                    F8(v7, 116) = 1;
                    F16(v7, 114) = v183;
                  }
                  else
                  {
                    F16(v7, 114) = F16(v181, 338);
                  }
                }
              }
              goto LABEL_838;
            }
            if ( (*v222__f) != 0 )
            {
              F16(v7, 60) = 0;
              F16(v7, 58) = 0;
              goto LABEL_735;
            }
            if ( F32(v7, 40) > 0 )
            {
              v175 = F16(v7, 60);
              if ( v175 < 0x28u )
              {
                F16(v7, 60) = v175 + 1;
                goto LABEL_735;
              }
            }
          }
          else
          {
            if ( F32(v7, 40) < F32(c, 0x15990) )
            {
              F16(v7, 60) = 0;
            }
            else
            {
              v176 = F16(v7, 60);
              if ( v176 < 0xC8u )
                F16(v7, 60) = v176 + 1;
            }
            if ( (*v222__f) != 0 )
              goto LABEL_735;
            if ( F16(v7, 60) < 0x50u || (*v227__f) == 0 )
            {
              F16(v7, 60) = 0;
              F16(v7, 58) = 1;
              goto LABEL_735;
            }
          }
LABEL_715:
          W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          goto LABEL_735;
        case 5:
          v156 = F16(v7, 58);
          if ( (uint16_t)v156 != 0 )
            goto LABEL_639;
          v157 = a2;
          ++F16(v7, 60);
          if ( F32(v157, 12) != 11 )
          {
            v158 = F16(v7, 60) <= 0x10u;
LABEL_647:
            if ( !v158 )
            {
              F16(v7, 58) = 1;
              F16(v7, 60) = 0;
            }
          }
          goto LABEL_656;
        case 6:
          v162 = F32(c, 0x15958);
          a4 = F32(c, 0x1595c);
          v163 = F16(v7, 60);
          (*v223__f) = v162;
          if ( v163 >= 8u )
            goto LABEL_642;
          F16(v7, 60) = v163 + 1;
          goto LABEL_656;
        default:
          goto LABEL_656;
      }
    case 0x76:
      v31 = F16(v7, 88);
      switch ( v31 )
      {
        case 0:
          v32 = F32(c, 0x15928);
          if ( v32 != 0 && F32(v7, 108) != v32 )
          {
            F32(v7, 108) = v32;
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          F16(v7, 88) = v31 + 1;
          goto LABEL_60;
        case 1:
LABEL_60:
          ++F32(v7, 92);
          if ( (F32(c, 0x204) & 0x1F0000) == 0 )
          {
            v33 = rnd();
            W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100112), (intptr_t)((_DWORD *)v7), (intptr_t)(1600 * v33 / 0x8000 - 800), (intptr_t)(-800), (intptr_t)(99), (intptr_t)(3), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          }
          break;
        case 10:
          v34 = (F32(v7, 92) + 1) % 8;
          F32(v7, 92) = v34;
          v51 = v34 < 4;
          v35 = F32(v7, 64);
          if ( !v51 )
            goto LABEL_66;
          v36 = v35 - 2;
          goto LABEL_67;
        case 20:
          v37 = (F32(v7, 92) + 1) % 4;
          F32(v7, 92) = v37;
          v51 = v37 < 2;
          v35 = F32(v7, 64);
          if ( v51 )
            v36 = v35 - 2;
          else
LABEL_66:
            v36 = v35 + 2;
LABEL_67:
          F32(v7, 64) = v36;
          break;
        default:
          break;
      }
      F8(v7, 124) = 1;
      v38 = F32(c, 0x1594c);
      (*v220__f) = 3;
      if ( v38 <= 0 )
        goto LABEL_73;
      if ( v38 > 2 )
      {
        if ( v38 == 3 )
        {
          if ( W_shot_445ce0((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a6)) )
          {
            F32(v7, 40) = 0;
            F32(v7, 24) = 0;
            goto LABEL_77;
          }
        }
        else
        {
LABEL_73:
          if ( F32(c, 0x15950) > 56000 )
            goto LABEL_77;
        }
      }
      v39 = F32(c, 0x15950);
      if ( v39 <= 64000 )
        v39 = 64000;
      F32(v7, 28) = v39;
LABEL_77:
      v40 = F32(v7, 36);
      v41 = F32(c, 0x15950);
      a4 = F32(c, 0x15954);
      v42 = F32(v7, 648);
      a3 = v40;
      (*v229__f) = (*v220__f);
      (*v223__f) = v41;
      (*v228__f) = 0;
      (*v225__f) = 0;
      if ( v42 != 0 )
        (*v223__f) = v41 / 3;
      v43 = F32(c, 0x1594c);
      if ( v43 != 0 )
      {
        if ( v43 <= 0 || v43 > 3 )
        {
          v44 = a2;
          v45 = a2;
        }
        else
        {
          v44 = W_body_can_act((intptr_t)(v7));
          (*v226__f) = 0;
          v45 = v44;
          (*v222__f) = v44;
          F8(v7, 32) = 1;
          (*v228__f) = 1;
        }
      }
      else
      {
        v46 = W_body_can_act((intptr_t)(v7));
        v44 = v46;
        if ( v46 == 0 || ((*v222__f) = 1, v40 == 0) )
          (*v222__f) = 0;
        v45 = v46 != 0 && v40 != 0;
        if ( errno_decorate == 0 && a3 != 0 )
          F16(c, 0x158bc) = 0;
        if ( F16(c, 0x158bc) < F16(c, 0xc7e8) )
          (*v222__f) = 1;
        (*v226__f) = 1;
        if ( F32(a2, 8) == 9 )
        {
          F8(v7, 32) = 8;
        }
        else
        {
          v47 = F8(v7, 32);
          if ( v47 != 0 )
            F8(v7, 32) = v47 - 1;
        }
      }
      if ( F32(c, 0x15964) > 0 && F32(c, 0x15964) <= W_grid_floor_below((intptr_t)((_DWORD *)v7), (intptr_t)(0)) )
      {
        if ( (*v223__f) < 50000 )
          (*v223__f) = 50000;
        if ( a4 < 2000 )
          a4 = 2000;
        (*v225__f) = 1;
      }
      v48 = F32(v7, 28);
      if ( v48 != 0 )
      {
        (*v223__f) = F32(v7, 28);
        a4 = v48 / 10;
      }
      switch ( (*v229__f) )
      {
        case 0:
          goto LABEL_115;
        case 1:
          if ( F16(v7, 56) == 3 )
            goto LABEL_113;
          v45 = 0;
          F16(v7, 56) = 0;
          F16(v7, 58) = 0;
          F16(v7, 60) = 0;
          goto LABEL_114;
        case 2:
        case 3:
          v45 = 0;
          W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          goto LABEL_114;
        default:
LABEL_113:
          v45 = 0;
LABEL_114:
          v44 = 0;
          (*v222__f) = 0;
LABEL_115:
          if ( F32(a2, 12) != 10 )
            goto LABEL_119;
          if ( v45 != 0 && F32(c, 0x159d4) != 0 )
          {
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(2));
            goto LABEL_119;
          }
          if ( (*v228__f) == 0 )
          {
LABEL_119:
            if ( a3 != 0 )
              goto LABEL_122;
            goto LABEL_120;
          }
          if ( a3 == 0 )
          {
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(6));
LABEL_120:
            if ( F32(c, 0x15a24) == 0 )
              v44 = 0;
          }
LABEL_122:
          if ( F32(a2, 12) == 11 && v44 != 0 && F32(c, 0x15a10) != 0 )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(5));
          if ( F32(c, 0x15a24) != 0 && F16(v7, 56) == 5 )
            (*v226__f) = 0;
          if ( F32(a2, 8) == 7
            && ((*v222__f) != 0 || F16(v7, 56) == 5 && a3 != 0)
            && F32(c, 0x15998) != 0
            && (*v225__f) == 0 )
          {
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(3));
            if ( errno_decorate == 0 )
            {
              v49 = F16(c, 0x158bc);
              if ( v49 < F16(c, 0xc7e8) && a3 == 0 )
                F16(c, 0x158bc) = v49 + 1;
            }
          }
          break;
      }
      switch ( F16(v7, 56) )
      {
        case 2:
          v55 = F16(v7, 58);
          if ( (uint16_t)v55 == 0 )
          {
            v54 = ++F16(v7, 60) <= 4u;
            goto LABEL_170;
          }
          v52 = v55 - 1;
          if ( v52 != 0 )
            goto LABEL_162;
          ++F16(v7, 60);
          v56 = a2;
          v57 = F16(v7, 60);
          F32(v7, 20) = F32(c, 0x159d8);
          if ( F32(v56, 12) != 10 && v57 > 4u )
          {
            F16(v7, 58) = 2;
            F16(v7, 60) = 0;
          }
          goto LABEL_179;
        case 3:
          switch ( F16(v7, 58) )
          {
            case 0:
              if ( ++F16(v7, 60) > 4u )
              {
                v50 = F32(c, 0x1599c);
                F16(v7, 58) = 1;
                F32(v7, 24) = v50;
                F16(v7, 60) = 0;
              }
              goto LABEL_179;
            case 1:
              F16(v7, 58) = 2;
              goto LABEL_150;
            case 2:
              if ( F32(v7, 24) > -20000 )
                F16(v7, 58) = 3;
              goto LABEL_150;
            case 3:
              ++F16(v7, 60);
LABEL_150:
              if ( F32(v7, 24) >= 0 )
              {
                v51 = F16(v7, 60) <= 6u;
                goto LABEL_164;
              }
              if ( F32(a2, 8) != 0 )
                a4 = F32(c, 0x159a0);
              else
                a4 = F32(c, 0x159a4);
              if ( (*v225__f) == 1 )
                a4 = F32(c, 0x159a4);
              break;
            default:
              goto LABEL_179;
          }
          goto LABEL_179;
        case 4:
          v60 = F16(v7, 58);
          if ( (uint16_t)v60 != 0 )
          {
            v52 = v60 - 1;
            if ( v52 != 0 )
            {
LABEL_162:
              if ( v52 == 1 )
              {
                v51 = ++F16(v7, 60) <= 4u;
LABEL_164:
                if ( !v51 )
LABEL_165:
                  W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
              }
            }
            else
            {
              F16(v7, 58) = 2;
            }
          }
          else
          {
            if ( a3 == 0 )
              goto LABEL_182;
            F16(v7, 58) = 1;
          }
LABEL_179:
          if ( a3 != 0 && F32(v7, 24) >= 0 )
          {
            F32(v7, 24) = 0;
            goto LABEL_197;
          }
LABEL_182:
          v61 = F32(c, 0x1594c);
          if ( v61 >= 0 )
          {
            if ( v61 <= 2 )
            {
              v62 = v7 + 8;
              SETLO(v62, F8(v7, 32));
              if ( W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(F32(v7, 24) / 100), (intptr_t)(v62), (intptr_t)(a6), (intptr_t)(v7 + 8), (intptr_t)((_DWORD *)(v7 + 644))) == 0 )
              {
                if ( F32(v7, 24) > 0 )
                  a3 = 1;
                F32(v7, 24) = 0;
              }
            }
            else if ( v61 == 3 )
            {
              W_el_467750((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a6));
            }
          }
          if ( (*v226__f) != 0 && (F16(v7, 56) < 3u || F16(v7, 56) > 4u) )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(4));
          v63 = F32(v7, 24) + a4;
          if ( (*v223__f) < v63 )
            v63 = (*v223__f);
          F32(v7, 24) = v63;
LABEL_197:
          v64 = F32(c, 0x1596c);
          v65 = F32(c, 0x15978);
          v66 = F16(v7, 56);
          (*v227__f) = 0;
          (*v222__f) = 0;
          (*v225__f) = 0;
          v67 = 1;
          a4 = v64;
          v68 = F32(c, 0x15970);
          (*v223__f) = v65;
          (*v228__f) = v65;
          (*v226__f) = 1;
          if ( (uint16_t)v66 == 2 || (uint16_t)v66 == 5 )
            v67 = 0;
          switch ( (*v229__f) )
          {
            case 1:
              goto LABEL_235;
            case 2:
              if ( a3 != 0 )
                goto LABEL_235;
              (*v226__f) = 0;
              break;
            case 3:
              goto LABEL_235;
            default:
              break;
          }
          if ( v67 != 0 )
          {
            if ( a3 != 0 || F32(c, 0x159a8) == 0 )
            {
              v69 = F32(a2, 0);
              if ( F32(a2, 0) >= 5 && v69 <= 6 && F32(c, 0x1598c) != 0 )
              {
                if ( F32(v7, 40) < a4 )
                  v68 = F32(c, 0x15974);
                a4 = F32(c, 0x15990);
              }
            }
            else
            {
              v69 = F32(a2, 0);
              switch ( F32(a2, 0) )
              {
                case 1:
                case 2:
                case 5:
                case 6:
                  if ( F32(c, 0x15998) != 0 )
                  {
                    if ( F32(v7, 40) >= a4 )
                      v68 = F32(c, 0x159ac);
                    else
                      v68 = F32(c, 0x159b0);
                    a4 = F32(c, 0x159a8);
                  }
                  break;
                default:
                  break;
              }
            }
            if ( F32(c, 0x14810) != 0 )
              a4 += 24000;
            if ( F32(c, 0x14814) != 0 )
            {
              v68 += 2000;
              (*v228__f) -= 800;
            }
            switch ( v69 )
            {
              case 1:
              case 5:
                v70 = F32(v7, 44) == 3;
                goto LABEL_227;
              case 2:
              case 6:
                v70 = F32(v7, 44) == 1;
LABEL_227:
                if ( v70 )
                {
                  v65 = v68;
                  (*v222__f) = 1;
                }
                else if ( F32(v7, 40) != 0 )
                {
                  v65 = -v68;
                  (*v225__f) = 1;
                }
                else
                {
                  if ( (*v226__f) != 0 )
                    (*v227__f) = 1;
LABEL_234:
                  v65 = (*v223__f);
                }
                break;
              default:
                goto LABEL_234;
            }
          }
LABEL_235:
          if ( v66 != 0 )
          {
            if ( v66 != 1 )
              goto LABEL_258;
            if ( F16(v7, 72) != 0 )
              goto LABEL_238;
            if ( F16(v7, 58) != 0 )
            {
              if ( F16(v7, 58) != 1 )
                goto LABEL_258;
              if ( (*v222__f) != 0 )
              {
                F16(v7, 60) = 0;
                F16(v7, 58) = 0;
                goto LABEL_258;
              }
              if ( F32(v7, 40) > 0 )
              {
                v71 = F16(v7, 60);
                if ( v71 < 0x28u )
                {
                  F16(v7, 60) = v71 + 1;
                  goto LABEL_258;
                }
              }
LABEL_238:
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
              goto LABEL_258;
            }
            if ( F32(v7, 40) < F32(c, 0x15990) )
            {
              F16(v7, 60) = 0;
            }
            else
            {
              v72 = F16(v7, 60);
              if ( v72 < 0xC8u )
                F16(v7, 60) = v72 + 1;
            }
            if ( (*v222__f) == 0 )
            {
              if ( F16(v7, 60) >= 0x50u && (*v225__f) != 0 )
                goto LABEL_238;
              F16(v7, 60) = 0;
              F16(v7, 58) = 1;
            }
          }
          else if ( (*v222__f) != 0 && F16(v7, 72) == 0 )
          {
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(1));
          }
LABEL_258:
          if ( (*v227__f) != 0 )
          {
            if ( F32(c, 0x15984) != 0 && (a3 != 0 || F32(c, 0x15988) != 0) )
            {
              if ( F16(v7, 72) == 0 )
              {
                W_body_action_start((intptr_t)(v7), (intptr_t)(2), (intptr_t)(200));
                v73 = W_speed_ramp((intptr_t)(F32(v7, 40)), (intptr_t)(v65), (intptr_t)((*v228__f)), (intptr_t)(a4));
                goto LABEL_508;
              }
            }
            else
            {
              F32(v7, 44) = F32(v7, 44) != 3 ? 3 : 1;
            }
          }
          v73 = W_speed_ramp((intptr_t)(F32(v7, 40)), (intptr_t)(v65), (intptr_t)((*v228__f)), (intptr_t)(a4));
LABEL_508:
          v125 = v73;
          F32(v7, 40) = v73;
          if ( F32(c, 0x1594c) == 3 )
          {
            W_el_4675c0((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(a6));
          }
          else
          {
            v126 = errno_decorate == 0 || F32(c, 0x200) == 0;
            SETHI(v127, HIWORD(a6));
            (*v217__f) = a6;
            SETLO(v127, F8(v7, 32));
            (*v215__f) = v127;
            (*v213__f) = v126;
            v128 = W_body_facing_sign((intptr_t)(v7), (intptr_t)(v125 / 100));
            if ( W_entity_move_horizontal((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(v128), (intptr_t)((*v213__f)), (intptr_t)(1), (intptr_t)((*v215__f)), (intptr_t)((*v217__f)), (intptr_t)(0)) == 0 )
              F32(v7, 40) = 0;
          }
          goto LABEL_838;
        case 5:
          v52 = F16(v7, 58);
          if ( (uint16_t)v52 != 0 )
            goto LABEL_162;
          v53 = a2;
          ++F16(v7, 60);
          if ( F32(v53, 12) != 11 )
          {
            v54 = F16(v7, 60) <= 0x10u;
LABEL_170:
            if ( !v54 )
            {
              F16(v7, 58) = 1;
              F16(v7, 60) = 0;
            }
          }
          goto LABEL_179;
        case 6:
          v58 = F32(c, 0x15958);
          a4 = F32(c, 0x1595c);
          v59 = F16(v7, 60);
          (*v223__f) = v58;
          if ( v59 >= 8u )
            goto LABEL_165;
          F16(v7, 60) = v59 + 1;
          goto LABEL_179;
        default:
          goto LABEL_179;
      }
    case 0x77:
      v84 = F16(v7, 88);
      if ( (uint16_t)v84 == 10 )
      {
        v88 = (F32(v7, 92) + 1) % 8;
        F32(v7, 92) = v88;
        v51 = v88 < 4;
        v86 = F32(v7, 64);
        if ( v51 )
        {
          v87 = v86 - 2;
          goto LABEL_309;
        }
      }
      else
      {
        if ( v84 != 20 )
          goto LABEL_310;
        v85 = (F32(v7, 92) + 1) % 4;
        F32(v7, 92) = v85;
        v51 = v85 < 2;
        v86 = F32(v7, 64);
        if ( v51 )
        {
          v87 = v86 - 2;
          goto LABEL_309;
        }
      }
      v87 = v86 + 2;
LABEL_309:
      F32(v7, 64) = v87;
LABEL_310:
      F8(v7, 124) = 1;
      v89 = F32(c, 0x1594c);
      (*v220__f) = 3;
      if ( v89 <= 0 )
        goto LABEL_315;
      if ( v89 > 2 )
      {
        if ( v89 == 3 )
        {
          if ( W_shot_445ce0((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a6)) )
          {
            F32(v7, 40) = 0;
            F32(v7, 24) = 0;
            goto LABEL_319;
          }
        }
        else
        {
LABEL_315:
          if ( F32(c, 0x15950) > 56000 )
            goto LABEL_319;
        }
      }
      v90 = F32(c, 0x15950);
      if ( v90 <= 64000 )
        v90 = 64000;
      F32(v7, 28) = v90;
LABEL_319:
      v91 = F32(v7, 648);
      v92 = F32(v7, 36);
      v93 = F32(c, 0x15954);
      (*v230__f) = (*v220__f);
      v94 = F32(c, 0x15950);
      a3 = v92;
      (*v223__f) = v94;
      a4 = v93;
      (*v229__f) = 0;
      (*v228__f) = 0;
      if ( v91 != 0 )
        (*v223__f) = v94 / 3;
      v95 = F32(c, 0x1594c);
      if ( v95 != 0 )
      {
        if ( v95 <= 0 || v95 > 3 )
        {
          v96 = a2;
          v97 = a2;
        }
        else
        {
          v96 = W_body_can_act((intptr_t)(v7));
          (*v227__f) = 0;
          v97 = v96;
          (*v222__f) = v96;
          F8(v7, 32) = 1;
          (*v229__f) = 1;
        }
      }
      else
      {
        v98 = W_body_can_act((intptr_t)(v7));
        v96 = v98;
        if ( v98 == 0 || ((*v222__f) = 1, v92 == 0) )
          (*v222__f) = 0;
        v97 = v98 != 0 && v92 != 0;
        if ( errno_decorate == 0 && a3 != 0 )
          F16(c, 0x158bc) = 0;
        if ( F16(c, 0x158bc) < F16(c, 0xc7e8) )
          (*v222__f) = 1;
        (*v227__f) = 1;
        if ( F32(a2, 8) == 9 )
        {
          F8(v7, 32) = 8;
        }
        else
        {
          v99 = F8(v7, 32);
          if ( v99 != 0 )
            F8(v7, 32) = v99 - 1;
        }
      }
      if ( F32(c, 0x15964) > 0 && F32(c, 0x15964) <= W_grid_floor_below((intptr_t)((_DWORD *)v7), (intptr_t)(0)) )
      {
        if ( (*v223__f) < 50000 )
          (*v223__f) = 50000;
        if ( a4 < 2000 )
          a4 = 2000;
        (*v228__f) = 1;
      }
      v100 = F32(v7, 28);
      if ( v100 != 0 )
      {
        (*v223__f) = F32(v7, 28);
        a4 = v100 / 10;
      }
      switch ( (*v230__f) )
      {
        case 0:
          goto LABEL_356;
        case 1:
          if ( F16(v7, 56) != 3 )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          goto LABEL_354;
        case 2:
        case 3:
          v97 = 0;
          W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          goto LABEL_355;
        default:
LABEL_354:
          v97 = 0;
LABEL_355:
          v96 = 0;
          (*v222__f) = 0;
LABEL_356:
          if ( F32(a2, 12) == 10 )
          {
            if ( v97 != 0 && F32(c, 0x159d4) != 0 )
            {
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(2));
            }
            else if ( (*v229__f) != 0 )
            {
              if ( a3 != 0 )
                goto LABEL_367;
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(6));
              goto LABEL_365;
            }
          }
          if ( a3 == 0 )
          {
LABEL_365:
            if ( F32(c, 0x15a24) == 0 )
              v96 = 0;
          }
LABEL_367:
          if ( F32(a2, 12) == 11 && v96 != 0 && F32(c, 0x15a10) != 0 )
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(5));
          if ( F32(c, 0x15a24) != 0 && F16(v7, 56) == 5 )
            (*v227__f) = 0;
          if ( F32(a2, 8) == 7
            && ((*v222__f) != 0 || F16(v7, 56) == 5 && a3 != 0)
            && F32(c, 0x15998) != 0
            && (*v228__f) == 0 )
          {
            W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(3));
            if ( errno_decorate == 0 )
            {
              v101 = F16(c, 0x158bc);
              if ( v101 < F16(c, 0xc7e8) && a3 == 0 )
                F16(c, 0x158bc) = v101 + 1;
            }
          }
          break;
      }
      break;
    case 0x78:
      if ( errno_decorate == 0 )
      {
        v184 = rnd();
        W_sfx_play_at((intptr_t)(c), (intptr_t)(0), (intptr_t)(126), (intptr_t)(0), (intptr_t)(2000 * v184 / 0x8000 - 2000), (intptr_t)(0));
        W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100090), (intptr_t)((_DWORD *)v7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(99), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(20));
        W_body_action_reset((intptr_t)(v7));
        W_ih_4466b0((intptr_t)(c), (intptr_t)(v7), (intptr_t)(0), (intptr_t)(1));
      }
      goto LABEL_838;
    case 0x79:
      v74 = F16(v7, 88);
      (*v220__f) = 3;
      F8(v7, 124) = 1;
      if ( v74 != 0 )
      {
        if ( v74 != 1 )
          goto LABEL_282;
      }
      else
      {
        v75 = F32(c, 0x1592c);
        if ( v75 != 0 && F32(v7, 108) != v75 )
        {
          F32(v7, 108) = v75;
          F8(v7, 116) = 0;
          F16(v7, 114) = 0;
          F16(v7, 112) = 0;
        }
        F32(v7, 24) = 0;
        F32(v7, 40) = 0;
      }
      v76 = F32(v7, 92) + 1;
      F32(v7, 92) = v76;
      if ( v76 > 5 )
      {
        F16(v7, 88) = v74 + 1;
        if ( v29 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a4), (intptr_t)(&a3), (intptr_t)(&a2)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x14u), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(a3), (intptr_t)(a2), (intptr_t)(0));
        }
      }
LABEL_282:
      a2 = F32(c, 0x15930);
      if ( a2 == 0 )
        goto LABEL_296;
      v77 = F32(c, 0xc7a4);
      if ( v77 != 0 )
      {
        v78 = a6;
        if ( (uint16_t)a6 >= F16(v77, 68) )
          v78 = 0;
        if ( F32(F32(v77, 64) + 660 * v78, 40) > 20000 )
          goto LABEL_292;
        v79 = a6;
        if ( (uint16_t)a6 >= F16(v77, 68) )
          v79 = 0;
        if ( F32(F32(v77, 64) + 660 * v79, 24) > 20000 )
        {
LABEL_292:
          v80 = a2;
          if ( F32(v7, 108) == a2 )
            goto LABEL_296;
        }
        else
        {
          v80 = F32(c, 0x1592c);
          if ( F32(v7, 108) == v80 )
            goto LABEL_296;
        }
      }
      else
      {
        v80 = F32(c, 0x1592c);
        if ( F32(v7, 108) == v80 )
          goto LABEL_296;
      }
      F32(v7, 108) = v80;
      F8(v7, 116) = 0;
      F16(v7, 114) = 0;
      F16(v7, 112) = 0;
LABEL_296:
      v81 = F32(v7, 108);
      if ( v81 != 0 && ++F16(v7, 112) >= F16(v81, 68) )
      {
        v82 = ++F16(v7, 114);
        F16(v7, 112) = 0;
        if ( v82 >= F16(v81, 66) )
        {
          if ( F32(v81, 72) != 0 )
          {
            F16(v7, 112) = 1;
            v83 = F16(v81, 66) - 1;
            F8(v7, 116) = 1;
            F16(v7, 114) = v83;
          }
          else
          {
            F16(v7, 114) = F16(v81, 338);
          }
        }
      }
      goto LABEL_838;
    case 0x7A:
      if ( errno_decorate == 0 )
      {
        F32(c, 0xc764) = 1;
        F32(v7, 24) = 0;
        F32(v7, 40) = 0;
        if ( F16(F32(c, 0x750), 1098) == 0 )
        {
          F32(c, 0xc764) = 0;
          W_body_action_reset((intptr_t)(v7));
        }
      }
      goto LABEL_838;
    case 0x81:
      if ( F32(v7, 108) != D(0x629a70) )
      {
        F32(v7, 108) = D(0x629a70);
        F8(v7, 116) = 0;
        F16(v7, 114) = 0;
        F16(v7, 112) = 0;
      }
      v30 = SYS32(10340);
      F32(c, 0xf4) = SYS32(10340);
      F32(c, 0xf8) = v30;
      goto LABEL_838;
    default:
      goto LABEL_838;
  }
  switch ( F16(v7, 56) )
  {
    case 2:
      v107 = F16(v7, 58);
      if ( (uint16_t)v107 == 0 )
      {
        v106 = ++F16(v7, 60) <= 4u;
        goto LABEL_412;
      }
      v104 = v107 - 1;
      if ( v104 != 0 )
        goto LABEL_404;
      ++F16(v7, 60);
      v108 = a2;
      v109 = F16(v7, 60);
      F32(v7, 20) = F32(c, 0x159d8);
      if ( F32(v108, 12) != 10 && v109 > 4u )
      {
        F16(v7, 58) = 2;
        F16(v7, 60) = 0;
      }
      goto LABEL_421;
    case 3:
      switch ( F16(v7, 58) )
      {
        case 0:
          if ( ++F16(v7, 60) > 4u )
          {
            v102 = F32(c, 0x1599c);
            F16(v7, 58) = 1;
            F32(v7, 24) = v102;
            F16(v7, 60) = 0;
          }
          goto LABEL_421;
        case 1:
          F16(v7, 58) = 2;
          goto LABEL_392;
        case 2:
          if ( F32(v7, 24) > -20000 )
            F16(v7, 58) = 3;
          goto LABEL_392;
        case 3:
          ++F16(v7, 60);
LABEL_392:
          if ( F32(v7, 24) >= 0 )
          {
            v103 = F16(v7, 60) <= 6u;
            goto LABEL_406;
          }
          if ( F32(a2, 8) != 0 )
            a4 = F32(c, 0x159a0);
          else
            a4 = F32(c, 0x159a4);
          if ( (*v228__f) == 1 )
            a4 = F32(c, 0x159a4);
          break;
        default:
          goto LABEL_421;
      }
      goto LABEL_421;
    case 4:
      v112 = F16(v7, 58);
      if ( (uint16_t)v112 != 0 )
      {
        v104 = v112 - 1;
        if ( v104 != 0 )
        {
LABEL_404:
          if ( v104 == 1 )
          {
            v103 = ++F16(v7, 60) <= 4u;
LABEL_406:
            if ( !v103 )
LABEL_407:
              W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
          }
        }
        else
        {
          F16(v7, 58) = 2;
        }
      }
      else
      {
        if ( a3 == 0 )
          goto LABEL_424;
        F16(v7, 58) = 1;
      }
LABEL_421:
      if ( a3 != 0 && F32(v7, 24) >= 0 )
      {
        F32(v7, 24) = 0;
        goto LABEL_439;
      }
LABEL_424:
      v113 = F32(c, 0x1594c);
      if ( v113 >= 0 )
      {
        if ( v113 <= 2 )
        {
          v114 = v7 + 8;
          SETLO(v114, F8(v7, 32));
          if ( W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(F32(v7, 24) / 100), (intptr_t)(v114), (intptr_t)(a6), (intptr_t)(v7 + 8), (intptr_t)((_DWORD *)(v7 + 644))) == 0 )
          {
            if ( F32(v7, 24) > 0 )
              a3 = 1;
            F32(v7, 24) = 0;
          }
        }
        else if ( v113 == 3 )
        {
          W_el_467750((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a6));
        }
      }
      if ( (*v227__f) != 0 && (F16(v7, 56) < 3u || F16(v7, 56) > 4u) )
        W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(4));
      v115 = F32(v7, 24) + a4;
      if ( (*v223__f) < v115 )
        v115 = (*v223__f);
      F32(v7, 24) = v115;
LABEL_439:
      v116 = F32(c, 0x1596c);
      v117 = F32(c, 0x15978);
      v118 = F16(v7, 56);
      (*v226__f) = 0;
      (*v222__f) = 0;
      (*v227__f) = 0;
      v119 = 1;
      a4 = v116;
      v120 = F32(c, 0x15970);
      (*v223__f) = v117;
      (*v225__f) = v117;
      (*v228__f) = 1;
      if ( (uint16_t)v118 == 2 || (uint16_t)v118 == 5 )
        v119 = 0;
      switch ( (*v230__f) )
      {
        case 1:
          goto LABEL_477;
        case 2:
          if ( a3 != 0 )
            goto LABEL_477;
          (*v228__f) = 0;
          break;
        case 3:
          goto LABEL_477;
        default:
          break;
      }
      if ( v119 != 0 )
      {
        if ( a3 != 0 || F32(c, 0x159a8) == 0 )
        {
          v121 = F32(a2, 0);
          if ( F32(a2, 0) >= 5 && v121 <= 6 && F32(c, 0x1598c) != 0 )
          {
            if ( F32(v7, 40) < a4 )
              v120 = F32(c, 0x15974);
            a4 = F32(c, 0x15990);
          }
        }
        else
        {
          v121 = F32(a2, 0);
          switch ( F32(a2, 0) )
          {
            case 1:
            case 2:
            case 5:
            case 6:
              if ( F32(c, 0x15998) != 0 )
              {
                if ( F32(v7, 40) >= a4 )
                  v120 = F32(c, 0x159ac);
                else
                  v120 = F32(c, 0x159b0);
                a4 = F32(c, 0x159a8);
              }
              break;
            default:
              break;
          }
        }
        if ( F32(c, 0x14810) != 0 )
          a4 += 24000;
        if ( F32(c, 0x14814) != 0 )
        {
          v120 += 2000;
          (*v225__f) -= 800;
        }
        switch ( v121 )
        {
          case 1:
          case 5:
            v122 = F32(v7, 44) == 3;
            goto LABEL_469;
          case 2:
          case 6:
            v122 = F32(v7, 44) == 1;
LABEL_469:
            if ( v122 )
            {
              v117 = v120;
              (*v222__f) = 1;
            }
            else if ( F32(v7, 40) != 0 )
            {
              v117 = -v120;
              (*v227__f) = 1;
            }
            else
            {
              if ( (*v228__f) != 0 )
                (*v226__f) = 1;
LABEL_476:
              v117 = (*v223__f);
            }
            break;
          default:
            goto LABEL_476;
        }
      }
LABEL_477:
      if ( v118 == 0 )
      {
        if ( (*v222__f) != 0 && F16(v7, 72) == 0 )
          W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(1));
        goto LABEL_500;
      }
      if ( v118 != 1 )
        goto LABEL_500;
      if ( F16(v7, 72) != 0 )
        goto LABEL_480;
      if ( F16(v7, 58) != 0 )
      {
        if ( F16(v7, 58) != 1 )
        {
LABEL_500:
          if ( (*v226__f) != 0 )
          {
            if ( F32(c, 0x15984) != 0 && (a3 != 0 || F32(c, 0x15988) != 0) )
            {
              if ( F16(v7, 72) == 0 )
                W_body_action_start((intptr_t)(v7), (intptr_t)(2), (intptr_t)(200));
            }
            else
            {
              F32(v7, 44) = F32(v7, 44) != 3 ? 3 : 1;
            }
          }
          v73 = W_speed_ramp((intptr_t)(F32(v7, 40)), (intptr_t)(v117), (intptr_t)((*v225__f)), (intptr_t)(a4));
          goto LABEL_508;
        }
        if ( (*v222__f) != 0 )
        {
          F16(v7, 60) = 0;
          F16(v7, 58) = 0;
          goto LABEL_500;
        }
        if ( F32(v7, 40) > 0 )
        {
          v123 = F16(v7, 60);
          if ( v123 < 0x28u )
          {
            F16(v7, 60) = v123 + 1;
            goto LABEL_500;
          }
        }
      }
      else
      {
        if ( F32(v7, 40) < F32(c, 0x15990) )
        {
          F16(v7, 60) = 0;
        }
        else
        {
          v124 = F16(v7, 60);
          if ( v124 < 0xC8u )
            F16(v7, 60) = v124 + 1;
        }
        if ( (*v222__f) != 0 )
          goto LABEL_500;
        if ( F16(v7, 60) < 0x50u || (*v227__f) == 0 )
        {
          F16(v7, 60) = 0;
          F16(v7, 58) = 1;
          goto LABEL_500;
        }
      }
LABEL_480:
      W_body_set_state((intptr_t)((_WORD *)v7), (intptr_t)(0));
      goto LABEL_500;
    case 5:
      v104 = F16(v7, 58);
      if ( (uint16_t)v104 != 0 )
        goto LABEL_404;
      v105 = a2;
      ++F16(v7, 60);
      if ( F32(v105, 12) != 11 )
      {
        v106 = F16(v7, 60) <= 0x10u;
LABEL_412:
        if ( !v106 )
        {
          F16(v7, 58) = 1;
          F16(v7, 60) = 0;
        }
      }
      goto LABEL_421;
    case 6:
      v110 = F32(c, 0x15958);
      a4 = F32(c, 0x1595c);
      v111 = F16(v7, 60);
      (*v223__f) = v110;
      if ( v111 >= 8u )
        goto LABEL_407;
      F16(v7, 60) = v111 + 1;
      goto LABEL_421;
    default:
      goto LABEL_421;
  }
LABEL_838:
  return;
}
