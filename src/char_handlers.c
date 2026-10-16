/* char_handlers.c — the integrator's per-type handlers (FUN_00442a70 char_integrate) + the item / skill effect.  See char_handlers.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (the decompile's control flow kept verbatim: labels, gotos across
 * the cases; IDA's local types, fields through width-typed accessors), then finished by hand against the asm.
 * IDA-__fastcall functions that never read edx are thiscall here; IDA-__stdcall ones that use their caller's ecx
 * take it as a leading ecx_ (always the caller's `this`). */
#include "char_handlers.h"

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
#include "behave_helpers.h"
#include "char_attack.h"
#include "char_def.h"
#include "char_hit.h"
#include "char_integrate.h"
#include "char_physics.h"
#include "char_state.h"
#include "char_status.h"
#include "entity_fx.h"
#include "entity_life.h"
#include "entity_move.h"
#include "gimmick.h"
#include "particle_spawn.h"
#include "skill_cast.h"
#include "sound_fx.h"
#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
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
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F64(p, o) (*(int64_t *)((uint8_t *)(intptr_t)(p) + (o)))

static inline intptr_t W_beh_47ae00(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)beh_47ae00((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t *)a7, (int32_t *)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t *)a12); }
static inline intptr_t W_beh_48be60(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)beh_48be60((wl_char *)a0, (uint16_t)a1, (int32_t)a2); }
static inline intptr_t W_body_action_reset(intptr_t a0, ...) { body_action_reset((wl_body *)a0); return 0; }
static inline intptr_t W_body_hit_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, intptr_t a20, intptr_t a21, intptr_t a22, intptr_t a23, ...) { return (intptr_t)body_hit_spawn((wl_char *)a0, (wl_char *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19, (int32_t)a20, (int32_t)a21, (int32_t)a22, (int32_t)a23); }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1, ...) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_char_add_hp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_add_hp((wl_char *)a0, (int32_t)a1, (int32_t *)a2, (int32_t)a3); }
static inline intptr_t W_char_add_mp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_add_mp((wl_char *)a0, (int32_t)a1, (int32_t *)a2, (int32_t)a3); }
static inline intptr_t W_char_anim_select(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { char_anim_select((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_attack_hits(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)char_attack_hits((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (const uint8_t *)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_char_drift(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)char_drift((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12); }
static inline intptr_t W_char_drift_move(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)char_drift_move((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_char_lunge(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)char_lunge((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_char_mag_sum(intptr_t a0, ...) { return (intptr_t)char_mag_sum((const wl_char *)a0); }
static inline intptr_t W_char_physics_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)char_physics_step((wl_char *)a0, (const wl_cmd *)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_char_room_logs(intptr_t a0, ...) { return (intptr_t)char_room_logs((const wl_char *)a0); }
static inline intptr_t W_char_skill_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_skill_roll((const wl_char *)a0, (const uint8_t *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_char_status_decay(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { char_status_decay((wl_char *)a0, (uint16_t)a1, (uint16_t)a2, (int32_t)a3, (uint32_t)a4, (char *)a5); return 0; }
static inline intptr_t W_char_status_inflict(intptr_t a0, intptr_t a1, intptr_t a2, ...) { char_status_inflict((wl_char *)a0, (uint16_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_dmg_glyphs(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)dmg_glyphs((const char *)a0, (uint16_t *)a1, (uint16_t *)a2); }
static inline intptr_t W_entity_fx_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)entity_fx_add((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_entity_fx_remove(intptr_t a0, intptr_t a1, ...) { entity_fx_remove((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_near(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)entity_near((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (const wl_entity *)a3, (int32_t)a4); }
static inline intptr_t W_entity_release(intptr_t a0, intptr_t a1, ...) { entity_release((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_faction_relation(intptr_t a0, intptr_t a1, ...) { return (intptr_t)faction_relation((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, ...) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_skill_mp_pay(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)skill_mp_pay((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_slash_step(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)slash_step((wl_char *)a0, (wl_body *)a1, (int32_t)a2, (intptr_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_slash_trail(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)slash_trail((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_speed_ramp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)speed_ramp((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_stat_jitter(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)stat_jitter((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_summon_init(intptr_t a0, intptr_t a1, ...) { return (intptr_t)summon_init((wl_char *)a0, (int32_t *)a1); }
static inline intptr_t W_summon_recall(intptr_t a0, intptr_t a1, ...) { return (intptr_t)summon_recall((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_summon_room_ok(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)summon_room_ok((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_summon_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)summon_spawn((int32_t)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8); }
static inline intptr_t W_voice_set_free(intptr_t a0, ...) { voice_set_free((wl_voiceset *)a0); return 0; }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }
static inline intptr_t W_world_log_line(intptr_t a0, intptr_t a1, ...) { world_log_line((wl_world *)a0, (const char *)a1); return 0; }
int32_t ih_455d80(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t * a7, int32_t a8);
int32_t ih_44d2e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t ih_4512d0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7);
void ih_4466b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t ih_467260(wl_char *self, int32_t a3, int32_t a4, int32_t a5);
int32_t item_effect(wl_char *self, int32_t a2, int32_t a3, uint16_t * a4, int32_t a5, int32_t a6, int16_t a7);
int32_t ih_5b28a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, char * Buffer);
int32_t ih_456c60(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);

/* 0x455d80: int __thiscall sub_455D80(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, _DWORD *a7, int a8) */
int32_t ih_455d80(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t * a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8;
    intptr_t v10;
    intptr_t v11;
    intptr_t * v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t result;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    int16_t v21;
    int32_t * v22;
    intptr_t v23;
    uint16_t v24;
    int32_t * v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    int32_t * v30;
    intptr_t v31;
    int32_t ** v32;
    intptr_t v33;
    int32_t * v34;
    intptr_t v35;
    intptr_t v36;
    intptr_t v37;
    intptr_t v38;
    intptr_t v39;
    intptr_t v40;
    int32_t * v41;
    intptr_t v42;
    int32_t * v43;
    int32_t * v44;
    intptr_t v45;
    intptr_t v46;
    intptr_t v47;
    intptr_t v48;
    int32_t * v49;
    intptr_t v50;
    intptr_t v51;
    intptr_t v52;
    intptr_t v53;
    intptr_t v54;
    intptr_t v55;
    _Alignas(8) uint8_t frame_[0x3c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3c */
    intptr_t *const v57__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x34 */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x30 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x28 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x24 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x1c */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x1c */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x8 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x4 */

  v8 = a2;
  (*v64__f) = 0;
  (*v63__f) = 0;
  switch ( F16(a2, 72) )
  {
    case 0x13:
      v49 = (_DWORD *)a3;
      *a4 = 3;
      v50 = F16(v8, 88);
      F8(v8, 124) = 1;
      switch ( v50 )
      {
        case 0:
          if ( a6 != 0 )
          {
            v51 = F32(c, 0x1d4);
            if ( v51 >= 0xC744 && (v51 <= 0xC747 || v51 == 100186) )
            {
              if ( F32(v8, 108) != D(0x65b7f8) )
              {
                F32(v8, 108) = D(0x65b7f8);
                F8(v8, 116) = 0;
                F16(v8, 114) = 0;
                F16(v8, 112) = 0;
              }
            }
            else
            {
              W_body_set_clip(v8, (int)D(0x65b660));
            }
            F16(v8, 88) = 1;
          }
          goto LABEL_107;
        case 1:
          if ( F8(v8, 116) != 0 )
          {
            F16(v8, 88) = 2;
            F32(v8, 92) = 0;
            (*v63__f) = 7;
          }
          goto LABEL_107;
        case 2:
          W_char_anim_select(c, v8, v49, a6);
          v52 = F32(v8, 92) + 1;
          F32(v8, 92) = v52;
          if ( v52 < 30 )
            (*v64__f) = 2;
          if ( v52 < 12 )
            (*v63__f) = 7;
          if ( v52 > 10 )
          {
            if ( F16(v8, 56) == 4 )
            {
              F16(v8, 88) = 3;
              v53 = rnd();
              W_sfx_play_at(c, (int)v49, 138, 0, 1000 * v53 / 0x8000 - 1000, 0);
              W_body_set_clip(v8, (int)D(0x65b990));
            }
            if ( a6 != 0 )
              W_body_action_reset(v8);
          }
          goto LABEL_107;
        case 3:
          v54 = F32(v8, 92) + 1;
          F32(v8, 92) = v54;
          if ( v54 < 30 )
            (*v64__f) = 2;
          if ( W_char_attack_hits(c, v8, 800, 800, 3200, 3200, 0, (int)v49, F32(v8, 76), 1000, 3) != 0 )
          {
            F32(v8, 40) = -32000;
            F32(v8, 24) = -32000;
            F16(v8, 88) = 5;
            F32(v8, 92) = 0;
            W_body_set_clip(v8, (int)D(0x65ab38));
          }
          else if ( a6 != 0 )
          {
            W_body_set_clip(v8, (int)D(0x65b4c8));
            F16(v8, 88) = 4;
            F32(v8, 92) = 0;
          }
          goto LABEL_107;
        case 4:
          if ( F8(v8, 116) != 0 )
            goto LABEL_126;
          return 0;
        case 5:
          v55 = F32(v8, 92) + 1;
          F32(v8, 92) = v55;
          if ( v55 <= 20 )
            return 0;
          W_body_action_reset(v8);
          return 0;
        default:
LABEL_107:
          ih_456c60(c, v8, (int)v49, a5, (*v64__f), (*v63__f), a6);
          result = 1;
          break;
      }
      return result;
    case 0x14:
      v47 = a3;
      (*v62__f) = a6;
      (*v58__f) = a3;
      (*v57__f) = a2;
      *a4 = 2;
      if ( W_slash_step(c, (*v57__f), (*v58__f), (int)D(0x65c188), 800, -800, 0, (*v62__f)) != 0 && F16(v8, 88) == 3 )
      {
        v48 = rnd();
        W_sfx_play_at(c, v47, 138, 0, 1000 * v48 / 0x8000 - 1000, 0);
      }
      if ( F16(v8, 90) != 0 || F16(v8, 88) != 3 )
        return 0;
      W_char_attack_hits(c, v8, 2400, -1600, 3200, 3200, 0, v47, F32(v8, 76), 1000, 3);
      return 0;
    case 0x15:
      v27 = a3;
      *a4 = 2;
      v28 = 0;
      switch ( F16(v8, 88) )
      {
        case 0:
          W_body_set_clip(v8, (int)D(0x65ce28));
          v28 = 1;
          F16(v8, 88) = 1;
          goto LABEL_62;
        case 1:
LABEL_62:
          if ( F8(v8, 116) == 0 )
            goto LABEL_70;
          W_body_set_clip(v8, (int)D(0x65cfbc));
          F16(v8, 88) = 2;
          goto LABEL_71;
        case 2:
          if ( F8(v8, 116) != 0 )
          {
            W_body_set_clip(v8, (int)D(0x65d150));
            W_char_lunge(c, v8, 0, v27, 1);
            F16(v8, 88) = 3;
          }
          else
          {
LABEL_70:
            if ( v28 == 0 )
              goto LABEL_76;
          }
LABEL_71:
          if ( F16(v8, 88) == 2 )
          {
            v29 = rnd();
            v30 = (_DWORD *)F32(c, 0x40);
            v31 = 1000 * v29 / 0x8000 - 1000;
            if ( a3 == 0 )
            {
              v32 = (_DWORD **)W_vpool_take(SYS32(1420), 0);
              if ( v32 != 0 )
              {
                a4 = *v32;
                if ( W_sfx_place(
                       c,
                       F32(CFG(), 360),
                       0,
                       0,
                       v30,
                       &a6,
                       (int *)&a7) != 0 )
                {
                  v33 = a4;
                  g_eng.dsb_volume_5bb870(a4, a6);
                  g_eng.dsb_pan_5bb880(v33, (int)a7);
                  g_eng.dsb_freq_5bb890(v33, v31);
                  g_eng.dsb_play_5bb8c0(v33);
                }
              }
            }
          }
LABEL_76:
          if ( F16(v8, 90) != 0 || F16(v8, 88) != 3 )
            return 0;
          v34 = F32(v8, 76);
          a8 = v34[185];
          v35 = rnd();
          v36 = W_char_skill_roll(c, v34, 1000, 1000 * v35 / 0x8000);
          v37 = v34[44];
          a2 = v36;
          a6 = v37;
          v38 = W_char_mag_sum(c);
          v39 = v34[186];
          v40 = v34[45] + v38;
          a5 = 0;
          a7 = 0;
          a4 = 0;
          if ( v39 == 0 && v34[183] == 0 && a6 == 0 )
          {
            v41 = (int *)F32(c, 0x750);
            v42 = v41[75];
            v40 = v41[74];
            a6 = v41[73];
            v43 = (_DWORD *)v41[76];
            a5 = v42;
            v44 = (_DWORD *)v41[77];
            a7 = v43;
            a4 = v44;
          }
          W_body_hit_spawn(
            c,
            (int)c,
            v8,
            7200,
            -8000,
            3200,
            3200,
            2,
            a3,
            a8,
            a6,
            v40,
            0,
            3,
            a2,
            1,
            v34[184],
            F32(v8, 96),
            0,
            v34[189],
            F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228),
            a5,
            (int)a7,
            (int)a4);
          if ( F32(c, 0xc7f4) == 0 )
            return 0;
          W_body_hit_spawn(
            c,
            (int)c,
            v8,
            F32(c, 0xc7f8) + 7200,
            -8000,
            3200,
            3200,
            2,
            a3,
            a8,
            a6,
            v40,
            0,
            3,
            a2,
            1,
            v34[184],
            F32(v8, 96),
            0,
            v34[189],
            F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228),
            a5,
            (int)a7,
            (int)a4);
          W_body_hit_spawn(
            c,
            (int)c,
            v8,
            7200 - F32(c, 0xc7f8),
            -8000,
            3200,
            3200,
            2,
            a3,
            a8,
            a6,
            v40,
            0,
            3,
            a2,
            1,
            v34[184],
            F32(v8, 96),
            0,
            v34[189],
            F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228),
            a5,
            (int)a7,
            (int)a4);
          result = 0;
          break;
        case 3:
          if ( F8(v8, 116) == 0 )
            goto LABEL_76;
          W_body_set_clip(v8, (int)D(0x65d2e4));
          W_char_lunge(c, v8, 0, v27, 1);
          F16(v8, 88) = 4;
          goto LABEL_71;
        case 4:
          if ( F8(v8, 116) == 0 )
            goto LABEL_76;
          W_body_action_reset(v8);
          F16(v8, 88) = 5;
          goto LABEL_71;
        default:
          goto LABEL_76;
      }
      return result;
    case 0x16:
      v45 = a3;
      (*v61__f) = a6;
      *a4 = 2;
      if ( W_slash_step(c, v8, v45, (int)D(0x65c7d8), 0, 0, 0, (*v61__f)) != 0 && F16(v8, 88) == 2 )
      {
        v46 = rnd();
        W_sfx_play_at(c, v45, 138, 0, 1000 * v46 / 0x8000 - 1000, 0);
      }
      if ( F16(v8, 90) != 0 )
        return 0;
      if ( F16(v8, 88) == 2 )
      {
        W_char_attack_hits(c, v8, 6400, -1600, 3200, 3200, 0, v45, F32(v8, 76), 1000, 3);
        return 0;
      }
      else
      {
        if ( F16(v8, 88) != 3 )
          return 0;
        W_char_attack_hits(c, v8, 10400, -1600, 3200, 3200, 0, v45, F32(v8, 76), 1000, 3);
        return 0;
      }
    case 0x17:
      v18 = 0;
      *a4 = 2;
      v19 = F32(v8, 76);
      switch ( F16(v8, 88) )
      {
        case 0:
          if ( W_skill_mp_pay(c, v19, a3, 0) == 0 )
          {
LABEL_126:
            W_body_action_reset(v8);
            return 0;
          }
          if ( F32(v8, 108) != D(0x65d478) )
          {
            F32(v8, 108) = D(0x65d478);
            F8(v8, 116) = 0;
            F16(v8, 114) = 0;
            F16(v8, 112) = 0;
          }
          v18 = 1;
          F16(v8, 88) = 1;
          goto LABEL_21;
        case 1:
LABEL_21:
          if ( F8(v8, 116) == 0 )
            goto LABEL_24;
          W_body_set_clip(v8, (int)D(0x65d60c));
          F16(v8, 88) = 2;
          goto LABEL_23;
        case 2:
          if ( F16(v8, 102) >= 0x64u )
          {
            F16(v8, 88) = 3;
            v18 = 1;
          }
          goto LABEL_31;
        case 3:
LABEL_31:
          if ( F16(v8, 102) >= 0x3E8u )
          {
            W_body_set_clip(v8, (int)D(0x65d7a0));
            if ( F8(v8, 104) == 0 )
              *a7 = 0;
          }
          else if ( F8(v8, 104) == 0 )
          {
            v21 = F16(v19, 728) + F16(v8, 102);
            if ( F16(v19, 728) + (unsigned int)F16(v8, 102) > 0x3E8 )
              v21 = 1000;
            v22 = a7;
            F16(v8, 102) = v21;
            *v22 = 0;
          }
          goto LABEL_24;
        case 4:
          W_body_set_clip(v8, (int)D(0x65d934));
          F16(v8, 88) = 5;
          F8(v8, 104) = 1;
          v18 = 1;
          goto LABEL_39;
        case 5:
LABEL_39:
          if ( F8(v8, 116) != 0 )
          {
            if ( W_skill_mp_pay(c, v19, a3, 1) == 0 )
              goto LABEL_28;
            W_body_set_clip(v8, (int)D(0x65dac8));
            v18 = 1;
            F16(v8, 88) = 6;
          }
LABEL_24:
          if ( F16(v8, 88) >= 4u )
            goto LABEL_47;
          v20 = F32(a8, 16);
          if ( v20 == 15 )
            goto LABEL_28;
          if ( v20 == 14 )
            goto LABEL_47;
          if ( F16(v8, 102) >= 0x3E8u )
          {
            F16(v8, 88) = 4;
            v18 = 1;
LABEL_47:
            v24 = F16(v8, 88);
            if ( v24 >= 4u && v24 <= 6u )
              *a7 = 0;
          }
          else
          {
LABEL_28:
            W_body_action_reset(v8);
          }
          if ( v18 == 0 )
            return 0;
LABEL_51:
          if ( F16(v8, 88) != 6 )
            return 0;
          v25 = F32(v8, 76);
          if ( a3 != 0 )
            return 0;
          if ( !W_summon_room_ok(c, (int)c, v25[177], v25[178], v25[179]) )
          {
            if ( F32(c, 0xc794) == 0 )
              return 0;
            while ( !W_summon_room_ok(c, (int)c, v25[177], v25[178], v25[179])
                 && (W_summon_recall((int)c, v25[177]) != 0 || W_summon_recall((int)c, 0) != 0) )
              ;
          }
          (*v60__f) = F32(v8, 44);
          (*v56__f) = v25[177];
          a3 = 0;
          v26 = W_summon_spawn((*v56__f), (_DWORD *)v8, 0, -1600, 4, 0, (*v60__f), 5, 0);
          if ( v26 == 0 )
            return 0;
          W_summon_init(v26, c);
          result = 0;
          break;
        case 6:
          v23 = F32(v8, 92) + 1;
          F32(v8, 92) = v23;
          if ( v23 > 20 )
          {
            W_body_set_clip(v8, (int)D(0x65dc5c));
            F16(v8, 88) = 7;
LABEL_23:
            v18 = 1;
          }
          goto LABEL_24;
        case 7:
          if ( F8(v8, 116) == 0 )
            goto LABEL_24;
          W_body_action_reset(v8);
          goto LABEL_51;
        default:
          goto LABEL_24;
      }
      return result;
    case 0x18:
      if ( F16(a2, 88) != 0 )
      {
        if ( F16(a2, 88) != 1 )
        {
LABEL_15:
          v14 = W_speed_ramp(F32(v8, 40), 1000, -1000, 0);
          (*v59__f) = F32(v8, 24);
          F32(v8, 40) = v14;
          v15 = W_speed_ramp((*v59__f), 1000, -1000, 56000);
          v16 = a3;
          F32(v8, 24) = v15;
          W_char_drift_move(c, v8, v16, a5, 0, 0, 1);
          return 0;
        }
      }
      else
      {
        if ( F32(a2, 108) != D(0x65bff0) )
        {
          F32(a2, 108) = D(0x65bff0);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
        v10 = rnd();
        a4 = (_DWORD *)F32(c, 0x40);
        v11 = 1000 * v10 / 0x8000 - 1000;
        if ( a3 == 0 )
        {
          v12 = W_vpool_take(SYS32(1420), 0);
          if ( v12 != 0 )
          {
            v13 = F32(v12, 0);
            if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, a4, (int *)&a7, &a2) != 0 )
            {
              g_eng.dsb_volume_5bb870(v13, (int)a7);
              g_eng.dsb_pan_5bb880(v13, a2);
              g_eng.dsb_freq_5bb890(v13, v11);
              g_eng.dsb_play_5bb8c0(v13);
            }
          }
        }
        ++F16(v8, 88);
        F32(v8, 24) = -40000;
        F32(v8, 40) = 40000;
      }
      if ( F8(v8, 116) != 0 || a6 != 0 )
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
      }
      goto LABEL_15;
    default:
      return 0;
  }
}

/* 0x44d2e0: int __thiscall sub_44D2E0(cls_4022A0 *this, int a2, int a3, int a4, int a5, _DWORD *a6) */
int32_t ih_44d2e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    intptr_t v8;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    int32_t * v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    int32_t * v19;
    intptr_t v20;
    int32_t ** v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    int32_t * v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    intptr_t v31;
    intptr_t v32;
    intptr_t v33;
    intptr_t v34;
    int32_t * v35;
    intptr_t v36;
    _Alignas(8) uint8_t frame_[0x3c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3c */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x30 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x1c */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x18 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x18 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x14 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x10 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0xc */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x8 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x4 */

  (*v46__f) = 1;
  v7 = F32(c, 0x1d4);
  (*v45__f) = 0;
  (*v42__f) = 0;
  (*v44__f) = 20;
  if ( v7 == 51062 )
  {
    (*v44__f) = 5;
    (*v46__f) = 2;
  }
  v8 = a2;
  if ( F16(a2, 72) != 51 )
    return 0;
  if ( F16(a2, 84) == 1 )
  {
    v10 = 40000;
    v11 = 10;
    (*v40__f) = -64000;
  }
  else
  {
    if ( F16(a2, 84) == 3 )
    {
      v10 = 80000;
      v11 = 10;
    }
    else
    {
      v10 = 40000;
      v11 = 0;
    }
    (*v40__f) = 0;
  }
  F8(a2, 124) = 1;
  switch ( F16(v8, 88) )
  {
    case 0:
      v12 = rnd();
      a6 = (_DWORD *)F32(c, 0x40);
      v13 = 1000 * v12 / 0x8000;
      if ( a3 == 0 )
      {
        v14 = (int *)W_vpool_take(SYS32(1432), 0);
        if ( v14 != 0 )
        {
          a2 = *v14;
          if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, a6, &(*v46__f), &(*v43__f)) != 0 )
          {
            g_eng.dsb_volume_5bb870(a2, (*v46__f));
            g_eng.dsb_pan_5bb880(a2, (*v43__f));
            (*v38__f) = v13;
            v15 = a2;
            g_eng.dsb_freq_5bb890(a2, (*v38__f));
            g_eng.dsb_play_5bb8c0(v15);
          }
        }
      }
      if ( F32(v8, 108) != D(0x662548) )
      {
        F32(v8, 108) = D(0x662548);
        F8(v8, 116) = 0;
        F16(v8, 114) = 0;
        F16(v8, 112) = 0;
      }
      ++F16(v8, 88);
      F32(v8, 24) = -40000;
      goto LABEL_19;
    case 1:
LABEL_19:
      v16 = (*v44__f);
      (*v42__f) = (*v40__f);
      v17 = F32(v8, 92) + 1;
      F32(v8, 92) = v17;
      if ( v17 > v16 )
      {
        F32(v8, 92) = 0;
        ++F16(v8, 88);
        F32(v8, 24) = 500 * v10 / 1000;
        v18 = rnd();
        v19 = (_DWORD *)F32(c, 0x40);
        v20 = 1000 * v18 / 0x8000;
        if ( a3 == 0 )
        {
          v21 = (_DWORD **)W_vpool_take(SYS32(1436), 0);
          if ( v21 != 0 )
          {
            a6 = *v21;
            if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v19, &a2, &(*v46__f)) != 0 )
            {
              v22 = a6;
              g_eng.dsb_volume_5bb870(a6, a2);
              g_eng.dsb_pan_5bb880(v22, (*v46__f));
              g_eng.dsb_freq_5bb890(v22, v20);
              g_eng.dsb_play_5bb8c0(v22);
            }
          }
        }
        if ( F32(v8, 108) != D(0x662878) )
        {
          F32(v8, 108) = D(0x662878);
          F8(v8, 116) = 0;
          F16(v8, 114) = 0;
          F16(v8, 112) = 0;
        }
      }
      break;
    case 2:
      if ( F8(v8, 116) != 0 && F32(v8, 108) != D(0x662a10) )
      {
        F32(v8, 108) = D(0x662a10);
        F8(v8, 116) = 0;
        F16(v8, 114) = 0;
        F16(v8, 112) = 0;
      }
      goto LABEL_30;
    case 3:
LABEL_30:
      v26 = F32(v8, 92) + 1;
      (*v42__f) = v10;
      F32(v8, 92) = v26;
      (*v45__f) = v11 >= v26 ? 20000 : 64000;
      if ( v26 > 10 )
      {
        v27 = F32(v8, 76);
        (*v44__f) = v27[185];
        v28 = rnd();
        v29 = W_char_skill_roll(c, v27, 1000, 1000 * v28 / 0x8000);
        v30 = v27[44];
        (*v43__f) = v29;
        a2 = v30;
        v31 = W_char_mag_sum(c);
        v32 = v27[186];
        v33 = v27[45] + v31;
        (*v41__f) = 0;
        (*v39__f) = 0;
        a6 = 0;
        if ( v32 == 0 && v27[183] == 0 && a2 == 0 )
        {
          v34 = F32(c, 0x750);
          v33 = F32(v34, 296);
          a2 = F32(v34, 292);
          (*v41__f) = F32(v34, 300);
          (*v39__f) = F32(v34, 304);
          a6 = F32(v34, 308);
        }
        W_body_hit_spawn(
          c,
          (int)c,
          v8,
          800,
          400,
          4000,
          4000,
          0,
          a3,
          (*v44__f),
          a2,
          v33,
          0,
          3,
          (*v43__f),
          1,
          v27[184],
          F32(v8, 96),
          0,
          v27[189],
          F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228),
          (*v41__f),
          (*v39__f),
          (int)a6);
        if ( F32(c, 0xc7f4) != 0 )
        {
          W_body_hit_spawn(
            c,
            (int)c,
            v8,
            F32(c, 0xc7f8) + 800,
            400,
            4000,
            4000,
            0,
            a3,
            (*v44__f),
            a2,
            v33,
            0,
            3,
            (*v43__f),
            1,
            v27[184],
            F32(v8, 96),
            0,
            v27[189],
            F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228),
            (*v41__f),
            (*v39__f),
            (int)a6);
          W_body_hit_spawn(
            c,
            (int)c,
            v8,
            800 - F32(c, 0xc7f8),
            400,
            4000,
            4000,
            0,
            a3,
            (*v44__f),
            a2,
            v33,
            0,
            3,
            (*v43__f),
            1,
            v27[184],
            F32(v8, 96),
            0,
            v27[189],
            F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228),
            (*v41__f),
            (*v39__f),
            (int)a6);
        }
      }
      if ( F32(v8, 92) > 24 || F16(v8, 90) >= (*v46__f) )
      {
        ++F16(v8, 88);
        F32(v8, 92) = 0;
      }
      if ( F32(v8, 92) % 3 == 0 )
        W_slash_trail(c, v8, a3, 16, 10);
      break;
    case 4:
      v35 = (_DWORD *)a3;
      v36 = F32(v8, 92) + 1;
      (*v45__f) = 16000;
      F32(v8, 92) = v36;
      (*v42__f) = -48000;
      W_char_anim_select(c, v8, v35, (int)a6);
      if ( F32(v8, 92) > 30 )
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
      }
      break;
    default:
      break;
  }
  v23 = W_speed_ramp(F32(v8, 40), 3200, -1600, (*v45__f));
  (*v37__f) = F32(v8, 24);
  F32(v8, 40) = v23;
  v24 = W_speed_ramp((*v37__f), 4000, -1600, (*v42__f));
  v25 = a3;
  F32(v8, 24) = v24;
  W_char_drift_move(c, v8, v25, a5, 0, 0, 1);
  return 1;
}

/* 0x4512d0: int __thiscall sub_4512D0(cls_4022A0 *this, int a2, int a3, _DWORD *a4, int a5, int a6, int a7) */
int32_t ih_4512d0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    uint16_t * v8;
    void * v9;
    intptr_t v10;
    int32_t * v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t * v15;
    intptr_t v16;
    intptr_t v17;
    int32_t * v18;
    intptr_t * v19;
    intptr_t v20;
    intptr_t result;
    intptr_t v22;
    intptr_t v23;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  (*v25__f) = 0;
  (*v24__f) = 1;
  v8 = D(0x663d20);
  v9 = D(0x6636d0);
  if ( F32(c, 0x1d4) == 51052 )
  {
    (*v24__f) = 0;
    v8 = D(0x664b58);
    v9 = D(0x664508);
  }
  v10 = a2;
  if ( F16(a2, 72) == 29 )
  {
    v22 = a3;
    *a4 = 2;
    if ( W_slash_step(c, v10, v22, (int)v9, 1600, -1600, 0, a6) != 0 && F16(v10, 88) == 3 )
    {
      v23 = rnd();
      W_sfx_play_at(c, v22, 149, 0, 1000 * v23 / 0x8000, 0);
    }
    if ( F16(v10, 90) == 0 && F16(v10, 88) == 3 )
      W_char_attack_hits(c, v10, 1600, -1600, 6400, 3200, 0, v22, F32(v10, 76), 1000, 3);
    return (*v25__f);
  }
  if ( F16(a2, 72) != 30 )
    return (*v25__f);
  v11 = a4;
  F8(a2, 124) = 1;
  v12 = 0;
  *v11 = 3;
  ++F32(v10, 92);
  switch ( F16(v10, 88) )
  {
    case 0:
      if ( F32(v10, 108) != v8 )
      {
        F32(v10, 108) = v8;
        F8(v10, 116) = 0;
        F16(v10, 114) = 0;
        F16(v10, 112) = 0;
      }
      v12 = 1;
      F16(v10, 88) = 1;
      goto LABEL_9;
    case 1:
LABEL_9:
      if ( F8(v10, 116) == 0 )
        goto LABEL_23;
      W_body_set_clip(v10, (int)(v8 + 202));
      F16(v10, 88) = 2;
      F32(v10, 24) = -40000;
      F32(v10, 40) = 48000;
      F32(v10, 92) = 0;
      goto LABEL_24;
    case 2:
      if ( F32(v10, 24) > 0 )
      {
        if ( v8[437] != 0 )
          W_body_set_clip(v10, (int)(v8 + 404));
        F16(v10, 88) = 3;
        v12 = 1;
      }
      goto LABEL_15;
    case 3:
LABEL_15:
      if ( F32(v10, 92) <= 24 )
      {
        if ( W_char_drift(c, v10, a3, a5, 48000, 4000, -1000, 48000, 3000, -1000, 1, 0, 0) != 0 )
        {
LABEL_23:
          if ( v12 == 0 )
            goto LABEL_36;
        }
        else
        {
          F16(v10, 88) = 4;
LABEL_19:
          F32(v10, 40) = -32000;
          F32(v10, 24) = -40000;
          W_body_set_clip(v10, (int)(v8 + 808));
          F16(v10, 88) = 6;
        }
      }
      else
      {
        F16(v10, 88) = 5;
        W_body_set_clip(v10, (int)(v8 + 606));
      }
LABEL_24:
      if ( F16(v10, 88) == 1 )
      {
        if ( (*v24__f) != 0 )
        {
          v18 = (_DWORD *)F32(c, 0x40);
          if ( a3 == 0 )
          {
            v19 = W_vpool_take(SYS32(1468), 0);
            if ( v19 != 0 )
            {
              v20 = F32(v19, 0);
              if ( W_sfx_place(
                     c,
                     F32(CFG(), 360),
                     0,
                     0,
                     v18,
                     (int *)&a4,
                     &a6) != 0 )
              {
                g_eng.dsb_volume_5bb870(v20, (int)a4);
                g_eng.dsb_pan_5bb880(v20, a6);
                g_eng.dsb_freq_5bb890(v20, 0);
                v17 = v20;
                goto LABEL_35;
              }
            }
          }
        }
      }
      else if ( F16(v10, 88) == 2 )
      {
        v13 = rnd();
        a4 = (_DWORD *)F32(c, 0x40);
        v14 = 1000 * v13 / 0x8000 - 1000;
        if ( a3 == 0 )
        {
          v15 = W_vpool_take(SYS32(1464), 0);
          if ( v15 != 0 )
          {
            v16 = F32(v15, 0);
            if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, a4, &a6, &a2) != 0 )
            {
              g_eng.dsb_volume_5bb870(v16, a6);
              g_eng.dsb_pan_5bb880(v16, a2);
              g_eng.dsb_freq_5bb890(v16, v14);
              v17 = v16;
LABEL_35:
              g_eng.dsb_play_5bb8c0(v17);
            }
          }
        }
      }
LABEL_36:
      if ( F16(v10, 88) < 2u )
        return (*v25__f);
      if ( F16(v10, 88) > 3u )
        return (*v25__f);
      (*v25__f) = 1;
      if ( F16(v10, 90) != 0
        || W_char_attack_hits(c, v10, 0, -1600, 4000, 4000, 0, a3, F32(v10, 76), 1000, 3) == 0 )
      {
        return (*v25__f);
      }
      F16(v10, 88) = 4;
      result = 1;
      break;
    case 4:
      goto LABEL_19;
    case 5:
    case 6:
      if ( F8(v10, 116) == 0 && a6 == 0 )
        goto LABEL_36;
      W_body_action_reset(v10);
      goto LABEL_24;
    default:
      goto LABEL_36;
  }
  return result;
}

/* 0x4466b0: void __thiscall sub_4466B0(cls_4022A0 *this, int a2, int a3, int a4) */
void ih_4466b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    int32_t * v9;
    intptr_t i;
    int32_t * v11;
    intptr_t j;
    void * v13;

  if ( a4 == 0 || F32(c, 0x15904) != 0 )
  {
    if ( a3 != 0 )
    {
      F8(a2, 0) = 0;
    }
    else
    {
      v6 = F32(c, 0xc798);
      if ( v6 != 0 && F32(v6, 88372) != 0 )
      {
        v7 = F32(v6, 51108);
        if ( v7 != 0 )
        {
          F32(v7, 51096) = 0;
          F32(v6, 51108) = 0;
        }
      }
      v8 = F32(c, 0xc7a4);
      if ( v8 != 0 && F32(c, 0x15934) != 0 )
      {
        F32(v8, 51096) = 0;
        F32(c, 0xc7a4) = 0;
      }
      v9 = &F32(c, 0xc824);
      for ( i = 2; i != 0; --i )
      {
        if ( *v9 != 0 )
        {
          W_entity_release(*v9, /*edx*/ 1);
          *v9 = 0;
        }
        ++v9;
      }
      v11 = &F32(c, 0xc7fc);
      for ( j = 10; j != 0; --j )
      {
        if ( *v11 != 0 )
        {
          W_entity_release(*v11, /*edx*/ 0);
          *v11 = 0;
        }
        ++v11;
      }
      W_entity_release((int)c, /*edx*/ 0);
      v13 = (void *)F32(c, 0x15b54);
      if ( v13 != 0 )
      {
        W_voice_set_free(F32(c, 0x15b54));
        g_eng.op_delete((void *)v13);
        F32(c, 0x15b54) = 0;
      }
    }
  }
  else
  {
    F16(a2, 88) = 0;
    F32(a2, 92) = 0;
    F32(a2, 120) = 0;
    F16(a2, 84) = 0;
    F32(a2, 76) = 0;
    F32(a2, 80) = 0;
    F16(a2, 90) = 0;
    F16(a2, 100) = 0;
    F16(a2, 72) = 122;
    F16(a2, 86) = 800;
  }
}

/* 0x467260: int __thiscall sub_467260(int this, int a3, int a4, int a5) */
int32_t ih_467260(wl_char *self, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    uint16_t i;
    uint16_t v7;
    uint16_t v8;
    intptr_t v9;
    intptr_t v10;

  result = a4;
  if ( a4 == 0 && F32(c, 88784) == 8 && a5 != 0 )
  {
    for ( i = 0; i < F16(c, 428); ++i )
    {
      result = c + 20 * i + 268;
      if ( F32(result, 0) == 100156 )
        F32(result, 0) = 0;
    }
    v7 = F16(c, 428);
    v8 = 0;
    if ( v7 != 0 )
    {
      while ( 1 )
      {
        result = 5 * v8;
        if ( F32(c + 20 * v8, 268) == 0 )
          break;
        if ( ++v8 >= v7 )
          return result;
      }
      v9 = 5 * v8;
      v10 = v9 + 70;
      result = c + 4 * v9;
      F32(result, 268) = 100156;
      F16(result, 272) = 12;
      F16(result, 274) = 0;
      F16(result, 276) = 0;
      F16(result, 278) = 0;
      F32((c + 4 * v10), 0) = 0;
      F32(result, 284) = 0;
    }
  }
  return result;
}

/* 0x5b3b00: int __thiscall sub_5B3B00(cls_5B3B00 *this, int a2, int a3, _WORD *a4, int a5, int a6, __int16 a7) */
int32_t item_effect(wl_char *self, int32_t a2, int32_t a3, uint16_t * a4, int32_t a5, int32_t a6, int16_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    intptr_t v9;
    intptr_t v10;
    int32_t * v11;
    int32_t * v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    char * v20;
    uint16_t v21;
    uint16_t v22;
    intptr_t v23;
    int v24;
    _Alignas(8) uint8_t frame_[0x214 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x14);   /* [512] at ebp-0x200 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x214 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x210 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x208 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x204 */

  v7 = c;
  (*v25__f) = 0;
  if ( a3 != 0 )
    return 0;
  v9 = rnd();
  v10 = W_stat_jitter(
          (F32(*(_DWORD *)&GAP((v7 + (3) * 0x1e0), 0x0, 432), 108)
         + F32(*(_DWORD *)&GAP((v7 + (3) * 0x1e0), 0x0, 432), 124)
         + F32(*(_DWORD *)&GAP((v7 + (3) * 0x1e0), 0x0, 432), 148)
         + *(_DWORD *)&GAP((v7 + (106) * 0x1e0), 0x0, 292))
        / 2,
          1000,
          1000 * v9 / 0x8000);
  (*v29__f) = v10;
  switch ( F32(a5, 184) )
  {
    case 1:
      goto LABEL_34;
    case 2:
      (*v26__f) = 4448;
      (*v28__f) = 32;
      break;
    case 3:
    case 4:
      W_beh_48be60(v7, F16(a5, 708), 1);
LABEL_34:
      (*v25__f) = ih_5b28a0(
              a4,
              F32(a5, 176),
              F32(a5, 708),
              F32(a5, 712),
              F32(a5, 716),
              F32(a5, 720),
              v10,
              1u,
              1,
              0);
      if ( a6 != 0 )
        W_entity_fx_add(a4, a6, 1, a7, 0, 0, 0, 0);
      return (*v25__f);
    case 5:
      v11 = F32(W, 0x1030 + (20));
      if ( F32(a5, 176) != 20 )
        return 0;
      v11[4] = 1;
      v12 = (_DWORD *)F32(W, 0x2784);
      v13 = v12[4150];
      v14 = v12[4149];
      v15 = v12[4148];
      v11[577] = v14;
      v11[4] = 1;
      v11[579] = 1;
      v11[576] = v15;
      v11[578] = v13;
      return 1;
    default:
      return (*v25__f);
  }
  while ( 1 )
  {
    v16 = F32(W, 0x0 + ((*v26__f)));
    if ( F32(v16, 464) != 0 )
    {
      if ( W_entity_near(v16, 48000, 28000, (int)v7, 0) )
      {
        if ( W_beh_47ae00(
               v16,
               (int)v7,
               F32(a5, 176),
               F32(a5, 708),
               F32(a5, 712),
               F32(a5, 716),
               F32(a5, 720),
               0,
               0,
               0,
               0,
               0,
               0) != 0 )
        {
          if ( ih_5b28a0(
                 v16,
                 F32(a5, 176),
                 F32(a5, 708),
                 F32(a5, 712),
                 F32(a5, 716),
                 F32(a5, 720),
                 (*v29__f),
                 1u,
                 1,
                 0) != 0 )
            (*v25__f) = 1;
          if ( a6 != 0 )
          {
            W_entity_fx_remove(v16, a6);
            v21 = F16(v16, 428);
            v22 = 0;
            if ( v21 != 0 )
            {
              while ( F32(v16 + 20 * v22, 268) != 0 )
              {
                if ( ++v22 >= v21 )
                  goto LABEL_31;
              }
              v23 = v16 + 20 * v22;
              F32(v23, 268) = a6;
              F16(v23, 272) = a7;
              F16(v23, 274) = 0;
              F16(v23, 276) = 0;
              F16(v23, 278) = 0;
              F32((v16 + 4 * (5 * v22 + 70)), 0) = 0;
              F32(v23, 284) = 0;
            }
          }
        }
      }
      else
      {
        v17 = F32(a5, 176);
        if ( F32(v16, 88328) == 0
          && (c == 0 || W_faction_relation(F32(v16, 476), F32(c, 0x1dc)) == 2)
          && (F16(F32(v16, 1872), 1098) == 0 || v17 == 10)
          && F32(v16, 51092) != 0 )
        {
          g_eng.h5bf3ee(Buffer, "%s is outside the range of your magic!", *(const char **)(v16 + 1872));
          v18 = 0;
          v19 = F32(W, 0x1024);
          while ( F32(W, 0x0 + (516 * v19 + 4)) != 0 )
          {
            v19 = (v19 + 1) % 8;
            if ( (uint16_t)++v18 >= 8u )
              goto LABEL_31;
          }
          v20 = (W + 0x0 + (516 * v19));
          F32(v20, 4 * 1) = 1;
          strcpy(v20 + 8, Buffer);
        }
      }
    }
LABEL_31:
    v24 = (*v28__f) == 1;
    (*v26__f) += 4;
    --(*v28__f);
    if ( v24 )
      return (*v25__f);
    v7 = c;
  }
}

/* 0x5b28a0: int __thiscall sub_5B28A0( cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, unsigned int a8, int a9, char *Buffer) */
int32_t ih_5b28a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, char * Buffer)
{
    intptr_t c = (intptr_t)self;
    int32_t * v11;
    intptr_t v12;
    int32_t * v13;
    intptr_t * v14;
    intptr_t v15;
    intptr_t result;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    int32_t * v23;
    intptr_t * v24;
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
    int32_t * v35;
    intptr_t * v36;
    intptr_t v37;
    int32_t * v38;
    intptr_t * v39;
    intptr_t v40;
    uint16_t * v41;
    intptr_t v42;
    intptr_t v43;
    int32_t * v44;
    intptr_t * v45;
    intptr_t v46;
    int32_t * v47;
    intptr_t * v48;
    intptr_t v49;
    _Alignas(8) uint8_t frame_[0x488 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v54 = (char *)(frame_ + 0x10);   /* [40] at ebp-0x478 */
    char *v55 = (char *)(frame_ + 0x38);   /* [80] at ebp-0x450 */
    char *v56 = (char *)(frame_ + 0x88);   /* [512] at ebp-0x400 */
    char *Format = (char *)(frame_ + 0x288);   /* [512] at ebp-0x200 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x488 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x484 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x480 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x47c */

  v11 = (_DWORD *)F32(c, 0x40);
  (*v52__f) = 0;
  (*v50__f) = 0;
  switch ( a2 )
  {
    case 9:
      v17 = rnd();
      W_char_add_hp(c, a4 * v17 / 0x8000 + a3 + a7 * a5 / 1000, &(*v52__f), 0);
      if ( a8 != 0 )
      {
        v18 = v11[11];
        v19 = v11[2] + v11[4];
        (*v51__f) = v11[3] + 3200 * rnd() / 0x8000 + v11[1] - 1600;
        g_eng.h5bf3ee(v54, "%d", (*v52__f));
        v20 = W_particle_spawn(c, (int)c, 100042, v11, (*v51__f), v19, v18, 0, 0, 0, 0, 0, 0, 20);
        v21 = v20;
        if ( v20 != 0 )
        {
          v22 = F32(v20, 64);
          W_dmg_glyphs(v54, v20 + 512, (_WORD *)(v20 + 592));
          F16(v21, 504) = 7;
          F32(v21, 508) = 9;
          F16(v21, 600) = 60;
          F32(v22, 24) = -48000;
        }
      }
      if ( (*v52__f) <= 0 )
        goto LABEL_83;
      if ( a9 != 0 )
      {
        v23 = (_DWORD *)F32(c, 0x40);
        v24 = W_vpool_take(SYS32(1208), 0);
        if ( v24 != 0 )
        {
          v25 = F32(v24, 0);
          if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v23, &(*v51__f), &(*v50__f)) != 0 )
          {
            g_eng.dsb_volume_5bb870(v25, (*v51__f));
            g_eng.dsb_pan_5bb880(v25, (*v50__f));
            g_eng.dsb_freq_5bb890(v25, 0);
            g_eng.dsb_play_5bb8c0(v25);
          }
        }
      }
      (*v50__f) = 1;
      if ( Buffer == 0 )
        goto LABEL_83;
      g_eng.h5bf3ee(Buffer, "%s recovered %d HP.", (const char *)F32(c, 0x750), (*v52__f));
      return (*v50__f);
    case 10:
      v27 = F32(c, 0x750);
      if ( F16(v27, 1098) != 0 )
      {
        F16(v27, 1098) = 0;
        strcpy(Format, D(0x85530c));
        v28 = F32(c, 0x15910);
        F32(c, 0xfc) = v28;
        F32(c, 0x100) = v28;
        if ( F32(c, 0xc794) != 0 && W_char_room_logs(c) != 0 )
        {
          g_eng.h5bf3ee(v56, Format, F32(c, 0x750));
          if ( a8 != 0 )
            W_world_log_line(W, v56);
          if ( Buffer != 0 )
            strcpy(Buffer, v56);
        }
        (*v50__f) = 1;
      }
      v29 = rnd();
      W_char_add_hp(c, a4 * v29 / 0x8000 + a3 + a7 * a5 / 1000, &(*v52__f), 0);
      if ( Buffer != 0 && (*v50__f) == 0 )
        g_eng.h5bf3ee(Buffer, "%s recovered %d HP.", (const char *)F32(c, 0x750), (*v52__f));
      if ( a8 != 0 )
      {
        v30 = v11[11];
        v31 = v11[2] + v11[4];
        (*v51__f) = v11[3] + 3200 * rnd() / 0x8000 + v11[1] - 1600;
        g_eng.h5bf3ee(v54, "%d", (*v52__f));
        v32 = W_particle_spawn(c, (int)c, 100042, v11, (*v51__f), v31, v30, 0, 0, 0, 0, 0, 0, 20);
        v33 = v32;
        if ( v32 != 0 )
        {
          v34 = F32(v32, 64);
          W_dmg_glyphs(v54, v32 + 512, (_WORD *)(v32 + 592));
          F16(v33, 504) = 7;
          F32(v33, 508) = 9;
          F16(v33, 600) = 60;
          F32(v34, 24) = -48000;
        }
      }
      if ( (*v52__f) <= 0 )
      {
        if ( (*v50__f) == 0 )
          goto LABEL_83;
      }
      else
      {
        (*v50__f) = 1;
      }
      if ( a9 == 0 )
        goto LABEL_83;
      v35 = (_DWORD *)F32(c, 0x40);
      v36 = W_vpool_take(SYS32(1208), 0);
      if ( v36 != 0 )
      {
        v37 = F32(v36, 0);
        if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v35, &(*v51__f), &(*v53__f)) != 0 )
        {
          g_eng.dsb_volume_5bb870(v37, (*v51__f));
          g_eng.dsb_pan_5bb880(v37, (*v53__f));
          g_eng.dsb_freq_5bb890(v37, 0);
          g_eng.dsb_play_5bb8c0(v37);
        }
      }
      v38 = (_DWORD *)F32(c, 0x40);
      v39 = W_vpool_take(SYS32(1212), 0);
      if ( v39 == 0 )
        goto LABEL_83;
      v40 = F32(v39, 0);
      if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v38, &(*v53__f), &(*v51__f)) == 0 )
        goto LABEL_83;
      g_eng.dsb_volume_5bb870(v40, (*v53__f));
      g_eng.dsb_pan_5bb880(v40, (*v51__f));
      g_eng.dsb_freq_5bb890(v40, 0);
      g_eng.dsb_play_5bb8c0(v40);
      return (*v50__f);
    case 11:
      v41 = (_WORD *)(F32(c, 0x750) + 2 * (uint16_t)a3 + 1096);
      if ( *v41 == 0 )
        goto LABEL_83;
      v42 = 0;
      *v41 = 0;
      switch ( (int16_t)a3 )
      {
        case 1:
          strcpy(v56, D(0x85530c));
          v43 = F32(c, 0x15910);
          F32(c, 0xfc) = v43;
          F32(c, 0x100) = v43;
          goto LABEL_60;
        case 2:
          strcpy(v56, D(0x855340));
          goto LABEL_60;
        case 3:
          strcpy(v56, D(0x855324));
          goto LABEL_60;
        case 5:
          strcpy(v56, D(0x855354));
          goto LABEL_60;
        case 6:
          strcpy(v56, D(0x855388));
          goto LABEL_60;
        case 7:
          strcpy(v56, D(0x855360));
LABEL_60:
          v42 = 1;
          break;
        default:
          break;
      }
      if ( F32(c, 0xc794) != 0 && v42 != 0 && W_char_room_logs(c) != 0 )
      {
        g_eng.h5bf3ee(Format, v56, F32(c, 0x750));
        if ( a8 != 0 )
          W_world_log_line(W, Format);
        if ( Buffer != 0 )
          strcpy(Buffer, Format);
      }
      if ( a9 != 0 )
      {
        v44 = (_DWORD *)F32(c, 0x40);
        v45 = W_vpool_take(SYS32(1208), 0);
        if ( v45 != 0 )
        {
          v46 = F32(v45, 0);
          if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v44, &(*v53__f), &(*v51__f)) != 0 )
          {
            g_eng.dsb_volume_5bb870(v46, (*v53__f));
            g_eng.dsb_pan_5bb880(v46, (*v51__f));
            g_eng.dsb_freq_5bb890(v46, 0);
            g_eng.dsb_play_5bb8c0(v46);
          }
        }
      }
      goto LABEL_82;
    case 12:
      if ( F16(F32(c, 0x750), 1106) != 0 )
      {
        W_char_status_decay(c, 5u, 0, 1, a8, Buffer);
        if ( a9 != 0 )
        {
          v47 = (_DWORD *)F32(c, 0x40);
          v48 = W_vpool_take(SYS32(1208), 0);
          if ( v48 != 0 )
          {
            v49 = F32(v48, 0);
            if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v47, &(*v53__f), &(*v51__f)) != 0 )
            {
              g_eng.dsb_volume_5bb870(v49, (*v53__f));
              g_eng.dsb_pan_5bb880(v49, (*v51__f));
              g_eng.dsb_freq_5bb890(v49, 0);
              g_eng.dsb_play_5bb8c0(v49);
            }
          }
        }
        (*v50__f) = 1;
      }
      if ( F16(F32(c, 0x750), 1102) == 0 )
        goto LABEL_83;
      W_char_status_decay(c, 3u, 0, 1, a8, Buffer);
      if ( a9 != 0 )
        W_sfx_play_at(c, 0, 85, 0, 0, 0);
LABEL_82:
      (*v50__f) = 1;
LABEL_83:
      result = (*v50__f);
      break;
    case 13:
    case 15:
    case 16:
      W_char_status_inflict(c, a3, a4 + a7 * a5 / 1000);
      goto LABEL_82;
    case 18:
      v12 = rnd();
      W_char_add_mp(c, a4 * v12 / 0x8000 + a3 + a7 * a5 / 1000, &(*v52__f), 0);
      if ( (*v52__f) <= 0 )
        goto LABEL_83;
      if ( a9 != 0 )
      {
        v13 = (_DWORD *)F32(c, 0x40);
        v14 = W_vpool_take(SYS32(1208), 0);
        if ( v14 != 0 )
        {
          v15 = F32(v14, 0);
          if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v13, &(*v50__f), &(*v51__f)) != 0 )
          {
            g_eng.dsb_volume_5bb870(v15, (*v50__f));
            g_eng.dsb_pan_5bb880(v15, (*v51__f));
            g_eng.dsb_freq_5bb890(v15, 0);
            g_eng.dsb_play_5bb8c0(v15);
          }
        }
      }
      (*v50__f) = 1;
      if ( Buffer == 0 )
        goto LABEL_83;
      g_eng.h5bf3ee(Buffer, "%s recovered %d MP.", (const char *)F32(c, 0x750), (*v52__f));
      result = (*v50__f);
      break;
    case 19:
      v26 = a4 + a5 * rnd() / 0x8000;
      (*v52__f) = v26;
      switch ( a3 )
      {
        case 1:
          strcpy(v55, D(0x854c60));
          F32(F32(c, 0x750), 140) += v26;
          break;
        case 2:
          strcpy(v55, D(0x854c58));
          F32(F32(c, 0x750), 144) += v26;
          break;
        case 3:
          strcpy(v55, D(0x854c50));
          F32(F32(c, 0x750), 148) += v26;
          break;
        case 4:
          strcpy(v55, D(0x854c48));
          F32(F32(c, 0x750), 152) += v26;
          break;
        case 5:
          strcpy(v55, D(0x854c40));
          F32(F32(c, 0x750), 156) += v26;
          break;
        case 6:
          strcpy(v55, D(0x854c38));
          F32(F32(c, 0x750), 160) += v26;
          break;
        default:
          goto LABEL_83;
      }
      (*v50__f) = 1;
      if ( Buffer == 0 )
        goto LABEL_83;
      g_eng.h5bf3ee(Buffer, "%s's %s increased by %d.", (const char *)F32(c, 0x750), v55, (*v52__f));
      result = (*v50__f);
      break;
    default:
      goto LABEL_83;
  }
  return result;
}

/* 0x456c60: int __stdcall sub_456C60(int a1, int a2, int a3, int a4, int a5, int a6) */
int32_t ih_456c60(intptr_t ecx_, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{

    intptr_t v6;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v8 = (intptr_t *)(frame_ + 0x0);   /* [2] at ebp-0x20 */
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x18 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x14 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x10 */
    int16_t *const v12__f = (int16_t *)(frame_ + 0x14);   /* ebp-0xc */
    int16_t *const v13__f = (int16_t *)(frame_ + 0x16);   /* ebp-0xa */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    int16_t *const v15__f = (int16_t *)(frame_ + 0x1c);   /* ebp-0x4 */

  v8[1] = 0;
  v8[0] = 0;
  (*v9__f) = 0;
  (*v10__f) = 0;
  (*v12__f) = 0;
  (*v11__f) = 0;
  (*v13__f) = 0;
  (*v14__f) = 0;
  (*v15__f) = 0;
  switch ( a4 )
  {
    case 1:
      v6 = (F32(a1, 44) != 1) + 1;
      goto LABEL_6;
    case 2:
      v6 = 2 - (F32(a1, 44) != 1);
      goto LABEL_6;
    case 5:
      v6 = (F32(a1, 44) != 1) + 5;
      goto LABEL_6;
    case 6:
      v6 = 6 - (F32(a1, 44) != 1);
LABEL_6:
      v8[0] = v6;
      break;
    default:
      break;
  }
  if ( a5 == 7 )
    (*v9__f) = (F16(a1, 56) == 3) + 7;
  return W_char_physics_step(ecx_, v8, a1, a2, 0, a3, a6);
}
/* unported callees: none */
