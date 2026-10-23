/* char_handlers2.c — the INTEGRATOR's per-type handlers the corpus does not reach yet (0x442a70's type dispatch:
 * monster / boss / ally skill bodies — 0x4539b0 0x456d30 0x452730 0x446900 0x44f690 0x44e8d0 0x44c250 0x44a740
 * 0x44b160 0x4516e0 0x449810 0x448dc0 0x44d8f0 0x4484b0 0x452020 0x450810 0x44a200 0x44bc10) + their helpers (0x445e50
 * 0x44a160 0x44e1c0 0x44e730 0x455c90 0x455cf0 0x455d20 0x45e260 0x45e3e0 0x461640 0x467330).  See char_handlers2.h.
 * (The reached ones are char_handlers.c.)
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  UNVERIFIED: no recording meets
 * these types yet; REPLACE-wired, compile-checked. */
#include "char_handlers2.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "actor_ops.h"
#include "actor_update.h"
#include "arche_pose.h"
#include "char_attack.h"
#include "char_def.h"
#include "char_handlers.h"
#include "char_integrate.h"
#include "char_state.h"
#include "char_status.h"
#include "entity_fx.h"
#include "entity_life.h"
#include "entity_move.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "particle_spawn.h"
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

static inline intptr_t W_body_action_reset(intptr_t a0, ...) { body_action_reset((wl_body *)a0); return 0; }
static inline intptr_t W_body_action_start(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)body_action_start((wl_body *)a0, (int16_t)a1, (int32_t)a2); }
static inline intptr_t W_body_facing_sign(intptr_t a0, intptr_t a1, ...) { return (intptr_t)body_facing_sign((const wl_body *)a0, (int32_t)a1); }
static inline intptr_t W_body_hit_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, intptr_t a20, intptr_t a21, intptr_t a22, intptr_t a23, ...) { return (intptr_t)body_hit_spawn((wl_char *)a0, (wl_char *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19, (int32_t)a20, (int32_t)a21, (int32_t)a22, (int32_t)a23); }
static inline intptr_t W_body_pose_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { body_pose_set((wl_body *)a0, (uint16_t)a1, (uint16_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_body_pose_swap(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)body_pose_swap((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7); }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1, ...) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_char_attack_hits(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)char_attack_hits((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (const uint8_t *)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_char_carry(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_carry((wl_char *)a0, (wl_char *)a1); }
static inline intptr_t W_char_drift(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)char_drift((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12); }
static inline intptr_t W_char_drift_move(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)char_drift_move((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_char_lunge(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)char_lunge((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_char_mag_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_mag_roll((const wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_char_mag_sum(intptr_t a0, ...) { return (intptr_t)char_mag_sum((const wl_char *)a0); }
static inline intptr_t W_char_party_mode_set(intptr_t a0, intptr_t a1, ...) { char_party_mode_set((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_char_release(intptr_t a0, ...) { char_release((wl_char *)a0); return 0; }
static inline intptr_t W_char_room_logs(intptr_t a0, ...) { return (intptr_t)char_room_logs((const wl_char *)a0); }
static inline intptr_t W_char_set_look_up(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)char_set_look_up((wl_char *)a0, (int16_t)a1, (const void *)a2, (const void *)a3, (int16_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_char_shot_fire(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)char_shot_fire((wl_char *)a0, (int32_t)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int16_t)a7, (int32_t)a8, (const uint8_t *)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_char_skill_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skill_find((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_char_skill_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_skill_roll((const wl_char *)a0, (const uint8_t *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_clip_seq_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)clip_seq_step((wl_body *)a0, (intptr_t)a1, (uint16_t)a2, (uint16_t)a3); }
static inline intptr_t W_entity_dust(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { entity_dust((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (uint32_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_entity_fx_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_fx_add((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_fx_remove(intptr_t a0, intptr_t a1, ...) { entity_fx_remove((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_move_horizontal(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_move_horizontal((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (uint16_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_entity_scatter(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)entity_scatter((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_entity_snd_keep(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_snd_keep((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2); return 0; }
static inline intptr_t W_entity_snd_release(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { entity_snd_release((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_entity_snd_start(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_snd_start((wl_entity *)a0, (uint16_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_sprite_descs(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_sprite_descs((const wl_entity *)a0, (const wl_body *)a1, (es_desc *)a2, (uint16_t *)a3); }
static inline intptr_t W_entity_track_on(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_track_on((wl_entity *)a0, (uint16_t)a1, (int16_t)a2, (int16_t)a3); }
static inline intptr_t W_facing_back(intptr_t a0, ...) { return (intptr_t)facing_back((int32_t)a0); }
static inline intptr_t W_ih_4466b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { ih_4466b0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_item_effect(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)item_effect((wl_char *)a0, (int32_t)a1, (int32_t)a2, (uint16_t *)a3, (int32_t)a4, (int32_t)a5, (int16_t)a6); }
static inline intptr_t W_part_center_x(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_x((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_center_y(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_y((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_particle_tint_set(intptr_t a0, intptr_t a1, ...) { particle_tint_set((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_party_find(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)party_find((const wl_char *)a0, (int32_t)a1, (int16_t)a2, (uint16_t *)a3, (uint16_t)a4); }
static inline intptr_t W_room_hitstop_set(intptr_t a0, intptr_t a1, ...) { room_hitstop_set((int32_t)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_shot_445ce0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)shot_445ce0((wl_char *)a0, (int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_shot_449700(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)shot_449700((wl_char *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_shot_4497b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)shot_4497b0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_shot_4504c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, ...) { return (intptr_t)shot_4504c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14); }
static inline intptr_t W_shot_4538f0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)shot_4538f0((int32_t *)a0, (int32_t *)a1, (int32_t)a2); }
static inline intptr_t W_shot_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, ...) { return (intptr_t)shot_spawn((void *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (wl_body *)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int16_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19); }
static inline intptr_t W_skill_cast(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)skill_cast((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t *)a6, (uint16_t *)a7, (int32_t)a8); }
static inline intptr_t W_skill_charge(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)skill_charge((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t *)a4, (int32_t)a5, (uint16_t *)a6, (uint16_t)a7, (int32_t)a8); }
static inline intptr_t W_skill_mp_pay(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)skill_mp_pay((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_slash_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)slash_step((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (intptr_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_slash_trail(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)slash_trail((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_speed_ramp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)speed_ramp((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_tint_fade_a(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tint_fade_a((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_voice_cue(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)voice_cue((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_voice_group_prio(intptr_t a0, intptr_t a1, ...) { return (intptr_t)voice_group_prio((const wl_voiceset *)a0, (uint16_t)a1); }
static inline intptr_t W_voice_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)voice_place((const wl_char *)a0, (int32_t *)a1, (int32_t *)a2, (int32_t *)a3); }
static inline intptr_t W_voice_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)voice_play((wl_voiceset *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_voiceset_cut(intptr_t a0, ...) { voiceset_cut((wl_voiceset *)a0); return 0; }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }
static inline intptr_t W_world_log_line(intptr_t a0, intptr_t a1, ...) { world_log_line((wl_world *)a0, (const char *)a1); return 0; }

int32_t ih_445e50(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5);
int32_t ih_446900(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t * a4, int32_t i, int32_t a6, int32_t * a7, int32_t a8);
int32_t ih_4484b0(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ih_448dc0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
void ih_449810(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);
int32_t ih_44a160(int32_t a1, int32_t a2, int16_t a3, int32_t a4);
int32_t ih_44a200(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_44a740(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t a6, int32_t a7);
int32_t ih_44b160(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t a6, int32_t a7);
void ih_44bc10(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5);
int32_t ih_44c250(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5);
int32_t ih_44d8f0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8);
int32_t ih_44e1c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t ih_44e730(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6);
int32_t ih_44e8d0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_44f690(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9);
int32_t ih_450810(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_4516e0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ih_452020(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t ih_452730(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6);
int32_t ih_4539b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, int32_t a7);
int32_t ih_455c90(wl_char *self, uint16_t a2, int32_t a3);
int32_t ih_455cf0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t ih_455d20(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4);
int32_t ih_456d30(wl_char *self, int32_t * a3, int32_t a4, uint16_t a5, int32_t a6, int32_t a7);
int32_t ih_45e260(wl_char *self, int32_t * a2, int32_t * a3, int32_t a4);
int32_t ih_45e3e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t ih_461640(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t ih_467330(wl_char *self);

/* 0x445e50: int __thiscall sub_445E50(cls_4022A0 *this, int a2, int a3, _DWORD *a4, _DWORD *a5) */
int32_t ih_445e50(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5)
{
    intptr_t c = (intptr_t)self;
    int32_t * v5 = 0;
    uint16_t * v6 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    int16_t v9 = 0;
    intptr_t v10 = 0;
    int32_t * v11 = 0;
    uint16_t v12 = 0;
    uint32_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v5 = (_DWORD *)a2;
  v6 = F32(a2, 108);
  if ( v6 != 0 )
  {
    v7 = F16(a2, 114);
    v8 = F32(&v6[2 * v7 + 40], 0);
    v9 = *v6 + v6[v7 + 1];
    (*v22__f) = F32(&v6[2 * v7 + 104], 0);
    v10 = F32(a2, 44);
    a2 = v8;
    if ( v10 == 3 )
      v9 += F16(F32(c, 0x8), 24);
    SETLO(v6, v9);
  }
  v11 = (int *)F32(c, 0x8);
  v12 = 0;
  v13 = v11[7];
  if ( v13 == 0 )
    return 0;
  v14 = *v11;
  v15 = v13 * (uint16_t)v6;
  v16 = 0;
  while ( a3 != F32((v14 + 52 * (v15 + v16)), 0) )
  {
    v16 = ++v12;
    if ( v12 >= v13 )
      return 0;
  }
  v18 = v14 + 52 * (v15 + v12);
  if ( v18 == 0 )
    return 0;
  v19 = F32(v18, 44);
  v20 = v5[16];
  if ( v5[11] == 1 )
    v21 = v19 + v20;
  else
    v21 = v20 - v19;
  *a4 = 100 * (a2 + v21);
  *a5 = 100 * ((*v22__f) + F32(v18, 48) + v5[17]);
  return 1;
}

/* 0x446900: int __thiscall sub_446900(cls_4022A0 *this, int a2, int errno_decorate, _DWORD *a4, int i, int a6, _DWORD *a7, int a8) */
int32_t ih_446900(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t * a4, int32_t i, int32_t a6, int32_t * a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    void * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t * v15 = 0;
    intptr_t v16 = 0;
    void * v17 = 0;
    intptr_t result = 0;
    intptr_t v19 = 0;
    void * v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t * v25 = 0;
    intptr_t v26 = 0;
    void * v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    int32_t * v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    int32_t * v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    uint16_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x20 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1c */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x18 */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x18 */

  W_skill_cast((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)((int)a4), (intptr_t)(a8), (intptr_t)(a6), (intptr_t)(a7), (intptr_t)(D(0x641078)), (intptr_t)(210));
  switch ( F16(a2, 72) )
  {
    case 2:
      *a4 = 2;
      if ( F16(a2, 88) != 0 )
      {
        if ( F16(a2, 88) != 1 )
        {
          if ( F16(a2, 88) != 2 || F8(a2, 116) == 0 )
            return 0;
          goto LABEL_152;
        }
      }
      else
      {
        if ( F32(a2, 108) != D(0x63da90) )
        {
          F32(a2, 108) = D(0x63da90);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        F16(a2, 88) = 1;
      }
      if ( F16(a2, 114) != 1 )
        return 0;
      v30 = W_facing_back((intptr_t)(F32(a2, 44)));
      ++F16(a2, 88);
      F32(a2, 44) = v30;
      F32(a2, 40) = 0;
      return 0;
    case 0x59:
      *a4 = 2;
      v37 = 0;
      switch ( F16(a2, 88) )
      {
        case 0:
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x640a28)));
          F16(a2, 88) = 1;
          v37 = 1;
          goto LABEL_79;
        case 1:
LABEL_79:
          if ( F8(a2, 116) == 0 )
            goto LABEL_87;
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x640bbc)));
          F16(a2, 88) = 2;
          goto LABEL_88;
        case 2:
          if ( F8(a2, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x640d50)));
            W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(1));
            F16(a2, 88) = 3;
          }
          else
          {
LABEL_87:
            if ( v37 == 0 )
              goto LABEL_101;
          }
LABEL_88:
          if ( F16(a2, 88) == 2 )
          {
            if ( errno_decorate == 0
              && F32(c, 0x15b54) != 0
              && F32(c, 0x15b5c) == 0
              && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
              && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)((int *)&a7), (intptr_t)(&i)) != 0 )
            {
              W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x45u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)((int)a7), (intptr_t)(i), (intptr_t)(0));
            }
          }
          else if ( F16(a2, 88) == 3 )
          {
            v38 = 1000 * rnd() / 0x8000 + 1000;
            a7 = (_DWORD *)F32(c, 0x40);
            if ( errno_decorate == 0 )
            {
              v39 = (int *)W_vpool_take((intptr_t)(SYS32(1140)), (intptr_t)(0));
              if ( v39 != 0 )
              {
                i = *v39;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a7), (intptr_t)(&a6), (intptr_t)(&a8)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(i, a6);
                  g_eng.dsb_pan_5bb880(i, a8);
                  (*v67__f) = v38;
                  v40 = i;
                  g_eng.dsb_freq_5bb890(i, (*v67__f));
                  g_eng.dsb_play_5bb8c0(v40);
                }
              }
            }
          }
LABEL_101:
          if ( F16(a2, 90) >= 2u )
            return 0;
          if ( F16(a2, 88) == 2 )
          {
            W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(4000), (intptr_t)(-2000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(11));
            result = 0;
          }
          else
          {
            if ( F16(a2, 88) != 3 )
              return 0;
            W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(4800), (intptr_t)(3600), (intptr_t)(4000), (intptr_t)(4800), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(11));
            result = 0;
          }
          break;
        case 3:
          if ( F8(a2, 116) == 0 )
            goto LABEL_101;
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x640ee4)));
          W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(1));
          F16(a2, 88) = 4;
          goto LABEL_88;
        case 4:
          if ( F8(a2, 116) == 0 )
            goto LABEL_101;
          W_body_action_reset((intptr_t)(a2));
          F16(a2, 88) = 5;
          goto LABEL_88;
        default:
          goto LABEL_101;
      }
      return result;
    case 0x5A:
      *a4 = 2;
      v31 = 0;
      switch ( F16(a2, 88) )
      {
        case 0:
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x6403d8)));
          F16(a2, 88) = 1;
          v31 = 1;
          goto LABEL_56;
        case 1:
LABEL_56:
          if ( F8(a2, 116) == 0 )
            goto LABEL_64;
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x64056c)));
          F16(a2, 88) = 2;
          goto LABEL_65;
        case 2:
          if ( F8(a2, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x640700)));
            W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(1));
            F16(a2, 88) = 3;
          }
          else
          {
LABEL_64:
            if ( v31 == 0 )
              return 0;
          }
LABEL_65:
          if ( F16(a2, 88) == 1 )
          {
            v32 = F32(a2, 96);
            v33 = F32(a2, 76);
            a7 = F32(a2, 44);
            i = 0;
            F8(a2, 127) = 1;
            a6 = v32;
            v34 = rnd();
            a8 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v33), (intptr_t)(1000), (intptr_t)(1000 * v34 / 0x8000));
            if ( v33[44] == 7 )
            {
              (*v66__f) = v33[180];
              v35 = rnd();
              i = W_char_mag_roll((intptr_t)(c), (intptr_t)(v33[179]), (intptr_t)(1000 * v35 / 0x8000), (intptr_t)((*v66__f)));
            }
            (*v65__f) = F32(F32(c, 0x750), 216)
                + F32(F32(c, 0x750), 224)
                + F32(F32(c, 0x750), 228);
            (*v64__f) = F32(c, 0x1f0);
            (*v63__f) = i;
            v36 = W_char_mag_sum((intptr_t)(c));
            W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100034), (intptr_t)((_DWORD *)a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(errno_decorate), (intptr_t)(0), (intptr_t)(a8), (intptr_t)(0), (intptr_t)(a6), (intptr_t)((int)a7), (intptr_t)(v33[44]), (intptr_t)(v33[45] + v36), (intptr_t)((*v63__f)), (intptr_t)((*v64__f)), (intptr_t)((*v65__f)), (intptr_t)(2));
            return 0;
          }
          if ( F16(a2, 88) == 2
            && errno_decorate == 0
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)((int *)&a7), (intptr_t)(&i)) != 0 )
          {
            W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x46u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)((int)a7), (intptr_t)(i), (intptr_t)(0));
            return 0;
          }
          break;
        case 3:
          if ( F8(a2, 116) == 0 )
            return 0;
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x640894)));
          W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(1));
          F16(a2, 88) = 4;
          goto LABEL_65;
        case 4:
          if ( F8(a2, 116) == 0 )
            return 0;
          W_body_action_reset((intptr_t)(a2));
          F16(a2, 88) = 5;
          goto LABEL_65;
        default:
          return 0;
      }
      return 0;
    case 0x5B:
      *a4 = 3;
      if ( F16(a2, 84) == 1 )
      {
        v46 = F16(a2, 88);
        if ( v46 != 0 )
        {
          if ( F8(a2, 116) == 0 )
            return 0;
          if ( v46 < 3u )
            W_body_set_clip((intptr_t)(a2), (intptr_t)(404 * F16(a2, 88) + 6553368));
        }
        else
        {
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x63ff18)));
        }
        if ( ++F16(a2, 88) == 1 )
        {
          W_voice_cue((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          v47 = 0;
          i = 75;
          do
          {
            v48 = W_char_shot_fire((intptr_t)(c), (intptr_t)(100033), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(errno_decorate), (intptr_t)(v47), (intptr_t)(i), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(F32(a2, 44)), (intptr_t)(0), (intptr_t)(2));
            if ( v48 != 0 && (uint16_t)v47 != 0 )
              F32(v48, 504) = 1;
            v47 += 5;
            i -= 15;
          }
          while ( (uint16_t)v47 < 0x19u );
          return 0;
        }
        else
        {
          if ( F16(a2, 88) != 4 )
            return 0;
          F16(a2, 86) = 210;
          F16(a2, 72) = 0;
          F16(a2, 88) = 0;
          F32(a2, 92) = 0;
          F32(a2, 120) = 0;
          F16(a2, 84) = 0;
          F32(a2, 76) = 0;
          F32(a2, 80) = 0;
          F16(a2, 90) = 0;
          F16(a2, 100) = 0;
          return 0;
        }
      }
      if ( F16(a2, 84) == 3 )
      {
        if ( W_clip_seq_step((intptr_t)(a2), (intptr_t)((int)D(0x63f8c8)), (intptr_t)(4u), (intptr_t)(0)) == 0 )
          return 0;
        v41 = F16(a2, 88) - 2;
        if ( F16(a2, 88) == 2 )
        {
          W_voice_cue((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          v44 = 0;
          i = 120;
          do
          {
            v45 = W_char_shot_fire((intptr_t)(c), (intptr_t)(100033), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(errno_decorate), (intptr_t)(v44), (intptr_t)(i), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(F32(a2, 44)), (intptr_t)(0), (intptr_t)(2));
            if ( v45 != 0 && (uint16_t)v44 != 0 )
              F32(v45, 504) = 1;
            v44 += 5;
            i += 15;
          }
          while ( (uint16_t)v44 < 0x19u );
          return 0;
        }
      }
      else
      {
        if ( W_clip_seq_step((intptr_t)(a2), (intptr_t)((int)D(0x63f278)), (intptr_t)(4u), (intptr_t)(0)) == 0 )
          return 0;
        v41 = F16(a2, 88) - 2;
        if ( F16(a2, 88) == 2 )
        {
          W_voice_cue((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          v42 = 0;
          i = 70;
          do
          {
            v43 = W_char_shot_fire((intptr_t)(c), (intptr_t)(100033), (intptr_t)(a2), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(1), (intptr_t)(errno_decorate), (intptr_t)(v42), (intptr_t)(i), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(F32(a2, 44)), (intptr_t)(0), (intptr_t)(2));
            if ( v43 != 0 && (uint16_t)v42 != 0 )
              F32(v43, 504) = 1;
            v42 += 5;
            i += 10;
          }
          while ( (uint16_t)v42 < 0x19u );
          return 0;
        }
      }
      if ( v41 != 3 )
        return 0;
      goto LABEL_152;
    case 0x5C:
      SETHI(v49, 0);
      *a4 = 3;
      SETLO(v49, F16(a2, 88));
      F8(a2, 124) = 1;
      F8(a2, 125) = 1;
      if ( (uint16_t)v49 != 0 )
      {
        if ( v49 == 1 )
        {
          v50 = F32(a2, 92) + 1;
          F32(a2, 92) = v50;
          if ( v50 % 4 == 0 )
            W_slash_trail((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)(10), (intptr_t)(10));
          if ( F32(a2, 92) > 20 )
          {
            v51 = F32(a2, 40) + 2000;
            F32(a2, 40) = v51;
            if ( v51 <= 0 )
            {
              F32(a2, 40) = v51 + 2000;
              v52 = v51 + 2000;
            }
            else
            {
              v52 = 0;
            }
            v53 = F32(a2, 24);
            F32(a2, 40) = v52;
            v54 = v53 + 1500;
            F32(a2, 24) = v54;
            if ( v54 <= 0 )
            {
              v55 = v54 + 1500;
              F32(a2, 24) = v54 + 1500;
            }
            else
            {
              v55 = 0;
            }
            F32(a2, 24) = v55;
            if ( v52 >= 0 )
            {
LABEL_152:
              W_body_action_reset((intptr_t)(a2));
              return 0;
            }
          }
        }
      }
      else
      {
        if ( F16(a2, 84) == 2 )
        {
          v56 = W_facing_back((intptr_t)(F32(a2, 44)));
          v57 = F32(a2, 40);
          F32(a2, 44) = v56;
          F32(a2, 40) = -v57;
        }
        W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x63e5b8)));
        v58 = rnd();
        W_sfx_play_at((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(134), (intptr_t)(0), (intptr_t)(1000 * v58 / 0x8000 + 1000), (intptr_t)(0));
        ++F16(a2, 88);
        F32(a2, 40) = -80000;
        F32(a2, 24) = -30000;
        W_voice_cue((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      }
      return 0;
    case 0x5D:
      F8(a2, 125) = 1;
      F8(a2, 124) = 1;
      *a4 = 3;
      switch ( F16(a2, 88) )
      {
        case 0:
          W_room_hitstop_set((intptr_t)(1), (intptr_t)(200));
          if ( F32(a2, 108) != D(0x63ec18) )
          {
            F32(a2, 108) = D(0x63ec18);
            F8(a2, 116) = 0;
            F16(a2, 114) = 0;
            F16(a2, 112) = 0;
          }
          ++F16(a2, 88);
          F32(a2, 40) = -40000;
          F32(a2, 24) = -60000;
          if ( errno_decorate == 0
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place((intptr_t)(c), (intptr_t)(&a8), (intptr_t)(&a6), (intptr_t)(&a7)) != 0 )
          {
            W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0xCu), (intptr_t)(1), (intptr_t)(a8), (intptr_t)(a6), (intptr_t)((int)a7), (intptr_t)(0));
          }
          goto LABEL_12;
        case 1:
LABEL_12:
          if ( F8(a2, 116) != 0 )
          {
            v9 = F32(a2, 108);
            ++F16(a2, 88);
            if ( v9 != D(0x63edb0) )
            {
              F32(a2, 108) = D(0x63edb0);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
          }
          v10 = W_speed_ramp((intptr_t)(F32(a2, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(-20000));
          (*v59__f) = F32(a2, 24);
          F32(a2, 40) = v10;
          F32(a2, 24) = W_speed_ramp((intptr_t)((*v59__f)), (intptr_t)(1000), (intptr_t)(-1600), (intptr_t)(64000));
          W_char_drift_move((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)(i), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          goto LABEL_43;
        case 2:
          v11 = W_speed_ramp((intptr_t)(F32(a2, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(-15000));
          (*v60__f) = F32(a2, 24);
          F32(a2, 40) = v11;
          F32(a2, 24) = W_speed_ramp((intptr_t)((*v60__f)), (intptr_t)(4000), (intptr_t)(-1600), (intptr_t)(64000));
          W_char_drift_move((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)(i), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          if ( F32(a2, 36) == 0 )
            goto LABEL_43;
          if ( errno_decorate == 0 )
          {
            for ( i = 8; i != 0; --i )
            {
              v12 = F32(a2, 12) + 1600;
              v13 = rnd();
              W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)a2), (intptr_t)(v12 * v13 / 0x8000 - v12 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
            }
          }
          v14 = 500 - 1000 * rnd() / 0x8000;
          i = F32(c, 0x40);
          if ( errno_decorate == 0 )
          {
            v15 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1396)), (intptr_t)(0));
            if ( v15 != 0 )
            {
              v16 = F32(v15, 0);
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)i), (intptr_t)((int *)&a7), (intptr_t)(&a6)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v16, (int)a7);
                g_eng.dsb_pan_5bb880(v16, a6);
                g_eng.dsb_freq_5bb890(v16, v14);
                g_eng.dsb_play_5bb8c0(v16);
              }
            }
          }
          v17 = F32(a2, 108);
          ++F16(a2, 88);
          if ( v17 != D(0x63ef48) )
          {
            F32(a2, 108) = D(0x63ef48);
            F8(a2, 116) = 0;
            F16(a2, 114) = 0;
            F16(a2, 112) = 0;
          }
          F32(a2, 24) = -60000;
          return 1;
        case 3:
          F32(a2, 40) = W_speed_ramp((intptr_t)(F32(a2, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(-10000));
          v19 = W_speed_ramp((intptr_t)(F32(a2, 24)), (intptr_t)(4000), (intptr_t)(-1600), (intptr_t)(64000));
          (*v61__f) = i;
          F32(a2, 24) = v19;
          W_char_drift_move((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)((*v61__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          if ( F8(a2, 116) == 0 )
            goto LABEL_43;
          v20 = F32(a2, 108);
          ++F16(a2, 88);
          if ( v20 == D(0x63edb0) )
            goto LABEL_43;
          F32(a2, 108) = D(0x63edb0);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
          return 1;
        case 4:
          F32(a2, 40) = W_speed_ramp((intptr_t)(F32(a2, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(-5000));
          v21 = W_speed_ramp((intptr_t)(F32(a2, 24)), (intptr_t)(4000), (intptr_t)(-1600), (intptr_t)(64000));
          (*v62__f) = i;
          F32(a2, 24) = v21;
          W_char_drift_move((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)((*v62__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          if ( F32(a2, 36) == 0 )
            goto LABEL_43;
          if ( errno_decorate == 0 )
          {
            for ( i = 8; i != 0; --i )
            {
              v22 = F32(a2, 12) + 1600;
              v23 = rnd();
              W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)a2), (intptr_t)(v22 * v23 / 0x8000 - v22 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
            }
          }
          v24 = 500 - 1000 * rnd() / 0x8000;
          i = F32(c, 0x40);
          if ( errno_decorate == 0 )
          {
            v25 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1396)), (intptr_t)(0));
            if ( v25 != 0 )
            {
              v26 = F32(v25, 0);
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)i), (intptr_t)((int *)&a7), (intptr_t)(&a6)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v26, (int)a7);
                g_eng.dsb_pan_5bb880(v26, a6);
                g_eng.dsb_freq_5bb890(v26, v24);
                g_eng.dsb_play_5bb8c0(v26);
              }
            }
          }
          v27 = F32(a2, 108);
          ++F16(a2, 88);
          if ( v27 == D(0x63f0e0) )
            goto LABEL_43;
          F32(a2, 108) = D(0x63f0e0);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
          return 1;
        case 5:
          if ( F8(a2, 116) != 0 )
          {
            W_room_hitstop_set((intptr_t)(0), (intptr_t)(0));
            ++F16(a2, 88);
          }
          goto LABEL_42;
        case 6:
LABEL_42:
          F32(a2, 40) = W_speed_ramp((intptr_t)(F32(a2, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(0));
          v28 = W_speed_ramp((intptr_t)(F32(a2, 24)), (intptr_t)(4000), (intptr_t)(-1600), (intptr_t)(64000));
          v29 = i;
          F32(a2, 24) = v28;
          W_char_drift_move((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          goto LABEL_43;
        default:
LABEL_43:
          result = 1;
          break;
      }
      return result;
    default:
      return 0;
  }
}

/* 0x4484b0: int __thiscall sub_4484B0(cls_4022A0 *this, int a2, int errno_decorate, int a4, int a5, int a6, int a7, int a8) */
int32_t ih_4484b0(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    int32_t * v8 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    int32_t * v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    int32_t * v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    int16_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1c */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1c */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x18 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x18 */

  v8 = (_DWORD *)a4;
  if ( W_skill_cast((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)(a4), (intptr_t)(a6), (intptr_t)(a5), (intptr_t)((_DWORD *)a8), (intptr_t)(D(0x66ac08)), (intptr_t)(210)) != 0
    && errno_decorate == 0
    && F16(a2, 88) == 1
    && F32(c, 0x15b54) != 0
    && F32(c, 0x15b5c) == 0
    && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
    && W_voice_place((intptr_t)(c), (intptr_t)(&a4), (intptr_t)(&a6), (intptr_t)(&a8)) != 0 )
  {
    W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0xAu), (intptr_t)(1), (intptr_t)(a4), (intptr_t)(a6), (intptr_t)(a8), (intptr_t)(0));
  }
  switch ( F16(a2, 72) )
  {
    case 0x12:
      *v8 = 3;
      v10 = F16(a2, 88);
      F8(a2, 124) = 1;
      F8(a2, 125) = 1;
      if ( v10 != 0 )
      {
        v11 = v10 - 1;
        if ( v11 != 0 )
        {
          if ( v11 == 1 )
          {
            v12 = F32(a2, 92) + 1;
            F32(a2, 92) = v12;
            if ( v12 < 40 )
            {
              v13 = W_tint_fade_a((intptr_t)(40 - v12), (intptr_t)(40));
              F32(c, 0xf4) = v13;
              F32(c, 0xf8) = v13;
              return 1;
            }
            if ( F32(c, 0x15904) == 0 )
            {
              W_body_action_reset((intptr_t)(a2));
              W_body_action_start((intptr_t)(a2), (intptr_t)(122), (intptr_t)(800));
              return 1;
            }
            if ( errno_decorate == 0 )
            {
              W_char_release((intptr_t)(c));
              return 1;
            }
            F8(a2, 0) = 0;
          }
          return 1;
        }
      }
      else
      {
        if ( F32(a2, 108) != D(0x66a5a8) )
        {
          F32(a2, 108) = D(0x66a5a8);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        W_entity_fx_add((intptr_t)(c), (intptr_t)(100155), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(0));
        ++F16(a2, 88);
      }
      v15 = F32(a2, 92) + 1;
      F32(a2, 92) = v15;
      if ( v15 > 200 )
      {
        ++F16(a2, 88);
        F32(a2, 92) = 0;
        return 1;
      }
      return 1;
    case 0x56:
      v33 = a5;
      *v8 = 2;
      if ( W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)((int)D(0x66b718)), (intptr_t)(1600), (intptr_t)(-1600), (intptr_t)(0), (intptr_t)(v33)) != 0 && F16(a2, 88) == 3 )
      {
        W_voice_cue((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v34 = rnd();
        W_sfx_play_at((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(68), (intptr_t)(0), (intptr_t)(1000 * v34 / 0x8000 + 1000), (intptr_t)(0));
      }
      if ( F16(a2, 90) >= 2u || F16(a2, 88) != 3 )
        return 0;
      W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(2400), (intptr_t)(800), (intptr_t)(3200), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(errno_decorate), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(11));
      return 0;
    case 0x57:
      *v8 = 3;
      v35 = F16(a2, 88);
      F8(a2, 124) = 1;
      if ( v35 == 0 )
      {
        if ( F16(a2, 84) == 2 )
        {
          v43 = -F32(a2, 40);
          F32(a2, 44) = F32(a2, 44) != 3 ? 3 : 1;
          F32(a2, 40) = v43;
        }
        if ( F32(a2, 108) != D(0x66a8d8) )
        {
          F32(a2, 108) = D(0x66a8d8);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        v44 = rnd();
        W_sfx_play_at((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(134), (intptr_t)(0), (intptr_t)(1000 * v44 / 0x8000 + 1000), (intptr_t)(0));
        ++F16(a2, 88);
        F32(a2, 40) = -48000;
        F32(a2, 24) = -30000;
        W_voice_cue((intptr_t)(c), (intptr_t)(errno_decorate), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        return 0;
      }
      if ( v35 != 1 )
        return 0;
      v36 = F32(a2, 92) + 1;
      F32(a2, 92) = v36;
      if ( v36 < 20 && v36 % 5 == 0 )
        W_slash_trail((intptr_t)(c), (intptr_t)(a2), (intptr_t)(errno_decorate), (intptr_t)(10), (intptr_t)(10));
      v37 = F32(a2, 92);
      if ( v37 > 10 )
      {
        v38 = F32(a2, 40) + 2000;
        F32(a2, 40) = v38;
        if ( v38 <= 0 )
        {
          v39 = v38 + 2000;
          F32(a2, 40) = v38 + 2000;
        }
        else
        {
          v39 = 0;
        }
        v40 = F32(a2, 24);
        F32(a2, 40) = v39;
        v41 = v40 + 1500;
        F32(a2, 24) = v41;
        if ( v41 <= 0 )
        {
          v42 = v41 + 1500;
          F32(a2, 24) = v41 + 1500;
        }
        else
        {
          v42 = 0;
        }
        F32(a2, 24) = v42;
      }
      if ( v37 <= 20 )
        return 0;
      W_body_action_reset((intptr_t)(a2));
      return 0;
    case 0x58:
      *v8 = 3;
      v16 = F16(a2, 88);
      if ( v16 != 0 )
      {
        if ( F8(a2, 116) == 0 )
          return 0;
        if ( v16 < 4u )
          W_body_set_clip((intptr_t)(a2), (intptr_t)(404 * F16(a2, 88) + 6733160));
        v17 = ++F16(a2, 88);
      }
      else
      {
        W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x66bd68)));
        v17 = ++F16(a2, 88);
      }
      v18 = v17 - 2;
      if ( v18 != 0 )
      {
        if ( v18 != 3 )
          return 0;
        F16(a2, 72) = 0;
        F16(a2, 86) = 210;
        F16(a2, 88) = 0;
        F32(a2, 92) = 0;
        F32(a2, 120) = 0;
        F16(a2, 84) = 0;
        F32(a2, 76) = 0;
        F32(a2, 80) = 0;
        F16(a2, 90) = 0;
        F16(a2, 100) = 0;
        return 0;
      }
      else
      {
        if ( errno_decorate == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)(&a8), (intptr_t)(&a5)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(a8), (intptr_t)(a5), (intptr_t)(0));
        }
        if ( F16(a2, 84) != 0 )
        {
          v19 = F32(a2, 96);
          v20 = F32(a2, 76);
          a5 = F32(a2, 44);
          v21 = 0;
          F8(a2, 127) = 1;
          a8 = v19;
          v22 = rnd();
          a6 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v20), (intptr_t)(1000), (intptr_t)(1000 * v22 / 0x8000));
          if ( v20[44] == 7 )
          {
            (*v49__f) = v20[180];
            v23 = rnd();
            v21 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v20[179]), (intptr_t)(1000 * v23 / 0x8000), (intptr_t)((*v49__f)));
          }
          (*v47__f) = F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228);
          (*v45__f) = F32(c, 0x1f0);
          v24 = W_char_mag_sum((intptr_t)(c));
          v25 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100031), (intptr_t)((_DWORD *)a2), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(1), (intptr_t)(errno_decorate), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(90), (intptr_t)(a8), (intptr_t)(a5), (intptr_t)(v20[44]), (intptr_t)(v20[45] + v24), (intptr_t)(v21), (intptr_t)((*v45__f)), (intptr_t)((*v47__f)), (intptr_t)(2));
          if ( v25 == 0 )
            return 0;
          F32(v25, 516) = 1000;
          return 0;
        }
        else
        {
          v26 = F32(a2, 96);
          v27 = F32(a2, 76);
          a5 = F32(a2, 44);
          v28 = 0;
          F8(a2, 127) = 1;
          a8 = v26;
          v29 = rnd();
          a6 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v27), (intptr_t)(1000), (intptr_t)(1000 * v29 / 0x8000));
          if ( v27[44] == 7 )
          {
            (*v50__f) = v27[180];
            v30 = rnd();
            v28 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v27[179]), (intptr_t)(1000 * v30 / 0x8000), (intptr_t)((*v50__f)));
          }
          (*v48__f) = F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228);
          (*v46__f) = F32(c, 0x1f0);
          v31 = W_char_mag_sum((intptr_t)(c));
          v32 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100031), (intptr_t)((_DWORD *)a2), (intptr_t)(2000), (intptr_t)(0), (intptr_t)(1), (intptr_t)(errno_decorate), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(90), (intptr_t)(a8), (intptr_t)(a5), (intptr_t)(v27[44]), (intptr_t)(v27[45] + v31), (intptr_t)(v28), (intptr_t)((*v46__f)), (intptr_t)((*v48__f)), (intptr_t)(2));
          if ( v32 == 0 )
            return 0;
          F32(v32, 516) = 700;
          return 0;
        }
      }
    default:
      return 0;
  }
}

/* 0x448dc0: char __thiscall sub_448DC0(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7, int a8) */
int32_t ih_448dc0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    int32_t * v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    int32_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1c */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    void * *const v45__f = (void * *)(frame_ + 0x1c);   /* ebp-0x4 */

  v8 = a3;
  v9 = a4;
  v10 = a2;
  (*v45__f) = D(0x6545e8);
  (*v44__f) = 0;
  switch ( F16(a2, 72) )
  {
    case 5:
      (*v42__f) = a2;
      *a4 = 3;
      W_body_set_clip((intptr_t)((*v42__f)), (intptr_t)((int)D(0x652fa8)));
      v22 = F32(v10, 92);
      F8(v10, 124) = 1;
      F8(v10, 125) = 1;
      F32(v10, 28) = 1;
      if ( v22 == 0 )
        W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(93), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v23 = F32(v10, 92);
      if ( v23 < 40 )
      {
        F32(v10, 92) = v23 + 1;
        W_shot_4497b0((intptr_t)(c), (intptr_t)(v8), (intptr_t)(v23 + 1), (intptr_t)(40));
        W_shot_449700((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v10, 12)), (intptr_t)(F32(v10, 16)));
        break;
      }
      goto LABEL_24;
    case 6:
      (*v41__f) = a2;
      *a4 = 3;
      W_body_set_clip((intptr_t)((*v41__f)), (intptr_t)((int)D(0x652fa8)));
      v19 = F32(v10, 92);
      F8(v10, 124) = 1;
      F8(v10, 125) = 1;
      if ( v19 == 0 )
        W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(94), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v20 = F32(v10, 92);
      if ( v20 >= 40 )
      {
        W_body_action_reset((intptr_t)(v10));
        W_ih_4466b0((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)(0));
      }
      else
      {
        v21 = v20 + 1;
        F32(v10, 92) = v21;
        W_shot_4497b0((intptr_t)(c), (intptr_t)(v8), (intptr_t)(40 - v21), (intptr_t)(40));
        W_shot_449700((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v10, 12)), (intptr_t)(F32(v10, 16)));
      }
      break;
    case 7:
      (*v40__f) = a2;
      *a4 = 3;
      W_body_set_clip((intptr_t)((*v40__f)), (intptr_t)((int)D(0x652fa8)));
      SETHI(v12, 0);
      F8(v10, 124) = 1;
      SETLO(v12, F16(v10, 88));
      F8(v10, 125) = 1;
      if ( (uint16_t)v12 == 0 )
      {
        v14 = F32(v10, 92);
        if ( v14 >= 20 )
        {
          if ( !ih_467330(c) )
          {
            v17 = F32(c, 0x15b60);
            if ( v17 != 0 )
            {
              ih_45e260(v17, &a2, &a3, (int)c);
              v18 = a3;
              F32(v10, 4) = a2;
              F32(v10, 8) = v18;
            }
          }
          W_shot_4497b0((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(1));
          ++F16(v10, 88);
          F32(v10, 92) = 0;
          F32(v10, 24) = 0;
          F32(v10, 40) = 0;
        }
        else
        {
          v15 = v14 + 1;
          F32(v10, 92) = v15;
          if ( v8 == 0 )
          {
            v16 = W_tint_fade_a((intptr_t)(20 - v15), (intptr_t)(20));
            F32(c, 0xf4) = v16;
            F32(c, 0xf8) = v16;
          }
        }
        break;
      }
      if ( v12 == 1 )
      {
        v13 = F32(v10, 92);
        if ( v13 < 40 )
        {
          F32(v10, 92) = v13 + 1;
          W_shot_4497b0((intptr_t)(c), (intptr_t)(v8), (intptr_t)(v13 + 1), (intptr_t)(40));
          break;
        }
LABEL_24:
        W_body_action_reset((intptr_t)(v10));
      }
      break;
    default:
      break;
  }
  SETLO(v24, F16(v10, 72) - 2);
  switch ( F16(v10, 72) )
  {
    case 2:
      SETHI(v25, 0);
      *v9 = 2;
      SETLO(v25, F16(v10, 88));
      if ( (uint16_t)v25 == 0 )
      {
        SETLB(v24, W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x653ad0))));
        ++F16(v10, 88);
LABEL_32:
        if ( F16(v10, 114) == 1 )
        {
          v26 = F32(v10, 44);
          F32(v10, 40) = 0;
          v24 = v26 != 3 ? 3 : 1;
          ++F16(v10, 88);
          F32(v10, 44) = v24;
        }
        return v24;
      }
      v24 = v25 - 1;
      if ( v24 == 0 )
        goto LABEL_32;
      if ( --v24 == 0 )
      {
        v24 = F8(v10, 116);
        if ( (uint8_t)v24 != 0 )
LABEL_108:
          SETLB(v24, W_body_action_reset((intptr_t)(v10)));
      }
      return v24;
    case 0x42:
      *v9 = 2;
      v34 = a5;
      if ( F16(v10, 88) == 0 )
        W_body_pose_swap((intptr_t)(c), (intptr_t)(v10), (intptr_t)(a5), (intptr_t)(0x42u), (intptr_t)(0), (intptr_t)(0x42u), (intptr_t)(6), (intptr_t)(1));
      v24 = F16(v10, 84);
      if ( (uint16_t)v24 == 0 )
        goto LABEL_83;
      if ( v24 == 6 )
      {
        (*v44__f) = 1;
        (*v45__f) = D(0x653f98);
LABEL_83:
        v24 = W_slash_step((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)((int)(*v45__f)), (intptr_t)(2000), (intptr_t)(-2000), (intptr_t)((*v44__f)), (intptr_t)(v34));
        if ( v24 != 0 )
        {
          if ( F16(v10, 88) == 1 )
          {
            SETLB(v24, W_voice_cue((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0)));
          }
          else
          {
            SETLO(v24, F16(v10, 88) - 3);
            if ( F16(v10, 88) == 3 )
            {
              W_voice_cue((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0x45u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
              SETLB(v24, W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(69), (intptr_t)(-500), (intptr_t)(0), (intptr_t)(0)));
            }
          }
        }
        if ( F16(v10, 90) < 2u && F16(v10, 88) == 3 )
          SETLB(v24, W_char_attack_hits((intptr_t)(c), (intptr_t)(v10), (intptr_t)(4000), (intptr_t)(800), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v8), (intptr_t)(F32(v10, 76)), (intptr_t)(1000), (intptr_t)(3)));
      }
      return v24;
    case 0x43:
      v27 = a4;
      F8(v10, 124) = 1;
      v28 = 0;
      *v27 = 3;
      SETLO(v24, F16(v10, 88));
      ++F32(v10, 92);
      switch ( (int16_t)v24 )
      {
        case 0:
          W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x654c38)));
          v28 = 1;
          F16(v10, 88) = 1;
          goto LABEL_36;
        case 1:
LABEL_36:
          SETLB(v24, F8(v10, 116));
          if ( (uint8_t)v24 == 0 )
            goto LABEL_48;
          W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x654dcc)));
          F16(v10, 88) = 2;
          F32(v10, 24) = -40000;
          F32(v10, 40) = 56000;
          F32(v10, 92) = 0;
          goto LABEL_49;
        case 2:
          if ( F32(v10, 24) > 0 )
          {
            W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x654f60)));
            F16(v10, 88) = 3;
            v28 = 1;
          }
          goto LABEL_40;
        case 3:
LABEL_40:
          if ( F32(v10, 92) <= 16 )
          {
            v24 = W_char_drift((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)(a7), (intptr_t)(56000), (intptr_t)(4000), (intptr_t)(-1000), (intptr_t)(40000), (intptr_t)(3000), (intptr_t)(-1000), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
            if ( v24 != 0 )
            {
LABEL_48:
              if ( v28 == 0 )
                goto LABEL_70;
            }
            else
            {
              F16(v10, 88) = 4;
LABEL_44:
              F32(v10, 40) = -32000;
              F32(v10, 24) = -40000;
              W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x655288)));
              F16(v10, 88) = 6;
            }
          }
          else
          {
            F16(v10, 88) = 5;
            W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x6550f4)));
          }
LABEL_49:
          SETLO(v24, F16(v10, 88) - 1);
          if ( F16(v10, 88) == 1 )
          {
            if ( v8 == 0 )
            {
              v24 = F32(c, 0x15b54);
              if ( v24 != 0 )
              {
                v24 = F32(c, 0x15b5c);
                if ( v24 == 0
                  && (F32(c, 0x15a8c) != 1 || (v24 = F32(CFG(), 400)) != 0) )
                {
                  v24 = W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)((int *)&a4), (intptr_t)(&a5));
                  if ( v24 != 0 )
                    SETLB(v24, W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x46u), (intptr_t)(1), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(a5), (intptr_t)(0)));
                }
              }
            }
          }
          else
          {
            SETLO(v24, F16(v10, 88) - 2);
            if ( F16(v10, 88) == 2 )
            {
              if ( v8 == 0
                && F32(c, 0x15b54) != 0
                && F32(c, 0x15b5c) == 0
                && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                && W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)((int *)&a4), (intptr_t)(&a5)) != 0 )
              {
                W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x47u), (intptr_t)(1), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(a5), (intptr_t)(0));
              }
              v29 = 1000 * rnd() / 0x8000;
              a4 = (_DWORD *)F32(c, 0x40);
              if ( v8 == 0 )
              {
                v30 = (int *)W_vpool_take((intptr_t)(SYS32(1404)), (intptr_t)(0));
                if ( v30 != 0 )
                {
                  a5 = *v30;
                  if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(&a2), (intptr_t)(&a3)) != 0 )
                  {
                    g_eng.dsb_volume_5bb870(a5, a2);
                    g_eng.dsb_pan_5bb880(a5, a3);
                    (*v43__f) = v29;
                    v31 = a5;
                    g_eng.dsb_freq_5bb890(a5, (*v43__f));
                    g_eng.dsb_play_5bb8c0(v31);
                  }
                }
              }
              v32 = rnd();
              SETLB(v24, W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(72), (intptr_t)(-500), (intptr_t)(1000 * v32 / 0x8000 + 500), (intptr_t)(0)));
            }
          }
LABEL_70:
          if ( F16(v10, 88) == 3 && F16(v10, 90) == 0 )
          {
            v24 = F32(v10, 92);
            if ( v24 > 4 )
            {
              v24 = W_char_attack_hits((intptr_t)(c), (intptr_t)(v10), (intptr_t)(4000), (intptr_t)(-1600), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(0), (intptr_t)(v8), (intptr_t)(F32(v10, 76)), (intptr_t)(1000), (intptr_t)(5));
              if ( v24 != 0 )
              {
                v33 = rnd();
                SETLB(v24, W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(134), (intptr_t)(0), (intptr_t)(1000 * v33 / 0x8000), (intptr_t)(0)));
                F16(v10, 88) = 4;
              }
            }
          }
          if ( F32(v10, 92) % 4 == 0 && F16(v10, 88) >= 2u )
            SETLB(v24, W_slash_trail((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)(10), (intptr_t)(10)));
          break;
        case 4:
          goto LABEL_44;
        case 5:
        case 6:
          if ( F8(v10, 116) == 0 )
          {
            SETLB(v24, a5);
            if ( a5 == 0 )
              goto LABEL_70;
          }
          W_body_action_reset((intptr_t)(v10));
          goto LABEL_49;
        default:
          goto LABEL_70;
      }
      return v24;
    case 0x44:
      *v9 = 3;
      v24 = F16(v10, 88);
      F8(v10, 124) = 1;
      switch ( v24 )
      {
        case 0:
          if ( F16(v10, 84) == 2 )
          {
            v35 = -F32(v10, 40);
            F32(v10, 44) = F32(v10, 44) != 3 ? 3 : 1;
            F32(v10, 40) = v35;
          }
          W_char_lunge((intptr_t)(c), (intptr_t)(v10), (intptr_t)(-800), (intptr_t)(v8), (intptr_t)(0));
          goto LABEL_103;
        case 1:
          SETLB(v24, F8(v10, 116));
          if ( (uint8_t)v24 != 0 )
          {
            v36 = rnd();
            W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(134), (intptr_t)(0), (intptr_t)(1000 * v36 / 0x8000 + 1000), (intptr_t)(0));
            SETLB(v24, W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x653e00))));
            ++F16(v10, 88);
            F32(v10, 40) = -80000;
            F32(v10, 24) = -30000;
          }
          break;
        case 2:
          v37 = F32(v10, 92) + 1;
          F32(v10, 92) = v37;
          if ( v37 < 20 && v37 % 5 == 0 )
            W_slash_trail((intptr_t)(c), (intptr_t)(v10), (intptr_t)(v8), (intptr_t)(10), (intptr_t)(10));
          if ( F32(v10, 92) > 40 )
            goto LABEL_108;
          SETLB(v24, a5);
          if ( a5 != 0 )
          {
            F32(v10, 40) = 0;
            W_char_lunge((intptr_t)(c), (intptr_t)(v10), (intptr_t)(800), (intptr_t)(v8), (intptr_t)(0));
LABEL_103:
            SETLB(v24, W_body_set_clip((intptr_t)(v10), (intptr_t)((int)D(0x653c68))));
            ++F16(v10, 88);
          }
          break;
        case 3:
          v38 = F32(v10, 40) + 4000;
          F32(v10, 40) = v38;
          if ( v38 <= 0 )
          {
            v24 = v38 + 4000;
            F32(v10, 40) = v38 + 4000;
          }
          else
          {
            v24 = 0;
          }
          F32(v10, 40) = v24;
          SETLB(v24, F8(v10, 116));
          if ( (uint8_t)v24 != 0 )
            goto LABEL_108;
          break;
        default:
          return v24;
      }
      return v24;
    default:
      return v24;
  }
}

/* 0x449810: void __thiscall sub_449810(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7, _DWORD *a8) */
void ih_449810(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint32_t v22 = 0;
    intptr_t v23 = 0;
    uint16_t v24 = 0;
    uint16_t v25 = 0;
    int32_t * v26 = 0;
    int32_t * v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    int32_t * v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    int32_t * v39 = 0;
    int32_t * v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */

  switch ( F16(a2, 72) )
  {
    case 2:
      *a4 = 2;
      ih_44a160(a2, (int)D(0x655f48), 1, 1);
      return;
    case 0x12:
      v10 = F32(c, 0x1594c);
      if ( v10 <= 0 )
        goto LABEL_9;
      if ( v10 <= 2 )
        goto LABEL_10;
      if ( v10 == 3 )
      {
        a8 = F32(a2, 8);
        SETHI(v11, HIWORD(a8));
        SETLO(v11, F8(a2, 32));
        v12 = W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)a2), (intptr_t)(1), (intptr_t)(v11), (intptr_t)(a7), (intptr_t)((int)&a8), (intptr_t)((_DWORD *)(a2 + 644))) == 0;
        if ( F32(a2, 644) != 0 )
          F32(a2, 8) = a8;
        if ( v12 )
        {
          F32(a2, 40) = 0;
          F32(a2, 24) = 0;
          goto LABEL_13;
        }
      }
      else
      {
LABEL_9:
        if ( F32(c, 0x15950) > 56000 )
          goto LABEL_13;
      }
LABEL_10:
      v13 = F32(c, 0x15950);
      if ( v13 <= 64000 )
        v13 = 64000;
      F32(a2, 28) = v13;
LABEL_13:
      *a4 = 3;
      v14 = F16(a2, 88);
      F8(a2, 124) = 1;
      F8(a2, 125) = 1;
      if ( v14 != 0 )
      {
        v15 = v14 - 1;
        if ( v15 != 0 )
        {
          if ( v15 == 1 )
          {
            v16 = F32(a2, 92) + 1;
            F32(a2, 92) = v16;
            if ( v16 >= 40 )
            {
              if ( F32(c, 0x15904) != 0 )
              {
                if ( a3 != 0 )
                  F8(a2, 0) = 0;
                else
                  W_char_release((intptr_t)(c));
              }
              else
              {
                W_body_action_reset((intptr_t)(a2));
                W_body_action_start((intptr_t)(a2), (intptr_t)(122), (intptr_t)(800));
              }
            }
            else
            {
              v17 = W_tint_fade_a((intptr_t)(40 - v16), (intptr_t)(40));
              F32(c, 0xf4) = v17;
              F32(c, 0xf8) = v17;
            }
          }
          return;
        }
      }
      else
      {
        if ( F32(a2, 108) != D(0x655c18) )
        {
          F32(a2, 108) = D(0x655c18);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        W_entity_fx_add((intptr_t)(c), (intptr_t)(100155), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(0));
        ++F16(a2, 88);
      }
      v18 = F32(a2, 92) + 1;
      F32(a2, 92) = v18;
      if ( v18 > 200 )
      {
        ++F16(a2, 88);
        F32(a2, 92) = 0;
      }
      return;
    case 0x45:
      *a4 = 2;
      if ( W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x656278)), (intptr_t)(1200), (intptr_t)(-1200), (intptr_t)(0), (intptr_t)(a5)) != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          W_voice_cue((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
        }
        else if ( F16(a2, 88) == 3 )
        {
          W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(68), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        }
      }
      if ( F16(a2, 90) == 0 && F16(a2, 88) == 3 )
        W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(7200), (intptr_t)(1600), (intptr_t)(6000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(5));
      return;
    case 0x46:
      *a4 = 2;
      if ( F16(a2, 84) == 1 )
      {
        if ( W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x657568)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5)) != 0 )
        {
          if ( F16(a2, 88) == 1 )
          {
            W_voice_cue((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
          }
          else if ( F16(a2, 88) == 3 )
          {
            W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(72), (intptr_t)(-500), (intptr_t)(0), (intptr_t)(0));
          }
        }
        if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
          W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(7200), (intptr_t)(-11200), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(5));
      }
      else if ( F16(a2, 84) == 4 )
      {
        v34 = 0;
        switch ( F16(a2, 88) )
        {
          case 0:
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x656f18)));
            v34 = 1;
            F16(a2, 88) = 1;
            goto LABEL_79;
          case 1:
LABEL_79:
            if ( F8(a2, 116) != 0 )
              goto LABEL_80;
            goto LABEL_86;
          case 2:
LABEL_80:
            if ( F8(a2, 116) != 0 )
            {
              W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x657240)));
              W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
              F16(a2, 88) = 3;
            }
            else
            {
LABEL_86:
              if ( v34 == 0 )
                goto LABEL_93;
            }
LABEL_87:
            switch ( F16(a2, 88) )
            {
              case 1:
                W_voice_cue((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
                break;
              case 3:
                W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(72), (intptr_t)(-500), (intptr_t)(0), (intptr_t)(0));
                break;
              case 4:
                v35 = -F32(a2, 40);
                F32(a2, 44) = F32(a2, 44) != 3 ? 3 : 1;
                F32(a2, 40) = v35;
                break;
              default:
                break;
            }
LABEL_93:
            if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
              W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(-12000), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(700), (intptr_t)(5));
            break;
          case 3:
            if ( F8(a2, 116) == 0 )
              goto LABEL_93;
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x6573d4)));
            W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
            F16(a2, 88) = 4;
            goto LABEL_87;
          case 4:
            if ( F8(a2, 116) == 0 )
              goto LABEL_93;
            W_body_action_reset((intptr_t)(a2));
            F16(a2, 88) = 5;
            goto LABEL_87;
          default:
            goto LABEL_93;
        }
      }
      else
      {
        if ( W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x6568c8)), (intptr_t)(2800), (intptr_t)(-2800), (intptr_t)(0), (intptr_t)(a5)) != 0 )
        {
          if ( F16(a2, 88) == 1 )
          {
            W_voice_cue((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
          }
          else if ( F16(a2, 88) == 3 )
          {
            W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(72), (intptr_t)(-500), (intptr_t)(0), (intptr_t)(0));
          }
        }
        if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
          W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(7200), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(5));
      }
      return;
    case 0x47:
      v19 = 0;
      *a4 = 3;
      v20 = F32(a2, 76);
      F16(a2, 86) = 230;
      switch ( F16(a2, 88) )
      {
        case 0:
          if ( W_skill_mp_pay((intptr_t)(c), (intptr_t)(v20), (intptr_t)(a3), (intptr_t)(0)) != 0 )
          {
            F16(a2, 88) = 1;
            goto LABEL_32;
          }
          W_body_action_reset((intptr_t)(a2));
          return;
        case 1:
LABEL_32:
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x657d4c)));
          F16(a2, 88) = 2;
          goto LABEL_33;
        case 2:
          if ( F16(a2, 102) >= 0x64u )
          {
            F16(a2, 88) = 3;
            v19 = 1;
          }
          goto LABEL_41;
        case 3:
LABEL_41:
          if ( F16(a2, 102) >= 0x3E8u )
          {
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x657ee0)));
            if ( F8(a2, 104) == 0 )
              *a8 = 0;
          }
          else if ( F8(a2, 104) == 0 )
          {
            v22 = F16(v20, 728) + F16(a2, 102);
            if ( v22 > 0x3E8 )
              SETLO(v22, 1000);
            F16(a2, 102) = v22;
            *a8 = 0;
          }
          goto LABEL_34;
        case 4:
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x658074)));
          F16(a2, 88) = 5;
          F8(a2, 104) = 1;
          v19 = 1;
          goto LABEL_49;
        case 5:
LABEL_49:
          if ( F8(a2, 116) != 0 )
          {
            if ( W_skill_mp_pay((intptr_t)(c), (intptr_t)(v20), (intptr_t)(a3), (intptr_t)(1)) == 0 )
              goto LABEL_38;
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x658208)));
            v19 = 1;
            F16(a2, 88) = 6;
          }
LABEL_34:
          if ( F16(a2, 88) >= 4u )
            goto LABEL_57;
          v21 = F32(a6, 16);
          if ( v21 == 15 )
            goto LABEL_38;
          if ( v21 == 14 )
            goto LABEL_57;
          if ( F16(a2, 102) >= 0x3E8u )
          {
            F16(a2, 88) = 4;
            v19 = 1;
LABEL_57:
            v24 = F16(a2, 88);
            if ( v24 >= 4u && v24 <= 6u )
              *a8 = 0;
          }
          else
          {
LABEL_38:
            W_body_action_reset((intptr_t)(a2));
          }
          if ( v19 != 0 )
          {
LABEL_61:
            if ( F16(a2, 88) == 6 )
            {
              v25 = 0;
              a8 = 0;
              do
              {
                v26 = F32(a2, 96);
                v27 = F32(a2, 76);
                v41 = F32(a2, 44);
                v40 = 0;
                F8(a2, 127) = 1;
                v39 = v26;
                v28 = rnd();
                v42 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v27), (intptr_t)(1000), (intptr_t)(1000 * v28 / 0x8000));
                if ( v27[44] == 7 )
                {
                  (*v38__f) = v27[180];
                  v29 = rnd();
                  v40 = (_DWORD *)W_char_mag_roll((intptr_t)(c), (intptr_t)(v27[179]), (intptr_t)(1000 * v29 / 0x8000), (intptr_t)((*v38__f)));
                }
                (*v37__f) = F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228);
                (*v36__f) = F32(c, 0x1f0);
                v30 = W_char_mag_sum((intptr_t)(c));
                v31 = v27[45];
                v32 = v27[44];
                v33 = a8;
                W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100027), (intptr_t)((_DWORD *)a2), (intptr_t)(8000), (intptr_t)(-12000), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(v25 + 10), (intptr_t)(v42), (intptr_t)((int)a8), (intptr_t)((int)v39), (intptr_t)(v41), (intptr_t)(v32), (intptr_t)(v31 + v30), (intptr_t)((int)v40), (intptr_t)((*v36__f)), (intptr_t)((*v37__f)), (intptr_t)(2));
                ++v25;
                a8 = (_DWORD *)((uint8_t *)v33 + 30);
              }
              while ( v25 < 5u );
            }
          }
          break;
        case 6:
          v23 = F32(a2, 92) + 1;
          F32(a2, 92) = v23;
          if ( v23 > 20 )
          {
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x65839c)));
            F16(a2, 88) = 7;
LABEL_33:
            v19 = 1;
          }
          goto LABEL_34;
        case 7:
          if ( F8(a2, 116) == 0 )
            goto LABEL_34;
          W_body_action_reset((intptr_t)(a2));
          goto LABEL_61;
        default:
          goto LABEL_34;
      }
      return;
    default:
      return;
  }
}

/* 0x44a160: int __stdcall sub_44A160(int a1, int a2, __int16 a3, int a4) */
int32_t ih_44a160(int32_t a1, int32_t a2, int16_t a3, int32_t a4)
{

    intptr_t result = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;

  result = a1;
  v5 = 0;
  if ( F16(a1, 88) != 0 )
  {
    if ( F16(a1, 88) != 1 )
    {
      if ( F16(a1, 88) == 2 && F8(a1, 116) != 0 )
      {
        F16(a1, 72) = 0;
        F16(a1, 86) = 210;
        F16(a1, 88) = 0;
        F32(a1, 92) = 0;
        F32(a1, 120) = 0;
        F16(a1, 84) = 0;
        F32(a1, 76) = 0;
        F32(a1, 80) = 0;
        F16(a1, 90) = 0;
        F16(a1, 100) = 0;
      }
      return result;
    }
  }
  else
  {
    if ( F32(a1, 108) != a2 )
    {
      F32(a1, 108) = a2;
      F8(a1, 116) = 0;
      F16(a1, 114) = 0;
      F16(a1, 112) = 0;
    }
    F16(a1, 88) = 1;
  }
  if ( F16(a1, 114) == a3 )
  {
    v6 = F32(a1, 44) != 3 ? 3 : 1;
    ++F16(a1, 88);
    F32(a1, 44) = v6;
    if ( a4 == 0 )
      v5 = -F32(a1, 40);
    F32(a1, 40) = v5;
  }
  return result;
}

/* 0x44a200: __int16 __thiscall sub_44A200(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7) */
int32_t ih_44a200(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    int16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t * v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    int32_t * v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    int32_t * v35 = 0;
    int32_t * v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x20 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1c */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x8 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x4 */

  if ( F16(a2, 72) == 61 )
  {
    *a4 = 2;
    if ( F16(a2, 88) == 0 )
    {
      v24 = a5;
      if ( F32(a2, 652) != 0 )
        v24 = 1;
      if ( F16(c, 0x158bc) < F16(c, 0xc7e8) )
        v24 = 1;
      if ( F16(a2, 72) == 61 )
      {
        if ( v24 != 0 )
        {
          if ( F16(a2, 84) == 6 && v24 == 1 )
          {
            W_body_action_reset((intptr_t)(a2));
            v25 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x3Du));
            W_body_pose_set((intptr_t)(a2), (intptr_t)(61), (intptr_t)(F16(a2, 86)), (intptr_t)(0), (intptr_t)(F32(a2, 80)), (intptr_t)(v25));
          }
        }
        else
        {
          W_body_action_reset((intptr_t)(a2));
          (*v32__f) = W_char_skill_find((intptr_t)(c), (intptr_t)(0x3Du));
          W_body_pose_set((intptr_t)(a2), (intptr_t)(61), (intptr_t)(F16(a2, 86)), (intptr_t)(6), (intptr_t)(F32(a2, 80)), (intptr_t)((*v32__f)));
        }
      }
    }
    v9 = F16(a2, 84);
    if ( (uint16_t)v9 != 0 )
    {
      if ( v9 == 6 )
      {
        v9 = W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x64f848)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a5));
        if ( v9 != 0 )
        {
          SETLO(v9, F16(a2, 88) - 2);
          if ( F16(a2, 88) == 2 )
          {
            v26 = rnd();
            SETLO(v9, W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(72), (intptr_t)(0), (intptr_t)(1000 * v26 / 0x8000 - 1000), (intptr_t)(0)));
          }
        }
        if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
          SETLO(v9, W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(3000), (intptr_t)(1600), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1200), (intptr_t)(6)));
      }
    }
    else
    {
      v9 = W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x64f1f8)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5));
      if ( v9 != 0 )
      {
        SETLO(v9, F16(a2, 88) - 1);
        if ( F16(a2, 88) == 1 )
        {
          F32(a2, 24) = -60000;
        }
        else
        {
          SETLO(v9, F16(a2, 88) - 2);
          if ( F16(a2, 88) == 2 )
          {
            v27 = rnd();
            SETLO(v9, W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(72), (intptr_t)(0), (intptr_t)(1000 * v27 / 0x8000 - 1000), (intptr_t)(0)));
          }
        }
      }
      if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
        SETLO(v9, W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(3000), (intptr_t)(800), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(6)));
    }
  }
  else
  {
    SETLO(v9, F16(a2, 72) - 62);
    if ( F16(a2, 72) == 62 )
    {
      v10 = 0;
      *a4 = 2;
      if ( F16(a2, 88) == 0 )
      {
        v11 = F16(a2, 72);
        if ( v11 == 62 && F16(a2, 84) == 8 && a5 == 0 )
        {
          W_body_action_reset((intptr_t)(a2));
          v12 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x3Eu));
          W_body_pose_set((intptr_t)(a2), (intptr_t)(62), (intptr_t)(F16(a2, 86)), (intptr_t)(7), (intptr_t)(F32(a2, 80)), (intptr_t)(v12));
        }
        else if ( v11 == 62 && F16(a2, 84) == 7 && a5 == 1 )
        {
          W_body_action_reset((intptr_t)(a2));
          v13 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x3Eu));
          W_body_pose_set((intptr_t)(a2), (intptr_t)(62), (intptr_t)(F16(a2, 86)), (intptr_t)(8), (intptr_t)(F32(a2, 80)), (intptr_t)(v13));
        }
      }
      if ( F16(a2, 84) == 1 )
      {
        v37 = 45;
        v38 = 8;
      }
      else
      {
        if ( F16(a2, 84) == 7 )
          v37 = 120;
        else
          v37 = 90;
        v38 = 20;
      }
      SETLO(v9, F16(a2, 88));
      if ( (uint16_t)v9 != 0 )
      {
        if ( F8(a2, 116) == 0 )
          return v9;
        if ( (uint16_t)v9 < 3u )
          W_body_set_clip((intptr_t)(a2), (intptr_t)(404 * F16(a2, 88) + 6618776));
        v14 = ++F16(a2, 88);
      }
      else
      {
        if ( F32(a2, 108) != D(0x64fe98) )
        {
          F32(a2, 108) = D(0x64fe98);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        v14 = 1;
        F16(a2, 88) = 1;
      }
      v15 = v14 - 2;
      if ( v15 != 0 )
      {
        v9 = v15 - 2;
        if ( v9 == 0 )
        {
          F16(a2, 72) = 0;
          F16(a2, 86) = 210;
          F16(a2, 88) = 0;
          F32(a2, 92) = 0;
          F32(a2, 120) = 0;
          F16(a2, 84) = 0;
          F32(a2, 76) = 0;
          F32(a2, 80) = 0;
          F16(a2, 90) = 0;
          F16(a2, 100) = 0;
        }
      }
      else
      {
        v16 = 1000 * rnd();
        v35 = (_DWORD *)F32(c, 0x40);
        if ( a3 == 0 )
        {
          v17 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1476)), (intptr_t)(0));
          if ( v17 != 0 )
          {
            v18 = F32(v17, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) - 800), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v35), (intptr_t)(&(*v33__f)), (intptr_t)(&(*v34__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v18, (*v33__f));
              g_eng.dsb_pan_5bb880(v18, (*v34__f));
              g_eng.dsb_freq_5bb890(v18, v16 / 0x8000 - 500);
              g_eng.dsb_play_5bb8c0(v18);
            }
            v10 = 0;
          }
        }
        v19 = F32(a2, 96);
        v20 = F32(a2, 76);
        v36 = F32(a2, 44);
        F8(a2, 127) = 1;
        (*v34__f) = v19;
        v21 = rnd();
        (*v33__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)(v20), (intptr_t)(1000), (intptr_t)(1000 * v21 / 0x8000));
        if ( v20[44] == 7 )
        {
          (*v31__f) = v20[180];
          v22 = rnd();
          v10 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v20[179]), (intptr_t)(1000 * v22 / 0x8000), (intptr_t)((*v31__f)));
        }
        (*v30__f) = F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228);
        (*v29__f) = F32(c, 0x1f0);
        v23 = W_char_mag_sum((intptr_t)(c));
        v9 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100030), (intptr_t)((_DWORD *)a2), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(10), (intptr_t)((*v33__f)), (intptr_t)(v37), (intptr_t)((*v34__f)), (intptr_t)((int)v36), (intptr_t)(v20[44]), (intptr_t)(v20[45] + v23), (intptr_t)(v10), (intptr_t)((*v29__f)), (intptr_t)((*v30__f)), (intptr_t)(2));
        if ( v9 != 0 )
          F32(v9, 516) = v38;
      }
    }
  }
  return v9;
}

/* 0x44a740: __int16 __thiscall sub_44A740(cls_4022A0 *this, int a2, int a3, _DWORD *a4, _DWORD *a5, int a6, int a7) */
int32_t ih_44a740(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    void * v11 = 0;
    intptr_t v12 = 0;
    int16_t v13 = 0;
    char v14 = 0;
    intptr_t i = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    intptr_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
    intptr_t v23 = 0;
    intptr_t * v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    int32_t * v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    int32_t * v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    int32_t * *const v57__f = (int32_t * *)(frame_ + 0x18);   /* ebp-0x20 */
    int32_t * *const v56__f = (int32_t * *)(frame_ + 0x18);   /* ebp-0x20 */
    int32_t * *const v55__f = (int32_t * *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  v7 = a2;
  v9 = 0;
  SETLO(v10, F16(a2, 72) - 57);
  (*v62__f) = 1000;
  v11 = D(0x651018);
  switch ( F16(a2, 72) )
  {
    case '9':
      if ( F16(a2, 88) == 0 )
        W_body_pose_swap((intptr_t)(c), (intptr_t)(a2), (intptr_t)((int)a5), (intptr_t)(0x39u), (intptr_t)(0), (intptr_t)(0x39u), (intptr_t)(6), (intptr_t)(1));
      v10 = F16(v7, 84);
      if ( (uint16_t)v10 == 0 )
        goto LABEL_67;
      if ( v10 == 6 )
      {
        v9 = 1;
        v11 = D(0x651668);
        (*v62__f) = 1200;
LABEL_67:
        (*v57__f) = a5;
        (*v54__f) = v9;
        v46 = a3;
        (*v53__f) = a3;
        *a4 = 2;
        v10 = W_slash_step((intptr_t)(c), (intptr_t)(v7), (intptr_t)((*v53__f)), (intptr_t)((int)v11), (intptr_t)(1600), (intptr_t)(-1600), (intptr_t)((*v54__f)), (intptr_t)((int)(*v57__f)));
        if ( v10 != 0 )
        {
          SETLO(v10, F16(v7, 88) - 2);
          if ( F16(v7, 88) == 2 )
          {
            v47 = rnd();
            SETLO(v10, W_sfx_play_at((intptr_t)(c), (intptr_t)(v46), (intptr_t)(68), (intptr_t)(0), (intptr_t)(1000 * v47 / 0x8000 - 1000), (intptr_t)(0)));
          }
        }
        if ( F16(v7, 90) < 2u )
        {
          if ( F16(v7, 88) == 2 )
          {
            SETLO(v10, W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(2400), (intptr_t)(-2400), (intptr_t)(2000), (intptr_t)(3000), (intptr_t)(0), (intptr_t)(v46), (intptr_t)(F32(v7, 76)), (intptr_t)((*v62__f)), (intptr_t)(2)));
          }
          else
          {
            SETLO(v10, F16(v7, 88) - 3);
            if ( F16(v7, 88) == 3 )
              SETLO(v10, W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(4600), (intptr_t)(800), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v46), (intptr_t)(F32(v7, 76)), (intptr_t)((*v62__f)), (intptr_t)(2)));
          }
        }
      }
      return v10;
    case ':':
      v44 = a3;
      (*v56__f) = a5;
      (*v52__f) = a3;
      (*v50__f) = a2;
      *a4 = 2;
      v10 = W_slash_step((intptr_t)(c), (intptr_t)((*v50__f)), (intptr_t)((*v52__f)), (intptr_t)((int)D(0x651cb8)), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(0), (intptr_t)((int)(*v56__f)));
      if ( v10 != 0 )
      {
        if ( F16(v7, 88) == 2 )
        {
          v45 = rnd();
          SETLO(v10, W_sfx_play_at((intptr_t)(c), (intptr_t)(v44), (intptr_t)(68), (intptr_t)(0), (intptr_t)(1000 * v45 / 0x8000 - 2500), (intptr_t)(0)));
        }
        else
        {
          SETLO(v10, F16(v7, 88) - 4);
          if ( F16(v7, 88) == 4 )
            SETLO(v10, W_char_lunge((intptr_t)(c), (intptr_t)(v7), (intptr_t)(2000), (intptr_t)(v44), (intptr_t)(1)));
        }
      }
      if ( F16(v7, 90) < 2u && F16(v7, 88) == 3 )
        SETLO(v10, W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(6000), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v44), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(2)));
      return v10;
    case ';':
      v42 = a3;
      (*v55__f) = a5;
      (*v51__f) = a3;
      (*v49__f) = a2;
      *a4 = 2;
      v10 = W_slash_step((intptr_t)(c), (intptr_t)((*v49__f)), (intptr_t)((*v51__f)), (intptr_t)((int)D(0x652308)), (intptr_t)(800), (intptr_t)(-800), (intptr_t)(0), (intptr_t)((int)(*v55__f)));
      if ( v10 != 0 )
      {
        SETLO(v10, F16(v7, 88) - 2);
        if ( F16(v7, 88) == 2 )
        {
          v43 = rnd();
          SETLO(v10, W_sfx_play_at((intptr_t)(c), (intptr_t)(v42), (intptr_t)(68), (intptr_t)(0), (intptr_t)(500 * v43 / 0x8000 - 1000), (intptr_t)(0)));
        }
      }
      if ( F16(v7, 90) < 2u && F16(v7, 88) == 3 )
        SETLO(v10, W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(3200), (intptr_t)(-5600), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(2), (intptr_t)(v42), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(2)));
      return v10;
    case '<':
      v12 = 0;
      *a4 = 2;
      v13 = F16(v7, 88);
      ++F32(v7, 92);
      switch ( v13 )
      {
        case 0:
          if ( F32(v7, 108) != D(0x652958) )
          {
            F32(v7, 108) = D(0x652958);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          v12 = 1;
          F16(v7, 88) = 1;
          goto LABEL_6;
        case 1:
LABEL_6:
          v14 = F8(v7, 116);
          F32(v7, 28) = 0;
          if ( v14 != 0 )
          {
            W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x652aec)));
            F16(v7, 88) = 2;
            v12 = 1;
          }
          if ( a5 != 0 )
            W_body_action_reset((intptr_t)(v7));
          if ( v12 != 0 )
            break;
          goto LABEL_31;
        case 2:
          v22 = a5;
          F32(v7, 28) = 64000;
          if ( v22 == 0 )
            goto LABEL_31;
          W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x652c80)));
          F16(v7, 88) = 3;
          break;
        case 3:
          if ( F8(v7, 116) == 0 )
            goto LABEL_31;
          W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x652e14)));
          F16(v7, 88) = 4;
          break;
        case 4:
          if ( F8(v7, 116) != 0 )
            W_body_action_reset((intptr_t)(v7));
          goto LABEL_31;
        default:
          goto LABEL_31;
      }
      if ( F16(v7, 88) == 2 )
      {
        v23 = 1000 * rnd();
        a5 = (_DWORD *)F32(c, 0x40);
        if ( a3 == 0 )
        {
          v24 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1140)), (intptr_t)(0));
          if ( v24 != 0 )
          {
            v25 = F32(v24, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5), (intptr_t)((int *)&a4), (intptr_t)(&a2)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v25, (int)a4);
              g_eng.dsb_pan_5bb880(v25, a2);
              g_eng.dsb_freq_5bb890(v25, v23 / 0x8000 - 1500);
              v21 = v25;
              goto LABEL_30;
            }
          }
        }
      }
      else if ( F16(v7, 88) == 3 )
      {
        if ( a3 == 0 )
        {
          for ( i = 16; i != 0; --i )
          {
            v16 = F32(v7, 12) + 1600;
            v17 = rnd();
            W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)v7), (intptr_t)(v16 * v17 / 0x8000 - v16 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
          }
        }
        v18 = (_DWORD *)F32(c, 0x40);
        if ( a3 == 0 )
        {
          v19 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1152)), (intptr_t)(0));
          if ( v19 != 0 )
          {
            v20 = F32(v19, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v18), (intptr_t)((int *)&a5), (intptr_t)((int *)&a4)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v20, (int)a5);
              g_eng.dsb_pan_5bb880(v20, (int)a4);
              g_eng.dsb_freq_5bb890(v20, 1000);
              v21 = v20;
LABEL_30:
              g_eng.dsb_play_5bb8c0(v21);
            }
          }
        }
      }
LABEL_31:
      SETLO(v10, F16(v7, 88) - 2);
      if ( F16(v7, 88) == 2 )
      {
        if ( F16(v7, 90) < 3u )
        {
          v34 = F32(v7, 76);
          (*v61__f) = F32(v34, 740);
          v35 = rnd();
          (*v64__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v34), (intptr_t)(1000), (intptr_t)(1000 * v35 / 0x8000));
          a5 = F32(v34, 176);
          v36 = W_char_mag_sum((intptr_t)(c));
          v37 = F32(v34, 744);
          v38 = F32(v34, 180) + v36;
          (*v59__f) = 0;
          a2 = 0;
          a4 = 0;
          if ( v37 == 0 && F32(v34, 732) == 0 && a5 == 0 )
          {
            v39 = F32(c, 0x750);
            v38 = F32(v39, 296);
            a5 = F32(v39, 292);
            (*v59__f) = F32(v39, 300);
            v40 = F32(v39, 304);
            v41 = F32(v39, 308);
            a2 = v40;
            a4 = v41;
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4), (intptr_t)(a3), (intptr_t)((*v61__f)), (intptr_t)((int)a5), (intptr_t)(v38), (intptr_t)(0), (intptr_t)(2), (intptr_t)((*v64__f)), (intptr_t)(1), (intptr_t)(F32(v34, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v34, 756)), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v59__f)), (intptr_t)(a2), (intptr_t)((int)a4));
          v10 = F32(c, 0xc7f4);
          if ( v10 != 0 )
          {
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(F32(c, 0xc7f8)), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4), (intptr_t)(a3), (intptr_t)((*v61__f)), (intptr_t)((int)a5), (intptr_t)(v38), (intptr_t)(0), (intptr_t)(2), (intptr_t)((*v64__f)), (intptr_t)(1), (intptr_t)(F32(v34, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v34, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v59__f)), (intptr_t)(a2), (intptr_t)((int)a4));
            SETLO(v10, W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(-F32(c, 0xc7f8)), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4), (intptr_t)(a3), (intptr_t)((*v61__f)), (intptr_t)((int)a5), (intptr_t)(v38), (intptr_t)(0), (intptr_t)(2), (intptr_t)((*v64__f)), (intptr_t)(1), (intptr_t)(F32(v34, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v34, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
                          + F32(F32(c, 0x750), 224)
                          + F32(F32(c, 0x750), 228)), (intptr_t)((*v59__f)), (intptr_t)(a2), (intptr_t)((int)a4)));
          }
        }
      }
      else
      {
        SETLO(v10, F16(v7, 88) - 3);
        if ( F16(v7, 88) == 3 && F32(v7, 92) > 20 && F16(v7, 90) < 3u )
        {
          v26 = F32(v7, 76);
          (*v63__f) = F32(v26, 740);
          v27 = rnd();
          (*v60__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v26), (intptr_t)(1000), (intptr_t)(1000 * v27 / 0x8000));
          a5 = F32(v26, 176);
          v28 = W_char_mag_sum((intptr_t)(c));
          v29 = F32(v26, 744);
          v30 = F32(v26, 180) + v28;
          (*v58__f) = 0;
          a2 = 0;
          a4 = 0;
          if ( v29 == 0 && F32(v26, 732) == 0 && a5 == 0 )
          {
            v31 = F32(c, 0x750);
            v30 = F32(v31, 296);
            a5 = F32(v31, 292);
            (*v58__f) = F32(v31, 300);
            v32 = F32(v31, 304);
            v33 = F32(v31, 308);
            a2 = v32;
            a4 = v33;
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(4), (intptr_t)(a3), (intptr_t)((*v63__f)), (intptr_t)((int)a5), (intptr_t)(v30), (intptr_t)(0), (intptr_t)(2), (intptr_t)((*v60__f)), (intptr_t)(1), (intptr_t)(F32(v26, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v26, 756)), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v58__f)), (intptr_t)(a2), (intptr_t)((int)a4));
          v10 = F32(c, 0xc7f4);
          if ( v10 != 0 )
          {
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(F32(c, 0xc7f8) + 2000), (intptr_t)(4000), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(4), (intptr_t)(a3), (intptr_t)((*v63__f)), (intptr_t)((int)a5), (intptr_t)(v30), (intptr_t)(0), (intptr_t)(2), (intptr_t)((*v60__f)), (intptr_t)(1), (intptr_t)(F32(v26, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v26, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v58__f)), (intptr_t)(a2), (intptr_t)((int)a4));
            SETLO(v10, W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(2000 - F32(c, 0xc7f8)), (intptr_t)(4000), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(4), (intptr_t)(a3), (intptr_t)((*v63__f)), (intptr_t)((int)a5), (intptr_t)(v30), (intptr_t)(0), (intptr_t)(2), (intptr_t)((*v60__f)), (intptr_t)(1), (intptr_t)(F32(v26, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v26, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
                          + F32(F32(c, 0x750), 224)
                          + F32(F32(c, 0x750), 228)), (intptr_t)((*v58__f)), (intptr_t)(a2), (intptr_t)((int)a4)));
          }
        }
      }
      return v10;
    default:
      return v10;
  }
}

/* 0x44b160: __int16 __thiscall sub_44B160(cls_4022A0 *this, int a2, int a3, _DWORD *a4, _DWORD *a5, int a6, int a7) */
int32_t ih_44b160(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    void * v11 = 0;
    intptr_t * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int v15 = 0;
    int16_t v16 = 0;
    intptr_t * v17 = 0;
    intptr_t v18 = 0;
    uint16_t v19 = 0;
    uint16_t v20 = 0;
    char * v21 = 0;
    intptr_t * v22 = 0;
    intptr_t v23 = 0;
    intptr_t * v24 = 0;
    intptr_t v25 = 0;
    void * v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    int16_t v29 = 0;
    char v30 = 0;
    intptr_t v31 = 0;
    int32_t * v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    int32_t ** v35 = 0;
    intptr_t v36 = 0;
    int32_t * v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x30 */
    int32_t * *const v42__f = (int32_t * *)(frame_ + 0xc);   /* ebp-0x24 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x20 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x1c */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x1c */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x4 */

  (*v47__f) = 0;
  (*v46__f) = 1000;
  v8 = a2;
  v9 = F32(F32(c, 0x750), 212) == 1;
  v10 = F16(a2, 72);
  switch ( (int16_t)v10 )
  {
    case 3:
      *a4 = 3;
      v16 = F16(v8, 88);
      if ( v16 == 0 )
      {
        a4 = (_DWORD *)F32(c, 0x40);
        if ( a3 == 0 )
        {
          v17 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1444)), (intptr_t)(0));
          if ( v17 != 0 )
          {
            v18 = F32(v17, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)((int *)&a5), (intptr_t)(&a2)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v18, (int)a5);
              g_eng.dsb_pan_5bb880(v18, a2);
              g_eng.dsb_freq_5bb890(v18, -2000);
              g_eng.dsb_play_5bb8c0(v18);
            }
          }
        }
        v14 = (int)D(0x6591f0);
        if ( F32(v8, 108) != D(0x6591f0) )
        {
          F32(v8, 108) = D(0x6591f0);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        if ( a3 == 0 )
        {
          F32(F32(c, 0x750), 212) = 1;
          v14 = F32(c, 0x1d4);
          if ( v14 == 50010 )
          {
            F32(c, 0x158a0) = 2;
            SETLO(v14, W_char_set_look_up((intptr_t)(c), (intptr_t)(700), (intptr_t)((int)D(0x62e3c0)), (intptr_t)((int)D(0x62e558)), (intptr_t)(75), (intptr_t)(600), (intptr_t)(0)));
            F16(c, 0xe8) = 1;
            F32(c, 0x15914) = (int)D(0x62def8);
            F32(c, 0x15a80) = 0;
          }
        }
        ++F16(v8, 88);
        return v14;
      }
      SETLO(v14, F16(v8, 88) - 1);
      if ( F16(v8, 88) == 1 )
      {
        if ( F8(v8, 116) != 0 )
        {
          F32(v8, 92) = 0;
          F16(v8, 88) = v16 + 1;
        }
        return v14;
      }
      SETLO(v14, F16(v8, 88) - 2);
      if ( F16(v8, 88) != 2 )
        return v14;
      v14 = F32(v8, 92);
      if ( v14 < 16 )
      {
        F32(v8, 92) = ++v14;
        return v14;
      }
      goto LABEL_51;
    case 4:
      *a4 = 3;
      if ( F16(v8, 88) != 0 )
      {
        SETLO(v14, F16(v8, 88) - 1);
        if ( F16(v8, 88) == 1 && F8(v8, 116) != 0 )
        {
LABEL_51:
          F16(v8, 72) = 0;
          F16(v8, 86) = 210;
          F16(v8, 88) = 0;
          F32(v8, 92) = 0;
          F32(v8, 120) = 0;
          F16(v8, 84) = 0;
          F32(v8, 76) = 0;
          F32(v8, 80) = 0;
          F16(v8, 90) = 0;
          F16(v8, 100) = 0;
        }
      }
      else
      {
        a4 = (_DWORD *)F32(c, 0x40);
        if ( a3 == 0 )
        {
          v24 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1444)), (intptr_t)(0));
          if ( v24 != 0 )
          {
            v25 = F32(v24, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)((int *)&a5), (intptr_t)(&a2)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v25, (int)a5);
              g_eng.dsb_pan_5bb880(v25, a2);
              g_eng.dsb_freq_5bb890(v25, -2000);
              g_eng.dsb_play_5bb8c0(v25);
            }
          }
        }
        v14 = (int)D(0x659058);
        if ( F32(v8, 108) != D(0x659058) )
        {
          F32(v8, 108) = D(0x659058);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        if ( a3 == 0 )
        {
          F32(F32(c, 0x750), 212) = 0;
          v14 = F32(c, 0x1d4);
          if ( v14 == 50010 )
          {
            F32(c, 0x158a0) = 0;
            SETLO(v14, W_char_set_look_up((intptr_t)(c), (intptr_t)(500), (intptr_t)((int)D(0x62e3c0)), (intptr_t)((int)D(0x62e558)), (intptr_t)(78), (intptr_t)(200), (intptr_t)(0)));
            F16(c, 0xe8) = 0;
            F32(c, 0x15914) = (int)D(0x62a8c8);
            F32(c, 0x15a80) = 1;
          }
        }
        ++F16(v8, 88);
      }
      return v14;
    case 5:
      *a4 = 3;
      v11 = F32(v8, 108);
      F8(v8, 124) = 1;
      F8(v8, 125) = 1;
      F32(v8, 28) = 1;
      if ( v11 != D(0x658530) )
      {
        F32(v8, 108) = D(0x658530);
        F8(v8, 116) = 0;
        F16(v8, 114) = 0;
        F16(v8, 112) = 0;
      }
      if ( F32(v8, 92) == 0 )
      {
        a4 = (_DWORD *)F32(c, 0x40);
        if ( a3 == 0 )
        {
          v12 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1240)), (intptr_t)(0));
          if ( v12 != 0 )
          {
            v13 = F32(v12, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)((int *)&a5), (intptr_t)(&a2)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v13, (int)a5);
              g_eng.dsb_pan_5bb880(v13, a2);
              g_eng.dsb_freq_5bb890(v13, 0);
              g_eng.dsb_play_5bb8c0(v13);
            }
          }
        }
      }
      v14 = F32(v8, 92);
      if ( v14 >= 40 )
        goto LABEL_51;
      ++v14;
      v15 = a3 == 0;
      F32(v8, 92) = v14;
      if ( v15 )
      {
        v14 = W_tint_fade_a((intptr_t)(v14), (intptr_t)(40));
        F32(c, 0xf4) = v14;
        F32(c, 0xf8) = v14;
      }
      return v14;
    case 65:
      *a4 = 3;
      v14 = F16(v8, 88);
      F8(v8, 124) = 1;
      F8(v8, 125) = 1;
      switch ( v14 )
      {
        case 0:
          if ( F32(v8, 108) != D(0x658530) )
          {
            F32(v8, 108) = D(0x658530);
            F8(v8, 116) = 0;
            F16(v8, 114) = 0;
            F16(v8, 112) = 0;
          }
          if ( a3 != 0 )
            goto LABEL_38;
          W_entity_fx_remove((intptr_t)(c), (intptr_t)(100155));
          v19 = F16(c, 0x1ac);
          v20 = 0;
          if ( v19 == 0 )
            goto LABEL_38;
          break;
        case 1:
LABEL_39:
          v14 = F32(v8, 92) + 1;
          F32(v8, 92) = v14;
          if ( v14 > 240 )
            ++F16(v8, 88);
          return v14;
        case 2:
          a4 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v22 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1444)), (intptr_t)(0));
            if ( v22 != 0 )
            {
              v23 = F32(v22, 0);
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(&a3), (intptr_t)((int *)&a5)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v23, a3);
                g_eng.dsb_pan_5bb880(v23, (int)a5);
                g_eng.dsb_freq_5bb890(v23, -2000);
                g_eng.dsb_play_5bb8c0(v23);
              }
            }
          }
          v14 = (int)D(0x659058);
          if ( F32(v8, 108) != D(0x659058) )
          {
            F32(v8, 108) = D(0x659058);
            F8(v8, 116) = 0;
            F16(v8, 114) = 0;
            F16(v8, 112) = 0;
          }
          ++F16(v8, 88);
          return v14;
        case 3:
          if ( F8(v8, 116) == 0 )
            return v14;
          F32(F32(c, 0x750), 212) = 0;
          v14 = F32(c, 0x1d4);
          if ( v14 == 50010 )
          {
            F32(c, 0x158a0) = 0;
            SETLO(v14, W_char_set_look_up((intptr_t)(c), (intptr_t)(500), (intptr_t)((int)D(0x62e3c0)), (intptr_t)((int)D(0x62e558)), (intptr_t)(78), (intptr_t)(200), (intptr_t)(0)));
            F16(c, 0xe8) = 0;
            F32(c, 0x15914) = (int)D(0x62a8c8);
            F32(c, 0x15a80) = 1;
          }
          goto LABEL_51;
        default:
          return v14;
      }
      break;
    default:
      if ( v9 || F16(a2, 72) >= 3u && (F16(a2, 72) <= 4u || v10 == 65) )
      {
        v27 = v10 - 63;
        if ( v27 != 0 )
        {
          v14 = v27 - 1;
          if ( v14 == 0 )
          {
            v28 = 0;
            *a4 = 2;
            v29 = F16(v8, 88);
            ++F32(v8, 92);
            switch ( v29 )
            {
              case 0:
                W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x6599d8)));
                v28 = 1;
                F16(v8, 88) = 1;
                goto LABEL_77;
              case 1:
LABEL_77:
                v30 = F8(v8, 116);
                F32(v8, 28) = 0;
                if ( v30 != 0 )
                {
                  W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x659b6c)));
                  F16(v8, 88) = 2;
                  v28 = 1;
                }
                if ( a5 != 0 )
                  W_body_action_reset((intptr_t)(v8));
                if ( v28 != 0 )
                  break;
                goto LABEL_95;
              case 2:
                v32 = a5;
                F32(v8, 28) = 64000;
                if ( v32 == 0 )
                  goto LABEL_95;
                W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x659d00)));
                F16(v8, 88) = 3;
                break;
              case 3:
                if ( F8(v8, 116) == 0 )
                  goto LABEL_95;
                W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x659e94)));
                F16(v8, 88) = 4;
                break;
              case 4:
                if ( F8(v8, 116) != 0 )
                  W_body_action_reset((intptr_t)(v8));
                goto LABEL_95;
              default:
                goto LABEL_95;
            }
            if ( F16(v8, 88) == 2 )
            {
              v33 = rnd();
              a5 = (_DWORD *)F32(c, 0x40);
              v34 = 1000 * v33 / 0x8000 - 1500;
              if ( a3 == 0 )
              {
                v35 = (_DWORD **)W_vpool_take((intptr_t)(SYS32(1140)), (intptr_t)(0));
                if ( v35 != 0 )
                {
                  a4 = *v35;
                  if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5), (intptr_t)(&a2), (intptr_t)(&(*v47__f))) != 0 )
                  {
                    g_eng.dsb_volume_5bb870(a4, a2);
                    g_eng.dsb_pan_5bb880(a4, (*v47__f));
                    (*v44__f) = v34;
                    v36 = a4;
                    g_eng.dsb_freq_5bb890(a4, (*v44__f));
                    g_eng.dsb_play_5bb8c0(v36);
                  }
                }
              }
            }
            else if ( F16(v8, 88) == 3 )
            {
              v31 = a3;
              W_entity_dust((intptr_t)(c), (intptr_t)(v8), (intptr_t)(a3), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
              W_sfx_play_at((intptr_t)(c), (intptr_t)(v31), (intptr_t)(71), (intptr_t)(0), (intptr_t)(1000), (intptr_t)(0));
            }
LABEL_95:
            SETLO(v14, F16(v8, 88) - 2);
            if ( F16(v8, 88) == 2 )
            {
              if ( F16(v8, 90) < 2u )
                SETLO(v14, W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(5600), (intptr_t)(4), (intptr_t)(a3), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(2)));
            }
            else
            {
              SETLO(v14, F16(v8, 88) - 3);
              if ( F16(v8, 88) == 3 && F32(v8, 92) > 20 && F16(v8, 90) < 2u )
                SETLO(v14, W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(4), (intptr_t)(a3), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(2)));
            }
          }
        }
        else
        {
          v37 = a5;
          if ( F16(a2, 88) == 0 )
            W_body_pose_swap((intptr_t)(c), (intptr_t)(a2), (intptr_t)((int)a5), (intptr_t)(0x3Fu), (intptr_t)(0), (intptr_t)(0x3Fu), (intptr_t)(6), (intptr_t)(1));
          v14 = F16(v8, 84);
          if ( (uint16_t)v14 != 0 )
          {
            if ( v14 != 6 )
              return v14;
            (*v47__f) = 1;
            (*v46__f) = 1200;
          }
          (*v45__f) = (int)v37;
          v38 = a3;
          (*v43__f) = (*v47__f);
          (*v41__f) = a3;
          *a4 = 2;
          v14 = W_slash_step((intptr_t)(c), (intptr_t)(v8), (intptr_t)((*v41__f)), (intptr_t)((int)D(0x659388)), (intptr_t)(1600), (intptr_t)(-1600), (intptr_t)((*v43__f)), (intptr_t)((*v45__f)));
          if ( v14 != 0 )
          {
            SETLO(v14, F16(v8, 88) - 2);
            if ( F16(v8, 88) == 2 )
            {
              v39 = rnd();
              SETLO(v14, W_sfx_play_at((intptr_t)(c), (intptr_t)(v38), (intptr_t)(68), (intptr_t)(0), (intptr_t)(1000 * v39 / 0x8000 - 4000), (intptr_t)(0)));
            }
          }
          if ( F16(v8, 90) < 2u )
          {
            if ( F16(v8, 88) == 2 )
            {
              (*v42__f) = F32(v8, 76);
              F16(v8, 86) = 210;
              SETLO(v14, W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(2400), (intptr_t)(-4000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v38), (intptr_t)((*v42__f)), (intptr_t)((*v46__f)), (intptr_t)(2)));
            }
            else
            {
              SETLO(v14, F16(v8, 88) - 3);
              if ( F16(v8, 88) == 3 )
                SETLO(v14, W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(6000), (intptr_t)(3200), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v38), (intptr_t)(F32(v8, 76)), (intptr_t)((*v46__f)), (intptr_t)(2)));
            }
          }
        }
      }
      else
      {
        *a4 = 3;
        v26 = F32(v8, 108);
        F8(v8, 126) = 1;
        F8(v8, 124) = 1;
        F8(v8, 125) = 1;
        if ( v26 != D(0x658ec0) )
        {
          F32(v8, 108) = D(0x658ec0);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        SETLO(v14, W_body_action_reset((intptr_t)(v8)));
      }
      return v14;
  }
  while ( *(_DWORD *)&GAP(c, 0x10c, 20 * v20) != 0 )
  {
    if ( ++v20 >= v19 )
      goto LABEL_38;
  }
  v21 = (uint8_t *)c + 20 * v20;
  F32(v21, 4 * 67) = 100155;
  F16(v21, 2 * 136) = 200;
  F16(v21, 2 * 137) = 0;
  F16(v21, 2 * 138) = 0;
  F16(v21, 2 * 139) = 0;
  *(_DWORD *)&GAP(c, 0x10c, 20 * v20 + 12) = 0;
  F32(v21, 4 * 71) = 0;
LABEL_38:
  ++F16(v8, 88);
  goto LABEL_39;
}

/* 0x44bc10: void __thiscall sub_44BC10(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5) */
void ih_44bc10(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v5 = a2;
  v7 = F16(a2, 72);
  if ( (uint16_t)v7 != 18 )
  {
    if ( v7 == 55 )
    {
      v8 = 0;
      *a4 = 2;
      switch ( F16(v5, 88) )
      {
        case 0:
          W_body_set_clip((intptr_t)(v5), (intptr_t)((int)D(0x66cd48)));
          v8 = 1;
          F16(v5, 88) = 1;
          goto LABEL_5;
        case 1:
LABEL_5:
          if ( F8(v5, 116) == 0 )
            goto LABEL_13;
          W_body_set_clip((intptr_t)(v5), (intptr_t)((int)D(0x66cedc)));
          F16(v5, 88) = 2;
          goto LABEL_14;
        case 2:
          if ( F8(v5, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(v5), (intptr_t)((int)D(0x66d070)));
            W_char_lunge((intptr_t)(c), (intptr_t)(v5), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
            F16(v5, 88) = 3;
          }
          else
          {
LABEL_13:
            if ( v8 == 0 )
              goto LABEL_21;
          }
LABEL_14:
          if ( F16(v5, 88) == 1 )
          {
            W_voice_cue((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0x45u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
          }
          else if ( F16(v5, 88) == 3 )
          {
            v9 = rnd();
            a4 = (_DWORD *)F32(c, 0x40);
            v10 = 1000 * v9 / 0x8000 - 2000;
            if ( a3 == 0 )
            {
              v11 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1144)), (intptr_t)(0));
              if ( v11 != 0 )
              {
                v12 = F32(v11, 0);
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(&a2), (intptr_t)(&(*v18__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v12, a2);
                  g_eng.dsb_pan_5bb880(v12, (*v18__f));
                  g_eng.dsb_freq_5bb890(v12, v10);
                  g_eng.dsb_play_5bb8c0(v12);
                }
              }
            }
          }
LABEL_21:
          if ( F16(v5, 90) == 0 && F16(v5, 88) == 3 )
            W_char_attack_hits((intptr_t)(c), (intptr_t)(v5), (intptr_t)(6000), (intptr_t)(1200), (intptr_t)(6400), (intptr_t)(2800), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(v5, 76)), (intptr_t)(1000), (intptr_t)(3));
          break;
        case 3:
          if ( F8(v5, 116) == 0 )
            goto LABEL_21;
          W_body_set_clip((intptr_t)(v5), (intptr_t)((int)D(0x66d204)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v5), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
          F16(v5, 88) = 4;
          goto LABEL_14;
        case 4:
          if ( F8(v5, 116) == 0 )
            goto LABEL_21;
          W_body_action_reset((intptr_t)(v5));
          F16(v5, 88) = 5;
          goto LABEL_14;
        default:
          goto LABEL_21;
      }
    }
    return;
  }
  SETHI(v13, 0);
  *a4 = 3;
  SETLO(v13, F16(v5, 88));
  F8(v5, 124) = 1;
  F8(v5, 125) = 1;
  if ( (uint16_t)v13 != 0 )
  {
    v14 = v13 - 1;
    if ( v14 != 0 )
    {
      if ( v14 == 1 )
      {
        v15 = F32(v5, 92) + 1;
        F32(v5, 92) = v15;
        if ( v15 >= 40 )
        {
          if ( F32(c, 0x15904) != 0 )
          {
            if ( a3 != 0 )
              F8(v5, 0) = 0;
            else
              W_char_release((intptr_t)(c));
          }
          else
          {
            F16(v5, 72) = 0;
            F16(v5, 86) = 210;
            F16(v5, 88) = 0;
            F32(v5, 92) = 0;
            F32(v5, 120) = 0;
            F16(v5, 84) = 0;
            F32(v5, 76) = 0;
            F32(v5, 80) = 0;
            F16(v5, 90) = 0;
            F16(v5, 100) = 0;
            W_body_action_start((intptr_t)(v5), (intptr_t)(122), (intptr_t)(800));
          }
        }
        else
        {
          v16 = W_tint_fade_a((intptr_t)(40 - v15), (intptr_t)(40));
          F32(c, 0xf4) = v16;
          F32(c, 0xf8) = v16;
        }
      }
      return;
    }
  }
  else
  {
    if ( F32(v5, 108) != D(0x66cbb0) )
    {
      F32(v5, 108) = D(0x66cbb0);
      F8(v5, 116) = 0;
      F16(v5, 114) = 0;
      F16(v5, 112) = 0;
    }
    W_entity_fx_add((intptr_t)(c), (intptr_t)(100155), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(0));
    ++F16(v5, 88);
  }
  v17 = F32(v5, 92) + 1;
  F32(v5, 92) = v17;
  if ( v17 > 200 )
  {
    ++F16(v5, 88);
    F32(v5, 92) = 0;
  }
}

/* 0x44c250: char __thiscall sub_44C250(cls_4022A0 *this, int a2, int a3, int a4, _DWORD *a5) */
int32_t ih_44c250(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
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
    int32_t * v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x30 */
    int32_t * *const v42__f = (int32_t * *)(frame_ + 0xc);   /* ebp-0x2c */
    int32_t * *const v44__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x28 */
    int32_t * *const v43__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  (*v50__f) = F32(c, 0x15a28) == 1;
  v6 = a2;
  if ( F16(a2, 72) == 52 )
  {
    v33 = a5;
    if ( F16(a2, 88) == 0 )
      W_body_pose_swap((intptr_t)(c), (intptr_t)(a2), (intptr_t)((int)a5), (intptr_t)(0x34u), (intptr_t)(0), (intptr_t)(0x34u), (intptr_t)(6), (intptr_t)(1));
    if ( F16(v6, 84) == 1 )
    {
      (*v47__f) = (int)v33;
      v36 = a3;
      v7 = W_slash_step((intptr_t)(c), (intptr_t)(v6), (intptr_t)(a3), (intptr_t)((int)D(0x66f198)), (intptr_t)(2000), (intptr_t)(-2000), (intptr_t)(0), (intptr_t)((*v47__f)));
      if ( v7 != 0 )
      {
        if ( F16(v6, 88) == 1 )
        {
          SETLB(v7, W_voice_cue((intptr_t)(c), (intptr_t)(v36), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0)));
        }
        else
        {
          SETLO(v7, F16(v6, 88) - 2);
          if ( F16(v6, 88) == 2 )
          {
            v37 = rnd();
            SETLB(v7, W_sfx_play_at((intptr_t)(c), (intptr_t)(v36), (intptr_t)(72), (intptr_t)(0), (intptr_t)(1000 * v37 / 0x8000 - 4000), (intptr_t)(0)));
          }
        }
      }
      if ( F16(v6, 90) < 2u && F16(v6, 88) == 3 )
        SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(8000), (intptr_t)(-2400), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(2), (intptr_t)(v36), (intptr_t)(F32(v6, 76)), (intptr_t)(1000), (intptr_t)(2)));
    }
    else
    {
      v34 = a3;
      F32(a4, 0) = 2;
      v7 = W_slash_step((intptr_t)(c), (intptr_t)(v6), (intptr_t)(v34), (intptr_t)((int)D(0x66f7e8)), (intptr_t)(2000), (intptr_t)(-2000), (intptr_t)(0), (intptr_t)((int)v33));
      if ( v7 != 0 )
      {
        if ( F16(v6, 88) == 1 )
        {
          SETLB(v7, W_voice_cue((intptr_t)(c), (intptr_t)(v34), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0)));
        }
        else
        {
          SETLO(v7, F16(v6, 88) - 2);
          if ( F16(v6, 88) == 2 )
          {
            v35 = rnd();
            SETLB(v7, W_sfx_play_at((intptr_t)(c), (intptr_t)(v34), (intptr_t)(68), (intptr_t)(0), (intptr_t)(1000 * v35 / 0x8000 - 4000), (intptr_t)(0)));
          }
        }
      }
      if ( F16(v6, 90) < 2u )
      {
        SETLO(v7, F16(v6, 88) - 1);
        if ( F16(v6, 88) == 1 )
        {
          if ( (*v50__f) == 1 )
            F16(v6, 86) = 230;
        }
        else if ( F16(v6, 88) == 2 )
        {
          (*v44__f) = F32(v6, 76);
          F16(v6, 86) = 210;
          SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(4000), (intptr_t)(-2800), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v34), (intptr_t)((*v44__f)), (intptr_t)(1000), (intptr_t)(2)));
        }
        else
        {
          SETLO(v7, F16(v6, 88) - 3);
          if ( F16(v6, 88) == 3 )
            SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(6000), (intptr_t)(3200), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v34), (intptr_t)(F32(v6, 76)), (intptr_t)(1000), (intptr_t)(2)));
        }
      }
    }
  }
  else if ( F16(a2, 72) == 53 )
  {
    F32(a4, 0) = 2;
    if ( F16(v6, 88) == 0 )
    {
      v12 = (int)a5;
      if ( F32(v6, 652) != 0 )
        v12 = 1;
      if ( F16(c, 0x158bc) < F16(c, 0xc7e8) )
        v12 = 1;
      if ( F16(v6, 72) == 53 )
      {
        if ( v12 != 0 )
        {
          if ( F16(v6, 84) == 6 && v12 == 1 )
          {
            W_body_action_reset((intptr_t)(v6));
            v13 = W_char_skill_find((intptr_t)(c), (intptr_t)(0x35u));
            W_body_pose_set((intptr_t)(v6), (intptr_t)(53), (intptr_t)(F16(v6, 86)), (intptr_t)(0), (intptr_t)(F32(v6, 80)), (intptr_t)(v13));
          }
        }
        else
        {
          F16(v6, 72) = 0;
          F16(v6, 86) = 210;
          F16(v6, 88) = 0;
          F32(v6, 92) = 0;
          F32(v6, 120) = 0;
          F16(v6, 84) = 0;
          F32(v6, 76) = 0;
          F32(v6, 80) = 0;
          F16(v6, 90) = 0;
          F16(v6, 100) = 0;
          (*v46__f) = W_char_skill_find((intptr_t)(c), (intptr_t)(0x35u));
          W_body_pose_set((intptr_t)(v6), (intptr_t)(53), (intptr_t)(F16(v6, 86)), (intptr_t)(6), (intptr_t)(F32(v6, 80)), (intptr_t)((*v46__f)));
        }
      }
    }
    if ( F16(v6, 84) == 1 )
    {
      v20 = a3;
      SETLO(v7, F16(v6, 88));
      v21 = 0;
      switch ( (int16_t)v7 )
      {
        case 0:
          if ( F32(v6, 108) != D(0x66dea8) )
          {
            F32(v6, 108) = D(0x66dea8);
            F8(v6, 116) = 0;
            F16(v6, 114) = 0;
            F16(v6, 112) = 0;
          }
          v21 = 1;
          F16(v6, 88) = 1;
          goto LABEL_72;
        case 1:
LABEL_72:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 == 0 )
            goto LABEL_80;
          W_body_set_clip((intptr_t)(v6), (intptr_t)((int)D(0x66e03c)));
          F16(v6, 88) = 2;
          goto LABEL_81;
        case 2:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 != 0 )
          {
            W_body_set_clip((intptr_t)(v6), (intptr_t)((int)D(0x66e1d0)));
            W_char_lunge((intptr_t)(c), (intptr_t)(v6), (intptr_t)(2000), (intptr_t)(v20), (intptr_t)(1));
            F16(v6, 88) = 3;
          }
          else
          {
LABEL_80:
            if ( v21 == 0 )
              goto LABEL_94;
          }
LABEL_81:
          SETLO(v7, F16(v6, 88) - 1);
          if ( F16(v6, 88) == 1 )
          {
            if ( v20 == 0 && F32(c, 0x15b54) != 0 && F32(c, 0x15b5c) == 0 )
            {
              if ( F32(c, 0x15a8c) != 1
                || (v7 = CFG(),
                    F32(CFG(), 400) != 0) )
              {
                v7 = W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)(&a4), (intptr_t)(&a5));
                if ( v7 != 0 )
                  SETLB(v7, W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(a2), (intptr_t)(a4), (intptr_t)((int)a5), (intptr_t)(0)));
              }
            }
          }
          else
          {
            SETLO(v7, F16(v6, 88) - 2);
            if ( F16(v6, 88) == 2 )
            {
              v7 = rnd();
              v22 = (_DWORD *)F32(c, 0x40);
              v23 = 1000 * v7;
              SETLB(v7, a3);
              if ( a3 == 0 )
              {
                v7 = W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
                if ( v7 != 0 )
                {
                  a4 = F32(v7, 0);
                  v7 = W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v22), (intptr_t)((int *)&a5), (intptr_t)(&a2));
                  if ( v7 != 0 )
                  {
                    v24 = a4;
                    g_eng.dsb_volume_5bb870(a4, (int)a5);
                    g_eng.dsb_pan_5bb880(v24, a2);
                    g_eng.dsb_freq_5bb890(v24, v23 / 0x8000 - 4000);
                    SETLB(v7, g_eng.dsb_play_5bb8c0(v24));
                  }
                }
              }
            }
          }
LABEL_94:
          if ( F16(v6, 90) < 2u && F16(v6, 88) == 3 )
          {
            v25 = F32(v6, 76);
            (*v50__f) = F32(v25, 740);
            v26 = rnd();
            (*v49__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v25), (intptr_t)(1000), (intptr_t)(1000 * v26 / 0x8000));
            a5 = F32(v25, 176);
            v27 = W_char_mag_sum((intptr_t)(c));
            v28 = F32(v25, 744);
            v29 = F32(v25, 180) + v27;
            (*v48__f) = 0;
            a2 = 0;
            a4 = 0;
            if ( v28 == 0 && F32(v25, 732) == 0 && a5 == 0 )
            {
              v30 = F32(c, 0x750);
              v31 = F32(v30, 300);
              v29 = F32(v30, 296);
              a5 = F32(v30, 292);
              (*v48__f) = v31;
              v32 = F32(v30, 308);
              a2 = F32(v30, 304);
              a4 = v32;
            }
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v6), (intptr_t)(8000), (intptr_t)(-2400), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(2), (intptr_t)(a3), (intptr_t)((*v50__f)), (intptr_t)((int)a5), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v49__f)), (intptr_t)(1), (intptr_t)(F32(v25, 736)), (intptr_t)(F32(v6, 96)), (intptr_t)(0), (intptr_t)(F32(v25, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v48__f)), (intptr_t)(a2), (intptr_t)(a4));
            v7 = F32(c, 0xc7f4);
            if ( v7 != 0 )
            {
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v6), (intptr_t)(F32(c, 0xc7f8) + 8000), (intptr_t)(-2400), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(2), (intptr_t)(a3), (intptr_t)((*v50__f)), (intptr_t)((int)a5), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v49__f)), (intptr_t)(1), (intptr_t)(F32(v25, 736)), (intptr_t)(F32(v6, 96)), (intptr_t)(0), (intptr_t)(F32(v25, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v48__f)), (intptr_t)(a2), (intptr_t)(a4));
              SETLB(v7, W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v6), (intptr_t)(8000 - F32(c, 0xc7f8)), (intptr_t)(-2400), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(2), (intptr_t)(a3), (intptr_t)((*v50__f)), (intptr_t)((int)a5), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v49__f)), (intptr_t)(1), (intptr_t)(F32(v25, 736)), (intptr_t)(F32(v6, 96)), (intptr_t)(0), (intptr_t)(F32(v25, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
                           + F32(F32(c, 0x750), 224)
                           + F32(F32(c, 0x750), 228)), (intptr_t)((*v48__f)), (intptr_t)(a2), (intptr_t)(a4)));
            }
          }
          break;
        case 3:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 == 0 )
            goto LABEL_94;
          W_body_set_clip((intptr_t)(v6), (intptr_t)((int)D(0x66e364)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v6), (intptr_t)(-2000), (intptr_t)(v20), (intptr_t)(1));
          F16(v6, 88) = 4;
          goto LABEL_81;
        case 4:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 == 0 )
            goto LABEL_94;
          W_body_action_reset((intptr_t)(v6));
          F16(v6, 88) = 5;
          goto LABEL_81;
        default:
          goto LABEL_94;
      }
    }
    else
    {
      if ( F16(v6, 84) == 6 )
      {
        v14 = 1;
        v15 = D(0x66eb48);
        a4 = 1200;
      }
      else
      {
        v14 = 0;
        a4 = 1000;
        v15 = D(0x66e4f8);
      }
      v16 = a3;
      SETLO(v7, F16(v6, 88));
      v17 = 0;
      switch ( (int16_t)v7 )
      {
        case 0:
          W_body_set_clip((intptr_t)(v6), (intptr_t)((int)v15));
          v17 = 1;
          F16(v6, 88) = 1;
          goto LABEL_36;
        case 1:
LABEL_36:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 == 0 )
            goto LABEL_47;
          if ( v15[235] != 0 )
          {
            W_body_set_clip((intptr_t)(v6), (intptr_t)((int)(v15 + 202)));
            F16(v6, 88) = 2;
          }
          else
          {
LABEL_39:
            SETLB(v7, F8(v6, 116));
            if ( (uint8_t)v7 != 0 )
            {
              W_body_set_clip((intptr_t)(v6), (intptr_t)((int)(v15 + 404)));
              W_char_lunge((intptr_t)(c), (intptr_t)(v6), (intptr_t)(2000), (intptr_t)(v16), (intptr_t)(1));
              F16(v6, 88) = 3;
            }
            else
            {
LABEL_47:
              if ( v17 == 0 )
                goto LABEL_55;
            }
          }
LABEL_48:
          SETLO(v7, F16(v6, 88) - 1);
          if ( F16(v6, 88) == 1 )
          {
            if ( v16 == 0 )
            {
              v7 = F32(c, 0x15b54);
              if ( v7 != 0 )
              {
                v7 = F32(c, 0x15b5c);
                if ( v7 == 0
                  && (F32(c, 0x15a8c) != 1 || (v7 = F32(CFG(), 400)) != 0) )
                {
                  v7 = W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)((int *)&a5), (intptr_t)(&a3));
                  if ( v7 != 0 )
                    SETLB(v7, W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(a2), (intptr_t)((int)a5), (intptr_t)(a3), (intptr_t)(0)));
                }
              }
            }
          }
          else
          {
            SETLO(v7, F16(v6, 88) - 2);
            if ( F16(v6, 88) == 2 )
            {
              v18 = 1000 * rnd();
              v7 = F32(c, 0x40);
              a5 = (_DWORD *)v7;
              if ( v16 == 0 )
              {
                v7 = W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
                if ( v7 != 0 )
                {
                  v19 = F32(v7, 0);
                  v7 = W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5), (intptr_t)(&a2), (intptr_t)(&(*v50__f)));
                  if ( v7 != 0 )
                  {
                    g_eng.dsb_volume_5bb870(v19, a2);
                    g_eng.dsb_pan_5bb880(v19, (*v50__f));
                    g_eng.dsb_freq_5bb890(v19, v18 / 0x8000 - 4000);
                    SETLB(v7, g_eng.dsb_play_5bb8c0(v19));
                  }
                  v16 = a3;
                }
              }
            }
          }
LABEL_55:
          if ( F16(v6, 90) < 2u )
          {
            SETLO(v7, F16(v6, 88) - 1);
            if ( F16(v6, 88) != 1 )
            {
              if ( F16(v6, 88) == 2 )
              {
                (*v45__f) = a4;
                (*v43__f) = F32(v6, 76);
                F16(v6, 86) = 210;
                SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(4000), (intptr_t)(-2800), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v16), (intptr_t)((*v43__f)), (intptr_t)((*v45__f)), (intptr_t)(5)));
              }
              else
              {
                SETLO(v7, F16(v6, 88) - 3);
                if ( F16(v6, 88) == 3 )
                  SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(6000), (intptr_t)(3200), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v16), (intptr_t)(F32(v6, 76)), (intptr_t)(a4), (intptr_t)(5)));
              }
            }
          }
          break;
        case 2:
          goto LABEL_39;
        case 3:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 == 0 && (v14 != 1 || a5 != (_DWORD *)1) )
            goto LABEL_55;
          W_body_set_clip((intptr_t)(v6), (intptr_t)((int)(v15 + 606)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v6), (intptr_t)(-2000), (intptr_t)(v16), (intptr_t)(1));
          F16(v6, 88) = 4;
          goto LABEL_48;
        case 4:
          SETLB(v7, F8(v6, 116));
          if ( (uint8_t)v7 == 0 )
            goto LABEL_55;
          W_body_action_reset((intptr_t)(v6));
          F16(v6, 88) = 5;
          goto LABEL_48;
        default:
          goto LABEL_55;
      }
    }
  }
  else
  {
    SETLO(v7, F16(a2, 72) - 56);
    if ( F16(a2, 72) == 56 )
    {
      v8 = a3;
      (*v42__f) = a5;
      (*v41__f) = a4;
      (*v40__f) = a3;
      (*v39__f) = a2;
      F32(a4, 0) = 2;
      if ( ih_461640((*v39__f), (*v40__f), (*v41__f), (int)(*v42__f), (int)D(0x670488), 0, 64000) != 0 )
      {
        if ( F16(v6, 88) == 2 )
        {
          v9 = 1000 * rnd();
          a5 = (_DWORD *)F32(c, 0x40);
          if ( v8 == 0 )
          {
            v10 = (int *)W_vpool_take((intptr_t)(SYS32(1140)), (intptr_t)(0));
            if ( v10 != 0 )
            {
              a3 = *v10;
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5), (intptr_t)(&a4), (intptr_t)(&a2)) != 0 )
              {
                g_eng.dsb_volume_5bb870(a3, a4);
                g_eng.dsb_pan_5bb880(a3, a2);
                v11 = a3;
                g_eng.dsb_freq_5bb890(a3, v9 / 0x8000 - 1500);
                g_eng.dsb_play_5bb8c0(v11);
              }
            }
          }
        }
        else if ( F16(v6, 88) == 3 )
        {
          W_entity_dust((intptr_t)(c), (intptr_t)(v6), (intptr_t)(v8), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
          W_sfx_play_at((intptr_t)(c), (intptr_t)(v8), (intptr_t)(71), (intptr_t)(0), (intptr_t)(1000), (intptr_t)(0));
        }
      }
      SETLO(v7, F16(v6, 88) - 2);
      if ( F16(v6, 88) == 2 )
      {
        if ( F16(v6, 90) < 3u )
          SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(0), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(4), (intptr_t)(v8), (intptr_t)(F32(v6, 76)), (intptr_t)(1000), (intptr_t)(2)));
      }
      else
      {
        SETLO(v7, F16(v6, 88) - 3);
        if ( F16(v6, 88) == 3 && F32(v6, 92) > 20 && F16(v6, 90) < 3u )
          SETLB(v7, W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(2800), (intptr_t)(4000), (intptr_t)(8000), (intptr_t)(3200), (intptr_t)(4), (intptr_t)(v8), (intptr_t)(F32(v6, 76)), (intptr_t)(1000), (intptr_t)(2)));
      }
    }
  }
  return v7;
}

/* 0x44d8f0: int __thiscall sub_44D8F0(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7, _DWORD *a8) */
int32_t ih_44d8f0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v9 = 0;
    intptr_t v11 = 0;
    int16_t v12 = 0;
    intptr_t v13 = 0;
    int v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
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
    uint16_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x10 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  result = 0;
  v9 = a2;
  v11 = 0;
  switch ( F16(a2, 72) )
  {
    case 3:
      *a4 = 3;
      v41 = F16(v9, 88);
      v42 = a3;
      F8(v9, 125) = 1;
      if ( v41 != 0 )
      {
        if ( v41 != 1 )
          return 1;
      }
      else
      {
        W_voice_cue((intptr_t)(c), (intptr_t)(v42), (intptr_t)(0x45u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
        W_body_set_clip((intptr_t)(v9), (intptr_t)((int)D(0x647ff0)));
        if ( v42 == 0 )
        {
          W_char_party_mode_set((intptr_t)(c), (intptr_t)((void *)1));
          F32(c, 0xc764) = 0;
        }
        ++F16(v9, 88);
      }
      v43 = F32(v9, 92);
      if ( v43 >= 30 )
      {
        if ( v42 == 0 )
        {
          F32(c, 0xf4) = 0;
          F32(c, 0xf8) = 0;
        }
        goto LABEL_70;
      }
      if ( v42 == 0 )
      {
        v44 = W_tint_fade_a((intptr_t)(v43), (intptr_t)(30));
        F32(c, 0xf4) = v44;
        F32(c, 0xf8) = v44;
      }
      ++F32(v9, 92);
      return 1;
    case 0x12:
      v40 = a3;
      if ( ih_44e730(c, a2, a3, a4, (int)D(0x6484b8), 1) == 0 )
        return 1;
      W_ih_4466b0((intptr_t)(c), (intptr_t)(v9), (intptr_t)(v40), (intptr_t)(1));
      return 1;
    case 0x31:
      v12 = F16(a2, 88);
      v13 = F32(a2, 92) + 1;
      F8(a2, 124) = 1;
      F32(v9, 92) = v13;
      F16(v9, 86) = 230;
      switch ( v12 )
      {
        case 0:
          v14 = a3 == 0;
          F16(v9, 88) = v12 + 1;
          if ( v14
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)(&a2), (intptr_t)(&a4)) != 0 )
          {
            W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(0));
          }
          if ( F32(v9, 108) != D(0x647ff0) )
          {
            F32(v9, 108) = D(0x647ff0);
            F8(v9, 116) = 0;
            F16(v9, 114) = 0;
            F16(v9, 112) = 0;
          }
          goto LABEL_12;
        case 1:
LABEL_12:
          (*v49__f) = 0;
          (*v50__f) = -80000;
          if ( F32(v9, 92) >= 10 )
          {
            if ( F32(v9, 108) != D(0x648320) )
            {
              F32(v9, 108) = D(0x648320);
              F8(v9, 116) = 0;
              F16(v9, 114) = 0;
              F16(v9, 112) = 0;
            }
            ++F16(v9, 88);
            F32(v9, 92) = 0;
          }
          goto LABEL_24;
        case 2:
          (*v49__f) = 40000;
          (*v50__f) = 80000;
          if ( a6 != 0 )
          {
            F32(v9, 24) = 0;
            F16(v9, 88) = v12 + 1;
          }
          v15 = F32(v9, 76);
          (*v53__f) = F32(v15, 740);
          v16 = rnd();
          (*v51__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v15), (intptr_t)(1000), (intptr_t)(1000 * v16 / 0x8000));
          a4 = F32(v15, 176);
          v17 = W_char_mag_sum((intptr_t)(c));
          v18 = F32(v15, 744);
          v19 = F32(v15, 180) + v17;
          (*v47__f) = 0;
          a6 = 0;
          a2 = 0;
          if ( v18 == 0 && F32(v15, 732) == 0 && a4 == 0 )
          {
            v20 = F32(c, 0x750);
            v19 = F32(v20, 296);
            a4 = F32(v20, 292);
            (*v47__f) = F32(v20, 300);
            v21 = F32(v20, 304);
            v22 = F32(v20, 308);
            a6 = v21;
            a2 = v22;
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v9), (intptr_t)(1600), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v53__f)), (intptr_t)((int)a4), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v51__f)), (intptr_t)(1), (intptr_t)(F32(v15, 736)), (intptr_t)(F32(v9, 96)), (intptr_t)(0), (intptr_t)(F32(v15, 756)), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v47__f)), (intptr_t)(a6), (intptr_t)(a2));
          if ( F32(c, 0xc7f4) != 0 )
          {
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v9), (intptr_t)(F32(c, 0xc7f8) + 1600), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v53__f)), (intptr_t)((int)a4), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v51__f)), (intptr_t)(1), (intptr_t)(F32(v15, 736)), (intptr_t)(F32(v9, 96)), (intptr_t)(0), (intptr_t)(F32(v15, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v47__f)), (intptr_t)(a6), (intptr_t)(a2));
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v9), (intptr_t)(1600 - F32(c, 0xc7f8)), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v53__f)), (intptr_t)((int)a4), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v51__f)), (intptr_t)(1), (intptr_t)(F32(v15, 736)), (intptr_t)(F32(v9, 96)), (intptr_t)(0), (intptr_t)(F32(v15, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v47__f)), (intptr_t)(a6), (intptr_t)(a2));
          }
          goto LABEL_24;
        case 3:
          (*v49__f) = 80000;
          (*v50__f) = 0;
          if ( v13 > 70 )
          {
            F16(v9, 88) = v12 + 1;
            if ( F32(v9, 108) != D(0x647ff0) )
            {
              F32(v9, 108) = D(0x647ff0);
              F8(v9, 116) = 0;
              F16(v9, 114) = 0;
              F16(v9, 112) = 0;
            }
          }
          v29 = F32(v9, 76);
          (*v52__f) = F32(v29, 740);
          v30 = rnd();
          (*v54__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v29), (intptr_t)(1400), (intptr_t)(1000 * v30 / 0x8000));
          a4 = F32(v29, 176);
          v31 = W_char_mag_sum((intptr_t)(c));
          v32 = F32(v29, 744);
          v33 = F32(v29, 180) + v31;
          (*v48__f) = 0;
          a6 = 0;
          a2 = 0;
          if ( v32 == 0 && F32(v29, 732) == 0 && a4 == 0 )
          {
            v34 = F32(c, 0x750);
            v33 = F32(v34, 296);
            a4 = F32(v34, 292);
            (*v48__f) = F32(v34, 300);
            v35 = F32(v34, 304);
            v36 = F32(v34, 308);
            a6 = v35;
            a2 = v36;
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v9), (intptr_t)(1600), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v52__f)), (intptr_t)((int)a4), (intptr_t)(v33), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v54__f)), (intptr_t)(1), (intptr_t)(F32(v29, 736)), (intptr_t)(F32(v9, 96)), (intptr_t)(0), (intptr_t)(F32(v29, 756)), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v48__f)), (intptr_t)(a6), (intptr_t)(a2));
          if ( F32(c, 0xc7f4) != 0 )
          {
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v9), (intptr_t)(F32(c, 0xc7f8) + 1600), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v52__f)), (intptr_t)((int)a4), (intptr_t)(v33), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v54__f)), (intptr_t)(1), (intptr_t)(F32(v29, 736)), (intptr_t)(F32(v9, 96)), (intptr_t)(0), (intptr_t)(F32(v29, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v48__f)), (intptr_t)(a6), (intptr_t)(a2));
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v9), (intptr_t)(1600 - F32(c, 0xc7f8)), (intptr_t)(800), (intptr_t)(4800), (intptr_t)(9600), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v52__f)), (intptr_t)((int)a4), (intptr_t)(v33), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v54__f)), (intptr_t)(1), (intptr_t)(F32(v29, 736)), (intptr_t)(F32(v9, 96)), (intptr_t)(0), (intptr_t)(F32(v29, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v48__f)), (intptr_t)(a6), (intptr_t)(a2));
          }
          goto LABEL_24;
        case 4:
          (*v50__f) = -80000;
          v23 = W_speed_ramp((intptr_t)(F32(v9, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)(0));
          goto LABEL_25;
        default:
LABEL_24:
          v23 = W_speed_ramp((intptr_t)(F32(v9, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)((*v49__f)));
LABEL_25:
          v24 = F32(v9, 24);
          F32(v9, 40) = v23;
          v25 = W_speed_ramp((intptr_t)(v24), (intptr_t)(4000), (intptr_t)(-4000), (intptr_t)((*v50__f)));
          v26 = a3;
          (*v46__f) = a5;
          (*v45__f) = a3;
          F32(v9, 24) = v25;
          W_char_drift_move((intptr_t)(c), (intptr_t)(v9), (intptr_t)((*v45__f)), (intptr_t)((*v46__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          v27 = F32(c, 0xc798);
          if ( v27 != 0 )
          {
            ih_44e1c0(v27, v26, 4800, 3200, 3, 0);
            *a8 = 1;
          }
          if ( F32(v9, 92) <= 80 )
            return 1;
          v28 = F32(c, 0xc798);
          if ( v28 != 0 )
            ih_44e1c0(v28, v26, 0, 0, 3, 1);
          F16(v9, 72) = 0;
          F16(v9, 86) = 210;
          F16(v9, 88) = 0;
          F32(v9, 92) = 0;
          F32(v9, 120) = 0;
          F16(v9, 84) = 0;
          F32(v9, 76) = 0;
          F32(v9, 80) = 0;
          F16(v9, 90) = 0;
          F16(v9, 100) = 0;
          result = 1;
          break;
      }
      return result;
    case 0x32:
      v37 = a3;
      *a4 = 3;
      if ( F16(v9, 88) == 0 )
        W_voice_cue((intptr_t)(c), (intptr_t)(v37), (intptr_t)(0x45u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v38 = F16(v9, 88);
      if ( v38 != 0 )
      {
        if ( F8(v9, 116) == 0 )
          return 1;
        if ( v38 < 4u )
          W_body_set_clip((intptr_t)(v9), (intptr_t)(404 * F16(v9, 88) + 6590816));
      }
      else
      {
        W_body_set_clip((intptr_t)(v9), (intptr_t)((int)D(0x649160)));
      }
      if ( ++F16(v9, 88) != 2 )
      {
        if ( F16(v9, 88) == 5 )
LABEL_70:
          W_body_action_reset((intptr_t)(v9));
        return 1;
      }
      do
      {
        v39 = W_char_shot_fire((intptr_t)(c), (intptr_t)(100033), (intptr_t)(v9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(v37), (intptr_t)(0), (intptr_t)(v11), (intptr_t)(F32(v9, 76)), (intptr_t)(1000), (intptr_t)(F32(v9, 44)), (intptr_t)(0), (intptr_t)(2));
        if ( v39 != 0 && v11 != 0 )
          F32(v39, 504) = 1;
        v11 += 30;
      }
      while ( v11 < 360 );
      return 1;
    default:
      return result;
  }
}

/* 0x44e1c0: int __thiscall sub_44E1C0(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t ih_44e1c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t result = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t * v15 = 0;
    intptr_t v16 = 0;
    int v17 = 0;
    int16_t v18 = 0;
    int32_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    _Alignas(8) uint8_t frame_[0x510 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Format = (char *)(frame_ + 0x10);   /* [512] at ebp-0x500 */
    char *Buffer = (char *)(frame_ + 0x210);   /* [256] at ebp-0x300 */
    char *v36 = (char *)(frame_ + 0x310);   /* [512] at ebp-0x200 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x510 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x510 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x510 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x50c */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x50c */
    uint16_t * *const v32__f = (uint16_t * *)(frame_ + 0x8);   /* ebp-0x508 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x504 */

  v7 = F32(c, 0x40);
  (*v27__f) = F32(c, 0x15938);
  result = a2;
  if ( a2 != 0 )
    return result;
  if ( F32(c, 0x15934) == 0 )
    return result;
  v9 = F32(c, 0xc7a4);
  if ( v9 == 0 )
    return result;
  if ( F16(v9, 68) == 0 )
  {
    g_eng.h5bf3ee(Buffer, "%d", F32(v9, 468));
    g_eng.h408b90(D(0x8a9530), ((char *)D(0x85451c)), 0, Buffer);
  }
  if ( F32(F32((v9 + 64), 0), 44) == 3 )
  {
    a3 = -a3;
    if ( a5 == 99 )
      goto LABEL_11;
    v10 = a5 != 3 ? 3 : 1;
  }
  else
  {
    v10 = a5;
    if ( a5 == 99 )
      goto LABEL_11;
  }
  F32(v7, 44) = v10;
LABEL_11:
  if ( a6 != 0 )
  {
    if ( F32(c, 0x15934) != 0 )
    {
      v11 = F32(c, 0xc7a4);
      if ( v11 != 0 )
      {
        F32(v11, 51096) = 0;
        F32(c, 0xc7a4) = 0;
      }
    }
    v12 = -1;
    v13 = 1096;
    F16(v7, 72) = 0;
    F16(v7, 86) = 210;
    F16(v7, 88) = 0;
    F32(v7, 92) = 0;
    F32(v7, 120) = 0;
    F16(v7, 84) = 0;
    F32(v7, 76) = 0;
    F32(v7, 80) = 0;
    F16(v7, 90) = 0;
    F16(v7, 100) = 0;
    (*v30__f) = -1;
    (*v33__f) = 1096;
    (*v32__f) = D(0x628778);
    (*v28__f) = 18;
    do
    {
      if ( *(*v32__f) != 0 )
      {
        v14 = 0;
        v15 = (_WORD *)(F32(c, 0x750) + v13);
        if ( *v15 != 0 )
        {
          *v15 = 0;
          switch ( v12 )
          {
            case 0:
              strcpy(Format, ((char *)D(0x85530c)));
              v16 = F32(c, 0x15910);
              F32(c, 0xfc) = v16;
              F32(c, 0x100) = v16;
              goto LABEL_25;
            case 1:
              strcpy(Format, ((char *)D(0x855340)));
              goto LABEL_25;
            case 2:
              strcpy(Format, ((char *)D(0x855324)));
              goto LABEL_25;
            case 4:
              strcpy(Format, ((char *)D(0x855354)));
              goto LABEL_25;
            case 5:
              strcpy(Format, ((char *)D(0x855388)));
              goto LABEL_25;
            case 6:
              strcpy(Format, ((char *)D(0x855360)));
LABEL_25:
              v14 = 1;
              break;
            default:
              break;
          }
          if ( F32(c, 0xc794) != 0 && v14 != 0 && W_char_room_logs((intptr_t)(c)) != 0 )
          {
            g_eng.h5bf3ee(v36, Format, F32(c, 0x750));
            W_world_log_line((intptr_t)(W), (intptr_t)(v36));
          }
        }
      }
      (*v32__f) += 70;
      v13 = (*v33__f) + 2;
      v12 = (*v30__f) + 1;
      v17 = (*v28__f) == 1;
      (*v33__f) += 2;
      ++(*v30__f);
      --(*v28__f);
    }
    while ( !v17 );
    result = F32(c, 0x15940);
    if ( result != 0 )
    {
      v18 = F32(c, 0x15940);
      result = 0;
      if ( (F16(v7, 86) / 100 <= 2 || F16(v7, 72) == 0) && v18 != F16(v7, 72) )
      {
        F16(v7, 88) = 0;
        F32(v7, 92) = 0;
        F32(v7, 120) = 0;
        F16(v7, 84) = 0;
        F32(v7, 76) = 0;
        F32(v7, 80) = 0;
        F16(v7, 90) = 0;
        F16(v7, 100) = 0;
        F16(v7, 72) = v18;
        F16(v7, 86) = 210;
      }
    }
  }
  else
  {
    v19 = F32(F32(c, 0xc7a4), 64);
    v20 = a3 + v19[1] + v19[3] / 2;
    (*v31__f) = v19[5];
    v21 = v19[2] + (v19[4] - (*v31__f)) / 2;
    v22 = (*v27__f);
    v23 = a4 + (*v31__f) + v21;
    if ( F32(v7, 44) == 3 )
      v22 = -(*v27__f);
    (*v29__f) = v22 + v20 - F32(v7, 12) / 2;
    v24 = v23 + F32(c, 0x1593c) - F32(v7, 16) / 2;
    v25 = W_part_center_x((intptr_t)(c), (intptr_t)(0));
    W_entity_move_horizontal((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)((*v29__f) - v25), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
    v26 = W_part_center_y((intptr_t)(c), (intptr_t)(0));
    W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(v24 - v26), (intptr_t)(1), (intptr_t)(0), (intptr_t)(v7 + 8), (intptr_t)((_DWORD *)(v7 + 644)));
    if ( (*v29__f) - W_part_center_x((intptr_t)(c), (intptr_t)(0)) > 1600 )
      return W_char_carry((intptr_t)(c), (intptr_t)(0));
    result = W_part_center_y((intptr_t)(c), (intptr_t)(0));
    if ( v24 - result > 1600 )
      return W_char_carry((intptr_t)(c), (intptr_t)(0));
  }
  return result;
}

/* 0x44e730: int __thiscall sub_44E730(cls_44E730 *this, int a2, int a3, _DWORD *a4, int a5, int a6) */
int32_t ih_44e730(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t result = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t i = 0;
    uint16_t v13 = 0;
    uint16_t v14 = 0;
    char * v15 = 0;
    intptr_t v16 = 0;

  SETHI(v6, 0);
  *a4 = 3;
  SETLO(v6, F16(a2, 88));
  result = 1;
  F8(a2, 124) = 1;
  F8(a2, 125) = 1;
  if ( (uint16_t)v6 == 0 )
  {
    if ( F32(a2, 108) != a5 )
    {
      F32(a2, 108) = a5;
      F8(a2, 116) = 0;
      F16(a2, 114) = 0;
      F16(a2, 112) = 0;
    }
    if ( a3 == 0 )
    {
      for ( i = 0; i < F16(c, 0x1ac); ++i )
      {
        if ( *(_DWORD *)&GAP(c, 0xfc, 20 * i + 16) == 100155 )
          *(_DWORD *)&GAP(c, 0xfc, 20 * i + 16) = 0;
      }
      v13 = F16(c, 0x1ac);
      v14 = 0;
      if ( v13 != 0 )
      {
        while ( *(_DWORD *)&GAP(c, 0xfc, 20 * v14 + 16) != 0 )
        {
          if ( ++v14 >= v13 )
            goto LABEL_22;
        }
        v15 = &GAP(c, 0x0, 20 * v14);
        F32(v15, 4 * 67) = 100155;
        F16(v15, 2 * 136) = 200;
        F16(v15, 2 * 137) = 0;
        F16(v15, 2 * 138) = 0;
        F16(v15, 2 * 139) = 0;
        *(_DWORD *)&GAP(c, 0xfc, 20 * v14 + 28) = 0;
        F32(v15, 4 * 71) = 0;
      }
    }
LABEL_22:
    ++F16(a2, 88);
LABEL_23:
    v16 = F32(a2, 92) + 1;
    F32(a2, 92) = v16;
    if ( v16 > 200 )
    {
      if ( a6 == 0 )
        return result;
      ++F16(a2, 88);
      F32(a2, 92) = 0;
    }
    return 0;
  }
  v8 = v6 - 1;
  if ( v8 == 0 )
    goto LABEL_23;
  if ( v8 != 1 )
    return 0;
  v9 = F32(a2, 92) + 1;
  F32(a2, 92) = v9;
  if ( v9 >= 40 )
    return result;
  v10 = 20 * (40 - v9) / 40;
  if ( v10 < 0 )
  {
    v10 = 0;
LABEL_7:
    v11 = SYS32(4 * v10 + 10152);
    F32(c, 0xf4) = v11;
    F32(c, 0xf8) = v11;
    return 0;
  }
  if ( v10 < 20 )
    goto LABEL_7;
  F32(c, 0xf4) = 0;
  F32(c, 0xf8) = 0;
  return 0;
}

/* 0x44e8d0: int __thiscall sub_44E8D0(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7) */
int32_t ih_44e8d0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    int16_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    uint16_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    int32_t * v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    int32_t * v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    int32_t * v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    int16_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    int32_t ** v63 = 0;
    intptr_t v64 = 0;
    _Alignas(8) uint8_t frame_[0x3c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3c */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x38 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x2c */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x2c */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x2c */
    intptr_t *const v72__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x28 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x28 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x28 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x24 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x24 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x24 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x24 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x24 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x10 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0xc */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x8 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x4 */

  (*v78__f) = 0;
  (*v79__f) = 1;
  v8 = a2;
  (*v80__f) = F32(F32(c, 0x750), 212) == 1;
  v9 = F16(a2, 72);
  v10 = F32(CFG(), 148) != 0 ? 700 : 1000;
  (*v81__f) = v10;
  switch ( (int16_t)v9 )
  {
    case 3:
    case 5:
      v18 = a3;
      *a4 = 3;
      v19 = F16(v8, 88);
      F8(v8, 125) = 1;
      if ( v19 != 0 )
      {
        if ( v19 != 1 )
          return 1;
      }
      else
      {
        if ( v18 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)((int *)&a4), (intptr_t)(&a3)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x45u), (intptr_t)(1), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(a3), (intptr_t)(0));
        }
        if ( F32(v8, 108) != D(0x6497b0) )
        {
          F32(v8, 108) = D(0x6497b0);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        if ( v18 == 0 )
        {
          F32(F32(c, 0x750), 212) = 1;
          if ( F32(c, 0x1d4) == 50010 )
          {
            F32(c, 0x158a0) = 2;
            W_char_set_look_up((intptr_t)(c), (intptr_t)(700), (intptr_t)((int)D(0x62e3c0)), (intptr_t)((int)D(0x62e558)), (intptr_t)(75), (intptr_t)(600), (intptr_t)(0));
            F16(c, 0xe8) = 1;
            F32(c, 0x15914) = (int)D(0x62def8);
            F32(c, 0x15a80) = 0;
          }
          F32(c, 0xc764) = 0;
        }
        ++F16(v8, 88);
      }
      v20 = F32(v8, 92);
      if ( v20 < 30 )
      {
        if ( v18 != 0 )
        {
LABEL_56:
          ++F32(v8, 92);
          return 1;
        }
        v21 = 20 * (v10 * v20 / 30) / 1000;
        if ( v21 >= 0 )
        {
          if ( v21 >= 20 )
          {
            v22 = 0;
            goto LABEL_55;
          }
        }
        else
        {
          v21 = 0;
        }
        v22 = SYS32(4 * v21 + 10152);
LABEL_55:
        F32(c, 0xf4) = v22;
        F32(c, 0xf8) = v22;
        goto LABEL_56;
      }
      if ( v18 != 0 )
        goto LABEL_64;
      v23 = 20 * v10 / 1000;
      if ( v23 >= 0 )
      {
        if ( v23 >= 20 )
        {
          v24 = 0;
          goto LABEL_63;
        }
      }
      else
      {
        v23 = 0;
      }
      v24 = SYS32(4 * v23 + 10152);
LABEL_63:
      F32(c, 0xf4) = v24;
      F32(c, 0xf8) = v24;
      goto LABEL_64;
    case 4:
      *a4 = 3;
      v25 = F16(v8, 88);
      F8(v8, 125) = 1;
      if ( v25 != 0 )
      {
        if ( v25 != 1 )
          return 1;
      }
      else
      {
        if ( F32(v8, 108) != D(0x6497b0) )
        {
          F32(v8, 108) = D(0x6497b0);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        F32(v8, 92) = 30;
        F16(v8, 88) = v25 + 1;
      }
      v26 = F32(v8, 92);
      if ( v26 > 0 )
      {
        if ( a3 != 0 )
        {
LABEL_79:
          --F32(v8, 92);
          return 1;
        }
        v27 = 20 * (v10 * v26 / 30) / 1000;
        if ( v27 >= 0 )
        {
          if ( v27 >= 20 )
          {
            v28 = 0;
            goto LABEL_78;
          }
        }
        else
        {
          v27 = 0;
        }
        v28 = SYS32(4 * v27 + 10152);
LABEL_78:
        F32(c, 0xf4) = v28;
        F32(c, 0xf8) = v28;
        goto LABEL_79;
      }
      if ( a3 == 0 )
      {
        F32(F32(c, 0x750), 212) = 0;
        if ( F32(c, 0x1d4) == 50010 )
        {
          F32(c, 0x158a0) = 0;
          W_char_set_look_up((intptr_t)(c), (intptr_t)(500), (intptr_t)((int)D(0x62e3c0)), (intptr_t)((int)D(0x62e558)), (intptr_t)(78), (intptr_t)(200), (intptr_t)(0));
          F16(c, 0xe8) = 0;
          F32(c, 0x15914) = (int)D(0x62a8c8);
          F32(c, 0x15a80) = 1;
        }
        F32(c, 0xc764) = 1;
      }
LABEL_64:
      F16(v8, 72) = 0;
      F16(v8, 86) = 210;
      F16(v8, 88) = 0;
      F32(v8, 92) = 0;
      F32(v8, 120) = 0;
      F16(v8, 84) = 0;
      F32(v8, 76) = 0;
      F32(v8, 80) = 0;
      F16(v8, 90) = 0;
      F16(v8, 100) = 0;
      return 1;
    case 45:
      v11 = a3;
      *a4 = 3;
      v12 = F16(v8, 88);
      F8(v8, 124) = 1;
      F8(v8, 125) = 1;
      if ( v12 != 0 )
      {
        if ( v12 != 1 )
          goto LABEL_32;
      }
      else
      {
        if ( v11 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)((int *)&a4), (intptr_t)(&a3)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0xCu), (intptr_t)(1), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(a3), (intptr_t)(0));
        }
        if ( F32(v8, 108) != D(0x649e10) )
        {
          F32(v8, 108) = D(0x649e10);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        ++F16(v8, 88);
        F32(v8, 24) = 0;
        F32(v8, 40) = 0;
      }
      v13 = F32(v8, 24);
      if ( v13 <= -20000 )
      {
        if ( v13 < -20000 )
          v13 = v13 + 500 > 0 ? 0 : v13 + 500;
      }
      else
      {
        v13 -= 500;
        if ( v13 < -20000 )
          v13 = -20000;
      }
      F32(v8, 24) = v13;
      v14 = F32(v8, 92);
      if ( v14 >= 100 )
      {
        F16(v8, 72) = 0;
        F16(v8, 86) = 210;
        F16(v8, 88) = 0;
        F32(v8, 92) = 0;
        F32(v8, 120) = 0;
        F16(v8, 84) = 0;
        F32(v8, 76) = 0;
        F32(v8, 80) = 0;
        F16(v8, 90) = 0;
        F16(v8, 100) = 0;
        if ( F32(c, 0x15904) != 0 )
        {
          if ( v11 != 0 )
            F8(v8, 0) = 0;
          else
            W_char_release((intptr_t)(c));
        }
        else
        {
          W_body_action_reset((intptr_t)(v8));
          W_body_action_start((intptr_t)(v8), (intptr_t)(122), (intptr_t)(800));
        }
      }
      else
      {
        F32(v8, 92) = v14 + 1;
        if ( v11 != 0 )
          goto LABEL_32;
        v15 = 20 * (100 - (v14 + 1)) / 100;
        if ( v15 < 0 )
        {
          v15 = 0;
LABEL_24:
          v16 = SYS32(4 * v15 + 10152);
          F32(c, 0xf4) = v16;
          F32(c, 0xf8) = v16;
          goto LABEL_32;
        }
        if ( v15 < 20 )
          goto LABEL_24;
        F32(c, 0xf4) = 0;
        F32(c, 0xf8) = 0;
      }
LABEL_32:
      F32(v8, 8) += F32(v8, 24) / 100;
      return 1;
    default:
      if ( !(*v80__f) )
      {
        F8(a2, 126) = 1;
        F8(v8, 124) = 1;
        F8(v8, 125) = 1;
        return 0;
      }
      v29 = a3;
      v30 = v9 - 43;
      if ( v30 != 0 )
      {
        v31 = v30 - 1;
        if ( v31 == 0 )
        {
          (*v79__f) = 0;
          *a4 = 3;
          v55 = F16(v8, 88);
          F8(v8, 124) = 1;
          F32(v8, 24) = 0;
          switch ( v55 )
          {
            case 0:
              if ( F16(v8, 84) == 2 )
              {
                F32(v8, 44) = F32(v8, 44) != 3 ? 3 : 1;
                F32(v8, 40) = -F32(v8, 40);
              }
              W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x649ae0)));
              W_voice_cue((intptr_t)(c), (intptr_t)(v29), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
              ++F16(v8, 88);
              goto LABEL_153;
            case 1:
              if ( F8(v8, 116) == 0 )
                goto LABEL_153;
              W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x649c78)));
              v56 = a3;
              ++F16(v8, 88);
              F32(v8, 40) = -64000;
              if ( v56 != 0 )
                return (*v78__f);
              v57 = W_tint_fade_a((intptr_t)(400), (intptr_t)(1000));
              break;
            case 2:
              v58 = F32(v8, 92);
              F8(v8, 125) = 1;
              F32(v8, 92) = ++v58;
              if ( v58 > 16 )
              {
                F32(v8, 40) = 0;
                F32(v8, 92) = 0;
                F16(v8, 88) = v55 + 1;
              }
              goto LABEL_153;
            case 3:
              v59 = F32(v8, 92);
              F8(v8, 125) = 1;
              if ( v59 >= 10 )
              {
                W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x649ae0)));
                ++F16(v8, 88);
              }
              else
              {
                F32(v8, 92) = v59 + 1;
              }
              if ( a3 != 0 )
                return (*v78__f);
              v57 = W_tint_fade_a((intptr_t)(v10 * (F32(v8, 92) + 10) / 20), (intptr_t)(1000));
              break;
            case 4:
              if ( F8(v8, 116) != 0 )
              {
                if ( v29 == 0 )
                {
                  v60 = W_tint_fade_a((intptr_t)(v10), (intptr_t)(1000));
                  F32(c, 0xf4) = v60;
                  F32(c, 0xf8) = v60;
                }
                W_body_action_reset((intptr_t)(v8));
              }
              goto LABEL_153;
            default:
              goto LABEL_153;
          }
          goto LABEL_156;
        }
        if ( v31 == 2 )
        {
          *a4 = 3;
          if ( F16(v8, 88) == 0
            && v29 == 0
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)(&a2), (intptr_t)(&a4)) != 0 )
          {
            W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(0));
          }
          v32 = F16(v8, 88);
          if ( v32 != 0 )
          {
            if ( F8(v8, 116) == 0 )
            {
LABEL_122:
              v10 = (*v81__f);
              v29 = a3;
              (*v78__f) = 1;
              goto LABEL_153;
            }
            if ( v32 < 4u )
              W_body_set_clip((intptr_t)(v8), (intptr_t)(404 * F16(v8, 88) + 6596088));
          }
          else
          {
            W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x64a5f8)));
          }
          if ( ++F16(v8, 88) == 2 )
          {
            v33 = F16(v8, 84);
            if ( (uint16_t)v33 != 0 )
            {
              if ( v33 == 3 )
              {
                v41 = F32(v8, 96);
                v42 = F32(v8, 76);
                a4 = F32(v8, 44);
                v43 = 0;
                F8(v8, 127) = 1;
                a2 = v41;
                v44 = rnd();
                a6 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v42), (intptr_t)(1000), (intptr_t)(1000 * v44 / 0x8000));
                if ( v42[44] == 7 )
                {
                  (*v74__f) = v42[180];
                  v45 = rnd();
                  v43 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v42[179]), (intptr_t)(1000 * v45 / 0x8000), (intptr_t)((*v74__f)));
                }
                (*v71__f) = F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228);
                (*v68__f) = F32(c, 0x1f0);
                v46 = W_char_mag_sum((intptr_t)(c));
                v47 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100031), (intptr_t)((_DWORD *)v8), (intptr_t)(1600), (intptr_t)(1600), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(115), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(v42[44]), (intptr_t)(v42[45] + v46), (intptr_t)(v43), (intptr_t)((*v68__f)), (intptr_t)((*v71__f)), (intptr_t)(2));
                if ( v47 != 0 )
                  F32(v47, 516) = 800;
              }
              else
              {
                v34 = F32(v8, 96);
                v35 = F32(v8, 76);
                a4 = F32(v8, 44);
                v36 = 0;
                F8(v8, 127) = 1;
                a2 = v34;
                v37 = rnd();
                a6 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v35), (intptr_t)(1000), (intptr_t)(1000 * v37 / 0x8000));
                if ( v35[44] == 7 )
                {
                  (*v73__f) = v35[180];
                  v38 = rnd();
                  v36 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v35[179]), (intptr_t)(1000 * v38 / 0x8000), (intptr_t)((*v73__f)));
                }
                (*v70__f) = F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228);
                (*v67__f) = F32(c, 0x1f0);
                v39 = W_char_mag_sum((intptr_t)(c));
                v40 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100031), (intptr_t)((_DWORD *)v8), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(90), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(v35[44]), (intptr_t)(v35[45] + v39), (intptr_t)(v36), (intptr_t)((*v67__f)), (intptr_t)((*v70__f)), (intptr_t)(2));
                if ( v40 != 0 )
                  F32(v40, 516) = 1000;
              }
            }
            else
            {
              v48 = F32(v8, 96);
              v49 = F32(v8, 76);
              a4 = F32(v8, 44);
              v50 = 0;
              F8(v8, 127) = 1;
              a2 = v48;
              v51 = rnd();
              a6 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v49), (intptr_t)(1000), (intptr_t)(1000 * v51 / 0x8000));
              if ( v49[44] == 7 )
              {
                (*v75__f) = v49[180];
                v52 = rnd();
                v50 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v49[179]), (intptr_t)(1000 * v52 / 0x8000), (intptr_t)((*v75__f)));
              }
              (*v72__f) = F32(F32(c, 0x750), 216)
                  + F32(F32(c, 0x750), 224)
                  + F32(F32(c, 0x750), 228);
              (*v69__f) = F32(c, 0x1f0);
              v53 = W_char_mag_sum((intptr_t)(c));
              v54 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100031), (intptr_t)((_DWORD *)v8), (intptr_t)(2000), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(a6), (intptr_t)(90), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(v49[44]), (intptr_t)(v49[45] + v53), (intptr_t)(v50), (intptr_t)((*v69__f)), (intptr_t)((*v72__f)), (intptr_t)(2));
              if ( v54 != 0 )
                F32(v54, 516) = 700;
            }
          }
          else if ( F16(v8, 88) == 5 )
          {
            F16(v8, 86) = 210;
            F16(v8, 72) = 0;
            F16(v8, 88) = 0;
            F32(v8, 92) = 0;
            F32(v8, 120) = 0;
            F16(v8, 84) = 0;
            F32(v8, 76) = 0;
            F32(v8, 80) = 0;
            F16(v8, 90) = 0;
            F16(v8, 100) = 0;
          }
          goto LABEL_122;
        }
      }
      else
      {
        (*v76__f) = a6;
        (*v66__f) = a3;
        (*v65__f) = a2;
        *a4 = 3;
        if ( W_slash_step((intptr_t)(c), (intptr_t)((*v65__f)), (intptr_t)((*v66__f)), (intptr_t)((int)D(0x649fa8)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v76__f))) != 0 )
        {
          if ( F16(v8, 88) == 1 )
          {
            W_voice_cue((intptr_t)(c), (intptr_t)(v29), (intptr_t)(0x44u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          }
          else if ( F16(v8, 88) == 3 )
          {
            v61 = rnd();
            a2 = F32(c, 0x40);
            v62 = 1000 * v61 / 0x8000 - 1000;
            if ( a3 == 0 )
            {
              v63 = (_DWORD **)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
              if ( v63 != 0 )
              {
                a4 = *v63;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)a2), (intptr_t)(&a6), (intptr_t)(&(*v81__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(a4, a6);
                  g_eng.dsb_pan_5bb880(a4, (*v81__f));
                  (*v77__f) = v62;
                  v64 = a4;
                  g_eng.dsb_freq_5bb890(a4, (*v77__f));
                  g_eng.dsb_play_5bb8c0(v64);
                }
              }
            }
            v29 = a3;
          }
        }
        if ( F16(v8, 90) < 2u && F16(v8, 88) == 3 )
          W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(4000), (intptr_t)(2400), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(v29), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(3));
        (*v78__f) = 1;
      }
LABEL_153:
      if ( v29 == 0 && (*v79__f) != 0 )
      {
        v57 = W_tint_fade_a((intptr_t)(v10), (intptr_t)(1000));
LABEL_156:
        F32(c, 0xf4) = v57;
        F32(c, 0xf8) = v57;
      }
      return (*v78__f);
  }
}

/* 0x44f690: int __thiscall sub_44F690(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7, int a8, _DWORD *a9) */
int32_t ih_44f690(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t j = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    uint32_t v32 = 0;
    intptr_t result = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    int16_t v36 = 0;
    int16_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t * v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    int32_t * v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    uint32_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t * v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    _Alignas(8) uint8_t frame_[0x2d8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v79 = (uint16_t *)(frame_ + 0x40);   /* [2] at ebp-0x298 */
    intptr_t *v85 = (intptr_t *)(frame_ + 0x58);   /* [2] at ebp-0x280 */
    int16_t *v86 = (int16_t *)(frame_ + 0x60);   /* [316] at ebp-0x278 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2d8 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2d8 */
    intptr_t *const v72__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2c4 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x2b8 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x2b8 */
    int32_t * *const v74__f = (int32_t * *)(frame_ + 0x20);   /* ebp-0x2b8 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x2a4 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x2a0 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x29c */
    int32_t * *const v80__f = (int32_t * *)(frame_ + 0x44);   /* ebp-0x294 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x290 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x28c */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x288 */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x284 */

  v10 = F16(a2, 72) - 5;
  (*v81__f) = 0;
  (*v80__f) = 0;
  (*v78__f) = 0;
  if ( v10 == 0 )
  {
    *a4 = 3;
    W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x65ff40)));
    v21 = F32(a2, 92);
    F8(a2, 124) = 1;
    F8(a2, 125) = 1;
    F32(a2, 28) = 1;
    if ( v21 == 0 )
      W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(93), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v22 = F32(a2, 92);
    if ( v22 < 40 )
    {
      F32(a2, 92) = v22 + 1;
      W_shot_4497b0((intptr_t)(c), (intptr_t)(a3), (intptr_t)(v22 + 1), (intptr_t)(40));
      W_shot_449700((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(a2, 12)), (intptr_t)(F32(a2, 16)));
      goto LABEL_26;
    }
    goto LABEL_25;
  }
  v11 = v10 - 1;
  if ( v11 == 0 )
  {
    *a4 = 3;
    W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x65ff40)));
    v18 = F32(a2, 92);
    F8(a2, 124) = 1;
    F8(a2, 125) = 1;
    if ( v18 == 0 )
      W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(94), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v19 = F32(a2, 92);
    if ( v19 >= 40 )
    {
      W_body_action_reset((intptr_t)(a2));
      W_ih_4466b0((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0));
    }
    else
    {
      v20 = v19 + 1;
      F32(a2, 92) = v20;
      W_shot_4497b0((intptr_t)(c), (intptr_t)(a3), (intptr_t)(40 - v20), (intptr_t)(40));
      W_shot_449700((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(a2, 12)), (intptr_t)(F32(a2, 16)));
    }
    goto LABEL_26;
  }
  if ( v11 == 1 )
  {
    *a4 = 3;
    if ( F32(a2, 108) != D(0x65ff40) )
    {
      F32(a2, 108) = D(0x65ff40);
      F8(a2, 116) = 0;
      F16(a2, 114) = 0;
      F16(a2, 112) = 0;
    }
    F8(a2, 124) = 1;
    v12 = F16(a2, 88);
    F8(a2, 125) = 1;
    if ( v12 == 0 )
    {
      v14 = F32(a2, 92);
      if ( v14 >= 20 )
      {
        if ( !ih_467330(c) )
        {
          v16 = F32(c, 0x15b60);
          if ( v16 != 0 )
          {
            ih_45e260(v16, (int *)&(*i__f), &(*v77__f), (int)c);
            v17 = (*v77__f);
            F32(a2, 4) = (*i__f);
            F32(a2, 8) = v17;
          }
        }
        W_shot_4497b0((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(1));
        ++F16(a2, 88);
        F32(a2, 92) = 0;
        F32(a2, 24) = 0;
        F32(a2, 40) = 0;
      }
      else
      {
        v15 = v14 + 1;
        F32(a2, 92) = v15;
        W_shot_4497b0((intptr_t)(c), (intptr_t)(a3), (intptr_t)(20 - v15), (intptr_t)(20));
      }
      goto LABEL_26;
    }
    if ( v12 == 1 )
    {
      v13 = F32(a2, 92);
      if ( v13 < 40 )
      {
        F32(a2, 92) = v13 + 1;
        W_shot_4497b0((intptr_t)(c), (intptr_t)(a3), (intptr_t)(v13 + 1), (intptr_t)(40));
        goto LABEL_26;
      }
LABEL_25:
      W_body_action_reset((intptr_t)(a2));
    }
  }
LABEL_26:
  switch ( F16(a2, 72) )
  {
    case '$':
      *a4 = 3;
      if ( W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x660738)), (intptr_t)(1600), (intptr_t)(-1600), (intptr_t)(0), (intptr_t)(a6)) != 0 && F16(a2, 88) == 3 )
      {
        v62 = rnd();
        (*v80__f) = (_DWORD *)F32(c, 0x40);
        v63 = 1000 * v62 / 0x8000 - 500;
        if ( a3 == 0 )
        {
          v64 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
          if ( v64 != 0 )
          {
            (*i__f) = F32(v64, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v80__f)), (intptr_t)(&(*v81__f)), (intptr_t)((int *)&(*v77__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870((*i__f), (*v81__f));
              g_eng.dsb_pan_5bb880((*i__f), (int)(*v77__f));
              (*v75__f) = v63;
              v65 = (*i__f);
              g_eng.dsb_freq_5bb890((*i__f), (*v75__f));
              g_eng.dsb_play_5bb8c0(v65);
            }
          }
        }
      }
      if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
        W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(3));
      if ( F32(a2, 24) <= 0 )
        return (int)(*v78__f);
      goto LABEL_144;
    case '%':
      if ( F16(a2, 84) == 1 )
      {
        v35 = 40000;
        v34 = 10;
        (*v77__f) = -64000;
      }
      else
      {
        if ( F16(a2, 84) == 3 )
        {
          v35 = 80000;
          v34 = 10;
        }
        else
        {
          v34 = 0;
          v35 = 40000;
        }
        (*v77__f) = 0;
      }
      v36 = F16(a2, 88);
      F8(a2, 124) = 1;
      switch ( v36 )
      {
        case 0:
          if ( F32(a2, 108) != D(0x65ff40) )
          {
            F32(a2, 108) = D(0x65ff40);
            F8(a2, 116) = 0;
            F16(a2, 114) = 0;
            F16(a2, 112) = 0;
          }
          v37 = F16(a2, 88);
          F32(a2, 24) = -40000;
          F16(a2, 88) = v37 + 1;
          goto LABEL_69;
        case 1:
LABEL_69:
          (*v80__f) = &F32((*v77__f), 0x0);
          v38 = F32(a2, 92) + 1;
          F32(a2, 92) = v38;
          if ( v38 > 10 )
          {
            F32(a2, 92) = 0;
            ++F16(a2, 88);
            F32(a2, 24) = 500 * v35 / 1000;
            v39 = rnd();
            (*i__f) = F32(c, 0x40);
            v40 = 1000 * v39 / 0x8000;
            if ( a3 == 0 )
            {
              v41 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1436)), (intptr_t)(0));
              if ( v41 != 0 )
              {
                (*v77__f) = F32(v41, 0);
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*i__f)), (intptr_t)((int *)&(*v78__f)), (intptr_t)(&(*v82__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870((*v77__f), (int)(*v78__f));
                  g_eng.dsb_pan_5bb880((*v77__f), (*v82__f));
                  (*v73__f) = v40;
                  v42 = (*v77__f);
                  g_eng.dsb_freq_5bb890((*v77__f), (*v73__f));
                  g_eng.dsb_play_5bb8c0(v42);
                }
              }
            }
            if ( F32(a2, 108) != D(0x660d88) )
            {
              F32(a2, 108) = D(0x660d88);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
          }
          goto LABEL_76;
        case 2:
          v45 = F32(a2, 92) + 1;
          (*v80__f) = (_DWORD *)v35;
          F32(a2, 92) = v45;
          (*v81__f) = v34 >= v45 ? 20000 : 64000;
          if ( v45 > 10 )
          {
            v46 = F32(a2, 76);
            (*i__f) = F32(v46, 740);
            v47 = rnd();
            v48 = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v46), (intptr_t)(1000), (intptr_t)(1000 * v47 / 0x8000));
            v49 = F32(v46, 176);
            (*v78__f) = v48;
            F32(v79, 0) = v49;
            v50 = W_char_mag_sum((intptr_t)(c));
            v51 = F32(v46, 744);
            (*v77__f) = (F32(v46, 180) + v50);
            (*v82__f) = 0;
            (*v83__f) = 0;
            (*v84__f) = 0;
            if ( v51 == 0 && F32(v46, 732) == 0 && F32(v79, 0) == 0 )
            {
              v52 = (_DWORD *)F32(c, 0x750);
              v53 = v52[74];
              F32(v79, 0) = v52[73];
              v54 = v52[75];
              (*v77__f) = v53;
              v55 = v52[76];
              (*v82__f) = v54;
              v56 = v52[77];
              (*v83__f) = v55;
              (*v84__f) = v56;
            }
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)(2000), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((int)(*i__f)), (intptr_t)(F32(v79, 0)), (intptr_t)((int)(*v77__f)), (intptr_t)(0), (intptr_t)(4), (intptr_t)((int)(*v78__f)), (intptr_t)(1), (intptr_t)(F32(v46, 736)), (intptr_t)(F32(a2, 96)), (intptr_t)(0), (intptr_t)(F32(v46, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v82__f)), (intptr_t)((*v83__f)), (intptr_t)((*v84__f)));
            if ( F32(c, 0xc7f4) != 0 )
            {
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)(F32(c, 0xc7f8) + 2000), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((int)(*i__f)), (intptr_t)(F32(v79, 0)), (intptr_t)((int)(*v77__f)), (intptr_t)(0), (intptr_t)(4), (intptr_t)((int)(*v78__f)), (intptr_t)(1), (intptr_t)(F32(v46, 736)), (intptr_t)(F32(a2, 96)), (intptr_t)(0), (intptr_t)(F32(v46, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v82__f)), (intptr_t)((*v83__f)), (intptr_t)((*v84__f)));
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)(2000 - F32(c, 0xc7f8)), (intptr_t)(2000), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((int)(*i__f)), (intptr_t)(F32(v79, 0)), (intptr_t)((int)(*v77__f)), (intptr_t)(0), (intptr_t)(4), (intptr_t)((int)(*v78__f)), (intptr_t)(1), (intptr_t)(F32(v46, 736)), (intptr_t)(F32(a2, 96)), (intptr_t)(0), (intptr_t)(F32(v46, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v82__f)), (intptr_t)((*v83__f)), (intptr_t)((*v84__f)));
            }
          }
          if ( F32(a2, 92) > 24 || F16(a2, 90) >= 3u || F32(c, 0xc798) != 0 )
          {
            ++F16(a2, 88);
            F32(a2, 92) = 0;
          }
          if ( F32(a2, 92) % 3 == 0 && W_entity_sprite_descs((intptr_t)(c), (intptr_t)(a2), (intptr_t)((int)v85), (intptr_t)(v79)) != 0 )
          {
            for ( (*i__f) = 0; (uint16_t)(*i__f) < v79[0]; (*i__f) = ((uint8_t *)(*i__f) + 1) )
            {
              v57 = 10 * (uint16_t)(*i__f);
              (*v78__f) = W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100082), (intptr_t)((_DWORD *)a2), (intptr_t)(F32(a2, 4) + 100 * (F32(a2, 64) + v85[v57 / 2])), (intptr_t)(F32(a2, 8) + 100 * (F32(a2, 68) + v85[v57 / 2 + 1])), (intptr_t)(99), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
              if ( (*v78__f) != 0 )
              {
                W_entity_track_on((intptr_t)((*v78__f)), (intptr_t)(0), (intptr_t)(v86[v57]), (intptr_t)(v86[v57 + 1]));
                v58 = (*v78__f);
                W_particle_tint_set((intptr_t)((*v78__f)), (intptr_t)(16));
                F32(v58, 0x260) = 10;
              }
            }
          }
          goto LABEL_76;
        case 3:
          v59 = F32(a2, 92);
          (*v80__f) = (_DWORD *)-48000;
          v60 = v59 + 1;
          F32(a2, 92) = v60;
          if ( F32(c, 0xc798) != 0 )
          {
            if ( F32(a2, 108) != D(0x660d88) )
            {
              F32(a2, 108) = D(0x660d88);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
            (*v81__f) = 64000;
          }
          else
          {
            if ( F32(a2, 108) != D(0x65ff40) )
            {
              F32(a2, 108) = D(0x65ff40);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
            (*v81__f) = 16000;
          }
          if ( v60 <= 30 )
            goto LABEL_76;
          F16(a2, 72) = 0;
          F16(a2, 86) = 210;
          F16(a2, 88) = 0;
          F32(a2, 92) = 0;
          F32(a2, 120) = 0;
          F16(a2, 84) = 0;
          F32(a2, 76) = 0;
          F32(a2, 80) = 0;
          F16(a2, 90) = 0;
          F16(a2, 100) = 0;
          v61 = F32(c, 0xc798);
          if ( v61 == 0 )
            return (int)(*v78__f);
          ih_44e1c0(v61, a3, 0, 0, 3, 1);
          result = (int)(*v78__f);
          break;
        default:
LABEL_76:
          v43 = W_speed_ramp((intptr_t)(F32(a2, 40)), (intptr_t)(3200), (intptr_t)(-1600), (intptr_t)((*v81__f)));
          (*v74__f) = (*v80__f);
          (*v72__f) = F32(a2, 24);
          F32(a2, 40) = v43;
          F32(a2, 24) = W_speed_ramp((intptr_t)((*v72__f)), (intptr_t)(4000), (intptr_t)(-1600), (intptr_t)((int)(*v74__f)));
          W_char_drift_move((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          v44 = F32(c, 0xc798);
          if ( v44 != 0 )
          {
            ih_44e1c0(v44, a3, 800, 3200, 3, 0);
            *a9 = 1;
          }
          (*v78__f) = 1;
          return 1;
      }
      return result;
    case '&':
      if ( a3 == 0 )
      {
        v66 = F32(c, 0x15b54);
        if ( v66 != 0 )
          W_voiceset_cut((intptr_t)(v66));
      }
      if ( F16(a2, 88) != 0 )
      {
        if ( F16(a2, 88) != 1 )
          return (int)(*v78__f);
      }
      else
      {
        if ( F32(a2, 108) != D(0x661898) )
        {
          F32(a2, 108) = D(0x661898);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        F16(a2, 88) = 1;
      }
      if ( F8(a2, 116) == 0 )
        return (int)(*v78__f);
      W_body_action_reset((intptr_t)(a2));
      return (int)(*v78__f);
    case '\'':
      SETHI(v23, 0);
      *a4 = 3;
      SETLO(v23, F16(a2, 88));
      F8(a2, 124) = 1;
      F8(a2, 125) = 1;
      if ( (uint16_t)v23 != 0 )
      {
        if ( v23 != 1 )
          goto LABEL_58;
      }
      else
      {
        if ( a3 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)((int *)&(*v80__f)), (intptr_t)((int *)&(*v78__f)), (intptr_t)(&(*i__f))) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0xCu), (intptr_t)(1), (intptr_t)((int)(*v80__f)), (intptr_t)((int)(*v78__f)), (intptr_t)((int)(*i__f)), (intptr_t)(0));
        }
        if ( F32(a2, 108) != D(0x661a30) )
        {
          F32(a2, 108) = D(0x661a30);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        ++F16(a2, 88);
        F32(a2, 24) = 0;
        F32(a2, 40) = 0;
        for ( j = 40; j != 0; --j )
        {
          (*v70__f) = 4000 * rnd() / 0x8000 - 2000;
          v25 = rnd();
          W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100124), (intptr_t)((_DWORD *)a2), (intptr_t)(1600 * v25 / 0x8000 - 800), (intptr_t)((*v70__f)), (intptr_t)(99), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        }
      }
      v26 = F32(a2, 24);
      if ( v26 <= -10000 )
      {
        if ( v26 < -10000 )
          v26 = v26 + 500 > 0 ? 0 : v26 + 500;
      }
      else
      {
        v26 -= 500;
        if ( v26 < -10000 )
          v26 = -10000;
      }
      F32(a2, 24) = v26;
      v27 = F32(a2, 92);
      if ( v27 >= 100 )
      {
        F16(a2, 72) = 0;
        F16(a2, 86) = 210;
        F16(a2, 88) = 0;
        F32(a2, 92) = 0;
        F32(a2, 120) = 0;
        F16(a2, 84) = 0;
        F32(a2, 76) = 0;
        F32(a2, 80) = 0;
        F16(a2, 90) = 0;
        F16(a2, 100) = 0;
        if ( F32(c, 0x15904) != 0 )
        {
          if ( a3 != 0 )
            F8(a2, 0) = 0;
          else
            W_char_release((intptr_t)(c));
        }
        else
        {
          W_body_action_reset((intptr_t)(a2));
          W_body_action_start((intptr_t)(a2), (intptr_t)(122), (intptr_t)(800));
        }
      }
      else
      {
        F32(a2, 92) = v27 + 1;
        if ( a3 != 0 )
          goto LABEL_58;
        v28 = 20 * (100 - (v27 + 1));
        if ( v28 / 50 < 0 )
        {
          v29 = 0;
LABEL_50:
          v30 = SYS32(4 * v29 + 10152);
          F32(c, 0xf4) = v30;
          F32(c, 0xf8) = v30;
          goto LABEL_58;
        }
        v29 = v28 / 50;
        if ( v28 / 50 < 20 )
          goto LABEL_50;
        F32(c, 0xf4) = 0;
        F32(c, 0xf8) = 0;
      }
LABEL_58:
      v31 = F32(a2, 8);
      v32 = (int)((uint64_t)(1374389535LL * F32(a2, 24)) >> 32) >> 5;
      (*v78__f) = 1;
      F32(a2, 8) = (v32 >> 31) + v32 + v31;
      return (int)(*v78__f);
    case '(':
    case ')':
    case '*':
      if ( W_skill_charge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(a7), (intptr_t)((_DWORD *)a8), (intptr_t)(F32(a2, 76)), (intptr_t)(D(0x660f20)), (intptr_t)(0x46u), (intptr_t)(38)) != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          W_voice_cue((intptr_t)(c), (intptr_t)(a3), (intptr_t)(0xAu), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
        }
        else if ( F16(a2, 88) == 6 )
        {
          v67 = F16(a2, 72);
          if ( (uint16_t)v67 == 40 )
          {
            W_item_effect((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(F32(a2, 80)), (intptr_t)(F32(a2, 76)), (intptr_t)(100150), (intptr_t)(30));
          }
          else if ( (uint16_t)v67 > 0x28u && F16(a2, 72) <= 0x2Au )
          {
            v68 = 0;
            for ( (*i__f) = 8; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
            {
              W_char_shot_fire((intptr_t)(c), (intptr_t)(100035), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(-1600), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(v68), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(F32(a2, 44)), (intptr_t)(0), (intptr_t)(2));
              W_char_shot_fire((intptr_t)(c), (intptr_t)(100035), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(-1600), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(40), (intptr_t)(v68 + 23), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(F32(a2, 44)), (intptr_t)(0), (intptr_t)(2));
              v68 += 45;
            }
          }
        }
      }
      if ( F16(a2, 88) < 6u && (F32(c, 0x204) & 0x70000) == 0 )
      {
        (*v71__f) = 800 * rnd() / 0x8000 - 800;
        v69 = rnd();
        W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100130), (intptr_t)((_DWORD *)a2), (intptr_t)(3200 * v69 / 0x8000 - 1600), (intptr_t)((*v71__f)), (intptr_t)(99), (intptr_t)(3), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      }
      if ( F32(a2, 24) > 0 )
LABEL_144:
        F32(a2, 28) = 4000;
      return (int)(*v78__f);
    default:
      return (int)(*v78__f);
  }
}

/* 0x450810: int __thiscall sub_450810(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7) */
int32_t ih_450810(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    intptr_t result = 0;
    int16_t v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
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
    int32_t * v36 = 0;
    int32_t * v37 = 0;
    intptr_t v38 = 0;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x24 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x1c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x18 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x18 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x18 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x14 */

  v9 = 0;
  result = 0;
  if ( F16(a2, 72) == 33 )
  {
    *a4 = 3;
    if ( W_slash_step((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)((int)D(0x661ef8)), (intptr_t)(1600), (intptr_t)(-1600), (intptr_t)(0), (intptr_t)(a6)) != 0 )
    {
      if ( F16(a2, 88) == 1 )
      {
        (*v34__f) = 1000 * rnd() / 0x8000;
        W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(159), (intptr_t)(0), (intptr_t)((*v34__f)), (intptr_t)(0));
      }
      else if ( F16(a2, 88) == 3 )
      {
        (*v33__f) = 1000 * rnd() / 0x8000;
        W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(149), (intptr_t)(0), (intptr_t)((*v33__f)), (intptr_t)(0));
      }
    }
    if ( F16(a2, 90) == 0 && F16(a2, 88) == 3 )
      W_char_attack_hits((intptr_t)(c), (intptr_t)(a2), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(4000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(1));
    return 1;
  }
  if ( F16(a2, 72) == 34 )
  {
    *a4 = 3;
    v27 = 0;
    switch ( F16(a2, 88) )
    {
      case 0:
        W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x661ef8)));
        v27 = 1;
        F16(a2, 88) = 1;
        goto LABEL_45;
      case 1:
LABEL_45:
        if ( F8(a2, 116) == 0 )
          goto LABEL_53;
        W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x66208c)));
        F16(a2, 88) = 2;
        goto LABEL_54;
      case 2:
        if ( F8(a2, 116) != 0 )
        {
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x662220)));
          W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(1600), (intptr_t)(a3), (intptr_t)(1));
          F16(a2, 88) = 3;
        }
        else
        {
LABEL_53:
          if ( v27 == 0 )
            goto LABEL_58;
        }
LABEL_54:
        if ( F16(a2, 88) == 1 )
        {
          v28 = rnd();
          W_sfx_play_at((intptr_t)(c), (intptr_t)(a3), (intptr_t)(159), (intptr_t)(0), (intptr_t)(1000 * v28 / 0x8000 - 1000), (intptr_t)(0));
        }
        else if ( F16(a2, 88) == 3 )
        {
          W_char_shot_fire((intptr_t)(c), (intptr_t)(100025), (intptr_t)(a2), (intptr_t)(1600), (intptr_t)(2400), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(10), (intptr_t)(90), (intptr_t)(F32(a2, 76)), (intptr_t)(1000), (intptr_t)(F32(a2, 44)), (intptr_t)(0), (intptr_t)(2));
          return 1;
        }
LABEL_58:
        result = 1;
        break;
      case 3:
        if ( F8(a2, 116) == 0 )
          goto LABEL_58;
        W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x6623b4)));
        W_char_lunge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(-1600), (intptr_t)(a3), (intptr_t)(1));
        F16(a2, 88) = 4;
        goto LABEL_54;
      case 4:
        if ( F8(a2, 116) == 0 )
          goto LABEL_58;
        W_body_action_reset((intptr_t)(a2));
        F16(a2, 88) = 5;
        goto LABEL_54;
      default:
        goto LABEL_58;
    }
    return result;
  }
  if ( F16(a2, 72) != 35 )
    return result;
  *a4 = 3;
  v11 = F16(a2, 88);
  F8(a2, 124) = 1;
  if ( v11 == 0 )
  {
    if ( F32(a2, 108) != D(0x661bc8) )
    {
      F32(a2, 108) = D(0x661bc8);
      F8(a2, 116) = 0;
      F16(a2, 114) = 0;
      F16(a2, 112) = 0;
    }
    F32(a2, 24) = 0;
    F16(a2, 88) = v11 + 1;
    F32(a2, 40) = 0;
  }
  else if ( v11 != 1 )
  {
LABEL_21:
    v19 = F32(a2, 40);
    v20 = v19 / 100;
    v21 = v19 / 100;
    if ( F32(a2, 44) == 3 )
      v21 = v19 / -100;
    if ( W_entity_move_horizontal((intptr_t)(c), (intptr_t)((_DWORD *)a2), (intptr_t)(v21), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 && a3 == 0 )
      F32(c, 0x14834) -= v20;
    v22 = 2000 * rnd() / 0x8000 - 1000;
    if ( a3 == 0 )
    {
      v23 = F32(c, 0x1b0);
      if ( v23 != 0 )
      {
        if ( F16(c, 0x1b4) > 2u )
        {
          if ( F32(v23, 104) == 0 )
            W_entity_snd_start((intptr_t)(c), (intptr_t)(2u), (intptr_t)(0), (intptr_t)(0x9Eu), (intptr_t)(0), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(0));
          W_entity_snd_keep((intptr_t)(c), (intptr_t)(2u), (intptr_t)(0));
          v24 = F32(v23, 104) + 1;
          F32(v23, 104) = v24;
          if ( v24 >= 30 )
            F32(v23, 104) = 0;
        }
        else
        {
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
        }
      }
      v25 = F32(c, 0x1b0);
      if ( v25 != 0 )
      {
        if ( F16(c, 0x1b4) > 3u )
        {
          if ( F32(v25, 140) == 0 )
            W_entity_snd_start((intptr_t)(c), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0x9Du), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          W_entity_snd_keep((intptr_t)(c), (intptr_t)(3u), (intptr_t)(0));
          v26 = F32(v25, 140) + 1;
          F32(v25, 140) = v26;
          if ( v26 >= 30 )
          {
            F32(v25, 140) = 0;
            return 1;
          }
        }
        else
        {
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85525c)), D(0x8a9b6c));
        }
      }
    }
    return 1;
  }
  if ( F32(a2, 92) % 4 == 0 )
  {
    (*v29__f) = 1600 - 3200 * rnd() / 0x8000;
    v12 = rnd();
    W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100047), (intptr_t)((_DWORD *)a2), (intptr_t)(2000 - 4000 * v12 / 0x8000), (intptr_t)((*v29__f)), (intptr_t)(99), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  }
  if ( F32(a2, 92) % 20 == 10 )
  {
    v13 = F32(a2, 96);
    v14 = F32(a2, 76);
    v37 = F32(a2, 44);
    F8(a2, 127) = 1;
    v36 = v13;
    v15 = rnd();
    v38 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v14), (intptr_t)(1000), (intptr_t)(1000 * v15 / 0x8000));
    if ( v14[44] == 7 )
    {
      (*v35__f) = v14[180];
      v16 = rnd();
      v9 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v14[179]), (intptr_t)(1000 * v16 / 0x8000), (intptr_t)((*v35__f)));
    }
    (*v32__f) = F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228);
    (*v31__f) = F32(c, 0x1f0);
    (*v30__f) = v14[45] + W_char_mag_sum((intptr_t)(c));
    W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100026), (intptr_t)((_DWORD *)a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(4), (intptr_t)(a3), (intptr_t)(10), (intptr_t)(v38), (intptr_t)(180), (intptr_t)((int)v36), (intptr_t)((int)v37), (intptr_t)(v14[44]), (intptr_t)((*v30__f)), (intptr_t)(v9), (intptr_t)((*v31__f)), (intptr_t)((*v32__f)), (intptr_t)(4));
  }
  v17 = F32(a2, 40) + 1000;
  if ( v17 > 24000 )
    v17 = 24000;
  F32(a2, 40) = v17;
  v18 = F32(a2, 92);
  if ( v18 >= 160 )
  {
    F16(a2, 72) = 0;
    F16(a2, 86) = 210;
    F16(a2, 88) = 0;
    F32(a2, 92) = 0;
    F32(a2, 120) = 0;
    F16(a2, 84) = 0;
    F32(a2, 76) = 0;
    F32(a2, 80) = 0;
    F16(a2, 90) = 0;
    F16(a2, 100) = 0;
  }
  else
  {
    F32(a2, 92) = v18 + 1;
  }
  goto LABEL_21;
}

/* 0x4516e0: int __thiscall sub_4516E0(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7, int a8) */
int32_t ih_4516e0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
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
    int32_t * v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x4 */

  v8 = a2;
  if ( F16(a2, 72) != 18 )
  {
    if ( F16(a2, 72) == 27 )
    {
      v25 = a6;
      *a4 = 2;
      if ( F16(v8, 88) == 0 )
        W_body_pose_swap((intptr_t)(c), (intptr_t)(v8), (intptr_t)(v25), (intptr_t)(0x1Bu), (intptr_t)(0), (intptr_t)(0x1Bu), (intptr_t)(6), (intptr_t)(0));
      v26 = F16(v8, 84);
      if ( (uint16_t)v26 != 0 )
      {
        v27 = v26 - 1;
        if ( v27 != 0 )
        {
          if ( v27 == 5 )
          {
            (*v41__f) = v25;
            v28 = a3;
            if ( W_slash_step((intptr_t)(c), (intptr_t)(v8), (intptr_t)(a3), (intptr_t)((int)D(0x64d578)), (intptr_t)(2800), (intptr_t)(-2800), (intptr_t)(1), (intptr_t)((*v41__f))) != 0 )
            {
              if ( F16(v8, 88) == 1 )
              {
                W_voice_cue((intptr_t)(c), (intptr_t)(v28), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
              }
              else if ( F16(v8, 88) == 3 )
              {
                v29 = rnd();
                W_sfx_play_at((intptr_t)(c), (intptr_t)(v28), (intptr_t)(69), (intptr_t)(0), (intptr_t)(1000 * v29 / 0x8000 - 1000), (intptr_t)(0));
              }
            }
            if ( F16(v8, 90) < 2u && F16(v8, 88) == 3 )
            {
              W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(6400), (intptr_t)(5800), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(v28), (intptr_t)(F32(v8, 76)), (intptr_t)(1200), (intptr_t)(1));
              return 0;
            }
          }
        }
        else
        {
          v30 = a3;
          if ( W_slash_step((intptr_t)(c), (intptr_t)(v8), (intptr_t)(a3), (intptr_t)((int)D(0x64dbc8)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v25)) != 0 )
          {
            if ( F16(v8, 88) == 1 )
            {
              W_voice_cue((intptr_t)(c), (intptr_t)(v30), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
            }
            else if ( F16(v8, 88) == 3 )
            {
              v31 = 1000 * rnd() / 0x8000 - 1000;
              a4 = (_DWORD *)F32(c, 0x40);
              if ( v30 == 0 )
              {
                v32 = (int *)W_vpool_take((intptr_t)(SYS32(1144)), (intptr_t)(0));
                if ( v32 != 0 )
                {
                  a3 = *v32;
                  if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(&a2), (intptr_t)(&a6)) != 0 )
                  {
                    g_eng.dsb_volume_5bb870(a3, a2);
                    g_eng.dsb_pan_5bb880(a3, a6);
                    (*v42__f) = v31;
                    v33 = a3;
                    g_eng.dsb_freq_5bb890(a3, (*v42__f));
                    g_eng.dsb_play_5bb8c0(v33);
                  }
                }
              }
            }
          }
          if ( F16(v8, 90) < 2u && F16(v8, 88) == 3 )
          {
            W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(8000), (intptr_t)(-5200), (intptr_t)(3200), (intptr_t)(3200), (intptr_t)(2), (intptr_t)(v30), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(1));
            return 0;
          }
        }
      }
      else
      {
        (*v43__f) = v25;
        v34 = a3;
        if ( W_slash_step((intptr_t)(c), (intptr_t)(v8), (intptr_t)(a3), (intptr_t)((int)D(0x64cf28)), (intptr_t)(2800), (intptr_t)(-2800), (intptr_t)(0), (intptr_t)((*v43__f))) != 0 )
        {
          if ( F16(v8, 88) == 1 )
          {
            W_voice_cue((intptr_t)(c), (intptr_t)(v34), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
          }
          else if ( F16(v8, 88) == 3 )
          {
            v35 = rnd();
            W_sfx_play_at((intptr_t)(c), (intptr_t)(v34), (intptr_t)(69), (intptr_t)(0), (intptr_t)(1000 * v35 / 0x8000 - 1000), (intptr_t)(0));
          }
        }
        if ( F16(v8, 90) < 2u )
        {
          if ( F16(v8, 88) == 2 )
          {
            W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(6400), (intptr_t)(3600), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(v34), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(1));
            return 0;
          }
          if ( F16(v8, 88) == 3 )
          {
            W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(8000), (intptr_t)(3600), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(v34), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(1));
            return 0;
          }
        }
      }
    }
    else if ( F16(a2, 72) == 28 )
    {
      v10 = 0;
      switch ( F16(a2, 88) )
      {
        case 0:
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x64e218)));
          v10 = 1;
          F16(v8, 88) = 1;
          goto LABEL_6;
        case 1:
LABEL_6:
          if ( F8(v8, 116) == 0 )
            goto LABEL_14;
          W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x64e3ac)));
          F16(v8, 88) = 2;
          goto LABEL_15;
        case 2:
          if ( F8(a2, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x64e540)));
            W_char_lunge((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
            F16(v8, 88) = 3;
          }
          else
          {
LABEL_14:
            if ( v10 == 0 )
              goto LABEL_28;
          }
LABEL_15:
          if ( F16(v8, 88) == 1 )
          {
            if ( a3 == 0
              && F32(c, 0x15b54) != 0
              && F32(c, 0x15b5c) == 0
              && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
              && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)(&a2), (intptr_t)(&a4)) != 0 )
            {
              W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(0x44u), (intptr_t)(1), (intptr_t)(a6), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)(0));
            }
          }
          else if ( F16(v8, 88) == 3 )
          {
            v11 = rnd();
            v12 = (_DWORD *)F32(c, 0x40);
            v13 = 1000 * v11 / 0x8000 + 1000;
            if ( a3 == 0 )
            {
              v14 = (int *)W_vpool_take((intptr_t)(SYS32(1140)), (intptr_t)(0));
              if ( v14 != 0 )
              {
                a2 = *v14;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v12), (intptr_t)((int *)&a4), (intptr_t)(&a6)) != 0 )
                {
                  v15 = a2;
                  g_eng.dsb_volume_5bb870(a2, (int)a4);
                  g_eng.dsb_pan_5bb880(v15, a6);
                  g_eng.dsb_freq_5bb890(v15, v13);
                  g_eng.dsb_play_5bb8c0(v15);
                }
              }
            }
          }
LABEL_28:
          if ( F16(v8, 90) >= 2u )
            return 0;
          if ( F16(v8, 88) != 2 )
          {
            if ( F16(v8, 88) != 3 )
              return 0;
            W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(5600), (intptr_t)(3600), (intptr_t)(4800), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(11));
            return 0;
          }
          v17 = F32(v8, 76);
          (*v46__f) = F32(v17, 740);
          v18 = rnd();
          (*v45__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v17), (intptr_t)(1000), (intptr_t)(1000 * v18 / 0x8000));
          a4 = F32(v17, 176);
          v19 = W_char_mag_sum((intptr_t)(c));
          v20 = F32(v17, 744);
          v21 = F32(v17, 180) + v19;
          (*v44__f) = 0;
          a6 = 0;
          a2 = 0;
          if ( v20 == 0 && F32(v17, 732) == 0 && a4 == 0 )
          {
            v22 = F32(c, 0x750);
            v21 = F32(v22, 296);
            a4 = F32(v22, 292);
            (*v44__f) = F32(v22, 300);
            v23 = F32(v22, 304);
            v24 = F32(v22, 308);
            a6 = v23;
            a2 = v24;
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v8), (intptr_t)(2000), (intptr_t)(-2000), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v46__f)), (intptr_t)((int)a4), (intptr_t)(v21), (intptr_t)(0), (intptr_t)(11), (intptr_t)((*v45__f)), (intptr_t)(1), (intptr_t)(F32(v17, 736)), (intptr_t)(F32(v8, 96)), (intptr_t)(0), (intptr_t)(F32(v17, 756)), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v44__f)), (intptr_t)(a6), (intptr_t)(a2));
          if ( F32(c, 0xc7f4) != 0 )
          {
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v8), (intptr_t)(F32(c, 0xc7f8) + 2000), (intptr_t)(-2000), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v46__f)), (intptr_t)((int)a4), (intptr_t)(v21), (intptr_t)(0), (intptr_t)(11), (intptr_t)((*v45__f)), (intptr_t)(1), (intptr_t)(F32(v17, 736)), (intptr_t)(F32(v8, 96)), (intptr_t)(0), (intptr_t)(F32(v17, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v44__f)), (intptr_t)(a6), (intptr_t)(a2));
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v8), (intptr_t)(2000 - F32(c, 0xc7f8)), (intptr_t)(-2000), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v46__f)), (intptr_t)((int)a4), (intptr_t)(v21), (intptr_t)(0), (intptr_t)(11), (intptr_t)((*v45__f)), (intptr_t)(1), (intptr_t)(F32(v17, 736)), (intptr_t)(F32(v8, 96)), (intptr_t)(0), (intptr_t)(F32(v17, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v44__f)), (intptr_t)(a6), (intptr_t)(a2));
            return 0;
          }
          break;
        case 3:
          if ( F8(a2, 116) == 0 )
            goto LABEL_28;
          W_body_set_clip((intptr_t)(a2), (intptr_t)((int)D(0x64e6d4)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
          F16(v8, 88) = 4;
          goto LABEL_15;
        case 4:
          if ( F8(a2, 116) == 0 )
            goto LABEL_28;
          W_body_action_reset((intptr_t)(a2));
          F16(v8, 88) = 5;
          goto LABEL_15;
        default:
          goto LABEL_28;
      }
    }
    return 0;
  }
  SETHI(v36, 0);
  *a4 = 3;
  SETLO(v36, F16(v8, 88));
  F8(v8, 124) = 1;
  F8(v8, 125) = 1;
  if ( (uint16_t)v36 == 0 )
  {
    if ( F32(v8, 108) != D(0x64c280) )
    {
      F32(v8, 108) = D(0x64c280);
      F8(v8, 116) = 0;
      F16(v8, 114) = 0;
      F16(v8, 112) = 0;
    }
    W_entity_fx_add((intptr_t)(c), (intptr_t)(100155), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(0));
    ++F16(v8, 88);
LABEL_80:
    v40 = F32(v8, 92) + 1;
    F32(v8, 92) = v40;
    if ( v40 > 200 )
    {
      ++F16(v8, 88);
      F32(v8, 92) = 0;
      return 0;
    }
    return 0;
  }
  v37 = v36 - 1;
  if ( v37 == 0 )
    goto LABEL_80;
  if ( v37 == 1 )
  {
    v38 = F32(v8, 92) + 1;
    F32(v8, 92) = v38;
    if ( v38 < 40 )
    {
      v39 = W_tint_fade_a((intptr_t)(40 - v38), (intptr_t)(40));
      F32(c, 0xf4) = v39;
      F32(c, 0xf8) = v39;
      return 0;
    }
    if ( F32(c, 0x15904) == 0 )
    {
      W_body_action_reset((intptr_t)(v8));
      W_body_action_start((intptr_t)(v8), (intptr_t)(122), (intptr_t)(800));
      return 0;
    }
    if ( a3 == 0 )
    {
      W_char_release((intptr_t)(c));
      return 0;
    }
    F8(v8, 0) = 0;
  }
  return 0;
}

/* 0x452020: int __thiscall sub_452020(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t ih_452020(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t result = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    int32_t * v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x30 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x2c */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x28 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x24 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x4 */

  v8 = a2;
  switch ( F16(a2, 72) )
  {
    case 0x19:
      v10 = 0;
      F32(a4, 0) = 2;
      switch ( F16(v8, 88) )
      {
        case 0:
          if ( F32(v8, 108) != D(0x665cd0) )
          {
            F32(v8, 108) = D(0x665cd0);
            F8(v8, 116) = 0;
            F16(v8, 114) = 0;
            F16(v8, 112) = 0;
          }
          v10 = 1;
          F16(v8, 88) = 1;
          goto LABEL_6;
        case 1:
LABEL_6:
          if ( F8(v8, 116) == 0 )
            goto LABEL_14;
          W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x665e64)));
          F16(v8, 88) = 2;
          goto LABEL_15;
        case 2:
          if ( F8(v8, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x665ff8)));
            W_char_lunge((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
            F16(v8, 88) = 3;
          }
          else
          {
LABEL_14:
            if ( v10 == 0 )
              goto LABEL_20;
          }
LABEL_15:
          if ( F16(v8, 88) == 3 )
          {
            v11 = rnd();
            v12 = (_DWORD *)F32(c, 0x40);
            v13 = 1000 * v11 / 0x8000 - 1000;
            if ( a3 == 0 )
            {
              v14 = (int *)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
              if ( v14 != 0 )
              {
                a2 = *v14;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v12), (intptr_t)(&a4), (intptr_t)(&a6)) != 0 )
                {
                  v15 = a2;
                  g_eng.dsb_volume_5bb870(a2, a4);
                  g_eng.dsb_pan_5bb880(v15, a6);
                  g_eng.dsb_freq_5bb890(v15, v13);
                  g_eng.dsb_play_5bb8c0(v15);
                }
              }
            }
          }
LABEL_20:
          if ( F16(v8, 90) != 0 || F16(v8, 88) != 3 )
            return 0;
          v16 = F32(v8, 76);
          (*v37__f) = v16[185];
          v17 = rnd();
          v18 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v16), (intptr_t)(1000), (intptr_t)(1000 * v17 / 0x8000));
          v19 = v16[44];
          a8 = v18;
          a4 = v19;
          v20 = W_char_mag_sum((intptr_t)(c));
          v21 = v16[186];
          v22 = v16[45] + v20;
          a7 = 0;
          a6 = 0;
          a2 = 0;
          if ( v21 == 0 && v16[183] == 0 && a4 == 0 )
          {
            v23 = (int *)F32(c, 0x750);
            v22 = v23[74];
            a4 = v23[73];
            a7 = v23[75];
            v24 = v23[76];
            v25 = v23[77];
            a6 = v24;
            a2 = v25;
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v8), (intptr_t)(6400), (intptr_t)(1600), (intptr_t)(6400), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v37__f)), (intptr_t)(a4), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(3), (intptr_t)(a8), (intptr_t)(1), (intptr_t)(v16[184]), (intptr_t)(F32(v8, 96)), (intptr_t)(0), (intptr_t)(v16[189]), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)(a7), (intptr_t)(a6), (intptr_t)(a2));
          if ( F32(c, 0xc7f4) == 0 )
            return 0;
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v8), (intptr_t)(F32(c, 0xc7f8) + 6400), (intptr_t)(1600), (intptr_t)(6400), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v37__f)), (intptr_t)(a4), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(3), (intptr_t)(a8), (intptr_t)(1), (intptr_t)(v16[184]), (intptr_t)(F32(v8, 96)), (intptr_t)(0), (intptr_t)(v16[189]), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)(a7), (intptr_t)(a6), (intptr_t)(a2));
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v8), (intptr_t)(6400 - F32(c, 0xc7f8)), (intptr_t)(1600), (intptr_t)(6400), (intptr_t)(3200), (intptr_t)(0), (intptr_t)(a3), (intptr_t)((*v37__f)), (intptr_t)(a4), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(3), (intptr_t)(a8), (intptr_t)(1), (intptr_t)(v16[184]), (intptr_t)(F32(v8, 96)), (intptr_t)(0), (intptr_t)(v16[189]), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)(a7), (intptr_t)(a6), (intptr_t)(a2));
          result = 0;
          break;
        case 3:
          if ( F8(v8, 116) == 0 )
            goto LABEL_20;
          W_body_set_clip((intptr_t)(v8), (intptr_t)((int)D(0x66618c)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(a3), (intptr_t)(1));
          F16(v8, 88) = 4;
          goto LABEL_15;
        case 4:
          if ( F8(v8, 116) == 0 )
            goto LABEL_20;
          W_body_action_reset((intptr_t)(v8));
          F16(v8, 88) = 5;
          goto LABEL_15;
        default:
          goto LABEL_20;
      }
      return result;
    case 0x1A:
      v27 = a3;
      (*v36__f) = a6;
      (*v35__f) = a4;
      (*v34__f) = a3;
      (*v33__f) = a2;
      F32(a4, 0) = 2;
      if ( ih_461640((*v33__f), (*v34__f), (*v35__f), (*v36__f), (int)D(0x666c98), 20000, 64000) != 0 )
      {
        switch ( F16(v8, 88) )
        {
          case 2:
            v28 = 1000 * rnd();
            a4 = F32(c, 0x40);
            if ( v27 == 0 )
            {
              v29 = (int *)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
              if ( v29 != 0 )
              {
                a3 = *v29;
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)a4), (intptr_t)(&a2), (intptr_t)(&a6)) != 0 )
                {
                  g_eng.dsb_volume_5bb870(a3, a2);
                  g_eng.dsb_pan_5bb880(a3, a6);
                  v30 = a3;
                  g_eng.dsb_freq_5bb890(a3, v28 / 0x8000 - 1000);
                  g_eng.dsb_play_5bb8c0(v30);
                }
              }
            }
            break;
          case 3:
            W_entity_dust((intptr_t)(c), (intptr_t)(v8), (intptr_t)(v27), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
            W_sfx_play_at((intptr_t)(c), (intptr_t)(v27), (intptr_t)(71), (intptr_t)(0), (intptr_t)(1000), (intptr_t)(0));
            break;
          case 4:
            F32(v8, 24) = -32000;
            break;
          default:
            break;
        }
      }
      if ( F16(v8, 88) == 2 )
      {
        if ( F16(v8, 90) >= 2u )
          return 0;
        W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(400), (intptr_t)(5600), (intptr_t)(4000), (intptr_t)(1600), (intptr_t)(4), (intptr_t)(v27), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(5));
        return 0;
      }
      else
      {
        if ( F16(v8, 88) != 3 || F32(v8, 92) <= 40 || F16(v8, 90) >= 2u )
          return 0;
        W_char_attack_hits((intptr_t)(c), (intptr_t)(v8), (intptr_t)(1600), (intptr_t)(5600), (intptr_t)(4800), (intptr_t)(1600), (intptr_t)(4), (intptr_t)(v27), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(5));
        return 0;
      }
    case 0x83:
      a4 = 100019;
      goto LABEL_48;
    case 0x84:
      a4 = 100018;
      goto LABEL_48;
    case 0x85:
      a4 = 100020;
LABEL_48:
      v31 = a3;
      if ( W_skill_charge((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(a8), (intptr_t)((_DWORD *)a7), (intptr_t)(F32(a2, 76)), (intptr_t)(D(0x666320)), (intptr_t)(0x14u), (intptr_t)(0)) != 0
        && F16(v8, 88) == 6 )
      {
        W_sfx_play_at((intptr_t)(c), (intptr_t)(v31), (intptr_t)(235), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v32 = 20;
        do
        {
          W_char_shot_fire((intptr_t)(c), (intptr_t)(a4), (intptr_t)(v8), (intptr_t)(2000), (intptr_t)(0), (intptr_t)(1), (intptr_t)(v31), (intptr_t)(v32), (intptr_t)(90), (intptr_t)(F32(v8, 76)), (intptr_t)(1000), (intptr_t)(F32(v8, 44)), (intptr_t)(0), (intptr_t)(2));
          v32 += 10;
        }
        while ( (uint16_t)v32 < 0x64u );
      }
      break;
    default:
      return 0;
  }
  return 0;
}

/* 0x452730: int __thiscall sub_452730(cls_4022A0 *this, int a2, FILE *a3, _DWORD *a4, int a5, int a6) */
int32_t ih_452730(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
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
    intptr_t * v19 = 0;
    intptr_t v20 = 0;
    int32_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    int32_t * v24 = 0;
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
    intptr_t * v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    void * v45 = 0;
    intptr_t v46 = 0;
    int16_t v47 = 0;
    intptr_t v48 = 0;
    int32_t * v49 = 0;
    intptr_t * v50 = 0;
    intptr_t v51 = 0;
    int32_t * v52 = 0;
    int32_t * v53 = 0;
    int32_t * v54 = 0;
    intptr_t ** v55 = 0;
    intptr_t v56 = 0;
    intptr_t * v57 = 0;
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
    int32_t * v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    intptr_t v73 = 0;
    intptr_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    intptr_t v78 = 0;
    int32_t * v79 = 0;
    intptr_t v80 = 0;
    intptr_t v81 = 0;
    intptr_t v82 = 0;
    intptr_t v83 = 0;
    intptr_t v84 = 0;
    int32_t * v85 = 0;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v86__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v87__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x30 */
    intptr_t *const v88__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x2c */
    intptr_t *const v89__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x28 */
    uint16_t *const v90__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x14 */
    intptr_t *const v92__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x14 */
    intptr_t *const v91__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x14 */
    intptr_t *const v94__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x10 */
    intptr_t *const v93__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x10 */
    intptr_t *const v95__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0xc */
    intptr_t *const v96__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x8 */
    intptr_t *const v97__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x4 */

  v6 = a2;
  v7 = F16(a2, 72);
  (*v97__f) = 0;
  switch ( v7 )
  {
    case 18:
      *a4 = 3;
      v9 = F16(v6, 88);
      F8(v6, 124) = 1;
      F8(v6, 125) = 1;
      if ( (uint16_t)v9 == 0 )
      {
        if ( F32(v6, 108) != D(0x65e450) )
        {
          F32(v6, 108) = D(0x65e450);
          F8(v6, 116) = 0;
          F16(v6, 114) = 0;
          F16(v6, 112) = 0;
        }
        W_entity_fx_add((intptr_t)(c), (intptr_t)(100155), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)(0));
        ++F16(v6, 88);
LABEL_10:
        v14 = F32(v6, 92) + 1;
        F32(v6, 92) = v14;
        if ( v14 > 200 )
        {
          ++F16(v6, 88);
          F32(v6, 92) = 0;
          (*v97__f) = 1;
          return 1;
        }
        goto LABEL_17;
      }
      v10 = v9 - 1;
      if ( v10 == 0 )
        goto LABEL_10;
      if ( v10 == 1 )
      {
        v11 = F32(v6, 92) + 1;
        F32(v6, 92) = v11;
        if ( v11 < 40 )
        {
          v12 = W_tint_fade_a((intptr_t)(40 - v11), (intptr_t)(40));
          F32(c, 0xf4) = v12;
          F32(c, 0xf8) = v12;
          (*v97__f) = 1;
          return 1;
        }
        if ( F32(c, 0x15904) == 0 )
        {
          F16(v6, 72) = 0;
          F16(v6, 86) = 210;
          F16(v6, 88) = 0;
          F32(v6, 92) = 0;
          F32(v6, 120) = 0;
          F16(v6, 84) = 0;
          F32(v6, 76) = 0;
          F32(v6, 80) = 0;
          F16(v6, 90) = 0;
          F16(v6, 100) = 0;
          W_body_action_start((intptr_t)(v6), (intptr_t)(122), (intptr_t)(800));
          (*v97__f) = 1;
          return 1;
        }
        if ( a3 == 0 )
        {
          W_char_release((intptr_t)(c));
          (*v97__f) = 1;
          return 1;
        }
        F8(v6, 0) = 0;
      }
LABEL_17:
      (*v97__f) = 1;
      return 1;
    case 94:
    case 95:
      v43 = v7 - 94;
      if ( v43 != 0 )
      {
        if ( v43 == 1 )
        {
          a2 = 100053;
          (*v90__f) = 230;
          (*v96__f) = 0;
          (*v95__f) = 12;
        }
      }
      else
      {
        a2 = 100050;
        (*v90__f) = 83;
        (*v96__f) = 400;
        (*v95__f) = 8;
      }
      SETHI(v44, 0);
      *a4 = 3;
      SETLO(v44, F16(v6, 84));
      if ( (uint16_t)v44 == 1 )
      {
        v45 = D(0x65eab0);
        (*v94__f) = 40;
        a4 = D(0x65eab0);
        v46 = 200;
      }
      else
      {
        if ( (uint16_t)v44 == 2 )
        {
          v45 = D(0x65ef70);
          (*v94__f) = 40;
          a4 = D(0x65ef70);
        }
        else
        {
          v45 = D(0x65f430);
          (*v94__f) = 60;
          a4 = D(0x65f430);
        }
        v46 = 800;
      }
      v47 = F16(v6, 88);
      switch ( v47 )
      {
        case 0:
          if ( F32(v6, 108) != v45 )
          {
            F32(v6, 108) = v45;
            F8(v6, 116) = 0;
            F16(v6, 114) = 0;
            F16(v6, 112) = 0;
          }
          v48 = a3;
          F16(v6, 88) = v47 + 1;
          v49 = (_DWORD *)F32(c, 0x40);
          if ( v48 == 0 )
          {
            v50 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1696)), (intptr_t)(0));
            if ( v50 != 0 )
            {
              v51 = F32(v50, 0);
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) - 800), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v49), (intptr_t)(&a2), (intptr_t)(&(*v95__f))) != 0 )
              {
                g_eng.dsb_volume_5bb870(v51, a2);
                g_eng.dsb_pan_5bb880(v51, (*v95__f));
                g_eng.dsb_freq_5bb890(v51, 0);
                g_eng.dsb_play_5bb8c0(v51);
              }
            }
          }
          goto LABEL_87;
        case 1:
LABEL_87:
          if ( F8(v6, 116) != 0 )
          {
            v52 = a4;
            ++F16(v6, 88);
            v53 = v52 + 101;
            v54 = F32(v6, 108);
            F32(v6, 92) = 0;
            if ( v54 != v53 )
            {
              F32(v6, 108) = v53;
              F8(v6, 116) = 0;
              F16(v6, 114) = 0;
              F16(v6, 112) = 0;
            }
            if ( F16(v6, 84) != 2 && a3 == 0 && F32(c, 0x1b0) != 0 )
            {
              if ( F16(c, 0x1b4) > 2u )
              {
                if ( W_sfx_place((intptr_t)(c), (intptr_t)((*v96__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)F32(c, 0x40)), (intptr_t)(0), (intptr_t)(0)) != 0 )
                {
                  v55 = (intptr_t **)(F32(c, 0x1b0) + 72);
                  v56 = SYS32(4 * (*v90__f) + 868);
                  if ( F32(v55, 0) != 0 )
                    W_entity_snd_release((intptr_t)(c), (intptr_t)(2u), (intptr_t)(0), (intptr_t)(1));
                  v57 = (intptr_t *)W_vpool_take((intptr_t)(v56), (intptr_t)(1));
                  if ( v57 != 0 )
                  {
                    v58 = (*v96__f);
                    F32(v55, 0) = v57;
                    v55[1] = (intptr_t *)v56;
                    v55[2] = (intptr_t *)v58;
                    v55[3] = 0;
                    v55[4] = 0;
                    v55[5] = (intptr_t *)1;
                    v55[6] = 0;
                    v55[7] = 0;
                    g_eng.dsb_volume_5bb870(F32(v57, 0), -10000);
                    g_eng.dsb_play_5bb8c0(*v57);
                  }
                }
              }
              else
              {
                g_eng.h408b90(D(0x8a9530), ((char *)D(0x85525c)), 0, D(0x8a9b6c));
              }
            }
          }
          break;
        case 2:
          v59 = v44 - 1;
          if ( v59 != 0 )
          {
            if ( v59 == 1 )
            {
              if ( F16(v6, 72) == 94 )
              {
                if ( F32(v6, 92) == 0 )
                {
                  v69 = F32(v6, 96);
                  v70 = F32(v6, 76);
                  (*v96__f) = F32(v6, 44);
                  a2 = 0;
                  F8(v6, 127) = 1;
                  (*v95__f) = v69;
                  v71 = rnd();
                  (*v92__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)(v70), (intptr_t)(1000), (intptr_t)(1000 * v71 / 0x8000));
                  if ( v70[44] == 7 )
                  {
                    (*v89__f) = v70[180];
                    v72 = rnd();
                    a2 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v70[179]), (intptr_t)(1000 * v72 / 0x8000), (intptr_t)((*v89__f)));
                  }
                  (*v88__f) = F32(F32(c, 0x750), 216)
                      + F32(F32(c, 0x750), 224)
                      + F32(F32(c, 0x750), 228);
                  (*v87__f) = F32(c, 0x1f0);
                  (*v86__f) = a2;
                  v73 = W_char_mag_sum((intptr_t)(c));
                  v74 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100038), (intptr_t)((_DWORD *)v6), (intptr_t)(4400), (intptr_t)(v46 + 800), (intptr_t)(1), (intptr_t)((int)a3), (intptr_t)(5), (intptr_t)((*v92__f)), (intptr_t)(90), (intptr_t)((*v95__f)), (intptr_t)((*v96__f)), (intptr_t)(v70[44]), (intptr_t)(v70[45] + v73), (intptr_t)((*v86__f)), (intptr_t)((*v87__f)), (intptr_t)((*v88__f)), (intptr_t)(2));
                  if ( v74 != 0 )
                    F32(v74, 516) = 30 - F32(v6, 92);
                }
              }
              else if ( F16(v6, 72) == 95 && F32(v6, 92) == 0 )
              {
                v66 = v46 + 800;
                a2 = 0;
                v67 = 84;
                do
                {
                  v68 = W_char_shot_fire((intptr_t)(c), (intptr_t)(100039), (intptr_t)(v6), (intptr_t)(4400), (intptr_t)(v66), (intptr_t)(1), (intptr_t)((int)a3), (intptr_t)(5), (intptr_t)(v67), (intptr_t)(F32(v6, 76)), (intptr_t)(1000), (intptr_t)(F32(v6, 44)), (intptr_t)(0), (intptr_t)(2));
                  if ( v68 != 0 && (uint16_t)a2 != 0 )
                    F32(v68, 504) = 1;
                  v67 += 3;
                  ++a2;
                }
                while ( (uint16_t)a2 < 5u );
              }
            }
            else
            {
              if ( a3 == 0 )
              {
                v60 = 130 - 21 * rnd() / 0x8000;
                (*v96__f) = F32(v6, 44);
                if ( W_shot_4538f0((intptr_t)(*(_DWORD **)&GAP(W, 0x1030, 28)), (intptr_t)((_DWORD *)v6), (intptr_t)(16000)) )
                {
                  v61 = W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)((_DWORD *)v6), (intptr_t)(4400), (intptr_t)(v46), (intptr_t)((*v96__f)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(v60), (intptr_t)(10));
                  if ( v61 != 0 )
                    W_entity_scatter((intptr_t)(v61), (intptr_t)(80000), (intptr_t)(20000), (intptr_t)(0), (intptr_t)(0));
                }
                v62 = rnd();
                v63 = W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)((_DWORD *)v6), (intptr_t)(4400), (intptr_t)(v46), (intptr_t)(F32(v6, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(130 - 21 * v62 / 0x8000), (intptr_t)(0));
                if ( v63 != 0 )
                  W_entity_scatter((intptr_t)(v63), (intptr_t)(80000), (intptr_t)(20000), (intptr_t)(0), (intptr_t)(0));
              }
              v64 = F32(v6, 92);
              if ( v64 > 10 )
              {
                v65 = 20;
                if ( v64 > 20 )
                  v64 = 20;
                else
                  v65 = F32(v6, 92);
                W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(12000 * v64 / 20 + 2800), (intptr_t)(3200 * v65 / 20 + v46 + 1600), (intptr_t)(8000), (intptr_t)(8000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)(F32(v6, 76)), (intptr_t)(1000), (intptr_t)((*v95__f)));
              }
            }
          }
          else
          {
            if ( a3 == 0 )
            {
              v75 = 105 - 31 * rnd() / 0x8000;
              (*v96__f) = F32(v6, 44);
              if ( W_shot_4538f0((intptr_t)(*(_DWORD **)&GAP(W, 0x1030, 28)), (intptr_t)((_DWORD *)v6), (intptr_t)(16000)) )
              {
                v76 = W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)((_DWORD *)v6), (intptr_t)(4400), (intptr_t)(v46), (intptr_t)((*v96__f)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(v75), (intptr_t)(10));
                if ( v76 != 0 )
                {
                  v77 = F32(v76, 64);
                  F32(v77, 24) = 2000 * rnd() / 0x8000 - 1000;
                  F32(v77, 40) = 20000 * rnd() / 0x8000 + 70000;
                }
              }
              v78 = 105 - 31 * rnd() / 0x8000;
              v79 = *(_DWORD **)&GAP(W, 0x1030, 28);
              (*v96__f) = F32(v6, 44);
              if ( W_shot_4538f0((intptr_t)(v79), (intptr_t)((_DWORD *)v6), (intptr_t)(16000)) )
              {
                v80 = W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(a2), (intptr_t)((_DWORD *)v6), (intptr_t)(4400), (intptr_t)(v46), (intptr_t)((*v96__f)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(v78), (intptr_t)(0));
                if ( v80 != 0 )
                {
                  v81 = F32(v80, 64);
                  F32(v81, 24) = 2000 * rnd() / 0x8000 - 1000;
                  F32(v81, 40) = 20000 * rnd() / 0x8000 + 70000;
                }
              }
            }
            v82 = F32(v6, 92);
            if ( v82 > 10 )
            {
              v83 = 20;
              if ( v82 > 20 )
                v82 = 20;
              else
                v83 = F32(v6, 92);
              W_char_attack_hits((intptr_t)(c), (intptr_t)(v6), (intptr_t)(12000 * v82 / 20 + 2800), (intptr_t)(v46), (intptr_t)(12000 * v83 / 20), (intptr_t)(6400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)(F32(v6, 76)), (intptr_t)(900), (intptr_t)((*v95__f)));
            }
          }
          v84 = F32(v6, 92) + 1;
          F32(v6, 92) = v84;
          if ( v84 > (*v94__f) )
          {
            v85 = a4;
            ++F16(v6, 88);
            W_body_set_clip((intptr_t)(v6), (intptr_t)((int)(v85 + 202)));
          }
          W_entity_snd_keep((intptr_t)(c), (intptr_t)(2u), (intptr_t)(a3));
          break;
        case 3:
          if ( F8(v6, 116) != 0 )
          {
            W_entity_snd_release((intptr_t)(c), (intptr_t)(2u), (intptr_t)(a3), (intptr_t)(0));
            W_body_action_reset((intptr_t)(v6));
          }
          break;
        default:
          break;
      }
      if ( F32(v6, 24) > 0 )
        F32(v6, 28) = 4000;
      return (*v97__f);
    case 96:
      SETHI(v30, 0);
      *a4 = 3;
      SETLO(v30, F16(v6, 88));
      F8(v6, 124) = 1;
      if ( (uint16_t)v30 != 0 )
      {
        if ( v30 == 1 )
        {
          v31 = F32(v6, 92) + 1;
          F32(v6, 92) = v31;
          if ( v31 > 15 )
          {
            v32 = F32(v6, 40) + 2000;
            F32(v6, 40) = v32;
            if ( v32 <= 0 )
            {
              F32(v6, 40) = v32 + 2000;
              v33 = v32 + 2000;
            }
            else
            {
              v33 = 0;
            }
            v34 = F32(v6, 24);
            F32(v6, 40) = v33;
            v35 = v34 + 1500;
            F32(v6, 24) = v35;
            if ( v35 <= 0 )
            {
              v36 = v35 + 1500;
              F32(v6, 24) = v35 + 1500;
            }
            else
            {
              v36 = 0;
            }
            F32(v6, 24) = v36;
            if ( v33 >= 0 )
            {
              F16(v6, 86) = 210;
              F16(v6, 72) = 0;
              F16(v6, 88) = 0;
              F32(v6, 92) = 0;
              F32(v6, 120) = 0;
              F16(v6, 84) = 0;
              F32(v6, 76) = 0;
              F32(v6, 80) = 0;
              F16(v6, 90) = 0;
              F16(v6, 100) = 0;
              return (*v97__f);
            }
          }
        }
      }
      else
      {
        if ( F16(v6, 84) == 2 )
        {
          v37 = -F32(v6, 40);
          F32(v6, 44) = F32(v6, 44) != 3 ? 3 : 1;
          F32(v6, 40) = v37;
        }
        if ( F32(v6, 108) != D(0x65e918) )
        {
          F32(v6, 108) = D(0x65e918);
          F8(v6, 116) = 0;
          F16(v6, 114) = 0;
          F16(v6, 112) = 0;
        }
        v38 = rnd();
        a4 = (_DWORD *)F32(c, 0x40);
        v39 = 1000 * v38 / 0x8000 + 1000;
        if ( a3 == 0 )
        {
          v40 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1440)), (intptr_t)(0));
          if ( v40 != 0 )
          {
            v41 = F32(v40, 0);
            if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(&a2), (intptr_t)(&(*v96__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v41, a2);
              g_eng.dsb_pan_5bb880(v41, (*v96__f));
              g_eng.dsb_freq_5bb890(v41, v39);
              g_eng.dsb_play_5bb8c0(v41);
            }
          }
        }
        v42 = a3;
        ++F16(v6, 88);
        F32(v6, 40) = -80000;
        F32(v6, 24) = -30000;
        if ( v42 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a2), (intptr_t)((int *)&a4), (intptr_t)(&a3)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(a2), (intptr_t)((int)a4), (intptr_t)((int)a3), (intptr_t)(0));
          return (*v97__f);
        }
      }
      return (*v97__f);
    case 97:
      v15 = a3;
      v16 = 0;
      *a4 = 3;
      switch ( F16(v6, 88) )
      {
        case 0:
          if ( F32(v6, 108) != D(0x65f8f0) )
          {
            F32(v6, 108) = D(0x65f8f0);
            F8(v6, 116) = 0;
            F16(v6, 114) = 0;
            F16(v6, 112) = 0;
          }
          F16(v6, 88) = 1;
          v16 = 1;
          goto LABEL_22;
        case 1:
LABEL_22:
          if ( F8(v6, 116) == 0 )
            goto LABEL_30;
          W_body_set_clip((intptr_t)(v6), (intptr_t)((int)D(0x65fa84)));
          F16(v6, 88) = 2;
          goto LABEL_31;
        case 2:
          if ( F8(v6, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(v6), (intptr_t)((int)D(0x65fc18)));
            W_char_lunge((intptr_t)(c), (intptr_t)(v6), (intptr_t)(0), (intptr_t)((int)v15), (intptr_t)(1));
            F16(v6, 88) = 3;
          }
          else
          {
LABEL_30:
            if ( v16 == 0 )
              goto LABEL_36;
          }
LABEL_31:
          if ( F16(v6, 88) == 3 )
          {
            v17 = rnd();
            a4 = (_DWORD *)F32(c, 0x40);
            v18 = 1000 * v17 / 0x8000 - 500;
            if ( v15 == 0 )
            {
              v19 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
              if ( v19 != 0 )
              {
                v20 = F32(v19, 0);
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a4), (intptr_t)(&a2), (intptr_t)(&(*v96__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v20, a2);
                  g_eng.dsb_pan_5bb880(v20, (*v96__f));
                  g_eng.dsb_freq_5bb890(v20, v18);
                  g_eng.dsb_play_5bb8c0(v20);
                }
              }
            }
          }
LABEL_36:
          if ( F16(v6, 90) >= 2u || F16(v6, 88) != 3 )
            return (*v97__f);
          v21 = F32(v6, 76);
          (*v96__f) = v21[185];
          v22 = rnd();
          v23 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v21), (intptr_t)(1000), (intptr_t)(1000 * v22 / 0x8000));
          v24 = (_DWORD *)v21[44];
          v25 = F32(c, 0xc7e4);
          (*v95__f) = v23;
          v26 = F32(c, 0x750);
          a4 = v24;
          v27 = v21[186];
          v28 = v21[45] + F32(v26, 108) + F32(v26, 124) + F32(v26, 148) + v25;
          (*v93__f) = 0;
          (*v91__f) = 0;
          a2 = 0;
          if ( v27 == 0 && v21[183] == 0 && a4 == 0 )
          {
            v29 = F32(v26, 300);
            v28 = F32(v26, 296);
            a4 = F32(v26, 292);
            (*v93__f) = v29;
            (*v91__f) = F32(v26, 304);
            a2 = F32(v26, 308);
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v6), (intptr_t)(2400), (intptr_t)(3200), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v96__f)), (intptr_t)((int)a4), (intptr_t)(v28), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v95__f)), (intptr_t)(1), (intptr_t)(v21[184]), (intptr_t)(F32(v6, 96)), (intptr_t)(0), (intptr_t)(v21[189]), (intptr_t)(F32(v26, 216) + F32(v26, 224) + F32(v26, 228)), (intptr_t)((*v93__f)), (intptr_t)((*v91__f)), (intptr_t)(a2));
          if ( F32(c, 0xc7f4) == 0 )
            return (*v97__f);
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v6), (intptr_t)(F32(c, 0xc7f8) + 2400), (intptr_t)(3200), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v96__f)), (intptr_t)((int)a4), (intptr_t)(v28), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v95__f)), (intptr_t)(1), (intptr_t)(v21[184]), (intptr_t)(F32(v6, 96)), (intptr_t)(0), (intptr_t)(v21[189]), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v93__f)), (intptr_t)((*v91__f)), (intptr_t)(a2));
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v6), (intptr_t)(2400 - F32(c, 0xc7f8)), (intptr_t)(3200), (intptr_t)(2400), (intptr_t)(2400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v96__f)), (intptr_t)((int)a4), (intptr_t)(v28), (intptr_t)(0), (intptr_t)(3), (intptr_t)((*v95__f)), (intptr_t)(1), (intptr_t)(v21[184]), (intptr_t)(F32(v6, 96)), (intptr_t)(0), (intptr_t)(v21[189]), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v93__f)), (intptr_t)((*v91__f)), (intptr_t)(a2));
          result = (*v97__f);
          break;
        case 3:
          if ( F8(v6, 116) == 0 )
            goto LABEL_36;
          W_body_set_clip((intptr_t)(v6), (intptr_t)((int)D(0x65fdac)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v6), (intptr_t)(0), (intptr_t)((int)v15), (intptr_t)(1));
          F16(v6, 88) = 4;
          goto LABEL_31;
        case 4:
          if ( F8(v6, 116) == 0 )
            goto LABEL_36;
          W_body_action_reset((intptr_t)(v6));
          F16(v6, 88) = 5;
          goto LABEL_31;
        default:
          goto LABEL_36;
      }
      return result;
    default:
      return (*v97__f);
  }
}

/* 0x4539b0: int __thiscall sub_4539B0(cls_4022A0 *this, int a2, FILE *a3, int a4, _DWORD *a5, int a6, int a7) */
int32_t ih_4539b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    uint16_t v15 = 0;
    char * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t i = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    int32_t * v28 = 0;
    intptr_t v29 = 0;
    intptr_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t * v35 = 0;
    intptr_t v36 = 0;
    uint16_t v37 = 0;
    uint32_t v38 = 0;
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
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    uint16_t v56 = 0;
    uint32_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    int32_t * v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    int32_t * v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    int32_t * v73 = 0;
    intptr_t * v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    intptr_t * v78 = 0;
    intptr_t v79 = 0;
    char v80 = 0;
    intptr_t v81 = 0;
    intptr_t v82 = 0;
    intptr_t v83 = 0;
    uint16_t v84 = 0;
    uint32_t v85 = 0;
    intptr_t v86 = 0;
    intptr_t v87 = 0;
    intptr_t v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    intptr_t v91 = 0;
    int32_t * v92 = 0;
    intptr_t v93 = 0;
    intptr_t v94 = 0;
    int32_t * v95 = 0;
    intptr_t v96 = 0;
    intptr_t v97 = 0;
    intptr_t v98 = 0;
    intptr_t v99 = 0;
    uint16_t v100 = 0;
    intptr_t v101 = 0;
    intptr_t v102 = 0;
    intptr_t v103 = 0;
    intptr_t v104 = 0;
    intptr_t * v105 = 0;
    intptr_t v106 = 0;
    intptr_t v107 = 0;
    intptr_t v108 = 0;
    intptr_t v109 = 0;
    uint16_t v110 = 0;
    uint32_t v111 = 0;
    intptr_t v112 = 0;
    intptr_t v113 = 0;
    intptr_t v114 = 0;
    intptr_t v115 = 0;
    intptr_t v116 = 0;
    intptr_t v117 = 0;
    int32_t * v118 = 0;
    intptr_t v119 = 0;
    intptr_t v120 = 0;
    intptr_t v121 = 0;
    intptr_t v122 = 0;
    int32_t * v123 = 0;
    intptr_t j = 0;
    intptr_t v125 = 0;
    intptr_t v126 = 0;
    intptr_t v127 = 0;
    intptr_t v128 = 0;
    intptr_t v129 = 0;
    int32_t * v130 = 0;
    intptr_t v131 = 0;
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
    intptr_t v151 = 0;
    intptr_t v152 = 0;
    intptr_t v153 = 0;
    intptr_t v154 = 0;
    intptr_t v155 = 0;
    intptr_t v156 = 0;
    intptr_t v157 = 0;
    intptr_t v158 = 0;
    intptr_t v159 = 0;
    intptr_t v160 = 0;
    _Alignas(8) uint8_t frame_[0x44 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int32_t * *const v161__f = (int32_t * *)(frame_ + 0x0);   /* ebp-0x44 */
    intptr_t *const v165__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x40 */
    intptr_t *const v164__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x40 */
    intptr_t *const v163__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x40 */
    intptr_t *const v162__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x40 */
    intptr_t *const v167__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x3c */
    intptr_t *const v166__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x3c */
    intptr_t *const v169__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x20 */
    intptr_t *const v168__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x20 */
    intptr_t *const v173__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1c */
    intptr_t *const v172__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1c */
    intptr_t *const v171__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1c */
    intptr_t *const v170__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1c */
    intptr_t *const v177__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v176__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v175__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v174__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v181__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x14 */
    intptr_t *const v180__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x14 */
    intptr_t *const v179__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x14 */
    intptr_t *const v178__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x14 */
    intptr_t *const v185__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    intptr_t *const v184__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    intptr_t *const v183__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    intptr_t *const v182__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    intptr_t *const v189__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xc */
    intptr_t *const v188__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xc */
    intptr_t *const v187__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xc */
    intptr_t *const v186__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xc */
    intptr_t *const v190__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x8 */
    intptr_t *const v191__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x4 */

  v7 = a2;
  switch ( F16(a2, 72) )
  {
    case 2:
      (*v165__f) = a2;
      F32(a4, 0) = 2;
      ih_44a160((*v165__f), (int)D(0x667480), 3, 0);
      return 0;
    case 0x12:
      F32(a4, 0) = 3;
      v12 = F16(v7, 88);
      F8(v7, 124) = 1;
      F8(v7, 125) = 1;
      v13 = a3;
      switch ( v12 )
      {
        case 0:
          if ( F32(v7, 108) != D(0x667948) )
          {
            F32(v7, 108) = D(0x667948);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          if ( v13 != 0 )
            goto LABEL_29;
          W_entity_fx_remove((intptr_t)(c), (intptr_t)(100155));
          v14 = F16(c, 0x1ac);
          v15 = 0;
          if ( v14 == 0 )
            goto LABEL_29;
          break;
        case 1:
          goto LABEL_30;
        case 2:
          v18 = F32(c, 0x1594c);
          if ( v18 <= 0 )
            goto LABEL_44;
          if ( v18 > 2 )
          {
            if ( v18 == 3 )
            {
              if ( W_shot_445ce0((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)((uint16_t)a5)) )
              {
                F32(v7, 40) = 0;
                F32(v7, 24) = 0;
                goto LABEL_48;
              }
            }
            else
            {
LABEL_44:
              if ( F32(c, 0x15950) > 56000 )
                goto LABEL_48;
            }
          }
          v19 = F32(c, 0x15950);
          if ( v19 <= 64000 )
            v19 = 64000;
          F32(v7, 28) = v19;
LABEL_48:
          v20 = F32(v7, 92) + 1;
          F32(v7, 92) = v20;
          if ( v20 <= 50 || a6 == 0 && v20 <= 100 )
            return 0;
          if ( F32(v7, 108) != D(0x667ae0) )
          {
            F32(v7, 108) = D(0x667ae0);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          ++F16(v7, 88);
          return 0;
        case 3:
          v21 = F32(c, 0x1594c);
          if ( v21 <= 0 )
            goto LABEL_59;
          if ( v21 > 2 )
          {
            if ( v21 == 3 )
            {
              if ( W_shot_445ce0((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)((uint16_t)a5)) )
              {
                F32(v7, 40) = 0;
                F32(v7, 24) = 0;
                goto LABEL_63;
              }
            }
            else
            {
LABEL_59:
              if ( F32(c, 0x15950) > 56000 )
                goto LABEL_63;
            }
          }
          v22 = F32(c, 0x15950);
          if ( v22 <= 64000 )
            v22 = 64000;
          F32(v7, 28) = v22;
LABEL_63:
          if ( F8(v7, 116) == 0 || a6 == 0 )
            return 0;
          v23 = a3;
          ++F16(v7, 88);
          F32(v7, 92) = 0;
          if ( v23 == 0 )
          {
            for ( i = 20; i != 0; --i )
            {
              v25 = F32(v7, 12) + 1600;
              v26 = rnd();
              W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)v7), (intptr_t)(v25 * v26 / 0x8000 - v25 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
            }
          }
          v27 = rnd();
          v28 = (_DWORD *)F32(c, 0x40);
          v29 = 500 * v27 / 0x8000 - 500;
          if ( a3 != 0 )
            return 0;
          v30 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1016)), (intptr_t)(0));
          if ( v30 == 0 )
            return 0;
          v31 = F32(v30, 0);
          if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v28), (intptr_t)((int *)&a3), (intptr_t)((int *)&a5)) == 0 )
            return 0;
          g_eng.dsb_volume_5bb870(v31, (int)a3);
          g_eng.dsb_pan_5bb880(v31, (int)a5);
          g_eng.dsb_freq_5bb890(v31, v29);
          g_eng.dsb_play_5bb8c0(v31);
          return 0;
        default:
          return 0;
      }
      while ( *(_DWORD *)&GAP(c, 0x10c, 20 * v15) != 0 )
      {
        if ( ++v15 >= v14 )
        {
          ++F16(v7, 88);
          F32(v7, 24) = 0;
          F32(v7, 40) = 0;
          goto LABEL_30;
        }
      }
      v16 = (uint8_t *)c + 20 * v15;
      F32(v16, 4 * 67) = 100155;
      F16(v16, 2 * 136) = 200;
      F16(v16, 2 * 137) = 0;
      F16(v16, 2 * 138) = 0;
      F16(v16, 2 * 139) = 0;
      *(_DWORD *)&GAP(c, 0x10c, 20 * v15 + 12) = 0;
      F32(v16, 4 * 71) = 0;
LABEL_29:
      ++F16(v7, 88);
      F32(v7, 24) = 0;
      F32(v7, 40) = 0;
LABEL_30:
      v17 = F32(v7, 92) + 1;
      F32(v7, 92) = v17;
      if ( v17 > 200 )
      {
        ++F16(v7, 88);
        F32(v7, 92) = 0;
        if ( v13 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)((int *)&a5), (intptr_t)(&a3)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(9u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)((int)a5), (intptr_t)((int)a3), (intptr_t)(0));
        }
      }
      return 1;
    case 0x4F:
      v32 = a3;
      F32(a4, 0) = 2;
      v33 = 0;
      switch ( F16(v7, 88) )
      {
        case 0:
          if ( F32(v7, 108) != D(0x667c78) )
          {
            F32(v7, 108) = D(0x667c78);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          F16(v7, 88) = 1;
          v33 = 1;
          goto LABEL_76;
        case 1:
LABEL_76:
          if ( F8(v7, 116) == 0 )
            goto LABEL_84;
          W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x667e0c)));
          F16(v7, 88) = 2;
          goto LABEL_85;
        case 2:
          if ( F8(v7, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x667fa0)));
            W_char_lunge((intptr_t)(c), (intptr_t)(v7), (intptr_t)(0), (intptr_t)((int)v32), (intptr_t)(1));
            F16(v7, 88) = 3;
          }
          else
          {
LABEL_84:
            if ( v33 == 0 )
              goto LABEL_90;
          }
LABEL_85:
          if ( F16(v7, 88) == 3 )
          {
            v34 = 500 * rnd() / 0x8000 - 2500;
            a5 = (_DWORD *)F32(c, 0x40);
            if ( v32 == 0 )
            {
              v35 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
              if ( v35 != 0 )
              {
                v36 = F32(v35, 0);
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5), (intptr_t)(&a6), (intptr_t)(&(*v191__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v36, a6);
                  g_eng.dsb_pan_5bb880(v36, (*v191__f));
                  g_eng.dsb_freq_5bb890(v36, v34);
                  g_eng.dsb_play_5bb8c0(v36);
                }
              }
            }
          }
LABEL_90:
          if ( F16(v7, 88) == 2 )
          {
            v56 = ih_455d20(c, v7, &a5, &a6);
            v57 = ih_455c90(F32(c, 0x8), v56, 1001);
            if ( v57 != 0 )
            {
              if ( F32(v7, 44) == 1 )
                v58 = F32(v7, 64) + F32(v57, 44);
              else
                v58 = F32(v7, 64) - F32(v57, 44);
              v59 = F32(v7, 68);
              a4 = 100 * ((int32_t)a5 + v58);
              v60 = 25 * (a6 + v59 + F32(v57, 48));
              v61 = F32(v7, 12) / -2 + a4;
              v62 = F32(v7, 16);
              a4 = v61;
              v63 = v62 / 2 + 4 * v60;
            }
            else
            {
              v61 = 0;
              v63 = 0;
              a4 = 0;
            }
            v64 = F32(v7, 76);
            a2 = v63;
            (*v183__f) = v63 - 3200;
            (*v179__f) = v61 + 3200;
            (*v187__f) = v64[185];
            v65 = rnd();
            v66 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v64), (intptr_t)(1000), (intptr_t)(1000 * v65 / 0x8000));
            v67 = (_DWORD *)v64[44];
            v68 = F32(c, 0xc7e4);
            (*v190__f) = v66;
            v69 = F32(c, 0x750);
            a5 = v67;
            v70 = v64[186];
            v71 = v64[45] + F32(v69, 108) + F32(v69, 124) + F32(v69, 148) + v68;
            (*v171__f) = 0;
            (*v175__f) = 0;
            a6 = 0;
            if ( v70 == 0 && v64[183] == 0 && a5 == 0 )
            {
              v72 = F32(v69, 300);
              v71 = F32(v69, 296);
              a5 = F32(v69, 292);
              (*v171__f) = v72;
              (*v175__f) = F32(v69, 304);
              a6 = F32(v69, 308);
            }
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v179__f)), (intptr_t)((*v183__f)), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v187__f)), (intptr_t)((int)a5), (intptr_t)(v71), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v64[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v64[189]), (intptr_t)(F32(v69, 216) + F32(v69, 224) + F32(v69, 228)), (intptr_t)((*v171__f)), (intptr_t)((*v175__f)), (intptr_t)(a6));
            if ( F32(c, 0xc7f4) != 0 )
            {
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(F32(c, 0xc7f8) + (*v179__f)), (intptr_t)((*v183__f)), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v187__f)), (intptr_t)((int)a5), (intptr_t)(v71), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v64[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v64[189]), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v171__f)), (intptr_t)((*v175__f)), (intptr_t)(a6));
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v179__f) - F32(c, 0xc7f8)), (intptr_t)((*v183__f)), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v187__f)), (intptr_t)((int)a5), (intptr_t)(v71), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v64[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v64[189]), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v171__f)), (intptr_t)((*v175__f)), (intptr_t)(a6));
              return 0;
            }
          }
          else if ( F16(v7, 88) == 3 )
          {
            v37 = ih_455d20(c, v7, &a5, &a6);
            v38 = ih_455c90(F32(c, 0x8), v37, 1001);
            if ( v38 != 0 )
            {
              if ( F32(v7, 44) == 1 )
                v39 = F32(v7, 64) + F32(v38, 44);
              else
                v39 = F32(v7, 64) - F32(v38, 44);
              v40 = F32(v7, 68);
              a4 = 100 * ((int32_t)a5 + v39);
              v41 = 25 * (a6 + v40 + F32(v38, 48));
              v42 = F32(v7, 12) / -2 + a4;
              v43 = F32(v7, 16);
              a4 = v42;
              v44 = v43 / 2 + 4 * v41;
            }
            else
            {
              v42 = 0;
              v44 = 0;
              a4 = 0;
            }
            v45 = F32(v7, 76);
            a2 = v44;
            v46 = F32(v45, 740);
            (*v186__f) = v44 - 2000;
            (*v190__f) = v42 + 3200;
            (*v182__f) = v46;
            v47 = rnd();
            v48 = W_char_skill_roll((intptr_t)(c), (intptr_t)((_DWORD *)v45), (intptr_t)(1000), (intptr_t)(1000 * v47 / 0x8000));
            v49 = F32(v45, 180);
            v50 = F32(c, 0xc7e4);
            (*v178__f) = v48;
            v51 = F32(c, 0x750);
            a5 = F32(v45, 176);
            v52 = F32(v51, 124) + F32(v51, 148) + v50 + v49;
            v53 = F32(v45, 744);
            v54 = F32(v51, 108) + v52;
            (*v174__f) = 0;
            (*v170__f) = 0;
            a6 = 0;
            if ( v53 == 0 && F32(v45, 732) == 0 && a5 == 0 )
            {
              v55 = F32(v51, 300);
              v54 = F32(v51, 296);
              a5 = F32(v51, 292);
              (*v174__f) = v55;
              (*v170__f) = F32(v51, 304);
              a6 = F32(v51, 308);
            }
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v190__f)), (intptr_t)((*v186__f)), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v182__f)), (intptr_t)((int)a5), (intptr_t)(v54), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v178__f)), (intptr_t)(1), (intptr_t)(F32(v45, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v45, 756)), (intptr_t)(F32(v51, 216) + F32(v51, 224) + F32(v51, 228)), (intptr_t)((*v174__f)), (intptr_t)((*v170__f)), (intptr_t)(a6));
            if ( F32(c, 0xc7f4) != 0 )
            {
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(F32(c, 0xc7f8) + (*v190__f)), (intptr_t)((*v186__f)), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v182__f)), (intptr_t)((int)a5), (intptr_t)(v54), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v178__f)), (intptr_t)(1), (intptr_t)(F32(v45, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v45, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v174__f)), (intptr_t)((*v170__f)), (intptr_t)(a6));
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v190__f) - F32(c, 0xc7f8)), (intptr_t)((*v186__f)), (intptr_t)(6400), (intptr_t)(4000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v182__f)), (intptr_t)((int)a5), (intptr_t)(v54), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v178__f)), (intptr_t)(1), (intptr_t)(F32(v45, 736)), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(F32(v45, 756)), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v174__f)), (intptr_t)((*v170__f)), (intptr_t)(a6));
              return 0;
            }
          }
          break;
        case 3:
          if ( F8(v7, 116) == 0 )
            goto LABEL_90;
          W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x668134)));
          W_char_lunge((intptr_t)(c), (intptr_t)(v7), (intptr_t)(0), (intptr_t)((int)v32), (intptr_t)(1));
          F16(v7, 88) = 4;
          goto LABEL_85;
        case 4:
          if ( F8(v7, 116) == 0 )
            goto LABEL_90;
          W_body_action_reset((intptr_t)(v7));
          F16(v7, 88) = 5;
          goto LABEL_85;
        default:
          goto LABEL_90;
      }
      return 0;
    case 0x50:
      v157 = a3;
      F32(a4, 0) = 2;
      if ( W_slash_step((intptr_t)(c), (intptr_t)(v7), (intptr_t)((int)v157), (intptr_t)((int)D(0x668c58)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 && F16(v7, 88) == 3 )
      {
        v158 = rnd();
        W_sfx_play_at((intptr_t)(c), (intptr_t)((int)v157), (intptr_t)(72), (intptr_t)(0), (intptr_t)(500 * v158 / 0x8000 - 2500), (intptr_t)(0));
      }
      if ( F16(v7, 88) == 2 )
      {
        if ( ih_445e50(c, v7, 1011, &a4, &a2) != 0 )
        {
          v160 = F32(v7, 16);
          a4 += F32(v7, 12) / -2;
          a2 += v160 / 2;
        }
        else
        {
          a4 = 0;
          a2 = 0;
        }
        W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a4 + 3200), (intptr_t)(a2 - 3200), (intptr_t)(6400), (intptr_t)(6400), (intptr_t)(1), (intptr_t)((int)v157), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(3));
        return 0;
      }
      else
      {
        if ( F16(v7, 88) != 3 )
          return 0;
        if ( ih_445e50(c, v7, 1011, &a4, &a2) != 0 )
        {
          v159 = F32(v7, 16);
          a4 += F32(v7, 12) / -2;
          a2 += v159 / 2;
        }
        else
        {
          a4 = 0;
          a2 = 0;
        }
        W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a4 + 4000), (intptr_t)(a2 - 2000), (intptr_t)(8000), (intptr_t)(4000), (intptr_t)(0), (intptr_t)((int)v157), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(3));
        return 0;
      }
    case 0x51:
      v103 = a3;
      F32(a4, 0) = 3;
      switch ( F16(v7, 88) )
      {
        case 0:
          v104 = 500 * rnd();
          a6 = F32(c, 0x40);
          if ( v103 == 0 )
          {
            v105 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1156)), (intptr_t)(0));
            if ( v105 != 0 )
            {
              v106 = F32(v105, 0);
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)a6), (intptr_t)(&(*v191__f)), (intptr_t)(&(*v190__f))) != 0 )
              {
                g_eng.dsb_volume_5bb870(v106, (*v191__f));
                g_eng.dsb_pan_5bb880(v106, (*v190__f));
                g_eng.dsb_freq_5bb890(v106, v104 / 0x8000 - 3000);
                g_eng.dsb_play_5bb8c0(v106);
              }
            }
          }
          if ( F32(v7, 108) != D(0x668790) )
          {
            F32(v7, 108) = D(0x668790);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          ++F16(v7, 88);
          goto LABEL_186;
        case 1:
LABEL_186:
          v107 = F32(v7, 40);
          v108 = F32(v7, 92) + 1;
          F32(v7, 92) = v108;
          F32(v7, 24) = -56000;
          if ( v107 >= 32000 )
          {
            if ( v107 > 32000 )
              v107 = v107 - 4000 < 0 ? 0 : v107 - 4000;
          }
          else
          {
            v107 += 4000;
            if ( v107 > 32000 )
              v107 = 32000;
          }
          F32(v7, 40) = v107;
          if ( v108 > 20 )
            ++F16(v7, 88);
          break;
        case 2:
          v109 = F32(v7, 24);
          if ( v109 >= 160000 )
          {
            if ( v109 > 160000 )
              v109 = v109 - 16000 < 0 ? 0 : v109 - 16000;
          }
          else
          {
            v109 += 16000;
            if ( v109 > 160000 )
              v109 = 160000;
          }
          F32(v7, 24) = v109;
          v110 = ih_455d20(c, v7, &(*v191__f), &(*v190__f));
          v111 = ih_455c90(F32(c, 0x8), v110, 1011);
          if ( v111 != 0 )
          {
            if ( F32(v7, 44) == 1 )
              v112 = F32(v7, 64) + F32(v111, 44);
            else
              v112 = F32(v7, 64) - F32(v111, 44);
            v113 = F32(v7, 68);
            a4 = 100 * ((*v191__f) + v112);
            v114 = 25 * ((*v190__f) + v113 + F32(v111, 48));
            v115 = F32(v7, 12) / -2 + a4;
            v116 = F32(v7, 16);
            a4 = v115;
            v117 = v116 / 2 + 4 * v114;
          }
          else
          {
            v115 = 0;
            v117 = 0;
            a4 = 0;
          }
          v118 = F32(v7, 76);
          a2 = v117;
          (*v191__f) = v115 + 4000;
          (*v185__f) = v117;
          (*v189__f) = v118[185];
          v119 = rnd();
          v120 = W_char_skill_roll((intptr_t)(c), (intptr_t)(v118), (intptr_t)(1000), (intptr_t)(1000 * v119 / 0x8000));
          v121 = v118[44];
          (*v190__f) = v120;
          (*v169__f) = v121;
          v122 = v118[45] + W_char_mag_sum((intptr_t)(c));
          (*v181__f) = 0;
          (*v173__f) = 0;
          (*v177__f) = 0;
          if ( v118[186] == 0 && v118[183] == 0 && (*v169__f) == 0 )
          {
            v123 = (_DWORD *)F32(c, 0x750);
            v122 = v123[74];
            (*v169__f) = v123[73];
            (*v181__f) = v123[75];
            (*v173__f) = v123[76];
            (*v177__f) = v123[77];
          }
          W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v191__f)), (intptr_t)((*v185__f)), (intptr_t)(12000), (intptr_t)(8000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v189__f)), (intptr_t)((*v169__f)), (intptr_t)(v122), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v118[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v118[189]), (intptr_t)(F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228)), (intptr_t)((*v181__f)), (intptr_t)((*v173__f)), (intptr_t)((*v177__f)));
          if ( F32(c, 0xc7f4) != 0 )
          {
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v191__f) + F32(c, 0xc7f8)), (intptr_t)((*v185__f)), (intptr_t)(12000), (intptr_t)(8000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v189__f)), (intptr_t)((*v169__f)), (intptr_t)(v122), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v118[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v118[189]), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v181__f)), (intptr_t)((*v173__f)), (intptr_t)((*v177__f)));
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v191__f) - F32(c, 0xc7f8)), (intptr_t)((*v185__f)), (intptr_t)(12000), (intptr_t)(8000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v189__f)), (intptr_t)((*v169__f)), (intptr_t)(v122), (intptr_t)(0), (intptr_t)(5), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v118[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v118[189]), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v181__f)), (intptr_t)((*v173__f)), (intptr_t)((*v177__f)));
          }
          if ( a6 != 0 )
          {
            if ( a3 == 0 )
            {
              for ( j = 20; j != 0; --j )
              {
                v125 = F32(v7, 12) + 1600;
                v126 = rnd();
                W_particle_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(100080), (intptr_t)((_DWORD *)v7), (intptr_t)(v125 * v126 / 0x8000 - v125 / 2), (intptr_t)(0), (intptr_t)(99), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10));
              }
              v127 = rnd();
              W_sfx_play_at((intptr_t)(c), (intptr_t)(0), (intptr_t)(37), (intptr_t)(0), (intptr_t)(500 * v127 / 0x8000 - 500), (intptr_t)(0));
              ih_455cf0(*(intptr_t *)&GAP(W, 0x1030, 28), 1, 60, 1000);
            }
            W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x668928)));
            ++F16(v7, 88);
            F32(v7, 40) = 0;
            F32(v7, 24) = 0;
          }
          break;
        case 3:
          if ( ih_445e50(c, v7, 1011, &a4, &a2) != 0 )
          {
            v128 = F32(v7, 16);
            a4 += F32(v7, 12) / -2;
            a2 += v128 / 2;
          }
          else
          {
            a4 = 0;
            a2 = 0;
          }
          W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a4 + 2000), (intptr_t)(a2 - 4000), (intptr_t)(22000), (intptr_t)(8000), (intptr_t)(0), (intptr_t)((int)v103), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(5));
          if ( F8(v7, 116) != 0 )
          {
            W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x668ac0)));
            ++F16(v7, 88);
          }
          break;
        case 4:
          if ( F8(v7, 116) != 0 )
            W_body_action_reset((intptr_t)(v7));
          break;
        default:
          break;
      }
      W_char_drift_move((intptr_t)(c), (intptr_t)(v7), (intptr_t)((int)a3), (intptr_t)((int)a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      return 1;
    case 0x52:
      F8(a2, 124) = 1;
      switch ( F16(v7, 88) )
      {
        case 0:
          v73 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v74 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1448)), (intptr_t)(0));
            if ( v74 != 0 )
            {
              v75 = F32(v74, 0);
              if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360) + 600), (intptr_t)(-3000), (intptr_t)(0), (intptr_t)(v73), (intptr_t)(&a6), (intptr_t)(&(*v191__f))) != 0 )
              {
                g_eng.dsb_volume_5bb870(v75, a6);
                g_eng.dsb_pan_5bb880(v75, (*v191__f));
                g_eng.dsb_freq_5bb890(v75, -2000);
                g_eng.dsb_play_5bb8c0(v75);
              }
            }
          }
          if ( F32(v7, 108) != D(0x6682c8) )
          {
            F32(v7, 108) = D(0x6682c8);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          ++F16(v7, 88);
          F32(v7, 24) = -56000;
          goto LABEL_125;
        case 1:
LABEL_125:
          v76 = F32(v7, 92) + 1;
          F32(v7, 92) = v76;
          if ( v76 > 20 )
          {
            ++F16(v7, 88);
            F32(v7, 24) = 32000;
            F32(v7, 92) = 0;
            v77 = 1000 * rnd();
            a6 = F32(c, 0x40);
            if ( a3 == 0 )
            {
              v78 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1436)), (intptr_t)(0));
              if ( v78 != 0 )
              {
                v79 = F32(v78, 0);
                if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((_DWORD *)a6), (intptr_t)((int *)&a3), (intptr_t)(&(*v191__f))) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v79, (int)a3);
                  g_eng.dsb_pan_5bb880(v79, (*v191__f));
                  g_eng.dsb_freq_5bb890(v79, v77 / 0x8000);
                  g_eng.dsb_play_5bb8c0(v79);
                }
              }
            }
            if ( F32(v7, 108) != D(0x668460) )
            {
              F32(v7, 108) = D(0x668460);
              F8(v7, 116) = 0;
              F16(v7, 114) = 0;
              F16(v7, 112) = 0;
            }
          }
          break;
        case 2:
          v80 = F8(v7, 116);
          v81 = F32(v7, 92) + 1;
          F32(v7, 92) = v81;
          if ( v80 != 0 && F32(v7, 108) != D(0x6685f8) )
          {
            F32(v7, 108) = D(0x6685f8);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          v82 = F32(v7, 40);
          if ( v82 >= 64000 )
          {
            if ( v82 > 64000 )
              v82 = v82 - 4000 < 0 ? 0 : v82 - 4000;
          }
          else
          {
            v82 += 4000;
            if ( v82 > 64000 )
              v82 = 64000;
          }
          F32(v7, 40) = v82;
          v83 = F32(v7, 24);
          if ( v83 >= 80000 )
          {
            if ( v83 > 80000 )
              v83 = v83 - 8000 < 0 ? 0 : v83 - 8000;
          }
          else
          {
            v83 += 8000;
            if ( v83 > 80000 )
              v83 = 80000;
          }
          F32(v7, 24) = v83;
          if ( v81 > 10 )
          {
            v84 = ih_455d20(c, v7, &a6, &(*v191__f));
            v85 = ih_455c90(F32(c, 0x8), v84, 1015);
            if ( v85 != 0 )
            {
              if ( F32(v7, 44) == 1 )
                v86 = F32(v7, 64) + F32(v85, 44);
              else
                v86 = F32(v7, 64) - F32(v85, 44);
              v87 = F32(v7, 68);
              a4 = 100 * (a6 + v86);
              v88 = 25 * ((*v191__f) + v87 + F32(v85, 48));
              v89 = F32(v7, 12) / -2 + a4;
              v90 = F32(v7, 16);
              a4 = v89;
              v91 = v90 / 2 + 4 * v88;
            }
            else
            {
              v89 = 0;
              v91 = 0;
              a4 = 0;
            }
            v92 = F32(v7, 76);
            a2 = v91;
            (*v184__f) = v91 - 9000;
            (*v180__f) = v89 + 6000;
            (*v188__f) = v92[185];
            v93 = rnd();
            (*v190__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)(v92), (intptr_t)(1000), (intptr_t)(1000 * v93 / 0x8000));
            a6 = v92[44];
            v94 = v92[45] + W_char_mag_sum((intptr_t)(c));
            (*v168__f) = 0;
            (*v172__f) = 0;
            (*v176__f) = 0;
            if ( v92[186] == 0 && v92[183] == 0 && a6 == 0 )
            {
              v95 = (int *)F32(c, 0x750);
              v96 = v95[75];
              v94 = v95[74];
              a6 = v95[73];
              (*v168__f) = v96;
              (*v172__f) = v95[76];
              (*v176__f) = v95[77];
            }
            W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v180__f)), (intptr_t)((*v184__f)), (intptr_t)(9000), (intptr_t)(12000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v188__f)), (intptr_t)(a6), (intptr_t)(v94), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v92[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v92[189]), (intptr_t)(F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228)), (intptr_t)((*v168__f)), (intptr_t)((*v172__f)), (intptr_t)((*v176__f)));
            if ( F32(c, 0xc7f4) != 0 )
            {
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)(F32(c, 0xc7f8) + (*v180__f)), (intptr_t)((*v184__f)), (intptr_t)(9000), (intptr_t)(12000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v188__f)), (intptr_t)(a6), (intptr_t)(v94), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v92[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v92[189]), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v168__f)), (intptr_t)((*v172__f)), (intptr_t)((*v176__f)));
              W_body_hit_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(v7), (intptr_t)((*v180__f) - F32(c, 0xc7f8)), (intptr_t)((*v184__f)), (intptr_t)(9000), (intptr_t)(12000), (intptr_t)(0), (intptr_t)((int)a3), (intptr_t)((*v188__f)), (intptr_t)(a6), (intptr_t)(v94), (intptr_t)(0), (intptr_t)(4), (intptr_t)((*v190__f)), (intptr_t)(1), (intptr_t)(v92[184]), (intptr_t)(F32(v7, 96)), (intptr_t)(0), (intptr_t)(v92[189]), (intptr_t)(F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228)), (intptr_t)((*v168__f)), (intptr_t)((*v172__f)), (intptr_t)((*v176__f)));
            }
          }
          if ( F32(v7, 92) > 60 )
          {
            ++F16(v7, 88);
            F32(v7, 92) = 0;
          }
          break;
        case 3:
          if ( F32(v7, 108) != D(0x6672e8) )
          {
            F32(v7, 108) = D(0x6672e8);
            F8(v7, 116) = 0;
            F16(v7, 114) = 0;
            F16(v7, 112) = 0;
          }
          v97 = F32(v7, 40);
          v98 = F32(v7, 92) + 1;
          F32(v7, 92) = v98;
          if ( v97 >= 12000 )
          {
            if ( v97 > 12000 )
              v97 = v97 - 4000 < 0 ? 0 : v97 - 4000;
          }
          else
          {
            v97 += 4000;
            if ( v97 > 12000 )
              v97 = 12000;
          }
          F32(v7, 40) = v97;
          v99 = F32(v7, 24);
          if ( v99 <= -80000 )
          {
            if ( v99 < -80000 )
              v99 = v99 + 8000 > 0 ? 0 : v99 + 8000;
          }
          else
          {
            v99 -= 8000;
            if ( v99 < -80000 )
              v99 = -80000;
          }
          F32(v7, 24) = v99;
          if ( v98 > 30 )
          {
            F16(v7, 72) = 0;
            F16(v7, 86) = 210;
            F16(v7, 88) = 0;
            F32(v7, 92) = 0;
            F32(v7, 120) = 0;
            F16(v7, 84) = 0;
            F32(v7, 76) = 0;
            F32(v7, 80) = 0;
            F16(v7, 90) = 0;
            F16(v7, 100) = 0;
          }
          break;
        default:
          break;
      }
      v100 = (uint16_t)a5;
      if ( W_entity_move_vertical((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(F32(v7, 24) / 100), (intptr_t)(F8(v7, 32)), (intptr_t)((uint16_t)a5), (intptr_t)(v7 + 8), (intptr_t)((_DWORD *)(v7 + 644))) == 0 )
        F32(v7, 24) = 0;
      SETLO(v101, F8(v7, 32));
      (*v166__f) = v101;
      v102 = W_body_facing_sign((intptr_t)(v7), (intptr_t)(F32(v7, 40) / 100));
      if ( W_entity_move_horizontal((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(v102), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v166__f)), (intptr_t)(v100), (intptr_t)(0)) != 0 )
        return 1;
      F32(v7, 40) = 0;
      return 1;
    case 0x53:
      SETHI(v129, 0);
      F32(a4, 0) = 2;
      SETLO(v129, F16(v7, 84));
      if ( (uint16_t)v129 == 1 )
      {
        v130 = D(0x669dc0);
        a5 = D(0x669dc0);
      }
      else
      {
        v130 = D(0x6695d8);
        a5 = D(0x6695d8);
        if ( (uint16_t)v129 != 2 )
        {
          a6 = 60;
          break;
        }
      }
      a6 = 40;
      break;
    case 0x54:
      v151 = a3;
      v152 = F16(a2, 88);
      F8(a2, 124) = 1;
      if ( (uint16_t)v152 != 0 )
      {
        v153 = v152 - 1;
        if ( v153 != 0 )
        {
          if ( v153 == 1 )
          {
            v154 = W_speed_ramp((intptr_t)(F32(v7, 40)), (intptr_t)(16000), (intptr_t)(-16000), (intptr_t)(0));
            (*v162__f) = F32(v7, 24);
            F32(v7, 40) = v154;
            F32(v7, 24) = W_speed_ramp((intptr_t)((*v162__f)), (intptr_t)(16000), (intptr_t)(-16000), (intptr_t)(0));
            if ( F8(v7, 116) != 0 )
              W_body_action_reset((intptr_t)(v7));
          }
          goto LABEL_286;
        }
      }
      else
      {
        v155 = rnd();
        W_sfx_play_at((intptr_t)(c), (intptr_t)((int)v151), (intptr_t)(145), (intptr_t)(0), (intptr_t)(1000 * v155 / 0x8000 - 1000), (intptr_t)(0));
        W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x6692a8)));
        (*v167__f) = F32(v7, 44);
        (*v161__f) = F32(v7, 76);
        ++F16(v7, 88);
        F32(v7, 24) = 0;
        F32(v7, 40) = 0;
        W_char_shot_fire((intptr_t)(c), (intptr_t)(100036), (intptr_t)(v7), (intptr_t)(10000), (intptr_t)(10000), (intptr_t)(1), (intptr_t)((int)v151), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v161__f)), (intptr_t)(0), (intptr_t)((*v167__f)), (intptr_t)(0), (intptr_t)(2));
      }
      (*v163__f) = F32(v7, 40);
      ++F32(v7, 92);
      v156 = W_speed_ramp((intptr_t)((*v163__f)), (intptr_t)(16000), (intptr_t)(-16000), (intptr_t)(-96000));
      (*v164__f) = F32(v7, 24);
      F32(v7, 40) = v156;
      F32(v7, 24) = W_speed_ramp((intptr_t)((*v164__f)), (intptr_t)(4000), (intptr_t)(-4000), (intptr_t)(-24000));
      if ( F32(v7, 92) > 15 )
      {
        ++F16(v7, 88);
        W_body_set_clip((intptr_t)(v7), (intptr_t)((int)D(0x669440)));
      }
LABEL_286:
      W_char_drift_move((intptr_t)(c), (intptr_t)(v7), (intptr_t)((int)v151), (intptr_t)((int)a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      return 1;
    case 0x55:
      if ( F16(a2, 88) != 0 )
      {
        if ( F16(a2, 88) != 1 )
        {
          if ( F16(a2, 88) == 2 && F8(a2, 116) != 0 )
          {
            F16(a2, 72) = 0;
            F16(v7, 86) = 210;
            F16(v7, 88) = 0;
            F32(v7, 92) = 0;
            F32(v7, 120) = 0;
            F16(v7, 84) = 0;
            F32(v7, 76) = 0;
            F32(v7, 80) = 0;
            F16(v7, 90) = 0;
            F16(v7, 100) = 0;
          }
          goto LABEL_18;
        }
      }
      else
      {
        if ( F32(a2, 108) != D(0x669c28) )
        {
          F32(a2, 108) = D(0x669c28);
          F8(v7, 116) = 0;
          F16(v7, 114) = 0;
          F16(v7, 112) = 0;
        }
        F16(v7, 88) = 1;
      }
      v9 = F32(v7, 92) + 1;
      F32(v7, 92) = v9;
      if ( v9 > 12 )
      {
        v10 = a3;
        ++F16(v7, 88);
        if ( v10 == 0
          && F32(c, 0x15b54) != 0
          && F32(c, 0x15b5c) == 0
          && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&a6), (intptr_t)((int *)&a5), (intptr_t)(&a3)) != 0 )
        {
          W_voice_play((intptr_t)(F32(c, 0x15b54)), (intptr_t)(9u), (intptr_t)(0), (intptr_t)(a6), (intptr_t)((int)a5), (intptr_t)((int)a3), (intptr_t)(0));
        }
      }
LABEL_18:
      if ( F32(v7, 24) <= 0 )
        return 0;
      F32(v7, 28) = 4000;
      return 0;
    default:
      return 0;
  }
  switch ( F16(v7, 88) )
  {
    case 0:
      W_body_set_clip((intptr_t)(v7), (intptr_t)((int)v130));
      v131 = a3;
      ++F16(v7, 88);
      W_sfx_play_at((intptr_t)(c), (intptr_t)((int)v131), (intptr_t)(155), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      goto LABEL_231;
    case 1:
LABEL_231:
      if ( F8(v7, 116) != 0 )
      {
        ++F16(v7, 88);
        W_body_set_clip((intptr_t)(v7), (intptr_t)((int)(v130 + 101)));
      }
      goto LABEL_276;
    case 2:
      if ( F8(v7, 116) != 0 )
      {
        ++F16(v7, 88);
        F32(v7, 92) = 0;
        W_body_set_clip((intptr_t)(v7), (intptr_t)((int)(v130 + 202)));
        if ( F16(v7, 84) != 2 )
          W_entity_snd_start((intptr_t)(c), (intptr_t)(2u), (intptr_t)((int)a3), (intptr_t)(0x99u), (intptr_t)(400), (intptr_t)(0), (intptr_t)(-3000), (intptr_t)(1));
      }
      goto LABEL_276;
    case 3:
      v132 = a3;
      v133 = v129 - 1;
      if ( v133 != 0 )
      {
        if ( v133 == 1 )
        {
          v140 = F32(v7, 92);
          if ( v140 == 0 || v140 == 5 || v140 == 10 )
          {
            ih_445e50(c, v7, 1001, &a4, &a2);
            v141 = W_char_shot_fire((intptr_t)(c), (intptr_t)(100037), (intptr_t)(v7), (intptr_t)(a4 + 1600), (intptr_t)(a2 + 800), (intptr_t)(1), (intptr_t)((int)v132), (intptr_t)(5), (intptr_t)(90), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(F32(v7, 44)), (intptr_t)(0), (intptr_t)(2));
            if ( v141 != 0 )
              F32(v141, 516) = 30 - F32(v7, 92);
          }
        }
        else
        {
          if ( a3 == 0 )
          {
            ih_445e50(c, v7, 1001, &a4, &a2);
            v134 = rnd();
            W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100050), (intptr_t)((_DWORD *)v7), (intptr_t)(a4 + 1600), (intptr_t)(a2), (intptr_t)(F32(v7, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(100 - 21 * v134 / 0x8000), (intptr_t)(10));
            v135 = rnd();
            W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100050), (intptr_t)((_DWORD *)v7), (intptr_t)(a4 + 1600), (intptr_t)(a2), (intptr_t)(F32(v7, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(100 - 21 * v135 / 0x8000), (intptr_t)(0));
            if ( F32(CFG(), 148) != 0 )
            {
              v136 = rnd();
              W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100050), (intptr_t)((_DWORD *)v7), (intptr_t)(a4 + 1600), (intptr_t)(a2), (intptr_t)(F32(v7, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(100 - 21 * v136 / 0x8000), (intptr_t)(0));
            }
          }
          if ( F32(v7, 92) > 10 )
          {
            if ( ih_445e50(c, v7, 1001, &a4, &a2) != 0 )
            {
              v137 = F32(v7, 16);
              a4 += F32(v7, 12) / -2;
              a2 += v137 / 2;
            }
            else
            {
              a4 = 0;
              a2 = 0;
            }
            v138 = F32(v7, 92);
            v139 = 20;
            if ( v138 > 20 )
              v138 = 20;
            else
              v139 = F32(v7, 92);
            W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a4 + 26000 * v138 / 20), (intptr_t)(a2 - 3200), (intptr_t)(24000 * v139 / 20 + 2000), (intptr_t)(6400), (intptr_t)(0), (intptr_t)((int)v132), (intptr_t)(F32(v7, 76)), (intptr_t)(1000), (intptr_t)(8));
LABEL_270:
            v130 = a5;
          }
        }
      }
      else
      {
        if ( a3 == 0 )
        {
          ih_445e50(c, v7, 1001, &a4, &a2);
          v142 = rnd();
          v143 = W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100050), (intptr_t)((_DWORD *)v7), (intptr_t)(a4 + 1600), (intptr_t)(a2), (intptr_t)(F32(v7, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(115 - 31 * v142 / 0x8000), (intptr_t)(10));
          if ( v143 != 0 )
            W_entity_scatter((intptr_t)(v143), (intptr_t)(80000), (intptr_t)(20000), (intptr_t)(-1000), (intptr_t)(2000));
          v144 = rnd();
          v145 = W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100050), (intptr_t)((_DWORD *)v7), (intptr_t)(a4 + 1600), (intptr_t)(a2), (intptr_t)(F32(v7, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(115 - 31 * v144 / 0x8000), (intptr_t)(0));
          if ( v145 != 0 )
            W_entity_scatter((intptr_t)(v145), (intptr_t)(80000), (intptr_t)(20000), (intptr_t)(-1000), (intptr_t)(2000));
          if ( F32(CFG(), 148) != 0 )
          {
            v146 = rnd();
            v147 = W_shot_4504c0((intptr_t)(c), (intptr_t)(16000), (intptr_t)((int)c), (intptr_t)(100050), (intptr_t)((_DWORD *)v7), (intptr_t)(a4 + 1600), (intptr_t)(a2), (intptr_t)(F32(v7, 44)), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(115 - 31 * v146 / 0x8000), (intptr_t)(0));
            if ( v147 != 0 )
              W_entity_scatter((intptr_t)(v147), (intptr_t)(80000), (intptr_t)(20000), (intptr_t)(-1000), (intptr_t)(2000));
          }
        }
        if ( F32(v7, 92) > 10 )
        {
          if ( ih_445e50(c, v7, 1001, &a4, &a2) != 0 )
          {
            v148 = F32(v7, 16);
            a4 += F32(v7, 12) / -2;
            a2 += v148 / 2;
          }
          else
          {
            a4 = 0;
            a2 = 0;
          }
          v149 = F32(v7, 92);
          v150 = 20;
          if ( v149 > 20 )
            v149 = 20;
          else
            v150 = F32(v7, 92);
          W_char_attack_hits((intptr_t)(c), (intptr_t)(v7), (intptr_t)(a4 + 16000 * v149 / 20), (intptr_t)(a2 - 3200), (intptr_t)(12000 * v150 / 20), (intptr_t)(9600), (intptr_t)(0), (intptr_t)((int)v132), (intptr_t)(F32(v7, 76)), (intptr_t)(900), (intptr_t)(8));
          goto LABEL_270;
        }
      }
      if ( ++F32(v7, 92) > a6 )
      {
        ++F16(v7, 88);
        W_body_set_clip((intptr_t)(v7), (intptr_t)((int)(v130 + 303)));
      }
      W_entity_snd_keep((intptr_t)(c), (intptr_t)(2u), (intptr_t)(v132));
LABEL_276:
      if ( F32(v7, 24) <= 0 )
        return 0;
      F32(v7, 28) = 4000;
      return 0;
    case 4:
      if ( F8(v7, 116) != 0 )
      {
        W_entity_snd_release((intptr_t)(c), (intptr_t)(2u), (intptr_t)(a3), (intptr_t)(0));
        W_body_action_reset((intptr_t)(v7));
      }
      goto LABEL_276;
    default:
      goto LABEL_276;
  }
}

/* 0x455c90: unsigned int __thiscall sub_455C90(cls_455C90 *this, unsigned __int16 a2, int a3) */
int32_t ih_455c90(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint32_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;

  v3 = F32(c, 0x1c);
  v4 = 0;
  if ( v3 == 0 )
    return 0;
  v5 = F32(c, 0x0);
  v6 = v3 * a2;
  v7 = 0;
  while ( a3 != F32((v5 + 52 * (v6 + v7)), 0) )
  {
    v7 = ++v4;
    if ( v4 >= v3 )
      return 0;
  }
  return v5 + 52 * (v6 + v4);
}

/* 0x455cf0: int __thiscall sub_455CF0(cls_455CF0 *this, int a2, int a3, int a4) */
int32_t ih_455cf0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x3c) = 0;
  F32(c, 0x40) = 0;
  F32(c, 0x44) = 0;
  F16(c, 0x48) = 0;
  F16(c, 0x4a) = 0;
  F32(c, 0x4c) = 0;
  F32(c, 0x40) = a3;
  F32(c, 0x3c) = a2;
  F32(c, 0x50) = a4;
  return a4;
}

/* 0x455d20: __int16 __thiscall sub_455D20(cls_455D20 *this, int a2, _DWORD *a3, _DWORD *a4) */
int32_t ih_455d20(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    uint16_t * v4 = 0;
    int16_t result = 0;
    uint16_t v6 = 0;

  v4 = F32(a2, 108);
  if ( v4 == 0 )
    return 0;
  v6 = F16(a2, 114);
  result = *v4 + v4[v6 + 1];
  *a3 = F32(&v4[2 * v6 + 40], 0);
  *a4 = F32(&v4[2 * F16(a2, 114) + 104], 0);
  if ( F32(a2, 44) == 3 )
    result += F16(F32(c, 0x8), 24);
  return result;
}

/* 0x456d30: int __thiscall sub_456D30(cls_5B5990 *this, _DWORD *a3, int a4, unsigned __int16 a5, int a6, int a7) */
int32_t ih_456d30(wl_char *self, int32_t * a3, int32_t a4, uint16_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    int16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    void * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    uint16_t v25 = 0;
    void * v26 = 0;
    intptr_t v27 = 0;
    intptr_t * v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    int32_t * v35 = 0;
    intptr_t * v36 = 0;
    intptr_t v37 = 0;
    int16_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    uint16_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    uint16_t v46 = 0;
    uint16_t v47 = 0;
    int32_t * v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    int16_t v53 = 0;
    intptr_t * v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    int32_t * v58 = 0;
    int16_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    int v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    int v68 = 0;
    intptr_t v69 = 0;
    int32_t * v70 = 0;
    intptr_t v71 = 0;
    uint16_t v72 = 0;
    intptr_t v73 = 0;
    uint16_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    int32_t * v77 = 0;
    intptr_t v78 = 0;
    _Alignas(8) uint8_t frame_[0x154 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x54);   /* [256] at ebp-0x100 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x154 */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x134 */
    int32_t * *const v81__f = (int32_t * *)(frame_ + 0x34);   /* ebp-0x120 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x11c */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x118 */
    uint16_t *const v84__f = (uint16_t *)(frame_ + 0x42);   /* ebp-0x112 */
    int32_t * *const v85__f = (int32_t * *)(frame_ + 0x44);   /* ebp-0x110 */
    intptr_t *const v86__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x10c */
    int32_t * *const v87__f = (int32_t * *)(frame_ + 0x4c);   /* ebp-0x108 */
    intptr_t *const v88__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x104 */

  v7 = a4;
  v8 = 0;
  v9 = c;
  *((uint8_t *)a3 + 125) = 1;
  *((uint8_t *)a3 + 126) = 1;
  if ( a4 == 0 )
  {
    if ( F32(W, 0x2770) != 0 || F32(W, 0x2764) == 1 )
      v10 = 12;
    else
      v10 = 13;
    *(_DWORD *)&GAP(c, 0x0, 252) = v10;
    *(_DWORD *)&GAP(c, 0x0, 256) = v10;
  }
  switch ( F16(a3, 2 * 36) )
  {
    case 0:
      if ( a4 != 0 || *(_DWORD *)&GAP(c, 0x0, 616) != 0 )
        return v8;
      *(_DWORD *)&GAP(c, 0x0, 244) = 0;
      *(_DWORD *)&GAP(c, 0x0, 248) = 0;
      if ( W_party_find((intptr_t)(c), (intptr_t)(0), (intptr_t)(10113), (intptr_t)(&(*v84__f)), (intptr_t)(0)) != 0 && (*v84__f) < 5u )
      {
        if ( *((uint16_t *)a3 + 43) / 100 > 2 && F16(a3, 2 * 36) != 0 || F16(a3, 2 * 36) == 72 )
          return v8;
        F16(a3, 2 * 36) = 72;
        F16(a3, 2 * 43) = 210;
        F16(a3, 2 * 44) = 0;
        a3[23] = 0;
        a3[30] = 0;
        F16(a3, 2 * 42) = 0;
        a3[19] = 0;
        a3[20] = 0;
        F16(a3, 2 * 45) = 0;
        F16(a3, 2 * 50) = 0;
        return 0;
      }
      else
      {
        if ( W_party_find((intptr_t)(v9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xFu)) == 0
          || *((uint16_t *)a3 + 43) / 0x64u > 2 && F16(a3, 2 * 36) != 0
          || F16(a3, 2 * 36) == 73 )
        {
          return v8;
        }
        F16(a3, 2 * 36) = 73;
        F16(a3, 2 * 43) = 210;
        F16(a3, 2 * 44) = 0;
        a3[23] = 0;
        a3[30] = 0;
        F16(a3, 2 * 42) = 0;
        a3[19] = 0;
        a3[20] = 0;
        F16(a3, 2 * 45) = 0;
        F16(a3, 2 * 50) = 0;
        return 0;
      }
    case 2:
      if ( F16(a3, 2 * 44) != 0 )
      {
        if ( F16(a3, 2 * 44) != 1 )
          return 0;
      }
      else
      {
        if ( (_UNKNOWN *)a3[27] != D(0x6429e0) )
        {
          a3[27] = D(0x6429e0);
          *((uint8_t *)a3 + 116) = 0;
          F16(a3, 2 * 57) = 0;
          F16(a3, 2 * 56) = 0;
        }
        F16(a3, 2 * 44) = 1;
      }
      if ( *((uint8_t *)a3 + 116) == 0 )
        return v8;
      v19 = a3[11];
      F16(a3, 2 * 36) = 0;
      F16(a3, 2 * 43) = 210;
      F16(a3, 2 * 44) = 0;
      a3[23] = 0;
      a3[30] = 0;
      F16(a3, 2 * 42) = 0;
      a3[19] = 0;
      a3[20] = 0;
      F16(a3, 2 * 45) = 0;
      F16(a3, 2 * 50) = 0;
      a3[11] = v19 != 3 ? 3 : 1;
      return 0;
    case 0x11:
      if ( (_UNKNOWN *)a3[27] != D(0x642b78) )
      {
        a3[27] = D(0x642b78);
        *((uint8_t *)a3 + 116) = 0;
        F16(a3, 2 * 57) = 0;
        F16(a3, 2 * 56) = 0;
      }
      if ( *((uint8_t *)a3 + 116) == 0 )
        return v8;
      goto LABEL_216;
    case 0x48:
      if ( a4 != 0 )
        return v8;
      if ( *(_DWORD *)&GAP(c, 0x0, 616) != 0 )
        goto LABEL_216;
      v42 = W_party_find((intptr_t)(c), (intptr_t)(0), (intptr_t)(10113), (intptr_t)(&(*v84__f)), (intptr_t)(0));
      (*v82__f) = v42;
      if ( v42 == 0 )
        goto LABEL_216;
      v43 = F16(v42, 68);
      (*v81__f) = (_DWORD *)a5;
      if ( a5 >= (int)v43 )
      {
        (*v80__f) = F32(v42, 468);
        (*v81__f) = 0;
        g_eng.h5bf3ee(Buffer, "%d", (*v80__f));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85451c)), Buffer);
      }
      v44 = F32((*v82__f), 64);
      v46 = a5;
      (*v85__f) = F32(v44 + 660 * (int32_t)(*v81__f), 44);
      SETHI(v45, HIWORD((*v85__f)));
      SETLO(v45, F16((*v82__f), 68));
      (*v83__f) = v45;
      if ( a5 >= (uint16_t)v45 )
        v46 = 0;
      (*v81__f) = (_DWORD *)(v44 + 660 * v46);
      v47 = a5;
      (*v82__f) = (*v81__f)[1] + (*v81__f)[3] / 2 - a3[3] / 2;
      if ( a5 >= (uint16_t)(*v83__f) )
        v47 = 0;
      (*v83__f) = 3200;
      v48 = F32(v44, 660 * v47 + 20);
      v49 = v44 + 660 * v47;
      v50 = F32(v49, 16) - (int32_t)v48;
      (*v81__f) = v48;
      v51 = v50 / 2 - a3[4] / 2;
      v52 = F32(v49, 8);
      v53 = F16(a3, 2 * 44);
      (*v81__f) = (_DWORD *)((uint8_t *)v48 + v52 + v51 - 800);
      if ( v53 == 4 )
      {
        if ( a3[23] == 20 )
          F16(a3, 2 * 44) = v53 + 1;
      }
      else if ( v53 != 5 )
      {
        if ( (*v84__f) >= 6u )
        {
          F16(a3, 2 * 44) = 4;
          (*v87__f) = *(_DWORD **)&GAP(v9, 0x0, 64);
          v54 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1240)), (intptr_t)(0));
          if ( v54 != 0 )
          {
            v55 = F32(v54, 0);
            if ( W_sfx_place((intptr_t)(v9), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v87__f)), (intptr_t)(&(*v88__f)), (intptr_t)(&(*v86__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v55, (*v88__f));
              g_eng.dsb_pan_5bb880(v55, (*v86__f));
              g_eng.dsb_freq_5bb890(v55, 0);
              g_eng.dsb_play_5bb8c0(v55);
            }
          }
          if ( (_UNKNOWN *)a3[27] != D(0x6421e8) )
          {
            a3[27] = D(0x6421e8);
            *((uint8_t *)a3 + 116) = 0;
            F16(a3, 2 * 57) = 0;
            F16(a3, 2 * 56) = 0;
          }
          if ( *(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 37820) != 0
            && *(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 37828) == 0
            && (*(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 37620) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place((intptr_t)(v9), (intptr_t)((int *)&(*v87__f)), (intptr_t)(&(*v88__f)), (intptr_t)(&(*v86__f))) != 0 )
          {
            W_voice_play((intptr_t)(*(intptr_t *)&GAP((v9 + (1) * 0xc798), 0x0, 37820)), (intptr_t)(0x42u), (intptr_t)(0), (intptr_t)((int)(*v87__f)), (intptr_t)((*v88__f)), (intptr_t)((*v86__f)), (intptr_t)(0));
          }
        }
        goto LABEL_172;
      }
      v56 = a3[23] + 1;
      a3[23] = v56;
      if ( v56 >= 60 )
      {
        ++F16(a3, 2 * 44);
      }
      else if ( v56 >= 20 )
      {
        v57 = SYS32(-4 * ((v56 - 20) / 2) + 10228);
        *(_DWORD *)&GAP(v9, 0x0, 244) = v57;
        *(_DWORD *)&GAP(v9, 0x0, 248) = v57;
      }
      (*v83__f) = 80 * (40 - a3[23]);
LABEL_172:
      v58 = (*v85__f);
      if ( (*v85__f) == (_DWORD *)3 )
        (*v82__f) -= (*v83__f);
      else
        (*v82__f) += (*v83__f);
      v59 = F16(a3, 2 * 44);
      switch ( v59 )
      {
        case 0:
          *(_DWORD *)&GAP(v9, 0x0, 244) = 0;
          *(_DWORD *)&GAP(v9, 0x0, 248) = 0;
          if ( (_UNKNOWN *)a3[27] != D(0x641d20) )
          {
            a3[27] = D(0x641d20);
            *((uint8_t *)a3 + 116) = 0;
            F16(a3, 2 * 57) = 0;
            F16(a3, 2 * 56) = 0;
          }
          ++F16(a3, 2 * 44);
          goto LABEL_179;
        case 1:
LABEL_179:
          v60 = a3[1];
          v61 = (*v82__f) - v60;
          (*v85__f) = (_DWORD *)((uint8_t *)(*v81__f) - a3[2]);
          v62 = abs32((*v82__f) - v60);
          v63 = v62 / 10;
          if ( v62 / 10 > 1600 )
            goto LABEL_183;
          if ( v63 >= 400 )
          {
            if ( v63 > 1600 )
LABEL_183:
              v63 = 1600;
          }
          else
          {
            v63 = 400;
          }
          v64 = v61 < 0;
          if ( v61 > 0 )
          {
            a3[11] = 1;
            a3[1] = v63 + v60;
            v64 = v61 < 0;
          }
          if ( v64 )
          {
            v65 = a3[1];
            a3[11] = 3;
            a3[1] = v65 - v63;
          }
          v66 = abs32((int)(*v85__f));
          v67 = v66 / 10;
          if ( v66 / 10 > 1600 )
            goto LABEL_192;
          if ( v67 >= 400 )
          {
            if ( v67 > 1600 )
LABEL_192:
              v67 = 1600;
          }
          else
          {
            v67 = 400;
          }
          v68 = (int)(*v85__f) < 0;
          if ( (int)(*v85__f) > 0 )
          {
            v68 = (int)(*v85__f) < 0;
            a3[2] += v67;
          }
          if ( v68 )
            a3[2] -= v67;
          if ( v62 >= 800 || v66 >= 800 )
            return 1;
          v18 = D(0x641eb8);
          break;
        case 2:
          if ( *((uint8_t *)a3 + 116) != 0 )
          {
            if ( (_UNKNOWN *)a3[27] != D(0x6421e8) )
            {
              a3[27] = D(0x6421e8);
              *((uint8_t *)a3 + 116) = 0;
              F16(a3, 2 * 57) = 0;
              F16(a3, 2 * 56) = 0;
            }
            F16(a3, 2 * 44) = v59 + 1;
          }
          goto LABEL_207;
        case 3:
        case 4:
        case 5:
LABEL_207:
          v69 = *(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 144);
          *(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 152) = 1;
          if ( v69 == 0 )
            *(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 144) = W_particle_spawn((intptr_t)(v9), (intptr_t)((int)v9), (intptr_t)(100096), (intptr_t)(a3), (intptr_t)(-400), (intptr_t)(400), (intptr_t)(99), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0), (intptr_t)(10));
          v70 = (*v81__f);
          a3[1] = (*v82__f);
          a3[2] = v70;
          a3[11] = v58 != (_DWORD *)3 ? 3 : 1;
          return 1;
        case 6:
          if ( *((uint16_t *)a3 + 43) / 100 > 2 && F16(a3, 2 * 36) != 0 || F16(a3, 2 * 36) == 73 )
            return 1;
          F16(a3, 2 * 44) = 0;
          a3[23] = 0;
          a3[30] = 0;
          F16(a3, 2 * 42) = 0;
          a3[19] = 0;
          a3[20] = 0;
          F16(a3, 2 * 45) = 0;
          F16(a3, 2 * 50) = 0;
          F16(a3, 2 * 36) = 73;
          F16(a3, 2 * 43) = 210;
          return 1;
        default:
          return 1;
      }
      goto LABEL_200;
    case 0x49:
      if ( a4 != 0 )
        return v8;
      if ( *(_DWORD *)&GAP(c, 0x0, 616) != 0 )
      {
LABEL_216:
        F16(a3, 2 * 36) = 0;
        F16(a3, 2 * 43) = 210;
        F16(a3, 2 * 44) = 0;
        a3[23] = 0;
        a3[30] = 0;
        F16(a3, 2 * 42) = 0;
        a3[19] = 0;
        a3[20] = 0;
        F16(a3, 2 * 45) = 0;
        F16(a3, 2 * 50) = 0;
        return 0;
      }
      v71 = W_party_find((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xFu));
      (*v82__f) = v71;
      if ( v71 != 0 )
      {
        SETLO(v9, F16(v71, 68));
        v72 = a5;
        (*v83__f) = (int)v9;
        if ( a5 >= (uint16_t)v9 )
          v72 = 0;
        (*v85__f) = F32(v71, 64);
        v73 = (*v85__f)[165 * v72 + 1] + (*v85__f)[165 * v72 + 3] / 2 - a3[3] / 2;
        v74 = a5;
        if ( a5 >= (uint16_t)(*v83__f) )
          v74 = 0;
        (*v85__f) += 165 * v74;
        v75 = (*v85__f)[4] - (*v85__f)[5];
        (*v86__f) = (*v85__f)[5];
        v76 = a5;
        (*v81__f) = (_DWORD *)((*v86__f) + (*v85__f)[2] + v75 / 2 - a3[4] / 2 - 800);
        if ( a5 >= (unsigned int)(uint16_t)(*v83__f) )
        {
          v76 = 0;
          g_eng.h5bf3ee(Buffer, "%d", F32((*v82__f), 468));
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85451c)), Buffer);
        }
        v77 = (*v81__f);
        v78 = F32(F32(((*v82__f) + 64), 0) + 660 * v76, 44) - 3;
        a3[1] = v73;
        a3[2] = v77;
        a3[11] = v78 != 0 ? 3 : 1;
        return 1;
      }
      if ( (*((uint16_t *)a3 + 43) / 0x64u <= 2 || F16(a3, 2 * 36) == 0) && F16(a3, 2 * 36) != 74 )
      {
        F16(a3, 2 * 36) = 74;
        F16(a3, 2 * 43) = 210;
        F16(a3, 2 * 44) = 0;
        a3[23] = 0;
        a3[30] = 0;
        F16(a3, 2 * 42) = 0;
        a3[19] = 0;
        a3[20] = 0;
        F16(a3, 2 * 45) = 0;
        F16(a3, 2 * 50) = 0;
        return 0;
      }
      return v8;
    case 0x4A:
      if ( a4 != 0 )
        return v8;
      if ( F16(a3, 2 * 44) != 0 )
      {
        v27 = *((uint16_t *)a3 + 44) - 1;
        if ( F16(a3, 2 * 44) != 1 )
          goto LABEL_114;
      }
      else
      {
        if ( (_UNKNOWN *)a3[27] != D(0x6421e8) )
        {
          a3[27] = D(0x6421e8);
          *((uint8_t *)a3 + 116) = 0;
          F16(a3, 2 * 57) = 0;
          F16(a3, 2 * 56) = 0;
        }
        (*v81__f) = *(_DWORD **)&GAP(c, 0x0, 64);
        v28 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1244)), (intptr_t)(0));
        if ( v28 != 0 )
        {
          v29 = F32(v28, 0);
          if ( W_sfx_place((intptr_t)(v9), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v81__f)), (intptr_t)(&(*v83__f)), (intptr_t)(&(*v82__f))) != 0 )
          {
            g_eng.dsb_volume_5bb870(v29, (*v83__f));
            g_eng.dsb_pan_5bb880(v29, (*v82__f));
            g_eng.dsb_freq_5bb890(v29, 0);
            g_eng.dsb_play_5bb8c0(v29);
          }
        }
        ++F16(a3, 2 * 44);
      }
      v30 = a3[23] + 1;
      a3[23] = v30;
      if ( v30 < 20 )
      {
        v31 = SYS32(4 * (v30 / 2) + 10192);
        *(_DWORD *)&GAP(v9, 0x0, 244) = v31;
        *(_DWORD *)&GAP(v9, 0x0, 248) = v31;
      }
      if ( (int)a3[23] <= 40 )
        return 1;
      if ( (_UNKNOWN *)a3[27] != D(0x642050) )
      {
        a3[27] = D(0x642050);
        *((uint8_t *)a3 + 116) = 0;
        F16(a3, 2 * 57) = 0;
        F16(a3, 2 * 56) = 0;
      }
      *(_DWORD *)&GAP(v9, 0x0, 244) = 0;
      *(_DWORD *)&GAP(v9, 0x0, 248) = 0;
      ++F16(a3, 2 * 44);
      return 1;
    case 0x4B:
      v25 = F16(a3, 2 * 44);
      switch ( v25 )
      {
        case 0u:
          if ( (_UNKNOWN *)a3[27] != D(0x642380) )
          {
            a3[27] = D(0x642380);
            *((uint8_t *)a3 + 116) = 0;
            F16(a3, 2 * 57) = 0;
            F16(a3, 2 * 56) = 0;
          }
          F16(a3, 2 * 44) = v25 + 1;
          goto LABEL_92;
        case 1u:
        case 3u:
LABEL_92:
          if ( *((uint8_t *)a3 + 116) == 0 )
            return 1;
          v18 = D(0x642518);
          break;
        case 2u:
          if ( *((uint8_t *)a3 + 116) == 0 )
            return 1;
          v26 = D(0x642380);
          goto LABEL_119;
        case 4u:
          goto LABEL_115;
        default:
          return 1;
      }
      goto LABEL_200;
    case 0x4C:
      if ( F16(a3, 2 * 44) != 0 )
      {
        if ( F16(a3, 2 * 44) != 1 )
          goto LABEL_84;
      }
      else
      {
        if ( (_UNKNOWN *)a3[27] != D(0x6426b0) )
        {
          a3[27] = D(0x6426b0);
          *((uint8_t *)a3 + 116) = 0;
          F16(a3, 2 * 57) = 0;
          F16(a3, 2 * 56) = 0;
        }
        F16(a3, 2 * 44) = 1;
        a3[10] = 64000;
        if ( a4 == 0
          && *(_DWORD *)&GAP(c, 0xc798, 37820) != 0
          && *(_DWORD *)&GAP(c, 0xc798, 37828) == 0
          && (*(_DWORD *)&GAP(c, 0xc798, 37620) != 1 || F32(CFG(), 400) != 0)
          && W_voice_place((intptr_t)(c), (intptr_t)(&(*v82__f)), (intptr_t)(&(*v83__f)), (intptr_t)(&(*v81__f))) != 0 )
        {
          W_voice_play((intptr_t)(*(intptr_t *)&GAP((v9 + (1) * 0xc798), 0x0, 37820)), (intptr_t)(6u), (intptr_t)(0), (intptr_t)((*v82__f)), (intptr_t)((*v83__f)), (intptr_t)((int)(*v81__f)), (intptr_t)(0));
        }
      }
      a3[10] = a3[10] - 1000 < 0 ? 0 : a3[10] - 1000;
      if ( *((uint8_t *)a3 + 116) != 0 )
      {
        F16(a3, 2 * 36) = 0;
        F16(a3, 2 * 43) = 210;
        F16(a3, 2 * 44) = 0;
        a3[23] = 0;
        a3[30] = 0;
        F16(a3, 2 * 42) = 0;
        a3[19] = 0;
        a3[20] = 0;
        F16(a3, 2 * 45) = 0;
        F16(a3, 2 * 50) = 0;
      }
LABEL_84:
      if ( a3[11] == 1 )
        a3[1] += a3[10] / 100;
      else
        a3[1] += a3[10] / -100;
      return 1;
    case 0x4D:
      if ( F16(a3, 2 * 44) != 0 )
      {
        if ( F16(a3, 2 * 44) == 1 )
        {
          v32 = a3[23];
          if ( v32 >= 60 )
          {
            *(_DWORD *)&GAP(c, 0x0, 244) = 0;
            *(_DWORD *)&GAP(c, 0x0, 248) = 0;
            ++F16(a3, 2 * 44);
          }
          else
          {
            v33 = SYS32(4 * (v32 / 4) + 10152);
            *(_DWORD *)&GAP(c, 0x0, 244) = v33;
            *(_DWORD *)&GAP(c, 0x0, 248) = v33;
            ++a3[23];
          }
        }
      }
      else
      {
        if ( (_UNKNOWN *)a3[27] != D(0x6421e8) )
        {
          a3[27] = D(0x6421e8);
          *((uint8_t *)a3 + 116) = 0;
          F16(a3, 2 * 57) = 0;
          F16(a3, 2 * 56) = 0;
        }
        F16(a3, 2 * 44) = 1;
        v34 = SYS32(10152);
        v35 = *(_DWORD **)&GAP(c, 0x0, 64);
        *(_DWORD *)&GAP(c, 0x0, 244) = SYS32(10152);
        *(_DWORD *)&GAP(c, 0x0, 248) = v34;
        (*v81__f) = v35;
        if ( a4 == 0 )
        {
          v36 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1240)), (intptr_t)(0));
          if ( v36 != 0 )
          {
            v37 = F32(v36, 0);
            if ( W_sfx_place((intptr_t)(v9), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v81__f)), (intptr_t)(&(*v83__f)), (intptr_t)(&(*v82__f))) != 0 )
            {
              g_eng.dsb_volume_5bb870(v37, (*v83__f));
              g_eng.dsb_pan_5bb880(v37, (*v82__f));
              g_eng.dsb_freq_5bb890(v37, 0);
              g_eng.dsb_play_5bb8c0(v37);
            }
            v7 = a4;
          }
        }
      }
      v38 = F16(a3, 2 * 44);
      if ( v38 == 2 )
      {
        v39 = W_speed_ramp((intptr_t)(a3[6]), (intptr_t)(100), (intptr_t)(-400), (intptr_t)(-4000));
        a3[6] = v39;
        if ( v39 <= -4000 )
          F16(a3, 2 * 44) = 3;
      }
      else if ( v38 == 3 )
      {
        v40 = W_speed_ramp((intptr_t)(a3[6]), (intptr_t)(100), (intptr_t)(-400), (intptr_t)(4000));
        a3[6] = v40;
        if ( v40 >= 4000 )
          F16(a3, 2 * 44) = 2;
      }
      a3[2] += a3[6] / 100;
      (*v79__f) = 1600 * rnd() / 0x8000;
      v41 = rnd();
      W_particle_spawn((intptr_t)(v9), (intptr_t)((int)v9), (intptr_t)(100097), (intptr_t)(a3), (intptr_t)(6400 * v41 / 0x8000 - 3200), (intptr_t)((*v79__f)), (intptr_t)(99), (intptr_t)(4), (intptr_t)(v7), (intptr_t)(0), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      return 1;
    case 0x4E:
      v25 = F16(a3, 2 * 44);
      a3[6] = 0;
      a3[10] = 0;
      if ( v25 == 0 )
      {
        *(_DWORD *)&GAP(v9, 0x0, 244) = 0;
        *(_DWORD *)&GAP(v9, 0x0, 248) = 0;
        v18 = D(0x642d10);
        goto LABEL_200;
      }
      v27 = v25 - 1;
      if ( v25 != 1 )
      {
LABEL_114:
        if ( v27 != 1 )
          return 1;
        goto LABEL_115;
      }
      if ( *((uint8_t *)a3 + 116) == 0 )
        return 1;
      v26 = D(0x642ea8);
LABEL_119:
      if ( (void *)a3[27] != v26 )
      {
        a3[27] = v26;
        *((uint8_t *)a3 + 116) = 0;
        F16(a3, 2 * 57) = 0;
        F16(a3, 2 * 56) = 0;
      }
      F16(a3, 2 * 44) = v25 + 1;
      return 1;
    case 0x64:
    case 0x65:
      *(_DWORD *)&GAP(c, 0x0, 244) = 0;
      *(_DWORD *)&GAP(c, 0x0, 248) = 0;
      v12 = F16(a3, 2 * 44);
      if ( v12 != 0 )
      {
        if ( F16(a3, 2 * 44) != 1 )
        {
          if ( F16(a3, 2 * 44) == 2 )
          {
            v14 = F16(a3, 2 * 36) != 101 ? 630 : 250;
            v15 = a3[23] + 1;
            a3[23] = v15;
            if ( v15 > v14 )
            {
              F16(a3, 2 * 44) = v12 + 1;
              if ( (_UNKNOWN *)a3[27] != D(0x6431d8) )
              {
                a3[27] = D(0x6431d8);
                *((uint8_t *)a3 + 116) = 0;
                F16(a3, 2 * 57) = 0;
                F16(a3, 2 * 56) = 0;
              }
              a3[6] = -32000;
              return 1;
            }
          }
          else
          {
            v13 = a3[6] + 4000 > 0 ? 0 : a3[6] + 4000;
            a3[6] = v13;
            a3[2] += v13 / 100;
LABEL_115:
            if ( *((uint8_t *)a3 + 116) != 0 )
            {
              F16(a3, 2 * 36) = 0;
              F16(a3, 2 * 44) = 0;
              a3[23] = 0;
              a3[30] = 0;
              F16(a3, 2 * 42) = 0;
              a3[19] = 0;
              a3[20] = 0;
              F16(a3, 2 * 45) = 0;
              F16(a3, 2 * 50) = 0;
              F16(a3, 2 * 43) = 210;
              return 1;
            }
          }
          return 1;
        }
      }
      else
      {
        if ( (_UNKNOWN *)a3[27] != D(0x643040) )
        {
          a3[27] = D(0x643040);
          *((uint8_t *)a3 + 116) = 0;
          F16(a3, 2 * 57) = 0;
          F16(a3, 2 * 56) = 0;
        }
        F16(a3, 2 * 44) = 1;
      }
      v16 = F16(a3, 2 * 36) != 101 ? 520 : 200;
      v17 = a3[23] + 1;
      a3[23] = v17;
      if ( v17 <= v16 )
        return 1;
      v18 = D(0x643370);
LABEL_200:
      if ( (void *)a3[27] != v18 )
      {
        a3[27] = v18;
        *((uint8_t *)a3 + 116) = 0;
        F16(a3, 2 * 57) = 0;
        F16(a3, 2 * 56) = 0;
      }
      ++F16(a3, 2 * 44);
      return 1;
    case 0x70:
    case 0x71:
      *(_DWORD *)&GAP(c, 0x0, 244) = 0;
      *(_DWORD *)&GAP(c, 0x0, 248) = 0;
      if ( F16(a3, 2 * 44) != 0 )
      {
        if ( F16(a3, 2 * 44) != 1 )
          goto LABEL_66;
      }
      else
      {
        if ( (_UNKNOWN *)a3[27] != D(0x642848) )
        {
          a3[27] = D(0x642848);
          *((uint8_t *)a3 + 116) = 0;
          F16(a3, 2 * 57) = 0;
          F16(a3, 2 * 56) = 0;
        }
        F16(a3, 2 * 44) = 1;
        if ( a4 == 0 )
        {
          v20 = *(intptr_t *)&GAP(c, 0xc798, 37820);
          if ( v20 != 0
            && *(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 37828) == 0
            && (*(_DWORD *)&GAP((v9 + (1) * 0xc798), 0x0, 37620) != 1 || F32(CFG(), 400) != 0) )
          {
            if ( *(int *)&GAP(v9, 0x0, 51052) <= 0
              || (F16(v20, 0x4) <= 0xEu ? (v21 = 0) : (v21 = F16(F32(v20, 0x0), 18146)),
                  v21 >= (uint16_t)W_voice_group_prio((intptr_t)(v20), (intptr_t)(*(_WORD *)&GAP(v9, 0x0, 51056)))) )
            {
              *(_DWORD *)&GAP(v9, 0x0, 51052) = 50;
              *(_WORD *)&GAP(v9, 0x0, 51056) = 14;
              *(_DWORD *)&GAP(v9, 0x0, 51060) = 1;
              *(_DWORD *)&GAP(v9, 0x0, 51064) = 0;
            }
          }
        }
      }
      v22 = a3[10] + 4000;
      v23 = a3[23] + 1;
      v64 = a3[10] - 28000 < 0;
      a3[23] = v23;
      if ( !(v64 ^ __OFSUB__(v22, 32000) | (v22 == 32000)) )
        v22 = 32000;
      a3[10] = v22;
      if ( v23 > 30 )
      {
        a3[10] = -v22;
        v24 = a3[11] - 3;
        a3[23] = 0;
        a3[11] = v24 != 0 ? 3 : 1;
      }
LABEL_66:
      if ( a3[11] == 1 )
        a3[1] += a3[10] / 100;
      else
        a3[1] += a3[10] / -100;
      return 1;
    default:
      return v8;
  }
}

/* 0x45e260: _DWORD *__thiscall sub_45E260(cls_45E260 *this, int *a2, _DWORD *a3, int a4) */
int32_t ih_45e260(wl_char *self, int32_t * a2, int32_t * a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    int32_t * v4 = 0;
    int32_t * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    int32_t * result = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    int32_t * *const v17__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  v4 = F32(a4, 64);
  v5 = (_DWORD *)F32(c, 0x40);
  (*v18__f) = v4[3];
  v6 = v4[4] - v4[5];
  (*v17__f) = 0;
  (*v15__f) = 1;
  (*v16__f) = 0;
  v7 = -25600;
  v8 = 25600;
  (*v14__f) = 0;
  do
  {
    (*v13__f) = v8;
    if ( v5[11] != 3 )
      (*v13__f) = v7;
    if ( ih_45e3e0((*v13__f) + v5[1] - v5[3] / 2, v5[2] - v5[4] / 2, (*v18__f), v6) != 0
      && ((int)abs32((*v14__f)) > (int)abs32(v8) || (*v15__f) != 0) )
    {
      (*v17__f) = (int *)v7;
      (*v14__f) = v8;
      (*v16__f) = 1;
      (*v15__f) = 0;
    }
    v8 -= 1600;
    v7 += 1600;
  }
  while ( v8 > -25600 );
  if ( (*v16__f) != 0 )
    v9 = (*v17__f);
  else
    v9 = 0;
  if ( &a2 == 0 )
    v9 = a2;
  if ( (*v16__f) != 0 )
    v10 = F32(F32(c, 0x40), 4) + F32(F32(c, 0x40), 12) / 2;
  else
    v10 = (int)v9 + F32(F32(c, 0x40), 4) + F32(F32(c, 0x40), 12) / 2;
  *a2 = v10;
  result = a3;
  *a3 = F32(F32(c, 0x40), 8)
      + F32(F32(c, 0x40), 16)
      + F32(F32((a4 + 64), 0), 20)
      - F32(F32((a4 + 64), 0), 16);
  return result;
}

/* 0x45e3e0: int __stdcall sub_45E3E0(int a1, int a2, int a3, int a4) */
int32_t ih_45e3e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t * v10 = 0;

  v4 = (a1 - a3 / 2) / 3200;
  v5 = (a1 + a3 / 2) / 3200 + 1;
  v6 = (a2 - a4 / 2) / 3200;
  v7 = (a2 + a4 / 2) / 3200 + 1;
  if ( v4 >= 0 )
  {
    v8 = *(_DWORD *)&GAP(W, 0x1030, 24);
    if ( v5 <= F32(v8, 2887728) && v6 >= 0 && v7 <= F32(v8, 2887732) )
    {
      if ( v6 >= v7 )
        return 1;
      while ( 1 )
      {
        v9 = (a1 - a3 / 2) / 3200;
        if ( v4 < v5 )
          break;
LABEL_10:
        if ( ++v6 >= v7 )
          return 1;
      }
      v10 = (_WORD *)(v8 + 2 * (v6 + (v4 << 7)) + 2887744);
      while ( *v10 != 1 )
      {
        ++v9;
        v10 += 128;
        if ( v9 >= v5 )
          goto LABEL_10;
      }
    }
  }
  return 0;
}

/* 0x461640: int __stdcall sub_461640(int a1, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t ih_461640(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{

    intptr_t result = 0;
    char v8 = 0;

  result = 0;
  ++F32(a1, 92);
  switch ( F16(a1, 88) )
  {
    case 0:
      if ( F32(a1, 108) != a5 )
      {
        F32(a1, 108) = a5;
        F8(a1, 116) = 0;
        F16(a1, 114) = 0;
        F16(a1, 112) = 0;
      }
      F16(a1, 88) = 1;
      result = 1;
      goto LABEL_5;
    case 1:
LABEL_5:
      v8 = F8(a1, 116);
      F32(a1, 28) = a6;
      if ( v8 != 0 )
      {
        if ( F32(a1, 108) != a5 + 404 )
        {
          F32(a1, 108) = a5 + 404;
          F8(a1, 116) = 0;
          F16(a1, 114) = 0;
          F16(a1, 112) = 0;
        }
        F16(a1, 88) = 2;
        result = 1;
      }
      if ( a4 != 0 )
        goto LABEL_20;
      return result;
    case 2:
      F32(a1, 28) = a7;
      if ( a4 != 0 )
      {
        if ( F32(a1, 108) != a5 + 808 )
        {
          F32(a1, 108) = a5 + 808;
          F8(a1, 116) = 0;
          F16(a1, 114) = 0;
          F16(a1, 112) = 0;
        }
        F16(a1, 88) = 3;
        return 1;
      }
      return result;
    case 3:
      if ( F8(a1, 116) != 0 )
      {
        if ( F32(a1, 108) != a5 + 1212 )
        {
          F32(a1, 108) = a5 + 1212;
          F8(a1, 116) = 0;
          F16(a1, 114) = 0;
          F16(a1, 112) = 0;
        }
        F16(a1, 88) = 4;
        return 1;
      }
      return result;
    case 4:
      if ( F8(a1, 116) != 0 )
      {
LABEL_20:
        F16(a1, 72) = 0;
        F16(a1, 86) = 210;
        F16(a1, 88) = 0;
        F32(a1, 92) = 0;
        F32(a1, 120) = 0;
        F16(a1, 84) = 0;
        F32(a1, 76) = 0;
        F32(a1, 80) = 0;
        F16(a1, 90) = 0;
        F16(a1, 100) = 0;
      }
      break;
    default:
      return result;
  }
  return result;
}

/* 0x467330: BOOL __thiscall sub_467330(cls_4022A0 *this) */
int32_t ih_467330(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;

  v1 = F32(c, 0x15b60);
  if ( v1 == 0 )
    return false;
  if ( F32(v1, 464) == 0 )
    return true;
  return F32(v1, 464) != 1 || F8(F32(v1, 64), 0) != 1 || F16(F32(v1, 1872), 1098) != 0;
}
/* unported callees: none */
