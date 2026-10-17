/* shots.c — FUN_004710c0 (the shot band), FUN_0055d140 (the shot init), FUN_0046da00 (the 96-band) + helpers.  See shots.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (the decompile's control flow kept verbatim: labels, gotos across
 * the cases; IDA's local types, fields through width-typed accessors), then finished by hand against the asm.
 * Hand finishes: 0x4710c0's int64 parameter (IDA merged the flag / part dwords) split back; the four stdcall
 * helpers that pass their caller's ecx through to a thiscall (0x445ce0 0x4504c0 0x46e110 0x46e150) take it as an
 * explicit first parameter (always the caller's `this`). */
#include "shots.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }




#include "actor_update.h"
#include "camera_follow.h"
#include "char_attack.h"
#include "char_integrate.h"
#include "char_physics.h"
#include "char_state.h"
#include "device_init.h"
#include "entity_fx.h"
#include "entity_life.h"
#include "entity_move.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "particle_spawn.h"
#include "room_tick.h"
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
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define qmemcpy memcpy   /* IDA's inline memcpy */
typedef char CHAR;
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define FU32(p, o) (*(uint32_t *)((uint8_t *)(intptr_t)(p) + (o)))   /* an `unsigned int` load: >> / < / / stay unsigned */
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F64(p, o) (*(int64_t *)((uint8_t *)(intptr_t)(p) + (o)))

static inline intptr_t W_angle_wrap(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)angle_wrap((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_body_action_reset(intptr_t a0, ...) { body_action_reset((wl_body *)a0); return 0; }
static inline intptr_t W_body_action_start(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)body_action_start((wl_body *)a0, (int16_t)a1, (int32_t)a2); }
static inline intptr_t W_body_clip_step(intptr_t a0, ...) { body_clip_step((wl_body *)a0); return 0; }
static inline intptr_t W_body_hit_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, intptr_t a20, intptr_t a21, intptr_t a22, intptr_t a23, ...) { return (intptr_t)body_hit_spawn((wl_char *)a0, (wl_char *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19, (int32_t)a20, (int32_t)a21, (int32_t)a22, (int32_t)a23); }
static inline intptr_t W_body_region_flags(intptr_t a0, ...) { return (intptr_t)body_region_flags((wl_body *)a0); }
static inline intptr_t W_body_set_angle(intptr_t a0, intptr_t a1, intptr_t a2, ...) { body_set_angle((wl_body *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1, ...) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_body_set_state(intptr_t a0, intptr_t a1, ...) { body_set_state((wl_body *)a0, (int16_t)a1); return 0; }
static inline intptr_t W_char_copy_rects(intptr_t a0, intptr_t a1, ...) { char_copy_rects((const wl_char *)a0, (wl_body *)a1); return 0; }
static inline intptr_t W_char_part_top(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_part_top((const wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_entity_anchor(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { entity_anchor((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4, (int32_t *)a5); return 0; }
static inline intptr_t W_entity_fx_reset(intptr_t a0, intptr_t a1, ...) { entity_fx_reset((wl_entity *)a0, (int16_t)a1); return 0; }
static inline intptr_t W_entity_move_horizontal(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_move_horizontal((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (uint16_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_entity_release(intptr_t a0, intptr_t a1, ...) { entity_release((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_ride_part(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_ride_part((wl_entity *)a0, (const wl_body *)a1, (uint16_t)a2); return 0; }
static inline intptr_t W_entity_scatter(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)entity_scatter((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_entity_set_track(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_set_track((wl_entity *)a0, (uint16_t)a1, (int16_t)a2, (int16_t)a3, (uint8_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_setup(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, ...) { entity_setup((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (uint8_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int16_t)a14); return 0; }
static inline intptr_t W_entity_snd_free(intptr_t a0, ...) { entity_snd_free((wl_entity *)a0); return 0; }
static inline intptr_t W_entity_snd_keep(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_snd_keep((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2); return 0; }
static inline intptr_t W_entity_snd_loop(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { entity_snd_loop((wl_entity *)a0, (uint16_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8); return 0; }
static inline intptr_t W_entity_snd_release(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { entity_snd_release((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_entity_snd_start(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_snd_start((wl_entity *)a0, (uint16_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_track_on(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_track_on((wl_entity *)a0, (uint16_t)a1, (int16_t)a2, (int16_t)a3); }
static inline intptr_t W_facing_back(intptr_t a0, ...) { return (intptr_t)facing_back((int32_t)a0); }
static inline intptr_t W_item_sparkle(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)item_sparkle((wl_entity *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_part_center_x(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_x((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_center_y(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_y((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_facing(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_facing((const wl_entity *)a0, (int32_t)a1); }
static inline intptr_t W_part_left(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_left((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_particle_tint_set(intptr_t a0, intptr_t a1, ...) { particle_tint_set((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_parts_free(intptr_t a0, intptr_t a1, ...) { parts_free((void *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_polar_div(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { polar_div((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t *)a5, (int32_t *)a6); return 0; }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_shot_end(intptr_t a0, intptr_t a1, intptr_t a2, ...) { shot_end((wl_entity *)a0, (uint8_t *)a1, (int32_t)a2); return 0; }
static inline intptr_t W_speed_ramp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)speed_ramp((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_tint_fade_b(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tint_fade_b((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_view_box_visible(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)view_box_visible((const camera_view *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_vpool_load(intptr_t a0, ...) { return (intptr_t)vpool_load((wl_vpool *)a0); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }

int32_t shots_tick(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t shot_init(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int16_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15, int32_t a16, int32_t a17, int32_t a18);
int32_t band96_tick(wl_char *self);
int32_t shot_445ce0(wl_char *self, int32_t * a1, uint16_t a2);
int32_t shot_449700(wl_char *self, int32_t a2, int32_t a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t shot_4497b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t shot_4504c0(wl_char *self, int32_t a1, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14);
int32_t shot_4538f0(int32_t * a1, int32_t * a2, int32_t a3);
int32_t shot_46e110(wl_char *self, int32_t * a1, uint16_t a2);
int32_t shot_46e150(wl_char *self, int32_t * a1, uint16_t a2);
int32_t shot_46e190(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t shot_476500(int32_t a1, int32_t a2, int32_t a3);
int32_t shot_476550(int32_t a1, int32_t a2, int32_t * a3, double * a4);
int32_t shot_476740(wl_char *self, int32_t a2);
int32_t shot_476a80(int32_t * a1);
int32_t shot_476ab0(wl_char *self, int32_t * a1, int32_t a2, uint16_t a3, int32_t a4);
int64_t shot_476ba0(int32_t * a1, int32_t a2);
int32_t shot_476d30(wl_char *self, int32_t a2, uint16_t a3, uint16_t a4, int32_t a5);
int32_t shot_476e20(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_476ef0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13);
int32_t shot_477070(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_4771d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, uint16_t a5, int32_t * a6, int32_t * a7, int32_t a8, int32_t a9, int32_t a10);
int32_t shot_477310(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t shot_477560(wl_char *self, int32_t * a1, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t shot_477650(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t shot_477b70(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_477f10(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_4781f0(wl_char *self, int32_t a2, int32_t a3);
int32_t shot_478550(int32_t a1, int32_t a2);
int32_t shot_4785a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);
int32_t shot_478820(int32_t * a1, uint16_t a2, int32_t a3, int32_t * a4, int32_t a5);
int32_t shot_54cab0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint16_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12);
int32_t shot_54cca0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t shot_55f150(int32_t a1, int32_t a2);
double shot_54cfc0(int32_t a1, int32_t a2);
int32_t room_band64_tick(wl_char *self);

/* 0x4710c0: __int16 __thiscall sub_4710C0(cls_4022A0 *this, int a2, int a3, __int64 errno_decorate) */
int32_t shots_tick(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    int64_t errno_decorate = (int64_t)(((uint64_t)(uint32_t)a5 << 32) | (uint32_t)a4);   /* IDA merged the two dwords (flag, part) */
    intptr_t v5;
    uint16_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    uint16_t v10;
    intptr_t v11;
    uint16_t v12;
    intptr_t v13;
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
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    uint16_t v30;
    int16_t v31;
    intptr_t v32;
    intptr_t v33;
    intptr_t v34;
    intptr_t v35;
    intptr_t v36;
    intptr_t v37;
    intptr_t v38;
    intptr_t v39;
    intptr_t v40;
    intptr_t v41;
    intptr_t v42;
    intptr_t v43;
    intptr_t v44;
    int32_t * v45;
    intptr_t * v46;
    intptr_t v47;
    intptr_t v48;
    intptr_t v49;
    intptr_t v50;
    intptr_t v51;
    uint16_t v52;
    intptr_t v53;
    intptr_t v54;
    intptr_t v55;
    intptr_t v56;
    int v57;
    intptr_t v58;
    intptr_t v59;
    intptr_t v60;
    intptr_t v61;
    intptr_t v62;
    intptr_t v63;
    intptr_t v64;
    intptr_t v65;
    intptr_t v66;
    intptr_t v67;
    intptr_t v68;
    intptr_t v69;
    intptr_t v70;
    intptr_t v71;
    intptr_t v72;
    intptr_t v73;
    intptr_t v74;
    intptr_t v75;
    intptr_t v76;
    uint32_t v77;
    intptr_t v78;
    intptr_t v79;
    intptr_t v80;
    intptr_t v81;
    intptr_t v82;
    intptr_t v83;
    intptr_t v84;
    intptr_t v85;
    intptr_t v86;
    intptr_t v87;
    intptr_t v88;
    intptr_t v89;
    intptr_t v90;
    intptr_t v91;
    intptr_t v92;
    intptr_t v93;
    intptr_t * v94;
    intptr_t v95;
    intptr_t v96;
    intptr_t v97;
    intptr_t v98;
    intptr_t v99;
    intptr_t v100;
    intptr_t v101;
    intptr_t v102;
    intptr_t v103;
    intptr_t v104;
    intptr_t v105;
    intptr_t v106;
    intptr_t v107;
    intptr_t v108;
    intptr_t ** v109;
    intptr_t * v110;
    intptr_t * v111;
    intptr_t v112;
    intptr_t v113;
    intptr_t v114;
    intptr_t v115;
    intptr_t v116;
    double v117;
    double v118;
    double v119;
    double v120;
    intptr_t v121;
    intptr_t v122;
    intptr_t v123;
    intptr_t v124;
    intptr_t v125;
    intptr_t v126;
    intptr_t v127;
    intptr_t ** v128;
    intptr_t * v129;
    intptr_t * v130;
    intptr_t v131;
    intptr_t v132;
    intptr_t v133;
    intptr_t v134;
    intptr_t v135;
    intptr_t v136;
    intptr_t v137;
    intptr_t v138;
    intptr_t v139;
    intptr_t v140;
    intptr_t v141;
    intptr_t v142;
    intptr_t v143;
    intptr_t v144;
    intptr_t v145;
    intptr_t * v146;
    intptr_t v147;
    intptr_t v148;
    intptr_t v149;
    intptr_t v150;
    double v151;
    double v152;
    double v153;
    double v154;
    intptr_t v155;
    intptr_t v156;
    intptr_t v157;
    intptr_t v158;
    intptr_t v159;
    intptr_t v160;
    int16_t v161;
    intptr_t v162;
    intptr_t v163;
    intptr_t * v164;
    intptr_t v165;
    intptr_t ** v166;
    intptr_t * v167;
    intptr_t v168;
    intptr_t * v169;
    intptr_t v170;
    intptr_t v171;
    intptr_t v172;
    intptr_t v173;
    intptr_t v174;
    intptr_t v175;
    intptr_t v176;
    uint16_t v177;
    intptr_t v178;
    intptr_t v179;
    intptr_t v180;
    intptr_t v181;
    intptr_t v182;
    uint16_t v183;
    intptr_t v184;
    intptr_t v185;
    intptr_t v186;
    intptr_t v187;
    intptr_t v188;
    intptr_t v189;
    intptr_t v190;
    intptr_t v191;
    intptr_t v192;
    int16_t v193;
    intptr_t v194;
    int v195;
    intptr_t v196;
    intptr_t v197;
    int16_t v198;
    intptr_t v199;
    intptr_t v200;
    intptr_t v201;
    intptr_t v202;
    intptr_t v203;
    intptr_t v204;
    intptr_t v205;
    intptr_t v206;
    intptr_t v207;
    intptr_t v208;
    intptr_t v209;
    intptr_t v210;
    intptr_t v211;
    intptr_t v212;
    intptr_t v213;
    intptr_t v214;
    intptr_t v215;
    intptr_t v216;
    intptr_t v217;
    intptr_t v218;
    intptr_t v219;
    int16_t v220;
    intptr_t ** v221;
    intptr_t * v222;
    intptr_t * v223;
    intptr_t v224;
    intptr_t v225;
    intptr_t v226;
    intptr_t v227;
    intptr_t * v228;
    intptr_t v229;
    intptr_t v230;
    intptr_t v231;
    intptr_t v232;
    int16_t v233;
    intptr_t v234;
    intptr_t v235;
    intptr_t v236;
    intptr_t v237;
    intptr_t v238;
    intptr_t v239;
    intptr_t v240;
    intptr_t v241;
    intptr_t v242;
    intptr_t v243;
    intptr_t v244;
    intptr_t v245;
    intptr_t v246;
    intptr_t v247;
    intptr_t v248;
    intptr_t v249;
    intptr_t v250;
    intptr_t v251;
    intptr_t v252;
    intptr_t v253;
    intptr_t v254;
    int16_t v255;
    intptr_t v256;
    intptr_t * v257;
    intptr_t v258;
    intptr_t v259;
    intptr_t v260;
    intptr_t v261;
    intptr_t * v262;
    intptr_t v263;
    intptr_t v264;
    intptr_t v265;
    intptr_t v266;
    intptr_t v267;
    intptr_t v268;
    intptr_t v269;
    intptr_t v270;
    int16_t v271;
    intptr_t v272;
    intptr_t v273;
    intptr_t v274;
    intptr_t v275;
    intptr_t v276;
    intptr_t v277;
    intptr_t v278;
    intptr_t v279;
    intptr_t v280;
    intptr_t v281;
    intptr_t v282;
    intptr_t v283;
    intptr_t v284;
    intptr_t v285;
    intptr_t v286;
    intptr_t v287;
    intptr_t v288;
    intptr_t v289;
    intptr_t v290;
    intptr_t v291;
    intptr_t v292;
    intptr_t v293;
    intptr_t v294;
    uint16_t v295;
    intptr_t v296;
    intptr_t v297;
    intptr_t v298;
    intptr_t v299;
    int16_t v300;
    intptr_t v301;
    intptr_t v302;
    intptr_t v303;
    intptr_t v304;
    intptr_t v305;
    intptr_t v306;
    intptr_t v307;
    intptr_t v308;
    intptr_t v309;
    intptr_t v310;
    intptr_t v311;
    intptr_t v312;
    intptr_t v313;
    intptr_t v314;
    intptr_t v315;
    intptr_t v316;
    intptr_t v317;
    intptr_t v318;
    intptr_t v319;
    intptr_t v320;
    intptr_t v321;
    intptr_t v322;
    intptr_t v323;
    intptr_t v324;
    intptr_t v325;
    int16_t v326;
    int32_t * v327;
    intptr_t v328;
    intptr_t v329;
    intptr_t v330;
    intptr_t v331;
    intptr_t v332;
    intptr_t v333;
    _Alignas(8) uint8_t frame_[0x104 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v390 = (intptr_t *)(frame_ + 0x44);   /* [2] at ebp-0xc0 */
    intptr_t *const v337__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x104 */
    intptr_t *const v336__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x104 */
    intptr_t *const v335__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x104 */
    intptr_t *const v363__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v362__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v361__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v360__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v359__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v358__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v357__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v356__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v355__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v354__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v353__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v352__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v351__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v350__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v349__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v348__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v347__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v346__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v345__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v340__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v339__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v338__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x100 */
    char * *const v344__f = (char * *)(frame_ + 0x4);   /* ebp-0x100 */
    char * *const v343__f = (char * *)(frame_ + 0x4);   /* ebp-0x100 */
    char * *const v342__f = (char * *)(frame_ + 0x4);   /* ebp-0x100 */
    char * *const v341__f = (char * *)(frame_ + 0x4);   /* ebp-0x100 */
    intptr_t *const v376__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v375__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v374__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v373__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v372__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v371__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v370__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v369__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v368__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v367__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v366__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v365__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v364__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xfc */
    intptr_t *const v378__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xf4 */
    intptr_t *const v377__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xf4 */
    intptr_t *const v379__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xec */
    intptr_t *const v380__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0xe8 */
    intptr_t *const v384__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xe4 */
    intptr_t *const v382__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xe4 */
    intptr_t *const v381__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xe4 */
    int64_t *const v386__f = (int64_t *)(frame_ + 0x20);   /* ebp-0xe4 */
    int64_t *const v385__f = (int64_t *)(frame_ + 0x20);   /* ebp-0xe4 */
    int64_t *const v383__f = (int64_t *)(frame_ + 0x20);   /* ebp-0xe4 */
    int32_t * *const v387__f = (int32_t * *)(frame_ + 0x38);   /* ebp-0xcc */
    intptr_t *const v388__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0xc8 */
    intptr_t *const v389__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0xc4 */
    double *const v391__f = (double *)(frame_ + 0x4c);   /* ebp-0xb8 */
    intptr_t *const v392__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0xb0 */
    intptr_t *const v393__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0xac */
    intptr_t *const v394__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0xa8 */
    intptr_t *const v395__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0xa4 */
    intptr_t *const v396__f = (intptr_t *)(frame_ + 0x64);   /* ebp-0xa0 */
    intptr_t *const v397__f = (intptr_t *)(frame_ + 0x68);   /* ebp-0x9c */
    intptr_t *const v398__f = (intptr_t *)(frame_ + 0x6c);   /* ebp-0x98 */
    intptr_t *const v399__f = (intptr_t *)(frame_ + 0x70);   /* ebp-0x94 */
    intptr_t *const v400__f = (intptr_t *)(frame_ + 0x74);   /* ebp-0x90 */
    intptr_t *const v401__f = (intptr_t *)(frame_ + 0x78);   /* ebp-0x8c */
    intptr_t *const v402__f = (intptr_t *)(frame_ + 0x7c);   /* ebp-0x88 */
    intptr_t *const v403__f = (intptr_t *)(frame_ + 0x80);   /* ebp-0x84 */
    intptr_t *const v404__f = (intptr_t *)(frame_ + 0x84);   /* ebp-0x80 */
    intptr_t *const v405__f = (intptr_t *)(frame_ + 0x88);   /* ebp-0x7c */
    intptr_t *const v406__f = (intptr_t *)(frame_ + 0x8c);   /* ebp-0x78 */
    intptr_t *const v407__f = (intptr_t *)(frame_ + 0x90);   /* ebp-0x74 */
    intptr_t *const v408__f = (intptr_t *)(frame_ + 0x94);   /* ebp-0x70 */
    intptr_t *const v409__f = (intptr_t *)(frame_ + 0x98);   /* ebp-0x6c */
    intptr_t *const v410__f = (intptr_t *)(frame_ + 0x9c);   /* ebp-0x68 */
    intptr_t *const v411__f = (intptr_t *)(frame_ + 0xa0);   /* ebp-0x64 */
    intptr_t *const v412__f = (intptr_t *)(frame_ + 0xa4);   /* ebp-0x60 */
    intptr_t *const v413__f = (intptr_t *)(frame_ + 0xa8);   /* ebp-0x5c */
    intptr_t *const v414__f = (intptr_t *)(frame_ + 0xac);   /* ebp-0x58 */
    intptr_t *const v415__f = (intptr_t *)(frame_ + 0xb0);   /* ebp-0x54 */
    intptr_t *const v416__f = (intptr_t *)(frame_ + 0xb4);   /* ebp-0x50 */
    intptr_t *const v417__f = (intptr_t *)(frame_ + 0xb8);   /* ebp-0x4c */
    intptr_t *const v418__f = (intptr_t *)(frame_ + 0xbc);   /* ebp-0x48 */
    intptr_t *const v419__f = (intptr_t *)(frame_ + 0xc0);   /* ebp-0x44 */
    intptr_t *const v420__f = (intptr_t *)(frame_ + 0xc4);   /* ebp-0x40 */
    intptr_t *const v421__f = (intptr_t *)(frame_ + 0xc8);   /* ebp-0x3c */
    intptr_t *const v422__f = (intptr_t *)(frame_ + 0xcc);   /* ebp-0x38 */
    intptr_t *const v423__f = (intptr_t *)(frame_ + 0xd0);   /* ebp-0x34 */
    intptr_t *const v424__f = (intptr_t *)(frame_ + 0xd4);   /* ebp-0x30 */
    intptr_t *const v425__f = (intptr_t *)(frame_ + 0xd8);   /* ebp-0x2c */
    intptr_t *const v426__f = (intptr_t *)(frame_ + 0xdc);   /* ebp-0x28 */
    intptr_t *const v427__f = (intptr_t *)(frame_ + 0xe0);   /* ebp-0x24 */
    intptr_t *const v428__f = (intptr_t *)(frame_ + 0xe4);   /* ebp-0x20 */
    intptr_t *const v429__f = (intptr_t *)(frame_ + 0xe8);   /* ebp-0x1c */
    intptr_t *const v430__f = (intptr_t *)(frame_ + 0xec);   /* ebp-0x18 */
    intptr_t *const v431__f = (intptr_t *)(frame_ + 0xf0);   /* ebp-0x14 */
    intptr_t *const v432__f = (intptr_t *)(frame_ + 0xf4);   /* ebp-0x10 */
    intptr_t *const v433__f = (intptr_t *)(frame_ + 0xf8);   /* ebp-0xc */
    intptr_t *const v434__f = (intptr_t *)(frame_ + 0xfc);   /* ebp-0x8 */
    intptr_t *const v435__f = (intptr_t *)(frame_ + 0x100);   /* ebp-0x4 */

  F8(a3, 0) = F8(a2, 0);
  F32(a3, 4) = F32(a2, 4);
  F32(a3, 8) = F32(a2, 8);
  F32(a3, 12) = F32(a2, 12);
  F32(a3, 16) = F32(a2, 16);
  F32(a3, 20) = 0;
  F32(a3, 24) = F32(a2, 24);
  F32(a3, 28) = 0;
  F8(a3, 32) = F8(a2, 32);
  F32(a3, 36) = F32(a2, 36);
  F32(a3, 40) = F32(a2, 40);
  F32(a3, 44) = F32(a2, 44);
  F32(a3, 48) = F32(a2, 48);
  F32(a3, 52) = F32(a2, 52);
  F16(a3, 56) = F16(a2, 56);
  F16(a3, 58) = F16(a2, 58);
  F16(a3, 60) = F16(a2, 60);
  F32(a3, 64) = F32(c, 0x30);
  F32(a3, 68) = F32(c, 0x34);
  F16(a3, 72) = F16(a2, 72);
  F32(a3, 76) = F32(a2, 76);
  F32(a3, 80) = F32(a2, 80);
  F16(a3, 84) = F16(a2, 84);
  F16(a3, 86) = F16(a2, 86);
  F16(a3, 88) = F16(a2, 88);
  F16(a3, 90) = F16(a2, 90);
  F32(a3, 92) = F32(a2, 92);
  F32(a3, 96) = F32(a2, 96);
  F16(a3, 100) = F16(a2, 100);
  F16(a3, 102) = F16(a2, 102);
  F32(a3, 108) = F32(a2, 108);
  F16(a3, 112) = F16(a2, 112);
  F16(a3, 114) = F16(a2, 114);
  F8(a3, 116) = F8(a2, 116);
  F32(a3, 120) = F32(a2, 120);
  F8(a3, 124) = 0;
  F8(a3, 125) = 0;
  F8(a3, 126) = 0;
  F8(a3, 127) = 0;
  F16(a3, 128) = 0;
  F32(a3, 644) = F32(a2, 644);
  F32(a3, 648) = F32(a2, 648);
  F32(a3, 652) = F32(a2, 652);
  if ( F32(c, 0x0) != 0 )
    W_char_copy_rects(c, a3);
  v5 = F32(c, 0x10);
  if ( v5 != 0 )
  {
    v6 = WORD2(errno_decorate);
    if ( WORD2(errno_decorate) >= F16(v5, 68) )
      v6 = 0;
    v7 = F32(v5, 64) + 660 * v6;
    F32(a3, 4) = F32(c, 0x14) + F32(v7, 4) + F32(v7, 12) / 2;
    F32(a3, 8) = F32(c, 0x18) + W_part_center_y(F32(c, 0x10), WORD2(errno_decorate));
  }
  SETLO(v8, F16(a3, 72) - 1);
  if ( F16(a3, 72) == 1 )
  {
    SETLO(v8, F16(a3, 60));
    if ( (uint16_t)v8 != 0 )
    {
      SETLO(v8, v8 - 1);
      F16(a3, 60) = v8;
    }
    else
    {
      v9 = F32(c, 0x250);
      if ( v9 != 0 )
      {
        v10 = WORD2(errno_decorate);
        if ( WORD2(errno_decorate) >= F16(v9, 68) )
          v10 = 0;
        v11 = F32(c, 0x248) + F32(F32((v9 + 64), 0) + 660 * v10, 4);
        v12 = WORD2(errno_decorate);
        F32(a3, 4) = v11;
        v13 = F32(c, 0x250);
        if ( WORD2(errno_decorate) >= F16(v13, 68) )
          v12 = 0;
        v14 = F32(v13, 64);
        v15 = 165 * v12;
        v16 = F32(v14 + 4 * v15, 20);
        v17 = F32(v14 + 4 * v15, 8);
        v8 = F32(c, 0x24c);
        F32(a3, 8) = v8 + v17 + v16;
      }
      F8(a3, 0) = 1;
      F16(a3, 72) = 0;
      F16(a3, 86) = 210;
      F16(a3, 88) = 0;
      F32(a3, 92) = 0;
      F32(a3, 120) = 0;
      F16(a3, 84) = 0;
      F32(a3, 76) = 0;
      F32(a3, 80) = 0;
      F16(a3, 90) = 0;
      F16(a3, 100) = 0;
    }
  }
  if ( F8(a3, 0) != 0 )
  {
    switch ( F32(c, 0x1d4) )
    {
      case 0x186A1:
        v326 = F16(a3, 88);
        v327 = (_DWORD *)errno_decorate;
        switch ( v326 )
        {
          case 0:
            v328 = rnd();
            v327 = (_DWORD *)errno_decorate;
            W_sfx_play_at(c, errno_decorate, 87, -1000, 1000 * v328 / 0x8000 - 2000, 0);
            ++F16(a3, 88);
            break;
          case 1:
          case 3:
            v329 = F32(a3, 92) + 1;
            F32(a3, 92) = v329;
            if ( v329 > 14 * F32(c, 0x204) / 1000 )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v326 + 1;
            }
            v327 = (_DWORD *)errno_decorate;
            break;
          case 2:
            v331 = F32(a3, 92) + 1;
            F32(a3, 92) = v331;
            if ( v331 > 16 * F32(c, 0x204) / 1000 )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v326 + 1;
              F32(a3, 40) = 0;
            }
            v327 = (_DWORD *)errno_decorate;
            shot_476ef0(c, a3, errno_decorate, 0, 7, 1, 1, 0, 7, 0, 0, 0, 0);
            break;
          case 4:
            goto LABEL_503;
          default:
            break;
        }
        if ( F16(a3, 88) < 3u )
        {
          v330 = F32(a3, 40) + 2000;
          if ( v330 > 96000 )
            v330 = 96000;
          F32(a3, 40) = v330;
        }
        shot_476ba0((_DWORD *)a3, 0);
        switch ( F16(a3, 88) )
        {
          case 1:
            goto LABEL_761;
          case 2:
            shot_477310(c, 100044, (_DWORD *)a3, v327, 0, 0, 10, 0);
            shot_477310(c, 100044, (_DWORD *)a3, v327, 90, 16000, 0, 0);
            shot_477310(c, 100044, (_DWORD *)a3, v327, 90, 32000, 0, 0);
            break;
          case 3:
LABEL_761:
            (*v363__f) = 2000 - 4000 * rnd() / 0x8000;
            v332 = rnd();
            v333 = W_particle_spawn(
                                   c,
                                   (int)c,
                                   100044,
                                   (_DWORD *)a3,
                                   1600 - 3200 * v332 / 0x8000,
                                   (*v363__f),
                                   99,
                                   1,
                                   (int)v327,
                                   0,
                                   2,
                                   0,
                                   0,
                                   0);
            if ( v333 != 0 )
              W_particle_tint_set(v333, 12);
            break;
          default:
            break;
        }
        goto LABEL_763;
      case 0x186A2:
        v321 = F16(a3, 88);
        if ( (uint16_t)v321 != 0 )
        {
          if ( v321 == 1 )
          {
            shot_477310(c, 100044, (_DWORD *)a3, (_DWORD *)errno_decorate, 0, 0, 10, 0);
            shot_477310(c, 100044, (_DWORD *)a3, (_DWORD *)errno_decorate, 90, 16000, 0, 0);
            shot_477310(c, 100044, (_DWORD *)a3, (_DWORD *)errno_decorate, 90, 32000, 0, 0);
            shot_476ef0(c, a3, errno_decorate, 0, 7, 1, 1, 0, 7, 0, 0, 0, 0);
          }
        }
        else
        {
          v322 = rnd();
          W_sfx_play_at(c, errno_decorate, 87, -1000, 1000 * v322 / 0x8000 - 2000, 0);
          ++F16(a3, 88);
        }
        v305 = F32(a3, 40) + 2000;
        if ( v305 > 96000 )
          v305 = 96000;
        goto LABEL_671;
      case 0x186A3:
        v300 = F16(a3, 88);
        if ( v300 != 0 )
        {
          if ( F16(a3, 88) == 1 )
          {
            v301 = F32(a3, 92) + 1;
            F32(a3, 92) = v301;
            if ( v301 > 5 )
              F16(a3, 88) = v300 + 1;
          }
          else if ( F16(a3, 88) == 2 )
          {
            shot_476ef0(c, a3, errno_decorate, 1, 12, 1, 1, 0, 0, 0, 0, 0, 0);
          }
        }
        else
        {
          v302 = rnd();
          W_sfx_play_at(c, errno_decorate, 84, 1000, 500 * v302 / 0x8000, 0);
          ++F16(a3, 88);
        }
        (*v357__f) = 1600 * rnd() / 0x8000 - 800;
        v303 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100043,
          (_DWORD *)a3,
          2400 * v303 / 0x8000 - 1200,
          (*v357__f),
          99,
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          0);
        (*v358__f) = 1600 * rnd() / 0x8000 - 800;
        v304 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100043,
          (_DWORD *)a3,
          2400 * v304 / 0x8000 - 1200,
          (*v358__f),
          99,
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          0);
        v305 = F32(a3, 40) + 6000;
        if ( v305 > 100000 )
          v305 = 100000;
LABEL_671:
        F32(a3, 40) = v305;
        shot_477070(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186A4:
        v161 = F16(a3, 88);
        v162 = errno_decorate;
        switch ( v161 )
        {
          case 0:
            v163 = 1000 * rnd();
            (*v387__f) = (_DWORD *)F32(c, 0x40);
            if ( (int32_t)errno_decorate == 0 )
            {
              v164 = (intptr_t *)W_vpool_take(SYS32(1200), 0);
              if ( v164 != 0 )
              {
                v165 = F32(v164, 0);
                if ( W_sfx_place(
                       c,
                       F32(CFG(), 360),
                       0,
                       0,
                       (*v387__f),
                       v390,
                       (int *)&(*v388__f)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v165, v390[0]);
                  g_eng.dsb_pan_5bb880(v165, (int)(*v388__f));
                  g_eng.dsb_freq_5bb890(v165, v163 / 0x8000 + 500);
                  g_eng.dsb_play_5bb8c0(v165);
                }
                v162 = errno_decorate;
              }
            }
            (*v387__f) = (_DWORD *)(1000 * rnd() / 0x8000 + 500);
            if ( v162 == 0 && F32(c, 0x1b0) != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
              {
                if ( W_sfx_place(c, 1000, 0, 0, (_DWORD *)F32(c, 0x40), 0, 0) != 0 )
                {
                  v166 = (intptr_t **)F32(c, 0x1b0);
                  (*v388__f) = SYS32(1104);
                  if ( F32(v166, 0) != 0 )
                    W_entity_snd_release(c, 0, 0, 0);
                  v167 = (intptr_t *)W_vpool_take((*v388__f), 1);
                  if ( v167 != 0 )
                  {
                    v168 = (*v388__f);
                    v169 = (intptr_t *)(*v387__f);
                    *v166 = v167;
                    v166[1] = (intptr_t *)v168;
                    v166[2] = (intptr_t *)1000;
                    v166[3] = v169;
                    v166[4] = 0;
                    v166[5] = (intptr_t *)1;
                    v166[6] = 0;
                    v166[7] = 0;
                    g_eng.dsb_volume_5bb870(F32(v167, 0), -10000);
                    g_eng.dsb_play_5bb8c0(*v167);
                  }
                }
              }
              else
              {
                g_eng.log_fail_406440(g_eng.log, (const char *)D(0x85525c), (const char *)D(0x8a9b6c));
              }
            }
            ++F16(a3, 88);
            v162 = errno_decorate;
            goto LABEL_371;
          case 1:
LABEL_371:
            v170 = F32(a3, 92);
            if ( v170 >= 24 )
            {
              ++F16(a3, 88);
              F32(a3, 92) = 0;
            }
            else
            {
              F32(a3, 16) += 400;
              F32(a3, 12) += 400;
              F32(a3, 4) -= 200;
              v171 = v170 + 1;
              F32(a3, 8) -= 200;
LABEL_378:
              F32(a3, 92) = v171;
            }
LABEL_379:
            if ( v162 == 0 )
            {
              v174 = F32(c, 0x1b0);
              if ( v174 != 0 )
              {
                if ( F16(c, 0x1b4) != 0 )
                  F32(v174, 20) = 1;
                else
                  g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
              }
            }
            v175 = F32(a3, 52);
            if ( F32(a3, 44) == 3 )
              v176 = v175 + 1000;
            else
              v176 = v175 - 1000;
            F32(a3, 52) = v176;
            if ( F16(a3, 88) == 1 || F16(a3, 88) == 3 )
            {
              (*v387__f) = F32(a3, 92);
              (*v399__f) = 63;
              (*v401__f) = 63;
              (*v403__f) = 63;
              (*v404__f) = 26;
              (*v406__f) = 26;
              (*v408__f) = 113;
              (*v410__f) = 113;
              (*v396__f) = 69;
              (*v411__f) = 69;
              (*v395__f) = 64;
              (*v400__f) = 64;
              (*v418__f) = 78;
              (*v422__f) = 78;
              v183 = 0;
              (*v392__f) = 67;
              (*v393__f) = 17;
              (*v394__f) = 130;
              (*v397__f) = 109;
              (*v398__f) = 75;
              (*v402__f) = 44;
              (*v405__f) = 55;
              (*v407__f) = 74;
              (*v409__f) = 59;
              (*v412__f) = 22;
              (*v413__f) = 3;
              (*v414__f) = 55;
              (*v415__f) = 8;
              (*v416__f) = 73;
              (*v417__f) = 25;
              (*v419__f) = 49;
              (*v420__f) = 96;
              (*v421__f) = 64;
              (*v423__f) = 79;
              (*v424__f) = 75;
              (*v425__f) = 98;
              (*v426__f) = 60;
              (*v427__f) = 116;
              (*v428__f) = 29;
              (*v429__f) = 125;
              (*v430__f) = 77;
              (*v431__f) = 39;
              (*v432__f) = 77;
              (*v433__f) = 88;
              (*v434__f) = 0;
              (*v435__f) = 0;
              if ( v162 == 0 )
              {
                while ( 1 )
                {
                  v184 = *(&(*v392__f) + 2 * v183);
                  if ( v184 == 0 && *(&(*v393__f) + 2 * v183) == 0 )
                    break;
                  shot_476550(100 * (v184 - 64), 100 * ((int)(*v387__f) * *(&(*v393__f) + 2 * v183) / 24 - 64), v390, &(*v391__f));
                  v185 = W_angle_wrap((int64_t)((*v391__f) * 1000.0 / (*(const double *)D(0x8a9a38))), -90000, 1000);
                  v186 = W_angle_wrap(v185, F32(a3, 52), 1000);
                  W_polar_div(v390[0], v186, 1000, 0, 0, &(*v389__f), &(*v388__f));
                  (*v384__f) = F32(a3, 52) / 1000;
                  (*v368__f) = F32(a3, 44);
                  (*v343__f) = ((*v388__f) + (-20) * 0x14) + 800 * rnd() / 0x8000;
                  v187 = rnd();
                  W_particle_spawn(
                    c,
                    (int)c,
                    100063,
                    (_DWORD *)a3,
                    800 * v187 / 0x8000 + (*v389__f) - 400,
                    (int)(*v343__f),
                    (*v368__f),
                    1,
                    0,
                    0,
                    2,
                    0,
                    (*v384__f),
                    v183 < 4u ? 0xA : 0);
                  (*v385__f) = (unsigned int)(F32(a3, 52) / 1000);
                  (*v369__f) = F32(a3, 44);
                  (*v344__f) = ((*v388__f) + (-20) * 0x14) + 800 * rnd() / 0x8000;
                  v188 = rnd();
                  W_particle_spawn(
                    c,
                    (int)c,
                    100063,
                    (_DWORD *)a3,
                    800 * v188 / 0x8000 + (*v389__f) - 400,
                    (int)(*v344__f),
                    (*v369__f),
                    1,
                    0,
                    0,
                    2,
                    0,
                    (*v385__f),
                    SHIDWORD((*v385__f)));
                  ++v183;
                }
              }
            }
            else
            {
              (*v399__f) = 63;
              (*v401__f) = 63;
              (*v403__f) = 63;
              (*v404__f) = 26;
              (*v406__f) = 26;
              (*v408__f) = 113;
              (*v410__f) = 113;
              (*v396__f) = 69;
              (*v411__f) = 69;
              (*v395__f) = 64;
              (*v400__f) = 64;
              (*v418__f) = 78;
              (*v422__f) = 78;
              v177 = 0;
              (*v392__f) = 67;
              (*v393__f) = 17;
              (*v394__f) = 130;
              (*v397__f) = 109;
              (*v398__f) = 75;
              (*v402__f) = 44;
              (*v405__f) = 55;
              (*v407__f) = 74;
              (*v409__f) = 59;
              (*v412__f) = 22;
              (*v413__f) = 3;
              (*v414__f) = 55;
              (*v415__f) = 8;
              (*v416__f) = 73;
              (*v417__f) = 25;
              (*v419__f) = 49;
              (*v420__f) = 96;
              (*v421__f) = 64;
              (*v423__f) = 79;
              (*v424__f) = 75;
              (*v425__f) = 98;
              (*v426__f) = 60;
              (*v427__f) = 116;
              (*v428__f) = 29;
              (*v429__f) = 125;
              (*v430__f) = 77;
              (*v431__f) = 39;
              (*v432__f) = 77;
              (*v433__f) = 88;
              (*v434__f) = 0;
              (*v435__f) = 0;
              if ( v162 == 0 )
              {
                while ( 1 )
                {
                  v178 = *(&(*v392__f) + 2 * v177);
                  if ( v178 == 0 && *(&(*v393__f) + 2 * v177) == 0 )
                    break;
                  shot_476550(100 * (v178 - 64), 100 * (*(&(*v393__f) + 2 * v177) - 64), &(*v387__f), (double *)v390);
                  v179 = W_angle_wrap((int64_t)(*(double *)v390 * 1000.0 / (*(const double *)D(0x8a9a38))), -90000, 1000);
                  v180 = W_angle_wrap(v179, F32(a3, 52), 1000);
                  W_polar_div((int)(*v387__f), v180, 1000, 0, 0, &(*v389__f), &(*v388__f));
                  (*v382__f) = F32(a3, 52) / 1000;
                  (*v366__f) = F32(a3, 44);
                  (*v341__f) = ((*v388__f) + (-20) * 0x14) + 800 * rnd() / 0x8000;
                  v181 = rnd();
                  W_particle_spawn(
                    c,
                    (int)c,
                    100063,
                    (_DWORD *)a3,
                    800 * v181 / 0x8000 + (*v389__f) - 400,
                    (int)(*v341__f),
                    (*v366__f),
                    1,
                    0,
                    0,
                    2,
                    0,
                    (*v382__f),
                    v177 < 4u ? 0xA : 0);
                  (*v383__f) = (unsigned int)(F32(a3, 52) / 1000);
                  (*v367__f) = F32(a3, 44);
                  (*v342__f) = ((*v388__f) + (-20) * 0x14) + 800 * rnd() / 0x8000;
                  v182 = rnd();
                  W_particle_spawn(
                    c,
                    (int)c,
                    100063,
                    (_DWORD *)a3,
                    800 * v182 / 0x8000 + (*v389__f) - 400,
                    (int)(*v342__f),
                    (*v367__f),
                    1,
                    0,
                    0,
                    2,
                    0,
                    (*v383__f),
                    SHIDWORD((*v383__f)));
                  ++v177;
                }
              }
            }
            if ( W_body_hit_spawn(
                   c,
                   F32(c, 0x20c),
                   a3,
                   0,
                   0,
                   F32(a3, 12),
                   F32(a3, 16),
                   7,
                   errno_decorate,
                   1,
                   F32(c, 0x234),
                   F32(c, 0x238),
                   F32(c, 0x23c),
                   8,
                   F32(c, 0x200),
                   1,
                   1,
                   F32(c, 0x220),
                   1,
                   F32(c, 0x244),
                   F32(c, 0x224),
                   0,
                   0,
                   0) != 0 )
            {
              v189 = F32(c, 0x22c) + 1;
              if ( v189 > 100000 )
                v189 = 100000;
              v190 = F32(c, 0x228);
              F32(c, 0x22c) = v189;
              if ( v190 != 0 && v189 >= v190 )
                W_shot_end(c, (_BYTE *)a3, errno_decorate);
            }
            shot_476ba0((_DWORD *)a3, 0);
            shot_476e20(c, a3, errno_decorate);
            break;
          case 2:
            v172 = F32(a3, 92) + 1;
            F32(a3, 92) = v172;
            if ( v172 >= 30 )
            {
              F32(a3, 92) = 24;
              F16(a3, 88) = v161 + 1;
            }
            goto LABEL_379;
          case 3:
            v173 = F32(a3, 92);
            if ( v173 <= 0 )
              goto LABEL_379;
            F32(a3, 16) -= 400;
            F32(a3, 12) -= 400;
            F32(a3, 4) += 200;
            v171 = v173 - 1;
            F32(a3, 8) += 200;
            goto LABEL_378;
          default:
            goto LABEL_379;
        }
        goto LABEL_763;
      case 0x186A5:
        shot_477560(c, a3, SHIDWORD(errno_decorate), -56000, -4000, 80000, 4000, 1);
        v234 = errno_decorate;
        v290 = F16(a3, 88);
        if ( (uint16_t)v290 != 0 )
        {
          v291 = v290 - 1;
          if ( v291 != 0 )
          {
            if ( v291 == 1 )
            {
              v292 = F32(a3, 40) - 1000;
              if ( v292 < 20000 )
                v292 = 20000;
LABEL_646:
              F32(a3, 40) = v292;
            }
            if ( (int32_t)errno_decorate == 0 )
            {
              v295 = F16(a3, 60);
              if ( v295 >= 0x28u )
              {
                v297 = F16(c, 0x1fc) - F16(a3, 60);
                if ( v297 > 19 )
                {
                  v297 = 19;
                }
                else if ( v297 < 0 )
                {
                  v297 = 0;
                }
                v296 = SYS32(4 * v297 + 10072);
              }
              else if ( v295 <= 0x13u )
              {
                v296 = SYS32(4 * F16(a3, 60) + 10072);
              }
              else
              {
                v296 = SYS32(10148);
              }
              F32(c, 0xf4) = v296;
              F32(c, 0xf8) = v296;
            }
            (*v355__f) = 12000 * rnd() / 0x8000 - 4000;
            v298 = rnd();
            shot_4504c0(
              c,
              16000,
              (int)c,
              100117,
              (_DWORD *)a3,
              6000 * v298 / 0x8000 + 3000,
              (*v355__f),
              99,
              1,
              errno_decorate,
              0,
              2,
              0,
              0,
              0);
            (*v356__f) = 12000 * rnd() / 0x8000 - 4000;
            v299 = rnd();
            shot_4504c0(
              c,
              16000,
              (int)c,
              100118,
              (_DWORD *)a3,
              6000 * v299 / 0x8000 - 7000,
              (*v356__f),
              99,
              1,
              errno_decorate,
              0,
              2,
              0,
              0,
              0);
            W_entity_snd_keep(c, 0, errno_decorate);
            if ( shot_476ef0(c, a3, errno_decorate, 1, 17, 1, 1, 1, 7, 6400, 0, 12800, 0) != 0 )
              F32(a3, 40) = 0;
LABEL_545:
            shot_477070(c, a3, v234);
            goto LABEL_763;
          }
        }
        else
        {
          v293 = rnd();
          W_entity_snd_start(c, 0, errno_decorate, 0x67u, 0, 500 * v293 / 0x8000, 0, 0);
          ++F16(a3, 88);
        }
        v294 = F32(a3, 92) + 1;
        F32(a3, 92) = v294;
        if ( v294 > 80 )
          F16(a3, 88) = 2;
        v292 = F32(a3, 40) + 1000;
        if ( v292 > 56000 )
          v292 = 56000;
        goto LABEL_646;
      case 0x186A6:
        v285 = F16(a3, 88);
        if ( (uint16_t)v285 != 0 )
        {
          v286 = v285 - 1;
          if ( v286 != 0 )
          {
            if ( v286 == 1 )
              shot_476ef0(c, a3, errno_decorate, 1, 17, 1, 1, 0, 7, 0, 0, 0, 0);
            goto LABEL_629;
          }
        }
        else
        {
          v287 = rnd();
          W_sfx_play_at(c, errno_decorate, 102, 1000, 500 * v287 / 0x8000, 0);
          ++F16(a3, 88);
        }
        if ( F16(a3, 60) > 5u )
          ++F16(a3, 88);
LABEL_629:
        v288 = F32(a3, 52);
        if ( v288 < 90000 || v288 > 270000 )
        {
          (*v373__f) = F32(a3, 44);
          (*v354__f) = 800 - 1600 * rnd() / 0x8000;
          (*v337__f) = 400 - 800 * rnd() / 0x8000;
          shot_4504c0(c, 16000, (int)c, 100120, (_DWORD *)a3, (*v337__f), (*v354__f), (*v373__f), 1, errno_decorate, 0, 2, 0, 0, 0);
        }
        else
        {
          (*v372__f) = F32(a3, 44);
          (*v353__f) = 800 - 1600 * rnd() / 0x8000;
          (*v336__f) = 400 - 800 * rnd() / 0x8000;
          shot_4504c0(c, 16000, (int)c, 100119, (_DWORD *)a3, (*v336__f), (*v353__f), (*v372__f), 1, errno_decorate, 0, 2, 0, 0, 0);
        }
        shot_477070(c, a3, errno_decorate);
        v289 = F32(a3, 40) + 2000;
        if ( v289 > 80000 )
          v289 = 80000;
        F32(a3, 40) = v289;
        goto LABEL_763;
      case 0x186A7:
        shot_477560(c, a3, SHIDWORD(errno_decorate), -20000, -2000, 80000, 4000, F16(a3, 88) < 3u);
        v271 = F16(a3, 88);
        switch ( v271 )
        {
          case 0:
            v272 = rnd();
            W_sfx_play_at(c, errno_decorate, 83, 0, 1000 * v272 / 0x8000 - 500, 0);
            ++F16(a3, 88);
            break;
          case 1:
            v273 = F32(a3, 92);
            F32(a3, 40) = 60000;
            F32(a3, 92) = v273 + 1;
            if ( v273 + 1 > 20 )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v271 + 1;
            }
            break;
          case 2:
            if ( F32(a3, 92) > 40 )
              F16(a3, 88) = v271 + 1;
            goto LABEL_598;
          case 3:
LABEL_598:
            v274 = F32(a3, 92);
            F32(a3, 40) = 0;
            v275 = v274 + 1;
            F32(a3, 92) = v275;
            if ( (int32_t)errno_decorate == 0 && v275 % 40 == 0 )
            {
              v276 = rnd();
              W_sfx_play_at(c, 0, 227, 0, 1000 * v276 / 0x8000 - 500, 0);
            }
            break;
          default:
            break;
        }
        v277 = F32(a3, 16);
        if ( v277 < 14400 )
        {
          F32(a3, 16) = v277 + 400;
          F32(a3, 8) -= 400;
        }
        if ( F32(a3, 40) != 0 )
          shot_476ef0(c, a3, errno_decorate, 1, 8, 1, 1, 1, 7, 0, 0, 0, 0);
        else
          shot_476ef0(c, a3, errno_decorate, 1, 8, 1, 1, 1, 8, 0, 0, 0, 0);
        (*v349__f) = 3200 * rnd() / 0x8000 - 1600;
        v278 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100064,
          (_DWORD *)a3,
          3200 * v278 / 0x8000 - 1600,
          (*v349__f),
          99,
          4,
          errno_decorate,
          0,
          2,
          0,
          0,
          0);
        if ( F32(a3, 16) > 9600 && F32(CFG(), 148) != 0 )
        {
          (*v350__f) = 3200 * rnd() / 0x8000 - 1600;
          v279 = rnd();
          shot_4504c0(
            c,
            16000,
            (int)c,
            100064,
            (_DWORD *)a3,
            3200 * v279 / 0x8000 - 1600,
            (*v350__f),
            99,
            4,
            errno_decorate,
            0,
            2,
            0,
            0,
            0);
        }
        goto LABEL_682;
      case 0x186A8:
        v233 = F16(a3, 88);
        v234 = errno_decorate;
        switch ( v233 )
        {
          case 0:
            v235 = rnd();
            W_sfx_play_at(c, errno_decorate, 82, -500, 1000 * v235 / 0x8000 + 500, 0);
            ++F16(a3, 88);
            break;
          case 1:
            v236 = F32(a3, 92) + 1;
            F32(a3, 92) = v236;
            v237 = v236;
            v238 = F32(a3, 40) + 4000;
            if ( v238 > 80000 )
              v238 = 80000;
            F32(a3, 40) = v238;
            if ( v237 >= 20 )
            {
              F16(a3, 88) = v233 + 1;
              F32(a3, 92) = 0;
              F32(a3, 40) = 0;
            }
            break;
          case 2:
            v239 = F32(a3, 92) + 1;
            F32(a3, 92) = v239;
            if ( v239 >= F32(c, 0x204) )
            {
              F16(a3, 88) = v233 + 1;
              F32(a3, 92) = 0;
              v240 = rnd();
              W_sfx_play_at(c, errno_decorate, 83, 0, 1000 * v240 / 0x8000 + 500, 0);
              F32(a3, 40) = 80000;
              W_body_set_angle(a3, 90000, 1);
              if ( (int32_t)errno_decorate == 0 && F32(c, 0x240) == 0 )
              {
                v241 = shot_54cca0(c, 48000, 40, 1, 1, 0);
                F32(c, 0x240) = v241;
                if ( v241 == 0 )
                  F32(c, 0x240) = shot_54cca0(c, 72000, 60, 1, 1, 0);
              }
            }
            break;
          case 3:
            v242 = F32(c, 0x240);
            if ( v242 != 0 )
            {
              v243 = shot_478820((_DWORD *)a3, WORD2(errno_decorate), v242, 0, 0);
              if ( (int)abs32(v243) > 2000 )
              {
                if ( v243 >= 180000 )
                  v244 = W_angle_wrap(F32(a3, 52), -2000, 1000);
                else
                  v244 = W_angle_wrap(F32(a3, 52), 2000, 1000);
                F32(a3, 52) = v244;
              }
            }
            v245 = F32(a3, 92) + 1;
            F32(a3, 92) = v245;
            if ( v245 >= 40 )
            {
              ++F16(a3, 88);
              F32(a3, 92) = 0;
            }
            goto LABEL_538;
          case 4:
LABEL_538:
            v246 = F32(a3, 40) + 4000;
            if ( v246 > 80000 )
              v246 = 80000;
            F32(a3, 40) = v246;
            shot_476ef0(c, a3, errno_decorate, 1, 8, 1, 1, 0, 7, 0, 0, 0, 0);
            break;
          default:
            break;
        }
        if ( F16(a3, 88) >= 3u )
        {
          v247 = rnd();
          shot_477310(c, 100062, (_DWORD *)a3, (_DWORD *)errno_decorate, 0, 0, 10, 5000 - 10000 * v247 / 0x8000);
          v248 = rnd();
          shot_477310(c, 100062, (_DWORD *)a3, (_DWORD *)errno_decorate, 90, 20000, 0, 5000 - 10000 * v248 / 0x8000);
          v249 = rnd();
          shot_477310(c, 100062, (_DWORD *)a3, (_DWORD *)errno_decorate, 90, 40000, 0, 5000 - 10000 * v249 / 0x8000);
          shot_477070(c, a3, errno_decorate);
          goto LABEL_763;
        }
        (*v370__f) = F32(a3, 44);
        (*v347__f) = 2000 * rnd() / 0x8000 - 1000;
        v250 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100056,
          (_DWORD *)a3,
          2000 * v250 / 0x8000 - 1000,
          (*v347__f),
          (*v370__f),
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          10);
        if ( F32(CFG(), 148) != 0 )
        {
          (*v371__f) = F32(a3, 44);
          (*v348__f) = 2000 * rnd() / 0x8000 - 1000;
          v251 = rnd();
          shot_4504c0(
            c,
            16000,
            (int)c,
            100056,
            (_DWORD *)a3,
            2000 * v251 / 0x8000 - 1000,
            (*v348__f),
            (*v371__f),
            1,
            errno_decorate,
            0,
            2,
            0,
            0,
            0);
        }
        goto LABEL_545;
      case 0x186A9:
        v219 = errno_decorate;
        v220 = F16(a3, 88);
        ++F32(a3, 92);
        switch ( v220 )
        {
          case 0:
            (*v387__f) = (_DWORD *)(2000 * rnd() / 0x8000 - 1000);
            if ( (int32_t)errno_decorate == 0 && F32(c, 0x1b0) != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
              {
                if ( W_sfx_place(c, -500, 0, 0, (_DWORD *)F32(c, 0x40), 0, 0) != 0 )
                {
                  v221 = (intptr_t **)F32(c, 0x1b0);
                  (*v388__f) = SYS32(1320);
                  if ( F32(v221, 0) != 0 )
                    W_entity_snd_release(c, 0, 0, 0);
                  v222 = (intptr_t *)W_vpool_take((*v388__f), 1);
                  if ( v222 != 0 )
                  {
                    v223 = (intptr_t *)(*v387__f);
                    v221[1] = (intptr_t *)(*v388__f);
                    *v221 = v222;
                    v221[2] = (intptr_t *)-500;
                    v221[3] = v223;
                    v221[4] = 0;
                    v221[5] = (intptr_t *)1;
                    v221[6] = 0;
                    v221[7] = 0;
                    g_eng.dsb_volume_5bb870(F32(v222, 0), -10000);
                    g_eng.dsb_play_5bb8c0(*v222);
                  }
                }
              }
              else
              {
                g_eng.h408b90(D(0x8a9530), D(0x85525c), 0, D(0x8a9b6c));
              }
            }
            ++F16(a3, 88);
            v219 = errno_decorate;
            goto LABEL_483;
          case 1:
LABEL_483:
            if ( F32(a3, 92) > 10 )
              ++F16(a3, 88);
            goto LABEL_485;
          case 2:
            if ( W_body_hit_spawn(
                   c,
                   F32(c, 0x20c),
                   a3,
                   0,
                   0,
                   F32(a3, 12),
                   F32(a3, 16),
                   7,
                   errno_decorate,
                   1,
                   F32(c, 0x234),
                   F32(c, 0x238),
                   F32(c, 0x23c),
                   8,
                   F32(c, 0x200),
                   1,
                   1,
                   F32(c, 0x220),
                   0,
                   F32(c, 0x244),
                   F32(c, 0x224),
                   0,
                   0,
                   0) == 0 )
              goto LABEL_485;
            v225 = F32(c, 0x22c) + 1;
            if ( v225 > 100000 )
              v225 = 100000;
            v226 = F32(c, 0x228);
            F32(c, 0x22c) = v225;
            if ( v226 != 0 && v225 >= v226 )
              W_shot_end(c, (_BYTE *)a3, errno_decorate);
            F16(a3, 88) = 3;
LABEL_496:
            v227 = 2000 * rnd();
            (*v387__f) = (_DWORD *)F32(c, 0x40);
            if ( (int32_t)errno_decorate == 0 )
            {
              v228 = (intptr_t *)W_vpool_take(SYS32(1376), 0);
              if ( v228 != 0 )
              {
                v229 = F32(v228, 0);
                if ( W_sfx_place(
                       c,
                       F32(CFG(), 360) + 400,
                       0,
                       0,
                       (*v387__f),
                       v390,
                       (int *)&(*v388__f)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v229, v390[0]);
                  g_eng.dsb_pan_5bb880(v229, (int)(*v388__f));
                  g_eng.dsb_freq_5bb890(v229, v227 / 0x8000 - 1000);
                  g_eng.dsb_play_5bb8c0(v229);
                }
                v219 = errno_decorate;
              }
            }
            W_particle_spawn(c, (int)c, 100090, (_DWORD *)a3, 0, 0, 99, 1, v219, 0, 2, 0, 0, 0);
            if ( v219 == 0 )
              F32(c, 0x254) = 0;
            goto LABEL_503;
          case 3:
            goto LABEL_496;
          default:
LABEL_485:
            if ( v219 == 0 )
            {
              v224 = F32(c, 0x1b0);
              if ( v224 != 0 )
              {
                if ( F16(c, 0x1b4) != 0 )
                  F32(v224, 20) = 1;
                else
                  g_eng.h408b90(D(0x8a9530), D(0x85525c), 0, D(0x8a9b6c));
              }
            }
            v230 = F32(a3, 40) + 6000;
            if ( v230 > 120000 )
              v230 = 120000;
            F32(a3, 40) = v230;
            (*v387__f) = F32(a3, 44);
            v231 = 800 * rnd() / 0x8000 - 400;
            v232 = 800 * rnd() / 0x8000 - 400;
            if ( (int32_t)errno_decorate == 0 && shot_4538f0(*(_DWORD **)&GAP(W, 0x1030, 28), (_DWORD *)a3, 16000) )
              W_particle_spawn(c, (int)c, 100061, (_DWORD *)a3, v232, v231, (int)(*v387__f), 1, 0, 0, 2, 0, 0, 10);
            if ( F32(a3, 92) <= 24 )
            {
              W_polar_div(
                F32(a3, 40) / 100,
                F32(a3, 52),
                1000,
                F32(a3, 4),
                F32(a3, 8),
                (_DWORD *)(a3 + 4),
                (_DWORD *)(a3 + 8));
            }
            else
            {
              W_polar_div(F32(a3, 40) / 100, F32(a3, 52), 1000, 0, 0, &(*v387__f), v390);
              if ( (*v387__f) != 0
                && W_entity_move_horizontal(c, (_DWORD *)a3, (int)(*v387__f), 0, 0, 0, WORD2(errno_decorate), 0) == 0
                || v390[0] != 0
                && W_entity_move_vertical(c, (_DWORD *)a3, v390[0], 0, WORD2(errno_decorate), a3 + 8, 0) == 0 )
              {
                F16(a3, 88) = 3;
              }
            }
            if ( ++F16(a3, 60) > F16(c, 0x1fc) )
              W_shot_end(c, (_BYTE *)a3, errno_decorate);
            goto LABEL_763;
        }
      case 0x186AA:
        v211 = W_part_center_y(c, WORD2(errno_decorate));
        SETLODW((*v391__f), 0);
        v212 = 100072;
        if ( (int32_t)errno_decorate != 0 )
          goto LABEL_763;
        if ( F32(a3, 44) == 1 )
          v213 = F32(a3, 52);
        else
          v213 = 360000 - F32(a3, 52);
        SETHI(v214, 0);
        (*v388__f) = v213;
        SETLO(v214, F16(a3, 88));
        if ( (uint16_t)v214 != 0 )
        {
          if ( v214 != 1 )
            goto LABEL_457;
        }
        else
        {
          v215 = rnd();
          W_entity_snd_start(c, 0, 0, 0x58u, -600, 1000 * v215 / 0x8000 + 500, 0, 0);
          ++F16(a3, 88);
          v213 = (int)(*v388__f);
        }
        v216 = F32(a3, 92);
        if ( v216 >= 5 )
        {
          F32(a3, 92) = 0;
          F16(a3, 88) = v213 >= 90000 ? 12 : 10;
        }
        else
        {
          F32(a3, 92) = v216 + 1;
        }
LABEL_457:
        switch ( F16(a3, 88) )
        {
          case 0xA:
            if ( v211 < F32(c, 0x21c) )
              F16(a3, 88) = 11;
            SETLODW((*v391__f), 2500);
            break;
          case 0xB:
            if ( v211 > F32(c, 0x21c) )
              F16(a3, 88) = 12;
            SETLODW((*v391__f), 2500);
            break;
          case 0xC:
            v212 = 100071;
            if ( v211 > F32(c, 0x21c) )
              F16(a3, 88) = 13;
            goto LABEL_468;
          case 0xD:
            v212 = 100071;
            if ( v211 < F32(c, 0x21c) )
              F16(a3, 88) = 10;
LABEL_468:
            SETLODW((*v391__f), -2500);
            break;
          default:
            break;
        }
        shot_4504c0(
          c,
          16000,
          (int)c,
          v212,
          (_DWORD *)a3,
          0,
          0,
          F32(a3, 44),
          1,
          0,
          0,
          2,
          0,
          F32(a3, 52) / 1000,
          10);
        W_entity_snd_keep(c, 0, 0);
        v217 = F32(a3, 40) + 2000;
        if ( v217 > 96000 )
          v217 = 96000;
        v218 = (*v388__f);
        F32(a3, 40) = v217;
        W_body_set_angle(a3, (int)v218 + LODWORD((*v391__f)), 1);
        shot_476ef0(c, a3, 0, 1, 7, 1, 1, 0, 7, 0, 0, 0, 0);
        shot_477070(c, a3, 0);
        goto LABEL_763;
      case 0x186AB:
        v198 = F16(a3, 88);
        switch ( v198 )
        {
          case 0:
            v199 = rnd();
            W_sfx_play_at(c, errno_decorate, 106, -500, 1000 * v199 / 0x8000 + 500, 0);
            ++F16(a3, 88);
            break;
          case 1:
            v200 = F32(a3, 92) + 1;
            F32(a3, 92) = v200;
            v201 = v200;
            v202 = F32(a3, 40) + 4000;
            if ( v202 > 80000 )
              v202 = 80000;
            F32(a3, 40) = v202;
            if ( v201 >= 20 )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v198 + 1;
              if ( (int32_t)errno_decorate == 0 && F32(c, 0x240) == 0 )
              {
                v203 = F32(a3, 52);
                W_body_set_angle(a3, 90000, 1);
                v204 = shot_54cca0(c, 48000, 45, 1, 1, 0);
                F32(c, 0x240) = v204;
                if ( v204 == 0 )
                  F32(c, 0x240) = shot_54cca0(c, 0, 80, 1, 1, 0);
                F32(a3, 52) = v203;
              }
            }
            break;
          case 2:
            if ( F32(c, 0x240) != 0 )
            {
              v205 = F32(a3, 92) + 1;
              F32(a3, 40) = F32(a3, 40) - 4000 < 0 ? 0 : F32(a3, 40) - 4000;
              F32(a3, 92) = v205;
              if ( v205 >= 40
                || (v206 = shot_478820((_DWORD *)a3, 0, F32(c, 0x240), 0, 0), (int)abs32(v206) <= 5000) )
              {
                v207 = rnd();
                W_sfx_play_at(c, errno_decorate, 106, -500, 1000 * v207 / 0x8000 + 500, 0);
                ++F16(a3, 88);
                F32(a3, 92) = 0;
              }
              else if ( v206 >= 180000 )
              {
                F32(a3, 52) = W_angle_wrap(F32(a3, 52), -5000, 1000);
              }
              else
              {
                F32(a3, 52) = W_angle_wrap(F32(a3, 52), 5000, 1000);
              }
            }
            else
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v198 + 1;
            }
            break;
          case 3:
            v208 = F32(a3, 40) + 2000;
            if ( v208 > 64000 )
              v208 = 64000;
            v209 = F32(a3, 92);
            F32(a3, 40) = v208;
            F32(a3, 92) = v209 + 1;
            if ( v209 + 1 >= 40 )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = 2;
            }
            shot_476e20(c, a3, errno_decorate);
            break;
          default:
            break;
        }
        if ( F16(a3, 88) >= 2u )
          shot_476ef0(c, a3, errno_decorate, 1, 10, 1, 1, 0, 7, 0, 0, 0, 0);
        (*v346__f) = 2000 - 4000 * rnd() / 0x8000;
        v210 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100046,
          (_DWORD *)a3,
          2000 - 4000 * v210 / 0x8000,
          (*v346__f),
          99,
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          0);
        shot_476ba0((_DWORD *)a3, 0);
        goto LABEL_763;
      case 0x186AC:
        v193 = F16(a3, 88);
        if ( v193 != 0 )
        {
          if ( F16(a3, 88) == 1 )
          {
            v194 = F32(a3, 92) + 1;
            F32(a3, 92) = v194;
            if ( v194 >= 20 )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v193 + 1;
            }
          }
          else if ( F16(a3, 88) == 2 )
          {
            (*v380__f) = F32(a3, 52);
            ++F32(a3, 92);
            F32(a3, 52) = W_angle_wrap((*v380__f), 5000, 1000);
          }
        }
        else
        {
          F16(a3, 88) = 1;
        }
        v195 = F16(a3, 88) < 2u;
        F32(a3, 40) = F32(a3, 40) - 500 < 0 ? 0 : F32(a3, 40) - 500;
        if ( !v195 )
          shot_476ef0(c, a3, errno_decorate, 1, 10, 1, 1, 0, 7, 0, 0, 0, 0);
        (*v345__f) = 2000 - 4000 * rnd() / 0x8000;
        v196 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100048,
          (_DWORD *)a3,
          2000 - 4000 * v196 / 0x8000,
          (*v345__f),
          99,
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          0);
        if ( (int32_t)errno_decorate == 0 )
        {
          v197 = shot_476d30(c, 2, F16(c, 0x1fc) - 100, F16(c, 0x1fc), 700);
          F32(c, 0xf4) = v197;
          F32(c, 0xf8) = v197;
        }
        W_polar_div(
          400,
          F32(a3, 52),
          1000,
          F32(a3, 4),
          F32(a3, 8),
          (_DWORD *)(a3 + 4),
          (_DWORD *)(a3 + 8));
        shot_476e20(c, a3, errno_decorate);
        shot_476a80((_DWORD *)a3);
        goto LABEL_763;
      case 0x186AD:
        v191 = rnd();
        W_entity_snd_loop(c, 0, errno_decorate, 0x6Bu, 0, 500 - 1000 * v191 / 0x8000, 0, 0, 30);
        F32(a3, 40) = F32(a3, 40) - 500 < 0 ? 0 : F32(a3, 40) - 500;
        shot_476e20(c, a3, errno_decorate);
        shot_476a80((_DWORD *)a3);
        v192 = F32(a3, 92) + 1;
        F32(a3, 92) = v192;
        if ( v192 > 20 )
          F32(a3, 92) = 0;
        if ( F32(a3, 92) > 10 )
        {
          shot_54cab0(
            F32(a3, 4),
            F32(a3, 8),
            F32(a3, 12) / 2,
            F32(a3, 16),
            F32(c, 0x20c),
            1,
            errno_decorate,
            WORD2(errno_decorate),
            2500,
            0,
            25000,
            0);
          shot_54cab0(
            F32(a3, 4) + F32(a3, 12) / 2,
            F32(a3, 8),
            F32(a3, 12) / 2,
            F32(a3, 16),
            F32(c, 0x20c),
            3,
            errno_decorate,
            WORD2(errno_decorate),
            2500,
            0,
            25000,
            0);
        }
        goto LABEL_763;
      case 0x186AE:
        v88 = 0;
        if ( F32(c, 0x20c) == 0 )
          goto LABEL_503;
        switch ( F16(a3, 88) )
        {
          case 0:
            v88 = 1;
            F16(a3, 88) = 1;
            goto LABEL_193;
          case 1:
LABEL_193:
            v89 = F32(a3, 24) + 400;
            if ( v89 > 8800 )
              v89 = 8800;
            F32(a3, 24) = v89;
            if ( v89 == 8800 )
            {
              F16(a3, 88) = 2;
              v88 = 1;
            }
            goto LABEL_197;
          case 2:
            if ( F32(c, 0x204) != 0 )
            {
              F32(a3, 92) = 0;
              v88 = 1;
              F16(a3, 88) = 3;
              F32(c, 0x228) = 1;
            }
            goto LABEL_197;
          case 3:
            v91 = F32(a3, 92);
            F32(a3, 24) += 200;
            if ( v91 >= 120 )
            {
              F16(a3, 88) = 4;
            }
            else
            {
              F32(a3, 92) = v91 + 1;
LABEL_197:
              shot_4771d0(
                c,
                a3,
                F32(a3, 24),
                F32(a3, 52),
                WORD2(errno_decorate),
                (_DWORD *)(a3 + 4),
                (_DWORD *)(a3 + 8),
                F32(a3, 12),
                F32(a3, 16),
                1);
              if ( F32(a3, 44) == 1 )
                v90 = W_angle_wrap(F32(a3, 52), F32(a3, 40), 1000);
              else
                v90 = W_angle_wrap(F32(a3, 52), -F32(a3, 40), 1000);
              F32(a3, 52) = v90;
              if ( v88 == 0 )
                goto LABEL_218;
            }
            switch ( F16(a3, 88) )
            {
              case 1:
                v93 = 1000 * rnd() / 0x8000 + 500;
                (*v387__f) = (_DWORD *)F32(c, 0x40);
                if ( (int32_t)errno_decorate == 0 )
                {
                  v94 = (intptr_t *)W_vpool_take(SYS32(1196), 0);
                  if ( v94 != 0 )
                  {
                    v95 = F32(v94, 0);
                    if ( W_sfx_place(
                           c,
                           F32(CFG(), 360),
                           0,
                           0,
                           (*v387__f),
                           v390,
                           (int *)&(*v388__f)) != 0 )
                    {
                      g_eng.dsb_volume_5bb870(v95, v390[0]);
                      g_eng.dsb_pan_5bb880(v95, (int)(*v388__f));
                      g_eng.dsb_freq_5bb890(v95, v93);
                      g_eng.dsb_play_5bb8c0(v95);
                    }
                  }
                }
                break;
              case 2:
                if ( (int32_t)errno_decorate == 0 )
                {
                  v92 = F32(c, 0x1b0);
                  if ( v92 != 0 && F16(c, 0x1b4) != 0 )
                    F32(v92, 32) = F32(c, 0x208);
                }
                break;
              case 4:
                goto LABEL_503;
              default:
                break;
            }
LABEL_218:
            if ( F16(a3, 88) < 2u )
            {
              (*v387__f) = F32(a3, 44);
              v97 = 2000 * rnd() / 0x8000 - 1000;
              v98 = 2000 * rnd();
              v390[0] = F32(c, 0x20c);
              if ( (int32_t)errno_decorate == 0
                && W_view_box_visible(
                     *(intptr_t *)&GAP(W, 0x1030, 28),
                     F32(a3, 4),
                     F32(a3, 8),
                     F32(a3, 12),
                     F32(a3, 16),
                     16000) )
              {
                W_particle_spawn(
                  c,
                  v390[0],
                  100057,
                  (_DWORD *)a3,
                  v98 / 0x8000 - 1000,
                  v97,
                  (int)(*v387__f),
                  1,
                  0,
                  0,
                  2,
                  0,
                  0,
                  10);
              }
              if ( F32(CFG(), 148) != 0 )
              {
                (*v364__f) = F32(a3, 44);
                (*v339__f) = 2000 * rnd() / 0x8000 - 1000;
                (*v335__f) = 2000 * rnd() / 0x8000 - 1000;
                W_particle_spawn(
                  c,
                  F32(c, 0x20c),
                  100057,
                  (_DWORD *)a3,
                  (*v335__f),
                  (*v339__f),
                  (*v364__f),
                  1,
                  errno_decorate,
                  0,
                  2,
                  0,
                  0,
                  0);
              }
            }
            else
            {
              v96 = F32(a3, 40) + 100;
              if ( v96 > 5000 )
                v96 = 5000;
              F32(a3, 40) = v96;
              (*v365__f) = F32(a3, 44);
              (*v340__f) = 1600 * rnd() / 0x8000 - 800;
              v99 = rnd();
              W_particle_spawn(
                c,
                F32(c, 0x20c),
                100055,
                (_DWORD *)a3,
                1600 * v99 / 0x8000 - 800,
                (*v340__f),
                (*v365__f),
                1,
                errno_decorate,
                0,
                2,
                0,
                0,
                10);
              if ( F32(CFG(), 148) != 0 )
              {
                (*v387__f) = F32(a3, 44);
                v100 = 1600 * rnd() / 0x8000 - 800;
                v101 = rnd();
                v390[0] = F32(c, 0x20c);
                v102 = 1600 * v101 / 0x8000 - 800;
                if ( (int32_t)errno_decorate == 0
                  && W_view_box_visible(
                       *(intptr_t *)&GAP(W, 0x1030, 28),
                       F32(a3, 4),
                       F32(a3, 8),
                       F32(a3, 12),
                       F32(a3, 16),
                       16000) )
                {
                  W_particle_spawn(c, v390[0], 100055, (_DWORD *)a3, v102, v100, (int)(*v387__f), 1, 0, 0, 2, 0, 0, 0);
                }
              }
            }
            if ( F16(a3, 88) == 2 && (int32_t)errno_decorate == 0 )
            {
              v103 = rnd();
              v104 = F32(c, 0x1b0);
              v105 = 1000 * v103 / 0x8000 + 500;
              if ( v104 != 0 )
              {
                if ( F16(c, 0x1b4) != 0 )
                {
                  if ( F32(v104, 32) == 0 )
                    W_entity_snd_start(c, 0, 0, 0x52u, -1600, v105, 0, 0);
                  W_entity_snd_keep(c, 0, 0);
                  v106 = F32(v104, 32) + 1;
                  F32(v104, 32) = v106;
                  if ( v106 >= 80 )
                    F32(v104, 32) = 0;
                }
                else
                {
                  g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
                }
              }
            }
            if ( F16(a3, 88) >= 2u
              && W_body_hit_spawn(
                   c,
                   F32(c, 0x20c),
                   a3,
                   0,
                   0,
                   F32(a3, 12),
                   F32(a3, 16),
                   7,
                   errno_decorate,
                   1,
                   F32(c, 0x234),
                   F32(c, 0x238),
                   F32(c, 0x23c),
                   8,
                   F32(c, 0x200),
                   1,
                   1,
                   F32(c, 0x220),
                   1,
                   F32(c, 0x244),
                   F32(c, 0x224),
                   0,
                   0,
                   0) != 0 )
            {
              v107 = F32(c, 0x22c) + 1;
              if ( v107 > 100000 )
                v107 = 100000;
              v108 = F32(c, 0x228);
              F32(c, 0x22c) = v107;
              if ( v108 != 0 && v107 >= v108 )
                W_shot_end(c, (_BYTE *)a3, errno_decorate);
            }
            break;
          default:
            goto LABEL_197;
        }
        goto LABEL_763;
      case 0x186AF:
        v61 = F32(c, 0x20c);
        if ( v61 == 0 )
          goto LABEL_503;
        if ( F16(v61, 68) == 0 )
          g_eng.h407f30((const char *)D(0x8a9530), D(0x85451c), 0, F32(v61, 468));
        (*v388__f) = F32(F32(v61, 64), 44);
        v62 = W_part_center_x(F32(c, 0x20c), 0);
        (*v389__f) = F32(c, 0x214) + W_part_center_y(F32(c, 0x20c), 0);
        v63 = F32(a3, 4) + F32(a3, 12) / 2;
        v64 = F32(a3, 8) + F32(a3, 16) / 2;
        v65 = F32(c, 0x210);
        if ( F32(c, 0x218) == (*v388__f) )
          SETLODW((*v391__f), v62 + v65);
        else
          SETLODW((*v391__f), v62 - v65);
        switch ( F16(a3, 88) )
        {
          case 0:
            shot_4497b0(c, errno_decorate, F32(a3, 92), 20);
            if ( F32(a3, 92) == 0 )
              W_sfx_play_at(c, errno_decorate, 108, 800, 0, 0);
            v66 = F32(a3, 92);
            if ( v66 >= 20 )
              goto LABEL_175;
            F32(a3, 92) = v66 + 1;
            goto LABEL_177;
          case 1:
            if ( (int32_t)errno_decorate != 0 )
            {
              v69 = LODWORD((*v391__f));
            }
            else
            {
              if ( F32(c, 0x204) != 0 )
              {
                F16(a3, 88) = 7;
                goto LABEL_176;
              }
              v69 = LODWORD((*v391__f));
              if ( (int)abs32(v63 - LODWORD((*v391__f))) > 800 || (int)abs32(v64 - (*v389__f)) > 3200 )
              {
                F16(a3, 88) = 2;
                F32(a3, 92) = 0;
                F32(a3, 40) = 0;
LABEL_177:
                if ( F16(a3, 88) != 0 && F16(a3, 88) <= 2u )
                {
                  if ( (*v388__f) != F32(a3, 44) )
                    F32(a3, 44) = (*v388__f);
                  v86 = shot_476500(120000, F32(a3, 52), 1000);
                  if ( (int)abs32(v86) > 2000 )
                  {
                    if ( v86 >= 180000 )
                      v87 = W_angle_wrap(F32(a3, 52), -2000, 1000);
                    else
                      v87 = W_angle_wrap(F32(a3, 52), 2000, 1000);
                    F32(a3, 52) = v87;
                  }
                }
                if ( F16(a3, 88) == 0
                  || F16(a3, 88) > 2u && F16(a3, 88) <= 5u )
                {
                  shot_449700(c, a3, errno_decorate, 1u, 0, 0, 4800, 800);
                }
LABEL_763:
                W_entity_ride_part(c, a3, WORD2(errno_decorate));
                SETLO(v8, W_body_clip_step(a3));
                return v8;
              }
              v70 = shot_54cca0(c, 20000, 40, 1, 1, 1);
              F32(c, 0x240) = v70;
              if ( v70 != 0 )
              {
                F32(a3, 40) = 0;
                F16(a3, 88) = 3;
                F32(a3, 92) = 0;
                F32(a3, 24) = 0;
                goto LABEL_177;
              }
            }
            v71 = F32(a3, 92);
            F32(a3, 4) = v69 - F32(a3, 12) / 2;
            if ( v71 != 0 )
            {
              v73 = W_speed_ramp(F32(a3, 24), 500, 500, 10000);
              F32(a3, 24) = v73;
              if ( v73 >= 10000 )
                F32(a3, 92) = 0;
            }
            else
            {
              v72 = W_speed_ramp(F32(a3, 24), 500, 500, -10000);
              F32(a3, 24) = v72;
              if ( v72 <= -10000 )
                F32(a3, 92) = 1;
            }
            v74 = F32(a3, 24);
            goto LABEL_143;
          case 2:
            v75 = abs32(v63 - LODWORD((*v391__f)));
            if ( v75 <= 800 && (int)abs32(v64 - (*v389__f)) <= 800 && F32(a3, 44) == (*v388__f) )
              goto LABEL_175;
            v76 = F32(a3, 40) + 2000;
            if ( v76 > 80000 )
              v76 = 80000;
            F32(a3, 40) = v76;
            if ( v75 > 800 )
            {
              if ( v63 <= SLODWORD((*v391__f)) )
                v77 = (int)((uint64_t)(1374389535LL * v76) >> 32) >> 5;
              else
                v77 = (int)((uint64_t)(-1374389535LL * v76) >> 32) >> 5;
              F32(a3, 4) += (v77 >> 31) + v77;
            }
            if ( (int)abs32(v64 - (*v389__f)) > 800 )
            {
              v74 = F32(a3, 40);
              if ( v64 <= (*v389__f) )
LABEL_143:
                F32(a3, 8) += v74 / 100;
              else
                F32(a3, 8) += v74 / -100;
            }
            goto LABEL_177;
          case 3:
            v78 = F32(c, 0x240);
            if ( v78 == 0 )
              goto LABEL_167;
            v79 = shot_478820((_DWORD *)a3, WORD2(errno_decorate), v78, 0, 1);
            if ( (int)abs32(v79) > 2000 )
            {
              if ( v79 >= 180000 )
                v80 = W_angle_wrap(F32(a3, 52), -2000, 1000);
              else
                v80 = W_angle_wrap(F32(a3, 52), 2000, 1000);
              F32(a3, 52) = v80;
              if ( v80 < 80000 )
              {
                v81 = 80000;
              }
              else
              {
                if ( v80 <= 160000 )
                  goto LABEL_177;
                v81 = 160000;
              }
              F32(a3, 52) = v81;
            }
LABEL_167:
            F16(a3, 88) = 4;
            F32(a3, 40) = -40000;
LABEL_176:
            F32(a3, 92) = 0;
            goto LABEL_177;
          case 4:
            shot_476ba0((_DWORD *)a3, 1);
            v82 = F32(a3, 92);
            if ( v82 >= 20 )
            {
              F32(a3, 40) = 80000;
              F32(a3, 92) = 0;
              F16(a3, 88) = 5;
              (*v381__f) = 500 * rnd() / 0x8000;
              W_sfx_play_at(c, errno_decorate, 109, 800, (*v381__f), 0);
            }
            else
            {
              F32(a3, 92) = v82 + 1;
            }
            goto LABEL_177;
          case 5:
            shot_476ba0((_DWORD *)a3, 1);
            v83 = F32(a3, 92);
            if ( v83 >= 40 )
              goto LABEL_175;
            F32(a3, 92) = v83 + 1;
            if ( shot_476ef0(c, a3, errno_decorate, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0) != 0 )
            {
              v84 = F32(a3, 40);
              F16(a3, 88) = 6;
              F32(a3, 40) = -v84;
            }
            goto LABEL_177;
          case 6:
            shot_476ba0((_DWORD *)a3, 1);
            v85 = F32(a3, 40) + 4000 > 0 ? 0 : F32(a3, 40) + 4000;
            F32(a3, 40) = v85;
            if ( v85 != 0 )
              goto LABEL_177;
LABEL_175:
            F16(a3, 88) = 1;
            goto LABEL_176;
          case 7:
            v67 = errno_decorate;
            shot_4497b0(c, errno_decorate, 20 - F32(a3, 92), 20);
            v68 = F32(a3, 92);
            if ( v68 >= 20 )
              goto LABEL_354;
            F32(a3, 92) = v68 + 1;
            goto LABEL_177;
          default:
            goto LABEL_177;
        }
      case 0x186B0:
        switch ( F16(a3, 88) )
        {
          case 0:
            (*v387__f) = (_DWORD *)(1000 * rnd() / 0x8000 - 500);
            if ( (int32_t)errno_decorate == 0 && F32(c, 0x1b0) != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
              {
                if ( W_sfx_place(c, 0, 0, 0, (_DWORD *)F32(c, 0x40), 0, 0) != 0 )
                {
                  v128 = (intptr_t **)F32(c, 0x1b0);
                  (*v388__f) = SYS32(1312);
                  if ( F32(v128, 0) != 0 )
                    W_entity_snd_release(c, 0, 0, 0);
                  v129 = (intptr_t *)W_vpool_take((*v388__f), 1);
                  if ( v129 != 0 )
                  {
                    v130 = (intptr_t *)(*v387__f);
                    v128[1] = (intptr_t *)(*v388__f);
                    *v128 = v129;
                    v128[2] = 0;
                    v128[3] = v130;
                    v128[4] = 0;
                    v128[5] = (intptr_t *)1;
                    v128[6] = 0;
                    v128[7] = 0;
                    g_eng.dsb_volume_5bb870(F32(v129, 0), -10000);
                    g_eng.dsb_play_5bb8c0(*v129);
                  }
                }
              }
              else
              {
                g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
              }
            }
            ++F16(a3, 88);
            goto LABEL_299;
          case 1:
LABEL_299:
            v131 = F32(a3, 40) + 4000;
            if ( v131 > 120000 )
              v131 = 120000;
            F32(a3, 40) = v131;
            v132 = 5000 - 10000 * rnd() / 0x8000;
            v133 = F32(a3, 52) / 1000;
            if ( (int32_t)errno_decorate == 0 )
            {
              (*v377__f) = F32(a3, 52);
              v134 = rnd();
              W_polar_div(400 - 800 * v134 / 0x8000, (*v377__f), 1000, 0, 0, v390, &(*v387__f));
              v135 = shot_4504c0(
                                     c,
                                     16000,
                                     (int)c,
                                     100062,
                                     (_DWORD *)a3,
                                     v390[0],
                                     (int)(*v387__f),
                                     99,
                                     1,
                                     0,
                                     0,
                                     2,
                                     0,
                                     v133,
                                     10);
              if ( v135 != 0 )
                W_entity_scatter(v135, 0, 0, v132, 0);
            }
            v136 = 5000 - 10000 * rnd() / 0x8000;
            v137 = F32(a3, 52) / 1000;
            if ( (int32_t)errno_decorate == 0 )
            {
              (*v378__f) = F32(a3, 52);
              v138 = rnd();
              W_polar_div(400 - 800 * v138 / 0x8000, (*v378__f), 1000, 0, 0, &(*v389__f), &(*v388__f));
              v139 = shot_4504c0(
                                     c,
                                     16000,
                                     (int)c,
                                     100062,
                                     (_DWORD *)a3,
                                     (*v389__f),
                                     (int)(*v388__f),
                                     99,
                                     1,
                                     0,
                                     0,
                                     2,
                                     0,
                                     v137 + 90,
                                     0);
              if ( v139 != 0 )
                W_entity_scatter(v139, 20000, 0, v136, 0);
              v140 = shot_4504c0(
                                     c,
                                     16000,
                                     (int)c,
                                     100062,
                                     (_DWORD *)a3,
                                     (*v389__f),
                                     (int)(*v388__f),
                                     99,
                                     1,
                                     0,
                                     0,
                                     2,
                                     0,
                                     v137 - 90,
                                     0);
              if ( v140 != 0 )
                W_entity_scatter(v140, 20000, 0, v136, 0);
            }
            v141 = F32(a3, 92);
            if ( v141 >= 24
              || (v142 = F32(a3, 40),
                  F32(a3, 92) = v141 + 1,
                  W_polar_div(v142 / 100, F32(a3, 52), 1000, 0, 0, &(*v387__f), v390),
                  (*v387__f) != 0)
              && W_entity_move_horizontal(c, (_DWORD *)a3, (int)(*v387__f), 0, 0, 0, WORD2(errno_decorate), 0) == 0
              || v390[0] != 0
              && W_entity_move_vertical(c, (_DWORD *)a3, v390[0], 0, WORD2(errno_decorate), a3 + 8, 0) == 0 )
            {
              ++F16(a3, 88);
              F32(a3, 92) = 0;
              F32(a3, 40) = 0;
            }
            break;
          case 2:
            v143 = F32(a3, 12);
            v144 = F32(a3, 92) + 1;
            F32(a3, 92) = v144;
            if ( v143 < 36000 )
            {
              F32(a3, 12) = v143 + 3200;
              F32(a3, 4) -= 1600;
              F32(a3, 16) += 3200;
              F32(a3, 8) -= 1600;
            }
            if ( v144 % 4 == 0 )
            {
              v145 = 2000 * rnd();
              (*v387__f) = (_DWORD *)F32(c, 0x40);
              if ( (int32_t)errno_decorate == 0 )
              {
                v146 = (intptr_t *)W_vpool_take(SYS32(1316), 0);
                if ( v146 != 0 )
                {
                  v147 = F32(v146, 0);
                  if ( W_sfx_place(
                         c,
                         F32(CFG(), 360),
                         0,
                         0,
                         (*v387__f),
                         v390,
                         (int *)&(*v388__f)) != 0 )
                  {
                    g_eng.dsb_volume_5bb870(v147, v390[0]);
                    g_eng.dsb_pan_5bb880(v147, (int)(*v388__f));
                    g_eng.dsb_freq_5bb890(v147, v145 / 0x8000 - 1000);
                    g_eng.dsb_play_5bb8c0(v147);
                  }
                }
              }
            }
            if ( F32(a3, 92) > 120 )
              ++F16(a3, 88);
            break;
          case 3:
            v148 = F32(a3, 12);
            ++F32(a3, 92);
            if ( v148 > 3200 )
            {
              F32(a3, 12) = v148 - 800;
              F32(a3, 4) += 400;
            }
            v149 = F32(a3, 16);
            if ( v149 > 3200 )
            {
              F32(a3, 16) = v149 - 800;
              F32(a3, 8) += 400;
            }
            break;
          default:
            break;
        }
        if ( F16(a3, 88) < 2u || F32(a3, 92) % 2 != 0 )
        {
          v67 = errno_decorate;
          if ( (int32_t)errno_decorate != 0 )
            goto LABEL_345;
        }
        else
        {
          v67 = errno_decorate;
          if ( (int32_t)errno_decorate != 0 )
            goto LABEL_345;
          (*v387__f) = (_DWORD *)(360 * rnd() / 0x8000);
          v150 = rnd();
          v151 = (double)(int)(*v387__f) * (*(const double *)D(0x8a9a38));
          v390[0] = v150 * (F32(a3, 12) / 2 - 4000) / 0x8000;
          v152 = (double)v390[0];
          v153 = sin(v151) * v152;
          if ( v153 >= 0.0 )
            v154 = v153 + 0.5;
          else
            v154 = v153 - 0.5;
          v155 = (int64_t)v154;
          v156 = ftol_round_(cos(v151) * v152);
          (*v387__f) = F32(a3, 44);
          v157 = -v156;
          if ( W_view_box_visible(
                 *(intptr_t *)&GAP(W, 0x1030, 28),
                 F32(a3, 4),
                 F32(a3, 8),
                 F32(a3, 12),
                 F32(a3, 16),
                 16000) )
          {
            W_particle_spawn(c, (int)c, 100068, (_DWORD *)a3, v155, v157, (int)(*v387__f), 1, 0, 0, 2, 0, 0, 20);
          }
          (*v387__f) = F32(a3, 44);
          if ( W_view_box_visible(
                 *(intptr_t *)&GAP(W, 0x1030, 28),
                 F32(a3, 4),
                 F32(a3, 8),
                 F32(a3, 12),
                 F32(a3, 16),
                 16000) )
          {
            W_particle_spawn(c, (int)c, 100068, (_DWORD *)a3, -v155, -v157, (int)(*v387__f), 1, 0, 0, 2, 0, 0, 0);
          }
          v67 = 0;
        }
        v158 = F32(c, 0x1b0);
        if ( v158 != 0 )
        {
          if ( F16(c, 0x1b4) != 0 )
            F32(v158, 20) = 1;
          else
            g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
        }
LABEL_345:
        if ( W_body_hit_spawn(
               c,
               F32(c, 0x20c),
               a3,
               0,
               0,
               F32(a3, 12),
               F32(a3, 16),
               0,
               v67,
               1,
               F32(c, 0x234),
               F32(c, 0x238),
               F32(c, 0x23c),
               9,
               F32(c, 0x200),
               1,
               1,
               F32(c, 0x220),
               1,
               F32(c, 0x244),
               F32(c, 0x224),
               0,
               0,
               0) != 0 )
        {
          v159 = F32(c, 0x22c) + 1;
          if ( v159 > 100000 )
            v159 = 100000;
          v160 = F32(c, 0x228);
          F32(c, 0x22c) = v159;
          if ( v160 != 0 && v159 >= v160 )
            W_shot_end(c, (_BYTE *)a3, v67);
          if ( F16(a3, 88) < 2u )
          {
            F32(a3, 92) = 0;
            F32(a3, 40) = 0;
            F16(a3, 88) = 2;
          }
        }
        if ( ++F16(a3, 60) > F16(c, 0x1fc) )
LABEL_354:
          W_shot_end(c, (_BYTE *)a3, v67);
        goto LABEL_763;
      case 0x186B1:
        v252 = F16(a3, 88) < 3u;
        if ( shot_445ce0(c, (_DWORD *)a3, WORD2(errno_decorate)) )
        {
          if ( v252 )
          {
            v253 = F32(a3, 24) - 2000;
            if ( v253 < -20000 )
            {
              v253 = -20000;
            }
            else if ( v253 > 0 )
            {
              v253 = 0;
            }
            F32(a3, 24) = v253;
            F32(a3, 8) += v253 / 100;
          }
          else
          {
            F32(a3, 24) = 0;
          }
        }
        else
        {
          v254 = F32(a3, 24) + 4000;
          if ( v254 > 80000 )
            v254 = 80000;
          F32(a3, 24) = v254;
          shot_46e150(c, (_DWORD *)a3, WORD2(errno_decorate));
        }
        v255 = F16(a3, 88);
        switch ( v255 )
        {
          case 0:
            v256 = 1000 * rnd();
            (*v387__f) = (_DWORD *)F32(c, 0x40);
            if ( (int32_t)errno_decorate == 0 )
            {
              v257 = (intptr_t *)W_vpool_take(SYS32(1200), 0);
              if ( v257 != 0 )
              {
                v258 = F32(v257, 0);
                if ( W_sfx_place(
                       c,
                       F32(CFG(), 360),
                       0,
                       0,
                       (*v387__f),
                       v390,
                       (int *)&(*v388__f)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v258, v390[0]);
                  g_eng.dsb_pan_5bb880(v258, (int)(*v388__f));
                  g_eng.dsb_freq_5bb890(v258, v256 / 0x8000 - 500);
                  g_eng.dsb_play_5bb8c0(v258);
                }
              }
            }
            ++F16(a3, 88);
            break;
          case 1:
            v259 = F32(a3, 92);
            F32(a3, 40) = 40000;
            F32(a3, 92) = ++v259;
            if ( v259 > F32(c, 0x204) )
            {
              F32(a3, 92) = 0;
              F16(a3, 88) = v255 + 1;
            }
            break;
          case 2:
            if ( F32(a3, 92) > 40 )
              F16(a3, 88) = v255 + 1;
            goto LABEL_568;
          case 3:
LABEL_568:
            v260 = F32(a3, 92) + 1;
            F32(a3, 40) = F32(a3, 40) - 2000 < 0 ? 0 : F32(a3, 40) - 2000;
            F32(a3, 92) = v260;
            if ( (int32_t)errno_decorate == 0 && v260 % 40 == 0 )
            {
              v261 = 1000 * rnd() / 0x8000 - 500;
              (*v387__f) = (_DWORD *)F32(c, 0x40);
              v262 = (intptr_t *)W_vpool_take(SYS32(1776), 0);
              if ( v262 != 0 )
              {
                v263 = F32(v262, 0);
                if ( W_sfx_place(
                       c,
                       F32(CFG(), 360),
                       0,
                       0,
                       (*v387__f),
                       v390,
                       (int *)&(*v388__f)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v263, v390[0]);
                  g_eng.dsb_pan_5bb880(v263, (int)(*v388__f));
                  g_eng.dsb_freq_5bb890(v263, v261);
                  g_eng.dsb_play_5bb8c0(v263);
                }
              }
            }
            break;
          default:
            break;
        }
        if ( F32(a3, 40) != 0 )
        {
          v264 = errno_decorate;
          if ( W_body_hit_spawn(
                 c,
                 F32(c, 0x20c),
                 a3,
                 0,
                 0,
                 F32(a3, 12),
                 F32(a3, 16) + 4,
                 7,
                 errno_decorate,
                 1,
                 F32(c, 0x234),
                 F32(c, 0x238),
                 F32(c, 0x23c),
                 8,
                 F32(c, 0x200),
                 1,
                 1,
                 F32(c, 0x220),
                 0,
                 F32(c, 0x244),
                 F32(c, 0x224),
                 0,
                 0,
                 0) != 0 )
          {
            v267 = F32(c, 0x22c) + 1;
            if ( v267 > 100000 )
              v267 = 100000;
            v268 = F32(c, 0x228);
            F32(c, 0x22c) = v267;
            if ( v268 != 0 && v267 >= v268 )
              goto LABEL_585;
          }
        }
        else
        {
          v264 = errno_decorate;
          if ( W_body_hit_spawn(
                 c,
                 F32(c, 0x20c),
                 a3,
                 0,
                 0,
                 F32(a3, 12),
                 F32(a3, 16),
                 8,
                 errno_decorate,
                 1,
                 F32(c, 0x234),
                 F32(c, 0x238),
                 F32(c, 0x23c),
                 8,
                 F32(c, 0x200),
                 1,
                 1,
                 F32(c, 0x220),
                 0,
                 F32(c, 0x244),
                 F32(c, 0x224),
                 0,
                 0,
                 0) != 0 )
          {
            v265 = F32(c, 0x22c) + 1;
            if ( v265 > 100000 )
              v265 = 100000;
            v266 = F32(c, 0x228);
            F32(c, 0x22c) = v265;
            if ( v266 != 0 && v265 >= v266 )
LABEL_585:
              W_shot_end(c, (_BYTE *)a3, v264);
          }
        }
        if ( F32(CFG(), 148) != 0 || (F16(a3, 60) & 1) == 0 )
        {
          v269 = 3200 * rnd() / 0x8000 - 1600;
          v270 = 3200 * rnd() / 0x8000 - 1600;
          if ( (int32_t)errno_decorate == 0 && shot_4538f0(*(_DWORD **)&GAP(W, 0x1030, 28), (_DWORD *)a3, 16000) )
            W_particle_spawn(c, (int)c, 100058, (_DWORD *)a3, v270, v269, 99, 4, 0, 0, 2, 0, 0, 0);
        }
        shot_476ba0((_DWORD *)a3, 0);
        shot_476e20(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186B2:
      case 0x186C9:
        v306 = 100114;
        goto LABEL_675;
      case 0x186B3:
        v306 = 100115;
        goto LABEL_675;
      case 0x186B4:
        v306 = 100116;
LABEL_675:
        if ( (F16(a3, 60) & 7) == 0 )
        {
          (*v359__f) = 6400 * rnd() / 0x8000 - 3200;
          v307 = rnd();
          shot_4504c0(
            c,
            16000,
            (int)c,
            v306,
            (_DWORD *)a3,
            4000 * v307 / 0x8000 - 2000,
            (*v359__f),
            99,
            1,
            errno_decorate,
            0,
            2,
            0,
            0,
            0);
        }
        shot_476ef0(c, a3, errno_decorate, 1, 0, 1, 1, 0, 13, 0, 0, 0, 0);
        shot_477070(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186B5:
        if ( F16(a3, 88) != 0 )
        {
          if ( F16(a3, 88) != 1 )
            goto LABEL_619;
        }
        else
        {
          F16(a3, 88) = 1;
          v282 = rnd();
          W_entity_snd_start(c, 0, errno_decorate, 0x64u, -600, 500 - 1000 * v282 / 0x8000, 0, 0);
          W_polar_div(
            1600,
            F32(a3, 52),
            1000,
            F32(a3, 4),
            F32(a3, 8),
            (_DWORD *)(a3 + 4),
            (_DWORD *)(a3 + 8));
        }
        v283 = F32(a3, 92);
        if ( v283 < 40 )
        {
          F32(a3, 92) = v283 + 1;
          W_entity_snd_keep(c, 0, errno_decorate);
        }
LABEL_619:
        F32(a3, 52) = W_angle_wrap(F32(a3, 52), 2000, 1000);
        shot_4504c0(c, 6400, (int)c, 100065, (_DWORD *)a3, 0, 0, 99, 1, errno_decorate, 0, 2, 0, 0, 10);
        if ( F32(CFG(), 148) != 0 )
        {
          (*v352__f) = 400 * rnd() / 0x8000 - 200;
          v284 = rnd();
          shot_4504c0(
            c,
            6400,
            (int)c,
            100065,
            (_DWORD *)a3,
            400 * v284 / 0x8000 - 200,
            (*v352__f),
            99,
            1,
            errno_decorate,
            0,
            2,
            0,
            0,
            0);
        }
        shot_477070(c, a3, errno_decorate);
        shot_476ef0(c, a3, errno_decorate, 1, 0, 1, 1, 0, 7, 0, 0, 0, 0);
        goto LABEL_763;
      case 0x186B6:
        if ( F16(a3, 88) == 0 )
        {
          v280 = rnd();
          W_sfx_play_at(c, errno_decorate, 101, 0, 500 - 1000 * v280 / 0x8000, 0);
          ++F16(a3, 88);
        }
        (*v351__f) = 400 * rnd() / 0x8000 - 200;
        v281 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100066,
          (_DWORD *)a3,
          400 * v281 / 0x8000 - 200,
          (*v351__f),
          99,
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          10);
        shot_476ef0(c, a3, errno_decorate, 1, 0, 1, 1, 0, 7, 0, 0, 0, 0);
        shot_476ba0((_DWORD *)a3, 0);
        shot_476e20(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186B7:
        v18 = errno_decorate;
        switch ( F16(a3, 88) )
        {
          case 0:
            W_sfx_play_at(c, errno_decorate, 89, -400, 0, 0);
            ++F16(a3, 88);
            goto LABEL_21;
          case 1:
LABEL_21:
            v19 = F32(a3, 92);
            if ( v19 < 64 )
              F32(a3, 92) = v19 + 1;
            goto LABEL_36;
          case 0x64:
            v20 = F32(a3, 92);
            if ( v20 >= 64 )
              F16(a3, 88) = 101;
            else
              F32(a3, 92) = v20 + 4;
            goto LABEL_36;
          case 0x65:
            v21 = F32(a3, 92);
            if ( v21 <= 0 )
              F16(a3, 88) = 100;
            else
              F32(a3, 92) = v21 - 4;
            goto LABEL_36;
          case 0x96:
            if ( F8(a3, 116) != 0 )
            {
              if ( F32(a3, 108) != 0 )
              {
                F32(a3, 108) = 0;
                F8(a3, 116) = 0;
                F16(a3, 114) = 0;
                F16(a3, 112) = 0;
              }
              F16(a3, 88) = 1;
            }
            goto LABEL_36;
          case 0xC8:
            W_sfx_play_at(c, errno_decorate, 90, -400, 0, 0);
            ++F16(a3, 88);
            goto LABEL_34;
          case 0xC9:
LABEL_34:
            v22 = F32(a3, 92);
            if ( v22 <= 0 )
              goto LABEL_503;
            v18 = errno_decorate;
            F32(a3, 92) = v22 - 1;
LABEL_36:
            if ( v18 == 0 )
            {
              v23 = W_tint_fade_b(600 * F32(a3, 92) / 64, 1000);
              F32(c, 0xf4) = v23;
              F32(c, 0xf8) = v23;
              switch ( F32(c, 0x204) )
              {
                case 0:
                  goto LABEL_57;
                case 1:
                  goto LABEL_60;
                case 2:
                  goto LABEL_62;
                case 3:
                  goto LABEL_64;
                default:
                  goto LABEL_763;
              }
            }
            break;
          default:
            goto LABEL_36;
        }
        goto LABEL_763;
      case 0x186B8:
        v24 = errno_decorate;
        switch ( F16(a3, 88) )
        {
          case 0:
            W_sfx_play_at(c, errno_decorate, 89, -400, 0, 0);
            ++F16(a3, 88);
            goto LABEL_40;
          case 1:
LABEL_40:
            v25 = F32(a3, 92);
            if ( v25 < 64 )
              F32(a3, 92) = v25 + 1;
            goto LABEL_55;
          case 0x64:
            v26 = F32(a3, 92);
            if ( v26 >= 64 )
              F16(a3, 88) = 101;
            else
              F32(a3, 92) = v26 + 4;
            goto LABEL_55;
          case 0x65:
            v27 = F32(a3, 92);
            if ( v27 <= 0 )
              F16(a3, 88) = 100;
            else
              F32(a3, 92) = v27 - 4;
            goto LABEL_55;
          case 0x96:
            if ( F8(a3, 116) != 0 )
            {
              if ( F32(a3, 108) != 0 )
              {
                F32(a3, 108) = 0;
                F8(a3, 116) = 0;
                F16(a3, 114) = 0;
                F16(a3, 112) = 0;
              }
              F16(a3, 88) = 1;
            }
            goto LABEL_55;
          case 0xC8:
            W_sfx_play_at(c, errno_decorate, 90, -400, 0, 0);
            ++F16(a3, 88);
            goto LABEL_53;
          case 0xC9:
LABEL_53:
            v28 = F32(a3, 92);
            if ( v28 <= 0 )
              goto LABEL_503;
            v24 = errno_decorate;
            F32(a3, 92) = v28 - 1;
LABEL_55:
            if ( v24 == 0 )
            {
              v29 = W_tint_fade_b(700 * F32(a3, 92) / 64, 1000);
              F32(c, 0xf4) = v29;
              F32(c, 0xf8) = v29;
              switch ( F32(c, 0x204) )
              {
                case 0:
LABEL_57:
                  v30 = F16(a3, 88);
                  if ( v30 >= 0x64u && v30 < 0x96u )
                    F16(a3, 88) = 1;
                  break;
                case 1:
LABEL_60:
                  if ( F16(a3, 88) < 0x64u )
                    F16(a3, 88) = 100;
                  break;
                case 2:
LABEL_62:
                  if ( F16(a3, 88) < 0xC8u )
                  {
                    F32(a3, 92) = 64;
                    F16(a3, 88) = 200;
                  }
                  break;
                case 3:
LABEL_64:
                  if ( F16(a3, 88) <= 0x96u )
                  {
                    W_body_set_clip(a3, (int)D(0x646010));
                    F16(a3, 88) = 150;
                    F32(c, 0x204) = 0;
                  }
                  break;
                default:
                  goto LABEL_763;
              }
            }
            break;
          default:
            goto LABEL_55;
        }
        goto LABEL_763;
      case 0x186B9:
        (*v386__f) = 0x1000003E8LL;
        (*v379__f) = 149;
        goto LABEL_724;
      case 0x186BA:
        v320 = F32(a3, 40) + 2000;
        if ( v320 > 30000 )
          v320 = 30000;
        F32(a3, 40) = v320;
        shot_476ba0((_DWORD *)a3, 0);
        shot_476e20(c, a3, errno_decorate);
        shot_476ef0(c, a3, errno_decorate, 1, 0, 1, 1, 0, 13, 0, 0, 0, 0);
        goto LABEL_763;
      case 0x186BB:
        if ( F16(a3, 88) == 0 )
        {
          W_sfx_play_at(c, errno_decorate, 139, 0, 0, 0);
          ++F16(a3, 88);
        }
        (*v360__f) = 800 * rnd() / 0x8000 - 400;
        v308 = rnd();
        shot_4504c0(
          c,
          16000,
          (int)c,
          100110,
          (_DWORD *)a3,
          800 * v308 / 0x8000 - 400,
          (*v360__f),
          99,
          1,
          errno_decorate,
          0,
          2,
          0,
          0,
          0);
        shot_476ef0(c, a3, errno_decorate, 1, 16, 1, 0, 0, 7, 0, 0, 0, 0);
        v309 = F32(a3, 52);
        if ( v309 <= 180000 )
          F32(a3, 52) = v309 + 3000;
        else
          F32(a3, 52) = v309 - 3000;
LABEL_682:
        shot_476ba0((_DWORD *)a3, 0);
        shot_476e20(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186BC:
        (*v386__f) = 0xF000003E8LL;
        (*v379__f) = 234;
LABEL_724:
        shot_4785a0(c, a3, errno_decorate, 8, (*v379__f), 0, (*v386__f), (_DWORD *)HIDWORD((*v386__f)));
        shot_477070(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186BD:
        if ( F16(a3, 60) > F32(c, 0x204) )
          shot_478550(a3, 2000);
        v310 = errno_decorate;
        shot_4785a0(c, a3, errno_decorate, 8, 73, 0, 1000, (_DWORD *)0xF);
        F8(a3, 124) = 1;
        if ( shot_476ab0(c, a3, errno_decorate, SHIDWORD(errno_decorate), 0) == 0 )
          goto LABEL_721;
        goto LABEL_689;
      case 0x186BE:
        if ( F16(a3, 60) > F32(c, 0x204) )
          shot_478550(a3, 2000);
        v310 = errno_decorate;
        shot_4785a0(c, a3, errno_decorate, 8, 0, 0, 1000, (_DWORD *)3);
        F8(a3, 124) = 1;
        if ( shot_476ab0(c, a3, errno_decorate, SHIDWORD(errno_decorate), 0) == 0 )
LABEL_721:
          W_shot_end(c, (_BYTE *)a3, v310);
        goto LABEL_689;
      case 0x186BF:
        if ( shot_477b70(c, a3, errno_decorate) == 0 )
          goto LABEL_763;
        goto LABEL_503;
      case 0x186C0:
        shot_477f10(c, a3, errno_decorate);
        goto LABEL_763;
      case 0x186C1:
        if ( shot_4781f0(c, a3, errno_decorate) == 0 )
          goto LABEL_763;
        goto LABEL_503;
      case 0x186C2:
        if ( F32(c, 0x20c) == 0 )
          goto LABEL_503;
        v31 = F16(a3, 88);
        if ( v31 != 0 )
        {
          if ( F16(a3, 88) != 1 )
          {
            if ( F16(a3, 88) != 2 )
              goto LABEL_100;
            v32 = 24 - F32(a3, 92);
            if ( (int32_t)errno_decorate == 0 )
            {
              v33 = 20 * v32 / 12;
              if ( v33 >= 0 )
              {
                v34 = 20 * v32 / 12;
                if ( v33 >= 20 )
                {
                  v35 = 0;
                  goto LABEL_74;
                }
              }
              else
              {
                v34 = 0;
              }
              v35 = SYS32(4 * v34 + 10152);
LABEL_74:
              F32(c, 0xf4) = v35;
              F32(c, 0xf8) = v35;
            }
            v36 = F32(a3, 92);
            if ( v36 >= 24 )
            {
LABEL_503:
              SETLO(v8, W_shot_end(c, (_BYTE *)a3, errno_decorate));
              return v8;
            }
            F32(a3, 92) = v36 + 1;
            goto LABEL_100;
          }
          v37 = F32(a3, 52) + 10000;
          if ( v37 > 110000 )
            v37 = 110000;
          F32(a3, 52) = v37;
          v38 = F32(a3, 92);
          if ( v38 < 15 )
          {
            F32(a3, 92) = v38 + 1;
            goto LABEL_100;
          }
          F16(a3, 88) = v31 + 1;
        }
        else
        {
          if ( F32(a3, 92) % 2 == 0 && (int32_t)errno_decorate == 0 )
          {
            (*v338__f) = 6400 * rnd() / 0x8000 - 6400;
            v39 = rnd();
            W_particle_spawn(
              c,
              (int)c,
              100049,
              (_DWORD *)a3,
              9600 * v39 / 0x8000 - 4800,
              (*v338__f),
              99,
              1,
              0,
              0,
              2,
              0,
              0,
              0);
          }
          v40 = F32(a3, 92);
          if ( v40 < 20 )
          {
            v41 = v40 + 1;
            F32(a3, 92) = v41;
            if ( (int32_t)errno_decorate != 0 )
              goto LABEL_100;
            v42 = 20 * v41 / 12;
            if ( v42 >= 0 )
            {
              v43 = 20 * v41 / 12;
              if ( v42 >= 20 )
              {
                F32(c, 0xf4) = 0;
                F32(c, 0xf8) = 0;
LABEL_100:
                v48 = F32(a3, 44);
                v49 = F32(a3, 16);
                (*v389__f) = F32(a3, 12);
                v50 = F32(a3, 52);
                v390[0] = v49;
                (*v388__f) = v50;
                if ( v48 == 3 )
                  (*v388__f) = (360000 - (int32_t)v50);
                v51 = F32(c, 0x20c);
                v52 = WORD2(errno_decorate);
                if ( WORD2(errno_decorate) >= F16(v51, 0x44) )
                  v52 = 0;
                v53 = F32(v51, 0x40) + 660 * v52;
                v54 = F32(v53, 4) + F32(v53, 12) / 2;
                v55 = W_part_center_y(v51, WORD2(errno_decorate));
                W_polar_div(8000, (int)(*v388__f), 1000, v54 + F32(c, 0x210), F32(c, 0x214) + v55, &(*v391__f), &(*v387__f));
                v56 = v390[0];
                F32(a3, 4) = (*v389__f) / -2 + LODWORD((*v391__f));
                v57 = F16(a3, 88) == 1;
                F32(a3, 8) = (uint8_t *)(*v387__f) + v56 / -2;
                if ( v57
                  && W_body_hit_spawn(
                       c,
                       F32(c, 0x20c),
                       a3,
                       0,
                       0,
                       F32(a3, 12),
                       F32(a3, 16),
                       0,
                       errno_decorate,
                       1,
                       F32(c, 0x234),
                       F32(c, 0x238),
                       F32(c, 0x23c),
                       5,
                       F32(c, 0x200),
                       1,
                       0,
                       F32(c, 0x220),
                       0,
                       F32(c, 0x244),
                       F32(c, 0x224),
                       0,
                       0,
                       0) != 0 )
                {
                  v59 = F32(c, 0x22c) + 1;
                  if ( v59 > 100000 )
                    v59 = 100000;
                  v60 = F32(c, 0x228);
                  F32(c, 0x22c) = v59;
                  if ( v60 != 0 && v59 >= v60 )
                  {
                    if ( (int32_t)errno_decorate != 0 )
                    {
                      F8(a3, 0) = 0;
                    }
                    else
                    {
                      if ( F32(c, 0x254) != 0 )
                        shot_476740(c, 0);
                      W_entity_release((int)c, /*edx*/ 0);
                    }
                  }
                }
                goto LABEL_763;
              }
            }
            else
            {
              v43 = 0;
            }
            v44 = SYS32(4 * v43 + 10152);
            F32(c, 0xf4) = v44;
            F32(c, 0xf8) = v44;
            goto LABEL_100;
          }
          v45 = (_DWORD *)F32(c, 0x40);
          if ( (int32_t)errno_decorate == 0 )
          {
            v46 = (intptr_t *)W_vpool_take(SYS32(1152), 0);
            if ( v46 != 0 )
            {
              v47 = F32(v46, 0);
              if ( W_sfx_place(
                     c,
                     F32(CFG(), 360),
                     0,
                     0,
                     v45,
                     (int *)&(*v388__f),
                     &(*v389__f)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v47, (int)(*v388__f));
                g_eng.dsb_pan_5bb880(v47, (*v389__f));
                g_eng.dsb_freq_5bb890(v47, 0);
                g_eng.dsb_play_5bb8c0(v47);
              }
            }
          }
          ++F16(a3, 88);
        }
        F32(a3, 92) = 0;
        goto LABEL_100;
      case 0x186C3:
        shot_476ef0(c, a3, errno_decorate, 1, 0, 1, 1, 0, 7, 0, 0, 0, 0);
        v323 = F32(a3, 40) + 2000;
        if ( v323 > 50000 )
          v323 = 50000;
        F32(a3, 40) = v323;
        if ( F32(a3, 44) == 1 )
          v324 = W_angle_wrap(F32(a3, 52), 2000, 1000);
        else
          v324 = W_angle_wrap(F32(a3, 52), -2000, 1000);
        F32(a3, 52) = v324;
        shot_477070(c, a3, errno_decorate);
        if ( (int32_t)errno_decorate == 0 )
        {
          v325 = shot_476d30(c, 0, 0x78u, 0xA0u, 1000);
          F32(c, 0xf4) = v325;
          F32(c, 0xf8) = v325;
        }
        goto LABEL_763;
      case 0x186C4:
        v310 = errno_decorate;
        if ( F32(a3, 92) % 8 == 0 )
        {
          (*v374__f) = F32(a3, 44);
          (*v361__f) = 6400 - 16000 * rnd() / 0x8000;
          v311 = rnd();
          shot_4504c0(
            c,
            25600,
            (int)c,
            100136,
            (_DWORD *)a3,
            12000 - 24000 * v311 / 0x8000,
            (*v361__f),
            (*v374__f),
            3,
            errno_decorate,
            0,
            2,
            1,
            110,
            10);
        }
        v312 = F16(a3, 88);
        ++F32(a3, 92);
        if ( (uint16_t)v312 != 0 )
        {
          v313 = v312 - 1;
          if ( v313 != 0 )
          {
            if ( v313 != 1 )
            {
LABEL_689:
              shot_476e20(c, a3, v310);
              goto LABEL_763;
            }
LABEL_693:
            shot_54cab0(
              F32(a3, 4),
              F32(a3, 8),
              F32(a3, 12),
              F32(a3, 16),
              F32(c, 0x20c),
              F32(a3, 44),
              errno_decorate,
              WORD2(errno_decorate),
              5000,
              5000,
              56000,
              0);
            shot_476e20(c, a3, errno_decorate);
            goto LABEL_763;
          }
        }
        else
        {
          W_sfx_play_at(c, errno_decorate, 154, 800, 0, -3000);
          ++F16(a3, 88);
        }
        if ( F32(a3, 92) > 10 )
          ++F16(a3, 88);
        goto LABEL_693;
      case 0x186C5:
        if ( shot_477650(c, a3, errno_decorate, SHIDWORD(errno_decorate)) != 0 && F16(a3, 88) == 1 )
        {
          v319 = rnd();
          W_sfx_play_at(c, errno_decorate, 83, 0, 1000 * v319 / 0x8000 - 500, 0);
        }
        goto LABEL_763;
      case 0x186C6:
        if ( shot_477650(c, a3, errno_decorate, SHIDWORD(errno_decorate)) != 0 && F16(a3, 88) == 1 )
        {
          v318 = rnd();
          W_sfx_play_at(c, errno_decorate, 82, 0, 1000 * v318 / 0x8000 - 500, 0);
        }
        goto LABEL_763;
      case 0x186C7:
        v314 = F16(a3, 88);
        if ( (uint16_t)v314 != 0 )
        {
          v315 = v314 - 1;
          if ( v315 != 0 )
          {
            if ( v315 == 1 )
            {
              (*v375__f) = F32(a3, 44);
              (*v362__f) = 1000 * rnd() / 0x8000 - 500;
              v316 = rnd();
              shot_4504c0(
                c,
                16000,
                (int)c,
                100054,
                (_DWORD *)a3,
                1000 * v316 / 0x8000 - 500,
                (*v362__f),
                (*v375__f),
                1,
                errno_decorate,
                0,
                2,
                0,
                0,
                10);
              shot_476ef0(c, a3, errno_decorate, 1, 12, 1, 1, 1, 7, 0, 0, 0, 0);
              W_entity_snd_keep(c, 0, errno_decorate);
              shot_477070(c, a3, errno_decorate);
              goto LABEL_763;
            }
LABEL_703:
            W_entity_snd_keep(c, 0, errno_decorate);
            shot_477070(c, a3, errno_decorate);
            goto LABEL_763;
          }
        }
        else
        {
          if ( F32(c, 0x1f8) == 0 )
          {
            v317 = rnd();
            W_entity_snd_start(c, 0, errno_decorate, 0xE6u, 0, 1000 * v317 / 0x8000 - 500, 0, 0);
          }
          ++F16(a3, 88);
        }
        (*v376__f) = F32(a3, 44);
        ++F32(a3, 92);
        shot_4504c0(c, 16000, (int)c, 100054, (_DWORD *)a3, 0, 0, (*v376__f), 1, errno_decorate, 0, 2, 0, 0, 10);
        if ( F32(a3, 92) > 5 )
        {
          ++F16(a3, 88);
          F32(a3, 92) = 0;
        }
        goto LABEL_703;
      case 0x186C8:
        switch ( F16(a3, 88) )
        {
          case 0:
            (*v387__f) = (_DWORD *)(1000 * rnd() / 0x8000 - 500);
            if ( (int32_t)errno_decorate == 0 && F32(c, 0x1b0) != 0 )
            {
              if ( F16(c, 0x1b4) != 0 )
              {
                if ( W_sfx_place(c, 0, 0, 0, (_DWORD *)F32(c, 0x40), 0, 0) != 0 )
                {
                  v109 = (intptr_t **)F32(c, 0x1b0);
                  (*v388__f) = SYS32(1312);
                  if ( F32(v109, 0) != 0 )
                    W_entity_snd_release(c, 0, 0, 0);
                  v110 = (intptr_t *)W_vpool_take((*v388__f), 1);
                  if ( v110 != 0 )
                  {
                    v111 = (intptr_t *)(*v387__f);
                    v109[1] = (intptr_t *)(*v388__f);
                    *v109 = v110;
                    v109[2] = 0;
                    v109[3] = v111;
                    v109[4] = 0;
                    v109[5] = (intptr_t *)1;
                    v109[6] = 0;
                    v109[7] = 0;
                    g_eng.dsb_volume_5bb870(F32(v110, 0), -10000);
                    g_eng.dsb_play_5bb8c0(*v110);
                  }
                }
              }
              else
              {
                g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
              }
            }
            ++F16(a3, 88);
            goto LABEL_258;
          case 1:
LABEL_258:
            v112 = F32(a3, 12);
            if ( v112 >= 32000 )
            {
              ++F16(a3, 88);
              F32(a3, 92) = 0;
            }
            else
            {
              F32(a3, 12) = v112 + 1600;
              F32(a3, 4) -= 800;
              F32(a3, 16) += 1600;
              v113 = F32(a3, 8) - 800;
LABEL_265:
              F32(a3, 8) = v113;
            }
LABEL_266:
            v115 = F32(a3, 92) + 1;
            F32(a3, 92) = v115;
            if ( v115 % 4 == 0 && (int32_t)errno_decorate == 0 )
            {
              (*v387__f) = (_DWORD *)(360 * rnd() / 0x8000);
              v116 = rnd();
              v117 = (double)(int)(*v387__f) * (*(const double *)D(0x8a9a38));
              v390[0] = v116 * (F32(a3, 12) / 2 - 4000) / 0x8000;
              v118 = (double)v390[0];
              v119 = sin(v117) * v118;
              if ( v119 >= 0.0 )
                v120 = v119 + 0.5;
              else
                v120 = v119 - 0.5;
              v121 = (int64_t)v120;
              v122 = ftol_round_(cos(v117) * v118);
              (*v387__f) = F32(a3, 44);
              v123 = -v122;
              if ( W_view_box_visible(
                     *(intptr_t *)&GAP(W, 0x1030, 28),
                     F32(a3, 4),
                     F32(a3, 8),
                     F32(a3, 12),
                     F32(a3, 16),
                     16000) )
              {
                W_particle_spawn(
                  c,
                  (int)c,
                  100068,
                  (_DWORD *)a3,
                  v121,
                  v123,
                  (int)(*v387__f),
                  1,
                  0,
                  0,
                  2,
                  0,
                  0,
                  20);
              }
              (*v387__f) = F32(a3, 44);
              if ( W_view_box_visible(
                     *(intptr_t *)&GAP(W, 0x1030, 28),
                     F32(a3, 4),
                     F32(a3, 8),
                     F32(a3, 12),
                     F32(a3, 16),
                     16000) )
              {
                W_particle_spawn(
                  c,
                  (int)c,
                  100068,
                  (_DWORD *)a3,
                  -v121,
                  -v123,
                  (int)(*v387__f),
                  1,
                  0,
                  0,
                  2,
                  0,
                  0,
                  0);
              }
            }
            if ( F16(a3, 88) >= 3u )
            {
              v124 = errno_decorate;
            }
            else
            {
              v124 = errno_decorate;
              if ( W_body_hit_spawn(
                     c,
                     F32(c, 0x20c),
                     a3,
                     0,
                     0,
                     F32(a3, 12),
                     F32(a3, 16),
                     9,
                     errno_decorate,
                     1,
                     F32(c, 0x234),
                     F32(c, 0x238),
                     F32(c, 0x23c),
                     9,
                     F32(c, 0x200),
                     1,
                     1,
                     F32(c, 0x220),
                     1,
                     F32(c, 0x244),
                     F32(c, 0x224),
                     0,
                     0,
                     0) != 0 )
              {
                v125 = F32(c, 0x22c) + 1;
                if ( v125 > 100000 )
                  v125 = 100000;
                v126 = F32(c, 0x228);
                F32(c, 0x22c) = v125;
                if ( v126 != 0 && v125 >= v126 )
                  W_shot_end(c, (_BYTE *)a3, errno_decorate);
              }
            }
            if ( v124 == 0 )
            {
              v127 = F32(c, 0x1b0);
              if ( v127 != 0 )
              {
                if ( F16(c, 0x1b4) != 0 )
                  F32(v127, 20) = 1;
                else
                  g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
              }
            }
            break;
          case 2:
            if ( F32(a3, 92) > 40 )
              ++F16(a3, 88);
            goto LABEL_266;
          case 3:
            v114 = F32(a3, 12);
            if ( v114 <= 3200 )
              goto LABEL_503;
            F32(a3, 12) = v114 - 800;
            F32(a3, 4) += 400;
            F32(a3, 16) -= 800;
            v113 = F32(a3, 8) + 400;
            goto LABEL_265;
          default:
            goto LABEL_266;
        }
        goto LABEL_763;
      default:
        goto LABEL_763;
    }
  }
  return v8;
}

/* 0x55d140: __int16 __thiscall sub_55D140( cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, __int16 a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18) */
int32_t shot_init(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int16_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15, int32_t a16, int32_t a17, int32_t a18)
{
    intptr_t c = (intptr_t)self;
    void * v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    uint16_t * v31;
    intptr_t k;
    intptr_t v33;
    int v34;
    intptr_t v35;
    intptr_t v36;
    intptr_t v37;
    int v38;
    intptr_t v39;
    intptr_t v40;
    intptr_t v41;
    intptr_t v42;
    intptr_t v43;
    intptr_t v44;
    intptr_t v45;
    intptr_t v46;
    intptr_t v47;
    intptr_t v48;
    intptr_t v49;
    intptr_t v50;
    intptr_t v51;
    intptr_t v52;
    intptr_t v53;
    intptr_t v54;
    uint16_t * v55;
    intptr_t i;
    intptr_t v57;
    intptr_t v58;
    uint16_t * v59;
    intptr_t v60;
    intptr_t v61;
    intptr_t v62;
    uint16_t * v63;
    intptr_t j;
    intptr_t v65;
    intptr_t v66;
    uint16_t * v67;
    intptr_t v68;
    intptr_t v69;
    intptr_t v70;
    intptr_t v71;
    uint16_t * v72;
    intptr_t n;
    intptr_t v74;
    intptr_t v75;
    intptr_t v76;
    intptr_t v77;
    intptr_t v78;
    intptr_t v79;
    uint16_t * v80;
    intptr_t m;
    intptr_t v82;
    intptr_t v83;
    intptr_t v84;
    intptr_t v85;
    intptr_t v86;
    intptr_t v87;
    uint16_t * v88;
    intptr_t ii;
    intptr_t v90;
    intptr_t v91;
    intptr_t v92;
    intptr_t v93;
    uint32_t v94;
    intptr_t v95;
    uint32_t v96;
    intptr_t v97;
    intptr_t v98;
    intptr_t v99;
    intptr_t v100;
    intptr_t v101;
    intptr_t v102;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v108__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v107__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v106__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v105__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v104__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    char *const v110__f = (char *)(frame_ + 0x24);   /* ebp-0x10 */
    intptr_t *const v111__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0xc */
    intptr_t *const v112__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x8 */
    intptr_t *const v113__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x4 */

  (*v110__f) = a9 == 0;
  (*v111__f) = F32(c, 0x40);
  (*v113__f) = 0;
  (*v112__f) = 1;
  W_entity_snd_free(c);
  F16(c, 0x1b4) = 1;
  v19 = (intptr_t)g_eng.op_new(0x24u);
  F32(c, 0x1b0) = (int)v19;
  memset(v19, 0, 36 * F16(c, 0x1b4));
  v20 = a2;
  F32(c, 0x200) = a10;
  F32(c, 0x20c) = v20;
  F32(c, 0x204) = 0;
  F32(c, 0x208) = 0;
  v21 = F32(W, 0x2788);
  if ( F16(v21, 8) < F16(v21, 0) )
    F32((F32(v21, 4) + 4 * (uint16_t)(F16(v21, 8))++), 0) = c;
  v22 = a13;
  v23 = a14;
  F32(c, 0x220) = a12;
  v24 = a15;
  F32(c, 0x234) = v22;
  v25 = a18;
  F32(c, 0x238) = v23;
  F32(c, 0x23c) = v24;
  v26 = a3;
  F32(c, 0x228) = 0;
  v27 = a7;
  F32(c, 0x22c) = 0;
  F32(c, 0x244) = 3;
  F32(c, 0x240) = 0;
  F32(c, 0x224) = v25;
  F32(c, 0x230) = 0;
  F32(c, 0x254) = 0;
  F32(c, 0x1f8) = 0;
  switch ( v26 )
  {
    case 100001:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 60000;
      F32(c, 0x244) = 2;
      F32(c, 0x228) = 1;
      goto LABEL_20;
    case 100002:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      (*v113__f) = 1;
      F32((*v111__f), 40) = 20000;
      F32(c, 0x228) = 1;
      F16(c, 0x1fc) = 60;
      goto LABEL_20;
    case 100003:
      W_entity_setup(c, 6, v26, a4, a5, a6, 7200, 7200, v27, (*v110__f), -12, -12, a11, 16, 0);
      W_entity_track_on(c, 0, 407, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 0;
      F16(c, 0x1fc) = 80;
      F32(c, 0xfc) = 24;
      F32(c, 0x100) = 24;
      goto LABEL_20;
    case 100004:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 3200, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 80000;
      F16(c, 0x1fc) = 80;
      F32(c, 0x254) = 1;
      goto LABEL_20;
    case 100005:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 19200, v27, (*v110__f), -64, 0, a11, 16, 0);
      W_entity_set_track(c, 0, 413, 0, 1, 0, 0, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 40000;
      F16(c, 0x1fc) = 200;
      F32(c, 0x254) = 1;
      if ( F32((*v111__f), 108) != D(0x6461a8) )
      {
        F32((*v111__f), 108) = D(0x6461a8);
        F8((*v111__f), 116) = 0;
        F16((*v111__f), 114) = 0;
        F16((*v111__f), 112) = 0;
      }
      v98 = SYS32(10072);
      F32(c, 0xf4) = SYS32(10072);
      F32(c, 0xf8) = v98;
      goto LABEL_20;
    case 100006:
      W_entity_setup(c, 6, v26, a4, a5, a6, 6400, 6400, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F16(c, 0x1fc) = 30;
      F32((*v111__f), 40) = 64000;
      F32(c, 0x228) = 1;
      goto LABEL_20;
    case 100007:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 8000, v27, (*v110__f), 0, 0, a11, 16, 0);
      F16(c, 0x1fc) = 280;
      F32(c, 0x228) = 4;
      goto LABEL_19;
    case 100008:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 20000;
      F16(c, 0x1fc) = 160;
      F32(c, 0x228) = 1;
      goto LABEL_20;
    case 100009:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 3200, v27, (*v110__f), 16, 16, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 20000;
      F16(c, 0x1fc) = 60;
      F32(c, 0x254) = 1;
      goto LABEL_20;
    case 100010:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4800, 4800, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 20000;
      F32(c, 0x228) = 3;
      F16(c, 0x1fc) = 96;
      F32(c, 0x254) = 1;
      goto LABEL_20;
    case 100011:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), -12, -12, a11, 16, 0);
      F32(c, 0x228) = 1;
      F16(c, 0x1fc) = 199;
      W_entity_track_on(c, 0, 430, 24);
      v41 = shot_55f150(70, 100);
      goto LABEL_18;
    case 100012:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), -12, -12, a11, 16, 0);
      W_entity_track_on(c, 0, 430, 24);
      v99 = shot_55f150(700, 1000);
      v42 = (*v111__f);
      F32(c, 0xf4) = v99;
      F32(c, 0xf8) = v99;
      F16(c, 0x1fc) = 250;
      F32((*v111__f), 40) = 40000;
      goto LABEL_20;
    case 100013:
      W_entity_setup(c, 6, v26, a4, a5, a6, 56000, 40000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F16(c, 0x1fc) = 250;
      F32((*v111__f), 40) = 40000;
      F32(c, 0x254) = 1;
      goto LABEL_20;
    case 100014:
      W_entity_setup(c, 6, v26, a4, a5, a6, 2400, 2400, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 0;
      F32((*v111__f), 24) = 0;
      v96 = F32(W, 0x27a0);
      if ( v96 >= 0xFFFFFF )
      {
        v97 = v96 + 1;
        F32(W, 0x27a0) = 0;
      }
      else
      {
        v97 = v96 + 1;
        F32(W, 0x27a0) = v97;
      }
      F32(c, 0x220) = v97;
      goto LABEL_20;
    case 100015:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 3200, v27, (*v110__f), -16, -16, a11, 16, 0);
      W_entity_set_track(c, 0, 410, 0, 1, 0, 0, 0);
      F32(c, 0xec) = 1;
      F16(c, 0xf0) = 24;
      v93 = SYS32(10152);
      (*v112__f) = 0;
      F32(c, 0xf4) = SYS32(10152);
      F32(c, 0xf8) = v93;
      v94 = F32(W, 0x27a0);
      if ( v94 >= 0xFFFFFF )
      {
        v95 = v94 + 1;
        F32(W, 0x27a0) = 0;
      }
      else
      {
        v95 = v94 + 1;
        F32(W, 0x27a0) = v95;
      }
      F32(c, 0x220) = v95;
      goto LABEL_19;
    case 100016:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F16(c, 0x1fc) = 140;
      F32((*v111__f), 40) = 20000;
      goto LABEL_20;
    case 100017:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 6400, v27, (*v110__f), 0, 0, a11, 16, 0);
      F16(c, 0x1fc) = 280;
      F32(c, 0x228) = 2;
      goto LABEL_19;
    case 100018:
    case 100019:
    case 100020:
    case 100041:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 6400, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 40000;
      F16(c, 0x1fc) = 60;
      goto LABEL_20;
    case 100021:
      W_entity_setup(c, 6, v26, a4, a5, a6, 8000, 8000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 96000;
      F16(c, 0x1fc) = 60;
      goto LABEL_20;
    case 100022:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 3200, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 80000;
      F16(c, 0x1fc) = 60;
      goto LABEL_20;
    case 100023:
      v52 = a6;
      if ( F32(CFG(), 148) != 0 )
      {
        F32(c, 0x1b8) = 6;
        F32(c, 0x1bc) = 0;
        F32(c, 0x1d4) = v26;
        v53 = a4;
        v54 = a5;
        F32(c, 0x1d0) = 1;
        F32(c, 0x1dc) = v53;
        F32(c, 0xf4) = 0;
        F32(c, 0xf8) = 0;
        F32(c, 0x3c) = 0;
        F16(c, 0x1ec) = 1;
        F32(c, 0x1c4) = 0;
        F32(c, 0x1c8) = 0;
        F32(c, 0x1cc) = 0;
        F32(c, 0x1c0) = 1;
        F32(c, 0x1c) = 0;
        F32(c, 0x20) = 32;
        a6 = v54;
        F32(c, 0x24) = 0;
        a10 = v52;
        F32(c, 0x1f0) = 0;
        F32(c, 0x1f4) = 0;
        F32(c, 0x38) = 0;
        v55 = &F16(c, 0x4a);
        for ( i = 8; i != 0; --i )
        {
          *(v55 - 1) = 0;
          *v55 = 0;
          *((uint8_t *)v55 + 2) = 0;
          F32(v55 + (3), 0) = 0;
          F32(v55 + (5), 0) = 0;
          F32(v55 + (7), 0) = 0;
          v55 += 10;
        }
        F32(c, 0xfc) = 16;
        F32(c, 0x100) = 16;
        v57 = a11;
        F32(c, 0x28) = 9600;
        F32(c, 0x2c) = 9600;
        F32(c, 0x30) = -24;
        F32(c, 0x34) = -24;
        F16(c, 0xe8) = 0;
        (*v105__f) = a5;
        F32(c, 0x1d8) = 0;
        F32(c, 0x104) = 0;
        F32(c, 0x108) = 0;
        F32(c, 0x1e8) = 0;
        F16(c, 0x1e4) = 1;
        F32(c, 0x1e0) = 0;
        F32(c, 0xc) = 0;
        F32(c, 0x10) = 0;
        W_entity_anchor(c, (*v105__f), v52, v57, &a6, &a10);
        v34 = F16(c, 0x44) == 0;
        F32(c, 0xec) = 0;
        F16(c, 0xf0) = 1;
        a5 = 0;
        if ( !v34 )
        {
          do
          {
            v58 = 660 * (uint16_t)a5;
            F8((v58 + F32(c, 0x40)), 0) = (*v110__f);
            F32(v58 + F32(c, 0x40), 4) = a6;
            F32(v58 + F32(c, 0x40), 8) = a10;
            F32(v58 + F32(c, 0x40), 12) = 9600;
            F32(v58 + F32(c, 0x40), 16) = 9600;
            F32(v58 + F32(c, 0x40), 20) = 0;
            F32(v58 + F32(c, 0x40), 24) = 0;
            F32(v58 + F32(c, 0x40), 28) = 0;
            F32(v58 + F32(c, 0x40), 40) = 0;
            F32(v58 + F32(c, 0x40), 44) = v27;
            F32(v58 + F32(c, 0x40), 48) = 0;
            F32(v58 + F32(c, 0x40), 52) = 0;
            F8(v58 + F32(c, 0x40), 32) = 0;
            F32(v58 + F32(c, 0x40), 64) = -24;
            F32(v58 + F32(c, 0x40), 68) = -24;
            F16(v58 + F32(c, 0x40), 102) = 0;
            F8(v58 + F32(c, 0x40), 104) = 0;
            F8(v58 + F32(c, 0x40), 124) = 0;
            F8(v58 + F32(c, 0x40), 125) = 0;
            F8(v58 + F32(c, 0x40), 126) = 0;
            F32(v58 + F32(c, 0x40), 644) = 0;
            F32(v58 + F32(c, 0x40), 36) = 1;
            W_body_action_reset(F32(c, 0x40) + v58);
            v59 = (_WORD *)(F32(c, 0x40) + v58);
            v59[28] = 0;
            v59[29] = 0;
            v59[30] = 0;
            W_body_set_clip(F32(c, 0x40) + v58, 0);
            v34 = (uint16_t)a5 == 0;
            F16(v58 + F32(c, 0x40), 128) = 0;
            if ( v34 )
            {
              W_body_region_flags((_DWORD *)F32(c, 0x40));
            }
            else
            {
              F32(v58 + F32(c, 0x40), 652) = F32(F32(c, 0x40), 652);
              F32(v58 + F32(c, 0x40), 648) = F32(F32(c, 0x40), 648);
            }
            F32(v58 + F32(c, 0x40), 656) = 0;
            v38 = (uint16_t)++a5 < F16(c, 0x44);
          }
          while ( v38 );
        }
        W_entity_fx_reset(c, 0);
        if ( F32(c, 0x0) != 0 )
        {
          g_eng.op_delete((void *)(void *)F32(c, 0x0));
          F32(c, 0x0) = 0;
        }
        v60 = F32(c, 0x8);
        if ( v60 != 0 )
        {
          W_parts_free(v60, 1);
          F32(c, 0x8) = 0;
        }
        F16(c, 0x4) = 0;
        W_entity_set_track(c, 0, 429, 0, 1, 0, 0, 0);
      }
      else
      {
        F32(c, 0x1b8) = 6;
        F32(c, 0x1bc) = 0;
        F32(c, 0x1d4) = v26;
        v61 = a4;
        v62 = a5;
        F32(c, 0x1d0) = 1;
        F32(c, 0x1dc) = v61;
        F32(c, 0xf4) = 0;
        F32(c, 0xf8) = 0;
        F32(c, 0x3c) = 0;
        F16(c, 0x1ec) = 1;
        F32(c, 0x1c4) = 0;
        F32(c, 0x1c8) = 0;
        F32(c, 0x1cc) = 0;
        F32(c, 0x1c0) = 1;
        F32(c, 0x1c) = 0;
        F32(c, 0x20) = 32;
        a6 = v62;
        F32(c, 0x24) = 0;
        a10 = v52;
        F32(c, 0x1f0) = 0;
        F32(c, 0x1f4) = 0;
        F32(c, 0x38) = 0;
        v63 = &F16(c, 0x4a);
        for ( j = 8; j != 0; --j )
        {
          *(v63 - 1) = 0;
          *v63 = 0;
          *((uint8_t *)v63 + 2) = 0;
          F32(v63 + (3), 0) = 0;
          F32(v63 + (5), 0) = 0;
          F32(v63 + (7), 0) = 0;
          v63 += 10;
        }
        F32(c, 0xfc) = 16;
        F32(c, 0x100) = 16;
        v65 = a11;
        F32(c, 0x28) = 9600;
        F32(c, 0x2c) = 9600;
        (*v106__f) = a5;
        F16(c, 0xe8) = 0;
        F32(c, 0x1d8) = 0;
        F32(c, 0x104) = 0;
        F32(c, 0x108) = 0;
        F32(c, 0x1e8) = 0;
        F16(c, 0x1e4) = 1;
        F32(c, 0x1e0) = 0;
        F32(c, 0xc) = 0;
        F32(c, 0x10) = 0;
        F32(c, 0x30) = 0;
        F32(c, 0x34) = 0;
        W_entity_anchor(c, (*v106__f), v52, v65, &a6, &a10);
        v34 = F16(c, 0x44) == 0;
        F32(c, 0xec) = 0;
        F16(c, 0xf0) = 1;
        a5 = 0;
        if ( !v34 )
        {
          do
          {
            v66 = 660 * (uint16_t)a5;
            F8((v66 + F32(c, 0x40)), 0) = (*v110__f);
            F32(v66 + F32(c, 0x40), 4) = a6;
            F32(v66 + F32(c, 0x40), 8) = a10;
            F32(v66 + F32(c, 0x40), 12) = 9600;
            F32(v66 + F32(c, 0x40), 16) = 9600;
            F32(v66 + F32(c, 0x40), 20) = 0;
            F32(v66 + F32(c, 0x40), 24) = 0;
            F32(v66 + F32(c, 0x40), 28) = 0;
            F32(v66 + F32(c, 0x40), 40) = 0;
            F32(v66 + F32(c, 0x40), 44) = v27;
            F32(v66 + F32(c, 0x40), 48) = 0;
            F32(v66 + F32(c, 0x40), 52) = 0;
            F8(v66 + F32(c, 0x40), 32) = 0;
            F32(v66 + F32(c, 0x40), 64) = 0;
            F32(v66 + F32(c, 0x40), 68) = 0;
            F16(v66 + F32(c, 0x40), 102) = 0;
            F8(v66 + F32(c, 0x40), 104) = 0;
            F8(v66 + F32(c, 0x40), 124) = 0;
            F8(v66 + F32(c, 0x40), 125) = 0;
            F8(v66 + F32(c, 0x40), 126) = 0;
            F32(v66 + F32(c, 0x40), 644) = 0;
            F32(v66 + F32(c, 0x40), 36) = 1;
            W_body_action_reset(F32(c, 0x40) + v66);
            v67 = (_WORD *)(F32(c, 0x40) + v66);
            v67[28] = 0;
            v67[29] = 0;
            v67[30] = 0;
            W_body_set_clip(F32(c, 0x40) + v66, 0);
            v34 = (uint16_t)a5 == 0;
            F16(v66 + F32(c, 0x40), 128) = 0;
            if ( v34 )
            {
              W_body_region_flags((_DWORD *)F32(c, 0x40));
            }
            else
            {
              F32(v66 + F32(c, 0x40), 652) = F32(F32(c, 0x40), 652);
              F32(v66 + F32(c, 0x40), 648) = F32(F32(c, 0x40), 648);
            }
            F32(v66 + F32(c, 0x40), 656) = 0;
            v38 = (uint16_t)++a5 < F16(c, 0x44);
          }
          while ( v38 );
        }
        W_entity_fx_reset(c, 0);
        if ( F32(c, 0x0) != 0 )
        {
          g_eng.op_delete((void *)(void *)F32(c, 0x0));
          F32(c, 0x0) = 0;
        }
        v68 = F32(c, 0x8);
        if ( v68 != 0 )
        {
          W_parts_free(v68, 1);
          F32(c, 0x8) = 0;
        }
        F16(c, 0x4) = 0;
        W_entity_set_track(c, 0, 428, 2, 1, 0, 0, 0);
      }
      goto LABEL_17;
    case 100024:
      F32(c, 0x1b8) = 6;
      F32(c, 0x1bc) = 0;
      F32(c, 0x1d4) = v26;
      v28 = a4;
      v29 = a5;
      F32(c, 0x1d0) = 1;
      F32(c, 0x1dc) = v28;
      F32(c, 0xf4) = 0;
      v30 = a6;
      F32(c, 0xf8) = 0;
      F32(c, 0x3c) = 0;
      F16(c, 0x1ec) = 1;
      F32(c, 0x1c4) = 0;
      F32(c, 0x1c8) = 0;
      F32(c, 0x1cc) = 0;
      F32(c, 0x1c0) = 1;
      F32(c, 0x1c) = 0;
      F32(c, 0x20) = 32;
      a6 = v29;
      F32(c, 0x24) = 0;
      a10 = v30;
      F32(c, 0x1f0) = 0;
      F32(c, 0x1f4) = 0;
      F32(c, 0x38) = 0;
      v31 = &F16(c, 0x4a);
      for ( k = 8; k != 0; --k )
      {
        *(v31 - 1) = 0;
        *v31 = 0;
        *((uint8_t *)v31 + 2) = 0;
        F32(v31 + (3), 0) = 0;
        F32(v31 + (5), 0) = 0;
        F32(v31 + (7), 0) = 0;
        v31 += 10;
      }
      F32(c, 0xfc) = 16;
      F32(c, 0x100) = 16;
      v33 = a11;
      F32(c, 0x28) = 9600;
      F32(c, 0x2c) = 9600;
      (*v104__f) = a5;
      F16(c, 0xe8) = 0;
      F32(c, 0x1d8) = 0;
      F32(c, 0x104) = 0;
      F32(c, 0x108) = 0;
      F32(c, 0x1e8) = 0;
      F16(c, 0x1e4) = 1;
      F32(c, 0x1e0) = 0;
      F32(c, 0xc) = 0;
      F32(c, 0x10) = 0;
      F32(c, 0x30) = 0;
      F32(c, 0x34) = 0;
      W_entity_anchor(c, (*v104__f), v30, v33, &a6, &a10);
      v34 = F16(c, 0x44) == 0;
      F32(c, 0xec) = 0;
      F16(c, 0xf0) = 1;
      a5 = 0;
      if ( !v34 )
      {
        do
        {
          v35 = 660 * (uint16_t)a5;
          F8((F32(c, 0x40) + v35), 0) = (*v110__f);
          F32(F32(c, 0x40) + v35, 4) = a6;
          F32(F32(c, 0x40) + v35, 8) = a10;
          F32(F32(c, 0x40) + v35, 12) = 9600;
          F32(F32(c, 0x40) + v35, 16) = 9600;
          F32(F32(c, 0x40) + v35, 20) = 0;
          F32(F32(c, 0x40) + v35, 24) = 0;
          F32(F32(c, 0x40) + v35, 28) = 0;
          F32(F32(c, 0x40) + v35, 40) = 0;
          F32(F32(c, 0x40) + v35, 44) = v27;
          F32(F32(c, 0x40) + v35, 48) = 0;
          F32(F32(c, 0x40) + v35, 52) = 0;
          F8(F32(c, 0x40) + v35, 32) = 0;
          F32(F32(c, 0x40) + v35, 64) = 0;
          F32(F32(c, 0x40) + v35, 68) = 0;
          F16(F32(c, 0x40) + v35, 102) = 0;
          F8(F32(c, 0x40) + v35, 104) = 0;
          F8(F32(c, 0x40) + v35, 124) = 0;
          F8(F32(c, 0x40) + v35, 125) = 0;
          F8(F32(c, 0x40) + v35, 126) = 0;
          F32(F32(c, 0x40) + v35, 644) = 0;
          F32(F32(c, 0x40) + v35, 36) = 1;
          W_body_action_reset(v35 + F32(c, 0x40));
          v36 = F32(c, 0x40);
          F16(v36 + v35, 56) = 0;
          v37 = v35 + v36;
          F16(v37, 58) = 0;
          F16(v37, 60) = 0;
          W_body_set_clip(v35 + F32(c, 0x40), 0);
          v34 = (uint16_t)a5 == 0;
          F16(F32(c, 0x40) + v35, 128) = 0;
          if ( v34 )
          {
            W_body_region_flags((_DWORD *)F32(c, 0x40));
          }
          else
          {
            F32(v35 + F32(c, 0x40), 652) = F32(F32(c, 0x40), 652);
            F32(v35 + F32(c, 0x40), 648) = F32(F32(c, 0x40), 648);
          }
          F32(F32(c, 0x40) + v35, 656) = 0;
          v38 = (uint16_t)++a5 < F16(c, 0x44);
        }
        while ( v38 );
      }
      W_entity_fx_reset(c, 0);
      if ( F32(c, 0x0) != 0 )
      {
        g_eng.op_delete((void *)(void *)F32(c, 0x0));
        F32(c, 0x0) = 0;
      }
      v39 = F32(c, 0x8);
      if ( v39 != 0 )
      {
        W_parts_free(v39, 1);
        F32(c, 0x8) = 0;
      }
      F16(c, 0x4) = 0;
      W_entity_set_track(c, 0, 428, 0, 1, 0, 0, 0);
LABEL_17:
      v40 = F32(c, 0x40);
      F32(c, 0x10) = a2;
      W_entity_ride_part(c, v40, 0);
      v41 = SYS32(10072);
LABEL_18:
      F32(c, 0xf4) = v41;
      F32(c, 0xf8) = v41;
      goto LABEL_19;
    case 100025:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 800, v27, (*v110__f), -8, -20, a11, 16, 0);
      W_entity_set_track(c, 0, 408, 0, 1, 0, 0, 0);
      goto LABEL_92;
    case 100026:
      F32(c, 0x1b8) = 6;
      F32(c, 0x1bc) = 0;
      F32(c, 0x1d4) = v26;
      v77 = a4;
      v78 = a5;
      F32(c, 0x1d0) = 1;
      F32(c, 0x1dc) = v77;
      F32(c, 0xf4) = 0;
      v79 = a6;
      F32(c, 0xf8) = 0;
      F32(c, 0x3c) = 0;
      F16(c, 0x1ec) = 1;
      F32(c, 0x1c4) = 0;
      F32(c, 0x1c8) = 0;
      F32(c, 0x1cc) = 0;
      F32(c, 0x1c0) = 1;
      F32(c, 0x1c) = 0;
      F32(c, 0x20) = 32;
      a6 = v78;
      F32(c, 0x24) = 0;
      a2 = v79;
      F32(c, 0x1f0) = 0;
      F32(c, 0x1f4) = 0;
      F32(c, 0x38) = 0;
      v80 = &F16(c, 0x4a);
      for ( m = 8; m != 0; --m )
      {
        *(v80 - 1) = 0;
        *v80 = 0;
        *((uint8_t *)v80 + 2) = 0;
        F32(v80 + (3), 0) = 0;
        F32(v80 + (5), 0) = 0;
        F32(v80 + (7), 0) = 0;
        v80 += 10;
      }
      F32(c, 0xfc) = 16;
      F32(c, 0x100) = 16;
      v82 = a11;
      F16(c, 0xe8) = 0;
      (*v108__f) = a5;
      F32(c, 0x1d8) = 0;
      F32(c, 0x104) = 0;
      F32(c, 0x108) = 0;
      F32(c, 0x1e8) = 0;
      F16(c, 0x1e4) = 1;
      F32(c, 0x1e0) = 0;
      F32(c, 0xc) = 0;
      F32(c, 0x10) = 0;
      F32(c, 0x28) = 4000;
      F32(c, 0x2c) = 3200;
      F32(c, 0x30) = 0;
      F32(c, 0x34) = 0;
      W_entity_anchor(c, (*v108__f), v79, v82, &a6, &a2);
      v34 = F16(c, 0x44) == 0;
      F32(c, 0xec) = 0;
      F16(c, 0xf0) = 1;
      a5 = 0;
      if ( !v34 )
      {
        do
        {
          v83 = 660 * (uint16_t)a5;
          F8((v83 + F32(c, 0x40)), 0) = (*v110__f);
          F32(v83 + F32(c, 0x40), 4) = a6;
          F32(v83 + F32(c, 0x40), 8) = a2;
          F32(v83 + F32(c, 0x40), 12) = 4000;
          F32(v83 + F32(c, 0x40), 16) = 3200;
          F32(v83 + F32(c, 0x40), 20) = 0;
          F32(v83 + F32(c, 0x40), 24) = 0;
          F32(v83 + F32(c, 0x40), 28) = 0;
          F32(v83 + F32(c, 0x40), 40) = 0;
          F32(v83 + F32(c, 0x40), 44) = v27;
          F32(v83 + F32(c, 0x40), 48) = 0;
          F32(v83 + F32(c, 0x40), 52) = 0;
          F8(v83 + F32(c, 0x40), 32) = 0;
          F32(v83 + F32(c, 0x40), 64) = 0;
          F32(v83 + F32(c, 0x40), 68) = 0;
          F16(v83 + F32(c, 0x40), 102) = 0;
          F8(v83 + F32(c, 0x40), 104) = 0;
          F8(v83 + F32(c, 0x40), 124) = 0;
          F8(v83 + F32(c, 0x40), 125) = 0;
          F8(v83 + F32(c, 0x40), 126) = 0;
          F32(v83 + F32(c, 0x40), 644) = 0;
          F32(v83 + F32(c, 0x40), 36) = 1;
          W_body_action_reset(F32(c, 0x40) + v83);
          W_body_set_state((_WORD *)(F32(c, 0x40) + v83), 0);
          W_body_set_clip(F32(c, 0x40) + v83, 0);
          v34 = (uint16_t)a5 == 0;
          F16(v83 + F32(c, 0x40), 128) = 0;
          if ( v34 )
          {
            W_body_region_flags((_DWORD *)F32(c, 0x40));
          }
          else
          {
            F32(v83 + F32(c, 0x40), 652) = F32(F32(c, 0x40), 652);
            F32(v83 + F32(c, 0x40), 648) = F32(F32(c, 0x40), 648);
          }
          F32(v83 + F32(c, 0x40), 656) = 0;
          v38 = (uint16_t)++a5 < F16(c, 0x44);
        }
        while ( v38 );
      }
      W_entity_fx_reset(c, 0);
      if ( F32(c, 0x0) != 0 )
      {
        g_eng.op_delete((void *)(void *)F32(c, 0x0));
        F32(c, 0x0) = 0;
      }
      v84 = F32(c, 0x8);
      if ( v84 != 0 )
      {
        W_parts_free(v84, 1);
        F32(c, 0x8) = 0;
      }
      F16(c, 0x4) = 0;
      F16(c, 0x1fc) = 40;
      goto LABEL_19;
    case 100027:
      F32(c, 0x1b8) = 6;
      F32(c, 0x1bc) = 0;
      F32(c, 0x1d4) = v26;
      v69 = a4;
      v70 = a5;
      F32(c, 0x1d0) = 1;
      F32(c, 0x1dc) = v69;
      F32(c, 0xf4) = 0;
      v71 = a6;
      F32(c, 0xf8) = 0;
      F32(c, 0x3c) = 0;
      F16(c, 0x1ec) = 1;
      F32(c, 0x1c4) = 0;
      F32(c, 0x1c8) = 0;
      F32(c, 0x1cc) = 0;
      F32(c, 0x1c0) = 1;
      F32(c, 0x1c) = 0;
      F32(c, 0x20) = 32;
      a6 = v70;
      F32(c, 0x24) = 0;
      a2 = v71;
      F32(c, 0x1f0) = 0;
      F32(c, 0x1f4) = 0;
      F32(c, 0x38) = 0;
      v72 = &F16(c, 0x4a);
      for ( n = 8; n != 0; --n )
      {
        *(v72 - 1) = 0;
        *v72 = 0;
        *((uint8_t *)v72 + 2) = 0;
        F32(v72 + (3), 0) = 0;
        F32(v72 + (5), 0) = 0;
        F32(v72 + (7), 0) = 0;
        v72 += 10;
      }
      F32(c, 0xfc) = 16;
      F32(c, 0x100) = 16;
      v74 = a11;
      F32(c, 0x28) = 1600;
      F32(c, 0x2c) = 1600;
      (*v107__f) = a5;
      F16(c, 0xe8) = 0;
      F32(c, 0x1d8) = 0;
      F32(c, 0x104) = 0;
      F32(c, 0x108) = 0;
      F32(c, 0x1e8) = 0;
      F16(c, 0x1e4) = 1;
      F32(c, 0x1e0) = 0;
      F32(c, 0xc) = 0;
      F32(c, 0x10) = 0;
      F32(c, 0x30) = 0;
      F32(c, 0x34) = 0;
      W_entity_anchor(c, (*v107__f), v71, v74, &a6, &a2);
      v34 = F16(c, 0x44) == 0;
      F32(c, 0xec) = 0;
      F16(c, 0xf0) = 1;
      a5 = 0;
      if ( !v34 )
      {
        do
        {
          v75 = 660 * (uint16_t)a5;
          F8((v75 + F32(c, 0x40)), 0) = (*v110__f);
          F32(v75 + F32(c, 0x40), 4) = a6;
          F32(v75 + F32(c, 0x40), 8) = a2;
          F32(v75 + F32(c, 0x40), 12) = 1600;
          F32(v75 + F32(c, 0x40), 16) = 1600;
          F32(v75 + F32(c, 0x40), 20) = 0;
          F32(v75 + F32(c, 0x40), 24) = 0;
          F32(v75 + F32(c, 0x40), 28) = 0;
          F32(v75 + F32(c, 0x40), 40) = 0;
          F32(v75 + F32(c, 0x40), 44) = v27;
          F32(v75 + F32(c, 0x40), 48) = 0;
          F32(v75 + F32(c, 0x40), 52) = 0;
          F8(v75 + F32(c, 0x40), 32) = 0;
          F32(v75 + F32(c, 0x40), 64) = 0;
          F32(v75 + F32(c, 0x40), 68) = 0;
          F16(v75 + F32(c, 0x40), 102) = 0;
          F8(v75 + F32(c, 0x40), 104) = 0;
          F8(v75 + F32(c, 0x40), 124) = 0;
          F8(v75 + F32(c, 0x40), 125) = 0;
          F8(v75 + F32(c, 0x40), 126) = 0;
          F32(v75 + F32(c, 0x40), 644) = 0;
          F32(v75 + F32(c, 0x40), 36) = 1;
          W_body_action_reset(F32(c, 0x40) + v75);
          W_body_set_state((_WORD *)(F32(c, 0x40) + v75), 0);
          W_body_set_clip(F32(c, 0x40) + v75, 0);
          v34 = (uint16_t)a5 == 0;
          F16(v75 + F32(c, 0x40), 128) = 0;
          if ( v34 )
          {
            W_body_region_flags((_DWORD *)F32(c, 0x40));
          }
          else
          {
            F32(v75 + F32(c, 0x40), 652) = F32(F32(c, 0x40), 652);
            F32(v75 + F32(c, 0x40), 648) = F32(F32(c, 0x40), 648);
          }
          F32(v75 + F32(c, 0x40), 656) = 0;
          v38 = (uint16_t)++a5 < F16(c, 0x44);
        }
        while ( v38 );
      }
      W_entity_fx_reset(c, 0);
      if ( F32(c, 0x0) != 0 )
      {
        g_eng.op_delete((void *)(void *)F32(c, 0x0));
        F32(c, 0x0) = 0;
      }
      v76 = F32(c, 0x8);
      if ( v76 != 0 )
      {
        W_parts_free(v76, 1);
        F32(c, 0x8) = 0;
      }
      v42 = (*v111__f);
      F16(c, 0x4) = 0;
      F32((*v111__f), 40) = 60000;
      F16(c, 0x1fc) = 120;
      goto LABEL_99;
    case 100028:
      W_entity_setup(c, 6, v26, a4, a5, a6, 1600, 1600, v27, (*v110__f), -16, -16, a11, 16, 0);
      W_entity_set_track(c, 0, 408, 1, 1, 0, 0, 0);
LABEL_92:
      v42 = (*v111__f);
      F32((*v111__f), 40) = 56000;
      F16(c, 0x1fc) = 60;
      goto LABEL_99;
    case 100029:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 1600, v27, (*v110__f), -8, -16, a11, 16, 0);
      W_entity_set_track(c, 0, 409, 0, 1, 0, 0, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 80000;
      F32(c, 0x228) = 1;
      F16(c, 0x1fc) = 90;
      F32(c, 0x230) = 1;
      F32(c, 0xec) = 1;
      F16(c, 0xf0) = 16;
      goto LABEL_20;
    case 100030:
      W_entity_setup(c, 6, v26, a4, a5, a6, 3200, 1600, v27, (*v110__f), -8, -16, a11, 16, 0);
      W_entity_set_track(c, 0, 321, 0, 1, 0, 0, 0);
      v42 = (*v111__f);
      if ( F32((*v111__f), 108) != D(0x650358) )
      {
        F32((*v111__f), 108) = D(0x650358);
        F8((*v111__f), 116) = 0;
        F16((*v111__f), 114) = 0;
        F16((*v111__f), 112) = 0;
      }
      F32((*v111__f), 40) = 80000;
      F16(c, 0x1fc) = 90;
LABEL_99:
      F32(c, 0x228) = 1;
      F32(c, 0x230) = 1;
      goto LABEL_20;
    case 100031:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), -12, -12, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 40000;
      F32(c, 0x228) = 1;
      goto LABEL_132;
    case 100032:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), -12, -12, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 0;
      F32(c, 0x228) = 1;
      F16(c, 0x1fc) = 80;
LABEL_132:
      W_entity_track_on(c, 0, 430, 24);
      v102 = shot_55f150(70, 100);
      F32(c, 0xf4) = v102;
      F32(c, 0xf8) = v102;
      goto LABEL_20;
    case 100033:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), -12, -12, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 80000;
      W_entity_track_on(c, 0, 430, 32);
      v101 = W_tint_fade_b(100, 100);
      F32(c, 0xf4) = v101;
      F32(c, 0xf8) = v101;
      goto LABEL_20;
    case 100034:
      W_entity_setup(c, 6, v26, a4, a5, a6, 12800, 12800, v27, (*v110__f), -16, -16, a11, 16, 0);
      W_entity_track_on(c, 0, 415, 0);
      F32(c, 0xec) = 1;
      F16(c, 0xf0) = 24;
      (*v112__f) = 0;
      goto LABEL_19;
    case 100035:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), -12, -12, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 0;
      F16(c, 0x1fc) = 160;
      v100 = rnd();
      W_entity_track_on(c, 0, 412, 4 * v100 / 0x8000 + 8);
      goto LABEL_20;
    case 100036:
      W_entity_setup(c, 6, v26, a4, a5, a6, 32000, 20000, v27, (*v110__f), 0, 0, a11, 16, 0);
      F16(c, 0x1fc) = 16;
      goto LABEL_19;
    case 100037:
    case 100038:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), 0, 0, a11, 16, 0);
      v42 = (*v111__f);
      F32((*v111__f), 40) = 80000;
      F16(c, 0x1fc) = 250;
      F32(c, 0x228) = 2;
      goto LABEL_20;
    case 100039:
      F32(c, 0x1b8) = 6;
      F32(c, 0x1bc) = 0;
      F32(c, 0x1d4) = v26;
      v85 = a4;
      v86 = a5;
      F32(c, 0x1d0) = 1;
      F32(c, 0x1dc) = v85;
      F32(c, 0xf4) = 0;
      v87 = a6;
      F32(c, 0xf8) = 0;
      F32(c, 0x3c) = 0;
      F16(c, 0x1ec) = 1;
      F32(c, 0x1c4) = 0;
      F32(c, 0x1c8) = 0;
      F32(c, 0x1cc) = 0;
      F32(c, 0x1c0) = 1;
      F32(c, 0x1c) = 0;
      F32(c, 0x20) = 32;
      a6 = v86;
      F32(c, 0x24) = 0;
      a2 = v87;
      F32(c, 0x1f0) = 0;
      F32(c, 0x1f4) = 0;
      F32(c, 0x38) = 0;
      v88 = &F16(c, 0x4a);
      for ( ii = 8; ii != 0; --ii )
      {
        *(v88 - 1) = 0;
        *v88 = 0;
        *((uint8_t *)v88 + 2) = 0;
        F32(v88 + (3), 0) = 0;
        F32(v88 + (5), 0) = 0;
        F32(v88 + (7), 0) = 0;
        v88 += 10;
      }
      F32(c, 0xfc) = 16;
      F32(c, 0x100) = 16;
      v90 = a11;
      F32(c, 0x28) = 4000;
      F32(c, 0x2c) = 4000;
      (*v109__f) = a5;
      F16(c, 0xe8) = 0;
      F32(c, 0x1d8) = 0;
      F32(c, 0x104) = 0;
      F32(c, 0x108) = 0;
      F32(c, 0x1e8) = 0;
      F16(c, 0x1e4) = 1;
      F32(c, 0x1e0) = 0;
      F32(c, 0xc) = 0;
      F32(c, 0x10) = 0;
      F32(c, 0x30) = 0;
      F32(c, 0x34) = 0;
      W_entity_anchor(c, (*v109__f), v87, v90, &a6, &a2);
      v34 = F16(c, 0x44) == 0;
      F32(c, 0xec) = 0;
      F16(c, 0xf0) = 1;
      a5 = 0;
      if ( !v34 )
      {
        do
        {
          v91 = 660 * (uint16_t)a5;
          F8((v91 + F32(c, 0x40)), 0) = (*v110__f);
          F32(v91 + F32(c, 0x40), 4) = a6;
          F32(v91 + F32(c, 0x40), 8) = a2;
          F32(v91 + F32(c, 0x40), 12) = 4000;
          F32(v91 + F32(c, 0x40), 16) = 4000;
          F32(v91 + F32(c, 0x40), 20) = 0;
          F32(v91 + F32(c, 0x40), 24) = 0;
          F32(v91 + F32(c, 0x40), 28) = 0;
          F32(v91 + F32(c, 0x40), 40) = 0;
          F32(v91 + F32(c, 0x40), 44) = v27;
          F32(v91 + F32(c, 0x40), 48) = 0;
          F32(v91 + F32(c, 0x40), 52) = 0;
          F8(v91 + F32(c, 0x40), 32) = 0;
          F32(v91 + F32(c, 0x40), 64) = 0;
          F32(v91 + F32(c, 0x40), 68) = 0;
          F16(v91 + F32(c, 0x40), 102) = 0;
          F8(v91 + F32(c, 0x40), 104) = 0;
          F8(v91 + F32(c, 0x40), 124) = 0;
          F8(v91 + F32(c, 0x40), 125) = 0;
          F8(v91 + F32(c, 0x40), 126) = 0;
          F32(v91 + F32(c, 0x40), 644) = 0;
          F32(v91 + F32(c, 0x40), 36) = 1;
          W_body_action_reset(F32(c, 0x40) + v91);
          W_body_set_state((_WORD *)(F32(c, 0x40) + v91), 0);
          W_body_set_clip(F32(c, 0x40) + v91, 0);
          v34 = (uint16_t)a5 == 0;
          F16(v91 + F32(c, 0x40), 128) = 0;
          if ( v34 )
          {
            W_body_region_flags((_DWORD *)F32(c, 0x40));
          }
          else
          {
            F32(v91 + F32(c, 0x40), 652) = F32(F32(c, 0x40), 652);
            F32(v91 + F32(c, 0x40), 648) = F32(F32(c, 0x40), 648);
          }
          F32(v91 + F32(c, 0x40), 656) = 0;
          v38 = (uint16_t)++a5 < F16(c, 0x44);
        }
        while ( v38 );
      }
      W_entity_fx_reset(c, 0);
      if ( F32(c, 0x0) != 0 )
      {
        g_eng.op_delete((void *)(void *)F32(c, 0x0));
        F32(c, 0x0) = 0;
      }
      v92 = F32(c, 0x8);
      if ( v92 != 0 )
      {
        W_parts_free(v92, 1);
        F32(c, 0x8) = 0;
      }
      v42 = (*v111__f);
      F16(c, 0x4) = 0;
      F32((*v111__f), 40) = 80000;
      F16(c, 0x1fc) = 60;
      F32(c, 0x228) = 1;
      goto LABEL_20;
    case 100040:
      W_entity_setup(c, 6, v26, a4, a5, a6, 4000, 4000, v27, (*v110__f), 0, 0, a11, 16, 0);
      F32(c, 0x254) = 1;
      goto LABEL_19;
    default:
LABEL_19:
      v42 = (*v111__f);
LABEL_20:
      F32(c, 0x1f0) = a17;
      if ( (*v113__f) != 0 )
      {
        a8 = (a8 + 360) % 360;
        if ( a8 > 180 )
        {
          a8 = 360 - a8;
          v27 = W_facing_back(v27);
        }
      }
      F32(v42, 44) = v27;
      W_body_set_angle(v42, 1000 * a8, (*v112__f));
      F32(c, 0x250) = 0;
      v43 = W_part_center_y(c, 0);
      v44 = F32(c, 0x20c);
      F32(c, 0x21c) = v43;
      if ( v44 != 0 )
      {
        a5 = W_part_center_x(c, 0);
        v45 = W_part_center_x(v44, 0);
        F32(c, 0x210) = a5 - v45;
        v46 = W_part_center_y(c, 0);
        v47 = W_part_center_y(F32(c, 0x20c), 0);
        v48 = F32(c, 0x20c);
        F32(c, 0x214) = v46 - v47;
        F32(c, 0x218) = W_part_facing(v48, 0);
      }
      SETLO(v49, a9);
      v34 = a9 == 0;
      F16(v42, 60) = a9;
      if ( !v34 )
      {
        W_body_action_start(v42, 1, 210);
        v50 = a16;
        v51 = a16;
        F32(c, 0x250) = a16;
        F32(c, 0x248) = F32(v42, 4) - W_part_left(v51, 0);
        v49 = W_char_part_top(v50, 0);
        F32(c, 0x24c) = F32(v42, 8) - v49;
      }
      return v49;
  }
}

/* 0x46da00: char __thiscall sub_46DA00(cls_4022A0 *this) */
int32_t band96_tick(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1;
    intptr_t * v3;
    intptr_t v4;
    int16_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    uint16_t v13;
    int16_t v14;
    int32_t * v15;
    intptr_t * v16;
    intptr_t v17;
    intptr_t v18;
    int v19;
    intptr_t v20;
    intptr_t * v21;
    intptr_t v22;
    void * v23;
    intptr_t v24;
    intptr_t v25;
    int32_t * v26;
    intptr_t v27;
    uint16_t v28;
    int16_t v29;
    intptr_t v30;
    intptr_t v31;
    intptr_t v32;
    intptr_t v33;
    _Alignas(8) uint8_t frame_[0x3c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3c */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x8 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x4 */

  v3 = (intptr_t *)F32(c, 0x204);
  v4 = F32(c, 0x40);
  if ( v3 != (intptr_t *)2 )
  {
    switch ( F32(c, 0x1d4) )
    {
      case 0x1873F:
      case 0x18740:
      case 0x18741:
        if ( v3 == (intptr_t *)3 && F16(v4, 88) < 0x64u )
        {
          v3 = (intptr_t *)W_vpool_take(SYS32(1736), 0);
          if ( v3 != 0 )
          {
            v30 = F32(v3, 0);
            v3 = (intptr_t *)W_sfx_place(
                                  c,
                                  F32(CFG(), 360) - 800,
                                  0,
                                  0,
                                  (_DWORD *)v4,
                                  &(*v37__f),
                                  &(*v36__f));
            if ( v3 != 0 )
            {
              g_eng.dsb_volume_5bb870(v30, (*v37__f));
              g_eng.dsb_pan_5bb880(v30, (*v36__f));
              g_eng.dsb_freq_5bb890(v30, 0);
              SETLB(v3, g_eng.dsb_play_5bb8c0(v30));
            }
          }
          F16(v4, 88) = 100;
          F32(v4, 92) = 0;
          F32(v4, 24) = -120000;
        }
        else
        {
          shot_46e190(c, v4, 216, F32(c, 0x1f8), 750, 0);
          if ( F16(v4, 88) == 100 )
          {
            v31 = F32(v4, 24) + 5000;
            if ( v31 > 160000 )
              v31 = 160000;
            F32(v4, 24) = v31;
            F32(v4, 8) += v31 / 100;
            if ( F32(CFG(), 148) != 0 )
            {
              (*v35__f) = 1600 * rnd() / 0x8000 - 800;
              v32 = rnd();
              W_particle_spawn(
                c,
                (int)c,
                100093,
                (_DWORD *)v4,
                1600 * v32 / 0x8000 - 800,
                (*v35__f),
                99,
                1,
                0,
                0,
                2,
                0,
                0,
                0);
            }
            v33 = F32(v4, 92) + 1;
            F32(v4, 92) = v33;
            if ( v33 < 20 )
            {
              F32(c, 0xf4) = SYS32(-4 * v33 + 10228);
              SETLB(v3, W_body_clip_step(v4));
              return (char)v3;
            }
            W_entity_release((int)c, /*edx*/ 0);
          }
          SETLB(v3, W_body_clip_step(v4));
        }
        return (char)v3;
      case 0x18742:
        if ( v3 == (intptr_t *)3 && F16(v4, 88) < 0x64u )
        {
          v21 = (intptr_t *)W_vpool_take(SYS32(1744), 0);
          if ( v21 != 0 )
          {
            v22 = F32(v21, 0);
            if ( W_sfx_place(
                   c,
                   F32(CFG(), 360) - 800,
                   0,
                   0,
                   (_DWORD *)v4,
                   &(*v37__f),
                   &(*v36__f)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v22, (*v37__f));
              g_eng.dsb_pan_5bb880(v22, (*v36__f));
              g_eng.dsb_freq_5bb890(v22, 0);
              g_eng.dsb_play_5bb8c0(v22);
            }
          }
          v23 = F32(v4, 108);
          v3 = (intptr_t *)D(0x6474c8);
          F16(v4, 88) = 100;
          F32(v4, 92) = 0;
          if ( v23 != D(0x6474c8) )
          {
            F32(v4, 108) = D(0x6474c8);
            F8(v4, 116) = 0;
            F16(v4, 114) = 0;
            F16(v4, 112) = 0;
          }
          if ( F32(CFG(), 148) != 0 )
            SETLB(v3, W_particle_spawn(c, (int)c, 100092, (_DWORD *)v4, 0, 0, 99, 1, 0, 0, 2, 0, 0, 0));
        }
        else
        {
          shot_46e190(c, v4, 218, F32(c, 0x1f8), 500, 1);
          SETLO(v24, F16(v4, 88));
          if ( (uint16_t)v24 == 100 )
          {
            v27 = F32(v4, 108);
            if ( v27 != 0 && ++F16(v4, 112) >= F16(v27, 68) )
            {
              v28 = ++F16(v4, 114);
              F16(v4, 112) = 0;
              if ( v28 >= F16(v27, 66) )
              {
                if ( F32(v27, 72) != 0 )
                {
                  F16(v4, 112) = 1;
                  v29 = F16(v27, 66) - 1;
                  F8(v4, 116) = 1;
                  F16(v4, 114) = v29;
                }
                else
                {
                  F16(v4, 114) = F16(v27, 338);
                }
              }
            }
            SETLB(v3, F8(v4, 116));
            if ( (uint8_t)v3 != 0 )
              F16(v4, 88) = v24 + 1;
          }
          else
          {
            SETLO(v3, F16(v4, 88) - 101);
            if ( F16(v4, 88) == 101 )
            {
              v25 = F32(v4, 92) + 1;
              F32(v4, 92) = v25;
              if ( v25 >= 20 )
              {
                W_entity_release((int)c, /*edx*/ 0);
              }
              else
              {
                v26 = (int *)((uint8_t *)g_eng.sys + (-4 * v25 + 10228));
                v3 = (intptr_t *)*v26;
                F32(c, 0xf4) = *v26;
              }
            }
          }
        }
        return (char)v3;
      case 0x18743:
        v5 = F16(v4, 88);
        switch ( v5 )
        {
          case 0:
          case 1:
            (*v36__f) = F32(c, 0x1f8);
            switch ( v5 )
            {
              case 0:
              case 1:
                v6 = 0;
                if ( F32(v4, 24) > 0 && shot_445ce0(c, (_DWORD *)v4, 0) )
                {
                  v7 = F32(v4, 24);
                  v6 = 1;
                  if ( v7 >= 10000 )
                  {
                    F32(v4, 24) = -500 * v7 / 1000;
                  }
                  else
                  {
                    F16(v4, 88) = 2;
                    F32(v4, 24) = 0;
                    F32(v4, 40) = 0;
                    F32(c, 0x204) = 0;
                  }
                }
                else
                {
                  v7 = (*v37__f);
                }
                v8 = F32(v4, 24) + 5000;
                if ( v8 > 160000 )
                  v8 = 160000;
                F32(v4, 24) = v8;
                F32(v4, 40) = F32(v4, 40) - 1000 < 0 ? 0 : F32(v4, 40) - 1000;
                if ( shot_46e110(c, v4, 0) == 0 )
                {
                  v7 = F32(v4, 40);
                  v6 = 1;
                  F32(v4, 44) = F32(v4, 44) != 3 ? 3 : 1;
                }
                if ( shot_46e150(c, v4, 0) != 0 || (v9 = F32(v4, 24)) >= 0 )
                {
                  if ( v6 == 0 )
                    goto LABEL_27;
                }
                else
                {
                  v7 = F32(v4, 24);
                  F32(v4, 24) = -v9;
                }
                v10 = (int)(1500 * abs32(v7)) / 100000;
                if ( v10 > 1500 )
                  v10 = 1500;
                W_sfx_play_at(c, 0, 218, v10 - 2000, (*v36__f), 0);
                break;
              case 2:
                v11 = F32(v4, 92) + 1;
                F32(v4, 92) = v11;
                if ( v11 > 500 || F32(c, 0x204) == 4 )
                {
                  F32(v4, 92) = 0;
                  F16(v4, 88) = v5 + 1;
                }
                break;
              case 3:
                v12 = F32(v4, 92) + 1;
                F32(v4, 92) = v12;
                v1 = SYS32(8 * abs32(v12 % 20 - 10) + 10152);
                F32(c, 0xf4) = v1;
                if ( F32(v4, 92) >= 120 )
                  goto LABEL_26;
                break;
              default:
                goto LABEL_27;
            }
            goto LABEL_27;
          case 2:
            if ( F32(v4, 108) != D(0x647660) )
            {
              F32(v4, 108) = D(0x647660);
              F8(v4, 116) = 0;
              F16(v4, 114) = 0;
              F16(v4, 112) = 0;
            }
            F16(v4, 88) = v5 + 1;
            v15 = (_DWORD *)F32(c, 0x40);
            v16 = (intptr_t *)W_vpool_take(SYS32(1744), 0);
            if ( v16 != 0 )
            {
              v17 = F32(v16, 0);
              if ( W_sfx_place(
                     c,
                     F32(CFG(), 360) - 800,
                     0,
                     0,
                     v15,
                     &(*v36__f),
                     &(*v37__f)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v17, (*v36__f));
                g_eng.dsb_pan_5bb880(v17, (*v37__f));
                g_eng.dsb_freq_5bb890(v17, 0);
                g_eng.dsb_play_5bb8c0(v17);
              }
            }
            goto LABEL_37;
          case 3:
LABEL_37:
            if ( F8(v4, 116) != 0 )
            {
              W_item_sparkle(c, F32(c, 0x208), 0);
              ++F16(v4, 88);
              F32(v4, 92) = 0;
            }
            goto LABEL_27;
          case 4:
            v18 = F32(v4, 92) + 1;
            v19 = F32(v4, 92) - 19 < 0;
            F32(v4, 92) = v18;
            if ( v19 == __OFSUB__(v18, 20) )
            {
LABEL_26:
              W_entity_release((int)c, /*edx*/ 0);
              goto LABEL_27;
            }
            v20 = 20 * (20 - v18) / 20;
            if ( v20 < 0 )
            {
              v20 = 0;
LABEL_42:
              F32(c, 0xf4) = SYS32(4 * v20 + 10152);
              goto LABEL_27;
            }
            if ( v20 < 20 )
              goto LABEL_42;
            F32(c, 0xf4) = 0;
LABEL_27:
            v3 = F32(v4, 108);
            if ( v3 != 0 && ++F16(v4, 112) >= F16(v3, 2 * 34) )
            {
              v13 = ++F16(v4, 114);
              F16(v4, 112) = 0;
              if ( v13 >= F16(v3, 2 * 33) )
              {
                if ( v3[18] != 0 )
                {
                  F16(v4, 112) = 1;
                  v14 = F16(v3, 2 * 33) - 1;
                  F8(v4, 116) = 1;
                  F16(v4, 114) = v14;
                }
                else
                {
                  F16(v4, 114) = F16(v3, 2 * 169);
                }
              }
            }
            break;
          default:
            goto LABEL_27;
        }
        break;
      default:
        return (char)v3;
    }
  }
  return (char)v3;
}

/* 0x445ce0: BOOL __thiscall sub_445CE0(cls_4022A0 *this, _DWORD *a1, unsigned __int16 a2) */
int32_t shot_445ce0(wl_char *self, int32_t * a1, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    int32_t * v3;
    int32_t ** v4;
    int32_t * v5;
    intptr_t result;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int32_t * *const v7__f = (int32_t * *)(frame_ + 0x0);   /* ebp-0xc */

  v3 = a1;
  v4 = &a1;
  v5 = a1 + 161;
  (*v7__f) = a1 + 161;
  SETLO(v4, *((uint8_t *)a1 + 32));
  a1 = (_DWORD *)a1[2];
  result = W_entity_move_vertical(c, v3, 1, (int)v4, a2, (int)&a1, (*v7__f)) == 0;
  if ( *v5 != 0 )
    v3[2] = a1;
  return result;
}

/* 0x449700: int __thiscall sub_449700(cls_4022A0 *this, int a2, int a3, unsigned __int16 a4, int a5, int a6, int a7, int a8) */
int32_t shot_449700(wl_char *self, int32_t a2, int32_t a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t i;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */

  result = a3;
  if ( a3 == 0 && a4 != 0 )
  {
    for ( i = a4; i != 0; --i )
    {
      (*v12__f) = a6 + a8 * rnd() / 0x8000 - a8 / 2;
      (*v11__f) = a5 + a7 * rnd() / 0x8000 - a7 / 2;
      result = W_particle_spawn(c, (int)c, 100049, (_DWORD *)a2, (*v11__f), (*v12__f), 99, 1, 0, 0, 2, 0, 0, 0);
    }
  }
  return result;
}

/* 0x4497b0: int __thiscall sub_4497B0(cls_4022A0 *this, int a2, int a3, int a4) */
int32_t shot_4497b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t v5;

  result = a2;
  if ( a2 == 0 )
  {
    if ( a4 <= 0 || (v5 = 20 * a3 / a4) < 0 )
    {
      v5 = 0;
    }
    else if ( v5 >= 20 )
    {
      F32(c, 0xf4) = 0;
      F32(c, 0xf8) = 0;
      return 0;
    }
    result = SYS32(4 * v5 + 10152);
    F32(c, 0xf4) = result;
    F32(c, 0xf8) = result;
  }
  return result;
}

/* 0x4504c0: int __thiscall sub_4504C0( cls_4022A0 *this, int a1, int a2, int a3, _DWORD *a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14) */
int32_t shot_4504c0(wl_char *self, int32_t a1, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14)
{
    intptr_t c = (intptr_t)self;
    int32_t * v16;
    intptr_t v17;
    intptr_t v18;

  if ( a9 != 0 )
    return 0;
  v16 = *(_DWORD **)&GAP(W, 0x1030, 28);
  v17 = a4[1] - v16[24] - v16[13];
  v18 = a4[2] - 100 * v16[29] - v16[23] - v16[19];
  if ( a1 + v17 + a4[3] >= 0 && a1 + v18 + a4[4] >= 0 && v16[25] > v17 - a1 && v16[26] > v18 - a1 )
    return W_particle_spawn(c, a2, a3, a4, a5, a6, a7, a8, 0, a10, a11, a12, a13, a14);
  else
    return 0;
}

/* 0x4538f0: BOOL __stdcall sub_4538F0(_DWORD *a1, _DWORD *a2, int a3) */
int32_t shot_4538f0(int32_t * a1, int32_t * a2, int32_t a3)
{

    intptr_t v3;
    intptr_t v4;

  v3 = a2[1] - a1[24] - a1[13];
  v4 = a2[2] - 100 * a1[29] - a1[23] - a1[19];
  return a3 + v3 + a2[3] >= 0 && a3 + v4 + a2[4] >= 0 && a1[25] > v3 - a3 && a1[26] > v4 - a3;
}

/* 0x46e110: int __thiscall sub_46E110(cls_4022A0 *this, _DWORD *a1, unsigned __int16 a2) */
int32_t shot_46e110(wl_char *self, int32_t * a1, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    uint32_t v4;

  v3 = 1374389535;
  if ( a1[11] != 1 )
    v3 = -1374389535;
  v4 = (int)((uint64_t)((int)a1[10] * (int64_t)v3) >> 32) >> 5;
  return W_entity_move_horizontal(c, a1, (v4 >> 31) + v4, 1, 1, 0, a2, 0);
}

/* 0x46e150: int __thiscall sub_46E150(cls_4022A0 *this, _DWORD *a1, unsigned __int16 a2) */
int32_t shot_46e150(wl_char *self, int32_t * a1, uint16_t a2)
{
    intptr_t c = (intptr_t)self;

  return W_entity_move_vertical(c, a1, a1[6] / 100, 0, a2, (int)(a1 + 2), 0);
}

/* 0x46e190: int __thiscall sub_46E190(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t shot_46e190(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6;
    uint16_t v8;
    intptr_t result;
    intptr_t v10;
    intptr_t v11;
    int16_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    uint16_t * v21;
    uint16_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    uint16_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;

  v6 = a2;
  v8 = F16(a2, 88);
  result = v8;
  v10 = a6;
  switch ( v8 )
  {
    case 0u:
      if ( F32(a2, 24) > 0 && a6 == 0 )
      {
        F16(a2, 88) = v8 + 1;
        F32(c, 0x204) = 0;
      }
      goto LABEL_5;
    case 1u:
LABEL_5:
      v11 = F32(v6, 24);
      a6 = 0;
      if ( v11 <= 0 )
        goto LABEL_13;
      v12 = F8(v6, 32);
      a2 = F32(v6, 8);
      v13 = W_entity_move_vertical(c, (_DWORD *)v6, 1, v12, 0, (int)&a2, (_DWORD *)(v6 + 644)) == 0;
      if ( F32(v6, 644) != 0 )
        F32(v6, 8) = a2;
      if ( v13 )
      {
        v14 = F32(v6, 24);
        a6 = 1;
        if ( v14 >= 10000 )
        {
          F32(v6, 24) = a5 * v14 / -1000;
        }
        else
        {
          F16(v6, 88) = 2;
          F32(v6, 24) = 0;
          F32(v6, 40) = 0;
          if ( v10 != 0 )
            F32(c, 0x204) = 0;
        }
      }
      else
      {
LABEL_13:
        v14 = a2;
      }
      v15 = F32(v6, 24) + 5000;
      if ( v15 > 160000 )
        v15 = 160000;
      F32(v6, 24) = v15;
      v16 = (F32(v6, 40) - 1000) & ((F32(v6, 40) - 1000 < 0) - 1);
      v17 = F32(v6, 44);
      F32(v6, 40) = v16;
      if ( v17 == 1 )
        v18 = v16 / 100;
      else
        v18 = v16 / -100;
      if ( W_entity_move_horizontal(c, (_DWORD *)v6, v18, 1, 1, 0, 0, 0) != 0 )
      {
        v19 = a6;
      }
      else
      {
        v14 = F32(v6, 40);
        v19 = 1;
        F32(v6, 44) = F32(v6, 44) != 3 ? 3 : 1;
      }
      result = W_entity_move_vertical(c, (_DWORD *)v6, F32(v6, 24) / 100, 0, 0, v6 + 8, 0);
      if ( result != 0 || (result = F32(v6, 24)) >= 0 )
      {
        if ( v19 == 0 )
          return result;
      }
      else
      {
        v14 = F32(v6, 24);
        F32(v6, 24) = -result;
      }
      a2 = 1500;
      v20 = (int)(1500 * abs32(v14)) / 100000;
      if ( v20 <= 1500 )
        a2 = v20;
      a6 = F32(c, 0x40);
      v21 = SYS32(4 * (uint16_t)a3 + 868);
      result = F32(v21, 4 * 2);
      if ( result == 0 )
        return result;
      result = F32(v21, 4 * 1);
      if ( result == 0 )
      {
        result = W_vpool_load(SYS32(4 * (uint16_t)a3 + 868));
        if ( result == 0 )
          return result;
      }
      v22 = 0;
      if ( *v21 == 0 )
        return result;
      v23 = F32(v21, 4 * 1);
      SETHI(v24, 0);
      break;
    case 2u:
      result = F32(a2, 92) + 1;
      F32(a2, 92) = result;
      if ( result > 500 || F32(c, 0x204) == 4 )
      {
        F32(v6, 92) = 0;
        F16(v6, 88) = v8 + 1;
      }
      return result;
    case 3u:
      v29 = F32(a2, 92) + 1;
      F32(a2, 92) = v29;
      v30 = SYS32(8 * abs32(v29 % 20 - 10) + 10152);
      F32(c, 0xf4) = v30;
      result = F32(v6, 92);
      if ( result >= 120 )
        W_entity_release((int)c, /*edx*/ 0);
      return result;
    default:
      return result;
  }
  while ( 1 )
  {
    SETLO(v24, v21[1]);
    v25 = (v24 + v22) % *v21;
    result = (uint16_t)v25;
    if ( F32(v23 + 8 * (uint16_t)v25, 4) == 0 )
      break;
    if ( ++v22 >= *v21 )
      return result;
  }
  v26 = v25 + 1;
  v27 = 8 * (uint16_t)v25;
  v21[1] = v26;
  F32(v27 + v23, 4) = 0;
  result = v27 + F32(v21, 4 * 1);
  if ( result != 0 )
  {
    v28 = F32(result, 0);
    result = W_sfx_place(
               c,
               a2 + F32(CFG(), 360) - 2000,
               0,
               0,
               (_DWORD *)a6,
               &a5,
               &a3);
    if ( result != 0 )
    {
      g_eng.dsb_volume_5bb870(v28, a5);
      g_eng.dsb_pan_5bb880(v28, a3);
      g_eng.dsb_freq_5bb890(v28, a4);
      return g_eng.dsb_play_5bb8c0(v28);
    }
  }
  return result;
}

/* 0x476500: int __stdcall sub_476500(int a1, int a2, int a3) */
int32_t shot_476500(int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t v3;
    intptr_t v4;

  v3 = a1 - a2;
  if ( a1 - a2 >= 0 )
    v4 = v3 / (360 * a3);
  else
    v4 = v3 / (360 * a3) - 1;
  return v3 - 360 * a3 * v4;
}

/* 0x476550: int __stdcall sub_476550(int a1, int a2, _DWORD *a3, long double *a4) */
int32_t shot_476550(int32_t a1, int32_t a2, int32_t * a3, double * a4)
{

    double v4;
    double v5;
    double v6;
    double v7;
    int64_t v8;
    double v9;

  if ( a1 != 0 || a2 != 0 )
  {
    if ( (int)abs32(a2) >= (int)abs32(a1) )
    {
      if ( a2 >= 0 )
      {
        v6 = atan2((double)a1 / (double)a2, 1.0);
        if ( a1 >= 0 )
          v4 = (*(const double *)D(0x8a6b48)) - v6;
        else
          v4 = (*(const double *)D(0x8a6b40)) - v6;
      }
      else
      {
        v4 = atan2((double)a1 / (double)a2, 1.0) * -1.0;
      }
    }
    else
    {
      v5 = atan2((double)a2 / (double)a1, 1.0);
      if ( a1 >= 0 )
        v4 = v5 + (*(const double *)D(0x8a6b28));
      else
        v4 = v5 + (*(const double *)D(0x8a6618));
    }
  }
  else
  {
    v4 = 0.0;
  }
  *a4 = v4;
  if ( (v4 < (*(const double *)D(0x8a6b28)) || v4 >= (*(const double *)D(0x8a6b48))) && (v4 < (*(const double *)D(0x8a6618)) || v4 >= 0.0) )
  {
    v9 = (double)a2 / cos(v4) * -1.0;
    if ( v9 >= 0.0 )
      v8 = (int64_t)(v9 + 0.5);
    else
      v8 = (int64_t)(v9 - 0.5);
    *a3 = v8;
  }
  else
  {
    v7 = (double)a1 / sin(v4);
    if ( v7 >= 0.0 )
      v8 = (int64_t)(v7 + 0.5);
    else
      v8 = (int64_t)(v7 - 0.5);
    *a3 = v8;
  }
  return v8;
}

/* 0x476740: __int16 __thiscall sub_476740(cls_4022A0 *this, int a2) */
int32_t shot_476740(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2;
    uint16_t i;
    intptr_t v4;
    intptr_t v5;
    intptr_t * v6;
    int32_t * v7;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */
    intptr_t *const SystemTime__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x10 */

  v2 = F32(c, 0x1b0);
  (*v9__f) = c;
  if ( v2 != 0 )
  {
    SETLO(v2, F16(c, 0x1b4));
    for ( i = 0; i < (uint16_t)v2; SETLO(v2, F16((*v9__f), 0x1b4) ))
    {
      v4 = F32(c, 0x1b0);
      if ( v4 != 0 )
      {
        if ( (uint16_t)v2 > i )
        {
          v6 = F32(v4, 36 * i);
          v7 = (_DWORD *)(v4 + 36 * i);
          if ( v6 != 0 )
          {
            if ( a2 != 0 )
              g_eng.dsb_stop_5bb8e0(F32(v6, 0));
            F32(*v7, 4) = 0;
            *v7 = 0;
            v7[1] = 0;
            v7[8] = 0;
          }
        }
        else
        {
          g_eng.log_fail_406440(g_eng.log, (const char *)(D(0x85525c)), (const char *)(D(0x8a9b6c)));
        }
      }
      c = (*v9__f);
      ++i;
    }
  }
  return v2;
}

/* 0x476a80: unsigned int __stdcall sub_476A80(_DWORD *a1) */
int32_t shot_476a80(int32_t * a1)
{

    intptr_t v1;
    uint32_t v2;
    uint32_t result;

  v1 = 1374389535;
  if ( a1[11] != 1 )
    v1 = -1374389535;
  v2 = (int)((uint64_t)((int)a1[10] * (int64_t)v1) >> 32) >> 5;
  result = (v2 >> 31) + v2 + a1[1];
  a1[1] = result;
  return result;
}

/* 0x476ab0: int __thiscall sub_476AB0(cls_4022A0 *this, _DWORD *a1, int a2, unsigned __int16 a3, int a4) */
int32_t shot_476ab0(wl_char *self, int32_t * a1, int32_t a2, uint16_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    double v6;
    double v7;
    double v8;
    double v9;
    intptr_t v10;
    double v11;
    double v12;
    intptr_t v13;
    intptr_t result;

  v6 = (double)(int)a1[13] * (*(const double *)D(0x8a9a38)) * 0.001;
  v7 = (double)(a1[10] / 100);
  v8 = sin(v6) * v7;
  if ( v8 >= 0.0 )
    v9 = v8 + 0.5;
  else
    v9 = v8 - 0.5;
  v10 = (int64_t)v9;
  v11 = cos(v6) * v7;
  if ( v11 >= 0.0 )
    v12 = v11 + 0.5;
  else
    v12 = v11 - 0.5;
  v13 = -(int)(int64_t)v12;
  if ( v10 != 0 )
  {
    result = W_entity_move_horizontal(c, a1, v10, a4, a4, 0, a3, 0);
    if ( result == 0 )
      return result;
    v13 = -(int)(int64_t)v12;
  }
  if ( v13 == 0 )
    return 1;
  result = W_entity_move_vertical(c, a1, v13, 0, a3, (int)(a1 + 2), 0);
  if ( result != 0 )
    return 1;
  return result;
}

/* 0x476ba0: __int64 __stdcall sub_476BA0(_DWORD *a1, int a2) */
int64_t shot_476ba0(int32_t * a1, int32_t a2)
{

    intptr_t v3;
    double v4;
    intptr_t v5;
    double v6;
    double v7;
    double v8;
    double v9;
    int64_t result;
    intptr_t v11;

  v3 = a1[13];
  v11 = v3;
  if ( a2 != 0 && a1[11] == 3 )
    v11 = 360000 - v3;
  v4 = (double)v11 * (*(const double *)D(0x8a9a38)) * 0.001;
  v5 = a1[2];
  v6 = (double)(a1[10] / 100);
  v7 = sin(v4) * v6;
  if ( v7 >= 0.0 )
    v8 = v7 + 0.5;
  else
    v8 = v7 - 0.5;
  a1[1] += (int64_t)v8;
  v9 = cos(v4) * v6;
  if ( v9 >= 0.0 )
    result = (int64_t)(v9 + 0.5);
  else
    result = (int64_t)(v9 - 0.5);
  a1[2] = v5 - result;
  return result;
}

/* 0x476d30: int __thiscall sub_476D30(cls_476D30 *this, int a2, unsigned __int16 a3, unsigned __int16 a4, int a5) */
int32_t shot_476d30(wl_char *self, int32_t a2, uint16_t a3, uint16_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    uint16_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v11;
    intptr_t v12;

  v5 = F16(F32(c, 0x40), 60);
  v6 = a4 - a3;
  if ( v5 <= a4 )
  {
    if ( a3 <= v5 )
      v7 = a4 - v5;
    else
      v7 = a4 - a3;
  }
  else
  {
    v7 = 0;
  }
  v8 = a5 * v7 / 1000;
  if ( a2 == 1 )
  {
    v12 = 20 * v8 / v6;
    if ( v12 >= 0 )
    {
      if ( v12 >= 20 )
        return SYS32(10148);
    }
    else
    {
      v12 = 0;
    }
    return SYS32(4 * v12 + 10072);
  }
  if ( a2 == 2 )
  {
    v11 = 20 * v8 / v6;
    if ( v11 < 20 )
      return SYS32(4 * (v11 < 0 ? 0 : v11) + 10252);
    else
      return SYS32(10328);
  }
  else
  {
    if ( v6 <= 0 || (v9 = 20 * v8 / v6) < 0 )
    {
      v9 = 0;
    }
    else if ( v9 >= 20 )
    {
      return 0;
    }
    return SYS32(4 * v9 + 10152);
  }
}

/* 0x476e20: int __thiscall sub_476E20(cls_4022A0 *this, int a2, int a3) */
int32_t shot_476e20(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    uint16_t v5;
    uint16_t i;
    intptr_t v7;
    int32_t * v8;

  if ( ++F16(a2, 60) <= F16(c, 0x1fc) )
  {
    return 0;
  }
  else if ( a3 != 0 )
  {
    F8(a2, 0) = 0;
    return 1;
  }
  else
  {
    if ( F32(c, 0x254) != 0 && F32(c, 0x1b0) != 0 )
    {
      v5 = F16(c, 0x1b4);
      for ( i = 0; i < v5; ++i )
      {
        v7 = F32(c, 0x1b0);
        if ( v7 != 0 )
        {
          if ( v5 > i )
          {
            v8 = (_DWORD *)(v7 + 36 * i);
            if ( *v8 != 0 )
            {
              F32(*v8, 4) = 0;
              *v8 = 0;
              v8[1] = 0;
              v8[8] = 0;
            }
          }
          else
          {
            g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
          }
        }
        v5 = F16(c, 0x1b4);
      }
    }
    W_entity_release((int)c, /*edx*/ 0);
    return 1;
  }
}

/* 0x476ef0: int __thiscall sub_476EF0( cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13) */
int32_t shot_476ef0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13)
{
    intptr_t c = (intptr_t)self;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    uint16_t v17;
    uint16_t i;
    intptr_t v19;
    int32_t * v20;

  if ( W_body_hit_spawn(
         c,
         F32(c, 0x20c),
         a2,
         a10,
         a11,
         a12 + F32(a2, 12),
         a13 + F32(a2, 16),
         a9,
         a3,
         a4,
         F32(c, 0x234),
         F32(c, 0x238),
         F32(c, 0x23c),
         a5,
         F32(c, 0x200),
         a6,
         a7,
         F32(c, 0x220),
         a8,
         F32(c, 0x244),
         F32(c, 0x224),
         0,
         0,
         0) == 0 )
    return 0;
  v15 = F32(c, 0x22c) + 1;
  if ( v15 > 100000 )
    v15 = 100000;
  v16 = F32(c, 0x228);
  F32(c, 0x22c) = v15;
  if ( v16 == 0 || v15 < v16 )
    return 1;
  if ( a3 != 0 )
  {
    F8(a2, 0) = 0;
    return 1;
  }
  if ( F32(c, 0x254) != 0 && F32(c, 0x1b0) != 0 )
  {
    v17 = F16(c, 0x1b4);
    for ( i = 0; i < v17; ++i )
    {
      v19 = F32(c, 0x1b0);
      if ( v19 != 0 )
      {
        if ( v17 > i )
        {
          v20 = (_DWORD *)(v19 + 36 * i);
          if ( *v20 != 0 )
          {
            F32(*v20, 4) = 0;
            *v20 = 0;
            v20[1] = 0;
            v20[8] = 0;
          }
        }
        else
        {
          g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
        }
      }
      v17 = F16(c, 0x1b4);
    }
  }
  W_entity_release((int)c, /*edx*/ 0);
  return 1;
}

/* 0x477070: int __thiscall sub_477070(cls_477070 *this, int a2, int a3) */
int32_t shot_477070(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    double v4;
    intptr_t v5;
    double v6;
    double v7;
    double v8;
    double v9;
    double v10;
    int64_t v11;
    uint16_t v12;
    uint16_t i;
    intptr_t v14;

  v4 = (double)F32(a2, 52) * (*(const double *)D(0x8a9a38)) * 0.001;
  v5 = F32(a2, 8);
  v6 = (double)(F32(a2, 40) / 100);
  v7 = sin(v4) * v6;
  if ( v7 >= 0.0 )
    v8 = v7 + 0.5;
  else
    v8 = v7 - 0.5;
  F32(a2, 4) += (int64_t)v8;
  v9 = cos(v4) * v6;
  if ( v9 >= 0.0 )
    v10 = v9 + 0.5;
  else
    v10 = v9 - 0.5;
  v11 = (int64_t)v10;
  SETLO(v11, ++F16(a2, 60));
  F32(a2, 8) = v5 - (int64_t)v10;
  if ( (uint16_t)v11 <= F16(c, 0x1fc) )
    return 0;
  if ( a3 != 0 )
  {
    F8(a2, 0) = 0;
    return 1;
  }
  else
  {
    if ( F32(c, 0x254) != 0 && F32(c, 0x1b0) != 0 )
    {
      v12 = F16(c, 0x1b4);
      for ( i = 0; i < v12; ++i )
      {
        v14 = F32(c, 0x1b0);
        if ( v14 != 0 )
        {
          if ( v12 > i )
          {
            SETHIDW(v11, 9 * i);
            SETLODW(v11, v14 + 36 * i);
            if ( F32(v11, 0) != 0 )
            {
              F32(F32(v11, 0), 4) = 0;
              F32(v11, 0) = 0;
              F32(v11, 4) = 0;
              F32(v11, 32) = 0;
            }
          }
          else
          {
            g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
          }
        }
        v12 = F16(c, 0x1b4);
      }
    }
    W_entity_release((int)c, /*edx*/ 0);
    return 1;
  }
}

/* 0x4771d0: int __thiscall sub_4771D0( cls_4771D0 *this, int a2, int a3, int a4, unsigned __int16 a5, _DWORD *a6, _DWORD *a7, int a8, int a9, int a10) */
int32_t shot_4771d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, uint16_t a5, int32_t * a6, int32_t * a7, int32_t a8, int32_t a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10;
    uint16_t v11;
    uint16_t v12;
    double v13;
    intptr_t v14;
    double v15;
    intptr_t v16;
    intptr_t v17;
    double v18;
    double v19;
    double v20;
    double v21;

  if ( a10 == 3 )
    a4 = 360000 - a4;
  v10 = F32(c, 0x20c);
  v11 = a5;
  v12 = a5;
  if ( a5 >= F16(v10, 68) )
  {
    v12 = 0;
    v11 = 0;
  }
  v13 = (double)a4 * (*(const double *)D(0x8a9a38)) * 0.001;
  v14 = F32(v10, 64);
  v15 = (double)a3;
  v16 = F32(v14 + 660 * v12, 20);
  v17 = v16 + F32(c, 0x214) + F32(v14 + 660 * v12, 8) + (F32(v14 + 660 * v12, 16) - v16) / 2;
  v18 = sin(v13) * v15;
  if ( v18 >= 0.0 )
    v19 = v18 + 0.5;
  else
    v19 = v18 - 0.5;
  v20 = cos(v13) * v15;
  if ( v20 >= 0.0 )
    v21 = v20 + 0.5;
  else
    v21 = v20 - 0.5;
  *a6 = (int64_t)v19
      + F32(c, 0x210)
      + F32(v14 + 660 * v11, 4)
      + F32(v14 + 660 * v11, 12) / 2
      - a8 / 2;
  *a7 = v17 - a9 / 2 - (int64_t)v21;
  return a9 / 2;
}

/* 0x477310: _DWORD *__thiscall sub_477310(cls_4022A0 *this, int a2, _DWORD *a3, _DWORD *a4, int a5, int a6, int a7, int a8) */
int32_t shot_477310(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    int32_t * result;
    intptr_t v11;
    intptr_t v12;
    double v13;
    double v14;
    double v15;
    double v16;
    double v17;
    double v18;
    int32_t * v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    int32_t * v27;
    int32_t * v28;
    intptr_t v29;

  result = a4;
  v11 = a3[13] / 1000;
  if ( a4 == 0 )
  {
    v27 = (_DWORD *)a3[13];
    v12 = rnd();
    v13 = (double)(int)v27 * (*(const double *)D(0x8a9a38)) * 0.001;
    v14 = (double)(400 - 800 * v12 / 0x8000);
    v15 = sin(v13) * v14;
    if ( v15 >= 0.0 )
      v16 = v15 + 0.5;
    else
      v16 = v15 - 0.5;
    v28 = (_DWORD *)(int64_t)v16;
    v17 = cos(v13) * v14;
    if ( v17 >= 0.0 )
      v18 = v17 + 0.5;
    else
      v18 = v17 - 0.5;
    v29 = -(int)(int64_t)v18;
    v19 = *(_DWORD **)&GAP(W, 0x1030, 28);
    v20 = a3[1] - v19[24] - v19[13];
    v21 = a3[2] - 100 * v19[29] - v19[23] - v19[19];
    if ( v20 + a3[3] + 16000 >= 0 && v21 + a3[4] + 16000 >= 0 && v19[25] > v20 - 16000 && v19[26] > v21 - 16000 )
    {
      v22 = W_particle_spawn(c, (int)c, a2, a3, (int)v28, v29, 99, 1, 0, 0, 2, 0, v11 + a5, a7);
      if ( v22 != 0 )
      {
        v23 = F32(v22, 64);
        rnd();
        F32(v23, 24) = a8;
        rnd();
        F32(v23, 40) = a6;
      }
    }
    result = (_DWORD *)a5;
    if ( a5 > 0 )
    {
      result = *(_DWORD **)&GAP(W, 0x1030, 28);
      v24 = a3[1] - result[24] - result[13];
      v25 = a3[2] - 100 * result[29] - result[23] - result[19];
      if ( v24 + a3[3] + 16000 >= 0 && v25 + a3[4] + 16000 >= 0 && result[25] > v24 - 16000 && result[26] > v25 - 16000 )
      {
        result = (_DWORD *)W_particle_spawn(
                             c,
                             (int)c,
                             a2,
                             a3,
                             (int)v28,
                             v29,
                             99,
                             1,
                             0,
                             0,
                             2,
                             0,
                             v11 - a5,
                             a7);
        if ( result != 0 )
        {
          v26 = result[16];
          rnd();
          F32(v26, 24) = a8;
          result = (_DWORD *)rnd();
          F32(v26, 40) = a6;
        }
      }
    }
  }
  return result;
}

/* 0x477560: int __thiscall sub_477560(cls_4022A0 *this, _DWORD *a1, unsigned __int16 a2, int a3, int a4, int a5, int a6, int a7) */
int32_t shot_477560(wl_char *self, int32_t * a1, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8;
    intptr_t v9;
    int32_t * v10;
    intptr_t v12;
    intptr_t result;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v19__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x1c */

  v8 = (int)a1;
  v10 = a1 + 2;
  a1 = (_DWORD *)a1[2];
  SETHI(v9, HIWORD(a1));
  SETLO(v9, F8(v8, 32));
  v12 = W_entity_move_vertical(c, (_DWORD *)v8, 1, v9, a2, (int)&a1, (_DWORD *)(v8 + 644)) == 0;
  if ( F32(v8, 644) != 0 )
    *v10 = a1;
  if ( v12 )
  {
    result = a7;
    if ( a7 != 0 )
    {
      v14 = a3;
      v15 = F32(v8, 24) + a4;
      v16 = a3;
      if ( a3 <= v15 )
        v16 = F32(v8, 24) + a4;
      if ( v16 <= 0 )
      {
        if ( a3 <= v15 )
          v14 = F32(v8, 24) + a4;
      }
      else
      {
        v14 = 0;
      }
      F32(v8, 24) = v14;
      result = v14 / 100 + *v10;
      *v10 = result;
    }
    else
    {
      F32(v8, 24) = 0;
    }
  }
  else
  {
    v17 = F32(v8, 24);
    v18 = a5;
    if ( a5 >= v17 + a6 )
      v18 = v17 + a6;
    (*v19__f) = a2;
    F32(v8, 24) = v18;
    return W_entity_move_vertical(c, (_DWORD *)v8, v18 / 100, 0, (*v19__f), (int)v10, 0);
  }
  return result;
}

/* 0x477650: int __thiscall sub_477650(cls_4022A0 *this, int a2, int a3, int a4) */
int32_t shot_477650(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    int32_t * v22;
    int32_t * v23;
    int32_t * v24;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  (*v21__f) = 0;
  switch ( F16(a2, 88) )
  {
    case 0:
      (*v21__f) = 1;
      ++F16(a2, 88);
      goto LABEL_3;
    case 1:
LABEL_3:
      v6 = F32(a2, 44);
      ++F32(a2, 92);
      if ( a3 == 0
        && W_view_box_visible(
             *(intptr_t *)&GAP(W, 0x1030, 28),
             F32(a2, 4),
             F32(a2, 8),
             F32(a2, 12),
             F32(a2, 16),
             16000) )
      {
        W_particle_spawn(c, (int)c, 100051, (_DWORD *)a2, 0, 0, v6, 1, 0, 0, 2, 0, 0, 10);
      }
      if ( F32(a2, 92) > 5 )
      {
        ++F16(a2, 88);
        F32(a2, 92) = 0;
        (*v21__f) = 1;
      }
      break;
    case 2:
      v22 = F32(a2, 44);
      v7 = 2000 * rnd() / 0x8000 - 1000;
      v8 = 2000 * rnd() / 0x8000 - 1000;
      if ( a3 == 0
        && W_view_box_visible(
             *(intptr_t *)&GAP(W, 0x1030, 28),
             F32(a2, 4),
             F32(a2, 8),
             F32(a2, 12),
             F32(a2, 16),
             16000) )
      {
        W_particle_spawn(c, (int)c, 100051, (_DWORD *)a2, v8, v7, (int)v22, 1, 0, 0, 2, 0, 0, 10);
      }
      v23 = F32(a2, 44);
      v9 = 2000 * rnd() / 0x8000 - 1000;
      v10 = 2000 * rnd() / 0x8000 - 1000;
      if ( a3 == 0
        && W_view_box_visible(
             *(intptr_t *)&GAP(W, 0x1030, 28),
             F32(a2, 4),
             F32(a2, 8),
             F32(a2, 12),
             F32(a2, 16),
             16000) )
      {
        W_particle_spawn(c, (int)c, 100051, (_DWORD *)a2, v10, v9, (int)v23, 1, 0, 0, 2, 0, 0, 0);
      }
      if ( F32(CFG(), 148) != 0 )
      {
        v24 = F32(a2, 44);
        v11 = 2000 * rnd() / 0x8000 - 1000;
        v12 = 2000 * rnd() / 0x8000 - 1000;
        if ( a3 == 0
          && W_view_box_visible(
               *(intptr_t *)&GAP(W, 0x1030, 28),
               F32(a2, 4),
               F32(a2, 8),
               F32(a2, 12),
               F32(a2, 16),
               16000) )
        {
          W_particle_spawn(c, (int)c, 100051, (_DWORD *)a2, v12, v11, (int)v24, 1, 0, 0, 2, 0, 0, 0);
        }
      }
      v13 = F32(a2, 92) + 1;
      F32(a2, 92) = v13;
      if ( v13 > F32(c, 0x204) )
      {
        ++F16(a2, 88);
        F32(a2, 92) = 0;
        (*v21__f) = 1;
      }
      break;
    case 3:
      F32(a2, 40) = F32(a2, 40) - 4000 < 0 ? 0 : F32(a2, 40) - 4000;
      v14 = F32(a2, 24) + 4000;
      if ( v14 > 80000 )
        v14 = 80000;
      ++F16(a2, 60);
      F32(a2, 24) = v14;
      v15 = 1000 * rnd() / 0x8000 - 500;
      if ( a3 == 0 )
      {
        v16 = F32(c, 0x1b0);
        if ( v16 != 0 )
        {
          if ( F16(c, 0x1b4) != 0 )
          {
            if ( F32(v16, 32) == 0 )
              W_entity_snd_start(c, 0, 0, 0xE3u, 0, v15, 0, 0);
            W_entity_snd_keep(c, 0, 0);
            v17 = F32(v16, 32) + 1;
            F32(v16, 32) = v17;
            if ( v17 >= 40 )
              F32(v16, 32) = 0;
          }
          else
          {
            g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
          }
        }
      }
      if ( F32(CFG(), 148) != 0 || (F16(a2, 60) & 1) == 0 )
      {
        v18 = 3200 * rnd() / 0x8000 - 1600;
        v19 = 3200 * rnd() / 0x8000 - 1600;
        if ( a3 == 0
          && W_view_box_visible(
               *(intptr_t *)&GAP(W, 0x1030, 28),
               F32(a2, 4),
               F32(a2, 8),
               F32(a2, 12),
               F32(a2, 16),
               16000) )
        {
          W_particle_spawn(c, (int)c, 100052, (_DWORD *)a2, v19, v18, 99, 4, 0, 0, 2, 0, 0, 0);
        }
      }
      if ( W_entity_move_vertical(c, (_DWORD *)a2, F32(a2, 24) / 100, 0, a4, a2 + 8, 0) == 0 )
        F32(a2, 40) = 0;
      break;
    default:
      break;
  }
  if ( F16(a2, 88) >= 2u )
    shot_476ef0(c, a2, a3, 1, 8, 1, 1, 1, 7, 0, 0, 0, 0);
  W_polar_div(
    F32(a2, 40) / 100,
    F32(a2, 52),
    1000,
    F32(a2, 4),
    F32(a2, 8),
    (_DWORD *)(a2 + 4),
    (_DWORD *)(a2 + 8));
  if ( ++F16(a2, 60) > F16(c, 0x1fc) )
    W_shot_end(c, (_BYTE *)a2, a3);
  return (*v21__f);
}

/* 0x477b70: int __thiscall sub_477B70(cls_4022A0 *this, int a2, int a3) */
int32_t shot_477b70(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    int16_t v5;
    intptr_t v6;
    int32_t * v7;
    intptr_t v8;
    intptr_t * v9;
    void * v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    void * v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t result;
    double v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    double v24;
    intptr_t v25;
    intptr_t v26;
    _Alignas(8) uint8_t frame_[0x44 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v31 = (intptr_t *)(frame_ + 0x3c);   /* [2] at ebp-0x8 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x44 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x38 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    double *const v30__f = (double *)(frame_ + 0x34);   /* ebp-0x10 */

  v3 = a2;
  v5 = F16(a2, 88);
  switch ( v5 )
  {
    case 0:
      F16(a2, 88) = v5 + 1;
      v6 = rnd();
      v7 = (_DWORD *)F32(c, 0x40);
      v8 = 1000 * v6 / 0x8000 - 2000;
      if ( a3 == 0 )
      {
        v9 = (intptr_t *)W_vpool_take(SYS32(1292), 0);
        if ( v9 != 0 )
        {
          (*v29__f) = F32(v9, 0);
          if ( W_sfx_place(c, F32(CFG(), 360) + 400, 0, 0, v7, &a2, v31) != 0 )
          {
            g_eng.dsb_volume_5bb870((*v29__f), a2);
            g_eng.dsb_pan_5bb880((*v29__f), v31[0]);
            g_eng.dsb_freq_5bb890((*v29__f), v8);
            g_eng.dsb_play_5bb8c0((*v29__f));
          }
        }
      }
      v10 = D(0x646cd0);
      goto LABEL_7;
    case 1:
      v13 = F32(a2, 92) + 1;
      F32(a2, 92) = v13;
      if ( v13 > 18 * F32(c, 0x204) / 1000 )
      {
        F32(v3, 92) = 0;
        F16(v3, 88) = v5 + 1;
        v10 = D(0x646e68);
LABEL_7:
        if ( F32(v3, 108) != v10 )
        {
          F32(v3, 108) = v10;
          F8(v3, 116) = 0;
          F16(v3, 114) = 0;
          F16(v3, 112) = 0;
        }
      }
      goto LABEL_9;
    case 2:
      v14 = F32(a2, 92) + 1;
      F32(a2, 92) = v14;
      if ( v14 > 18 * F32(c, 0x204) / 1000 )
      {
        v15 = F32(v3, 108);
        F32(v3, 92) = 0;
        F16(v3, 88) = v5 + 1;
        if ( v15 != D(0x647000) )
        {
          F32(v3, 108) = D(0x647000);
          F8(v3, 116) = 0;
          F16(v3, 114) = 0;
          F16(v3, 112) = 0;
        }
      }
      if ( W_body_hit_spawn(
             c,
             F32(c, 0x20c),
             v3,
             0,
             0,
             F32(v3, 12),
             F32(v3, 16),
             7,
             a3,
             0,
             F32(c, 0x234),
             F32(c, 0x238),
             F32(c, 0x23c),
             10,
             F32(c, 0x200),
             1,
             1,
             F32(c, 0x220),
             0,
             F32(c, 0x244),
             F32(c, 0x224),
             0,
             0,
             0) != 0 )
      {
        v17 = F32(c, 0x22c) + 1;
        if ( v17 > 100000 )
          v17 = 100000;
        v18 = F32(c, 0x228);
        F32(c, 0x22c) = v17;
        if ( v18 != 0 && v17 >= v18 )
        {
          if ( a3 != 0 )
          {
            F8(v3, 0) = 0;
          }
          else
          {
            if ( F32(c, 0x254) != 0 )
              shot_476740(c, 0);
            W_entity_release((int)c, /*edx*/ 0);
          }
        }
      }
      goto LABEL_9;
    case 3:
      if ( F8(a2, 116) == 0 )
        goto LABEL_9;
      result = 1;
      break;
    default:
LABEL_9:
      v11 = F32(v3, 40);
      if ( F16(v3, 88) >= 3u )
      {
        v12 = v11 - 4000 < 0 ? 0 : v11 - 4000;
      }
      else
      {
        v12 = v11 + 2000;
        if ( v12 > 80000 )
          v12 = 80000;
      }
      v20 = (double)F32(v3, 52);
      F32(v3, 40) = v12;
      v21 = F32(v3, 8);
      v22 = F32(v3, 4);
      *(double *)v31 = v20 * (*(const double *)D(0x8a9a38)) * 0.001;
      a2 = v12 / 100;
      (*v30__f) = (double)(v12 / 100);
      v23 = ftol_round_(sin(*(long double *)v31) * (*v30__f));
      v24 = cos(*(long double *)v31);
      F32(v3, 4) = v22 + v23;
      v25 = ftol_round_(v24 * (*v30__f));
      (*v28__f) = a3;
      F32(v3, 8) = v21 - v25;
      (*v27__f) = 2000 - 4000 * rnd() / 0x8000;
      v26 = rnd();
      shot_4504c0(
        c,
        16000,
        (int)c,
        100045,
        (_DWORD *)v3,
        2000 - 4000 * v26 / 0x8000,
        (*v27__f),
        99,
        1,
        (*v28__f),
        0,
        2,
        0,
        0,
        0);
      result = 0;
      break;
  }
  return result;
}

/* 0x477f10: int __thiscall sub_477F10(cls_4022A0 *this, int a2, int a3) */
int32_t shot_477f10(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v5;
    int32_t * v6;
    intptr_t v7;
    intptr_t * v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    uint16_t i;
    intptr_t v13;
    intptr_t result;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x30 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x28 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x24 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x4 */

  v3 = a2;
  if ( F16(a2, 88) != 0 )
  {
    if ( F16(a2, 88) != 1 )
      goto LABEL_22;
  }
  else
  {
    F16(a2, 88) = 1;
    v5 = rnd();
    v6 = (_DWORD *)F32(c, 0x40);
    v7 = 1000 * v5 / 0x8000 - 2000;
    if ( a3 == 0 )
    {
      v8 = (intptr_t *)W_vpool_take(SYS32(1292), 0);
      if ( v8 != 0 )
      {
        (*v18__f) = F32(v8, 0);
        if ( W_sfx_place(c, F32(CFG(), 360) + 400, 0, 0, v6, &a2, &(*v19__f)) != 0 )
        {
          g_eng.dsb_volume_5bb870((*v18__f), a2);
          g_eng.dsb_pan_5bb880((*v18__f), (*v19__f));
          g_eng.dsb_freq_5bb890((*v18__f), v7);
          g_eng.dsb_play_5bb8c0((*v18__f));
        }
      }
    }
    if ( F32(v3, 108) != D(0x646e68) )
    {
      F32(v3, 108) = D(0x646e68);
      F8(v3, 116) = 0;
      F16(v3, 114) = 0;
      F16(v3, 112) = 0;
    }
  }
  if ( W_body_hit_spawn(
         c,
         F32(c, 0x20c),
         v3,
         0,
         0,
         F32(v3, 12),
         F32(v3, 16),
         7,
         a3,
         0,
         F32(c, 0x234),
         F32(c, 0x238),
         F32(c, 0x23c),
         10,
         F32(c, 0x200),
         1,
         1,
         F32(c, 0x220),
         0,
         F32(c, 0x244),
         F32(c, 0x224),
         0,
         0,
         0) != 0 )
  {
    v10 = F32(c, 0x22c) + 1;
    if ( v10 > 100000 )
      v10 = 100000;
    v11 = F32(c, 0x228);
    F32(c, 0x22c) = v10;
    if ( v11 != 0 && v10 >= v11 )
    {
      if ( a3 != 0 )
      {
        F8(v3, 0) = 0;
      }
      else
      {
        if ( F32(c, 0x254) != 0 && F32(c, 0x1b0) != 0 )
        {
          for ( i = 0; i < F16(c, 0x1b4); ++i )
            W_entity_snd_release(c, i, 0, 0);
        }
        W_entity_release((int)c, /*edx*/ 0);
      }
    }
  }
LABEL_22:
  v13 = F32(v3, 40) + 2000;
  if ( v13 > 40000 )
    v13 = 40000;
  (*v17__f) = F32(v3, 8);
  (*v16__f) = F32(v3, 4);
  (*v15__f) = F32(v3, 52);
  F32(v3, 40) = v13;
  W_polar_div(v13 / 100, (*v15__f), 1000, (*v16__f), (*v17__f), (_DWORD *)(v3 + 4), (_DWORD *)(v3 + 8));
  shot_476e20(c, v3, a3);
  a2 = 2000 - 4000 * rnd() / 0x8000;
  (*v19__f) = 2000 - 4000 * rnd() / 0x8000;
  result = a3;
  if ( a3 == 0 )
  {
    result = W_view_box_visible(
               *(intptr_t *)&GAP(W, 0x1030, 28),
               F32(v3, 4),
               F32(v3, 8),
               F32(v3, 12),
               F32(v3, 16),
               16000);
    if ( result != 0 )
      return W_particle_spawn(c, (int)c, 100045, (_DWORD *)v3, (*v19__f), a2, 99, 1, 0, 0, 2, 0, 0, 0);
  }
  return result;
}

/* 0x4781f0: int __thiscall sub_4781F0(cls_4022A0 *this, int a2, int a3) */
int32_t shot_4781f0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v12;
    int32_t * v13;
    intptr_t v14;
    intptr_t * v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    double v24;
    intptr_t v25;
    intptr_t v26;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v29 = (intptr_t *)(frame_ + 0x8);   /* [2] at ebp-0x8 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    double *const v28__f = (double *)(frame_ + 0x0);   /* ebp-0x10 */

  v3 = a2;
  if ( F16(a2, 88) == 0 )
  {
    F16(a2, 88) = 1;
    if ( F32(c, 0x1f8) == 0 )
    {
      v12 = rnd();
      v13 = (_DWORD *)F32(c, 0x40);
      v14 = 1000 * v12 / 0x8000 - 2000;
      if ( a3 == 0 )
      {
        v15 = (intptr_t *)W_vpool_take(SYS32(1292), 0);
        if ( v15 != 0 )
        {
          (*v27__f) = F32(v15, 0);
          if ( W_sfx_place(c, F32(CFG(), 360) + 400, 0, 0, v13, &a2, v29) != 0 )
          {
            g_eng.dsb_volume_5bb870((*v27__f), a2);
            g_eng.dsb_pan_5bb880((*v27__f), v29[0]);
            g_eng.dsb_freq_5bb890((*v27__f), v14);
            g_eng.dsb_play_5bb8c0((*v27__f));
          }
        }
      }
    }
  }
  else if ( F16(a2, 88) != 1 )
  {
    if ( F16(a2, 88) == 2 )
    {
      F32(a2, 40) = F32(a2, 40) - 4000 < 0 ? 0 : F32(a2, 40) - 4000;
      v5 = F32(v3, 92);
      if ( v5 > 10 )
        return 1;
      F32(v3, 92) = v5 + 1;
      v6 = 10 - (v5 + 1);
      if ( a3 == 0 )
      {
        v7 = 20 * v6 / 10;
        if ( v7 >= 0 )
        {
          v8 = 20 * v6 / 10;
          if ( v7 >= 20 )
          {
            v10 = SYS32(10148);
            F32(c, 0xf4) = SYS32(10148);
            F32(c, 0xf8) = v10;
            goto LABEL_29;
          }
        }
        else
        {
          v8 = 0;
        }
        v9 = SYS32(4 * v8 + 10072);
        F32(c, 0xf4) = v9;
        F32(c, 0xf8) = v9;
      }
    }
LABEL_29:
    v20 = F32(v3, 8);
    v21 = F32(v3, 40) / 100;
    v22 = F32(v3, 4);
    *(double *)v29 = (double)F32(v3, 52) * (*(const double *)D(0x8a9a38)) * 0.001;
    a2 = v21;
    (*v28__f) = (double)v21;
    v23 = ftol_round_(sin(*(long double *)v29) * (*v28__f));
    v24 = cos(*(long double *)v29);
    F32(v3, 4) = v22 + v23;
    F32(v3, 8) = v20 - ftol_round_(v24 * (*v28__f));
    v25 = 2000 - 4000 * rnd() / 0x8000;
    v26 = 2000 - 4000 * rnd() / 0x8000;
    if ( a3 == 0 && shot_4538f0(*(_DWORD **)&GAP(W, 0x1030, 28), (_DWORD *)v3, 16000) )
      W_particle_spawn(c, (int)c, 100045, (_DWORD *)v3, v26, v25, 99, 1, 0, 0, 2, 0, 0, 0);
    return 0;
  }
  v16 = F32(v3, 92) + 1;
  F32(v3, 92) = v16;
  if ( v16 > 40 )
  {
    ++F16(v3, 88);
    F32(v3, 92) = 0;
  }
  if ( W_body_hit_spawn(
         c,
         F32(c, 0x20c),
         v3,
         0,
         0,
         F32(v3, 12),
         F32(v3, 16),
         7,
         a3,
         0,
         F32(c, 0x234),
         F32(c, 0x238),
         F32(c, 0x23c),
         10,
         F32(c, 0x200),
         1,
         1,
         F32(c, 0x220),
         0,
         F32(c, 0x244),
         F32(c, 0x224),
         0,
         0,
         0) != 0 )
  {
    v18 = F32(c, 0x22c) + 1;
    if ( v18 > 100000 )
      v18 = 100000;
    v19 = F32(c, 0x228);
    F32(c, 0x22c) = v18;
    if ( v19 != 0 && v18 >= v19 )
    {
      if ( a3 != 0 )
      {
        F8(v3, 0) = 0;
      }
      else
      {
        if ( F32(c, 0x254) != 0 )
          shot_476740(c, 0);
        W_entity_release((int)c, /*edx*/ 0);
      }
    }
  }
  goto LABEL_29;
}

/* 0x478550: int __stdcall sub_478550(int a1, int a2) */
int32_t shot_478550(int32_t a1, int32_t a2)
{

    intptr_t v2;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;

  v2 = F32(a1, 52);
  if ( v2 == 180000 )
    return 1;
  if ( v2 > 180000 )
  {
    v4 = v2 - a2;
    if ( v4 < 180000 )
      v4 = 180000;
    F32(a1, 52) = v4;
  }
  v5 = F32(a1, 52);
  if ( v5 < 180000 )
  {
    v6 = a2 + v5;
    if ( v6 > 180000 )
      v6 = 180000;
    F32(a1, 52) = v6;
  }
  return 0;
}

/* 0x4785a0: __int16 __thiscall sub_4785A0(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, _DWORD *a8) */
int32_t shot_4785a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    uint16_t v15;
    int16_t * v16;
    uint16_t v17;
    int v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    int16_t v23;
    intptr_t v24;
    intptr_t v25;

  v8 = a2;
  v10 = F16(a2, 88);
  if ( F16(a2, 88) != 0 )
  {
    v11 = v10 - 1;
    if ( v11 != 0 )
    {
      v10 = v11 - 1;
      if ( v10 != 0 )
        return v10;
    }
    else
    {
      SETLO(v10, F16(a2, 60));
      if ( (uint16_t)v10 <= (uint16_t)a4 )
        return v10;
      ++F16(a2, 88);
    }
    v12 = a3;
    v10 = W_body_hit_spawn(
            c,
            F32(c, 0x20c),
            a2,
            0,
            0,
            F32(a2, 12),
            F32(a2, 16),
            7,
            a3,
            0,
            F32(c, 0x234),
            F32(c, 0x238),
            F32(c, 0x23c),
            (int)a8,
            F32(c, 0x200),
            1,
            1,
            F32(c, 0x220),
            0,
            F32(c, 0x244),
            F32(c, 0x224),
            0,
            0,
            0);
    if ( v10 != 0 )
    {
      v14 = F32(c, 0x22c) + 1;
      if ( v14 > 100000 )
        v14 = 100000;
      v10 = F32(c, 0x228);
      F32(c, 0x22c) = v14;
      if ( v10 != 0 && v14 >= v10 )
      {
        if ( v12 != 0 )
        {
          F8(a2, 0) = 0;
        }
        else
        {
          if ( F32(c, 0x254) != 0 && F32(c, 0x1b0) != 0 && F16(c, 0x1b4) != 0 )
          {
            do
            {
              W_entity_snd_release(c, v12, 0, 0);
              SETLO(v12, v12 + 1);
            }
            while ( (uint16_t)v12 < F16(c, 0x1b4) );
          }
          W_entity_release((int)c, /*edx*/ 0);
        }
      }
    }
  }
  else
  {
    v15 = a5;
    if ( (uint16_t)a5 != 0 )
    {
      a7 = a6 + a7 * rnd() / 0x8000;
      a8 = (_DWORD *)F32(c, 0x40);
      SETLO(v10, a3);
      if ( a3 == 0 )
      {
        v16 = SYS32(4 * v15 + 868);
        v10 = F32(v16, 4 * 2);
        if ( v10 != 0 && (F32(v16, 4 * 1) != 0 || (v10 = W_vpool_load(v16)) != 0) )
        {
          v17 = *v16;
          SETLO(v10, 0);
          v18 = *v16 == 0;
          a4 = 0;
          if ( !v18 )
          {
            v19 = F32(v16, 4 * 1);
            v20 = (uint16_t)v16[1];
            while ( 1 )
            {
              v21 = (v20 + (uint16_t)v10) % v17;
              if ( F32(v19 + 8 * (uint16_t)v21, 4) == 0 )
                break;
              SETLO(v10, a4 + 1);
              if ( (uint16_t)++a4 >= (uint16_t)*v16 )
              {
                ++F16(a2, 88);
                return v10;
              }
            }
            v23 = v21 + 1;
            v24 = 8 * (uint16_t)v21;
            v16[1] = v23;
            F32(v24 + v19, 4) = 0;
            v10 = v24 + F32(v16, 4 * 1);
            if ( v10 != 0 )
            {
              v25 = F32(v10, 0);
              v10 = W_sfx_place(c, F32(CFG(), 360), 0, 0, a8, &a3, &a5);
              if ( v10 != 0 )
              {
                g_eng.dsb_volume_5bb870(v25, a3);
                g_eng.dsb_pan_5bb880(v25, a5);
                g_eng.dsb_freq_5bb890(v25, a7);
                SETLO(v10, g_eng.dsb_play_5bb8c0(v25));
              }
            }
            v8 = a2;
          }
        }
      }
    }
    ++F16(v8, 88);
  }
  return v10;
}

/* 0x478820: int __stdcall sub_478820(_DWORD *a1, unsigned __int16 a2, int a3, _DWORD *a4, int a5) */
int32_t shot_478820(int32_t * a1, uint16_t a2, int32_t a3, int32_t * a4, int32_t a5)
{

    uint16_t v6;
    uint16_t v7;
    intptr_t v8;
    intptr_t v9;
    uint16_t v10;
    int32_t * v11;
    int64_t v12;
    intptr_t v13;
    double v14;
    double v15;
    double v16;
    double v17;
    double v18;
    intptr_t v19;
    intptr_t v21;

  v6 = F16(a3, 68);
  v7 = a2;
  if ( a2 >= v6 )
    v7 = 0;
  v8 = F32(a3, 64);
  v9 = F32(v8 + 660 * v7, 4) + F32(v8 + 660 * v7, 12) / 2 - a1[3] / 2 - a1[1];
  v21 = v9;
  if ( a5 != 0 && a1[11] == 3 )
  {
    v9 = -v9;
    v21 = v9;
  }
  v10 = a2;
  if ( a2 >= v6 )
    v10 = 0;
  v11 = (_DWORD *)(v8 + 660 * v10);
  v12 = a1[4] - a1[5];
  v13 = v11[5] + v11[2] + (v11[4] - v11[5]) / 2 - (((int)v12 - HIDWORD(v12)) >> 1) - a1[2] - a1[5];
  if ( v9 != 0 || v13 != 0 )
  {
    if ( (int)abs32(v13) >= (int)abs32(v9) )
    {
      if ( v13 >= 0 )
      {
        v16 = atan2((double)v21 / (double)v13, 1.0);
        if ( v9 >= 0 )
          v14 = (*(const double *)D(0x8a6b48)) - v16;
        else
          v14 = (*(const double *)D(0x8a6b40)) - v16;
      }
      else
      {
        v14 = atan2((double)v21 / (double)v13, 1.0) * -1.0;
      }
    }
    else
    {
      v15 = atan2((double)v13 / (double)v21, 1.0);
      if ( v9 >= 0 )
        v14 = v15 + (*(const double *)D(0x8a6b28));
      else
        v14 = v15 + (*(const double *)D(0x8a6618));
    }
  }
  else
  {
    v14 = 0.0;
  }
  if ( (v14 < (*(const double *)D(0x8a6b28)) || v14 >= (*(const double *)D(0x8a6b48))) && (v14 < (*(const double *)D(0x8a6618)) || v14 >= 0.0) )
    v17 = (double)v13 / cos(v14) * -1.0;
  else
    v17 = (double)v21 / sin(v14);
  if ( v17 >= 0.0 )
    v18 = v17 + 0.5;
  else
    v18 = v17 - 0.5;
  if ( a4 != 0 )
    *a4 = (int64_t)v18;
  v19 = -a1[13] - (int64_t)(v14 * -1000.0 / (*(const double *)D(0x8a9a38)));
  if ( v19 >= 0 )
    return v19 % 360000;
  else
    return v19 + 360000 * (1 - v19 / 360000);
}

/* 0x54cab0: int __stdcall sub_54CAB0( int a1, int a2, int a3, int a4, int a5, int a6, int a7, unsigned __int16 a8, int a9, int a10, int a11, int a12) */
int32_t shot_54cab0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint16_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12)
{

    intptr_t result;
    intptr_t v13;
    intptr_t v14;
    uint8_t * v15;
    uint16_t v16;
    uint16_t v17;
    uint16_t v18;
    uint16_t v19;
    uint16_t v20;
    intptr_t v21;
    uint8_t * v22;
    intptr_t v23;
    intptr_t v24;
    int v25;
    intptr_t v26;
    int v27;
    intptr_t v29;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  result = a7;
  if ( a7 == 0 )
  {
    v13 = a1;
    v29 = 4448;
    for ( (*i__f) = 32; ; --(*i__f) )
    {
      v14 = *(_DWORD *)&GAP(W, 0x0, v29);
      if ( F32(v14, 464) != 1 )
        goto LABEL_40;
      v15 = F32(v14, 64);
      if ( *v15 != 1 || v15[126] != 0 || F16(F32(v14, 1872), 1098) != 0 || v14 == a5 )
        goto LABEL_40;
      v16 = F16(v14, 68);
      v17 = a8;
      if ( a8 >= v16 )
        v17 = 0;
      if ( F32(&v15[660 * v17 + 4], 0) > v13 + a3 )
        goto LABEL_40;
      v18 = a8;
      if ( a8 >= v16 )
        v18 = 0;
      if ( F32(&v15[660 * v18 + 4], 0) + F32(&v15[660 * v18 + 12], 0) <= v13 )
        goto LABEL_40;
      v19 = a8;
      if ( a8 >= v16 )
        v19 = 0;
      if ( F32(&v15[660 * v19 + 8], 0) + F32(&v15[660 * v19 + 20], 0) > a2 + a4 )
        goto LABEL_40;
      v20 = a8;
      if ( a8 >= v16 )
        v20 = 0;
      if ( F32(&v15[660 * v20 + 16], 0) + F32(&v15[660 * v20 + 8], 0) - 1 <= a2 )
        goto LABEL_40;
      v21 = a9;
      v22 = &v15[660 * a8];
      if ( F16(v22, 2 * 36) != 0 && F16(v22, 2 * 43) > 0xD2u )
        goto LABEL_40;
      if ( a6 != F32(v22, 4 * 11) )
        v21 = -a9;
      if ( a12 != 0 )
        v21 = -v21;
      if ( v21 <= 0 )
        break;
      v23 = F32(v22, 4 * 10);
      v24 = a11;
      if ( v23 >= a11 )
        goto LABEL_31;
LABEL_33:
      F32(v22, 4 * 10) = v21 + v23;
LABEL_34:
      v25 = a10 < 0;
      if ( a10 > 0 )
      {
        v26 = F32(v22, 4 * 6);
        if ( v26 < v24 )
          goto LABEL_39;
        v25 = a10 < 0;
      }
      if ( v25 )
      {
        v26 = F32(v22, 4 * 6);
        if ( v26 > -v24 )
LABEL_39:
          F32(v22, 4 * 6) = a10 + v26;
      }
LABEL_40:
      result = (*i__f) - 1;
      v27 = (*i__f) == 1;
      v29 += 4;
      if ( v27 )
        return result;
      v13 = a1;
    }
    v24 = a11;
LABEL_31:
    if ( v21 >= 0 )
      goto LABEL_34;
    v23 = F32(v22, 4 * 10);
    if ( v23 <= -v24 )
      goto LABEL_34;
    goto LABEL_33;
  }
  return result;
}

/* 0x54cca0: int __thiscall sub_54CCA0(cls_54CCA0 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t shot_54cca0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    int32_t * v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    double v18;
    double v19;
    double v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    int v25;
    _Alignas(8) uint8_t frame_[0xb4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v37 = (intptr_t *)(frame_ + 0x34);   /* [32] at ebp-0x80 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xb4 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xa4 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xa0 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x9c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x98 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x94 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x90 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8c */
    int32_t * *const v35__f = (int32_t * *)(frame_ + 0x2c);   /* ebp-0x88 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x84 */

  v6 = (_DWORD *)F32(c, 0x40);
  v7 = 0;
  (*v34__f) = c;
  (*v35__f) = v6;
  (*v28__f) = 0;
  (*v29__f) = 4448;
  (*v31__f) = 32;
  do
  {
    v8 = *(_DWORD *)&GAP(W, 0x0, (*v29__f));
    (*v36__f) = v8;
    if ( F32(v8, 464) == 1 )
    {
      v9 = F32(v8, 64);
      if ( F8(v9, 0) == 1 && (a5 != 0 || F8(v9, 126) == 0) && F16(F32(v8, 1872), 1098) == 0 )
      {
        v10 = F32(c, 0x1dc);
        v11 = F32(v8, 476);
        if ( v10 != 0 && v11 != 0 )
        {
          if ( v10 == 3 || v11 == 3 )
          {
            v12 = 1;
          }
          else
          {
            v12 = (v10 == v11) + 1;
            v6 = (*v35__f);
          }
        }
        else
        {
          v12 = 0;
        }
        if ( F32(c, 0x1f0) != 0 && (v12 == 1 || v12 == 2) || v12 == 1 )
        {
          if ( a4 != 0 )
          {
            SETLO(c, F16(v8, 68));
            v13 = F32(v9, 12);
            (*v33__f) = c;
            v14 = F32(v9, 4) + v13 / 2 - v6[3] / 2 - v6[1];
            (*v32__f) = v14;
            if ( a6 != 0 && v6[11] == 3 )
            {
              v14 = -v14;
              (*v32__f) = v14;
            }
            v15 = v6[5];
            v16 = F32(v9, 20);
            v17 = (F32(v9, 16) - v16) / 2 - (v6[4] - v15) / 2;
            v6 = (*v35__f);
            (*v33__f) = (v16 + F32(v9, 8) + v17 - (*v35__f)[2] - v15);
            v18 = shot_54cfc0(v14, (int)(*v33__f));
            if ( (v18 < (*(const double *)D(0x8a6b28)) || v18 >= (*(const double *)D(0x8a6b48))) && (v18 < (*(const double *)D(0x8a6618)) || v18 >= 0.0) )
              v19 = (double)(int)(*v33__f) / cos(v18) * -1.0;
            else
              v19 = (double)(*v32__f) / sin(v18);
            if ( v19 >= 0.0 )
              v20 = v19 + 0.5;
            else
              v20 = v19 - 0.5;
            if ( &(*v27__f) != (int *)-24 )
              (*v30__f) = (int64_t)v20;
            v21 = -v6[13] - (int64_t)(v18 * -1000.0 / (*(const double *)D(0x8a9a38)));
            if ( v21 >= 0 )
              v22 = v21 % 360000;
            else
              v22 = v21 + 360000 * (1 - v21 / 360000);
            if ( (*v30__f) >= a2 && a2 != 0 || v22 >= 1000 * a3 && v22 <= 1000 * (360 - a3) )
            {
              c = (*v34__f);
              v7 = (*v28__f);
            }
            else
            {
              v23 = (uint16_t)(*v28__f);
              v7 = ++(*v28__f);
              v37[v23] = (*v36__f);
              c = (*v34__f);
            }
          }
          else
          {
            v24 = (uint16_t)v7++;
            (*v28__f) = v7;
            v37[v24] = v8;
          }
        }
      }
    }
    v25 = (*v31__f) == 1;
    (*v29__f) += 4;
    --(*v31__f);
  }
  while ( !v25 );
  if ( (uint16_t)v7 != 0 )
    return v37[(uint16_t)v7 * rnd() / 0x8000];
  else
    return 0;
}

/* 0x55f150: int __stdcall sub_55F150(int a1, int a2) */
int32_t shot_55f150(int32_t a1, int32_t a2)
{

    intptr_t v2;

  v2 = 20 * a1 / a2;
  if ( v2 < 20 )
    return SYS32(4 * (v2 < 0 ? 0 : v2) + 10252);
  else
    return SYS32(10328);
}

/* 0x54cfc0: double __stdcall sub_54CFC0(int a1, int a2) */
double shot_54cfc0(int32_t a1, int32_t a2)
{

    double v3;
    double v4;

  if ( a1 == 0 && a2 == 0 )
    return 0.0;
  if ( (int)abs32(a2) >= (int)abs32(a1) )
  {
    if ( a2 >= 0 )
    {
      v4 = atan2((double)a1 / (double)a2, 1.0);
      if ( a1 >= 0 )
        return (*(const double *)D(0x8a6b48)) - v4;
      else
        return (*(const double *)D(0x8a6b40)) - v4;
    }
    else
    {
      return atan2((double)a1 / (double)a2, 1.0) * -1.0;
    }
  }
  else
  {
    v3 = atan2((double)a2 / (double)a1, 1.0);
    if ( a1 >= 0 )
      return v3 + (*(const double *)D(0x8a6b28));
    else
      return v3 + (*(const double *)D(0x8a6618));
  }
}

/* 0x590230: unsigned __int16 __thiscall sub_590230(cls_4022A0 *this) */
int32_t room_band64_tick(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t result;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    int32_t * v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t * v12;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    int64_t *const v14__f = (int64_t *)(frame_ + 0x4);   /* ebp-0x20 */
    uint16_t *const i__f = (uint16_t *)(frame_ + 0x18);   /* ebp-0xc */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x8 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x4 */

  result = shots_tick(c, F32(c, 0x40), F32(c, 0x40), 0, 0);
  v3 = 1;
  if ( F16(c, 0x44) > 1u )
  {
    do
    {
      SETHIDW((*v14__f), v3);
      SETLODW((*v14__f), 1);
      (*v13__f) = F32(c, 0x40) + 660 * (uint16_t)v3;
      result = shots_tick(c, (*v13__f) - 660, (*v13__f), (int32_t)(*v14__f), (int32_t)((uint64_t)(*v14__f) >> 32));
      ++v3;
    }
    while ( (uint16_t)v3 < F16(c, 0x44) );
  }
  if ( F32(c, 0x1b0) != 0 )
  {
    result = 0;
    for ( (*i__f) = 0; (*i__f) < F16(c, 0x1b4); ++(*i__f) )
    {
      v4 = 36 * result;
      v5 = F32(c, 0x1b0) + v4;
      if ( F32(v5, 0) == 0 )
        goto LABEL_27;
      v6 = (_DWORD *)F32(c, 0x40);
      if ( F32(v5, 20) == 0 )
      {
        if ( F32(v5, 24) != 0 )
          goto LABEL_10;
        F32(v5, 24) = 1;
        F32(v5, 28) = 0;
      }
      if ( F32(v5, 24) == 0 )
        goto LABEL_14;
LABEL_10:
      v7 = F32(v5, 28);
      if ( v7 > -5000 )
      {
        v8 = v7 - 100;
        if ( v8 < -5000 )
          v8 = -5000;
        F32(v5, 28) = v8;
LABEL_14:
        if ( W_sfx_place(
               c,
               F32(CFG(), 360) + F32(v5, 8),
               F32(v5, 16),
               0,
               v6,
               &(*v16__f),
               &(*v17__f)) != 0 )
        {
          v9 = **(intptr_t **)v5;
          v10 = F32(v5, 28) + (*v16__f);
          if ( v10 < -10000 )
            v10 = -10000;
          g_eng.dsb_volume_5bb870(v9, v10);
          g_eng.dsb_pan_5bb880(v9, (*v17__f));
          g_eng.dsb_freq_5bb890(v9, F32(v5, 12));
          goto LABEL_27;
        }
        v11 = F32(c, 0x1b0);
        if ( v11 != 0 )
        {
          if ( F16(c, 0x1b4) <= (*i__f) )
          {
LABEL_20:
            g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85525c), D(0x8a9b6c));
            goto LABEL_27;
          }
          v12 = F32(v4, v11);
          if ( v12 != 0 )
          {
LABEL_26:
            g_eng.dsb_stop_5bb8e0(F32(v12, 0));
            F32(F32((v4 + v11), 0), 4) = 0;
            F32((v4 + v11), 0) = 0;
            F32(v4 + v11, 4) = 0;
            F32(v4 + v11, 32) = 0;
            goto LABEL_27;
          }
        }
        goto LABEL_27;
      }
      v11 = F32(c, 0x1b0);
      if ( v11 != 0 )
      {
        if ( F16(c, 0x1b4) <= (*i__f) )
          goto LABEL_20;
        v12 = F32(v4, v11);
        if ( v12 != 0 )
          goto LABEL_26;
      }
LABEL_27:
      result = (*i__f) + 1;
      F32(v5, 20) = 0;
    }
  }
  return result;
}
/* unported callees: none */
