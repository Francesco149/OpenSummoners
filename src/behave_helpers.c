/* behave_helpers.c — the behaviour helpers of the AI tick 0x47b990 (FUN_0047b990 char_behave).  See behave_helpers.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (the decompile's control flow kept verbatim: labels, gotos across
 * the cases; IDA's local types, fields through width-typed accessors), then finished by hand against the asm.
 * IDA-__fastcall functions that never read edx are thiscall here; IDA-__stdcall ones that use their caller's ecx
 * take it as a leading ecx_ (always the caller's `this`). */
#include "behave_helpers.h"

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
#include "behave_ops.h"
#include "char_ai.h"
#include "char_attack.h"
#include "char_def.h"
#include "char_integrate.h"
#include "char_physics.h"
#include "char_state.h"
#include "entity_move.h"
#include "gimmick.h"
#include "npc_ai.h"
#include "scene_cast.h"
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

static inline intptr_t W_actor_by_handle(intptr_t a0, ...) { return (intptr_t)actor_by_handle((int32_t)a0); }
static inline intptr_t W_behave_set_state(intptr_t a0, intptr_t a1, ...) { return (intptr_t)behave_set_state((wl_char *)a0, (int16_t)a1); }
static inline intptr_t W_behave_step_ok(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)behave_step_ok((wl_char *)a0, (const wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_body_can_act(intptr_t a0, ...) { return (intptr_t)body_can_act((const wl_body *)a0); }
static inline intptr_t W_body_frame_box(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)body_frame_box((uint16_t)a0, (const wl_body *)a1, (int32_t *)a2); }
static inline intptr_t W_body_hitbox_hit(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)body_hitbox_hit((const wl_char *)a0, (const wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4); }
static inline intptr_t W_body_near_x(intptr_t a0, intptr_t a1, ...) { return (intptr_t)body_near_x((const wl_body *)a0, (int32_t)a1); }
static inline intptr_t W_body_probe_rect(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { body_probe_rect((const wl_body *)a0, (const int32_t *)a1, (int32_t *)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_atk_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)char_atk_roll((const wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_char_hazard_probe(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_hazard_probe((wl_char *)a0, (uint16_t)a1, (const wl_body *)a2, (int32_t *)a3); }
static inline intptr_t W_char_hit_fresh(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_hit_fresh((const wl_char *)a0, (const int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_char_hit_kind_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_hit_kind_ok((const wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_char_integrate(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)char_integrate((wl_char *)a0, (wl_cmd *)a1, (wl_body *)a2, (wl_body *)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_char_mag_sum(intptr_t a0, ...) { return (intptr_t)char_mag_sum((const wl_char *)a0); }
static inline intptr_t W_char_part_top(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_part_top((const wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_char_room_logs(intptr_t a0, ...) { return (intptr_t)char_room_logs((const wl_char *)a0); }
static inline intptr_t W_char_skill_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_skill_ok((wl_char *)a0, (const wl_skill *)a1, (int32_t)a2); }
static inline intptr_t W_char_targetable(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)char_targetable((const wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_cmd_clear(intptr_t a0, ...) { cmd_clear((wl_cmd *)a0); return 0; }
static inline intptr_t W_entity_blocks(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)entity_blocks((const wl_entity *)a0, (const wl_entity *)a1, (uint16_t)a2, (int32_t)a3, (int16_t)a4, (int32_t)a5); }
static inline intptr_t W_entity_ground_probe(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)entity_ground_probe((wl_entity *)a0, (wl_body *)a1, (uint16_t)a2); }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_entity_near(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)entity_near((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (const wl_entity *)a3, (int32_t)a4); }
static inline intptr_t W_entity_rect_hit(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)entity_rect_hit((const wl_entity *)a0, (const int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_facing_back(intptr_t a0, ...) { return (intptr_t)facing_back((int32_t)a0); }
static inline intptr_t W_faction_rel(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)faction_rel((const wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_faction_relation(intptr_t a0, intptr_t a1, ...) { return (intptr_t)faction_relation((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_grid_floor_below(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_floor_below((const wl_body *)a0, (int32_t *)a1); }
static inline intptr_t W_npc_floor_ahead(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)npc_floor_ahead((wl_char *)a0, (const wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_npc_navigate(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)npc_navigate((wl_char *)a0, (int32_t)a1, (int32_t)a2, (wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_npc_probe_free(intptr_t a0, intptr_t a1, intptr_t a2, ...) { npc_probe_free((wl_char *)a0, (wl_body *)a1, (uint16_t)a2); return 0; }
static inline intptr_t W_part_bottom(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_bottom((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_center_x(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_x((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_facing(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_facing((const wl_entity *)a0, (int32_t)a1); }
static inline intptr_t W_part_left(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_left((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_right(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_right((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_party_find(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)party_find((const wl_char *)a0, (int32_t)a1, (int16_t)a2, (uint16_t *)a3, (uint16_t)a4); }
static inline intptr_t W_path_dfs(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)path_dfs((path_finder *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (uint16_t *)a4, (int32_t)a5, (int32_t)a6, (uint16_t *)a7); }
static inline intptr_t W_path_search(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)path_search((path_finder *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (uint8_t *)a9, (uint16_t *)a10, (path_rec *)a11, (int32_t)a12); }
static inline intptr_t W_skill_pose_ok(intptr_t a0, intptr_t a1, ...) { return (intptr_t)skill_pose_ok((const wl_skill *)a0, (const wl_body *)a1); }
static inline intptr_t W_stat_jitter(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)stat_jitter((int32_t)a0, (int32_t)a1, (int32_t)a2); }

int32_t beh_441f50(wl_char *self, int32_t * a2, int32_t a3);
int32_t beh_47adb0(wl_char *self, int32_t a1, int32_t * a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t * a7);
int32_t beh_47d790(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t beh_47da60(wl_char *self, int32_t * a2);
int32_t beh_47de50(int32_t * a1, int32_t a2);
int32_t beh_47e010(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t beh_47e7c0(int32_t a1, uint16_t a2, int32_t * a3, int32_t a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8);
int32_t beh_47eae0(wl_char *self, int32_t * a2, int32_t * a3, int16_t * a4, int32_t a5);
int32_t beh_47ecd0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint16_t a8, int32_t * a9, int32_t * a10);
int32_t beh_47f670(wl_char *self, uint16_t a2, int32_t a3);
int32_t beh_47f9d0(wl_char *self, uint16_t a2, int32_t a3);
int32_t beh_481310(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t beh_481ac0(wl_char *self, int32_t * a3, int32_t a4);
int32_t beh_482240(wl_char *self, int32_t a2);
int32_t beh_482380(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, uint16_t * a6, int32_t a7);
int32_t beh_482460(wl_char *self, int32_t a2);
int32_t beh_482880(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t beh_482ac0(wl_char *self, int32_t * a2);
void beh_482e10(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_4834f0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_483b50(wl_char *self, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6, uint16_t * a7, int32_t a8);
int32_t beh_483de0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_483fb0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5);
int32_t beh_484530(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t beh_484ea0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_485350(wl_char *self, int16_t a2, int16_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_4854e0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_485990(wl_char *self, int32_t * a2, int32_t a3);
int32_t beh_442490(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t * a10);
int32_t beh_47ae00(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8, int32_t * a9, int32_t a10, int32_t a11, int32_t a12, int32_t * a13);
int32_t beh_47f610(int32_t a1, uint16_t a2, int32_t a3);
int32_t beh_47fd40(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t beh_480b30(wl_char *self, int32_t a2, int32_t a3, uint16_t a4, int32_t a5, int32_t a6, int32_t * a7);
int32_t beh_48bdd0(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_48be60(wl_char *self, uint16_t a2, int32_t a3);
int32_t beh_479e40(int32_t a1, int16_t a2, int16_t a3, int16_t a4, int32_t a5);
int32_t beh_481790(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8);
int32_t beh_481a20(wl_char *self, uint16_t a2, uint16_t a3);
int32_t beh_479e10(int32_t a1, int16_t a2, int16_t a3, int32_t a4);
int32_t beh_47a970(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t i, int32_t a8);
int32_t beh_4826b0(wl_char *self, int32_t * a2);
int32_t beh_482700(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t beh_482760(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_483040(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t beh_484830(wl_char *self, int32_t a2, int32_t a3);
int32_t beh_484d60(wl_char *self, int32_t a2);
int32_t beh_485330(wl_char *self);
int32_t beh_483920(int32_t a1, int32_t * a2, int32_t * a3, uint16_t a4);
int32_t beh_4848a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t beh_442640(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_4843c0(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4, uint16_t a5, int32_t a6);
int32_t beh_442710(wl_char *self, uint16_t a2);
int32_t beh_485cb0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t beh_47b870(int32_t a1, int32_t a2);
int32_t beh_48b920(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t beh_5b4200(wl_char *self);
int32_t beh_480260(wl_char *self, int32_t * a2, uint16_t a3, int32_t a4, int32_t * a5);
int32_t beh_480580(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, uint16_t a6, int32_t a7, int32_t a8);
int32_t beh_4832e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t beh_484bc0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5);

/* 0x441f50: __int16 __thiscall sub_441F50(cls_4022A0 *this, _DWORD *a2, int a3) */
int32_t beh_441f50(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v4;
    int32_t * v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    int v10;
    intptr_t v11;
    intptr_t v12;
    int64_t v13;
    uint16_t v14;
    char * v15;
    int32_t * v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    uint16_t v22;
    uint16_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    int v30;
    int v31;
    int v32;
    intptr_t v33;
    intptr_t v34;
    int16_t result;
    intptr_t v44;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v43 = (intptr_t *)(frame_ + 0x18);   /* [8] at ebp-0x20 */
    uint16_t *const v36__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x38 */
    uint16_t *const v37__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x34 */
    void ** *const Block__f = (void ** *)(frame_ + 0x8);   /* ebp-0x30 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c */
    uint16_t *const v40__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */

  F32(c, 0x14858) = 0;
  F32(c, 0x14854) = 0;
  F32(c, 0x1485c) = 0;
  F32(c, 0x14860) = 0;
  F16(c, 0x14868) = 0;
  F32(c, 0x14864) = 0;
  F16(c, 0x1486a) = 0;
  F32(c, 0x1486c) = 0;
  F16(c, 0x14870) = 0;
  v4 = F16(c, 0x1422e);
  if ( v4 == 0 )
  {
    v5 = a2;
    v6 = F32(c, 0x147b0);
    W_npc_navigate(c, v6, 1600, (int)a2, a3, 0, 0, 0, 0, 0);
    v7 = a2[3];
    v8 = a2[1];
    if ( v7 <= 4000 )
    {
      if ( (int)abs32(v6 - v8) >= 1600 )
        goto LABEL_41;
    }
    else if ( v7 / 2 + 1600 < (int)abs32(v6 - v8) )
    {
      goto LABEL_41;
    }
    v9 = F32(c, 0x147b8);
    v10 = v8 <= v9;
    if ( v8 >= v9 )
    {
      v11 = 3;
      if ( v10 )
        v11 = a2[11];
    }
    else
    {
      v11 = 1;
    }
    if ( v11 == 99 || a2[11] == v11 )
    {
      if ( a3 != 0 && (int)a2[10] >= 0 )
      {
        v12 = F32(c, 0x40);
        v13 = F32(v12, 16) - F32(c, 0x147b4) + F32(v12, 8) - 1;
        if ( (int)((HIDWORD(v13) ^ v13) - HIDWORD(v13)) >= 3200 )
        {
          F16(c, 0x1422c) = 0;
          F16(c, 0x1422e) = 0;
          F16(c, 0x14230) = 0;
        }
        else
        {
          v14 = F16(c, 0x147f0);
          (*v40__f) = F16(c, 0x147ae);
          (*v36__f) = F16(c, 0x44);
          (*v37__f) = v14;
          (*v41__f) = 0;
          v15 = (char *)(intptr_t)g_eng.op_new(0xA500u);
          (*Block__f) = (void **)v15;
          if ( F32(c, 0x0) != 0 )
          {
            v16 = v15 + 656;
            for ( (*i__f) = 64; (*i__f) != 0; --(*i__f) )
            {
              *v16 = (intptr_t)g_eng.op_new(0x50u);
              v16 += 165;
            }
          }
          v17 = 1;
          if ( (*v36__f) > 1u )
          {
            while ( (*v40__f) > v14 )
            {
              W_cmd_clear((int)v43);
              v18 = v14;
              v19 = v14 + 1;
              v20 = (*v40__f) - 1;
              if ( v20 < v19 )
                v19 = (*v40__f) - 1;
              else
                v20 = v19;
              if ( beh_442490(
                     c,
                     *(&F32(c, 0x147a8) + 2 * v18),
                     *((_DWORD *)&F16(c, 0x147ac) + 2 * v18),
                     *(&F32(c, 0x147b0) + 2 * v18),
                     *(&F32(c, 0x147b4) + 2 * v18),
                     *(&F32(c, 0x147b0) + 2 * v19),
                     *(&F32(c, 0x147b4) + 2 * v20),
                     v12,
                     v17 - 1,
                     v43) )
              {
                ++(*v37__f);
              }
              v21 = (int)&(*Block__f)[165 * (uint16_t)v17];
              W_char_integrate(c, (int)v43, v12, v21, 2, v17++);
              v12 = v21;
              if ( (uint16_t)v17 >= (*v36__f) )
                goto LABEL_34;
              v14 = (*v37__f);
            }
            (*v36__f) = v17;
          }
LABEL_34:
          v22 = 1;
          if ( (*v36__f) <= 1u )
          {
LABEL_37:
            (*v41__f) = 1;
          }
          else
          {
            while ( W_char_hazard_probe(&F32(c, 0x0), /*edx*/ v22, (int)&(*Block__f)[165 * v22], 0) == 0 )
            {
              if ( ++v22 >= (*v36__f) )
                goto LABEL_37;
            }
          }
          W_npc_probe_free(c, (*Block__f), 0x40u);
          if ( (*v41__f) != 0 )
          {
            ++F16(c, 0x1422e);
            ++F16(c, 0x147f0);
          }
          v5 = a2;
        }
      }
    }
    else
    {
      F32(c, 0x14854) = 2 - (v11 != 1);
    }
LABEL_41:
    v4 = F16(c, 0x1422e);
    if ( v4 == 0 )
      goto LABEL_80;
    goto LABEL_45;
  }
  if ( v4 == 1 )
  {
    v5 = a2;
LABEL_45:
    if ( a3 == 0 )
      F16(c, 0x1422e) = v4 + 1;
    goto LABEL_48;
  }
  if ( v4 != 2 )
    goto LABEL_80;
  v5 = a2;
LABEL_48:
  if ( F16(c, 0x1422e) == 2 && a3 != 0 )
  {
    F16(c, 0x1422c) = 0;
    F16(c, 0x1422e) = 0;
    F16(c, 0x14230) = 0;
  }
  else
  {
    v23 = F16(c, 0x147f0);
    v24 = *(&F32(c, 0x147b4) + 2 * v23);
    v25 = *(&F32(c, 0x147b0) + 2 * v23);
    v26 = *((_DWORD *)&F16(c, 0x147ac) + 2 * v23);
    v27 = *(&F32(c, 0x147a8) + 2 * v23);
    (*v42__f) = v26;
    v44 = v27;
    if ( v27 >= v25 )
    {
      v28 = (int)a2;
    }
    else
    {
      v28 = v25 - v5[1];
      if ( v28 <= 7200 )
      {
        if ( v28 > 0 )
        {
          v29 = v5[10];
          v31 = __OFSUB__(v29, F32(c, 0x1596c));
          v32 = v29 == F32(c, 0x1596c);
          v30 = v29 - F32(c, 0x1596c) < 0;
          v27 = v44;
          F32(c, 0x14854) = (v30 ^ v31 | v32) + 1;
        }
      }
      else
      {
        F32(c, 0x14854) = 6;
      }
    }
    v32 = v27 == v25;
    if ( v27 > v25 )
    {
      v28 = v5[1] - v25;
      if ( v28 <= 7200 )
      {
        if ( v28 > 0 )
        {
          v33 = v5[10];
          v31 = __OFSUB__(v33, F32(c, 0x1596c));
          v32 = v33 == F32(c, 0x1596c);
          v30 = v33 - F32(c, 0x1596c) < 0;
          v27 = v44;
          F32(c, 0x14854) = !(v30 ^ v31 | v32) + 1;
        }
      }
      else
      {
        F32(c, 0x14854) = 5;
      }
      v32 = v27 == v25;
    }
    if ( v32 )
      v28 = 0;
    if ( v26 <= v24 )
    {
      v34 = (int)a2;
    }
    else
    {
      v34 = a2[2] + v5[4] - v24 - 3200;
      if ( v34 > 0 )
      {
        if ( W_entity_ground_probe(c, (int)a2, 0) && W_body_can_act((int)a2) != 0 )
          F32(c, 0x1485c) = 7;
        else
          F32(c, 0x1485c) = 8;
      }
    }
    if ( (*v42__f) < v24 )
    {
      v34 = v24 - a2[4] - a2[2] + 3200;
      if ( v34 > 6400 )
        F32(c, 0x1485c) = 9;
    }
    if ( v28 <= 0 && v34 <= 0 )
      ++F16(c, 0x147f0);
    if ( F16(c, 0x147f0) >= F16(c, 0x147ae) )
    {
      F16(c, 0x1422c) = 0;
      F16(c, 0x1422e) = 0;
      F16(c, 0x14230) = 0;
    }
  }
LABEL_80:
  result = F16(c, 0x147f2);
  if ( result != 0 )
  {
    F16(c, 0x147f2) = --result;
  }
  else
  {
    F16(c, 0x1422c) = 0;
    F16(c, 0x1422e) = 0;
    F16(c, 0x14230) = 0;
  }
  return result;
}

/* 0x47adb0: int __thiscall sub_47ADB0(cls_4022A0 *this, int a1, _DWORD *a2, int *a3, int *a4, int a5, int a6, int *a7) */
int32_t beh_47adb0(wl_char *self, int32_t a1, int32_t * a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t * a7)
{
    intptr_t c = (intptr_t)self;

  return beh_47ae00(c, a1, a2[44], a2[177], a2[178], a2[179], a2[180], a3, a4, a5, a6, 0, a7);
}

/* 0x47d790: int __thiscall sub_47D790(cls_47D790 *this, int a2, int a3, int a4) */
int32_t beh_47d790(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    int32_t * v6;
    int32_t * v7;
    intptr_t i;
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
    uint16_t v20;
    intptr_t v21;
    intptr_t v22;
    int64_t v23;
    uint16_t v24;
    intptr_t v25;
    void ** v26;
    intptr_t j;
    intptr_t v36;
    intptr_t v37;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int32_t * *const v28__f = (int32_t * *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  v4 = c;
  if ( F32(c, 0xc84c) != 0 && F16(c, 0x14288) == 0 )
    return 1;
  (*v32__f) = 0;
  (*v30__f) = F32(c, 0x40);
  if ( F32(c, 0x15908) != 0 )
    return 1;
  v6 = (intptr_t)g_eng.op_new(0xA500u);
  (*v28__f) = v6;
  if ( F32(v4, 0x0) != 0 )
  {
    v7 = v6 + 164;
    for ( i = 64; i != 0; --i )
    {
      *v7 = (intptr_t)g_eng.op_new(0x50u);
      v7 += 165;
    }
  }
  v10 = 1;
  if ( a3 > 1 )
  {
    v11 = 1;
    do
    {
      v12 = (int)&v6[165 * v11];
      W_char_integrate(v4, a2, (*v30__f), v12, 2, v10++);
      (*v30__f) = v12;
      v6 = (*v28__f);
      v11 = (uint16_t)v10;
    }
    while ( (uint16_t)v10 < a3 );
  }
  if ( F32(v4, 0x0) == 0 )
  {
    v13 = v6[165 * a3 - 163];
    v14 = v6[165 * a3 - 160];
    v15 = (int)&v6[165 * a3 - 165];
    v16 = v13 + v14;
    (*v34__f) = v16;
    v36 = F32(v15, 16) + v13 - 1;
    (*v31__f) = F32(v15, 4);
    (*v33__f) = (*v31__f) + F32(v15, 12);
    SETLO(v17, 0);
    v18 = *(_DWORD *)&GAP(W, 0x2788, 4);
    v37 = 0;
    if ( F16(v18, 8) != 0 )
    {
      do
      {
        v19 = F32((F32(v18, 4) + 4 * (uint16_t)v17), 0);
        if ( c != v19 )
        {
          v20 = a3 - 1;
          if ( (uint16_t)(a3 - 1) >= F16(v19, 0x44) )
            v20 = 0;
          v21 = F32(v19, 0x40) + 660 * v20;
          if ( v16 < F32(v21, 16) + F32(v21, 8) - 1 )
          {
            if ( v36 > W_char_part_top(v19, a3 - 1)
              && (*v31__f) < W_part_right(v19, a3 - 1)
              && (*v33__f) > W_part_left(v19, a3 - 1)
              && W_entity_blocks(v19, (int)c, a3 - 1, 3, 0, F8(v15, 124)) )
            {
              v6 = (*v28__f);
              v4 = c;
              goto LABEL_34;
            }
            v16 = (*v34__f);
          }
        }
        v17 = v37 + 1;
        v18 = *(_DWORD *)&GAP(W, 0x2788, 4);
        v37 = v17;
      }
      while ( (uint16_t)v17 < F16(v18, 8) );
    }
    v6 = (*v28__f);
    v4 = c;
  }
  if ( a4 == 0
    || (v22 = F32(v4, 0x40),
        v23 = F32(v22, 4) - v6[165 * a3 - 164],
        (int)((HIDWORD(v23) ^ v23) - HIDWORD(v23)) >= 400)
    || F32(v22, 44) != v6[165 * a3 - 154] )
  {
    v24 = 1;
    (*v35__f) = 0;
    if ( a3 <= 1 )
    {
LABEL_32:
      (*v32__f) = 1;
    }
    else
    {
      v25 = 1;
      while ( W_char_hazard_probe(&F32(v4, 0x0), /*edx*/ v24, (int)&v6[165 * v25], &(*v35__f)) == 0 )
      {
        v25 = ++v24;
        if ( v24 >= a3 )
          goto LABEL_32;
      }
    }
  }
LABEL_34:
  if ( F32(v4, 0x0) != 0 )
  {
    v26 = (void **)(v6 + 164);
    for ( j = 64; j != 0; --j )
    {
      if ( *v26 != 0 )
      {
        g_eng.op_delete((void *)*v26);
        *v26 = 0;
      }
      v26 += 165;
    }
  }
  if ( v6 != 0 )
    g_eng.op_delete((void *)v6);
  return (*v32__f);
}

/* 0x47da60: int __thiscall sub_47DA60(cls_47DA60 *this, _DWORD *a2) */
int32_t beh_47da60(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t result;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v17;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v3 = a2[1];
  v4 = *(_DWORD *)&GAP(W, 0x1030, 24);
  (*v16__f) = v4;
  v5 = F32(v4, 2887728) - 1;
  if ( v5 >= (v3 / 3200 < 0 ? 0 : v3 / 3200) )
    v6 = v3 / 3200 < 0 ? 0 : v3 / 3200;
  else
    v6 = F32(v4, 2887728) - 1;
  v7 = (a2[3] + v3 - 1) / 3200;
  if ( v5 >= (v7 < 0 ? 0 : v7) )
    v17 = v7 < 0 ? 0 : v7;
  else
    v17 = F32(v4, 2887728) - 1;
  v8 = a2[2];
  result = 1;
  if ( F32(c, 0x1594c) != 3 )
  {
    v10 = F32(v4, 2887732);
    v11 = v8 / 3200 - 1;
    v12 = v10 - 1;
    while ( 1 )
    {
      v13 = v12 >= (v11 < 0 ? 0 : v11) ? (v11 < 0 ? 0 : v11) : v12;
      if ( F16(((*v16__f) + 2 * ((v6 << 7) + 1443872 + v13)), 0) == 1 )
        break;
      v14 = v12 >= (v11 < 0 ? 0 : v11) ? (v11 < 0 ? 0 : v11) : v12;
      if ( F16((*v16__f) + 2 * (v14 + (v17 << 7)), 2887744) == 1 )
        break;
      ++result;
      --v11;
      if ( result > 2 )
        return 1;
    }
    return 0;
  }
  return result;
}

/* 0x47de50: int __stdcall sub_47DE50(_DWORD *a1, int a2) */
int32_t beh_47de50(int32_t * a1, int32_t a2)
{

    int64_t v2;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  (*v12__f) = 0;
  v2 = a1[1] / 3200 + (a1[3] + a1[1] - 1) / 3200;
  (*v13__f) = (a1[4] + a1[2] - 1) / 3200;
  if ( a2 > 0 )
  {
    v3 = *(_DWORD *)&GAP(W, 0x1030, 24);
    v4 = 3200 * (((int)v2 - HIDWORD(v2)) >> 1) / 3200;
    do
    {
      v5 = 3200 * ((*v13__f) + (*v12__f) / 3200);
      if ( F32(v3, 2887728) - 1 >= (v4 < 0 ? 0 : v4) )
        v6 = v4 < 0 ? 0 : v4;
      else
        v6 = F32(v3, 2887728) - 1;
      v7 = F32(v3, 2887732) - 1;
      if ( v7 >= (v5 / 3200 < 0 ? 0 : v5 / 3200) )
        v7 = v5 / 3200 < 0 ? 0 : v5 / 3200;
      v8 = v7 + (v6 << 7);
      v9 = F32((v3 + 4 * (3 * v8 + 414732)), 0);
      if ( v9 == 1 )
      {
        if ( 3200 * ((*v13__f) + (*v12__f) / 3200) % 3200 <= 1600 )
        {
          if ( &a1 != 0 )
            a1 = 0;
          goto LABEL_14;
        }
        if ( &a1 != 0 )
          a1 = F32(v3, 12 * v8 + 1658932);
      }
      else
      {
        if ( &(*v11__f) != (int *)-28 )
          a1 = F32(v3, 12 * v8 + 1658932);
        if ( v9 == 0 )
          goto LABEL_14;
      }
      if ( a1 != 0 && a1 != (_DWORD *)2 )
        return 0;
LABEL_14:
      (*v12__f) += 3200;
    }
    while ( (*v12__f) < a2 );
  }
  return 1;
}

/* 0x47e010: int __thiscall sub_47E010(cls_47E010 *this, unsigned __int16 a2, int a3, int a4, int a5) */
int32_t beh_47e010(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    uint16_t v6;
    uint16_t v7;
    intptr_t result;
    intptr_t v9;
    char * v10;
    intptr_t v11;
    intptr_t v12;
    uint16_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;

  v6 = F16(c, 0xc754);
  v7 = 0;
  if ( v6 == 0 )
    return 1;
  while ( *(_DWORD *)&GAP(c, 0x754, 768 * v7) != a2 )
  {
    if ( ++v7 >= v6 )
      return 1;
  }
  v9 = 768 * v7;
  v10 = &GAP(c, 0x754, v9);
  if ( &GAP(c, 0x0, v9) == (char *)-1876 )
    return 1;
  v11 = *(_DWORD *)&GAP(c, 0x754, v9 + 192);
  switch ( v11 )
  {
    case 15728640:
      goto LABEL_14;
    case 15728641:
      if ( F16(F32(c, 0x750), 1126) == 0 && W_party_find(c, 50013, 0, 0, 0) != 0 )
        goto LABEL_14;
      goto LABEL_37;
    case 15728642:
      goto LABEL_11;
    case 15728643:
      if ( F32(F32(c, 0x750), 212) != 1 )
        goto LABEL_37;
LABEL_11:
      if ( F16(F32(c, 0x750), 1126) == 0 )
        goto LABEL_37;
LABEL_14:
      switch ( F32(v10, 4 * 190) )
      {
        case 1:
        case 2:
          break;
        case 0xFF:
          if ( F32(c, 0x1f0) == 0 )
            goto LABEL_37;
          break;
        default:
          goto LABEL_37;
      }
      v12 = F32(v10, 4 * 187);
      if ( v12 != 0 )
      {
        v13 = F16(a4, 100);
        if ( v13 < F16(v10, 2 * 376) || v13 > F16(v10, 2 * 377) )
          goto LABEL_37;
        v14 = F16(a4, 72);
        if ( (uint16_t)v14 != v12 )
        {
          if ( v12 != 10101 )
            goto LABEL_37;
          v15 = v14 - 10115;
          if ( v15 != 0 )
          {
            if ( v15 != 7 )
              return 0;
          }
          else if ( F16(a4, 84) != 0 )
          {
            return 0;
          }
        }
      }
      else if ( F16(a4, 72) != 0 && F16(a4, 86) > 0xC8u )
      {
        goto LABEL_37;
      }
      v16 = F32(v10, 4 * 47) - 1;
      if ( v16 != 0 )
      {
        v17 = v16 - 1;
        if ( v17 != 0 )
        {
          if ( v17 != 1 )
            return 1;
          if ( a5 != 0 && F32(a4, 40) >= 900 * F32(c, 0x15990) / 1000 )
            return 1;
        }
        else if ( a5 == 0 )
        {
          return 1;
        }
      }
      else if ( a5 != 0 )
      {
        return 1;
      }
LABEL_37:
      result = 0;
      break;
    default:
      if ( v11 == F32(F32(c, 0x750), 212) )
        goto LABEL_14;
      goto LABEL_37;
  }
  return result;
}

/* 0x47e7c0: int __stdcall sub_47E7C0(int a1, unsigned __int16 a2, _DWORD *a3, int a4, _DWORD *a5, _DWORD *a6, _DWORD *a7, int *a8) */
int32_t beh_47e7c0(int32_t a1, uint16_t a2, int32_t * a3, int32_t a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8)
{

    intptr_t v8;
    uint16_t v9;
    int32_t * v10;
    intptr_t v11;
    intptr_t v12;
    int64_t v13;
    intptr_t v14;
    int v15;
    intptr_t v16;
    int32_t * v17;
    intptr_t v18;
    int32_t * v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t result;
    intptr_t v23;
    uint16_t v24;
    uint16_t v25;
    uint16_t v26;
    intptr_t v27;
    intptr_t v28;
    uint16_t v29;
    intptr_t v30;
    uint16_t v31;
    intptr_t v39;
    intptr_t v40;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  v8 = a1;
  (*v36__f) = a3[1] + a3[3] / 2;
  (*v37__f) = a3[2] + a3[4] / 2;
  if ( F32(a1, 0) != 0 )
  {
    v9 = -1;
    (*v34__f) = -1;
    if ( F16(a1, 4) != 0 )
    {
      (*v33__f) = 0;
      v10 = (_DWORD *)(F32(a1, 64) + 660 * a2);
      v11 = 0;
      (*v32__f) = 0;
      (*v35__f) = F16(a1, 4);
      while ( 1 )
      {
        v12 = F32((v11 + v10[164]), 0);
        if ( v10[11] == 3 )
          v12 = -v12;
        v13 = (*v37__f) - (v10[2] + F32((*v32__f) + v10[164], 4) + v10[4] / 2);
        v14 = (HIDWORD(v13) ^ v13) - HIDWORD(v13) + abs32((*v36__f) - (v12 + v10[1] + v10[3] / 2));
        if ( (*v34__f) == -1 || v14 < (*v38__f) )
        {
          (*v38__f) = v14;
          (*v34__f) = (*v33__f);
        }
        v15 = (*v35__f) == 1;
        ++(*v33__f);
        (*v32__f) += 20;
        --(*v35__f);
        if ( v15 )
          break;
        v11 = (*v32__f);
      }
      v8 = a1;
      v9 = (*v34__f);
    }
    v16 = 5 * v9;
    v17 = (_DWORD *)(F32(v8, 64) + 660 * a2);
    v18 = v17[164];
    v19 = (int *)(v18 + 4 * v16);
    v40 = *v19;
    if ( v17[11] == 3 )
      v40 = -F32((v18 + 4 * v16), 0);
    v39 = v19[2];
    v20 = v40 + v17[1] + v17[3] / 2 - v39 / 2;
    v21 = v20 + v39 / 2;
    result = v17[2] + v19[1] + v17[4] / 2;
    v23 = v20 + v39;
  }
  else
  {
    v24 = a2;
    v25 = F16(a1, 68);
    v26 = a2;
    if ( a2 >= v25 )
      v26 = 0;
    v27 = F32(a1, 64);
    v28 = v27 + 660 * v26;
    v29 = a2;
    v21 = F32(v28, 4) + F32(v28, 12) / 2;
    if ( a2 >= v25 )
      v29 = 0;
    v30 = F32(v27 + 660 * v29, 20);
    result = v30 + F32(v27 + 660 * v29, 8) + (F32(v27 + 660 * v29, 16) - v30) / 2;
    v31 = a2;
    if ( a2 >= v25 )
      v31 = 0;
    v20 = F32(v27 + 660 * v31, 4);
    if ( a2 >= v25 )
      v24 = 0;
    v23 = F32(v27 + 660 * v24, 4) + F32(v27 + 660 * v24, 12);
  }
  if ( (*v36__f) >= v21 )
  {
    if ( a5 != 0 )
      *a5 = v23 + a4;
    if ( a6 != 0 )
      *a6 = 3;
    if ( a7 != 0 )
      *a7 = a3[1] - v23;
  }
  else
  {
    if ( a5 != 0 )
      *a5 = v20 - a3[3] - a4;
    if ( a6 != 0 )
      *a6 = 1;
    if ( a7 != 0 )
      *a7 = v20 - a3[3] - a3[1];
  }
  if ( a8 != 0 )
  {
    result -= (*v37__f);
    *a8 = result;
  }
  return result;
}

/* 0x47eae0: int __thiscall sub_47EAE0(cls_47EAE0 *this, int *a2, int *a3, __int16 *a4, int a5) */
int32_t beh_47eae0(wl_char *self, int32_t * a2, int32_t * a3, int16_t * a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    int32_t * v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    int16_t v14;

  v5 = *a2;
  v6 = 0;
  v7 = 1;
  v8 = F32(*a2, 724);
  if ( v8 <= 0 )
  {
    v11 = 1000;
  }
  else
  {
    if ( a5 != 0 && v8 > F32(F32(c, 0x750), 92) )
      return 0;
    v9 = (_DWORD *)F32(c, 0x750);
    v10 = v9[34] + v9[40] + v9[24];
    if ( v10 <= 0 )
      v11 = 0;
    else
      v11 = 1000 * v9[23] / v10;
  }
  if ( F16(v5, 174) == 1 )
  {
    v12 = F16(c, 0x14828);
LABEL_13:
    if ( F16(c, 0x14832) == 0 )
      goto LABEL_14;
    return 0;
  }
  if ( F16(v5, 174) != 2 )
  {
    v12 = F16(c, 0x1482c);
    goto LABEL_13;
  }
  v7 = 0;
  v12 = F16(c, 0x1482a);
  v6 = 1;
LABEL_14:
  if ( F32(c, 0x1f0) == 0 )
  {
    switch ( v12 )
    {
      case 1:
        v13 = 1200;
        v14 = 800;
        if ( v6 == 0 )
        {
          if ( v11 >= 100 )
          {
            if ( v11 < 300 )
            {
              v13 = 1500;
              v14 = 1000;
            }
          }
          else
          {
            v13 = 1800;
            v14 = 1200;
          }
        }
        goto LABEL_36;
      case 2:
        v13 = 800;
        v14 = 400;
        if ( v6 == 0 )
        {
          if ( v11 >= 100 )
          {
            if ( v11 >= 300 )
            {
              if ( v11 < 500 )
              {
                v13 = 1000;
                v14 = 600;
              }
            }
            else
            {
              v13 = 1200;
              v14 = 800;
            }
          }
          else
          {
            v13 = 1600;
            v14 = 1000;
          }
        }
        goto LABEL_36;
      case 3:
        v13 = 600;
        v14 = 300;
        if ( v6 == 0 )
        {
          if ( v11 >= 100 )
          {
            if ( v11 < 300 )
            {
              v13 = 800;
              v14 = 400;
            }
          }
          else
          {
            v13 = 1000;
            v14 = 600;
          }
        }
        goto LABEL_36;
      case 4:
        v13 = 300;
        v14 = 200;
        goto LABEL_36;
      case 5:
        v13 = 200;
        v14 = 50;
        goto LABEL_36;
      default:
        return 0;
    }
  }
  v13 = 500;
  v14 = 500;
LABEL_36:
  if ( v7 == 0 )
    v14 = 0;
  if ( a3 != 0 )
    *a3 = v13;
  if ( a4 != 0 )
    *a4 = v14;
  return 1;
}

/* 0x47ecd0: int *__thiscall sub_47ECD0( cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, unsigned __int16 a8, int *a9, int *a10) */
int32_t beh_47ecd0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint16_t a8, int32_t * a9, int32_t * a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10;
    int v11;
    intptr_t v12;
    int32_t * v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    int32_t * v19;
    intptr_t v20;
    uint16_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    int32_t * v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    intptr_t v31;
    intptr_t v32;
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
    intptr_t v43;
    intptr_t v44;
    intptr_t v45;
    int32_t * v46;
    uint16_t * v47;
    intptr_t j;
    intptr_t v49;
    intptr_t v50;
    intptr_t v51;
    intptr_t v52;
    intptr_t v53;
    intptr_t v55;
    int32_t * result;
    _Alignas(8) uint8_t frame_[0x64 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v57 = (uint8_t *)(frame_ + 0x0);   /* [16] at ebp-0x64 */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x54 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x50 */
    int32_t * *const v60__f = (int32_t * *)(frame_ + 0x18);   /* ebp-0x4c */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x48 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x44 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x40 */
    int32_t * *const v64__f = (int32_t * *)(frame_ + 0x28);   /* ebp-0x3c */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x38 */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x34 */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x30 */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x2c */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x28 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x24 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x20 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x1c */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x18 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x14 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x10 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0xc */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x8 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0x4 */

  v10 = c;
  (*v61__f) = c;
  *a10 = 0;
  v11 = F16(c, 0x14234) == 0;
  (*v64__f) = 0;
  (*v73__f) = 0;
  (*v75__f) = 0;
  (*v74__f) = 0;
  (*v76__f) = 0;
  if ( !v11 || F16(c, 0x14228) == 0 || a2 == 0 )
    return 0;
  (*v71__f) = 0;
  do
  {
    (*i__f) = a2;
    (*v70__f) = 0;
    v12 = 320 * (uint16_t)(*v71__f);
    v11 = *(_WORD *)&GAP(v10, 0xca26, v12 + 282) == 0;
    v13 = (int *)&GAP(v10, 0xca26, v12 + 2);
    (*v59__f) = (int)v13;
    if ( !v11 )
      continue;
    if ( *(_WORD *)&GAP(v10, 0xca26, v12 + 318) != 0 || *(_WORD *)&GAP(v10, 0xca26, v12 + 320) != 0 )
    {
      v14 = 1000
          * F32(F32(v10, 0x750), 84)
          / (F32(F32(v10, 0x750), 88) + F32(F32(v10, 0x750), 132) + F32(F32(v10, 0x750), 156));
      if ( v14 < *(uint16_t *)&GAP(v10, 0xca26, v12 + 318) || v14 > F16((*v59__f), 318) )
        continue;
      v13 = (int *)(*v59__f);
    }
    v15 = *v13;
    if ( W_char_skill_ok(v10, *v13, 1) == 0 || W_skill_pose_ok(v15, (uint16_t *)a4) == 0 )
      continue;
    v16 = F32(v15, 188) - 1;
    if ( v16 != 0 )
    {
      v17 = v16 - 1;
      if ( v17 != 0 )
      {
        if ( v17 == 1 && (a5 == 0 || F32(a4, 40) < 900 * F32(v10, 0x15990) / 1000) )
          continue;
      }
      else if ( a5 != 0 )
      {
        continue;
      }
    }
    else if ( a5 == 0 )
    {
      continue;
    }
    v18 = F32((*v59__f), 8);
    if ( v18 >= 6 )
    {
      if ( v18 <= 7 )
      {
        if ( a5 != 0 )
          continue;
      }
      else if ( v18 == 8 && a5 == 0 )
      {
        continue;
      }
    }
    v19 = *(_DWORD **)(*v59__f);
    (*v60__f) = v19;
    if ( *v19 != a8 && a8 != 0 )
      continue;
    if ( (int)v19[181] > 0 )
    {
      v20 = *((uint16_t *)v19 + 87);
      if ( F32(v10, 0x15a5c) != 0 )
      {
        v21 = 0;
        do
        {
          v22 = *(_DWORD *)&GAP(W, 0x1030, 4 * v21 + 304);
          if ( v22 != v10
            && F32(v22, 464) == 1
            && F8(F32(v22, 64), 0) == 1
            && F16(F32(v22, 1872), 1098) == 0
            && W_faction_rel(v10, F32(v10, 0x1dc), F32(v22, 476)) == 2
            && beh_48bdd0(v22, v20, 0) != 0 )
          {
            goto LABEL_164;
          }
        }
        while ( ++v21 < 0x20u );
      }
      v23 = 0;
      (*v67__f) = 1;
      (*v60__f) = *(_DWORD **)(*v59__f);
      v24 = (*v60__f)[181];
      if ( v24 <= 0 )
      {
        v28 = 1000;
      }
      else
      {
        v25 = (_DWORD *)F32(v10, 0x750);
        v26 = v25[23];
        if ( v24 > v26 )
          continue;
        v27 = v25[34] + v25[40] + v25[24];
        if ( v27 <= 0 )
          v28 = 0;
        else
          v28 = 1000 * v26 / v27;
      }
      if ( F16((*v60__f), 2 * 87) == 1 )
      {
        v29 = F16(v10, 0x14828);
      }
      else
      {
        if ( F16((*v60__f), 2 * 87) == 2 )
        {
          (*v67__f) = 0;
          v29 = F16(v10, 0x1482a);
          v23 = 1;
LABEL_52:
          if ( F32(v10, 0x1f0) == 0 )
          {
            switch ( v29 )
            {
              case 1:
                v30 = 1200;
                if ( v23 == 0 )
                {
                  if ( v28 >= 100 )
                  {
                    if ( v28 < 300 )
                      v30 = 1500;
                  }
                  else
                  {
                    v30 = 1800;
                  }
                }
                goto LABEL_74;
              case 2:
                v30 = 800;
                if ( v23 == 0 )
                {
                  if ( v28 >= 100 )
                  {
                    if ( v28 >= 300 )
                    {
                      if ( v28 < 500 )
                        v30 = 1000;
                    }
                    else
                    {
                      v30 = 1200;
                    }
                  }
                  else
                  {
                    v30 = 1600;
                  }
                }
                goto LABEL_74;
              case 3:
                v30 = 600;
                if ( v23 == 0 )
                {
                  if ( v28 >= 100 )
                  {
                    if ( v28 < 300 )
                      v30 = 800;
                  }
                  else
                  {
                    v30 = 1000;
                  }
                }
                goto LABEL_74;
              case 4:
                v30 = 300;
                goto LABEL_74;
              case 5:
                v30 = 200;
                goto LABEL_74;
              default:
                continue;
            }
          }
          v30 = 500;
LABEL_74:
          if ( v57 != (_BYTE *)-60 )
            (*v69__f) = v30;
          goto LABEL_77;
        }
        v29 = F16(v10, 0x1482c);
      }
      if ( F16(v10, 0x14832) != 0 )
        continue;
      goto LABEL_52;
    }
    (*v69__f) = 0;
LABEL_77:
    v31 = *((uint16_t *)(*v60__f) + 87);
    if ( (uint16_t)v31 != 1 )
    {
      if ( (uint16_t)v31 == 2 )
      {
        if ( a7 != 0 )
          continue;
      }
      else if ( (uint16_t)v31 != 3 || F32(v10, 0x1f0) != 0 )
      {
        continue;
      }
    }
    v32 = F32((*v59__f), 12);
    switch ( v32 )
    {
      case 0:
      case 1:
      case 2:
      case 4:
        goto LABEL_86;
      case 3:
        if ( a6 == 0 || F32(v10, 0x1f0) != 0 )
          break;
LABEL_86:
        if ( v32 == 3 )
        {
          if ( v31 == 1 )
          {
            v50 = (*v59__f);
            (*v63__f) = F32(a4, 44);
            v33 = beh_47fd40(v10, (_DWORD *)a4, (*v59__f), 1, 0, (*v63__f), 0);
            (*v58__f) = v33;
            if ( v33 == 0 && F32(v50, 308) == 0 )
            {
              (*v63__f) = W_facing_back(F32(a4, 44));
              v33 = beh_47fd40(v10, (_DWORD *)a4, v50, 1, 0, (*v63__f), 0) - 20;
              (*v58__f) = v33;
            }
          }
          else if ( v31 == 2 )
          {
            v33 = (*v69__f) + 1;
            (*v58__f) = (*v69__f) + 1;
            if ( a2 == v10 )
              (*v63__f) = F32(a4, 44);
            else
              (*v63__f) = F32(a4, 4) + F32(a4, 12) / 2 >= F32(F32((a2 + 64), 0), 4)
                                                                    + F32(F32((a2 + 64), 0), 12) / 2
                  ? 3
                  : 1;
          }
          else
          {
            if ( v31 == 3 )
            {
              v34 = (*v60__f);
              v35 = 0;
              v36 = 0;
              v37 = 0;
              v38 = (*v60__f)[46];
              (*v67__f) = 0;
              (*v66__f) = 0;
              (*v62__f) = 0;
              switch ( v38 )
              {
                case 1:
                  goto LABEL_94;
                case 2:
                  v41 = (*v60__f);
                  v42 = 4448;
                  for ( (*i__f) = 32; (*i__f) != 0; --(*i__f) )
                  {
                    v43 = *(intptr_t *)&GAP(W, 0x0, v42);
                    if ( F32(v43, 0x1d0) != 0
                      && beh_47adb0(v43, (int)(*v61__f), v41, &(*v65__f), &(*v68__f), 0, 1, 0) != 0
                      && (*v65__f) > 0 )
                    {
                      if ( v36 == 0 )
                        v36 = (int)v43;
                      if ( (*v67__f) <= (*v65__f) )
                        (*v67__f) = (*v65__f);
                      if ( (*v62__f) <= (*v68__f) )
                        (*v62__f) = (*v68__f);
                      ++(*v66__f);
                    }
                    v42 += 4;
                  }
                  v37 = (*v62__f);
                  v34 = (*v60__f);
                  goto LABEL_124;
                case 3:
                  if ( (*v60__f)[44] == 14 || beh_48be60((*v61__f), F16((*v60__f), 2 * 354), 0) == 0 )
                  {
LABEL_94:
                    v39 = 4448;
                    for ( (*i__f) = 32; (*i__f) != 0; --(*i__f) )
                    {
                      v40 = *(intptr_t *)&GAP(W, 0x0, v39);
                      if ( F32(v40, 0x1d0) != 0 && beh_47adb0(v40, (int)(*v61__f), (*v60__f), &(*v65__f), &(*v68__f), 0, 1, &(*v77__f)) != 0 && (*v65__f) > 0 )
                      {
                        if ( v36 == 0 || v35 < (*v77__f) )
                        {
                          v35 = (*v77__f);
                          v36 = (int)v40;
                          (*v67__f) = (*v65__f);
                          (*v62__f) = (*v68__f);
                        }
                        ++(*v66__f);
                      }
                      v39 += 4;
                    }
                    v37 = (*v62__f);
                    v34 = (*v60__f);
                  }
                  goto LABEL_124;
                case 4:
                  if ( (*v60__f)[44] == 14 )
                  {
                    v44 = (*v61__f);
                  }
                  else
                  {
                    v44 = (*v61__f);
                    if ( beh_48be60((*v61__f), F16((*v60__f), 2 * 354), 0) != 0 )
                      goto LABEL_124;
                  }
                  if ( beh_47adb0(v44, (int)v44, v34, &(*v65__f), &(*v68__f), 0, 1, 0) != 0 && (*v65__f) > 0 )
                  {
                    (*v67__f) = (*v65__f);
                    v36 = (int)v44;
                  }
LABEL_124:
                  if ( v34[46] == 2 && (*v66__f) > 0 )
                  {
                    if ( (*v66__f) < 2 )
                      v37 /= 2;
                    else
                      v37 *= 2;
                    (*v62__f) = v37;
                  }
                  if ( v57 != (_BYTE *)-16 )
                    (*v58__f) = (*v67__f);
                  if ( v57 != (_BYTE *)-64 )
                    (*v70__f) = v37;
                  (*i__f) = v36;
                  if ( v36 == 0 )
                  {
                    v10 = (*v61__f);
                    continue;
                  }
                  v45 = 0;
                  v46 = F32(D(0x628760), 0);
                  v47 = (_WORD *)(F32(v36, 1872) + 1096);
                  for ( j = 18; j != 0; --j )
                  {
                    if ( *v47 != 0 && *v46 != 0 )
                      ++v45;
                    ++v47;
                    v46 += 35;
                  }
                  (*v58__f) -= 100 * v45;
                  if ( v36 == (*v61__f) )
                    v49 = F32(a4, 44);
                  else
                    v49 = beh_47f610(v36, 0, a4);
                  v10 = (*v61__f);
                  (*v63__f) = v49;
                  break;
                default:
                  goto LABEL_124;
              }
            }
            v33 = (*v58__f);
          }
        }
        else
        {
          v33 = beh_480b30(v10, a2, (*v59__f), 0x60u, 1, a3, &(*v63__f));
          (*v58__f) = v33;
        }
        if ( F32(F32((*v59__f), 0), 724) > 0 && (*v69__f) > v33 )
        {
          (*v58__f) = 0;
          break;
        }
        if ( v33 <= 0 )
          break;
        v51 = (*v59__f);
        v52 = (*v58__f);
        v53 = (*v58__f);
        if ( F32((*v59__f), 296) != 0 )
        {
          v52 += F32(v51, 296) + F32(v51, 300) * rnd() / 0x8000;
          (*v58__f) = v52;
        }
        if ( v52 > (*v73__f) || (*v64__f) == 0 )
          goto LABEL_162;
        if ( v53 == (*v74__f) && (*v70__f) > (*v75__f) )
        {
          v52 = (*v58__f);
          v51 = (*v59__f);
LABEL_162:
          (*v64__f) = (int *)v51;
          (*v73__f) = v52;
          (*v74__f) = v53;
          (*v75__f) = (*v70__f);
          (*v78__f) = (*v63__f);
          (*v76__f) = (*i__f);
        }
        break;
      default:
        break;
    }
LABEL_164:
    ;
  }
  while ( (uint16_t)++(*v71__f) < F16(v10, 0x14228) );
  if ( (*v64__f) != 0 && (*v64__f)[72] <= 1000 * rnd() / 0x8000 )
    (*v64__f) = 0;
  v55 = (*v76__f);
  *a9 = (*v78__f);
  result = (*v64__f);
  *a10 = v55;
  return result;
}

/* 0x47f670: BOOL __thiscall sub_47F670(cls_47F670 *this, unsigned __int16 a2, int a3) */
int32_t beh_47f670(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    int32_t * v4;
    intptr_t v5;
    int v7;
    intptr_t v8;
    int v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    uint16_t v17;
    intptr_t v18;
    intptr_t v19;
    uint16_t v20;
    intptr_t v21;
    int32_t * v22;
    uint16_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v31;
    uint16_t v59;
    _Alignas(8) uint8_t frame_[0x62c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v35 = (uint16_t *)(frame_ + 0x14);   /* [2] at ebp-0x618 */
    intptr_t *v43 = (intptr_t *)(frame_ + 0x34);   /* [3] at ebp-0x5f8 */
    uint8_t *v56 = (uint8_t *)(frame_ + 0x104);   /* [656] at ebp-0x528 */
    intptr_t *v58 = (intptr_t *)(frame_ + 0x398);   /* [165] at ebp-0x294 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x62c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x61c */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x614 */
    uint16_t *const v37__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x610 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x60c */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x608 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x604 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x600 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x5fc */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x5ec */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x5e8 */
    int16_t *const v46__f = (int16_t *)(frame_ + 0x48);   /* ebp-0x5e4 */
    int16_t *const v47__f = (int16_t *)(frame_ + 0x4a);   /* ebp-0x5e2 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x5e0 */
    int16_t *const v49__f = (int16_t *)(frame_ + 0x50);   /* ebp-0x5dc */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x5d8 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x5d4 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x5d0 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0x5cc */
    char *const v54__f = (char *)(frame_ + 0x64);   /* ebp-0x5c8 */
    char *const v55__f = (char *)(frame_ + 0xb4);   /* ebp-0x578 */
    char * *const v57__f = (char * *)(frame_ + 0x394);   /* ebp-0x298 */

  v3 = c;
  v4 = 0;
  v5 = F32(c, 0x40);
  (*v57__f) = &(*v54__f);
  v7 = a2 <= 0x40u;
  (*v34__f) = v3;
  (*v36__f) = 0;
  v58[164] = (int)&(*v55__f);
  v59 = 64;
  if ( v7 )
    v59 = a2;
  v8 = 1;
  F32(v35, 0) = 1;
  if ( v59 <= 1u )
    return a3 == 0 || (*v36__f) != 0;
  while ( 1 )
  {
    v9 = v4 == (int *)v56;
    v4 = v58;
    if ( !v9 )
      v4 = (int *)v56;
    memset(v43, 0, (3 * sizeof(intptr_t)));
    (*v44__f) = 0;
    (*v46__f) = 0;
    (*v45__f) = 0;
    (*v47__f) = 0;
    (*v48__f) = 0;
    (*v49__f) = 0;
    if ( a3 != 0 )
      (*v44__f) = 11;
    W_char_integrate(v3, (int)v43, v5, (int)v4, 2, v8);
    if ( F16(F32(v3, 0x750), 1098) == 0 && F16(v3, 0x14234) == 0 && *((uint8_t *)v4 + 125) == 0 )
    {
      if ( F16(v4, 2 * 28) != 5 || (v10 = F32(v3, 0x15a10), (*v38__f) = 1, v10 == 0) )
        (*v38__f) = 0;
      SETLO(v11, 0);
      (*v40__f) = 0;
      v12 = F32(W, 0x2788);
      if ( F16(v12, 8) != 0 )
        break;
    }
LABEL_55:
    ++v8;
    v5 = (int)v4;
    F32(v35, 0) = v8;
    if ( (uint16_t)v8 >= v59 )
      return a3 == 0 || (*v36__f) != 0;
  }
  while ( 1 )
  {
    v13 = F32((F32(v12, 4) + 4 * (uint16_t)v11), 0);
    if ( v13 != v3 )
    {
      if ( (v14 = F32(v3, 0x1dc), v15 = F32(v13, 0x1dc), v14 != 0) && v15 != 0 && (v14 == 3 || v15 == 3 || v14 != v15)
        || F32(v13, 0x1f0) == 1 )
      {
        v16 = F32(v3, 0x0);
        (*v37__f) = F16(v3, 0x4);
        v17 = F16(v13, 0x44);
        (*v41__f) = v16;
        if ( v17 > (int)v35[0] )
        {
          v18 = F32(v13, 0x40);
          v19 = 660 * v35[0];
          (*v42__f) = v19;
          if ( F8((v18 + v19), 0) != 0 )
          {
            v20 = 0;
            v9 = F16(v18 + v19, 128) == 0;
            (*v39__f) = 0;
            if ( !v9 )
              break;
          }
        }
      }
    }
LABEL_53:
    v11 = (*v40__f) + 1;
    v12 = F32(W, 0x2788);
    (*v40__f) = v11;
    if ( (uint16_t)v11 >= F16(v12, 8) )
    {
      v8 = F32(v35, 0);
      goto LABEL_55;
    }
  }
  while ( 1 )
  {
    v21 = v18 + (v20 << 6);
    v22 = (_DWORD *)(v21 + v19 + 132);
    if ( (*v41__f) != 0 )
      break;
    v26 = v4[1];
    v27 = F32(v21 + v19, 152);
    if ( v27 <= v26 + v4[3] && v27 + v22[7] > v26 )
    {
      v28 = v4[2];
      v29 = v22[6];
      if ( v29 <= v28 + v4[4] && v29 + v22[8] > v28 + v4[5] )
        goto LABEL_40;
    }
LABEL_37:
    v3 = (*v34__f);
LABEL_38:
    v18 = F32(v13, 0x40);
    v20 = (*v39__f) + 1;
    if ( (uint16_t)++(*v39__f) >= F16(v18 + v19, 128) )
      goto LABEL_53;
  }
  v23 = 0;
  if ( (*v37__f) == 0 )
    goto LABEL_37;
  while ( 1 )
  {
    W_body_frame_box(v23, v4, &(*v50__f));
    v24 = v22[5];
    if ( v24 <= (*v50__f) + (*v52__f) && v24 + v22[7] > (*v50__f) )
    {
      v25 = v22[6];
      if ( v25 <= (*v51__f) + (*v53__f) && v25 + v22[8] > (*v51__f) )
        break;
    }
    if ( ++v23 >= (*v37__f) )
    {
      v3 = (*v34__f);
      v19 = (*v42__f);
      goto LABEL_38;
    }
  }
LABEL_40:
  v3 = (*v34__f);
  if ( v22 == 0 || W_char_hit_fresh((*v34__f), (int)v22, v35[0]) == 0 )
    goto LABEL_53;
  v31 = v22[4];
  if ( v22[3] != 0 )
  {
    if ( F32(v3, 0x15a28) == 1 )
    {
      if ( v31 == 0 )
        goto LABEL_50;
      return false;
    }
    if ( F32(v3, 0x15a28) != 2 )
    {
      if ( F32(v3, 0x15a28) == 3 )
        goto LABEL_50;
      return false;
    }
    if ( v31 != 1 )
      return false;
  }
LABEL_50:
  if ( (*v38__f) != 0 )
  {
    if ( &(*v33__f) != (int *)-24 )
      (*v36__f) = 1;
    goto LABEL_53;
  }
  return false;
}

/* 0x47f9d0: int __thiscall sub_47F9D0(cls_4022A0 *this, unsigned __int16 a2, int a3) */
int32_t beh_47f9d0(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    int32_t * v6;
    char * v7;
    intptr_t v8;
    intptr_t v10;
    int v11;
    int16_t v12;
    int16_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    uint16_t v21;
    intptr_t v22;
    int32_t * v23;
    uint16_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    _Alignas(8) uint8_t frame_[0x614 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v43 = (intptr_t *)(frame_ + 0x2c);   /* [5] at ebp-0x5e8 */
    uint8_t *v50 = (uint8_t *)(frame_ + 0xec);   /* [656] at ebp-0x528 */
    intptr_t *v52 = (intptr_t *)(frame_ + 0x380);   /* [165] at ebp-0x294 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x614 */
    uint16_t *const v33__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x610 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x60c */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x608 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x604 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x600 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x5fc */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x5f8 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x5f4 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x5f0 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x5ec */
    int16_t *const v44__f = (int16_t *)(frame_ + 0x40);   /* ebp-0x5d4 */
    int16_t *const v45__f = (int16_t *)(frame_ + 0x42);   /* ebp-0x5d2 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x5d0 */
    int16_t *const v47__f = (int16_t *)(frame_ + 0x48);   /* ebp-0x5cc */
    char *const v48__f = (char *)(frame_ + 0x4c);   /* ebp-0x5c8 */
    char *const v49__f = (char *)(frame_ + 0x9c);   /* ebp-0x578 */
    char * *const v51__f = (char * *)(frame_ + 0x37c);   /* ebp-0x298 */

  v3 = a3;
  v4 = c;
  v5 = F32(c, 0x40);
  (*v51__f) = &(*v48__f);
  v6 = 0;
  v52[164] = (int)&(*v49__f);
  if ( a3 == 0 )
  {
    if ( F16(c, 0x14228) == 0 )
      return 0;
    v7 = &GAP(c, 0xca26, 2);
    v8 = F16(v4, 0x14228);
    do
    {
      if ( F32(v7, 4 * 3) == 3 )
        a3 = (int)v7;
      v7 += 320;
      --v8;
    }
    while ( v8 != 0 );
    v3 = a3;
    if ( a3 == 0 )
      return 0;
  }
  v10 = 1;
  (*v32__f) = 1;
  if ( a2 <= 1u )
    return 1;
  do
  {
    v11 = v6 == (int *)v50;
    v6 = v52;
    if ( !v11 )
      v6 = (int *)v50;
    v12 = F16(v3, 4);
    memset(v43, 0, 16);
    (*v46__f) = 0;
    (*v44__f) = v12;
    v13 = F16(v3, 8);
    v43[4] = 14;
    (*v47__f) = 210;
    (*v45__f) = v13;
    W_char_integrate(v4, (int)v43, v5, (int)v6, 2, v10);
    SETLO(v14, 0);
    if ( F16(F32(v4, 0x750), 1098) != 0 || F16(v4, 0x14234) != 0 || *((uint8_t *)v6 + 125) != 0 )
      goto LABEL_52;
    if ( F16(v6, 2 * 28) != 5 || ((*v34__f) = 1, F32(v4, 0x15a10) == 0) )
      (*v34__f) = 0;
    (*v36__f) = 0;
    v15 = F32(W, 0x2788);
    if ( F16(v15, 8) == 0 )
      goto LABEL_52;
    while ( 2 )
    {
      v16 = F32((F32(v15, 4) + 4 * (uint16_t)v14), 0);
      if ( v16 == v4 )
        goto LABEL_50;
      v17 = F32(v4, 0x1dc);
      v18 = F32(v16, 0x1dc);
      if ( (v17 == 0 || v18 == 0 || v17 != 3 && v18 != 3 && v17 == v18) && F32(v16, 0x1f0) != 1 )
        goto LABEL_50;
      (*v33__f) = F16(v4, 0x4);
      (*v37__f) = F32(v4, 0x0);
      if ( F16(v16, 0x44) <= (int)(uint16_t)v10 )
        goto LABEL_50;
      v19 = F32(v16, 0x40);
      v20 = 660 * (uint16_t)v10;
      (*v38__f) = v20;
      if ( F8((v19 + v20), 0) == 0 )
        goto LABEL_50;
      v21 = 0;
      (*v35__f) = 0;
      if ( F16(v19 + v20, 128) == 0 )
        goto LABEL_50;
      while ( 1 )
      {
        v22 = v19 + (v21 << 6);
        v23 = (int *)(v22 + v20 + 132);
        if ( (*v37__f) != 0 )
          break;
        v27 = v6[1];
        v28 = F32(v22 + v20, 152);
        if ( v28 <= v27 + v6[3] && v28 + v23[7] > v27 )
        {
          v29 = v6[2];
          v30 = v23[6];
          if ( v30 <= v29 + v6[4] && v30 + v23[8] > v29 + v6[5] )
            goto LABEL_46;
        }
LABEL_43:
        v4 = c;
LABEL_44:
        v19 = F32(v16, 0x40);
        v21 = ++(*v35__f);
        if ( (uint16_t)(*v35__f) >= F16(v19 + v20, 128) )
          goto LABEL_50;
      }
      v24 = 0;
      if ( (*v33__f) == 0 )
        goto LABEL_43;
      while ( 1 )
      {
        W_body_frame_box(v24, v6, &(*v39__f));
        v25 = v23[5];
        if ( v25 <= (*v39__f) + (*v41__f) && v25 + v23[7] > (*v39__f) )
        {
          v26 = v23[6];
          if ( v26 <= (*v40__f) + (*v42__f) && v26 + v23[8] > (*v40__f) )
            break;
        }
        if ( ++v24 >= (*v33__f) )
        {
          v4 = c;
          v20 = (*v38__f);
          goto LABEL_44;
        }
      }
LABEL_46:
      v4 = c;
      if ( v23 != 0 && W_char_hit_fresh(c, (int)v23, (*v32__f)) != 0 && (!W_char_hit_kind_ok(c, v23[3], v23[4]) || (*v34__f) == 0) )
        return 0;
LABEL_50:
      v14 = (*v36__f) + 1;
      v15 = F32(W, 0x2788);
      (*v36__f) = v14;
      if ( (uint16_t)v14 < F16(v15, 8) )
      {
        SETLO(v10, (*v32__f));
        continue;
      }
      break;
    }
    v3 = a3;
    v10 = (*v32__f);
LABEL_52:
    if ( *((uint16_t *)v6 + 36) != F32(v3, 4) )
      break;
    ++v10;
    v5 = (int)v6;
    (*v32__f) = v10;
  }
  while ( (uint16_t)v10 < a2 );
  return 1;
}

/* 0x481310: int __thiscall sub_481310(cls_4022A0 *this, _DWORD *a2, int a3, int a4) */
int32_t beh_481310(wl_char *self, int32_t * a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    int32_t * v5;
    int v6;
    char * v7;
    intptr_t i;
    char * v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    uint16_t v13;
    int32_t * v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    uint16_t v19;
    int32_t * v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    char * v26;
    intptr_t v27;
    intptr_t v28;
    int v29;
    intptr_t v31;
    intptr_t v32;
    intptr_t v33;
    _Alignas(8) uint8_t frame_[0x624 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v36 = (uint8_t *)(frame_ + 0x10);   /* [16] at ebp-0x614 */
    intptr_t *v44 = (intptr_t *)(frame_ + 0x3c);   /* [8] at ebp-0x5e8 */
    uint8_t *v47 = (uint8_t *)(frame_ + 0xfc);   /* [656] at ebp-0x528 */
    intptr_t *v49 = (intptr_t *)(frame_ + 0x390);   /* [165] at ebp-0x294 */
    int16_t *const v34__f = (int16_t *)(frame_ + 0x0);   /* ebp-0x624 */
    int16_t *const v35__f = (int16_t *)(frame_ + 0x8);   /* ebp-0x61c */
    int32_t * *const v37__f = (int32_t * *)(frame_ + 0x20);   /* ebp-0x604 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x600 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x5fc */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x5f8 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x5f4 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x5f0 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x5ec */
    char *const v45__f = (char *)(frame_ + 0x5c);   /* ebp-0x5c8 */
    char *const v46__f = (char *)(frame_ + 0xac);   /* ebp-0x578 */
    char * *const v48__f = (char * *)(frame_ + 0x38c);   /* ebp-0x298 */

  v5 = 0;
  v6 = F16(c, 0x1428a) == 0;
  (*v38__f) = 0;
  (*v39__f) = 0;
  if ( v6 )
  {
    v7 = 0;
    (*v37__f) = (int *)F32(c, 0x40);
    SETLO(i, F16(c, 0x14228));
    (*v48__f) = &(*v45__f);
    v49[164] = (int)&(*v46__f);
    if ( (uint16_t)i != 0 )
    {
      v9 = &GAP(c, 0xca26, 2);
      for ( i = (uint16_t)i; i != 0; --i )
      {
        if ( F32(v9, 4 * 3) == 3 )
          v7 = v9;
        v9 += 320;
      }
      if ( v7 != 0 )
      {
        v10 = 1;
        do
        {
          v6 = v5 == (int *)v47;
          v5 = v49;
          if ( !v6 )
            v5 = (int *)v47;
          W_cmd_clear((int)v44);
          (*v35__f) = F32(v7, 4 * 2);
          (*v34__f) = F32(v7, 4 * 1);
          v44[4] = 14;
          beh_479e40((int)v44, (*v34__f), 210, (*v35__f), 0);
          W_char_integrate(c, (int)v44, (int)(*v37__f), (int)v5, 2, v10);
          if ( W_char_hazard_probe(&F32(c, 0x0), /*edx*/ v10, (int)v5, 0) != 0 )
            goto LABEL_32;
          if ( *((uint16_t *)v5 + 36) != F32(v7, 4 * 1) )
            break;
          ++v10;
          (*v37__f) = v5;
        }
        while ( (uint16_t)v10 < 0x40u );
        v12 = F32(c, 0x1428c);
        (*v38__f) = 1;
        if ( v12 == 0 )
        {
          v14 = a2;
          goto LABEL_34;
        }
        v13 = 0;
        while ( F16(F32(v12 + (26) * 0x48, 0x0) + 2 * v13, 1096) == 0 || ((int32_t *)D(0x628764))[35 * v13] == 0 )
        {
          if ( ++v13 >= 0x12u )
            goto LABEL_19;
        }
        v14 = a2;
        v15 = a2[1] + a2[3] / 2;
        v16 = a2[2] + a2[4] / 2;
        beh_481790(v12, 0, v15, v16, &(*v37__f), &(*v41__f), &(*v43__f), &(*v42__f));
        if ( v15 >= (int)(*v37__f) )
        {
          if ( v36 != (_BYTE *)-16 )
          {
            v17 = a2[1] - (*v42__f);
            goto LABEL_26;
          }
        }
        else if ( v36 != (_BYTE *)-16 )
        {
          v17 = (*v43__f) - a2[3] - a2[1];
          goto LABEL_26;
        }
        v17 = (int)(*v37__f);
LABEL_26:
        if ( v36 == (_BYTE *)-32 )
          v18 = (*v41__f);
        else
          v18 = (*v41__f) - v16;
        if ( (int)abs32(v17) < 9600 && (int)abs32(v18) < 9600 )
        {
          v19 = F16(c, 0xc89e);
          (*v38__f) = 0;
          F16(c, 0x1428a) = v19;
        }
        goto LABEL_34;
      }
    }
LABEL_32:
    v14 = a2;
    F16(c, 0x1428a) = F16(c, 0xc89e);
  }
  else
  {
LABEL_19:
    v14 = a2;
  }
LABEL_34:
  if ( F16(c, 0x14288) == 1 )
  {
    v20 = beh_47ecd0(c, F32(c, 0x1427c), 1, (int)v14, a3, (*v38__f), 1, 0, &(*v40__f), &(*v39__f));
  }
  else
  {
    if ( F16(c, 0x14288) != 2 )
      return 0;
    v20 = beh_47ecd0(c, F32(c, 0x1427c), 1, (int)v14, a3, (*v38__f), 0, **(_WORD **)F32(c, 0x14298), &(*v40__f), &(*v39__f));
    if ( v20 != 0 )
      goto LABEL_40;
    F16(c, 0x14288) = 0;
  }
  if ( v20 == 0 )
    return 0;
LABEL_40:
  if ( v20[3] == 3 )
  {
    v32 = (*v40__f);
    v33 = (*v39__f);
    F16(c, 0x1422c) = 6;
    F16(c, 0x1422e) = 0;
    F16(c, 0x14230) = 0;
    F32(c, 0x14818) = (int)v20;
    F32(c, 0x1481c) = v32;
    F32(c, 0x14820) = v33;
    return 1;
  }
  if ( a4 == 0 )
  {
    v31 = (*v40__f);
    F16(c, 0x1422c) = 3;
    F16(c, 0x1422e) = 0;
    F16(c, 0x14230) = 0;
    F32(c, 0x14818) = (int)v20;
    F32(c, 0x1481c) = v31;
    return 1;
  }
  v21 = F32(c, 0x1427c);
  v22 = v20[2];
  v23 = v20[1];
  F16(c, 0x14870) = 210;
  F16(c, 0x14868) = v23;
  F16(c, 0x1486a) = v22;
  F32(c, 0x1486c) = v21;
  beh_481a20(c, F16(c, 0xc878), 0x32u);
  v24 = 0;
  v6 = F16(c, 0x14228) == 0;
  (*v37__f) = 0;
  if ( v6 )
    return 1;
  do
  {
    v25 = 320 * (uint16_t)v24;
    v26 = &GAP(c, 0xca26, v25 + 2);
    if ( *(_DWORD *)&GAP(c, 0xca26, v25 + 6) == v20[1] )
    {
      v27 = *(uint16_t *)&GAP(c, 0xca26, v25 + 284);
      v28 = rnd();
      v24 = (int)(*v37__f);
      F16(v26, 2 * 140) = v27 * (300 * v28 / 0x8000) / 1000 + 700 * v27 / 0x3E8u;
    }
    v29 = (uint16_t)++v24 < F16(c, 0x14228);
    (*v37__f) = (int *)v24;
  }
  while ( v29 );
  return 1;
}

/* 0x481ac0: int __thiscall sub_481AC0(int this, _DWORD *a3, int a4) */
int32_t beh_481ac0(wl_char *self, int32_t * a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    int32_t * v8;
    intptr_t v9;
    intptr_t v10;
    int v11;
    intptr_t v12;
    intptr_t v13;
    int32_t * v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    int32_t * v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    int32_t * v26;
    intptr_t v27;
    uint16_t v28;
    intptr_t v29;
    uint16_t v30;
    intptr_t result;
    intptr_t v32;
    intptr_t v33;
    int32_t * v34;
    intptr_t v35;
    int32_t * v36;
    intptr_t v37;
    intptr_t v38;
    uint16_t v39;
    intptr_t v40;
    intptr_t v41;
    intptr_t v42;
    _Alignas(8) uint8_t frame_[0x614 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v45 = (uint8_t *)(frame_ + 0x8);   /* [16] at ebp-0x60c */
    intptr_t *v51 = (intptr_t *)(frame_ + 0x2c);   /* [5] at ebp-0x5e8 */
    uint8_t *v58 = (uint8_t *)(frame_ + 0xec);   /* [656] at ebp-0x528 */
    intptr_t *v60 = (intptr_t *)(frame_ + 0x380);   /* [165] at ebp-0x294 */
    uint16_t *const v44__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x614 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x614 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x5fc */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x5f8 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x5f4 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x5f0 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x5ec */
    int16_t *const v52__f = (int16_t *)(frame_ + 0x40);   /* ebp-0x5d4 */
    int16_t *const v53__f = (int16_t *)(frame_ + 0x42);   /* ebp-0x5d2 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x5d0 */
    int16_t *const v55__f = (int16_t *)(frame_ + 0x48);   /* ebp-0x5cc */
    char *const v56__f = (char *)(frame_ + 0x4c);   /* ebp-0x5c8 */
    char *const v57__f = (char *)(frame_ + 0x9c);   /* ebp-0x578 */
    char * *const v59__f = (char * *)(frame_ + 0x37c);   /* ebp-0x298 */

  (*v49__f) = F32(F32(c, 83992), 0);
  (*v46__f) = c + 84052;
  F32(c, 84056) = 0;
  F32(c, 84052) = 0;
  F32(c, 84060) = 0;
  F32(c, 84064) = 0;
  F16(c, 84072) = 0;
  F32(c, 84068) = 0;
  F16(c, 84074) = 0;
  F32(c, 84076) = 0;
  F16(c, 84080) = 0;
  v5 = F32(c, 84000);
  if ( v5 == 0 || F32(v5, 464) != 1 || F8(F32(v5, 64), 0) != 1 )
    goto LABEL_99;
  v6 = 1;
  if ( F16(c, 82478) != 0 )
  {
    v7 = F32(c, 83992);
    if ( *((uint16_t *)a3 + 36) != F32(v7, 4) )
    {
LABEL_100:
      F16(c, 82476) = 0;
      F16(c, 82478) = 0;
      F16(c, 82480) = 0;
      return 0;
    }
    if ( F16(a3, 2 * 43) < 0xDCu && F16(F32(c, 1872), 1124) == 0 )
    {
      v8 = 0;
      (*v47__f) = F32(c, 64);
      (*v59__f) = &(*v56__f);
      v60[164] = (int)&(*v57__f);
      if ( v7 != 0 )
        goto LABEL_103;
      if ( F16(c, 82472) != 0 )
      {
        v9 = c + 51752;
        v10 = F16(c, 82472);
        do
        {
          if ( F32(v9, 12) == 3 )
            v7 = v9;
          v9 += 320;
          --v10;
        }
        while ( v10 != 0 );
        if ( v7 != 0 )
        {
LABEL_103:
          while ( 1 )
          {
            v11 = v8 == (int *)v58;
            v8 = v60;
            if ( !v11 )
              v8 = (int *)v58;
            v12 = F32(v7, 4);
            memset(v51, 0, 16);
            (*v52__f) = 0;
            (*v53__f) = 0;
            (*v54__f) = 0;
            (*v55__f) = 0;
            (*v43__f) = F32(v7, 8);
            v51[4] = 14;
            beh_479e10((int)v51, v12, (*v43__f), 0);
            W_char_integrate(c, (int)v51, (*v47__f), (int)v8, 2, v6);
            if ( W_char_hazard_probe((int *)c, /*edx*/ v6, (int)v8, 0) != 0 )
              break;
            if ( *((uint16_t *)v8 + 36) == F32(v7, 4) )
            {
              ++v6;
              (*v47__f) = (int)v8;
              if ( (uint16_t)v6 < 0x20u )
                continue;
            }
            goto LABEL_20;
          }
        }
      }
      v11 = F16(c, 82568) == 2;
      F16(c, 82570) = F16(c, 51360);
      if ( v11 )
        F16(c, 82568) = 0;
      goto LABEL_99;
    }
  }
LABEL_20:
  v14 = F32(c, 83992);
  v15 = 0;
  v16 = 1;
  v17 = *v14;
  v18 = F32(*v14, 724);
  if ( v18 <= 0 )
  {
    v22 = 1000;
  }
  else
  {
    v19 = F32(c, 1872);
    v20 = v19[23];
    if ( v18 > v20 )
      goto LABEL_99;
    v21 = v19[34] + v19[40] + v19[24];
    if ( v21 <= 0 )
      v22 = 0;
    else
      v22 = 1000 * v20 / v21;
  }
  if ( F16(v17, 174) == 1 )
  {
    v23 = F16(c, 84008);
  }
  else
  {
    if ( F16(v17, 174) == 2 )
    {
      v16 = 0;
      v23 = F16(c, 84010);
      v15 = 1;
      goto LABEL_34;
    }
    v23 = F16(c, 84012);
  }
  if ( F16(c, 84018) != 0 )
  {
LABEL_99:
    F32(c, 84068) = 15;
    goto LABEL_100;
  }
LABEL_34:
  if ( F32(c, 496) == 0 )
  {
    switch ( v23 )
    {
      case 1:
        v24 = 1200;
        v25 = 800;
        if ( v15 == 0 )
        {
          if ( v22 >= 100 )
          {
            if ( v22 < 300 )
            {
              v24 = 1500;
              v25 = 1000;
            }
          }
          else
          {
            v24 = 1800;
            v25 = 1200;
          }
        }
        goto LABEL_56;
      case 2:
        v24 = 800;
        v25 = 400;
        if ( v15 == 0 )
        {
          if ( v22 >= 100 )
          {
            if ( v22 >= 300 )
            {
              if ( v22 < 500 )
              {
                v24 = 1000;
                v25 = 600;
              }
            }
            else
            {
              v24 = 1200;
              v25 = 800;
            }
          }
          else
          {
            v24 = 1600;
            v25 = 1000;
          }
        }
        goto LABEL_56;
      case 3:
        v24 = 600;
        v25 = 300;
        if ( v15 == 0 )
        {
          if ( v22 >= 100 )
          {
            if ( v22 < 300 )
            {
              v24 = 800;
              v25 = 400;
            }
          }
          else
          {
            v24 = 1000;
            v25 = 600;
          }
        }
        goto LABEL_56;
      case 4:
        v24 = 300;
        v25 = 200;
        goto LABEL_56;
      case 5:
        v24 = 200;
        v25 = 50;
        goto LABEL_56;
      default:
        goto LABEL_99;
    }
  }
  v24 = 500;
  v25 = 500;
LABEL_56:
  if ( v16 == 0 )
    v25 = 0;
  if ( v45 != (_BYTE *)-20 )
    (*v47__f) = v24;
  if ( v45 != (_BYTE *)-32 )
    (*v50__f) = v25;
  if ( F32((*v49__f), 724) <= 0 )
  {
LABEL_70:
    v26 = a3;
  }
  else
  {
    v26 = a3;
    v27 = *((uint16_t *)a3 + 51);
    (*v48__f) = F16((*v49__f), 174);
    if ( F32(c, 88668) != 0 )
    {
      v28 = 0;
      while ( 1 )
      {
        v29 = *(_DWORD *)&GAP(W, 0x1030, 4 * v28 + 304);
        if ( v29 != c
          && W_char_targetable(v29, 0, 1)
          && W_faction_rel(c, F32(c, 476), F32(v29, 476)) == 2
          && beh_48bdd0(v29, (*v48__f), v27) != 0 )
        {
          goto LABEL_99;
        }
        if ( ++v28 >= 0x20u )
          goto LABEL_70;
      }
    }
  }
  v30 = F16(c, 82478);
  result = v30;
  switch ( v30 )
  {
    case 0u:
      result = F32(c, 83996);
      if ( result == 99 || v26[11] == result )
      {
        if ( F16(v26, 2 * 36) == 0 )
        {
          F16(c, 82478) = v30 + 1;
          goto LABEL_77;
        }
      }
      else
      {
        result = 2 - (result != 1);
        F32((*v46__f), 0) = result;
      }
      return result;
    case 1u:
LABEL_77:
      v32 = F32(c, 83992);
      v33 = F32(c, 84000);
      F32(c, 84068) = 14;
      result = beh_479e40((*v46__f), F32(v32, 4), 210, F32(v32, 8), v33);
      ++F16(c, 82478);
      return result;
    case 2u:
      F32(c, 84068) = 14;
      if ( F16(v26, 2 * 51) >= 0x3E8u )
        F16(c, 82478) = v30 + 1;
      return result;
    case 3u:
      v34 = F32(c, 83992);
      v35 = F16(*v34, 174);
      if ( (uint16_t)v35 != 1 )
      {
        if ( (uint16_t)v35 > 1u && F16(*v34, 174) <= 3u )
        {
          if ( beh_47eae0(c, v34, &(*v46__f), 0, 0) == 0 )
            goto LABEL_99;
          v36 = F32(c, 83992);
          (*v46__f) = 50 * (*v46__f) / 100;
          if ( beh_47a970(c, *v36, &(*v48__f), 0, 0, 1, F32(c, 84000), (*v46__f)) == 0
            || (*v48__f) < (*v46__f) )
          {
            goto LABEL_99;
          }
        }
      }
      else if ( (F16(c, 82480) & 7) != 0
             || (v37 = beh_47fd40(c, a3, (int)v34, 1, 0, a3[11], 1)) < (*v47__f) )
      {
        v42 = F32(c, 83992);
        F32(c, 84068) = 14;
        result = beh_47fd40(c, a3, v42, 0, 0, a3[11], 1);
        if ( result >= (*v47__f) )
          ++F16(c, 82480);
        else
          F16(c, 82480) += 4;
        if ( F16(c, 82480) > 0x28u )
          goto LABEL_99;
        return result;
      }
      v38 = F32(c, 83992);
      v39 = 0;
      (*v44__f) = F16(c, 51320);
      F32(c, 84068) = 0;
      F16(c, 82476) = 0;
      F16(c, 82478) = 0;
      F16(c, 82480) = 0;
      (*v48__f) = v38;
      beh_481a20(c, (*v44__f), 0x32u);
      if ( F16(c, 82472) != 0 )
      {
        do
        {
          v40 = 320 * v39;
          if ( F32(v40 + c, 51756) == F32((*v48__f), 4) )
          {
            v41 = F16(v40 + c, 52034);
            F16(v40 + c, 52032) = 700 * v41 / 1000 + v41 * (300 * rnd() / 0x8000) / 1000;
          }
          ++v39;
        }
        while ( v39 < F16(c, 82472) );
      }
      result = F32((*v49__f), 724);
      if ( result > 0 )
        F16(c, 84018) = (*v50__f);
      return result;
    default:
      return result;
  }
}

/* 0x482240: int __thiscall sub_482240(cls_482240 *this, int a2) */
int32_t beh_482240(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3;
    intptr_t result;
    intptr_t v5;
    uint16_t i;
    char * v7;
    uint16_t v8;

  v3 = F16(c, 0xc878);
  result = 1000 * rnd() / 0x8000;
  if ( v3 > result )
  {
    v5 = (int)(50 * (unsigned int)F16(c, 0xc87c) / 0x3E8) / 2;
    result = v5 + (uint16_t)v5 * rnd() / 0x8000;
    F16(c, 0x14234) = result;
  }
  for ( i = 0; i < F16(c, 0x14228); ++i )
  {
    result = 320 * i;
    v7 = &GAP(c, 0xc87e, result + 426);
    if ( *(_DWORD *)&GAP(c, 0xc87e, result + 430) == F32(a2, 4) )
    {
      v8 = *(_WORD *)&GAP(c, 0xc87e, result + 708);
      result = 700 * v8 / 1000 + v8 * (300 * rnd() / 0x8000) / 1000;
      F16(v7, 2 * 140) = result;
    }
  }
  return result;
}

/* 0x482380: int __stdcall sub_482380(int a1, int a2, int a3, int a4, _DWORD *a5, _WORD *a6, int a7) */
int32_t beh_482380(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, uint16_t * a6, int32_t a7)
{

    intptr_t v7;
    intptr_t v9;
    intptr_t v10;
    uint16_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v15;

  SETHI(v7, 0);
  if ( *a6 != 0 )
  {
    SETLO(v7, *a6);
    if ( v7 != 1 )
      return 0;
    goto LABEL_20;
  }
  if ( a7 != 99 && F32(a3, 44) != a7 )
  {
    F32(a4, 0) = 2 - (a7 != 1);
    return 0;
  }
  v9 = *a5;
  v10 = F32(*a5, 748);
  if ( v10 != 0 )
  {
    v11 = F16(a3, 100);
    if ( v11 < F16(v9, 752) || v11 > F16(v9, 754) )
      return 0;
    v12 = F16(a3, 72);
    if ( (uint16_t)v12 != v10 )
    {
      if ( v10 != 10101 )
        return 0;
      v13 = v12 - 10115;
      if ( !(v13 != 0 ? v13 == 7 : F16(a3, 84) == 0) )
        return 0;
    }
  }
  else if ( F16(a3, 72) != 0 && F16(a3, 86) > 0xC8u )
  {
    return 0;
  }
  *a6 = 1;
LABEL_20:
  v15 = a5[1];
  F16(a4, 22) = a5[2];
  F16(a4, 20) = v15;
  F16(a4, 28) = 210;
  F32(a4, 24) = a1;
  return 1;
}

/* 0x482460: int __thiscall sub_482460(cls_4022A0 *this, int a2) */
int32_t beh_482460(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    int32_t * v4;
    uint16_t v5;
    intptr_t result;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    int32_t * v12;
    intptr_t v13;
    intptr_t v14;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x30 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0xc */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x4 */

  v3 = F32(c, 0x1427c);
  v4 = (_DWORD *)F32(c, 0x40);
  if ( v3 == 0 )
    return 0;
  v5 = F16(c, 0x147f4);
  if ( v5 != 0 && a2 == 0 )
  {
    F16(c, 0x147f4) = v5 - 1;
    return 0;
  }
  (*v15__f) = F32(c, 0xc834);
  F16(c, 0x147f4) = 8;
  beh_47e7c0(v3, 0, v4, (*v15__f), &a2, &(*v18__f), &(*v17__f), 0);
  v7 = v4[161];
  v8 = a2;
  v9 = a2;
  if ( v7 != 0 )
  {
    if ( !beh_4826b0(v7, (_DWORD *)F32(c, 0x14238)) )
    {
      W_behave_set_state(c, 7);
      F32(c, 0x14824) = v9;
      return 0;
    }
    v8 = a2;
  }
  v10 = 9;
  if ( F32(c, 0x1f0) == 0 )
    v10 = F32(c, 0x14240);
  switch ( v10 )
  {
    case 1:
      if ( !beh_482700(c, v8, 0, 8000, 0) )
        return 0;
      goto LABEL_22;
    case 2:
    case 4:
    case 8:
    case 9:
    case 10:
      goto LABEL_22;
    case 3:
    case 11:
      if ( !beh_482760(c, v8, 0, 12800, 0, 1) )
        return 0;
      goto LABEL_22;
    case 5:
      if ( F32(c, 0x14264) + 40000 < v8 || F32(c, 0x14268) - 40000 > v8 )
        return 0;
      goto LABEL_22;
    case 6:
      if ( F32(c, 0x14264) < v8 )
        return 0;
      if ( F32(c, 0x14268) > v8 )
        return 0;
LABEL_22:
      v11 = F32(c, 0x1594c);
      if ( v11 <= 0 || v11 > 3 )
      {
        v12 = F32(v3, 64);
        v13 = v12[161];
        if ( v13 != 0 )
        {
          v14 = W_char_part_top(v13, 0) - 1;
        }
        else
        {
          W_grid_floor_below(v12, &(*v16__f));
          v14 = 3200 * (*v16__f) - 3200;
        }
        if ( beh_483040(c, v4, a2, v14, 3, 0) == 0 )
          return 0;
      }
      F16(c, 0x1422c) = 2;
      F16(c, 0x1422e) = 0;
      F16(c, 0x14230) = 0;
      F32(c, 0x147f8) = F32(c, 0xc834) + F32(c, 0xc838) * rnd() / 0x8000;
      F16(c, 0x1429c) = 0;
      F16(c, 0x147a0) = 0;
      F32(c, 0x1423c) = F32(c, 0xc83c);
      result = 1;
      break;
    default:
      return 0;
  }
  return result;
}

/* 0x482880: int __thiscall sub_482880(cls_4022A0 *this, int a2, int a3, int a4) */
int32_t beh_482880(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    int32_t * v13;
    intptr_t v14;
    intptr_t v15;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x10 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x4 */

  v5 = 20 * rnd();
  if ( a4 == 0 )
    goto LABEL_27;
  v6 = 660 * (uint16_t)(v5 / 0x8000);
  beh_47e7c0(a4, v5 / 0x8000, (_DWORD *)(v6 + F32(c, 0x40)), F32(c, 0x147f8), &(*v17__f), &(*v20__f), &(*v19__f), 0);
  v7 = (*v17__f);
  v8 = (*v17__f);
  v9 = F32(a2, 644);
  if ( v9 == 0 )
  {
LABEL_6:
    v10 = 9;
    if ( F32(c, 0x1f0) == 0 )
      v10 = F32(c, 0x14240);
    switch ( v10 )
    {
      case 1:
        if ( !beh_482700(c, v7, 0, 8000, 0) )
          goto LABEL_27;
        v7 = (*v17__f);
        break;
      case 2:
      case 4:
      case 8:
      case 9:
      case 10:
        break;
      case 3:
      case 11:
        if ( !beh_482760(c, v7, 0, 12800, 0, 1) )
          goto LABEL_27;
        v7 = (*v17__f);
        break;
      case 5:
        if ( F32(c, 0x14264) + 40000 < v7 || F32(c, 0x14268) - 40000 > v7 )
          goto LABEL_27;
        break;
      case 6:
        if ( F32(c, 0x14264) < v7 || F32(c, 0x14268) > v7 )
          goto LABEL_27;
        break;
      default:
        goto LABEL_27;
    }
    v11 = F32(c, 0x1594c);
    if ( v11 <= 0 || v11 > 3 )
    {
      v13 = (_DWORD *)(v6 + F32(a4, 64));
      v14 = v13[161];
      if ( v14 != 0 )
      {
        v15 = W_char_part_top(v14, v5 / 0x8000) - 1;
      }
      else
      {
        W_grid_floor_below(v13, &a4);
        v15 = 3200 * a4 - 3200;
      }
      if ( beh_483040(c, (_DWORD *)a2, (*v17__f), v15, 3, &(*v18__f)) == 0 )
        goto LABEL_27;
    }
    else if ( &(*v16__f) != (int *)-20 )
    {
      (*v18__f) = v7;
    }
    return W_npc_navigate(c, (*v18__f), 800, a2, a3, 0, 0, 0, 0, 0);
  }
  if ( beh_4826b0(v9, (_DWORD *)F32(c, 0x14238)) )
  {
    v7 = (*v17__f);
    goto LABEL_6;
  }
  W_behave_set_state(c, 7);
  F32(c, 0x14824) = v8;
LABEL_27:
  F16(c, 0x1422c) = 0;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  return 0;
}

/* 0x482ac0: int __thiscall sub_482AC0(cls_4022A0 *this, _DWORD *a2) */
int32_t beh_482ac0(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    int32_t * v4;
    intptr_t v5;
    int32_t * v6;
    uint16_t v7;
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
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    int32_t * v23;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v24 = (uint8_t *)(frame_ + 0x0);   /* [16] at ebp-0x1c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    int32_t * *const v26__f = (int32_t * *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  v3 = F32(c, 0x1428c);
  if ( v3 == 0 )
    return 0;
  v4 = a2;
  v5 = a2[1] + a2[3] / 2;
  beh_481790(v3, 0, v5, a2[2] + a2[4] / 2, &a2, &(*v27__f), &(*v25__f), &(*v26__f));
  if ( v5 >= (int)a2 )
  {
    if ( v24 != (_BYTE *)-32 )
    {
      a2 = (*v26__f);
      v6 = (_DWORD *)(v4[1] - (int32_t)(*v26__f));
      goto LABEL_8;
    }
LABEL_7:
    v6 = a2;
    goto LABEL_8;
  }
  if ( v24 == (_BYTE *)-32 )
    goto LABEL_7;
  a2 = (_DWORD *)((*v25__f) - v4[3]);
  v6 = (_DWORD *)((*v25__f) - v4[3] - v4[1]);
LABEL_8:
  if ( F32(c, 0xc880) == 0 )
    return 0;
  v7 = F16(c, 0x14802);
  if ( v7 != 0 )
  {
    F16(c, 0x14802) = v7 - 1;
    return 0;
  }
  if ( F32(c, 0xc884) <= (int)v6 )
    return 0;
  v9 = v4[1];
  v10 = F32(c, 0xc888) <= (int)v6 ? 1600 : 4800;
  v11 = F32(c, 0x1594c);
  if ( v9 < (int)a2 )
  {
    v12 = v11 <= 0 || v11 >= 4;
    v13 = W_npc_floor_ahead(c, v4, v9 - v10, 1, v12, 5);
    if ( !v13 )
    {
      v14 = 1;
      goto LABEL_25;
    }
    goto LABEL_24;
  }
  v15 = v11 <= 0 || v11 >= 4;
  v13 = W_npc_floor_ahead(c, v4, v10 + v9 + v4[3] - 1, 1, v15, 5);
  v14 = 3;
  if ( v13 )
LABEL_24:
    v14 = (int)a2;
LABEL_25:
  if ( F32(c, 0xc888) <= (int)v6 )
  {
    if ( v13 )
      return 0;
  }
  else if ( v13 )
  {
    v16 = v4[1];
    v17 = F32(c, 0x1594c);
    if ( v16 < (int)a2 )
    {
      v18 = v17 <= 0 || v17 >= 4;
      if ( W_npc_floor_ahead(c, v4, v10 + v16 + v4[3] - 1, 1, v18, 5) )
        return 0;
      goto LABEL_33;
    }
    v21 = v17 <= 0 || v17 >= 4;
    if ( W_npc_floor_ahead(c, v4, v16 - v10, 1, v21, 5) )
      return 0;
    goto LABEL_45;
  }
  if ( v14 != 1 )
  {
LABEL_33:
    F32(c, 0x147fc) = F32(F32((F32(c, 0x1428c) + 64), 0), 4) - F32(c, 0xc88c);
    goto LABEL_34;
  }
LABEL_45:
  F32(c, 0x147fc) = F32(F32((F32(c, 0x1428c) + 64), 0), 4)
                    + F32(F32((F32(c, 0x1428c) + 64), 0), 12)
                    + F32(c, 0xc88c);
LABEL_34:
  v19 = v4[161];
  v20 = F32(c, 0x147fc);
  if ( v19 != 0 && !beh_4826b0(v19, (_DWORD *)F32(c, 0x14238)) )
  {
    W_behave_set_state(c, 7);
    F32(c, 0x14824) = v20;
    return 0;
  }
  v22 = F32(c, 0x1594c);
  if ( v22 > 0 && v22 <= 3 )
  {
    F16(c, 0x1422c) = 4;
    F16(c, 0x1422e) = 0;
    F16(c, 0x14230) = 0;
    F16(c, 0x14800) = 120;
    return 1;
  }
  if ( beh_483040(
         c,
         v4,
         F32(c, 0x147fc),
         F32(F32((F32(c, 0x1428c) + 64), 0), 16)
       + F32(F32((F32(c, 0x1428c) + 64), 0), 8)
       - 1,
         3,
         (int *)&a2) != 0 )
  {
    W_behave_set_state(c, 4);
    v23 = a2;
    F16(c, 0x14800) = 120;
    F32(c, 0x147fc) = (int)v23;
    return 1;
  }
  F16(c, 0x14802) = 60;
  return 0;
}

/* 0x482e10: void __thiscall sub_482E10(cls_4022A0 *this, int a2, int a3) */
void beh_482e10(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    int32_t * v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x28 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x4 */

  if ( F32(c, 0x1428c) != 0 )
  {
    v4 = a3;
    v5 = a2;
    v6 = F32(c, 0x147fc);
    --F16(c, 0x14800);
    W_npc_navigate(c, v6, 800, v5, v4, 1, 0, 0, 0, 0);
    if ( (int)abs32(F32(v5, 4) - F32(c, 0x147fc)) <= 800 || F16(c, 0x14800) == 0 )
    {
      v7 = (_DWORD *)F32(c, 0x40);
      v8 = F32(c, 0x1427c);
      if ( v8 == 0 )
        goto LABEL_14;
      (*v16__f) = F32(c, 0xc834);
      F16(c, 0x147f4) = 8;
      beh_47e7c0((int)v8, 0, v7, (*v16__f), &a3, &(*v17__f), &a2, 0);
      if ( beh_484830(c, (int)v7, a3) != 0 )
        goto LABEL_14;
      switch ( beh_485330(c) )
      {
        case 1:
          if ( !beh_482700(c, a3, 0, 8000, 0) )
            goto LABEL_14;
          break;
        case 2:
        case 4:
        case 8:
        case 9:
        case 10:
          break;
        case 3:
        case 11:
          if ( !beh_482760(c, a3, 0, 12800, 0, 1) )
            goto LABEL_14;
          break;
        case 5:
          if ( F32(c, 0x14264) + 40000 < a3 || F32(c, 0x14268) - 40000 > a3 )
            goto LABEL_14;
          break;
        case 6:
          if ( F32(c, 0x14264) < a3 || F32(c, 0x14268) > a3 )
            goto LABEL_14;
          break;
        default:
          goto LABEL_14;
      }
      if ( (v9 = F32(c, 0x1594c)) > 0 && v9 <= 3
        || (v10 = beh_484d60(v8, 0), beh_483040(c, v7, a3, v10, 3, 0) != 0) )
      {
        W_behave_set_state(c, 2);
        v11 = rnd();
        v12 = F32(c, 0xc834);
        v13 = F32(c, 0xc838) * v11;
        F16(c, 0x1429c) = 0;
        F16(c, 0x147a0) = 0;
        v14 = v12 + v13 / 0x8000;
        v15 = F32(c, 0xc83c);
        F32(c, 0x147f8) = v14;
        F32(c, 0x1423c) = v15;
        F16(c, 0x14802) = 200;
      }
      else
      {
LABEL_14:
        F16(c, 0x1422c) = 0;
        F16(c, 0x1422e) = 0;
        F16(c, 0x14230) = 0;
        F16(c, 0x14802) = 200;
      }
    }
  }
  else
  {
    F16(c, 0x1422c) = 0;
    F16(c, 0x1422e) = 0;
    F16(c, 0x14230) = 0;
  }
}

/* 0x4834f0: int __thiscall sub_4834F0(cls_4022A0 *this, int a2, int a3) */
int32_t beh_4834f0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    int32_t * v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    char * v8;
    intptr_t v9;
    uint16_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    uint16_t v16;
    char * v17;
    intptr_t v18;
    int32_t * v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    int v25;
    intptr_t v26;
    _Alignas(8) uint8_t frame_[0x610 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v38 = (intptr_t *)(frame_ + 0x28);   /* [8] at ebp-0x5e8 */
    uint8_t *v41 = (uint8_t *)(frame_ + 0xe8);   /* [656] at ebp-0x528 */
    intptr_t *v43 = (intptr_t *)(frame_ + 0x37c);   /* [165] at ebp-0x294 */
    uint16_t *const v27__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x610 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x610 */
    uint16_t *const v29__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x60c */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x608 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x604 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x600 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x5fc */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x5f8 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x5f4 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x5f0 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x5ec */
    char *const v39__f = (char *)(frame_ + 0x48);   /* ebp-0x5c8 */
    char *const v40__f = (char *)(frame_ + 0x98);   /* ebp-0x578 */
    char * *const v42__f = (char * *)(frame_ + 0x378);   /* ebp-0x298 */

  if ( a2 == 0 && F16(c, 0x158bc) >= F16(c, 0xc7e8) )
    return 0;
  if ( F32(c, 0x1427c) == 0 )
    return 0;
  if ( F16(c, 0xca24) == 0 )
    return 0;
  if ( F16(c, 0x14234) != 0 )
    return 0;
  if ( F16(c, 0x14804) != 0 )
    return 0;
  F16(c, 0x14804) = 10;
  if ( F32(c, 0xc844) <= 1000 * rnd() / 0x8000 )
    return 0;
  v4 = (int *)a3;
  v5 = F32(a3, 644);
  if ( v5 != 0 && v5 == F32(c, 0x14238) && F32(c, 0xc870) == 10 )
    return 0;
  (*v27__f) = 0;
  if ( F16(c, 0x14228) == 0 )
    return 0;
  while ( 1 )
  {
    v6 = 320 * (*v27__f);
    v7 = *(_DWORD *)&GAP(c, 0xca26, v6 + 2);
    v8 = &GAP(c, 0xca26, v6 + 2);
    if ( W_char_skill_ok(c, v7, 1) == 0 )
      goto LABEL_32;
    v9 = F32(v7, 748);
    if ( v9 == 0 )
      break;
    v10 = F16(a3, 100);
    if ( v10 >= F16(v7, 752) && v10 <= F16(v7, 754) )
    {
      v11 = F16(a3, 72);
      if ( (uint16_t)v11 == v9 )
        goto LABEL_25;
      if ( v9 == 10101 )
      {
        v12 = v11 - 10115;
        if ( v12 != 0 )
        {
          if ( v12 == 7 )
            goto LABEL_25;
        }
        else if ( F16(a3, 84) == 0 )
        {
          goto LABEL_25;
        }
      }
    }
LABEL_32:
    if ( ++(*v27__f) >= F16(c, 0x14228) )
      return 0;
  }
  if ( F16(a3, 72) != 0 && F16(a3, 86) > 0xC8u )
    goto LABEL_32;
LABEL_25:
  v13 = F32(v7, 188) - 1;
  if ( v13 == 0 || v13 == 2 || F16(v8, 2 * 140) != 0 )
    goto LABEL_32;
  if ( F16(v8, 2 * 158) != 0 || F16(v8, 2 * 159) != 0 )
  {
    v14 = 1000
        * F32(F32(c, 0x750), 84)
        / (F32(F32(c, 0x750), 88) + F32(F32(c, 0x750), 132) + F32(F32(c, 0x750), 156));
    if ( v14 < *((uint16_t *)v8 + 158) || v14 > *((uint16_t *)v8 + 159) )
      goto LABEL_32;
  }
  v16 = 0;
  (*v28__f) = 0;
  if ( F16(c, 0xca24) == 0 )
    return 0;
  while ( 2 )
  {
    v17 = &GAP(c, 0xc8a2, 48 * v16 + 2);
    (*v30__f) = (int)v17;
    if ( v4[10] < F32(v17, 0)
      || 1000 * rnd() / 0x8000 >= *(uint16_t *)&GAP(c, 0xc8a2, 48 * v16 + 46) )
    {
LABEL_54:
      v16 = ++(*v28__f);
      if ( (uint16_t)(*v28__f) >= F16(c, 0xca24) )
        return 0;
      continue;
    }
    break;
  }
  (*v31__f) = F32(c, 0x1427c);
  (*v33__f) = 0;
  (*v32__f) = 0;
  v18 = v4[11];
  (*v42__f) = &(*v39__f);
  v19 = 0;
  v43[164] = (int)&(*v40__f);
  W_body_probe_rect(v4, (int32_t *)v17 + 1, &(*v34__f), v18);
  v20 = W_part_left((*v31__f), 0);
  if ( v20 > (*v34__f) + (*v36__f) )
    goto LABEL_53;
  v21 = W_part_right((*v31__f), 0);
  if ( v21 <= (*v34__f) )
    goto LABEL_53;
  v22 = W_part_bottom((*v31__f), 0);
  if ( v22 > (*v35__f) + (*v37__f) )
    goto LABEL_53;
  v23 = W_part_bottom((*v31__f), 0);
  if ( v23 <= (*v35__f) )
    goto LABEL_53;
  (*v29__f) = 1;
  while ( 1 )
  {
    v24 = F16(c, 0x44) - 1;
    if ( v24 >= (*v29__f) )
      v24 = (*v29__f);
    v25 = v19 == (int *)v41;
    v19 = v43;
    if ( !v25 )
      v19 = (int *)v41;
    W_cmd_clear((int)v38);
    if ( beh_483b50((int)c, /*edx*/ (int)(*v31__f), v24, (int)v4, v38, &(*v33__f), (*v30__f)) != 0 )
      break;
    W_char_integrate(c, (int)v38, (int)v4, (int)v19, 2, v24);
    if ( W_char_hazard_probe(&F32(c, 0x0), /*edx*/ v24, (int)v19, 0) == 0 )
    {
      if ( beh_483920((int)(*v31__f), v19, (_DWORD *)((*v30__f) + 20), v24) != 0 )
        (*v32__f) = 1;
      v4 = v19;
      if ( ++(*v29__f) < 0x60u )
        continue;
    }
    goto LABEL_53;
  }
  if ( (*v32__f) == 0 )
  {
LABEL_53:
    v4 = (int *)a3;
    goto LABEL_54;
  }
  F16(c, 0x1422c) = 5;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  F32(c, 0x14808) = (*v30__f);
  F32(c, 0x1480c) = 0;
  return 1;
}

/* 0x483b50: int __thiscall sub_483B50(cls_4022A0 * this, cls_481790 *a3, int a4, _DWORD *a5, _DWORD *a6, _WORD *a7, int a8) */
int32_t beh_483b50(wl_char *self, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6, uint16_t * a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    uint16_t v8;
    int32_t * v9;
    intptr_t v10;
    intptr_t v11;
    uint16_t * v12;
    intptr_t v13;
    uint16_t v14;
    intptr_t v15;
    intptr_t v16;
    int32_t * v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    int32_t * v22;
    uint16_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    int32_t * v30;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v31 = (uint8_t *)(frame_ + 0x0);   /* [16] at ebp-0x10 */

  v8 = a4;
  v9 = a5;
  v10 = c;
  SETLO(c, *((uint8_t *)a5 + 32));
  v11 = W_entity_move_vertical(v10, a5, 1, (int)c, a4, (int)&a4, 0);
  v12 = a7;
  v13 = v11 == 0;
  switch ( *a7 )
  {
    case 0:
      v14 = v8;
      if ( v8 >= F32(a3, 0x44) )
        v14 = 0;
      v15 = *(_DWORD *)&GAP(a3, 0x24, 28) + 660 * v14;
      v16 = v9[1] + v9[3] / 2 >= F32(v15, 4) + F32(v15, 12) / 2 ? 3 : 1;
      if ( v9[11] == v16 )
      {
        v12 = a7;
        *a7 = 1;
        goto LABEL_7;
      }
      *a6 = 2 - (v16 != 1);
      return 0;
    case 1:
LABEL_7:
      switch ( F16(v9, 2 * 28) )
      {
        case 0:
        case 1:
        case 4:
          break;
        case 2:
          if ( F16(v9, 2 * 29) != 2 )
            return 0;
          break;
        default:
          return 0;
      }
      if ( !v13 && F16(v10, 0x158bc) >= F16(v10, 0xc7e8) )
        return 0;
      v18 = a6;
      *v12 = 2;
      v18[2] = 7;
      return 0;
    case 2:
      if ( F16(v9, 2 * 28) != 3 )
        return 1;
      a6[2] = 8;
      if ( v11 == 0 )
        return 0;
      *v12 = 3;
      return 0;
    case 3:
      if ( (int)v9[6] < 0 || v11 != 0 )
      {
        v19 = v9[1] + v9[3] / 2;
        v20 = a3;
        beh_481790(a3, v8, v19, v9[2] + v9[4] / 2, &a7, &a5, (int *)&a3, &a4);
        if ( v19 >= (int)a7 )
        {
          v21 = 3;
          if ( &a6 == 0 )
            v21 = (int)a6;
          if ( v31 != (_BYTE *)-32 )
          {
            v22 = (_DWORD *)(v9[1] - a4);
            goto LABEL_29;
          }
        }
        else
        {
          v21 = 1;
          if ( &a6 == 0 )
            v21 = (int)a6;
          if ( v31 != (_BYTE *)-32 )
          {
            v22 = (int *)((char *)&F32(a3, 0x0) - v9[3] - v9[1]);
LABEL_29:
            v23 = v8;
            if ( v8 >= F16(v20, 0x44) )
              v23 = 0;
            v24 = 165 * v23;
            v25 = *(_DWORD *)&GAP(v20, 0x24, 28);
            v26 = F32(v25 + 4 * v24, 16);
            v27 = v25 + 4 * v24;
            v28 = a8;
            v29 = F32(v27, 8) + v26 - F32(a8, 40);
            v30 = a6;
            if ( v29 - v9[4] - v9[2] < (int)v22 )
              a6[2] = 8;
            if ( (int)v22 > F32(v28, 36) )
              *v30 = (v21 != 3) + 5;
            return 0;
          }
        }
        v22 = a6;
        goto LABEL_29;
      }
      return 1;
    default:
      return 0;
  }
}

/* 0x483de0: int __thiscall sub_483DE0(cls_483DE0 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t beh_483de0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    uint16_t * v7;
    uint16_t v8;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  (*v13__f) = (int)abs32(F32(a2, 4) - a4) < 64000;
  v7 = &F16(c, 0x1429c);
  if ( F16(c, 0x147a0) < F16(c, 0x1429c)
    && ((int)abs32(a4 - F32(c, 0x147a4)) > 3200 || (int)abs32(a5 - F32(c, 0x147a8)) > 3200) )
  {
    *v7 = 0;
    F16(c, 0x147a0) = 0;
  }
  v8 = F16(c, 0x147ac);
  if ( v8 != 0 )
  {
    F16(c, 0x147ac) = v8 - 1;
    return beh_4848a0(c, a2, a3, a6, 0, F32(c, 0x147a4), (*v13__f));
  }
  if ( F16(c, 0x147a0) < *v7 )
    return beh_4848a0(c, a2, a3, a6, 0, F32(c, 0x147a4), (*v13__f));
  v10 = F32(c, 0x15a90);
  *v7 = 0;
  F16(c, 0x147a0) = 0;
  F32(c, 0x147a4) = a4;
  F32(c, 0x147a8) = a5;
  v11 = v10 != 0 && F32(c, 0xc7ec) == 0 && F16(F32(c, 0x750), 1118) == 0;
  if ( W_path_search(
         *(intptr_t *)&GAP(W, 0x1030, 32),
         F32(a2, 4),
         F32(a2, 8),
         F32(a2, 12),
         F32(a2, 16),
         a4,
         a5,
         1,
         -1,
         *(_DWORD *)&GAP(W, 0x1030, 24),
         &F16(c, 0x1429c),
         (int)&GAP(c, 0x1429e, 2),
         v11) != 0 )
    return beh_4848a0(c, a2, a3, a6, 0, F32(c, 0x147a4), (*v13__f));
  F16(c, 0x147ac) = 8;
  *v7 = 0;
  F16(c, 0x147a0) = 0;
  v12 = F32(a2, 644);
  if ( v12 == 0 )
    return beh_4848a0(c, a2, a3, a6, 0, F32(c, 0x147a4), (*v13__f));
  switch ( F32(v12, 448) )
  {
    case 0:
    case 1:
      if ( F32(c, 0x14238) == v12 )
        return beh_4848a0(c, a2, a3, a6, 0, F32(c, 0x147a4), (*v13__f));
      break;
    case 3:
      break;
    default:
      return beh_4848a0(c, a2, a3, a6, 0, F32(c, 0x147a4), (*v13__f));
  }
  F16(c, 0x1422c) = 7;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  F32(c, 0x14824) = a4;
  return 0;
}

/* 0x483fb0: __int16 __thiscall sub_483FB0(cls_4022A0 *this, _DWORD *a2, int a3, int a4, int a5) */
int32_t beh_483fb0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    uint16_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    uint16_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    int32_t * v22;
    intptr_t v23;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v26 = (uint8_t *)(frame_ + 0xc);   /* [16] at ebp-0x18 */
    uint16_t *v27 = (uint16_t *)(frame_ + 0x1c);   /* [2] at ebp-0x8 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x4 */

  v6 = F16(c, 0x14288) == 1;
  (*v28__f) = v6;
  if ( a4 == 0
    && (v7 = a2[1], F32(c, 0x1424c) != 0)
    && (beh_4843c0(c, &a4, v27, 0, 0, v6), (int)abs32(v7 - a4) > 12800)
    || F16(c, 0x14228) == 0
    || (v8 = beh_482ac0(c, a2)) == 0
    && ((SETLO(v8, F16(c, 0x14288)), (uint16_t)v8 == 0)
     || v8 > 2
     || (v8 = beh_482460(c, 0)) == 0
     && (v8 = beh_4834f0(c, a3, (int)a2)) == 0
     && (v8 = beh_481310(c, a2, a3, 0)) == 0) )
  {
    v9 = F32(c, 0x1424c);
    v10 = 24;
    v11 = F32(v9, 51068);
    F32(v27, 0) = 24;
    if ( W_part_facing(v9, 24) != v11 )
    {
      F32(v27, 0) = 0;
      v10 = 0;
    }
    if ( (*v28__f) != 0 )
    {
      v12 = F32(c, 0x14250);
      v13 = F32(c, 0x14244);
    }
    else
    {
      v12 = F32(c, 0x14254);
      v13 = F32(v9, 51068);
    }
    if ( v13 == 3 )
    {
      if ( v26 != (_BYTE *)-36 )
        a4 = 3;
    }
    else
    {
      v12 = -v12;
      if ( v26 != (_BYTE *)-36 )
        a4 = 1;
    }
    v14 = v10;
    if ( v10 >= F16(c, 0x44) )
      v14 = 0;
    (*v28__f) = F32(F32(c, 0x40) + 660 * v14, 12);
    v15 = W_part_center_x(v9, v10);
    v16 = v12 - (*v28__f) / 2 + v15;
    v17 = W_part_bottom(v9, v27[0]);
    v18 = F32(c, 0x1424c);
    v19 = v17;
    v20 = F32(v18, 0x1594c);
    if ( v20 != 0 )
      v21 = v20 > 0 && v20 <= 3;
    else
      v21 = W_entity_ground_probe(v18, (_DWORD *)(F32(v18, 0x40) + 15840), 24);
    if ( a5 != 0 || F16(F32(c, 0x750), 1098) == 0 )
    {
      v22 = a2;
    }
    else
    {
      v22 = a2;
      if ( F32(W, 0x1030) != 2
        && (int)abs32(a2[1] - v16) < 64000
        && (int)abs32(a2[2] - v19) < 40000
        && F16(c, 0x147a0) >= F16(c, 0x1429c) )
      {
        F32(c, 0x14858) = 0;
        F32(c, 0x14854) = 0;
        F32(c, 0x1485c) = 0;
        F32(c, 0x14860) = 0;
        F16(c, 0x14868) = 0;
        F32(c, 0x14864) = 0;
        F16(c, 0x1486a) = 0;
        F32(c, 0x1486c) = 0;
        F16(c, 0x14870) = 0;
        SETLO(v8, 0);
        return v8;
      }
    }
    v8 = F32(c, 0x1594c);
    if ( v8 != 0 )
    {
      if ( v8 > 0 && v8 <= 3 )
      {
        (*v25__f) = v16;
        v23 = a3;
        v8 = beh_442640(
               c,
               v22,
               a3,
               (*v25__f),
               F32(F32((F32(c, 0x1424c) + 64), 0), 16)
             + F32(F32((F32(c, 0x1424c) + 64), 0), 8)
             - 1,
               a4);
        if ( v8 == 0 )
          return v8;
        goto LABEL_44;
      }
    }
    else
    {
      v8 = beh_484530(c, (int)v22, a3, v16, v19, a4, v21, 0);
      if ( v8 == 0 )
        return v8;
    }
    v23 = a3;
LABEL_44:
    if ( F16(c, 0x14228) != 0 )
    {
      v8 = beh_482ac0(c, v22);
      if ( v8 == 0 )
      {
        SETLO(v8, F16(c, 0x14288));
        if ( (uint16_t)v8 != 0 && v8 <= 2 )
        {
          v8 = beh_482460(c, 0);
          if ( v8 == 0 )
          {
            v8 = beh_4834f0(c, v23, (int)v22);
            if ( v8 == 0 )
              SETLO(v8, beh_481310(c, v22, v23, 0));
          }
        }
      }
    }
  }
  return v8;
}

/* 0x484530: int __thiscall sub_484530(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t beh_484530(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    uint16_t * v9;
    uint16_t v10;
    uint16_t v11;
    uint16_t v12;
    intptr_t v13;
    char * v14;
    intptr_t v15;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    int64_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;

  v9 = &F16(c, 0x1429c);
  if ( F16(c, 0x147a0) < F16(c, 0x1429c)
    && a7 != 0
    && ((int)abs32(a4 - F32(c, 0x147a4)) > 3200 || (int)abs32(a5 - F32(c, 0x147a8)) > 3200) )
  {
    *v9 = 0;
    F16(c, 0x147a0) = 0;
  }
  v10 = F16(c, 0x147ac);
  if ( v10 != 0 )
  {
    F16(c, 0x147ac) = v10 - 1;
    goto LABEL_8;
  }
  if ( F16(c, 0x147a0) < *v9 )
    goto LABEL_8;
  v17 = false;
  if ( a7 == 0 )
    goto LABEL_8;
  v18 = F32(c, 0x15a90);
  *v9 = 0;
  F16(c, 0x147a0) = 0;
  F32(c, 0x147a4) = a4;
  F32(c, 0x147a8) = a5;
  if ( v18 != 0 && F32(c, 0xc7ec) == 0 )
    v17 = F16(F32(c, 0x750), 1118) == 0;
  if ( W_path_search(
         *(intptr_t *)&GAP(W, 0x1030, 32),
         F32(a2, 4),
         F32(a2, 8),
         F32(a2, 12),
         F32(a2, 16),
         a4,
         a5,
         1,
         -1,
         *(_DWORD *)&GAP(W, 0x1030, 24),
         &F16(c, 0x1429c),
         (int)&GAP(c, 0x1429e, 2),
         v17) != 0
    || (F16(c, 0x147ac) = 8, *v9 = 0, F16(c, 0x147a0) = 0, (v19 = F32(a2, 644)) == 0) )
  {
LABEL_8:
    v11 = F16(c, 0x147a0);
    v12 = *v9;
    v23 = 0;
    if ( v11 < *v9 )
    {
      v13 = v11;
      v14 = (uint8_t *)c + 20 * v11;
      v22 = (int)v14;
      v15 = *((uint16_t *)v14 + 41296);
      if ( (uint16_t)v15 != 0 )
      {
        if ( v15 == 1 )
        {
          beh_442710(c, v11);
          F16(c, 0x1429c) = 0;
          F16(c, 0x147a0) = 0;
          return 0;
        }
      }
      else
      {
        if ( v11 == 0 && v12 == 1 )
        {
          v20 = F32(c, 0x147a4) - F32(a2, 4);
          if ( (int)((HIDWORD(v20) ^ v20) - HIDWORD(v20)) <= 1600 )
          {
            F16(c, 0x147a0) = 1;
            goto LABEL_33;
          }
          v14 = (char *)v22;
        }
        if ( v13 == v12 - 1 )
          v21 = F32(c, 0x147a4);
        else
          v21 = 3200 * F32(v14, 4 * 20649);
        if ( W_body_near_x(a2, v21) )
          ++F16(c, 0x147a0);
        else
          W_npc_navigate(c, v21, 1600, a2, a3, 0, 0, 0, a8, 0);
      }
    }
LABEL_33:
    if ( F16(c, 0x147a0) < F16(c, 0x1429c)
      || W_npc_navigate(c, F32(a2, 4), 800, a2, a3, 0, 0, 0, a8, 0) == 0 )
    {
      return v23;
    }
    if ( a6 != 99 )
    {
      if ( F32(c, 0x14854) != 0 )
        return v23;
      if ( F32(a2, 44) != a6 )
      {
        F32(c, 0x14854) = 2 - (a6 != 1);
        return 0;
      }
    }
    return 1;
  }
  switch ( F32(v19, 448) )
  {
    case 0:
    case 1:
      if ( F32(c, 0x14238) == v19 )
        goto LABEL_8;
      break;
    case 3:
      break;
    default:
      goto LABEL_8;
  }
  F16(c, 0x1422c) = 7;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  F32(c, 0x14824) = a4;
  return 0;
}

/* 0x484ea0: __int16 __thiscall sub_484EA0(cls_4022A0 *this, int a2, int a3) */
int32_t beh_484ea0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    int v5;
    int v6;
    intptr_t v7;
    intptr_t v8;
    char * v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    int32_t * v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    int32_t * *const v30__f = (int32_t * *)(frame_ + 0x10);   /* ebp-0x28 */
    char * *const v31__f = (char * *)(frame_ + 0x14);   /* ebp-0x24 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x18 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x10 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  SETLO(v4, 0);
  v5 = F16(c, 0x1482e) <= 1u;
  (*v30__f) = 0;
  (*v36__f) = 0;
  (*v34__f) = 0;
  if ( !v5 )
    return v4;
  v6 = F16(c, 0x14228) == 0;
  (*v35__f) = 0;
  if ( v6 )
    return v4;
  do
  {
    v7 = 320 * (uint16_t)v4;
    v8 = *(_DWORD *)&GAP(c, 0xca26, v7 + 2);
    v9 = &GAP(c, 0xca26, v7 + 2);
    (*v31__f) = v9;
    SETLO(v7, F16(v8, 174));
    (*v38__f) = v8;
    if ( (uint16_t)v7 != 2 || F16(v9, 2 * 140) != 0 )
      goto LABEL_65;
    if ( F16(v9, 2 * 158) != 0 || F16(v9, 2 * 159) != 0 )
    {
      v10 = 1000
          * F32(F32(c, 0x750), 84)
          / (F32(F32(c, 0x750), 88) + F32(F32(c, 0x750), 132) + F32(F32(c, 0x750), 156));
      if ( v10 < *((uint16_t *)v9 + 158) || v10 > *((uint16_t *)v9 + 159) )
        goto LABEL_65;
    }
    if ( W_char_skill_ok(c, v8, 1) == 0 || W_skill_pose_ok(v8, (uint16_t *)a2) == 0 )
      goto LABEL_65;
    if ( F32(v8, 188) == 1 )
    {
      if ( a3 == 0 )
        goto LABEL_65;
    }
    else if ( F32(v8, 188) == 2 )
    {
      if ( a3 != 0 )
        goto LABEL_65;
    }
    else if ( F32(v8, 188) == 3 && (a3 == 0 || F32(a2, 40) < 900 * F32(c, 0x15990) / 1000) )
    {
      goto LABEL_65;
    }
    if ( F32(v8, 176) == 11 && F32(v8, 708) == 6 && F16(c, 0x14830) > 1u )
      goto LABEL_65;
    v11 = F32(v9, 0);
    v12 = 0;
    (*v37__f) = 1;
    v13 = F32(v11, 724);
    if ( v13 <= 0 )
    {
      v17 = 1000;
    }
    else
    {
      v14 = (_DWORD *)F32(c, 0x750);
      v15 = v14[23];
      if ( v13 > v15 )
        goto LABEL_65;
      v16 = v14[34] + v14[40] + v14[24];
      if ( v16 <= 0 )
      {
        v9 = (*v31__f);
        v17 = 0;
      }
      else
      {
        v17 = 1000 * v15 / v16;
        v9 = (*v31__f);
      }
    }
    if ( F16(v11, 174) == 1 )
    {
      v18 = F16(c, 0x14828);
LABEL_34:
      if ( F16(c, 0x14832) != 0 )
        goto LABEL_65;
      goto LABEL_35;
    }
    if ( F16(v11, 174) != 2 )
    {
      v18 = F16(c, 0x1482c);
      goto LABEL_34;
    }
    (*v37__f) = 0;
    v18 = F16(c, 0x1482a);
    v12 = 1;
LABEL_35:
    if ( F32(c, 0x1f0) != 0 )
    {
      v19 = 500;
LABEL_57:
      if ( &(*v28__f) == (int *)-24 )
      {
        v20 = (*v32__f);
      }
      else
      {
        v20 = v19;
        (*v32__f) = v19;
      }
      v21 = (*v38__f);
      v22 = beh_47a970(c, (*v38__f), &(*v39__f), &(*v33__f), 0, 0, 0, v20);
      if ( v22 != 0 && (*v39__f) >= v20 )
      {
        (*v33__f) -= 2 * F32(v21, 724);
        if ( (*v33__f) > (*v34__f) || (*v30__f) == 0 )
        {
          (*v30__f) = (int *)v9;
          (*v36__f) = v22;
          (*v34__f) = (*v33__f);
        }
      }
    }
    else
    {
      switch ( v18 )
      {
        case 1:
          v19 = 1200;
          if ( v12 == 0 )
          {
            if ( v17 >= 100 )
            {
              if ( v17 < 300 )
                v19 = 1500;
            }
            else
            {
              v19 = 1800;
            }
          }
          goto LABEL_57;
        case 2:
          v19 = 800;
          if ( v12 == 0 )
          {
            if ( v17 >= 100 )
            {
              if ( v17 >= 300 )
              {
                if ( v17 < 500 )
                  v19 = 1000;
              }
              else
              {
                v19 = 1200;
              }
            }
            else
            {
              v19 = 1600;
            }
          }
          goto LABEL_57;
        case 3:
          v19 = 600;
          if ( v12 == 0 )
          {
            if ( v17 >= 100 )
            {
              if ( v17 < 300 )
                v19 = 800;
            }
            else
            {
              v19 = 1000;
            }
          }
          goto LABEL_57;
        case 4:
          v19 = 300;
          goto LABEL_57;
        case 5:
          v19 = 200;
          goto LABEL_57;
        default:
          break;
      }
    }
LABEL_65:
    SETLO(v4, (*v35__f) + 1);
  }
  while ( (uint16_t)++(*v35__f) < F16(c, 0x14228) );
  v24 = (int)(*v30__f);
  if ( (*v30__f) != 0 )
  {
    v4 = *(*v30__f);
    if ( *(*v30__f) != 0 && F32(v4, 176) == 11 && F32(v4, 708) == 6 )
    {
      if ( F16(c, 0x14830) == 0 )
      {
        F16(c, 0x14830) = 500;
        return v4;
      }
      F16(c, 0x14830) = 0;
    }
    if ( F16(c, 0x1482e) != 0 || (SETLO(v4, F16(c, 0xc89c)), (uint16_t)v4 == 0) )
    {
      v25 = (*v36__f);
      F16(c, 0x1482e) = 0;
      F16(c, 0x14288) = 2;
      F32(c, 0x1427c) = (int)v25;
      if ( v25 != 0 )
      {
        v26 = F32(v25, 0x40);
        (*v29__f) = 0;
        F32(c, 0x14280) = F32(v26, 4);
        v4 = W_char_part_top(v25, (*v29__f));
        F32(c, 0x14284) = v4;
      }
      F32(c, 0x14298) = v24;
    }
    else
    {
      F16(c, 0x1482e) = v4;
    }
  }
  return v4;
}

/* 0x485350: int __thiscall sub_485350(cls_485350 *this, __int16 a2, __int16 a3, signed int a4, int a5, int a6) */
int32_t beh_485350(wl_char *self, int16_t a2, int16_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    int32_t * v12;
    int32_t * v13;

  if ( F32(c, 0xc790) == 1 )
  {
    if ( F32(F32(c, 0x750), 212) == 1 )
    {
      if ( F32(c, 0x1f0) == 0 && F16(c, 0x14288) == 0 )
      {
        F16(c, 0x1486a) = 0;
        F32(c, 0x1486c) = 0;
        F16(c, 0x14868) = a3;
        F16(c, 0x14870) = 210;
        return 1;
      }
    }
    else
    {
      v8 = F32(c, 0xc768);
      v9 = false;
      if ( v8 != 0 )
      {
        v10 = W_actor_by_handle(v8);
        if ( v10 != 0 )
          v9 = F32(v10, 12) == 1;
      }
      else
      {
        v11 = F32(c, 0x1427c);
        if ( v11 != 0 )
        {
          v12 = (_DWORD *)F32(c, 0x40);
          v13 = F32(v11, 64);
          if ( (int)abs32(v12[3] / 2 - v13[3] / 2 - v13[1] + v12[1]) < a4 )
            v9 = (int)abs32(v12[2] + (v12[4] - v12[5]) / 2 - (v13[4] - v13[5]) / 2 - v13[2] - v13[5] + v12[5]) < a5;
        }
      }
      if ( (a6 != 0 || v9) && F16(c, 0x14288) == 1 )
      {
        F16(c, 0x14870) = 210;
        F16(c, 0x14868) = a2;
        F16(c, 0x1486a) = 0;
        F32(c, 0x1486c) = 0;
        return 1;
      }
    }
  }
  return 0;
}

/* 0x4854e0: BOOL __thiscall sub_4854E0(cls_4854E0 *this, int a2, int a3) */
int32_t beh_4854e0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    int64_t v11;
    _Alignas(8) uint8_t frame_[0x100 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [256] at ebp-0x100 */

  if ( F32(c, 0x1f0) != 0 )
    return false;
  v4 = F32(c, 0x14240);
  if ( v4 < 3 || v4 > 4 || F16(c, 0x1422c) != 0 )
    return false;
  if ( F32(c, 0x1594c) == 0 )
  {
    if ( a3 != 0 && F16(c, 0x147a0) < F16(c, 0x1429c) && *(_WORD *)&GAP(c, 0x1429e, 20 * F16(c, 0x147a0) + 2) == 0 )
      return (int)abs32(F32(a2, 4) - 3200 * *(_DWORD *)&GAP(c, 0x1429e, 20 * F16(c, 0x147a0) + 6)) > 16000;
    return false;
  }
  v6 = F32(c, 0x1424c);
  v7 = F16(c, 0x14288) == 1;
  if ( F16(v6, 68) == 0 )
  {
    g_eng.h5bf3ee(Buffer, "%d", F32(v6, 468));
    g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85451c), Buffer);
  }
  v8 = F32(v6, 51068);
  if ( v7 )
  {
    v9 = F32(c, 0x14250);
    v8 = F32(c, 0x14244);
  }
  else
  {
    v9 = F32(c, 0x14254);
  }
  if ( v8 != 3 )
    v9 = -v9;
  v10 = v9;
  v11 = F32(F32(c, 0x40), 12);
  return (int)abs32(
                v10
              + F32(F32((v6 + 64), 0), 4)
              + F32(F32((v6 + 64), 0), 12) / 2
              - (((int)v11 - HIDWORD(v11)) >> 1)
              - F32(a2, 4)) > 16000;
}

/* 0x485990: int __thiscall sub_485990(cls_4022A0 *this, _DWORD *a2, int a3) */
int32_t beh_485990(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    int32_t * v5;
    uint16_t v6;
    intptr_t v7;
    int32_t * v8;
    uint16_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x4 */

  if ( F16(c, 0x14228) == 0 )
    return 0;
  v4 = F32(c, 0x1428c);
  v5 = a2;
  if ( v4 == 0 )
    goto LABEL_6;
  beh_47e7c0(v4, 0, a2, 0, &(*v22__f), &(*v24__f), &a2, 0);
  if ( F32(c, 0xc880) == 0 )
    goto LABEL_6;
  v6 = F16(c, 0x14802);
  if ( v6 != 0 )
  {
    F16(c, 0x14802) = v6 - 1;
    goto LABEL_6;
  }
  if ( F32(c, 0xc884) <= (int)a2 )
    goto LABEL_6;
  v10 = F32(c, 0xc888) <= (int)a2 ? 1600 : 4800;
  v11 = v10;
  if ( v5[1] >= (*v22__f) )
  {
    v12 = W_behave_step_ok(c, v5, v10, 1, 1, 1, 1);
    v13 = 3;
    if ( !v12 )
      goto LABEL_17;
  }
  else
  {
    v12 = W_behave_step_ok(c, v5, v10, 3, 1, 1, 1);
    if ( !v12 )
    {
      v13 = 1;
      goto LABEL_17;
    }
  }
  v13 = (int)a2;
LABEL_17:
  if ( F32(c, 0xc888) <= (int)a2 )
  {
    if ( v12 )
      goto LABEL_6;
  }
  else if ( v12 )
  {
    if ( v5[1] >= (*v22__f) )
    {
      if ( W_behave_step_ok(c, v5, v11, 3, 1, 1, 1) )
        goto LABEL_6;
LABEL_30:
      F32(c, 0x147fc) = F32(F32((F32(c, 0x1428c) + 64), 0), 4)
                        + F32(F32((F32(c, 0x1428c) + 64), 0), 12)
                        + F32(c, 0xc88c);
LABEL_22:
      if ( beh_484830(c, (int)v5, F32(c, 0x147fc)) == 0 )
      {
        v14 = F32(c, 0x1594c);
        if ( v14 > 0 && v14 <= 3 )
        {
          W_behave_set_state(c, 4);
          F16(c, 0x14800) = 120;
          return 1;
        }
        v16 = W_part_bottom(F32(c, 0x1428c), 0);
        if ( beh_483040(c, v5, F32(c, 0x147fc), v16, 3, &(*v23__f)) != 0 )
        {
          W_behave_set_state(c, 4);
          v17 = (*v23__f);
          F16(c, 0x14800) = 120;
          F32(c, 0x147fc) = v17;
          return 1;
        }
        F16(c, 0x14802) = 60;
      }
      goto LABEL_6;
    }
    if ( !W_behave_step_ok(c, v5, v11, 1, 1, 1, 1) )
    {
LABEL_21:
      F32(c, 0x147fc) = W_part_left(F32(c, 0x1428c), 0) - F32(c, 0xc88c);
      goto LABEL_22;
    }
LABEL_6:
    if ( F16(c, 0x14288) != 0 && F16(c, 0x14288) <= 2u )
    {
      v7 = F32(c, 0x1427c);
      v8 = (_DWORD *)F32(c, 0x40);
      if ( v7 != 0 )
      {
        v9 = F16(c, 0x147f4);
        if ( v9 != 0 )
        {
          F16(c, 0x147f4) = v9 - 1;
        }
        else
        {
          (*v21__f) = F32(c, 0xc834);
          F16(c, 0x147f4) = 8;
          if ( beh_485cb0(c, v8, v7, 0, (*v21__f), 0) != 0 )
          {
            W_behave_set_state(c, 2);
            v18 = F32(c, 0xc838) * rnd();
            v19 = F32(c, 0xc834);
            F16(c, 0x1429c) = 0;
            F16(c, 0x147a0) = 0;
            F32(c, 0x1423c) = F32(c, 0xc83c);
            F32(c, 0x147f8) = v19 + v18 / 0x8000;
            return 1;
          }
        }
      }
      v20 = a3;
      if ( beh_4834f0(c, a3, (int)v5) != 0 )
        return 1;
      if ( beh_481310(c, v5, v20, 0) != 0 )
        return 1;
    }
    return 0;
  }
  if ( v13 != 1 )
    goto LABEL_21;
  goto LABEL_30;
}

/* 0x442490: BOOL __thiscall sub_442490( cls_442490 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, _DWORD *a10) */
int32_t beh_442490(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t * a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    int v14;
    intptr_t v15;
    intptr_t v16;
    int32_t * v17;

  v10 = a4;
  v11 = a2;
  v12 = a8;
  if ( a2 >= a4 )
  {
    v13 = (int)a10;
  }
  else
  {
    v13 = a4 - F32(a8, 4);
    if ( v13 <= 7200 )
    {
      if ( v13 > 0 )
        *a10 = (F32(a8, 40) <= F32(c, 0x1596c)) + 1;
    }
    else
    {
      *a10 = 6;
    }
  }
  v14 = v11 == v10;
  if ( v11 > v10 )
  {
    v13 = F32(v12, 4) - v10;
    if ( v13 <= 7200 )
    {
      if ( v13 > 0 )
        *a10 = (F32(v12, 40) > F32(c, 0x1596c)) + 1;
    }
    else
    {
      *a10 = 5;
    }
    v14 = v11 == v10;
  }
  if ( v14 )
    v13 = 0;
  v15 = a5;
  if ( a3 <= a5 )
  {
    v16 = (int)a10;
  }
  else
  {
    v16 = F32(v12, 8) + F32(v12, 16) - a5 - 3200;
    if ( v16 > 0 )
    {
      v17 = &a2;
      SETLO(v17, F8(v12, 32));
      if ( W_entity_move_vertical(c, (_DWORD *)v12, 1, (int)v17, a9, (int)&a2, 0) != 0 )
      {
LABEL_21:
        a10[2] = 8;
      }
      else
      {
        switch ( F16(v12, 56) )
        {
          case 0:
          case 1:
          case 4:
            break;
          case 2:
            if ( F16(v12, 58) != 2 )
              goto LABEL_21;
            break;
          default:
            goto LABEL_21;
        }
        a10[2] = 7;
      }
    }
  }
  if ( a3 < v15 )
  {
    v16 = v15 - F32(v12, 16) - F32(v12, 8) + 3200;
    if ( v16 > 6400 )
      a10[2] = 9;
  }
  return v13 <= 0 && v16 <= 0;
}

/* 0x47ae00: int __thiscall sub_47AE00( cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int *a8, int *a9, int a10, int a11, int a12, int *a13) */
int32_t beh_47ae00(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t * a8, int32_t * a9, int32_t a10, int32_t a11, int32_t a12, int32_t * a13)
{
    intptr_t c = (intptr_t)self;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    int16_t v17;
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
    intptr_t v30;
    intptr_t v31;
    char * v32;
    intptr_t v33;
    intptr_t v34;
    intptr_t v35;
    intptr_t v36;
    intptr_t v37;
    intptr_t v38;
    intptr_t v39;
    intptr_t v40;
    uint8_t * v41;
    intptr_t v42;
    uint16_t i;
    intptr_t v44;
    intptr_t v45;
    intptr_t v46;
    intptr_t v47;
    intptr_t v48;
    intptr_t v49;
    intptr_t v50;
    intptr_t v51;
    intptr_t v52;
    uint8_t * v53;
    intptr_t v54;
    intptr_t v55;
    int v56;
    intptr_t v57;
    intptr_t j;
    intptr_t v68;
    intptr_t v69;
    intptr_t v70;
    intptr_t v71;
    intptr_t v72;
    intptr_t v73;
    intptr_t v74;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  (*v59__f) = 0;
  (*v61__f) = 0;
  (*v63__f) = 0;
  if ( F32(c, 0x15908) == 0 )
  {
    if ( a2 == 0
      || (v14 = F32(c, 0x1dc), v15 = F32(a2, 476), v14 != 0)
      && v15 != 0
      && v14 != 3
      && v15 != 3
      && v14 == v15
      && (a11 == 0 || W_entity_near(c, 48000, 28000, a2, 0)) )
    {
      v16 = F32(c, 0x750);
      v17 = F16(v16, 1098);
      if ( v17 == 0 || a3 == 10 )
      {
        switch ( a3 )
        {
          case 9:
            v19 = F32(v16, 156);
            v20 = F32(v16, 132);
            v21 = F32(v16, 88);
            v22 = F32(v16, 84);
            v23 = v21 + v20 + v19;
            (*v59__f) = 1200 * (1000 - 1000 * v22 / v23) / 750 < 0 ? 0 : 1200 * (1000 - 1000 * v22 / v23) / 750;
            v24 = v19 + v20 + v21 - v22;
            v25 = a4 + a5 / 2;
            if ( v25 >= v24 )
              v26 = (int)abs32(v24 - v25) / 4;
            else
              v26 = abs32(v24 - v25);
            (*v61__f) = 1000 - 1000 * v26 / v23;
            goto LABEL_138;
          case 10:
            if ( v17 != 0 )
            {
              (*v59__f) = 1000;
              (*v61__f) = 1200;
            }
            (*v63__f) = (*v59__f);
            goto LABEL_139;
          case 11:
            if ( F16(v16 + 2 * (uint16_t)a4, 1096) > 0x64u )
            {
              (*v61__f) = 1100;
              v27 = 1200
                  * (1000
                   - 1000
                   * F32(v16, 84)
                   / (F32(v16, 88) + F32(v16, 132) + F32(v16, 156)))
                  / 750;
              (*v59__f) = v27 < 0 ? 700 : v27 + 700;
            }
            goto LABEL_138;
          case 12:
            if ( F16(v16, 1102) <= 0x64u )
            {
              if ( F16(v16, 1106) <= 0xC8u )
                goto LABEL_25;
              v30 = 1200
                  * (1000
                   - 1000
                   * F32(v16, 84)
                   / (F32(v16, 88) + F32(v16, 132) + F32(v16, 156)));
              v29 = v30 / 750 < 0 ? 800 : v30 / 750 + 800;
            }
            else
            {
              v28 = 1200
                  * (1000
                   - 1000
                   * F32(v16, 84)
                   / (F32(v16, 88) + F32(v16, 132) + F32(v16, 156)))
                  / 750;
              v29 = v28 < 0 ? 700 : v28 + 700;
            }
            (*v59__f) = v29;
            (*v61__f) = 1000;
LABEL_25:
            (*v63__f) = (*v59__f);
            goto LABEL_139;
          case 13:
            if ( F16(c, 0x15a84) == 0 )
              goto LABEL_139;
            for ( i = 0; i < 0x12u; ++i )
            {
              if ( F16(v16 + 2 * i, 1096) != 0 && ((int32_t *)D(0x628764))[35 * i] != 0 )
                goto LABEL_139;
            }
            (*v66__f) = F16(c, 0x15a84);
            v44 = 2;
            v45 = 1000
                * ((F32(c, 0xc7d8) + F32(v16, 100) + F32(v16, 116) + F32(v16, 140))
                 / 2)
                / 1000;
            v46 = 20 * v45 / 100;
            if ( v46 >= 2 )
              v44 = 20 * v45 / 100;
            v47 = 50 * v45 / 100;
            if ( v47 >= v44 )
            {
              if ( v46 < 2 )
              {
LABEL_86:
                v49 = 2;
                goto LABEL_88;
              }
              v48 = 20 * v45 / 100;
            }
            else
            {
              v48 = 50 * v45 / 100;
            }
            if ( v48 > 100 )
            {
              v46 = 100;
              goto LABEL_92;
            }
            if ( v46 < 2 )
              goto LABEL_86;
            v49 = 20 * v45 / 100;
LABEL_88:
            if ( v47 >= v49 )
            {
              if ( v46 < 2 )
                v46 = 2;
            }
            else
            {
              v46 = 50 * v45 / 100;
            }
LABEL_92:
            v50 = a2;
            (*v65__f) = v45 + 1000 * v46 / 1000 - v46;
LABEL_95:
            if ( a12 != 0 )
            {
              (*v59__f) = 1;
              goto LABEL_139;
            }
            if ( F16(v16 + 2 * (uint16_t)a4, 1096) == 0 )
            {
              (*v62__f) = v50;
              v69 = 0;
              v72 = 0;
              v68 = 0;
              (*v64__f) = 0;
              if ( v50 == 0 )
                (*v62__f) = c;
              if ( (uint16_t)a4 != 0 && ((int32_t *)D(0x628780))[35 * (uint16_t)a4] > 0 && F32(c, 0xc794) != 0 )
              {
                v51 = beh_5b4200(c);
                v68 = 1;
              }
              else
              {
                v51 = a4;
              }
              v74 = 4448;
              (*v60__f) = 32;
              while ( 1 )
              {
                v52 = *(intptr_t *)&GAP(W, 0x0, v74);
                if ( F32(v52, 0x1d0) == 1 )
                {
                  v53 = (_BYTE *)F32(v52, 0x40);
                  if ( *v53 != 1
                    || v53[126] != 0
                    || F16(F32(v52, 0x750), 1098) != 0
                    || W_faction_rel(c, F32(c, 0x1dc), F32(v52, 0x1dc)) != 1
                    || v52 == c
                    || !W_entity_near(c, 48000, 16000, (int)v52, 0) )
                  {
                    goto LABEL_123;
                  }
                  v54 = beh_47b870((int)(*v62__f), (int)v52);
                  if ( v68 == 0 )
                    goto LABEL_120;
                  if ( v54 > 0 )
                    break;
LABEL_122:
                  v69 += v54;
                }
LABEL_123:
                v56 = (*v60__f) == 1;
                v74 += 4;
                --(*v60__f);
                if ( v56 )
                {
                  if ( v68 != 0 && ((int32_t *)D(0x628780))[35 * (uint16_t)a4] > v72 )
                  {
                    (*v59__f) = 0;
                  }
                  else
                  {
                    v57 = v69;
                    if ( v69 > 0 && (uint16_t)a4 != 0 )
                    {
                      v57 = ((int32_t *)D(0x628784))[35 * (uint16_t)a4] + v69;
                      if ( (*v64__f) >= 2 )
                        v57 += ((int32_t *)D(0x628788))[35 * (uint16_t)a4];
                    }
                    (*v59__f) = v57;
                  }
                  if ( a10 != 0 && (*v59__f) < 100 )
                    (*v59__f) = 100;
                  (*v63__f) = (*v66__f) * ((*v65__f) + (*v59__f)) / 1000;
                  (*v61__f) = a5 + 100 * a6 / 1000;
                  goto LABEL_139;
                }
              }
              if ( beh_48b920(v52, v51, 0, 750) >= 1 )
                v55 = beh_48b920(v52, v51, 0, 750);
              else
                v55 = 1;
              v72 += F32(F32(v52, 0x750), 84) / v55 + 1;
              if ( F32(v52, 0x15a94) != 0 )
                v68 = 0;
LABEL_120:
              if ( v54 > 0 )
                ++(*v64__f);
              goto LABEL_122;
            }
LABEL_139:
            if ( a8 != 0 )
              *a8 = (*v59__f);
            if ( a9 != 0 )
              *a9 = (*v61__f);
            if ( a13 != 0 )
              *a13 = (*v63__f);
            return 1;
          case 14:
            if ( a12 != 0 )
            {
              (*v59__f) = 1;
              goto LABEL_139;
            }
            v31 = 0;
            v73 = 0;
            v71 = 0;
            v32 = &GAP(W, 0x1030, 304);
            v33 = 32;
            break;
          case 15:
            if ( F16(c, 0x15a86) == 0 )
              goto LABEL_139;
            (*v65__f) = 0;
            (*v66__f) = F16(c, 0x15a86);
            v50 = c;
            goto LABEL_95;
          case 16:
          case 19:
            if ( a12 != 0 )
              (*v59__f) = 1;
LABEL_138:
            (*v63__f) = (*v59__f);
            goto LABEL_139;
          case 18:
            (*v61__f) = 0;
            v18 = 1200
                * (1000
                 - 1000
                 * F32(v16, 92)
                 / (F32(v16, 96) + F32(v16, 136) + F32(v16, 160)))
                / 750;
            (*v59__f) = v18 < 0 ? 0 : v18;
            (*v63__f) = (*v59__f);
            goto LABEL_139;
          default:
            return 0;
        }
        while ( 1 )
        {
          v34 = F32(v32, 0);
          if ( F32(F32(v32, 0), 464) == 1
            && F8(F32(v34, 64), 0) == 1
            && F16(F32(v34, 1872), 1098) == 0 )
          {
            v35 = F32(c, 0x1dc);
            v36 = F32(v34, 476);
            if ( v35 != 0 && v36 != 0 && v35 != 3 && v36 != 3 && v35 == v36 )
            {
              if ( F32(v34, 468) != a4 && a4 != 0 )
                goto LABEL_44;
              ++v31;
              if ( F32(v34, 88928) == c )
                break;
            }
          }
LABEL_46:
          v32 += 4;
          if ( --v33 == 0 )
          {
            v37 = F32(c, 0x15a6c);
            if ( (v37 == 0 || v37 > v73) && (a6 <= 0 || a6 > v31) && (a5 <= 0 || a5 > v71) )
            {
              v38 = 0;
              v70 = a2;
              if ( a2 == 0 )
                v70 = c;
              v39 = 4448;
              for ( j = 32; j != 0; --j )
              {
                v40 = *(intptr_t *)&GAP(W, 0x0, v39);
                if ( F32(v40, 0x1d0) == 1 )
                {
                  v41 = (_BYTE *)F32(v40, 0x40);
                  if ( *v41 == 1 && v41[126] == 0 && F16(F32(v40, 0x750), 1098) == 0 )
                  {
                    if ( (v42 = W_faction_relation(F32(c, 0x1dc), F32(v40, 0x1dc)), F32(c, 0x1f0) != 0)
                      && (v42 == 1 || v42 == 2)
                      || v42 == 1 )
                    {
                      if ( v40 != c && W_entity_near(c, 48000, 16000, (int)v40, 0) )
                        v38 += beh_47b870((int)v70, (int)v40);
                    }
                  }
                }
                v39 += 4;
              }
              (*v59__f) = v38;
              if ( a10 != 0 && v38 < 100 )
                (*v59__f) = 100;
              (*v61__f) = 100;
              (*v63__f) = (*v59__f);
            }
            goto LABEL_139;
          }
        }
        ++v71;
LABEL_44:
        if ( F32(v34, 88928) == c )
          ++v73;
        goto LABEL_46;
      }
    }
  }
  return 0;
}

/* 0x47f610: int __stdcall sub_47F610(int a1, unsigned __int16 a2, int a3) */
int32_t beh_47f610(int32_t a1, uint16_t a2, int32_t a3)
{

    uint16_t v3;
    intptr_t v4;

  v3 = a2;
  if ( a2 >= F16(a1, 68) )
    v3 = 0;
  v4 = F32(a1, 64) + 660 * v3;
  return F32(a3, 4) + F32(a3, 12) / 2 >= F32(v4, 4) + F32(v4, 12) / 2 ? 3 : 1;
}

/* 0x47fd40: int __thiscall sub_47FD40(cls_4022A0 *this, _DWORD *a2, int a3, int a4, int a5, int a6, int a7) */
int32_t beh_47fd40(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    int32_t * v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    uint16_t v17;
    int32_t * v18;
    uint16_t v19;
    int32_t * v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    int v24;
    intptr_t result;
    intptr_t v47;
    _Alignas(8) uint8_t frame_[0x74 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v38 = (intptr_t *)(frame_ + 0x44);   /* [4] at ebp-0x30 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x74 */
    uint16_t *const v27__f = (uint16_t *)(frame_ + 0x18);   /* ebp-0x5c */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x54 */
    uint16_t *const v30__f = (uint16_t *)(frame_ + 0x24);   /* ebp-0x50 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4c */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x48 */
    uint16_t *const v33__f = (uint16_t *)(frame_ + 0x30);   /* ebp-0x44 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x40 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x3c */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x38 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x34 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x20 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x1c */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x18 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0x14 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x64);   /* ebp-0x10 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x68);   /* ebp-0xc */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x6c);   /* ebp-0x8 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x70);   /* ebp-0x4 */

  v7 = c;
  v8 = *(_DWORD **)a3;
  (*v29__f) = 0;
  (*v32__f) = 0;
  (*v36__f) = 0;
  (*v33__f) = 0;
  (*v34__f) = 0;
  if ( F32(F32(a3, 0), 732) != 0 )
  {
    v9 = v8[178];
    (*v26__f) = 1000 * v8[177] / 1000;
    v10 = W_char_mag_sum(c);
    (*v31__f) = W_stat_jitter(v9 + v10 / 2, (*v26__f), 750);
  }
  else
  {
    (*v31__f) = W_char_atk_roll(c, 1000 * v8[177] / 1000, 750, F32(F32(a3, 0), 176) == 6, v8[178]);
  }
  (*v37__f) = beh_5b4200(v7);
  if ( a4 != 0 )
    (*v30__f) = F16(a3, 148);
  else
    (*v30__f) = F16(a3, 16);
  switch ( v8[44] )
  {
    case 1:
      (*v27__f) = 5;
      goto LABEL_9;
    case 3:
      (*v27__f) = 6;
      goto LABEL_9;
    case 4:
      (*v27__f) = 8;
      goto LABEL_9;
    case 5:
      (*v27__f) = 2;
      goto LABEL_9;
    case 6:
      (*v27__f) = 3;
      goto LABEL_9;
    case 7:
      (*v27__f) = 7;
      goto LABEL_9;
    case 8:
      (*v27__f) = 9;
      goto LABEL_9;
    case 0x11:
      (*v27__f) = 4;
LABEL_9:
      (*v36__f) = ((int32_t *)D(0x628788))[35 * (*v27__f)];
      break;
    default:
      (*v27__f) = 0;
      break;
  }
  if ( ((*v27__f) == 3 || (*v27__f) == 5) && F32(v7, 0xc794) != 0 )
  {
    (*v33__f) = 2;
    if ( a6 == 1 )
      (*v39__f) = a2[3] + a2[1] - 40000;
    else
      (*v39__f) = a2[1] - 40000;
    v12 = a2[2];
    (*v41__f) = 80000;
    (*v42__f) = 24000;
    (*v40__f) = v12 - 12000;
  }
  v47 = 4448;
  (*v35__f) = 32;
  do
  {
    v13 = *(_DWORD *)&GAP(W, 0x0, v47);
    if ( !W_char_targetable(v13, 0, 0)
      || F16(v13, 622) != 0
      || F8(F32(v13, 64), 125) != 0
      || v13 == v7 )
    {
      goto LABEL_67;
    }
    v14 = F32(v7, 0x1dc);
    v15 = F32(v13, 476);
    if ( v14 != 0 && v15 != 0 )
    {
      if ( v14 == 3 || v15 == 3 )
      {
        v16 = 1;
      }
      else
      {
        v7 = c;
        v16 = (v14 == v15) + 1;
      }
    }
    else
    {
      v16 = 0;
    }
    if ( (F32(v7, 0x1f0) == 0 || v16 != 1 && v16 != 2) && v16 != 1 )
      goto LABEL_67;
    if ( (*v33__f) != 0 )
    {
      if ( F32(v13, 0) != 0 )
      {
        v19 = 0;
        if ( F16(v13, 4) == 0 )
          goto LABEL_45;
        while ( 1 )
        {
          if ( F32(32 * v19 + F32(v13, 0), 16) != 0 )
          {
            v20 = (_DWORD *)(F32(v13, 64) + 660 * (uint16_t)a5);
            if ( F32(v20[164] + 20 * v19, 16) == 0 )
            {
              W_body_frame_box(v19, v20, &(*v43__f));
              if ( (*v43__f) <= (*v41__f) + (*v39__f) && (*v43__f) + (*v45__f) > (*v39__f) && (*v44__f) <= (*v42__f) + (*v40__f) && (*v44__f) + (*v46__f) > (*v40__f) )
                break;
            }
          }
          if ( ++v19 >= F16(v13, 4) )
            goto LABEL_45;
        }
LABEL_44:
        ++(*v34__f);
        goto LABEL_45;
      }
      if ( W_entity_rect_hit(v13, &(*v39__f), a5) )
        goto LABEL_44;
    }
LABEL_45:
    v17 = 0;
    if ( (*v30__f) != 0 )
    {
      while ( 1 )
      {
        v18 = (_DWORD *)(a4 != 0 ? 16 * v17 + a3 + 152 : 16 * v17 + a3 + 20);
        v38[0] = a6 == 1 ? *v18 + a2[1] + a2[3] : a2[1] - v18[1] - *v18;
        v21 = v18[1];
        v38[1] = a2[2] + v18[2];
        v22 = v18[3];
        v38[2] = v21;
        v38[3] = v22;
        if ( beh_480260(v13, v38, a5, 0, 0) != 0 )
          break;
        if ( ++v17 >= (*v30__f) )
          goto LABEL_66;
      }
      v23 = beh_480580(c, v13, v8, (*v31__f), F32(a3, 312), (*v27__f), a7, (*v37__f));
      if ( v23 > 0 )
      {
        (*v29__f) += v23;
        ++(*v32__f);
      }
    }
LABEL_66:
    v7 = c;
LABEL_67:
    v24 = (*v35__f) == 1;
    v47 += 4;
    --(*v35__f);
  }
  while ( !v24 );
  if ( (*v31__f) != 0 || (*v36__f) <= 0 )
    result = (*v29__f);
  else
    result = (*v36__f) * ((*v32__f) - 2) + (*v29__f);
  if ( (*v33__f) != 0 && (*v33__f) > (*v34__f) )
    return 0;
  return result;
}

/* 0x480b30: int __thiscall sub_480B30(cls_4022A0 *this, int a2, int a3, unsigned __int16 a4, int a5, int a6, int *a7) */
int32_t beh_480b30(wl_char *self, int32_t a2, int32_t a3, uint16_t a4, int32_t a5, int32_t a6, int32_t * a7)
{
    intptr_t c = (intptr_t)self;
    uint8_t * v8;
    intptr_t v9;
    intptr_t v10;
    int32_t * v11;
    intptr_t v13;
    int v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    int32_t * v19;
    int32_t * v20;
    intptr_t v21;
    uint16_t v22;
    uint16_t v23;
    int32_t * v24;
    uint16_t v25;
    int32_t * v26;
    intptr_t v27;
    intptr_t v28;
    _Alignas(8) uint8_t frame_[0x644 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v51 = (intptr_t *)(frame_ + 0x4c);   /* [5] at ebp-0x5f8 */
    intptr_t *v56 = (intptr_t *)(frame_ + 0x6c);   /* [4] at ebp-0x5d8 */
    uint8_t *v59 = (uint8_t *)(frame_ + 0x11c);   /* [656] at ebp-0x528 */
    intptr_t *v61 = (intptr_t *)(frame_ + 0x3b0);   /* [165] at ebp-0x294 */
    uint16_t *const v29__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x644 */
    uint16_t *const v31__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x640 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x640 */
    uint16_t *const v32__f = (uint16_t *)(frame_ + 0x8);   /* ebp-0x63c */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x63c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x638 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x634 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x630 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x630 */
    uint16_t *const v38__f = (uint16_t *)(frame_ + 0x18);   /* ebp-0x62c */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x628 */
    uint8_t * *const v40__f = (uint8_t * *)(frame_ + 0x20);   /* ebp-0x624 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x620 */
    int16_t *const v42__f = (int16_t *)(frame_ + 0x28);   /* ebp-0x61c */
    uint16_t *const v43__f = (uint16_t *)(frame_ + 0x2c);   /* ebp-0x618 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x614 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x610 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x60c */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x608 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x604 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x600 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x5fc */
    int16_t *const v52__f = (int16_t *)(frame_ + 0x60);   /* ebp-0x5e4 */
    int16_t *const v53__f = (int16_t *)(frame_ + 0x62);   /* ebp-0x5e2 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x64);   /* ebp-0x5e0 */
    int16_t *const v55__f = (int16_t *)(frame_ + 0x68);   /* ebp-0x5dc */
    char *const v57__f = (char *)(frame_ + 0x7c);   /* ebp-0x5c8 */
    char *const v58__f = (char *)(frame_ + 0xcc);   /* ebp-0x578 */
    char * *const v60__f = (char * *)(frame_ + 0x3ac);   /* ebp-0x298 */

  v8 = 0;
  (*v30__f) = F32(c, 0x40);
  (*v60__f) = &(*v57__f);
  v61[164] = &(*v58__f);
  (*v43__f) = 0;
  (*v38__f) = 0;
  (*v34__f) = 0;
  (*v41__f) = 0;
  (*v42__f) = 0;
  (*v44__f) = 0;
  v9 = F16(a3, 294) * rnd() / 0x8000;
  SETLO(v9, F16(a3, 292) + v9);
  (*v35__f) = v9;
  v10 = F32(c, 0x40);
  (*v50__f) = 660 * (uint16_t)v9;
  v11 = (_DWORD *)(v10 + (*v50__f));
  if ( F32(a3, 12) == 4 )
  {
    (*v39__f) = v11[11];
    if ( beh_47fd40(c, v11, a3, 0, v9, (*v39__f), 0) <= 0 )
    {
      (*v39__f) = v11[11] != 3 ? 3 : 1;
      if ( beh_47fd40(c, v11, a3, 0, (*v35__f), (*v39__f), 0) <= 0 )
        return 0;
    }
  }
  else
  {
    (*v32__f) = 0;
    if ( F16(a3, 16) == 0 )
      return 0;
    while ( 1 )
    {
      (*v39__f) = v11[11];
      W_body_probe_rect(v11, (_DWORD *)(16 * (*v32__f) + a3 + 20), &(*v45__f), (*v39__f));
      if ( beh_480260(a2, &(*v45__f), (*v35__f), 0, 0) != 0 )
        break;
      (*v39__f) = v11[11] != 3 ? 3 : 1;
      W_body_probe_rect(v11, (_DWORD *)(16 * (*v32__f) + a3 + 20), &(*v45__f), (*v39__f));
      if ( beh_480260(a2, &(*v45__f), (*v35__f), 0, 0) != 0 )
        break;
      if ( ++(*v32__f) >= F16(a3, 16) )
        return 0;
    }
    v8 = 0;
  }
  if ( (*v39__f) == 99 )
    return 0;
  *a7 = (*v39__f);
  if ( F32(a3, 12) == 0 )
    return 10 * rnd() / 0x8000 + 200;
  (*v29__f) = 1;
  if ( a4 <= 1u )
    return (*v34__f);
  while ( 1 )
  {
    v13 = F16(c, 0x44) - 1;
    (*v49__f) = (*v29__f);
    (*v33__f) = v13;
    if ( v13 >= (*v29__f) )
      (*v33__f) = (*v29__f);
    v14 = v8 == v59;
    v8 = v61;
    if ( !v14 )
      v8 = v59;
    (*v40__f) = v8;
    memset(v51, 0, (5 * sizeof(intptr_t)));
    (*v52__f) = 0;
    (*v53__f) = 0;
    (*v54__f) = 0;
    (*v55__f) = 0;
    if ( (*v41__f) == 0 )
    {
      if ( (*v42__f) != 0 )
      {
        if ( (*v42__f) != 1 )
          goto LABEL_23;
        v21 = a3;
      }
      else
      {
        if ( F32((*v30__f), 44) != (*v39__f) )
        {
          v51[0] = 2 - ((*v39__f) != 1);
LABEL_23:
          (*v44__f) = 1;
          goto LABEL_24;
        }
        v21 = a3;
        if ( W_skill_pose_ok(F32(a3, 0), (uint16_t *)(*v30__f)) == 0 )
          goto LABEL_23;
        (*v42__f) = 1;
      }
      beh_479e40((int)v51, F32(v21, 4), 210, F32(v21, 8), a2);
      (*v41__f) = 1;
    }
LABEL_24:
    W_char_integrate(c, (int)v51, (*v30__f), (int)v8, 2, (*v33__f));
    if ( a5 != 0
      && (*v29__f) < F16(c, 0x44)
      && F16(F32(c, 0x750), 1098) == 0
      && F16(c, 0x14234) == 0
      && v8[125] == 0 )
    {
      if ( F16(v8, 2 * 28) != 5 || ((*v36__f) = 1, F32(c, 0x15a10) == 0) )
        (*v36__f) = 0;
      (*v31__f) = 0;
      v15 = F32(W, 0x2788);
      if ( F16(v15, 8) != 0 )
        break;
    }
LABEL_45:
    if ( (*v41__f) != 0 )
    {
      if ( F16(v8, 2 * 36) == 0 )
        goto LABEL_87;
      if ( ((*v49__f) % 4 == 0 || v8[127] == 1) && (*v38__f) == 0 )
      {
        if ( F32(a3, 12) != 1 )
        {
          if ( F32(a3, 12) == 2 )
          {
            if ( v8[127] == 1 )
            {
              v22 = 0;
              if ( F16(a3, 148) == 0 )
                return 0;
              while ( 1 )
              {
                W_body_probe_rect(v8, (_DWORD *)(16 * v22 + a3 + 152), v56, F32(v8, 4 * 11));
                if ( beh_480260(a2, v56, (*v35__f), 0, 0) != 0 )
                  break;
                if ( ++v22 >= F16(a3, 148) )
                  return 0;
              }
              (*v38__f) = (*v29__f);
            }
          }
          else if ( F32(a3, 12) == 4 && v8[127] == 1 )
          {
            if ( beh_47fd40(c, v8, a3, 1, (*v35__f), F32(v8, 4 * 11), 0) <= 0 )
              return 0;
            (*v38__f) = (*v29__f);
          }
          goto LABEL_73;
        }
        v23 = 0;
        (*v37__f) = 0;
        if ( F16(v8, 2 * 64) != 0 )
        {
          while ( 1 )
          {
            v24 = &v8[64 * v23 + 152];
            if ( F32(a2, 0) != 0 )
              break;
            if ( W_entity_rect_hit(a2, v24, (*v35__f)) )
              goto LABEL_71;
LABEL_72:
            v8 = (*v40__f);
            v23 = ++(*v37__f);
            if ( (uint16_t)(*v37__f) >= F16((*v40__f), 2 * 64) )
              goto LABEL_73;
          }
          v25 = 0;
          if ( F16(a2, 4) == 0 )
            goto LABEL_72;
          while ( 1 )
          {
            if ( F32(32 * v25 + F32(a2, 0), 16) != 0 )
            {
              v26 = (_DWORD *)(F32(a2, 64) + (*v50__f));
              if ( F32(v26[164] + 20 * v25, 16) == 0 )
              {
                W_body_frame_box(v25, v26, &(*v45__f));
                if ( (*v45__f) <= *v24 + v24[2] && (*v45__f) + (*v47__f) > *v24 )
                {
                  v27 = v24[1];
                  if ( (*v46__f) <= v27 + v24[3] && (*v46__f) + (*v48__f) > v27 )
                    break;
                }
              }
            }
            if ( ++v25 >= F16(a2, 4) )
              goto LABEL_72;
          }
LABEL_71:
          (*v38__f) = (*v29__f);
          goto LABEL_72;
        }
      }
    }
LABEL_73:
    (*v30__f) = (int)v8;
    if ( ++(*v29__f) >= a4 )
      goto LABEL_87;
  }
  while ( 1 )
  {
    v16 = F32((F32(v15, 4) + 4 * (*v31__f)), 0);
    if ( v16 != c )
    {
      if ( (v17 = F32(c, 0x1dc), v18 = F32(v16, 476), v17 != 0)
        && v18 != 0
        && (v17 == 3 || v18 == 3 || v17 != v18)
        || F32(v16, 496) == 1 )
      {
        v19 = W_body_hitbox_hit(v16, v8, (uint16_t)(*v33__f), F32(c, 0x0), F16(c, 0x4));
        v20 = v19;
        if ( v19 != 0 && W_char_hit_fresh(c, (int)v19, (*v33__f)) != 0 && (!W_char_hit_kind_ok(c, v20[3], v20[4]) || (*v36__f) == 0) )
          break;
      }
    }
    v15 = F32(W, 0x2788);
    if ( ++(*v31__f) >= F16(v15, 8) )
      goto LABEL_45;
  }
  if ( a6 != 0 )
    return 0;
  (*v43__f) = (*v29__f);
LABEL_87:
  if ( (*v38__f) == 0 )
    return (*v34__f);
  v28 = 10 * rnd() / 0x8000 + 200;
  (*v34__f) = v28;
  if ( (*v44__f) != 0 )
  {
    v28 -= 20;
    (*v34__f) = v28;
  }
  if ( (*v43__f) == 0 || a5 == 0 )
    return (*v34__f);
  if ( (*v38__f) >= (*v43__f) )
  {
    if ( (*v38__f) == (*v43__f) )
      return v28 - 100;
    return 0;
  }
  if ( F32(a3, 304) != 0 )
    return v28 + 100;
  else
    return v28 - 50;
}

/* 0x48bdd0: int __thiscall sub_48BDD0(cls_48BDD0 *this, int a2, int a3) */
int32_t beh_48bdd0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v4;
    int32_t * v6;
    intptr_t v7;

  v3 = F32(c, 0x40);
  if ( F32(c, 0x15a5c) != 0 )
  {
    if ( F32(c, 0x200) != 0 )
    {
      if ( F16(c, 0x1422c) == 6 )
      {
        v6 = (int *)F32(c, 0x14818);
        if ( v6 != 0 )
        {
          v7 = *v6;
          if ( v7 != 0 && F16(v7, 174) == a2 && F16(v3, 102) >= a3 )
            return 1;
        }
      }
    }
    else if ( F16(v3, 102) != 0 )
    {
      v4 = F32(v3, 76);
      if ( v4 != 0 && F16(v4, 174) == a2 )
        return 1;
    }
  }
  return 0;
}

/* 0x48be60: int __thiscall sub_48BE60(cls_48BE60 *this, unsigned __int16 a2, int a3) */
int32_t beh_48be60(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t result;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    char * v14;
    _Alignas(8) uint8_t frame_[0x40c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Format = (char *)(frame_ + 0xc);   /* [512] at ebp-0x400 */
    char *Buffer = (char *)(frame_ + 0x20c);   /* [512] at ebp-0x200 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x40c */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x408 */

  v3 = 0;
  v4 = W;
  (*v16__f) = 0;
  (*v15__f) = 0;
  do
  {
    v5 = *(_DWORD *)&GAP(v4, 0x1030, 4 * v3 + 304);
    if ( F32(v5, 464) == 1 && F8(F32(v5, 64), 0) == 1 )
    {
      v6 = F32(v5, 1872);
      if ( F16(v6, 1098) == 0 )
      {
        v7 = F32(v5, 476);
        v8 = F32(c, 0x1dc);
        if ( v8 != 0 && v7 != 0 && v8 != 3 && v7 != 3 && v8 == v7 && F16(v6 + 2 * a2, 1096) != 0 )
        {
          result = 1;
          (*v16__f) = 1;
          if ( a3 == 0 )
            return result;
          v10 = 0;
          if ( F16(v6 + 2 * a2, 1096) != 0 )
          {
            F16(v6 + 2 * a2, 1096) = 0;
            switch ( a2 )
            {
              case 1u:
                strcpy(Format, D(0x85530c));
                v11 = F32(v5, 88336);
                F32(v5, 252) = v11;
                F32(v5, 256) = v11;
                goto LABEL_20;
              case 2u:
                strcpy(Format, D(0x855340));
                goto LABEL_20;
              case 3u:
                strcpy(Format, D(0x855324));
                goto LABEL_20;
              case 5u:
                strcpy(Format, D(0x855354));
                goto LABEL_20;
              case 6u:
                strcpy(Format, D(0x855388));
                goto LABEL_20;
              case 7u:
                strcpy(Format, D(0x855360));
LABEL_20:
                v10 = 1;
                break;
              default:
                break;
            }
            if ( F32(v5, 51092) != 0 && v10 != 0 && W_char_room_logs(v5) != 0 )
            {
              g_eng.h5bf3ee(Buffer, Format, F32(v5, 1872));
              v4 = W;
              v12 = 0;
              v13 = F32(W, 0x1024);
              while ( *(_DWORD *)&GAP(W, 0x0, 516 * v13 + 4) != 0 )
              {
                v13 = (v13 + 1) % 8;
                if ( (uint16_t)++v12 >= 8u )
                  goto LABEL_30;
              }
              v14 = &GAP(W, 0x0, 516 * v13);
              F32(v14, 4 * 1) = 1;
              strcpy(v14 + 8, Buffer);
            }
            v4 = W;
          }
        }
      }
    }
LABEL_30:
    v3 = ++(*v15__f);
  }
  while ( (uint16_t)(*v15__f) < 0x20u );
  return (*v16__f);
}

/* 0x479e40: int __stdcall sub_479E40(int a1, __int16 a2, __int16 a3, __int16 a4, int a5) */
int32_t beh_479e40(int32_t a1, int16_t a2, int16_t a3, int16_t a4, int32_t a5)
{


  F16(a1, 20) = a2;
  F16(a1, 28) = a3;
  F16(a1, 22) = a4;
  F32(a1, 24) = a5;
  return a1;
}

/* 0x481790: _DWORD *__thiscall sub_481790( cls_481790 *this, unsigned __int16 a2, int a3, int a4, _DWORD *a5, _DWORD *a6, int *a7, _DWORD *a8) */
int32_t beh_481790(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6, int32_t * a7, int32_t * a8)
{
    intptr_t c = (intptr_t)self;
    uint16_t v8;
    uint16_t v9;
    uint16_t v10;
    int32_t * v11;
    intptr_t v12;
    int64_t v13;
    intptr_t v14;
    int v15;
    intptr_t v16;
    intptr_t v17;
    int32_t * v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    uint16_t v27;
    uint16_t v28;
    intptr_t v29;
    uint16_t v30;
    intptr_t v31;
    uint16_t v32;
    intptr_t v39;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  (*v38__f) = c;
  if ( F32(c, 0x0) != 0 )
  {
    v8 = F16(c, 0x4);
    v9 = a2;
    v10 = -1;
    (*v35__f) = -1;
    if ( v8 != 0 )
    {
      v11 = (_DWORD *)(*(_DWORD *)&GAP(c, 0x24, 28) + 660 * a2);
      v12 = 0;
      (*v33__f) = 0;
      (*v36__f) = v8;
      do
      {
        (*v34__f) = F32((v12 + v11[164]), 0);
        if ( v11[11] == 3 )
          (*v34__f) = -F32((v12 + v11[164]), 0);
        v10 = (*v35__f);
        v13 = a3 - ((*v34__f) + v11[1] + v11[3] / 2);
        v14 = (HIDWORD(v13) ^ v13) - HIDWORD(v13) + abs32(a4 - (v11[2] + F32(v12 + v11[164], 4) + v11[4] / 2));
        if ( (*v35__f) == -1 || v14 < (*v37__f) )
        {
          v10 = (*v33__f);
          (*v37__f) = v14;
          (*v35__f) = (*v33__f);
        }
        v12 += 20;
        v15 = (*v36__f) == 1;
        ++(*v33__f);
        --(*v36__f);
      }
      while ( !v15 );
      c = (*v38__f);
      v9 = a2;
    }
    v16 = *(_DWORD *)&GAP(c, 0x24, 28);
    v17 = 5 * v10;
    v18 = (_DWORD *)(v16 + 660 * v9);
    v19 = v18[164];
    v20 = F32((v19 + 20 * v10), 0);
    v21 = v19 + 4 * v17;
    if ( v18[11] == 3 )
      v20 = -v20;
    v39 = F32(v21, 8);
    v22 = v20 + v18[1] + v18[3] / 2 - v39 / 2;
    v23 = F32(v21, 4);
    v24 = v18[2];
    v25 = v23 + v18[4] / 2;
    *a5 = v22 + v39 / 2;
    *a6 = v24 + v25;
    *a7 = v22;
    *a8 = v22 + v39;
    return (_DWORD *)(v22 + v39);
  }
  else
  {
    v27 = a2;
    v28 = a2;
    if ( a2 >= F16(c, 0x44) )
      v28 = 0;
    v29 = *(_DWORD *)&GAP(c, 0x24, 28) + 660 * v28;
    *a5 = F32(v29, 4) + F32(v29, 12) / 2;
    v30 = a2;
    if ( a2 >= F16(c, 0x44) )
      v30 = 0;
    v31 = *(_DWORD *)&GAP(c, 0x24, 28);
    *a6 = F32(v31 + 660 * v30, 20)
        + F32(v31 + 660 * v30, 8)
        + (F32(v31 + 660 * v30, 16) - F32(v31 + 660 * v30, 20)) / 2;
    v32 = a2;
    if ( a2 >= F16(c, 0x44) )
      v32 = 0;
    *a7 = F32(*(_DWORD *)&GAP(c, 0x24, 28) + 660 * v32, 4);
    if ( a2 >= F16(c, 0x44) )
      v27 = 0;
    *a8 = F32(*(_DWORD *)&GAP(c, 0x24, 28) + 660 * v27, 4)
        + F32(*(_DWORD *)&GAP(c, 0x24, 28) + 660 * v27, 12);
    return a8;
  }
}

/* 0x481a20: int __thiscall sub_481A20(cls_481A20 *this, unsigned __int16 a2, unsigned __int16 a3) */
int32_t beh_481a20(wl_char *self, uint16_t a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    int64_t v4;

  if ( a2 <= 1000 * rnd() / 0x8000 )
    return 0;
  v4 = a3 * F16(c, 0xc87c) / 1000;
  F16(c, 0x14234) = (((int)v4 - HIDWORD(v4)) >> 1)
                    + (uint16_t)(((int)v4 - HIDWORD(v4)) >> 1) * rnd() / 0x8000;
  return 1;
}

/* 0x479e10: int __stdcall sub_479E10(int a1, __int16 a2, __int16 a3, int a4) */
int32_t beh_479e10(int32_t a1, int16_t a2, int16_t a3, int32_t a4)
{


  F16(a1, 20) = a2;
  F16(a1, 28) = 210;
  F16(a1, 22) = a3;
  F32(a1, 24) = a4;
  return a1;
}

/* 0x47a970: _DWORD *__thiscall sub_47A970(cls_4022A0 *this, int a2, int *a3, _DWORD *a4, int a5, int a6, int i, int a8) */
int32_t beh_47a970(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5, int32_t a6, int32_t i, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    int32_t * v8;
    uint16_t v9;
    uint16_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    uint16_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  v8 = (int *)a2;
  (*v27__f) = c;
  (*v26__f) = 0;
  (*v29__f) = 0;
  (*v28__f) = 0;
  (*v25__f) = 0;
  (*v30__f) = 0;
  switch ( F32(a2, 184) )
  {
    case 1:
      goto LABEL_11;
    case 2:
      v16 = a5;
      v17 = 4448;
      i = 32;
      while ( 1 )
      {
        v18 = *(intptr_t *)&GAP(W, 0x0, v17);
        if ( F32(v18, 0x1d0) != 0
          && beh_47ae00(v18, (int)c, v8[44], v8[177], v8[178], v8[179], v8[180], &a6, &a2, v16, 1, 0, 0) != 0
          && a6 > 0
          && a6 >= a8 )
        {
          if ( (*v26__f) == 0 )
            (*v26__f) = v18;
          if ( (*v29__f) <= a6 )
            (*v29__f) = a6;
          if ( (*v25__f) <= a2 )
            (*v25__f) = a2;
          ++(*v28__f);
        }
        v17 += 4;
        if ( --i == 0 )
          break;
        c = (*v27__f);
      }
      goto LABEL_60;
    case 3:
      if ( a6 == 0 || F32(a2, 176) == 14 )
        goto LABEL_11;
      v9 = F16(a2, 708);
      v10 = 0;
      while ( 1 )
      {
        v11 = *(_DWORD *)&GAP(W, 0x1030, 4 * v10 + 304);
        if ( F32(v11, 464) == 1
          && F8(F32(v11, 64), 0) == 1
          && F16(F32(v11, 1872), 1098) == 0
          && W_faction_relation(F32((*v27__f), 0x1dc), F32(v11, 476)) == 2
          && F16(F32(v11, 1872) + 2 * v9, 1096) != 0 )
        {
          goto LABEL_60;
        }
        if ( ++v10 >= 0x20u )
        {
LABEL_11:
          v12 = i;
          if ( i != 0 )
          {
            if ( F32(i, 464) != 0
              && beh_47ae00(
                   i,
                   (int)(*v27__f),
                   v8[44],
                   v8[177],
                   v8[178],
                   v8[179],
                   v8[180],
                   &a6,
                   &a2,
                   a5,
                   1,
                   0,
                   0) != 0
              && a6 > 0
              && a6 >= a8 )
            {
              (*v29__f) = a6;
              (*v26__f) = v12;
              (*v25__f) = a2;
              (*v28__f) = 1;
            }
          }
          else
          {
            v13 = a5;
            v14 = 4448;
            for ( i = 32; i != 0; --i )
            {
              v15 = *(intptr_t *)&GAP(W, 0x0, v14);
              if ( F32(v15, 0x1d0) != 0
                && beh_47ae00(v15, (int)(*v27__f), v8[44], v8[177], v8[178], v8[179], v8[180], &a6, &a2, v13, 1, 0, &(*v31__f)) != 0
                && a6 > 0 )
              {
                if ( (*v26__f) == 0 || (*v30__f) < (*v31__f) && a6 >= a8 )
                {
                  (*v30__f) = (*v31__f);
                  (*v26__f) = v15;
                  (*v29__f) = a6;
                  (*v25__f) = a2;
                }
                ++(*v28__f);
              }
              v14 += 4;
            }
          }
          goto LABEL_60;
        }
      }
    case 4:
      if ( a6 == 0 || F32(a2, 176) == 14 )
        goto LABEL_56;
      v19 = 0;
      break;
    default:
      goto LABEL_60;
  }
  while ( 1 )
  {
    v20 = *(_DWORD *)&GAP(W, 0x1030, 4 * v19 + 304);
    if ( F32(v20, 464) == 1 && F8(F32(v20, 64), 0) == 1 )
    {
      v21 = F32(v20, 1872);
      if ( F16(v21, 1098) == 0 )
      {
        v22 = F32(c, 0x1dc);
        v23 = F32(v20, 476);
        if ( v22 != 0
          && v23 != 0
          && v22 != 3
          && v23 != 3
          && v22 == v23
          && F16(v21 + 2 * F16(a2, 708), 1096) != 0 )
        {
          break;
        }
      }
    }
    c = (*v27__f);
    if ( ++v19 >= 0x20u )
    {
LABEL_56:
      if ( beh_47ae00(
             c,
             (int)c,
             F32(a2, 176),
             F32(a2, 708),
             F32(a2, 712),
             F32(a2, 716),
             F32(a2, 720),
             &a6,
             &a2,
             0,
             1,
             0,
             0) != 0
        && a6 > 0
        && a6 >= a8 )
      {
        (*v29__f) = a6;
        (*v26__f) = (*v27__f);
      }
      break;
    }
  }
LABEL_60:
  if ( v8[46] == 2 && (*v28__f) > 0 )
  {
    if ( (*v28__f) < 2 )
      (*v25__f) /= 2;
    else
      (*v25__f) *= 2;
  }
  if ( a3 != 0 )
    *a3 = (*v29__f);
  if ( a4 != 0 )
    *a4 = (*v25__f);
  return &F32((*v26__f), 0x0);
}

/* 0x4826b0: BOOL __thiscall sub_4826B0(cls_4826B0 *this, _DWORD *a2) */
int32_t beh_4826b0(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;

  result = true;
  switch ( F32(c, 0x1c0) )
  {
    case 0:
    case 1:
      result = a2 == (_DWORD *)c;
      break;
    case 2:
      result = true;
      break;
    case 3:
      result = false;
      break;
    default:
      return result;
  }
  return result;
}

/* 0x482700: BOOL __thiscall sub_482700(cls_4022A0 *this, int a2, int a3, signed int a4, signed int a5) */
int32_t beh_482700(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x14258) == 0 )
    return false;
  if ( a4 != 0 && (int)abs32(a2 - F32(c, 0x1425c)) > a4 )
    return false;
  return a5 == 0 || (int)abs32(a3 - F32(c, 0x14260)) <= a5;
}

/* 0x482760: BOOL __thiscall sub_482760(cls_4022A0 *this, int a2, int a3, signed int a4, signed int a5, int a6) */
int32_t beh_482760(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    int32_t * v8;
    intptr_t v9;
    intptr_t v10;
    _Alignas(8) uint8_t frame_[0x100 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [256] at ebp-0x100 */

  v7 = F32(c, 0x1424c);
  if ( v7 == 0 )
    return true;
  if ( F16(v7, 68) == 0 )
  {
    g_eng.h5bf3ee(Buffer, "%d", F32(v7, 468));
    g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85451c), Buffer);
  }
  v8 = F32(v7, 64);
  v9 = F32(v7, 51068);
  if ( a6 != 0 )
  {
    v10 = F32(c, 0x14250);
    v9 = F32(c, 0x14244);
  }
  else
  {
    v10 = F32(c, 0x14254);
  }
  if ( v9 != 3 )
    v10 = -v10;
  if ( a4 != 0 && (int)abs32(a2 - (v10 + v8[1] + v8[3] / 2 - F32(F32(c, 0x40), 12) / 2)) > a4 )
    return false;
  return a5 == 0 || (int)abs32(a3 - (v8[4] + v8[2] - 1)) <= a5;
}

/* 0x483040: int __thiscall sub_483040(cls_4022A0 *this, _DWORD *a2, int a3, int a4, int a5, int *a6) */
int32_t beh_483040(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6;
    int32_t * v7;
    intptr_t v8;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    int32_t * v14;
    uint32_t v15;
    int v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    uint16_t v21;
    intptr_t v22;
    intptr_t v23;
    int32_t * v24;
    int16_t v25;
    intptr_t v26;
    uint32_t v27;
    _Alignas(8) uint8_t frame_[0x508 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v31 = (uint16_t *)(frame_ + 0x8);   /* [2] at ebp-0x500 */
    intptr_t *v32 = (intptr_t *)(frame_ + 0xc);   /* [319] at ebp-0x4fc */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x508 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x508 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x504 */

  v6 = a2[161];
  if ( v6 != 0 )
  {
    v7 = F32(v6, 64);
    if ( a4 + 1 >= v7[2] + v7[5] && a4 + 1 <= v7[4] + v7[2] - 1 )
    {
      v8 = F32(v6, 64);
      if ( a3 >= F32(v8, 4) && a3 <= F32(v8, 4) + F32(v8, 12) )
      {
        if ( a6 != 0 )
          *a6 = a3;
        return 1;
      }
    }
  }
  if ( F32(c, 0x15a90) != 0 )
    (*v28__f) = F32(c, 0xc7ec) == 0 && F16(F32(c, 0x750), 1118) == 0;
  else
    (*v28__f) = false;
  v10 = a2[4];
  v11 = a2[3];
  v12 = a2[2];
  v13 = a2[1];
  v14 = *(_DWORD **)&GAP(W, 0x1030, 32);
  *v14 = *(_DWORD *)&GAP(W, 0x1030, 24);
  v15 = (int)((uint64_t)(1374389535LL * v13) >> 32) >> 10;
  v14[10246] = 0;
  v16 = (int)((v15 >> 31) + v15) < 0;
  v17 = (v15 >> 31) + v15;
  if ( F32(*v14, 2887728) - 1 >= (v16 ? 0 : v17) )
    v18 = v17 < 0 ? 0 : v17;
  else
    v18 = F32(*v14, 2887728) - 1;
  v14[10570] = v10;
  v19 = (v12 + v10 - 1) / 3200;
  v14[10569] = (v11 + 3199) / 3200;
  v20 = beh_4832e0(v14, v18, v19, a3 / 3200, a4 / 3200, (*v28__f));
  (*v30__f) = v20;
  if ( a5 == -1 || v14[10245] <= a5 )
  {
    if ( v20 != 0 )
      goto LABEL_22;
    v20 = beh_4832e0(v14, v18, v19, v14[10243], v14[10244], (*v28__f));
    (*v30__f) = v20;
  }
  if ( v20 == 0 )
    return 0;
LABEL_22:
  v21 = 0;
  v22 = *((uint16_t *)v14 + 20496);
  v23 = 0;
  (*v29__f) = 0;
  if ( (uint16_t)v22 != 0 )
  {
    v24 = v14 + 10255;
    do
    {
      v25 = *((uint16_t *)v24 - 12);
      if ( v25 != 0 || v23 >= v22 - 1 || *((uint16_t *)v24 - 2) != 0 || *(v24 - 5) > *(v24 - 3) != *v24 > v24[2] )
      {
        v26 = *(v24 - 5);
        v27 = 20 * v21++;
        v31[v27 / 2] = v25;
        v32[v27 / 4] = v26;
        v32[v27 / 4 + 1] = *(v24 - 4);
        v32[v27 / 4 + 2] = *(v24 - 3);
        v32[v27 / 4 + 3] = *(v24 - 2);
      }
      v24 += 5;
      v23 = ++(*v29__f);
    }
    while ( (*v29__f) < v22 );
    v20 = (*v30__f);
  }
  if ( v20 == 0 || v21 != 1 || v31[0] != 0 )
    return 0;
  if ( a6 != 0 )
    *a6 = 3200 * v32[0];
  return 1;
}

/* 0x484830: int __thiscall sub_484830(cls_4022A0 *this, int a2, int a3) */
int32_t beh_484830(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;

  v3 = F32(a2, 644);
  if ( v3 == 0 )
    return 0;
  switch ( F32(v3, 448) )
  {
    case 0:
    case 1:
      if ( F32(c, 0x14238) == v3 )
        return 0;
      break;
    case 3:
      break;
    default:
      return 0;
  }
  F16(c, 0x1422c) = 7;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  F32(c, 0x14824) = a3;
  return 1;
}

/* 0x484d60: int __thiscall sub_484D60(cls_484D60 *this, int a2) */
int32_t beh_484d60(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2;
    intptr_t v3;
    int32_t * v4;
    uint16_t v5;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t i;

  v2 = F32(c, 0x40);
  v3 = F32(v2 + 660 * (uint16_t)a2, 644);
  v4 = (_DWORD *)(v2 + 660 * (uint16_t)a2);
  if ( v3 != 0 )
  {
    v5 = a2;
    if ( (uint16_t)a2 >= F16(v3, 68) )
      v5 = 0;
    return F32(F32((v3 + 64), 0) + 660 * v5, 20) + F32(F32((v3 + 64), 0) + 660 * v5, 8) - 1;
  }
  else
  {
    v7 = v4[1];
    v8 = *(_DWORD *)&GAP(W, 0x1030, 24);
    v9 = F32(v8, 2887728) - 1;
    if ( v9 >= (v7 / 3200 < 0 ? 0 : v7 / 3200) )
      v10 = v7 / 3200 < 0 ? 0 : v7 / 3200;
    else
      v10 = F32(v8, 2887728) - 1;
    v11 = (v4[3] + v7 - 1) / 3200;
    if ( v9 >= (v11 < 0 ? 0 : v11) )
      v9 = v11 < 0 ? 0 : v11;
    for ( i = (v4[4] + v4[2] - 1) / 3200 + 1;
          i < F32(v8, 2887732)
       && F16(v8 + 2 * (i + (v10 << 7)), 2887744) == 0
       && F16(v8 + 2 * (i + (v9 << 7)), 2887744) == 0;
          ++i )
    {
      ;
    }
    if ( &a2 == 0 )
      i = a2;
    return 3200 * i - 3200;
  }
}

/* 0x485330: int __thiscall sub_485330(cls_4022A0 *this) */
int32_t beh_485330(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;

  result = 9;
  if ( F32(c, 0x1f0) == 0 )
    return F32(c, 0x14240);
  return result;
}

/* 0x483920: int __stdcall sub_483920(cls_4022A0 *a1, _DWORD *a2, _DWORD *a3, unsigned __int16 a4) */
int32_t beh_483920(int32_t a1, int32_t * a2, int32_t * a3, uint16_t a4)
{

    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    uint16_t v8;
    intptr_t v9;
    uint16_t v10;
    uint16_t v11;
    uint16_t v12;
    intptr_t v14;
    int32_t * v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    int32_t * v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v28;
    intptr_t v29;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v23__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x14 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x10 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xc */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x4 */

  if ( a2[11] == 1 )
  {
    v4 = *a3 + a2[1] + a2[3];
    (*v24__f) = v4;
  }
  else
  {
    (*v24__f) = a2[1] - a3[1] - *a3;
    v4 = (*v24__f);
  }
  v5 = a1;
  v6 = F32(a1, 0x0);
  v7 = a3[2] + a2[2];
  (*v25__f) = v7;
  (*v26__f) = a3[1];
  (*v27__f) = a3[3];
  if ( F32(a1, 0x0) != 0 )
  {
    v14 = 0;
    v29 = 0;
    (*v23__f) = F32(a1, 0x4);
    if ( (*v23__f) != 0 )
    {
      do
      {
        if ( F32(32 * (uint16_t)v14 + v6, 16) != 0 )
        {
          v15 = (_DWORD *)(F32(v5, 0x40) + 660 * a4);
          v16 = 20 * (uint16_t)v14;
          v17 = v15[164];
          if ( F32(v17 + v16, 16) == 0 )
          {
            v18 = F32((v17 + 20 * (uint16_t)v14), 0);
            v19 = (_DWORD *)(v17 + v16);
            if ( v15[11] == 3 )
              v18 = -v18;
            v20 = v19[2];
            v28 = v19[3];
            v21 = v18 + v15[1] + v15[3] / 2 - v20 / 2;
            v22 = v15[2] + v19[1] + v15[4] / 2 - v28 / 2;
            if ( v21 <= (*v24__f) + (*v26__f) && v21 + v20 > (*v24__f) && v22 <= (*v25__f) + (*v27__f) && v22 + v28 > (*v25__f) )
              return 1;
            v5 = a1;
            v14 = v29;
            v6 = F32(a1, 0x0);
          }
        }
        v29 = ++v14;
      }
      while ( (uint16_t)v14 < (*v23__f) );
    }
  }
  else
  {
    v8 = a4;
    if ( a4 >= F32(a1, 0x44) )
      v8 = 0;
    v9 = F32(a1, 0x40);
    if ( F32(v9 + 660 * v8, 4) <= v4 + (*v26__f) )
    {
      v10 = a4;
      v11 = F32(a1, 0x44);
      if ( a4 >= v11 )
        v10 = 0;
      if ( F32(v9 + 660 * v10, 4) + F32(v9 + 660 * v10, 12) > v4 )
      {
        v12 = a4;
        if ( a4 >= v11 )
          v12 = 0;
        if ( F32(v9 + 660 * v12, 8) + F32(v9 + 660 * v12, 20) <= v7 + (*v27__f) && W_part_bottom(a1, a4) > v7 )
          return 1;
      }
    }
  }
  return 0;
}

/* 0x4848a0: int __thiscall sub_4848A0(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t beh_4848a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    uint16_t v8;
    uint16_t v9;
    char * v10;
    intptr_t v11;
    char * v12;
    uint16_t v13;
    char * v14;
    intptr_t v15;
    int32_t * v16;
    int32_t * v17;
    intptr_t v18;
    int64_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    uint16_t i;
    intptr_t v27;

  v8 = F16(c, 0x147a0);
  v9 = F16(c, 0x1429c);
  if ( v8 >= v9 )
    goto LABEL_20;
  v10 = (uint8_t *)c + 20 * v8;
  v11 = *((uint16_t *)v10 + 41296);
  if ( (uint16_t)v11 == 0 )
  {
    if ( v8 == 0 && v9 == 1 )
    {
      v20 = F32(c, 0x147a4) - F32(a2, 4);
      if ( (int)((HIDWORD(v20) ^ v20) - HIDWORD(v20)) <= 1600 )
      {
        F16(c, 0x147a0) = 1;
        goto LABEL_20;
      }
    }
    if ( v8 == F16(c, 0x1429c) - 1 )
      v23 = a6;
    else
      v23 = 3200 * F32(v10, 4 * 20649);
    v21 = a2;
    v24 = F32(a2, 12);
    v25 = F32(a2, 4);
    if ( v24 <= 4000 )
    {
      if ( (int)abs32(v23 - v25) >= 1600 )
        goto LABEL_32;
    }
    else if ( v24 / 2 + 1600 < (int)abs32(v23 - v25) )
    {
LABEL_32:
      W_npc_navigate(c, v23, 1600, a2, a3, 0, 0, 0, a7, 0);
      goto LABEL_21;
    }
    F16(c, 0x147a0) = v8 + 1;
    goto LABEL_21;
  }
  if ( v11 != 1 )
  {
LABEL_20:
    v21 = a2;
LABEL_21:
    if ( F16(c, 0x147a0) >= F16(c, 0x1429c) )
    {
      v22 = a6;
      if ( a5 == 0 )
        v22 = F32(v21, 4);
      if ( W_npc_navigate(c, v22, 800, v21, a3, 0, 0, 0, a7, 0) != 0 )
      {
        if ( a4 == 99 )
          return 1;
        if ( F32(c, 0x14854) == 0 )
        {
          if ( F32(v21, 44) == a4 )
            return 1;
          F32(c, 0x14854) = 2 - (a4 != 1);
        }
      }
    }
    return 0;
  }
  v12 = (uint8_t *)c + 20 * v8;
  v27 = *(_DWORD *)&GAP(W, 0x1030, 24);
  if ( *(_WORD *)(v27 + 2 * (F32(v12, 4 * 20652) + (F32(v12, 4 * 20651) << 7)) + 2887746) != 0 )
  {
    v13 = 1;
    for ( i = v8; i < v9; ++i )
    {
      v14 = (uint8_t *)c + 20 * i;
      if ( F16(v14, 2 * 41296) != 1 )
        break;
      ++v13;
      if ( *(_WORD *)(v27 + 2 * ((F32(v14, 4 * 20649) << 7) + *(_DWORD *)&GAP(c, 0x1429e, 20 * i + 10)) + 2887746) != 0 )
      {
        if ( v13 <= 8u )
        {
          F16(c, 0x147ae) = v13;
          F16(c, 0x147f0) = 0;
          F16(c, 0x147f2) = 80;
          v15 = *(_DWORD *)&GAP(W, 0x1030, 24)
              + 48 * (F32(v12, 4 * 20652) + (F32(v12, 4 * 20651) << 7));
          F32(c, 0x147b0) = F32(v15, 1904728);
          F32(c, 0x147b4) = F32(v15, 1904732);
          if ( v13 > 1u )
          {
            v16 = (int *)(v12 + 82596);
            v17 = &F32(c, 0x147b8);
            v18 = v13 - 1;
            do
            {
              beh_484bc0(*(intptr_t *)&GAP(W, 0x1030, 24), *v16, v16[1], v17, v17 + 1);
              v16 += 5;
              v17 += 2;
              --v18;
            }
            while ( v18 != 0 );
          }
          F16(c, 0x1422c) = 1;
          F16(c, 0x1422e) = 0;
          F16(c, 0x14230) = 0;
        }
        break;
      }
    }
  }
  F16(c, 0x1429c) = 0;
  F16(c, 0x147a0) = 0;
  return 0;
}

/* 0x442640: int __thiscall sub_442640(cls_4022A0 *this, _DWORD *a2, int a3, int a4, int a5, int a6) */
int32_t beh_442640(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    int64_t v8;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  (*v10__f) = 0;
  v7 = W_npc_navigate(c, a4, 800, (int)a2, a3, 0, 1, a5, 0, 0);
  if ( F32(c, 0x14854) == 0 && F32(c, 0x14860) == 0 && F32(c, 0x1485c) == 0 && (int)abs32(a4 - a2[1]) <= 1600 )
  {
    v8 = a5 - F32(c, 0xc890) - a2[4] - a2[2];
    if ( (int)((HIDWORD(v8) ^ v8) - HIDWORD(v8)) <= 1600 && v7 != 0 )
    {
      if ( a6 != 99 && a2[11] != a6 )
      {
        F32(c, 0x14854) = 2 - (a6 != 1);
        return 0;
      }
      return 1;
    }
  }
  return (*v10__f);
}

/* 0x4843c0: int __thiscall sub_4843C0(cls_4022A0 *this, _DWORD *a2, _DWORD *a3, _DWORD *a4, unsigned __int16 a5, int a6) */
int32_t beh_4843c0(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4, uint16_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    uint16_t v6;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    uint16_t v12;
    uint16_t v13;
    intptr_t v14;
    int64_t v15;
    uint16_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t result;
    _Alignas(8) uint8_t frame_[0x104 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x4);   /* [256] at ebp-0x100 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x104 */

  v6 = a5;
  v8 = F32(c, 0x1424c);
  v9 = a5;
  if ( a5 >= (int)F16(v8, 68) )
  {
    v9 = 0;
    g_eng.h5bf3ee(Buffer, "%d", F32(v8, 468));
    g_eng.log_fail_406440((const char *)D(0x8a9530), D(0x85451c), Buffer);
  }
  v10 = F32(v8, 51068);
  if ( F32(F32((v8 + 64), 0) + 660 * v9, 44) != v10 )
    v6 = 0;
  if ( a6 != 0 )
  {
    v11 = F32(c, 0x14250);
    v10 = F32(c, 0x14244);
  }
  else
  {
    v11 = F32(c, 0x14254);
  }
  if ( v10 == 3 )
  {
    (*v21__f) = v11;
    if ( a4 != 0 )
      *a4 = 3;
  }
  else
  {
    (*v21__f) = -v11;
    if ( a4 != 0 )
      *a4 = 1;
  }
  v12 = v6;
  if ( v6 >= F16(v8, 68) )
    v12 = 0;
  v13 = v6;
  if ( v6 >= F16(c, 0x44) )
    v13 = 0;
  v14 = F32(v8, 64) + 660 * v12;
  v15 = F32(F32(c, 0x40) + 660 * v13, 12);
  *a2 = (*v21__f) + F32(v14, 4) + F32(v14, 12) / 2 - (((int)v15 - HIDWORD(v15)) >> 1);
  v16 = v6;
  if ( v6 >= F16(v8, 68) )
    v16 = 0;
  v17 = F32(v8, 64);
  v18 = 165 * v16;
  v19 = F32(v17 + 4 * v18, 16);
  result = F32(v17 + 4 * v18, 8);
  *a3 = v19 + result - 1;
  return result;
}

/* 0x442710: int __thiscall sub_442710(cls_4022A0 *this, unsigned __int16 a2) */
int32_t beh_442710(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2;
    intptr_t v3;
    char * v4;
    uint16_t i;
    char * v6;
    intptr_t v8;
    char * v9;
    int32_t * v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;

  v2 = a2;
  v3 = *(_DWORD *)&GAP(c, 0x1429e, 20 * a2 + 18);
  v4 = (uint8_t *)c + 20 * a2;
  v15 = *(_DWORD *)&GAP(W, 0x1030, 24);
  if ( *(_WORD *)(v15 + 2 * (v3 + (F32(v4, 4 * 20651) << 7)) + 2887746) != 0 )
  {
    for ( i = 1; v2 < F16(c, 0x1429c); ++v2 )
    {
      v6 = (uint8_t *)c + 20 * v2;
      if ( F16(v6, 2 * 41296) != 1 )
        break;
      ++i;
      if ( *(_WORD *)(v15 + 2 * ((F32(v6, 4 * 20649) << 7) + *(_DWORD *)&GAP(c, 0x1429e, 20 * v2 + 10)) + 2887746) != 0 )
      {
        if ( i > 8u )
          return 0;
        F16(c, 0x147ae) = i;
        F16(c, 0x147f0) = 0;
        F16(c, 0x147f2) = 80;
        v8 = *(_DWORD *)&GAP(W, 0x1030, 24) + 48 * (F32(v4, 4 * 20652) + (F32(v4, 4 * 20651) << 7));
        F32(c, 0x147b0) = F32(v8, 1904728);
        F32(c, 0x147b4) = F32(v8, 1904732);
        if ( i > 1u )
        {
          v9 = ((uint8_t *)(c) + 0x147bc);
          v10 = (int *)(v4 + 82596);
          v11 = i - 1;
          do
          {
            v12 = *v10;
            v13 = v10[1];
            v10 += 5;
            v9 += 8;
            v14 = *(_DWORD *)&GAP(W, 0x1030, 24) + 48 * (v13 + (v12 << 7));
            --v11;
            *((int32_t *)v9 - 3) = F32(v14, 1904728);
            *((int32_t *)v9 - 2) = F32(v14, 1904732);
          }
          while ( v11 != 0 );
        }
        F16(c, 0x1422e) = 0;
        F16(c, 0x14230) = 0;
        F16(c, 0x1422c) = 1;
        return 1;
      }
    }
  }
  return 0;
}

/* 0x485cb0: int __thiscall sub_485CB0(cls_4022A0 *this, _DWORD *a2, int a3, int a4, int a5, int *a6) */
int32_t beh_485cb0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7;
    int32_t * v8;
    intptr_t v9;
    intptr_t v10;
    int32_t * v11;
    intptr_t v12;
    intptr_t result;
    intptr_t v14;
    int v15;
    intptr_t v16;
    int32_t * v17;
    intptr_t v18;
    uint16_t v19;
    intptr_t v20;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v21 = (uint8_t *)(frame_ + 0x0);   /* [16] at ebp-0x20 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x10 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x4 */

  v7 = 660 * (uint16_t)a4;
  v8 = (_DWORD *)(v7 + F32(c, 0x40));
  v9 = v8[1] + v8[3] / 2;
  beh_481790(a3, a4, v9, v8[2] + v8[4] / 2, &(*v22__f), &(*v25__f), &(*v23__f), &(*v24__f));
  if ( v9 >= (*v22__f) )
  {
    if ( v21 != (_BYTE *)-44 )
    {
      v10 = (*v24__f) + a5;
      goto LABEL_7;
    }
  }
  else if ( v21 != (_BYTE *)-44 )
  {
    v10 = (*v23__f) - v8[3] - a5;
    goto LABEL_7;
  }
  v10 = a4;
LABEL_7:
  v11 = a2;
  v12 = a2[161];
  if ( v12 != 0 )
  {
    switch ( F32(v12, 448) )
    {
      case 0:
      case 1:
        if ( F32(c, 0x14238) == v12 )
          goto LABEL_11;
        break;
      case 3:
        break;
      default:
        goto LABEL_11;
    }
    W_behave_set_state(c, 7);
    F32(c, 0x14824) = v10;
    return 0;
  }
  else
  {
LABEL_11:
    v14 = 9;
    if ( F32(c, 0x1f0) == 0 )
      v14 = F32(c, 0x14240);
    switch ( v14 )
    {
      case 1:
        if ( F32(c, 0x14258) == 0 )
          goto LABEL_17;
        v15 = (int)abs32(v10 - F32(c, 0x1425c)) <= 8000;
        goto LABEL_16;
      case 2:
      case 4:
      case 8:
      case 9:
      case 10:
        goto LABEL_25;
      case 3:
      case 11:
        if ( F32(c, 0x1424c) != 0 )
        {
          beh_4843c0(c, &a5, &a2, 0, 0, 1);
          v15 = (int)abs32(v10 - a5) <= 12800;
LABEL_16:
          if ( !v15 )
            goto LABEL_17;
        }
        goto LABEL_25;
      case 5:
        if ( F32(c, 0x14264) + 40000 >= v10 && F32(c, 0x14268) - 40000 <= v10 )
          goto LABEL_25;
        goto LABEL_17;
      case 6:
        if ( F32(c, 0x14264) < v10 )
          goto LABEL_17;
        if ( F32(c, 0x14268) <= v10 )
        {
LABEL_25:
          v16 = F32(c, 0x1594c);
          if ( v16 <= 0 || v16 > 3 )
          {
            v17 = (_DWORD *)(v7 + F32(a3, 64));
            v18 = v17[161];
            if ( v18 != 0 )
            {
              v19 = a4;
              if ( (uint16_t)a4 >= F16(v18, 68) )
                v19 = 0;
              v20 = F32(F32((v18 + 64), 0) + 660 * v19, 20)
                  + F32(F32((v18 + 64), 0) + 660 * v19, 8)
                  - 1;
            }
            else
            {
              W_grid_floor_below(v17, &a4);
              v20 = 3200 * a4 - 3200;
            }
            result = beh_483040(c, v11, v10, v20, 3, a6);
          }
          else
          {
            if ( a6 != 0 )
              *a6 = v10;
            result = 1;
          }
        }
        else
        {
          result = 0;
        }
        break;
      default:
LABEL_17:
        result = 0;
        break;
    }
  }
  return result;
}

/* 0x47b870: int __stdcall sub_47B870(int a1, int a2) */
int32_t beh_47b870(int32_t a1, int32_t a2)
{

    intptr_t v2;
    intptr_t result;
    intptr_t v4;

  v2 = F32(F32((a2 + 1872), 0), 216)
     + F32(F32((a2 + 1872), 0), 224)
     + F32(F32((a2 + 1872), 0), 228)
     - F32(F32((a1 + 1872), 0), 228)
     - F32(F32((a1 + 1872), 0), 224)
     - F32(F32((a1 + 1872), 0), 216);
  if ( F32(a2, 88724) != 0 && v2 < 2 )
    v2 = 2;
  switch ( v2 )
  {
    case -2:
      result = 100;
      break;
    case -1:
      result = 200;
      break;
    case 0:
      result = 300;
      break;
    case 1:
      result = 600;
      break;
    case 2:
      result = 1000;
      break;
    case 3:
      result = 1200;
      break;
    default:
      v4 = 50;
      if ( v2 > -3 )
        v4 = a2;
      if ( v2 >= 4 )
        v4 = 1500;
      result = v4 < 0 ? 0 : v4;
      break;
  }
  return result;
}

/* 0x48b920: int __thiscall sub_48B920(cls_48B920 *this, int a2, int a3, int a4) */
int32_t beh_48b920(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result;
    intptr_t v5;
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

  if ( a2 <= 0 )
    return 0;
  v5 = F32(c, 0x750);
  if ( a3 == 0 )
  {
    v12 = 2;
    v13 = F32(v5, 104) + F32(v5, 120) + F32(v5, 144) + F32(c, 0xc7dc);
    v7 = 1000 * ((v13 < 0 ? 0 : v13) / 4) / 1000;
    v8 = 20 * v7 / 100;
    if ( v8 >= 2 )
      v12 = 20 * v7 / 100;
    v14 = 50 * v7 / 100;
    if ( v14 >= v12 )
    {
      if ( v8 < 2 )
        goto LABEL_28;
      v15 = 20 * v7 / 100;
    }
    else
    {
      v15 = 50 * v7 / 100;
    }
    if ( v15 > 100 )
    {
      v8 = 100;
      goto LABEL_34;
    }
    if ( v8 >= 2 )
    {
      v16 = 20 * v7 / 100;
      goto LABEL_30;
    }
LABEL_28:
    v16 = 2;
LABEL_30:
    if ( v14 < v16 )
    {
      v8 = 50 * v7 / 100;
      goto LABEL_34;
    }
    if ( v8 >= 2 )
      goto LABEL_34;
    goto LABEL_33;
  }
  v6 = 2;
  v7 = 1000 * ((F32(v5, 112) + F32(v5, 128) + F32(v5, 152) + F32(c, 0xc7e0)) / 4) / 1000;
  v8 = 20 * v7 / 100;
  if ( v8 >= 2 )
    v6 = 20 * v7 / 100;
  v9 = 50 * v7 / 100;
  if ( v9 >= v6 )
  {
    if ( v8 < 2 )
      goto LABEL_13;
    v10 = 20 * v7 / 100;
  }
  else
  {
    v10 = 50 * v7 / 100;
  }
  if ( v10 <= 100 )
  {
    if ( v8 >= 2 )
    {
      v11 = 20 * v7 / 100;
      goto LABEL_15;
    }
LABEL_13:
    v11 = 2;
LABEL_15:
    if ( v9 < v11 )
    {
      v8 = 50 * v7 / 100;
      goto LABEL_34;
    }
    if ( v8 >= 2 )
      goto LABEL_34;
LABEL_33:
    v8 = 2;
    goto LABEL_34;
  }
  v8 = 100;
LABEL_34:
  v17 = a2 - (v7 + a4 * v8 / 1000 - v8);
  result = 1;
  if ( v17 >= 1 )
    result = v17;
  if ( F16(v5, 1106) != 0 )
    return 1500 * result / 1000;
  return result;
}

/* 0x5b4200: int __thiscall sub_5B4200(cls_4022A0 *this) */
int32_t beh_5b4200(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1;
    intptr_t v2;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;

  v1 = F32(c, 0xc794);
  v2 = 0;
  if ( v1 == 0 )
    return W_stat_jitter(
             (F32(F32(c, 0x750), 100)
            + F32(F32(c, 0x750), 116)
            + F32(F32(c, 0x750), 140)
            + F32(c, 0xc7d8))
           / 2,
             1000,
             750);
  v4 = 8;
  v5 = F32(v1, 2540) + 16432;
  do
  {
    v6 = F32(v5, 0);
    if ( F32(F32(v5, 0), 2500) == 1 && F32(v6, 2544) != 100000104 )
    {
      v7 = F32(v6, 2548);
      if ( v7 != 0 && F32(v7, 464) == 1 && F8(F32(v7, 64), 0) == 1 )
      {
        v8 = F32(v7, 1872);
        if ( F16(v8, 1098) == 0 )
        {
          if ( v2 <= W_stat_jitter(
                       (F32(v8, 100) + F32(v8, 116) + F32(v8, 140) + F32(v7, 51160))
                     / 2,
                       1000,
                       750) )
            v9 = W_stat_jitter(
                   (F32(F32((F32(v6, 2548) + 1872), 0), 100)
                  + F32(F32((F32(v6, 2548) + 1872), 0), 116)
                  + F32(F32((F32(v6, 2548) + 1872), 0), 140)
                  + F32(F32((v6 + 2548), 0), 51160))
                 / 2,
                   1000,
                   750);
          else
            v9 = v2;
          v2 = v9;
        }
      }
    }
    v5 += 4;
    --v4;
  }
  while ( v4 != 0 );
  return v2;
}

/* 0x480260: int __thiscall sub_480260(cls_480260 *this, _DWORD *a2, unsigned __int16 a3, int a4, _DWORD *a5) */
int32_t beh_480260(wl_char *self, int32_t * a2, uint16_t a3, int32_t a4, int32_t * a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5;
    uint16_t v6;
    uint16_t v7;
    intptr_t v8;
    uint16_t v9;
    uint16_t v10;
    intptr_t v11;
    uint16_t v12;
    uint16_t v14;
    intptr_t v15;
    int32_t * v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    int32_t * v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x10 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xc */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8 */
    uint16_t *const v29__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0x4 */

  v5 = F32(c, 0x0);
  (*v27__f) = c;
  (*v28__f) = F32(c, 0x0);
  if ( F32(c, 0x0) == 0 )
  {
    v6 = F16(c, 0x44);
    v7 = a3;
    if ( a3 >= v6 )
      v7 = 0;
    v8 = *(_DWORD *)&GAP(c, 0x24, 28);
    if ( F32(v8 + 660 * v7, 4) <= *a2 + a2[2] )
    {
      v9 = a3;
      if ( a3 >= v6 )
        v9 = 0;
      if ( F32(v8 + 660 * v9, 4) + F32(v8 + 660 * v9, 12) > *a2 )
      {
        v10 = a3;
        if ( a3 >= v6 )
          v10 = 0;
        v11 = a2[1];
        if ( F32(v8 + 660 * v10, 8) + F32(v8 + 660 * v10, 20) <= v11 + a2[3] )
        {
          v12 = a3;
          if ( a3 >= v6 )
            v12 = 0;
          if ( F32(v8 + 660 * v12, 16) + F32(v8 + 660 * v12, 8) - 1 > v11 )
            return 1;
        }
      }
    }
    return 0;
  }
  v14 = F16(c, 0x4);
  v15 = 0;
  (*v25__f) = 0;
  (*v29__f) = v14;
  if ( v14 == 0 )
    return 0;
  while ( 1 )
  {
    if ( a4 != 0
      || F32(32 * (uint16_t)v15 + v5, 16) != 0
      && F32(F32((*(_DWORD *)&GAP(c, 0x24, 28) + 660 * a3 + 656), 0) + 20 * (uint16_t)v15, 16) == 0 )
    {
      v16 = (_DWORD *)(*(_DWORD *)&GAP(c, 0x24, 28) + 660 * a3);
      v17 = v16[164];
      v18 = 5 * (uint16_t)v15;
      v19 = F32((v17 + 20 * (uint16_t)v15), 0);
      v20 = (_DWORD *)(v17 + 4 * v18);
      if ( v16[11] == 3 )
        v19 = -v19;
      v21 = v20[2];
      (*v26__f) = v20[3];
      v22 = v19 + v16[1] + v16[3] / 2 - v21 / 2;
      v23 = v16[2] + v20[1] + v16[4] / 2 - (*v26__f) / 2;
      if ( v22 <= *a2 + a2[2] && v22 + v21 > *a2 )
      {
        v24 = a2[1];
        if ( v23 <= v24 + a2[3] && v23 + (*v26__f) > v24 )
          break;
      }
      c = (*v27__f);
      v15 = (*v25__f);
      v5 = (*v28__f);
      v14 = (*v29__f);
    }
    (*v25__f) = ++v15;
    if ( (uint16_t)v15 >= v14 )
      return 0;
  }
  if ( a5 != 0 )
    *a5 = (uint16_t)(*v25__f);
  return 1;
}

/* 0x480580: int __thiscall sub_480580(cls_480580 *this, int a2, _DWORD *a3, int a4, int a5, unsigned __int16 a6, int a7, int a8) */
int32_t beh_480580(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, uint16_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    int32_t * v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    uint32_t v15;
    uint32_t v16;
    intptr_t v17;
    intptr_t v18;
    intptr_t v19;
    uint32_t v20;
    intptr_t v21;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    uint32_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    intptr_t v31;
    intptr_t result;
    intptr_t v33;
    intptr_t v34;
    intptr_t v39;
    intptr_t v40;
    intptr_t v41;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  v8 = (_DWORD *)a2;
  v9 = F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228);
  v10 = F32(a2, 1872);
  v11 = 0;
  (*v38__f) = F32(a2, 88724);
  (*v35__f) = 0;
  v12 = F32(v10, 216) + F32(v10, 224) + F32(v10, 228) - v9;
  if ( (*v38__f) != 0 && v12 < 2 )
    v12 = 2;
  (*v36__f) = F32(v10, 84);
  if ( a4 > 0 )
  {
    if ( a3[184] != 0 )
    {
      v13 = F32(a2, 51168) + F32(v10, 112) + F32(v10, 128) + F32(v10, 152);
    }
    else
    {
      v14 = F32(a2, 51164) + F32(v10, 104) + F32(v10, 120) + F32(v10, 144);
      v13 = v14 < 0 ? 0 : v14;
    }
    v11 = a4 - W_stat_jitter(v13 / 4, 1000, 750);
    if ( v11 < 1 )
      v11 = 1;
    v10 = F32(a2, 1872);
    if ( F16(v10, 1106) != 0 )
    {
      v15 = (int)((uint64_t)(412316860500LL * v11) >> 32) >> 6;
      v11 = (v15 >> 31) + v15;
    }
  }
  v16 = (int)((uint64_t)(274877907LL * a5 * v11) >> 32) >> 6;
  v39 = (v16 >> 31) + v16;
  if ( a8 <= 0 )
    goto LABEL_18;
  v17 = F32(v10, 104) + F32(v10, 120) + F32(v10, 144) + F32(a2, 51164);
  v18 = W_stat_jitter((v17 < 0 ? 0 : v17) / 4, 1000, 750);
  v19 = 1;
  if ( a8 - v18 >= 1 )
    v19 = a8 - v18;
  v10 = F32(a2, 1872);
  if ( F16(v10, 1106) != 0 )
  {
    v20 = (int)((uint64_t)(412316860500LL * v19) >> 32) >> 6;
    v19 = (v20 >> 31) + v20;
  }
  if ( v19 >= 1 )
  {
    v22 = F32(v10, 104) + F32(v10, 120) + F32(v10, 144) + F32(a2, 51164);
    v23 = W_stat_jitter((v22 < 0 ? 0 : v22) / 4, 1000, 750);
    v24 = 1;
    if ( a8 - v23 >= 1 )
      v24 = a8 - v23;
    v10 = F32(a2, 1872);
    if ( F16(v10, 1106) != 0 )
    {
      v25 = (int)((uint64_t)(412316860500LL * v24) >> 32) >> 6;
      v24 = (v25 >> 31) + v25;
    }
    v21 = v24;
  }
  else
  {
LABEL_18:
    v21 = 1;
  }
  v40 = v12;
  v26 = (*v36__f) / v21 + 1;
  v41 = v26;
  if ( v26 > 20 && v12 < 2 )
    v12 = 2;
  if ( v26 > 10 && v12 < 1 )
    v12 = 1;
  if ( v26 > 5 && v12 < 0 )
    v12 = 0;
  switch ( v12 )
  {
    case -2:
      a2 = 200;
      break;
    case -1:
      a2 = 300;
      break;
    case 0:
      a2 = 400;
      break;
    case 1:
      a2 = 600;
      break;
    case 2:
      a2 = 800;
      break;
    case 3:
      a2 = 1000;
      break;
    default:
      if ( v12 <= -3 )
        a2 = 150;
      if ( v12 >= 4 )
        a2 = 1200;
      break;
  }
  if ( a6 == 0 )
    goto LABEL_72;
  if ( F16(v10, 1124) != 0 )
    goto LABEL_72;
  v27 = a3[45]
      + F32(c, 0xc7e4)
      + F32(F32(c, 0x750), 108)
      + F32(F32(c, 0x750), 124)
      + F32(F32(c, 0x750), 148);
  if ( F16(v10 + 2 * a6, 1096) != 0
    || a6 == 4
    && (v9 - 1200 * (F32(v10, 216) + F32(v10, 224) + F32(v10, 228)) / 1000 < 6
     || v8[22181] != 0) )
  {
    goto LABEL_72;
  }
  v28 = ((int32_t *)D(0x62877c))[35 * a6];
  if ( v28 != 0 )
  {
    v29 = F32(v10 + 4 * v28, 164) * v8[v28 + 12778] * v8[v28 + 22155] / 1000;
    if ( v29 / 1000 == 0 )
      goto LABEL_72;
    v27 = v27 * (v29 / 1000) / 1000;
  }
  if ( a6 == 9 )
    v30 = v27 - (v8[12791] + F32(v10, 104) + F32(v10, 120) + F32(v10, 144));
  else
    v30 = v27 - (v8[12792] + F32(v10, 112) + F32(v10, 128) + F32(v10, 152));
  if ( a7 != 0 && v30 > 0 || 10000 * rnd() / 0x8000 % 1000 < v30 )
  {
    (*v35__f) = ((int32_t *)D(0x628784))[35 * a6];
    if ( (*v35__f) > 0 )
    {
      if ( v40 < -1 )
        a2 += (v40 + 1) * ((int32_t *)D(0x62878c))[35 * a6];
      if ( v40 > 1 )
        a2 += (v40 - 1) * ((int32_t *)D(0x62878c))[35 * a6];
      if ( v39 == 0 )
      {
        v31 = ((int32_t *)D(0x628780))[35 * a6];
        if ( v31 > 0 && (*v38__f) == 0 )
        {
          if ( v31 > v41 )
            return 0;
          v33 = v41 / 4;
          if ( v41 / 4 < 1 )
            v33 = 1;
          (*v35__f) *= v33;
        }
      }
    }
  }
LABEL_72:
  if ( (*v36__f) <= v39 )
    v34 = a2 + 80 * ((*v36__f) - v39) / (*v36__f) + 120;
  else
    v34 = 100 * v39 / (*v36__f) + a2;
  result = (*v35__f) + 10 * rnd() / 0x8000 + v34 - 2 * a3[181];
  if ( v39 == 0 && (*v35__f) == 0 )
    return 0;
  if ( result < 100 )
    return 100;
  return result;
}

/* 0x4832e0: int __thiscall sub_4832E0(cls_43F370 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t beh_4832e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    int v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    intptr_t v21;
    intptr_t v22;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v24 = (uint8_t *)(frame_ + 0x0);   /* [16] at ebp-0x10 */

  v6 = a2;
  v8 = a3;
  v9 = a6;
  memset(((uint8_t *)(c) + 0x4), 0, 40960);
  v10 = a5;
  F32(c, 0xa004) = a4;
  F32(c, 0xa008) = v10;
  v11 = F32(c, 0x0);
  F32(c, 0xa01c) = v9;
  v12 = (v6 << 7) + v8;
  SETHI(v13, 0);
  F32(c, 0xa014) = -1;
  SETLO(v13, F16(v11 + 2 * v12, 2887744));
  if ( (uint16_t)v13 != 0 && v13 <= 3 && F16(v11 + 2 * v12, 2887742) != 1 )
    --v8;
  v14 = v8 + (v6 << 7);
  SETLO(v13, F16(v11 + 2 * v14, 2887746));
  a4 = v13;
  if ( (uint16_t)v13 == 0 )
  {
    v15 = v6 + F32(c, 0xa524);
    if ( F16(v11 + 2 * (v8 + (v15 << 7)), 2887746) != 0 )
    {
      a2 = v6 + F32(c, 0xa524);
      v6 = v15;
      goto LABEL_12;
    }
    SETLO(v13, a4);
  }
  if ( F16(v11 + 2 * v14, 2887744) != 0 && (uint16_t)v13 == 0 )
    --v8;
LABEL_12:
  if ( F16(v11 + 2 * (v8 + (v6 << 7)), 2887744) >= 2u
    && F16(v11 + 2 * (v8 + (v6 << 7)), 2887744) <= 3u )
  {
    --v8;
  }
  if ( a6 != 0 )
  {
    v16 = 640 * (5 * v8 + 5) - F32(c, 0xa528);
    v17 = 3200 * v6 / 3200 < 0;
    v18 = 3200 * v6 / 3200;
    if ( F32(v11, 2887728) - 1 >= (v17 ? 0 : v18) )
      a6 = v18 < 0 ? 0 : v18;
    else
      a6 = F32(v11, 2887728) - 1;
    v19 = F32(v11, 2887732) - 1;
    if ( v19 >= (v16 / 3200 < 0 ? 0 : v16 / 3200) )
      v19 = v16 / 3200 < 0 ? 0 : v16 / 3200;
    v20 = v19 + (a6 << 7);
    v21 = F32((v11 + 4 * (3 * v20 + 414732)), 0);
    if ( v21 == 1 )
    {
      if ( v16 % 3200 <= 1600 )
      {
LABEL_34:
        v6 = a2;
        return W_path_dfs(c, v6, v8, 0, ((uint8_t *)(c) + 0xa020), 1, 64, &a2);
      }
      if ( v24 == (_BYTE *)-20 )
        v22 = a2;
      else
        v22 = F32(v11 + 12 * v20, 1658932);
    }
    else
    {
      if ( v24 == (_BYTE *)-20 )
        v22 = a2;
      else
        v22 = F32(v11 + 12 * v20, 1658932);
      if ( v21 == 0 )
        goto LABEL_34;
    }
    if ( v22 != 0 && v22 != 2 )
      F32(c, 0xa01c) = 0;
    goto LABEL_34;
  }
  return W_path_dfs(c, v6, v8, 0, ((uint8_t *)(c) + 0xa020), 1, 64, &a2);
}

/* 0x484bc0: int __thiscall sub_484BC0(cls_484BC0 *this, int a2, int a3, _DWORD *a4, _DWORD *a5) */
int32_t beh_484bc0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t * a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5;
    intptr_t result;

  v5 = c + 4 * (1536 * a2 + 12 * a3);
  *a4 = F32(v5, (476182) * 4);
  result = (int)F32(v5, (476183) * 4);
  *a5 = result;
  return result;
}
/* unported callees: none */
