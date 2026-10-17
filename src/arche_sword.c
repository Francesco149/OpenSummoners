/* arche_sword.c — FUN_00461790, Arche's (50010) sword skills.  See arche_sword.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (the decompile's control flow kept verbatim: labels, gotos across
 * the cases; IDA's local types, fields through width-typed accessors), then finished by hand against the asm.
 * Retail returns the skill-handled flag (0 = the integrator runs the physics step). */
#include "arche_sword.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "arche_pose.h"
#include "char_attack.h"
#include "char_def.h"
#include "char_integrate.h"
#include "char_physics.h"
#include "char_status.h"
#include "engine_calls.h"
#include "entity_move.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "particle_spawn.h"
#include "rng.h"
#include "sound_fx.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }

static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }

uint8_t *arche_combo_skill(wl_char *c)
{
    const uint8_t *pt = (const uint8_t *)c->party;
    if (ld16(pt, 0x466) == 0) return NULL;
    uint8_t *rec = NULL;
    for (uint16_t i = 0, n = ld16(c, 0xc754); i < n; i++)
        if (ld32(c, 0x754 + 0x300u * i) == 10115) { rec = (uint8_t *)c + 0x754 + 0x300u * i; break; }
    if (rec == NULL) return NULL;
    int32_t cls = ld32(rec, 0xc0);
    switch (cls) {
    case 0xf00000: case 0xf00002: break;
    case 0xf00001: return NULL;
    case 0xf00003: if (ld32(pt, 0xd4) != 1) return NULL; break;
    default: if (cls != ld32(pt, 0xd4)) return NULL; break;
    }
    switch (ld32(rec, 0x2f8)) {
    case 1: case 2: case 3: return rec;
    case 255: return c->base.coll_off != 0 ? rec : NULL;
    default: return NULL;
    }
}

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
static inline intptr_t W_arche_combo_skill(intptr_t a0, ...) { return (intptr_t)arche_combo_skill((wl_char *)a0); }
static inline intptr_t W_body_action_reset(intptr_t a0, ...) { body_action_reset((wl_body *)a0); return 0; }
static inline intptr_t W_body_action_start(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)body_action_start((wl_body *)a0, (int16_t)a1, (int32_t)a2); }
static inline intptr_t W_body_facing_sign(intptr_t a0, intptr_t a1, ...) { return (intptr_t)body_facing_sign((const wl_body *)a0, (int32_t)a1); }
static inline intptr_t W_body_hit_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, intptr_t a20, intptr_t a21, intptr_t a22, intptr_t a23, ...) { return (intptr_t)body_hit_spawn((wl_char *)a0, (wl_char *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19, (int32_t)a20, (int32_t)a21, (int32_t)a22, (int32_t)a23); }
static inline intptr_t W_body_pose_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { body_pose_set((wl_body *)a0, (uint16_t)a1, (uint16_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1, ...) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_char_atk_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)char_atk_roll((const wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_char_attack_hits(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)char_attack_hits((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (const uint8_t *)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_char_drift(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)char_drift((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12); }
static inline intptr_t W_char_lunge(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)char_lunge((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_char_mag_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_mag_roll((const wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_char_mag_sum(intptr_t a0, ...) { return (intptr_t)char_mag_sum((const wl_char *)a0); }
static inline intptr_t W_char_part_top(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_part_top((const wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_char_particle(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)char_particle((wl_char *)a0, (int32_t)a1, (const wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12); }
static inline intptr_t W_char_party_mode_set(intptr_t a0, intptr_t a1, ...) { char_party_mode_set((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_char_shot_fire(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)char_shot_fire((wl_char *)a0, (int32_t)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int16_t)a7, (int32_t)a8, (const uint8_t *)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_char_skill_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skill_find((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_char_skill_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_skill_ok((wl_char *)a0, (const wl_skill *)a1, (int32_t)a2); }
static inline intptr_t W_char_skill_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_skill_roll((const wl_char *)a0, (const uint8_t *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_crate_push(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)crate_push((wl_entity *)a0, (const int32_t *)a1, (wl_body *)a2, (int32_t)a3, (uint16_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_entity_move_horizontal(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_move_horizontal((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (uint16_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_slide_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_slide_step((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t *)a3); }
static inline intptr_t W_entity_sprite_descs(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_sprite_descs((const wl_entity *)a0, (const wl_body *)a1, (es_desc *)a2, (uint16_t *)a3); }
static inline intptr_t W_entity_track_on(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_track_on((wl_entity *)a0, (uint16_t)a1, (int16_t)a2, (int16_t)a3); }
static inline intptr_t W_facing_back(intptr_t a0, ...) { return (intptr_t)facing_back((int32_t)a0); }
static inline intptr_t W_orbit_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)orbit_init((int32_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int16_t)a6, (int16_t)a7, (int32_t)a8); }
static inline intptr_t W_orbit_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)orbit_step((wl_char *)a0, (const wl_body *)a1, (int32_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_part_bottom(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_bottom((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_left(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_left((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_right(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_right((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_particle_tint_set(intptr_t a0, intptr_t a1, ...) { particle_tint_set((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_polar_div(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { polar_div((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t *)a5, (int32_t *)a6); return 0; }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_shot_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, ...) { return (intptr_t)shot_spawn((void *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (wl_body *)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int16_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19); }
static inline intptr_t W_slash_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)slash_step((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (intptr_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_slash_trail(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)slash_trail((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_stat_jitter(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)stat_jitter((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_voice_cue(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)voice_cue((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_voice_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)voice_place((const wl_char *)a0, (int32_t *)a1, (int32_t *)a2, (int32_t *)a3); }
static inline intptr_t W_voice_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)voice_play((wl_voiceset *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }


/* 0x461790: int __thiscall sub_461790(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, _DWORD *a7) */
int32_t arche_sword(wl_char *self, wl_body *body, int32_t probe, int32_t *ctl, int32_t grounded, int32_t part, const wl_cmd *cmd)
{
    intptr_t c = (intptr_t)self, a2 = (intptr_t)body, a3 = probe, a5 = grounded, a6 = part;
    int32_t *a4 = ctl, *a7 = (int32_t *)cmd;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    uint32_t v13;
    uint16_t v14;
    void * v15;
    int16_t v16;
    intptr_t v17;
    int16_t v18;
    intptr_t v19;
    uint32_t v20;
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
    intptr_t v41;
    intptr_t v42;
    intptr_t v43;
    intptr_t v44;
    int32_t * v45;
    uint16_t v46;
    intptr_t v47;
    intptr_t v48;
    intptr_t v49;
    intptr_t v50;
    intptr_t v51;
    int16_t v52;
    intptr_t v53;
    char v54;
    intptr_t v55;
    intptr_t v56;
    intptr_t * v57;
    intptr_t v58;
    intptr_t v59;
    intptr_t v60;
    intptr_t * v61;
    intptr_t v62;
    intptr_t v63;
    intptr_t v64;
    intptr_t v65;
    intptr_t v66;
    int32_t * v67;
    intptr_t v68;
    intptr_t v69;
    intptr_t v70;
    intptr_t v71;
    intptr_t v72;
    intptr_t v73;
    intptr_t v74;
    intptr_t v75;
    intptr_t v76;
    intptr_t v77;
    int32_t * v78;
    intptr_t v79;
    intptr_t v80;
    intptr_t v81;
    intptr_t v82;
    intptr_t v83;
    intptr_t v85;
    intptr_t v86;
    int v87;
    int32_t * v88;
    uint16_t v89;
    intptr_t v90;
    intptr_t v91;
    intptr_t v92;
    intptr_t v93;
    intptr_t v94;
    int16_t v95;
    intptr_t v96;
    intptr_t v97;
    intptr_t * v98;
    intptr_t v99;
    intptr_t v100;
    intptr_t v101;
    int v102;
    char v103;
    char v104;
    intptr_t v105;
    intptr_t v106;
    uint16_t v107;
    intptr_t v108;
    int32_t * v109;
    uint16_t v110;
    intptr_t v111;
    intptr_t v112;
    intptr_t v113;
    intptr_t v114;
    intptr_t v115;
    intptr_t v116;
    intptr_t v117;
    intptr_t * v118;
    intptr_t v119;
    intptr_t v120;
    intptr_t v121;
    intptr_t v122;
    intptr_t v123;
    intptr_t v124;
    intptr_t v125;
    intptr_t v126;
    intptr_t v127;
    intptr_t v128;
    intptr_t v129;
    int32_t * v130;
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
    int32_t * v141;
    intptr_t v142;
    intptr_t v143;
    intptr_t v144;
    intptr_t v145;
    intptr_t v146;
    int32_t * v147;
    uint16_t v148;
    intptr_t v149;
    intptr_t v150;
    intptr_t v151;
    intptr_t v152;
    intptr_t v153;
    intptr_t v154;
    intptr_t v155;
    intptr_t v156;
    intptr_t v157;
    intptr_t v158;
    int32_t * v159;
    intptr_t v160;
    intptr_t v161;
    intptr_t v162;
    intptr_t v163;
    intptr_t v164;
    intptr_t * v165;
    intptr_t v166;
    intptr_t v167;
    intptr_t v168;
    intptr_t v169;
    intptr_t v170;
    intptr_t v171;
    intptr_t v172;
    int32_t * v173;
    intptr_t v174;
    intptr_t v175;
    intptr_t v176;
    intptr_t v177;
    intptr_t v178;
    int32_t * v179;
    uint16_t v180;
    intptr_t v181;
    intptr_t v182;
    intptr_t v183;
    intptr_t v184;
    intptr_t v185;
    intptr_t v186;
    intptr_t v187;
    intptr_t v188;
    intptr_t * v189;
    intptr_t v190;
    intptr_t v191;
    intptr_t v192;
    intptr_t v193;
    intptr_t v194;
    intptr_t v195;
    intptr_t v196;
    intptr_t v197;
    int16_t v198;
    intptr_t v199;
    intptr_t v200;
    intptr_t v201;
    intptr_t v202;
    intptr_t v203;
    int32_t * v204;
    intptr_t v205;
    intptr_t v206;
    intptr_t v207;
    intptr_t v208;
    intptr_t v209;
    int16_t v210;
    intptr_t v211;
    intptr_t * v212;
    intptr_t v213;
    intptr_t v214;
    intptr_t v215;
    intptr_t v216;
    intptr_t v217;
    intptr_t v218;
    uint32_t v219;
    intptr_t v220;
    intptr_t v221;
    intptr_t v222;
    int16_t v223;
    intptr_t v224;
    intptr_t * v225;
    intptr_t v226;
    intptr_t v227;
    intptr_t * v228;
    intptr_t v229;
    intptr_t v230;
    intptr_t v231;
    intptr_t v232;
    intptr_t v233;
    intptr_t v234;
    int32_t * v235;
    intptr_t v236;
    intptr_t v237;
    intptr_t v238;
    intptr_t v239;
    intptr_t v240;
    intptr_t v241;
    intptr_t * v242;
    intptr_t v243;
    uint32_t v244;
    intptr_t v245;
    intptr_t v246;
    intptr_t v247;
    uint16_t v248;
    int v249;
    intptr_t v250;
    intptr_t v251;
    intptr_t v252;
    intptr_t v253;
    intptr_t v254;
    char v255;
    intptr_t v256;
    int16_t v257;
    char v258;
    intptr_t v259;
    intptr_t v260;
    intptr_t v261;
    intptr_t v262;
    intptr_t v263;
    intptr_t v264;
    uint16_t v265;
    intptr_t v266;
    intptr_t v267;
    intptr_t v268;
    intptr_t v269;
    intptr_t v270;
    intptr_t v271;
    intptr_t v272;
    intptr_t v273;
    _Alignas(8) uint8_t frame_[0x2d0 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v303 = (uint16_t *)(frame_ + 0x3c);   /* [2] at ebp-0x294 */
    intptr_t *v308 = (intptr_t *)(frame_ + 0x50);   /* [2] at ebp-0x280 */
    int16_t *v309 = (int16_t *)(frame_ + 0x58);   /* [316] at ebp-0x278 */
    intptr_t *const v274__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2d0 */
    intptr_t *const v277__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c4 */
    intptr_t *const v276__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c4 */
    intptr_t *const v275__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c4 */
    intptr_t *const v283__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x2c0 */
    intptr_t *const v282__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x2c0 */
    intptr_t *const v281__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x2c0 */
    int32_t * *const v280__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x2c0 */
    int32_t * *const v279__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x2c0 */
    int32_t * *const v278__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x2c0 */
    intptr_t *const v290__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v289__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v288__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v287__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v286__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v285__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v284__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x2bc */
    intptr_t *const v298__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v297__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v296__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v295__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v294__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v293__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v292__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const v291__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b8 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x2a4 */
    intptr_t *const v300__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x2a0 */
    intptr_t *const v301__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x29c */
    intptr_t *const v302__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x298 */
    intptr_t *const v304__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x290 */
    intptr_t *const v305__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x28c */
    int32_t * *const v306__f = (int32_t * *)(frame_ + 0x48);   /* ebp-0x288 */
    intptr_t *const v307__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x284 */

  (*i__f) = 1000;
  v8 = F32(c, 0x750);
  (*v302__f) = (int)D(0x6327c8);
  F32(v303, 0) = F32(v8, 212) == 1;
  v102 = F16(a2, 88) == 0;
  (*v301__f) = 0;
  (*v307__f) = 0;
  (*v300__f) = 0;
  if ( v102 )
  {
    v9 = a5;
    if ( F32(a2, 652) != 0 )
      v9 = 1;
    if ( F16(c, 0x158bc) < F16(c, 0xc7e8) )
      v9 = 1;
    if ( F16(a2, 72) == 10101 )
    {
      if ( v9 != 0 )
      {
        if ( F16(a2, 84) == 6 && v9 == 1 )
        {
          W_body_action_reset(a2);
          (*v291__f) = W_char_skill_find(c, 0x2775u);
          W_body_pose_set(a2, 10101, F16(a2, 86), 0, F32(a2, 80), (*v291__f));
        }
      }
      else
      {
        W_body_action_reset(a2);
        v10 = W_char_skill_find(c, 0x2775u);
        W_body_pose_set(a2, 10101, F16(a2, 86), 6, F32(a2, 80), v10);
      }
    }
    if ( F16(F32(c, 0x750), 1126) != 0 )
    {
      v11 = W_char_skill_find(c, 0x2783u);
      v12 = v11;
      if ( v11 != 0 && W_char_skill_ok(c, v11, 0) != 0 )
      {
        switch ( F16(a2, 72) )
        {
          case 0x2775:
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
            if ( W_body_action_start(a2, 10115, 210) == 0 )
              goto LABEL_28;
            F16(a2, 84) = 0;
            break;
          case 0x2778:
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
            if ( W_body_action_start(a2, 10115, 210) == 0 )
              goto LABEL_28;
            F16(a2, 84) = 1;
            break;
          case 0x277D:
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
            if ( W_body_action_start(a2, 10115, 210) == 0 )
              goto LABEL_28;
            F16(a2, 84) = 2;
            break;
          default:
            goto LABEL_28;
        }
        F32(a2, 76) = v12;
        F32(a2, 80) = 0;
        v13 = F32(W, 0x27a0);
        if ( v13 >= 0xFFFFFF )
          F32(W, 0x27a0) = 0;
        else
          F32(W, 0x27a0) = v13 + 1;
        F32(a2, 96) = v13 + 1;
      }
    }
  }
LABEL_28:
  v14 = F16(a2, 72);
  if ( v14 > 0x277Bu )
  {
    switch ( F16(a2, 72) )
    {
      case 0x277C:
        *a4 = 3;
        if ( a5 == 0 )
          goto LABEL_200;
        break;
      case 0x277D:
        goto LABEL_699;
      case 0x277E:
        F8(a2, 124) = 1;
        v222 = 0;
        *a4 = 3;
        v223 = F16(a2, 88);
        ++F32(a2, 92);
        switch ( v223 )
        {
          case 0:
            W_body_set_clip(a2, (int)D(0x62ea20));
            v222 = 1;
            F16(a2, 88) = 1;
            goto LABEL_538;
          case 1:
LABEL_538:
            if ( F8(a2, 116) == 0 )
              goto LABEL_550;
            W_body_set_clip(a2, (int)D(0x62ebb4));
            F16(a2, 88) = 2;
            F32(a2, 24) = -40000;
            F32(a2, 40) = 64000;
            F32(a2, 92) = 0;
            goto LABEL_551;
          case 2:
            if ( F32(a2, 24) > 0 )
            {
              F16(a2, 88) = 3;
              v222 = 1;
            }
            goto LABEL_542;
          case 3:
LABEL_542:
            if ( F32(a2, 92) <= 16 )
            {
              if ( W_char_drift(c, a2, a3, a6, 64000, 4000, -1000, 40000, 3000, -1000, 1, 0, 0) != 0 )
              {
LABEL_550:
                if ( v222 == 0 )
                  goto LABEL_566;
              }
              else
              {
                F16(a2, 88) = 4;
LABEL_546:
                F32(a2, 40) = -32000;
                F32(a2, 24) = -40000;
                W_body_set_clip(a2, (int)D(0x62f070));
                F16(a2, 88) = 6;
              }
            }
            else
            {
              F16(a2, 88) = 5;
              W_body_set_clip(a2, (int)D(0x62eedc));
            }
LABEL_551:
            if ( F16(a2, 88) == 2 )
            {
              v224 = 1000 * rnd();
              (*v301__f) = F32(c, 0x40);
              if ( a3 == 0 )
              {
                v225 = (intptr_t *)W_vpool_take(SYS32(1404), 0);
                if ( v225 != 0 )
                {
                  (*i__f) = F32(v225, 0);
                  if ( W_sfx_place(
                         c,
                         F32(CFG(), 360),
                         0,
                         0,
                         (*v301__f),
                         (int *)&(*v300__f),
                         &(*v302__f)) != 0 )
                  {
                    g_eng.dsb_volume_5bb870((*i__f), (int)(*v300__f));
                    g_eng.dsb_pan_5bb880((*i__f), (*v302__f));
                    g_eng.dsb_freq_5bb890((*i__f), v224 / 0x8000);
                    g_eng.dsb_play_5bb8c0((*i__f));
                  }
                }
              }
              v226 = rnd();
              (*v301__f) = F32(c, 0x40);
              v227 = 1000 * v226 / 0x8000 + 500;
              if ( a3 == 0 )
              {
                v228 = (intptr_t *)W_vpool_take(SYS32(1156), 0);
                if ( v228 != 0 )
                {
                  (*i__f) = F32(v228, 0);
                  if ( W_sfx_place(
                         c,
                         F32(CFG(), 360) - 500,
                         0,
                         0,
                         (*v301__f),
                         (int *)&(*v300__f),
                         &(*v302__f)) != 0 )
                  {
                    g_eng.dsb_volume_5bb870((*i__f), (int)(*v300__f));
                    g_eng.dsb_pan_5bb880((*i__f), (*v302__f));
                    g_eng.dsb_freq_5bb890((*i__f), v227);
                    g_eng.dsb_play_5bb8c0((*i__f));
                  }
                }
                if ( F32(c, 0x15b54) != 0
                  && F32(c, 0x15b5c) == 0
                  && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                  && W_voice_place(c, (int *)&(*v300__f), (int *)&(*v301__f), &(*i__f)) != 0 )
                {
                  W_voice_play(F32(c, 0x15b54), 0x1Fu, 0, (int)(*v300__f), (int)(*v301__f), (int)(*i__f), 0);
                }
              }
            }
LABEL_566:
            if ( F16(a2, 88) >= 2u && F16(a2, 88) <= 3u )
            {
              v87 = F16(a2, 90) == 0;
              (*v307__f) = 1;
              if ( v87 && F32(a2, 92) > 4 )
              {
                v229 = F32(a2, 76);
                (*v301__f) = F32(v229, 740);
                v230 = rnd();
                v231 = W_char_skill_roll(c, (_DWORD *)v229, 1000, 1000 * v230 / 0x8000);
                v232 = F32(v229, 176);
                (*i__f) = v231;
                (*v305__f) = v232;
                v233 = W_char_mag_sum(c);
                v234 = F32(v229, 744);
                F32(v303, 0) = F32(v229, 180) + v233;
                (*v304__f) = 0;
                (*v302__f) = 0;
                (*v300__f) = 0;
                if ( v234 == 0 && F32(v229, 732) == 0 && (*v305__f) == 0 )
                {
                  v235 = (int *)F32(c, 0x750);
                  v236 = v235[74];
                  (*v305__f) = v235[73];
                  v237 = v235[75];
                  F32(v303, 0) = v236;
                  v238 = v235[76];
                  v239 = v235[77];
                  (*v304__f) = v237;
                  (*v302__f) = v238;
                  (*v300__f) = v239;
                }
                (*v306__f) = (_DWORD *)W_body_hit_spawn(
                                   c,
                                   (int)c,
                                   a2,
                                   2400,
                                   800,
                                   4000,
                                   4000,
                                   0,
                                   a3,
                                   (int)(*v301__f),
                                   (*v305__f),
                                   F32(v303, 0),
                                   0,
                                   6,
                                   (int)(*i__f),
                                   1,
                                   F32(v229, 736),
                                   F32(a2, 96),
                                   0,
                                   F32(v229, 756),
                                   F32(F32(c, 0x750), 216)
                                 + F32(F32(c, 0x750), 224)
                                 + F32(F32(c, 0x750), 228),
                                   (*v304__f),
                                   (*v302__f),
                                   (int)(*v300__f));
                if ( F32(c, 0xc7f4) != 0 )
                {
                  W_body_hit_spawn(
                    c,
                    (int)c,
                    a2,
                    F32(c, 0xc7f8) + 2400,
                    800,
                    4000,
                    4000,
                    0,
                    a3,
                    (int)(*v301__f),
                    (*v305__f),
                    F32(v303, 0),
                    0,
                    6,
                    (int)(*i__f),
                    1,
                    F32(v229, 736),
                    F32(a2, 96),
                    0,
                    F32(v229, 756),
                    F32(F32(c, 0x750), 216)
                  + F32(F32(c, 0x750), 224)
                  + F32(F32(c, 0x750), 228),
                    (*v304__f),
                    (*v302__f),
                    (int)(*v300__f));
                  W_body_hit_spawn(
                    c,
                    (int)c,
                    a2,
                    2400 - F32(c, 0xc7f8),
                    800,
                    4000,
                    4000,
                    0,
                    a3,
                    (int)(*v301__f),
                    (*v305__f),
                    F32(v303, 0),
                    0,
                    6,
                    (int)(*i__f),
                    1,
                    F32(v229, 736),
                    F32(a2, 96),
                    0,
                    F32(v229, 756),
                    F32(F32(c, 0x750), 216)
                  + F32(F32(c, 0x750), 224)
                  + F32(F32(c, 0x750), 228),
                    (*v304__f),
                    (*v302__f),
                    (int)(*v300__f));
                }
                if ( (*v306__f) != 0 )
                {
                  v240 = rnd();
                  (*v306__f) = (_DWORD *)F32(c, 0x40);
                  v241 = 1000 * v240 / 0x8000;
                  if ( a3 == 0 )
                  {
                    v242 = (intptr_t *)W_vpool_take(SYS32(1404), 0);
                    if ( v242 != 0 )
                    {
                      (*i__f) = F32(v242, 0);
                      if ( W_sfx_place(
                             c,
                             F32(CFG(), 360),
                             0,
                             0,
                             (*v306__f),
                             (int *)&(*v301__f),
                             (int *)&(*v300__f)) != 0 )
                      {
                        g_eng.dsb_volume_5bb870((*i__f), (int)(*v301__f));
                        g_eng.dsb_pan_5bb880((*i__f), (int)(*v300__f));
                        (*v298__f) = v241;
                        v243 = (*i__f);
                        g_eng.dsb_freq_5bb890((*i__f), (*v298__f));
                        g_eng.dsb_play_5bb8c0(v243);
                      }
                    }
                  }
                  F16(a2, 88) = 4;
                }
              }
            }
            if ( F32(a2, 92) % 4 == 0 && W_entity_sprite_descs(c, a2, (int)v308, v303) != 0 )
            {
              for ( (*i__f) = 0; (uint16_t)(*i__f) < v303[0]; (*i__f) = ((uint8_t *)(*i__f) + 1) )
              {
                v244 = 10 * (uint16_t)(*i__f);
                v245 = W_particle_spawn(
                                       c,
                                       (int)c,
                                       100082,
                                       (_DWORD *)a2,
                                       F32(a2, 4) + 100 * (F32(a2, 64) + v308[v244 / 2]),
                                       F32(a2, 8) + 100 * (F32(a2, 68) + v308[v244 / 2 + 1]),
                                       99,
                                       0,
                                       a3,
                                       0,
                                       0,
                                       0,
                                       0,
                                       10);
                (*v301__f) = v245;
                if ( v245 != 0 )
                {
                  W_entity_track_on(v245, 0, v309[v244], v309[v244 + 1]);
                  v246 = (*v301__f);
                  W_particle_tint_set((*v301__f), 12);
                  *(_DWORD *)((uint8_t *)(v246 + (1) * 0x260) + 0x0) = 10;
                }
              }
            }
            break;
          case 4:
            goto LABEL_546;
          case 5:
          case 6:
            if ( F8(a2, 116) == 0 && a5 == 0 )
              goto LABEL_566;
            W_body_action_reset(a2);
            goto LABEL_551;
          default:
            goto LABEL_566;
        }
        goto LABEL_723;
      case 0x277F:
        SETHI(v178, 0);
        *a4 = 2;
        SETLO(v178, F16(a2, 88));
        if ( (uint16_t)v178 != 0 )
        {
          if ( v178 > 1 && v178 <= 3 )
          {
            v179 = &F32(c, 0x14834);
            if ( a3 == 0 )
            {
              v180 = F16(c, 0x1484c);
              if ( v180 >= F16(c, 0x1484e) )
              {
                if ( v180 < F16(c, 0x14850) )
                {
                  for ( (*i__f) = 2; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
                  {
                    v181 = F32(c, 0x14844);
                    v182 = F32(c, 0x14840);
                    ++F16(c, 0x1484c);
                    v183 = W_angle_wrap(v182, v181, 1000);
                    v184 = F32(c, 0x14844);
                    v185 = *v179;
                    F32(c, 0x14840) = v183;
                    F32(c, 0x14844) = v184 - 400;
                    W_polar_div(
                      F32(c, 0x1483c),
                      v183,
                      1000,
                      v185,
                      F32(c, 0x14838),
                      &F32(c, 0x14834),
                      &F32(c, 0x14838));
                    W_char_particle(c, 100077, a2, *v179, F32(c, 0x14838), F32(a2, 44), 1, 0, 0, 2, 1, 1, 0);
                  }
                }
              }
              else
              {
                F16(c, 0x1484c) = v180 + 1;
              }
            }
          }
        }
        else if ( a3 == 0 )
        {
          F32(c, 0x14834) = 4800;
          F32(c, 0x14838) = 1600;
          F32(c, 0x1483c) = 600;
          F32(c, 0x14840) = 0;
          F32(c, 0x14844) = -5000;
          F16(c, 0x1484c) = 0;
          F16(c, 0x1484e) = 4;
          F16(c, 0x14850) = 16;
        }
        v186 = 0;
        switch ( F16(a2, 88) )
        {
          case 0:
            if ( F32(a2, 108) != D(0x631b28) )
            {
              F32(a2, 108) = D(0x631b28);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
            v186 = 1;
            F16(a2, 88) = 1;
            goto LABEL_439;
          case 1:
LABEL_439:
            if ( F8(a2, 116) == 0 )
              goto LABEL_447;
            W_body_set_clip(a2, (int)D(0x631cbc));
            F16(a2, 88) = 2;
            goto LABEL_448;
          case 2:
            if ( F8(a2, 116) != 0 )
            {
              W_body_set_clip(a2, (int)D(0x631e50));
              W_char_lunge(c, a2, 0, a3, 1);
              F16(a2, 88) = 3;
            }
            else
            {
LABEL_447:
              if ( v186 == 0 )
                goto LABEL_475;
            }
LABEL_448:
            switch ( F16(a2, 88) )
            {
              case 1:
                v197 = F32(a2, 44) != 3 ? 1000 : -1000;
                F32(a2, 120) -= v197;
                if ( W_entity_move_horizontal(c, (_DWORD *)a2, v197, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
                  F32(c, 0x14834) -= 1000;
                break;
              case 2:
                if ( a3 == 0
                  && F32(c, 0x15b54) != 0
                  && F32(c, 0x15b5c) == 0
                  && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                  && W_voice_place(c, (int *)&(*v300__f), (int *)&(*v301__f), &(*i__f)) != 0 )
                {
                  W_voice_play(F32(c, 0x15b54), 4u, 0, (int)(*v300__f), (int)(*v301__f), (int)(*i__f), 0);
                }
                v188 = 500 * rnd() / 0x8000 - 1000;
                (*v301__f) = F32(c, 0x40);
                if ( a3 == 0 )
                {
                  v189 = (intptr_t *)W_vpool_take(SYS32(1140), 0);
                  if ( v189 != 0 )
                  {
                    (*i__f) = F32(v189, 0);
                    if ( W_sfx_place(
                           c,
                           F32(CFG(), 360),
                           0,
                           0,
                           (*v301__f),
                           (int *)&(*v300__f),
                           &(*v302__f)) != 0 )
                    {
                      g_eng.dsb_volume_5bb870((*i__f), (int)(*v300__f));
                      g_eng.dsb_pan_5bb880((*i__f), (*v302__f));
                      (*v296__f) = v188;
                      v190 = (*i__f);
                      g_eng.dsb_freq_5bb890((*i__f), (*v296__f));
                      g_eng.dsb_play_5bb8c0(v190);
                    }
                  }
                }
                if ( F16(F32(c, 0x750), 1126) != 0 )
                {
                  v191 = W_char_skill_find(c, 0x2783u);
                  if ( v191 != 0 && W_char_skill_ok(c, v191, 0) != 0 )
                  {
                    v192 = F32(a2, 96);
                    v193 = F32(a2, 76);
                    (*v301__f) = F32(a2, 44);
                    (*i__f) = 0;
                    F8(a2, 127) = 1;
                    (*v300__f) = v192;
                    v194 = rnd();
                    (*v302__f) = W_char_skill_roll(c, (_DWORD *)v193, 1000, 1000 * v194 / 0x8000);
                    if ( F32(v193, 176) == 7 )
                    {
                      (*i__f) = F32(v193, 720);
                      (*v297__f) = 1000 * rnd() / 0x8000;
                      (*v289__f) = F32(v193, 716);
                      v195 = W_char_mag_sum(c);
                      (*i__f) = W_stat_jitter((int)&GAP((*i__f), 0x0, v195 / 2), (*v289__f), (*v297__f));
                    }
                    (*v290__f) = F32(F32(c, 0x750), 216)
                         + F32(F32(c, 0x750), 224)
                         + F32(F32(c, 0x750), 228);
                    (*v283__f) = F32(c, 0x1f0);
                    (*v277__f) = (*i__f);
                    v196 = W_char_mag_sum(c);
                    W_shot_spawn(
                      c,
                      (int)c,
                      F32(c, 0x1dc),
                      100006,
                      (_DWORD *)a2,
                      1600,
                      -1600,
                      1,
                      a3,
                      5,
                      (*v302__f),
                      60,
                      (int)(*v300__f),
                      (int)(*v301__f),
                      F32(v193, 176),
                      F32(v193, 180) + v196,
                      (int)(*v277__f),
                      (*v283__f),
                      (*v290__f),
                      2);
                  }
                }
                break;
              case 3:
                v187 = F32(a2, 44) != 3 ? 2500 : -2500;
                F32(a2, 120) -= v187;
                if ( W_entity_move_horizontal(c, (_DWORD *)a2, v187, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
                  F32(c, 0x14834) -= 2500;
                F16(a2, 90) = 0;
                break;
              default:
                break;
            }
LABEL_475:
            v198 = F16(a2, 88);
            if ( v198 == 4 )
              ++F16(a2, 100);
            else
              F16(a2, 100) = 0;
            if ( F16(a2, 90) == 0 && v198 == 3 )
            {
              v199 = F32(a2, 76);
              (*v301__f) = F32(v199, 740);
              v200 = rnd();
              v201 = W_char_skill_roll(c, (_DWORD *)v199, 1000, 1000 * v200 / 0x8000);
              v202 = F32(v199, 176);
              v203 = F32(v199, 180);
              (*i__f) = v201;
              v204 = (_DWORD *)F32(c, 0x750);
              (*v305__f) = v202;
              v205 = F32(v199, 744);
              F32(v303, 0) = v204[27] + v204[31] + v204[37] + F32(c, 0xc7e4) + v203;
              (*v304__f) = 0;
              (*v302__f) = 0;
              (*v300__f) = 0;
              if ( v205 == 0 && F32(v199, 732) == 0 && (*v305__f) == 0 )
              {
                v206 = v204[74];
                (*v305__f) = v204[73];
                v207 = v204[75];
                F32(v303, 0) = v206;
                v208 = v204[76];
                (*v304__f) = v207;
                v209 = v204[77];
                (*v302__f) = v208;
                (*v300__f) = v209;
              }
              W_body_hit_spawn(
                c,
                (int)c,
                a2,
                2400,
                -4000,
                4800,
                6400,
                2,
                a3,
                (int)(*v301__f),
                (*v305__f),
                F32(v303, 0),
                0,
                2,
                (int)(*i__f),
                1,
                F32(v199, 736),
                F32(a2, 96),
                0,
                F32(v199, 756),
                v204[54] + v204[56] + v204[57],
                (*v304__f),
                (*v302__f),
                (int)(*v300__f));
              if ( F32(c, 0xc7f4) != 0 )
              {
                W_body_hit_spawn(
                  c,
                  (int)c,
                  a2,
                  F32(c, 0xc7f8) + 2400,
                  -4000,
                  4800,
                  6400,
                  2,
                  a3,
                  (int)(*v301__f),
                  (*v305__f),
                  F32(v303, 0),
                  0,
                  2,
                  (int)(*i__f),
                  1,
                  F32(v199, 736),
                  F32(a2, 96),
                  0,
                  F32(v199, 756),
                  F32(F32(c, 0x750), 216)
                + F32(F32(c, 0x750), 224)
                + F32(F32(c, 0x750), 228),
                  (*v304__f),
                  (*v302__f),
                  (int)(*v300__f));
                W_body_hit_spawn(
                  c,
                  (int)c,
                  a2,
                  2400 - F32(c, 0xc7f8),
                  -4000,
                  4800,
                  6400,
                  2,
                  a3,
                  (int)(*v301__f),
                  (*v305__f),
                  F32(v303, 0),
                  0,
                  2,
                  (int)(*i__f),
                  1,
                  F32(v199, 736),
                  F32(a2, 96),
                  0,
                  F32(v199, 756),
                  F32(F32(c, 0x750), 216)
                + F32(F32(c, 0x750), 224)
                + F32(F32(c, 0x750), 228),
                  (*v304__f),
                  (*v302__f),
                  (int)(*v300__f));
              }
            }
            break;
          case 3:
            if ( F8(a2, 116) == 0 )
              goto LABEL_475;
            W_body_set_clip(a2, (int)D(0x631fe4));
            W_char_lunge(c, a2, 0, a3, 1);
            F16(a2, 88) = 4;
            goto LABEL_448;
          case 4:
            if ( F8(a2, 116) == 0 )
              goto LABEL_475;
            W_body_action_reset(a2);
            F16(a2, 88) = 5;
            goto LABEL_448;
          default:
            goto LABEL_475;
        }
        goto LABEL_723;
      case 0x2780:
        SETHI(v146, 0);
        *a4 = 2;
        SETLO(v146, F16(a2, 88));
        if ( (uint16_t)v146 != 0 )
        {
          if ( v146 != 1 )
          {
            v147 = &F32(c, 0x14834);
            if ( a3 == 0 )
            {
              v148 = F16(c, 0x1484c);
              if ( v148 >= F16(c, 0x1484e) )
              {
                if ( v148 < F16(c, 0x14850) )
                {
                  for ( (*i__f) = 2; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
                  {
                    v149 = F32(c, 0x14844);
                    v150 = F32(c, 0x14840);
                    ++F16(c, 0x1484c);
                    v151 = W_angle_wrap(v150, v149, 1000);
                    v152 = F32(c, 0x14844);
                    v153 = *v147;
                    F32(c, 0x14840) = v151;
                    F32(c, 0x14844) = v152 + 100;
                    W_polar_div(
                      F32(c, 0x1483c),
                      v151,
                      1000,
                      v153,
                      F32(c, 0x14838),
                      &F32(c, 0x14834),
                      &F32(c, 0x14838));
                    W_char_particle(c, 100077, a2, *v147, F32(c, 0x14838), F32(a2, 44), 1, 0, 0, 2, 1, 1, 0);
                  }
                }
              }
              else
              {
                F16(c, 0x1484c) = v148 + 1;
              }
            }
          }
        }
        else if ( a3 == 0 )
        {
          F32(c, 0x14834) = -800;
          F32(c, 0x14838) = -800;
          F32(c, 0x1483c) = 500;
          F32(c, 0x14840) = 90000;
          F32(c, 0x14844) = 0;
          F16(c, 0x1484c) = 0;
          F16(c, 0x1484e) = 0;
          F16(c, 0x14850) = 18;
        }
        v154 = 0;
        switch ( F16(a2, 88) )
        {
          case 0:
            if ( F32(a2, 108) != D(0x632178) )
            {
              F32(a2, 108) = D(0x632178);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
            v154 = 1;
            F16(a2, 88) = 1;
            goto LABEL_377;
          case 1:
LABEL_377:
            if ( F8(a2, 116) == 0 )
              goto LABEL_385;
            W_body_set_clip(a2, (int)D(0x63230c));
            F16(a2, 88) = 2;
            goto LABEL_386;
          case 2:
            if ( F8(a2, 116) != 0 )
            {
              W_body_set_clip(a2, (int)D(0x6324a0));
              W_char_lunge(c, a2, 0, a3, 1);
              F16(a2, 88) = 3;
            }
            else
            {
LABEL_385:
              if ( v154 == 0 )
                goto LABEL_415;
            }
LABEL_386:
            switch ( F16(a2, 88) )
            {
              case 1:
                v155 = F32(a2, 44) != 3 ? 800 : -800;
                F32(a2, 120) -= v155;
                if ( W_entity_move_horizontal(c, (_DWORD *)a2, v155, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
                {
                  v156 = F32(c, 0x14834) - 800;
                  goto LABEL_414;
                }
                break;
              case 2:
                if ( a3 == 0
                  && F32(c, 0x15b54) != 0
                  && F32(c, 0x15b5c) == 0
                  && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                  && W_voice_place(c, (int *)&(*v300__f), (int *)&(*v301__f), &(*i__f)) != 0 )
                {
                  W_voice_play(F32(c, 0x15b54), 3u, 0, (int)(*v300__f), (int)(*v301__f), (int)(*i__f), 0);
                }
                if ( F16(F32(c, 0x750), 1126) != 0 )
                {
                  v157 = W_char_skill_find(c, 0x2783u);
                  if ( v157 != 0 && W_char_skill_ok(c, v157, 0) != 0 )
                  {
                    v158 = F32(a2, 96);
                    v159 = F32(a2, 76);
                    (*v301__f) = F32(a2, 44);
                    (*i__f) = 0;
                    F8(a2, 127) = 1;
                    (*v300__f) = v158;
                    v160 = rnd();
                    (*v302__f) = W_char_skill_roll(c, v159, 1000, 1000 * v160 / 0x8000);
                    if ( v159[44] == 7 )
                    {
                      (*v294__f) = v159[180];
                      v161 = rnd();
                      (*i__f) = W_char_mag_roll(c, v159[179], 1000 * v161 / 0x8000, (*v294__f));
                    }
                    (*v288__f) = F32(F32(c, 0x750), 216)
                         + F32(F32(c, 0x750), 224)
                         + F32(F32(c, 0x750), 228);
                    (*v282__f) = F32(c, 0x1f0);
                    (*v276__f) = (*i__f);
                    v162 = W_char_mag_sum(c);
                    W_shot_spawn(
                      c,
                      (int)c,
                      F32(c, 0x1dc),
                      100006,
                      (_DWORD *)a2,
                      4800,
                      0,
                      1,
                      a3,
                      5,
                      (*v302__f),
                      90,
                      (int)(*v300__f),
                      (int)(*v301__f),
                      v159[44],
                      v159[45] + v162,
                      (int)(*v276__f),
                      (*v282__f),
                      (*v288__f),
                      2);
                  }
                }
                v163 = rnd();
                (*v301__f) = F32(c, 0x40);
                v164 = 1000 * v163 / 0x8000 - 2500;
                if ( a3 == 0 )
                {
                  v165 = (intptr_t *)W_vpool_take(SYS32(1140), 0);
                  if ( v165 != 0 )
                  {
                    (*i__f) = F32(v165, 0);
                    if ( W_sfx_place(
                           c,
                           F32(CFG(), 360),
                           0,
                           0,
                           (*v301__f),
                           (int *)&(*v300__f),
                           &(*v302__f)) != 0 )
                    {
                      g_eng.dsb_volume_5bb870((*i__f), (int)(*v300__f));
                      g_eng.dsb_pan_5bb880((*i__f), (*v302__f));
                      (*v295__f) = v164;
                      v166 = (*i__f);
                      g_eng.dsb_freq_5bb890((*i__f), (*v295__f));
                      g_eng.dsb_play_5bb8c0(v166);
                    }
                  }
                }
                break;
              case 3:
                v167 = F32(a2, 44) != 3 ? 1800 : -1800;
                F32(a2, 120) -= v167;
                if ( W_entity_move_horizontal(c, (_DWORD *)a2, v167, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
                  F32(c, 0x14834) -= 1800;
                F16(a2, 90) = 0;
                break;
              case 4:
                v168 = F32(a2, 44) != 3 ? 1400 : -1400;
                F32(a2, 120) -= v168;
                if ( W_entity_move_horizontal(c, (_DWORD *)a2, v168, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
                {
                  v156 = F32(c, 0x14834) - 1400;
LABEL_414:
                  F32(c, 0x14834) = v156;
                }
                break;
              default:
                break;
            }
LABEL_415:
            if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
            {
              v169 = F32(a2, 76);
              F32(v303, 0) = v169;
              (*v301__f) = F32(v169, 740);
              v170 = rnd();
              (*i__f) = W_char_skill_roll(c, (_DWORD *)v169, 1000, 1000 * v170 / 0x8000);
              v171 = F32(v169, 176);
              v172 = F32(c, 0xc7e4);
              (*v305__f) = v171;
              v173 = (_DWORD *)F32(c, 0x750);
              v174 = F32(F32(v303, 0), 180) + v173[27] + v173[31] + v173[37] + v172;
              v102 = F32(F32(v303, 0), 744) == 0;
              (*v304__f) = 0;
              (*v302__f) = 0;
              (*v300__f) = 0;
              if ( v102 && F32(F32(v303, 0), 732) == 0 && (*v305__f) == 0 )
              {
                v175 = v173[75];
                v174 = v173[74];
                (*v305__f) = v173[73];
                v176 = v173[76];
                (*v304__f) = v175;
                v177 = v173[77];
                (*v302__f) = v176;
                (*v300__f) = v177;
              }
              W_body_hit_spawn(
                c,
                (int)c,
                a2,
                6000,
                1600,
                3200,
                3200,
                0,
                a3,
                (int)(*v301__f),
                (*v305__f),
                v174,
                0,
                2,
                (int)(*i__f),
                1,
                F32(F32(v303, 0), 736),
                F32(a2, 96),
                0,
                F32(F32(v303, 0), 756),
                v173[54] + v173[56] + v173[57],
                (*v304__f),
                (*v302__f),
                (int)(*v300__f));
              if ( F32(c, 0xc7f4) != 0 )
              {
                W_body_hit_spawn(
                  c,
                  (int)c,
                  a2,
                  F32(c, 0xc7f8) + 6000,
                  1600,
                  3200,
                  3200,
                  0,
                  a3,
                  (int)(*v301__f),
                  (*v305__f),
                  v174,
                  0,
                  2,
                  (int)(*i__f),
                  1,
                  F32(F32(v303, 0), 736),
                  F32(a2, 96),
                  0,
                  F32(F32(v303, 0), 756),
                  F32(F32(c, 0x750), 216)
                + F32(F32(c, 0x750), 224)
                + F32(F32(c, 0x750), 228),
                  (*v304__f),
                  (*v302__f),
                  (int)(*v300__f));
                W_body_hit_spawn(
                  c,
                  (int)c,
                  a2,
                  6000 - F32(c, 0xc7f8),
                  1600,
                  3200,
                  3200,
                  0,
                  a3,
                  (int)(*v301__f),
                  (*v305__f),
                  v174,
                  0,
                  2,
                  (int)(*i__f),
                  1,
                  F32(F32(v303, 0), 736),
                  F32(a2, 96),
                  0,
                  F32(F32(v303, 0), 756),
                  F32(F32(c, 0x750), 216)
                + F32(F32(c, 0x750), 224)
                + F32(F32(c, 0x750), 228),
                  (*v304__f),
                  (*v302__f),
                  (int)(*v300__f));
              }
            }
            break;
          case 3:
            if ( F8(a2, 116) == 0 )
              goto LABEL_415;
            W_body_set_clip(a2, (int)D(0x632634));
            W_char_lunge(c, a2, 0, a3, 1);
            F16(a2, 88) = 4;
            goto LABEL_386;
          case 4:
            if ( F8(a2, 116) == 0 )
              goto LABEL_415;
            W_body_action_reset(a2);
            F16(a2, 88) = 5;
            goto LABEL_386;
          default:
            goto LABEL_415;
        }
        goto LABEL_723;
      case 0x2783:
        (*v300__f) = 1;
        if ( F16(a2, 84) == 1 )
        {
LABEL_305:
          v102 = F32(v303, 0) == 0;
          *a4 = 2;
          if ( v102 || F32(a2, 24) < 0 )
            goto LABEL_200;
          if ( F16(a2, 88) != 0 )
          {
            if ( F16(a2, 88) > 1u && F16(a2, 88) <= 3u )
            {
              v109 = &F32(c, 0x14834);
              if ( a3 == 0 )
              {
                v110 = F16(c, 0x1484c);
                if ( v110 >= F16(c, 0x1484e) )
                {
                  if ( v110 < F16(c, 0x14850) )
                  {
                    for ( (*i__f) = 2; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
                    {
                      v111 = F32(c, 0x14844);
                      v112 = F32(c, 0x14840);
                      ++F16(c, 0x1484c);
                      v113 = W_angle_wrap(v112, v111, 1000);
                      v114 = F32(c, 0x14844);
                      v115 = F32(c, 0x14838);
                      F32(c, 0x14840) = v113;
                      F32(c, 0x14844) = v114 - 400;
                      W_polar_div(F32(c, 0x1483c), v113, 1000, *v109, v115, &F32(c, 0x14834), &F32(c, 0x14838));
                      W_char_particle(c, 100077, a2, *v109, F32(c, 0x14838), F32(a2, 44), 1, 0, 0, 2, 1, 1, 0);
                    }
                  }
                }
                else
                {
                  F16(c, 0x1484c) = v110 + 1;
                }
              }
            }
          }
          else if ( a3 == 0 )
          {
            F32(c, 0x14834) = 0;
            F32(c, 0x14838) = 1200;
            F32(c, 0x1483c) = 600;
            F32(c, 0x14840) = 90000;
            F32(c, 0x14844) = -5000;
            F16(c, 0x1484c) = 0;
            F16(c, 0x1484e) = 0;
            F16(c, 0x14850) = 18;
          }
          v116 = 0;
          switch ( F16(a2, 88) )
          {
            case 0:
              W_body_set_clip(a2, (int)D(0x6314d8));
              v116 = 1;
              F16(a2, 88) = 1;
              goto LABEL_321;
            case 1:
LABEL_321:
              if ( F8(a2, 116) == 0 )
                goto LABEL_329;
              W_body_set_clip(a2, (int)D(0x63166c));
              F16(a2, 88) = 2;
              goto LABEL_330;
            case 2:
              if ( F8(a2, 116) != 0 )
              {
                W_body_set_clip(a2, (int)D(0x631800));
                W_char_lunge(c, a2, 1000, a3, 1);
                F16(a2, 88) = 3;
              }
              else
              {
LABEL_329:
                if ( v116 == 0 )
                  goto LABEL_347;
              }
LABEL_330:
              if ( F16(a2, 88) == 2 )
              {
                v117 = 500 * rnd() / 0x8000 - 1000;
                (*v301__f) = F32(c, 0x40);
                if ( a3 == 0 )
                {
                  v118 = (intptr_t *)W_vpool_take(SYS32(1140), 0);
                  if ( v118 != 0 )
                  {
                    (*i__f) = F32(v118, 0);
                    if ( W_sfx_place(
                           c,
                           F32(CFG(), 360),
                           0,
                           0,
                           (*v301__f),
                           &(*v302__f),
                           &(*v304__f)) != 0 )
                    {
                      g_eng.dsb_volume_5bb870((*i__f), (*v302__f));
                      g_eng.dsb_pan_5bb880((*i__f), (*v304__f));
                      g_eng.dsb_freq_5bb890((*i__f), v117);
                      g_eng.dsb_play_5bb8c0((*i__f));
                    }
                  }
                  if ( F32(c, 0x15b54) != 0
                    && F32(c, 0x15b5c) == 0
                    && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                    && W_voice_place(c, &(*v302__f), (int *)&(*v301__f), &(*i__f)) != 0 )
                  {
                    W_voice_play(F32(c, 0x15b54), 4u, 0, (*v302__f), (int)(*v301__f), (int)(*i__f), 0);
                  }
                }
                if ( (*v300__f) != 0 )
                {
                  v119 = F32(a2, 96);
                  v120 = F32(a2, 76);
                  (*v301__f) = F32(a2, 44);
                  (*i__f) = 0;
                  F8(a2, 127) = 1;
                  (*v300__f) = v119;
                  v121 = rnd();
                  (*v302__f) = W_char_skill_roll(c, (_DWORD *)v120, 1000, 1000 * v121 / 0x8000);
                  if ( F32(v120, 176) == 7 )
                  {
                    (*i__f) = F32(v120, 720);
                    (*v293__f) = 1000 * rnd() / 0x8000;
                    (*v286__f) = F32(v120, 716);
                    v122 = W_char_mag_sum(c);
                    (*i__f) = W_stat_jitter((int)&GAP((*i__f), 0x0, v122 / 2), (*v286__f), (*v293__f));
                  }
                  (*v287__f) = F32(F32(c, 0x750), 216)
                       + F32(F32(c, 0x750), 224)
                       + F32(F32(c, 0x750), 228);
                  (*v281__f) = F32(c, 0x1f0);
                  (*v275__f) = (*i__f);
                  v123 = W_char_mag_sum(c);
                  W_shot_spawn(
                    c,
                    (int)c,
                    F32(c, 0x1dc),
                    100006,
                    (_DWORD *)a2,
                    1600,
                    -1600,
                    1,
                    a3,
                    5,
                    (*v302__f),
                    60,
                    (int)(*v300__f),
                    (int)(*v301__f),
                    F32(v120, 176),
                    F32(v120, 180) + v123,
                    (int)(*v275__f),
                    (*v281__f),
                    (*v287__f),
                    2);
                }
              }
              else if ( F16(a2, 88) == 3 )
              {
                F16(a2, 90) = 0;
              }
LABEL_347:
              if ( F16(a2, 90) == 0 )
              {
                if ( F16(a2, 88) == 2 )
                {
                  v135 = F32(a2, 76);
                  (*v301__f) = F32(v135, 740);
                  v136 = rnd();
                  v137 = W_char_skill_roll(c, (_DWORD *)v135, 700, 1000 * v136 / 0x8000);
                  v138 = F32(v135, 176);
                  (*i__f) = v137;
                  F32(v303, 0) = v138;
                  v139 = W_char_mag_sum(c);
                  v140 = F32(v135, 744);
                  (*v305__f) = F32(v135, 180) + v139;
                  (*v304__f) = 0;
                  (*v302__f) = 0;
                  (*v300__f) = 0;
                  if ( v140 == 0 && F32(v135, 732) == 0 && F32(v303, 0) == 0 )
                  {
                    v141 = (_DWORD *)F32(c, 0x750);
                    v142 = v141[74];
                    F32(v303, 0) = v141[73];
                    v143 = v141[75];
                    (*v305__f) = v142;
                    v144 = v141[76];
                    v145 = v141[77];
                    (*v304__f) = v143;
                    (*v302__f) = v144;
                    (*v300__f) = v145;
                  }
                  W_body_hit_spawn(
                    c,
                    (int)c,
                    a2,
                    4200,
                    0,
                    4000,
                    3200,
                    2,
                    a3,
                    (int)(*v301__f),
                    F32(v303, 0),
                    (*v305__f),
                    0,
                    2,
                    (int)(*i__f),
                    1,
                    F32(v135, 736),
                    F32(a2, 96),
                    0,
                    F32(v135, 756),
                    F32(F32(c, 0x750), 216)
                  + F32(F32(c, 0x750), 224)
                  + F32(F32(c, 0x750), 228),
                    (*v304__f),
                    (*v302__f),
                    (int)(*v300__f));
                  if ( F32(c, 0xc7f4) != 0 )
                  {
                    W_body_hit_spawn(
                      c,
                      (int)c,
                      a2,
                      F32(c, 0xc7f8) + 4200,
                      0,
                      4000,
                      3200,
                      2,
                      a3,
                      (int)(*v301__f),
                      F32(v303, 0),
                      (*v305__f),
                      0,
                      2,
                      (int)(*i__f),
                      1,
                      F32(v135, 736),
                      F32(a2, 96),
                      0,
                      F32(v135, 756),
                      F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228),
                      (*v304__f),
                      (*v302__f),
                      (int)(*v300__f));
                    W_body_hit_spawn(
                      c,
                      (int)c,
                      a2,
                      4200 - F32(c, 0xc7f8),
                      0,
                      4000,
                      3200,
                      2,
                      a3,
                      (int)(*v301__f),
                      F32(v303, 0),
                      (*v305__f),
                      0,
                      2,
                      (int)(*i__f),
                      1,
                      F32(v135, 736),
                      F32(a2, 96),
                      0,
                      F32(v135, 756),
                      F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228),
                      (*v304__f),
                      (*v302__f),
                      (int)(*v300__f));
                  }
                }
                else if ( F16(a2, 88) == 3 )
                {
                  v124 = F32(a2, 76);
                  (*v301__f) = F32(v124, 740);
                  v125 = rnd();
                  v126 = W_char_skill_roll(c, (_DWORD *)v124, 1000, 1000 * v125 / 0x8000);
                  v127 = F32(v124, 176);
                  (*i__f) = v126;
                  F32(v303, 0) = v127;
                  v128 = W_char_mag_sum(c);
                  v129 = F32(v124, 744);
                  (*v305__f) = F32(v124, 180) + v128;
                  (*v304__f) = 0;
                  (*v302__f) = 0;
                  (*v300__f) = 0;
                  if ( v129 == 0 && F32(v124, 732) == 0 && F32(v303, 0) == 0 )
                  {
                    v130 = (_DWORD *)F32(c, 0x750);
                    v131 = v130[74];
                    F32(v303, 0) = v130[73];
                    v132 = v130[75];
                    (*v305__f) = v131;
                    v133 = v130[76];
                    (*v304__f) = v132;
                    v134 = v130[77];
                    (*v302__f) = v133;
                    (*v300__f) = v134;
                  }
                  W_body_hit_spawn(
                    c,
                    (int)c,
                    a2,
                    3200,
                    -6400,
                    5600,
                    4800,
                    2,
                    a3,
                    (int)(*v301__f),
                    F32(v303, 0),
                    (*v305__f),
                    0,
                    2,
                    (int)(*i__f),
                    1,
                    F32(v124, 736),
                    F32(a2, 96),
                    0,
                    F32(v124, 756),
                    F32(F32(c, 0x750), 216)
                  + F32(F32(c, 0x750), 224)
                  + F32(F32(c, 0x750), 228),
                    (*v304__f),
                    (*v302__f),
                    (int)(*v300__f));
                  if ( F32(c, 0xc7f4) != 0 )
                  {
                    W_body_hit_spawn(
                      c,
                      (int)c,
                      a2,
                      F32(c, 0xc7f8) + 3200,
                      -6400,
                      5600,
                      4800,
                      2,
                      a3,
                      (int)(*v301__f),
                      F32(v303, 0),
                      (*v305__f),
                      0,
                      2,
                      (int)(*i__f),
                      1,
                      F32(v124, 736),
                      F32(a2, 96),
                      0,
                      F32(v124, 756),
                      F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228),
                      (*v304__f),
                      (*v302__f),
                      (int)(*v300__f));
                    W_body_hit_spawn(
                      c,
                      (int)c,
                      a2,
                      3200 - F32(c, 0xc7f8),
                      -6400,
                      5600,
                      4800,
                      2,
                      a3,
                      (int)(*v301__f),
                      F32(v303, 0),
                      (*v305__f),
                      0,
                      2,
                      (int)(*i__f),
                      1,
                      F32(v124, 736),
                      F32(a2, 96),
                      0,
                      F32(v124, 756),
                      F32(F32(c, 0x750), 216)
                    + F32(F32(c, 0x750), 224)
                    + F32(F32(c, 0x750), 228),
                      (*v304__f),
                      (*v302__f),
                      (int)(*v300__f));
                  }
                }
              }
              break;
            case 3:
              if ( F8(a2, 116) == 0 )
                goto LABEL_347;
              W_body_set_clip(a2, (int)D(0x631994));
              W_char_lunge(c, a2, -1000, a3, 1);
              F16(a2, 88) = 4;
              goto LABEL_330;
            case 4:
              if ( F8(a2, 116) == 0 )
                goto LABEL_347;
              W_body_action_reset(a2);
              F16(a2, 88) = 5;
              goto LABEL_330;
            default:
              goto LABEL_347;
          }
          goto LABEL_723;
        }
        if ( F16(a2, 84) != 2 )
          goto LABEL_284;
LABEL_699:
        *a4 = 3;
        if ( F32(v303, 0) != 0 )
        {
          v270 = F16(a2, 88);
          if ( (uint16_t)v270 != 0 )
          {
            if ( v270 != 1 )
              W_orbit_step(c, a2, &F32(c, 0x14834), 100, 2, a3, 100078, 0);
          }
          else
          {
            W_orbit_init((int)&F32(c, 0x14834), -800, 1600, 500, 70000, 0, 0, 20, a3);
          }
          switch ( F16(a2, 88) )
          {
            case 0:
              W_body_set_clip(a2, (int)D(0x634da8));
              ++F16(a2, 88);
              goto LABEL_707;
            case 1:
LABEL_707:
              if ( F16(a2, 114) < 2u )
                goto LABEL_717;
              v271 = rnd();
              W_sfx_play_at(c, a3, 68, 0, 1000 * v271 / 0x8000 - 2500, 0);
              W_char_lunge(c, a2, 1400, a3, 1);
              v272 = (*v300__f);
              ++F16(a2, 88);
              if ( v272 != 0 )
              {
                W_char_shot_fire(
                  c,
                  100006,
                  a2,
                  4800,
                  0,
                  1,
                  a3,
                  5,
                  90,
                  F32(a2, 76),
                  1000,
                  F32(a2, 44),
                  0,
                  2);
                W_voice_cue(c, a3, 0x1Cu, 0, 0, 0);
              }
              else
              {
                W_voice_cue(c, a3, 3u, 0, 0, 0);
              }
LABEL_711:
              v102 = F16(a2, 114) == 3;
              F32(a2, 20) = 1200;
              if ( v102 )
              {
                W_char_lunge(c, a2, 1400, a3, 1);
                ++F16(a2, 88);
              }
LABEL_713:
              v102 = F16(a2, 114) == 5;
              F32(a2, 20) = 1200;
              if ( v102 )
              {
                W_char_lunge(c, a2, 2600, a3, 1);
                ++F16(a2, 88);
              }
LABEL_715:
              if ( F8(a2, 116) != 0 )
                W_body_action_reset(a2);
LABEL_717:
              if ( F16(a2, 90) < 3u )
              {
                v273 = F16(a2, 114);
                if ( (uint16_t)v273 == 2 )
                {
                  W_char_attack_hits(c, a2, 4000, 800, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
                }
                else if ( (uint16_t)v273 > 2u && F16(a2, 114) <= 4u )
                {
                  W_char_attack_hits(c, a2, 6000, 800, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
                }
              }
              break;
            case 2:
              goto LABEL_711;
            case 3:
              goto LABEL_713;
            case 4:
              goto LABEL_715;
            default:
              goto LABEL_717;
          }
          goto LABEL_723;
        }
        goto LABEL_200;
      case 0x2784:
        v102 = F32(v303, 0) == 0;
        *a4 = 3;
        if ( v102 )
          goto LABEL_200;
        SETHI(v256, 0);
        F8(a2, 124) = 1;
        SETLO(v256, F16(a2, 88));
        if ( (uint16_t)v256 != 0 )
        {
          if ( v256 == 1 )
            W_orbit_init((int)&F32(c, 0x14834), 3200, 2000, 600, 9000, -5000, 0, 40, a3);
          else
            W_orbit_step(c, a2, &F32(c, 0x14834), 0, 2, a3, 100079, 0);
        }
        v257 = F16(a2, 88);
        switch ( v257 )
        {
          case 0:
            W_body_set_clip(a2, (int)D(0x635738));
            ++F16(a2, 88);
            goto LABEL_634;
          case 1:
LABEL_634:
            v258 = F8(a2, 116);
            F32(a2, 20) = 2400;
            if ( v258 != 0 )
            {
              ++F16(a2, 88);
              W_voice_cue(c, a3, 0x1Du, 0, 0, 0);
              v259 = rnd();
              W_sfx_play_at(c, a3, 133, 0, 1000 * v259 / 0x8000 + 1000, 0);
              v260 = rnd();
              W_sfx_play_at(c, a3, 68, 0, 1000 * v260 / 0x8000 - 3000, 0);
              W_body_set_clip(a2, (int)D(0x6358d0));
              F32(a2, 40) = 0;
            }
            break;
          case 2:
            v102 = F16(a2, 114) == 1;
            F32(a2, 24) = -30000;
            if ( v102 )
              F16(a2, 88) = v257 + 1;
            break;
          case 3:
            W_char_lunge(c, a2, -480, a3, 0);
            if ( F16(a2, 114) == 5 )
              ++F16(a2, 88);
            break;
          case 4:
            if ( F8(a2, 116) != 0 )
              W_body_action_reset(a2);
            break;
          default:
            break;
        }
        if ( F16(a2, 90) < 3u && F16(a2, 88) >= 2u )
        {
          switch ( F16(a2, 114) )
          {
            case 0:
              W_char_attack_hits(c, a2, 2000, 0, 4000, 6000, 0, a3, F32(a2, 76), 900, 2);
              break;
            case 1:
              W_char_attack_hits(c, a2, 4000, -4000, 8000, 6000, 2, a3, F32(a2, 76), 1000, 2);
              break;
            case 2:
              W_char_attack_hits(c, a2, 1600, -6000, 8000, 6000, 6, a3, F32(a2, 76), 1000, 2);
              break;
            case 3:
              W_char_attack_hits(c, a2, -4000, -4000, 4000, 6000, 1, a3, F32(a2, 76), 1000, 2);
              break;
            case 4:
              W_char_attack_hits(c, a2, -2000, 0, 4000, 6000, 1, a3, F32(a2, 76), 1000, 2);
              break;
            default:
              break;
          }
        }
        v261 = F32(a2, 92) + 1;
        F32(a2, 92) = v261;
        v255 = v261 & 3;
        v249 = (v261 & 0x80000003) == 0;
        if ( v261 < 0 )
          goto LABEL_651;
        goto LABEL_652;
      case 0x2785:
        *a4 = 3;
        if ( F32(v303, 0) == 0 )
          goto LABEL_200;
        v102 = F16(a2, 88) == 0;
        F8(a2, 124) = 1;
        if ( v102 )
          W_orbit_init((int)&F32(c, 0x14834), 4800, 0, 100, 90000, 0, 4, 30, a3);
        else
          W_orbit_step(c, a2, &F32(c, 0x14834), 0, 1, a3, 100077, 0);
        if ( W_slash_step(c, a2, a3, (int)D(0x634108), 2000, -2000, 0, a5) != 0 && F16(a2, 88) == 3 )
        {
          W_voice_cue(c, a3, 5u, 0, 0, 0);
          v247 = rnd();
          W_sfx_play_at(c, a3, 68, 0, 500 * v247 / 0x8000 - 500, 0);
        }
        v248 = F16(a2, 88);
        if ( v248 < 3u )
          F32(a2, 40) = 56000;
        if ( v248 == 3 )
        {
          v87 = F16(a2, 90) < 3u;
          F32(a2, 20) = 1200;
          if ( v87 )
            W_char_attack_hits(c, a2, 5600, 0, 4800, 4800, 0, a3, F32(a2, 76), 1000, 1);
        }
        v249 = F32(a2, 92) % 4 == 0;
        goto LABEL_652;
      case 0x2786:
        *a4 = 3;
        if ( F32(v303, 0) == 0 )
          goto LABEL_200;
        v267 = F16(a2, 88);
        if ( (uint16_t)v267 != 0 )
        {
          if ( v267 != 1 )
            W_orbit_step(c, a2, &F32(c, 0x14834), 100, 3, a3, 100078, 0);
        }
        else
        {
          W_orbit_init((int)&F32(c, 0x14834), -1600, 0, 400, 70000, 0, 0, 25, a3);
        }
        switch ( F16(a2, 88) )
        {
          case 0:
            W_body_set_clip(a2, (int)D(0x634f40));
            ++F16(a2, 88);
            goto LABEL_682;
          case 1:
LABEL_682:
            if ( F8(a2, 116) != 0 )
            {
              W_body_set_clip(a2, (int)D(0x6350d8));
              v268 = rnd();
              W_sfx_play_at(c, a3, 68, 0, 1000 * v268 / 0x8000 - 3500, 0);
              v269 = rnd();
              W_sfx_play_at(c, a3, 134, 0, 1000 * v269 / 0x8000 + 1000, 0);
              W_char_lunge(c, a2, 1400, a3, 1);
              ++F16(a2, 88);
              W_voice_cue(c, a3, 5u, 0, 0, 0);
              F32(a2, 40) = 60000;
            }
            break;
          case 2:
            v102 = F16(a2, 114) == 1;
            F32(a2, 24) = -24000;
            if ( v102 )
            {
              W_char_lunge(c, a2, 1400, a3, 1);
              ++F16(a2, 88);
            }
            goto LABEL_686;
          case 3:
LABEL_686:
            if ( F32(a2, 92) % 8 == 0 )
              W_slash_trail(c, a2, a3, 10, 10);
            if ( F16(a2, 114) == 3 )
            {
              W_char_lunge(c, a2, 2600, a3, 1);
              ++F16(a2, 88);
            }
            goto LABEL_690;
          case 4:
LABEL_690:
            if ( F8(a2, 116) != 0 )
              W_body_action_reset(a2);
            break;
          default:
            break;
        }
        if ( F16(a2, 90) < 3u && F16(a2, 88) >= 2u && F16(a2, 88) <= 3u )
        {
          if ( F16(a2, 114) != 0 )
          {
            if ( F16(a2, 114) <= 2u )
              W_char_attack_hits(c, a2, 6000, 0, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
          }
          else
          {
            W_char_attack_hits(c, a2, 4000, 0, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
          }
        }
        goto LABEL_723;
      case 0x2788:
        *a4 = 2;
        if ( F32(v303, 0) == 0 || a5 != 0 )
          goto LABEL_200;
        v250 = F16(a2, 88);
        if ( (uint16_t)v250 != 0 )
        {
          v251 = v250 - 1;
          if ( v251 != 0 )
          {
            if ( v251 == 1 )
            {
              if ( F16(a2, 90) < 4u )
              {
                if ( F16(a2, 114) != 0 )
                  W_char_attack_hits(c, a2, 4600, 2000, 6000, 6000, 4, a3, F32(a2, 76), 1100, 2);
                else
                  W_char_attack_hits(c, a2, 3200, -2000, 6000, 6000, 4, a3, F32(a2, 76), 1100, 2);
              }
              if ( F8(a2, 116) != 0 )
                W_body_action_reset(a2);
            }
            goto LABEL_625;
          }
        }
        else
        {
          if ( F32(a2, 24) > 20000 )
            goto LABEL_200;
          W_orbit_init((int)&F32(c, 0x14834), 1200, -3000, 800, 120000, 11250, 0, 40, a3);
          W_body_set_clip(a2, (int)D(0x635a68));
          ++F16(a2, 88);
          W_voice_cue(c, a3, 0x1Eu, 0, 0, 0);
          v252 = rnd();
          W_sfx_play_at(c, a3, 133, 0, 1000 * v252 / 0x8000 + 1000, 0);
          v253 = rnd();
          W_sfx_play_at(c, a3, 68, 0, 1000 * v253 / 0x8000 - 3000, 0);
        }
        if ( F16(a2, 90) < 4u )
        {
          switch ( F16(a2, 114) )
          {
            case 2:
              W_char_attack_hits(c, a2, 2000, -2000, 4000, 4000, 0, a3, F32(a2, 76), 900, 2);
              W_char_attack_hits(c, a2, 4000, 0, 4000, 4000, 4, a3, F32(a2, 76), 900, 2);
              break;
            case 3:
              W_char_attack_hits(c, a2, 4000, 1000, 6000, 6000, 5, a3, F32(a2, 76), 1000, 2);
              break;
            case 4:
              W_char_attack_hits(c, a2, 2000, 2000, 6000, 6000, 5, a3, F32(a2, 76), 1000, 2);
              break;
            case 5:
              W_char_attack_hits(c, a2, -400, 1000, 6000, 6000, 5, a3, F32(a2, 76), 1000, 2);
              break;
            case 6:
              W_char_attack_hits(c, a2, -2600, 0, 6000, 6000, 1, a3, F32(a2, 76), 1000, 2);
              break;
            case 7:
              W_char_attack_hits(c, a2, -400, -2000, 6000, 6000, 3, a3, F32(a2, 76), 1000, 2);
              W_char_attack_hits(c, a2, 2000, -4000, 6000, 6000, 2, a3, F32(a2, 76), 1000, 2);
              break;
            default:
              break;
          }
        }
        if ( F8(a2, 116) != 0 )
        {
          W_body_set_clip(a2, (int)D(0x635c00));
          ++F16(a2, 88);
        }
LABEL_625:
        F8(a2, 124) = 1;
        W_orbit_step(c, a2, &F32(c, 0x14834), 0, 2, a3, 100079, 0);
        v254 = F32(a2, 92) + 1;
        F32(a2, 92) = v254;
        v255 = v254 & 3;
        v249 = (v254 & 0x80000003) == 0;
        if ( v254 < 0 )
LABEL_651:
          v249 = ((v255 - 1) | 0xFFFFFFFC) == -1;
LABEL_652:
        if ( v249 )
          W_slash_trail(c, a2, a3, 10, 10);
        goto LABEL_723;
      case 0x278A:
        v102 = F32(v303, 0) == 0;
        *a4 = 2;
        if ( v102 )
          goto LABEL_200;
        if ( F16(a2, 88) != 0 )
        {
          if ( F16(a2, 88) <= 3u )
            W_orbit_step(c, a2, &F32(c, 0x14834), 400, 2, a3, 100077, 0);
        }
        else
        {
          v262 = W_facing_back(F32(a2, 44));
          v263 = -F32(a2, 40);
          F32(a2, 44) = v262;
          F32(a2, 40) = v263;
          W_char_lunge(c, a2, 800, a3, 0);
          W_orbit_init((int)&F32(c, 0x14834), -400, -400, 400, 45000, 2000, 6, 30, a3);
        }
        if ( W_slash_step(c, a2, a3, (int)D(0x634758), 1600, -1600, 0, a5) != 0 )
        {
          if ( F16(a2, 88) == 2 )
          {
            W_voice_cue(c, a3, 1u, 0, 0, 0);
            v264 = rnd();
            W_sfx_play_at(c, a3, 68, 0, 1000 * v264 / 0x8000 - 1000, 0);
            if ( W_arche_combo_skill(c) != 0 )
              W_char_shot_fire(
                c,
                100006,
                a2,
                3200,
                0,
                1,
                a3,
                5,
                90,
                F32(a2, 76),
                1000,
                F32(a2, 44),
                0,
                2);
          }
          else if ( F16(a2, 88) == 3 )
          {
            F16(a2, 90) = 0;
          }
        }
        v265 = F16(a2, 88);
        if ( v265 == 3 )
          ++F16(a2, 100);
        else
          F16(a2, 100) = 0;
        if ( F16(a2, 90) != 0 )
          goto LABEL_723;
        v266 = v265 - 2;
        if ( v266 == 0 )
        {
          (*v285__f) = 1000;
          (*v280__f) = F32(a2, 76);
          goto LABEL_674;
        }
        if ( v266 != 1 )
          goto LABEL_723;
        (*v284__f) = 1000;
        (*v279__f) = F32(a2, 76);
        goto LABEL_672;
      default:
        goto LABEL_723;
    }
    v210 = F16(a2, 88);
    F8(a2, 124) = 1;
    switch ( v210 )
    {
      case 0:
        if ( F32(a2, 108) != D(0x6355a0) )
        {
          F32(a2, 108) = D(0x6355a0);
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        F16(a2, 88) = v210 + 1;
        v211 = 1000 * rnd();
        (*v301__f) = F32(c, 0x40);
        if ( a3 == 0 )
        {
          v212 = (intptr_t *)W_vpool_take(SYS32(1404), 0);
          if ( v212 != 0 )
          {
            (*i__f) = F32(v212, 0);
            if ( W_sfx_place(
                   c,
                   F32(CFG(), 360),
                   0,
                   0,
                   (*v301__f),
                   (int *)&(*v300__f),
                   &(*v302__f)) != 0 )
            {
              g_eng.dsb_volume_5bb870((*i__f), (int)(*v300__f));
              g_eng.dsb_pan_5bb880((*i__f), (*v302__f));
              g_eng.dsb_freq_5bb890((*i__f), v211 / 0x8000 + 1000);
              g_eng.dsb_play_5bb8c0((*i__f));
            }
          }
        }
        F32(a2, 40) = 0;
        v213 = W_body_facing_sign(a2, -2800);
        if ( W_entity_move_horizontal(c, (_DWORD *)a2, v213, 1, 1, 0, 0, 0) != 0 )
        {
          if ( a3 == 0 )
          {
            F32(c, 0x14834) += 2800;
            goto LABEL_498;
          }
        }
        else if ( a3 == 0 )
        {
LABEL_498:
          if ( F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place(c, (int *)&(*v300__f), (int *)&(*v301__f), &(*i__f)) != 0 )
          {
            W_voice_play(F32(c, 0x15b54), 7u, 0, (int)(*v300__f), (int)(*v301__f), (int)(*i__f), 0);
          }
        }
LABEL_529:
        if ( F32(a2, 92) % 4 == 0 && W_entity_sprite_descs(c, a2, (int)v308, v303) != 0 )
        {
          for ( (*i__f) = 0; (uint16_t)(*i__f) < v303[0]; (*i__f) = ((uint8_t *)(*i__f) + 1) )
          {
            v219 = 10 * (uint16_t)(*i__f);
            v220 = W_particle_spawn(
                                   c,
                                   (int)c,
                                   100082,
                                   (_DWORD *)a2,
                                   F32(a2, 4) + 100 * (F32(a2, 64) + v308[v219 / 2]),
                                   F32(a2, 8) + 100 * (F32(a2, 68) + v308[v219 / 2 + 1]),
                                   99,
                                   0,
                                   a3,
                                   0,
                                   0,
                                   0,
                                   0,
                                   10);
            (*v301__f) = v220;
            if ( v220 != 0 )
            {
              W_entity_track_on(v220, 0, v309[v219], v309[v219 + 1]);
              v221 = (*v301__f);
              W_particle_tint_set((*v301__f), 14);
              *(_DWORD *)((uint8_t *)(v221 + (1) * 0x260) + 0x0) = 10;
            }
          }
        }
        ++F32(a2, 92);
        goto LABEL_723;
      case 1:
        if ( F16(a2, 114) == 1 )
        {
          v214 = W_body_facing_sign(a2, -800);
          if ( W_entity_move_horizontal(c, (_DWORD *)a2, v214, 1, 1, 0, 0, 0) == 0 || a3 != 0 )
            goto LABEL_516;
          F32(c, 0x14834) += 800;
          ++F16(a2, 88);
        }
        goto LABEL_529;
      case 2:
        if ( F16(a2, 114) == 2 )
        {
          v215 = W_body_facing_sign(a2, -1600);
          if ( W_entity_move_horizontal(c, (_DWORD *)a2, v215, 1, 1, 0, 0, 0) == 0 || a3 != 0 )
            goto LABEL_521;
          F32(c, 0x14834) += 1600;
          ++F16(a2, 88);
        }
        goto LABEL_529;
      case 3:
        if ( F16(a2, 114) == 3 )
        {
          v216 = W_body_facing_sign(a2, -800);
          if ( W_entity_move_horizontal(c, (_DWORD *)a2, v216, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
            F32(c, 0x14834) += 800;
LABEL_516:
          ++F16(a2, 88);
        }
        goto LABEL_529;
      case 4:
        if ( F16(a2, 114) == 4 )
        {
          v217 = W_body_facing_sign(a2, -1600);
          if ( W_entity_move_horizontal(c, (_DWORD *)a2, v217, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
            F32(c, 0x14834) += 1600;
LABEL_521:
          ++F16(a2, 88);
        }
        goto LABEL_529;
      case 5:
        if ( F16(a2, 114) == 5 )
        {
          v218 = W_body_facing_sign(a2, -2000);
          if ( W_entity_move_horizontal(c, (_DWORD *)a2, v218, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
            F32(c, 0x14834) += 2000;
          ++F16(a2, 88);
        }
        goto LABEL_529;
      case 6:
        if ( F8(a2, 116) != 0 )
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
        goto LABEL_529;
      default:
        goto LABEL_529;
    }
  }
  if ( v14 == 10107 )
  {
    v83 = 0;
    (*v300__f) = 0;
LABEL_224:
    *a4 = 3;
    if ( a5 != 0 )
    {
      F8(a2, 124) = 1;
      if ( v83 != 0 && F16(a2, 88) != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          if ( a3 == 0 )
          {
            F32(c, 0x14834) = 0;
            F32(c, 0x14838) = 4000;
            F32(c, 0x1483c) = 800;
            F32(c, 0x14840) = 4500;
            F32(c, 0x14844) = 4000;
            F16(c, 0x1484c) = 0;
            F16(c, 0x1484e) = 0;
            F16(c, 0x14850) = 20;
          }
        }
        else
        {
          v88 = &F32(c, 0x14834);
          if ( a3 == 0 )
          {
            v89 = F16(c, 0x1484c);
            if ( v89 >= F16(c, 0x1484e) )
            {
              if ( v89 < F16(c, 0x14850) )
              {
                for ( (*i__f) = 2; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
                {
                  v90 = F32(c, 0x14844);
                  v91 = F32(c, 0x14840);
                  ++F16(c, 0x1484c);
                  v92 = W_angle_wrap(v91, v90, 1000);
                  v93 = F32(c, 0x14844);
                  v94 = *v88;
                  F32(c, 0x14840) = v92;
                  F32(c, 0x14844) = v93 + 400;
                  W_polar_div(
                    F32(c, 0x1483c),
                    v92,
                    1000,
                    v94,
                    F32(c, 0x14838),
                    &F32(c, 0x14834),
                    &F32(c, 0x14838));
                  W_char_particle(c, 100077, a2, *v88, F32(c, 0x14838), F32(a2, 44), 1, 0, 0, 2, 1, 1, 0);
                }
              }
            }
            else
            {
              F16(c, 0x1484c) = v89 + 1;
            }
          }
        }
      }
      v95 = F16(a2, 88);
      switch ( v95 )
      {
        case 0:
          if ( F32(a2, 108) != D(0x635270) )
          {
            F32(a2, 108) = D(0x635270);
            F8(a2, 116) = 0;
            F16(a2, 114) = 0;
            F16(a2, 112) = 0;
          }
          F16(a2, 88) = v95 + 1;
          v96 = rnd();
          (*v301__f) = F32(c, 0x40);
          v97 = 1000 * v96 / 0x8000 + 1000;
          if ( a3 == 0 )
          {
            v98 = (intptr_t *)W_vpool_take(SYS32(1400), 0);
            if ( v98 != 0 )
            {
              (*i__f) = F32(v98, 0);
              if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, (*v301__f), &(*v302__f), &(*v304__f)) != 0 )
              {
                g_eng.dsb_volume_5bb870((*i__f), (*v302__f));
                g_eng.dsb_pan_5bb880((*i__f), (*v304__f));
                g_eng.dsb_freq_5bb890((*i__f), v97);
                g_eng.dsb_play_5bb8c0((*i__f));
              }
            }
          }
          goto LABEL_244;
        case 1:
LABEL_244:
          v102 = F16(a2, 114) == 2;
          F32(a2, 20) = 2400;
          if ( !v102 )
            goto LABEL_271;
          v99 = W_body_facing_sign(a2, 2400);
          if ( W_entity_move_horizontal(c, (_DWORD *)a2, v99, 1, 1, 0, 0, 0) != 0 && a3 == 0 )
            F32(c, 0x14834) -= 2400;
          v100 = (*v300__f);
          ++F16(a2, 88);
          if ( v100 == 0 )
          {
            W_voice_cue(c, a3, 7u, 0, 0, 0);
            goto LABEL_279;
          }
          if ( a3 == 0
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place(c, &(*v302__f), (int *)&(*v301__f), &(*i__f)) != 0 )
          {
            W_voice_play(F32(c, 0x15b54), 3u, 0, (*v302__f), (int)(*v301__f), (int)(*i__f), 0);
          }
          v101 = rnd();
          W_sfx_play_at(c, a3, 68, 0, 1000 * v101 / 0x8000 - 500, 0);
          goto LABEL_271;
        case 2:
          v102 = F16(a2, 114) == 3;
          F32(a2, 20) = 3200;
          goto LABEL_261;
        case 3:
          v102 = F16(a2, 114) == 4;
          F32(a2, 20) = 3200;
          goto LABEL_261;
        case 4:
          v102 = F16(a2, 114) == 5;
          F32(a2, 20) = 3200;
LABEL_261:
          if ( v102 )
          {
            W_char_lunge(c, a2, 2000, a3, 0);
            ++F16(a2, 88);
          }
          goto LABEL_271;
        case 5:
          v103 = F8(a2, 116);
          F32(a2, 20) = 2400;
          if ( v103 == 0 )
            goto LABEL_271;
          W_char_lunge(c, a2, 1600, a3, 0);
          if ( (*v300__f) != 0 )
          {
            if ( F32(a2, 108) != D(0x635408) )
            {
              F32(a2, 108) = D(0x635408);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
            ++F16(a2, 88);
LABEL_271:
            if ( (*v300__f) != 0 && F16(a2, 90) < 2u )
            {
              switch ( F16(a2, 114) )
              {
                case 2:
                  W_char_attack_hits(c, a2, 1600, -2400, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
                  break;
                case 3:
                  W_char_attack_hits(c, a2, 4800, -1600, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
                  break;
                case 4:
                  W_char_attack_hits(c, a2, 3200, 1200, 4000, 4000, 0, a3, F32(a2, 76), 1000, 2);
                  break;
                default:
                  break;
              }
            }
          }
          else
          {
            F16(a2, 56) = 2;
            F16(a2, 60) = 0;
            F16(a2, 58) = 1;
            *a4 = 0;
            W_body_action_reset(a2);
          }
LABEL_279:
          v105 = F32(a2, 92) + 1;
          F32(a2, 92) = v105;
          if ( v105 % 5 == 0 )
            W_slash_trail(c, a2, a3, 14, 10);
          break;
        case 6:
          v104 = F8(a2, 116);
          F32(a2, 20) = 2400;
          if ( v104 != 0 )
            W_body_action_reset(a2);
          goto LABEL_271;
        default:
          goto LABEL_271;
      }
      goto LABEL_723;
    }
LABEL_200:
    W_body_action_reset(a2);
    return 0;
  }
  if ( F16(a2, 72) > 0x2775u )
  {
    switch ( F16(a2, 72) )
    {
      case 0x2776:
        v102 = F32(v303, 0) == 0;
        *a4 = 2;
        if ( v102 )
          goto LABEL_200;
        if ( F16(a2, 88) != 0 )
          W_orbit_step(c, a2, &F32(c, 0x14834), 0, 3, a3, 100077, 0);
        else
          W_orbit_init((int)&F32(c, 0x14834), -800, 0, 400, 90000, 0, 2, 16, a3);
        if ( W_slash_step(c, a2, a3, (int)D(0x633ab8), 1000, -1000, 0, a5) == 0 )
          goto LABEL_220;
        if ( F16(a2, 88) == 3 )
        {
          W_voice_cue(c, a3, 2u, 0, 0, 0);
          v86 = rnd();
          W_sfx_play_at(c, a3, 69, 0, 1000 * v86 / 0x8000 - 1000, 0);
LABEL_220:
          if ( F16(a2, 88) == 3 )
          {
            v87 = F16(a2, 90) < 2u;
            F32(a2, 20) = 1200;
            if ( v87 )
              W_char_attack_hits(c, a2, 4800, 2000, 6000, 2000, 0, a3, F32(a2, 76), 1000, 1);
          }
        }
        goto LABEL_723;
      case 0x2777:
        v44 = F32(v303, 0);
        *a4 = 2;
        if ( v44 == 0 || F32(a2, 24) < 0 )
          goto LABEL_200;
        if ( F16(a2, 88) != 0 )
        {
          v45 = &F32(c, 0x14834);
          if ( a3 == 0 )
          {
            v46 = F16(c, 0x1484c);
            if ( v46 >= F16(c, 0x1484e) )
            {
              if ( v46 < F16(c, 0x14850) )
              {
                for ( (*i__f) = 2; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
                {
                  v47 = F32(c, 0x14844);
                  v48 = F32(c, 0x14840);
                  ++F16(c, 0x1484c);
                  v49 = W_angle_wrap(v48, v47, 1000);
                  v50 = F32(c, 0x14844);
                  v51 = *v45;
                  F32(c, 0x14840) = v49;
                  F32(c, 0x14844) = v50 + 200;
                  W_polar_div(
                    F32(c, 0x1483c),
                    v49,
                    1000,
                    v51,
                    F32(c, 0x14838),
                    &F32(c, 0x14834),
                    &F32(c, 0x14838));
                  W_char_particle(c, 100077, a2, *v45, F32(c, 0x14838), F32(a2, 44), 1, 0, 0, 2, 1, 1, 0);
                }
              }
            }
            else
            {
              F16(c, 0x1484c) = v46 + 1;
            }
          }
        }
        else if ( a3 == 0 )
        {
          F32(c, 0x14834) = 1600;
          F32(c, 0x14838) = -2400;
          F32(c, 0x1483c) = 300;
          F32(c, 0x14840) = 160000;
          F32(c, 0x14844) = 0;
          F16(c, 0x1484c) = 0;
          F16(c, 0x1484e) = 0;
          F16(c, 0x14850) = 16;
        }
        v52 = F16(a2, 88);
        v53 = 0;
        ++F32(a2, 92);
        switch ( v52 )
        {
          case 0:
            W_body_set_clip(a2, (int)D(0x630e88));
            v53 = 1;
            F16(a2, 88) = 1;
            goto LABEL_150;
          case 1:
LABEL_150:
            v54 = F8(a2, 116);
            F32(a2, 28) = 0;
            if ( v54 != 0 )
            {
              W_body_set_clip(a2, (int)D(0x63101c));
              F16(a2, 88) = 2;
              v53 = 1;
            }
            if ( a5 != 0 )
              W_body_action_reset(a2);
            if ( v53 != 0 )
              break;
            goto LABEL_182;
          case 2:
            F32(a2, 28) = 64000;
            if ( a5 == 0 )
              goto LABEL_182;
            W_body_set_clip(a2, (int)D(0x6311b0));
            F16(a2, 88) = 3;
            break;
          case 3:
            if ( F8(a2, 116) == 0 )
              goto LABEL_182;
            W_body_set_clip(a2, (int)D(0x631344));
            F16(a2, 88) = 4;
            break;
          case 4:
            if ( F8(a2, 116) != 0 )
              W_body_action_reset(a2);
            goto LABEL_182;
          default:
            goto LABEL_182;
        }
        if ( F16(a2, 88) == 2 )
        {
          if ( a3 == 0
            && F32(c, 0x15b54) != 0
            && F32(c, 0x15b5c) == 0
            && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
            && W_voice_place(c, (int *)&(*v300__f), (int *)&(*v301__f), &(*i__f)) != 0 )
          {
            W_voice_play(F32(c, 0x15b54), 0x1Bu, 0, (int)(*v300__f), (int)(*v301__f), (int)(*i__f), 0);
          }
          v59 = rnd();
          (*i__f) = F32(c, 0x40);
          v60 = 1000 * v59 / 0x8000 - 1500;
          if ( a3 == 0 )
          {
            v61 = (intptr_t *)W_vpool_take(SYS32(1140), 0);
            if ( v61 != 0 )
            {
              (*v300__f) = F32(v61, 0);
              if ( W_sfx_place(
                     c,
                     F32(CFG(), 360),
                     0,
                     0,
                     (*i__f),
                     (int *)&(*v301__f),
                     &(*v302__f)) != 0 )
              {
                g_eng.dsb_volume_5bb870((*v300__f), (int)(*v301__f));
                g_eng.dsb_pan_5bb880((*v300__f), (*v302__f));
                (*v292__f) = v60;
                v58 = (*v300__f);
                g_eng.dsb_freq_5bb890((*v300__f), (*v292__f));
                goto LABEL_181;
              }
            }
          }
        }
        else if ( F16(a2, 88) == 3 )
        {
          if ( a3 == 0 )
          {
            for ( (*i__f) = 16; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
            {
              v55 = F32(a2, 12) + 1600;
              v56 = rnd();
              W_particle_spawn(
                c,
                (int)c,
                100080,
                (_DWORD *)a2,
                v55 * v56 / 0x8000 - v55 / 2,
                0,
                99,
                4,
                0,
                0,
                2,
                0,
                0,
                10);
            }
          }
          (*i__f) = F32(c, 0x40);
          if ( a3 == 0 )
          {
            v57 = (intptr_t *)W_vpool_take(SYS32(1152), 0);
            if ( v57 != 0 )
            {
              v58 = F32(v57, 0);
              if ( W_sfx_place(
                     c,
                     F32(CFG(), 360),
                     0,
                     0,
                     (*i__f),
                     (int *)&(*v301__f),
                     (int *)&(*v300__f)) != 0 )
              {
                g_eng.dsb_volume_5bb870(v58, (int)(*v301__f));
                g_eng.dsb_pan_5bb880(v58, (int)(*v300__f));
                g_eng.dsb_freq_5bb890(v58, 1000);
LABEL_181:
                g_eng.dsb_play_5bb8c0(v58);
              }
            }
          }
        }
LABEL_182:
        if ( F16(a2, 88) == 2 )
        {
          if ( F16(a2, 90) < 3u )
          {
            v72 = F32(a2, 76);
            (*v301__f) = F32(v72, 740);
            v73 = rnd();
            v74 = W_char_skill_roll(c, (_DWORD *)v72, 1000, 1000 * v73 / 0x8000);
            v75 = F32(v72, 176);
            (*i__f) = v74;
            F32(v303, 0) = v75;
            v76 = W_char_mag_sum(c);
            v77 = F32(v72, 744);
            (*v305__f) = F32(v72, 180) + v76;
            (*v304__f) = 0;
            (*v302__f) = 0;
            (*v300__f) = 0;
            if ( v77 == 0 && F32(v72, 732) == 0 && F32(v303, 0) == 0 )
            {
              v78 = (_DWORD *)F32(c, 0x750);
              v79 = v78[74];
              F32(v303, 0) = v78[73];
              v80 = v78[75];
              (*v305__f) = v79;
              v81 = v78[76];
              v82 = v78[77];
              (*v304__f) = v80;
              (*v302__f) = v81;
              (*v300__f) = v82;
            }
            W_body_hit_spawn(
              c,
              (int)c,
              a2,
              1600,
              5600,
              4000,
              4000,
              4,
              a3,
              (int)(*v301__f),
              F32(v303, 0),
              (*v305__f),
              0,
              2,
              (int)(*i__f),
              1,
              F32(v72, 736),
              F32(a2, 96),
              0,
              F32(v72, 756),
              F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228),
              (*v304__f),
              (*v302__f),
              (int)(*v300__f));
            if ( F32(c, 0xc7f4) != 0 )
            {
              W_body_hit_spawn(
                c,
                (int)c,
                a2,
                F32(c, 0xc7f8) + 1600,
                5600,
                4000,
                4000,
                4,
                a3,
                (int)(*v301__f),
                F32(v303, 0),
                (*v305__f),
                0,
                2,
                (int)(*i__f),
                1,
                F32(v72, 736),
                F32(a2, 96),
                0,
                F32(v72, 756),
                F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228),
                (*v304__f),
                (*v302__f),
                (int)(*v300__f));
              W_body_hit_spawn(
                c,
                (int)c,
                a2,
                1600 - F32(c, 0xc7f8),
                5600,
                4000,
                4000,
                4,
                a3,
                (int)(*v301__f),
                F32(v303, 0),
                (*v305__f),
                0,
                2,
                (int)(*i__f),
                1,
                F32(v72, 736),
                F32(a2, 96),
                0,
                F32(v72, 756),
                F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228),
                (*v304__f),
                (*v302__f),
                (int)(*v300__f));
            }
          }
        }
        else if ( F16(a2, 88) == 3 )
        {
          v62 = F32(a2, 92);
          F32(a2, 20) = 2400;
          if ( v62 > 20 && F16(a2, 90) < 3u )
          {
            v63 = F32(a2, 76);
            (*i__f) = F32(v63, 740);
            v64 = rnd();
            (*v301__f) = W_char_skill_roll(c, (_DWORD *)v63, 1000, 1000 * v64 / 0x8000);
            F32(v303, 0) = F32(v63, 176);
            v65 = W_char_mag_sum(c);
            v66 = F32(v63, 744);
            (*v300__f) = (F32(v63, 180) + v65);
            (*v302__f) = 0;
            (*v304__f) = 0;
            (*v305__f) = 0;
            if ( v66 == 0 && F32(v63, 732) == 0 && F32(v303, 0) == 0 )
            {
              v67 = (_DWORD *)F32(c, 0x750);
              v68 = v67[74];
              F32(v303, 0) = v67[73];
              v69 = v67[75];
              (*v300__f) = v68;
              v70 = v67[76];
              v71 = v67[77];
              (*v302__f) = v69;
              (*v304__f) = v70;
              (*v305__f) = v71;
            }
            W_body_hit_spawn(
              c,
              (int)c,
              a2,
              2000,
              4000,
              6400,
              1600,
              0,
              a3,
              (int)(*i__f),
              F32(v303, 0),
              (int)(*v300__f),
              0,
              2,
              (int)(*v301__f),
              1,
              F32(v63, 736),
              F32(a2, 96),
              0,
              F32(v63, 756),
              F32(F32(c, 0x750), 216)
            + F32(F32(c, 0x750), 224)
            + F32(F32(c, 0x750), 228),
              (*v302__f),
              (*v304__f),
              (*v305__f));
            if ( F32(c, 0xc7f4) != 0 )
            {
              W_body_hit_spawn(
                c,
                (int)c,
                a2,
                F32(c, 0xc7f8) + 2000,
                4000,
                6400,
                1600,
                0,
                a3,
                (int)(*i__f),
                F32(v303, 0),
                (int)(*v300__f),
                0,
                2,
                (int)(*v301__f),
                1,
                F32(v63, 736),
                F32(a2, 96),
                0,
                F32(v63, 756),
                F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228),
                (*v302__f),
                (*v304__f),
                (*v305__f));
              W_body_hit_spawn(
                c,
                (int)c,
                a2,
                2000 - F32(c, 0xc7f8),
                4000,
                6400,
                1600,
                0,
                a3,
                (int)(*i__f),
                F32(v303, 0),
                (int)(*v300__f),
                0,
                2,
                (int)(*v301__f),
                1,
                F32(v63, 736),
                F32(a2, 96),
                0,
                F32(v63, 756),
                F32(F32(c, 0x750), 216)
              + F32(F32(c, 0x750), 224)
              + F32(F32(c, 0x750), 228),
                (*v302__f),
                (*v304__f),
                (*v305__f));
            }
          }
        }
        goto LABEL_723;
      case 0x2778:
        goto LABEL_305;
      case 0x2779:
        v102 = F32(v303, 0) == 0;
        *a4 = 2;
        if ( v102 )
          goto LABEL_200;
        if ( F16(a2, 88) != 0 )
          W_orbit_step(c, a2, &F32(c, 0x14834), 0, 2, a3, 100077, 0);
        else
          W_orbit_init((int)&F32(c, 0x14834), -800, 0, 500, 270000, 0, 10, 22, a3);
        if ( W_slash_step(c, a2, a3, (int)D(0x633468), 0, 0, 0, a5) != 0 )
        {
          if ( F16(a2, 88) == 3 )
          {
            W_voice_cue(c, a3, 2u, 0, 0, 0);
            v85 = rnd();
            W_sfx_play_at(c, a3, 69, 0, 1000 * v85 / 0x8000 - 2500, 0);
          }
          else if ( F16(a2, 88) == 5 )
          {
            F32(a2, 44) = W_facing_back(F32(a2, 44));
            F32(a2, 40) = -F32(a2, 40);
          }
        }
        if ( F16(a2, 90) < 2u && F16(a2, 88) == 3 )
        {
          (*v278__f) = F32(a2, 76);
          F32(a2, 20) = 1200;
          W_char_attack_hits(c, a2, -2800, 2000, 6000, 2000, 1, a3, (*v278__f), 1000, 1);
        }
        goto LABEL_723;
      case 0x277A:
        v83 = 1;
        (*v300__f) = 1;
        if ( F32(v303, 0) == 0 )
          goto LABEL_200;
        goto LABEL_224;
      default:
        goto LABEL_723;
    }
  }
  if ( v14 == 10101 )
  {
LABEL_284:
    if ( F16(a2, 84) == 6 )
    {
      (*v301__f) = 1;
      (*i__f) = 1250;
      (*v302__f) = (int)D(0x632e18);
    }
    v102 = F32(v303, 0) == 0;
    *a4 = 2;
    if ( !v102 )
    {
      if ( F16(a2, 88) != 0 )
      {
        if ( F16(a2, 88) <= 3u )
          W_orbit_step(c, a2, &F32(c, 0x14834), 400, 2, a3, 100077, 0);
      }
      else
      {
        W_orbit_init((int)&F32(c, 0x14834), -400, -400, 400, 45000, 2000, 0, 24, a3);
      }
      if ( W_slash_step(c, a2, a3, (*v302__f), 1600, -1600, (int)(*v301__f), a5) != 0 )
      {
        if ( F16(a2, 88) == 2 )
        {
          W_voice_cue(c, a3, 1u, 0, 0, 0);
          v106 = rnd();
          W_sfx_play_at(c, a3, 68, 0, 1000 * v106 / 0x8000 - 1000, 0);
          if ( (*v300__f) != 0 )
            W_char_shot_fire(c, 100006, a2, 3200, 0, 1, a3, 5, 90, F32(a2, 76), 1000, F32(a2, 44), 0, 2);
        }
        else if ( F16(a2, 88) == 3 )
        {
          F16(a2, 90) = 0;
        }
      }
      v107 = F16(a2, 88);
      if ( v107 == 3 )
        ++F16(a2, 100);
      else
        F16(a2, 100) = 0;
      if ( F16(a2, 90) == 0 )
      {
        v108 = v107 - 2;
        if ( v108 != 0 )
        {
          if ( v108 == 1 )
          {
            (*v284__f) = (int)(*i__f);
            (*v279__f) = F32(a2, 76);
LABEL_672:
            W_char_attack_hits(c, a2, 4600, 800, 4000, 4000, 0, a3, (*v279__f), (*v284__f), 2);
          }
        }
        else
        {
          (*v285__f) = (int)(*i__f);
          (*v280__f) = F32(a2, 76);
LABEL_674:
          W_char_attack_hits(c, a2, 2400, -2400, 2000, 3000, 0, a3, (*v280__f), (*v285__f), 2);
        }
      }
      goto LABEL_723;
    }
    goto LABEL_200;
  }
  switch ( F16(a2, 72) )
  {
    case 3:
      *a4 = 2;
      switch ( F16(a2, 88) )
      {
        case 0:
          if ( F32(v303, 0) != 0 )
            goto LABEL_200;
          W_body_set_clip(a2, (int)D(0x62e090));
          ++F16(a2, 88);
          goto LABEL_123;
        case 1:
LABEL_123:
          if ( F16(a2, 114) == 3 )
            ++F16(a2, 88);
          break;
        case 2:
          if ( F16(a2, 114) == 4 )
          {
            W_sfx_play_at(c, a3, 66, 0, 0, 0);
            if ( a3 == 0 )
              W_char_party_mode_set(c, (void *)1);
            ++F16(a2, 88);
          }
          break;
        case 3:
          if ( F8(a2, 116) != 0 )
          {
            F32(a2, 40) = 0;
            W_body_action_reset(a2);
          }
          break;
        default:
          break;
      }
      if ( F16(a2, 90) == 0 && F16(a2, 114) >= 4u && F16(a2, 114) <= 5u )
      {
        (*v274__f) = F32(a2, 96);
        v42 = rnd();
        v43 = W_char_atk_roll(c, 1300, 1000 * v42 / 0x8000, 0, 0);
        W_body_hit_spawn(c, (int)c, a2, 2000, 0, 4000, 5000, 0, a3, 1, 0, 0, 0, 2, v43, 1, 0, (*v274__f), 0, 3, 0, 0, 0, 0);
      }
      goto LABEL_723;
    case 4:
      *a4 = 2;
      switch ( F16(a2, 88) )
      {
        case 0:
          if ( F32(v303, 0) == 0 )
            goto LABEL_200;
          W_body_set_clip(a2, (int)D(0x62e228));
          ++F16(a2, 88);
          goto LABEL_112;
        case 1:
LABEL_112:
          if ( F16(a2, 114) == 0 )
          {
            if ( a3 == 0 )
              W_char_party_mode_set(c, 0);
            W_sfx_play_at(c, a3, 69, -500, -1000, 0);
            ++F16(a2, 88);
          }
          goto LABEL_116;
        case 2:
LABEL_116:
          if ( F16(a2, 114) == 10 )
          {
            W_sfx_play_at(c, a3, 67, 0, 0, 0);
            ++F16(a2, 88);
          }
          break;
        case 3:
          break;
        default:
          goto LABEL_723;
      }
      if ( F8(a2, 116) != 0 )
        W_body_action_reset(a2);
      goto LABEL_723;
    case 0xB:
      if ( F32(v303, 0) != 0 )
        goto LABEL_43;
      v15 = D(0x62f538);
      goto LABEL_35;
    case 0xC:
      if ( F32(v303, 0) != 0 )
        goto LABEL_43;
      v15 = D(0x62f6d0);
LABEL_35:
      if ( F32(a2, 108) != v15 )
      {
        F32(a2, 108) = v15;
        F8(a2, 116) = 0;
        F16(a2, 114) = 0;
        F16(a2, 112) = 0;
      }
      goto LABEL_42;
    case 0x64:
    case 0x65:
      v16 = F16(a2, 88);
      v17 = F32(a2, 92) + 1;
      F32(a2, 92) = v17;
      switch ( v16 )
      {
        case 0:
          F16(a2, 88) = 1;
          if ( F32(a2, 108) != D(0x62f208) )
          {
            F32(a2, 108) = D(0x62f208);
            F8(a2, 116) = 0;
            F16(a2, 114) = 0;
            F16(a2, 112) = 0;
          }
LABEL_46:
          if ( v17 > (v14 != 101 ? 520 : 180) && F16(a2, 114) == 0 )
          {
            if ( F32(a2, 108) == D(0x62f3a0) )
            {
LABEL_74:
              ++F16(a2, 88);
            }
            else
            {
              F32(a2, 108) = D(0x62f3a0);
              ++F16(a2, 88);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
          }
          break;
        case 1:
          goto LABEL_46;
        case 2:
LABEL_42:
          if ( F8(a2, 116) != 0 )
          {
LABEL_43:
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
          break;
        default:
          break;
      }
LABEL_723:
      if ( F32(F32(c, 0x750), 212) == 0
        && F16(a2, 72) == 0
        && F32(c, 0x200) == 0
        && W_crate_push(c, a7, (_DWORD *)a2, (_DWORD *)a5, a6, a3, 1) != 0 )
      {
        W_body_action_start(a2, 114, 210);
      }
      return (*v307__f);
    case 0x72:
      *a4 = 3;
      v18 = F16(a2, 88);
      switch ( v18 )
      {
        case 0:
          ++F32(a2, 92);
          (*i__f) = 0;
          if ( a3 != 0 || a5 == 0 || F32(a2, 40) > F32(c, 0x1596c) )
            goto LABEL_70;
          switch ( *a7 )
          {
            case 1:
            case 5:
              if ( F32(a2, 44) != 3 )
                goto LABEL_70;
              v19 = F32(a2, 4) - 400;
              break;
            case 2:
            case 6:
              if ( F32(a2, 44) != 1 )
                goto LABEL_70;
              v19 = F32(a2, 12) + F32(a2, 4) + 400;
              break;
            default:
              goto LABEL_70;
          }
          (*v302__f) = 0;
          v20 = (int)((uint64_t)(1374389535LL * v19) >> 32) >> 5;
          v21 = F32(a2, 16);
          (*v300__f) = (100 * ((v20 >> 31) + v20));
          v22 = F32(a2, 8);
          v23 = v21 + v22 - 1;
          v24 = F32(a2, 20);
          (*v304__f) = v23;
          (*v301__f) = (v24 + v22 + 400);
          v25 = *(_DWORD *)&GAP(W, 0x2788, 4);
          if ( F16(v25, 8) == 0 )
            goto LABEL_70;
          do
          {
            v26 = F32(v25, 4);
            v27 = F32((v26 + 4 * (uint16_t)(*v302__f)), 0);
            if ( c != v27
              && F32(v27, 260) == 3
              && F32(v27, 264) == 3
              && (int)(*v301__f) < W_part_bottom(F32((v26 + 4 * (uint16_t)(*v302__f)), 0), a6)
              && (*v304__f) > W_char_part_top(v27, a6)
              && (int)(*v300__f) <= W_part_right(v27, a6)
              && (int)(*v300__f) >= W_part_left(v27, a6) )
            {
              (*i__f) = v27;
            }
            v25 = *(_DWORD *)&GAP(W, 0x2788, 4);
            ++(*v302__f);
          }
          while ( (uint16_t)(*v302__f) < F16(v25, 8) );
          if ( (*i__f) == 0 )
          {
LABEL_70:
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
          goto LABEL_71;
        case 1:
          if ( F8(a2, 116) != 0 )
          {
            if ( F32(a2, 108) != D(0x62e6f0) )
            {
              F32(a2, 108) = D(0x62e6f0);
              F8(a2, 116) = 0;
              F16(a2, 114) = 0;
              F16(a2, 112) = 0;
            }
            F16(a2, 88) = v18 + 1;
          }
          goto LABEL_723;
        case 2:
          v28 = F32(a2, 92);
          if ( v28 >= 40 )
          {
            if ( a3 == 0
              && F32(c, 0x15b54) != 0
              && F32(c, 0x15b5c) == 0
              && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
              && W_voice_place(c, (int *)&(*v300__f), (int *)&(*v301__f), &(*i__f)) != 0 )
            {
              W_voice_play(F32(c, 0x15b54), 8u, 0, (int)(*v300__f), (int)(*v301__f), (int)(*i__f), 0);
            }
          }
          else
          {
            F32(a2, 92) = v28 + 1;
          }
          (*i__f) = 0;
          if ( a3 == 0 && a5 != 0 && F32(a2, 40) <= F32(c, 0x1596c) )
          {
            switch ( *a7 )
            {
              case 1:
              case 5:
                if ( F32(a2, 44) != 3 )
                  goto LABEL_71;
                v29 = F32(a2, 4);
                (*v301__f) = -1;
                v30 = v29 - 400;
                break;
              case 2:
              case 6:
                if ( F32(a2, 44) != 1 )
                  goto LABEL_71;
                v31 = F32(a2, 12);
                (*v301__f) = 1;
                v30 = v31 + F32(a2, 4) + 400;
                break;
              default:
                goto LABEL_71;
            }
            (*v302__f) = 0;
            v32 = F32(a2, 16);
            (*v300__f) = (100 * (v30 / 100));
            v33 = F32(a2, 8);
            v34 = v32 + v33 - 1;
            v35 = F32(a2, 20);
            (*v305__f) = v34;
            (*v304__f) = v35 + v33 + 400;
            v36 = *(_DWORD *)&GAP(W, 0x2788, 4);
            if ( F16(v36, 8) != 0 )
            {
              do
              {
                v37 = F32((F32(v36, 4) + 4 * (uint16_t)(*v302__f)), 0);
                if ( c != v37 && F32(v37, 0x104) == 3 && F32(v37, 0x108) == 3 )
                {
                  v38 = W_part_bottom(v37, a6);
                  if ( (*v304__f) < v38 )
                  {
                    v39 = W_char_part_top(v37, a6);
                    if ( (*v305__f) > v39 )
                    {
                      v40 = W_part_right(v37, a6);
                      if ( (int)(*v300__f) <= v40 )
                      {
                        v41 = W_part_left(v37, a6);
                        if ( (int)(*v300__f) >= v41 )
                          (*i__f) = v37;
                      }
                    }
                  }
                }
                v36 = *(_DWORD *)&GAP(W, 0x2788, 4);
                ++(*v302__f);
              }
              while ( (uint16_t)(*v302__f) < F16(v36, 8) );
              if ( (*i__f) != 0 )
              {
                if ( W_entity_slide_step((*i__f), (int)(*v301__f), a6, (int *)v303) )
                  W_entity_move_horizontal(c, (_DWORD *)a2, F32(v303, 0), 1, 1, 0, a6, 0);
                goto LABEL_723;
              }
            }
          }
LABEL_71:
          if ( F32(a2, 92) <= 10 )
            goto LABEL_723;
          if ( F32(a2, 108) != D(0x62e888) )
          {
            F32(a2, 108) = D(0x62e888);
            F8(a2, 116) = 0;
            F16(a2, 114) = 0;
            F16(a2, 112) = 0;
          }
          break;
        case 3:
          goto LABEL_42;
        default:
          goto LABEL_723;
      }
      goto LABEL_74;
    default:
      goto LABEL_723;
  }
}
/* unported callees: none */
