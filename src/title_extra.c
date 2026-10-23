/* title_extra.c — the title's other SCREENS and the ATTRACT demo: the input re-poll 0x40fe00 (reached: every menu),
 * the Options pages 0x569220 (+ 0x568780 0x56a270 0x56a450 0x565b00 0x566300), the load screen's helpers, 0x56a670 /
 * 0x56aa70 / 0x56ad50, the attract demo 0x583fe0 (the actor manifest 0x584710 + 0x585260..0x585e70; 0x585590's IDA
 * ebx argument is a scratch register: only its low word, written first, is read), the demo room 0x59dc60 / 0x59d690 /
 * 0x59d900 / 0x59df50 / 0x59e1d0, the opening 0x59e230 / 0x59e5d0 / 0x59e9e0, the room loop's 0x59d160, 0x410250,
 * 0x54c530, 0x55ff80, 0x5612f0, 0x5640b0, 0x564110, 0x56d620, 0x56dd80.  See title_extra.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  Reached by the corpus: the input
 * re-poll 0x40fe00; rh-early-game (2026-10-23): 0x565b00 0x566300 0x56a670 0x56ad50 0x583fe0 0x584400 0x584420 0x584550
 * 0x59dc60; the rest UNVERIFIED (REPLACE-wired, compile-checked). */
#include "title_extra.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "asset_register.h"
#include "char_def.h"
#include "char_hit.h"
#include "char_integrate.h"
#include "ending.h"
#include "entity_life.h"
#include "event.h"
#include "field_menu.h"
#include "frame_load.h"
#include "input_dev.h"
#include "menu_helpers.h"
#include "party_member.h"
#include "render_draw.h"
#include "render_platform.h"
#include "room_load.h"
#include "room_loop.h"
#include "save_game.h"
#include "scene_loop.h"
#include "script_helpers.h"
#include "script_io.h"
#include "sound_dev.h"
#include "sound_fx.h"
#include "sprite_slot.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "win_app.h"
#include "win_main.h"
#include "world_alloc.h"
#include "zdd_dev.h"
#include "asset_register.h"
#include "char_def.h"
#include "char_hit.h"
#include "char_integrate.h"
#include "ending.h"
#include "entity_life.h"
#include "event.h"
#include "field_menu.h"
#include "frame_load.h"
#include "input_dev.h"
#include "menu_helpers.h"
#include "party_member.h"
#include "render_draw.h"
#include "render_platform.h"
#include "room_load.h"
#include "room_loop.h"
#include "save_game.h"
#include "scene_loop.h"
#include "script_helpers.h"
#include "script_io.h"
#include "sound_dev.h"
#include "sound_fx.h"
#include "sprite_slot.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "win_app.h"
#include "win_main.h"
#include "world_alloc.h"
#include "zdd_dev.h"
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

static inline intptr_t W_actor_release(intptr_t a0, ...) { actor_release((wl_actor *)a0); return 0; }
static inline intptr_t W_area_xref(intptr_t a0, intptr_t a1, intptr_t a2, ...) { area_xref((uint8_t *)a0, (const uint8_t *)a1, (uint8_t *)a2); return 0; }
static inline intptr_t W_bs_5ba390(intptr_t a0, ...) { return (intptr_t)bs_5ba390((wl_char *)a0); }
static inline intptr_t W_cell_color_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)cell_color_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_char_carry(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_carry((wl_char *)a0, (wl_char *)a1); }
static inline intptr_t W_char_part_release(intptr_t a0, intptr_t a1, ...) { char_part_release((wl_char *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_input(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_input((input_mgr *)a0, (uint8_t *)a1, (uint32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_text_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ctrl_text_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_ctrl_text_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_text_set((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_en_410090(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)en_410090((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t *)a3, (int32_t *)a4, (int32_t *)a5, (int32_t *)a6, (int32_t *)a7); }
static inline intptr_t W_en_56c640(intptr_t a0, ...) { en_56c640((wl_char *)a0); return 0; }
static inline intptr_t W_en_56c900(intptr_t a0, intptr_t a1, ...) { return (intptr_t)en_56c900((wl_char *)a0, (char)a1); }
static inline intptr_t W_en_5a45c0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)en_5a45c0((wl_char *)a0, (char)a1); }
static inline intptr_t W_entity_release(intptr_t a0, intptr_t a1, ...) { entity_release((wl_entity *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_entity_unlist(intptr_t a0, ...) { entity_unlist((wl_entity *)a0); return 0; }
static inline intptr_t W_flag_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)flag_find((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_fm_56c430(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fm_56c430((wl_char *)a0, (char)a1); }
static inline intptr_t W_frame_cache_clear(intptr_t a0, ...) { frame_cache_clear((uint8_t *)a0); return 0; }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_col_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)grid_col_place((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_grid_row_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)grid_row_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_grid_row_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)grid_row_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); }
static inline intptr_t W_id_5ba2d0(intptr_t a0, ...) { return (intptr_t)id_5ba2d0((wl_char *)a0); }
static inline intptr_t W_id_5ba3a0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)id_5ba3a0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_id_5ba3d0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)id_5ba3d0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_id_5ba4c0(intptr_t a0, ...) { return (intptr_t)id_5ba4c0((wl_char *)a0); }
static inline intptr_t W_id_5ba650(intptr_t a0, ...) { return (intptr_t)id_5ba650((wl_char *)a0); }
static inline intptr_t W_input_dtor(intptr_t a0, ...) { input_dtor((uint8_t *)a0); return 0; }
static inline intptr_t W_input_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)input_init((wl_room *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_item_def(intptr_t a0, ...) { return (intptr_t)item_def((int32_t)a0); }
static inline intptr_t W_key_token_replace(intptr_t a0, intptr_t a1, ...) { return (intptr_t)key_token_replace((char *)a0, (const uint8_t *)a1); }
static inline intptr_t W_load_room(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, ...) { return (intptr_t)load_room((wl_roomloop *)a0, (wl_room *)a1, (uint16_t)a2, (uint8_t *)a3, (uint8_t *)a4, (int32_t)a5, (int32_t)a6, (const uint8_t *)a7, (const uint8_t *)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (uint8_t *)a13, (int32_t)a14, (int32_t)a15); }
static inline intptr_t W_menu_pace(intptr_t a0, ...) { return (intptr_t)menu_pace((uint8_t *)a0); }
static inline intptr_t W_mh_4e61d0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)mh_4e61d0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_mh_4e6520(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)mh_4e6520((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_pool_head_delete(intptr_t a0, intptr_t a1, ...) { return (intptr_t)pool_head_delete((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_pslot_by_id(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_by_id((wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_pslot_heal(intptr_t a0, intptr_t a1, ...) { pslot_heal((wl_pslot *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_pslot_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { pslot_init((wl_pslot *)a0, (wl_room *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); return 0; }
static inline intptr_t W_pslot_input_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)pslot_input_set((wl_pslot *)a0, (void *)a1); }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_pslot_reset(intptr_t a0, ...) { pslot_reset((wl_pslot *)a0); return 0; }
static inline intptr_t W_rd_48c820(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_48c820((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rd_491820(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)rd_491820((wl_char *)a0, (int32_t)a1, (char *)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_room_area_enter(intptr_t a0, intptr_t a1, intptr_t a2, ...) { room_area_enter((int32_t *)a0, (int32_t)a1, (wl_room *)a2); return 0; }
static inline intptr_t W_room_delete(intptr_t a0, intptr_t a1, ...) { room_delete((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_flag_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_flag_set((wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_formation(intptr_t a0, ...) { return (intptr_t)room_formation((wl_room *)a0); }
static inline intptr_t W_room_from_scene(intptr_t a0, intptr_t a1, ...) { room_from_scene((wl_room *)a0, (const uint8_t *)a1); return 0; }
static inline intptr_t W_room_inputs_alloc(intptr_t a0, intptr_t a1, ...) { room_inputs_alloc((wl_room *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_room_item_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)room_item_add((wl_room *)a0, (int32_t)a1, (uint16_t *)a2, (int32_t)a3, (uint8_t)a4, (int32_t)a5); }
static inline intptr_t W_room_item_equip(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { room_item_equip((wl_room *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_room_items_grant(intptr_t a0, intptr_t a1, ...) { room_items_grant((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_items_of_kind(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_items_of_kind((const wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_leader_set(intptr_t a0, intptr_t a1, ...) { room_leader_set((wl_room *)a0, (wl_pslot *)a1); return 0; }
static inline intptr_t W_room_member_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)room_member_add((wl_room *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_room_next_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { room_next_set((wl_room *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_room_party_defaults(intptr_t a0, ...) { room_party_defaults((wl_room *)a0); return 0; }
static inline intptr_t W_room_party_heal(intptr_t a0, intptr_t a1, ...) { room_party_heal((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_party_level(intptr_t a0, intptr_t a1, ...) { room_party_level((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_stats_refresh(intptr_t a0, intptr_t a1, ...) { room_stats_refresh((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_story_setup(intptr_t a0, ...) { room_story_setup((int32_t *)a0); return 0; }
static inline intptr_t W_room_story_setup2(intptr_t a0, ...) { room_story_setup2((int32_t *)a0); return 0; }
static inline intptr_t W_roomloop_free(intptr_t a0, ...) { roomloop_free((wl_roomloop *)a0); return 0; }
static inline intptr_t W_rp_5b94e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b94e0((wl_char *)a0, (intptr_t *)a1); }
static inline intptr_t W_rp_5b9500(intptr_t a0, intptr_t a1, ...) { rp_5b9500((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sc_4c5300(intptr_t a0, ...) { sc_4c5300((int32_t)a0); return 0; }
static inline intptr_t W_sc_4cc470(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sc_4cc470((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_scene_obj_init(intptr_t a0, ...) { return (intptr_t)scene_obj_init((uint8_t *)a0); }
static inline intptr_t W_sd_5bb3a0(intptr_t a0, ...) { return (intptr_t)sd_5bb3a0((wl_char *)a0); }
static inline intptr_t W_sd_5bc0c0(intptr_t a0, ...) { sd_5bc0c0((wl_char *)a0); return 0; }
static inline intptr_t W_session_end(intptr_t a0, ...) { session_end((uint8_t *)a0); return 0; }
static inline intptr_t W_sfx_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sfx_play((void *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_si_40a5d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { si_40a5d0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_slot_frames_free(intptr_t a0, intptr_t a1, ...) { slot_frames_free((ar_sprite_slot *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_sv_41b670(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sv_41b670((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t *)a3); }
static inline intptr_t W_sv_41bf60(intptr_t a0, intptr_t a1, ...) { sv_41bf60((int32_t *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sv_560530(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sv_560530((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_sv_561cf0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sv_561cf0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_sys_cursor_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_cursor_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_frame_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_frame_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_list_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)sys_list_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (uint16_t)a7, (uint16_t)a8, (uint8_t **)a9, (uint8_t **)a10, (const char *)a11, (int32_t)a12); }
static inline intptr_t W_sys_msg_keys(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_keys((void *)a0, (int32_t)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_msg_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_page_arrows(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_page_arrows((void *)a0, (const uint8_t *)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_unload(intptr_t a0, intptr_t a1, ...) { sys_unload((void *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_sys_win_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)sys_win_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const char *)a7, (int32_t)a8); }
static inline intptr_t W_text_layout_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)text_layout_set((uint8_t *)a0, (const char *)a1, (uint16_t)a2); }
static inline intptr_t W_text_metrics_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)text_metrics_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_tm_419900(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_419900((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_41bbe0(intptr_t a0, ...) { tm_41bbe0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_41c2f0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c2f0((wl_char *)a0, (int32_t *)a1, (uint16_t *)a2); }
static inline intptr_t W_tm_41c320(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c320((wl_char *)a0, (int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_tm_49a6e0(intptr_t a0, ...) { tm_49a6e0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_560a00(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_560a00((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_tm_560a40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)tm_560a40((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_tm_562a10(intptr_t a0, ...) { tm_562a10((wl_char *)a0); return 0; }
static inline intptr_t W_tm_5642e0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_5642e0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_5644a0(intptr_t a0, ...) { return (intptr_t)tm_5644a0((wl_char *)a0); }
static inline intptr_t W_tm_564690(intptr_t a0, ...) { return (intptr_t)tm_564690((wl_char *)a0); }
static inline intptr_t W_tm_564780(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_564780((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_tm_5658c0(intptr_t a0, ...) { return (intptr_t)tm_5658c0((wl_char *)a0); }
static inline intptr_t W_tm_565d10(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)tm_565d10((wl_char *)a0, (int32_t *)a1, (int16_t)a2, (int32_t *)a3, (int32_t)a4, (uint32_t)a5); }
static inline intptr_t W_tm_566250(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_566250((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_tm_566350(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_566350((wl_char *)a0, (int32_t *)a1); }
static inline intptr_t W_tm_566a80(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_566a80((int16_t)a0, (int32_t)a1, (char *)a2); }
static inline intptr_t W_tm_56a220(intptr_t a0, ...) { return (intptr_t)tm_56a220((wl_char *)a0); }
static inline intptr_t W_tm_56c2b0(intptr_t a0, ...) { tm_56c2b0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_56d710(intptr_t a0, ...) { return (intptr_t)tm_56d710((wl_char *)a0); }
static inline intptr_t W_tm_56da40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)tm_56da40((wl_char *)a0, (char *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_tm_56daa0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)tm_56daa0((wl_char *)a0, (int32_t)a1, (int16_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (int32_t)a6, (int32_t)a7, (int16_t *)a8, (uint16_t)a9, (int16_t)a10); }
static inline intptr_t W_tm_56dc20(intptr_t a0, intptr_t a1, intptr_t a2, ...) { tm_56dc20((int32_t)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_ui_ref_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)ui_ref_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_voice_set_free(intptr_t a0, ...) { voice_set_free((wl_voiceset *)a0); return 0; }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_ctrl_to_back(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_ctrl_to_back((uint8_t *)a0, (uint16_t)a1); }
static inline intptr_t W_win_grid_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_grid_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_grid_new(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_grid_new((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_win_mgr_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_mgr_open((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (const void *)a6); }
static inline intptr_t W_win_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_win_title_add(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_title_add((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_wl_list_remove(intptr_t a0, intptr_t a1, ...) { wl_list_remove((wl_list *)a0, (const wl_entity *)a1); return 0; }
static inline intptr_t W_wm_5b1030(void) { return (intptr_t)wm_5b1030(); }
static inline intptr_t W_wn_562b50(intptr_t a0, intptr_t a1, ...) { return (intptr_t)wn_562b50((wl_char *)a0, (char)a1); }
static inline intptr_t W_world_free(intptr_t a0, ...) { world_free((wl_world *)a0); return 0; }
static inline intptr_t W_zd_5b9410(intptr_t a0, ...) { return (intptr_t)zd_5b9410((wl_char *)a0); }
typedef intptr_t (__attribute__((thiscall)) *rt_43c110)(intptr_t, intptr_t, intptr_t);   /* 0x43c110 thiscall: unported, retail's */
#define RC_43c110 ((rt_43c110)g_eng.retail(0x43c110u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b8fc0)(intptr_t, intptr_t);   /* 0x5b8fc0 thiscall: unported, retail's */
#define RC_5b8fc0 ((rt_5b8fc0)g_eng.retail(0x5b8fc0u))
typedef uint32_t (__attribute__((stdcall)) *api_GetTickCount_t)(void);
#define API_GetTickCount ((api_GetTickCount_t)g_eng.api("GetTickCount"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
typedef intptr_t (__attribute__((stdcall)) *api_SelectObject_t)(intptr_t, intptr_t);
#define API_SelectObject ((api_SelectObject_t)g_eng.api("SelectObject"))
typedef intptr_t (__attribute__((stdcall)) *api_SetBkMode_t)(intptr_t, intptr_t);
#define API_SetBkMode ((api_SetBkMode_t)g_eng.api("SetBkMode"))
typedef uint32_t (__attribute__((stdcall)) *api_SetTextColor_t)(intptr_t, intptr_t);
#define API_SetTextColor ((api_SetTextColor_t)g_eng.api("SetTextColor"))
typedef intptr_t (__attribute__((stdcall)) *api_TextOutA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_TextOutA ((api_TextOutA_t)g_eng.api("TextOutA"))
int32_t ts_40fe00(wl_char *self);
int32_t ts_410250(wl_char *self, int32_t * a2, int32_t a3, int32_t a4);
int32_t ts_54c530(wl_char *self, char a2);
int32_t ts_55ff80(wl_char *self, int32_t a2, int32_t a3);
int32_t ts_5612f0(wl_char *self);
int32_t ts_5640b0(wl_char *self);
int32_t ts_564110(wl_char *self, int32_t a2, uint16_t a3);
int32_t ts_565b00(wl_char *self, char * a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7);
int32_t ts_566300(wl_char *self, int32_t a2);
int32_t ts_568780(int32_t a1);
int32_t ts_569220(wl_char *self, int32_t a2);
int32_t ts_56a270(wl_char *self, char * a2, int32_t a3);
int32_t ts_56a450(int32_t a1);
int32_t ts_56a670(wl_char *self, int32_t ** a2, uint16_t * a3, int32_t * a4);
int32_t ts_56aa70(wl_char *self, int32_t * a2, int32_t * a3);
void ts_56ad50(wl_char *self);
int32_t ts_56d620(wl_char *self);
int32_t ts_56dd80(int32_t a1, int32_t a2);
int32_t ts_583fe0(wl_char *self);
int32_t ts_584400(wl_char *self);
int32_t ts_584420(wl_char *self, int32_t a2, int32_t a3);
int32_t ts_584550(wl_char *self, int32_t a2, int32_t a3);
int32_t ts_5846f0(wl_char *self);
int32_t ts_584710(wl_char *self);
int32_t ts_585260(wl_char *self, int32_t a2, int32_t a3, int16_t a4, int32_t a5);
int32_t ts_585400(int32_t a1, int32_t a2);
int32_t ts_585530(int32_t * a1);
int32_t ts_585590(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t ts_585810(int32_t a1, uint16_t a2, int32_t a3, uint16_t a4, uint16_t a5, uint16_t a6, int32_t a7);
int32_t ts_585ae0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t ts_585e70(wl_char *self);
void ts_59d160(void);
void ts_59d690(wl_char *self);
int32_t ts_59d900(wl_char *self, char a2);
int32_t ts_59dc60(wl_char *self);
int32_t ts_59df50(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4);
int32_t ts_59e1d0(wl_char *self);
int32_t ts_59e230(wl_char *self);
int32_t ts_59e5d0(wl_char *self, int16_t a2);
int32_t ts_59e9e0(wl_char *self, uint32_t a2);

/* 0x40fe00: int __thiscall sub_40FE00(input_dev **this) */
int32_t ts_40fe00(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t * v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    uint32_t v11 = 0;
    int v12 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t * *const v15__f = (intptr_t * *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  W_id_5ba4c0((intptr_t)(*(((intptr_t *)c) + 2590)));
  W_id_5ba2d0((intptr_t)(*(((intptr_t *)c) + 2590)));
  v2 = c + 4 * (2591);
  (*v16__f) = 2;
  (*v15__f) = c + 4 * (2591);
  do
  {
    if ( *v2 != 0 )
    {
      W_id_5ba650((intptr_t)(*v2));
      v3 = *v2;
      v4 = F32(CFG(), 1340);
      if ( W_id_5ba2d0((intptr_t)(*v2)) )
      {
        v5 = F32(v3, 0x10);
        v6 = 0;
        (*v14__f) = 0;
        if ( v5 != 0 )
        {
          do
          {
            v7 = (int *)(F32(v3, 0x14) + 16 * v6);
            v8 = *v7;
            switch ( *v7 )
            {
              case 8:
                v10 = (int *)&GAP((v3 + (4) * 0x18), 0x0, 8);
                goto LABEL_19;
              case 12:
                v10 = ((uint8_t *)(v3 + (4) * 0x18) + 0xc);
                goto LABEL_19;
              case 16:
                v10 = ((uint8_t *)(v3 + (4) * 0x18) + 0x10);
                goto LABEL_19;
              case 20:
                v10 = ((uint8_t *)(v3 + (4) * 0x18) + 0x14);
LABEL_19:
                W_en_410090((intptr_t)(1), (intptr_t)(1000 * v4 / 1000), (intptr_t)(v7[1]), (intptr_t)(v10), (intptr_t)(&(*v17__f)), (intptr_t)(&(*v19__f)), (intptr_t)(&(*v18__f)), (intptr_t)(&(*v20__f)));
                if ( (*v17__f) != 0 )
                {
                  F32((16 * (*v14__f) + F32(v3, 0x14)), 0) = v8 + 248;
                  F32(16 * (*v14__f) + F32(v3, 0x14), 4) = 129;
                }
                if ( (*v18__f) != 0 )
                {
                  F32((16 * (*v14__f) + F32(v3, 0x14)), 0) = v8 + 248;
                  F32(16 * (*v14__f) + F32(v3, 0x14), 4) = 0;
                }
                if ( (*v19__f) != 0 )
                {
                  F32((16 * (*v14__f) + F32(v3, 0x14)), 0) = v8 + 148;
                  F32(16 * (*v14__f) + F32(v3, 0x14), 4) = 129;
                }
                if ( (*v20__f) != 0 )
                {
                  F32((16 * (*v14__f) + F32(v3, 0x14)), 0) = v8 + 148;
                  F32(16 * (*v14__f) + F32(v3, 0x14), 4) = 0;
                }
                break;
              case 32:
                v9 = v7[1];
                if ( v9 > 18000 )
                {
                  if ( v9 == 27000 )
                    ts_410250(v3, &(*v14__f), 506, 0);
                }
                else if ( v9 == 18000 )
                {
                  ts_410250(v3, &(*v14__f), 504, 0);
                }
                else if ( v9 != 0 )
                {
                  if ( v9 == 9000 )
                  {
                    *v7 = 550;
                    F32(16 * (*v14__f) + F32(v3, 0x14), 4) = 129;
                  }
                }
                else
                {
                  *v7 = 548;
                  F32(16 * (*v14__f) + F32(v3, 0x14), 4) = 129;
                }
                break;
              default:
                break;
            }
            v11 = F32(v3, 0x10);
            v6 = ++(*v14__f);
          }
          while ( (*v14__f) < v11 );
          v2 = (*v15__f);
        }
      }
    }
    ++v2;
    result = (*v16__f) - 1;
    v12 = (*v16__f) == 1;
    (*v15__f) = v2;
    --(*v16__f);
  }
  while ( !v12 );
  return result;
}

/* 0x410250: int __thiscall sub_410250(cls_410250 *this, int *a2, int a3, int a4) */
int32_t ts_410250(wl_char *self, int32_t * a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint32_t v5 = 0;
    uint32_t v6 = 0;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;

  F32((16 * *a2 + F32(c, 0x14)), 0) = a3 + 48;
  F32(16 * *a2 + F32(c, 0x14), 4) = 129;
  result = a4;
  if ( a4 != 0 )
  {
    v5 = F32(c, 0x10);
    if ( v5 < F32(c, 0xc) )
      F32(c, 0x10) = v5 + 1;
    v6 = F32(c, 0x10) - 1;
    if ( v6 > *a2 )
    {
      v7 = 16 * v6;
      do
      {
        v8 = (_DWORD *)(v7 + F32(c, 0x14));
        --v6;
        v7 -= 16;
        *v8 = *(v8 - 4);
        v8[1] = *(v8 - 3);
        v8[2] = *(v8 - 2);
        v8[3] = *(v8 - 1);
      }
      while ( v6 > *a2 );
    }
    v9 = *a2 + 1;
    *a2 = v9;
    F32((16 * v9 + F32(c, 0x14)), 0) = a4 + 48;
    result = F32(c, 0x14);
    F32(16 * *a2 + result, 4) = 129;
  }
  return result;
}

/* 0x54c530: char *__thiscall sub_54C530(cls_54C530 *this, char a2) */
int32_t ts_54c530(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t i = 0;
    char * v6 = 0;
    char * v7 = 0;

  if ( (a2 & 2) != 0 )
  {
    eh_vec_dtor(c, 0x4120u, *((int32_t *)c - 1), (void *)((intptr_t)D(0x54c5e0) /* el_54c5e0 by address */));
    if ( (a2 & 1) != 0 )
      g_eng.op_delete((void *)(uint8_t *)c - 4);
    return (uint8_t *)c - 4;
  }
  else
  {
    v4 = c + 8 * (2054);
    for ( i = 8; i != 0; --i )
    {
      if ( F32(((uint8_t *)(v4) + 0x0), 0) != 0 )
      {
        g_eng.op_delete((void *)F32((uint8_t *)(v4), 0x0));
        F32(((uint8_t *)(v4) + 0x0), 0) = 0;
      }
      v4 = ((uint8_t *)v4 + 4);
    }
    v6 = (char *)F32(c, 0x4);
    if ( v6 != 0 )
    {
      v7 = v6 - 4;
      eh_vec_dtor(v6, 0x170u, *((int32_t *)v6 - 1), (void *)((intptr_t)D(0x4017a0) /* input_dtor by address */));
      g_eng.op_delete((void *)v7);
      F32(c, 0x4) = 0;
    }
    if ( (a2 & 1) != 0 )
      g_eng.op_delete((void *)c);
    return ((uint8_t *)(c) + 0x0);
  }
}

/* 0x55ff80: int __thiscall sub_55FF80(cls_55FF80 *this, int a2, int a3) */
int32_t ts_55ff80(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t ExceptionList = 0;
    void * v4 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t i = 0;
    uint16_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    intptr_t j = 0;
    intptr_t v15 = 0;
    char * v16 = 0;
    uint16_t v17 = 0;
    char * v18 = 0;
    intptr_t v19 = 0;
    uint16_t n = 0;
    int32_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    uint32_t v26 = 0;
    uint16_t v27 = 0;
    intptr_t v28 = 0;
    _Alignas(8) uint8_t frame_[0x7824 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v34 = (uint16_t *)(frame_ + 0xc);   /* [2] at ebp-0x7818 */
    uint16_t *const k__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x7824 */
    uint16_t *const ii__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x7824 */
    intptr_t *const m__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x7824 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x7820 */
    char *const v35__f = (char *)(frame_ + 0x10);   /* ebp-0x7814 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x7818);   /* ebp-0xc */
    void * *const v37__f = (void * *)(frame_ + 0x781c);   /* ebp-0x8 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x7820);   /* ebp-0x4 */

  (*v38__f) = -1;
  v6 = (char *)(intptr_t)g_eng.op_new(0x4120u);
  v7 = (int)v6;
  (*v38__f) = 0;
  if ( v6 != 0 )
  {
    v8 = v6 + 16432;
    for ( i = 8; i != 0; --i )
      *v8++ = (intptr_t)g_eng.op_new(0xEECu);
    F32(v7, 4) = 0;
  }
  else
  {
    v7 = 0;
  }
  (*v38__f) = -1;
  v10 = 0;
  v11 = (_DWORD *)(v7 + 16516);
  do
  {
    v12 = *(v11 - 21);
    F16(v12, 2572) = v10;
    W_pslot_reset((intptr_t)(v12));
    *v11 = 1;
    ++v10;
    ++v11;
  }
  while ( v10 < 8u );
  F32(v7, 16548) = 1;
  F16(v7, 16552) = 0;
  F32(v7, 16408) = 1;
  F32(v7, 8204) = 0;
  F16(v7, 16400) = 0;
  F32(v7, 16404) = 0;
  F32(v7, 16412) = 0;
  F32(v7, 16464) = 0;
  F32(v7, 16468) = 3;
  F32(v7, 16472) = 0;
  F32(v7, 16556) = &(*v35__f);
  F16(v7, 8200) = 0;
  F32(v7, 16560) = 0;
  F32(v7, 16564) = 0;
  F32(v7, 16568) = 0;
  F32(v7, 16572) = 0;
  F32(v7, 16576) = 0;
  F32(v7, 16580) = 0;
  F32(v7, 16584) = 0;
  F32(v7, 16588) = 0;
  F32(v7, 16604) = 0;
  F32(v7, 16612) = 0;
  W_room_party_defaults((intptr_t)(v7));
  F32(v7, 16644) = 0;
  W_room_inputs_alloc((intptr_t)(v7), (intptr_t)(0));
  F32(v7, 16476) = 0;
  F16(v7, 16480) = 0;
  F16(v7, 16476) = 0;
  F16(v7, 16484) = 1;
  F16(v7, 16478) = 0;
  F16(v7, 16480) = 0;
  F16(v7, 16482) = 1;
  F32(v7, 16484) = 0;
  v13 = (_DWORD *)(v7 + 16648);
  for ( j = 3; j != 0; --j )
  {
    v13[1] = 0;
    *v13 = 1;
    v13 += 2;
  }
  F32(v7, 16488) = API_GetTickCount();
  F32(v7, 16492) = 0;
  F32(v7, 16404) = F32(c, 0x4);
  F32(v7, 16412) = F32(c, 0x1c);
  F32(v7, 16420) = *(_DWORD *)&F16(c, 0x10);
  F32(v7, 16424) = F32(c, 0x14);
  F32(v7, 16428) = F32(c, 0x18);
  F32(v7, 16592) = F32(c, 0x20);
  F32(v7, 16596) = F32(c, 0x24);
  F32(v7, 16600) = F32(c, 0x28);
  F32(v7, 16564) = F32(c, 0x30);
  F32(v7, 16568) = F32(c, 0x34);
  F32(v7, 16572) = F32(c, 0x38);
  F32(v7, 16576) = F32(c, 0x8c);
  F32(v7, 16580) = F32(c, 0x90);
  F32(v7, 16584) = F32(c, 0x94);
  F32(v7, 16588) = F32(c, 0x98);
  F32(v7, 16604) = F32(c, 0x9c);
  W_room_from_scene((intptr_t)(v7), (intptr_t)((int)c));
  F32(v7, 16488) = API_GetTickCount();
  F16(v7, 16400) = 0;
  for ( (*k__f) = 0; (*k__f) < F16(((uint8_t *)(c) + 0x8), 0); ++(*k__f) )
  {
    v15 = 16 * (*k__f);
    v16 = (uint8_t *)c + v15;
    if ( W_room_item_add((intptr_t)(v7), (intptr_t)(*(_DWORD *)&GAP(c, 0xa0, v15 + 4)), (intptr_t)(v34), (intptr_t)(*(uint16_t *)&GAP(c, 0xa0, 16 * (*k__f) + 16)), (intptr_t)(GAP(c, 0xa0, v15 + 18)), (intptr_t)(0)) != 0 )
      W_room_item_equip((intptr_t)(v7), (intptr_t)(v34[0]), (intptr_t)(F32(v16, 4 * 42)), (intptr_t)(0));
  }
  v17 = 0;
  for ( (*m__f) = 0; (uint16_t)(*m__f) < F16(c, 0x0); ++(*m__f) )
  {
    v18 = &GAP(c, 0x1804, 284 * v17 + 2208);
    if ( *(_WORD *)&GAP(c, 0x1804, 284 * v17 + 2216) != 0 )
    {
      (*v33__f) = F32(v7 + 4 * F16(v18, 0), 16432);
      W_tm_560a40((intptr_t)((*v33__f)), (intptr_t)(v7), (intptr_t)((int)v18), (intptr_t)((int)&GAP(c, 0x1804, 1056 * v17 + 4480)));
      if ( F16(v18, 2 * 5) != 0 )
        W_room_leader_set((intptr_t)(v7), (intptr_t)(&F32((*v33__f), 0x0)));
    }
    v17 = (*m__f) + 1;
  }
  W_room_party_level((intptr_t)(v7), (intptr_t)(1));
  ts_5612f0(v7);
  W_room_stats_refresh((intptr_t)(v7), (intptr_t)(1));
  v19 = F32(v7, 16556);
  if ( v19 != 0 )
    W_sv_560530((intptr_t)(v19), (intptr_t)((int)c));
  F16(v7, 8200) = 0;
  for ( n = 0; n < *(_WORD *)&GAP(c, 0x7808, 19076); ++n )
    W_room_flag_set((intptr_t)(v7), (intptr_t)(*(_DWORD *)&GAP(c, 0x7808, 8 * n + 19080)), (intptr_t)(*(_DWORD *)&GAP(c, 0x7808, 8 * n + 19084)));
  v21 = W_flag_find((intptr_t)(F32(v7, 16556)), (intptr_t)(100100101));
  if ( v21 != 0 )
    v22 = v21[1];
  else
    v22 = 0;
  v23 = F32(v7, 16644);
  if ( (v23 == 0 || (v24 = v23 - 1010) == 0 || v24 == 10) && v22 <= 2900 )
  {
    if ( W_tm_560a00((intptr_t)(F32(v7, 16556)), (intptr_t)(100100101)) > 500 )
      goto LABEL_53;
    for ( (*ii__f) = 0; (*ii__f) < 8u; ++(*ii__f) )
    {
      v25 = F32(v7 + 4 * (*ii__f), 16432);
      if ( W_pslot_ok((intptr_t)(v25), (intptr_t)(1), (intptr_t)(1)) )
      {
        v26 = F32(v25, 2544);
        if ( v26 < 0x5F5E165 )
          goto LABEL_54;
        if ( v26 <= 0x5F5E166 )
        {
          if ( F32(v25, 2664) + F32(v25, 2732) > 200
            || F32(v25, 2672) + F32(v25, 2736) > 200
            || F32(v25, 2676) + F32(v25, 2716) > 50
            || F32(v25, 2680) + F32(v25, 2720) > 50
            || F32(v25, 2684) + F32(v25, 2724) > 50
            || F32(v25, 2688) + F32(v25, 2728) > 50
            || F32(v25, 2692) > 10
            || F32(v25, 2696) > 10
            || F32(v25, 2700) > 10
            || F32(v25, 2704) > 10
            || F32(v25, 2792) + F32(v25, 2800) + F32(v25, 2804) > 4 )
          {
            goto LABEL_54;
          }
        }
        else if ( v26 != 100000104 )
        {
          goto LABEL_54;
        }
      }
    }
    if ( a3 == 0 || (v27 = 0, F16(((uint8_t *)(c) + 0x8), 0) == 0) )
    {
LABEL_53:
      v28 = 1;
      goto LABEL_55;
    }
    while ( W_item_def((intptr_t)(*(_DWORD *)&GAP(c, 0xa0, 16 * v27 + 4))) != 0 )
    {
      if ( ++v27 >= F16(((uint8_t *)(c) + 0x8), 0) )
        goto LABEL_53;
    }
  }
LABEL_54:
  v28 = 0;
LABEL_55:
  if ( v7 != 0 )
    W_room_delete((intptr_t)(v7), (intptr_t)(1));
  return v28;
}

/* 0x5612f0: int __thiscall sub_5612F0(cls_5612F0 *this) */
int32_t ts_5612f0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int32_t * v1 = 0;
    uint16_t v2 = 0;
    intptr_t v3 = 0;
    int16_t v4 = 0;
    char * v5 = 0;
    intptr_t i = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    char * v11 = 0;
    intptr_t j = 0;
    char * v13 = 0;
    int v14 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char * *const v16__f = (char * *)(frame_ + 0x0);   /* ebp-0x14 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x10 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0xc */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x8 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x4 */

  (*v16__f) = (uint8_t *)c + 16432;
  (*v19__f) = 8;
  do
  {
    if ( F32(F32((*v16__f), 0), 2500) == 1 || F32(F32((*v16__f), 0), 2500) == 2 )
    {
      (*v20__f) = F32(F32((*v16__f), 0), 2544);
      (*v18__f) = 0;
      v1 = (int32_t *)D(0x6c0cd0);
      do
      {
        if ( v1[174] == (*v20__f) && v1[175] != 0 )
        {
          v2 = F16(c, 0x4010);
          v3 = *v1;
          v4 = 0;
          (*v17__f) = *v1;
          if ( v2 == 0 )
            goto LABEL_13;
          v5 = &GAP(c, 0x0, 8208);
          for ( i = v2; i != 0; --i )
          {
            if ( F32(v5, 0) == v3 )
              ++v4;
            v5 += 16;
          }
          if ( v4 == 0 )
          {
LABEL_13:
            if ( v2 < 0x200u )
            {
              v7 = 0;
              v8 = 110010;
              while ( v3 != v8 )
              {
                v8 = ((int32_t *)D(0x6c0cd0))[184 * ++v7];
                if ( v8 == 0 )
                  goto LABEL_29;
              }
              v9 = &((int32_t *)D(0x6c0cd0))[184 * v7];
              if ( v9 != 0 )
              {
                if ( v9[171] >= 900 )
                  goto LABEL_28;
                v10 = 0;
                if ( v2 == 0 )
                  goto LABEL_28;
                v11 = &GAP(c, 0x0, 8220);
                for ( j = v2; j != 0; --j )
                {
                  if ( *(v11 - 2) == 0 )
                  {
                    if ( F32(F32(v11, 0), 684) < 900 )
                      ++v10;
                    v3 = (*v17__f);
                  }
                  v11 += 16;
                }
                if ( v10 < 48 )
                {
LABEL_28:
                  v13 = &GAP(c, 0x0, 16 * v2);
                  *(_DWORD *)&GAP(c, 0x0, 16 * v2 + 8208) = v3;
                  F32(v13, 4 * 2055) = v9;
                  F16(v13, 2 * 4108) = F16(v9, 2 * 50);
                  F32(v13, 4 * 2053) = 0;
                  v13[8218] = 0;
                  ++F16(c, 0x4010);
                }
              }
            }
          }
        }
LABEL_29:
        v1 = &((int32_t *)D(0x6c0cd0))[184 * (uint16_t)++(*v18__f)];
      }
      while ( *v1 != 0 );
    }
    result = (*v19__f) - 1;
    v14 = (*v19__f) == 1;
    (*v16__f) += 4;
    --(*v19__f);
  }
  while ( !v14 );
  return result;
}

/* 0x5640b0: int __thiscall sub_5640B0(cls_5640B0 *this) */
int32_t ts_5640b0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint16_t i = 0;
    intptr_t v4 = 0;
    void * v5 = 0;
    void * v6 = 0;

  result = F32(c, 0x8);
  if ( result != 0 )
  {
    for ( i = 0; i < F16(c, 0x4); ++i )
    {
      v4 = F32(c, 0x8);
      v5 = F32(v4, 24 * i);
      v6 = (void *)(v4 + 24 * i);
      if ( v5 != 0 )
      {
        W_sd_5bb3a0((intptr_t)(F32(v4, 24 * i)));
        g_eng.op_delete((void *)v5);
      }
      result = 0;
      memset(v6, 0, 0x18u);
    }
  }
  return result;
}

/* 0x564110: int __thiscall sub_564110(cls_564110 *this, int a2, unsigned __int16 a3) */
int32_t ts_564110(wl_char *self, int32_t a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    void * v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;

  result = a2;
  F32(c, 0x0) = a2;
  if ( a2 != 0 )
  {
    F16(c, 0x4) = a3;
    v5 = (intptr_t)g_eng.op_new(24 * a3);
    v6 = F16(c, 0x4);
    F32(c, 4 * 2) = v5;
    v7 = 3 * v6;
    memset(v5, 0, 8 * v7);
    return 0;
  }
  return result;
}

/* 0x565b00: int __thiscall sub_565B00( cls_565B00 *this, const char *a2, int a3, int a4, int a5, unsigned __int16 a6, unsigned __int16 a7) */
int32_t ts_565b00(wl_char *self, char * a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    char * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    int32_t ** v20 = 0;
    intptr_t i = 0;
    int32_t * v22 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x228 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v26 = (uint8_t *)(frame_ + 0x4);   /* [36] at ebp-0x224 */
    char *v27 = (char *)(frame_ + 0x28);   /* [512] at ebp-0x200 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x228 */

  v7 = 7 * a6 + 80;
  v8 = 28 * a7 + 48;
  if ( a5 != 0 )
  {
    v9 = 320 - v7 / 2;
    v10 = 240 - v8 / 2;
  }
  else
  {
    v10 = a4;
    v9 = a3;
  }
  (*v25__f) = F32(c, 0x0);
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v26), (intptr_t)(0));
  v11 = W_win_mgr_open((intptr_t)((*v25__f)), (intptr_t)(v9), (intptr_t)(v10), (intptr_t)(v7), (intptr_t)(v8), (intptr_t)(1u), (intptr_t)(v26));
  v12 = F16(v11, 78);
  if ( F16(v11, 76) <= v12 )
  {
    v13 = 0;
  }
  else
  {
    v13 = F32((F32(v11, 72) + 4 * v12), 0);
    W_ui_ref_set((intptr_t)(v13), (intptr_t)(v11), (intptr_t)(v12));
    ++F16(v11, 78);
    if ( v13 != 0 )
      W_ctrl_text_init((intptr_t)(v13), (intptr_t)(40), (intptr_t)(24), (intptr_t)(a6), (intptr_t)(a7));
    else
      v13 = 0;
  }
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v11), (intptr_t)(v13));
  if ( F32(v13, 356) != 0 )
  {
    strcpy(v27, a2);
    v15 = 0;
    if ( F32(v13, 364) != 0 )
    {
      v16 = 0;
      do
      {
        W_key_token_replace((intptr_t)(v27), (intptr_t)(F32(v13, 356) + 100 * v16));
        v16 = (uint16_t)++v15;
      }
      while ( (unsigned int)(uint16_t)v15 < F32(v13, 364) );
    }
    v14 = v27;
  }
  else
  {
    v14 = (char *)a2;
  }
  W_text_layout_set((intptr_t)(**(intptr_t **)(v13 + 368)), (intptr_t)(v14), (intptr_t)(F16(F32(v13, 368), 8)));
  v17 = 11;
  F16(F32(v13, 368), 4) = 0;
  F16(F32(v13, 368), 6) = 0;
  F32(v13, 28) = 0;
  v18 = F32(c, 0x4);
  F16(v18, 364) = 0;
  v19 = (_DWORD *)(v18 + 276);
  do
  {
    v19[11] = 0;
    *v19++ = 0;
    --v17;
  }
  while ( v17 != 0 );
  v20 = (_DWORD **)(v18 + 12);
  for ( i = 64; i != 0; --i )
  {
    v22 = *v20++;
    *v22 = 0;
  }
  F32(v18, 268) = 0;
  F32(v18, 272) = 0;
  do
    result = W_tm_565d10((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v11), (intptr_t)(0));
  while ( result == 0 );
  F32(v11, 80) = 0;
  if ( result == 12 )
    return 0;
  return result;
}

/* 0x566300: int __thiscall sub_566300(cls_566300 *this, int a2) */
int32_t ts_566300(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t result = 0;
    int32_t * v5 = 0;

  v2 = 63;
  v3 = c + 4 * (66);
  result = 1;
  while ( 1 )
  {
    v5 = (_DWORD *)F32(v3, 0);
    if ( **(_DWORD **)v3 != 0 && v5[2] == 1 && (unsigned int)(a2 - v5[1]) <= 0x64 )
      break;
    --v2;
    v3 -= 4;
    if ( v2 < 0 )
      return 0;
  }
  **((int32_t **)c + v2 + 3) = 0;
  return result;
}

/* 0x568780: int __stdcall sub_568780(cls_5BA3D0 *a1) */
int32_t ts_568780(int32_t a1)
{

    intptr_t v1 = 0;
    uint16_t v2 = 0;
    intptr_t v3 = 0;

  v1 = 0;
  v2 = 0;
  if ( W_bs_5ba390((intptr_t)(a1)) == 0 )
    return 0;
  v3 = 0;
  do
  {
    if ( W_id_5ba3d0((intptr_t)(a1), (intptr_t)(v3)) != 0 )
    {
      switch ( W_id_5ba3a0((intptr_t)(a1), (intptr_t)(v3)) )
      {
        case 16:
          v1 = 116;
          break;
        case 17:
          v1 = 122;
          break;
        case 18:
          v1 = 104;
          break;
        case 19:
          v1 = 117;
          break;
        case 20:
          v1 = 119;
          break;
        case 21:
          v1 = 124;
          break;
        case 22:
          v1 = 120;
          break;
        case 23:
          v1 = 108;
          break;
        case 24:
          v1 = 114;
          break;
        case 25:
          v1 = 115;
          break;
        case 30:
          v1 = 100;
          break;
        case 31:
          v1 = 118;
          break;
        case 32:
          v1 = 103;
          break;
        case 33:
          v1 = 105;
          break;
        case 34:
          v1 = 106;
          break;
        case 35:
          v1 = 107;
          break;
        case 36:
          v1 = 109;
          break;
        case 37:
          v1 = 110;
          break;
        case 38:
          v1 = 111;
          break;
        case 44:
          v1 = 125;
          break;
        case 45:
          v1 = 123;
          break;
        case 46:
          v1 = 102;
          break;
        case 47:
          v1 = 121;
          break;
        case 48:
          v1 = 101;
          break;
        case 49:
          v1 = 113;
          break;
        case 50:
          v1 = 112;
          break;
        case 55:
          v1 = 133;
          break;
        case 57:
          v1 = 126;
          break;
        case 71:
          v1 = 145;
          break;
        case 72:
          v1 = 146;
          break;
        case 73:
          v1 = 147;
          break;
        case 74:
          v1 = 134;
          break;
        case 75:
          v1 = 142;
          break;
        case 76:
          v1 = 143;
          break;
        case 77:
          v1 = 144;
          break;
        case 78:
          v1 = 135;
          break;
        case 79:
          v1 = 139;
          break;
        case 80:
          v1 = 140;
          break;
        case 81:
          v1 = 141;
          break;
        case 82:
          v1 = 148;
          break;
        case 83:
          v1 = 136;
          break;
        case 156:
          v1 = 138;
          break;
        case 181:
          v1 = 137;
          break;
        case 199:
          v1 = 127;
          break;
        case 201:
          v1 = 129;
          break;
        case 207:
          v1 = 128;
          break;
        case 209:
          v1 = 130;
          break;
        case 210:
          v1 = 131;
          break;
        case 211:
          v1 = 132;
          break;
        default:
          break;
      }
    }
    v3 = ++v2;
  }
  while ( v2 < (unsigned int)W_bs_5ba390((intptr_t)(a1)) );
  return v1;
}

/* 0x569220: int __thiscall sub_569220(cls_569220 *this, int a2) */
int32_t ts_569220(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    char * v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
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
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    int32_t * v48 = 0;
    char * v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    int32_t * v52 = 0;
    intptr_t v53 = 0;
    uint16_t v54 = 0;
    intptr_t v55 = 0;
    int32_t * v56 = 0;
    int32_t * v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    char * v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    int32_t * v64 = 0;
    intptr_t result = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    int32_t * v69 = 0;
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    int32_t * v72 = 0;
    int32_t * v73 = 0;
    intptr_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    intptr_t v78 = 0;
    intptr_t v79 = 0;
    int32_t * v80 = 0;
    intptr_t v81 = 0;
    intptr_t v82 = 0;
    intptr_t v83 = 0;
    intptr_t v84 = 0;
    intptr_t v85 = 0;
    intptr_t v86 = 0;
    intptr_t v87 = 0;
    int32_t * v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    intptr_t v91 = 0;
    int32_t * v92 = 0;
    intptr_t v93 = 0;
    uint16_t v94 = 0;
    intptr_t v95 = 0;
    int32_t * v96 = 0;
    intptr_t v97 = 0;
    intptr_t v98 = 0;
    intptr_t v99 = 0;
    intptr_t v100 = 0;
    uint16_t j = 0;
    _Alignas(8) uint8_t frame_[0x2c8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v114 = (int16_t *)(frame_ + 0x3c);   /* [2] at ebp-0x28c */
    char *Buffer = (char *)(frame_ + 0xc8);   /* [512] at ebp-0x200 */
    intptr_t *const v102__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c8 */
    intptr_t *const v103__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x2c0 */
    intptr_t *const v104__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2b0 */
    uint16_t *const v107__f = (uint16_t *)(frame_ + 0x2c);   /* ebp-0x29c */
    uint16_t *const v105__f = (uint16_t *)(frame_ + 0x2c);   /* ebp-0x29c */
    intptr_t *const v106__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x29c */
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x298 */
    intptr_t *const v108__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x298 */
    intptr_t *const v112__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x294 */
    intptr_t *const v111__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x294 */
    intptr_t *const v110__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x294 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x290 */
    int16_t *const v115__f = (int16_t *)(frame_ + 0x40);   /* ebp-0x288 */
    int16_t *const v116__f = (int16_t *)(frame_ + 0x42);   /* ebp-0x286 */
    intptr_t *const v117__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x284 */
    intptr_t *const v118__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x280 */
    intptr_t *const v119__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x27c */
    intptr_t *const v120__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x278 */
    intptr_t *const v121__f = (intptr_t *)(frame_ + 0x78);   /* ebp-0x250 */
    intptr_t *const v122__f = (intptr_t *)(frame_ + 0xa0);   /* ebp-0x228 */

  v2 = 0;
  (*v117__f) = c;
  (*v118__f) = 0;
  (*i__f) = 1;
  if ( a2 == 34 )
  {
    v2 = ((char *)D(0x8a26e8));
  }
  else if ( a2 == 35 )
  {
    v2 = ((char *)D(0x8a26fc));
  }
  v3 = F32(c, 0x0);
  v4 = 0;
  v5 = 2;
  if ( v2 != 0 )
  {
    v4 = 1;
    v5 = 3;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)&(*v120__f)), (intptr_t)(v4));
  v6 = W_win_mgr_open((intptr_t)(v3), (intptr_t)(272), (intptr_t)(16), (intptr_t)(360), (intptr_t)(360), (intptr_t)(v5), (intptr_t)(&(*v120__f)));
  v7 = v6;
  (*v119__f) = v6;
  if ( v2 != 0 )
    W_win_title_add((intptr_t)(v6), (intptr_t)(v2));
  v8 = W_win_ctrl_take((intptr_t)(v7));
  v9 = v8;
  if ( v8 != 0 )
    W_ctrl_grid_init((intptr_t)(v8), (intptr_t)(40), (intptr_t)(56), (intptr_t)(30), (intptr_t)(2), (intptr_t)(10), (intptr_t)(2));
  F32(F32(v9, 0x178), 0) = 0;
  v10 = F32(v9, 0x178);
  v114[0] = 0;
  F32(v10, 4) = 0;
  F32(F32(v9, 0x178), 8) = 0;
  v11 = F32(v9, 0x178);
  v114[1] = 1;
  (*v115__f) = 2;
  F32(v11, 12) = 0;
  v12 = F32(v9, 0x178);
  (*v116__f) = 3;
  F32(v12, 36) = 240;
  F32(F32(v9, 0x178), 40) = 0;
  F32(F32(v9, 0x178), 44) = 0;
  F32(F32(v9, 0x178), 48) = 0;
  W_ctrl_cursor_set((intptr_t)(v9), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v114), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v9) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v9, 0x14, 4) = -9;
  }
  (*v103__f) = F32(v7, 20) - 36;
  (*v102__f) = F32(v7, 24) / 2 - 16;
  v114[0] = 0;
  v114[1] = 1;
  (*v115__f) = 2;
  (*v116__f) = 3;
  W_tm_56daa0((intptr_t)(v9), (intptr_t)(SYS32(2956)), (intptr_t)(4), (intptr_t)(4), (intptr_t)((*v102__f)), (intptr_t)(8), (intptr_t)((*v103__f)), (intptr_t)((*v102__f)), (intptr_t)(v114), (intptr_t)(4u), (intptr_t)(8));
  W_tm_56da40((intptr_t)(v9), (intptr_t)((const char *)((char *)D(0x854898))), (intptr_t)(F32(v7, 20) - 56), (intptr_t)(8));
  F32(v9 + (1) * 0x180, 0x20) = 0xFFFFFF;
  *(_DWORD *)&GAP((v9 + (1) * 0x180), 0x14, 8) = 4546427;
  if ( a2 == 34 )
  {
    v13 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(23));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_18;
    v13 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(49));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x8a26dc))));
LABEL_18:
  if ( a2 == 34 )
  {
    v14 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(24));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_23;
    v14 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(48));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(((char *)D(0x8a26cc))));
LABEL_23:
  if ( a2 == 34 )
  {
    v15 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(25));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_28;
    v15 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(50));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(((char *)D(0x8a26b4))));
LABEL_28:
  if ( a2 == 34 )
  {
    v16 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(26));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_33;
    v16 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(51));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(((char *)D(0x8a26ac))));
LABEL_33:
  if ( a2 == 34 )
  {
    v17 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(27));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_38;
    v17 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(52));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v17), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2690))));
LABEL_38:
  if ( a2 == 34 )
  {
    v18 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(28));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_43;
    v18 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(53));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2684))));
LABEL_43:
  if ( a2 == 34 )
  {
    v19 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(30));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_48;
    v19 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(55));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2670))));
LABEL_48:
  if ( a2 == 34 )
  {
    v20 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(33));
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v20), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2668))));
    v21 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(34));
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v21), (intptr_t)(0), (intptr_t)(((char *)D(0x8a265c))));
    v22 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(29));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_53;
    v22 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(54));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2648))));
LABEL_53:
  if ( a2 == 34 )
  {
    v23 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(31));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_58;
    v23 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(56));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v23), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2634))));
LABEL_58:
  if ( a2 == 34 )
  {
    v24 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(32));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_63;
    v24 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(57));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v24), (intptr_t)(0), (intptr_t)(((char *)D(0x8a261c))));
LABEL_63:
  if ( a2 == 34 )
  {
    v25 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(35));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_68;
    v25 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(58));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v25), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2608))));
LABEL_68:
  if ( a2 == 35 )
  {
    v26 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(59));
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v26), (intptr_t)(0), (intptr_t)(((char *)D(0x8a25fc))));
    v27 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(60));
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v27), (intptr_t)(0), (intptr_t)(((char *)D(0x8a25ec))));
    v28 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(62));
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v28), (intptr_t)(0), (intptr_t)(((char *)D(0x8a25dc))));
    v29 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(61));
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(((char *)D(0x8a25cc))));
  }
  if ( a2 == 34 )
  {
    v30 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(36));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_75;
    v30 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(63));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v30), (intptr_t)(0), (intptr_t)(((char *)D(0x8a25bc))));
LABEL_75:
  if ( a2 == 34 )
  {
    v31 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(37));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_80;
    v31 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(64));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v31), (intptr_t)(0), (intptr_t)(((char *)D(0x8a25ac))));
LABEL_80:
  if ( a2 == 34 )
  {
    v32 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(38));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_85;
    v32 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(65));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v32), (intptr_t)(0), (intptr_t)(((char *)D(0x8a259c))));
LABEL_85:
  if ( a2 == 34 )
  {
    v33 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(39));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_90;
    v33 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(66));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v33), (intptr_t)(0), (intptr_t)(((char *)D(0x8a258c))));
LABEL_90:
  if ( a2 == 34 )
  {
    v34 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(40));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_95;
    v34 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(67));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v34), (intptr_t)(0), (intptr_t)(((char *)D(0x8a257c))));
LABEL_95:
  if ( a2 == 34 )
  {
    v35 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(41));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_100;
    v35 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(68));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v35), (intptr_t)(0), (intptr_t)(((char *)D(0x8a256c))));
LABEL_100:
  if ( a2 == 34 )
  {
    v36 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(42));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_105;
    v36 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(69));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v36), (intptr_t)(0), (intptr_t)(((char *)D(0x8a255c))));
LABEL_105:
  if ( a2 == 34 )
  {
    v37 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(43));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_110;
    v37 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(70));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v37), (intptr_t)(0), (intptr_t)(((char *)D(0x8a254c))));
LABEL_110:
  if ( a2 == 34 )
  {
    v38 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(44));
  }
  else
  {
    if ( a2 != 35 )
      goto LABEL_115;
    v38 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(71));
  }
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v38), (intptr_t)(0), (intptr_t)(((char *)D(0x8a253c))));
LABEL_115:
  if ( a2 == 34 )
  {
    v39 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(45));
    goto LABEL_119;
  }
  if ( a2 == 35 )
  {
    v39 = W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(72));
LABEL_119:
    W_grid_cell_text((intptr_t)(v9), (intptr_t)(v39), (intptr_t)(0), (intptr_t)(((char *)D(0x8a252c))));
  }
  v40 = W_grid_row_add((intptr_t)(v9), (intptr_t)(3), (intptr_t)(28));
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v40), (intptr_t)(0), (intptr_t)(((char *)D(0x8a110c))));
  v41 = W_grid_row_add((intptr_t)(v9), (intptr_t)(3), (intptr_t)(29));
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v41), (intptr_t)(0), (intptr_t)(((char *)D(0x8a10fc))));
  v42 = W_grid_row_add((intptr_t)(v9), (intptr_t)(3), (intptr_t)(27));
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v42), (intptr_t)(0), (intptr_t)(((char *)D(0x8a10ec))));
  v43 = SYS32(800);
  v44 = 0;
  if ( F16(SYS32(800), 4) == 0 )
    goto LABEL_124;
  do
  {
    v45 = 7 * (uint16_t)v44++;
    F32(F32(v43, 0) + 4 * v45, 12) = F32(F32(v43, 0) + 4 * v45, 8);
  }
  while ( (uint16_t)v44 < F16(v43, 4) );
  while ( 2 )
  {
    v46 = 0;
    v47 = 0;
    for ( (*i__f) = 0; (uint16_t)v47 < F32(F32(v9, 0x174), 16); v46 = (uint16_t)v47 )
    {
      v48 = (_DWORD *)(16 * v46 + F32(v9, 0x17c));
      if ( *v48 == 0 )
      {
        v49 = W_tm_566a80((intptr_t)(v48[1]), (intptr_t)(F32(CFG() + 28 * (uint16_t)v48[1], 12)), (intptr_t)(Buffer));
        W_grid_cell_text((intptr_t)(v9), (intptr_t)(v46), (intptr_t)(1), (intptr_t)(v49));
      }
      ++v47;
    }
    (*v105__f) = 0;
    v50 = F32(F32(v9, 0x174), 16);
    (*v110__f) = v50;
    if ( v50 > 0 )
    {
      v51 = 0;
      do
      {
        v52 = (_DWORD *)(16 * v51 + F32(v9, 0x17c));
        if ( *v52 == 0 )
        {
          v53 = F32(CFG() + 28 * (uint16_t)v52[1], 12);
          if ( v53 != 0 && (v54 = 0, v50 > 0) )
          {
            v55 = 0;
            while ( 1 )
            {
              v56 = (_DWORD *)(F32(v9, 0x17c) + 16 * v55);
              if ( *v56 == 0
                && (*v105__f) != v54
                && F32(CFG() + 28 * (uint16_t)v56[1], 12) == v53 )
              {
                break;
              }
              v55 = ++v54;
              if ( v54 >= (*v110__f) )
                goto LABEL_139;
            }
            W_cell_color_set((intptr_t)(v9), (intptr_t)(v51), (intptr_t)(1), (intptr_t)(1), (intptr_t)(255), (intptr_t)(255));
          }
          else
          {
LABEL_139:
            W_cell_color_set((intptr_t)(v9), (intptr_t)(v51), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          }
        }
        v50 = F32(F32(v9, 0x174), 16);
        v51 = ++(*v105__f);
        (*v110__f) = v50;
      }
      while ( (*v105__f) < v50 );
    }
LABEL_141:
    strcpy(Buffer, D(0x8a9b6c));
    v57 = (_DWORD *)(16 * F32(F32(v9, 0x174), 20) + F32(v9, 0x17c));
    if ( *v57 != 0 )
    {
      if ( *v57 != 3 )
        goto LABEL_155;
      v58 = v57[1] - 27;
      if ( v58 != 0 )
      {
        v59 = v58 - 1;
        if ( v59 != 0 )
        {
          if ( v59 != 1 )
            goto LABEL_155;
          v60 = ((char *)D(0x8a2518));
        }
        else
        {
          v60 = ((char *)D(0x8a2504));
        }
      }
      else
      {
        v60 = ((char *)D(0x8a24f0));
      }
    }
    else if ( a2 == 34 )
    {
      v60 = ((char *)D(0x8a2434));
    }
    else
    {
      if ( a2 != 35 )
        goto LABEL_155;
      v60 = ((char *)D(0x8a2490));
    }
    strcpy(Buffer, v60);
LABEL_155:
    v61 = (*v117__f);
    W_ctrl_text_set((intptr_t)(F32((*v117__f) + (1) * 0x8, 0x4)), (intptr_t)(Buffer));
    if ( F32((16 * F32(F32(v9, 0x174), 20) + F32(v9, 0x17c)), 0) != 0 )
    {
      W_tm_564690((intptr_t)(&(*v120__f)));
      while ( 1 )
      {
        do
        {
          while ( 1 )
          {
            v62 = W_menu_pace((intptr_t)(&(*v120__f))) - 1;
            if ( v62 != 0 )
              break;
            W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
            W_rd_48c820((intptr_t)(F32((uint8_t *)(v61), 0x0)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(*(intptr_t *)&SYSB(864)));
            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
          }
        }
        while ( v62 != 1 );
        v63 = F32(v120__f, 0x20);
        if ( W_tm_5642e0((intptr_t)(v61), (intptr_t)(F32(v120__f, 0x20)), (intptr_t)(0)) == 6 )
          break;
        if ( v9 != 0 )
        {
          switch ( W_ctrl_input((intptr_t)(F32(v61, 0x4)), (intptr_t)(v9), (intptr_t)(v63), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) )
          {
            case 1:
            case 2:
              goto LABEL_186;
            case 3:
              goto LABEL_187;
            case 4:
              goto LABEL_188;
            default:
              break;
          }
        }
        W_tm_566350((intptr_t)(v61), (intptr_t)(v64));
      }
LABEL_164:
      result = 6;
      (*v108__f) = 6;
      goto LABEL_190;
    }
    if ( a2 == 34 )
    {
      W_tm_564690((intptr_t)(&(*v121__f)));
      while ( 1 )
      {
        do
        {
          while ( 1 )
          {
            v70 = W_menu_pace((intptr_t)(&(*v121__f))) - 1;
            if ( v70 != 0 )
              break;
            W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
            W_rd_48c820((intptr_t)(F32((uint8_t *)(v61), 0x0)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(*(intptr_t *)&SYSB(864)));
            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
          }
        }
        while ( v70 != 1 );
        v71 = F32(v121__f, 0x20);
        if ( W_tm_5642e0((intptr_t)(v61), (intptr_t)(F32(v121__f, 0x20)), (intptr_t)(1)) == 6 )
          goto LABEL_164;
        v68 = ts_56a450(SYS32(10364));
        if ( v68 != 0 )
        {
LABEL_185:
          F32(v114, 0) = v68;
          result = 25;
          (*v108__f) = 25;
          goto LABEL_190;
        }
        if ( v9 != 0 )
        {
          switch ( W_ctrl_input((intptr_t)(F32(v61, 0x4)), (intptr_t)(v9), (intptr_t)(v71), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) )
          {
            case 1:
            case 2:
LABEL_186:
              result = 13;
              (*v108__f) = 13;
              break;
            case 3:
LABEL_187:
              result = 12;
              (*v108__f) = 12;
              break;
            case 4:
LABEL_188:
              W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(0));
              result = 11;
              (*v108__f) = 11;
              break;
            default:
              goto LABEL_183;
          }
          goto LABEL_190;
        }
LABEL_183:
        W_tm_566350((intptr_t)(v61), (intptr_t)(v72));
      }
    }
    if ( a2 == 35 )
    {
      W_tm_564690((intptr_t)(&(*v122__f)));
      while ( 1 )
      {
        do
        {
          while ( 1 )
          {
            v66 = W_menu_pace((intptr_t)(&(*v122__f))) - 1;
            if ( v66 != 0 )
              break;
            W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
            W_rd_48c820((intptr_t)(F32((uint8_t *)(v61), 0x0)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(*(intptr_t *)&SYSB(864)));
            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
          }
        }
        while ( v66 != 1 );
        v67 = F32(v122__f, 0x20);
        if ( W_tm_5642e0((intptr_t)(v61), (intptr_t)(F32(v122__f, 0x20)), (intptr_t)(0)) == 6 )
          goto LABEL_164;
        v68 = ts_568780(SYS32(10360));
        if ( v68 != 0 )
          goto LABEL_185;
        if ( v9 != 0 )
        {
          switch ( W_ctrl_input((intptr_t)(F32(v61, 0x4)), (intptr_t)(v9), (intptr_t)(v67), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0)) )
          {
            case 1:
            case 2:
              goto LABEL_186;
            case 3:
              goto LABEL_187;
            case 4:
              goto LABEL_188;
            default:
              break;
          }
        }
        W_tm_566350((intptr_t)(v61), (intptr_t)(v69));
      }
    }
    result = (*v108__f);
LABEL_190:
    switch ( result )
    {
      case 11:
        goto LABEL_223;
      case 12:
        v73 = (_DWORD *)(F32(v9, 0x17c) + 16 * F32(F32(v9, 0x174), 20));
        if ( *v73 != 3 )
          goto LABEL_124;
        v74 = 0;
        v75 = v73[1] - 28;
        if ( v75 != 0 )
        {
          if ( v75 != 1 )
            goto LABEL_199;
          v76 = ts_56a270(v61, ((char *)D(0x8a222c)), 380);
        }
        else
        {
          v76 = ts_56a270(v61, ((char *)D(0x8a2204)), 380);
        }
        if ( v76 != 14 )
        {
          v74 = v76;
          if ( v76 == 15 )
            v74 = 11;
        }
LABEL_199:
        (*v108__f) = v74;
        if ( v74 != 0 )
        {
          if ( v74 != 11 )
          {
            result = v74;
            goto LABEL_257;
          }
LABEL_124:
          if ( (*i__f) != 0 )
            continue;
          goto LABEL_141;
        }
        v77 = F32(v9, 0x174);
        if ( F32(16 * F32((v77 + 20), 0) + F32(v9, 0x17c), 4) != 27 )
        {
          if ( F32(16 * F32((v77 + 20), 0) + F32(v9, 0x17c), 4) == 28 )
          {
            v78 = 0;
            if ( F32(v77, 16) > 0 )
            {
              v79 = 0;
              do
              {
                v80 = (_DWORD *)(F32(v9, 0x17c) + 16 * v79);
                if ( *v80 == 0 )
                  F32(CFG() + 28 * (uint16_t)v80[1], 12) = F32(CFG() + 28 * (uint16_t)v80[1], 4);
                v79 = (uint16_t)++v78;
              }
              while ( (uint16_t)v78 < F32(F32(v9, 0x174), 16) );
            }
LABEL_123:
            (*i__f) = 1;
          }
          else if ( F32(16 * F32((v77 + 20), 0) + F32(v9, 0x17c), 4) == 29 )
          {
            goto LABEL_256;
          }
          goto LABEL_124;
        }
LABEL_223:
        if ( a2 == 35 )
        {
          v90 = CFG();
          if ( F32(CFG(), 1356) == 0
            || F32(v90, 1412) == 0
            || F32(v90, 1384) == 0 )
          {
            v97 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(((uint8_t *)(v61) + 0x0), 0)), (intptr_t)(140), (intptr_t)(174), (intptr_t)(360), (intptr_t)(132), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
            v98 = W_win_ctrl_take((intptr_t)(v97));
            v99 = v98;
            if ( v98 != 0 )
              W_ctrl_text_init((intptr_t)(v98), (intptr_t)(40), (intptr_t)(24), (intptr_t)(40), (intptr_t)(3));
            W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v97), (intptr_t)(v99));
            W_ctrl_text_set((intptr_t)(v99), (intptr_t)(((char *)D(0x8a23e4))));
            W_tm_56a220((intptr_t)(F32(v61, 0x4)));
            do
              result = W_tm_565d10((intptr_t)(v61), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v97), (intptr_t)(0));
            while ( result == 0 );
            if ( result == 12 )
              result = 0;
            F32(v97, 80) = 0;
            (*v108__f) = result;
            if ( result != 0 )
              goto LABEL_257;
            goto LABEL_124;
          }
        }
        (*v109__f) = 1;
        (*v107__f) = 0;
        (*v112__f) = F32(F32(v9, 0x174), 16);
        if ( (*v112__f) > 0 )
        {
          v91 = 0;
          do
          {
            v92 = (_DWORD *)(16 * v91 + F32(v9, 0x17c));
            if ( *v92 == 0 )
            {
              v93 = F32(CFG() + 28 * (uint16_t)v92[1], 12);
              if ( v93 != 0 && (v94 = 0, (*v112__f) > 0) )
              {
                v95 = 0;
                while ( 1 )
                {
                  v96 = (_DWORD *)(F32(v9, 0x17c) + 16 * v95);
                  if ( *v96 == 0
                    && (*v107__f) != v94
                    && F32(CFG() + 28 * (uint16_t)v96[1], 12) == v93 )
                  {
                    break;
                  }
                  v95 = ++v94;
                  if ( v94 >= (*v112__f) )
                  {
                    v61 = (*v117__f);
                    goto LABEL_238;
                  }
                }
                (*v109__f) = 0;
                W_cell_color_set((intptr_t)(v9), (intptr_t)(v91), (intptr_t)(1), (intptr_t)(1), (intptr_t)(255), (intptr_t)(255));
                v61 = (*v117__f);
              }
              else
              {
LABEL_238:
                W_cell_color_set((intptr_t)(v9), (intptr_t)(v91), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
              }
            }
            v91 = ++(*v107__f);
            (*v112__f) = F32(F32(v9, 0x174), 16);
          }
          while ( (*v107__f) < (*v112__f) );
          if ( (*v109__f) == 0 )
          {
            result = ts_565b00(v61, ((char *)D(0x8a2414)), 0, 0, 1, 0x14u, 2u);
            (*v108__f) = result;
            if ( result != 0 )
              goto LABEL_257;
            goto LABEL_124;
          }
        }
        v100 = SYS32(800);
        for ( j = 0; j < F16(v100, 4); ++j )
        {
          (*v104__f) = F32(v100, 0) + 28 * j;
          F32((*v104__f), 8) = F32((*v104__f), 12);
          W_sc_4c5300((intptr_t)((*v104__f)));
        }
        W_tm_5658c0((intptr_t)(SYS32(800)));
LABEL_256:
        result = (*v118__f);
LABEL_257:
        F32((*v119__f), 80) = 0;
        return result;
      case 13:
        goto LABEL_124;
      case 25:
        W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0));
        v81 = F32(v9, 0x174);
        v82 = F32(v9, 0x17c);
        v83 = F32(v114, 0);
        v84 = 28 * (uint16_t)F32(16 * F32((v81 + 20), 0) + v82, 4);
        v85 = CFG();
        if ( F32(v114, 0) == F32(v84 + CFG(), 12) )
        {
          F32(v84 + v85, 12) = 0;
          goto LABEL_123;
        }
        v86 = F32(v81, 16);
        (*v106__f) = 0;
        (*v111__f) = v86;
        if ( v86 <= 0 )
          goto LABEL_222;
        v87 = 0;
        while ( 2 )
        {
          v88 = (_DWORD *)(v82 + 16 * v87);
          if ( *v88 != 0 )
            goto LABEL_219;
          v89 = v88[1];
          v83 = F32(v114, 0);
          if ( F32(v114, 0) != F32(v85 + 28 * (uint16_t)v89, 12) )
          {
            v86 = (*v111__f);
LABEL_219:
            v87 = (uint16_t)++(*v106__f);
            if ( (uint16_t)(*v106__f) >= v86 )
            {
              F32(v84 + CFG(), 12) = v83;
              goto LABEL_123;
            }
            continue;
          }
          break;
        }
        F32(v85 + 28 * (uint16_t)v89, 12) = F32(v84 + v85, 12);
LABEL_222:
        F32(v84 + CFG(), 12) = v83;
        goto LABEL_123;
      default:
        goto LABEL_257;
    }
  }
}

/* 0x56a270: int __thiscall sub_56A270(cls_41BBE0 *this, const char *a2, int a3) */
int32_t ts_56a270(wl_char *self, char * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t result = 0;
    intptr_t v17 = 0;
    _Alignas(8) uint8_t frame_[0x48 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v15 = (uint8_t *)(frame_ + 0x0);   /* [36] at ebp-0x48 */
    uint8_t *v16 = (uint8_t *)(frame_ + 0x24);   /* [36] at ebp-0x24 */

  v4 = 320 - a3 / 2;
  v6 = F32(c, 0x0);
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v15), (intptr_t)(0));
  v17 = W_win_mgr_open((intptr_t)(v6), (intptr_t)(v4), (intptr_t)(200), (intptr_t)(a3), (intptr_t)(64), (intptr_t)(1u), (intptr_t)(v15));
  v7 = W_win_ctrl_take((intptr_t)(v17));
  v8 = v7;
  if ( v7 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v7), (intptr_t)(32), (intptr_t)(24), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_grid_row_add((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0));
  }
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v8) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v8, 0x14, 4) = -9;
  }
  W_grid_cell_text((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a2));
  v9 = F32(c, 0x0);
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v16), (intptr_t)(0));
  v10 = W_win_mgr_open((intptr_t)(v9), (intptr_t)(v4 + a3 - 160), (intptr_t)(248), (intptr_t)(128), (intptr_t)(96), (intptr_t)(2u), (intptr_t)(v16));
  v11 = W_win_grid_new((intptr_t)(v10), (intptr_t)(40), (intptr_t)(24), (intptr_t)(2), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v11), (intptr_t)(1));
  v12 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(14));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(D(0x854d04)));
  v13 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(15));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x854d00))));
  while ( 1 )
  {
    result = W_tm_565d10((intptr_t)(c), (intptr_t)((_DWORD *)v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      result = F32(16 * F32((F32(v11, 372) + 20), 0) + F32((v11 + 380), 0), 4);
      F32(v17, 80) = 0;
      F32(v10, 80) = 0;
      return result;
    }
    if ( result != 13 )
    {
      F32(v17, 80) = 0;
      F32(v10, 80) = 0;
      return result;
    }
  }
  F32(v17, 80) = 0;
  F32(v10, 80) = 0;
  return 15;
}

/* 0x56a450: int __stdcall sub_56A450(cls_5BA3D0 *a1) */
int32_t ts_56a450(int32_t a1)
{

    intptr_t v1 = 0;
    uint16_t v2 = 0;
    intptr_t v3 = 0;
    uint32_t v4 = 0;
    uint32_t v5 = 0;
    uint32_t v6 = 0;

  v1 = 0;
  v2 = 0;
  if ( W_bs_5ba390((intptr_t)(a1)) == 0 )
    return 0;
  v3 = 0;
  do
  {
    if ( W_id_5ba3d0((intptr_t)(a1), (intptr_t)(v3)) != 0 )
    {
      v4 = W_id_5ba3a0((intptr_t)(a1), (intptr_t)(v3));
      if ( v4 > 0x9C )
      {
        if ( v4 > 0x225 )
        {
          switch ( v4 )
          {
            case 0x226u:
              v1 = 302;
              break;
            case 0x227u:
              v1 = 303;
              break;
            case 0x228u:
              v1 = 304;
              break;
            case 0x229u:
              v1 = 305;
              break;
            case 0x22Au:
              v1 = 306;
              break;
            case 0x22Bu:
              v1 = 307;
              break;
            default:
              break;
          }
        }
        else if ( v4 == 549 )
        {
          v1 = 301;
        }
        else if ( v4 > 0x104 )
        {
          if ( v4 == 264 )
          {
            v1 = 210;
          }
          else if ( v4 == 548 )
          {
            v1 = 300;
          }
        }
        else if ( v4 == 260 )
        {
          v1 = 208;
        }
        else
        {
          v5 = v4 - 160;
          if ( v5 != 0 )
          {
            v6 = v5 - 4;
            if ( v6 != 0 )
            {
              if ( v6 == 92 )
                v1 = 206;
            }
            else
            {
              v1 = 209;
            }
          }
          else
          {
            v1 = 207;
          }
        }
      }
      else if ( v4 == 156 )
      {
        v1 = 205;
      }
      else
      {
        switch ( v4 )
        {
          case '0':
            v1 = 100;
            break;
          case '1':
            v1 = 101;
            break;
          case '2':
            v1 = 102;
            break;
          case '3':
            v1 = 103;
            break;
          case '4':
            v1 = 104;
            break;
          case '5':
            v1 = 105;
            break;
          case '6':
            v1 = 106;
            break;
          case '7':
            v1 = 107;
            break;
          case '8':
            v1 = 108;
            break;
          case '9':
            v1 = 109;
            break;
          case ':':
            v1 = 110;
            break;
          case ';':
            v1 = 111;
            break;
          case '<':
            v1 = 112;
            break;
          case '=':
            v1 = 113;
            break;
          case '>':
            v1 = 114;
            break;
          case '?':
            v1 = 115;
            break;
          default:
            break;
        }
      }
    }
    v3 = ++v2;
  }
  while ( v2 < (unsigned int)W_bs_5ba390((intptr_t)(a1)) );
  return v1;
}

/* 0x56a670: int __thiscall sub_56A670(cls_41BBE0 *this, int **a2, _WORD *a3, _DWORD *a4) */
int32_t ts_56a670(wl_char *self, int32_t ** a2, uint16_t * a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
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
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x1c530 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *Buffer = (uint8_t *)(frame_ + 0x10);   /* [49804] at ebp-0x1c520 */
    uint8_t *v29 = (uint8_t *)(frame_ + 0xc2a0);   /* [8192] at ebp-0x10290 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c530 */
    int32_t * *const v24__f = (int32_t * *)(frame_ + 0x4);   /* ebp-0x1c52c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1c528 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x1c524 */
    int16_t *const v28__f = (int16_t *)(frame_ + 0xc29c);   /* ebp-0x10294 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0xe2a0);   /* ebp-0xe290 */

  memset(Buffer, 0, 8356);
  (*v23__f) = 0;
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
  (*v28__f) = 0;
  memset(v29, 0, (8192 * sizeof(uint8_t)));
  if ( a4 != 0 )
    *a4 = 0;
  W_tm_5644a0((intptr_t)(c));
  v10 = F32(c, 0x0);
  W_scene_obj_init((intptr_t)(&(*v30__f)));
  v11 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(v10), (intptr_t)(8), (intptr_t)(8), (intptr_t)(624), (intptr_t)(464), (intptr_t)(2u), (intptr_t)(((char *)D(0x854f9c))), (intptr_t)(1));
  (*v26__f) = v11;
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
  W_sys_page_arrows((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(v11), (intptr_t)(v12));
  W_sv_41bf60((intptr_t)((int *)v12), (intptr_t)(0));
  W_tm_41c2f0((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(&(*v25__f)), (intptr_t)(&(*v24__f)));
  W_tm_419900((intptr_t)(v12), (intptr_t)((*v25__f)), (intptr_t)((uint16_t)(*v24__f)));
  v14 = F32((F32(v11, 72) + 4 * F16(v11, 78) - 4), 0);
  (*v25__f) = (int)v14;
  if ( F32(D(0x8a6b54), 0) == 0 )
    g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a2730)), D(0x8a9b6c));
  while ( 1 )
  {
    v15 = W_tm_565d10((intptr_t)(c), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( v15 == 11 )
      break;
    if ( v15 == 12 )
    {
      v16 = v14[95] + 16 * F32(v14[93], 20);
      v17 = F32(v16, 4);
      (*v24__f) = *(int **)v16;
      if ( W_sv_41b670((intptr_t)(Buffer), (intptr_t)((int)(*v24__f)), (intptr_t)(v17), (intptr_t)(0)) == 0 )
        goto LABEL_28;
      if ( a4 != 0 && (v15 = ts_56aa70(c, a4, Buffer)) != 0 )
      {
        if ( v15 != 11 )
        {
          if ( v15 != 12 )
          {
LABEL_24:
            (*v23__f) = v15;
            goto LABEL_28;
          }
LABEL_26:
          *a2 = (*v24__f);
          *a3 = v17;
          (*v23__f) = 12;
          W_tm_41c320((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(*a2), (intptr_t)(v17));
          goto LABEL_28;
        }
      }
      else
      {
        if ( ts_55ff80(Buffer, 0, 0) != 0 )
          goto LABEL_26;
        v18 = 0;
        v19 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(210), (intptr_t)(188), (intptr_t)(220), (intptr_t)(104), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
        v20 = W_win_text_add((intptr_t)(v19), (intptr_t)(40), (intptr_t)(24), (intptr_t)(20), (intptr_t)(2));
        W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v19), (intptr_t)(v20));
        W_ctrl_text_set((intptr_t)(v20), (intptr_t)(((char *)D(0x8a2710))));
        W_tm_56a220((intptr_t)(F32(c, 0x4)));
        do
          v21 = W_tm_565d10((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v19), (intptr_t)(0));
        while ( v21 == 0 );
        if ( v21 != 12 )
          v18 = v21;
        F32(v19, 80) = 0;
        if ( v18 != 0 )
        {
          v11 = (*v26__f);
          (*v23__f) = v18;
          goto LABEL_28;
        }
        v14 = (_DWORD *)(*v25__f);
        v11 = (*v26__f);
      }
    }
    else if ( v15 != 13 )
    {
      goto LABEL_24;
    }
  }
  (*v23__f) = 11;
LABEL_28:
  F32(v11, 80) = 0;
  W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  if ( (*v23__f) != 6 )
    W_tm_565d10((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0x14u));
  ts_56ad50(c);
  return (*v23__f);
}

/* 0x56aa70: int __thiscall sub_56AA70(cls_41BBE0 *this, _DWORD *a2, _DWORD *a3) */
int32_t ts_56aa70(wl_char *self, int32_t * a2, int32_t * a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint32_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0xb30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v28 = (char *)(frame_ + 0xc);   /* [116] at ebp-0xb24 */
    char *v31 = (char *)(frame_ + 0x88);   /* [37] at ebp-0xaa8 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xb30 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xb2c */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x80);   /* ebp-0xab0 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x84);   /* ebp-0xaac */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0xd8);   /* ebp-0xa58 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0xdc);   /* ebp-0xa54 */

  if ( ts_55ff80(a3, 1, 0) == 0 )
    return 0;
  v5 = a3[4769];
  v6 = 0;
  if ( v5 == 0 )
    return 0;
  v7 = 0;
  while ( a3[3 * v7 + 4771] != 100100101 )
  {
    v7 = ++v6;
    if ( v6 >= v5 )
      return 0;
  }
  if ( a3[3 * v6 + 4772] != 500 )
    return 0;
  strcpy(&v28[36], ((char *)D(0x8a2770)));
  (*v30__f) = 0;
  (*v29__f) = 1;
  strcpy(v31, ((char *)D(0x8a2748)));
  v8 = 0;
  (*v33__f) = 1020;
  v9 = 0;
  while ( a3[3 * v9 + 4771] != 101200105 )
  {
    v9 = ++v8;
    if ( v8 >= v5 )
    {
      v10 = 0;
      goto LABEL_13;
    }
  }
  v10 = a3[3 * v8 + 4772];
LABEL_13:
  v11 = F32(c, 0x0);
  v12 = 0;
  (*v32__f) = v10 == 0;
  v13 = 2;
  v14 = 24;
  v15 = 96;
  if ( ((char *)D(0x8a2738)) != 0 )
  {
    v15 = 128;
    v14 = 56;
    v12 = 1;
    v13 = 3;
  }
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v28), (intptr_t)(v12));
  v16 = W_win_mgr_open((intptr_t)(v11), (intptr_t)(120), (intptr_t)(200), (intptr_t)(400), (intptr_t)(v15), (intptr_t)(v13), (intptr_t)(v28));
  v17 = v16;
  (*v26__f) = v16;
  if ( ((char *)D(0x8a2738)) != 0 )
  {
    v18 = W_win_grid_add((intptr_t)(v16), (intptr_t)(32), (intptr_t)(7), (intptr_t)(0), (intptr_t)(0));
    v19 = v18;
    if ( v18 != 0 )
    {
      W_grid_cell_text((intptr_t)(v18), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2738))));
      *(_DWORD *)((uint8_t *)(v19 + (1) * 0x180) + 0x0) = 0xFFFFFF;
      *(_DWORD *)&GAP((v19 + (1) * 0x180), 0x0, 4) = 4546427;
    }
  }
  v20 = W_win_grid_new((intptr_t)(v17), (intptr_t)(40), (intptr_t)(v14), (intptr_t)(2), (intptr_t)(1), (intptr_t)(2), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v20), (intptr_t)(1));
  v21 = 0;
  v22 = &(*v29__f);
  for ( (*i__f) = 2; (*i__f) != 0; --(*i__f) )
  {
    v23 = W_grid_row_add((intptr_t)(v20), (intptr_t)(0), (intptr_t)(v21));
    W_grid_cell_text((intptr_t)(v20), (intptr_t)(v23), (intptr_t)(0), (intptr_t)((uint8_t *)v22 - 80));
    v24 = *v22;
    ++v21;
    v22 += 22;
    F32(16 * v23 + F32((v20 + 380), 0), 8) = v24;
  }
  while ( 1 )
  {
    result = W_tm_565d10((intptr_t)(c), (intptr_t)((_DWORD *)v20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 12 )
      break;
    if ( result != 13 )
      goto LABEL_25;
  }
  result = 12;
  *a2 = F32(&v31[88 * F32(16 * F32((F32(v20, 372) + 20), 0) + F32((v20 + 380), 0), 4) - 4], 0);
LABEL_25:
  *(_DWORD *)((uint8_t *)((*v26__f) + (1) * 0x50) + 0x0) = 0;
  return result;
}

/* 0x56ad50: void __thiscall sub_56AD50(cls_41BBE0 *this) */
void ts_56ad50(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint8_t * v1 = 0;
    intptr_t i = 0;
    intptr_t v3 = 0;
    uint16_t j = 0;
    uint8_t * v5 = 0;
    intptr_t k = 0;
    uint16_t * v7 = 0;
    intptr_t m = 0;
    intptr_t v9 = 0;
    ar_gdi_slot ** v10 = 0;
    intptr_t n = 0;
    ar_gdi_slot * v12 = 0;
    void * v13 = 0;
    void * v14 = 0;

  W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854780))));
  v1 = &SYSB(2732);
  for ( i = 909; i != 0; --i )
  {
    v3 = F32(v1, 0);
    if ( F32(v1, 0) != 0 && F16(v3, 66) >= 4u )
    {
      for ( j = 0; j < F16(v3, 4); ++j )
        W_slot_frames_free((intptr_t)(v3), (intptr_t)(j));
    }
    v1 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854770))));
  v5 = &SYSB(868);
  for ( k = 465; k != 0; --k )
  {
    v7 = *(uint16_t **)v5;
    if ( F32(v5, 0) != 0 && v7[10] >= 4u && F32(v7, 4 * 1) != 0 )
    {
      for ( m = *v7 - 1; m >= 0; --m )
      {
        v9 = F32(F32(v7, 4 * 1), 8 * m);
        if ( v9 != 0 )
        {
          W_wn_562b50((intptr_t)(v9), (intptr_t)(1));
          F32((F32(v7, 4 * 1) + 8 * m), 0) = 0;
        }
      }
      g_eng.op_delete((void *)*((void **)v7 + 1));
      F32(v7, 4 * 1) = 0;
    }
    v5 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854760))));
  v10 = (ar_gdi_slot **)&SYSB(10004);
  for ( n = 16; n != 0; --n )
  {
    v12 = *v10;
    if ( *v10 != 0 && v12->group >= 4u )
      W_tm_562a10((intptr_t)(v12));
    ++v10;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854744))));
  v13 = (void *)F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    W_en_56c640((intptr_t)(F32(c, 0x0)));
    g_eng.op_delete((void *)v13);
    F32(c, 0x0) = 0;
  }
  v14 = (void *)F32(c, 0x4);
  if ( v14 != 0 )
  {
    W_input_dtor((intptr_t)(F32(c, 0x4)));
    g_eng.op_delete((void *)v14);
    F32(c, 0x4) = 0;
  }
}

/* 0x56d620: DWORD __thiscall sub_56D620(cls_56D620 *this) */
int32_t ts_56d620(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint32_t result = 0;
    uint16_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t TickCount = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const i__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x4 */

  result = F32(c, 0x8);
  if ( result != 0 )
  {
    result = API_GetTickCount();
    v3 = 0;
    for ( (*i__f) = result; v3 < F16(c, 0x4); ++v3 )
    {
      result = 3 * v3;
      v4 = F32(c, 0x8) + 24 * v3;
      v5 = F32(v4, 0);
      if ( F32(v4, 0) != 0 )
      {
        if ( F32(v4, 16) == 1 )
        {
          TickCount = API_GetTickCount();
          v9 = F32(v4, 0);
          F32(v4, 12) = TickCount;
          v10 = F32(v4, 4);
          F32(v4, 16) = 2;
          g_eng.dsb_volume_5bb870(v9, v10);
          result = g_eng.dsb_play_5bb8c0(F32(v4, 0));
        }
        else if ( F32(v4, 16) == 2 )
        {
          result = F32(v4, 8);
          if ( (*i__f) - F32(v4, 12) > result )
          {
            F32(v4, 16) = 3;
            F32(v4, 20) = 200;
          }
        }
        else
        {
          result = F32(v4, 16) - 3;
          if ( F32(v4, 16) == 3 )
          {
            v6 = F32(v4, 4) - F32(v4, 20);
            if ( v6 < -10000 )
              v6 = -10000;
            F32(v4, 4) = v6;
            result = g_eng.dsb_volume_5bb870(v5, v6);
            if ( F32(v4, 4) <= -10000 )
            {
              v7 = F32(v4, 0);
              if ( F32(v4, 0) != 0 )
              {
                W_sd_5bb3a0((intptr_t)(F32(v4, 0)));
                g_eng.op_delete((void *)v7);
              }
              result = 0;
              memset((void *)v4, 0, 0x18u);
            }
          }
        }
      }
    }
  }
  return result;
}

/* 0x56dd80: int *__stdcall sub_56DD80(int a1, int a2) */
int32_t ts_56dd80(int32_t a1, int32_t a2)
{

    uint16_t v2 = 0;
    int32_t * result = 0;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;

  v2 = 0;
  if ( a1 == 0xFFFF )
    return 0;
  if ( a1 < 0 )
    return 0;
  if ( ((int32_t *)D(0x854080))[0] != 0 )
  {
    do
      ++v2;
    while ( ((int32_t *)D(0x854080))[6 * v2] != 0 );
  }
  if ( a1 >= v2 )
    return 0;
  if ( ((int32_t *)D(0x85408c))[6 * a1] != 0 && a2 == 0 )
    return 0;
  v4 = 1;
  v5 = 0;
  v6 = ((int32_t *)D(0x854084))[6 * a1];
  v7 = 0;
  while ( v4 != ((int32_t *)D(0x854080))[6 * a1] || ((int32_t *)D(0x67ac5c))[v7] != v6 && v6 != 0 )
  {
    v7 = 55 * ++v5;
    v4 = ((int32_t *)D(0x67ac58))[v7];
    if ( v4 == 0 )
    {
      result = (int32_t *)D(0x67ac58);
      goto LABEL_18;
    }
  }
  result = (int *)(220 * v5 + 6794328);
LABEL_18:
  if ( result != 0 )
    return (int *)(((int32_t *)D(0x854088))[6 * a1] <= F32(CFG(), 2780));
  return result;
}

/* 0x583fe0: int __thiscall sub_583FE0(cls_583FE0 *this) */
int32_t ts_583fe0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    uint16_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    uint16_t v14 = 0;
    _Alignas(8) uint8_t frame_[0x258 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v25 = (char *)(frame_ + 0x4c);   /* [512] at ebp-0x20c */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x258 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x244 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x240 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x23c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x238 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x234 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x230 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x220 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x21c */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x254);   /* ebp-0x4 */

  v1 = c;
  (*v20__f) = c;
  (*v21__f) = 0;
  F32(v22__f, 0x0) = 0;
  F32(v22__f, 0x4) = 0;
  (*v26__f) = 0;
  F32(D(0x8a9518), 0) = 1;
  F32(D(0x8a951c), 0) = 2;
  F32(D(0x8a9524), 0) = 4;
  F32(D(0x8a9520), 0) = 1;
  F32(D(0x8a9528), 0) = 0;
  do
  {
    (*v19__f) = 0;
    v2 = 3;
    v3 = F32(CFG(), 2684);
    W_tm_5644a0((intptr_t)(&(*v22__f)));
    ts_584420(&(*v22__f), 32, 392);
    if ( SYS32(780) != 0 )
      v2 = 5;
    W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(v22__f, 0x0)), (intptr_t)(32), (intptr_t)(8), (intptr_t)(320), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(v2), (intptr_t)(1u), (intptr_t)(&(*v23__f)), (intptr_t)((int *)&(*v17__f)), (intptr_t)(((char *)D(0x8a2c4c))), (intptr_t)(1));
    v4 = (*v17__f);
    v5 = W_grid_row_add((intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(32));
    W_grid_cell_text((intptr_t)(v4), (intptr_t)(v5), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2c40))));
    W_grid_row_text_add((intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(31), (intptr_t)(((char *)D(0x8a2c30))));
    if ( SYS32(780) != 0 )
    {
      W_grid_row_text_add((intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(34), (intptr_t)(((char *)D(0x8a2c20))));
      W_grid_row_text_add((intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(33), (intptr_t)(((char *)D(0x8a2c04))));
    }
    W_grid_row_text_add((intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2360))));
    W_tm_419900((intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(v3));
    while ( 1 )
    {
      strcpy(v25, D(0x8a9b6c));
      switch ( F32(16 * F32((F32((*v17__f), 0x174) + 20), 0) + F32((*v17__f), 0x17c), 4) )
      {
        case 0:
          v6 = ((char *)D(0x8a2a8c));
          goto LABEL_13;
        case 0x1F:
          v6 = ((char *)D(0x8a2b38));
          goto LABEL_13;
        case 0x20:
          v6 = ((char *)D(0x8a2b84));
          goto LABEL_13;
        case 0x21:
          v6 = ((char *)D(0x8a2ae0));
          goto LABEL_13;
        case 0x22:
          v6 = ((char *)D(0x8a2aac));
LABEL_13:
          strcpy(v25, v6);
          break;
        default:
          break;
      }
      W_ctrl_text_set((intptr_t)(F32(v22__f, 0xc)), (intptr_t)(v25));
      v7 = W_tm_565d10((intptr_t)(&(*v22__f)), (intptr_t)((*v17__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( v7 == 11 )
        break;
      if ( v7 == 12 )
      {
        (*v19__f) = F32(16 * F32((F32((*v17__f), 0x174) + 20), 0) + F32((*v17__f), 0x17c), 4);
        break;
      }
      if ( v7 != 13 )
      {
        (*v19__f) = v7;
        break;
      }
    }
    *(_DWORD *)((uint8_t *)((*v23__f) + (1) * 0x50) + 0x0) = 0;
    ts_584400(&(*v22__f));
    v8 = (*v19__f);
    if ( (*v19__f) != 6 )
      W_tm_565d10((intptr_t)(&(*v22__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0x14u));
    ts_56ad50(&(*v22__f));
    switch ( v8 )
    {
      case 31:
        (*v18__f) = ts_59dc60(v1);
        goto LABEL_32;
      case 32:
        F32(v24__f, 0x0) = 0;
        F32(v24__f, 0x4) = 0;
        SETLB((*v26__f), 1);
        while ( 1 )
        {
          W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          v9 = ts_584550(&(*v24__f), 39, 100);
          if ( v9 != 0 )
            break;
          v10 = (intptr_t)g_eng.op_new(0x18u);
          (*v18__f) = (int)v10;
          SETLB((*v26__f), 2);
          if ( v10 != 0 )
            v11 = ts_5846f0(v10);
          else
            v11 = 0;
          SETLB((*v26__f), 1);
          F32(v1, 0) = v11;
          ts_584710(v11);
          if ( F32(v1, 0) != 0 )
          {
            W_en_5a45c0((intptr_t)(F32(v1, 0)), (intptr_t)(1));
            F32(v1, 0) = 0;
          }
        }
        SETLB((*v26__f), 0);
        W_tm_41bbe0((intptr_t)(&(*v24__f)));
        (*v18__f) = v9;
        goto LABEL_32;
      case 33:
      case 34:
        (*v18__f) = 0;
LABEL_32:
        v12 = F32(CFG(), 2676);
        if ( v12 == 0 )
          continue;
        v13 = F16(CFG(), 2682);
        v14 = 0;
        if ( v13 == 0 )
          goto LABEL_38;
        break;
      default:
        goto LABEL_41;
    }
    while ( (*v19__f) != F32(v12 + 8 * v14, 4) )
    {
      if ( ++v14 >= v13 )
        goto LABEL_38;
    }
    (*v16__f) = CFG() + 2660;
    F32(CFG(), 2668) = F32((v12 + 8 * v14), 0);
    W_sc_4c5300((intptr_t)((*v16__f)));
LABEL_38:
    v1 = (*v20__f);
  }
  while ( (*v18__f) != 6 );
  (*v21__f) = 6;
LABEL_41:
  (*v26__f) = -1;
  W_tm_41bbe0((intptr_t)(&(*v22__f)));
  return (*v21__f);
}

/* 0x584400: int __thiscall sub_584400(cls_584400 *this) */
int32_t ts_584400(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = F32(c, 0x8);
  if ( result != 0 )
  {
    F32(result, 80) = 0;
    F32(c, 0x8) = 0;
    F32(c, 0xc) = 0;
  }
  return result;
}

/* 0x584420: int __thiscall sub_584420(cls_584420 *this, int a2, int a3) */
int32_t ts_584420(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    int16_t *const v11__f = (int16_t *)(frame_ + 0x18);   /* ebp-0x20 */
    int16_t *const v12__f = (int16_t *)(frame_ + 0x1a);   /* ebp-0x1e */
    int16_t *const v13__f = (int16_t *)(frame_ + 0x1c);   /* ebp-0x1c */
    int16_t *const v14__f = (int16_t *)(frame_ + 0x1e);   /* ebp-0x1a */
    int16_t *const v15__f = (int16_t *)(frame_ + 0x20);   /* ebp-0x18 */
    int16_t *const v16__f = (int16_t *)(frame_ + 0x22);   /* ebp-0x16 */
    int16_t *const v17__f = (int16_t *)(frame_ + 0x24);   /* ebp-0x14 */
    int16_t *const v18__f = (int16_t *)(frame_ + 0x26);   /* ebp-0x12 */
    int16_t *const v19__f = (int16_t *)(frame_ + 0x28);   /* ebp-0x10 */
    int16_t *const v20__f = (int16_t *)(frame_ + 0x2a);   /* ebp-0xe */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  (*v21__f) = 32;
  (*v22__f) = 32;
  (*v10__f) = SYS32(2952);
  v4 = F32(c, 0x0);
  (*v11__f) = 0;
  (*v12__f) = 0;
  (*v13__f) = 1;
  (*v14__f) = 2;
  (*v15__f) = 3;
  (*v16__f) = 4;
  (*v17__f) = 5;
  (*v18__f) = 6;
  (*v19__f) = 7;
  (*v20__f) = 8;
  (*v23__f) = 0;
  v5 = W_win_mgr_open((intptr_t)(v4), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(576), (intptr_t)(80), (intptr_t)(1u), (intptr_t)(&(*v10__f)));
  F32(c, 0x8) = v5;
  v6 = F16(v5, 78);
  if ( F16(v5, 76) > v6
    && (v7 = F32((F32(v5, 72) + 4 * v6), 0),
        F32(v7, 0) = v5,
        F16(v7, 4) = v6,
        F32(v7, 8) = 0,
        ++F16(v5, 78),
        v7 != 0) )
  {
    W_ctrl_text_init((intptr_t)(v7), (intptr_t)(40), (intptr_t)(24), (intptr_t)(68), (intptr_t)(2));
  }
  else
  {
    v7 = 0;
  }
  (*v9__f) = F32(c, 0x8);
  F32(c, 0xc) = v7;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v9__f)), (intptr_t)(v7));
  result = F32(c, 0xc);
  F32(F32((result + 368), 0), 12) = 0;
  F32(F32((result + 368), 0), 16) = 0;
  F32(F32((result + 368), 0), 32) = 0;
  F32(F32((result + 368), 0), 36) = 0;
  F32(F32((result + 368), 0), 40) = 0;
  F32(F32((result + 368), 0), 20) = 0;
  F32(F32((result + 368), 0), 24) = 0;
  F32(result, 28) = 0;
  return result;
}

/* 0x584550: int __thiscall sub_584550(cls_41BBE0 *this, int a2, int a3) */
int32_t ts_584550(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */

  W_tm_5644a0((intptr_t)(c));
  if ( F32(CFG(), 2780) >= a3 )
  {
    v9 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(32), (intptr_t)(392), (intptr_t)(576), (intptr_t)(80), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
    F32(c, 0x8) = (int)v9;
    v10 = W_win_ctrl_take((intptr_t)(v9));
    v11 = v10;
    if ( v10 != 0 )
    {
      W_ctrl_text_init((intptr_t)(v10), (intptr_t)(40), (intptr_t)(24), (intptr_t)(68), (intptr_t)(2));
      v10 = v11;
    }
    (*v14__f) = F32(c, 0x8);
    F32(c, 0xc) = (int)v10;
    W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v14__f)), (intptr_t)(v10));
    W_text_metrics_set((intptr_t)(F32(c, 0xc)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v4 = W_tm_564780((intptr_t)(c), (intptr_t)(a2));
    v12 = F32(c, 0x8);
    if ( v12 != 0 )
    {
      F32(v12, 80) = 0;
      F32(c, 0x8) = 0;
      F32(c, 0xc) = 0;
    }
  }
  else
  {
    v4 = 11;
    v5 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(140), (intptr_t)(188), (intptr_t)(360), (intptr_t)(104), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
    v6 = W_win_ctrl_take((intptr_t)(v5));
    v7 = v6;
    if ( v6 != 0 )
      W_ctrl_text_init((intptr_t)(v6), (intptr_t)(40), (intptr_t)(24), (intptr_t)(40), (intptr_t)(2));
    W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v5), (intptr_t)(v7));
    W_ctrl_text_set((intptr_t)(v7), (intptr_t)(((char *)D(0x8a2c58))));
    W_tm_56a220((intptr_t)(F32(c, 0x4)));
    do
      v8 = W_tm_565d10((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v5), (intptr_t)(0));
    while ( v8 == 0 );
    if ( v8 == 12 )
      v8 = 0;
    F32(v5, 80) = 0;
    if ( v8 == 0 )
      goto LABEL_15;
    v4 = v8;
  }
  if ( v4 != 6 )
LABEL_15:
    W_tm_565d10((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0x14u));
  W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
  W_tm_41bbe0((intptr_t)(c));
  return v4;
}

/* 0x5846f0: _DWORD *__thiscall sub_5846F0(cls_5846F0 *this) */
int32_t ts_5846f0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x4) = 0;
  F32(c, 0x8) = 0;
  F32(c, 0xc) = 0;
  F32(c, 0x0) = 0;
  F32(c, 0x10) = 0;
  F32(c, 0x14) = 0;
  return &F32(c, 0x0);
}

/* 0x584710: int __thiscall actor_manifest_build_584710(cls_584710 *this) */
int32_t ts_584710(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    char * v3 = 0;
    intptr_t i = 0;
    int32_t * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    int32_t * v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t j = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    int32_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    int32_t * v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    int32_t * v31 = 0;
    intptr_t * v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    int32_t * v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    int32_t * v40 = 0;
    _Alignas(8) uint8_t frame_[0x3e0 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v50 = (intptr_t *)(frame_ + 0xc);   /* [2] at ebp-0x3d4 */
    intptr_t *v51 = (intptr_t *)(frame_ + 0x14);   /* [80] at ebp-0x3cc */
    intptr_t *v52 = (intptr_t *)(frame_ + 0x154);   /* [80] at ebp-0x28c */
    intptr_t *v53 = (intptr_t *)(frame_ + 0x294);   /* [80] at ebp-0x14c */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3e0 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3e0 */
    intptr_t *const k__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x3e0 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x3dc */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x3d8 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x3d8 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x3d8 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x3dc);   /* ebp-0x4 */

  v1 = c;
  v2 = F32(CFG(), 2376);
  memset(v52, 0, 80);
  memset(v51, 0, 80);
  v52[20] = 511002;
  v52[21] = 515020;
  memset(&v52[22], 0, 0x48u);
  v52[40] = 511010;
  v52[41] = 515002;
  memset(&v52[42], 0, 0x48u);
  v52[60] = 511020;
  v52[61] = 515001;
  memset(&v52[62], 0, 0x48u);
  v51[20] = 521030;
  v51[21] = 525020;
  memset(&v51[22], 0, 0x48u);
  v51[40] = 521010;
  v51[41] = 525001;
  memset(&v51[42], 0, 0x48u);
  v51[60] = 521002;
  v51[61] = 525002;
  memset(&v51[62], 0, 0x48u);
  memset(v53, 0, 80);
  v53[20] = 531030;
  v53[21] = 535010;
  memset(&v53[22], 0, 0x48u);
  v53[40] = 531020;
  v53[41] = 535030;
  memset(&v53[42], 0, 0x48u);
  v53[60] = 531010;
  v53[61] = 535020;
  memset(&v53[62], 0, 0x48u);
  (*v47__f) = v2;
  F32(c, 0x10) = (int)(intptr_t)g_eng.op_new(0x5400Cu);
  F32(c, 0x14) = (int)(intptr_t)g_eng.op_new(0x7808u);
  v3 = &GAP(c, 0x0, 4);
  for ( i = 3; i != 0; --i )
  {
    v5 = (intptr_t)g_eng.op_new(0x88u);
    F32(v3, 0) = v5;
    *v5 = 0;
    v6 = F32(v3, 0);
    v3 += 4;
    F16(v6, 132) = 0;
  }
  F16(D(0x8a9400), 0) = 0;
  F32(D(0x8a9408), 0) = 0;
  v7 = F32(v1, 0x10);
  F32(v7, 0x0) = (int)D(0x693848);
  F32(v7 + (1) * 0x54008, 0x0) = 0;
  F16(v7, 0x54004) = 0;
  do
    ++F16(v7, 0x54004);
  while ( ((int32_t *)D(0x6940c8))[84 * F16(v7, 0x54004)] != 0 );
  (*v42__f) = 0;
  if ( F16(v7, 0x54004) != 0 )
  {
    do
    {
      v8 = 336 * (uint16_t)(*v42__f);
      qmemcpy(&GAP(v7, 0x4, v8), &((int32_t *)D(0x6940c8))[v8 / 4], 0x150u);
      W_area_xref((intptr_t)(v7), (intptr_t)(((int32_t *)D(0x6940c8))), (intptr_t)((int)&GAP(v7, 0x4, v8)));
      ++(*v42__f);
    }
    while ( (uint16_t)(*v42__f) < F16(v7, 0x54004) );
    v1 = c;
  }
  v9 = F32(v1, 0x14);
  F32(v9, 6144) = 0;
  F32(v9, 30724) = 0;
  v10 = (intptr_t)g_eng.op_new(0x8244u);
  (*v54__f) = 0;
  if ( v10 != 0 )
  {
    v11 = (int)(v10 + 1);
    *v10 = 2;
    eh_vec_ctor(
      v10 + 1,
      0x4120u,
      2,
      (void *)((intptr_t)D(0x5851a0) /* el_5851a0 by address */),
      (void *)((intptr_t)D(0x54c5e0) /* el_54c5e0 by address */));
  }
  else
  {
    v11 = 0;
  }
  (*v54__f) = -1;
  v12 = 0;
  v13 = (_DWORD *)(v11 + 16516);
  (*v43__f) = F32(c, 0x14);
  do
  {
    v14 = *(v13 - 21);
    F16(v14, 2572) = v12;
    W_pslot_reset((intptr_t)(v14));
    *v13 = 1;
    ++v12;
    ++v13;
  }
  while ( v12 < 8u );
  F32(v11, 16548) = 1;
  F16(v11, 16552) = 0;
  F32(v11, 16556) = (*v43__f);
  F32(v11, 16408) = 1;
  F32(v11, 8204) = 0;
  F16(v11, 16400) = 0;
  F32(v11, 16404) = 0;
  F32(v11, 16412) = 0;
  F32(v11, 16464) = 0;
  F32(v11, 16468) = 3;
  F32(v11, 16472) = 0;
  F16(v11, 8200) = 0;
  F32(v11, 16560) = 0;
  F32(v11, 16564) = 0;
  F32(v11, 16568) = 0;
  F32(v11, 16572) = 0;
  F32(v11, 16576) = 0;
  F32(v11, 16580) = 0;
  F32(v11, 16584) = 0;
  F32(v11, 16588) = 0;
  F32(v11, 16604) = 0;
  F32(v11, 16612) = 0;
  W_room_party_defaults((intptr_t)(v11));
  F32(v11, 16644) = 0;
  W_room_inputs_alloc((intptr_t)(v11), (intptr_t)(0));
  F32(v11, 16476) = 0;
  F16(v11, 16480) = 0;
  F16(v11, 16476) = 0;
  F16(v11, 16484) = 1;
  F16(v11, 16478) = 0;
  F16(v11, 16480) = 0;
  F16(v11, 16482) = 1;
  F32(v11, 16484) = 0;
  v15 = (_DWORD *)(v11 + 16648);
  for ( j = 3; j != 0; --j )
  {
    v15[1] = 0;
    *v15 = 1;
    v15 += 2;
  }
  F32(v11, 16488) = API_GetTickCount();
  F32(v11, 16492) = 0;
  F32(v11, 16644) = 3;
  F32(v11, 16412) = 0;
  W_room_party_level((intptr_t)(v11), (intptr_t)(1));
  v17 = 0;
  v18 = F32(CFG(), 2136);
  if ( ts_56dd80(v18, 0) != 0 )
  {
    v19 = 6 * v18;
    switch ( ((int32_t *)D(0x854080))[v19] )
    {
      case 50010:
        v20 = 2;
        break;
      case 50011:
        v20 = 5;
        break;
      case 50012:
        v20 = 3;
        break;
      default:
        v17 = 1;
        v20 = 0;
        break;
    }
    v21 = W_room_member_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(((int32_t *)D(0x854080))[v19]), (intptr_t)(((int32_t *)D(0x854084))[v19]), (intptr_t)(900000052), (intptr_t)(v20), (intptr_t)(1));
    if ( v21 != 0 )
    {
      v21[641] = F32(CFG(), 2208);
      v21[642] = F32(CFG(), 2236);
      v22 = ((int32_t *)D(0x854090))[v19];
      if ( v22 != 0 )
      {
        v21[666] = v22;
        v21[665] = v22;
      }
      v23 = ((int32_t *)D(0x854094))[v19];
      if ( v23 != 0 )
        v21[697] = v23;
      if ( v17 != 0 )
        W_mh_4e6520((intptr_t)(v21[635]), (intptr_t)(2), (intptr_t)(900000052));
      v24 = 0;
      v25 = F32(CFG(), 2164);
      if ( ts_56dd80(v25, 0) != 0 )
      {
        v26 = 6 * v25;
        switch ( ((int32_t *)D(0x854080))[v26] )
        {
          case 50010:
            v27 = 3;
            break;
          case 50011:
            v27 = 6;
            break;
          case 50012:
            v27 = 4;
            break;
          default:
            v24 = 1;
            v27 = 0;
            break;
        }
        v28 = W_room_member_add((intptr_t)(v11), (intptr_t)(1u), (intptr_t)(((int32_t *)D(0x854080))[v26]), (intptr_t)(((int32_t *)D(0x854084))[v26]), (intptr_t)(900000053), (intptr_t)(v27), (intptr_t)(1));
        if ( v28 != 0 )
        {
          v28[641] = F32(CFG(), 2208);
          v28[642] = F32(CFG(), 2236);
          v29 = ((int32_t *)D(0x854090))[v26];
          if ( v29 != 0 )
          {
            v28[666] = v29;
            v28[665] = v29;
          }
          v30 = ((int32_t *)D(0x854094))[v26];
          if ( v30 != 0 )
            v28[697] = v30;
          if ( v24 != 0 )
            W_mh_4e6520((intptr_t)(v28[635]), (intptr_t)(2), (intptr_t)(900000053));
        }
      }
      v31 = (int *)W_pslot_by_id((intptr_t)(v11), (intptr_t)(900000052), (intptr_t)(0));
      W_room_leader_set((intptr_t)(v11), (intptr_t)(v31));
      v32 = (intptr_t *)(v11 + 16432);
      for ( (*k__f) = 8; (*k__f) != 0; --(*k__f) )
      {
        v33 = *v32;
        if ( W_pslot_ok((intptr_t)(*v32), (intptr_t)(0), (intptr_t)(1)) )
          W_pslot_heal((intptr_t)(v33), (intptr_t)(1));
        ++v32;
      }
      F32(v11, 16468) = 4;
      W_room_formation((intptr_t)(v11));
      v34 = c;
      F32(v11, 16420) = (*v47__f);
      F32(v11, 16424) = 101;
      F32(v11, 16428) = 1;
      ts_585260((v11 + 16672), 2, 0, 1, F32(c, 0x14));
      F32(v11, 33084) = 0;
      W_room_party_level((intptr_t)((v11 + 16672)), (intptr_t)(1));
      (*v46__f) = 0;
      (*v48__f) = F32(CFG(), 2248);
      if ( ts_56dd80((*v48__f), 1) != 0 )
      {
        v35 = 6 * (*v48__f);
        (*v49__f) = v35 * 4;
        switch ( ((int32_t *)D(0x854080))[v35] )
        {
          case 50010:
            v36 = 2;
            break;
          case 50011:
            v36 = 5;
            break;
          case 50012:
            v36 = 3;
            break;
          default:
            (*v46__f) = 1;
            v36 = 0;
            break;
        }
        v37 = W_room_member_add((intptr_t)((v11 + 16672)), (intptr_t)(0), (intptr_t)(((int32_t *)D(0x854080))[v35]), (intptr_t)(((int32_t *)D(0x854084))[v35]), (intptr_t)(900000054), (intptr_t)(v36), (intptr_t)(1));
        if ( v37 != 0 )
        {
          v37[641] = F32(CFG(), 2320);
          v37[642] = F32(CFG(), 2348);
          v38 = F32(((uint8_t *)((int32_t *)D(0x854090)) + (*v49__f)), 0);
          if ( v38 != 0 )
          {
            v37[666] = v38;
            v37[665] = v38;
          }
          v39 = F32(((uint8_t *)((int32_t *)D(0x854094)) + (*v49__f)), 0);
          if ( v39 != 0 )
            v37[697] = v39;
          if ( (*v46__f) != 0 )
            W_mh_4e6520((intptr_t)(v37[635]), (intptr_t)(2), (intptr_t)(900000054));
          ts_585810((_WORD *)(v11 + 16672), 1u, 900000055, 0x51u, 0x52u, 0x53u, 1);
          v40 = (int *)W_pslot_by_id((intptr_t)((v11 + 16672)), (intptr_t)(900000054), (intptr_t)(0));
          W_room_leader_set((intptr_t)((v11 + 16672)), (intptr_t)(v40));
          F32(v11, 33092) = 0;
          F32(v11, 33096) = 102;
          F32(v11, 33100) = 3;
          W_room_party_heal((intptr_t)((v11 + 16672)), (intptr_t)(1));
          W_mh_4e61d0((intptr_t)((v11 + 16672)), (intptr_t)(4), (intptr_t)(0));
          W_room_inputs_alloc((intptr_t)(v11), (intptr_t)(1u));
          W_input_init((intptr_t)(v11), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0));
          F32(v11, 16472) = 1;
          if ( F32(CFG(), 2388) == 0 )
          {
            ts_585400((_DWORD *)v11, 0);
            W_pslot_input_set((intptr_t)(F32(v11, 8204)), (intptr_t)(F32(v11, 4)));
          }
          v50[1] = 0;
          v50[0] = v11 + 16672;
          ts_585590((int)v34, v11, 2, 50010, (int)v52);
          ts_585590((int)v34, v11, 2, 50011, (int)v51);
          ts_585590((int)v34, v11, 2, 50012, (int)v53);
          ts_585ae0(v34, (_DWORD *)v11, 30, 2, 2, v50);
          W_session_end((intptr_t)(v34));
          if ( F32(CFG(), 2388) == 0 )
            ts_585530((_DWORD *)v11);
        }
      }
    }
  }
  if ( v11 != 0 )
    ts_54c530(v11, 3);
  return 0;
}

/* 0x585260: DWORD __thiscall sub_585260(cls_5611D0 *this, int a2, int a3, __int16 a4, int a5) */
int32_t ts_585260(wl_char *self, int32_t a2, int32_t a3, int16_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    char * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t i = 0;
    uint32_t result = 0;

  v6 = 0;
  v7 = &GAP(c, 0x4070, 20);
  do
  {
    v8 = *((int32_t *)v7 - 21);
    v7 += 4;
    F16(v8, 2572) = v6;
    F32(v8, 2500) = 0;
    F32(v8, 2544) = 0;
    F32(v8, 4) = 0;
    F32(v8, 2548) = 0;
    F32(v8, 2552) = 0;
    F32(v8, 0) = 0;
    F16(v8, 2504) = 0;
    F32(v8, 8) = 0;
    *((int32_t *)v7 - 1) = 1;
    ++v6;
  }
  while ( (uint16_t)v6 < 8u );
  F32(c, 0x40a4) = a3;
  F16(c, 0x40a8) = a4;
  F32(c, 0x4018) = a2;
  F32(c, 0x200c) = 0;
  F16(c, 0x4010) = 0;
  F32(c, 0x4014) = 0;
  F32(c, 0x401c) = 0;
  F32(c, 0x4050) = 0;
  F32(c, 0x4054) = 3;
  F32(c, 0x4058) = 0;
  F32(c, 0x40ac) = a5;
  F16(c, 0x2008) = 0;
  F32(c, 0x40b0) = 0;
  F32(c, 0x40b4) = 0;
  F32(c, 0x40b8) = 0;
  F32(c, 0x40bc) = 0;
  F32(c, 0x40c0) = 0;
  F32(c, 0x40c4) = 0;
  F32(c, 0x40c8) = 0;
  F32(c, 0x40cc) = 0;
  F32(c, 0x40dc) = 0;
  F32(c, 0x40e4) = 0;
  F32(c, 0x40e0) = 0;
  F32(c, 0x40e8) = 0;
  F32(c, 0x40f0) = 255;
  F32(c, 0x40f4) = 255;
  F32(c, 0x40f8) = 255;
  F32(c, 0x40fc) = 255;
  F32(c, 0x4100) = 0;
  F32(c, 0x40ec) = 0;
  F32(c, 0x4104) = 0;
  W_room_inputs_alloc((intptr_t)(c), (intptr_t)(0));
  F32(c, 0x405c) = 0;
  F16(c, 0x4060) = 0;
  F16(c, 0x405c) = 0;
  F16(c, 0x4064) = 1;
  F16(c, 0x405c + 2) = 0;
  F16(c, 0x4060) = 0;
  F16(c, 0x4062) = 1;
  *(_DWORD *)&F16(c, 0x4064) = 0;
  v9 = c + 16648 * (1);
  for ( i = 3; i != 0; --i )
  {
    F32(v9, 0x4) = 0;
    *(_DWORD *)&F16(v9, 0x0) = 1;
    v9 = ((uint8_t *)v9 + 8);
  }
  result = API_GetTickCount();
  F32(c, 0x406c) = 0;
  F32(c, 0x4068) = result;
  return result;
}

/* 0x585400: int __stdcall sub_585400(cls_413810 *a1, int a2) */
int32_t ts_585400(int32_t a1, int32_t a2)
{

    intptr_t result = 0;
    intptr_t v3 = 0;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  result = F32(D(0x8a9518), 0);
  if ( F32(D(0x8a9518), 0) != 0 )
  {
    F32(((uint8_t *)(a1) + 0x4054), 0) = F32(D(0x8a9524), 0);
    W_room_formation((intptr_t)(a1));
    F32(a1, 0x4050) = F32(D(0x8a951c), 0);
    W_room_formation((intptr_t)(a1));
    result = a2;
    *(_DWORD *)&GAP(a1, 0x4054, 4) = F32(D(0x8a9520), 0);
    if ( a2 != 0 && F32(D(0x8a9528), 0) != 0 )
    {
      result = 0;
      while ( 1 )
      {
        v3 = *(_DWORD *)&GAP(a1, 0x4024, 4 * (uint16_t)result + 12);
        if ( F32(v3, 2544) == F32(D(0x8a9528), 0) )
          break;
        if ( (uint16_t)++result >= 8u )
          return result;
      }
      result = F32(v3, 2500) - 1;
      if ( F32(v3, 2500) == 1 && v3 != 0 )
      {
        v4 = (int *)F32(a1, 0x200c);
        v5 = 0;
        if ( v4 != 0 )
        {
          v5 = *v4;
          v6 = v4[625] - 1;
          *v4 = 0;
          if ( v6 == 0 && v4[636] != 100000104 )
          {
            v7 = v4[637];
            if ( v7 != 0 )
              W_sc_4cc470((intptr_t)(v7), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
          }
        }
        F32(v3, 0) = v5;
        result = W_pslot_ok((intptr_t)(v3), (intptr_t)(0), (intptr_t)(0));
        if ( result != 0 )
        {
          v8 = F32(v3, 2548);
          if ( v8 != 0 )
          {
            if ( F32(v3, 0) != 0 )
            {
              result = W_sc_4cc470((intptr_t)(v8), (intptr_t)(0), (intptr_t)(F32(v3, 0)), (intptr_t)(0));
              F32(a1, 0x200c) = v3;
              return result;
            }
            result = W_sc_4cc470((intptr_t)(v8), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
          }
        }
        F32(a1, 0x200c) = v3;
      }
    }
  }
  return result;
}

/* 0x585530: int __stdcall sub_585530(_DWORD *a1) */
int32_t ts_585530(int32_t * a1)
{

    intptr_t result = 0;

  F32(D(0x8a9518), 0) = 1;
  F32(D(0x8a951c), 0) = a1[4116];
  F32(D(0x8a9524), 0) = a1[4117];
  F32(D(0x8a9520), 0) = a1[4118];
  result = a1[2051];
  if ( result != 0 )
    F32(D(0x8a9528), 0) = F32(result, 2544);
  else
    F32(D(0x8a9528), 0) = 0;
  return result;
}

/* 0x585590: int __userpurge sub_585590@<eax>(int a1@<ebx>, int a2, int a3, int a4, int a5) */
int32_t ts_585590(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{

    intptr_t result = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    uint16_t j = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    uint16_t v17 = 0;
    int32_t * v18 = 0;
    char * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    char * v22 = 0;
    uint16_t m = 0;
    char * v24 = 0;
    int v25 = 0;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x30 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x20 */
    char * *const v28__f = (char * *)(frame_ + 0x14);   /* ebp-0x1c */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x18 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x14 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x10 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0xc */
    intptr_t *const k__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x8 */
    int32_t * *const v34__f = (int32_t * *)(frame_ + 0x2c);   /* ebp-0x4 */

  result = a3;
  v6 = 0;
  (*v27__f) = 0;
  for ( (*i__f) = 0; (uint16_t)(*i__f) < a3; v6 = (uint16_t)(*i__f) )
  {
    (*v31__f) = 8;
    v7 = (a2 + 16672 * v6);
    (*v28__f) = ((uint8_t *)(v7) + 0x4030);
    do
    {
      v8 = *(_DWORD **)(*v28__f);
      (*v34__f) = v8;
      if ( v8[625] == 1 )
      {
        for ( j = 0; ; ++j )
        {
          v10 = ((int32_t *)D(0x6b6ea8))[13 * j];
          if ( v10 == 0 )
            break;
          if ( v10 == v8[636] )
          {
            v11 = 13 * j;
            if ( 4 * v11 != -7040680 )
            {
              v12 = ((int32_t *)D(0x6b6eac))[v11];
              goto LABEL_11;
            }
            break;
          }
        }
        v12 = v8[1];
LABEL_11:
        if ( v12 == a4 )
        {
          v13 = 0;
          (*v30__f) = 0;
          v14 = 20 * (uint16_t)(*v27__f);
          for ( (*k__f) = v14; ; v14 = (*k__f) )
          {
            v15 = F32((a5 + 4 * (v14 + v13)), 0);
            if ( v15 == 0 )
              break;
            SETLO(a1, F16(v7, 0x4010));
            if ( (uint16_t)a1 < 0x200u )
            {
              v16 = 110010;
              v17 = 0;
              while ( v15 != v16 )
              {
                v16 = ((int32_t *)D(0x6c0cd0))[184 * ++v17];
                if ( v16 == 0 )
                  goto LABEL_26;
              }
              v18 = &((int32_t *)D(0x6c0cd0))[184 * v17];
              if ( v18 != 0 && (v18[171] >= 900 || W_room_items_of_kind((intptr_t)(v7), (intptr_t)(1), (intptr_t)(1)) < 48) )
              {
                v19 = &GAP(v7, 0x0, 16 * (uint16_t)a1);
                *(_DWORD *)&GAP(v7, 0x2010, 16 * (uint16_t)a1) = v15;
                F32(v19, 4 * 2055) = v18;
                F16(v19, 2 * 4108) = F16(v18, 2 * 50);
                F32(v19, 4 * 2053) = 0;
                v19[8218] = 0;
                if ( &(*v26__f) != (int *)-24 )
                  (*v29__f) = a1;
                ++F16(v7, 0x4010);
              }
            }
LABEL_26:
            v20 = (*v34__f)[636];
            v21 = 16 * (uint16_t)(*v29__f);
            a1 = *(_DWORD *)&GAP(v7, 0x2010, v21 + 12);
            v22 = &GAP(v7, 0x0, v21);
            if ( v20 != 0 )
            {
              for ( m = 0; m < F16(v7, 0x4010); ++m )
              {
                v24 = &GAP(v7, 0x0, 16 * m);
                if ( F32(v24, 4 * 2053) == v20
                  && F32(F32(v24, 4 * 2055), 120) == F32(a1, 120) )
                {
                  F32(v24, 4 * 2053) = 0;
                }
              }
            }
            F32(v22, 4 * 2053) = v20;
            v13 = ++(*v30__f);
            if ( (uint16_t)(*v30__f) >= 0x14u )
              break;
          }
          ++(*v27__f);
        }
      }
      v25 = (*v31__f) == 1;
      (*v28__f) += 4;
      --(*v31__f);
    }
    while ( !v25 );
    result = a3;
    ++(*i__f);
  }
  return result;
}

/* 0x585810: int __stdcall sub_585810( cls_4066D0 *a1, unsigned __int16 a2, int a3, unsigned __int16 a4, unsigned __int16 a5, unsigned __int16 a6, int a7) */
int32_t ts_585810(int32_t a1, uint16_t a2, int32_t a3, uint16_t a4, uint16_t a5, uint16_t a6, int32_t a7)
{

    intptr_t v7 = 0;
    intptr_t v8 = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    uint16_t v25 = 0;
    int32_t * v26 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v7 = a4;
  v28 = 0;
  v8 = F32(CFG() + 28 * v7, 8);
  v9 = 0;
  if ( v8 == 0xFFFF || v8 < 0 )
    return 0;
  if ( ((int32_t *)D(0x854080))[0] != 0 )
  {
    do
      ++v9;
    while ( ((int32_t *)D(0x854080))[6 * v9] != 0 );
  }
  if ( v8 >= v9 )
    return 0;
  v10 = 6 * v8;
  (*v27__f) = 6 * v8;
  if ( ((int32_t *)D(0x85408c))[6 * v8] != 0 && a7 == 0 )
    return 0;
  v11 = 1;
  v12 = ((int32_t *)D(0x854084))[v10];
  v13 = 0;
  v14 = 0;
  while ( v11 != ((int32_t *)D(0x854080))[v10] || ((int32_t *)D(0x67ac5c))[v14] != v12 && v12 != 0 )
  {
    v14 = 55 * ++v13;
    v11 = ((int32_t *)D(0x67ac58))[v14];
    if ( v11 == 0 )
    {
      v15 = (int32_t *)D(0x67ac58);
      goto LABEL_14;
    }
  }
  v15 = (int *)(220 * v13 + 6794328);
LABEL_14:
  if ( v15 == 0 || ((int32_t *)D(0x854088))[v10] > F32(CFG(), 2780) )
    return 0;
  v17 = ((int32_t *)D(0x854080))[v10];
  v29 = v17;
  switch ( v17 )
  {
    case 50010:
      v18 = a2 + 2;
      break;
    case 50011:
      v18 = a2 + 5;
      break;
    case 50012:
      v18 = a2 + 3;
      break;
    default:
      v28 = 1;
      v18 = 0;
      break;
  }
  v19 = 0;
  while ( 1 )
  {
    v20 = *(intptr_t *)&GAP(a1, 0x4030, 4 * v19);
    if ( F32(v20, 0x9f0) == a3 )
      break;
    if ( ++v19 >= 8u )
      goto LABEL_32;
  }
  if ( !W_pslot_ok((intptr_t)(v20), (intptr_t)(1), (intptr_t)(1)) || v20 == 0 )
  {
    v17 = v29;
LABEL_32:
    v21 = *(_DWORD *)&GAP(a1, 0x4030, 4 * a2);
    W_pslot_init((intptr_t)(v21), (intptr_t)((int)a1), (intptr_t)(v17), (intptr_t)(v12), (intptr_t)(a3), (intptr_t)(1), (intptr_t)(v18));
    W_room_items_grant((intptr_t)(a1), (intptr_t)(a3));
    W_room_party_level((intptr_t)(a1), (intptr_t)(1));
    if ( v21 != 0 )
    {
      F32(v21, 2564) = F32(CFG() + 28 * a5, 24);
      F32(v21, 2568) = F32(CFG() + 28 * a6, 24);
      v22 = ((int32_t *)D(0x854090))[(*v27__f)];
      if ( v22 != 0 )
      {
        F32(v21, 2664) = v22;
        F32(v21, 2660) = v22;
      }
      v23 = ((int32_t *)D(0x854094))[(*v27__f)];
      if ( v23 != 0 )
        F32(v21, 2788) = v23;
      if ( v28 != 0 )
      {
        v24 = F32(v21, 2540);
        if ( a3 != 0 )
        {
          v25 = 0;
          while ( 1 )
          {
            v26 = *(_DWORD **)&GAP(v24, 0x4024, 4 * v25 + 12);
            if ( (v26[625] == 1 || v26[625] == 2) && a3 == v26[636] )
              break;
            if ( ++v25 >= 8u )
              goto LABEL_48;
          }
          v26[639] = 2;
        }
        else
        {
          F32(v24, 0x4050) = 2;
        }
LABEL_48:
        W_room_formation((intptr_t)(v24));
      }
      return 1;
    }
  }
  return 0;
}

/* 0x585ae0: int __thiscall sub_585AE0(cls_585AE0 *this, _DWORD *a2, int a3, int a4, int a5, _DWORD *a6) */
int32_t ts_585ae0(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6)
{
    intptr_t c = (intptr_t)self;
    int32_t * v7 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    intptr_t v19 = 0;
    intptr_t room_586010 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    void * v24 = 0;
    uint8_t * v25 = 0;
    intptr_t j = 0;
    intptr_t v27 = 0;
    void * v28 = 0;
    uint8_t * v29 = 0;
    intptr_t i = 0;
    intptr_t v31 = 0;
    int32_t * v38 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int32_t * *const v33__f = (int32_t * *)(frame_ + 0x0);   /* ebp-0x2c */
    uint16_t *const v34__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x28 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x24 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x20 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x1c */

  v7 = 0;
  (*v35__f) = 0;
  (*v34__f) = 0;
  (*v33__f) = 0;
  if ( a5 == 1 )
  {
    (*v33__f) = a6;
    v7 = a6;
  }
  while ( 1 )
  {
    if ( v7 != 0 )
    {
      if ( (unsigned int)(*v34__f) >= v7[23] )
        break;
      v7[26] = v7[(*v34__f) + 3];
    }
    v9 = F32(c, 0x10);
    (*v36__f) = a2[4107];
    v10 = 0;
    (*v37__f) = a2[4106];
    if ( F16(v9, 344068) == 0 )
      break;
    while ( F32(336 * v10 + v9, 4) != a2[4105] )
    {
      if ( ++v10 >= F16(v9, 344068) )
        goto LABEL_39;
    }
    v11 = (int *)(336 * v10 + v9 + 4);
    v38 = v11;
    if ( 336 * v10 + v9 == -4 )
      goto LABEL_39;
    v12 = (intptr_t)g_eng.op_new(0x910u);
    if ( v12 != 0 )
    {
      *(intptr_t *)g_eng.world = 0;
      F32(v12, 0xe8) = 0;
      v13 = (intptr_t)g_eng.op_new(0x24u);
      if ( v13 != 0 )
      {
        v14 = (intptr_t)g_eng.op_new(0xCu);
        if ( v14 != 0 )
          v13[3] = W_sv_561cf0((intptr_t)(v14), (intptr_t)(40));
        else
          v13[3] = 0;
      }
      else
      {
        v13 = 0;
      }
      *(_DWORD *)&GAP(v12, 0xf0, 44) = v13;
      v15 = (intptr_t)g_eng.op_new(0x24u);
      if ( v15 != 0 )
      {
        v16 = (intptr_t)g_eng.op_new(0xCu);
        if ( v16 != 0 )
          v15[3] = W_sv_561cf0((intptr_t)(v16), (intptr_t)(40));
        else
          v15[3] = 0;
      }
      else
      {
        v15 = 0;
      }
      *(_DWORD *)&GAP(v12, 0xf0, 48) = v15;
      v17 = (intptr_t)g_eng.op_new(0x24u);
      if ( v17 != 0 )
        v18 = ts_585e70(v17);
      else
        v18 = 0;
      *(_DWORD *)&GAP(v12, 0xf0, 52) = v18;
      F32(v12, 0xe0) = 0;
      memset(((uint8_t *)(v12) + 0x14), 0, 0x2Cu);
      v11 = v38;
      v19 = v12;
    }
    else
    {
      v19 = 0;
    }
    F32(c, 0x0) = (int)v19;
    room_586010 = W_load_room((intptr_t)(v19), (intptr_t)((int)a2), (intptr_t)(a4), (intptr_t)(F32(c, 0x10)), (intptr_t)(v11), (intptr_t)((*v37__f)), (intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(F32(c, 0x14)), (intptr_t)(20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a5), (intptr_t)((int)a6), (intptr_t)(a3), (intptr_t)(a2[4161]));
    v21 = room_586010;
    if ( room_586010 <= 0 || room_586010 > 2 )
    {
      ts_59d690(F32(c, 0x0));
      v28 = (void *)F32(c, 0x0);
      if ( F32(c, 0x0) != 0 )
      {
        W_roomloop_free((intptr_t)(F32(c, 0x0)));
        g_eng.op_delete((void *)v28);
        F32(c, 0x0) = 0;
      }
      v29 = &SYSB(9904);
      for ( i = 25; i != 0; --i )
      {
        v31 = F32(v29, 0);
        v29 += 4;
        F32(v31, 16) = 0;
      }
      API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2c8c))));
      W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(3u));
      API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2c7c))));
      W_frame_cache_clear((intptr_t)(SYS32(10068)));
      (*v35__f) = v21;
LABEL_39:
      v7 = (*v33__f);
      break;
    }
    v22 = F32(F32(c, 0x0), 2312);
    v23 = F32(F32(c, 0x0), 2304);
    a2[4106] = F32(F32(c, 0x0), 2308);
    a2[4105] = v23;
    a2[4107] = v22;
    ts_59d690(F32(c, 0x0));
    v24 = (void *)F32(c, 0x0);
    if ( F32(c, 0x0) != 0 )
    {
      W_roomloop_free((intptr_t)(F32(c, 0x0)));
      g_eng.op_delete((void *)v24);
      F32(c, 0x0) = 0;
    }
    v25 = &SYSB(9904);
    for ( j = 25; j != 0; --j )
    {
      v27 = F32(v25, 0);
      v25 += 4;
      F32(v27, 16) = 0;
    }
    API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2c8c))));
    W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(3u));
    API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2c7c))));
    W_frame_cache_clear((intptr_t)(SYS32(10068)));
    v7 = (*v33__f);
    ++(*v34__f);
  }
  if ( v7 == 0 || v7[30] != 0 )
    return (*v35__f);
  else
    return 0;
}

/* 0x585e70: _DWORD *__thiscall sub_585E70(cls_585E70 *this) */
int32_t ts_585e70(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;
    intptr_t v3 = 0;

  v2 = (intptr_t)g_eng.op_new(0xCu);
  v3 = (int)v2;
  if ( v2 != 0 )
  {
    v2[1] = 40;
    *v2 = (intptr_t)g_eng.op_new(0x5A0u);
    F16(v3, 8) = 0;
    F16(v3, 10) = 0;
    F32(c, 0xc) = v3;
  }
  else
  {
    F32(c, 0xc) = 0;
  }
  return ((uint8_t *)(c) + 0x0);
}

/* 0x59d160: void sub_59D160() */
void ts_59d160(void)
{

    intptr_t v0 = 0;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const c_l__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  W_rd_491820((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(((char *)D(0x8a3ebc))), (intptr_t)(1u), (intptr_t)(9), (intptr_t)(320 - ((9 * strlen(((char *)D(0x8a3ebc)))) >> 1)), (intptr_t)(80), (intptr_t)(1), (intptr_t)(5), (intptr_t)(0), (intptr_t)(0));
  W_rd_491820((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(((char *)D(0x8a3eac))), (intptr_t)(1u), (intptr_t)(9), (intptr_t)(320 - ((9 * strlen(((char *)D(0x8a3eac)))) >> 1)), (intptr_t)(100), (intptr_t)(1), (intptr_t)(5), (intptr_t)(0), (intptr_t)(0));
  v0 = 320 - ((5 * strlen(((char *)D(0x8a3e80)))) >> 1);
  (*c_l__f) = F32(SYS32(10348), 364);
  v1 = strlen(((char *)D(0x8a3e80)));
  if ( W_rp_5b94e0((intptr_t)((intptr_t)(*c_l__f)), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
  {
    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10012)));
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
    (*v8__f) = -1;
    do
    {
      v2 = -1;
      do
      {
        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v2 + v0), (intptr_t)((*v8__f) + 140), (intptr_t)(((char *)D(0x8a3e80))), (intptr_t)(v1));
        ++v2;
      }
      while ( v2 <= 1 );
      ++(*v8__f);
    }
    while ( (*v8__f) <= 1 );
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v0), (intptr_t)(140), (intptr_t)(((char *)D(0x8a3e80))), (intptr_t)(v1));
    W_rp_5b9500((intptr_t)((intptr_t)(*c_l__f)), (intptr_t)((intptr_t)(*hdc__f)));
  }
  v3 = 320 - ((4 * strlen((const char *)D(0x855294))) >> 1);
  v4 = *(intptr_t *)(SYS32(10348) + 364);
  (*c_l__f) = strlen((const char *)D(0x855294));
  (*v10__f) = v4;
  if ( W_rp_5b94e0((intptr_t)(v4), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
  {
    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10012)));
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
    (*v8__f) = -1;
    do
    {
      v5 = -1;
      v6 = (*v8__f) + 160;
      do
      {
        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v5 + v3), (intptr_t)(v6), (intptr_t)((char *)D(0x855294)), (intptr_t)((*c_l__f)));
        ++v5;
      }
      while ( v5 <= 1 );
      ++(*v8__f);
    }
    while ( (*v8__f) <= 1 );
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v3), (intptr_t)(160), (intptr_t)((char *)D(0x855294)), (intptr_t)((*c_l__f)));
    W_rp_5b9500((intptr_t)((*v10__f)), (intptr_t)((intptr_t)(*hdc__f)));
  }
}

/* 0x59d690: void __thiscall sub_59D690(cls_59D690 *this) */
void ts_59d690(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t j = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t k = 0;
    uint16_t m = 0;
    intptr_t v13 = 0;
    void * v14 = 0;
    int v15 = 0;
    intptr_t v16 = 0;
    intptr_t n = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t ii = 0;
    intptr_t v26 = 0;
    intptr_t jj = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t kk = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    uint16_t mm = 0;
    intptr_t v34 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */

  if ( F32(c, 0x0) != 0 )
  {
    for ( i = 0; i < F16(c, 0x4); ++i )
    {
      v3 = 8;
      v4 = 16672 * i + 16432;
      do
      {
        v4 += 4;
        --v3;
        F32(F32((v4 + F32(c, 0x0) - 4), 0), 2548) = 0;
      }
      while ( v3 != 0 );
    }
  }
  v5 = 4192;
  for ( j = 64; j != 0; --j )
  {
    W_entity_release((intptr_t)(*(_DWORD *)&GAP(W, 0x0, v5)), (intptr_t)(/*edx*/ 0));
    v5 += 4;
  }
  (*v35__f) = 4448;
  (*v36__f) = 32;
  do
  {
    v8 = *(_DWORD *)&GAP(W, 0x0, (*v35__f));
    v9 = F32(v8, 51096);
    if ( v9 != 0 )
      W_char_carry((intptr_t)(v9), (intptr_t)(0));
    if ( F32(v8, 51108) != 0 )
      W_char_carry((intptr_t)(v8), (intptr_t)(0));
    v10 = (int *)(v8 + 51236);
    for ( k = 2; k != 0; --k )
    {
      if ( *v10 != 0 )
      {
        W_entity_release((intptr_t)(*v10), (intptr_t)(/*edx*/ 1));
        *v10 = 0;
      }
      ++v10;
    }
    for ( m = 0; m < 0xAu; ++m )
      W_char_part_release((intptr_t)(v8), (intptr_t)(m));
    W_entity_release((intptr_t)(v8), (intptr_t)(/*edx*/ 0));
    v14 = F32(v8, 88916);
    if ( v14 != 0 )
    {
      W_voice_set_free((intptr_t)(F32(v8, 88916)));
      g_eng.op_delete((void *)v14);
      F32(v8, 88916) = 0;
    }
    v15 = (*v36__f) == 1;
    (*v35__f) += 4;
    --(*v36__f);
  }
  while ( !v15 );
  v16 = 4576;
  for ( n = 128; n != 0; --n )
  {
    v18 = W;
    v19 = *(_DWORD *)&GAP(W, 0x0, v16);
    v20 = F32(v19, 504);
    if ( v20 != 0 )
    {
      W_actor_release((intptr_t)(v20));
      F32(v19, 504) = 0;
    }
    W_wl_list_remove((intptr_t)(*(_DWORD *)&GAP(W, 0x2788, 16)), (intptr_t)(/*edx*/ v19));
    W_entity_unlist((intptr_t)(v19 /*edx*/));
    W_entity_release((intptr_t)(v19), (intptr_t)(/*edx*/ 1));
    if ( F32(v19, 680) != 0 )
    {
      g_eng.op_delete((void *)F32(v19, 680));
      F32(v19, 680) = 0;
    }
    v16 += 4;
  }
  v24 = 5088;
  for ( ii = 1024; ii != 0; --ii )
  {
    W_entity_release((intptr_t)(*(_DWORD *)&GAP(W, 0x0, v24)), (intptr_t)(/*edx*/ 1));
    v24 += 4;
  }
  v26 = 9184;
  for ( jj = 96; jj != 0; --jj )
  {
    W_entity_release((intptr_t)(*(_DWORD *)&GAP(W, 0x0, v26)), (intptr_t)(/*edx*/ 0));
    v26 += 4;
  }
  v29 = 9568;
  for ( kk = 128; kk != 0; --kk )
  {
    W_entity_release((intptr_t)(*(_DWORD *)&GAP(W, 0x0, v29)), (intptr_t)(/*edx*/ 0));
    v29 += 4;
  }
  v31 = W;
  if ( W != 0 )
  {
    W_world_free((intptr_t)(W));
    g_eng.op_delete((void *)v31);
    *(intptr_t *)g_eng.world = 0;
  }
  v32 = F32(c, 4 * 58);
  if ( v32 != 0 )
  {
    for ( mm = 0; mm < F16(v32, 4); ++mm )
    {
      v34 = F32(F32(v32, 0), 4 * mm);
      if ( v34 != 0 )
      {
        ts_59d900(v34, 1);
        F32((F32(v32, 0) + 4 * mm), 0) = 0;
      }
    }
    if ( F32(v32, 0) != 0 )
    {
      g_eng.op_delete((void *)F32(v32, 0));
      F32(v32, 0) = 0;
    }
    g_eng.op_delete((void *)(void *)v32);
    F32(c, 4 * 58) = 0;
  }
}

/* 0x59d900: void *__thiscall sub_59D900(cls_59D900 *this, char a2) */
int32_t ts_59d900(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    void ** v5 = 0;
    intptr_t * v6 = 0;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4 */

  v2 = c;
  if ( *(_DWORD *)&GAP(c, 0x28, 32) != 0 )
  {
    v3 = 0;
    (*v20__f) = 0;
    if ( F16(c, 0x4c) != 0 )
    {
      do
      {
        v4 = 4 * (uint16_t)v3;
        (*v21__f) = v4;
        v5 = *(void ***)(v4 + *(_DWORD *)&GAP(v2, 0x28, 32));
        if ( v5 != 0 )
        {
          v6 = (intptr_t *)v5[92];
          if ( v6 != 0 )
          {
            v7 = *v6;
            if ( *v6 != 0 )
            {
              W_tm_49a6e0((intptr_t)(*v6));
              g_eng.op_delete((void *)v7);
              *(_DWORD *)v5[92] = 0;
            }
            v2 = c;
          }
          if ( v5[92] != 0 )
          {
            g_eng.op_delete((void *)v5[92]);
            v5[92] = 0;
          }
          if ( v5[89] != 0 )
          {
            g_eng.op_delete((void *)v5[89]);
            v5[89] = 0;
          }
          if ( v5[94] != 0 )
          {
            g_eng.op_delete((void *)v5[94]);
            v5[94] = 0;
          }
          if ( v5[95] != 0 )
          {
            v8 = (int *)v5[93];
            (*v19__f) = 0;
            if ( v8[1] > 0 )
            {
              v9 = 0;
              do
              {
                v10 = 16 * v9;
                if ( F32((char *)v5[95] + v10, 12) != 0 )
                {
                  (*v18__f) = 0;
                  if ( v8[2] > 0 )
                  {
                    v11 = 0;
                    do
                    {
                      v12 = 24 * v11;
                      v13 = F32(F32((char *)v5[95] + v10, 12), 24 * v11);
                      if ( v13 != 0 )
                      {
                        W_en_56c900((intptr_t)(v13), (intptr_t)(1));
                        F32((F32((char *)v5[95] + v10, 12) + v12), 0) = 0;
                      }
                      v14 = F32((char *)v5[95] + v10, 12);
                      if ( F32(v14 + v12, 4) != 0 )
                      {
                        g_eng.op_delete((void *)F32(v14, v12 + 4));
                        F32(F32(((char *)v5[95] + v10 + 12), 0) + v12, 4) = 0;
                      }
                      v15 = F32((char *)v5[95] + v10, 12);
                      if ( F32(v15 + v12, 8) != 0 )
                      {
                        g_eng.op_delete((void *)F32(v15, v12 + 8));
                        F32(F32(((char *)v5[95] + v10 + 12), 0) + v12, 8) = 0;
                      }
                      v11 = (uint16_t)++(*v18__f);
                    }
                    while ( (uint16_t)(*v18__f) < *((int *)v5[93] + 2) );
                    v4 = (*v21__f);
                  }
                }
                if ( F32((char *)v5[95] + v10, 12) != 0 )
                {
                  g_eng.op_delete((void *)F32((char *)v5[95], v10 + 12));
                  F32((char *)v5[95] + v10, 12) = 0;
                }
                v8 = (int *)v5[93];
                v9 = (uint16_t)++(*v19__f);
              }
              while ( (uint16_t)(*v19__f) < v8[1] );
              v2 = c;
            }
            if ( v5[95] != 0 )
            {
              g_eng.op_delete((void *)v5[95]);
              v5[95] = 0;
            }
          }
          if ( v5[93] != 0 )
          {
            g_eng.op_delete((void *)v5[93]);
            v5[93] = 0;
          }
          g_eng.op_delete((void *)v5);
          v3 = (*v20__f);
          F32((v4 + *(_DWORD *)&GAP(v2, 0x28, 32)), 0) = 0;
        }
        (*v20__f) = ++v3;
      }
      while ( (uint16_t)v3 < F16(v2, 0x4c) );
    }
    if ( *(_DWORD *)&GAP(v2, 0x28, 32) != 0 )
    {
      g_eng.op_delete((void *)*(void **)&GAP(v2, 0x28, 32));
      *(_DWORD *)&GAP(v2, 0x28, 32) = 0;
    }
  }
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)v2);
  return v2;
}

/* 0x59dc60: int __thiscall sub_59DC60(cls_59DC60 *this) */
int32_t ts_59dc60(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    int32_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t * v8 = 0;
    intptr_t v9 = 0;
    _Alignas(8) uint8_t frame_[0xbc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v24 = (intptr_t *)(frame_ + 0x54);   /* [23] at ebp-0x68 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xbc */
    intptr_t *const Block__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xa8 */
    intptr_t * *const v13__f = (intptr_t * *)(frame_ + 0x24);   /* ebp-0x98 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x94 */
    int32_t * *const v15__f = (int32_t * *)(frame_ + 0x30);   /* ebp-0x8c */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x88 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x84 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x80 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x7c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x78 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x74 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x70 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x6c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0xb8);   /* ebp-0x4 */

  (*v13__f) = (intptr_t *)c;
  F32(Block__f, 0x0) = 0;
  F32(Block__f, 0x4) = 0;
  (*v19__f) = 2;
  (*v20__f) = 2;
  (*v21__f) = 2;
  (*v22__f) = 2;
  (*v23__f) = 2;
  (*v16__f) = 200110;
  (*v17__f) = 101;
  (*v18__f) = 1;
  (*v25__f) = 0;
  memset(v24, 0, 0x3Cu);
  v24[15] = 5;
  memset(&v24[16], 0, 28);
  while ( 1 )
  {
    W_tm_5644a0((intptr_t)(&(*Block__f)));
    if ( F32(CFG(), 2780) >= 200 )
    {
      ts_584420(&(*Block__f), 32, 392);
      v1 = W_tm_564780((intptr_t)(&(*Block__f)), (intptr_t)(37));
      W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
      if ( F32(Block__f, 0x8) != 0 )
      {
        F32(F32(Block__f, 0x8), 80) = 0;
        F32(Block__f, 0x8) = 0;
        F32(Block__f, 0xc) = 0;
      }
LABEL_7:
      if ( v1 == 6 )
        goto LABEL_9;
      goto LABEL_8;
    }
    v1 = 11;
    v2 = ts_565b00(&(*Block__f), ((char *)D(0x8a2c58)), 0, 0, 1, 0x28u, 2u);
    if ( v2 != 0 )
    {
      v1 = v2;
      goto LABEL_7;
    }
LABEL_8:
    W_tm_565d10((intptr_t)(&(*Block__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0x14u));
LABEL_9:
    ts_56ad50(&(*Block__f));
    if ( v1 != 0 )
      break;
    v3 = (intptr_t)g_eng.op_new(0x18u);
    if ( v3 != 0 )
    {
      v4 = v3;
      F32(((uint8_t *)(v3) + 0x4), 0) = 0;
      *(_DWORD *)&GAP(v3, 0x4, 4) = 0;
      *(_DWORD *)&GAP(v3, 0x4, 8) = 0;
      F32(v3, 0x0) = 0;
      F32(v3, 0x10) = 0;
      F32(v3, 0x14) = 0;
    }
    else
    {
      v4 = 0;
    }
    *(*v13__f) = v4;
    ts_59e1d0(v4);
    ts_59df50(F32(v4, 0x10), D(0x693848), ((int32_t *)D(0x6940c8)), 0);
    v5 = F32(v4, 0x14);
    F32(v5, 6144) = 0;
    F32(v5, 30724) = 0;
    v6 = (intptr_t)g_eng.op_new(0x8244u);
    (*v15__f) = v6;
    SETLB((*v25__f), 1);
    if ( v6 != 0 )
    {
      v7 = (v6 + 1);
      *v6 = 2;
      eh_vec_ctor(
        v6 + 1,
        0x4120u,
        2,
        (void *)((intptr_t)D(0x5851a0) /* el_5851a0 by address */),
        (void *)((intptr_t)D(0x54c5e0) /* el_54c5e0 by address */));
    }
    else
    {
      v7 = 0;
    }
    (*v11__f) = F32(v4, 0x14);
    SETLB((*v25__f), 0);
    ts_585260(v7, 1, 1, 0, (*v11__f));
    F32(v7, 0x4104) = 1;
    W_room_area_enter((intptr_t)(&(*v14__f)), (intptr_t)(0), (intptr_t)((int)v7));
    if ( F32(v7, 0x200c) != 0 )
    {
      W_room_inputs_alloc((intptr_t)(v7), (intptr_t)(1u));
      W_input_init((intptr_t)(v7), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0));
      if ( (*v16__f) != 0 )
        W_room_next_set((intptr_t)(v7), (intptr_t)((*v16__f)), (intptr_t)((*v17__f)), (intptr_t)((*v18__f)));
      W_room_area_enter((intptr_t)(&(*v14__f)), (intptr_t)(1), (intptr_t)((int)v7));
      F32(v7, 0x4104) = 1;
      ts_585ae0(v4, v7, 40, 1, 1, &(*v16__f));
      W_session_end((intptr_t)(v4));
    }
    if ( v7 != 0 )
      ts_54c530(v7, 3);
    v8 = (*v13__f);
    v9 = *(*v13__f);
    if ( *(*v13__f) != 0 )
    {
      W_session_end((intptr_t)(*(*v13__f)));
      g_eng.op_delete((void *)v9);
      *v8 = 0;
    }
    W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  }
  (*v25__f) = -1;
  if ( F32(Block__f, 0x0) != 0 )
  {
    W_pool_head_delete((intptr_t)(F32(Block__f, 0x0)), (intptr_t)(1));
    F32(Block__f, 0x0) = 0;
  }
  if ( F32(Block__f, 0x4) != 0 )
    W_fm_56c430((intptr_t)(F32(Block__f, 0x4)), (intptr_t)(1));
  return v1;
}

/* 0x59df50: int __thiscall sub_59DF50(cls_59DF50 *this, int *a2, int *a3, int *a4) */
int32_t ts_59df50(wl_char *self, int32_t * a2, int32_t * a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    int32_t * v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    char * v8 = 0;
    int32_t * v9 = 0;
    uint16_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    int32_t * v14 = 0;
    int32_t * v15 = 0;
    intptr_t v16 = 0;
    int16_t v17 = 0;
    int16_t v18 = 0;
    uint16_t v19 = 0;
    uint16_t v20 = 0;
    char * v21 = 0;
    int v22 = 0;
    int32_t * v26 = 0;
    int32_t * v27 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */

  v4 = c;
  v5 = a3;
  F32(c, 0x0) = (int)a2;
  F32(c, 0x54008) = (int)a4;
  *(_WORD *)&GAP(c, 0x4, 344064) = 0;
  result = *a3;
  if ( *a3 != 0 )
  {
    do
      result = 336 * (uint16_t)++*(_WORD *)&GAP(c, 0x4, 344064);
    while ( F32(((uint8_t *)a3 + result), 0) != 0 );
  }
  v27 = 0;
  if ( *(_WORD *)&GAP(c, 0x4, 344064) != 0 )
  {
    do
    {
      v7 = 84 * (uint16_t)v27;
      v8 = &GAP(v4, 0x4, v7 * 4);
      v9 = &v5[v7];
      v10 = 0;
      qmemcpy(v8, v9, 0x150u);
      v11 = (_DWORD *)F32(v4, 0x0);
      v12 = F32(F32(v4, 0x0), 0);
      if ( v12 != 0 )
      {
        while ( v12 != F32(v8, 4 * 1) )
        {
          v12 = v11[16 * ++v10];
          if ( v12 == 0 )
            goto LABEL_39;
        }
        v13 = &v11[16 * v10];
        if ( v13 != 0 )
        {
          if ( F32(v8, 4 * 68) == 0 )
            F32(v8, 4 * 68) = v13[11];
          if ( F32(v8, 4 * 67) == 0 )
            F32(v8, 4 * 67) = v13[13];
          if ( F32(v8, 4 * 69) == 0 )
            F32(v8, 4 * 69) = v13[12];
          if ( F32(v8, 4 * 80) == 0 )
            F32(v8, 4 * 80) = v13[14];
          if ( F16(v8, 2 * 162) == 0 )
            F16(v8, 2 * 162) = F16(v13, 2 * 30);
          if ( F16(v8, 2 * 163) == 0 )
            F16(v8, 2 * 163) = F16(v13, 2 * 31);
          if ( v5 != 0 && *(_WORD *)&GAP(v4, 0x4, 344064) != 0 )
          {
            v14 = v5;
            v26 = v5;
            (*v24__f) = *(uint16_t *)&GAP(v4, 0x4, 344064);
            do
            {
              if ( *v14 != F32(v8, 0) )
              {
                v15 = v14 + 7;
                (*v23__f) = 20;
                while ( 1 )
                {
                  if ( v15[1] == F32(v8, 0) )
                  {
                    v16 = *v14;
                    v17 = F16(v15, 0);
                    v18 = F16(v15, 2 * 4);
                    v19 = 0;
                    while ( F16(&v8[12 * v19 + 28], 0) != v18 )
                    {
                      if ( ++v19 >= 0x14u )
                      {
                        v20 = 0;
                        while ( F16(&v8[12 * v20 + 28], 0) != 0 )
                        {
                          if ( ++v20 >= 0x14u )
                            goto LABEL_36;
                        }
                        v21 = &v8[12 * v20];
                        F16(v21, 2 * 14) = v18;
                        F32(v21, 4 * 8) = v16;
                        F16(&v8[12 * v20 + 36], 0) = v17;
                        F16(v21, 2 * 19) = 0;
                        break;
                      }
                    }
                  }
LABEL_36:
                  v15 += 3;
                  if ( --(*v23__f) == 0 )
                    break;
                  v14 = v26;
                }
                v5 = a3;
                v14 = v26;
                v4 = c;
              }
              v14 += 84;
              v22 = (*v24__f) == 1;
              v26 = v14;
              --(*v24__f);
            }
            while ( !v22 );
          }
        }
      }
LABEL_39:
      result = (int)v27 + 1;
      v27 = (int *)((uint8_t *)v27 + 1);
    }
    while ( (uint16_t)v27 < *(_WORD *)&GAP(v4, 0x4, 344064) );
  }
  return result;
}

/* 0x59e1d0: int __thiscall sub_59E1D0(cls_59E1D0 *this) */
int32_t ts_59e1d0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    char * v2 = 0;
    intptr_t i = 0;
    int32_t * v4 = 0;
    intptr_t result = 0;

  F32(c, 0x10) = (int)(intptr_t)g_eng.op_new(0x5400Cu);
  F32(c, 0x14) = (int)(intptr_t)g_eng.op_new(0x7808u);
  v2 = &GAP(c, 0x0, 4);
  for ( i = 3; i != 0; --i )
  {
    v4 = (intptr_t)g_eng.op_new(0x88u);
    F32(v2, 0) = v4;
    *v4 = 0;
    result = F32(v2, 0);
    v2 += 4;
    F16(result, 132) = 0;
  }
  F16(D(0x8a9400), 0) = 0;
  F32(D(0x8a9408), 0) = 0;
  return result;
}

/* 0x59e230: int __thiscall sub_59E230(cls_59E230 *this) */
int32_t ts_59e230(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t * v3 = 0;
    intptr_t v4 = 0;
    int32_t * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint32_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x170 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v25 = (intptr_t *)(frame_ + 0x4c);   /* [60] at ebp-0x124 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x170 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x15c */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14c */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x148 */
    int32_t * *const v18__f = (int32_t * *)(frame_ + 0x30);   /* ebp-0x140 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x13c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x138 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x134 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x130 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x12c */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x128 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x13c);   /* ebp-0x34 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x16c);   /* ebp-0x4 */

  (*v16__f) = c;
  (*v27__f) = 1;
  memset(&(*v15__f), 0, 12);
  v25[29] = 200110;
  v25[30] = 101;
  v25[31] = 1;
  v25[32] = 2;
  v25[33] = 2;
  v25[34] = 2;
  (*v19__f) = 200120;
  memset(&v25[35], 0, 0x44u);
  (*v20__f) = 101;
  (*v21__f) = 1;
  (*v22__f) = 2;
  (*v23__f) = 2;
  (*v24__f) = 2;
  memset(v25, 0, 0x44u);
  v25[52] = 3;
  v25[53] = 3000;
  memset(&v25[54], 0, 24);
  v25[17] = 3;
  v25[18] = 3000;
  memset(&v25[19], 0, 32);
  v1 = (intptr_t)g_eng.op_new(0x18u);
  if ( v1 != 0 )
  {
    v2 = v1;
    *(_DWORD *)&GAP(v1, 0x0, 4) = 0;
    *(_DWORD *)&GAP(v1, 0x0, 8) = 0;
    *(_DWORD *)&GAP(v1, 0x0, 12) = 0;
    F32(((uint8_t *)(v1) + 0x0), 0) = 0;
    F32(v1, 0x10) = 0;
    F32(v1, 0x14) = 0;
  }
  else
  {
    v2 = 0;
  }
  v3 = (intptr_t *)(*v16__f);
  F32(v15__f, 0xc) = 0;
  F32((*v16__f), 0) = v2;
  ts_59e1d0(v2);
  ts_59df50(F32(v2, 0x10), D(0x693848), ((int32_t *)D(0x6940c8)), 0);
  v4 = F32(v2, 0x14);
  F32(v4, 6144) = 0;
  F32(v4, 30724) = 0;
  v5 = (intptr_t)g_eng.op_new(0x8244u);
  (*v18__f) = v5;
  SETLB((*v27__f), 2);
  if ( v5 != 0 )
  {
    v6 = (v5 + 1);
    *v5 = 2;
    eh_vec_ctor(
      v5 + 1,
      0x4120u,
      2,
      (void *)((intptr_t)D(0x5851a0) /* el_5851a0 by address */),
      (void *)((intptr_t)D(0x54c5e0) /* el_54c5e0 by address */));
    v7 = v6;
    v3 = (intptr_t *)(*v16__f);
  }
  else
  {
    v7 = 0;
  }
  (*v14__f) = F32(v2, 0x14);
  SETLB((*v27__f), 1);
  ts_585260(v7, 1, 1, 0, (*v14__f));
  F32(v7, 0x4104) = 1;
  F32(v17__f, 0x0) = 0;
  F32(v17__f, 0x4) = (int)v7;
  W_room_story_setup((intptr_t)(&(*v17__f)));
  W_room_story_setup2((intptr_t)(&(*v17__f)));
  if ( F32(v7, 0x200c) != 0 )
  {
    W_room_inputs_alloc((intptr_t)(v7), (intptr_t)(1u));
    W_input_init((intptr_t)(v7), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0));
    if ( (*v19__f) != 0 )
      W_room_next_set((intptr_t)(v7), (intptr_t)((*v19__f)), (intptr_t)((*v20__f)), (intptr_t)((*v21__f)));
    W_room_area_enter((intptr_t)(&(*v17__f)), (intptr_t)(1), (intptr_t)((int)v7));
    F32(v7, 0x4104) = 1;
    F32(v15__f, 0xc) = ts_585ae0(v2, v7, 40, 1, 1, &(*v19__f));
    W_session_end((intptr_t)(v2));
  }
  if ( v7 != 0 )
    ts_54c530(v7, 3);
  v8 = *v3;
  if ( *v3 != 0 )
  {
    W_session_end((intptr_t)(*v3));
    g_eng.op_delete((void *)v8);
    *v3 = 0;
  }
  W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  if ( F32(v15__f, 0xc) != 0 )
    goto LABEL_27;
  v9 = 0;
  v10 = 0;
  W_tm_56d710((intptr_t)(&(*v15__f)));
  W_tm_564690((intptr_t)(&(*v26__f)));
  while ( 1 )
  {
    do
    {
      while ( 1 )
      {
        v11 = W_menu_pace((intptr_t)(&(*v26__f))) - 1;
        if ( v11 != 0 )
          break;
        W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
      }
    }
    while ( v11 != 1 );
    v12 = F32(v26__f, 0x20);
    if ( W_tm_5642e0((intptr_t)(&(*v15__f)), (intptr_t)(F32(v26__f, 0x20)), (intptr_t)(0)) == 6 )
      break;
    if ( ts_566300(F32(v15__f, 0x4), v12) != 0 )
    {
      W_tm_56a220((intptr_t)(F32(v15__f, 0x4)));
      break;
    }
    if ( v10 >= 0xC8 )
      goto LABEL_26;
    ++v10;
  }
  v9 = 6;
LABEL_26:
  W_tm_56c2b0((intptr_t)(&(*v15__f)));
  if ( v9 != 0 )
LABEL_27:
    F32(v15__f, 0x8) = 6;
  SETLB((*v27__f), 0);
  W_tm_41bbe0((intptr_t)(&(*v15__f)));
  return F32(v15__f, 0x8);
}

/* 0x59e5d0: int __thiscall sub_59E5D0(cls_59E5D0 *this, __int16 a2) */
int32_t ts_59e5d0(wl_char *self, int16_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t * v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    _Alignas(8) uint8_t frame_[0x138 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v25 = (intptr_t *)(frame_ + 0x44);   /* [27] at ebp-0xf4 */
    intptr_t *v30 = (intptr_t *)(frame_ + 0xc0);   /* [27] at ebp-0x78 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x138 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x120 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x11c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x10c */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x104 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x100 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0xfc */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0xf8 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0xb0);   /* ebp-0x88 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0xb4);   /* ebp-0x84 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0xb8);   /* ebp-0x80 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0xbc);   /* ebp-0x7c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x134);   /* ebp-0x4 */

  v2 = (intptr_t *)c;
  (*v26__f) = 0;
  (*v27__f) = 0;
  (*v28__f) = 0;
  (*v29__f) = 2;
  memset(v30, 0, 0x4Cu);
  memset(&v30[22], 0, 20);
  (*v21__f) = 0;
  (*v22__f) = 0;
  (*v23__f) = 0;
  (*v24__f) = 2;
  memset(v25, 0, 0x4Cu);
  memset(&v25[22], 0, 20);
  v30[19] = 1;
  v30[20] = 3000;
  v30[21] = 1;
  v25[19] = 1;
  v25[20] = 5000;
  v25[21] = 1;
  v3 = (intptr_t)g_eng.op_new(0x18u);
  if ( v3 != 0 )
  {
    v4 = v3;
    F32(((uint8_t *)(v3) + 0x4), 0) = 0;
    *(_DWORD *)&GAP(v3, 0x4, 4) = 0;
    *(_DWORD *)&GAP(v3, 0x4, 8) = 0;
    F32(v3, 0x0) = 0;
    F32(v3, 0x10) = 0;
    F32(v3, 0x14) = 0;
  }
  else
  {
    v4 = 0;
  }
  *v2 = v4;
  if ( a2 == 1 )
  {
    v5 = 0;
    ts_59e1d0(v4);
    ts_59df50(F32(v4, 0x10), D(0x693848), ((int32_t *)D(0x6940c8)), 0);
    v10 = F32(v4, 0x14);
    F32(v10, 6144) = 0;
    F32(v10, 30724) = 0;
    v11 = (intptr_t)g_eng.op_new(0x8244u);
    (*v31__f) = 0;
    if ( v11 != 0 )
    {
      v8 = (v11 + 1);
      *v11 = 2;
      eh_vec_ctor(
        v11 + 1,
        0x4120u,
        2,
        (void *)((intptr_t)D(0x5851a0) /* el_5851a0 by address */),
        (void *)((intptr_t)D(0x54c5e0) /* el_54c5e0 by address */));
    }
    else
    {
      v8 = 0;
    }
    v12 = F32(v4, 0x14);
    (*v31__f) = -1;
    ts_585260(v8, 1, 1, 0, v12);
    F32(v8, 0x4104) = 1010;
    F32(v19__f, 0x8) = 0;
    F32(v19__f, 0xc) = (int)v8;
    W_room_story_setup((intptr_t)(&F32(v19__f, 0x8)));
    W_room_story_setup2((intptr_t)(&F32(v19__f, 0x8)));
    if ( F32(v8, 0x200c) == 0 )
      goto LABEL_21;
    W_room_inputs_alloc((intptr_t)(v8), (intptr_t)(1u));
    W_input_init((intptr_t)(v8), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0));
    if ( (*v26__f) != 0 )
      W_room_next_set((intptr_t)(v8), (intptr_t)((*v26__f)), (intptr_t)((*v27__f)), (intptr_t)((*v28__f)));
    W_room_area_enter((intptr_t)(&F32(v19__f, 0x8)), (intptr_t)(1), (intptr_t)((int)v8));
    F32(v8, 0x4104) = 1010;
    v9 = ts_585ae0(v4, v8, 30, 1, 1, &(*v26__f));
    goto LABEL_20;
  }
  if ( a2 != 2 )
  {
    v13 = (*v18__f);
    goto LABEL_25;
  }
  v5 = 0;
  ts_59e1d0(v4);
  ts_59df50(F32(v4, 0x10), D(0x693848), ((int32_t *)D(0x6940c8)), 0);
  v6 = F32(v4, 0x14);
  F32(v6, 6144) = 0;
  F32(v6, 30724) = 0;
  v7 = (intptr_t)g_eng.op_new(0x8244u);
  (*v31__f) = 1;
  if ( v7 != 0 )
  {
    v8 = (v7 + 1);
    *v7 = 2;
    eh_vec_ctor(
      v7 + 1,
      0x4120u,
      2,
      (void *)((intptr_t)D(0x5851a0) /* el_5851a0 by address */),
      (void *)((intptr_t)D(0x54c5e0) /* el_54c5e0 by address */));
  }
  else
  {
    v8 = 0;
  }
  (*v16__f) = F32(v4, 0x14);
  (*v31__f) = -1;
  ts_585260(v8, 1, 1, 0, (*v16__f));
  F32(v8, 0x4104) = 2;
  W_room_area_enter((intptr_t)(&(*v20__f)), (intptr_t)(0), (intptr_t)((int)v8));
  if ( F32(v8, 0x200c) != 0 )
  {
    W_room_inputs_alloc((intptr_t)(v8), (intptr_t)(1u));
    W_input_init((intptr_t)(v8), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0));
    if ( (*v21__f) != 0 )
      W_room_next_set((intptr_t)(v8), (intptr_t)((*v21__f)), (intptr_t)((*v22__f)), (intptr_t)((*v23__f)));
    W_room_area_enter((intptr_t)(&(*v20__f)), (intptr_t)(1), (intptr_t)((int)v8));
    F32(v8, 0x4104) = 1010;
    v9 = ts_585ae0(v4, v8, 30, 1, 1, &(*v21__f));
LABEL_20:
    v5 = v9;
    W_session_end((intptr_t)(v4));
  }
LABEL_21:
  if ( v8 != 0 )
    ts_54c530(v8, 3);
  v13 = v5;
  v2 = (intptr_t *)c;
LABEL_25:
  v14 = *v2;
  if ( *v2 != 0 )
  {
    W_session_end((intptr_t)(*v2));
    g_eng.op_delete((void *)v14);
    *v2 = 0;
  }
  F32(v19__f, 0x0) = 0;
  F32(v19__f, 0x4) = 0;
  (*v31__f) = 2;
  W_si_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  if ( v13 != 0 || ts_59e9e0(&(*v19__f), 0xC8u) == 0 )
  {
    (*v31__f) = -1;
    W_tm_41bbe0((intptr_t)(&(*v19__f)));
    return v13;
  }
  else
  {
    (*v31__f) = -1;
    W_tm_41bbe0((intptr_t)(&(*v19__f)));
    return 6;
  }
}

/* 0x59e9e0: int __thiscall sub_59E9E0(cls_41BBE0 *this, unsigned int a2) */
int32_t ts_59e9e0(wl_char *self, uint32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    uint32_t v4 = 0;
    uint32_t v5 = 0;
    uint32_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    int32_t ** v14 = 0;
    intptr_t i = 0;
    int32_t * v16 = 0;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const v19__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x30 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x2c */
    uint32_t *const v21__f = (uint32_t *)(frame_ + 0x8);   /* ebp-0x28 */
    uint32_t *const v22__f = (uint32_t *)(frame_ + 0x10);   /* ebp-0x20 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x1c */
    uint32_t *const TickCount__f = (uint32_t *)(frame_ + 0x1c);   /* ebp-0x14 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x10 */

  v2 = c;
  v3 = 0;
  (*v20__f) = 0;
  (*v19__f) = 0;
  W_tm_56d710((intptr_t)(c));
  (*v21__f) = 0;
  v4 = 17;
  (*v22__f) = 0;
  (*v23__f) = 0;
  (*v25__f) = 0;
  (*TickCount__f) = API_GetTickCount();
  while ( 1 )
  {
LABEL_2:
    v5 = API_GetTickCount();
    if ( v3 == 0 )
    {
      (*v21__f) = v5;
      W_wm_5b1030();
LABEL_13:
      v3 = 2;
LABEL_14:
      (*v23__f) = v3;
      goto LABEL_15;
    }
    if ( v3 != 1 )
    {
      if ( v5 - (*v22__f) <= v4 )
      {
        v4 -= 16;
        if ( v4 > 0x10 )
          goto LABEL_15;
        v3 = 1;
      }
      else
      {
        v4 = 0;
        v3 = 1;
      }
      goto LABEL_14;
    }
    v6 = v4 - (*v21__f) + v5;
    if ( v6 > 0x64 )
      v6 = 100;
    v4 = v6;
    (*v21__f) = v5;
    W_wm_5b1030();
    if ( v4 > 0x10 )
      goto LABEL_13;
LABEL_15:
    if ( v3 == 1 )
    {
      if ( v5 - (*TickCount__f) <= 0x3E8 )
      {
        ++(*v25__f);
      }
      else
      {
        (*v25__f) = 0;
        (*TickCount__f) = v5;
      }
    }
    else
    {
      (*v22__f) = v5;
    }
    if ( v3 != 1 )
      break;
    W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
    RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  }
  if ( SYS32(0x2884) != 0 )
    W_sd_5bc0c0((intptr_t)(SYS32(0x2884)));
  if ( SYS32(2728) != 0 )
    ts_56d620(SYS32(2728));
  ts_40fe00((intptr_t *)((intptr_t)g_eng.sys));
  W_tm_566250((intptr_t)(F32(v2, 0x4)), (intptr_t)(0));
  if ( RC_43c110((intptr_t)(F32(v2, 0x4)), (intptr_t)(v5), (intptr_t)(34)) != 0 )
  {
    (*v20__f) = 6;
  }
  else
  {
    v7 = F32(v2, 0x4);
    v8 = 63;
    v9 = (_DWORD *)(v7 + 264);
    do
    {
      v10 = (_DWORD *)*v9;
      if ( *(_DWORD *)*v9 != 0 && v10[2] == 1 )
      {
        if ( v5 - v10[1] <= 0x64 )
        {
          F32(F32(v7 + 4 * v8, 12), 0) = 0;
          v11 = F32(c, 0x4);
          v12 = 11;
          F16(v11, 364) = 0;
          v13 = (_DWORD *)(v11 + 276);
          do
          {
            v13[11] = 0;
            *v13++ = 0;
            --v12;
          }
          while ( v12 != 0 );
          v14 = (_DWORD **)(v11 + 12);
          for ( i = 64; i != 0; --i )
          {
            v16 = *v14++;
            *v16 = 0;
          }
          F32(v11, 268) = 0;
          F32(v11, 272) = 0;
          (*v20__f) = 6;
          goto LABEL_40;
        }
        v3 = (*v23__f);
      }
      --v8;
      --v9;
    }
    while ( v8 >= 0 );
    if ( (*v19__f) < a2 )
    {
      v2 = c;
      ++(*v19__f);
      goto LABEL_2;
    }
LABEL_40:
    v2 = c;
  }
  W_tm_56dc20((intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_frame_cache_clear((intptr_t)(SYS32(10068)));
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
  W_tm_41bbe0((intptr_t)(v2));
  return (*v20__f);
}
/* unported callees: 0x43c110 0x5b8fc0 */
