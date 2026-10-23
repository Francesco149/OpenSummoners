/* engine_rest.c — the engine's remaining callees the corpus does not reach: the scene runner's helpers 0x43c1b0 /
 * 0x43d4b0 / 0x43d540 / 0x43d830 / 0x43dd30 / 0x43e010 / 0x4428f0 / 0x468970, the AI skill helpers 0x479ca0 / 0x47a300
 * / 0x47a420 / 0x47a7f0, the integrator's 0x44bf60 / 0x4675c0 / 0x467750, the shot emitter 0x40ac90 + 0x407f30, the
 * shop's buy confirm 0x415ae0 + 0x4192f0 / 0x419980, the save backup copy 0x41b470, the chapter scripts' helpers
 * 0x41c390 / 0x41cb10 / 0x49d330 / 0x49eea0 / 0x4c5af0 / 0x4cc0e0 / 0x402890 / 0x40a2e0 / 0x41e070 / 0x41e4c0 /
 * 0x41f0c0 / 0x438970, the party level / status sync 0x41db30 / 0x41dba0, the error box 0x426110, the cast's 0x441de0,
 * the key use 0x54ed80, 0x401df0 / 0x41c230 / 0x54c5e0 / 0x5534d0 / 0x556e50 / 0x55fdb0 (the cue) / 0x5851a0.  See
 * engine_rest.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  REPLACE-verified by rh-early-game
 * (2026-10-23): 0x401df0 0x4192f0 (the shop's Sell) 0x419980 0x43d4b0 0x43d540 0x43d830 0x43dd30 0x43e010 0x479ca0
 * 0x47a300 0x47a7f0 0x49eea0 0x4cc0e0; the rest UNVERIFIED (unreached; REPLACE-wired, compile-checked). */
#include "engine_rest.h"

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

#include "actor_ops.h"
#include "actor_update.h"
#include "char_ai.h"
#include "char_attack.h"
#include "char_def.h"
#include "char_hit.h"
#include "char_integrate.h"
#include "char_state.h"
#include "char_status.h"
#include "entity_life.h"
#include "entity_move.h"
#include "gimmick.h"
#include "leftovers.h"
#include "menu_helpers.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "party_member.h"
#include "render_platform.h"
#include "room_loop.h"
#include "save_game.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "script_io.h"
#include "sound_dev.h"
#include "sound_fx.h"
#include "title_extra.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
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
static inline intptr_t W_actor_shot(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)actor_shot((wl_actor *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_actor_state_restore(intptr_t a0, intptr_t a1, ...) { actor_state_restore((wl_actor *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_actor_unlist_all(intptr_t a0, ...) { actor_unlist_all((wl_actor *)a0); return 0; }
static inline intptr_t W_bgm_pick(intptr_t a0, ...) { return (intptr_t)bgm_pick((const wl_roomloop *)a0); }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1, ...) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_char_ai_reset(intptr_t a0, intptr_t a1, ...) { char_ai_reset((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_char_ground_probe(intptr_t a0, ...) { return (intptr_t)char_ground_probe((wl_char *)a0); }
static inline intptr_t W_char_mag_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_mag_roll((const wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_char_mag_sum(intptr_t a0, ...) { return (intptr_t)char_mag_sum((const wl_char *)a0); }
static inline intptr_t W_char_skill_avail(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)char_skill_avail((wl_char *)a0, (const wl_skill *)a1, (const wl_body *)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_char_skill_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_skill_roll((const wl_char *)a0, (const uint8_t *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_cmd_clear(intptr_t a0, ...) { cmd_clear((wl_cmd *)a0); return 0; }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_text_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_text_set((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_entity_dust(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { entity_dust((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (uint32_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_entity_move_vertical(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)entity_move_vertical((wl_entity *)a0, (wl_body *)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (int32_t *)a5, (wl_entity **)a6); }
static inline intptr_t W_entity_release(intptr_t a0, intptr_t a1, ...) { entity_release((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_set_collide(intptr_t a0, intptr_t a1, intptr_t a2, ...) { entity_set_collide((wl_entity *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_entity_unlist(intptr_t a0, ...) { entity_unlist((wl_entity *)a0); return 0; }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_col_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)grid_col_place((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_grid_row_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)grid_row_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_grid_row_reset(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_row_reset((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_grid_row_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)grid_row_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); }
static inline intptr_t W_icon_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)icon_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5); }
static inline intptr_t W_item_list_fill(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { item_list_fill((uint8_t *)a0, (wl_room *)a1, (int32_t)a2, (int32_t)a3, (uint8_t *)a4, (int32_t)a5, (int32_t)a6); return 0; }
static inline intptr_t W_lo_479960(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)lo_479960((wl_char *)a0, (int32_t)a1, (uint32_t)a2, (uint32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t *)a8, (int32_t)a9); }
static inline intptr_t W_menu_party_close(intptr_t a0, ...) { return (intptr_t)menu_party_close((uint8_t *)a0); }
static inline intptr_t W_menu_party_fill(intptr_t a0, ...) { menu_party_fill((uint8_t *)a0); return 0; }
static inline intptr_t W_menu_party_open(intptr_t a0, ...) { menu_party_open((uint8_t *)a0); return 0; }
static inline intptr_t W_menu_run(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)menu_run((uint8_t *)a0, (uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_mh_4190d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)mh_4190d0((wl_char *)a0, (char *)a1, (uint16_t)a2, (int32_t *)a3, (int32_t)a4); }
static inline intptr_t W_mh_4199b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)mh_4199b0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_mh_419c70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)mh_419c70((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t *)a7, (int32_t *)a8, (char *)a9, (int32_t)a10); }
static inline intptr_t W_mh_419eb0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)mh_419eb0((wl_char *)a0, (char *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7, (int32_t *)a8, (int32_t)a9); }
static inline intptr_t W_money_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)money_text((char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_money_value(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)money_value((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_msg_pages(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)msg_pages((uint8_t *)a0, (const char *)a1, (const char *)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (uint16_t)a7, (uint16_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_msg_window_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)msg_window_open((uint8_t *)a0, (const char *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_npc_floor_ahead(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)npc_floor_ahead((wl_char *)a0, (const wl_body *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_npc_navigate(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)npc_navigate((wl_char *)a0, (int32_t)a1, (int32_t)a2, (wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_party_find(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)party_find((const wl_char *)a0, (int32_t)a1, (int16_t)a2, (uint16_t *)a3, (uint16_t)a4); }
static inline intptr_t W_party_gather(intptr_t a0, ...) { party_gather((const wl_body *)a0); return 0; }
static inline intptr_t W_pickup_spawn(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pickup_spawn((int32_t)a0, (const wl_body *)a1, (int32_t)a2); }
static inline intptr_t W_pslot_heal(intptr_t a0, intptr_t a1, ...) { pslot_heal((wl_pslot *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_gold_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_gold_add((wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_input_flush(intptr_t a0, ...) { room_input_flush((wl_room *)a0); return 0; }
static inline intptr_t W_room_item_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)room_item_add((wl_room *)a0, (int32_t)a1, (uint16_t *)a2, (int32_t)a3, (uint8_t)a4, (int32_t)a5); }
static inline intptr_t W_room_items_of_kind(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_items_of_kind((const wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_views_mark(intptr_t a0, intptr_t a1, ...) { room_views_mark((wl_roomloop *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_sc_401d80(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sc_401d80((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_sc_4024e0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { sc_4024e0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); return 0; }
static inline intptr_t W_sc_402f40(intptr_t a0, ...) { return (intptr_t)sc_402f40((wl_char *)a0); }
static inline intptr_t W_sc_406500(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sc_406500((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_scene_face(intptr_t a0, intptr_t a1, intptr_t a2, ...) { scene_face((wl_event *)a0, (void *)a1, (int32_t)a2); return 0; }
static inline intptr_t W_scene_msg(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { scene_msg((wl_event *)a0, (uint32_t)a1, (void *)a2, (const char *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (const char *)a7, (int32_t)a8, (int32_t)a9, (uint16_t)a10); return 0; }
static inline intptr_t W_scene_obj_init(intptr_t a0, ...) { return (intptr_t)scene_obj_init((uint8_t *)a0); }
static inline intptr_t W_scene_party_stance(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)scene_party_stance((wl_event *)a0, (int32_t)a1, (wl_room *)a2); }
static inline intptr_t W_scene_talk_face(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)scene_talk_face((wl_event *)a0, (int32_t)a1, (void *)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_scene_yes_no(intptr_t a0, ...) { return (intptr_t)scene_yes_no((wl_event *)a0); }
static inline intptr_t W_script_begin(intptr_t a0, ...) { script_begin((wl_event *)a0); return 0; }
static inline intptr_t W_script_end(intptr_t a0, ...) { script_end((wl_event *)a0); return 0; }
static inline intptr_t W_script_leader(intptr_t a0, ...) { return (intptr_t)script_leader((const wl_event *)a0); }
static inline intptr_t W_script_party_cast(intptr_t a0, ...) { script_party_cast((wl_event *)a0); return 0; }
static inline intptr_t W_script_wait(intptr_t a0, ...) { return (intptr_t)script_wait((wl_event *)a0); }
static inline intptr_t W_sd_5bb250(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sd_5bb250((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_sd_5bb3a0(intptr_t a0, ...) { return (intptr_t)sd_5bb3a0((wl_char *)a0); }
static inline intptr_t W_sd_5bbc20(intptr_t a0, ...) { return (intptr_t)sd_5bbc20((wl_char *)a0); }
static inline intptr_t W_sd_5bbc60(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbc60((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_sd_5bbc90(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbc90((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_sd_5bbcd0(intptr_t a0, intptr_t a1, ...) { sd_5bbcd0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sd_5bcb80(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sd_5bcb80((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_sfx_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sfx_play((void *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_shot_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19, ...) { return (intptr_t)shot_spawn((void *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (wl_body *)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int16_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19); }
static inline intptr_t W_si_40a5d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { si_40a5d0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_si_41a160(intptr_t a0, ...) { return (intptr_t)si_41a160((wl_char *)a0); }
static inline intptr_t W_structure_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { structure_init((wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (uint32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); return 0; }
static inline intptr_t W_sv_41a890(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)sv_41a890((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_sv_41b670(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sv_41b670((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t *)a3); }
static inline intptr_t W_sv_41bf60(intptr_t a0, intptr_t a1, ...) { sv_41bf60((int32_t *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sv_41cfd0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sv_41cfd0((int32_t)a0, (uint16_t)a1, (char *)a2, (int32_t *)a3); }
static inline intptr_t W_sv_55f620(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sv_55f620((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_switch_glow(intptr_t a0, intptr_t a1, intptr_t a2, ...) { switch_glow((wl_actor *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_sys_box_cursor(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)sys_box_cursor((void *)a0, (const uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_sys_cursor_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_cursor_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_msg_keys(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_keys((void *)a0, (int32_t)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_page_arrows(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_page_arrows((void *)a0, (const uint8_t *)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_win_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)sys_win_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const char *)a7, (int32_t)a8); }
static inline intptr_t W_text_metrics_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)text_metrics_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_tm_419900(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_419900((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_41bb80(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41bb80((wl_char *)a0, (uint16_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_41bbe0(intptr_t a0, ...) { tm_41bbe0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_41c2f0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c2f0((wl_char *)a0, (int32_t *)a1, (uint16_t *)a2); }
static inline intptr_t W_tm_41c320(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c320((wl_char *)a0, (int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_ts_40fe00(intptr_t a0, ...) { return (intptr_t)ts_40fe00((wl_char *)a0); }
static inline intptr_t W_ui_scroll_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ui_scroll_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_vpool_load(intptr_t a0, ...) { return (intptr_t)vpool_load((wl_vpool *)a0); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }
static inline intptr_t W_warp_request(intptr_t a0, intptr_t a1, ...) { warp_request((wl_actor *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_ctrl_to_back(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_ctrl_to_back((uint8_t *)a0, (uint16_t)a1); }
static inline intptr_t W_win_grid_new(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_grid_new((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_win_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)win_init((uint8_t *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const void *)a7); }
static inline intptr_t W_win_mgr_topmost(intptr_t a0, ...) { win_mgr_topmost((uint8_t *)a0); return 0; }
static inline intptr_t W_win_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }

typedef intptr_t (__attribute__((stdcall)) *api_CopyFileA_t)(intptr_t, intptr_t, intptr_t);
#define API_CopyFileA ((api_CopyFileA_t)g_eng.api("CopyFileA"))
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
int32_t el_401df0(wl_char *self, int32_t a2, int32_t a3);
int32_t el_402890(wl_char *self, int32_t a2);
int32_t el_407f30(wl_char *self, char * lpOutputString, int32_t a3, int32_t a4);
int32_t el_40a2e0(int32_t a1);
int32_t el_40ac90(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t el_415ae0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t el_4192f0(wl_char *self, int32_t a2);
int32_t el_419980(wl_char *self, uint16_t a2);
int32_t el_41b470(int32_t a1, uint16_t a2);
void el_41c230(void);
int32_t el_41c390(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, char * Format, int32_t a7, int32_t a8, int32_t a9, int32_t a10);
int32_t el_41cb10(wl_char *self, int32_t a2);
int32_t el_41db30(int32_t a1, int32_t a2);
int32_t el_41dba0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4);
int32_t el_41e070(wl_char *self, int32_t a2);
int32_t el_41e4c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t el_41f0c0(int32_t a1, int32_t a2, int32_t a3);
int32_t el_426110(wl_char *self, char * lpOutputString, int32_t a3, char * a4);
int32_t el_438970(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t el_43c1b0(wl_char *self, int32_t hModule, int32_t a3, int32_t a4);
int32_t el_43d4b0(uint16_t * a1, int32_t a2);
int32_t el_43d540(wl_char *self);
int32_t el_43d830(wl_char *self);
void el_43dd30(wl_char *self, int32_t * a2, uint16_t a3);
int32_t el_43e010(int32_t a1, char * a2);
int32_t el_441de0(wl_char *self, int32_t * a2, int32_t a3);
int32_t el_4428f0(wl_char *self);
void el_44bf60(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t el_4675c0(wl_char *self, int32_t * a2, uint16_t a3);
int32_t el_467750(wl_char *self, int32_t a2, uint16_t a3);
int32_t el_468970(wl_char *self, char a2);
int32_t el_479ca0(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t el_47a300(wl_char *self, int32_t a2, int32_t a3);
int32_t el_47a420(wl_char *self, int32_t a2, int32_t a3);
int32_t el_47a7f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void el_49d330(int32_t a1, int32_t a2);
int32_t el_49eea0(wl_char *self, int32_t a2, int32_t a3);
int32_t el_4c5af0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void el_4cc0e0(wl_char *self, int32_t a2);
void el_54c5e0(wl_char *self);
int32_t el_54ed80(void);
int32_t el_5534d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void el_556e50(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
void el_55fdb0(wl_char *self);
void el_5851a0(wl_char *self);

/* 0x401df0: int __thiscall sub_401DF0(cls_401D40 *this, int a2, int a3) */
int32_t el_401df0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v4 = 16432;
  for ( (*i__f) = 8; (*i__f) != 0; --(*i__f) )
  {
    v5 = F32((v4 + F32(c, 0x8)), 0);
    if ( F32(v5, 2500) == 1 )
    {
      v6 = F32(v5, 2548);
      if ( v6 != 0 )
      {
        v7 = W_sc_401d80((intptr_t)(v6), (intptr_t)(a2), (intptr_t)(a3));
        W_scene_face((intptr_t)(c), (intptr_t)(F32(v5, 2548)), (intptr_t)(v7));
      }
    }
    v4 += 4;
    result = (*i__f) - 1;
  }
  return result;
}

/* 0x402890: int __thiscall sub_402890(cls_401D40 *this, int a2) */
int32_t el_402890(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int v17 = 0;
    intptr_t result = 0;
    uint16_t v28 = 0;
    _Alignas(8) uint8_t frame_[0x74 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x74 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x58 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x54 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x50 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x44 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x40 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x3c */

  (*v23__f) = 0;
  (*v20__f) = a2 + 16432;
  (*v22__f) = 8;
  do
  {
    v2 = F32((*v20__f), 0);
    (*v21__f) = F32((*v20__f), 0);
    if ( F32(F32((*v20__f), 0), 2500) != 1 )
      goto LABEL_23;
    v3 = F32(v2, 2548);
    v4 = 0;
    if ( v3 == 0 )
      goto LABEL_23;
    v5 = F32(v3, 468) - 50011;
    if ( v5 != 0 )
    {
      if ( v5 != 1 )
      {
        v28 = 0;
        goto LABEL_10;
      }
      (*v26__f) = 20135;
      (*v27__f) = 20115;
    }
    else
    {
      (*v26__f) = 20119;
      (*v27__f) = 20111;
    }
    v28 = 2;
LABEL_10:
    if ( v28 != 0 )
    {
      while ( 1 )
      {
        v6 = F32(v2, 2548);
        v7 = 0;
        v8 = F16(v6, 0xc754);
        v9 = F32(v6, 0x40);
        if ( v8 != 0 )
        {
          while ( 1 )
          {
            v10 = 768 * v7;
            if ( *(_DWORD *)&GAP(v6, 0x754, v10) == (uint16_t)*(&(*v26__f) + v4) )
              break;
            if ( ++v7 >= v8 )
              goto LABEL_19;
          }
          v11 = 768 * v7;
          v12 = (int)&GAP(v6, 0x754, v11);
          if ( ((uint8_t *)v6 + v11) != -1876 )
          {
            SETLO(v10, F8(v9, 32));
            v13 = W_entity_move_vertical((intptr_t)(v6), (intptr_t)((_DWORD *)v9), (intptr_t)(1), (intptr_t)(v10), (intptr_t)(0), (intptr_t)((int)&(*v25__f)), (intptr_t)(0));
            if ( W_char_skill_avail((intptr_t)(v6), (intptr_t)(v12), (intptr_t)(v9), (intptr_t)(v13 == 0), (intptr_t)(0)) != 0 )
              break;
          }
        }
LABEL_19:
        if ( ++v4 >= v28 )
          goto LABEL_23;
        v2 = (*v21__f);
      }
      v14 = *(&(*v26__f) + v4);
      if ( v14 != 0 )
      {
        F32(F32(((*v21__f) + 2548), 0), 616) = 0;
        v15 = F32((*v21__f), 2544);
        (*v19__f) = W_actor_by_handle((intptr_t)(v15));
        v16 = W_actor_by_handle((intptr_t)(v15));
        W_sc_4024e0((intptr_t)(c), (intptr_t)(v16), (intptr_t)(v14), (intptr_t)(0), (intptr_t)((*v19__f)), (intptr_t)(1), (intptr_t)(0));
        (*v23__f) = 1;
      }
    }
LABEL_23:
    v17 = (*v22__f) == 1;
    (*v20__f) += 4;
    --(*v22__f);
  }
  while ( !v17 );
  result = (*v23__f);
  if ( (*v23__f) != 0 )
    F32(c, 0x20) = 4;
  return result;
}

/* 0x407f30: FILE *__thiscall sub_407F30(const char *this, LPCSTR lpOutputString, int a3, int a4) */
int32_t el_407f30(wl_char *self, char * lpOutputString, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    uint32_t LastError = 0;
    uint32_t v6 = 0;
    FILE * v7 = 0;
    FILE * result = 0;
    FILE * v9 = 0;
    _Alignas(8) uint8_t frame_[0x310 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v11 = (char *)(frame_ + 0x10);   /* [256] at ebp-0x300 */
    char *OutputString = (char *)(frame_ + 0x110);   /* [256] at ebp-0x200 */
    char *Buffer = (char *)(frame_ + 0x210);   /* [256] at ebp-0x100 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x310 */

  g_eng.h5bf3ee(v11, "%d", a4);
  if ( a3 != 0 )
  {
    LastError = API_GetLastError();
    v6 = LastError;
    if ( LastError != 0 )
    {
      API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
      g_eng.h5bf3ee(OutputString, "[%08d] %s", v6, Buffer);
      API_OutputDebugStringA((intptr_t)(OutputString));
      API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
      v7 = CRT_fopen(c + 1028, ((char *)D(0x85456c)));
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
          OutputString,
          D(0x8a9b6c));
        CRT_fclose(v7);
      }
    }
  }
  API_OutputDebugStringA((intptr_t)(lpOutputString));
  API_OutputDebugStringA((intptr_t)(v11));
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
  result = CRT_fopen(c + 1028, ((char *)D(0x85456c)));
  v9 = result;
  if ( result != 0 )
  {
    API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
    CRT_fprintf(
      v9,
      "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
      (*SystemTime__f).wYear,
      (*SystemTime__f).wMonth,
      (*SystemTime__f).wDay,
      (*SystemTime__f).wHour,
      (*SystemTime__f).wMinute,
      (*SystemTime__f).wSecond,
      lpOutputString,
      v11);
    return CRT_fclose(v9);
  }
  return result;
}

/* 0x40a2e0: int __stdcall sub_40A2E0(int a1) */
int32_t el_40a2e0(int32_t a1)
{

    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t result = 0;

  v1 = 8;
  v2 = a1 + 16432;
  do
  {
    result = F32(v2, 0);
    if ( F32(F32(v2, 0), 2500) == 1 )
    {
      result = F32(F32((result + 2548), 0), 64);
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
    }
    v2 += 4;
    --v1;
  }
  while ( v1 != 0 );
  return result;
}

/* 0x40ac90: int __thiscall sub_40AC90(cls_4022A0 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t el_40ac90(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    uint32_t v7 = 0;
    uint16_t * v8 = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t * v15 = 0;
    int32_t * v16 = 0;
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
    int16_t j = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int32_t * *const v30__f = (int32_t * *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  v7 = F32(W, 0x27a0);
  if ( v7 >= 0xFFFFFF )
    F32(W, 0x27a0) = 0;
  else
    F32(W, 0x27a0) = v7 + 1;
  (*v34__f) = 3;
  (*v36__f) = v7 + 1;
  (*v30__f) = (_DWORD *)F32(c, 0x40);
  if ( a3 == 0 )
  {
    v8 = SYS32(1808);
    if ( F32(SYS32(1808), 8) != 0
      && (F32(SYS32(1808), 4) != 0
       || W_vpool_load((intptr_t)(SYS32(1808))) != 0) )
    {
      v9 = 0;
      if ( *v8 != 0 )
      {
        v10 = F32(v8, 4 * 1);
        SETHI(v11, 0);
        while ( 1 )
        {
          SETLO(v11, v8[1]);
          v12 = (v11 + v9) % *v8;
          if ( F32(v10 + 8 * (uint16_t)v12, 4) == 0 )
            break;
          if ( ++v9 >= *v8 )
            goto LABEL_31;
        }
        v13 = v12 + 1;
        v14 = 8 * (uint16_t)v12;
        v8[1] = v13;
        F32(v14 + v10, 4) = 0;
        v15 = (intptr_t *)(v14 + F32(v8, 4 * 1));
        if ( v15 != 0 )
        {
          (*v35__f) = *v15;
          v16 = *(_DWORD **)&GAP(W, 0x1030, 28);
          if ( v16[6] != 0 )
          {
            v17 = v16[4] + v16[24] + v16[25] / 2;
            (*v31__f) = v16[5] + v16[23] + v16[26] / 2;
          }
          else
          {
            v17 = v16[4];
            (*v31__f) = v16[5];
          }
          v18 = (*v30__f)[1];
          v19 = v18 + (*v30__f)[3];
          if ( v17 <= v18 || v17 >= v19 )
          {
            v20 = v18 - v17;
            if ( (int)abs32(v20) >= (int)abs32(v19 - v17) )
              v20 = v19 - v17;
          }
          else
          {
            v20 = 0;
          }
          v21 = abs32(v20);
          if ( v21 <= 128000 )
          {
            v22 = abs32(F32(c, 0x24) + (*v30__f)[2] - (*v31__f));
            if ( v22 <= 64000 )
            {
              v23 = 10000 * v20 / 128000;
              v24 = F32(CFG(), 360)
                  + 3000 * (F32(*(_DWORD *)&GAP(W, 0x1030, 16), 40) - 1000) / 1000
                  + -5000 * ((v21 - 8000) & ((v21 - 8000 < 0) - 1)) / 120000
                  - 5000 * (v22 - 16000 < 0 ? 0 : v22 - 16000) / 48000;
              if ( v24 < -10000 )
              {
                v24 = -10000;
              }
              else if ( v24 > 0 )
              {
                v24 = 0;
              }
              if ( &a3 == 0 )
              {
                v24 = a3;
                v23 = a3;
              }
              g_eng.dsb_volume_5bb870((*v35__f), v24);
              g_eng.dsb_pan_5bb880((*v35__f), v23);
              g_eng.dsb_freq_5bb890((*v35__f), 0);
              g_eng.dsb_play_5bb8c0((*v35__f));
            }
          }
        }
      }
    }
  }
LABEL_31:
  v25 = a7;
  v26 = a6;
  v27 = a5;
  for ( (*i__f) = 2; (*i__f) != 0; --(*i__f) )
  {
    for ( j = 10; (uint16_t)j < 0x32u; j += 10 )
      W_shot_spawn((intptr_t)(c), (intptr_t)(0), (intptr_t)(2), (intptr_t)(a4), (intptr_t)((_DWORD *)a2), (intptr_t)(2000), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(j), (intptr_t)(0), (intptr_t)(90), (intptr_t)((*v36__f)), (intptr_t)((*v34__f)), (intptr_t)(v27), (intptr_t)(v26), (intptr_t)(v25), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2));
    (*v34__f) = 1;
    result = (*i__f) - 1;
  }
  return result;
}

/* 0x415ae0: int __thiscall sub_415AE0(cls_412590 *this, int a2, int a3, int a4) */
int32_t el_415ae0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v7 = 0;
    intptr_t i = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t j = 0;
    intptr_t result = 0;
    int v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    _Alignas(8) uint8_t frame_[0x40c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v22 = (char *)(frame_ + 0xc);   /* [512] at ebp-0x400 */
    char *Buffer = (char *)(frame_ + 0x20c);   /* [512] at ebp-0x200 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x40c */
    uint16_t *const v20__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x408 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x404 */

  v4 = 0;
  v5 = 0;
  (*v21__f) = 0;
  v7 = F32(c, 0xc);
  (*v20__f) = 0;
  switch ( F16(a2, 102) )
  {
    case 1:
    case 2:
    case 3:
      (*v19__f) = 4;
      goto LABEL_5;
    case 5:
    case 6:
    case 7:
      (*v19__f) = 6;
      v5 = 1;
      goto LABEL_5;
    case 8:
    case 9:
      (*v19__f) = 10;
LABEL_5:
      SETLO(i, F16(v7, 16400));
      if ( (uint16_t)i != 0 )
      {
        v9 = v7 + 8220;
        for ( i = (uint16_t)i; i != 0; --i )
        {
          if ( F32((v9 - 12), 0) == F32(a2, 0)
            || v5 == 1 && F16(F32(v9, 0), 102) == F16(a2, 102) )
          {
            ++(*v20__f);
          }
          v9 += 16;
        }
        v4 = (*v20__f);
      }
      if ( v4 < (*v19__f) )
        goto LABEL_25;
      v10 = 0;
      strcpy(v22, ((char *)D(0x854be8)));
      j = W_msg_window_open((intptr_t)(c), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
      break;
    default:
LABEL_25:
      if ( F32(F32(c, 0xc), 16404) < F32(a2, 96) )
      {
        v14 = 0;
        strcpy(v22, ((char *)D(0x854bc8)));
        for ( j = W_msg_window_open((intptr_t)(c), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
              ;
              W_ctrl_text_set((intptr_t)(F32((F32(j, 72) + 4 * F16(j, 78) - 4), 0)), (intptr_t)(v22)) )
        {
          while ( 1 )
          {
            result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(j), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            if ( result == 12 )
              break;
            if ( result != 13 )
              goto LABEL_21;
          }
          if ( (uint16_t)++v14 != 0 )
            break;
        }
        result = 0;
        goto LABEL_21;
      }
      if ( a4 != 0 )
      {
        g_eng.h5bf3ee(Buffer, "Would you like to buy the %s?", (const char *)(a2 + 4));
        result = W_mh_4190d0((intptr_t)(c), (intptr_t)(Buffer), (intptr_t)(0x28u), (intptr_t)((_DWORD *)2), (intptr_t)(0));
        if ( result != 14 )
        {
          v13 = result == 15;
          goto LABEL_23;
        }
      }
      if ( W_room_item_add((intptr_t)(F32(c, 0xc)), (intptr_t)(F32(a2, 0)), (intptr_t)((uint16_t *)a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
      {
        result = W_msg_pages((intptr_t)(c), (intptr_t)(((char *)D(0x854b10))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
        goto LABEL_22;
      }
      v15 = F32(c, 0xc);
      v16 = -F32(a2, 96);
      if ( v16 < 0 || F32(a2, 96) == 0 || F32(v15, 16416) > F32(v15, 16404) )
      {
        v17 = F32(v15, 16404) - F32(a2, 96);
        F32(v15, 16404) = v17;
        if ( v16 > 0 )
        {
          v18 = F32(v15, 16416);
          if ( v18 >= v17 )
            v18 = v17;
          F32(v15, 16404) = v18;
        }
      }
      W_menu_party_fill((intptr_t)(c));
      F32(c, 0x14) = 1;
      W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
      return 12;
  }
  while ( 2 )
  {
    while ( 1 )
    {
      result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(j), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( result == 12 )
        break;
      if ( result != 13 )
        goto LABEL_21;
    }
    if ( (uint16_t)++v10 == 0 )
    {
      W_ctrl_text_set((intptr_t)(F32((F32(j, 72) + 4 * F16(j, 78) - 4), 0)), (intptr_t)(v22));
      continue;
    }
    break;
  }
  result = 0;
LABEL_21:
  F32(j, 80) = 0;
LABEL_22:
  v13 = result == 0;
LABEL_23:
  if ( v13 )
    return (*v21__f);
  return result;
}

/* 0x4192f0: int __thiscall sub_4192F0(cls_412590 *this, int a2) */
int32_t el_4192f0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    uint32_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t i = 0;
    _Alignas(8) uint8_t frame_[0x634 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x34);   /* [512] at ebp-0x600 */
    char *v35 = (char *)(frame_ + 0x234);   /* [512] at ebp-0x400 */
    char *v36 = (char *)(frame_ + 0x434);   /* [512] at ebp-0x200 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x634 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x620 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x620 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x61c */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x618 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x614 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x610 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x60c */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x608 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x604 */

  (*v30__f) = 0;
  (*v32__f) = 0;
  g_eng.h5bf3ee(Buffer, "%s", ((char *)D(0x854dd0)));
  v3 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(8), (intptr_t)(24), (intptr_t)(408), (intptr_t)(320), (intptr_t)(2u), (intptr_t)(Buffer), (intptr_t)(1));
  (*v33__f) = v3;
  v4 = W_win_grid_new((intptr_t)(v3), (intptr_t)(40), (intptr_t)(48), (intptr_t)(512), (intptr_t)(6), (intptr_t)(8), (intptr_t)(2));
  F32(v4, 428) = 32;
  W_grid_col_place((intptr_t)(v4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-8), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v4), (intptr_t)(1), (intptr_t)(0), (intptr_t)(-8), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v4), (intptr_t)(2), (intptr_t)(40), (intptr_t)(0), (intptr_t)(160), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v4), (intptr_t)(3), (intptr_t)(200), (intptr_t)(0), (intptr_t)(64), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v4), (intptr_t)(4), (intptr_t)(216), (intptr_t)(0), (intptr_t)(64), (intptr_t)(0));
  F32(c, 0x2c) = 1;
  W_grid_col_place((intptr_t)(v4), (intptr_t)(5), (intptr_t)(280), (intptr_t)(0), (intptr_t)(56), (intptr_t)(1));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v4), (intptr_t)(1));
  W_sys_page_arrows((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v3), (intptr_t)(v4));
  W_item_list_fill((intptr_t)(c), (intptr_t)(F32(c, 0xc)), (intptr_t)(-1), (intptr_t)(-1), (intptr_t)((int *)v4), (intptr_t)(a2), (intptr_t)(F32(c, 0x10)));
  v5 = F32(c, 0x24);
  if ( v5 != 0 )
    W_ctrl_text_set((intptr_t)(v5), (intptr_t)(((char *)D(0x854d80))));
  while ( 1 )
  {
    v6 = F32(v4, 372);
    if ( F32(v6, 16) == 0 )
      break;
    if ( (*v32__f) != 0 )
    {
      v7 = F32(16 * F32((v6 + 20), 0) + F32((v4 + 380), 0), 4);
      v8 = F32(c, 0xc);
      if ( (uint16_t)v7 < F16(v8, 16400) )
      {
        v9 = F32(16 * (uint16_t)v7 + v8, 8220);
        if ( v9 != 0 )
        {
          v10 = F32(c, 0x24);
          if ( v10 != 0 )
            W_ctrl_text_set((intptr_t)(v10), (intptr_t)((const char *)(v9 + 172)));
        }
      }
    }
    v11 = W_menu_run((intptr_t)(c), (intptr_t)((_DWORD *)v4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0));
    v12 = F32(v4, 380);
    (*v31__f) = F32(F32((v4 + 372), 0), 20);
    v13 = F32(16 * (*v31__f) + v12, 4);
    v14 = F32(c, 0xc);
    (*v28__f) = v13;
    if ( (uint16_t)v13 >= F16(v14, 0x4010) )
      (*v27__f) = 0;
    else
      (*v27__f) = *(_DWORD *)&GAP(v14, 0x0, 16 * (uint16_t)v13 + 8220);
    switch ( v11 )
    {
      case 11:
        goto LABEL_54;
      case 12:
        if ( (*v27__f) == 0 )
          continue;
        (*v25__f) = 500;
        v15 = el_419980(v14, v13);
        if ( F32(c, 0x30) > 0 )
          (*v25__f) = F32(c, 0x30);
        if ( F16(v15, 100) != 0 )
        {
          v16 = (*v25__f)
              * F32(v15, 96)
              / 1000
              * F16(F32(c, 0xc) + 16 * (uint16_t)v13, 8216)
              / F16(v15, 100);
          v13 = (*v28__f);
          if ( v16 >= 1 )
            (*v26__f) = v16;
          else
            (*v26__f) = 1;
        }
        else
        {
          v17 = (int)((uint64_t)(274877907LL * (*v25__f) * F32(v15, 96)) >> 32) >> 6;
          (*v26__f) = 1;
          v18 = (v17 >> 31) + v17;
          if ( v18 >= 1 )
            (*v26__f) = v18;
        }
        v19 = F32(F32(c, 0xc), 16484);
        (*v24__f) = W_money_value((intptr_t)(v19), (intptr_t)((*v26__f)), (intptr_t)(1));
        if ( v19 == 10 )
          g_eng.h5bf3ee(v35, "%dGP", (*v24__f));
        else
          g_eng.h5bf3ee(v35, "%dCP", (*v24__f));
        if ( F32((*v27__f), 684) < 200 )
        {
          g_eng.h5bf3ee(Buffer, "Selling %s for %s. Are you sure?", (const char *)((*v27__f) + 4), v35);
          v20 = W_mh_4190d0((intptr_t)(c), (intptr_t)(Buffer), (intptr_t)(0x28u), (intptr_t)((_DWORD *)2), (intptr_t)(0));
          goto LABEL_41;
        }
        g_eng.h5bf3ee(
          Buffer,
          "Selling %s for %s. This is a rare item, really sell it?",
          (const char *)((*v27__f) + 4),
          v35);
        (*v28__f) = 0;
        (*v27__f) = 0;
        v20 = W_mh_419eb0((intptr_t)(c), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(2u), (intptr_t)(0), (intptr_t)(&(*v27__f)), (intptr_t)(0));
        if ( v20 != 0 )
          goto LABEL_36;
        W_mh_419c70((intptr_t)(c), (intptr_t)(300), (intptr_t)(F32((*v27__f), 24) + F32((*v27__f), 16) - 16), (intptr_t)(128), (intptr_t)(0), (intptr_t)(1u), (intptr_t)(2u), (intptr_t)(&(*v28__f)), (intptr_t)((int *)&(*v29__f)), (intptr_t)(0), (intptr_t)(0));
        W_grid_row_text_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(14), (intptr_t)(D(0x854d04)));
        W_grid_row_text_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(15), (intptr_t)(((char *)D(0x854d00))));
        W_tm_419900((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(15));
        break;
      case 13:
        (*v32__f) = 1;
        continue;
      case 20:
        F32(c, 0x10) = F32(c, 0x10) == 0;
        W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(9u), (intptr_t)(0), (intptr_t)(0));
        W_item_list_fill((intptr_t)(c), (intptr_t)(F32(c, 0xc)), (intptr_t)((uint16_t)v13), (intptr_t)(-1), (intptr_t)((int *)v4), (intptr_t)(a2), (intptr_t)(F32(c, 0x10)));
        continue;
      default:
        (*v30__f) = v11;
        goto LABEL_54;
    }
    while ( 1 )
    {
      v20 = W_menu_run((intptr_t)(c), (intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( v20 == 11 )
        break;
      if ( v20 == 12 )
      {
        v20 = F32(16 * F32((F32((*v29__f), 0x174) + 20), 0) + F32((*v29__f), 0x17c), 4);
        goto LABEL_36;
      }
      if ( v20 != 13 )
        goto LABEL_36;
    }
    v20 = 15;
LABEL_36:
    if ( (*v27__f) != 0 )
      F32((*v27__f), 80) = 0;
    if ( (*v28__f) != 0 )
      F32((*v28__f), 80) = 0;
LABEL_41:
    if ( v20 != 15 )
    {
      if ( v20 != 14 )
        goto LABEL_52;
      W_room_gold_add((intptr_t)(F32(c, 0xc)), (intptr_t)((*v26__f)), (intptr_t)(0));
      W_mh_4199b0((intptr_t)(F32(c, 0xc)), (intptr_t)(v13), (intptr_t)(1), (intptr_t)(1));
      W_menu_party_fill((intptr_t)(c));
      W_item_list_fill((intptr_t)(c), (intptr_t)(F32(c, 0xc)), (intptr_t)(-1), (intptr_t)((uint16_t)(*v31__f)), (intptr_t)((int *)v4), (intptr_t)(a2), (intptr_t)(F32(c, 0x10)));
      F32(c, 0x14) = 1;
      W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
      (*v32__f) = 1;
    }
  }
  v21 = 0;
  strcpy(v36, ((char *)D(0x854d08)));
  (*v31__f) = 0;
  for ( i = W_msg_window_open((intptr_t)(c), (intptr_t)(v36), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
        ;
        W_ctrl_text_set((intptr_t)(F32((F32(i, 72) + 4 * F16(i, 78) - 4), 0)), (intptr_t)(v36)) )
  {
    while ( 1 )
    {
      v20 = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(i), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( v20 == 12 )
        break;
      if ( v20 != 13 )
        goto LABEL_51;
    }
    if ( (uint16_t)++v21 != 0 )
      break;
  }
  v20 = (*v31__f);
LABEL_51:
  F32(i, 80) = 0;
  if ( v20 != 0 )
LABEL_52:
    (*v30__f) = v20;
LABEL_54:
  *(_DWORD *)((uint8_t *)((*v33__f) + (1) * 0x50) + 0x0) = 0;
  return (*v30__f);
}

/* 0x419980: int __thiscall sub_419980(cls_419980 *this, unsigned __int16 a2) */
int32_t el_419980(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;

  if ( a2 >= F16(c, 0x4010) )
    return 0;
  else
    return *(_DWORD *)&GAP(c, 0x0, 16 * a2 + 8220);
}

/* 0x41b470: int __stdcall sub_41B470(int a1, unsigned __int16 a2) */
int32_t el_41b470(int32_t a1, uint16_t a2)
{


    _Alignas(8) uint8_t frame_[0x600 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *ExistingFileName = (char *)(frame_ + 0x0);   /* [512] at ebp-0x600 */
    char *Buffer = (char *)(frame_ + 0x200);   /* [512] at ebp-0x400 */
    char *NewFileName = (char *)(frame_ + 0x400);   /* [512] at ebp-0x200 */

  ExistingFileName[0] = 0;
  if ( a1 > 10020 )
  {
    if ( a1 == 10030 )
      goto LABEL_10;
    goto LABEL_8;
  }
  if ( a1 != 10020 )
  {
    if ( a1 == 1 )
    {
      g_eng.h5bf3ee(Buffer, "savedata%02d.bak", a2);
      goto LABEL_9;
    }
    if ( a1 != 10010 )
    {
LABEL_8:
      g_eng.h5bf3ee(Buffer, "savedata%02d.sdt", a2);
      goto LABEL_9;
    }
    g_eng.h5bf3ee(Buffer, "savedata%02d.dat", a2);
LABEL_9:
    strcpy(ExistingFileName, (const char *)&SYSB(259));
    strcat(ExistingFileName, ((char *)D(0x854f5c)));
    strcat(ExistingFileName, Buffer);
  }
LABEL_10:
  NewFileName[0] = 0;
  g_eng.h5bf3ee(Buffer, "savedata%02d.bak", a2);
  strcpy(NewFileName, (const char *)&SYSB(259));
  strcat(NewFileName, ((char *)D(0x854f5c)));
  strcat(NewFileName, Buffer);
  if ( API_CopyFileA((intptr_t)(ExistingFileName), (intptr_t)(NewFileName), (intptr_t)(false)) )
    return 1;
  g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x854ef4)), NewFileName);
  return 0;
}

/* 0x41c230: void nullsub_1() */
void el_41c230(void)
{


  ;
}

/* 0x41c390: int __thiscall sub_41C390( cls_401D40 *this, int a2, int a3, int a4, int a5, char *Format, int a7, int a8, int a9, int a10) */
int32_t el_41c390(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, char * Format, int32_t a7, int32_t a8, int32_t a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t i = 0;
    intptr_t v14 = 0;
    intptr_t j = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    char * v18 = 0;
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
    intptr_t * v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    void ** v37 = 0;
    uint32_t v38 = 0;
    void * v39 = 0;
    int v40 = 0;
    _Alignas(8) uint8_t frame_[0x668 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v49 = (char *)(frame_ + 0x5c);   /* [512] at ebp-0x60c */
    char *v50 = (char *)(frame_ + 0x25c);   /* [512] at ebp-0x40c */
    char *Buffer = (char *)(frame_ + 0x45c);   /* [512] at ebp-0x20c */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x668 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x654 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x650 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x64c */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x648 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x644 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x640 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x664);   /* ebp-0x4 */

  (*v43__f) = 0;
  memset(&(*v48__f), 0, 16);
  memset(&F32(v48__f, 0x1c), 0, 16);
  *(_WORD *)&GAP(v48__f, 0x14, 4) = 0;
  *(_DWORD *)&F16(v48__f, 0x30) = 0;
  v11 = F32(c, 0x8);
  v12 = F32(c, 0x0);
  (*v52__f) = 0;
  (*v45__f) = v11;
  W_ts_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
  for ( i = 0; i < F16(v12, 4); ++i )
    W_room_input_flush((intptr_t)((F32(v12, 0) + 16672 * i)));
  W_room_views_mark((intptr_t)(v12), (intptr_t)(0));
  v14 = 4448;
  for ( j = 32; j != 0; --j )
  {
    v16 = *(_DWORD *)&GAP(W, 0x0, v14);
    if ( F32(v16, 464) != 0 )
      W_cmd_clear((intptr_t)(v16 + 84052));
    v14 += 4;
  }
  W_ui_scroll_init((intptr_t)(&(*v48__f)), (intptr_t)(v12), (intptr_t)(F32(v12, 232)), (intptr_t)((*v45__f)), (intptr_t)(F32(F32(W, 0x2784), 4)));
  if ( a4 != 1 )
  {
    if ( Format != 0 )
    {
      g_eng.h5bf3ee(Buffer, Format, a2);
      W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(a5), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      (*v43__f) = W_script_wait((intptr_t)(c));
      if ( (*v43__f) == 6 )
        goto LABEL_75;
    }
    if ( a4 == 2 )
    {
      v17 = 0;
      W_menu_party_open((intptr_t)(&(*v48__f)));
      v23 = F32(F32(v48__f, 0xc), 16484);
      (*v42__f) = W_money_value((intptr_t)(v23), (intptr_t)(a2), (intptr_t)(1));
      if ( v23 == 10 )
        g_eng.h5bf3ee(v49, "%dGP", (*v42__f));
      else
        g_eng.h5bf3ee(v49, "%dCP", (*v42__f));
      g_eng.h5bf3ee(v50, "Healer - %s", v49);
      W_mh_419c70((intptr_t)(&(*v48__f)), (intptr_t)(32), (intptr_t)(64), (intptr_t)(176), (intptr_t)(160), (intptr_t)(2u), (intptr_t)(3u), (intptr_t)(&(*v46__f)), (intptr_t)((int *)&(*v44__f)), (intptr_t)(v50), (intptr_t)(1));
      W_grid_row_text_add((intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(23), (intptr_t)(((char *)D(0x854ff8))));
      W_grid_row_text_add((intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(24), (intptr_t)(((char *)D(0x854fec))));
      W_grid_row_text_add((intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(26), (intptr_t)(((char *)D(0x855010))));
      while ( 1 )
      {
        v24 = W_menu_run((intptr_t)(&(*v48__f)), (intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v24 == 11 )
          break;
        if ( v24 == 12 )
        {
          switch ( F32(16 * F32((F32((*v44__f), 0x174) + 20), 0) + F32((*v44__f), 0x17c), 4) )
          {
            case 0x17:
              v17 = 14;
              goto LABEL_39;
            case 0x18:
              v17 = 15;
              goto LABEL_39;
            case 0x1A:
              el_41cb10(&(*v48__f), a3);
              break;
            default:
              break;
          }
        }
        else if ( v24 != 13 )
        {
          v17 = v24;
          break;
        }
      }
LABEL_39:
      v22 = (*v46__f);
    }
    else
    {
      v17 = 0;
      W_menu_party_open((intptr_t)(&(*v48__f)));
      v18 = W_money_text((intptr_t)(v50), (intptr_t)(F32(F32(v48__f, 0xc), 16484)), (intptr_t)(a2), (intptr_t)(1));
      g_eng.h5bf3ee(v49, "Inn - %s to stay", v18);
      W_mh_419c70((intptr_t)(&(*v48__f)), (intptr_t)(32), (intptr_t)(64), (intptr_t)(176), (intptr_t)(160), (intptr_t)(2u), (intptr_t)(3u), (intptr_t)(&(*v46__f)), (intptr_t)((int *)&(*v44__f)), (intptr_t)(v49), (intptr_t)(1));
      v19 = (*v44__f);
      v20 = W_grid_row_add((intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(23));
      W_grid_cell_text((intptr_t)(v19), (intptr_t)(v20), (intptr_t)(0), (intptr_t)(((char *)D(0x855024))));
      W_grid_row_text_add((intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(24), (intptr_t)(((char *)D(0x855018))));
      W_grid_row_text_add((intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(26), (intptr_t)(((char *)D(0x855010))));
      while ( 1 )
      {
        v21 = W_menu_run((intptr_t)(&(*v48__f)), (intptr_t)((*v44__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v21 == 11 )
          break;
        if ( v21 == 12 )
        {
          switch ( F32(16 * F32((F32((*v44__f), 0x174) + 20), 0) + F32((*v44__f), 0x17c), 4) )
          {
            case 0x17:
              v17 = 14;
              goto LABEL_23;
            case 0x18:
              v22 = (*v46__f);
              v17 = 15;
              goto LABEL_40;
            case 0x1A:
              el_41cb10(&(*v48__f), a3);
              break;
            default:
              break;
          }
        }
        else if ( v21 != 13 )
        {
          v22 = (*v46__f);
          v17 = v21;
          goto LABEL_40;
        }
      }
LABEL_23:
      v22 = (*v46__f);
    }
LABEL_40:
    F32(v22, 80) = 0;
    W_menu_party_close((intptr_t)(&(*v48__f)));
    if ( v17 == 6 )
    {
      (*v43__f) = 6;
      goto LABEL_75;
    }
    if ( v17 != 14 )
    {
      if ( a7 != 0 )
      {
        W_scene_msg((intptr_t)(c), (intptr_t)(F32(c, 0x7c)), (intptr_t)(F32(c, 0x2e0)), (intptr_t)((const char *)a7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(&GAP(c, 0x180, 266)), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0));
        (*v43__f) = W_script_wait((intptr_t)(c));
      }
      goto LABEL_75;
    }
    v25 = F32(c, 0x8);
    if ( F32(v25, 16404) < a2 )
    {
      if ( a8 != 0 )
      {
        W_scene_msg((intptr_t)(c), (intptr_t)(F32(c, 0x7c)), (intptr_t)(F32(c, 0x2e0)), (intptr_t)((const char *)a8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(&GAP(c, 0x180, 266)), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0));
        (*v43__f) = W_script_wait((intptr_t)(c));
      }
      goto LABEL_75;
    }
    if ( a2 >= 0 || F32(v25, 16416) > F32(v25, 16404) )
    {
      v26 = F32(v25, 16404) - a2;
      F32(v25, 16404) = v26;
      if ( a2 < 0 )
      {
        v27 = F32(v25, 16416);
        if ( v27 >= v26 )
          v27 = v26;
        F32(v25, 16404) = v27;
      }
    }
  }
  (*v45__f) = W_si_41a160((intptr_t)(c));
  v28 = 8;
  v29 = (intptr_t *)(F32(c, 0x8) + 16432);
  do
  {
    v30 = *v29;
    if ( W_pslot_ok((intptr_t)(*v29), (intptr_t)(0), (intptr_t)(1)) )
      W_pslot_heal((intptr_t)(v30), (intptr_t)(1));
    ++v29;
    --v28;
  }
  while ( v28 != 0 );
  if ( (*v45__f) != 0 )
  {
    (*v43__f) = (*v45__f);
    goto LABEL_75;
  }
  *(_DWORD *)&GAP(c, 0x300, 636) = 200;
  F32(c, 0x20) = 6;
  (*v43__f) = W_script_wait((intptr_t)(c));
  if ( (*v43__f) != 6 )
  {
    F32(c, 0x30) = 1;
    F32(c, 0x34) = 2;
    F32(c, 0x3c) = 500;
    F32(c, 0x38) = 1;
    F32(c, 0x24) = 1;
    F32(c, 0x28) = 1;
    F32(c, 0x20) = 2;
    F32(c, 0x2c) = 1000;
    (*v43__f) = W_script_wait((intptr_t)(c));
    if ( (*v43__f) != 6 )
    {
      F32(c, 0x1c) = 0;
      if ( a9 != 0 )
      {
        if ( a5 != 0 )
        {
          W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(a5), (intptr_t)((const char *)a9), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          (*v43__f) = W_script_wait((intptr_t)(c));
          if ( (*v43__f) == 6 )
            goto LABEL_75;
        }
        else
        {
          W_scene_msg((intptr_t)(c), (intptr_t)(3u), (intptr_t)(0), (intptr_t)((const char *)a9), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          (*v43__f) = W_script_wait((intptr_t)(c));
          if ( (*v43__f) == 6 )
            goto LABEL_75;
        }
      }
      v31 = W_scene_yes_no((intptr_t)(c));
      if ( v31 == 14 )
      {
        if ( el_41cb10(&(*v48__f), a3) == 6 )
        {
          (*v43__f) = 6;
          goto LABEL_75;
        }
      }
      else if ( v31 != 15 )
      {
        (*v43__f) = v31;
        goto LABEL_75;
      }
      if ( a10 == 0
        || (W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(a5), (intptr_t)((const char *)a10), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)), ((*v43__f) = W_script_wait((intptr_t)(c))) != 6) )
      {
        F32(c, 0x2e4) = 1;
        v32 = F32(c, 0x0);
        F32(v32, 0xd8) = 0;
        F16(v32, 0xdc) = 0;
        SETLO(v33, W_bgm_pick((intptr_t)(v32)));
        W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v33), (intptr_t)(1), (intptr_t)(1), (intptr_t)(20), (intptr_t)(0));
      }
    }
  }
LABEL_75:
  (*v52__f) = -1;
  if ( *(_WORD *)&GAP(v48__f, 0x14, 4) != 0 )
  {
    v34 = 0;
    v35 = *(uint16_t *)&GAP(v48__f, 0x14, 4);
    (*v45__f) = 0;
    (*v47__f) = *(uint16_t *)&GAP(v48__f, 0x14, 4);
    v36 = SYS32(4 * *(uint16_t *)&GAP(v48__f, 0x14, 4) + 2732);
    if ( F16(v36, 4) != 0 )
    {
      do
      {
        if ( F32(v36, 0) != 0 )
        {
          v37 = (void **)(F32(v36, 0) + 8 * (uint16_t)v34);
          if ( *v37 != 0 )
          {
            v38 = 0;
            if ( F32(v36, 56) != 0 )
            {
              do
              {
                v39 = *((void **)*v37 + v38);
                if ( v39 != 0 )
                {
                  W_rp_5b9390((intptr_t)(*((intptr_t *)*v37 + v38)));
                  g_eng.op_delete((void *)v39);
                }
                *((_DWORD *)*v37 + v38++) = 0;
              }
              while ( v38 < F32(v36, 56) );
              v34 = (*v45__f);
            }
            g_eng.op_delete((void *)*v37);
            v35 = (*v47__f);
            *v37 = 0;
          }
        }
        v40 = (uint16_t)++v34 < F16(v36, 4);
        (*v45__f) = v34;
      }
      while ( v40 );
    }
    F32(SYS32(4 * v35 + 6368), 16) = 0;
  }
  return (*v43__f);
}

/* 0x41cb10: int __thiscall sub_41CB10(cls_412590 *this, int a2) */
int32_t el_41cb10(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t ExceptionList = 0;
    void * v3 = 0;
    uint8_t * v5 = 0;
    uint8_t * v6 = 0;
    intptr_t i = 0;
    void * v8 = 0;
    void * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    _Alignas(8) uint8_t frame_[0x1c954 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *NewFileName = (char *)(frame_ + 0x28);   /* [512] at ebp-0x1c92c */
    char *ExistingFileName = (char *)(frame_ + 0x228);   /* [512] at ebp-0x1c72c */
    uint8_t *Buffer = (uint8_t *)(frame_ + 0x428);   /* [49804] at ebp-0x1c52c */
    uint8_t *v31 = (uint8_t *)(frame_ + 0xc6b8);   /* [8192] at ebp-0x1029c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c954 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1c950 */
    int32_t * *const v22__f = (int32_t * *)(frame_ + 0x8);   /* ebp-0x1c94c */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x1c948 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x1c944 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x1c940 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x1c93c */
    int16_t *const v30__f = (int16_t *)(frame_ + 0xc6b4);   /* ebp-0x102a0 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0xe6b8);   /* ebp-0xe29c */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x1c948);   /* ebp-0xc */
    void * *const v34__f = (void * *)(frame_ + 0x1c94c);   /* ebp-0x8 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x1c950);   /* ebp-0x4 */

  (*v35__f) = -1;
  (*v20__f) = 0;
  F32(v26__f, 0x0) = 0;
  F32(v26__f, 0x4) = 0;
  memset(Buffer, 0, 8356);
  (*v35__f) = 0;
  v5 = &Buffer[10628];
  v6 = &Buffer[8356];
  for ( i = 8; i != 0; --i )
  {
    v8 = v6;
    v6 += 284;
    memset(v8, 0, 0x11Cu);
    v9 = v5;
    v5 += 1056;
    memset(v9, 0, 0x420u);
  }
  memset(&Buffer[19076], 0, 30728);
  v10 = F32(c, 0x0);
  (*v30__f) = 0;
  memset(v31, 0, (8192 * sizeof(uint8_t)));
  W_scene_obj_init((intptr_t)(&(*v32__f)));
  v11 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(v10), (intptr_t)(8), (intptr_t)(8), (intptr_t)(624), (intptr_t)(464), (intptr_t)(2u), (intptr_t)(((char *)D(0x854fc0))), (intptr_t)(1));
  (*v25__f) = v11;
  v12 = W_win_grid_new((intptr_t)(v11), (intptr_t)(40), (intptr_t)(64), (intptr_t)(24), (intptr_t)(9), (intptr_t)(8), (intptr_t)(2));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v12), (intptr_t)(1));
  F32(v12, 428) = 48;
  W_grid_col_place((intptr_t)(v12), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(2), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(3), (intptr_t)(272), (intptr_t)(0), (intptr_t)(128), (intptr_t)(1));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(1), (intptr_t)(408), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(4), (intptr_t)(32), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(5), (intptr_t)(132), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(6), (intptr_t)(232), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(7), (intptr_t)(332), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  SETLO(v13, F16(v12, 4));
  W_win_ctrl_to_back((intptr_t)(F32(v12, 0)), (intptr_t)(v13));
  W_sys_page_arrows((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v11), (intptr_t)(v12));
  W_sv_41bf60((intptr_t)((int *)v12), (intptr_t)(1));
  W_tm_41c2f0((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(&(*v21__f)), (intptr_t)(&(*v23__f)));
  W_tm_419900((intptr_t)(v12), (intptr_t)((*v21__f)), (intptr_t)((uint16_t)(*v23__f)));
  v14 = F32(F32(v11, 0x48), 4 * F16(v11, 0x4e) - 4);
  (*v21__f) = (int)v14;
  while ( 1 )
  {
    v15 = W_menu_run((intptr_t)(c), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( v15 == 11 )
      break;
    if ( v15 == 12 )
    {
      v16 = (int *)(v14[95] + 16 * F32(v14[93], 20));
      v17 = v16[1];
      v18 = (int *)*v16;
      if ( W_sv_41b670((intptr_t)(Buffer), (intptr_t)(*v16), (intptr_t)(v17), (intptr_t)(0)) == 0 )
        goto LABEL_26;
      (*v23__f) = 0;
      (*v24__f) = 0;
      v15 = W_mh_419eb0((intptr_t)(c), (intptr_t)(((char *)D(0x854e38))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(&(*v24__f)), (intptr_t)(0));
      if ( v15 == 0 )
      {
        W_mh_419c70((intptr_t)(c), (intptr_t)(300), (intptr_t)(F32((*v24__f), 24) + F32((*v24__f), 16) - 16), (intptr_t)(128), (intptr_t)(0), (intptr_t)(1u), (intptr_t)(2u), (intptr_t)(&(*v23__f)), (intptr_t)((int *)&(*v22__f)), (intptr_t)(0), (intptr_t)(0));
        W_grid_row_text_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(14), (intptr_t)(D(0x854d04)));
        W_grid_row_text_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(15), (intptr_t)(((char *)D(0x854d00))));
        while ( 1 )
        {
          v15 = W_menu_run((intptr_t)(c), (intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          if ( v15 == 11 )
            break;
          if ( v15 == 12 )
          {
            v15 = F32(16 * F32(((*v22__f)[93] + 20), 0) + (*v22__f)[95], 4);
            goto LABEL_17;
          }
          if ( v15 != 13 )
            goto LABEL_17;
        }
        v15 = 15;
      }
LABEL_17:
      if ( (*v24__f) != 0 )
        F32((*v24__f), 80) = 0;
      if ( (*v23__f) != 0 )
        F32((*v23__f), 80) = 0;
      if ( v15 == 14 )
      {
        W_sv_41cfd0((intptr_t)((int)v18), (intptr_t)(v17), (intptr_t)(ExistingFileName), (intptr_t)(0));
        W_sv_41cfd0((intptr_t)(1), (intptr_t)(v17), (intptr_t)(NewFileName), (intptr_t)(0));
        if ( !API_CopyFileA((intptr_t)(ExistingFileName), (intptr_t)(NewFileName), (intptr_t)(false)) )
          g_eng.h408b90(D(0x8a9530), ((char *)D(0x854ef4)), 0, NewFileName);
LABEL_26:
        W_sv_55f620((intptr_t)(c), (intptr_t)((int)Buffer), (intptr_t)(a2));
        if ( W_sv_41a890((intptr_t)(Buffer), (intptr_t)((int)v18), (intptr_t)(v17), (intptr_t)(1), (intptr_t)(0)) != 0 )
        {
          W_sv_41bf60((intptr_t)((int *)(*v21__f)), (intptr_t)(1));
          v15 = W_msg_pages((intptr_t)(c), (intptr_t)(((char *)D(0x854e00))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(128), (intptr_t)((uint16_t)v17 < 4u ? 320 : 80), (intptr_t)(0), (intptr_t)(0x28u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
          if ( v15 == 0 )
          {
            W_tm_41c320((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(v18), (intptr_t)(v17));
            W_tm_41bb80((intptr_t)(SYS32(800)), (intptr_t)(0x5Eu), (intptr_t)(2));
            (*v20__f) = 12;
            break;
          }
LABEL_31:
          (*v20__f) = v15;
          break;
        }
        v15 = W_msg_pages((intptr_t)(c), (intptr_t)(((char *)D(0x854e0c))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
        if ( v15 != 0 )
          goto LABEL_31;
        v14 = (_DWORD *)(*v21__f);
      }
      else
      {
        if ( v15 != 15 )
          goto LABEL_31;
        v14 = (_DWORD *)(*v21__f);
      }
    }
    else if ( v15 != 13 )
    {
      goto LABEL_31;
    }
  }
  (*v35__f) = -1;
  *(_DWORD *)((uint8_t *)((*v25__f) + (1) * 0x50) + 0x0) = 0;
  W_tm_41bbe0((intptr_t)(&(*v26__f)));
  return (*v20__f);
}

/* 0x41db30: int __stdcall sub_41DB30(int a1, int a2) */
int32_t el_41db30(int32_t a1, int32_t a2)
{

    intptr_t v3 = 0;
    intptr_t v4 = 0;

  if ( a1 >= a2 )
    return a1;
  v3 = 250 * (a2 - a1) / 1000;
  v4 = a2 - a1 - v3 + 2;
  if ( v4 > 0 )
    v4 = 100 * v4 * rnd() / 0x8000 % v4;
  return a1 + v4 + v3;
}

/* 0x41dba0: BOOL __thiscall sub_41DBA0(cls_41DBA0 *this, int a2, int a3, _DWORD *a4) */
int32_t el_41dba0(wl_char *self, int32_t a2, int32_t a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    uint16_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;

  if ( *a4 != a2 )
    return false;
  v5 = F16(c, 0xb54);
  v6 = 0;
  if ( v5 != 0 )
  {
    while ( F32(c, 4 * (3 * v6 + 726)) != a4[1] )
    {
      if ( ++v6 >= v5 )
        goto LABEL_6;
    }
    return false;
  }
LABEL_6:
  v7 = a4[3];
  if ( v7 != 0 )
  {
    v8 = 0;
    if ( v5 == 0 )
      return false;
    while ( F32(c, 4 * (3 * v8 + 726)) != v7 )
    {
      if ( ++v8 >= v5 )
        return false;
    }
  }
  v9 = a4[2];
  return v9 <= a3 && v9 != 0;
}

/* 0x41e070: int __thiscall sub_41E070(cls_401D40 *this, int a2) */
int32_t el_41e070(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t result = 0;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    int32_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;

  v2 = F32(c, 0x8);
  result = 0;
  while ( 1 )
  {
    v4 = F32(v2 + 4 * (uint16_t)result, 16432);
    if ( v4[636] == a2 )
      break;
    if ( (uint16_t)++result >= 8u )
      goto LABEL_6;
  }
  result = v4[625] - 1;
  if ( v4[625] != 1 )
LABEL_6:
    v4 = 0;
  v5 = 0;
  if ( v4 != 0 )
  {
    v6 = F32(v2, 8204);
    if ( v6 != 0 )
    {
      v5 = *v6;
      v7 = v6[625] - 1;
      *v6 = 0;
      if ( v7 == 0 && v6[636] != 100000104 )
      {
        v8 = v6[637];
        if ( v8 != 0 )
        {
          F32(v8, 0x158a4) = 0;
          F32(v8, 0x200) = 1;
          W_char_ai_reset((intptr_t)(v8), (intptr_t)(1));
          W_char_ground_probe((intptr_t)(v8));
        }
      }
    }
    v9 = v4[625];
    *v4 = v5;
    result = v9 - 1;
    if ( result != 0 )
    {
      F32(v2, 8204) = v4;
      return --result;
    }
    if ( v4[636] != 100000104 )
    {
      v10 = v4[637];
      if ( v10 != 0 )
      {
        if ( *v4 != 0 )
        {
          F32(v10, 0x158a4) = *v4;
          F32(v10, 0x200) = 0;
        }
        else
        {
          F32(v10, 0x158a4) = 0;
          F32(v10, 0x200) = 1;
        }
        W_char_ai_reset((intptr_t)(v10), (intptr_t)(1));
        result = W_char_ground_probe((intptr_t)(v10));
      }
    }
  }
  F32(v2, 8204) = v4;
  return result;
}

/* 0x41e4c0: int __thiscall sub_41E4C0(cls_401D40 *this, int a2, int a3, int a4) */
int32_t el_41e4c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t i = 0;
    intptr_t v11 = 0;
    int v12 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  result = F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    (*v13__f) = 4576;
    (*v14__f) = 128;
    do
    {
      v5 = *(_DWORD *)&GAP(W, 0x0, (*v13__f));
      if ( F32(v5, 464) != 0 && F32(v5, 468) == a2 )
      {
        if ( a3 != 0 )
        {
          v6 = F32(v5, 504);
          if ( v6 != 0 )
          {
            W_actor_release((intptr_t)(v6));
            F32(v5, 504) = 0;
          }
          v7 = *(_DWORD *)&GAP(W, 0x2788, 16);
          v8 = 0;
          SETLO(v6, F16(v7, 8));
          v9 = v6;
          if ( (uint16_t)v6 != 0 )
          {
            while ( F32((F32(v7, 4) + 4 * (uint16_t)v8), 0) != v5 )
            {
              v8 = ((uint8_t *)v8 + 1);
              if ( (uint16_t)v8 >= (uint16_t)v6 )
                goto LABEL_13;
            }
            v9 = v8;
          }
LABEL_13:
          if ( (uint16_t)v9 < (uint16_t)v6 )
          {
            for ( i = (uint16_t)v9;
                  (uint16_t)v9 < F16(v7, 8) - 1;
                  i = (uint16_t)v9 )
            {
              v9 = ((uint8_t *)v9 + 1);
              F32((F32(v7, 4) + 4 * i), 0) = F32(F32((v7 + 4), 0) + 4 * i, 4);
            }
            --F16(v7, 8);
          }
          W_entity_unlist((intptr_t)(v5 /*edx*/));
          W_entity_release((intptr_t)(v5), (intptr_t)(/*edx*/ 1));
          if ( F32(v5, 680) != 0 )
          {
            g_eng.op_delete((void *)F32(v5, 680));
            F32(v5, 680) = 0;
          }
        }
        else
        {
          W_actor_state_restore((intptr_t)(*(intptr_t *)&GAP(W, 0x0, (*v13__f))), (intptr_t)(a4));
        }
      }
      result = (*v14__f) - 1;
      v12 = (*v14__f) == 1;
      (*v13__f) += 4;
      --(*v14__f);
    }
    while ( !v12 );
  }
  return result;
}

/* 0x41f0c0: int __stdcall sub_41F0C0(int a1, int a2, int a3) */
int32_t el_41f0c0(int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t result = 0;

  result = a1;
  if ( a1 != 0 )
  {
    F32(a1, 89000) = a2;
    F32(a1, 89004) = a3;
  }
  return result;
}

/* 0x426110: FILE *__thiscall sub_426110(char *this, LPCSTR lpOutputString, int a3, LPCSTR a4) */
int32_t el_426110(wl_char *self, char * lpOutputString, int32_t a3, char * a4)
{
    intptr_t c = (intptr_t)self;
    uint32_t LastError = 0;
    FILE * v6 = 0;
    char * v7 = 0;
    uint32_t v8 = 0;
    char * v9 = 0;
    char * v10 = 0;
    char * v11 = 0;
    char v12 = 0;
    uint32_t v13 = 0;
    FILE * result = 0;
    FILE * v15 = 0;
    _Alignas(8) uint8_t frame_[0x218 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x18);   /* [256] at ebp-0x200 */
    char *Buffer = (char *)(frame_ + 0x118);   /* [256] at ebp-0x100 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x218 */
    char * *const v17__f = (char * *)(frame_ + 0x14);   /* ebp-0x204 */

  (*v17__f) = c;
  if ( a3 != 0 )
  {
    LastError = API_GetLastError();
    if ( LastError != 0 )
    {
      API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
      g_eng.h5bf3ee(OutputString, "[%08d] %s", LastError, Buffer);
      API_OutputDebugStringA((intptr_t)(OutputString));
      API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
      v6 = CRT_fopen(c + 1028, ((char *)D(0x85456c)));
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
  v7 = c + 4;
  F32(c, 0) = 1;
  if ( strlen(c + 4) != 0 )
  {
    strcat(v7, ((char *)D(0x854570)));
    v8 = strlen(lpOutputString) + 1;
    v9 = &v7[strlen(v7)];
    qmemcpy(v9, lpOutputString, 4 * (v8 >> 2));
    v11 = &lpOutputString[4 * (v8 >> 2)];
    v10 = &v9[4 * (v8 >> 2)];
    v12 = v8;
  }
  else
  {
    v13 = strlen(lpOutputString) + 1;
    qmemcpy(v7, lpOutputString, 4 * (v13 >> 2));
    v11 = &lpOutputString[4 * (v13 >> 2)];
    v10 = &v7[4 * (v13 >> 2)];
    v12 = v13;
  }
  qmemcpy(v10, v11, v12 & 3);
  strcat(v7, a4);
  API_OutputDebugStringA((intptr_t)(lpOutputString));
  API_OutputDebugStringA((intptr_t)(a4));
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
  result = CRT_fopen((*v17__f) + 1028, ((char *)D(0x85456c)));
  v15 = result;
  if ( result != 0 )
  {
    API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
    CRT_fprintf(
      v15,
      "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
      (*SystemTime__f).wYear,
      (*SystemTime__f).wMonth,
      (*SystemTime__f).wDay,
      (*SystemTime__f).wHour,
      (*SystemTime__f).wMinute,
      (*SystemTime__f).wSecond,
      lpOutputString,
      a4);
    return CRT_fclose(v15);
  }
  return result;
}

/* 0x438970: int __thiscall sub_438970(cls_401D40 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t el_438970(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    char * i = 0;
    intptr_t v13 = 0;

  if ( F32(c, 0x0) == 0 )
    return 0;
  v9 = 0;
  while ( 1 )
  {
    v10 = *(intptr_t *)&GAP(W, 0x1030, 4 * v9 + 5424);
    if ( F32(v10, 0x1d0) == 0 )
      break;
    if ( ++v9 >= 0x80u )
      return 0;
  }
  if ( v10 == 0 )
    return 0;
  v11 = 0;
  for ( i = &GAP(W, 0x1030, 432); ; i += 4 )
  {
    v13 = F32(i, 0);
    if ( F32(F32(i, 0), 464) != 0
      && F32(v13, 632) != 0
      && (F32(v13, 628) == a5 || a5 == 0) )
    {
      break;
    }
    if ( (unsigned int)++v11 >= 0x80 )
      return 0;
  }
  result = *(_DWORD *)&GAP(W, 0x1030, 4 * v11 + 432);
  if ( result != 0 )
  {
    W_structure_init((intptr_t)(v10), (intptr_t)(a6 + F32(F32((result + 64), 0), 4)), (intptr_t)(a7 + F32(F32((result + 64), 0), 8) + F32(F32((result + 64), 0), 16)), (intptr_t)(3), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(a8), (intptr_t)(a2));
    return (int)v10;
  }
  return result;
}

/* 0x43c1b0: int __thiscall sub_43C1B0(cls_43C1B0 *this, HMODULE hModule, int a3, int a4) */
int32_t el_43c1b0(wl_char *self, int32_t hModule, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    uint32_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t result = 0;
    uint16_t v8 = 0;
    uint16_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;
    uint32_t * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    void * v16 = 0;
    void * v17 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v19 = (intptr_t *)(frame_ + 0x4);   /* [3] at ebp-0x14 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x12);   /* ebp-0x6 */

  v5 = 0;
  v6 = F32(c, 0x8);
  if ( v6 == 0 )
    return 0;
  v8 = F16(c, 0x4);
  v9 = 0;
  if ( v8 != 0 )
  {
    while ( F32((v6 + 24 * v9), 0) != 0 )
    {
      if ( ++v9 >= v8 )
        goto LABEL_6;
    }
    v11 = v9;
  }
  else
  {
LABEL_6:
    v10 = 0;
    if ( v8 != 0 )
    {
      v11 = a3;
      v12 = (unsigned int *)(v6 + 12);
      do
      {
        if ( v5 > *v12 || v5 == 0 )
        {
          v5 = *v12;
          v11 = v10;
        }
        ++v10;
        v12 += 6;
      }
      while ( v10 < v8 );
    }
    else
    {
      v11 = a3;
    }
  }
  v13 = 3 * v11;
  v14 = F32(c, 0x8);
  v15 = 8 * v13;
  v16 = F32(v15, v14);
  v17 = (void *)(v15 + v14);
  if ( v16 != 0 )
  {
    W_sd_5bb3a0((intptr_t)(F32(v15, v14)));
    g_eng.op_delete((void *)v16);
  }
  memset(v17, 0, 0x18u);
  result = W_sd_5bb250((intptr_t)(F32((uint8_t *)(c), 0x0)), (intptr_t)((_DWORD *)(F32(c, 0x8) + v15)), (intptr_t)(hModule), (intptr_t)(a3), (intptr_t)((int)&(*v18__f)), (intptr_t)((int)v19));
  if ( result != 0 )
  {
    F32(v15 + F32(c, 0x8), 4) = a4;
    F32(v15 + F32(c, 0x8), 16) = 1;
    F32(v15 + F32(c, 0x8), 8) = 1000
                                         * (*v18__f)
                                         / ((v19[1] * HIWORD(v19[0]) * (unsigned int)(uint16_t)(*v20__f)) >> 3);
    return 1;
  }
  return result;
}

/* 0x43d4b0: int __stdcall sub_43D4B0(unsigned __int16 *a1, int a2) */
int32_t el_43d4b0(uint16_t * a1, int32_t a2)
{

    intptr_t v2 = 0;
    intptr_t v3 = 0;

  v2 = 1000 * rnd() / 0x8000 % 10;
  if ( v2 < 0 )
  {
    v3 = a2;
  }
  else if ( v2 >= 70 )
  {
    v3 = a2;
    if ( v2 >= 90 )
    {
      if ( v2 >= 95 )
      {
        if ( v2 < 100 )
          v3 = a2 + a1[3];
      }
      else
      {
        v3 = a2 + a1[2];
      }
    }
    else
    {
      v3 = a2 + a1[1];
    }
  }
  else
  {
    v3 = a2 + *a1;
  }
  return v3 % 4;
}

/* 0x43d540: int __thiscall sub_43D540(cls_43D540 *this) */
int32_t el_43d540(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    char * v18 = 0;
    intptr_t result = 0;
    int v20 = 0;
    _Alignas(8) uint8_t frame_[0xe8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v24 = (int16_t *)(frame_ + 0x1c);   /* [2] at ebp-0xcc */
    char *Buffer = (char *)(frame_ + 0x48);   /* [80] at ebp-0xa0 */
    char *v42 = (char *)(frame_ + 0x98);   /* [80] at ebp-0x50 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xe8 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xd4 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xd0 */
    int16_t *const v25__f = (int16_t *)(frame_ + 0x20);   /* ebp-0xc8 */
    int16_t *const v26__f = (int16_t *)(frame_ + 0x22);   /* ebp-0xc6 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0xc4 */
    int16_t *const v28__f = (int16_t *)(frame_ + 0x28);   /* ebp-0xc0 */
    int16_t *const v29__f = (int16_t *)(frame_ + 0x2a);   /* ebp-0xbe */
    int16_t *const v30__f = (int16_t *)(frame_ + 0x2c);   /* ebp-0xbc */
    int16_t *const v31__f = (int16_t *)(frame_ + 0x2e);   /* ebp-0xba */
    int16_t *const v32__f = (int16_t *)(frame_ + 0x30);   /* ebp-0xb8 */
    int16_t *const v33__f = (int16_t *)(frame_ + 0x32);   /* ebp-0xb6 */
    int16_t *const v34__f = (int16_t *)(frame_ + 0x34);   /* ebp-0xb4 */
    int16_t *const v35__f = (int16_t *)(frame_ + 0x36);   /* ebp-0xb2 */
    int16_t *const v36__f = (int16_t *)(frame_ + 0x38);   /* ebp-0xb0 */
    int16_t *const v37__f = (int16_t *)(frame_ + 0x3a);   /* ebp-0xae */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0xac */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0xa8 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0xa4 */

  (*v38__f) = 32;
  (*v39__f) = 32;
  v2 = F32(c, 0xc);
  (*v28__f) = 0;
  (*v29__f) = 0;
  (*v30__f) = 1;
  (*v31__f) = 2;
  (*v32__f) = 3;
  (*v33__f) = 4;
  (*v34__f) = 5;
  (*v35__f) = 6;
  (*v36__f) = 7;
  (*v37__f) = 8;
  (*v27__f) = SYS32(2984);
  (*v40__f) = 0;
  if ( F16(v2, 6) < F16(v2, 4) )
  {
    v4 = 4 * F16(v2, 6);
    W_win_init((intptr_t)(F32(F32(v2, 0), v4)), (intptr_t)(v2), (intptr_t)(360), (intptr_t)(64), (intptr_t)(240), (intptr_t)(208), (intptr_t)(4u), (intptr_t)(&(*v27__f)));
    ++F16(v2, 6);
    W_win_mgr_topmost((intptr_t)(v2));
    v3 = F32(F32(v2, 0), v4);
  }
  else
  {
    v3 = 0;
  }
  F32(c, 0x0) = (int)v3;
  v5 = W_win_ctrl_take((intptr_t)(v3));
  v6 = (int)v5;
  if ( v5 != 0 )
    W_ctrl_grid_init((intptr_t)(v5), (intptr_t)(40), (intptr_t)(40), (intptr_t)(4), (intptr_t)(3), (intptr_t)(4), (intptr_t)(0));
  else
    v6 = 0;
  F32(c, 0x4) = v6;
  v24[0] = 0;
  v24[1] = 1;
  (*v25__f) = 2;
  (*v26__f) = 3;
  W_ctrl_cursor_set((intptr_t)(v6), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v24), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v6, 20) = 1;
    F32(v6, 24) = -9;
  }
  (*v22__f) = 0;
  (*v23__f) = 0;
  F32(F32(c, 0x4), 428) = 40;
  v7 = F32(c, 0x4);
  F32(F32(v7, 376), 0) = -8;
  F32(F32((v7 + 376), 0), 4) = -16;
  F32(F32((v7 + 376), 0), 8) = 0;
  F32(F32((v7 + 376), 0), 12) = 0;
  v8 = F32(c, 0x4);
  F32(F32((v8 + 376), 0), 36) = 56;
  F32(F32((v8 + 376), 0), 40) = 0;
  F32(F32((v8 + 376), 0), 44) = 0;
  F32(F32((v8 + 376), 0), 48) = 0;
  v9 = F32(c, 0x4);
  F32(F32((v9 + 376), 0), 72) = 120;
  F32(F32((v9 + 376), 0), 76) = 0;
  F32(F32((v9 + 376), 0), 80) = 0;
  F32(F32((v9 + 376), 0), 84) = 0;
  do
  {
    v10 = F32(c, 0x4);
    v11 = F32(v10, 0x174);
    v12 = F32(v11, 16);
    if ( v12 < F32(v11, 4) )
    {
      (*v21__f) = F32(v11, 16);
      F32(v11, 16) = v12 + 1;
      v14 = 16 * v12;
      F32((F32(v10, 0x17c) + v14), 0) = 0;
      F32(F32(v10, 0x17c) + v14, 4) = (*v23__f);
      F32(F32(v10, 0x17c) + v14, 8) = 1;
      W_grid_row_reset((intptr_t)(v10), (intptr_t)((*v21__f)));
      v13 = v12;
    }
    else
    {
      v13 = -1;
    }
    v15 = F32(c, 0x4);
    F32(v24, 0) = SYS32(2968);
    v16 = F32(v15, 372);
    if ( F32(v16, 8) > 0 && F32(v16, 16) > v13 )
    {
      v17 = 16 * v13;
      if ( F32(F32((16 * v13 + F32(v15, 380) + 12), 0), 4) == 0 )
        F32(F32((v17 + F32(v15, 380) + 12), 0), 4) = (intptr_t)g_eng.op_new(0x54u);
      W_icon_set((intptr_t)(F32(F32((v17 + F32(v15, 380) + 12), 0), 4)), (intptr_t)(F32(v24, 0)), (intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
    v18 = el_43e010((*v23__f), v42);
    g_eng.h5bf3ee(Buffer, "%s", v18);
    W_grid_cell_text((intptr_t)(F32(c, 0x4)), (intptr_t)(v13), (intptr_t)(1), (intptr_t)(Buffer));
    result = (*v22__f) + 1;
    v20 = (uint16_t)++(*v22__f) < 4u;
    ++(*v23__f);
  }
  while ( v20 );
  return result;
}

/* 0x43d830: __int16 __thiscall sub_43D830(cls_43D830 *this) */
int32_t el_43d830(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t i = 0;
    uint16_t v3 = 0;
    char * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    char * v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    char * v29 = 0;
    uint32_t v30 = 0;
    _Alignas(8) uint8_t frame_[0xa4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v54 = (char *)(frame_ + 0x54);   /* [80] at ebp-0x50 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xa4 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xa0 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x8c */
    char * *const v36__f = (char * *)(frame_ + 0x20);   /* ebp-0x84 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x80 */
    int16_t *const v38__f = (int16_t *)(frame_ + 0x28);   /* ebp-0x7c */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x78 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x74 */
    int16_t *const v41__f = (int16_t *)(frame_ + 0x34);   /* ebp-0x70 */
    int16_t *const v42__f = (int16_t *)(frame_ + 0x36);   /* ebp-0x6e */
    int16_t *const v43__f = (int16_t *)(frame_ + 0x38);   /* ebp-0x6c */
    int16_t *const v44__f = (int16_t *)(frame_ + 0x3a);   /* ebp-0x6a */
    int16_t *const v45__f = (int16_t *)(frame_ + 0x3c);   /* ebp-0x68 */
    int16_t *const v46__f = (int16_t *)(frame_ + 0x3e);   /* ebp-0x66 */
    int16_t *const v47__f = (int16_t *)(frame_ + 0x40);   /* ebp-0x64 */
    int16_t *const v48__f = (int16_t *)(frame_ + 0x42);   /* ebp-0x62 */
    int16_t *const v49__f = (int16_t *)(frame_ + 0x44);   /* ebp-0x60 */
    int16_t *const v50__f = (int16_t *)(frame_ + 0x46);   /* ebp-0x5e */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x5c */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x58 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x54 */

  SETLO(i, F16(c, 0x12));
  v3 = 0;
  if ( (uint16_t)i != 0 )
  {
    v4 = ((uint8_t *)(c) + 0x14);
    for ( i = (uint16_t)i; i != 0; --i )
    {
      if ( F32(v4, 0) != 0 )
        ++v3;
      v4 += 48;
    }
  }
  v5 = F32(c, 0xc);
  (*v51__f) = 32;
  (*v52__f) = 32;
  (*v41__f) = 0;
  (*v42__f) = 0;
  (*v43__f) = 1;
  (*v44__f) = 2;
  (*v45__f) = 3;
  (*v46__f) = 4;
  (*v47__f) = 5;
  (*v48__f) = 6;
  (*v49__f) = 7;
  (*v50__f) = 8;
  (*v40__f) = SYS32(2984);
  (*v53__f) = 0;
  if ( F16(v5, 6) < F16(v5, 4) )
  {
    v7 = 4 * F16(v5, 6);
    W_win_init((intptr_t)(F32(F32(v5, 0), v7)), (intptr_t)(v5), (intptr_t)(360), (intptr_t)(64), (intptr_t)(240), (intptr_t)(40 * (v3 + 2)), (intptr_t)(4u), (intptr_t)(&(*v40__f)));
    ++F16(v5, 6);
    W_win_mgr_topmost((intptr_t)(v5));
    v6 = F32(F32(v5, 0), v7);
  }
  else
  {
    v6 = 0;
  }
  F32(c, 0x0) = (int)v6;
  v8 = W_win_ctrl_take((intptr_t)(v6));
  v9 = (int)v8;
  if ( v8 != 0 )
    W_ctrl_grid_init((intptr_t)(v8), (intptr_t)(32), (intptr_t)(64), (intptr_t)(4), (intptr_t)(4), (intptr_t)(4), (intptr_t)(1));
  else
    v9 = 0;
  F32(c, 0x4) = v9;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v9, 20) = 1;
    F32(v9, 24) = -9;
  }
  F32(F32(c, 0x4), 428) = 40;
  v10 = F32(c, 0x4);
  F32(F32(v10, 376), 0) = 0;
  F32(F32((v10 + 376), 0), 4) = 0;
  F32(F32((v10 + 376), 0), 8) = 0;
  F32(F32((v10 + 376), 0), 12) = 0;
  v11 = F32(c, 0x4);
  F32(F32((v11 + 376), 0), 36) = 80;
  F32(F32((v11 + 376), 0), 40) = -16;
  F32(F32((v11 + 376), 0), 44) = 0;
  F32(F32((v11 + 376), 0), 48) = 0;
  v12 = F32(c, 0x4);
  F32(F32((v12 + 376), 0), 72) = 144;
  F32(F32((v12 + 376), 0), 76) = 0;
  F32(F32((v12 + 376), 0), 80) = 0;
  F32(F32((v12 + 376), 0), 84) = 0;
  v13 = F32(c, 0x4);
  F32(F32((v13 + 376), 0), 108) = 80;
  F32(F32((v13 + 376), 0), 112) = -16;
  F32(F32((v13 + 376), 0), 116) = 0;
  F32(F32((v13 + 376), 0), 120) = 0;
  v14 = 0;
  for ( (*j__f) = 0; (uint16_t)(*j__f) < F16(c, 0x12); ++(*j__f) )
  {
    (*v36__f) = (uint8_t *)c + 48 * v14;
    if ( F32((*v36__f), 4 * 5) != 0 )
    {
      v15 = F32(c, 0x4);
      v16 = F32(v15, 0x174);
      v17 = F32(v16, 16);
      if ( v17 < F32(v16, 4) )
      {
        (*v33__f) = F32(v16, 16);
        F32(v16, 16) = v17 + 1;
        v19 = 16 * v17;
        F32((v19 + F32(v15, 0x17c)), 0) = 0;
        F32(v19 + F32(v15, 0x17c), 4) = v14;
        F32(F32(v15, 0x17c) + v19, 8) = 1;
        W_grid_row_reset((intptr_t)(v15), (intptr_t)((*v33__f)));
        v18 = v17;
      }
      else
      {
        v18 = -1;
      }
      W_grid_cell_text((intptr_t)(F32(c, 0x4)), (intptr_t)(v18), (intptr_t)(0), (intptr_t)((*v36__f) + 32));
      v20 = F32((*v36__f), 4 * 7);
      (*v34__f) = v20;
      (*v38__f) = v20;
      if ( F32((*v36__f), 4 * 14) == 0 )
        (*v38__f) = v20 + 4;
      v21 = F32(c, 0x4);
      (*v39__f) = SYS32(2968);
      v22 = F32(v21, 372);
      if ( F32(v22, 8) > 1 && F32(v22, 16) > v18 )
      {
        v23 = 16 * v18;
        if ( F32(F32((16 * v18 + F32(v21, 380) + 12), 0), 28) == 0 )
          F32(F32((v23 + F32(v21, 380) + 12), 0), 28) = (intptr_t)g_eng.op_new(0x54u);
        W_icon_set((intptr_t)(F32(F32((v23 + F32(v21, 380) + 12), 0), 28)), (intptr_t)((*v39__f)), (intptr_t)((*v38__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v20 = (*v34__f);
      }
      switch ( v20 )
      {
        case 0:
          v24 = ((char *)D(0x855304));
          goto LABEL_31;
        case 1:
          v24 = ((char *)D(0x8552fc));
          goto LABEL_31;
        case 2:
          v24 = ((char *)D(0x8552f4));
          goto LABEL_31;
        case 3:
          v24 = ((char *)D(0x8552ec));
LABEL_31:
          strcpy(v54, v24);
          break;
        default:
          break;
      }
      W_grid_cell_text((intptr_t)(F32(c, 0x4)), (intptr_t)(v18), (intptr_t)(2), (intptr_t)(v54));
      if ( F32(c, 0x3d4) == 0 && F32((*v36__f), 4 * 13) != 0 )
      {
        v25 = F32(c, 0x4);
        (*v35__f) = SYS32(2968);
        v26 = F32(v25, 372);
        if ( F32(v26, 8) > 3 && F32(v26, 16) > v18 )
        {
          v27 = 16 * v18;
          if ( F32(F32((F32(v25, 380) + v27 + 12), 0), 76) == 0 )
            F32(F32((F32(v25, 380) + v27 + 12), 0), 76) = (intptr_t)g_eng.op_new(0x54u);
          W_icon_set((intptr_t)(F32(F32((F32(v25, 380) + v27 + 12), 0), 76)), (intptr_t)((*v35__f)), (intptr_t)(8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        }
      }
    }
    v14 = (*j__f) + 1;
  }
  v28 = W_win_text_add((intptr_t)(F32(c, 0x0)), (intptr_t)(40), (intptr_t)(24), (intptr_t)(32), (intptr_t)(3));
  (*v32__f) = F32(c, 0x0);
  F32(c, 0x8) = (int)v28;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v32__f)), (intptr_t)(v28));
  W_sys_box_cursor((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(F32(c, 0x8)), (intptr_t)(0), (intptr_t)(0));
  F32(F32((F32(c, 0x8) + 368), 0), 28) = 0;
  W_text_metrics_set((intptr_t)(F32(c, 0x8)), (intptr_t)(1), (intptr_t)(F32(CFG(), 584)), (intptr_t)(1), (intptr_t)(0));
  if ( F32(c, 0x3d4) != 0 )
  {
    v29 = ((char *)D(0x8552e0));
  }
  else if ( F32(c, 0x34) != 0 )
  {
    v30 = 500 * F32(c, 0x3d8);
    if ( v30 > 0xFA0 )
      v30 = 4000;
    W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xBu), (intptr_t)(0), (intptr_t)(v30));
    v29 = ((char *)D(0x8552d4));
  }
  else
  {
    v29 = ((char *)D(0x8552c8));
  }
  strcpy(v54, v29);
  return W_ctrl_text_set((intptr_t)(F32(c, 0x8)), (intptr_t)(v54));
}

/* 0x43dd30: void __thiscall sub_43DD30(cls_43DD30 *this, _DWORD *a2, unsigned __int16 a3) */
void el_43dd30(wl_char *self, int32_t * a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    int32_t * v3 = 0;
    uint16_t v5 = 0;
    uint16_t v6 = 0;
    int32_t * v7 = 0;
    intptr_t i = 0;
    int32_t * v9 = 0;
    uint16_t v10 = 0;
    int32_t * v11 = 0;
    int32_t * v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    int32_t * v20 = 0;
    intptr_t j = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    int32_t * v25 = 0;
    intptr_t v26 = 0;
    intptr_t k = 0;
    int32_t * v28 = 0;
    uint16_t v29 = 0;
    int32_t * v30 = 0;
    uint16_t v31 = 0;
    int32_t * v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    int32_t * v35 = 0;
    intptr_t m = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *const v37__f = (int16_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  v3 = a2;
  v5 = 0;
  v6 = a3;
  (*v40__f) = c;
  if ( a3 != 0 )
  {
    v7 = a2 + 9;
    for ( i = a3; i != 0; --i )
    {
      *(v7 - 1) = 0;
      *v7 = *(v7 - 9);
      v7 += 12;
    }
  }
  F32(c, 0x3d4) = 0;
  if ( a3 != 0 )
  {
    v9 = a2 + 9;
    do
    {
      if ( *v9 != 0 )
      {
        v10 = 0;
        while ( 1 )
        {
          if ( v5 != v10 )
          {
            v11 = &a2[12 * v10];
            if ( v11[9] != 0 && (*(v9 - 7) + 2) % 4 == v11[2] )
              break;
          }
          if ( ++v10 >= a3 )
            goto LABEL_14;
        }
        *v9 = 0;
        a2[12 * v10 + 9] = 0;
      }
LABEL_14:
      ++v5;
      v9 += 12;
    }
    while ( v5 < a3 );
  }
  (*v37__f) = 0;
  if ( a3 != 0 )
  {
    v12 = a2 + 2;
    while ( 1 )
    {
      if ( v12[7] == 0 )
        goto LABEL_38;
      v13 = 0;
      v14 = 0;
      (*v39__f) = 0;
      (*v38__f) = 0;
      v15 = a2 + 2;
      do
      {
        if ( (*v37__f) != v13 && v15[7] != 0 )
        {
          v16 = *v12;
          v17 = *v15;
          if ( *v12 != 0 )
          {
            if ( v16 != 3 || v17 != 0 )
              goto LABEL_26;
          }
          else
          {
            if ( v17 == 3 )
            {
              v14 = 1;
              goto LABEL_33;
            }
LABEL_26:
            if ( v16 == v17 )
              goto LABEL_31;
            if ( v16 == (v17 + 2) % 4 )
            {
              v6 = a3;
LABEL_31:
              (*v38__f) = 1;
              goto LABEL_33;
            }
            v6 = a3;
            if ( v16 > v17 )
            {
              v14 = 1;
              goto LABEL_33;
            }
          }
          (*v39__f) = 1;
        }
LABEL_33:
        ++v13;
        v15 += 12;
      }
      while ( v13 < v6 );
      if ( (v14 != 0 || (*v38__f) == 0) && (*v39__f) == 0 )
        v12[6] = 1;
LABEL_38:
      v12 += 12;
      if ( (uint16_t)++(*v37__f) >= v6 )
      {
        v3 = a2;
        break;
      }
    }
  }
  v18 = 0;
  v19 = 0;
  if ( v6 == 0 )
    goto LABEL_49;
  v20 = v3;
  for ( j = v6; j != 0; --j )
  {
    if ( *v20 != 0 )
    {
      if ( v20[8] != 0 )
        v18 = 1;
      else
        v19 = 1;
    }
    v20 += 12;
  }
  if ( v18 == 0 || v19 == 0 )
  {
LABEL_49:
    v22 = 0;
    v23 = 0;
    v24 = -1;
    if ( v6 != 0 )
    {
      v25 = a2 + 2;
      v26 = v6;
      for ( k = v6; k != 0; --k )
      {
        if ( *(v25 - 2) != 0 )
        {
          if ( v25[7] != 0 )
          {
            if ( v24 == -1 )
            {
              v24 = *v25;
            }
            else if ( v24 != *v25 )
            {
              v22 = 1;
            }
          }
          else
          {
            v23 = 1;
          }
        }
        v25 += 12;
      }
      if ( v24 != -1 && v23 != 0 && v22 == 0 )
      {
        v28 = a2 + 8;
        do
        {
          if ( *(v28 - 8) != 0 && v24 == *(v28 - 6) )
            *v28 = 1;
          v28 += 12;
          --v26;
        }
        while ( v26 != 0 );
      }
    }
  }
  v29 = 0;
  if ( v6 != 0 )
  {
    v30 = a2 + 8;
    do
    {
      if ( *(v30 - 8) != 0 && *v30 == 0 )
      {
        v31 = 0;
        while ( 1 )
        {
          if ( v29 != v31 )
          {
            v32 = &a2[12 * v31];
            if ( v32[8] != 0 && *(v30 - 6) == v32[2] )
              break;
          }
          if ( ++v31 >= v6 )
            goto LABEL_78;
        }
        *v30 = 1;
      }
LABEL_78:
      ++v29;
      v30 += 12;
    }
    while ( v29 < v6 );
  }
  v33 = 0;
  v34 = 0;
  if ( v6 == 0 )
    goto LABEL_88;
  v35 = a2;
  for ( m = v6; m != 0; --m )
  {
    if ( *v35 != 0 )
    {
      if ( v35[8] != 0 )
        v33 = 1;
      else
        v34 = 1;
    }
    v35 += 12;
  }
  if ( v33 == 0 || v34 == 0 )
LABEL_88:
    F32((*v40__f), 0x3d4) = 1;
}

/* 0x43e010: char *__stdcall sub_43E010(int a1, char *a2) */
int32_t el_43e010(int32_t a1, char * a2)
{

    char * v2 = 0;

  switch ( a1 )
  {
    case 0:
      v2 = ((char *)D(0x855304));
      goto LABEL_6;
    case 1:
      v2 = ((char *)D(0x8552fc));
      goto LABEL_6;
    case 2:
      v2 = ((char *)D(0x8552f4));
      goto LABEL_6;
    case 3:
      v2 = ((char *)D(0x8552ec));
LABEL_6:
      strcpy(a2, v2);
      break;
    default:
      return a2;
  }
  return a2;
}

/* 0x441de0: int __thiscall sub_441DE0(cls_4022A0 *this, _DWORD *a2, int a3) */
int32_t el_441de0(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t result = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;

  v4 = F32(c, 0x14824);
  F32(c, 0x14858) = 0;
  F32(c, 0x14854) = 0;
  F32(c, 0x1485c) = 0;
  F32(c, 0x14860) = 0;
  F16(c, 0x14868) = 0;
  F32(c, 0x14864) = 0;
  F16(c, 0x1486a) = 0;
  F32(c, 0x1486c) = 0;
  F16(c, 0x14870) = 0;
  result = a2[161];
  if ( result == 0 )
    goto LABEL_19;
  v6 = a2[1];
  v7 = F32(c, 0x1594c);
  if ( v6 < v4 )
  {
    if ( v7 > 0 )
    {
      if ( v7 <= 2 )
      {
        v8 = 0;
        goto LABEL_9;
      }
      if ( v7 == 3 )
        goto LABEL_18;
    }
    v8 = 1;
LABEL_9:
    result = W_npc_floor_ahead((intptr_t)(c), (intptr_t)(a2), (intptr_t)(a2[3] + v6 + 1599), (intptr_t)(0), (intptr_t)(v8), (intptr_t)(5));
    if ( result != 0 )
      goto LABEL_18;
    goto LABEL_19;
  }
  if ( v7 > 0 )
  {
    if ( v7 <= 2 )
    {
      v9 = 0;
      goto LABEL_17;
    }
    if ( v7 == 3 )
    {
LABEL_18:
      W_npc_navigate((intptr_t)(c), (intptr_t)(v4), (intptr_t)(1600), (intptr_t)((int)a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      result = abs32(v4 - a2[1]);
      if ( result >= 1600 )
        return result;
      goto LABEL_19;
    }
  }
  v9 = 1;
LABEL_17:
  result = W_npc_floor_ahead((intptr_t)(c), (intptr_t)(a2), (intptr_t)(v6 - 1600), (intptr_t)(0), (intptr_t)(v9), (intptr_t)(5));
  if ( result != 0 )
    goto LABEL_18;
LABEL_19:
  F16(c, 0x1422c) = 0;
  F16(c, 0x1422e) = 0;
  F16(c, 0x14230) = 0;
  return result;
}

/* 0x4428f0: int __thiscall sub_4428F0(cls_4428F0 *this) */
int32_t el_4428f0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    int32_t * v2 = 0;
    intptr_t i = 0;
    intptr_t result = 0;

  v2 = (int *)(c + 4 * (12799));
  for ( i = 10; i != 0; --i )
  {
    if ( *v2 != 0 )
    {
      W_entity_release((intptr_t)(*v2), (intptr_t)(/*edx*/ 0));
      *v2 = 0;
    }
    ++v2;
  }
  return result;
}

/* 0x44bf60: void __thiscall sub_44BF60(cls_4022A0 *this, int a2, int a3, int a4, int a5) */
void el_44bf60(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    char * v7 = 0;
    intptr_t v8 = 0;
    uint16_t v9 = 0;
    uint16_t v10 = 0;
    char * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    int32_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t * v23 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x28 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x24 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x10 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x8 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4 */

  v5 = a2;
  if ( F16(a2, 72) != 54 )
    return;
  F32(a4, 0) = 2;
  if ( F16(v5, 84) == 1 )
  {
    v7 = (char *)D(0x670f98);
    a2 = 45;
    (*v28__f) = 8;
    a4 = -2400;
    goto LABEL_7;
  }
  if ( F16(v5, 84) != 7 )
  {
    v7 = (char *)D(0x670ad8);
    a2 = 90;
    (*v28__f) = 20;
    a4 = -800;
LABEL_7:
    v8 = 0;
    goto LABEL_8;
  }
  v8 = 0;
  v7 = (char *)D(0x671458);
  a2 = 120;
  (*v28__f) = 20;
  a4 = 0;
LABEL_8:
  v9 = F16(v5, 88);
  if ( v9 != 0 )
  {
    if ( F8(v5, 116) == 0 )
      return;
    if ( v9 < 3u )
    {
      v11 = &v7[404 * v9];
      if ( F32(v5, 108) != v11 )
      {
        F32(v5, 108) = v11;
        F8(v5, 116) = 0;
        F16(v5, 114) = 0;
        F16(v5, 112) = 0;
      }
    }
    v10 = v9 + 1;
  }
  else
  {
    if ( F32(v5, 108) != v7 )
    {
      F32(v5, 108) = v7;
      F8(v5, 116) = 0;
      F16(v5, 114) = 0;
      F16(v5, 112) = 0;
    }
    v10 = 1;
  }
  F16(v5, 88) = v10;
  v12 = v10 - 1;
  if ( v12 != 0 )
  {
    v13 = v12 - 1;
    if ( v13 != 0 )
    {
      if ( v13 == 2 )
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
      }
    }
    else
    {
      v14 = F32(v5, 96);
      v15 = F32(v5, 76);
      (*v29__f) = F32(v5, 44);
      F8(v5, 127) = 1;
      (*v30__f) = v14;
      v16 = rnd();
      (*v31__f) = W_char_skill_roll((intptr_t)(c), (intptr_t)(v15), (intptr_t)(1000), (intptr_t)(1000 * v16 / 0x8000));
      if ( v15[44] == 7 )
      {
        (*v27__f) = v15[180];
        v17 = rnd();
        v8 = W_char_mag_roll((intptr_t)(c), (intptr_t)(v15[179]), (intptr_t)(1000 * v17 / 0x8000), (intptr_t)((*v27__f)));
      }
      (*v26__f) = F32(F32(c, 0x750), 216) + F32(F32(c, 0x750), 224) + F32(F32(c, 0x750), 228);
      (*v25__f) = F32(c, 0x1f0);
      v18 = W_char_mag_sum((intptr_t)(c));
      v19 = W_shot_spawn((intptr_t)(c), (intptr_t)((int)c), (intptr_t)(F32(c, 0x1dc)), (intptr_t)(100029), (intptr_t)((_DWORD *)v5), (intptr_t)(2400), (intptr_t)(a4), (intptr_t)(1), (intptr_t)(a3), (intptr_t)(10), (intptr_t)((*v31__f)), (intptr_t)(a2), (intptr_t)((*v30__f)), (intptr_t)((*v29__f)), (intptr_t)(v15[44]), (intptr_t)(v15[45] + v18), (intptr_t)(v8), (intptr_t)((*v25__f)), (intptr_t)((*v26__f)), (intptr_t)(2));
      if ( v19 != 0 )
        F32(v19, 516) = (*v28__f);
    }
  }
  else
  {
    v20 = rnd();
    v21 = (_DWORD *)F32(c, 0x40);
    v22 = 1000 * v20 / 0x8000 - 500;
    if ( a3 == 0 )
    {
      v23 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1164)), (intptr_t)(0));
      if ( v23 != 0 )
      {
        v24 = *v23;
        if ( W_sfx_place((intptr_t)(c), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v21), (intptr_t)(&a3), (intptr_t)(&a2)) != 0 )
        {
          g_eng.dsb_volume_5bb870(v24, a3);
          g_eng.dsb_pan_5bb880(v24, a2);
          g_eng.dsb_freq_5bb890(v24, v22);
          g_eng.dsb_play_5bb8c0(v24);
        }
      }
    }
  }
}

/* 0x4675c0: int __thiscall sub_4675C0(cls_4675C0 *this, _DWORD *a2, unsigned __int16 a3) */
int32_t el_4675c0(wl_char *self, int32_t * a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint32_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;
    uint16_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    uint16_t v17 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  (*v19__f) = 1;
  v3 = 1374389535;
  if ( a2[11] != 1 )
    v3 = -1374389535;
  v4 = (int)((uint64_t)((int)a2[10] * (int64_t)v3) >> 32) >> 5;
  v5 = (v4 >> 31) + v4;
  if ( v5 < 0 )
  {
    v12 = F16(c, 0x44);
    v13 = a3;
    v14 = -F32(c, 0x15aa0);
    if ( a3 >= v12 )
      v13 = 0;
    v15 = F32(c, 0x40);
    if ( v14 > v5 + F32(v15 + 660 * v13, 4) )
    {
      v16 = a3;
      if ( a3 >= v12 )
        v16 = 0;
      if ( v14 - F32(v15 + 660 * v16, 4) <= 0 )
      {
        v17 = a3;
        if ( a3 >= v12 )
          v17 = 0;
        v5 = v14 - F32(v15 + 660 * v17, 4);
      }
      else
      {
        v5 = 0;
      }
      goto LABEL_24;
    }
  }
  else
  {
    v6 = F16(c, 0x44);
    v7 = F32(c, 0x15aa4) + F32(*(_DWORD *)&GAP(W, 0x1030, 24), 2887736);
    v8 = a3;
    if ( a3 >= v6 )
      v8 = 0;
    v9 = F32(c, 0x40);
    if ( v7 < v5 + F32(v9 + 660 * v8, 4) + F32(v9 + 660 * v8, 12) )
    {
      v10 = a3;
      if ( a3 >= v6 )
        v10 = 0;
      if ( v7 - F32(v9 + 660 * v10, 12) - F32(v9 + 660 * v10, 4) >= 0 )
      {
        v11 = a3;
        if ( a3 >= v6 )
          v11 = 0;
        v5 = v7 - F32(v9 + 660 * v11, 12) - F32(v9 + 660 * v11, 4);
      }
      else
      {
        v5 = 0;
      }
LABEL_24:
      (*v19__f) = 0;
    }
  }
  a2[1] += v5;
  return (*v19__f);
}

/* 0x467750: int __thiscall sub_467750(cls_467750 *this, int a2, unsigned __int16 a3) */
int32_t el_467750(wl_char *self, int32_t a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    uint16_t v16 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v3 = F32(a2, 24) / 100;
  (*v18__f) = 1;
  if ( v3 < 0 )
  {
    v11 = F16(c, 0x44);
    v12 = a3;
    v13 = -F32(c, 0x15a98);
    if ( a3 >= v11 )
      v12 = 0;
    v14 = F32(c, 0x40);
    if ( v13 > v3 + F32(v14 + 660 * v12, 8) + F32(v14 + 660 * v12, 20) )
    {
      v15 = a3;
      if ( a3 >= v11 )
        v15 = 0;
      if ( v13 - F32(v14 + 660 * v15, 20) - F32(v14 + 660 * v15, 8) > 0 )
      {
        v3 = 0;
        goto LABEL_23;
      }
      v16 = a3;
      if ( a3 >= v11 )
        v16 = 0;
      v10 = v13 - F32(v14 + 660 * v16, 20) - F32(v14 + 660 * v16, 8);
LABEL_22:
      v3 = v10;
      goto LABEL_23;
    }
  }
  else
  {
    v4 = F16(c, 0x44);
    v5 = F32(c, 0x15a9c) + F32(*(_DWORD *)&GAP(W, 0x1030, 24), 2887740);
    v6 = a3;
    if ( a3 >= v4 )
      v6 = 0;
    v7 = F32(c, 0x40);
    if ( v5 < F32(v7 + 660 * v6, 8) + F32(v7 + 660 * v6, 16) + v3 - 1 )
    {
      v8 = a3;
      if ( a3 >= v4 )
        v8 = 0;
      if ( v5 - F32(v7 + 660 * v8, 16) - F32(v7 + 660 * v8, 8) + 1 < 0 )
      {
        v3 = 0;
LABEL_23:
        (*v18__f) = 0;
        goto LABEL_24;
      }
      v9 = a3;
      if ( a3 >= v4 )
        v9 = 0;
      v10 = v5 - F32(v7 + 660 * v9, 16) - F32(v7 + 660 * v9, 8) + 1;
      goto LABEL_22;
    }
  }
LABEL_24:
  F32(a2, 8) += v3;
  return (*v18__f);
}

/* 0x468970: _WORD *__thiscall sub_468970(cls_468970 *this, char a2) */
int32_t el_468970(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t * v6 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  if ( F32(c, 0x0) != 0 )
  {
    v3 = 0;
    for ( (*i__f) = 0; (uint16_t)v3 < F16(c, 0x4); (*i__f) = v3 )
    {
      v4 = 0;
      v5 = F32(c, 0x0) + 1296 * (uint16_t)v3;
      if ( F16(v5, 6) != 0 )
      {
        do
        {
          v6 = F32(v5, 40 * v4 + 28);
          if ( v6 != 0 )
          {
            g_eng.dsb_stop_5bb8e0(*v6);
            F32(F32((v5 + 40 * v4 + 28), 0), 4) = 0;
            F32(v5 + 40 * v4, 28) = 0;
          }
          ++v4;
        }
        while ( v4 < F16(v5, 6) );
        v3 = (*i__f);
      }
      ++v3;
    }
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  F16(c, 0x4) = 0;
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return c;
}

/* 0x479ca0: BOOL __thiscall sub_479CA0(cls_479CA0 *this, int a2, unsigned int a3, unsigned int a4, int a5, int a6, int a7) */
int32_t el_479ca0(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t i = 0;
    int32_t * v11 = 0;
    uint32_t v12 = 0;

  if ( a6 == 5 || a6 == 6 )
    v8 = 0;
  else
    v8 = a6;
  v9 = 0;
  for ( i = c + 4 * (3); ; (i += 4) )
  {
    v11 = (_DWORD *)F32(i, 0);
    if ( **(_DWORD **)i == v8 )
    {
      v12 = a2 - v11[1];
      if ( a3 <= v12 && a4 >= v12 && v11[2] == a5 )
        break;
    }
    if ( ++v9 >= 64 )
      return false;
  }
  if ( a7 != 0 )
    **((int32_t **)c + v9 + 3) = 0;
  return v9 >= 0;
}

/* 0x47a300: int __thiscall sub_47A300(cls_47A300 *this, int a2, int a3) */
int32_t el_47a300(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t i = 0;
    intptr_t v6 = 0;
    int16_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    int32_t * v11 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    int16_t *const v15__f = (int16_t *)(frame_ + 0x8);   /* ebp-0x4 */

  v3 = F32(c, 0x750);
  v4 = F32(v3, 320);
  SETLO(i, F16(v3, 324));
  v6 = 0;
  (*v14__f) = v3;
  (*v13__f) = 0;
  (*v15__f) = i;
  while ( 1 )
  {
    if ( a3 != 0 || (uint16_t)v6 != 0 )
    {
      if ( a2 != 0 )
      {
        if ( v4 == 1 )
          goto LABEL_16;
        if ( v4 != 2 )
        {
          if ( v4 != 3 )
            goto LABEL_9;
LABEL_8:
          v4 = 2;
          goto LABEL_9;
        }
LABEL_20:
        v4 = 1;
        goto LABEL_9;
      }
      switch ( v4 )
      {
        case 1:
          goto LABEL_8;
        case 2:
LABEL_16:
          v4 = 3;
          break;
        case 3:
          goto LABEL_20;
        default:
          break;
      }
    }
LABEL_9:
    v7 = 0;
    if ( (uint16_t)i != 0 )
      break;
LABEL_27:
    (*v13__f) = ++v6;
    if ( (uint16_t)v6 >= 2u )
      return 0;
  }
  v8 = (_DWORD *)(v3 + 328);
  for ( i = (uint16_t)i; i != 0; --i )
  {
    if ( v8[1] == v4 )
    {
      v9 = 10101;
      v10 = 0;
      while ( v9 != *v8 )
      {
        v9 = ((int32_t *)D(0x6826c0))[192 * ++v10];
        if ( v9 == 0 )
          goto LABEL_24;
      }
      v11 = &((int32_t *)D(0x6826c0))[192 * v10];
      if ( v11 != 0 && v11[42] != 0 )
        ++v7;
    }
LABEL_24:
    v8 += 3;
  }
  if ( v7 == 0 )
  {
    SETLO(i, (*v15__f));
    v3 = (*v14__f);
    v6 = (*v13__f);
    goto LABEL_27;
  }
  F32((*v14__f), 320) = v4;
  return 1;
}

/* 0x47a420: int __thiscall sub_47A420(cls_47A420 *this, int a2, int a3) */
int32_t el_47a420(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    uint16_t v6 = 0;
    int16_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    int32_t * v14 = 0;
    uint16_t v15 = 0;
    intptr_t v16 = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    char * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint16_t v22 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  v3 = c;
  v4 = F32(c, 0x750);
  switch ( F32(v4, 320) )
  {
    case 1:
      v5 = 0;
      break;
    case 2:
      v5 = 1;
      break;
    case 3:
      v5 = 2;
      break;
    default:
      v5 = a3;
      break;
  }
  v6 = F16(v4 + 2 * v5, 312);
  SETLO((*v25__f), v6);
  if ( F16(v4, 324) == 0 )
    return 0;
  v7 = 0;
  (*v26__f) = 0;
  while ( 1 )
  {
    if ( a3 != 0 || v7 != 0 )
    {
      if ( a2 != 0 )
        v8 = v6 + 9;
      else
        v8 = v6 + 1;
      (*v25__f) = v8 % 10;
    }
    v9 = F32(v3, 0x750);
    v10 = 0;
    v11 = F16(v9, 324);
    if ( v11 != 0 )
    {
      while ( 1 )
      {
        if ( F32(v9 + 12 * v10, 332) == F32(v9, 320)
          && F16((v9 + 4 * (3 * v10 + 84)), 0) == (uint16_t)(*v25__f) )
        {
          v12 = 10101;
          v13 = 0;
          while ( v12 != F32(v9 + 12 * v10, 328) )
          {
            v12 = ((int32_t *)D(0x6826c0))[192 * ++v13];
            if ( v12 == 0 )
              goto LABEL_25;
          }
          v14 = &((int32_t *)D(0x6826c0))[192 * v13];
          if ( v14 != 0 && v14[42] != 0 )
            break;
        }
LABEL_25:
        if ( ++v10 >= v11 )
          goto LABEL_44;
      }
      if ( v9 + 12 * v10 != -328 )
      {
        v15 = 0;
        v16 = F16(v9 + 12 * v10, 328);
        v17 = F16(c, 0xc754);
        if ( v17 != 0 )
        {
          while ( *(_DWORD *)&GAP(c, 0x754, 768 * v15) != v16 )
          {
            if ( ++v15 >= v17 )
              goto LABEL_44;
          }
          v18 = 768 * v15;
          v19 = &GAP(c, 0x754, v18);
          if ( &GAP(c, 0x0, v18) != (char *)-1876 && *(_DWORD *)&GAP(c, 0x754, v18 + 168) != 0 )
            break;
        }
      }
    }
LABEL_44:
    v7 = ++(*v26__f);
    if ( (uint16_t)(*v26__f) >= 9u )
      return 0;
    v6 = (*v25__f);
    v3 = c;
  }
  v20 = *(_DWORD *)&GAP(c, 0x754, v18 + 192);
  switch ( v20 )
  {
    case 15728640:
      goto LABEL_42;
    case 15728641:
      if ( F16(v9, 1126) != 0 || W_party_find((intptr_t)(c), (intptr_t)(50013), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
        goto LABEL_44;
      goto LABEL_42;
    case 15728642:
      goto LABEL_39;
    case 15728643:
      if ( F32(v9, 212) != 1 )
        goto LABEL_44;
LABEL_39:
      if ( F16(v9, 1126) != 0 )
        goto LABEL_42;
      goto LABEL_44;
    default:
      if ( v20 != F32(v9, 212) )
        goto LABEL_44;
LABEL_42:
      switch ( F32(v19, 4 * 190) )
      {
        case 1:
        case 2:
        case 3:
          break;
        case 0xFF:
          if ( F32(c, 0x1f0) == 0 )
            goto LABEL_44;
          break;
        default:
          goto LABEL_44;
      }
      v21 = F32(c, 0x750);
      switch ( F32(v21, 320) )
      {
        case 1:
          v22 = 0;
          break;
        case 2:
          v22 = 1;
          break;
        case 3:
          v22 = 2;
          break;
        default:
          v22 = a3;
          break;
      }
      F16(v21 + 2 * v22, 312) = (*v25__f);
      result = 1;
      break;
  }
  return result;
}

/* 0x47a7f0: int __thiscall sub_47A7F0(cls_47A7F0 *this, int a2, int a3, int a4) */
int32_t el_47a7f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    uint16_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    uint16_t v12 = 0;
    uint16_t v13 = 0;

  if ( W_lo_479960((intptr_t)(F32(c, 0x158a4)), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0xC8u), (intptr_t)(1), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1)) < 0 )
    return 0;
  v5 = F32(c, 0x750);
  if ( F32(v5, 320) == 1 )
  {
    v6 = 0;
  }
  else if ( F32(v5, 320) == 2 )
  {
    v6 = 1;
  }
  else
  {
    v6 = F32(v5, 320) == 3 ? 2 : a4;
  }
  if ( F16(v5 + 2 * v6, 312) == a4 )
    return 0;
  v7 = 0;
  v13 = F16(v5, 324);
  if ( v13 == 0 )
    return 0;
  while ( 1 )
  {
    if ( F32(v5 + 12 * v7, 332) == F32(v5, 320) && F16((v5 + 4 * (3 * v7 + 84)), 0) == (uint16_t)a4 )
    {
      v8 = 0;
      v9 = 10101;
      while ( v9 != F32(v5 + 12 * v7, 328) )
      {
        v9 = ((int32_t *)D(0x6826c0))[192 * ++v8];
        if ( v9 == 0 )
          goto LABEL_19;
      }
      v10 = &((int32_t *)D(0x6826c0))[192 * v8];
      if ( v10 != 0 && v10[42] != 0 )
        break;
    }
LABEL_19:
    if ( ++v7 >= v13 )
      return 0;
  }
  if ( v5 + 12 * v7 == -328 )
    return 0;
  switch ( F32(v5, 320) )
  {
    case 1:
      v12 = 0;
      break;
    case 2:
      v12 = 1;
      break;
    case 3:
      v12 = 2;
      break;
    default:
      v12 = a4;
      break;
  }
  F16(v5 + 2 * v12, 312) = a4;
  return 1;
}

/* 0x49d330: void __stdcall sub_49D330(cls_4022A0 *a1, int a2) */
void el_49d330(int32_t a1, int32_t a2)
{

    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint32_t v6 = 0;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    intptr_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint32_t v13 = 0;
    uint32_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  if ( a1 == 0 )
    return;
  v3 = F32(a1, 0xc);
  v4 = a2;
  v5 = F32(a1, 0x40);
  v21 = v5;
  if ( a2 == v3 )
    return;
  v6 = F32(a1, 0x1d4);
  if ( v6 > 0x11300 )
  {
    v13 = v6 - 70450;
    if ( v13 != 0 )
    {
      v14 = v13 - 370;
      if ( v14 != 0 )
      {
        if ( v14 == 30 && a2 == 1 )
          W_party_gather((intptr_t)((_DWORD *)v5));
      }
      else if ( a2 != 0 )
      {
        if ( a2 == 1 )
          W_entity_set_collide((intptr_t)(a1), (intptr_t)(0), (intptr_t)(0));
      }
      else
      {
        W_entity_set_collide((intptr_t)(a1), (intptr_t)(3), (intptr_t)(3));
      }
    }
    else if ( a2 == 1 )
    {
      v15 = rnd();
      W_sfx_play_at((intptr_t)(a1), (intptr_t)(0), (intptr_t)(241), (intptr_t)(0), (intptr_t)(-2000 - 2000 * v15 / 0x8000), (intptr_t)(0));
      W_entity_dust((intptr_t)(a1), (intptr_t)(v5), (intptr_t)(0), (intptr_t)(20), (intptr_t)(0), (intptr_t)(0));
    }
  }
  else
  {
    if ( v6 != 70400 )
    {
      switch ( v6 )
      {
        case 0x1124Cu:
          if ( a2 == 1 )
          {
            switch ( F32(a1, 0x2cc) )
            {
              case 'Z':
                el_40ac90(a1, v5, 0, 100019, 3, 800, 0);
                break;
              case '[':
                F32(a1, 0x29c) = 1;
                break;
              case '\\':
                W_warp_request((intptr_t)(a1), (intptr_t)(2));
                break;
              case ']':
                W_actor_shot((intptr_t)(a1), (intptr_t)(v5), (intptr_t)(0), (intptr_t)(1200));
                F16(v5, 72) = 147;
                break;
              case '^':
                el_40ac90(a1, v5, 0, 100041, 7, 1000, F32(F32(a1, 0x2e0), 4));
                break;
              default:
                goto LABEL_34;
            }
          }
          goto LABEL_34;
        case 0x11256u:
          if ( F32(a1, 0x28c) != 1 || v3 != 1 )
            goto LABEL_34;
          break;
        case 0x1126Bu:
        case 0x1126Cu:
        case 0x112E3u:
          W_switch_glow((intptr_t)(a1), (intptr_t)(a2), (intptr_t)(1));
          goto LABEL_34;
        default:
          goto LABEL_34;
      }
      return;
    }
    if ( a2 == 1 )
    {
      v7 = rnd();
      v8 = (_DWORD *)F32(a1, 0x40);
      v9 = 1000 - 3000 * v7 / 0x8000;
      v10 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(1836)), (intptr_t)(0));
      if ( v10 != 0 )
      {
        v11 = *v10;
        if ( W_sfx_place((intptr_t)(a1), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v8), (intptr_t)(&(*v19__f)), (intptr_t)(&(*v20__f))) != 0 )
        {
          g_eng.dsb_volume_5bb870(v11, (*v19__f));
          g_eng.dsb_pan_5bb880(v11, (*v20__f));
          g_eng.dsb_freq_5bb890(v11, v9);
          g_eng.dsb_play_5bb8c0(v11);
        }
        v4 = 1;
      }
      W_entity_dust((intptr_t)(a1), (intptr_t)((int)v21), (intptr_t)(0), (intptr_t)(20), (intptr_t)(0), (intptr_t)(0));
      F32(a1, 0x284) = 1;
      W_actor_unlist_all((intptr_t)(a1 /*edx*/));
      v5 = (int)v21;
      F32(a1, 0x290) = 0;
    }
  }
LABEL_34:
  v16 = F32(a1, 0x268);
  v17 = 0;
  F32(a1, 0xc) = v4;
  if ( v16 != 0 )
  {
    while ( 1 )
    {
      v18 = (int)(&F32(a1, 0x208) + 3 * v17);
      if ( F32(v18, 0) == v4 )
        break;
      if ( ++v17 >= v16 )
        return;
    }
    if ( *(&F32(a1, 0x20c) + 3 * v17) != 0 )
      W_body_set_clip((intptr_t)(v5), (intptr_t)(*(&F32(a1, 0x20c) + 3 * v17)));
    SETLO(v18, F16(v18, 8));
    if ( (uint16_t)v18 != 0 )
      W_sfx_play_at((intptr_t)(a1), (intptr_t)(0), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  }
}

/* 0x49eea0: int __thiscall sub_49EEA0(cls_401D40 *this, int a2, int a3) */
int32_t el_49eea0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    int32_t * v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    uint16_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    uint16_t v27 = 0;
    int32_t * v28 = 0;
    intptr_t v29 = 0;
    uint16_t * v30 = 0;
    uint16_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    uint16_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t * v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    _Alignas(8) uint8_t frame_[0x254 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x54);   /* [512] at ebp-0x200 */
    char * *const v45__f = (char * *)(frame_ + 0x0);   /* ebp-0x254 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x240 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x240 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x224 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x220 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x21c */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x218 */
    int32_t * *const v52__f = (int32_t * *)(frame_ + 0x40);   /* ebp-0x214 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x210 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x20c */
    int32_t * *const v55__f = (int32_t * *)(frame_ + 0x4c);   /* ebp-0x208 */
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x204 */

  v4 = 0;
  v5 = W_sc_402f40((intptr_t)(c)) - 100000101;
  if ( v5 == 0 )
  {
    v10 = 110010;
    v11 = 0;
    while ( F32(a3, 44) != v10 )
    {
      v10 = ((int32_t *)D(0x6c0cd0))[184 * ++v11];
      if ( v10 == 0 )
        return 0;
    }
    v13 = &((int32_t *)D(0x6c0cd0))[184 * v11];
    (*v55__f) = v13;
    if ( v13 == 0 )
      return v4;
    W_script_begin((intptr_t)(c));
    v14 = W_script_leader((intptr_t)(c));
    F32(c, 0x4c) = 0;
    F32(c, 0x50) = 0;
    F32(c, 0x54) = 0;
    F32(c, 0x5c) = v14;
    F32(c, 0x58) = 800;
    F32(c, 0x20) = 3;
    W_script_party_cast((intptr_t)(c));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    if ( W_scene_party_stance((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x8))) != 0 )
    {
      v4 = W_script_wait((intptr_t)(c));
      if ( v4 == 6 )
        return v4;
    }
    W_scene_talk_face((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x4)), (intptr_t)(0), (intptr_t)(0));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    F32(c, 0x5c) = W_script_leader((intptr_t)(c));
    F32(c, 0x4c) = 0;
    F32(c, 0x50) = 0;
    F32(c, 0x54) = 0;
    F32(c, 0x58) = 800;
    F32(c, 0x20) = 3;
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    v15 = W_actor_by_handle((intptr_t)(100000101));
    W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v15), (intptr_t)(((char *)D(0x855c1c))), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    g_eng.h5bf3ee(
      Buffer,
      "Let's see, it costs %s and the best prize is a %s...",
      (const char *)(a3 + 4),
      (uint8_t *)v13 + 4);
    v16 = W_actor_by_handle((intptr_t)(100000101));
    W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v16), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    v17 = W_actor_by_handle((intptr_t)(100000101));
    W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v17), (intptr_t)(((char *)D(0x855bd0))), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    while ( 2 )
    {
      v18 = W_scene_yes_no((intptr_t)(c));
      if ( v18 == 6 )
        return 6;
      *(_DWORD *)&GAP(c, 0x300, 636) = 20;
      F32(c, 0x20) = 6;
      v4 = W_script_wait((intptr_t)(c));
      if ( v4 == 6 )
        return v4;
      if ( v18 != 14 )
      {
LABEL_92:
        W_script_end((intptr_t)(c));
        return v4;
      }
      F32(c, 0x2e4) = 1;
LABEL_33:
      v19 = 0;
      do
      {
        v20 = 10000 * rnd() / 0x8000 % 100;
        v21 = 0;
        while ( F32(a3 + 8 * v21, 48) > v20 )
        {
          if ( ++v21 >= 0x10u )
            goto LABEL_43;
        }
        v22 = 0;
        v23 = F32(a3 + 8 * v21, 44);
        v24 = 110010;
        while ( v23 != v24 )
        {
          v24 = ((int32_t *)D(0x6c0cd0))[184 * ++v22];
          if ( v24 == 0 )
            goto LABEL_33;
        }
        v19 = &((int32_t *)D(0x6c0cd0))[184 * v22];
LABEL_43:
        ;
      }
      while ( v19 == 0 );
      v25 = F32(c, 0x8);
      if ( F16(v25, 0x4010) >= 0x200u )
        goto LABEL_88;
      v26 = 110010;
      v27 = 0;
      while ( *v19 != v26 )
      {
        v26 = ((int32_t *)D(0x6c0cd0))[184 * ++v27];
        if ( v26 == 0 )
          goto LABEL_88;
      }
      v28 = &((int32_t *)D(0x6c0cd0))[184 * v27];
      if ( v28 != 0 && (v28[171] >= 900 || W_room_items_of_kind((intptr_t)(v25), (intptr_t)(1), (intptr_t)(1)) < 48) )
      {
        if ( F32(a3, 0) <= F32(F32(c, 0x8), 16404) )
        {
          if ( W_sc_406500((intptr_t)(c), (intptr_t)(1), (intptr_t)(F32(a3, 0))) != 6 )
          {
            if ( W_actor_by_handle((intptr_t)(F32(F32(c, 0x4), 472))) != 0 )
            {
              v29 = W_actor_by_handle((intptr_t)(F32(F32(c, 0x4), 472)));
              v30 = SYS32(1008);
              (*v49__f) = v29;
              (*v52__f) = F32(v29, 64);
              if ( F32(SYS32(1008), 8) != 0
                && (F32(SYS32(1008), 4) != 0
                 || W_vpool_load((intptr_t)(SYS32(1008))) != 0) )
              {
                v31 = 0;
                (*v48__f) = 0;
                if ( *v30 != 0 )
                {
                  v32 = F32(v30, 4 * 1);
                  v33 = *v30;
                  v34 = v30[1];
                  (*v56__f) = v33;
                  (*v54__f) = v32;
                  while ( 1 )
                  {
                    v35 = (v34 + v31) % v33;
                    v36 = (*v54__f);
                    if ( F32((*v54__f) + 8 * (uint16_t)v35, 4) == 0 )
                      break;
                    v31 = ++(*v48__f);
                    if ( (uint16_t)(*v48__f) >= *v30 )
                      goto LABEL_67;
                    v33 = (*v56__f);
                  }
                  v37 = v35 + 1;
                  v38 = 8 * (uint16_t)v35;
                  v30[1] = v37;
                  F32(v38 + v36, 4) = 0;
                  v39 = (intptr_t *)(F32(v30, 4 * 1) + v38);
                  if ( v39 != 0 )
                  {
                    v40 = *v39;
                    if ( W_sfx_place((intptr_t)((*v49__f)), (intptr_t)(F32(CFG(), 360)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v52__f)), (intptr_t)(&(*v50__f)), (intptr_t)(&(*v51__f))) != 0 )
                    {
                      g_eng.dsb_volume_5bb870(v40, (*v50__f));
                      g_eng.dsb_pan_5bb880(v40, (*v51__f));
                      g_eng.dsb_freq_5bb890(v40, 0);
                      g_eng.dsb_play_5bb8c0(v40);
                    }
                  }
                }
              }
            }
LABEL_67:
            *(_DWORD *)&GAP(c, 0x300, 636) = 220;
            F32(c, 0x20) = 6;
            v4 = W_script_wait((intptr_t)(c));
            if ( v4 == 6 )
              return v4;
            W_room_item_add((intptr_t)(F32(c, 0x8)), (intptr_t)(*v19), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            if ( a2 != 0 )
              (*v53__f) = W_pickup_spawn((intptr_t)(100163), (intptr_t)(F32(a2, 64)), (intptr_t)(*v19));
            v41 = 0;
            do
            {
              *(_DWORD *)&GAP(c, 0x300, 636) = 10;
              F32(c, 0x20) = 6;
              v4 = W_script_wait((intptr_t)(c));
              if ( v4 == 6 )
                return v4;
              if ( (*v53__f) != 0 )
                el_401df0(c, (*v53__f), 0);
              ++v41;
            }
            while ( (uint16_t)v41 < 8u );
            v4 = W_script_wait((intptr_t)(c));
            if ( v4 == 6 )
              return v4;
            if ( *(*v55__f) == *v19 )
            {
              g_eng.h5bf3ee(
                Buffer,
                "Yay! I won a %s, that's the best thing in here!",
                (uint8_t *)v19 + 4);
              (*v47__f) = 13;
            }
            else
            {
              if ( v19[24] < F32(a3, 0) )
              {
                g_eng.h5bf3ee(Buffer, "Aww, all I won was a %s...", (uint8_t *)v19 + 4);
                v43 = W_actor_by_handle((intptr_t)(100000101));
                W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v43), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(11), (intptr_t)(0), (intptr_t)(0));
                v4 = W_script_wait((intptr_t)(c));
                if ( v4 == 6 )
                  return v4;
LABEL_83:
                if ( a2 != 0 )
                {
                  el_401df0(c, a2, 0);
                  v4 = W_script_wait((intptr_t)(c));
                  if ( v4 == 6 )
                    return v4;
                }
                v44 = W_actor_by_handle((intptr_t)(100000101));
                W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v44), (intptr_t)(((char *)D(0x855b58))), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0));
                v4 = W_script_wait((intptr_t)(c));
                if ( v4 == 6 )
                  return v4;
                continue;
              }
              g_eng.h5bf3ee(Buffer, "Yay, I won a %s!", (uint8_t *)v19 + 4);
              (*v47__f) = 3;
            }
            v42 = W_actor_by_handle((intptr_t)(100000101));
            W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v42), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)((*v47__f)), (intptr_t)(0), (intptr_t)(0));
            v4 = W_script_wait((intptr_t)(c));
            if ( v4 == 6 )
              return v4;
            goto LABEL_83;
          }
          return 6;
        }
        (*v46__f) = 18;
        (*v45__f) = ((char *)D(0x855b04));
      }
      else
      {
LABEL_88:
        (*v46__f) = 18;
        (*v45__f) = ((char *)D(0x855b2c));
      }
      break;
    }
    v8 = W_actor_by_handle((intptr_t)(100000101));
    goto LABEL_91;
  }
  v6 = v5 - 1;
  if ( v6 != 0 )
  {
    if ( v6 != 1 )
      return v4;
    W_script_begin((intptr_t)(c));
    v7 = W_script_leader((intptr_t)(c));
    F32(c, 0x4c) = 0;
    F32(c, 0x50) = 0;
    F32(c, 0x54) = 0;
    F32(c, 0x5c) = v7;
    F32(c, 0x58) = 800;
    F32(c, 0x20) = 3;
    W_script_party_cast((intptr_t)(c));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    if ( W_scene_party_stance((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x8))) != 0 )
    {
      v4 = W_script_wait((intptr_t)(c));
      if ( v4 == 6 )
        return v4;
    }
    W_scene_talk_face((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x4)), (intptr_t)(0), (intptr_t)(0));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    F32(c, 0x5c) = W_script_leader((intptr_t)(c));
    F32(c, 0x4c) = 0;
    F32(c, 0x50) = 0;
    F32(c, 0x54) = 0;
    F32(c, 0x58) = 800;
    F32(c, 0x20) = 3;
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    (*v46__f) = 21;
    (*v45__f) = ((char *)D(0x855cc0));
    v8 = W_actor_by_handle((intptr_t)(100000103));
  }
  else
  {
    W_script_begin((intptr_t)(c));
    v9 = W_script_leader((intptr_t)(c));
    F32(c, 0x4c) = 0;
    F32(c, 0x50) = 0;
    F32(c, 0x54) = 0;
    F32(c, 0x5c) = v9;
    F32(c, 0x58) = 800;
    F32(c, 0x20) = 3;
    W_script_party_cast((intptr_t)(c));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    if ( W_scene_party_stance((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x8))) != 0 )
    {
      v4 = W_script_wait((intptr_t)(c));
      if ( v4 == 6 )
        return v4;
    }
    W_scene_talk_face((intptr_t)(c), (intptr_t)(0), (intptr_t)(F32(c, 0x4)), (intptr_t)(0), (intptr_t)(0));
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    F32(c, 0x5c) = W_script_leader((intptr_t)(c));
    F32(c, 0x4c) = 0;
    F32(c, 0x50) = 0;
    F32(c, 0x54) = 0;
    F32(c, 0x58) = 800;
    F32(c, 0x20) = 3;
    v4 = W_script_wait((intptr_t)(c));
    if ( v4 == 6 )
      return v4;
    (*v46__f) = 24;
    (*v45__f) = ((char *)D(0x855c6c));
    v8 = W_actor_by_handle((intptr_t)(100000102));
  }
LABEL_91:
  W_scene_msg((intptr_t)(c), (intptr_t)(0), (intptr_t)(v8), (intptr_t)((*v45__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)((*v46__f)), (intptr_t)(0), (intptr_t)(0));
  v4 = W_script_wait((intptr_t)(c));
  if ( v4 != 6 )
    goto LABEL_92;
  return v4;
}

/* 0x4c5af0: int __thiscall sub_4C5AF0(cls_4C5AF0 *this, int a2, int a3, int a4) */
int32_t el_4c5af0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    uint32_t v4 = 0;
    uint32_t v5 = 0;
    intptr_t v6 = 0;
    char * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    int v11 = 0;
    char * v12 = 0;
    uint32_t LastError = 0;
    uint32_t v14 = 0;
    intptr_t * v15 = 0;
    uint32_t v16 = 0;
    char * v17 = 0;
    char * v18 = 0;
    char * v19 = 0;
    char v20 = 0;
    uint32_t v21 = 0;
    intptr_t * v22 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x210 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x10);   /* [256] at ebp-0x200 */
    char *Buffer = (char *)(frame_ + 0x110);   /* [256] at ebp-0x100 */
    intptr_t *const SystemTime__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x210 */

  v4 = F32(c, 0x1800);
  v5 = 0;
  if ( v4 == 0 )
    goto LABEL_7;
  v6 = c;
  while ( F32(((uint8_t *)(v6) + 0x0), 0) != a2 )
  {
    ++v5;
    v6 = ((uint8_t *)v6 + 12);
    if ( v5 >= v4 )
      goto LABEL_7;
  }
  v7 = &GAP(c, 0x0, 12 * v5);
  if ( v7 == 0 )
  {
LABEL_7:
    v7 = 0;
    v8 = 0;
    if ( v4 == 0 )
      goto LABEL_13;
    v9 = 0;
    do
    {
      v10 = 3 * v9;
      v11 = *(_DWORD *)&GAP(c, 0x0, 4 * v10) == 0;
      v12 = &GAP(c, 0x0, 4 * v10);
      if ( v11 )
        v7 = v12;
      v9 = (uint16_t)++v8;
    }
    while ( (uint16_t)v8 < v4 );
    if ( v7 == 0 )
    {
LABEL_13:
      if ( v4 == 512 )
      {
        LastError = API_GetLastError();
        v14 = LastError;
        if ( LastError != 0 )
        {
          API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
          g_eng.h5bf3ee(OutputString, "[%08d] %s", v14, Buffer);
          g_eng.log_fail_406440(g_eng.log, (const char *)(OutputString), (const char *)(D(0x8a9b6c)));
        }
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v16 = strlen(((char *)D(0x855040))) + 1;
          v17 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v17, ((char *)D(0x855040)), 4 * (v16 >> 2));
          v19 = ((char *)D(0x855040) + (4 * (v16 >> 2)));
          v18 = &v17[4 * (v16 >> 2)];
          v20 = v16;
        }
        else
        {
          v21 = strlen(((char *)D(0x855040))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x855040)), 4 * (v21 >> 2));
          v19 = ((char *)D(0x855040) + (4 * (v21 >> 2)));
          v18 = &((char *)D(0x8a9530) + 0x404)[4 * (v21 >> 2) - 1024];
          v20 = v21;
        }
        qmemcpy(v18, v19, v20 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440(g_eng.log, (const char *)(((char *)D(0x855040))), (const char *)(D(0x8a9b6c)));
        g_eng.h5bf5db(0);
      }
      F32(c, 0x1800) = v4 + 1;
      v7 = &GAP(c, 0x0, 12 * v4);
    }
    F32(v7, 0) = a2;
    F32(v7, 4 * 2) = 0;
    F32(v7, 4 * 1) = 0;
  }
  result = a4;
  if ( a4 != 0 || a3 != 0 )
  {
    F32(v7, 4 * 1) = a3;
    F32(v7, 4 * 2) = a4;
  }
  else
  {
    F32(v7, 0) = 0;
  }
  return result;
}

/* 0x4cc0e0: void __thiscall sub_4CC0E0(int this, int a2) */
void el_4cc0e0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    char * v6 = 0;
    intptr_t * v7 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const SystemTime__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */

  v3 = F32(c, 10372);
  if ( v3 != 0
    && F32(F32(F32(c, 800), 0), 372) != 0
    && (!W_sd_5bbc60((intptr_t)(v3), (intptr_t)(0)) || W_sd_5bbc90((intptr_t)(F32(c, 10372)), (intptr_t)(0)) != 0) )
  {
    F16(c, 10376) = 0;
    if ( a2 != 0 )
      W_sd_5bbcd0((intptr_t)(F32(c, 10372)), (intptr_t)(20));
    v4 = W_sd_5bbc20((intptr_t)(F32(c, 10372)));
    v5 = F32(c, 10372);
    if ( v4 != 0 )
    {
      if ( W_sd_5bcb80((intptr_t)(v5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
        g_eng.log_fail_406440((const char *)D(0x8a9530), (char *)(F32(c, 10372) + 44), D(0x8a9b6c));
    }
    else if ( W_sd_5bcb80((intptr_t)(v5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
    {
      v6 = (const CHAR *)(F32(c, 10372) + 44);
      g_eng.log_fail_406440(g_eng.log, (const char *)(v6), (const char *)(D(0x8a9b6c)));
    }
  }
}

/* 0x54c5e0: void __thiscall sub_54C5E0(cls_54C5E0 *this) */
void el_54c5e0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t i = 0;
    char * v4 = 0;
    char * v5 = 0;

  v2 = c + 8 * (2054);
  for ( i = 8; i != 0; --i )
  {
    if ( F32(((uint8_t *)(v2) + 0x0), 0) != 0 )
    {
      g_eng.op_delete((void *)F32((uint8_t *)(v2), 0x0));
      F32(((uint8_t *)(v2) + 0x0), 0) = 0;
    }
    v2 = ((uint8_t *)v2 + 4);
  }
  v4 = (char *)F32(c, 0x4);
  if ( v4 != 0 )
  {
    v5 = v4 - 4;
    eh_vec_dtor(v4, 0x170u, *((int32_t *)v4 - 1), (void *)((intptr_t)D(0x4017a0) /* input_dtor by address */));
    g_eng.op_delete((void *)v5);
    F32(c, 0x4) = 0;
  }
}

/* 0x54ed80: int sub_54ED80() */
int32_t el_54ed80(void)
{

    intptr_t v0 = 0;
    intptr_t v1 = 0;
    uint16_t v2 = 0;
    char * v3 = 0;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    intptr_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    char * v10 = 0;
    int16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    char * v14 = 0;
    char * v15 = 0;
    _Alignas(8) uint8_t frame_[0x200 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [512] at ebp-0x200 */

  v0 = 0;
  v1 = F32(W, 0x2784);
  v2 = F16(v1, 0x4010);
  if ( v2 != 0 )
  {
    while ( 1 )
    {
      v3 = &GAP(v1, 0x0, 16 * (uint16_t)v0 + 8208);
      v4 = F32(v3, 4 * 3);
      if ( v3[10] == 0
        && F32(v4, 88) == 22
        && F32(*(_DWORD *)&GAP(W, 0x1030, 8), 4) == F32(v4, 104) )
      {
        break;
      }
      if ( (uint16_t)++v0 >= v2 )
        goto LABEL_6;
    }
    v10 = &GAP(v1, 0x0, 16 * (uint16_t)v0 + 8208);
    if ( F16(F32(v10, 4 * 3), 100) == 0 )
      goto LABEL_20;
    v11 = F16(v10, 2 * 4);
    if ( v11 != 0 )
      F16(v10, 2 * 4) = v11 - 1;
    if ( F16(v10, 2 * 4) != 0 )
    {
LABEL_20:
      g_eng.h5bf3ee(Buffer, "You used %s.", (const char *)(v4 + 4));
    }
    else
    {
      W_mh_4199b0((intptr_t)(v1), (intptr_t)(v0), (intptr_t)(1), (intptr_t)(0));
      g_eng.h5bf3ee(
        Buffer,
        "You used %s.%%nThe %s has broken.",
        (const char *)(v4 + 4),
        (const char *)(v4 + 4));
    }
    v12 = 0;
    v13 = F32(W, 0x1024);
    while ( *(_DWORD *)&GAP(W, 0x0, 516 * v13 + 4) != 0 )
    {
      v13 = (v13 + 1) % 8;
      if ( (uint16_t)++v12 >= 8u )
        return 1;
    }
    v14 = &GAP(W, 0x0, 516 * v13);
    F32(v14, 4 * 1) = 1;
    strcpy(v14 + 8, Buffer);
    return 1;
  }
  else
  {
LABEL_6:
    v5 = 0;
    while ( *(_DWORD *)&GAP(W, 0x0, 516 * v5 + 4) == 0 )
    {
      if ( ++v5 >= 8u )
      {
        v6 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(892)), (intptr_t)(0));
        if ( v6 != 0 )
        {
          g_eng.dsb_volume_5bb870(*v6, F32(CFG(), 360));
          g_eng.dsb_freq_5bb890(*v6, 0);
          g_eng.dsb_play_5bb8c0(*v6);
        }
        strcpy(Buffer, ((char *)D(0x8a0acc)));
        v7 = 0;
        v8 = F32(W, 0x1024);
        while ( *(_DWORD *)&GAP(W, 0x0, 516 * v8 + 4) != 0 )
        {
          v8 = (v8 + 1) % 8;
          if ( (uint16_t)++v7 >= 8u )
            return 0;
        }
        v15 = &GAP(W, 0x0, 516 * v8);
        F32(v15, 4 * 1) = 1;
        strcpy(v15 + 8, Buffer);
        return 0;
      }
    }
    return 0;
  }
}

/* 0x5534d0: int __thiscall sub_5534D0(cls_5534D0 *this, int a2, int a3, int a4) */
int32_t el_5534d0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;

  if ( a2 != 0 )
    return 0;
  if ( F32(c, 0x3c) == 0 )
  {
    F32(c, 0x2c4) = 0;
    return 0;
  }
  if ( F32(c, 0x2c4) == 0 )
    F32(c, 0x2c4) = a3;
  if ( F32(c, 0x2c4) == a4 )
    return 1;
  F32(c, 0x3c) = 0;
  return 0;
}

/* 0x556e50: void __fastcall sub_556E50(int a1, int a2, int a3, int a4, int a5) */
void el_556e50(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{

    intptr_t v5 = 0;
    intptr_t i = 0;
    int32_t * v7 = 0;

  if ( a5 == 0 )
  {
    v5 = 5088;
    for ( i = 1024; i != 0; --i )
    {
      v7 = *(_DWORD **)&GAP(W, 0x0, v5);
      if ( v7[116] != 0 && v7[157] == a3 && (v7[117] == a4 || a4 == 0) )
        W_entity_release((intptr_t)((int)v7), (intptr_t)(/*edx*/ 1));
      v5 += 4;
    }
  }
}

/* 0x55fdb0: void __thiscall sub_55FDB0(cls_55FDB0 *this) */
void el_55fdb0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    char * v5 = 0;
    intptr_t v6 = 0;
    intptr_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t * v10 = 0;
    uint16_t v11 = 0;
    uint16_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x200 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [512] at ebp-0x200 */

  if ( F32(c, 0x9f4) != 0 )
  {
    g_eng.h5bf3ee(Buffer, "%s leveled up!", (uint8_t *)c + 2576);
    v2 = W;
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
    strcpy(v5 + 8, Buffer);
    v2 = W;
LABEL_7:
    v6 = *(_DWORD *)&GAP(v2, 0x1030, 20);
    if ( F32(F32((v6 + 288), 0), 32) == 0 )
    {
      v7 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(920)), (intptr_t)(0));
      if ( v7 != 0 )
      {
        g_eng.dsb_volume_5bb870(*v7, F32(CFG(), 360) - 1500);
        g_eng.dsb_freq_5bb890(*v7, 0);
        g_eng.dsb_play_5bb8c0(*v7);
      }
      v8 = F32(v6, 288);
      F32(v8, 0) = 2;
      F32(v8, 32) = 1;
      F16(v8, 4) = 0;
      F16(v8, 6) = 0;
      F32(v8, 8) = 0;
      F32(v8, 28) = 0;
      F32(v8, 20) = 120;
      F32(v8, 24) = 96;
    }
    v9 = F32(c, 0x9f4);
    v10 = F32(v9, 88916);
    if ( v10 != 0
      && F32(v9, 88924) == 0
      && (F32(v9, 88716) != 1 || F32(CFG(), 400) != 0) )
    {
      if ( F32(v9, 51052) <= 0
        || (v10[2] <= 0x43u ? (v11 = 0) : (v11 = F16(F32(v10, 0), 86834)),
            (v12 = F16(v9, 51056)) == 0
         || v12 >= v10[2]
         || v11 >= F16(1296 * F16(v9, 51056) + F32(v10, 0), 2)) )
      {
        F32(v9, 51052) = 10;
        F16(v9, 51056) = 67;
        F32(v9, 51060) = 1;
        F32(v9, 51064) = 1;
      }
    }
  }
}

/* 0x5851a0: void __thiscall sub_5851A0(cls_5851A0 *this) */
void el_5851a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t i = 0;

  v2 = c + 8 * (2054);
  for ( i = 8; i != 0; --i )
  {
    F32(((uint8_t *)(v2) + 0x0), 0) = (intptr_t)g_eng.op_new(0xEECu);
    v2 = ((uint8_t *)v2 + 4);
  }
  F32(c, 0x4) = 0;
}
/* unported callees: none */
