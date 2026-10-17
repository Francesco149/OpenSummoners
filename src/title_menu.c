/* title_menu.c — the TITLE and its menus: the title drive, the splash / title menu, New Game options, the load
 * screen, the title sparkles and compositor.  See title_menu.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals).  Hand finish: 0x562ea0's display-mode switch
 * (on a set cooperative level retail jumps to the default case; IDA's decompile returns there).  Retail's own (RC_:
 * PORT-DEBT(engine-call-slots)): the asset group loaders 0x56e190 / 0x57ca40 / 0x5752e0 / 0x57b280 .., the sound
 * slot loader 0x563ef0, the save / scene file readers, the demo scene, the logs. */
#include "title_menu.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include <windows.h>
#include "asset_register.h"
#include "char_def.h"
#include "event.h"
#include "input_dik.h"
#include "particle_init.h"
#include "party_member.h"
#include "render_draw.h"
#include "render_platform.h"
#include "room_load.h"
#include "room_loop.h"
#include "scene_loop.h"
#include "script_helpers.h"
#include "sound_fx.h"
#include "sprite_slot.h"
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

static inline intptr_t W_char_skills_refresh(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skills_refresh((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_char_stats_init(intptr_t a0, intptr_t a1, intptr_t a2, ...) { char_stats_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_geometry_free(intptr_t a0, ...) { ctrl_geometry_free((uint8_t *)a0); return 0; }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_input(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_input((input_mgr *)a0, (uint8_t *)a1, (uint32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_key(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_key((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_ctrl_text_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_text_set((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_dramatist_row(intptr_t a0, ...) { return (intptr_t)dramatist_row((int32_t)a0); }
static inline intptr_t W_frame_cache_clear(intptr_t a0, ...) { frame_cache_clear((uint8_t *)a0); return 0; }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_col_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)grid_col_place((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_grid_page_fit(intptr_t a0, ...) { return (intptr_t)grid_page_fit((uint8_t *)a0); }
static inline intptr_t W_grid_row_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)grid_row_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_grid_row_reset(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_row_reset((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_grid_row_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)grid_row_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); }
static inline intptr_t W_icon_tick(intptr_t a0, ...) { return (intptr_t)icon_tick((uint8_t *)a0); }
static inline intptr_t W_input_ctor(intptr_t a0, ...) { input_ctor((uint8_t *)a0); return 0; }
static inline intptr_t W_input_dik_produce(intptr_t a0, intptr_t a1, ...) { input_dik_produce((input_mgr *)a0, (const void *)a1); return 0; }
static inline intptr_t W_input_dtor(intptr_t a0, ...) { input_dtor((uint8_t *)a0); return 0; }
static inline intptr_t W_menu_pace(intptr_t a0, ...) { return (intptr_t)menu_pace((uint8_t *)a0); }
static inline intptr_t W_mgr_win_to_back(intptr_t a0, intptr_t a1, ...) { return (intptr_t)mgr_win_to_back((uint8_t *)a0, (uint16_t)a1); }
static inline intptr_t W_pool_head_delete(intptr_t a0, intptr_t a1, ...) { return (intptr_t)pool_head_delete((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_rd_418470(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rd_418470((wl_char *)a0, (uint16_t)a1); }
static inline intptr_t W_rd_48c820(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_48c820((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rd_494e10(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)rd_494e10((wl_char *)a0, (int32_t)a1, (int32_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_rp_5b8b40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)rp_5b8b40((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_rp_5b94e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b94e0((wl_char *)a0, (intptr_t *)a1); }
static inline intptr_t W_rp_5b9500(intptr_t a0, intptr_t a1, ...) { rp_5b9500((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_rp_5b9520(intptr_t a0, ...) { return (intptr_t)rp_5b9520((wl_char *)a0); }
static inline intptr_t W_rp_5b9b70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)rp_5b9b70((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_rp_5b9bf0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)rp_5b9bf0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_rp_5bd550(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)rp_5bd550((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_sc_406c70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)sc_406c70((int32_t)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4); }
static inline intptr_t W_scene_loop(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)scene_loop((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t *)a4, (int32_t)a5); }
static inline intptr_t W_sfx_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sfx_play((void *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_slot_frames_free(intptr_t a0, intptr_t a1, ...) { slot_frames_free((ar_sprite_slot *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_slots_free_graded(intptr_t a0, ...) { slots_free_graded((void *)a0); return 0; }
static inline intptr_t W_sys_cursor_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_cursor_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_frame_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_frame_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_list_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)sys_list_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (uint16_t)a7, (uint16_t)a8, (uint8_t **)a9, (uint8_t **)a10, (const char *)a11, (int32_t)a12); }
static inline intptr_t W_sys_msg_keys(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_keys((void *)a0, (int32_t)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_msg_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_unload(intptr_t a0, intptr_t a1, ...) { sys_unload((void *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_sys_win_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)sys_win_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const char *)a7, (int32_t)a8); }
static inline intptr_t W_text_metrics_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)text_metrics_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_timed_sounds_tick(intptr_t a0, ...) { timed_sounds_tick((void *)a0); return 0; }
static inline intptr_t W_tint_fade_a(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tint_fade_a((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_win_anim(intptr_t a0, ...) { return (intptr_t)win_anim((uint8_t *)a0); }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_ctrls_free(intptr_t a0, ...) { win_ctrls_free((uint8_t *)a0); return 0; }
static inline intptr_t W_win_grid_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_grid_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_grid_new(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_grid_new((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_win_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)win_init((uint8_t *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const void *)a7); }
static inline intptr_t W_win_mgr_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_mgr_open((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (const void *)a6); }
static inline intptr_t W_win_mgr_topmost(intptr_t a0, ...) { win_mgr_topmost((uint8_t *)a0); return 0; }
static inline intptr_t W_win_slide(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_slide((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_win_tick(intptr_t a0, ...) { return (intptr_t)win_tick((uint8_t *)a0); }
static inline intptr_t W_win_title_add(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_title_add((uint8_t *)a0, (const char *)a1); }
typedef intptr_t (__attribute__((thiscall)) *rt_40a5d0)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x40a5d0 thiscall: unported, retail's */
#define RC_40a5d0 ((rt_40a5d0)g_eng.retail(0x40a5d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_40fe00)(intptr_t);   /* 0x40fe00 thiscall: unported, retail's */
#define RC_40fe00 ((rt_40fe00)g_eng.retail(0x40fe00u))
typedef intptr_t (__attribute__((thiscall)) *rt_4184a0)(intptr_t, intptr_t);   /* 0x4184a0 thiscall: unported, retail's */
#define RC_4184a0 ((rt_4184a0)g_eng.retail(0x4184a0u))
typedef intptr_t (__attribute__((stdcall)) *rt_41db30)(intptr_t, intptr_t);   /* 0x41db30 stdcall: unported, retail's */
#define RC_41db30 ((rt_41db30)g_eng.retail(0x41db30u))
typedef intptr_t (__attribute__((thiscall)) *rt_41dba0)(intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x41dba0 thiscall: unported, retail's */
#define RC_41dba0 ((rt_41dba0)g_eng.retail(0x41dba0u))
typedef intptr_t (__attribute__((thiscall)) *rt_43c110)(intptr_t, intptr_t, intptr_t);   /* 0x43c110 thiscall: unported, retail's */
#define RC_43c110 ((rt_43c110)g_eng.retail(0x43c110u))
typedef intptr_t (__attribute__((thiscall)) *rt_468b10)(intptr_t, intptr_t, intptr_t);   /* 0x468b10 thiscall: unported, retail's */
#define RC_468b10 ((rt_468b10)g_eng.retail(0x468b10u))
typedef intptr_t (__attribute__((thiscall)) *rt_49bf90)(intptr_t, intptr_t);   /* 0x49bf90 thiscall: unported, retail's */
#define RC_49bf90 ((rt_49bf90)g_eng.retail(0x49bf90u))
typedef intptr_t (__attribute__((thiscall)) *rt_55fdb0)(intptr_t);   /* 0x55fdb0 thiscall: unported, retail's */
#define RC_55fdb0 ((rt_55fdb0)g_eng.retail(0x55fdb0u))
typedef intptr_t (__attribute__((thiscall)) *rt_560900)(intptr_t);   /* 0x560900 thiscall: unported, retail's */
#define RC_560900 ((rt_560900)g_eng.retail(0x560900u))
typedef intptr_t (__attribute__((thiscall)) *rt_563ef0)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x563ef0 thiscall: unported, retail's */
#define RC_563ef0 ((rt_563ef0)g_eng.retail(0x563ef0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5640b0)(intptr_t);   /* 0x5640b0 thiscall: unported, retail's */
#define RC_5640b0 ((rt_5640b0)g_eng.retail(0x5640b0u))
typedef intptr_t (__attribute__((thiscall)) *rt_564110)(intptr_t, intptr_t, intptr_t);   /* 0x564110 thiscall: unported, retail's */
#define RC_564110 ((rt_564110)g_eng.retail(0x564110u))
typedef intptr_t (__attribute__((stdcall)) *rt_5646c0)(intptr_t, intptr_t);   /* 0x5646c0 stdcall: unported, retail's */
#define RC_5646c0 ((rt_5646c0)g_eng.retail(0x5646c0u))
typedef intptr_t (__attribute__((thiscall)) *rt_565b00)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x565b00 thiscall: unported, retail's */
#define RC_565b00 ((rt_565b00)g_eng.retail(0x565b00u))
typedef intptr_t (__attribute__((thiscall)) *rt_566300)(intptr_t, intptr_t);   /* 0x566300 thiscall: unported, retail's */
#define RC_566300 ((rt_566300)g_eng.retail(0x566300u))
typedef intptr_t (__attribute__((stdcall)) *rt_568780)(intptr_t);   /* 0x568780 stdcall: unported, retail's */
#define RC_568780 ((rt_568780)g_eng.retail(0x568780u))
typedef intptr_t (__attribute__((thiscall)) *rt_569220)(intptr_t, intptr_t);   /* 0x569220 thiscall: unported, retail's */
#define RC_569220 ((rt_569220)g_eng.retail(0x569220u))
typedef intptr_t (__attribute__((thiscall)) *rt_56a670)(intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x56a670 thiscall: unported, retail's */
#define RC_56a670 ((rt_56a670)g_eng.retail(0x56a670u))
typedef intptr_t (__attribute__((thiscall)) *rt_56c430)(intptr_t, intptr_t);   /* 0x56c430 thiscall: unported, retail's */
#define RC_56c430 ((rt_56c430)g_eng.retail(0x56c430u))
typedef intptr_t (__attribute__((thiscall)) *rt_56c640)(intptr_t);   /* 0x56c640 thiscall: unported, retail's */
#define RC_56c640 ((rt_56c640)g_eng.retail(0x56c640u))
typedef intptr_t (__attribute__((thiscall)) *rt_56d620)(intptr_t);   /* 0x56d620 thiscall: unported, retail's */
#define RC_56d620 ((rt_56d620)g_eng.retail(0x56d620u))
typedef intptr_t (__attribute__((stdcall)) *rt_56dd80)(intptr_t, intptr_t);   /* 0x56dd80 stdcall: unported, retail's */
#define RC_56dd80 ((rt_56dd80)g_eng.retail(0x56dd80u))
typedef intptr_t (__attribute__((stdcall)) *rt_56e190)(intptr_t, intptr_t, intptr_t);   /* 0x56e190 stdcall: unported, retail's */
#define RC_56e190 ((rt_56e190)g_eng.retail(0x56e190u))
typedef intptr_t (__attribute__((stdcall)) *rt_5749b0)(intptr_t, intptr_t, intptr_t);   /* 0x5749b0 stdcall: unported, retail's */
#define RC_5749b0 ((rt_5749b0)g_eng.retail(0x5749b0u))
typedef intptr_t (__attribute__((stdcall)) *rt_5752e0)(void);   /* 0x5752e0 stdcall: unported, retail's */
#define RC_5752e0 ((rt_5752e0)g_eng.retail(0x5752e0u))
typedef intptr_t (__attribute__((stdcall)) *rt_579a00)(intptr_t, intptr_t, intptr_t);   /* 0x579a00 stdcall: unported, retail's */
#define RC_579a00 ((rt_579a00)g_eng.retail(0x579a00u))
typedef intptr_t (__attribute__((thiscall)) *rt_579bd0)(intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x579bd0 thiscall: unported, retail's */
#define RC_579bd0 ((rt_579bd0)g_eng.retail(0x579bd0u))
typedef intptr_t (__attribute__((stdcall)) *rt_57a330)(intptr_t, intptr_t, intptr_t);   /* 0x57a330 stdcall: unported, retail's */
#define RC_57a330 ((rt_57a330)g_eng.retail(0x57a330u))
typedef intptr_t (__attribute__((stdcall)) *rt_57b280)(intptr_t, intptr_t, intptr_t);   /* 0x57b280 stdcall: unported, retail's */
#define RC_57b280 ((rt_57b280)g_eng.retail(0x57b280u))
typedef intptr_t (__attribute__((stdcall)) *rt_57ca40)(intptr_t, intptr_t, intptr_t);   /* 0x57ca40 stdcall: unported, retail's */
#define RC_57ca40 ((rt_57ca40)g_eng.retail(0x57ca40u))
typedef intptr_t (__attribute__((thiscall)) *rt_582e90)(intptr_t);   /* 0x582e90 thiscall: unported, retail's */
#define RC_582e90 ((rt_582e90)g_eng.retail(0x582e90u))
typedef intptr_t (__attribute__((thiscall)) *rt_583c90)(intptr_t, intptr_t);   /* 0x583c90 thiscall: unported, retail's */
#define RC_583c90 ((rt_583c90)g_eng.retail(0x583c90u))
typedef intptr_t (__attribute__((stdcall)) *rt_583ee0)(void);   /* 0x583ee0 stdcall: unported, retail's */
#define RC_583ee0 ((rt_583ee0)g_eng.retail(0x583ee0u))
typedef intptr_t (__attribute__((thiscall)) *rt_583fe0)(intptr_t);   /* 0x583fe0 thiscall: unported, retail's */
#define RC_583fe0 ((rt_583fe0)g_eng.retail(0x583fe0u))
typedef intptr_t (__attribute__((thiscall)) *rt_59e230)(intptr_t);   /* 0x59e230 thiscall: unported, retail's */
#define RC_59e230 ((rt_59e230)g_eng.retail(0x59e230u))
typedef intptr_t (__attribute__((thiscall)) *rt_59e5d0)(intptr_t, intptr_t);   /* 0x59e5d0 thiscall: unported, retail's */
#define RC_59e5d0 ((rt_59e5d0)g_eng.retail(0x59e5d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5a4320)(intptr_t, intptr_t);   /* 0x5a4320 thiscall: unported, retail's */
#define RC_5a4320 ((rt_5a4320)g_eng.retail(0x5a4320u))
typedef intptr_t (__attribute__((thiscall)) *rt_5a4440)(intptr_t, intptr_t);   /* 0x5a4440 thiscall: unported, retail's */
#define RC_5a4440 ((rt_5a4440)g_eng.retail(0x5a4440u))
typedef intptr_t (__attribute__((thiscall)) *rt_5a4760)(intptr_t);   /* 0x5a4760 thiscall: unported, retail's */
#define RC_5a4760 ((rt_5a4760)g_eng.retail(0x5a4760u))
typedef intptr_t (__attribute__((thiscall)) *rt_5aff00)(intptr_t);   /* 0x5aff00 thiscall: unported, retail's */
#define RC_5aff00 ((rt_5aff00)g_eng.retail(0x5aff00u))
typedef intptr_t (__attribute__((stdcall)) *rt_5b1030)(void);   /* 0x5b1030 stdcall: unported, retail's */
#define RC_5b1030 ((rt_5b1030)g_eng.retail(0x5b1030u))
typedef intptr_t (__attribute__((stdcall)) *rt_5b5dc0)(intptr_t, intptr_t, intptr_t);   /* 0x5b5dc0 stdcall: unported, retail's */
#define RC_5b5dc0 ((rt_5b5dc0)g_eng.retail(0x5b5dc0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b6060)(intptr_t);   /* 0x5b6060 thiscall: unported, retail's */
#define RC_5b6060 ((rt_5b6060)g_eng.retail(0x5b6060u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b60d0)(intptr_t);   /* 0x5b60d0 thiscall: unported, retail's */
#define RC_5b60d0 ((rt_5b60d0)g_eng.retail(0x5b60d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b60e0)(intptr_t, intptr_t, intptr_t);   /* 0x5b60e0 thiscall: unported, retail's */
#define RC_5b60e0 ((rt_5b60e0)g_eng.retail(0x5b60e0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b6250)(intptr_t, intptr_t);   /* 0x5b6250 thiscall: unported, retail's */
#define RC_5b6250 ((rt_5b6250)g_eng.retail(0x5b6250u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b6410)(intptr_t, intptr_t, intptr_t);   /* 0x5b6410 thiscall: unported, retail's */
#define RC_5b6410 ((rt_5b6410)g_eng.retail(0x5b6410u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b64b0)(intptr_t);   /* 0x5b64b0 thiscall: unported, retail's */
#define RC_5b64b0 ((rt_5b64b0)g_eng.retail(0x5b64b0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b6580)(intptr_t);   /* 0x5b6580 thiscall: unported, retail's */
#define RC_5b6580 ((rt_5b6580)g_eng.retail(0x5b6580u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b65a0)(intptr_t);   /* 0x5b65a0 thiscall: unported, retail's */
#define RC_5b65a0 ((rt_5b65a0)g_eng.retail(0x5b65a0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b65b0)(intptr_t, intptr_t, intptr_t);   /* 0x5b65b0 thiscall: unported, retail's */
#define RC_5b65b0 ((rt_5b65b0)g_eng.retail(0x5b65b0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b6990)(intptr_t, intptr_t, intptr_t);   /* 0x5b6990 thiscall: unported, retail's */
#define RC_5b6990 ((rt_5b6990)g_eng.retail(0x5b6990u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b6db0)(intptr_t, intptr_t);   /* 0x5b6db0 thiscall: unported, retail's */
#define RC_5b6db0 ((rt_5b6db0)g_eng.retail(0x5b6db0u))
typedef intptr_t (__attribute__((stdcall)) *rt_5b7ee0)(intptr_t);   /* 0x5b7ee0 stdcall: unported, retail's */
#define RC_5b7ee0 ((rt_5b7ee0)g_eng.retail(0x5b7ee0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b89d0)(intptr_t, intptr_t, intptr_t);   /* 0x5b89d0 thiscall: unported, retail's */
#define RC_5b89d0 ((rt_5b89d0)g_eng.retail(0x5b89d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b8fc0)(intptr_t, intptr_t);   /* 0x5b8fc0 thiscall: unported, retail's */
#define RC_5b8fc0 ((rt_5b8fc0)g_eng.retail(0x5b8fc0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b9410)(intptr_t);   /* 0x5b9410 thiscall: unported, retail's */
#define RC_5b9410 ((rt_5b9410)g_eng.retail(0x5b9410u))
typedef intptr_t (__attribute__((stdcall)) *rt_5b9cf0)(intptr_t, intptr_t);   /* 0x5b9cf0 stdcall: unported, retail's */
#define RC_5b9cf0 ((rt_5b9cf0)g_eng.retail(0x5b9cf0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b9fc0)(intptr_t, intptr_t, intptr_t);   /* 0x5b9fc0 thiscall: unported, retail's */
#define RC_5b9fc0 ((rt_5b9fc0)g_eng.retail(0x5b9fc0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba120)(intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x5ba120 thiscall: unported, retail's */
#define RC_5ba120 ((rt_5ba120)g_eng.retail(0x5ba120u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba290)(intptr_t);   /* 0x5ba290 thiscall: unported, retail's */
#define RC_5ba290 ((rt_5ba290)g_eng.retail(0x5ba290u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba2d0)(intptr_t);   /* 0x5ba2d0 thiscall: unported, retail's */
#define RC_5ba2d0 ((rt_5ba2d0)g_eng.retail(0x5ba2d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba390)(intptr_t);   /* 0x5ba390 thiscall: unported, retail's */
#define RC_5ba390 ((rt_5ba390)g_eng.retail(0x5ba390u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba3a0)(intptr_t, intptr_t);   /* 0x5ba3a0 thiscall: unported, retail's */
#define RC_5ba3a0 ((rt_5ba3a0)g_eng.retail(0x5ba3a0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba3d0)(intptr_t, intptr_t);   /* 0x5ba3d0 thiscall: unported, retail's */
#define RC_5ba3d0 ((rt_5ba3d0)g_eng.retail(0x5ba3d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba4c0)(intptr_t);   /* 0x5ba4c0 thiscall: unported, retail's */
#define RC_5ba4c0 ((rt_5ba4c0)g_eng.retail(0x5ba4c0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5ba650)(intptr_t);   /* 0x5ba650 thiscall: unported, retail's */
#define RC_5ba650 ((rt_5ba650)g_eng.retail(0x5ba650u))
typedef intptr_t (__attribute__((thiscall)) *rt_5baed0)(intptr_t, intptr_t, intptr_t);   /* 0x5baed0 thiscall: unported, retail's */
#define RC_5baed0 ((rt_5baed0)g_eng.retail(0x5baed0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bb3a0)(intptr_t);   /* 0x5bb3a0 thiscall: unported, retail's */
#define RC_5bb3a0 ((rt_5bb3a0)g_eng.retail(0x5bb3a0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbb10)(intptr_t);   /* 0x5bbb10 thiscall: unported, retail's */
#define RC_5bbb10 ((rt_5bbb10)g_eng.retail(0x5bbb10u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbc20)(intptr_t);   /* 0x5bbc20 thiscall: unported, retail's */
#define RC_5bbc20 ((rt_5bbc20)g_eng.retail(0x5bbc20u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbc60)(intptr_t, intptr_t);   /* 0x5bbc60 thiscall: unported, retail's */
#define RC_5bbc60 ((rt_5bbc60)g_eng.retail(0x5bbc60u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbc90)(intptr_t, intptr_t);   /* 0x5bbc90 thiscall: unported, retail's */
#define RC_5bbc90 ((rt_5bbc90)g_eng.retail(0x5bbc90u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbcd0)(intptr_t, intptr_t);   /* 0x5bbcd0 thiscall: unported, retail's */
#define RC_5bbcd0 ((rt_5bbcd0)g_eng.retail(0x5bbcd0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbdb0)(intptr_t, intptr_t, intptr_t);   /* 0x5bbdb0 thiscall: unported, retail's */
#define RC_5bbdb0 ((rt_5bbdb0)g_eng.retail(0x5bbdb0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbe20)(intptr_t, intptr_t);   /* 0x5bbe20 thiscall: unported, retail's */
#define RC_5bbe20 ((rt_5bbe20)g_eng.retail(0x5bbe20u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bbeb0)(intptr_t, intptr_t, intptr_t);   /* 0x5bbeb0 thiscall: unported, retail's */
#define RC_5bbeb0 ((rt_5bbeb0)g_eng.retail(0x5bbeb0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bc0c0)(intptr_t);   /* 0x5bc0c0 thiscall: unported, retail's */
#define RC_5bc0c0 ((rt_5bc0c0)g_eng.retail(0x5bc0c0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bcb30)(intptr_t);   /* 0x5bcb30 thiscall: unported, retail's */
#define RC_5bcb30 ((rt_5bcb30)g_eng.retail(0x5bcb30u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bcb80)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x5bcb80 thiscall: unported, retail's */
#define RC_5bcb80 ((rt_5bcb80)g_eng.retail(0x5bcb80u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bd3b0)(intptr_t, intptr_t, intptr_t, intptr_t);   /* 0x5bd3b0 thiscall: unported, retail's */
#define RC_5bd3b0 ((rt_5bd3b0)g_eng.retail(0x5bd3b0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bd3d0)(intptr_t, intptr_t);   /* 0x5bd3d0 thiscall: unported, retail's */
#define RC_5bd3d0 ((rt_5bd3d0)g_eng.retail(0x5bd3d0u))
typedef intptr_t (__attribute__((thiscall)) *rt_5bd4d0)(intptr_t);   /* 0x5bd4d0 thiscall: unported, retail's */
#define RC_5bd4d0 ((rt_5bd4d0)g_eng.retail(0x5bd4d0u))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteObject_t)(intptr_t);
#define API_DeleteObject ((api_DeleteObject_t)g_eng.api("DeleteObject"))
typedef intptr_t (__attribute__((stdcall)) *api_GetClientRect_t)(intptr_t, intptr_t);
#define API_GetClientRect ((api_GetClientRect_t)g_eng.api("GetClientRect"))
typedef intptr_t (__attribute__((stdcall)) *api_GetDesktopWindow_t)(void);
#define API_GetDesktopWindow ((api_GetDesktopWindow_t)g_eng.api("GetDesktopWindow"))
typedef intptr_t (__attribute__((stdcall)) *api_GetTickCount_t)(void);
#define API_GetTickCount ((api_GetTickCount_t)g_eng.api("GetTickCount"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
typedef intptr_t (__attribute__((stdcall)) *api_SelectObject_t)(intptr_t, intptr_t);
#define API_SelectObject ((api_SelectObject_t)g_eng.api("SelectObject"))
typedef intptr_t (__attribute__((stdcall)) *api_SetBkMode_t)(intptr_t, intptr_t);
#define API_SetBkMode ((api_SetBkMode_t)g_eng.api("SetBkMode"))
typedef intptr_t (__attribute__((stdcall)) *api_SetFocus_t)(intptr_t);
#define API_SetFocus ((api_SetFocus_t)g_eng.api("SetFocus"))
typedef intptr_t (__attribute__((stdcall)) *api_SetTextColor_t)(intptr_t, intptr_t);
#define API_SetTextColor ((api_SetTextColor_t)g_eng.api("SetTextColor"))
typedef intptr_t (__attribute__((stdcall)) *api_SetTimer_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_SetTimer ((api_SetTimer_t)g_eng.api("SetTimer"))
typedef intptr_t (__attribute__((stdcall)) *api_SetWindowLongA_t)(intptr_t, intptr_t, intptr_t);
#define API_SetWindowLongA ((api_SetWindowLongA_t)g_eng.api("SetWindowLongA"))
typedef intptr_t (__attribute__((stdcall)) *api_SetWindowPos_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_SetWindowPos ((api_SetWindowPos_t)g_eng.api("SetWindowPos"))
typedef intptr_t (__attribute__((stdcall)) *api_TextOutA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_TextOutA ((api_TextOutA_t)g_eng.api("TextOutA"))
typedef intptr_t (__attribute__((stdcall)) *api_UpdateWindow_t)(intptr_t);
#define API_UpdateWindow ((api_UpdateWindow_t)g_eng.api("UpdateWindow"))
void tm_41bbe0(wl_char *self);
int32_t tm_562ea0(wl_char *self, int32_t a2);
int32_t tm_564160(wl_char *self);
int32_t tm_5642e0(wl_char *self, int32_t a2, int32_t a3);
int32_t tm_5644a0(wl_char *self);
int32_t tm_564690(wl_char *self);
int32_t tm_564780(wl_char *self, int32_t a2);
int32_t tm_419900(wl_char *self, int32_t a2, int32_t a3);
int32_t tm_5657f0(wl_char *self, uint16_t a2, int32_t a3);
int32_t tm_565810(wl_char *self);
void tm_565840(wl_char *self);
int32_t tm_5658c0(wl_char *self);
int32_t tm_565d10(wl_char *self, int32_t * a2, int16_t a3, int32_t * a4, int32_t a5, uint32_t a6);
int32_t tm_566250(wl_char *self, int32_t a2);
int32_t tm_566350(wl_char *self, int32_t * a2);
int32_t tm_56d910(wl_char *self);
int32_t tm_566570(int16_t a1, char * a2);
int32_t tm_566850(int16_t a1, char * a2);
int32_t tm_566a80(int16_t a1, int32_t a2, char * Buffer);
int32_t tm_567ba0(wl_char *self, int32_t a2);
int32_t tm_568320(int16_t a1, int32_t * a2);
int32_t tm_56da40(wl_char *self, char * a2, int32_t a3, int32_t a4);
int32_t tm_56daa0(wl_char *self, int32_t a2, int16_t a3, int32_t a4, int32_t a5, int16_t a6, int32_t a7, int32_t a8, int16_t * a9, uint16_t a10, int16_t a11);
int32_t tm_568b40(wl_char *self, int32_t * a2);
void tm_56ded0(wl_char *self);
int32_t tm_56dfe0(wl_char *self);
int32_t tm_56c930(wl_char *self);
void tm_56cc10(wl_char *self);
void tm_56dc20(int32_t a1, int32_t a2, int32_t a3);
int32_t tm_568de0(wl_char *self, int32_t a2);
int32_t tm_56a220(wl_char *self);
int32_t tm_41c150(int32_t * a1, int32_t * a2, int32_t a3);
int32_t tm_41c240(wl_char *self, int32_t a2);
int32_t tm_560a40(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_41c2f0(wl_char *self, int32_t * a2, uint16_t * a3);
int32_t tm_41c320(wl_char *self, int32_t * a2, uint16_t a3);
int32_t tm_560a00(wl_char *self, int32_t a2);
void tm_562a10(wl_char *self);
int32_t tm_56aea0(wl_char *self, int32_t a2);
int32_t tm_41bb80(wl_char *self, uint16_t a2, int32_t a3);
int32_t tm_56bfd0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_56c030(wl_char *self, uint16_t a2);
int32_t tm_56c070(wl_char *self, int16_t a2, int16_t a3, int16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int16_t a10, uint16_t a11);
void tm_56c180(wl_char *self, int32_t a2);
void tm_56c2b0(wl_char *self);
void tm_562af0(wl_char *self);
int32_t tm_56c470(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t tm_56c4e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t tm_56c580(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
int32_t tm_56c610(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_56cb90(wl_char *self, uint16_t a2, int32_t a3, int32_t a4);
int32_t tm_56cd20(wl_char *self);
int32_t tm_40bb70(wl_char *self, char a2);
int32_t tm_56d710(wl_char *self);
int32_t tm_56de80(wl_char *self);
void tm_59ec30(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t tm_59ee50(wl_char *self, int32_t a2, int32_t a3);
int32_t tm_41d780(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void tm_49a6e0(wl_char *self);

/* 0x41bbe0: void __thiscall sub_41BBE0(cls_41BBE0 *this) */
void tm_41bbe0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    void * v8 = 0;
    void ** v9 = 0;
    void ** v10 = 0;
    intptr_t j = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10 */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xc */

  v1 = c;
  v2 = F32(c, 0x0);
  (*v13__f) = F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    v3 = 0;
    for ( (*i__f) = 0; (uint16_t)v3 < F16(v2, 4); (*i__f) = v3 )
    {
      v4 = 4 * (uint16_t)v3;
      v5 = F32((F32(v2, 0) + v4), 0);
      if ( v5 != 0 )
      {
        if ( F32(v5, 72) != 0 )
        {
          v6 = 0;
          if ( F16(v5, 76) != 0 )
          {
            do
            {
              v7 = F32(v5, 72);
              v8 = F32(v7, 4 * v6);
              if ( v8 != 0 )
              {
                W_ctrl_geometry_free((intptr_t)(F32(v7, 4 * v6)));
                g_eng.op_delete((void *)v8);
                F32((F32(v5, 72) + 4 * v6), 0) = 0;
              }
              ++v6;
            }
            while ( v6 < F16(v5, 76) );
            v2 = (*v13__f);
          }
          if ( F32(v5, 72) != 0 )
          {
            g_eng.op_delete((void *)F32(v5, 72));
            F32(v5, 72) = 0;
          }
        }
        g_eng.op_delete((void *)(void *)v5);
        v3 = (*i__f);
        v1 = c;
        F32((F32(v2, 0) + v4), 0) = 0;
      }
      ++v3;
    }
    if ( F32(v2, 0) != 0 )
    {
      g_eng.op_delete((void *)F32(v2, 0));
      F32(v2, 0) = 0;
    }
    g_eng.op_delete((void *)(void *)v2);
    F32(v1, 0x0) = 0;
  }
  v9 = (void **)F32(v1, 0x4);
  if ( v9 != 0 )
  {
    v10 = v9 + 3;
    for ( j = 64; j != 0; --j )
    {
      if ( *v10 != 0 )
      {
        g_eng.op_delete((void *)*v10);
        *v10 = 0;
      }
      ++v10;
    }
    g_eng.op_delete((void *)v9);
    F32(v1, 0x4) = 0;
  }
}

/* 0x562ea0: int __thiscall build_title_drive_562EA0(cls_562EA0 *this, int a2) */
int32_t tm_562ea0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t left = 0;
    uint16_t v3 = 0;
    uint32_t v4 = 0;
    char * v5 = 0;
    char * v6 = 0;
    char * v7 = 0;
    char v8 = 0;
    uint32_t v9 = 0;
    uint8_t * v10 = 0;
    intptr_t i = 0;
    int32_t * v12 = 0;
    int32_t * v13 = 0;
    uint8_t * v14 = 0;
    intptr_t j = 0;
    int32_t * v16 = 0;
    uint8_t * v17 = 0;
    intptr_t k = 0;
    uint16_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t DesktopWindow = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t * v33 = 0;
    intptr_t v34 = 0;
    intptr_t m = 0;
    intptr_t v36 = 0;
    intptr_t inited = 0;
    intptr_t * v38 = 0;
    intptr_t v39 = 0;
    intptr_t n = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t * v44 = 0;
    intptr_t ii = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    uint8_t * v48 = 0;
    intptr_t jj = 0;
    intptr_t v50 = 0;
    int32_t * v51 = 0;
    intptr_t * v52 = 0;
    intptr_t v53 = 0;
    intptr_t kk = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    void * v59 = 0;
    intptr_t v60 = 0;
    int v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    intptr_t mm = 0;
    double v66 = 0;
    void * v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t top = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x160 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *OutputString = (intptr_t *)(frame_ + 0x54);   /* [64] at ebp-0x10c */
    RECT *const Rect__f = (RECT *)(frame_ + 0x0);   /* ebp-0x160 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x150 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x14c */
    int32_t * *const v77__f = (int32_t * *)(frame_ + 0x18);   /* ebp-0x148 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x144 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x140 */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x130 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x12c */
    RECT *const v82__f = (RECT *)(frame_ + 0x44);   /* ebp-0x11c */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x15c);   /* ebp-0x4 */

  left = c;
  (*Rect__f).left = (intptr_t)c;
  memset(&(*v79__f), 0, sizeof((*v79__f)));
  (*v84__f) = 1;
  F32(v81__f, 0x0) = 0;
  F32(v81__f, 0x4) = 0;
  (*v76__f) = 0;
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0efc)), D(0x8a9b6c));
  if ( F32(CFG(), 36) == 2 )
  {
    F32(left, (5) * 4) = 1280;
    F32(left, (6) * 4) = 960;
  }
  else if ( F32(CFG(), 36) == 3 )
  {
    F32(left, (5) * 4) = 1920;
    F32(left, (6) * 4) = 1440;
  }
  else
  {
    F32(left, (5) * 4) = 640;
    F32(left, (6) * 4) = 480;
  }
  v3 = 0;
  F16(g_eng.sys, 770) = 0;
  F16(g_eng.sys, 772) = 0;
  F16(g_eng.sys, 774) = 0;
  F32(D(0x8a93fc), 0) = 0;
  F32(D(0x8a9510), 0) = 0;
  F32(D(0x8a9514), 0) = 1000;
  do
    ++v3;
  while ( ((int32_t *)D(0x6940c8))[84 * v3] != 0 );
  if ( v3 > 0x400u )
  {
    RC_560900((intptr_t)((const char *)D(0x8a9530)));
    F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
    if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
    {
      strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
      v4 = strlen(((char *)D(0x8a0ed0))) + 1;
      v5 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
      qmemcpy(v5, ((char *)D(0x8a0ed0)), 4 * (v4 >> 2));
      v7 = ((char *)D(0x8a0ed0) + (4 * (v4 >> 2)));
      v6 = &v5[4 * (v4 >> 2)];
      v8 = v4;
    }
    else
    {
      v9 = strlen(((char *)D(0x8a0ed0))) + 1;
      qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a0ed0)), 4 * (v9 >> 2));
      v7 = ((char *)D(0x8a0ed0) + (4 * (v9 >> 2)));
      v6 = &((char *)D(0x8a9530) + 0x404)[4 * (v9 >> 2) - 1024];
      v8 = v9;
    }
    qmemcpy(v6, v7, v8 & 3);
    strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
    g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a0ed0)), D(0x8a9b6c));
    g_eng.h5bf5db(0);
  }
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0eb4)), D(0x8a9b6c));
  v10 = &SYSB(6368);
  for ( i = 909; i != 0; --i )
  {
    v12 = (intptr_t)g_eng.op_new(0x44u);
    if ( v12 != 0 )
    {
      v12[7] = 0;
      v12[13] = 0;
      *v12 = 0;
      F16(v12, 2 * 2) = 0;
    }
    else
    {
      v12 = 0;
    }
    *((int32_t *)v10 - 909) = v12;
    v13 = (intptr_t)g_eng.op_new(0x14u);
    if ( v13 != 0 )
    {
      F16(v13, 0) = 0;
      v13[1] = 0;
      v13[2] = 0;
      v13[3] = 0;
      v13[4] = 0;
    }
    else
    {
      v13 = 0;
    }
    F32(v10, 0) = v13;
    v10 += 4;
  }
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0ea0)), D(0x8a9b6c));
  v14 = &SYSB(868);
  for ( j = 465; j != 0; --j )
  {
    v16 = (intptr_t)g_eng.op_new(0x18u);
    if ( v16 != 0 )
    {
      v16[1] = 0;
      v16[2] = 0;
    }
    else
    {
      v16 = 0;
    }
    F32(v14, 0) = v16;
    v14 += 4;
  }
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0e8c)), D(0x8a9b6c));
  v17 = &SYSB(10004);
  for ( k = 16; k != 0; --k )
  {
    v19 = (intptr_t)g_eng.op_new(0xCu);
    if ( v19 != 0 )
    {
      v19[4] = 0;
      F32(v19, 0) = 0;
      v19[2] = 0;
      v19[3] = 0;
    }
    else
    {
      v19 = 0;
    }
    F32(v17, 0) = v19;
    v17 += 4;
  }
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0e78)), D(0x8a9b6c));
  v20 = F32(CFG(), 36);
  if ( v20 == 1 )
  {
    if ( F32(CFG(), 64) == 1 )
    {
      F32(left, (1) * 4) = 0;
    }
    else if ( F32(CFG(), 64) == 2 )
    {
      F32(left, (1) * 4) = 3;
    }
    else
    {
      F32(left, (1) * 4) = 1;
    }
  }
  else if ( v20 <= 1 || v20 > 3 )
  {
    F32(left, (1) * 4) = 2;
  }
  else
  {
    F32(left, (1) * 4) = 4;
  }
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0e50)), D(0x8a9b6c));
  v21 = (int)F32(left, (1) * 4);
  F32(left, (7) * 4) = 0;
  F32(left, (4) * 4) = 0;
  F32(left, (3) * 4) = 0;
  if ( v21 > 0 && v21 <= 2 )
    F32(left, (3) * 4) = API_SetTimer((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))), (intptr_t)(1u), (intptr_t)(0xAu), (intptr_t)(0));
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0e30)), D(0x8a9b6c));
  switch ( (unsigned int)F32(left, (1) * 4) )
  {
    case 0u:
    case 1u:
    case 3u:
      v30 = (*(intptr_t *)D(0x8a93ec));
      API_SetWindowLongA((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))), (intptr_t)(-16), (intptr_t)(0x80000000));
      API_SetWindowLongA((intptr_t)((intptr_t)v30), (intptr_t)(-20), (intptr_t)(8));
      API_SetWindowPos((intptr_t)((intptr_t)v30), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(640), (intptr_t)(480), (intptr_t)(0x60u));
      break;
    case 2u:
      v26 = (*(intptr_t *)D(0x8a93ec));
      v27 = 0;
      v28 = 0;
      DesktopWindow = API_GetDesktopWindow();
      if ( DesktopWindow != 0 && API_GetClientRect((intptr_t)(DesktopWindow), (intptr_t)((RECT *)&(*Rect__f).top)) )
      {
        v27 = (((*Rect__f).bottom - (*Rect__f).top) / 2 - 320) & ((((*Rect__f).bottom - (*Rect__f).top) / 2 - 320 < 0) - 1);
        v28 = (((*v75__f) - (*Rect__f).right) / 2 - 240) & ((((*v75__f) - (*Rect__f).right) / 2 - 240 < 0) - 1);
      }
      API_SetWindowLongA((intptr_t)((intptr_t)v26), (intptr_t)(-16), (intptr_t)(-2134245376));
      API_SetWindowLongA((intptr_t)((intptr_t)v26), (intptr_t)(-20), (intptr_t)(0));
      API_SetWindowPos((intptr_t)((intptr_t)v26), (intptr_t)((intptr_t)0xFFFFFFFE), (intptr_t)(0), (intptr_t)(0), (intptr_t)(640), (intptr_t)(480), (intptr_t)(0x22u));
      API_GetClientRect((intptr_t)((intptr_t)v26), (intptr_t)(&(*v82__f)));
      API_SetWindowPos((intptr_t)((intptr_t)v26), (intptr_t)(0), (intptr_t)(v27), (intptr_t)(v28), (intptr_t)(1280 - (*v82__f).right), (intptr_t)(960 - (*v82__f).bottom), (intptr_t)(0x40u));
      left = (*Rect__f).left;
      break;
    case 4u:
      v22 = (int)F32(left, (5) * 4);
      v23 = (int)F32(left, (6) * 4);
      v24 = (*(intptr_t *)D(0x8a93ec));
      if ( v22 != 640 )
      {
        v25 = API_GetDesktopWindow();
        if ( API_GetClientRect((intptr_t)(v25), (intptr_t)((RECT *)&(*Rect__f).top)) )
        {
          if ( v22 <= (*Rect__f).bottom - (*Rect__f).top )
            v22 = (*Rect__f).bottom - (*Rect__f).top;
          if ( v23 <= (*v75__f) - (*Rect__f).right )
            v23 = (*v75__f) - (*Rect__f).right;
        }
      }
      API_SetWindowLongA((intptr_t)((intptr_t)v24), (intptr_t)(-16), (intptr_t)(0x80000000));
      API_SetWindowLongA((intptr_t)((intptr_t)v24), (intptr_t)(-20), (intptr_t)(8));
      API_SetWindowPos((intptr_t)((intptr_t)v24), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v22), (intptr_t)(v23), (intptr_t)(0x60u));
      left = (*Rect__f).left;
      break;
    default:
      break;
  }
  API_UpdateWindow((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  API_SetFocus((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a0e18)), D(0x8a9b6c));
  F32(left, (2) * 4) = 1;
  if ( RC_5b9cf0((intptr_t)(&SYSB(10352)), (intptr_t)(a2)) == 0 )
  {
    g_eng.h426110((char *)D(0x8a9530), ((char *)D(0x8a0dec)), 1, D(0x8a9b6c));
    g_eng.h5bf5db(0);
  }
  if ( RC_5b9fc0((intptr_t)(SYS32(10352)), (intptr_t)(&SYSB(10360)), (intptr_t)((int)(*(intptr_t *)D(0x8a93ec)))) == 0 )
  {
    g_eng.h426110((char *)D(0x8a9530), ((char *)D(0x8a0db8)), 1, D(0x8a9b6c));
    g_eng.h5bf5db(0);
  }
  tm_56de80((intptr_t *)((intptr_t)g_eng.sys));
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0da8)), 0, D(0x8a9b6c));
  strcpy((char *)OutputString, D(0x8a9b6c));
  if ( RC_5baed0((intptr_t)(OutputString), (intptr_t)((char **)&SYSB(10356)), (intptr_t)((int)(*(intptr_t *)D(0x8a93ec)))) == 0 )
  {
    g_eng.h408b90(D(0x8a9530), (char *)OutputString, 0, D(0x8a9b6c));
    SYS32(10356) = 0;
  }
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0d98)), 0, D(0x8a9b6c));
  if ( F32(CFG(), 540) != 1 )
  {
    v31 = (intptr_t)g_eng.op_new(0x22Cu);
    SETLB((*v84__f), 2);
    if ( v31 != 0 )
      v32 = (intptr_t)RC_5bbb10((intptr_t)(v31));
    else
      v32 = 0;
    SETLB((*v84__f), 1);
    SYS32(0x2884) = v32;
    RC_5bbeb0((intptr_t)(v32), (intptr_t)(50), (intptr_t)(8));
    g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0d88)), 0, D(0x8a9b6c));
  }
  if ( RC_5b7ee0((intptr_t)(&SYSB(10348))) == 0 )
  {
    g_eng.h426110((char *)D(0x8a9530), ((char *)D(0x8a0d68)), 1, D(0x8a9b6c));
    g_eng.h5bf5db(0);
  }
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0d58)), 0, D(0x8a9b6c));
  switch ( (unsigned int)F32(left, (1) * 4) )
  {
    case 0u:
    case 1u:
    case 3u:
    case 4u:
      if ( RC_5b89d0((intptr_t)(SYS32(10348)), (intptr_t)((int)(*(intptr_t *)D(0x8a93ec))), (intptr_t)(1)) == 0 )
      {
        g_eng.h426110((char *)D(0x8a9530), ((char *)D(0x8a0d2c)), 1, D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      break;   /* (retail: jnz to the default case; IDA: a return) */
    case 2u:
      if ( RC_5b89d0((intptr_t)(SYS32(10348)), (intptr_t)((int)(*(intptr_t *)D(0x8a93ec))), (intptr_t)(0)) == 0 )
      {
        g_eng.h426110((char *)D(0x8a9530), ((char *)D(0x8a0d2c)), 1, D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      break;   /* (retail: jnz to the default case; IDA: a return) */
    default:
      break;
  }
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0d10)), 0, D(0x8a9b6c));
  RC_582e90((intptr_t)(left));
  W_rp_5b8b40((intptr_t)(SYS32(10348)), (intptr_t)(&SYSB(860)), (intptr_t)(256), (intptr_t)(256), (intptr_t)(0), (intptr_t)(1));
  W_rp_5b9520((intptr_t)(SYS32(860)));
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0cf8)), 0, D(0x8a9b6c));
  v33 = (intptr_t *)&SYSB(10072);
  v34 = 1000;
  for ( m = 20; m != 0; --m )
  {
    v36 = (intptr_t)g_eng.op_new(0x50u);
    SETLB((*v84__f), 3);
    if ( v36 != 0 )
      inited = RC_5bd4d0((intptr_t)(v36));
    else
      inited = 0;
    *v33 = inited;
    SETLB((*v84__f), 1);
    F16(inited, 0x40) = v34 / 20;
    F32(F32(v33, 0), 0x44) = 1;
    RC_5bd3d0((intptr_t)(F32(v33, 0)), (intptr_t)(SYS32(10348)));
    v34 += 1000;
    ++v33;
  }
  v38 = (intptr_t *)&SYSB(10152);
  v39 = 1000;
  for ( n = 20; n != 0; --n )
  {
    v41 = (intptr_t)g_eng.op_new(0x50u);
    SETLB((*v84__f), 4);
    if ( v41 != 0 )
      v42 = RC_5bd4d0((intptr_t)(v41));
    else
      v42 = 0;
    *v38 = v42;
    SETLB((*v84__f), 1);
    F16(v42, 0x40) = v39 / 22;
    RC_5bd3d0((intptr_t)(F32(v38, 0)), (intptr_t)(SYS32(10348)));
    v39 += 1000;
    ++v38;
  }
  v43 = 0;
  v44 = (intptr_t *)&SYSB(10232);
  for ( ii = 5; ii != 0; --ii )
  {
    v46 = (intptr_t)g_eng.op_new(0x50u);
    SETLB((*v84__f), 5);
    if ( v46 != 0 )
      v47 = RC_5bd4d0((intptr_t)(v46));
    else
      v47 = 0;
    SETLB((*v84__f), 1);
    *v44 = v47;
    RC_5bd3b0((intptr_t)(v47), (intptr_t)(v43 / 5 + 1100), (intptr_t)(v43 / 5 + 1100), (intptr_t)(v43 / 5 + 1100));
    RC_5bd3d0((intptr_t)(F32(v44, 0)), (intptr_t)(SYS32(10348)));
    v43 += 800;
    ++v44;
  }
  v48 = &SYSB(10332);
  for ( jj = 4; jj != 0; --jj )
  {
    v50 = (intptr_t)g_eng.op_new(0x50u);
    SETLB((*v84__f), 6);
    if ( v50 != 0 )
      v51 = RC_5bd4d0((intptr_t)(v50));
    else
      v51 = 0;
    F32(v48, 0) = v51;
    v48 += 4;
    SETLB((*v84__f), 1);
  }
  RC_5bd3b0((intptr_t)(SYS32(10332)), (intptr_t)(600), (intptr_t)(600), (intptr_t)(600));
  RC_5bd3d0((intptr_t)(SYS32(10332)), (intptr_t)(SYS32(10348)));
  RC_5bd3b0((intptr_t)(SYS32(10336)), (intptr_t)(1000), (intptr_t)(600), (intptr_t)(600));
  RC_5bd3d0((intptr_t)(SYS32(10336)), (intptr_t)(SYS32(10348)));
  F32(SYS32(10344), 76) = 1;
  RC_5bd3b0((intptr_t)(SYS32(10344)), (intptr_t)(1000), (intptr_t)(1800), (intptr_t)(2000));
  RC_5bd3d0((intptr_t)(SYS32(10344)), (intptr_t)(SYS32(10348)));
  F32(SYS32(10340), 76) = 1;
  RC_5bd3b0((intptr_t)(SYS32(10340)), (intptr_t)(900), (intptr_t)(920), (intptr_t)(930));
  RC_5bd3d0((intptr_t)(SYS32(10340)), (intptr_t)(SYS32(10348)));
  v52 = (intptr_t *)&SYSB(10252);
  v53 = 1000;
  for ( kk = 20; kk != 0; --kk )
  {
    v55 = (intptr_t)g_eng.op_new(0x50u);
    SETLB((*v84__f), 7);
    if ( v55 != 0 )
      v56 = RC_5bd4d0((intptr_t)(v55));
    else
      v56 = 0;
    *v52 = v56;
    SETLB((*v84__f), 1);
    F16(v56, 0x40) = v53 / 20;
    F32(F32(v52, 0), 0x44) = 2;
    RC_5bd3d0((intptr_t)(F32(v52, 0)), (intptr_t)(SYS32(10348)));
    v53 += 1000;
    ++v52;
  }
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0ce0)), 0, D(0x8a9b6c));
  v57 = (intptr_t)g_eng.op_new(8u);
  v58 = v57;
  (*Rect__f).top = (intptr_t)v57;
  SETLB((*v84__f), 8);
  if ( v57 != 0 )
  {
    F16(v57, 0x4) = 500;
    F16(v57, 0x6) = 0;
    v59 = (intptr_t)g_eng.op_new(0x36B0u);
    v60 = 0;
    v61 = F16(v58, 0x4) == 0;
    F32(v58, 0x0) = (int)v59;
    if ( !v61 )
    {
      do
      {
        v62 = 7 * (uint16_t)v60++;
        F32(F32(v58, 0x0) + 4 * v62, 24) = 0;
      }
      while ( (uint16_t)v60 < F16(v58, 0x4) );
    }
    W_frame_cache_clear((intptr_t)(v58));
  }
  else
  {
    v58 = 0;
  }
  SYS32(10068) = v58;
  v63 = CFG();
  SETLB((*v84__f), 1);
  v64 = F32(v63, 332);
  if ( F32(D(0x8a9510), 0) != F32(v63, 304) || F32(D(0x8a9514), 0) != v64 )
  {
    F32(D(0x8a9510), 0) = F32(v63, 304);
    F32(D(0x8a9514), 0) = v64;
    for ( mm = 0; mm < 256; ++mm )
    {
      v66 = ((1.0 - cos((double)(F32(D(0x8a9514), 0) * mm / 1000) * (3.14159265358979 * 0.00392156862745098)))
           * 127.5
           * (double)F32(D(0x8a9510), 0)
           + (double)(F32(D(0x8a9514), 0) * mm / 1000 * (1000 - F32(D(0x8a9510), 0))))
          * 0.001;
      if ( v66 > 255.0 )
        v66 = 255.0;
      ((uint8_t *)D(0x8a9410))[mm] = (int64_t)v66;
    }
    W_slots_free_graded((intptr_t)(((intptr_t)g_eng.sys)));
    left = (*Rect__f).left;
  }
  RC_579bd0((intptr_t)(c), (intptr_t)(SYS32(10348)), (intptr_t)(1), (intptr_t)(SYS32(788)));
  RC_579a00((intptr_t)(SYS32(10348)), (intptr_t)(1u), (intptr_t)(*(void **)&SYSB(788)));
  RC_57a330((intptr_t)(*(void **)&SYSB(10348)), (intptr_t)(2u), (intptr_t)(*(void **)&SYSB(788)));
  v67 = *(void **)&SYSB(788);
  RC_563ef0((intptr_t)(*(ar_sound_slot **)&SYSB(956)), (intptr_t)(*(void **)&SYSB(10356)), (intptr_t)(*(void **)&SYSB(788)), (intptr_t)(0x4CBu), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(0));
  RC_563ef0((intptr_t)(*(ar_sound_slot **)&SYSB(960)), (intptr_t)(*(void **)&SYSB(10356)), (intptr_t)(v67), (intptr_t)(0x4CAu), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(0));
  RC_563ef0((intptr_t)(*(ar_sound_slot **)&SYSB(964)), (intptr_t)(*(void **)&SYSB(10356)), (intptr_t)(v67), (intptr_t)(0x4C8u), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(0));
  RC_563ef0((intptr_t)(*(ar_sound_slot **)&SYSB(968)), (intptr_t)(*(void **)&SYSB(10356)), (intptr_t)(v67), (intptr_t)(0x4C9u), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(0));
  RC_57ca40((intptr_t)(*(void **)&SYSB(10348)), (intptr_t)(3u), (intptr_t)(*(void **)&SYSB(788)));
  RC_57b280((intptr_t)(SYS32(10348)), (intptr_t)(3u), (intptr_t)(*(void **)&SYSB(788)));
  RC_5749b0((intptr_t)(*(void **)&SYSB(10348)), (intptr_t)(4u), (intptr_t)(*(void **)&SYSB(788)));
  RC_56e190((intptr_t)(*(void **)&SYSB(10348)), (intptr_t)(5u), (intptr_t)(*(void **)&SYSB(788)));
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0cc8)), 0, D(0x8a9b6c));
  RC_5752e0();
  if ( SYS32(784) != 0 )
  {
    v68 = (intptr_t)g_eng.op_new(0xCu);
    if ( v68 != 0 )
    {
      F32(v68, 0x0) = 0;
      v69 = v68;
    }
    else
    {
      v69 = 0;
    }
    SYS32(2728) = v69;
    RC_564110((intptr_t)(v69), (intptr_t)(SYS32(10356)), (intptr_t)(0x10u));
    g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0cb4)), 0, D(0x8a9b6c));
  }
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0ca4)), 0, D(0x8a9b6c));
  API_UpdateWindow((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  API_SetFocus((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  RC_583c90((intptr_t)(left), (intptr_t)(1));
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0c74)), 0, D(0x8a9b6c));
  g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0c58)), 0, D(0x8a9b6c));
  RC_583ee0();
  top = (*Rect__f).top;
  v71 = F32(v79__f, 0xc);
  while ( 2 )
  {
    if ( SYS32(0x2884) != 0 )
      RC_5bcb30((intptr_t)(SYS32(0x2884)));
    if ( SYS32(2728) != 0 )
      RC_5640b0((intptr_t)(SYS32(2728)));
    RC_583c90((intptr_t)(left), (intptr_t)(1));
    if ( F32(D(0x8a6b54), 0) == 0 )
      g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0c3c)), 0, D(0x8a9b6c));
    if ( SYS32(780) == 0 )
    {
      RC_5a4760((intptr_t)(&(*Rect__f).top));
      SETLB((*v84__f), 9);
      top = tm_56aea0(&(*Rect__f).top, (*v76__f));
      SETLB((*v84__f), 1);
      tm_41bbe0(&(*Rect__f).top);
    }
    (*v76__f) = 1;
    if ( F32(D(0x8a6b54), 0) == 0 )
      g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a0c24)), 0, D(0x8a9b6c));
    switch ( top )
    {
      case 0:
        if ( (uint16_t)v71 != 0 )
        {
          if ( (uint16_t)v71 == 1 )
          {
            ++v71;
            RC_59e5d0((intptr_t)(left), (intptr_t)(1));
          }
          else if ( (uint16_t)v71 == 2 )
          {
            v71 = 0;
            RC_59e5d0((intptr_t)(left), (intptr_t)(2));
          }
        }
        else
        {
          ++v71;
          RC_59e230((intptr_t)(left));
        }
        continue;
      case 6:
      case 8:
        v72 = F32(v79__f, 0x8);
        break;
      case 9:
        v72 = 9;
        break;
      case 26:
        if ( tm_564160(&(*v81__f)) != 6 && tm_56cd20(&(*v79__f)) != 6 )
          tm_59ec30(left, 0, 0, 1010);
        continue;
      case 27:
        tm_59ec30(left, 10020, 0, 0);
        continue;
      case 28:
        top = RC_56a670((intptr_t)(&(*v81__f)), (intptr_t)(&(*v77__f)), (intptr_t)(&(*v78__f)), (intptr_t)(&(*v80__f)));
        if ( top == 12 )
          tm_59ec30(left, (int)(*v77__f), (*v78__f), (*v80__f));
        continue;
      case 29:
        RC_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
        tm_568de0(&(*v81__f), 0);
        continue;
      case 30:
        RC_583fe0((intptr_t)(left));
        continue;
      default:
        continue;
    }
    break;
  }
  SETLB((*v84__f), 0);
  tm_41bbe0(&(*v81__f));
  (*v84__f) = -1;
  tm_41bbe0(&(*v79__f));
  return v72;
}

/* 0x564160: int __thiscall sub_564160(cls_41BBE0 *this) */
int32_t tm_564160(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    uint16_t i = 0;
    _Alignas(8) uint8_t frame_[0x40 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x40 */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x28 */

  v2 = 0;
  v3 = 0;
  tm_5644a0(c);
  v4 = F32(c, 0x0);
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)&(*v13__f)), (intptr_t)(0));
  v5 = W_win_mgr_open((intptr_t)(v4), (intptr_t)(32), (intptr_t)(392), (intptr_t)(576), (intptr_t)(80), (intptr_t)(1u), (intptr_t)(&(*v13__f)));
  F32(c, 0x8) = (int)v5;
  v6 = W_win_text_add((intptr_t)(v5), (intptr_t)(40), (intptr_t)(24), (intptr_t)(68), (intptr_t)(2));
  (*v12__f) = F32(c, 0x8);
  F32(c, 0xc) = (int)v6;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v12__f)), (intptr_t)(v6));
  W_text_metrics_set((intptr_t)(F32(c, 0xc)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  if ( tm_564780(c, 36) != 0 )
    v3 = 6;
  RC_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  v7 = F32(c, 0x8);
  if ( v7 != 0 )
  {
    F32(v7, 80) = 0;
    F32(c, 0x8) = 0;
    F32(c, 0xc) = 0;
  }
  if ( v3 != 6 )
  {
    tm_564690(&(*v13__f));
    while ( 1 )
    {
      do
      {
        while ( 1 )
        {
          v8 = W_menu_pace((intptr_t)(&(*v13__f))) - 1;
          if ( v8 != 0 )
            break;
          RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
          W_rd_48c820((intptr_t)(F32(c, 0x0)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(*(intptr_t *)&SYSB(864)));
          RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
        }
      }
      while ( v8 != 1 );
      if ( tm_5642e0(c, F32(((uint8_t *)(v13__f) + 0x20), 0), 0) == 6 )
        break;
      if ( (unsigned int)++v2 >= 0x14 )
        break;
      tm_56c930(F32(c, 0x0));
      v9 = (_DWORD *)F32(c, 0x0);
      for ( i = 0; i < F16(F32(c, 0x0), 6); ++i )
      {
        W_win_tick((intptr_t)(F32(*v9, 4 * i)));
        v9 = (_DWORD *)F32(c, 0x0);
      }
    }
  }
  tm_56dc20(0, 0, 0);
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
  tm_41bbe0(c);
  return v3;
}

/* 0x5642e0: int __thiscall sub_5642E0(cls_5642E0 *this, int a2, int a3) */
int32_t tm_5642e0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    uint32_t i = 0;
    intptr_t * v7 = 0;
    intptr_t j = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    uint8_t * v14 = 0;
    char * v15 = 0;
    intptr_t k = 0;
    uint32_t TickCount = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    intptr_t m = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * n = 0;
    int32_t * v24 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v27 = (uint8_t *)(frame_ + 0x0);   /* [44] at ebp-0x2c */

  if ( SYS32(0x2884) != 0 )
    RC_5bc0c0((intptr_t)(SYS32(0x2884)));
  v4 = SYS32(2728);
  if ( SYS32(2728) != 0 && F32(SYS32(2728), 8) != 0 )
  {
    v5 = 0;
    for ( i = API_GetTickCount(); v5 < F16(v4, 4); ++v5 )
      RC_5646c0((intptr_t)((int *)(F32(v4, 8) + 24 * v5)), (intptr_t)(i));
  }
  RC_5ba4c0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  RC_5ba2d0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  v7 = (intptr_t *)&SYSB(10364);
  for ( j = 2; j != 0; --j )
  {
    if ( *v7 != 0 )
    {
      RC_5ba650((intptr_t)(*v7));
      RC_49bf90((intptr_t)(*v7), (intptr_t)(1000 * F32(CFG(), 1340) / 1000));
    }
    ++v7;
  }
  v9 = F32(c, 0x4);
  v10 = (int *)v27;
  v11 = 11;
  v12 = &F32(v9, 0x114);
  do
  {
    v13 = *v12;
    *v12 = 0;
    *v10++ = v13;
    ++v12;
    --v11;
  }
  while ( v11 != 0 );
  if ( F32(v9, 0x0) != 0 )
    W_input_dik_produce((intptr_t)(v9), (intptr_t)(SYS32(10360)));
  v14 = &SYSB(10364);
  v15 = ((uint8_t *)(v9) + 0x4);
  for ( k = 2; k != 0; --k )
  {
    if ( F32(v15, 0) != 0 && F32(v14, 0) != 0 )
      RC_468b10((intptr_t)(v9), (intptr_t)(*(_DWORD **)v14), (intptr_t)(a3));
    v15 += 4;
    v14 += 4;
  }
  TickCount = API_GetTickCount();
  v18 = v9 + 320 * 1;
  v19 = v27;
  for ( m = 11; m != 0; --m )
  {
    if ( F32(v18 + (-1) * 0x140, 0x114) != 0 )
    {
      if ( *v19 == 0 )
        F32(v18, 0x0) = TickCount;
    }
    else
    {
      F32(v18, 0x0) = 0;
    }
    ++v19;
    v18 = ((uint8_t *)v18 + 4);
  }
  v21 = 63;
  v22 = F32(c, 0x4);
  for ( n = (_DWORD *)(v22 + 264); ; --n )
  {
    v24 = (_DWORD *)*n;
    if ( *(_DWORD *)*n == 34 && v24[2] == 1 && (unsigned int)(a2 - v24[1]) <= 0x64 )
      break;
    if ( --v21 < 0 )
      return 0;
  }
  F32(F32(v22 + 4 * v21, 12), 0) = 0;
  return 6;
}

/* 0x5644a0: int __thiscall sub_5644A0(cls_41BBE0 *this) */
int32_t tm_5644a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    void * v2 = 0;
    void * v3 = 0;
    uint16_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    void * v7 = 0;
    intptr_t v8 = 0;
    int v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    char * v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t i = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    int32_t ** v19 = 0;
    intptr_t j = 0;
    int32_t * v21 = 0;
    intptr_t result = 0;
    intptr_t * v23 = 0;
    intptr_t k = 0;

  v2 = (void *)F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    RC_56c640((intptr_t)(F32(c, 0x0)));
    g_eng.op_delete((void *)v2);
    F32(c, 0x0) = 0;
  }
  v3 = (void *)F32(c, 0x4);
  if ( v3 != 0 )
  {
    W_input_dtor((intptr_t)(F32(c, 0x4)));
    g_eng.op_delete((void *)v3);
    F32(c, 0x4) = 0;
  }
  v4 = (intptr_t)g_eng.op_new(0x10u);
  if ( v4 != 0 )
  {
    F32(v4, 0) = 0;
    v4[2] = 0;
    v4[3] = 0;
    v5 = (int)v4;
  }
  else
  {
    v5 = 0;
  }
  F32(c, 0x0) = v5;
  v6 = SYS32(10020);
  F32(v5, 8) = SYS32(10016);
  F32(v5, 12) = v6;
  F16(v5, 6) = 0;
  F16(v5, 4) = 12;
  v7 = (intptr_t)g_eng.op_new(0x30u);
  v8 = 0;
  v9 = F16(v5, 4) == 0;
  F32(v5, 0) = v7;
  if ( !v9 )
  {
    do
    {
      v10 = (intptr_t)g_eng.op_new(0x84u);
      if ( v10 != 0 )
      {
        v10[18] = 0;
        v10[1] = 0;
      }
      else
      {
        v10 = 0;
      }
      v11 = (uint16_t)v8++;
      F32((F32(v5, 0) + 4 * v11), 0) = v10;
    }
    while ( (uint16_t)v8 < F16(v5, 4) );
  }
  v12 = (char *)(intptr_t)g_eng.op_new(0x170u);
  v13 = (int)v12;
  if ( v12 != 0 )
  {
    v14 = v12 + 12;
    for ( i = 64; i != 0; --i )
      *v14++ = (intptr_t)g_eng.op_new(0xCu);
    v16 = v13;
  }
  else
  {
    v16 = 0;
  }
  v17 = 11;
  F32(c, 0x4) = v16;
  F32(v16, 0) = 1;
  F32(v16, 4) = 1;
  F32(v16, 8) = 1;
  F16(v16, 364) = 0;
  v18 = (_DWORD *)(v16 + 276);
  do
  {
    v18[11] = 0;
    *v18++ = 0;
    --v17;
  }
  while ( v17 != 0 );
  v19 = (_DWORD **)(v16 + 12);
  for ( j = 64; j != 0; --j )
  {
    v21 = *v19++;
    *v21 = 0;
  }
  F32(v16, 268) = 0;
  F32(v16, 272) = 0;
  F32(c, 0x8) = 0;
  F32(c, 0xc) = 0;
  RC_5ba4c0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  result = RC_5ba2d0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  v23 = (intptr_t *)&SYSB(10364);
  for ( k = 2; k != 0; --k )
  {
    if ( *v23 != 0 )
    {
      RC_5ba650((intptr_t)(*v23));
      result = RC_49bf90((intptr_t)(*v23), (intptr_t)(1000 * F32(CFG(), 1340) / 1000));
    }
    ++v23;
  }
  return result;
}

/* 0x564690: DWORD __thiscall sub_564690(cls_564690 *this) */
int32_t tm_564690(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint32_t result = 0;

  F32(c, 0x10) = 16;
  F32(c, 0x0) = 0;
  F32(c, 0x4) = 17;
  F32(c, 0x8) = 0;
  F32(c, 0x1c) = 0;
  F32(c, 0xc) = 0;
  F32(c, 0x24) = 0;
  F32(c, 0x18) = 0;
  result = API_GetTickCount();
  F32(c, 0x14) = result;
  return result;
}

/* 0x564780: int __thiscall sub_564780(cls_41BBE0 *this, int a2) */
int32_t tm_564780(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
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
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    char * v20 = 0;
    char * v21 = 0;
    char * v22 = 0;
    char * v23 = 0;
    char * v24 = 0;
    char * v25 = 0;
    char * v26 = 0;
    char * v27 = 0;
    char * v28 = 0;
    char * v29 = 0;
    char * v30 = 0;
    char * v31 = 0;
    char * v32 = 0;
    char * v33 = 0;
    char * v34 = 0;
    char * v35 = 0;
    char * v36 = 0;
    char * v37 = 0;
    char * v38 = 0;
    char * v39 = 0;
    char * v40 = 0;
    char * v41 = 0;
    char * v42 = 0;
    char * v43 = 0;
    char * v44 = 0;
    char * v45 = 0;
    char * v46 = 0;
    char * v47 = 0;
    char * v48 = 0;
    int16_t v49 = 0;
    char * v50 = 0;
    intptr_t v51 = 0;
    char * v52 = 0;
    char * v53 = 0;
    char * v54 = 0;
    char * v55 = 0;
    char * v56 = 0;
    char * v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    int32_t * v62 = 0;
    char * v63 = 0;
    int32_t * v64 = 0;
    intptr_t v65 = 0;
    int32_t * v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    int32_t * v72 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x43c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v80 = (char *)(frame_ + 0x18);   /* [512] at ebp-0x424 */
    uint8_t *v81 = (uint8_t *)(frame_ + 0x218);   /* [36] at ebp-0x224 */
    char *Buffer = (char *)(frame_ + 0x23c);   /* [512] at ebp-0x200 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x43c */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x438 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x434 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x430 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x42c */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x428 */

  (*v76__f) = 0;
  (*v78__f) = 1;
  (*v77__f) = 0;
  switch ( a2 )
  {
    case 31:
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(160), (intptr_t)(16), (intptr_t)(456), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(7u), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(((char *)D(0x8a1044))), (intptr_t)(1));
      W_grid_col_place((intptr_t)((*v74__f)), (intptr_t)(1), (intptr_t)(192), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v45 = tm_566570(3, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(3), (intptr_t)(v45));
      v46 = tm_566570(4, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(4), (intptr_t)(v46));
      v47 = tm_566570(9, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(9), (intptr_t)(v47));
      v48 = tm_566570(8, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(8), (intptr_t)(v48));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(28), (intptr_t)(((char *)D(0x8a110c))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(29), (intptr_t)(((char *)D(0x8a10fc))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(27), (intptr_t)(((char *)D(0x8a10ec))));
      break;
    case 32:
      v3 = 0;
      v4 = 2;
      (*v79__f) = F32(c, 0x0);
      v5 = 24;
      v6 = 292;
      if ( ((char *)D(0x8a1194)) != 0 )
      {
        v6 = 324;
        v5 = 56;
        v3 = 1;
        v4 = 3;
      }
      W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v81), (intptr_t)(v3));
      v7 = W_win_mgr_open((intptr_t)((*v79__f)), (intptr_t)(160), (intptr_t)(16), (intptr_t)(456), (intptr_t)(v6), (intptr_t)(v4), (intptr_t)(v81));
      v8 = v7;
      if ( ((char *)D(0x8a1194)) != 0 )
        W_win_title_add((intptr_t)(v7), (intptr_t)(((char *)D(0x8a1194))));
      v9 = W_win_grid_new((intptr_t)(v8), (intptr_t)(40), (intptr_t)(v5), (intptr_t)(9), (intptr_t)(2), (intptr_t)(9), (intptr_t)(0));
      W_grid_col_place((intptr_t)(v9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v9), (intptr_t)(1));
      (*v75__f) = v8;
      (*v74__f) = v9;
      F32(F32(v9, 0x178), 36) = 192;
      F32(F32(v9, 0x178), 40) = 0;
      F32(F32(v9, 0x178), 44) = 0;
      F32(F32(v9, 0x178), 48) = 0;
      strcpy(v80, ((char *)D(0x8a1180)));
      v10 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(5));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(v80));
      strcpy(v80, ((char *)D(0x8a1174)));
      v11 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(10));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v11), (intptr_t)(0), (intptr_t)(v80));
      strcpy(v80, ((char *)D(0x8a1168)));
      v12 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(11));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(v80));
      strcpy(v80, ((char *)D(0x8a1158)));
      v13 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(20));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(v80));
      strcpy(v80, ((char *)D(0x8a1144)));
      v14 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(7));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(v80));
      strcpy(v80, ((char *)D(0x8a112c)));
      v15 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(22));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(v80));
      strcpy(v80, ((char *)D(0x8a1120)));
      v16 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(6));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(v80));
      v17 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(28));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v17), (intptr_t)(0), (intptr_t)(((char *)D(0x8a110c))));
      v18 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(29));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(((char *)D(0x8a10fc))));
      v19 = W_grid_row_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(27));
      W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(((char *)D(0x8a10ec))));
      break;
    case 33:
      v49 = 4;
      if ( SYS32(784) != 0 )
        v49 = 7;
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(160), (intptr_t)(16), (intptr_t)(456), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(v49 + 3), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(((char *)D(0x8a102c))), (intptr_t)(1));
      W_grid_col_place((intptr_t)((*v74__f)), (intptr_t)(1), (intptr_t)(192), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v50 = tm_566570(13, v80);
      v51 = W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(13), (intptr_t)(v50));
      if ( F32(CFG(), 540) == 1 )
        F32(F32((*v74__f), 0x17c) + 16 * v51, 8) = 0;
      v52 = tm_566570(12, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(12), (intptr_t)(v52));
      v53 = tm_566570(14, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(14), (intptr_t)(v53));
      v54 = tm_566570(18, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(18), (intptr_t)(v54));
      if ( SYS32(784) != 0 )
      {
        v55 = tm_566570(15, v80);
        W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(15), (intptr_t)(v55));
        v56 = tm_566570(17, v80);
        W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(17), (intptr_t)(v56));
        v57 = tm_566570(16, v80);
        W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(16), (intptr_t)(v57));
      }
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(28), (intptr_t)(((char *)D(0x8a110c))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(29), (intptr_t)(((char *)D(0x8a10fc))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(27), (intptr_t)(((char *)D(0x8a10ec))));
      break;
    case 36:
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(32), (intptr_t)(32), (intptr_t)(400), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(3u), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(0), (intptr_t)(1));
      v20 = ((uint8_t *)((*v74__f)) + 0x178);
      F32(F32(((uint8_t *)((*v74__f)) + 0x178), 0), 36) = 160;
      F32(F32(v20, 0), 40) = 0;
      F32(F32(v20, 0), 44) = 0;
      F32(F32(v20, 0), 48) = 0;
      v21 = tm_566570(3, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(3), (intptr_t)(v21));
      v22 = tm_566570(4, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(4), (intptr_t)(v22));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(30), (intptr_t)(((char *)D(0x8a10e0))));
      (*v77__f) = 1;
      break;
    case 37:
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(32), (intptr_t)(32), (intptr_t)(400), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(5u), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(((char *)D(0x8a10cc))), (intptr_t)(1));
      W_grid_col_place((intptr_t)((*v74__f)), (intptr_t)(1), (intptr_t)(160), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v23 = tm_566570(73, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(73), (intptr_t)(v23));
      v24 = tm_566570(74, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(74), (intptr_t)(v24));
      v25 = tm_566570(75, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(75), (intptr_t)(v25));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(28), (intptr_t)(((char *)D(0x8a110c))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(30), (intptr_t)(((char *)D(0x8a10c4))));
      tm_419900((*v74__f), 3, 30);
      (*v77__f) = 1;
      break;
    case 38:
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(160), (intptr_t)(16), (intptr_t)(456), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(5u), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(((char *)D(0x8a1068))), (intptr_t)(1));
      W_grid_col_place((intptr_t)((*v74__f)), (intptr_t)(1), (intptr_t)(192), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v43 = tm_566570(46, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(46), (intptr_t)(v43));
      v44 = tm_566570(47, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(47), (intptr_t)(v44));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(28), (intptr_t)(((char *)D(0x8a110c))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(29), (intptr_t)(((char *)D(0x8a10fc))));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(27), (intptr_t)(((char *)D(0x8a10ec))));
      break;
    case 39:
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(32), (intptr_t)(8), (intptr_t)(480), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(0xBu), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(((char *)D(0x8a108c))), (intptr_t)(1));
      W_grid_col_place((intptr_t)((*v74__f)), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v33 = tm_566570(76, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(76), (intptr_t)(v33));
      v34 = tm_566570(77, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(77), (intptr_t)(v34));
      v35 = tm_566570(78, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(78), (intptr_t)(v35));
      v36 = tm_566570(79, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(79), (intptr_t)(v36));
      v37 = tm_566570(80, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(80), (intptr_t)(v37));
      v38 = tm_566570(81, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(81), (intptr_t)(v38));
      v39 = tm_566570(82, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(82), (intptr_t)(v39));
      v40 = tm_566570(83, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(83), (intptr_t)(v40));
      v41 = tm_566570(84, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(84), (intptr_t)(v41));
      v42 = tm_566570(85, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(85), (intptr_t)(v42));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(30), (intptr_t)(((char *)D(0x8a10c4))));
      goto LABEL_11;
    case 40:
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(32), (intptr_t)(8), (intptr_t)(480), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(8u), (intptr_t)(2u), (intptr_t)((intptr_t *)&(*v75__f)), (intptr_t)((int *)&(*v74__f)), (intptr_t)(((char *)D(0x8a10b0))), (intptr_t)(1));
      W_grid_col_place((intptr_t)((*v74__f)), (intptr_t)(1), (intptr_t)(200), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v26 = tm_566570(90, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(90), (intptr_t)(v26));
      v27 = tm_566570(91, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(91), (intptr_t)(v27));
      v28 = tm_566570(92, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(92), (intptr_t)(v28));
      v29 = tm_566570(86, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(86), (intptr_t)(v29));
      v30 = tm_566570(87, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(87), (intptr_t)(v30));
      v31 = tm_566570(88, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(88), (intptr_t)(v31));
      v32 = tm_566570(89, v80);
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(0), (intptr_t)(89), (intptr_t)(v32));
      W_grid_row_text_add((intptr_t)((*v74__f)), (intptr_t)(3), (intptr_t)(30), (intptr_t)(((char *)D(0x8a10a0))));
LABEL_11:
      (*v77__f) = 1;
      tm_419900((*v74__f), 3, 30);
      break;
    default:
      break;
  }
LABEL_21:
  tm_565810(SYS32(800));
LABEL_22:
  while ( 2 )
  {
    while ( 2 )
    {
      v58 = (*v74__f);
      do
      {
LABEL_23:
        v59 = 0;
        if ( (*v78__f) != 0 )
        {
          v60 = F32(v58, 0x174);
          v61 = 0;
          (*v78__f) = 0;
          if ( F32(v60, 16) > 0 )
          {
            do
            {
              v62 = (_DWORD *)(16 * v59 + F32(v58, 0x17c));
              if ( *v62 == 0 )
              {
                v63 = tm_566a80(
                        v62[1],
                        F32(CFG() + 28 * (uint16_t)v62[1], 12),
                        Buffer);
                W_grid_cell_text((intptr_t)((*v74__f)), (intptr_t)(v59), (intptr_t)(1), (intptr_t)(v63));
                v58 = (*v74__f);
              }
              v59 = (uint16_t)++v61;
            }
            while ( (uint16_t)v61 < F32(F32(v58, 0x174), 16) );
          }
        }
        (*v79__f) = strcpy(Buffer, D(0x8a9b6c));
        v64 = (_DWORD *)(F32(v58, 0x17c) + 16 * F32(F32(v58, 0x174), 20));
        if ( *v64 != 0 )
        {
          if ( *v64 == 3 )
          {
            switch ( v64[1] )
            {
              case 0x1B:
                strcpy(Buffer, ((char *)D(0x8a0fe0)));
                break;
              case 0x1C:
                strcpy(Buffer, ((char *)D(0x8a1010)));
                break;
              case 0x1D:
                strcpy(Buffer, ((char *)D(0x8a0ff8)));
                break;
              case 0x1E:
                switch ( a2 )
                {
                  case '$':
                    strcpy(Buffer, ((char *)D(0x8a0f88)));
                    break;
                  case '%':
                    strcpy(Buffer, ((char *)D(0x8a0f74)));
                    break;
                  case '\'':
                    strcpy(Buffer, ((char *)D(0x8a0f64)));
                    break;
                  case '(':
                    strcpy(Buffer, ((char *)D(0x8a0f3c)));
                    break;
                  default:
                    goto LABEL_40;
                }
                break;
              default:
                break;
            }
          }
        }
        else
        {
          tm_566850(v64[1], Buffer);
        }
LABEL_40:
        W_ctrl_text_set((intptr_t)(F32(c, 0xc)), (intptr_t)(Buffer));
        v65 = tm_565d10(c, (*v74__f), 0, 0, 0, 0);
        if ( v65 == 11 )
        {
          if ( (*v77__f) != 0 )
            (*v76__f) = 11;
LABEL_72:
          tm_565840(SYS32(800));
          tm_56ded0((int)((intptr_t)g_eng.sys));
          tm_56dfe0((int)((intptr_t)g_eng.sys));
          tm_5658c0(SYS32(800));
          goto LABEL_73;
        }
        if ( v65 != 12 )
        {
          if ( v65 == 13 )
            goto LABEL_22;
LABEL_69:
          (*v76__f) = v65;
          goto LABEL_73;
        }
        v58 = (*v74__f);
        v66 = (_DWORD *)(16 * F32(F32((*v74__f), 0x174), 20) + F32((*v74__f), 0x17c));
        if ( *v66 == 0 )
        {
          v65 = tm_567ba0(c, v66[1]);
          if ( v65 == 0 )
            goto LABEL_22;
          if ( v65 == 12 )
          {
            (*v78__f) = 1;
            goto LABEL_22;
          }
          goto LABEL_69;
        }
      }
      while ( *v66 != 3 );
      v65 = tm_568b40(c, (_DWORD *)v66[1]);
      if ( v65 != 0 )
      {
        if ( v65 == 11 )
          continue;
        goto LABEL_69;
      }
      break;
    }
    v58 = (*v74__f);
    v67 = F32((*v74__f), 0x174);
    switch ( F32(16 * F32((v67 + 20), 0) + F32((*v74__f), 0x17c), 4) )
    {
      case 0x1B:
        goto LABEL_72;
      case 0x1C:
        v70 = 0;
        if ( F32(v67, 16) > 0 )
        {
          v71 = 0;
          do
          {
            v72 = (_DWORD *)(F32(v58, 0x17c) + 16 * v71);
            if ( *v72 == 0 )
            {
              tm_5657f0(
                SYS32(800),
                v72[1],
                F32(CFG() + 28 * (uint16_t)v72[1], 4));
              v58 = (*v74__f);
            }
            v71 = (uint16_t)++v70;
          }
          while ( (uint16_t)v70 < F32(F32(v58, 0x174), 16) );
        }
        (*v78__f) = 1;
        goto LABEL_23;
      case 0x1D:
        break;
      case 0x1E:
        switch ( a2 )
        {
          case '$':
          case '\'':
          case '(':
            goto LABEL_72;
          case '%':
            v69 = CFG();
            if ( F32(CFG(), 2056) != 0
              || F32(v69, 2084) != 0
              || F32(v69, 2112) != 0 )
            {
              goto LABEL_72;
            }
            v65 = RC_565b00((intptr_t)(c), (intptr_t)(((char *)D(0x8a0f08))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u));
            if ( v65 == 0 )
              continue;
            goto LABEL_69;
          default:
            goto LABEL_23;
        }
      case 0x29:
        tm_565840(SYS32(800));
        v68 = tm_564780(c, 0);
        if ( v68 == 0 )
          goto LABEL_72;
        if ( v68 != 6 )
          goto LABEL_21;
        (*v76__f) = 6;
        break;
      default:
        goto LABEL_23;
    }
    break;
  }
LABEL_73:
  result = (*v76__f);
  *(_DWORD *)((uint8_t *)((*v75__f) + (1) * 0x50) + 0x0) = 0;
  return result;
}

/* 0x419900: int __thiscall picker_seek_value_419900(cls_419900 *this, int a2, int a3) */
int32_t tm_419900(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    int32_t * v7 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;

  v3 = F32(c, 0x174);
  v4 = 0;
  v5 = F32(v3, 16);
  if ( v5 <= 0 )
    return 0;
  v6 = 0;
  while ( 1 )
  {
    v7 = (_DWORD *)(F32(c, 0x17c) + 16 * v6);
    if ( *v7 == a2 && v7[1] == a3 )
      break;
    v6 = ++v4;
    if ( v4 >= v5 )
      return 0;
  }
  v9 = 0;
  F32(v3, 20) = v4;
  v10 = (_DWORD *)F32(c, 0x174);
  v11 = v10[3];
  v12 = v10[5];
  while ( v12 < v9 || v12 >= v11 + v9 )
    v9 += v11;
  if ( v9 != v10[6] )
    v10[6] = v9;
  return 1;
}

/* 0x5657f0: int __thiscall newgame_picker_dispatch_5657F0(cls_5657F0 *this, unsigned __int16 a2, int a3) */
int32_t tm_5657f0(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = F32(c, 0x0);
  F32(F32(c, 0x0) + 28 * a2, 12) = a3;
  return result;
}

/* 0x565810: int __thiscall sub_565810(cls_565810 *this) */
int32_t tm_565810(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t i = 0;
    intptr_t v2 = 0;
    intptr_t result = 0;

  for ( i = 0; (uint16_t)i < F16(c, 0x4); F32(result, 12) = F32(result, 8) )
  {
    v2 = 7 * (uint16_t)i++;
    result = F32(c, 0x0) + 4 * v2;
  }
  return result;
}

/* 0x565840: void __fastcall sub_565840(cls_565840 *this) */
void tm_565840(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    uint16_t v6 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v1 = 0;
  for ( (*i__f) = c; v1 < F16(c, 2 * 2); ++v1 )
  {
    v2 = F32(F32(c, 0) + 28 * v1, 16);
    v3 = F32(F32(c, 0) + 28 * v1, 12);
    v4 = F32(c, 0) + 28 * v1;
    F32(v4, 8) = v3;
    if ( v2 != 0 )
    {
      v5 = F16(v4, 22);
      v6 = 0;
      if ( v5 != 0 )
      {
        while ( F32((v2 + 8 * v6), 0) != v3 )
        {
          if ( ++v6 >= v5 )
            goto LABEL_8;
        }
        F32(v4, 24) = F32(v2 + 8 * v6, 4);
LABEL_8:
        c = (*i__f);
      }
    }
  }
}

/* 0x5658c0: int __thiscall sub_5658C0(cls_5658C0 *this) */
int32_t tm_5658c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    char * i = 0;
    uint16_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    char * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    _Alignas(8) uint8_t frame_[0x440 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v19 = (intptr_t *)(frame_ + 0x10);   /* [2] at ebp-0x430 */
    intptr_t *v22 = (intptr_t *)(frame_ + 0x20);   /* [52] at ebp-0x420 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x440 */
    uint32_t *const Buffer__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x43c */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x438 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x434 */
    uint32_t *const nNumberOfBytesToWrite__f = (uint32_t *)(frame_ + 0x18);   /* ebp-0x428 */
    uint32_t *const v21__f = (uint32_t *)(frame_ + 0x1c);   /* ebp-0x424 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x43c);   /* ebp-0x4 */

  RC_5b6060((intptr_t)(v22));
  v2 = F32(c, 4 * 67);
  v3 = 0;
  i = 0;
  v5 = 0;
  (*v23__f) = 0;
  (*v17__f) = 0;
  (*v15__f) = 0;
  (*v18__f) = 12195365;
  if ( v2 == 0 )
  {
    if ( RC_5b60e0((intptr_t)(v22), (intptr_t)(((uint8_t *)(c) + 0x6)), (intptr_t)(0)) == 0 )
      goto LABEL_22;
    goto LABEL_8;
  }
  (*v18__f) = 12198962;
  v6 = (intptr_t)g_eng.op_new(0x124u);
  SETLB((*v23__f), 1);
  if ( v6 != 0 )
    v7 = RC_5b6580((intptr_t)(v6));
  else
    v7 = 0;
  SETLB((*v23__f), 0);
  (*v15__f) = v7;
  RC_5b65b0((intptr_t)(v7), (intptr_t)(512), (intptr_t)(1));
  RC_5b6db0((intptr_t)(v7), (intptr_t)(F32(c, 4 * 67)));
  if ( RC_5b6250((intptr_t)(v22), (intptr_t)((int)v7)) != 0 )
  {
LABEL_8:
    v8 = 0;
    for ( i = (char *)(intptr_t)g_eng.op_new(8 * F16(c, 0x4)); v8 < F16(c, 0x4); ++v8 )
    {
      v9 = (_DWORD *)(F32(c, 0x0) + 28 * v8);
      if ( *v9 != 0 )
      {
        v10 = v5++;
        v11 = &i[8 * v10];
        F32(v11, 0) = *v9;
        F32(v11, 4 * 1) = v9[2];
      }
    }
    v19[0] = (*v18__f);
    v19[1] = 100001;
    (*v21__f) = v5;
    (*nNumberOfBytesToWrite__f) = 8;
    (*Buffer__f) = 16;
    if ( RC_5b6410((intptr_t)(v22), (intptr_t)(&(*Buffer__f)), (intptr_t)(4u)) != 0 && RC_5b6410((intptr_t)(v22), (intptr_t)(v19), (intptr_t)((*Buffer__f))) != 0 )
    {
      v12 = 0;
      if ( (*v21__f) != 0 )
      {
        v13 = 0;
        while ( RC_5b6410((intptr_t)(v22), (intptr_t)(&i[8 * v13]), (intptr_t)((*nNumberOfBytesToWrite__f))) != 0 )
        {
          v13 = (uint16_t)++v12;
          if ( (uint16_t)v12 >= (*v21__f) )
            goto LABEL_18;
        }
      }
      else
      {
LABEL_18:
        RC_5b64b0((intptr_t)(v22));
        if ( (*v15__f) != 0 )
          RC_5b6990((intptr_t)((*v15__f)), (intptr_t)(((uint8_t *)(c) + 0x6)), (intptr_t)(5000));
        (*v17__f) = 1;
      }
    }
  }
  v3 = (*v15__f);
LABEL_22:
  RC_5b64b0((intptr_t)(v22));
  if ( i != 0 )
    g_eng.op_delete((void *)i);
  if ( v3 != 0 )
  {
    RC_5b65a0((intptr_t)(v3));
    g_eng.op_delete((void *)v3);
  }
  (*v23__f) = -1;
  RC_5b60d0((intptr_t)(v22));
  return (*v17__f);
}

/* 0x565d10: int __thiscall sub_565D10(cls_41BBE0 *this, _DWORD *a2, __int16 a3, int *a4, int a5, unsigned int a6) */
int32_t tm_565d10(wl_char *self, int32_t * a2, int16_t a3, int32_t * a4, int32_t a5, uint32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    uint32_t v9 = 0;
    uint32_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint32_t v16 = 0;
    uint32_t v17 = 0;
    uint32_t v18 = 0;
    intptr_t result = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
    int32_t ** v23 = 0;
    intptr_t i = 0;
    int32_t * v25 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x30 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c */
    uint32_t *const v31__f = (uint32_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    uint32_t *const v33__f = (uint32_t *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1c */
    uint32_t *const TickCount__f = (uint32_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x10 */

  v6 = 1;
  v7 = 0;
  (*v27__f) = 0;
  (*v29__f) = false;
  (*v30__f) = 0;
  if ( a3 == 1 )
    (*v27__f) = 1;
  else
    (*v29__f) = a3 == 2;
  v8 = 17;
  (*v31__f) = 0;
  (*v32__f) = 17;
  (*v33__f) = 0;
  (*v34__f) = 0;
  (*v36__f) = 0;
  (*TickCount__f) = API_GetTickCount();
  while ( 1 )
  {
    while ( 1 )
    {
      v9 = API_GetTickCount();
      if ( v7 == 0 )
      {
        (*v31__f) = v9;
        RC_5b1030();
LABEL_16:
        v7 = 2;
LABEL_17:
        (*v34__f) = v7;
        goto LABEL_18;
      }
      if ( v7 != 1 )
      {
        if ( v9 - (*v33__f) <= v8 )
        {
          v8 -= 16;
          (*v32__f) = v8;
          if ( v8 > 0x10 )
            goto LABEL_18;
          v7 = 1;
        }
        else
        {
          v8 = 0;
          v7 = 1;
          (*v32__f) = 0;
        }
        goto LABEL_17;
      }
      v10 = v8 - (*v31__f) + v9;
      if ( v10 > 0x64 )
        v10 = 100;
      v8 = v10;
      (*v31__f) = v9;
      (*v32__f) = v10;
      RC_5b1030();
      if ( v8 > 0x10 )
        goto LABEL_16;
LABEL_18:
      if ( v7 == 1 )
      {
        if ( v9 - (*TickCount__f) <= 0x3E8 )
        {
          ++(*v36__f);
        }
        else
        {
          (*v36__f) = 0;
          (*TickCount__f) = v9;
        }
      }
      else
      {
        (*v33__f) = v9;
      }
      if ( v7 != 1 )
        break;
      RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
      W_rd_48c820((intptr_t)(F32(c, 0x0)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(*(intptr_t *)&SYSB(864)));
      RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
    }
    W_timed_sounds_tick((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    RC_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    tm_566250(F32(c, 0x4), (*v27__f));
    if ( RC_43c110((intptr_t)(F32(c, 0x4)), (intptr_t)(v9), (intptr_t)(34)) != 0 )
      return 6;
    if ( a3 == 1 )
    {
      v13 = SYS32(10364);
      v14 = 0;
      (*v28__f) = 0;
      if ( RC_5ba390((intptr_t)(SYS32(10364))) != 0 )
      {
        v15 = 0;
        do
        {
          if ( RC_5ba3d0((intptr_t)(v13), (intptr_t)(v15)) != 0 )
          {
            v16 = RC_5ba3a0((intptr_t)(v13), (intptr_t)(v15));
            if ( v16 > 0x9C )
            {
              if ( v16 > 0x225 )
              {
                switch ( v16 )
                {
                  case 0x226u:
                    v14 = 302;
                    break;
                  case 0x227u:
                    v14 = 303;
                    break;
                  case 0x228u:
                    v14 = 304;
                    break;
                  case 0x229u:
                    v14 = 305;
                    break;
                  case 0x22Au:
                    v14 = 306;
                    break;
                  case 0x22Bu:
                    v14 = 307;
                    break;
                  default:
                    break;
                }
              }
              else if ( v16 == 549 )
              {
                v14 = 301;
              }
              else if ( v16 > 0x104 )
              {
                if ( v16 == 264 )
                {
                  v14 = 210;
                }
                else if ( v16 == 548 )
                {
                  v14 = 300;
                }
              }
              else if ( v16 == 260 )
              {
                v14 = 208;
              }
              else
              {
                v17 = v16 - 160;
                if ( v17 != 0 )
                {
                  v18 = v17 - 4;
                  if ( v18 != 0 )
                  {
                    if ( v18 == 92 )
                      v14 = 206;
                  }
                  else
                  {
                    v14 = 209;
                  }
                }
                else
                {
                  v14 = 207;
                }
              }
            }
            else if ( v16 == 156 )
            {
              v14 = 205;
            }
            else
            {
              switch ( v16 )
              {
                case '0':
                  v14 = 100;
                  break;
                case '1':
                  v14 = 101;
                  break;
                case '2':
                  v14 = 102;
                  break;
                case '3':
                  v14 = 103;
                  break;
                case '4':
                  v14 = 104;
                  break;
                case '5':
                  v14 = 105;
                  break;
                case '6':
                  v14 = 106;
                  break;
                case '7':
                  v14 = 107;
                  break;
                case '8':
                  v14 = 108;
                  break;
                case '9':
                  v14 = 109;
                  break;
                case ':':
                  v14 = 110;
                  break;
                case ';':
                  v14 = 111;
                  break;
                case '<':
                  v14 = 112;
                  break;
                case '=':
                  v14 = 113;
                  break;
                case '>':
                  v14 = 114;
                  break;
                case '?':
                  v14 = 115;
                  break;
                default:
                  break;
              }
            }
          }
          v15 = (uint16_t)++(*v28__f);
        }
        while ( (uint16_t)(*v28__f) < (unsigned int)RC_5ba390((intptr_t)(v13)) );
        if ( v14 != 0 )
        {
          *a4 = v14;
          return 25;
        }
        v8 = (*v32__f);
      }
      v7 = (*v34__f);
      v6 = 0;
    }
    else if ( a3 == 2 )
    {
      v12 = RC_568780((intptr_t)(SYS32(10360)));
      if ( v12 != 0 )
      {
        *a4 = v12;
        return 25;
      }
      v6 = 0;
    }
    if ( a2 != 0 )
    {
      switch ( W_ctrl_input((intptr_t)(F32(c, 0x4)), (intptr_t)(a2), (intptr_t)(v9), (intptr_t)(v6), (intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(0)) )
      {
        case 1:
        case 2:
          result = 13;
          break;
        case 3:
          return 12;
        case 4:
          W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(0));
          result = 11;
          break;
        default:
          goto LABEL_81;
      }
      return result;
    }
LABEL_81:
    if ( a5 != 0 && RC_566300((intptr_t)(F32(c, 0x4)), (intptr_t)(v9)) != 0 )
      break;
    if ( a6 != 0 && ++(*v30__f) >= a6 )
      return 0;
    tm_566350(c, v11);
  }
  v20 = 11;
  v21 = F32(c, 0x4);
  F16(v21, 364) = 0;
  v22 = (_DWORD *)(v21 + 276);
  do
  {
    v22[11] = 0;
    *v22++ = 0;
    --v20;
  }
  while ( v20 != 0 );
  v23 = (_DWORD **)(v21 + 12);
  for ( i = 64; i != 0; --i )
  {
    v25 = *v23++;
    *v25 = 0;
  }
  F32(v21, 268) = 0;
  F32(v21, 272) = 0;
  return 12;
}

/* 0x566250: DWORD __thiscall sub_566250(cls_468B10 *this, int a2) */
int32_t tm_566250(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    int32_t * v4 = 0;
    int32_t * v5 = 0;
    intptr_t v6 = 0;
    uint8_t * v7 = 0;
    char * v8 = 0;
    intptr_t i = 0;
    uint32_t result = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t j = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v14 = (uint8_t *)(frame_ + 0x0);   /* [44] at ebp-0x2c */

  v2 = 11;
  v4 = (int *)v14;
  v5 = &F32(c, 0x114);
  do
  {
    v6 = *v5;
    *v5 = 0;
    *v4++ = v6;
    ++v5;
    --v2;
  }
  while ( v2 != 0 );
  if ( F32(c, 0x0) != 0 )
    W_input_dik_produce((intptr_t)(c), (intptr_t)(SYS32(10360)));
  v7 = &SYSB(10364);
  v8 = ((uint8_t *)(c) + 0x4);
  for ( i = 2; i != 0; --i )
  {
    if ( F32(v8, 0) != 0 && F32(v7, 0) != 0 )
      RC_468b10((intptr_t)(c), (intptr_t)(*(_DWORD **)v7), (intptr_t)(a2));
    v8 += 4;
    v7 += 4;
  }
  result = API_GetTickCount();
  v11 = c + 320 * (1);
  v12 = v14;
  for ( j = 11; j != 0; --j )
  {
    if ( F32(v11 + (-1) * 0x140, 0x114) != 0 )
    {
      if ( *v12 == 0 )
        F32(v11, 0x0) = result;
    }
    else
    {
      F32(v11, 0x0) = 0;
    }
    ++v12;
    v11 = ((uint8_t *)v11 + 4);
  }
  return result;
}

/* 0x566350: _DWORD *__fastcall sub_566350(cls_41BBE0 *this, _DWORD *a2) */
int32_t tm_566350(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t i = 0;
    int32_t * result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    uint16_t v17 = 0;
    int v18 = 0;
    uint16_t v19 = 0;
    uint16_t v20 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */

  v3 = 0;
  v4 = 0;
  for ( i = F32(c, 0x0); v4 < F16(i, 6); ++v4 )
    W_win_anim((intptr_t)(F32(F32(i, 0), 4 * v4) /*edx*/));
  result = (_DWORD *)F32(c, 0x0);
  v7 = 0;
  for ( (*j__f) = 0; (uint16_t)v7 < F16(F32(c, 0x0), 6); result = (_DWORD *)F32(c, 0x0) )
  {
    v8 = F32((*result + 4 * (uint16_t)v7), 0);
    (*v25__f) = v8;
    if ( F32(v8, 4) != 0 && (F32(v8, 84) >= 1000 || F32(v8, 88) != 0) )
    {
      v9 = 0;
      (*v23__f) = 0;
      if ( F16(v8, 78) != 0 )
      {
        do
        {
          v10 = F32(v8, 72);
          v11 = F32((v10 + 4 * (uint16_t)v9), 0);
          if ( F32(v11, 8) == 1 )
          {
            v12 = F32(v11, 372);
            v13 = v12[6];
            (*v21__f) = v13;
            if ( v13 < v12[4] )
            {
              v14 = 16 * v13;
              (*v22__f) = 16 * v13;
              do
              {
                v15 = v14 + F32(v11, 380);
                if ( v12[2] > 0 )
                {
                  v16 = 0;
                  do
                  {
                    if ( F32(v16 + F32((v15 + 12), 0), 4) != 0 )
                    {
                      W_icon_tick((intptr_t)(F32(F32(F32((v11 + 380), 0) + v14, 12), v16 + 4)));
                      v14 = (*v22__f);
                    }
                    ++v3;
                    v16 += 24;
                  }
                  while ( v3 < F32(F32((v11 + 372), 0), 8) );
                  v8 = (*v25__f);
                  v13 = (*v21__f);
                  v3 = 0;
                }
                v12 = F32(v11, 372);
                ++v13;
                v14 += 16;
                (*v21__f) = v13;
                (*v22__f) = v14;
              }
              while ( v13 < v12[4] );
              v9 = (*v23__f);
            }
            F32(v11, 28) = 0;
          }
          else if ( F32(v11, 8) == 2 )
          {
            tm_56d910(F32((v10 + 4 * (uint16_t)v9), 0));
          }
          if ( F32(v11, 132) != 0 )
          {
            v17 = F16(v11, 206);
            if ( v17 != 0 && ++F16(v11, 212) >= F16(v11, 208) )
            {
              v18 = ++F16(v11, 210) < v17;
              F16(v11, 212) = 0;
              if ( !v18 )
                F16(v11, 210) = 0;
            }
            v19 = F16(v11, 298);
            if ( v19 != 0 && ++F16(v11, 304) >= F16(v11, 300) )
            {
              v18 = ++F16(v11, 302) < v19;
              F16(v11, 304) = 0;
              if ( !v18 )
                F16(v11, 302) = 0;
            }
          }
          if ( F32(v11, 32) != 0 )
          {
            v20 = F16(v11, 110);
            if ( v20 != 0 && ++F16(v11, 116) >= F16(v11, 112) )
            {
              v18 = ++F16(v11, 114) < v20;
              F16(v11, 116) = 0;
              if ( !v18 )
                F16(v11, 114) = 0;
            }
          }
          (*v23__f) = ++v9;
        }
        while ( (uint16_t)v9 < F16(v8, 78) );
        v7 = (*j__f);
      }
    }
    (*j__f) = ++v7;
  }
  return result;
}

/* 0x56d910: int __thiscall sub_56D910(cls_56D910 *this) */
int32_t tm_56d910(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t result = 0;
    uint16_t v3 = 0;
    intptr_t v4 = 0;
    uint32_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;

  v1 = F32(c, 0x170);
  result = 0;
  if ( F32(v1, 12) == 0 )
  {
    v7 = F32(v1, 24);
    if ( v7 != 0 )
    {
      F32(v1, 24) = v7 - 1;
      return result;
    }
    if ( F32(v1, 20) != 0 && F32(c, 0x1c) != 8 )
    {
      F32(c, 0x1c) = 7;
      return 7;
    }
    return 8;
  }
  if ( F32(v1, 12) != 1 )
    return result;
  if ( F32(c, 0x1c) == 8 )
    return 8;
  v3 = F16(v1, 4);
  if ( v3 >= F16(F32(F32(v1, 0), 12), 8) )
  {
    v6 = F32(v1, 24);
    if ( v6 != 0 )
      F32(v1, 24) = v6 - 1;
    else
      result = 8 - (F32(v1, 20) != 0);
    F32(c, 0x1c) = result;
  }
  else
  {
    v4 = 0;
    if ( v3 != 0 )
    {
      switch ( F8(**(_DWORD **)v1 + 12 * (uint16_t)(v3 - 1), 9) )
      {
        case 1:
          v5 = F32(v1, 32);
          break;
        case 2:
        case 5:
          v5 = F32(v1, 36);
          break;
        case 3:
          v5 = F32(v1, 40);
          break;
        case 4:
          v5 = 0;
          v4 = 1;
          break;
        default:
          v5 = F32(v1, 16);
          break;
      }
    }
    else
    {
      v5 = 0;
    }
    if ( F16(v1, 6) >= v5 )
    {
      if ( v4 == 0 )
        result = 5;
      F16(v1, 6) = 0;
      ++F16(F32(c, 0x170), 4);
      F32(c, 0x1c) = result;
    }
    else
    {
      ++F16(v1, 6);
      F32(c, 0x1c) = 0;
    }
  }
  return result;
}

/* 0x566570: char *__stdcall newgame_option_label_566570(__int16 a1, char *a2) */
int32_t tm_566570(int16_t a1, char * a2)
{

    char * v2 = 0;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    _Alignas(8) uint8_t frame_[0x100 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [256] at ebp-0x100 */

  switch ( a1 )
  {
    case 3:
      v2 = ((char *)D(0x8a1380));
      return strcpy(a2, v2);
    case 4:
      v2 = ((char *)D(0x8a1374));
      return strcpy(a2, v2);
    case 5:
      v2 = ((char *)D(0x8a1180));
      return strcpy(a2, v2);
    case 6:
      v2 = ((char *)D(0x8a1120));
      return strcpy(a2, v2);
    case 7:
      v2 = ((char *)D(0x8a1144));
      return strcpy(a2, v2);
    case 8:
      v2 = ((char *)D(0x8a11e0));
      return strcpy(a2, v2);
    case 9:
      v2 = ((char *)D(0x8a1204));
      return strcpy(a2, v2);
    case 10:
      v2 = ((char *)D(0x8a1174));
      return strcpy(a2, v2);
    case 11:
      v2 = ((char *)D(0x8a1168));
      return strcpy(a2, v2);
    case 12:
      v2 = ((char *)D(0x8a128c));
      return strcpy(a2, v2);
    case 13:
      v2 = ((char *)D(0x8a127c));
      return strcpy(a2, v2);
    case 14:
      v2 = ((char *)D(0x8a125c));
      return strcpy(a2, v2);
    case 15:
      v2 = ((char *)D(0x8a124c));
      return strcpy(a2, v2);
    case 16:
      v2 = ((char *)D(0x8a1238));
      return strcpy(a2, v2);
    case 17:
      v2 = ((char *)D(0x8a1218));
      return strcpy(a2, v2);
    case 18:
      v2 = ((char *)D(0x8a126c));
      return strcpy(a2, v2);
    case 20:
      v2 = ((char *)D(0x8a1158));
      return strcpy(a2, v2);
    case 21:
      v2 = ((char *)D(0x8a11f8));
      return strcpy(a2, v2);
    case 22:
      v2 = ((char *)D(0x8a112c));
      return strcpy(a2, v2);
    case 46:
      v2 = ((char *)D(0x8a11cc));
      return strcpy(a2, v2);
    case 47:
      v2 = ((char *)D(0x8a11b0));
      return strcpy(a2, v2);
    case 73:
    case 86:
      v3 = 100000101;
      goto LABEL_13;
    case 74:
    case 87:
      v3 = 100000102;
      goto LABEL_13;
    case 75:
    case 88:
      v3 = 100000103;
      goto LABEL_13;
    case 76:
      v2 = ((char *)D(0x8a1340));
      return strcpy(a2, v2);
    case 77:
      v2 = ((char *)D(0x8a132c));
      return strcpy(a2, v2);
    case 78:
      v2 = ((char *)D(0x8a12f0));
      return strcpy(a2, v2);
    case 79:
      v2 = ((char *)D(0x8a12c4));
      return strcpy(a2, v2);
    case 80:
      v2 = ((char *)D(0x8a1318));
      return strcpy(a2, v2);
    case 81:
      v2 = ((char *)D(0x8a1304));
      return strcpy(a2, v2);
    case 82:
      v2 = ((char *)D(0x8a12dc));
      return strcpy(a2, v2);
    case 83:
      v2 = ((char *)D(0x8a12ac));
      return strcpy(a2, v2);
    case 84:
    case 90:
      v2 = ((char *)D(0x8a12a4));
      return strcpy(a2, v2);
    case 85:
      v2 = ((char *)D(0x8a129c));
      return strcpy(a2, v2);
    case 89:
      v3 = 100000104;
LABEL_13:
      v4 = 0;
      break;
    case 91:
      v2 = ((char *)D(0x8a136c));
      return strcpy(a2, v2);
    case 92:
      v2 = ((char *)D(0x8a1360));
      return strcpy(a2, v2);
    default:
      v2 = D(0x8a9b6c);
      return strcpy(a2, v2);
  }
  while ( 1 )
  {
    v5 = ((int32_t *)D(0x6b6ea8))[13 * v4];
    if ( v5 == 0 )
      break;
    if ( v5 == v3 )
    {
      v6 = 52 * v4 + 7040680;
      goto LABEL_19;
    }
    ++v4;
  }
  v6 = 0;
LABEL_19:
  g_eng.h5bf3ee(Buffer, "%s's Level", (const char *)(v6 + 8));
  v2 = Buffer;
  return strcpy(a2, v2);
}

/* 0x566850: char *__stdcall sub_566850(__int16 a1, char *a2) */
int32_t tm_566850(int16_t a1, char * a2)
{

    char * v2 = 0;

  switch ( a1 )
  {
    case 3:
      v2 = ((char *)D(0x8a1b88));
      break;
    case 4:
      v2 = ((char *)D(0x8a1b14));
      break;
    case 5:
      v2 = ((char *)D(0x8a1aac));
      break;
    case 6:
      v2 = ((char *)D(0x8a1a04));
      break;
    case 7:
      v2 = ((char *)D(0x8a1a54));
      break;
    case 8:
      v2 = ((char *)D(0x8a181c));
      break;
    case 9:
      v2 = (const char *)D(0x8a15d0);
      break;
    case 10:
      v2 = ((char *)D(0x8a17d0));
      break;
    case 11:
      v2 = ((char *)D(0x8a1780));
      break;
    case 12:
      v2 = ((char *)D(0x8a19c4));
      break;
    case 13:
      v2 = ((char *)D(0x8a19e8));
      break;
    case 14:
      v2 = ((char *)D(0x8a19a8));
      break;
    case 15:
      v2 = ((char *)D(0x8a1988));
      break;
    case 16:
      v2 = ((char *)D(0x8a1950));
      break;
    case 17:
      v2 = ((char *)D(0x8a18f0));
      break;
    case 18:
      v2 = ((char *)D(0x8a189c));
      break;
    case 20:
      v2 = ((char *)D(0x8a1870));
      break;
    case 21:
      v2 = ((char *)D(0x8a11f8));
      break;
    case 22:
      v2 = ((char *)D(0x8a1644));
      break;
    case 46:
      v2 = ((char *)D(0x8a16f8));
      break;
    case 47:
      v2 = ((char *)D(0x8a1690));
      break;
    case 73:
    case 74:
    case 75:
      v2 = ((char *)D(0x8a1598));
      break;
    case 76:
    case 80:
      v2 = ((char *)D(0x8a1580));
      break;
    case 77:
    case 81:
      v2 = ((char *)D(0x8a1568));
      break;
    case 78:
    case 82:
      v2 = ((char *)D(0x8a1544));
      break;
    case 79:
    case 83:
      v2 = ((char *)D(0x8a14f0));
      break;
    case 84:
    case 90:
      v2 = ((char *)D(0x8a14a4));
      break;
    case 85:
      v2 = ((char *)D(0x8a14c4));
      break;
    case 86:
    case 87:
    case 88:
    case 89:
      v2 = ((char *)D(0x8a1390));
      break;
    case 91:
      v2 = ((char *)D(0x8a1400));
      break;
    case 92:
      v2 = ((char *)D(0x8a1448));
      break;
    default:
      v2 = D(0x8a9b6c);
      break;
  }
  return strcpy(a2, v2);
}

/* 0x566a80: char *__stdcall newgame_option_value_566A80(__int16 a1, int a2, char *Buffer) */
int32_t tm_566a80(int16_t a1, int32_t a2, char * Buffer)
{

    char * v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;

  switch ( a1 )
  {
    case 3:
      switch ( a2 )
      {
        case 10:
          v3 = ((char *)D(0x8a1f74));
          break;
        case 20:
          v3 = ((char *)D(0x8a1f68));
          break;
        case 30:
          v3 = ((char *)D(0x8a1f60));
          break;
        case 40:
          v3 = ((char *)D(0x8a1f54));
          break;
        case 50:
          v3 = ((char *)D(0x8a1f48));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 4:
      if ( a2 == 0 )
        goto LABEL_211;
      if ( a2 != 1 )
        goto LABEL_242;
LABEL_95:
      v3 = ((char *)D(0x8a1f44));
      return strcpy(Buffer, v3);
    case 5:
      if ( a2 != 0 )
      {
        if ( a2 == 1 )
        {
          v3 = ((char *)D(0x8a1f34));
        }
        else
        {
          if ( a2 != 2 )
            goto LABEL_242;
          v3 = ((char *)D(0x8a1f3c));
        }
      }
      else
      {
        v3 = (const char *)D(0x8a1f30);
      }
      return strcpy(Buffer, v3);
    case 6:
      if ( a2 == 0 )
        goto LABEL_208;
      if ( a2 != 1 )
        goto LABEL_242;
LABEL_98:
      v3 = D(0x854d04);
      return strcpy(Buffer, v3);
    case 7:
    case 22:
      if ( a2 == 0 )
        goto LABEL_98;
      if ( a2 != 1 )
        goto LABEL_242;
LABEL_208:
      v3 = ((char *)D(0x854d00));
      return strcpy(Buffer, v3);
    case 8:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1ca4));
          break;
        case 1:
          v3 = ((char *)D(0x8a1c94));
          break;
        case 2:
          goto LABEL_239;
        case 3:
          v3 = ((char *)D(0x8a1c84));
          break;
        case 4:
          v3 = ((char *)D(0x8a1c78));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 9:
      if ( a2 != 0 )
      {
        if ( a2 == 1 )
        {
          v3 = ((char *)D(0x8a1cc0));
        }
        else
        {
          if ( a2 != 2 )
            goto LABEL_242;
          v3 = ((char *)D(0x8a1cd8));
        }
      }
      else
      {
        v3 = ((char *)D(0x8a1cb4));
      }
      return strcpy(Buffer, v3);
    case 10:
      switch ( a2 )
      {
        case 0:
          goto LABEL_219;
        case 10:
          goto LABEL_220;
        case 20:
          goto LABEL_221;
        case 30:
          goto LABEL_222;
        case 40:
          goto LABEL_223;
        case 50:
          goto LABEL_224;
        case 60:
          goto LABEL_225;
        case 70:
          goto LABEL_226;
        case 80:
          goto LABEL_227;
        case 90:
          goto LABEL_228;
        case 100:
          goto LABEL_229;
        default:
          goto LABEL_242;
      }
    case 11:
      switch ( a2 )
      {
        case 0:
LABEL_219:
          v3 = ((char *)D(0x8a1c6c));
          break;
        case 10:
LABEL_220:
          v3 = (const char *)D(0x854420);
          break;
        case 20:
LABEL_221:
          v3 = (const char *)D(0x85441c);
          break;
        case 30:
LABEL_222:
          v3 = (const char *)D(0x854414);
          break;
        case 40:
LABEL_223:
          v3 = (const char *)D(0x854410);
          break;
        case 50:
LABEL_224:
          v3 = (const char *)D(0x85440c);
          break;
        case 60:
LABEL_225:
          v3 = (const char *)D(0x854408);
          break;
        case 70:
LABEL_226:
          v3 = (const char *)D(0x854404);
          break;
        case 80:
LABEL_227:
          v3 = (const char *)D(0x854400);
          break;
        case 90:
LABEL_228:
          v3 = (const char *)D(0x8543fc);
          break;
        case 100:
LABEL_229:
          v3 = ((char *)D(0x8a1c5c));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 12:
    case 13:
    case 14:
    case 15:
    case 18:
      switch ( a2 )
      {
        case 0:
          goto LABEL_211;
        case 1:
          v3 = ((char *)D(0x8a1d58));
          break;
        case 2:
          v3 = ((char *)D(0x8a1d48));
          break;
        case 3:
          goto LABEL_239;
        case 4:
          v3 = ((char *)D(0x8a1d38));
          break;
        case 5:
          v3 = ((char *)D(0x8a1d2c));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 16:
      if ( a2 != 0 )
      {
        if ( a2 != 1 )
          goto LABEL_242;
        v3 = ((char *)D(0x8a1dcc));
      }
      else
      {
        v3 = ((char *)D(0x8a1dbc));
      }
      return strcpy(Buffer, v3);
    case 17:
      if ( a2 == 0 )
        goto LABEL_189;
      if ( a2 != 1 )
        goto LABEL_242;
LABEL_175:
      v3 = ((char *)D(0x8a1ddc));
      return strcpy(Buffer, v3);
    case 20:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1d20));
          break;
        case 1:
          v3 = ((char *)D(0x8a1d10));
          break;
        case 2:
          goto LABEL_239;
        case 3:
          v3 = ((char *)D(0x8a1d00));
          break;
        case 4:
          v3 = ((char *)D(0x8a1cf4));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 21:
      if ( a2 == 0 )
        goto LABEL_95;
      if ( a2 != 1 )
        goto LABEL_242;
LABEL_211:
      v3 = (const char *)D(0x8a1cb0);
      return strcpy(Buffer, v3);
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
      if ( a2 > 114 )
      {
        switch ( a2 )
        {
          case 115:
            v3 = ((char *)D(0x8a2014));
            break;
          case 205:
            v3 = ((char *)D(0x8a2008));
            break;
          case 206:
            v3 = ((char *)D(0x8a1ffc));
            break;
          case 207:
            v3 = ((char *)D(0x8a1ff0));
            break;
          case 208:
            v3 = ((char *)D(0x8a1fe8));
            break;
          case 209:
            v3 = ((char *)D(0x8a1fe0));
            break;
          case 210:
            v3 = ((char *)D(0x8a1fd8));
            break;
          case 300:
            v3 = ((char *)D(0x8a1fd0));
            break;
          case 301:
            v3 = ((char *)D(0x8a1fc4));
            break;
          case 302:
            v3 = ((char *)D(0x8a1fb8));
            break;
          case 303:
            v3 = ((char *)D(0x8a1fac));
            break;
          case 304:
            v3 = ((char *)D(0x8a1fa0));
            break;
          case 305:
            v3 = ((char *)D(0x8a1f94));
            break;
          case 306:
            v3 = ((char *)D(0x8a1f88));
            break;
          case 307:
            v3 = ((char *)D(0x8a1f7c));
            break;
          default:
            goto LABEL_242;
        }
      }
      else if ( a2 == 114 )
      {
        v3 = ((char *)D(0x8a2020));
      }
      else
      {
        switch ( a2 )
        {
          case 0:
            goto LABEL_69;
          case 100:
            v3 = ((char *)D(0x8a20d0));
            break;
          case 101:
            v3 = ((char *)D(0x8a20c4));
            break;
          case 102:
            v3 = ((char *)D(0x8a20b8));
            break;
          case 103:
            v3 = ((char *)D(0x8a20ac));
            break;
          case 104:
            v3 = ((char *)D(0x8a20a0));
            break;
          case 105:
            v3 = ((char *)D(0x8a2094));
            break;
          case 106:
            v3 = ((char *)D(0x8a2088));
            break;
          case 107:
            v3 = ((char *)D(0x8a207c));
            break;
          case 108:
            v3 = ((char *)D(0x8a2070));
            break;
          case 109:
            v3 = ((char *)D(0x8a2064));
            break;
          case 110:
            v3 = ((char *)D(0x8a2058));
            break;
          case 111:
            v3 = ((char *)D(0x8a204c));
            break;
          case 112:
            v3 = ((char *)D(0x8a2040));
            break;
          case 113:
            v3 = ((char *)D(0x8a2034));
            break;
          default:
            goto LABEL_242;
        }
      }
      return strcpy(Buffer, v3);
    case 46:
      if ( a2 != 0 )
      {
        if ( a2 == 1 )
        {
          v3 = ((char *)D(0x8a1c48));
        }
        else
        {
          if ( a2 != 2 )
            goto LABEL_242;
          v3 = ((char *)D(0x8a1c50));
        }
      }
      else
      {
        v3 = ((char *)D(0x8a1c40));
      }
      return strcpy(Buffer, v3);
    case 47:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1c38));
          break;
        case 1:
          v3 = ((char *)D(0x8a1c28));
          break;
        case 2:
LABEL_239:
          v3 = ((char *)D(0x8a1c1c));
          break;
        case 3:
          v3 = ((char *)D(0x8a1c0c));
          break;
        case 4:
          v3 = ((char *)D(0x8a1c00));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
      switch ( a2 )
      {
        case 0:
LABEL_69:
          v3 = D(0x8a202c);
          break;
        case 100:
          v3 = ((char *)D(0x8a2200));
          break;
        case 101:
          v3 = ((char *)D(0x8a21fc));
          break;
        case 102:
          v3 = ((char *)D(0x8a21f8));
          break;
        case 103:
          v3 = ((char *)D(0x8a21f4));
          break;
        case 104:
          v3 = ((char *)D(0x8a21f0));
          break;
        case 105:
          v3 = D(0x8a21ec);
          break;
        case 106:
          v3 = ((char *)D(0x8a21e8));
          break;
        case 107:
          v3 = D(0x8a21e4);
          break;
        case 108:
          v3 = ((char *)D(0x8a21e0));
          break;
        case 109:
          v3 = ((char *)D(0x8a21dc));
          break;
        case 110:
          v3 = ((char *)D(0x8a21d8));
          break;
        case 111:
          v3 = D(0x8a21d4);
          break;
        case 112:
          v3 = ((char *)D(0x8a21d0));
          break;
        case 113:
          v3 = ((char *)D(0x8a21cc));
          break;
        case 114:
          v3 = ((char *)D(0x8a21c8));
          break;
        case 115:
          v3 = ((char *)D(0x8a21c4));
          break;
        case 116:
          v3 = ((char *)D(0x8a21c0));
          break;
        case 117:
          v3 = ((char *)D(0x8a21bc));
          break;
        case 118:
          v3 = ((char *)D(0x8a21b8));
          break;
        case 119:
          v3 = ((char *)D(0x8a21b4));
          break;
        case 120:
          v3 = ((char *)D(0x8a21b0));
          break;
        case 121:
          v3 = ((char *)D(0x8a21ac));
          break;
        case 122:
          v3 = ((char *)D(0x8a21a8));
          break;
        case 123:
          v3 = D(0x8a21a4);
          break;
        case 124:
          v3 = ((char *)D(0x8a21a0));
          break;
        case 125:
          v3 = ((char *)D(0x8a219c));
          break;
        case 126:
          v3 = ((char *)D(0x8a2190));
          break;
        case 127:
          v3 = ((char *)D(0x8a2188));
          break;
        case 128:
          v3 = ((char *)D(0x8a2184));
          break;
        case 129:
          v3 = ((char *)D(0x8a217c));
          break;
        case 130:
          v3 = ((char *)D(0x8a2170));
          break;
        case 131:
          v3 = ((char *)D(0x8a2168));
          break;
        case 132:
          v3 = ((char *)D(0x8a2160));
          break;
        case 133:
          v3 = ((char *)D(0x8a2158));
          break;
        case 134:
          v3 = ((char *)D(0x8a2150));
          break;
        case 135:
          v3 = ((char *)D(0x8a2148));
          break;
        case 136:
          v3 = ((char *)D(0x8a2140));
          break;
        case 137:
          v3 = ((char *)D(0x8a2138));
          break;
        case 138:
          v3 = ((char *)D(0x8a212c));
          break;
        case 139:
          v3 = ((char *)D(0x8a2124));
          break;
        case 140:
          v3 = ((char *)D(0x8a211c));
          break;
        case 141:
          v3 = ((char *)D(0x8a2114));
          break;
        case 142:
          v3 = ((char *)D(0x8a210c));
          break;
        case 143:
          v3 = ((char *)D(0x8a2104));
          break;
        case 144:
          v3 = ((char *)D(0x8a20fc));
          break;
        case 145:
          v3 = ((char *)D(0x8a20f4));
          break;
        case 146:
          v3 = ((char *)D(0x8a20ec));
          break;
        case 147:
          v3 = ((char *)D(0x8a20e4));
          break;
        case 148:
          v3 = ((char *)D(0x8a20dc));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 73:
    case 74:
    case 75:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1f20));
          break;
        case 1:
          v3 = ((char *)D(0x8a1f18));
          break;
        case 2:
          v3 = ((char *)D(0x8a1f10));
          break;
        case 3:
          v3 = ((char *)D(0x8a1f08));
          break;
        case 4:
          v3 = ((char *)D(0x8a1f00));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 76:
    case 77:
    case 80:
    case 81:
      if ( a2 == 0xFFFF )
      {
        v3 = ((char *)D(0x8a1eec));
        return strcpy(Buffer, v3);
      }
      strcpy(Buffer, D(0x873528));
      v4 = 0;
      if ( a2 < 0 )
        return Buffer;
      if ( ((int32_t *)D(0x854080))[0] != 0 )
      {
        do
          ++v4;
        while ( ((int32_t *)D(0x854080))[6 * v4] != 0 );
      }
      if ( a2 >= v4 )
        return Buffer;
      v5 = 1;
      v6 = 0;
      v7 = ((int32_t *)D(0x854084))[6 * a2];
      v8 = 0;
      v9 = 1;
      break;
    case 78:
    case 82:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1ed0));
          break;
        case 1:
          v3 = ((char *)D(0x8a1ec8));
          break;
        case 2:
          v3 = ((char *)D(0x8a1ebc));
          break;
        case 3:
          v3 = ((char *)D(0x8a1eb0));
          break;
        case 4:
          v3 = ((char *)D(0x8a1ea0));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 79:
    case 83:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1e98));
          break;
        case 1:
          v3 = ((char *)D(0x8a1e94));
          break;
        case 2:
          v3 = ((char *)D(0x8a1e90));
          break;
        case 3:
          v3 = ((char *)D(0x8a1e8c));
          break;
        case 4:
          v3 = ((char *)D(0x8a1e88));
          break;
        case 5:
          v3 = ((char *)D(0x8a1e84));
          break;
        case 6:
          v3 = ((char *)D(0x8a1e80));
          break;
        case 7:
          v3 = ((char *)D(0x8a1e7c));
          break;
        case 8:
          v3 = ((char *)D(0x8a1e78));
          break;
        case 9:
          v3 = ((char *)D(0x8a1e74));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 84:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1e64));
          break;
        case 1:
          v3 = ((char *)D(0x8a1e5c));
          break;
        case 2:
          v3 = ((char *)D(0x8a1e50));
          break;
        case 3:
          v3 = ((char *)D(0x8a1e48));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 85:
      if ( a2 != 0 )
      {
        if ( a2 != 1 )
          goto LABEL_242;
        v3 = ((char *)D(0x8a1e3c));
      }
      else
      {
        v3 = ((char *)D(0x8a1e2c));
      }
      return strcpy(Buffer, v3);
    case 86:
    case 87:
    case 88:
    case 89:
      if ( a2 == 0 )
        goto LABEL_175;
      if ( a2 != 1 )
        goto LABEL_242;
LABEL_189:
      v3 = ((char *)D(0x8a1d64));
      return strcpy(Buffer, v3);
    case 90:
      switch ( a2 )
      {
        case 0:
          v3 = ((char *)D(0x8a1e24));
          break;
        case 1:
          v3 = ((char *)D(0x8a1e1c));
          break;
        case 2:
          v3 = ((char *)D(0x8a1e14));
          break;
        case 3:
          v3 = ((char *)D(0x8a1e0c));
          break;
        case 4:
          v3 = ((char *)D(0x8a1e04));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    case 91:
      if ( a2 != 0 )
      {
        if ( a2 == 1 )
          v3 = ((char *)D(0x8a1df0));
        else
LABEL_242:
          v3 = D(0x8a9b6c);
      }
      else
      {
        v3 = ((char *)D(0x8a1de4));
      }
      return strcpy(Buffer, v3);
    case 92:
      switch ( a2 )
      {
        case 1:
          v3 = ((char *)D(0x8a1db4));
          break;
        case 2:
          v3 = ((char *)D(0x8a1da4));
          break;
        case 3:
          v3 = ((char *)D(0x8a1d98));
          break;
        case 4:
          v3 = ((char *)D(0x8a1d90));
          break;
        case 5:
          v3 = ((char *)D(0x8a1d84));
          break;
        case 6:
          v3 = ((char *)D(0x8a1d70));
          break;
        default:
          goto LABEL_242;
      }
      return strcpy(Buffer, v3);
    default:
      goto LABEL_242;
  }
  while ( v9 != ((int32_t *)D(0x854080))[6 * a2] || ((int32_t *)D(0x67ac5c))[v8] != v7 && v7 != 0 )
  {
    v8 = 55 * ++v6;
    v9 = ((int32_t *)D(0x67ac58))[v8];
    if ( v9 == 0 )
    {
      v10 = (int32_t *)D(0x67ac58);
      goto LABEL_123;
    }
  }
  v10 = (int *)(220 * v6 + 6794328);
LABEL_123:
  if ( v10 != 0 && ((int32_t *)D(0x854088))[6 * a2] <= F32(CFG(), 2780) )
  {
    v11 = 0;
    v12 = 0;
    while ( v5 != ((int32_t *)D(0x854080))[6 * a2] || ((int32_t *)D(0x67ac5c))[v12] != 1 )
    {
      v12 = 55 * ++v11;
      v5 = ((int32_t *)D(0x67ac58))[v12];
      if ( v5 == 0 )
      {
        v13 = (int32_t *)D(0x67ac58);
        goto LABEL_132;
      }
    }
    v13 = (int *)(220 * v11 + 6794328);
LABEL_132:
    if ( v13 != 0 )
    {
      if ( v7 > 1 )
      {
        g_eng.h5bf3ee(Buffer, "%s Level %d", (uint8_t *)v13 + 58, v7);
        return Buffer;
      }
      strcpy(Buffer, (uint8_t *)v13 + 58);
    }
  }
  return Buffer;
}

/* 0x567ba0: int __thiscall sub_567BA0(cls_41BBE0 *this, int a2) */
int32_t tm_567ba0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    char * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    char * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    char * v23 = 0;
    intptr_t v24 = 0;
    char * v25 = 0;
    intptr_t i = 0;
    char * v27 = 0;
    int32_t * v28 = 0;
    char * v29 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x66c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v33 = (uint8_t *)(frame_ + 0x1c);   /* [16] at ebp-0x650 */
    char *v43 = (char *)(frame_ + 0x48);   /* [512] at ebp-0x624 */
    char *v44 = (char *)(frame_ + 0x248);   /* [36] at ebp-0x424 */
    char *Buffer = (char *)(frame_ + 0x26c);   /* [256] at ebp-0x400 */
    intptr_t *v46 = (intptr_t *)(frame_ + 0x36c);   /* [64] at ebp-0x300 */
    char *v47 = (char *)(frame_ + 0x46c);   /* [512] at ebp-0x200 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x66c */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x664 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x640 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x63c */
    int16_t *const v36__f = (int16_t *)(frame_ + 0x34);   /* ebp-0x638 */
    int16_t *const v37__f = (int16_t *)(frame_ + 0x36);   /* ebp-0x636 */
    int16_t *const v38__f = (int16_t *)(frame_ + 0x38);   /* ebp-0x634 */
    int16_t *const v39__f = (int16_t *)(frame_ + 0x3a);   /* ebp-0x632 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x630 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x62c */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x628 */

  v2 = a2;
  (*v42__f) = (uint16_t)a2;
  switch ( (int16_t)a2 )
  {
    case 'L':
      (*v41__f) = 0;
      (*v40__f) = 0;
      goto LABEL_6;
    case 'M':
      (*v41__f) = 0;
      (*v40__f) = 1;
      goto LABEL_6;
    case 'P':
      (*v41__f) = 1;
      (*v40__f) = 0;
      goto LABEL_6;
    case 'Q':
      (*v41__f) = 1;
      (*v40__f) = 1;
LABEL_6:
      switch ( (int16_t)a2 )
      {
        case 3:
          v4 = ((char *)D(0x8a1380));
          break;
        case 4:
          v4 = ((char *)D(0x8a1374));
          break;
        case 5:
          v4 = ((char *)D(0x8a1180));
          break;
        case 6:
          v4 = ((char *)D(0x8a1120));
          break;
        case 7:
          v4 = ((char *)D(0x8a1144));
          break;
        case 8:
          v4 = ((char *)D(0x8a11e0));
          break;
        case 9:
          v4 = ((char *)D(0x8a1204));
          break;
        case 10:
          v4 = ((char *)D(0x8a1174));
          break;
        case 11:
          v4 = ((char *)D(0x8a1168));
          break;
        case 12:
          v4 = ((char *)D(0x8a128c));
          break;
        case 13:
          v4 = ((char *)D(0x8a127c));
          break;
        case 14:
          v4 = ((char *)D(0x8a125c));
          break;
        case 15:
          v4 = ((char *)D(0x8a124c));
          break;
        case 16:
          v4 = ((char *)D(0x8a1238));
          break;
        case 17:
          v4 = ((char *)D(0x8a1218));
          break;
        case 18:
          v4 = ((char *)D(0x8a126c));
          break;
        case 20:
          v4 = ((char *)D(0x8a1158));
          break;
        case 21:
          v4 = ((char *)D(0x8a11f8));
          break;
        case 22:
          v4 = ((char *)D(0x8a112c));
          break;
        case 46:
          v4 = ((char *)D(0x8a11cc));
          break;
        case 47:
          v4 = ((char *)D(0x8a11b0));
          break;
        case 73:
        case 86:
          v5 = 100000101;
          goto LABEL_18;
        case 74:
        case 87:
          v5 = 100000102;
          goto LABEL_18;
        case 75:
        case 88:
          v5 = 100000103;
          goto LABEL_18;
        case 76:
          v4 = ((char *)D(0x8a1340));
          break;
        case 77:
          v4 = ((char *)D(0x8a132c));
          break;
        case 78:
          v4 = ((char *)D(0x8a12f0));
          break;
        case 79:
          v4 = ((char *)D(0x8a12c4));
          break;
        case 80:
          v4 = ((char *)D(0x8a1318));
          break;
        case 81:
          v4 = ((char *)D(0x8a1304));
          break;
        case 82:
          v4 = ((char *)D(0x8a12dc));
          break;
        case 83:
          v4 = ((char *)D(0x8a12ac));
          break;
        case 84:
        case 90:
          v4 = ((char *)D(0x8a12a4));
          break;
        case 85:
          v4 = ((char *)D(0x8a129c));
          break;
        case 89:
          v5 = 100000104;
LABEL_18:
          v6 = W_dramatist_row((intptr_t)(v5));
          g_eng.h5bf3ee(Buffer, "%s's Level", (const char *)(v6 + 8));
          v4 = Buffer;
          break;
        case 91:
          v4 = ((char *)D(0x8a136c));
          break;
        case 92:
          v4 = ((char *)D(0x8a1360));
          break;
        default:
          v4 = D(0x8a9b6c);
          break;
      }
      v7 = 0;
      strcpy(v43, v4);
      v8 = 0;
      v9 = F32(c, 0x0);
      v10 = 2;
      if ( v33 != (_BYTE *)-44 )
      {
        v8 = 1;
        v10 = 3;
      }
      W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v44), (intptr_t)(v8));
      v11 = W_win_mgr_open((intptr_t)(v9), (intptr_t)(288), (intptr_t)(16), (intptr_t)(320), (intptr_t)(256), (intptr_t)(v10), (intptr_t)(v44));
      v12 = v11;
      if ( v33 != (_BYTE *)-44 )
        W_win_title_add((intptr_t)(v11), (intptr_t)(v43));
      (*v35__f) = v12;
      v13 = W_win_ctrl_take((intptr_t)(v12));
      v14 = v13;
      if ( v13 != 0 )
      {
        W_ctrl_grid_init((intptr_t)(v13), (intptr_t)(40), (intptr_t)(56), (intptr_t)(256), (intptr_t)(2), (intptr_t)(6), (intptr_t)(2));
        v15 = v14;
      }
      else
      {
        v15 = 0;
      }
      (*v34__f) = v15;
      F32(F32(v15, 0), 0x178) = 0;
      F32(F32(v15, 0x178), 4) = 0;
      v16 = F32(v15, 0x178);
      (*v36__f) = 0;
      F32(v16, 8) = 0;
      v17 = F32(v15, 0x178);
      (*v37__f) = 1;
      F32(v17, 12) = 0;
      v18 = (*v34__f);
      (*v38__f) = 2;
      (*v39__f) = 3;
      W_ctrl_cursor_set((intptr_t)((*v34__f)), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(&(*v36__f)), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
      if ( F32(CFG(), 596) == 0 )
      {
        *(_DWORD *)&GAP(v18, 0x0, 20) = 1;
        *(_DWORD *)&GAP(v18, 0x0, 24) = -9;
      }
      (*v37__f) = 1;
      v19 = &GAP((*v35__f), 0x0, 20);
      (*v32__f) = *(_DWORD *)&GAP((*v35__f), 0x0, 20) - 36;
      (*v31__f) = *(_DWORD *)&GAP((*v35__f), 0x0, 24) / 2 - 16;
      v20 = (*v34__f);
      (*v36__f) = 0;
      (*v38__f) = 2;
      (*v39__f) = 3;
      tm_56daa0((*v34__f), SYS32(2956), 4, 4, (*v31__f), 8, (*v32__f), (*v31__f), &(*v36__f), 4u, 8);
      tm_56da40(v20, (const char *)((char *)D(0x854898)), F32(v19, 0) - 56, 8);
      *(_DWORD *)&GAP((v20 + (3) * 0x84), 0x0, 20) = 0xFFFFFF;
      *(_DWORD *)&GAP((v20 + (3) * 0x84), 0x0, 16) = 4546427;
      if ( ((int32_t *)D(0x854080))[0] != 0 )
      {
        v21 = 0;
        do
        {
          v22 = W_grid_row_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(v21));
          v23 = tm_566a80(a2, v21, v47);
          W_grid_cell_text((intptr_t)((*v34__f)), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(v23));
          if ( RC_56dd80((intptr_t)(v21), (intptr_t)((*v41__f))) == 0 )
            F32(*(_DWORD *)((uint8_t *)((*v34__f) + (2) * 0x84) + 0x74) + 16 * v22, 8) = 0;
          v21 = (uint16_t)++v7;
        }
        while ( ((int32_t *)D(0x854080))[6 * (uint16_t)v7] != 0 );
      }
      v24 = W_grid_row_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(0xFFFF));
      v25 = tm_566a80(a2, 0xFFFF, v47);
      W_grid_cell_text((intptr_t)((*v34__f)), (intptr_t)(v24), (intptr_t)(0), (intptr_t)(v25));
      v2 = a2;
      F32(*(_DWORD *)((uint8_t *)((*v34__f) + (2) * 0x84) + 0x74) + 16 * v24, 8) = (*v40__f);
      goto LABEL_64;
    default:
      SETLO(i, tm_568320(a2, v46));
      if ( (uint16_t)i != 0 )
      {
        v27 = tm_566570(a2, v43);
        W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(288), (intptr_t)(128), (intptr_t)(256), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(i), (intptr_t)(1u), (intptr_t)(&(*v35__f)), (intptr_t)((int *)&(*v34__f)), (intptr_t)(v27), (intptr_t)(0));
        v28 = v46;
        for ( i = (uint16_t)i; i != 0; --i )
        {
          v29 = tm_566a80(a2, *v28, v47);
          W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(*v28++), (intptr_t)(v29));
        }
LABEL_64:
        tm_419900(
          (*v34__f),
          0,
          F32(CFG() + 28 * (*v42__f), 12));
        while ( 1 )
        {
          result = tm_565d10(c, (*v34__f), 0, 0, 0, 0);
          if ( result == 11 )
            break;
          if ( result == 12 )
          {
            tm_5657f0(
              SYS32(800),
              v2,
              F32(16 * F32((*(_DWORD *)&GAP(((*v34__f) + (2) * 0x84), 0x2e, 62) + 20), 0) + *(_DWORD *)((uint8_t *)((*v34__f) + (2) * 0x84) + 0x74), 4));
            *(_DWORD *)((uint8_t *)((*v35__f) + (1) * 0x50) + 0x0) = 0;
            return 12;
          }
          if ( result != 13 )
          {
            *(_DWORD *)((uint8_t *)((*v35__f) + (1) * 0x50) + 0x0) = 0;
            return result;
          }
        }
        *(_DWORD *)((uint8_t *)((*v35__f) + (1) * 0x50) + 0x0) = 0;
        return 0;
      }
      else
      {
        *(_DWORD *)((uint8_t *)((*v35__f) + (1) * 0x50) + 0x0) = 0;
        return 0;
      }
  }
}

/* 0x568320: unsigned __int16 __stdcall sub_568320(__int16 a1, _DWORD *a2) */
int32_t tm_568320(int16_t a1, int32_t * a2)
{

    uint16_t v2 = 0;
    uint16_t result = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;

  v2 = 0;
  switch ( a1 )
  {
    case 3:
      result = 3;
      *a2 = 10;
      a2[1] = 20;
      a2[2] = 30;
      v4 = F32(CFG(), 2724);
      if ( (uint16_t)((v4 != 0) + 4) >= 4u )
      {
        a2[3] = 40;
        result = 4;
      }
      if ( (uint16_t)((v4 != 0) + 4) >= 5u )
      {
        v5 = result;
        a2[v5] = 50;
        ++result;
      }
      break;
    case 4:
    case 6:
    case 7:
    case 16:
    case 21:
    case 22:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
    case 91:
      *a2 = 0;
      a2[1] = 1;
      result = 2;
      break;
    case 5:
      *a2 = 2;
      a2[1] = 1;
      a2[2] = 0;
      result = 3;
      break;
    case 8:
    case 20:
    case 47:
    case 73:
    case 74:
    case 75:
    case 78:
    case 82:
      *a2 = 0;
      a2[1] = 1;
      a2[2] = 2;
      a2[3] = 3;
      a2[4] = 4;
      result = 5;
      break;
    case 9:
      *a2 = 1;
      a2[1] = 2;
      a2[2] = 0;
      result = 3;
      break;
    case 10:
    case 11:
      *a2 = 0;
      a2[1] = 10;
      a2[2] = 20;
      a2[3] = 30;
      a2[4] = 40;
      a2[5] = 50;
      a2[6] = 60;
      a2[7] = 70;
      a2[8] = 80;
      a2[9] = 90;
      a2[10] = 100;
      result = 11;
      break;
    case 12:
      goto LABEL_26;
    case 13:
    case 14:
    case 15:
    case 18:
      *a2 = 0;
      v2 = 1;
LABEL_26:
      v11 = v2;
      v12 = v2 + 1;
      a2[v11] = 1;
      v13 = v12++;
      a2[v13] = 2;
      v14 = v12++;
      a2[v14] = 3;
      v15 = v12++;
      a2[v15] = 4;
      v16 = v12;
      a2[v16] = 5;
      result = v12 + 1;
      break;
    case 17:
      *a2 = 1;
      a2[1] = 0;
      result = 2;
      break;
    case 46:
      *a2 = 0;
      a2[1] = 1;
      a2[2] = 2;
      result = 3;
      break;
    case 79:
    case 83:
      *a2 = 0;
      a2[1] = 1;
      a2[2] = 2;
      a2[3] = 3;
      a2[4] = 4;
      a2[5] = 5;
      a2[6] = 6;
      a2[7] = 7;
      a2[8] = 8;
      a2[9] = 9;
      result = 10;
      break;
    case 84:
      result = 1;
      *a2 = 0;
      if ( F32(CFG(), 2780) >= 400 )
      {
        a2[1] = 1;
        result = 2;
      }
      if ( F32(CFG(), 2780) >= 500 )
      {
        v6 = result++;
        a2[v6] = 2;
      }
      if ( F32(CFG(), 2780) >= 700 )
      {
        v7 = result;
        a2[v7] = 3;
        ++result;
      }
      break;
    case 90:
      result = 1;
      *a2 = 0;
      if ( F32(CFG(), 2780) >= 300 )
      {
        a2[1] = 1;
        result = 2;
      }
      if ( F32(CFG(), 2780) >= 500 )
      {
        v8 = result++;
        a2[v8] = 2;
      }
      if ( F32(CFG(), 2780) >= 700 )
      {
        v9 = result++;
        a2[v9] = 3;
      }
      if ( F32(CFG(), 2780) >= 800 )
      {
        v10 = result;
        a2[v10] = 4;
        ++result;
      }
      break;
    case 92:
      *a2 = 1;
      a2[1] = 2;
      a2[2] = 3;
      a2[3] = 4;
      a2[4] = 5;
      a2[5] = 6;
      result = 6;
      break;
    default:
      result = 0;
      break;
  }
  return result;
}

/* 0x56da40: int __thiscall sub_56DA40(cls_56DA40 *this, const char *a2, int a3, int a4) */
int32_t tm_56da40(wl_char *self, char * a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x140) = 1;
  strcpy(((uint8_t *)(c) + 0x144), a2);
  F32(c, 0x15c) = a4;
  F32(c, 0x158) = a3;
  F16(c, 0x160) = 0;
  F16(c, 0x162) = 0;
  return 0;
}

/* 0x56daa0: int __thiscall sub_56DAA0( cls_56DAA0 *this, int a2, __int16 a3, int a4, int a5, __int16 a6, int a7, int a8, __int16 *a9, unsigned __int16 a10, __int16 a11) */
int32_t tm_56daa0(wl_char *self, int32_t a2, int16_t a3, int32_t a4, int32_t a5, int16_t a6, int32_t a7, int32_t a8, int16_t * a9, uint16_t a10, int16_t a11)
{
    intptr_t c = (intptr_t)self;
    int16_t * v11 = 0;
    char * v12 = 0;
    intptr_t i = 0;
    int16_t v14 = 0;
    int16_t * v15 = 0;
    char * v16 = 0;
    intptr_t j = 0;
    int16_t v18 = 0;

  F32(c, 0x84) = 1;
  F32(c, 0x88) = a2;
  F16(c, 0x8c) = a3;
  F16(c, 0xce) = a10;
  if ( a9 != 0 && a10 != 0 )
  {
    v11 = a9;
    v12 = ((uint8_t *)(c) + 0x8e);
    for ( i = a10; i != 0; --i )
    {
      v14 = *v11++;
      F16(v12, 0) = v14;
      v12 += 2;
    }
  }
  F16(c, 0xd0) = a11;
  F16(c, 0xd4) = 0;
  F16(c, 0xd2) = 0;
  F32(c, 0xd8) = 0;
  F32(c, 0xdc) = a4;
  F32(c, 0xe0) = a5;
  F32(c, 0xe4) = a2;
  F16(c, 0xe8) = a6;
  F16(c, 0x12a) = a10;
  if ( a9 != 0 && a10 != 0 )
  {
    v15 = a9;
    v16 = ((uint8_t *)(c) + 0xea);
    for ( j = a10; j != 0; --j )
    {
      v18 = *v15++;
      F16(v16, 0) = v18;
      v16 += 2;
    }
  }
  F16(c, 0x130) = 0;
  F16(c, 0x12e) = 0;
  F32(c, 0x134) = 0;
  F16(c, 0x12c) = a11;
  F32(c, 0x138) = a7;
  F32(c, 0x13c) = a8;
  return a7;
}

/* 0x568b40: int __thiscall sub_568B40(cls_41BBE0 *this, _DWORD *a2) */
int32_t tm_568b40(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t result = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v16 = (uint8_t *)(frame_ + 0x4);   /* [36] at ebp-0x24 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x28 */

  if ( a2 == (_DWORD *)28 )
  {
    v11 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(130), (intptr_t)(200), (intptr_t)(380), (intptr_t)(64), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(0));
    v12 = W_win_grid_add((intptr_t)(v11), (intptr_t)(32), (intptr_t)(24), (intptr_t)(0), (intptr_t)(0));
    W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v12), (intptr_t)(0));
    W_grid_cell_text((intptr_t)(v12), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2204))));
    W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(350), (intptr_t)(248), (intptr_t)(128), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(1u), (intptr_t)(&(*v15__f)), (intptr_t)((int *)&a2), (intptr_t)(0), (intptr_t)(0));
    v13 = a2;
    v14 = W_grid_row_add((intptr_t)(a2), (intptr_t)(0), (intptr_t)(14));
    W_grid_cell_text((intptr_t)(v13), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(D(0x854d04)));
    W_grid_row_text_add((intptr_t)(a2), (intptr_t)(0), (intptr_t)(15), (intptr_t)(((char *)D(0x854d00))));
    while ( 1 )
    {
      result = tm_565d10(c, a2, 0, 0, 0, 0);
      if ( result == 11 )
        break;
      if ( result == 12 )
      {
        result = F32(16 * F32((a2[93] + 20), 0) + a2[95], 4);
        goto LABEL_20;
      }
      if ( result != 13 )
        goto LABEL_20;
    }
    result = 15;
LABEL_20:
    F32(v11, 80) = 0;
    v10 = (*v15__f);
  }
  else
  {
    if ( a2 != (_DWORD *)29 )
      return 0;
    v3 = F32(c, 0x0);
    W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v16), (intptr_t)(0));
    v4 = W_win_mgr_open((intptr_t)(v3), (intptr_t)(130), (intptr_t)(200), (intptr_t)(380), (intptr_t)(64), (intptr_t)(1u), (intptr_t)(v16));
    v5 = W_win_ctrl_take((intptr_t)(v4));
    v6 = v5;
    if ( v5 != 0 )
    {
      W_ctrl_grid_init((intptr_t)(v5), (intptr_t)(32), (intptr_t)(24), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
      W_grid_col_place((intptr_t)(v6), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      W_grid_row_add((intptr_t)(v6), (intptr_t)(0), (intptr_t)(0));
    }
    else
    {
      v6 = 0;
    }
    W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v6), (intptr_t)(0));
    W_grid_cell_text((intptr_t)(v6), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a222c))));
    W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(350), (intptr_t)(248), (intptr_t)(128), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(1u), (intptr_t)(&(*v15__f)), (intptr_t)((int *)&a2), (intptr_t)(0), (intptr_t)(0));
    v7 = a2;
    v8 = W_grid_row_add((intptr_t)(a2), (intptr_t)(0), (intptr_t)(14));
    W_grid_cell_text((intptr_t)(v7), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(D(0x854d04)));
    W_grid_row_text_add((intptr_t)(a2), (intptr_t)(0), (intptr_t)(15), (intptr_t)(((char *)D(0x854d00))));
    while ( 1 )
    {
      result = tm_565d10(c, a2, 0, 0, 0, 0);
      if ( result == 11 )
        break;
      if ( result == 12 )
      {
        result = F32(16 * F32((a2[93] + 20), 0) + a2[95], 4);
        F32(v4, 80) = 0;
        v10 = (*v15__f);
        goto LABEL_21;
      }
      if ( result != 13 )
      {
        F32(v4, 80) = 0;
        v10 = (*v15__f);
        goto LABEL_21;
      }
    }
    F32(v4, 80) = 0;
    v10 = (*v15__f);
    result = 15;
  }
LABEL_21:
  *(_DWORD *)((uint8_t *)(v10 + (1) * 0x50) + 0x0) = 0;
  if ( result == 14 )
    return 0;
  if ( result == 15 )
    return 11;
  return result;
}

/* 0x56ded0: void __thiscall sub_56DED0(int this) */
void tm_56ded0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;

  v2 = F32(c, 10372);
  if ( v2 != 0 )
  {
    v3 = F32(F32(c, 800), 0);
    if ( F32(v3, 372) != 0 )
    {
      RC_5bbe20((intptr_t)(v2), (intptr_t)(F32(v3, 388)));
    }
    else
    {
      RC_5bcb30((intptr_t)(v2));
      F16(c, 10376) = 0;
    }
  }
}

/* 0x56dfe0: int __thiscall sub_56DFE0(int this) */
int32_t tm_56dfe0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t result = 0;
    intptr_t i = 0;
    double v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    void ** v11 = 0;
    uint32_t v12 = 0;
    void * v13 = 0;
    int v14 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    int32_t * *const v16__f = (int32_t * *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  v2 = F32(c, 10672);
  v3 = F32(F32(c, 800), 0);
  v4 = F32(v3, 304);
  result = F32(v3, 332);
  if ( v2 != v4 || F32(c, 10676) != result )
  {
    F32(c, 10672) = v4;
    F32(c, 10676) = result;
    for ( i = 0; i < 256; ++i )
    {
      v7 = ((1.0 - cos((double)(F32(c, 10676) * i / 1000) * (3.14159265358979 * 0.00392156862745098)))
          * 127.5
          * (double)F32(c, 10672)
          + (double)(F32(c, 10676) * i / 1000 * (1000 - F32(c, 10672))))
         * 0.001;
      if ( v7 > 255.0 )
        v7 = 255.0;
      F8(c + i, 10416) = (int64_t)v7;
    }
    v8 = (_DWORD *)(c + 2732);
    (*v17__f) = 909;
    (*v16__f) = v8;
    do
    {
      v9 = *v8;
      if ( *v8 != 0 && F32(v8[909], 4) != 0 )
      {
        v10 = 0;
        (*v15__f) = 0;
        if ( F16(v9, 4) != 0 )
        {
          do
          {
            if ( F32(v9, 0) != 0 )
            {
              v11 = (void **)(F32(v9, 0) + 8 * (uint16_t)v10);
              if ( *v11 != 0 )
              {
                v12 = 0;
                if ( F32(v9, 56) != 0 )
                {
                  do
                  {
                    v13 = *((void **)*v11 + v12);
                    if ( v13 != 0 )
                    {
                      W_rp_5b9390((intptr_t)(*((intptr_t *)*v11 + v12)));
                      g_eng.op_delete((void *)v13);
                    }
                    *((_DWORD *)*v11 + v12++) = 0;
                  }
                  while ( v12 < F32(v9, 56) );
                  v10 = (*v15__f);
                }
                g_eng.op_delete((void *)*v11);
                *v11 = 0;
              }
            }
            (*v15__f) = ++v10;
          }
          while ( (uint16_t)v10 < F16(v9, 4) );
          v8 = (*v16__f);
        }
      }
      ++v8;
      result = (*v17__f) - 1;
      v14 = (*v17__f) == 1;
      (*v16__f) = v8;
      --(*v17__f);
    }
    while ( !v14 );
  }
  return result;
}

/* 0x56c930: int __thiscall menu_owner_transition_step_56C930(cls_56C930 *this) */
int32_t tm_56c930(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    uint16_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    int v13 = 0;
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
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v1 = c;
  v2 = 0;
  (*v30__f) = 0;
  if ( F16(c, 0x6) != 0 )
  {
    while ( 1 )
    {
      v3 = F32((F32(((uint8_t *)(v1) + 0x0), 0) + 4 * v2), 0);
      if ( F32(v3, 4) != 0 )
      {
        v4 = F32(v3, 28);
        if ( v4 == 0 )
        {
          if ( F32(v3, 80) != 0 )
          {
            F32(v3, 84) = 1000;
          }
          else
          {
            v24 = F32(v3, 0);
            SETHI(v26, 0);
            F32(v3, 84) = 0;
            v25 = 0;
            SETLO(v26, F16(v24, 0x6));
            if ( (uint16_t)v26 != 0 )
            {
              v27 = (_DWORD *)F32(v24, 0x0);
              while ( *v27 != v3 )
              {
                ++v25;
                ++v27;
                if ( v25 >= v26 )
                  goto LABEL_58;
              }
              if ( v25 >= 0 )
              {
                W_mgr_win_to_back((intptr_t)(v24), (intptr_t)(v25));
                --F16(v24, 0x6);
                W_win_mgr_topmost((intptr_t)(v24));
              }
            }
LABEL_58:
            F32(v3, 4) = 0;
            W_win_ctrls_free((intptr_t)(v3));
          }
          goto LABEL_59;
        }
        v5 = v4 - 1;
        if ( v5 == 0 )
        {
          v22 = F32(v3, 84);
          if ( F32(v3, 80) != 0 )
          {
            if ( v22 < 1000 )
            {
              v23 = v22 + 50;
              if ( v23 > 1000 )
                v23 = 1000;
              F32(v3, 84) = v23;
            }
            goto LABEL_59;
          }
          if ( v22 > 0 )
          {
            F32(v3, 84) = v22 - 40 < 0 ? 0 : v22 - 40;
            goto LABEL_59;
          }
          goto LABEL_41;
        }
        if ( v5 == 1 )
        {
          if ( F32(v3, 80) != 0 )
          {
            v6 = F32(v3, 56);
            v7 = F32(v3, 12);
            v8 = F32(v3, 60);
            v9 = F32(v3, 32);
            if ( v7 == v6 && F32(v3, 16) == v8 )
            {
              v1 = c;
              F32(v3, 84) = 1000;
            }
            else
            {
              v10 = v9 <= v6;
              if ( v9 < v6 )
              {
                v11 = 32 * F32(v3, 64) / 1000 + v7;
                if ( v6 < v11 )
                  v11 = F32(v3, 56);
                F32(v3, 12) = v11;
                v10 = v9 <= v6;
              }
              if ( !v10 )
              {
                v12 = F32(v3, 12) - 32 * F32(v3, 64) / 1000;
                if ( v6 > v12 )
                  v12 = v6;
                F32(v3, 12) = v12;
              }
              v13 = F32(v3, 36) <= v8;
              if ( F32(v3, 36) < v8 )
              {
                v14 = 32 * F32(v3, 68) / 1000 + F32(v3, 16);
                if ( v8 < v14 )
                  v14 = v8;
                F32(v3, 16) = v14;
                v13 = F32(v3, 36) <= v8;
              }
              if ( !v13 )
              {
                v15 = F32(v3, 16) - 32 * F32(v3, 68) / 1000;
                if ( v8 > v15 )
                  v15 = v8;
                F32(v3, 16) = v15;
              }
              v16 = F32(v3, 64) + 50;
              v17 = F32(v3, 52);
              v18 = v17;
              if ( v17 >= v16 )
                v18 = F32(v3, 64) + 50;
              v19 = F32(v3, 48);
              if ( v19 <= v18 )
              {
                if ( v17 < v16 )
                  v16 = F32(v3, 52);
              }
              else
              {
                v16 = F32(v3, 48);
              }
              F32(v3, 64) = v16;
              v20 = F32(v3, 68) + 50;
              v21 = v17;
              if ( v17 >= v20 )
                v21 = F32(v3, 68) + 50;
              if ( v19 <= v21 )
              {
                if ( v17 >= v20 )
                  v17 = F32(v3, 68) + 50;
                v1 = c;
                F32(v3, 68) = v17;
              }
              else
              {
                v1 = c;
                F32(v3, 68) = v19;
              }
            }
            goto LABEL_59;
          }
          if ( W_win_slide((intptr_t)(v3), (intptr_t)(F32(v3, 56)), (intptr_t)(F32(v3, 60)), (intptr_t)(F32(v3, 40)), (intptr_t)(F32(v3, 44))) != 0 )
          {
            F32(v3, 84) = 0;
LABEL_41:
            tm_56cc10(v3);
          }
        }
      }
LABEL_59:
      v2 = ++(*v30__f);
      if ( (uint16_t)(*v30__f) >= F16(v1, 0x6) )
        return 0;
    }
  }
  return 0;
}

/* 0x56cc10: void __thiscall sub_56CC10(cls_56CC10 *this) */
void tm_56cc10(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t i = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    void * v14 = 0;

  v2 = 0;
  v3 = F32(c, 0x0);
  v4 = F16(F32(c, 0x0), 6);
  if ( (uint16_t)v4 != 0 )
  {
    v5 = F32(v3, 0);
    while ( F32(v5, 0) != c )
    {
      ++v2;
      ++v5;
      if ( v2 >= v4 )
        goto LABEL_14;
    }
    if ( v2 >= 0 )
    {
      v6 = (uint16_t)v2;
      v7 = F32((F32(v3, 0) + 4 * (uint16_t)v2), 0);
      if ( (uint16_t)v2 < v4 - 1 )
      {
        do
        {
          ++v2;
          F32((F32(v3, 0) + 4 * v6), 0) = F32(F32(v3, 0) + 4 * v6, 4);
          v6 = (uint16_t)v2;
        }
        while ( (uint16_t)v2 < F16(v3, 6) - 1 );
      }
      F32((F32(v3, 0) + 4 * (uint16_t)(F16(v3, 6))-- - 4), 0) = v7;
      for ( i = 0; i < F16(v3, 6); ++i )
      {
        v9 = F32((F32(v3, 0) + 4 * i), 0);
        F32(v9, 8) = i == F16(v3, 6) - 1;
      }
    }
  }
LABEL_14:
  v10 = F32(c, 0x48);
  v11 = 0;
  F32(c, 0x4) = 0;
  if ( v10 != 0 )
  {
    if ( F16(((uint8_t *)(c) + 0x4c), 0) != 0 )
    {
      do
      {
        v12 = F32(c, 0x48);
        v13 = 4 * v11;
        v14 = F32(v13, v12);
        if ( v14 != 0 )
        {
          W_ctrl_geometry_free((intptr_t)(F32(v13, v12)));
          g_eng.op_delete((void *)v14);
          F32((v13 + F32(c, 0x48)), 0) = 0;
        }
        ++v11;
      }
      while ( v11 < F16(((uint8_t *)(c) + 0x4c), 0) );
    }
    if ( F32(c, 0x48) != 0 )
    {
      g_eng.op_delete((void *)(void *)F32(c, 0x48));
      F32(c, 0x48) = 0;
    }
  }
}

/* 0x56dc20: void __stdcall sub_56DC20(int a1, int a2, int a3) */
void tm_56dc20(int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t v3 = 0;
    intptr_t v4 = 0;
    _Alignas(8) uint8_t frame_[0x104 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *String = (char *)(frame_ + 0x4);   /* [256] at ebp-0x100 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x104 */

  RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  if ( a1 != 0 )
  {
    v3 = RC_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
    W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v3));
    if ( a2 < a3 )
    {
      v4 = 1;
      if ( a3 >= 1 )
        v4 = a3;
      g_eng.h5bf3ee(String, "Now Loading % 3d%%", 100 * a2 / v4);
    }
    else
    {
      strcpy(String, ((char *)D(0x8a27f8)));
    }
    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(String)) >> 1)), (intptr_t)(456), (intptr_t)(String), (intptr_t)(strlen(String)));
    W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
    API_DeleteObject((intptr_t)(v3));
  }
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
}

/* 0x568de0: int __thiscall sub_568DE0(cls_41BBE0 *this, int a2) */
int32_t tm_568de0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
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
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    uint16_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    char * v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    _Alignas(8) uint8_t frame_[0x244 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v32 = (char *)(frame_ + 0x20);   /* [36] at ebp-0x224 */
    char *v33 = (char *)(frame_ + 0x44);   /* [512] at ebp-0x200 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x244 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x22c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x228 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x228 */

  (*v29__f) = 0;
  tm_5644a0(c);
  v3 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(32), (intptr_t)(392), (intptr_t)(576), (intptr_t)(80), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x8) = (int)v3;
  v4 = W_win_text_add((intptr_t)(v3), (intptr_t)(40), (intptr_t)(24), (intptr_t)(68), (intptr_t)(2));
  (*v28__f) = F32(c, 0x8);
  F32(c, 0xc) = (int)v4;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v28__f)), (intptr_t)(v4));
  W_text_metrics_set((intptr_t)(F32(c, 0xc)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v5 = 0;
  v6 = 2;
  (*v30__f) = F32(c, 0x0);
  v7 = 24;
  v8 = 236;
  if ( ((char *)D(0x8a23dc)) != 0 )
  {
    v8 = 268;
    v7 = 56;
    v5 = 1;
    v6 = 3;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v32), (intptr_t)(v5));
  v9 = W_win_mgr_open((intptr_t)((*v30__f)), (intptr_t)(32), (intptr_t)(8), (intptr_t)(320), (intptr_t)(v8), (intptr_t)(v6), (intptr_t)(v32));
  v10 = v9;
  (*v31__f) = v9;
  if ( ((char *)D(0x8a23dc)) != 0 )
    W_win_title_add((intptr_t)(v9), (intptr_t)(((char *)D(0x8a23dc))));
  v11 = W_win_grid_new((intptr_t)(v10), (intptr_t)(40), (intptr_t)(v7), (intptr_t)(7), (intptr_t)(1), (intptr_t)(7), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v11), (intptr_t)(1));
  v12 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(31));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(((char *)D(0x8a23c4))));
  v13 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(32));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x8a23b0))));
  v14 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(33));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(((char *)D(0x8a23a0))));
  v15 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(35));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2390))));
  v16 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(34));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2380))));
  v17 = 0;
  while ( SYS32(4 * v17 + 10364) == 0 )
  {
    if ( ++v17 >= 2u )
    {
      v18 = 0;
      goto LABEL_9;
    }
  }
  v18 = 1;
LABEL_9:
  F32(16 * v16 + F32((v11 + 380), 0), 8) = v18;
  v19 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(38));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2368))));
  v20 = 0;
  while ( SYS32(4 * v20 + 10364) == 0 )
  {
    if ( ++v20 >= 2u )
    {
      v21 = 0;
      goto LABEL_15;
    }
  }
  v21 = 1;
LABEL_15:
  F32(16 * v19 + F32((v11 + 380), 0), 8) = v21;
  v22 = W_grid_row_add((intptr_t)(v11), (intptr_t)(0), (intptr_t)(27));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2360))));
  while ( 1 )
  {
    strcpy(v33, D(0x8a9b6c));
    switch ( F32(16 * F32((F32(v11, 372) + 20), 0) + F32((v11 + 380), 0), 4) )
    {
      case 0x1B:
        v23 = ((char *)D(0x8a2254));
        goto LABEL_24;
      case 0x1F:
        v23 = ((char *)D(0x8a232c));
        goto LABEL_24;
      case 0x20:
        v23 = ((char *)D(0x8a22dc));
        goto LABEL_24;
      case 0x21:
        v23 = ((char *)D(0x8a22b4));
        goto LABEL_24;
      case 0x22:
        v23 = ((char *)D(0x8a2294));
        goto LABEL_24;
      case 0x23:
        v23 = ((char *)D(0x8a2278));
        goto LABEL_24;
      case 0x26:
        v23 = ((char *)D(0x8a22f8));
LABEL_24:
        strcpy(v33, v23);
        break;
      default:
        break;
    }
    W_ctrl_text_set((intptr_t)(F32(c, 0xc)), (intptr_t)(v33));
    v24 = tm_565d10(c, (_DWORD *)v11, 0, 0, 0, 0);
    if ( v24 == 11 )
      break;
    if ( v24 == 12 )
    {
      v25 = F32(16 * F32((F32(v11, 372) + 20), 0) + F32((v11 + 380), 0), 4);
      switch ( v25 )
      {
        case 27:
          goto LABEL_34;
        case 31:
        case 32:
        case 33:
        case 38:
          v24 = tm_564780(c, v25);
          if ( v24 == 0 )
            continue;
          goto LABEL_33;
        case 34:
        case 35:
          v24 = RC_569220((intptr_t)(c), (intptr_t)(v25));
          if ( v24 != 0 )
            goto LABEL_33;
          continue;
        default:
          continue;
      }
    }
    if ( v24 != 13 )
    {
LABEL_33:
      (*v29__f) = v24;
      break;
    }
  }
LABEL_34:
  F32((*v31__f) + (10) * 0x8, 0x0) = 0;
  v26 = F32(c, 0x8);
  if ( v26 != 0 )
  {
    F32(v26, 80) = 0;
    F32(c, 0x8) = 0;
    F32(c, 0xc) = 0;
  }
  if ( (*v29__f) != 6 )
    tm_565d10(c, 0, 0, 0, 0, 0x14u);
  RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
  tm_41bbe0(c);
  return (*v29__f);
}

/* 0x56a220: _DWORD **__thiscall sub_56A220(cls_56A220 *this) */
int32_t tm_56a220(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    char * v1 = 0;
    intptr_t i = 0;
    int32_t ** result = 0;
    intptr_t j = 0;
    int32_t * v5 = 0;

  F16(c, 0x16c) = 0;
  v1 = ((uint8_t *)(c) + 0x114);
  for ( i = 11; i != 0; --i )
  {
    F32(v1, 4 * 11) = 0;
    F32(v1, 0) = 0;
    v1 += 4;
  }
  result = (_DWORD **)&GAP(c, 0x0, 12);
  for ( j = 64; j != 0; --j )
  {
    v5 = *result++;
    *v5 = 0;
  }
  F32(c, 0x10c) = 0;
  F32(c, 0x110) = 0;
  return result;
}

/* 0x41c150: int __stdcall sub_41C150(_DWORD *a1, _DWORD *a2, int a3) */
int32_t tm_41c150(int32_t * a1, int32_t * a2, int32_t a3)
{

    intptr_t result = 0;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    int32_t * v6 = 0;
    intptr_t v7 = 0;
    int16_t v8 = 0;
    int16_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;
    int32_t * v12 = 0;
    int v13 = 0;
    intptr_t i = 0;
    int32_t * v16 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  result = a3;
  if ( (uint16_t)a3 != 0 )
  {
    v4 = a1;
    v5 = (uint16_t)a3;
    v16 = a1;
    (*v14__f) = v5;
    do
    {
      if ( *v4 != *a2 )
      {
        v6 = v4 + 7;
        for ( i = 20; i != 0; --i )
        {
          if ( v6[1] == *a2 )
          {
            v7 = *v4;
            v8 = F16(v6, 0);
            v9 = F16(v6, 2 * 4);
            v10 = 0;
            while ( LOWORD(a2[3 * v10 + 7]) != v9 )
            {
              if ( ++v10 >= 0x14u )
              {
                v11 = 0;
                while ( LOWORD(a2[3 * v11 + 7]) != 0 )
                {
                  if ( ++v11 >= 0x14u )
                    goto LABEL_14;
                }
                v12 = &a2[3 * v11];
                F16(v12, 2 * 14) = v9;
                v12[8] = v7;
                SETLO(a2[3 * v11 + 9], v8);
                F16(v12, 2 * 19) = 0;
                break;
              }
            }
          }
LABEL_14:
          v4 = v16;
          v6 += 3;
        }
      }
      v4 += 84;
      result = (*v14__f) - 1;
      v13 = (*v14__f) == 1;
      v16 = v4;
      --(*v14__f);
    }
    while ( !v13 );
  }
  return result;
}

/* 0x41c240: int *__thiscall sub_41C240(cls_41C240 *this, int a2) */
int32_t tm_41c240(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    uint16_t v3 = 0;
    intptr_t v4 = 0;

  v2 = F32(c, 0x0);
  v3 = 0;
  v4 = *(_DWORD *)F32(c, 0x0);
  if ( v4 == 0 )
    return 0;
  while ( v4 != a2 )
  {
    v4 = F32(((++v3 << 6) + v2), 0);
    if ( v4 == 0 )
      return 0;
  }
  return (int *)(v2 + (v3 << 6));
}

/* 0x560a40: __int16 *__thiscall sub_560A40(cls_560A40 *this, int a2, int a3, int a4) */
int32_t tm_560a40(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v8 = 0;
    uint16_t i = 0;
    intptr_t v10 = 0;
    char * v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    intptr_t j = 0;
    intptr_t v18 = 0;
    uint16_t k = 0;
    intptr_t v20 = 0;
    int16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    uint16_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    char * v27 = 0;
    uint16_t * v28 = 0;
    int16_t * result = 0;
    intptr_t m = 0;
    int16_t v31 = 0;
    uint16_t v32 = 0;
    int32_t * v33 = 0;
    intptr_t v34 = 0;
    int v35 = 0;
    intptr_t v38 = 0;
    int16_t v39 = 0;
    intptr_t v40 = 0;
    int32_t * v41 = 0;
    intptr_t v42 = 0;
    int32_t * v43 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const n__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */

  v5 = F32(a3, 4);
  v6 = F32(a3, 84);
  v8 = F32(a3, 12);
  F32(c, 0x9c4) = F16(a3, 8);
  F32(c, 0x9ec) = a2;
  F32(c, 0x9f8) = v8;
  F32(c, 0x0) = 0;
  F16(((uint8_t *)(c) + 0x9c8), 0) = 0;
  F32(c, 0x8) = 0;
  F32(c, 0x9f4) = 0;
  F32(c, 0x4) = v6;
  F32(c, 0x9f0) = v5;
  F32(c, 0x9fc) = 255;
  F32(c, 0xa00) = 255;
  F32(c, 0xa04) = 7;
  F32(c, 0xa08) = 1000;
  for ( i = 0; ; ++i )
  {
    v10 = ((int32_t *)D(0x6b6ea8))[13 * i];
    if ( v10 == 0 )
      break;
    if ( v10 == v5 )
    {
      if ( 52 * i != -7040680 )
      {
        v38 = ((int32_t *)D(0x6b6eac))[13 * i];
        goto LABEL_8;
      }
      break;
    }
  }
  v38 = v6;
LABEL_8:
  v11 = &GAP(c, 0xa0c, 4);
  W_char_stats_init((intptr_t)((int)&GAP(c, 0xa0c, 4)), (intptr_t)(v38), (intptr_t)(0));
  (*v36__f) = 0;
  v12 = (int32_t *)D(0x6907c0);
  v13 = 0;
  v41 = (int32_t *)D(0x6907c0);
  while ( 1 )
  {
    if ( *v12 != v38 )
      goto LABEL_23;
    v14 = 0;
    if ( *(_WORD *)&GAP(c, 0xb00, 84) != 0 )
    {
      while ( F32(&v11[12 * v14 + 328], 0) != v12[1] )
      {
        if ( ++v14 >= *(_WORD *)&GAP(c, 0xb00, 84) )
        {
          v12 = v41;
          goto LABEL_15;
        }
      }
    }
    else
    {
LABEL_15:
      v15 = v12[3];
      if ( v15 != 0 )
      {
        v16 = 0;
        if ( *(_WORD *)&GAP(c, 0xb00, 84) == 0 )
          goto LABEL_23;
        while ( F32(&v11[12 * v16 + 328], 0) != v15 )
        {
          if ( ++v16 >= *(_WORD *)&GAP(c, 0xb00, 84) )
            goto LABEL_23;
        }
        v12 = v41;
      }
      if ( v12[2] < 0 )
        W_sc_406c70((intptr_t)((int)&GAP(c, 0xa0c, 4)), (intptr_t)(((int32_t *)D(0x6907c4))[v13]), (intptr_t)(255), (intptr_t)(0), (intptr_t)(1));
    }
LABEL_23:
    v13 = 4 * (uint16_t)++(*v36__f);
    v41 = &((int32_t *)D(0x6907c0))[v13];
    if ( ((int32_t *)D(0x6907c0))[v13] == 0 )
      break;
    v12 = &((int32_t *)D(0x6907c0))[v13];
  }
  F32(c, 0x9fc) = F32(a3, 16);
  F32(c, 0xa00) = F32(a3, 20);
  F32(c, 0xaf0) = F32(a3, 60);
  F32(c, 0xae8) = F32(a3, 64);
  F32(c, 0xaec) = F32(a3, 88);
  F32(c, 0xa68) = F32(a3, 32);
  F32(c, 0xaac) = F32(a3, 96);
  F32(c, 0xa64) = F32(a3, 28);
  F32(c, 0xab0) = F32(a3, 100);
  F32(c, 0xa70) = F32(a3, 40);
  F32(c, 0xa6c) = F32(a3, 36);
  F32(c, 0xaf8) = F32(a3, 68);
  F32(c, 0xafc) = F32(a3, 72);
  F32(c, 0xa74) = F32(a3, 44);
  F32(c, 0xa78) = F32(a3, 48);
  F32(c, 0xa7c) = F32(a3, 52);
  F32(c, 0xa80) = F32(a3, 56);
  F32(c, 0xa9c) = F32(a3, 104);
  F32(c, 0xaa0) = F32(a3, 108);
  F32(c, 0xaa4) = F32(a3, 112);
  F32(c, 0xaa8) = F32(a3, 116);
  *(_WORD *)&GAP(c, 0xb00, 1000) = 0;
  *(_WORD *)&GAP(c, 0xb00, 1002) = 0;
  for ( j = 0;
        (uint16_t)j < F16(a3, 152);
        *(_DWORD *)&GAP(c, 0x9c8, 4 * v18 + 4) = F32(a3 + 4 * v18, 156) )
  {
    v18 = (uint16_t)j++;
  }
  F16(((uint8_t *)(c) + 0x9c8), 0) = F16(a3, 152);
  for ( k = 0; k < F16(a3, 80); ++k )
  {
    v20 = a4 + 12 * k;
    v21 = F16(v20, 8);
    v22 = F32(v20, 4);
    v23 = F32(v20, 0);
    v24 = F16(v11, 2 * 162);
    v39 = v21;
    v25 = 10101;
    v42 = v22;
    v26 = 0;
    do
    {
      if ( v25 == v23 )
        break;
      v25 = ((int32_t *)D(0x6826c0))[192 * (uint16_t)++v26];
    }
    while ( v25 != 0 );
    v27 = &v11[12 * *((uint16_t *)v11 + 162)];
    F32(v27, 4 * 82) = v23;
    F32(v27, 4 * 83) = v42;
    F16(&v11[12 * v24 + 336], 0) = v39;
    ++F16(v11, 2 * 162);
  }
  v28 = v11 + 312;
  result = (int16_t *)(a3 + 120);
  for ( m = 3; m != 0; --m )
  {
    v31 = *result++;
    *v28++ = v31;
  }
  F32(v11, 4 * 80) = F32(a3, 92);
  v32 = 0;
  for ( (*n__f) = 0; (uint16_t)(*n__f) < F16(a3, 24); ++(*n__f) )
  {
    v43 = D(0x628708);
    v33 = v11 + 1168;
    v40 = 18;
    result = (int16_t *)(v11 + 1132);
    do
    {
      if ( F32((a4 + 16 * (v32 + 48)), 0) == *v43 )
      {
        v34 = a4 + 16 * v32;
        *(result - 18) = F16(v34, 772);
        *result = F16(v34, 780);
        *v33 = F32(v34, 776);
      }
      ++result;
      ++v33;
      v35 = v40 == 1;
      v43 += 35;
      --v40;
    }
    while ( !v35 );
    v32 = (*n__f) + 1;
  }
  return result;
}

/* 0x41c2f0: _WORD *__thiscall sub_41C2F0(_DWORD **this, _DWORD *a2, _WORD *a3) */
int32_t tm_41c2f0(wl_char *self, int32_t * a2, uint16_t * a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t * result = 0;

  if ( a2 != 0 )
    *a2 = 10040;
  result = a3;
  if ( a3 != 0 )
    *a3 = *(_DWORD *)(**(((int32_t * *)c) + 200) + 2612);
  return result;
}

/* 0x41c320: int *__thiscall sub_41C320(int **this, int *a2, unsigned __int16 a3) */
int32_t tm_41c320(wl_char *self, int32_t * a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    int32_t * result = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;

  result = a2;
  if ( a2 == (int *)10040 )
  {
    result = *(((int32_t * *)c) + 200);
    v4 = *result;
    F32(v4, 2612) = a3;
    v5 = v4 + 2604;
    v6 = F32(v5, 16);
    if ( v6 != 0 )
    {
      v7 = F16(v5, 22);
      result = 0;
      if ( v7 != 0 )
      {
        while ( F32((v6 + 8 * (uint16_t)result), 0) != a3 )
        {
          result = (int *)((uint8_t *)result + 1);
          if ( (uint16_t)result >= v7 )
            return result;
        }
        result = (int *)(uint16_t)result;
        F32(v5, 24) = F32(v6 + 8 * (uint16_t)result, 4);
      }
    }
  }
  return result;
}

/* 0x560a00: int __thiscall sub_560A00(cls_560A00 *this, int a2) */
int32_t tm_560a00(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint32_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t i = 0;
    char * v6 = 0;

  v2 = F32(c, 0x1800);
  v3 = 0;
  if ( v2 == 0 )
    return 0;
  for ( i = c; F32(((uint8_t *)(i) + 0x0), 0) != a2; i = ((uint8_t *)i + 12) )
  {
    if ( ++v3 >= v2 )
      return 0;
  }
  v6 = &GAP(c, 0x0, 12 * v3);
  if ( v6 != 0 )
    return F32(v6, 4 * 1);
  else
    return 0;
}

/* 0x562a10: void __thiscall sub_562A10(ar_gdi_slot *this) */
void tm_562a10(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v3 = 0;

  if ( ((ar_gdi_slot *)c)->array != 0 )
  {
    for ( i = 0; i < ((ar_gdi_slot *)c)->capacity; ++i )
    {
      v3 = i;
      if ( ((ar_gdi_slot *)c)->array[v3] != 0 )
      {
        API_DeleteObject((intptr_t)(((ar_gdi_slot *)c)->array[v3]));
        ((ar_gdi_slot *)c)->array[v3] = 0;
      }
    }
    g_eng.op_delete((void *)((ar_gdi_slot *)c)->array);
    ((ar_gdi_slot *)c)->capacity = 0;
    ((ar_gdi_slot *)c)->count = 0;
    ((ar_gdi_slot *)c)->array = 0;
  }
}

/* 0x56aea0: int __thiscall sub_56AEA0(cls_41BBE0 *this, int a2) */
int32_t tm_56aea0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint32_t v10 = 0;
    uint32_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    int32_t * v20 = 0;
    int32_t ** v21 = 0;
    intptr_t i = 0;
    int32_t * v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    int v26 = 0;
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
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    uint16_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    int32_t * v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    uint8_t * v61 = 0;
    intptr_t j = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    uint16_t v65 = 0;
    uint16_t v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    int16_t v73 = 0;
    int32_t * v74 = 0;
    uint16_t k = 0;
    intptr_t *** v76 = 0;
    intptr_t v77 = 0;
    intptr_t v78 = 0;
    intptr_t v79 = 0;
    intptr_t v80 = 0;
    intptr_t v81 = 0;
    intptr_t v82 = 0;
    int32_t * v83 = 0;
    intptr_t v84 = 0;
    int32_t * v85 = 0;
    intptr_t v86 = 0;
    int32_t * v87 = 0;
    intptr_t v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    intptr_t v91 = 0;
    intptr_t v92 = 0;
    int32_t * v93 = 0;
    uint16_t v94 = 0;
    intptr_t v95 = 0;
    int32_t * v96 = 0;
    int32_t * v97 = 0;
    intptr_t v98 = 0;
    _Alignas(8) uint8_t frame_[0x7c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v104__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x7c */
    intptr_t *const v103__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x7c */
    intptr_t *const v102__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x7c */
    intptr_t *const v101__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x7c */
    intptr_t *const v100__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x7c */
    intptr_t *const v105__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x68 */
    intptr_t *const v106__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x64 */
    intptr_t *const v107__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x60 */
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x58 */
    intptr_t *const v110__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x54 */
    intptr_t *const v111__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x50 */
    intptr_t *const v112__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x4c */
    intptr_t *const v113__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x48 */
    intptr_t *const v114__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x40 */
    uint32_t *const v115__f = (uint32_t *)(frame_ + 0x48);   /* ebp-0x34 */
    uint32_t *const v116__f = (uint32_t *)(frame_ + 0x4c);   /* ebp-0x30 */
    intptr_t *const v117__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x2c */
    intptr_t *const v118__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x28 */
    uint32_t *const TickCount__f = (uint32_t *)(frame_ + 0x5c);   /* ebp-0x20 */
    intptr_t *const v120__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0x1c */
    intptr_t *const v121__f = (intptr_t *)(frame_ + 0x68);   /* ebp-0x14 */
    intptr_t *const v122__f = (intptr_t *)(frame_ + 0x78);   /* ebp-0x4 */

  v3 = 0;
  (*v113__f) = 0;
  (*v112__f) = 0;
  (*v106__f) = 0;
  (*v105__f) = 0;
  (*v109__f) = 0;
  (*v111__f) = 0;
  (*v122__f) = 0;
  (*v110__f) = 0;
  (*v107__f) = 0;
  F16(v114__f, 0x4) = 50;
  F32(v114__f, 0x0) = (int)(intptr_t)g_eng.op_new(0x578u);
  F16(v114__f, 0x6) = 0;
  if ( F32(D(0x8a6b54), 0) == 0 )
    g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a27d0)), D(0x8a9b6c));
  tm_41bbe0(c);
  v4 = (intptr_t)g_eng.op_new(0x10u);
  if ( v4 != 0 )
  {
    F32(v4, 0x0) = 0;
    F16(((uint8_t *)(v4) + 0x4), 0) = 0;
    *(_WORD *)&GAP(v4, 0x4, 2) = 0;
  }
  else
  {
    v4 = 0;
  }
  F32(c, 0x0) = (int)v4;
  tm_56cb90(v4, 0xCu, SYS32(10016), SYS32(10020));
  v5 = (intptr_t)g_eng.op_new(0x170u);
  SETLB((*v122__f), 1);
  if ( v5 != 0 )
    { W_input_ctor((intptr_t)(v5)); v6 = v5; }   /* (a ctor returns its object) */
  else
    v6 = 0;
  SETLB((*v122__f), 0);
  F32(c, 0x4) = (int)v6;
  tm_56bfd0(v6, 1, 1, 1);
  W_frame_cache_clear((intptr_t)(SYS32(10068)));
  RC_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
  if ( F32(D(0x8a6b54), 0) == 0 )
    g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a27b0)), D(0x8a9b6c));
  (*v115__f) = 0;
  (*v116__f) = 17;
  (*v117__f) = 0;
  (*v118__f) = 0;
  (*v120__f) = 0;
  (*TickCount__f) = API_GetTickCount();
  RC_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
  while ( 1 )
  {
    do
    {
      while ( 1 )
      {
        v7 = API_GetTickCount();
        v8 = (*v118__f);
        v9 = v7;
        (*v121__f) = v7;
        if ( (*v118__f) == 0 )
        {
          (*v115__f) = v7;
          RC_5b1030();
LABEL_24:
          v8 = 2;
LABEL_25:
          (*v118__f) = v8;
          goto LABEL_26;
        }
        if ( (*v118__f) != 1 )
        {
          if ( (*v118__f) != 2 )
            goto LABEL_26;
          if ( v7 - (*v117__f) <= (*v116__f) )
          {
            (*v116__f) -= 16;
            if ( (*v116__f) > 0x10 )
              goto LABEL_26;
            v8 = 1;
          }
          else
          {
            (*v116__f) = 0;
            v8 = 1;
          }
          goto LABEL_25;
        }
        v10 = v7 + (*v116__f) - (*v115__f);
        if ( v10 > 0x64 )
          v10 = 100;
        v11 = v10;
        (*v115__f) = v9;
        (*v116__f) = v10;
        RC_5b1030();
        if ( v11 > 0x10 )
          goto LABEL_24;
LABEL_26:
        if ( v8 == 1 )
        {
          if ( v9 - (*TickCount__f) <= 0x3E8 )
          {
            ++(*v120__f);
          }
          else
          {
            (*v120__f) = 0;
            (*TickCount__f) = v9;
          }
        }
        else if ( v8 == 2 )
        {
          (*v117__f) = v9;
        }
        if ( v8 != 1 )
          break;
        if ( (*v106__f) != 0 )
        {
          if ( (*v106__f) > 1 && (*v106__f) <= 3 )
          {
            v76 = SYS32(2808);
            if ( ***(_DWORD ***)&SYSB(2808) == 0 )
              RC_4184a0((intptr_t)(SYS32(2808)), (intptr_t)(0));
            W_rp_5b9b70((intptr_t)(***v76), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(0), (intptr_t)(0));
          }
        }
        else
        {
          RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
        }
        switch ( (*v106__f) )
        {
          case 0:
          case 1:
          case 2:
            v77 = SYS32(2808);
            if ( ***(_DWORD ***)&SYSB(2808) == 0 )
              RC_4184a0((intptr_t)(SYS32(2808)), (intptr_t)(0));
            v78 = *(intptr_t *)(SYS32(10348) + 364);
            v79 = F32(**(_DWORD **)v77, 4);
            if ( v3 > 0 )
            {
              v80 = W_tint_fade_a((intptr_t)(v3), (intptr_t)(1000));
              if ( v80 != 0 )
                goto LABEL_222;
              goto LABEL_215;
            }
            break;
          case 3:
          case 4:
            v81 = SYS32(2808);
            if ( ***(_DWORD ***)&SYSB(2808) == 0 )
              RC_4184a0((intptr_t)(SYS32(2808)), (intptr_t)(0));
            v78 = *(intptr_t *)(SYS32(10348) + 364);
            v79 = F32(**(_DWORD **)v81, 8);
            if ( v3 > 0 )
            {
              v80 = W_tint_fade_a((intptr_t)(v3), (intptr_t)(1000));
              if ( v80 != 0 )
LABEL_222:
                W_rd_494e10((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)), (intptr_t)(v78), (intptr_t)(v79), (intptr_t)(v80), (intptr_t)(0), (intptr_t)(0), (intptr_t)(*(_DWORD *)&GAP(v79, 0x14, 20)));
              else
LABEL_215:
                W_rp_5b9b70((intptr_t)(v79), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(0), (intptr_t)(0));
            }
            break;
          case 5:
            v82 = W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(2u));
            tm_56c610(F32(SYS32(10348), 364), v82, 0, 0);
            v83 = (_DWORD *)W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(3u));
            tm_56c4e0(F32(SYS32(10348), 364), v83, v3, 1000, 0, 0);
            break;
          case 6:
            v84 = W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(3u));
            tm_56c610(F32(SYS32(10348), 364), v84, 0, 0);
            v85 = (_DWORD *)W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(4u));
            tm_56c4e0(F32(SYS32(10348), 364), v85, v3, 1000, 0, 0);
            break;
          case 7:
            v86 = W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(4u));
            tm_56c610(F32(SYS32(10348), 364), v86, 0, 0);
            v87 = (_DWORD *)W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(5u));
            v88 = 192;
            v89 = 7000 * v3 / 1000;
            do
            {
              v90 = v89;
              if ( v89 > 0 )
              {
                if ( v89 >= 1000 )
                  v90 = 1000;
                v91 = W_tint_fade_a((intptr_t)(v90), (intptr_t)(1000));
                tm_56c580(
                  F32(SYS32(10348), 364),
                  v87,
                  v88,
                  416,
                  4,
                  48,
                  v88,
                  416,
                  v91);
              }
              v88 += 4;
              v89 -= 100;
            }
            while ( v88 < 416 );
            break;
          case 8:
          case 9:
            if ( v3 < 1000 )
            {
              v92 = W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(5u));
              tm_56c610(F32(SYS32(10348), 364), v92, 0, 0);
            }
            v93 = (_DWORD *)W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(6u));
            tm_56c4e0(F32(SYS32(10348), 364), v93, v3, 1000, 0, 0);
            if ( (*v107__f) != 0 && v3 == 1000 )
            {
              v94 = 0;
              v95 = 16;
              do
              {
                if ( v94 == F16(F32((*v107__f), 0x174), 20) )
                {
                  v96 = (_DWORD *)W_rd_418470((intptr_t)(SYS32(2812)), (intptr_t)(v94));
                  tm_56c470(F32(SYS32(10348), 364), v96, (*v109__f), 1200, 0, v95);
                }
                ++v94;
                v95 += 32;
              }
              while ( v94 < 5u );
            }
            break;
          case 10:
            RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
            v97 = (_DWORD *)W_rd_418470((intptr_t)(SYS32(2808)), (intptr_t)(6u));
            tm_56c4e0(F32(SYS32(10348), 364), v97, v3, 1000, 0, 0);
            break;
        }
        tm_56c180(&(*v114__f), F32(SYS32(10348), 364));
        if ( (*v112__f) == 0 && F32(D(0x8a6b54), 0) == 0 )
          g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a277c)), 0, D(0x8a9b6c));
        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
        (*v112__f) = 1;
      }
    }
    while ( v8 != 2 );
    W_timed_sounds_tick((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    RC_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    v12 = c;
    tm_566250(F32(c, 0x4), 0);
    if ( RC_43c110((intptr_t)(F32(c, 0x4)), (intptr_t)(v9), (intptr_t)(34)) != 0 )
      break;
    if ( (*v106__f) >= 8 || (*v106__f) < 1 && a2 == 0 )
      goto LABEL_43;
    v13 = F32(c, 0x4);
    v14 = 63;
    v15 = (_DWORD *)(v13 + 264);
    while ( 1 )
    {
      v16 = (_DWORD *)*v15;
      if ( *(_DWORD *)*v15 != 0 && v16[2] == 1 && (unsigned int)(v9 - v16[1]) <= 0x64 )
        break;
      --v14;
      --v15;
      if ( v14 < 0 )
        goto LABEL_43;
    }
    v3 = 0;
    F32(F32(v13 + 4 * v14, 12), 0) = 0;
    if ( (*v106__f) < 3 )
    {
      if ( F32(D(0x8a6b54), 0) == 0 )
        g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a2794)), D(0x8a9b6c));
      if ( SYS32(0x2884) != 0
        && F32(CFG(), 372) != 0
        && (!RC_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Cu))
         || RC_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Cu)) != 0) )
      {
        F16(D(0x8a93e8), 0) = 0;
        RC_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
        if ( RC_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
        {
          if ( RC_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(28), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 )
            goto LABEL_60;
        }
        else if ( RC_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(28), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 )
        {
          goto LABEL_60;
        }
        g_eng.h408b90(D(0x8a9530), (SYS32(0x2884) + 0x2c), 0, D(0x8a9b6c));
      }
    }
LABEL_60:
    v18 = 11;
    v19 = F32(c, 0x4);
    F16(v19, 364) = 0;
    v20 = (_DWORD *)(v19 + 276);
    do
    {
      v20[11] = 0;
      *v20++ = 0;
      --v18;
    }
    while ( v18 != 0 );
    v21 = (_DWORD **)(v19 + 12);
    for ( i = 64; i != 0; --i )
    {
      v23 = *v21++;
      *v23 = 0;
    }
    F32(v19, 268) = 0;
    F32(v19, 272) = 0;
    (*v106__f) = 8;
    F16(v114__f, 0x6) = 0;
LABEL_43:
    switch ( (*v106__f) )
    {
      case 0:
        if ( v3 < 1000 )
        {
          v17 = v3 + 20;
          goto LABEL_46;
        }
        (*v106__f) = 1;
        goto LABEL_193;
      case 1:
        if ( (*v105__f) >= 50 )
          (*v106__f) = 2;
        else
          ++(*v105__f);
        goto LABEL_193;
      case 2:
        if ( v3 > 0 )
        {
          v24 = v3 - 20;
          v25 = 0;
          v26 = v24 < 0;
          goto LABEL_192;
        }
        v3 = 0;
        (*v105__f) = 0;
        (*v106__f) = 3;
        if ( F32(D(0x8a6b54), 0) == 0 )
          g_eng.log_fail_406440((const char *)D(0x8a6620), ((char *)D(0x8a2794)), D(0x8a9b6c));
        if ( SYS32(0x2884) == 0
          || F32(CFG(), 372) == 0
          || RC_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Cu))
          && RC_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Cu)) == 0 )
        {
          goto LABEL_193;
        }
        F16(D(0x8a93e8), 0) = 0;
        RC_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
        if ( RC_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
        {
          if ( RC_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(28), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 )
            goto LABEL_193;
LABEL_80:
          g_eng.h408b90(D(0x8a9530), (SYS32(0x2884) + 0x2c), 0, D(0x8a9b6c));
          goto LABEL_193;
        }
        if ( RC_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(28), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
          goto LABEL_80;
        goto LABEL_193;
      case 3:
        if ( v3 >= 1000 )
        {
          (*v106__f) = 4;
        }
        else
        {
          v17 = v3 + 10;
LABEL_46:
          if ( v17 <= 1000 )
            v3 = v17;
          else
            v3 = 1000;
        }
        goto LABEL_193;
      case 4:
        if ( (*v105__f) >= 20 )
        {
          v3 = 0;
          (*v106__f) = 5;
          (*v105__f) = 0;
        }
        else
        {
          ++(*v105__f);
        }
        goto LABEL_193;
      case 5:
        if ( v3 < 1000 )
        {
          v27 = v3 + 100;
          v3 = 1000;
          if ( v27 <= 1000 )
            v3 = v27;
        }
        if ( (*v105__f) >= 40 )
        {
          v3 = 0;
          (*v106__f) = 6;
          (*v105__f) = 0;
        }
        else
        {
          ++(*v105__f);
        }
        goto LABEL_193;
      case 6:
        if ( v3 < 1000 )
        {
          v28 = v3 + 10;
          v3 = 1000;
          if ( v28 <= 1000 )
            v3 = v28;
        }
        if ( (*v105__f) >= 120 )
        {
          v3 = 0;
          (*v106__f) = 7;
          (*v105__f) = 0;
        }
        else
        {
          ++(*v105__f);
        }
        goto LABEL_193;
      case 7:
        if ( v3 < 1000 )
        {
          v29 = v3 + 20;
          if ( v3 + 20 <= 1000 )
          {
            v3 += 20;
            if ( v29 < 850 )
              tm_56c070(&(*v114__f), 21, 0, 8, 800, 224 * v3 / 900 + 192, 416, 16, 24, 20, 0x14u);
          }
          else
          {
            v3 = 1000;
          }
        }
        if ( (*v105__f) >= 90 )
        {
          v3 = 0;
          (*v106__f) = 8;
          (*v105__f) = 0;
        }
        else
        {
          ++(*v105__f);
        }
        goto LABEL_193;
      case 8:
      case 9:
        if ( v3 >= 1000 )
        {
          if ( (*v106__f) == 8 )
          {
            if ( (*v109__f) >= 1000 )
            {
              (*v106__f) = 9;
            }
            else if ( (*v109__f) + 50 <= 1000 )
            {
              (*v109__f) += 50;
            }
            else
            {
              (*v109__f) = 1000;
            }
          }
          else if ( (*v106__f) == 9 )
          {
            if ( (*v109__f) <= 0 )
              (*v106__f) = 8;
            else
              (*v109__f) = ((*v109__f) - 50) & (((*v109__f) - 50 < 0) - 1);
          }
        }
        else if ( v3 + 20 <= 1000 )
        {
          v3 += 20;
        }
        else
        {
          v3 = 1000;
        }
        if ( (*v107__f) != 0 )
          goto LABEL_146;
        v30 = F32(c, 0x0);
        if ( F16(F32(c, 0x0), 6) < F16(F32(c, 0x0), 4) )
        {
          v32 = 4 * F16(F32(c, 0x0), 6);
          W_win_init((intptr_t)(F32(v32, F32(v30, 0x0))), (intptr_t)((int)v30), (intptr_t)(0), (intptr_t)(0), (intptr_t)(100), (intptr_t)(100), (intptr_t)(1u), (intptr_t)(0));
          ++F16(v30, 0x6);
          W_win_mgr_topmost((intptr_t)(v30));
          v31 = F32(v32, F32(v30, 0x0));
          (*v110__f) = v31;
        }
        else
        {
          v31 = 0;
          (*v110__f) = 0;
        }
        v33 = W_win_ctrl_take((intptr_t)(v31));
        v34 = v33;
        if ( v33 != 0 )
          W_ctrl_grid_init((intptr_t)(v33), (intptr_t)(0), (intptr_t)(0), (intptr_t)(6), (intptr_t)(1), (intptr_t)(6), (intptr_t)(0));
        v35 = F32(v34, 0x174);
        (*v107__f) = v34;
        v36 = F32(v35, 16);
        if ( v36 < F32(v35, 4) )
        {
          (*v100__f) = F32(v35, 16);
          F32(v35, 16) = v36 + 1;
          v37 = 16 * v36;
          F32((v37 + F32(v34, 0x17c)), 0) = 0;
          F32(v37 + F32(v34, 0x17c), 4) = 26;
          F32(v37 + F32(v34, 0x17c), 8) = 1;
          W_grid_row_reset((intptr_t)(v34), (intptr_t)((*v100__f)));
        }
        v38 = F32(v34, 0x174);
        v39 = F32(v38, 16);
        if ( v39 < F32(v38, 4) )
        {
          (*v101__f) = F32(v38, 16);
          F32(v38, 16) = v39 + 1;
          v40 = 16 * v39;
          F32((v40 + F32(v34, 0x17c)), 0) = 0;
          F32(v40 + F32(v34, 0x17c), 4) = 28;
          F32(v40 + F32(v34, 0x17c), 8) = 1;
          W_grid_row_reset((intptr_t)(v34), (intptr_t)((*v101__f)));
        }
        v41 = F32(v34, 0x174);
        v42 = F32(v41, 16);
        if ( v42 < F32(v41, 4) )
        {
          (*v102__f) = F32(v41, 16);
          F32(v41, 16) = v42 + 1;
          v43 = 16 * v42;
          F32((v43 + F32(v34, 0x17c)), 0) = 0;
          F32(v43 + F32(v34, 0x17c), 4) = 30;
          F32(v43 + F32(v34, 0x17c), 8) = 1;
          W_grid_row_reset((intptr_t)(v34), (intptr_t)((*v102__f)));
        }
        v44 = F32(v34, 0x174);
        v45 = F32(v44, 16);
        if ( v45 < F32(v44, 4) )
        {
          (*v103__f) = F32(v44, 16);
          F32(v44, 16) = v45 + 1;
          v46 = 16 * v45;
          F32((v46 + F32(v34, 0x17c)), 0) = 0;
          F32(v46 + F32(v34, 0x17c), 4) = 29;
          F32(v46 + F32(v34, 0x17c), 8) = 1;
          W_grid_row_reset((intptr_t)(v34), (intptr_t)((*v103__f)));
        }
        v47 = F32(v34, 0x174);
        v48 = F32(v47, 16);
        if ( v48 < F32(v47, 4) )
        {
          (*v104__f) = F32(v47, 16);
          F32(v47, 16) = v48 + 1;
          v49 = 16 * v48;
          F32((v49 + F32(v34, 0x17c)), 0) = 0;
          F32(v49 + F32(v34, 0x17c), 4) = 8;
          F32(v49 + F32(v34, 0x17c), 8) = 1;
          W_grid_row_reset((intptr_t)(v34), (intptr_t)((*v104__f)));
        }
        v50 = F32(v34, 0x174);
        v51 = F32(v50, 16);
        v52 = 0;
        if ( v51 > 0 )
        {
          v53 = 0;
          v54 = F32(v34, 0x17c);
          while ( 1 )
          {
            v55 = (_DWORD *)(v54 + 16 * v53);
            if ( *v55 == 0 && v55[1] == F32(CFG(), 2656) )
              break;
            v53 = ++v52;
            if ( v52 >= v51 )
              goto LABEL_146;
          }
          F32(v50, 20) = v52;
          W_grid_page_fit((intptr_t)((*v107__f)));
        }
LABEL_146:
        v56 = F32(c, 0x4);
        v57 = 0;
        if ( RC_43c110((intptr_t)(v56), (intptr_t)((*v121__f)), (intptr_t)(2)) != 0 )
          v57 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(2));
        if ( RC_43c110((intptr_t)(v56), (intptr_t)((*v121__f)), (intptr_t)(4)) != 0 )
          v57 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(3));
        if ( RC_43c110((intptr_t)(v56), (intptr_t)((*v121__f)), (intptr_t)(1)) != 0 )
          v57 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(0));
        if ( v57 == 0 )
        {
          if ( F32((v56 + (69) * 0x4), 0) != 0 )
            v58 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(6));
          else
            v58 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(7));
          v57 = v58;
        }
        if ( RC_43c110((intptr_t)(v56), (intptr_t)((*v121__f)), (intptr_t)(3)) != 0 )
          v57 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(1));
        if ( v57 == 0 )
        {
          if ( F32((v56 + (70) * 0x4), 0) != 0 )
            v59 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(4));
          else
            v59 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(5));
          v57 = v59;
        }
        if ( RC_43c110((intptr_t)(v56), (intptr_t)((*v121__f)), (intptr_t)(36)) != 0 )
          v57 = W_ctrl_key((intptr_t)((*v107__f)), (intptr_t)(9));
        switch ( v57 )
        {
          case 1:
          case 2:
            W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(9u), (intptr_t)(0), (intptr_t)(0));
            goto LABEL_172;
          case 3:
            if ( F32(16 * F32((F32((*v107__f), 0x174) + 20), 0) + *(_DWORD *)&GAP(((*v107__f) + (1) * 0x178), 0x0, 4), 8) == 0 )
            {
              W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(6u), (intptr_t)(0), (intptr_t)(0));
              goto LABEL_186;
            }
            W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0));
LABEL_172:
            if ( v57 != 3 )
              goto LABEL_186;
            v60 = 0;
            v61 = &SYSB(10364);
            for ( j = 2; j != 0; --j )
            {
              if ( F32(v61, 0) == 0 )
              {
                RC_5ba120((intptr_t)(SYS32(10352)), (intptr_t)(v61), (intptr_t)((int)(*(intptr_t *)D(0x8a93ec))), (intptr_t)(v60));
                if ( F32(v61, 0) != 0 )
                  RC_5ba290((intptr_t)(*(intptr_t *)v61));
              }
              ++v60;
              v61 += 4;
            }
            v63 = F32(16 * F32((F32((*v107__f), 0x174) + 20), 0) + *(_DWORD *)&GAP(((*v107__f) + (1) * 0x178), 0x0, 4), 4);
            if ( v63 == 29 )
              goto LABEL_185;
            v64 = F32(CFG(), 2648);
            if ( v64 == 0 )
              goto LABEL_185;
            v65 = F16(CFG(), 2654);
            v66 = 0;
            if ( v65 == 0 )
              goto LABEL_185;
            break;
          case 4:
            W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(7u), (intptr_t)(0), (intptr_t)(0));
            goto LABEL_172;
          default:
            goto LABEL_172;
        }
        while ( v63 != F32(v64 + 8 * v66, 4) )
        {
          if ( ++v66 >= v65 )
            goto LABEL_185;
        }
        tm_41bb80(SYS32(800), 0x5Eu, F32((v64 + 8 * v66), 0));
LABEL_185:
        (*v106__f) = 10;
        v3 = 1000;
        (*v113__f) = F32(16 * F32((F32((*v107__f), 0x174) + 20), 0) + *(_DWORD *)&GAP(((*v107__f) + (1) * 0x178), 0x0, 4), 4);
LABEL_186:
        if ( (*v111__f) >= 4500 )
        {
          RC_40a5d0((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          (*v106__f) = 10;
          v3 = 1000;
        }
        goto LABEL_193;
      case 10:
        if ( (*v107__f) != 0 )
        {
          v67 = (*v110__f);
          (*v110__f) = 0;
          (*v107__f) = 0;
          *(_DWORD *)((uint8_t *)(v67 + (1) * 0x50) + 0x0) = 0;
        }
        if ( v3 <= 0 )
        {
          v98 = (*v113__f);
          v12 = c;
          goto LABEL_248;
        }
        v24 = v3 - 20;
        v25 = 0;
        v26 = v24 < 0;
LABEL_192:
        SETLB(v25, v26);
        v3 = v24 & (v25 - 1);
LABEL_193:
        v68 = F16(v114__f, 0x6) - 1;
        if ( F16(v114__f, 0x6) != 0 )
        {
          v69 = 28 * v68;
          do
          {
            v70 = v69 + F32(v114__f, 0x0);
            v71 = F32(v69 + F32(v114__f, 0x0), 8);
            v72 = F32(v69 + F32(v114__f, 0x0), 4) - v71;
            F32(v70, 8) = v71 + 2;
            v73 = F16(v70, 12);
            F32(v70, 4) = v72;
            if ( v73 != 0 )
              F16(v70, 12) = v73 - 1;
            else
              tm_56c030(&(*v114__f), v68);
            --v68;
            v69 -= 28;
          }
          while ( v68 >= 0 );
        }
        if ( (*v111__f) < 4500 )
          ++(*v111__f);
        tm_56c930(F32(c, 0x0));
        v74 = (_DWORD *)F32(c, 0x0);
        for ( k = 0; k < F16(F32(c, 0x0), 6); ++k )
        {
          W_win_tick((intptr_t)(F32(*v74, 4 * k)));
          v74 = (_DWORD *)F32(c, 0x0);
        }
        break;
    }
  }
  v98 = 6;
LABEL_248:
  RC_5aff00((intptr_t)(&(*v114__f)));
  tm_56c2b0(v12);
  (*v122__f) = -1;
  RC_5aff00((intptr_t)(&(*v114__f)));
  return v98;
}

/* 0x41bb80: int __thiscall sub_41BB80(cls_41BB80 *this, unsigned __int16 a2, int a3) */
int32_t tm_41bb80(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;

  result = F32(c, 0x0);
  v4 = F32(c, 0x0) + 28 * a2;
  v5 = F32(v4, 16);
  F32(v4, 8) = a3;
  if ( v5 != 0 )
  {
    v6 = F16(v4, 22);
    result = 0;
    if ( v6 != 0 )
    {
      while ( F32((v5 + 8 * (uint16_t)result), 0) != a3 )
      {
        if ( (uint16_t)++result >= v6 )
          return result;
      }
      result = (uint16_t)result;
      F32(v4, 24) = F32(v5 + 8 * (uint16_t)result, 4);
    }
  }
  return result;
}

/* 0x56bfd0: _DWORD **__thiscall sub_56BFD0(cls_56BFD0 *this, int a2, int a3, int a4) */
int32_t tm_56bfd0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t i = 0;
    int32_t ** result = 0;
    intptr_t j = 0;
    int32_t * v8 = 0;

  F32(c, 0x0) = a2;
  F32(((uint8_t *)(c) + 0x4), 0) = a3;
  *(_DWORD *)&GAP(c, 0x4, 4) = a4;
  F16(c, 2 * 182) = 0;
  v4 = ((uint8_t *)c + 276);
  for ( i = 11; i != 0; --i )
  {
    *(_DWORD *)&GAP(v4, 0x4, 40) = 0;
    F32(v4, 0x0) = 0;
    v4 = ((uint8_t *)v4 + 4);
  }
  result = (_DWORD **)&GAP(c, 0x4, 8);
  for ( j = 64; j != 0; --j )
  {
    v8 = *result++;
    *v8 = 0;
  }
  F32(c, 4 * 67) = 0;
  F32(c, 4 * 68) = 0;
  return result;
}

/* 0x56c030: int __thiscall sub_56C030(cls_56C030 *this, unsigned __int16 a2) */
int32_t tm_56c030(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;

  qmemcpy((void *)(F32(c, 0x0) + 28 * a2), (const void *)(F32(c, 0x0) + 28 * --F16(c, 0x6)), 0x1Cu);
  return a2;
}

/* 0x56c070: __int16 __thiscall drive_spawn_sparkle_56C070( cls_56C070 *this, __int16 a2, __int16 a3, __int16 a4, int a5, int a6, int a7, int a8, int a9, __int16 a10, unsigned __int16 a11) */
int32_t tm_56c070(wl_char *self, int16_t a2, int16_t a3, int16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int16_t a10, uint16_t a11)
{
    intptr_t c = (intptr_t)self;
    intptr_t v12 = 0;
    intptr_t v13 = 0;

  SETLO(v12, F16(c, 0x6));
  if ( (uint16_t)v12 < F16(c, 0x4) )
  {
    F16(c, 0x6) = v12 + 1;
    v13 = 28 * (uint16_t)v12;
    F32((v13 + F32(c, 0x0)), 0) = 100 * (a6 + a8 * rnd() / 0x8000);
    F32(v13 + F32(c, 0x0), 4) = 100 * (a7 + a9 * rnd() / 0x8000);
    F16(v13 + F32(c, 0x0), 18) = a3;
    F16(v13 + F32(c, 0x0), 20) = a4;
    F16(v13 + F32(c, 0x0), 16) = a2;
    F32(v13 + F32(c, 0x0), 24) = a5;
    F32(v13 + F32(c, 0x0), 8) = 200 * rnd() / 0x8000;
    F16(v13 + F32(c, 0x0), 14) = a10 + a11 * rnd() / 0x8000;
    v12 = v13 + F32(c, 0x0);
    F16(v12, 12) = F16(v12, 14);
  }
  return v12;
}

/* 0x56c180: void __thiscall title_compositor_resolve_56C180(cls_56C180 *this, int a2) */
void tm_56c180(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    uint16_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v2 = 0;
  (*v11__f) = c;
  if ( F16(c, 0x6) != 0 )
  {
    while ( 1 )
    {
      v3 = F16(F32(c, 0x0) + 28 * v2, 20);
      v4 = F32(c, 0x0) + 28 * v2;
      v5 = v3 * F16(v4, 12) / F16(v4, 14);
      if ( v3 - 1 < v5 )
        SETLO(v5, v3 - 1);
      v6 = SYS32(4 * F16(v4, 16) + 2732);
      v7 = F16(v4, 18) - v5 + v3 - 1;
      if ( **(_DWORD **)((uint8_t *)(v6) + 0x0) == 0 )
        RC_4184a0((intptr_t)(v6), (intptr_t)(0));
      v8 = F32(**(_DWORD **)((uint8_t *)(v6) + 0x0), 4 * v7);
      v9 = 20 * F32(v4, 24) / 1000;
      if ( v9 >= 0 )
      {
        v10 = SYS32(10148);
        if ( v9 >= 20 )
          goto LABEL_12;
      }
      else
      {
        v9 = 0;
      }
      v10 = SYS32(4 * v9 + 10072);
LABEL_12:
      W_rp_5bd550((intptr_t)(v10), (intptr_t)((intptr_t)a2), (intptr_t)((int)v8), (intptr_t)(v8[3] + F32(v4, 0) / 100), (intptr_t)(v8[4] + F32(v4, 4) / 100), (intptr_t)(v8[5]), (intptr_t)(v8[6]), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v8[10]), (intptr_t)(*(intptr_t *)&SYSB(864)));
      if ( ++v2 >= F16((*v11__f), 0x6) )
        return;
      c = (*v11__f);
    }
  }
}

/* 0x56c2b0: void __thiscall sub_56C2B0(cls_56C2B0 *this) */
void tm_56c2b0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    uint16_t i = 0;
    intptr_t v3 = 0;
    void * v4 = 0;
    uint8_t * v5 = 0;
    intptr_t j = 0;
    intptr_t v7 = 0;
    uint16_t k = 0;
    uint8_t * v9 = 0;
    intptr_t m = 0;
    intptr_t v11 = 0;
    ar_gdi_slot ** v12 = 0;
    intptr_t n = 0;
    ar_gdi_slot * v14 = 0;
    intptr_t v15 = 0;

  RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  v1 = SYS32(10068);
  for ( i = 0; i < F16(v1, 4); F32(F32(v1, 0) + v3, 20) = 0 )
  {
    v3 = 28 * i;
    v4 = F32(F32(v1, 0), v3 + 24);
    if ( v4 != 0 )
    {
      W_rp_5b9390((intptr_t)(F32(F32(v1, 0), v3 + 24)));
      g_eng.op_delete((void *)v4);
      F32(F32(v1, 0) + v3, 24) = 0;
    }
    ++i;
    F32((F32(v1, 0) + v3), 0) = 0;
    F32(F32(v1, 0) + v3, 4) = 0;
    F32(F32(v1, 0) + v3, 8) = 0;
    F32(F32(v1, 0) + v3, 12) = 0;
    F32(F32(v1, 0) + v3, 16) = 0;
  }
  F16(v1, 6) = 0;
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854780))));
  v5 = &SYSB(2732);
  for ( j = 909; j != 0; --j )
  {
    v7 = F32(v5, 0);
    if ( F32(v5, 0) != 0 && F16(v7, 66) >= 4u )
    {
      for ( k = 0; k < F16(v7, 4); ++k )
        W_slot_frames_free((intptr_t)(v7), (intptr_t)(k));
    }
    v5 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854770))));
  v9 = &SYSB(868);
  for ( m = 465; m != 0; --m )
  {
    v11 = F32(v9, 0);
    if ( F32(v9, 0) != 0 && F16(v11, 20) >= 4u )
      tm_562af0(v11);
    v9 += 4;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854760))));
  v12 = (ar_gdi_slot **)&SYSB(10004);
  for ( n = 16; n != 0; --n )
  {
    v14 = *v12;
    if ( *v12 != 0 && v14->group >= 4u )
      tm_562a10(v14);
    ++v12;
  }
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854744))));
  if ( F32(c, 0x0) != 0 )
  {
    W_pool_head_delete((intptr_t)(F32(c, 0x0)), (intptr_t)(1));
    F32(c, 0x0) = 0;
  }
  v15 = F32(c, 0x4);
  if ( v15 != 0 )
  {
    RC_56c430((intptr_t)(v15), (intptr_t)(1));
    F32(c, 0x4) = 0;
  }
}

/* 0x562af0: void __thiscall sub_562AF0(cls_562AF0 *this) */
void tm_562af0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t i = 0;
    intptr_t v3 = 0;
    void * v4 = 0;

  if ( F32(c, 0x4) != 0 )
  {
    for ( i = F16(c, 0x0) - 1; i >= 0; --i )
    {
      v3 = F32(c, 0x4);
      v4 = F32(v3, 8 * i);
      if ( v4 != 0 )
      {
        RC_5bb3a0((intptr_t)(F32(v3, 8 * i)));
        g_eng.op_delete((void *)v4);
        F32((F32(c, 0x4) + 8 * i), 0) = 0;
      }
    }
    g_eng.op_delete((void *)(void *)F32(c, 0x4));
    F32(c, 0x4) = 0;
  }
}

/* 0x56c470: int __stdcall sub_56C470(paint_ctx *a1, _DWORD *a2, int a3, int a4, int a5, int a6) */
int32_t tm_56c470(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{

    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  result = a3;
  if ( a3 > 0 )
  {
    v7 = 20 * a3 / a4;
    if ( v7 >= 0 )
    {
      v8 = SYS32(10148);
      if ( v7 >= 20 )
        return W_rp_5bd550((intptr_t)(v8), (intptr_t)(a1), (intptr_t)((int)a2), (intptr_t)(a5 + a2[3]), (intptr_t)(a6 + a2[4]), (intptr_t)(a2[5]), (intptr_t)(a2[6]), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a2[10]), (intptr_t)(*(intptr_t *)&SYSB(864)));
    }
    else
    {
      v7 = 0;
    }
    v8 = SYS32(4 * v7 + 10072);
    return W_rp_5bd550((intptr_t)(v8), (intptr_t)(a1), (intptr_t)((int)a2), (intptr_t)(a5 + a2[3]), (intptr_t)(a6 + a2[4]), (intptr_t)(a2[5]), (intptr_t)(a2[6]), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a2[10]), (intptr_t)(*(intptr_t *)&SYSB(864)));
  }
  return result;
}

/* 0x56c4e0: int __stdcall sub_56C4E0(paint_ctx *a1, cls_5B9B70 *a2, int a3, int a4, int a5, int a6) */
int32_t tm_56c4e0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{

    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  result = a3;
  if ( a3 > 0 )
  {
    if ( a4 > 0 )
    {
      v7 = 20 * a3 / a4;
      if ( v7 >= 0 )
      {
        if ( v7 >= 20 )
          return W_rp_5b9b70((intptr_t)(a2), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(a5), (intptr_t)(a6));
      }
      else
      {
        v7 = 0;
      }
    }
    else
    {
      v7 = 0;
    }
    v8 = SYS32(4 * v7 + 10152);
    if ( v8 == 0 )
      return W_rp_5b9b70((intptr_t)(a2), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(a5), (intptr_t)(a6));
    return W_rp_5bd550((intptr_t)(v8), (intptr_t)(a1), (intptr_t)((int)a2), (intptr_t)(a5 + F32(a2, 0xc)), (intptr_t)(a6 + F32(a2, 0x10)), (intptr_t)(F32(((uint8_t *)(a2) + 0x14), 0)), (intptr_t)(*(_DWORD *)&GAP(a2, 0x14, 4)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(*(_DWORD *)&GAP(a2, 0x14, 20)), (intptr_t)(*(intptr_t *)&SYSB(864)));
  }
  return result;
}

/* 0x56c580: int __stdcall title_render_sparkle_56C580( paint_ctx *a1, cls_5B9BF0 *a2, int a3, int a4, int a5, int a6, int a7, int a8, cls_5BD550 *a9) */
int32_t tm_56c580(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9)
{


  if ( a9 != 0 )
    return W_rp_5bd550((intptr_t)(a9), (intptr_t)(a1), (intptr_t)((int)a2), (intptr_t)(a3 + F32(a2, 0xc)), (intptr_t)(a4 + F32(a2, 0x10)), (intptr_t)(a5), (intptr_t)(a6), (intptr_t)(a7), (intptr_t)(a8), (intptr_t)(*(_DWORD *)&GAP(a2, 0x1c, 12)), (intptr_t)(*(intptr_t *)&SYSB(864)));
  else
    return W_rp_5b9bf0((intptr_t)(a2), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(a5), (intptr_t)(a6), (intptr_t)(a7), (intptr_t)(a8));
}

/* 0x56c610: int __stdcall title_draw_sprite_56C610(int a1, cls_5B9B70 *a2, int a3, int a4) */
int32_t tm_56c610(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{


  return W_rp_5b9b70((intptr_t)(a2), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(a3), (intptr_t)(a4));
}

/* 0x56cb90: _DWORD *__thiscall sub_56CB90(cls_56CB90 *this, unsigned __int16 a2, int a3, int a4) */
int32_t tm_56cb90(wl_char *self, uint16_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    int32_t * result = 0;
    intptr_t v6 = 0;
    int v7 = 0;
    intptr_t v8 = 0;

  *(_DWORD *)&F16(c, 0x8) = a3;
  F16(((uint8_t *)(c) + 0x4), 0) = a2;
  F32(c, 0xc) = a4;
  *(_WORD *)&GAP(c, 0x4, 2) = 0;
  result = (intptr_t)g_eng.op_new(4 * a2);
  v6 = 0;
  v7 = F16(((uint8_t *)(c) + 0x4), 0) == 0;
  F32(c, 0x0) = (int)result;
  if ( !v7 )
  {
    do
    {
      result = (intptr_t)g_eng.op_new(0x84u);
      if ( result != 0 )
      {
        result[18] = 0;
        result[1] = 0;
      }
      else
      {
        result = 0;
      }
      v8 = (uint16_t)v6++;
      F32((F32(c, 0x0) + 4 * v8), 0) = result;
    }
    while ( (uint16_t)v6 < F16(((uint8_t *)(c) + 0x4), 0) );
  }
  return result;
}

/* 0x56cd20: int __thiscall sub_56CD20(cls_41BBE0 *this) */
int32_t tm_56cd20(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint32_t v1 = 0;
    intptr_t v2 = 0;
    uint8_t * v3 = 0;
    intptr_t i = 0;
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
    intptr_t j = 0;
    int32_t * v16 = 0;
    char * v17 = 0;
    intptr_t k = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    int32_t * v21 = 0;
    uint16_t v22 = 0;
    int16_t v23 = 0;
    uint16_t v24 = 0;
    int32_t ** v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    uint32_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    int32_t ** v33 = 0;
    int32_t * v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    int32_t ** v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    uint16_t m = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    _Alignas(8) uint8_t frame_[0xbc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v79 = (uint8_t *)(frame_ + 0x76);   /* [2] at ebp-0x46 */
    uint16_t *v80 = (uint16_t *)(frame_ + 0x78);   /* [34] at ebp-0x44 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xbc */
    int16_t *const v54__f = (int16_t *)(frame_ + 0x4);   /* ebp-0xb8 */
    int16_t *const v56__f = (int16_t *)(frame_ + 0xc);   /* ebp-0xb0 */
    uint16_t *const v57__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0xac */
    int16_t *const v58__f = (int16_t *)(frame_ + 0x14);   /* ebp-0xa8 */
    uint16_t *const v59__f = (uint16_t *)(frame_ + 0x18);   /* ebp-0xa4 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0xa0 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x9c */
    uint16_t * *const v62__f = (uint16_t * *)(frame_ + 0x24);   /* ebp-0x98 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x94 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x90 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8c */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x88 */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x84 */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x80 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x7c */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x78 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x74 */
    uint32_t *const v72__f = (uint32_t *)(frame_ + 0x4c);   /* ebp-0x70 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x6c */
    uint32_t *const v74__f = (uint32_t *)(frame_ + 0x54);   /* ebp-0x68 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x64 */
    uint32_t *const TickCount__f = (uint32_t *)(frame_ + 0x60);   /* ebp-0x5c */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x64);   /* ebp-0x58 */
    char *const v78__f = (char *)(frame_ + 0x74);   /* ebp-0x48 */

  (*v69__f) = 0;
  (*v56__f) = 4400;
  (*v54__f) = 0;
  (*v53__f) = 0;
  (*v61__f) = 0;
  (*v65__f) = false;
  (*v57__f) = 0;
  (*v59__f) = 0;
  SETLO((*v63__f), 0);
  (*v60__f) = -4000;
  (*v66__f) = 800;
  (*v67__f) = 0;
  (*v68__f) = 0;
  (*v58__f) = 10;
  (*v64__f) = 3800;
  tm_56d710(c);
  if ( SYS32(0x2884) != 0
    && F32(CFG(), 372) != 0
    && (!RC_5bbc60((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Du))
     || RC_5bbc90((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Du)) != 0) )
  {
    F16(D(0x8a93e8), 0) = 0;
    RC_5bbcd0((intptr_t)(SYS32(0x2884)), (intptr_t)(20));
    if ( RC_5bbc20((intptr_t)(SYS32(0x2884))) != 0 )
    {
      if ( RC_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(29), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 )
        goto LABEL_10;
    }
    else if ( RC_5bcb80((intptr_t)(SYS32(0x2884)), (intptr_t)(29), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) != 0 )
    {
      goto LABEL_10;
    }
    g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
  }
LABEL_10:
  v1 = 17;
  v2 = 0;
  (*v72__f) = 0;
  (*v73__f) = 17;
  (*v74__f) = 0;
  (*v75__f) = 0;
  (*v77__f) = 0;
  (*TickCount__f) = API_GetTickCount();
  v3 = v79;
  for ( i = 6; i != 0; --i )
  {
    *((uint16_t *)v3 - 1) = 0;
    F16(v3, 0) = 0;
    F32(v3 + (6), 0) = 32000;
    v3 += 12;
  }
  v80[0] = 680;
  v80[6] = 980;
  v80[12] = 1580;
  v80[18] = 1880;
  v80[24] = 2180;
  v80[30] = 2630;
  while ( 1 )
  {
    while ( 1 )
    {
LABEL_13:
      v5 = API_GetTickCount();
      if ( v2 == 0 )
      {
        (*v72__f) = v5;
        RC_5b1030();
LABEL_24:
        v2 = 2;
LABEL_25:
        (*v75__f) = v2;
        goto LABEL_26;
      }
      if ( v2 != 1 )
      {
        if ( v5 - (*v74__f) <= v1 )
        {
          v1 -= 16;
          (*v73__f) = v1;
          if ( v1 > 0x10 )
            goto LABEL_26;
          v2 = 1;
        }
        else
        {
          v1 = 0;
          v2 = 1;
          (*v73__f) = 0;
        }
        goto LABEL_25;
      }
      v6 = v1 - (*v72__f) + v5;
      if ( v6 > 0x64 )
        v6 = 100;
      v1 = v6;
      (*v72__f) = v5;
      (*v73__f) = v6;
      RC_5b1030();
      if ( v1 > 0x10 )
        goto LABEL_24;
LABEL_26:
      if ( v2 == 1 )
      {
        if ( v5 - (*TickCount__f) <= 0x3E8 )
        {
          ++(*v77__f);
        }
        else
        {
          (*v77__f) = 0;
          (*TickCount__f) = v5;
        }
      }
      else
      {
        (*v74__f) = v5;
      }
      if ( v2 != 1 )
        break;
      RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
      if ( (*v58__f) != 0 )
        goto LABEL_138;
      v25 = *(_DWORD ***)&SYSB(2796);
      if ( ***(_DWORD ***)&SYSB(2796) == 0 )
        RC_4184a0((intptr_t)(SYS32(2796)), (intptr_t)(0));
      v26 = F32((**v25 + 4 * (uint16_t)(*v63__f)), 0);
      v27 = 800 * (*v53__f) / 1000;
      v28 = *(intptr_t *)(SYS32(10348) + 364);
      if ( v27 > 0 )
      {
        v29 = (int)((uint64_t)(9162596900LL * v27) >> 32) >> 6;
        v30 = (v29 >> 31) + v29;
        if ( v30 >= 0 )
        {
          v31 = v30;
          if ( v30 >= 20 )
            goto LABEL_114;
        }
        else
        {
          v31 = 0;
        }
        v32 = SYS32(4 * v31 + 10152);
        if ( v32 == 0 )
        {
LABEL_114:
          W_rp_5b9b70((intptr_t)(v26), (intptr_t)((int)v28), (intptr_t)(248), (intptr_t)((*v60__f) / 100 + 48));
          goto LABEL_116;
        }
        W_rp_5bd550((intptr_t)(v32), (intptr_t)(v28), (intptr_t)(v26), (intptr_t)(F32(v26, 12) + 248), (intptr_t)((*v60__f) / 100 + 48 + F32(v26, 16)), (intptr_t)(F32(v26, 20)), (intptr_t)(F32(v26, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v26, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
      }
LABEL_116:
      v33 = *(_DWORD ***)&SYSB(2788);
      if ( ***(_DWORD ***)&SYSB(2788) == 0 )
        RC_4184a0((intptr_t)(SYS32(2788)), (intptr_t)(0));
      v34 = F32(**v33, 4 * (*v65__f));
      if ( (*v53__f) > 0 )
      {
        v35 = 20 * (*v53__f) / 600;
        if ( v35 < 0 )
        {
          v36 = 0;
LABEL_122:
          v37 = SYS32(4 * v36 + 10072);
          goto LABEL_123;
        }
        v37 = SYS32(10148);
        v36 = 20 * (*v53__f) / 600;
        if ( v35 < 20 )
          goto LABEL_122;
LABEL_123:
        W_rp_5bd550((intptr_t)(v37), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((int)v34), (intptr_t)(v34[3] + 208), (intptr_t)((*v60__f) / 100 + v34[4]), (intptr_t)(v34[5]), (intptr_t)(v34[6]), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v34[10]), (intptr_t)(*(intptr_t *)&SYSB(864)));
      }
      v38 = 0;
      (*v62__f) = (uint16_t *)v79;
      do
      {
        v39 = v38;
        (*v71__f) = 4;
        v40 = *(*v62__f);
        (*v70__f) = F32((*v62__f) + (3), 0) / 100;
        v41 = 16;
        do
        {
          v42 = *(_DWORD ***)&SYSB(2792);
          if ( ***(_DWORD ***)&SYSB(2792) == 0 )
            RC_4184a0((intptr_t)(SYS32(2792)), (intptr_t)(0));
          v43 = F32((**v42 + 4 * (uint16_t)v39), 0);
          v44 = *(intptr_t *)(SYS32(10348) + 364);
          if ( v40 > 0 )
          {
            v45 = 20 * v40 / 20;
            if ( v45 >= 0 )
            {
              v46 = 20 * v40 / 20;
              if ( v45 >= 20 )
                goto LABEL_133;
            }
            else
            {
              v46 = 0;
            }
            v47 = SYS32(4 * v46 + 10152);
            if ( v47 == 0 )
            {
LABEL_133:
              W_rp_5b9b70((intptr_t)(v43), (intptr_t)((int)v44), (intptr_t)(v41), (intptr_t)((*v70__f)));
              goto LABEL_135;
            }
            W_rp_5bd550((intptr_t)(v47), (intptr_t)(v44), (intptr_t)(v43), (intptr_t)(v41 + F32(v43, 12)), (intptr_t)((*v70__f) + F32(v43, 16)), (intptr_t)(F32(v43, 20)), (intptr_t)(F32(v43, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v43, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
          }
LABEL_135:
          ++v39;
          v41 += 152;
          --(*v71__f);
        }
        while ( (*v71__f) != 0 );
        v38 = v39;
        (*v62__f) += 6;
      }
      while ( (uint16_t)v39 < 0x18u );
      v1 = (*v73__f);
LABEL_138:
      RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
      v2 = (*v75__f);
    }
    if ( SYS32(0x2884) != 0 )
      RC_5bc0c0((intptr_t)(SYS32(0x2884)));
    if ( SYS32(2728) != 0 )
      RC_56d620((intptr_t)(SYS32(2728)));
    RC_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    tm_566250(F32(c, 0x4), 0);
    if ( RC_43c110((intptr_t)(F32(c, 0x4)), (intptr_t)(v5), (intptr_t)(34)) != 0 )
      break;
    if ( (*v56__f) == 0 )
      goto LABEL_140;
    --(*v56__f);
    if ( (*v58__f) != 0 )
    {
      --(*v58__f);
      goto LABEL_13;
    }
    if ( (*v64__f) != 0 && --(*v64__f) == 0 && SYS32(0x2884) != 0 )
      RC_5bbdb0((intptr_t)(SYS32(0x2884)), (intptr_t)(29), (intptr_t)(5));
    if ( (*v67__f) == 0 )
    {
      v7 = 63;
      v8 = F32(c, 0x4);
      v9 = (_DWORD *)(v8 + 264);
      do
      {
        v10 = (_DWORD *)*v9;
        if ( *(_DWORD *)*v9 != 0 && v10[2] == 1 )
        {
          if ( v5 - v10[1] <= 0x64 )
          {
            v11 = 11;
            F32(F32(v8 + 4 * v7, 12), 0) = 0;
            v12 = F32(c, 0x4);
            F16(v12, 364) = 0;
            v13 = (_DWORD *)(v12 + 276);
            do
            {
              v13[11] = 0;
              *v13++ = 0;
              --v11;
            }
            while ( v11 != 0 );
            v14 = (_DWORD **)(v12 + 12);
            for ( j = 64; j != 0; --j )
            {
              v16 = *v14++;
              *v16 = 0;
            }
            F32(v12, 268) = 0;
            F32(v12, 272) = 0;
            if ( (uint16_t)++(*v68__f) >= 3u )
            {
              (*v67__f) = 1;
              if ( SYS32(0x2884) != 0 )
                RC_5bbdb0((intptr_t)(SYS32(0x2884)), (intptr_t)(29), (intptr_t)(10));
              if ( (uint16_t)(*v56__f) > 0xC8u )
                (*v56__f) = 200;
              (*v54__f) = 2;
              v17 = &(*v78__f);
              for ( k = 6; k != 0; --k )
              {
                v19 = -(F16(v17, 0) != 0);
                v17 += 12;
                *((uint16_t *)v17 - 6) = v19 + 4;
              }
            }
            v1 = (*v73__f);
            break;
          }
          v1 = (*v73__f);
        }
        --v7;
        --v9;
      }
      while ( v7 >= 0 );
    }
    if ( (*v54__f) != 0 )
    {
      if ( (*v54__f) == 1 )
      {
        if ( (*v61__f) < 3200 )
        {
          ++(*v61__f);
          goto LABEL_76;
        }
      }
      else
      {
        if ( (*v54__f) != 2 )
          goto LABEL_76;
        if ( (*v53__f) > 0 )
        {
          if ( (*v67__f) != 0 )
            (*v53__f) = ((*v53__f) - 4) & (((*v53__f) - 4 < 0) - 1);
          else
            --(*v53__f);
          goto LABEL_76;
        }
      }
    }
    else if ( (*v53__f) < 600 )
    {
      ++(*v53__f);
      goto LABEL_76;
    }
    ++(*v54__f);
LABEL_76:
    if ( (*v59__f) >= 6u )
    {
      (*v59__f) = 0;
      (*v63__f) = ((uint16_t)(*v63__f) + 1) % 35;
    }
    else
    {
      ++(*v59__f);
    }
    if ( (*v57__f) >= 6u )
    {
      (*v57__f) = 0;
      (*v65__f) = !(*v65__f);
    }
    else
    {
      ++(*v57__f);
    }
    if ( (*v60__f) > 16000 )
      (*v66__f) = ((*v66__f) - 10) & (((*v66__f) - 10 < 0) - 1);
    v20 = 6;
    (*v60__f) += (*v66__f) / 100;
    v21 = (int *)v80;
    do
    {
      switch ( *((uint16_t *)v21 - 2) )
      {
        case 0:
          if ( F16(v21, 0) != 0 )
          {
            --F16(v21, 0);
          }
          else
          {
            F16(v21, 0) = 0;
            *((uint16_t *)v21 - 2) = 1;
          }
          break;
        case 1:
          if ( F16(v21, 0) >= 0xAu )
          {
            F16(v21, 0) = 0;
            v22 = *((uint16_t *)v21 - 1);
            if ( v22 >= 0x14u )
              *((uint16_t *)v21 - 2) = 2;
            else
              *((uint16_t *)v21 - 1) = v22 + 1;
          }
          else
          {
            ++F16(v21, 0);
          }
          break;
        case 2:
          if ( v21[1] <= 12000 )
            *((uint16_t *)v21 - 2) = 3;
          break;
        case 3:
          if ( F16(v21, 0) >= 0xAu )
          {
            F16(v21, 0) = 0;
            v23 = *((uint16_t *)v21 - 1);
            if ( v23 != 0 )
              *((uint16_t *)v21 - 1) = v23 - 1;
            else
              *((uint16_t *)v21 - 2) = 4;
          }
          else
          {
            ++F16(v21, 0);
          }
          break;
        default:
          break;
      }
      v24 = *((uint16_t *)v21 - 2);
      if ( v24 != 0 && v24 < 4u )
        v21[1] -= 16;
      v21 += 3;
      --v20;
    }
    while ( v20 != 0 );
    v2 = (*v75__f);
  }
  (*v69__f) = 6;
LABEL_140:
  RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
  v48 = SYS32(10068);
  for ( m = 0; m < F16(v48, 4); F32(v50 + F32(v48, 0), 20) = 0 )
  {
    v50 = 28 * m;
    v51 = F32(v50 + F32(v48, 0), 24);
    if ( v51 != 0 )
    {
      tm_40bb70(v51, 1);
      F32(v50 + F32(v48, 0), 24) = 0;
    }
    ++m;
    F32((v50 + F32(v48, 0)), 0) = 0;
    F32(v50 + F32(v48, 0), 4) = 0;
    F32(v50 + F32(v48, 0), 8) = 0;
    F32(v50 + F32(v48, 0), 12) = 0;
    F32(v50 + F32(v48, 0), 16) = 0;
  }
  F16(v48, 6) = 0;
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
  tm_41bbe0(c);
  return (*v69__f);
}

/* 0x40bb70: void *__thiscall sub_40BB70(cls_40BB70 *this, char a2) */
int32_t tm_40bb70(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;

  W_rp_5b9390((intptr_t)(c));
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return c;
}

/* 0x56d710: int __thiscall sub_56D710(cls_41BBE0 *this) */
int32_t tm_56d710(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    void * v2 = 0;
    void * v3 = 0;
    uint16_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    void * v7 = 0;
    intptr_t v8 = 0;
    int v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    char * v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t i = 0;
    intptr_t v16 = 0;
    int32_t * v17 = 0;
    intptr_t j = 0;
    int32_t ** v19 = 0;
    intptr_t k = 0;
    int32_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    void * v25 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const m__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */

  v2 = (void *)F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    RC_56c640((intptr_t)(F32(c, 0x0)));
    g_eng.op_delete((void *)v2);
    F32(c, 0x0) = 0;
  }
  v3 = (void *)F32(c, 0x4);
  if ( v3 != 0 )
  {
    W_input_dtor((intptr_t)(F32(c, 0x4)));
    g_eng.op_delete((void *)v3);
    F32(c, 0x4) = 0;
  }
  v4 = (intptr_t)g_eng.op_new(0x10u);
  if ( v4 != 0 )
  {
    F32(v4, 0) = 0;
    v4[2] = 0;
    v4[3] = 0;
    v5 = (int)v4;
  }
  else
  {
    v5 = 0;
  }
  F32(c, 0x0) = v5;
  v6 = SYS32(10020);
  F32(v5, 8) = SYS32(10016);
  F32(v5, 12) = v6;
  F16(v5, 6) = 0;
  F16(v5, 4) = 12;
  v7 = (intptr_t)g_eng.op_new(0x30u);
  v8 = 0;
  v9 = F16(v5, 4) == 0;
  F32(v5, 0) = v7;
  if ( !v9 )
  {
    do
    {
      v10 = (intptr_t)g_eng.op_new(0x84u);
      if ( v10 != 0 )
      {
        v10[18] = 0;
        v10[1] = 0;
      }
      else
      {
        v10 = 0;
      }
      v11 = (uint16_t)v8++;
      F32((F32(v5, 0) + 4 * v11), 0) = v10;
    }
    while ( (uint16_t)v8 < F16(v5, 4) );
  }
  v12 = (char *)(intptr_t)g_eng.op_new(0x170u);
  v13 = (int)v12;
  if ( v12 != 0 )
  {
    v14 = v12 + 12;
    for ( i = 64; i != 0; --i )
      *v14++ = (intptr_t)g_eng.op_new(0xCu);
    v16 = v13;
  }
  else
  {
    v16 = 0;
  }
  F32(c, 0x4) = v16;
  F32(v16, 0) = 1;
  F32(v16, 4) = 1;
  F32(v16, 8) = 1;
  F16(v16, 364) = 0;
  v17 = (_DWORD *)(v16 + 276);
  for ( j = 11; j != 0; --j )
  {
    v17[11] = 0;
    *v17++ = 0;
  }
  v19 = (_DWORD **)(v16 + 12);
  for ( k = 64; k != 0; --k )
  {
    v21 = *v19++;
    *v21 = 0;
  }
  F32(v16, 268) = 0;
  F32(v16, 272) = 0;
  v22 = SYS32(10068);
  v23 = 0;
  for ( (*m__f) = 0; (uint16_t)v23 < F16(v22, 4); F32(F32(v22, 0) + v24, 20) = 0 )
  {
    v24 = 28 * (uint16_t)v23;
    v25 = F32(F32(v22, 0), v24 + 24);
    if ( v25 != 0 )
    {
      W_rp_5b9390((intptr_t)(F32(F32(v22, 0), v24 + 24)));
      g_eng.op_delete((void *)v25);
      v23 = (*m__f);
      F32(F32(v22, 0) + v24, 24) = 0;
    }
    (*m__f) = ++v23;
    F32((F32(v22, 0) + v24), 0) = 0;
    F32(F32(v22, 0) + v24, 4) = 0;
    F32(F32(v22, 0) + v24, 8) = 0;
    F32(F32(v22, 0) + v24, 12) = 0;
    F32(F32(v22, 0) + v24, 16) = 0;
  }
  F16(v22, 6) = 0;
  return RC_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
}

/* 0x56de80: int __thiscall sub_56DE80(input_dev **this) */
int32_t tm_56de80(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t * v4 = 0;
    intptr_t result = 0;

  v2 = 0;
  v3 = 2;
  v4 = c + 4 * (2591);
  do
  {
    if ( *v4 == 0 )
    {
      result = RC_5ba120((intptr_t)(*(((intptr_t *)c) + 2588)), (intptr_t)(v4), (intptr_t)((int)*(((intptr_t *)c) + 2595)), (intptr_t)(v2));
      if ( *v4 != 0 )
        result = RC_5ba290((intptr_t)(*v4));
    }
    ++v2;
    ++v4;
    --v3;
  }
  while ( v3 != 0 );
  return result;
}

/* 0x59ec30: void __thiscall sub_59EC30(cls_59EC30 *this, int a2, int a3, int a4) */
void tm_59ec30(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    void ** v11 = 0;
    intptr_t i = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    void ** v15 = 0;
    uint16_t * v16 = 0;
    void * v17 = 0;
    void * v18 = 0;
    void ** v19 = 0;
    intptr_t j = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const Block__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  v4 = c;
  v5 = 0;
  v6 = 0;
  F32(Block__f, 0x0) = 0;
  F32(Block__f, 0x4) = 0;
  (*v25__f) = 0;
  while ( 1 )
  {
    while ( 1 )
    {
      v26 = 1;
      v8 = (intptr_t)g_eng.op_new(0x18u);
      if ( v8 != 0 )
      {
        *(_DWORD *)&GAP(v8, 0x0, 4) = 0;
        *(_DWORD *)&GAP(v8, 0x0, 8) = 0;
        *(_DWORD *)&GAP(v8, 0x0, 12) = 0;
        F32(((uint8_t *)(v8) + 0x0), 0) = 0;
        F32(v8, 0x10) = 0;
        F32(v8, 0x14) = 0;
      }
      else
      {
        v8 = 0;
      }
      F32(v4, 0x0) = (int)v8;
      v9 = W_scene_loop((intptr_t)(v8), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(v6), (intptr_t)(&(*v23__f)), (intptr_t)(a4));
      v10 = F32(v4, 0x0);
      (*v22__f) = v9;
      if ( F32(v4, 0x0) != 0 )
      {
        if ( F32(v10, 0) != 0 )
        {
          RC_5a4320((intptr_t)(F32(v10, 0)), (intptr_t)(1));
          F32(v10, 0) = 0;
        }
        v11 = (void **)(v10 + 4);
        for ( i = 3; i != 0; --i )
        {
          if ( *v11 != 0 )
          {
            g_eng.op_delete((void *)*v11);
            *v11 = 0;
          }
          ++v11;
        }
        API_OutputDebugStringA((intptr_t)(((char *)D(0x8a0bfc))));
        W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(2u));
        v13 = F32(v10, 16);
        if ( v13 != 0 )
        {
          RC_5a4440((intptr_t)(v13), (intptr_t)(1));
          F32(v10, 16) = 0;
        }
        if ( F32(v10, 20) != 0 )
        {
          g_eng.op_delete((void *)F32(v10, 20));
          F32(v10, 20) = 0;
        }
        g_eng.op_delete((void *)(void *)v10);
        v4 = c;
        F32(c, 0x0) = 0;
      }
      v6 = 0;
      if ( (*v22__f) != 4 )
      {
        if ( (*v22__f) != 5 )
          goto LABEL_25;
        if ( (*v23__f) == 1 )
          v26 = 0;
      }
      v14 = tm_59ee50(&(*Block__f), (*v23__f), v26) - 22;
      if ( v14 != 0 )
        break;
      a2 = 10020;
      v6 = 1;
    }
    if ( v14 != 1 )
      break;
    a2 = 10030;
  }
LABEL_25:
  v15 = (void **)F32(Block__f, 0x0);
  (*v25__f) = -1;
  if ( F32(Block__f, 0x0) != 0 )
  {
    v16 = (uint16_t *)(F32(Block__f, 0x0) + 4);
    v27 = F32(Block__f, 0x0) + 4;
    if ( F16(F32(Block__f, 0x0), 4) != 0 )
    {
      do
      {
        v17 = *((void **)*v15 + v5);
        if ( v17 != 0 )
        {
          W_win_ctrls_free((intptr_t)(*((intptr_t *)*v15 + v5)));
          g_eng.op_delete((void *)v17);
          v16 = (uint16_t *)v27;
          *((_DWORD *)*v15 + v5) = 0;
        }
        ++v5;
      }
      while ( v5 < *v16 );
    }
    if ( *v15 != 0 )
    {
      g_eng.op_delete((void *)*v15);
      *v15 = 0;
    }
    g_eng.op_delete((void *)v15);
    F32(Block__f, 0x0) = 0;
  }
  if ( F32(Block__f, 0x4) != 0 )
  {
    v18 = (void *)F32(Block__f, 0x4);
    v19 = (void **)(F32(Block__f, 0x4) + 12);
    for ( j = 64; j != 0; --j )
    {
      if ( *v19 != 0 )
      {
        g_eng.op_delete((void *)*v19);
        *v19 = 0;
      }
      ++v19;
    }
    g_eng.op_delete((void *)v18);
  }
}

/* 0x59ee50: int __thiscall sub_59EE50(cls_41BBE0 *this, int a2, int a3) */
int32_t tm_59ee50(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint32_t v5 = 0;
    uint32_t v6 = 0;
    uint32_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t *** v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    _Alignas(8) uint8_t frame_[0x68 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v25 = (int16_t *)(frame_ + 0x14);   /* [4] at ebp-0x54 */
    uint8_t *v26 = (uint8_t *)(frame_ + 0x1c);   /* [36] at ebp-0x4c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x68 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x64 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x60 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x5c */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x58 */
    uint32_t *const v27__f = (uint32_t *)(frame_ + 0x40);   /* ebp-0x28 */
    uint32_t *const v28__f = (uint32_t *)(frame_ + 0x44);   /* ebp-0x24 */
    uint32_t *const v29__f = (uint32_t *)(frame_ + 0x48);   /* ebp-0x20 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x1c */
    uint32_t *const TickCount__f = (uint32_t *)(frame_ + 0x54);   /* ebp-0x14 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x10 */

  v3 = 0;
  (*v24__f) = 0;
  (*v22__f) = 0;
  (*v20__f) = 0;
  (*v21__f) = 0;
  (*v23__f) = 0;
  tm_56d710(c);
  (*v27__f) = 0;
  (*v28__f) = 17;
  (*v29__f) = 0;
  (*v30__f) = 0;
  (*v32__f) = 0;
  (*TickCount__f) = API_GetTickCount();
  while ( 1 )
  {
    do
    {
      while ( 1 )
      {
        v5 = API_GetTickCount();
        switch ( (*v30__f) )
        {
          case 0:
            (*v27__f) = v5;
            RC_5b1030();
LABEL_14:
            (*v30__f) = 2;
            break;
          case 1:
            v6 = v5 + (*v28__f) - (*v27__f);
            if ( v6 > 0x64 )
              v6 = 100;
            v7 = v6;
            (*v27__f) = v5;
            (*v28__f) = v6;
            RC_5b1030();
            if ( v7 > 0x10 )
              goto LABEL_14;
            break;
          case 2:
            if ( v5 - (*v29__f) <= (*v28__f) )
            {
              (*v28__f) -= 16;
              if ( (*v28__f) <= 0x10 )
                (*v30__f) = 1;
            }
            else
            {
              (*v28__f) = 0;
              (*v30__f) = 1;
            }
            break;
          default:
            break;
        }
        if ( (*v30__f) == 1 )
        {
          if ( v5 - (*TickCount__f) <= 0x3E8 )
          {
            ++(*v32__f);
          }
          else
          {
            (*v32__f) = 0;
            (*TickCount__f) = v5;
          }
        }
        else if ( (*v30__f) == 2 )
        {
          (*v29__f) = v5;
        }
        if ( (*v30__f) != 1 )
          break;
        if ( (*v20__f) < 1000 )
          RC_5b9410((intptr_t)(F32(SYS32(10348), 364)));
        v15 = SYS32(2800);
        if ( ***(_DWORD ***)&SYSB(2800) == 0 )
          RC_4184a0((intptr_t)(SYS32(2800)), (intptr_t)(0));
        v16 = *(intptr_t *)(SYS32(10348) + 364);
        v17 = ***v15;
        if ( (*v20__f) > 0 )
        {
          v18 = W_tint_fade_a((intptr_t)((*v20__f)), (intptr_t)(1000));
          if ( v18 != 0 )
            W_rd_494e10((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)), (intptr_t)(v16), (intptr_t)(v17), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(0), (intptr_t)(*(_DWORD *)&GAP(v17, 0x14, 20)));
          else
            W_rp_5b9b70((intptr_t)(v17), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(0), (intptr_t)(0));
        }
        W_rd_48c820((intptr_t)(F32(c, 0x0)), (intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)(*(intptr_t *)&SYSB(864)));
        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
      }
    }
    while ( (*v30__f) != 2 );
    W_timed_sounds_tick((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    RC_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
    v8 = 0;
    tm_566250(F32(c, 0x4), 0);
    if ( RC_43c110((intptr_t)(F32(c, 0x4)), (intptr_t)(v5), (intptr_t)(34)) != 0 )
      break;
    if ( (*v22__f) != 0 )
    {
      if ( (*v22__f) == 1 )
      {
        if ( v3 == 0 )
        {
          if ( a3 != 0 )
            ++(*v21__f);
          if ( a2 != 0 )
            ++(*v21__f);
          v10 = F32(c, 0x0);
          W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v26), (intptr_t)(0));
          (*v23__f) = W_win_mgr_open((intptr_t)(v10), (intptr_t)(336), (intptr_t)(352), (intptr_t)(288), (intptr_t)(28 * (uint16_t)(*v21__f) + 48), (intptr_t)(2u), (intptr_t)(v26));
          v11 = W_win_ctrl_take((intptr_t)((*v23__f)));
          v3 = v11;
          if ( v11 != 0 )
            W_ctrl_grid_init((intptr_t)(v11), (intptr_t)(40), (intptr_t)(32), (intptr_t)((uint16_t)(*v21__f)), (intptr_t)(1), (intptr_t)((uint16_t)(*v21__f)), (intptr_t)(0));
          v25[0] = 0;
          v25[1] = 1;
          v25[2] = 2;
          v25[3] = 3;
          W_ctrl_cursor_set((intptr_t)(v3), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v25), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
          if ( F32(CFG(), 596) == 0 )
          {
            F32(((uint8_t *)(v3) + 0x14), 0) = 1;
            *(_DWORD *)&GAP(v3, 0x14, 4) = -9;
          }
          if ( a3 != 0 )
          {
            v12 = W_grid_row_add((intptr_t)(v3), (intptr_t)(0), (intptr_t)(22));
            W_grid_cell_text((intptr_t)(v3), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3ef0))));
          }
          if ( a2 != 0 )
          {
            v13 = W_grid_row_add((intptr_t)(v3), (intptr_t)(0), (intptr_t)(23));
            W_grid_cell_text((intptr_t)(v3), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3ed0))));
          }
        }
        v8 = 0;
        if ( W_ctrl_input((intptr_t)(F32(c, 0x4)), (intptr_t)(v3), (intptr_t)(v5), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0)) == 3 )
        {
          (*v22__f) = 2;
          (*v24__f) = F32(16 * F32((F32(v3, 0x174) + 20), 0) + F32(v3, 0x17c), 4);
        }
      }
      else
      {
        if ( v3 != 0 )
        {
          v9 = (*v23__f);
          (*v23__f) = 0;
          v3 = 0;
          *(_DWORD *)((uint8_t *)(v9 + (1) * 0x50) + 0x0) = 0;
        }
        if ( (*v20__f) <= 0 )
          goto LABEL_63;
        (*v20__f) = ((*v20__f) - 20) & (((*v20__f) - 20 < 0) - 1);
      }
    }
    else if ( (*v20__f) >= 1000 )
    {
      (*v22__f) = 1;
    }
    else if ( (*v20__f) + 20 <= 1000 )
    {
      (*v20__f) += 20;
    }
    else
    {
      (*v20__f) = 1000;
    }
    tm_56c930(F32(c, 0x0));
    v14 = (_DWORD *)F32(c, 0x0);
    if ( F16(F32(c, 0x0), 6) != 0 )
    {
      do
      {
        W_win_tick((intptr_t)(F32(*v14, 4 * v8)));
        v14 = (_DWORD *)F32(c, 0x0);
        ++v8;
      }
      while ( v8 < F16(F32(c, 0x0), 6) );
    }
  }
  (*v24__f) = 6;
LABEL_63:
  tm_56dc20(0, 0, 0);
  W_frame_cache_clear((intptr_t)(SYS32(10068)));
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
  tm_41bbe0(c);
  return (*v24__f);
}

/* 0x41d780: int __thiscall sub_41D780(cls_41D780 *this, int a2, int a3, int a4) */
int32_t tm_41d780(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    uint16_t v17 = 0;
    int32_t * v18 = 0;
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
    int32_t * v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t result = 0;
    int32_t * v48 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v45__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  v4 = c;
  v5 = F32(c, 0xaf0);
  (*v46__f) = v4;
  v6 = &GAP(v4, 0x0, 2576);
  if ( a3 > v5 && a3 - v5 > 0 )
  {
    (*v47__f) = a3 - v5;
    do
    {
      v7 = F32(v6, 4 * 54);
      if ( v7 <= 0 )
      {
        if ( a2 == 0 )
        {
          if ( *(_DWORD *)&GAP(v4, 0x0, 8) == 0 )
          {
            *(_DWORD *)&GAP(v4, 0x0, 8) = 1;
            qmemcpy(&GAP(v4, 0x0, 12), v6, 0x4DCu);
          }
          RC_55fdb0((intptr_t)(v4));
        }
        v10 = F32(v6, 4 * 56);
        F32(v6, 4 * 58) = 0;
        ++v10;
        v11 = 0;
        F32(v6, 4 * 56) = v10;
        v12 = v10;
        while ( 1 )
        {
          v13 = ((int32_t *)D(0x6b6ea8))[13 * v11];
          if ( v13 == 0 )
            break;
          if ( v13 == *(_DWORD *)&GAP(v4, 0x0, 2544) )
          {
            if ( 52 * v11 != -7040680 )
            {
              v14 = ((int32_t *)D(0x6b6eac))[13 * v11];
              goto LABEL_19;
            }
            break;
          }
          ++v11;
        }
        v14 = *(_DWORD *)&GAP(v4, 0x0, 4);
LABEL_19:
        v15 = 1;
        v16 = v12 + F32(v6, 4 * 54);
        v17 = 0;
        while ( v15 != v14 || ((int32_t *)D(0x67ac5c))[55 * v17] != v16 && v16 != 0 )
        {
          v15 = ((int32_t *)D(0x67ac58))[55 * ++v17];
          if ( v15 == 0 )
          {
            v48 = (int32_t *)D(0x67ac58);
            v18 = (int32_t *)D(0x67ac58);
            goto LABEL_25;
          }
        }
        v18 = (int *)(220 * v17 + 6794328);
        v48 = v18;
LABEL_25:
        if ( v18 != 0 )
        {
          v19 = v18[2];
          v20 = F32(v6, 4 * 22);
          (*v45__f) = 0;
          if ( v20 < v19 )
          {
            v22 = v19 - v20;
            v23 = 250 * v22 / 1000;
            v24 = v22 - v23 + 2;
            if ( v24 > 0 )
              v24 = 100 * v24 * rnd() / 0x8000 % v24;
            v18 = v48;
            v21 = v20 + v23 + v24;
          }
          else
          {
            v21 = F32(v6, 4 * 22);
          }
          v25 = F32(v6, 4 * 24);
          F32(v6, 4 * 22) = v21;
          v26 = v18[3];
          if ( v25 < v26 )
          {
            v28 = v26 - v25;
            v29 = 250 * v28 / 1000;
            v30 = v28 - v29 + 2;
            if ( v30 > 0 )
              v30 = 100 * v30 * rnd() / 0x8000 % v30;
            v27 = v25 + v29 + v30;
          }
          else
          {
            v27 = v25;
          }
          v31 = F32(v6, 4 * 25);
          F32(v6, 4 * 24) = v27;
          v32 = v48[4];
          if ( v31 < v32 )
          {
            v34 = v32 - v31;
            v35 = 250 * v34 / 1000;
            v36 = v34 - v35 + 2;
            if ( v36 > 0 )
              v36 = 100 * v36 * rnd() / 0x8000 % v36;
            v33 = v31 + v36 + v35;
          }
          else
          {
            v33 = v31;
          }
          v37 = F32(v6, 4 * 26);
          F32(v6, 4 * 25) = v33;
          v38 = RC_41db30((intptr_t)(v37), (intptr_t)(v48[5]));
          v39 = F32(v6, 4 * 27);
          F32(v6, 4 * 26) = v38;
          F32(v6, 4 * 27) = RC_41db30((intptr_t)(v39), (intptr_t)(v48[6]));
          F32(v6, 4 * 28) = RC_41db30((intptr_t)(F32(v6, 4 * 28)), (intptr_t)(v48[7]));
          F32(v6, 4 * 59) = v48[8];
          v40 = (int32_t *)D(0x6907c0);
          do
          {
            if ( RC_41dba0((intptr_t)((*v46__f)), (intptr_t)(*v48), (intptr_t)(v48[1]), (intptr_t)(v40)) )
              W_sc_406c70((intptr_t)((int)v6), (intptr_t)(((int32_t *)D(0x6907c4))[4 * (*v45__f)]), (intptr_t)(255), (intptr_t)(0), (intptr_t)(1));
            v41 = 4 * ++(*v45__f);
            v42 = ((int32_t *)D(0x6907c0))[v41];
            v40 = &((int32_t *)D(0x6907c0))[v41];
          }
          while ( v42 != 0 );
          v43 = *(intptr_t *)&GAP((*v46__f), 0x0, 2548);
          if ( v43 != 0 )
            W_char_skills_refresh((intptr_t)(v43), (intptr_t)(0));
          v4 = (*v46__f);
        }
        qmemcpy(&GAP(v4, 0x0, 1256), v6, 0x4DCu);
      }
      else
      {
        v8 = F32(v6, 4 * 58);
        F32(v6, 4 * 54) = v7 - 1;
        v9 = F32(v6, 4 * 59) - 1;
        if ( v9 >= v8 )
          v9 = v8;
        F32(v6, 4 * 58) = v9;
        ++F32(v6, 4 * 56);
      }
      --(*v47__f);
    }
    while ( (*v47__f) != 0 );
  }
  result = a4;
  if ( a4 > F32(v6, 4 * 55) )
    F32(v6, 4 * 55) = a4;
  return result;
}

/* 0x49a6e0: void __thiscall sub_49A6E0(cls_49A6E0 *this) */
void tm_49a6e0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    void ** v2 = 0;

  v2 = (void **)F32(c, 0xc);
  if ( v2 != 0 )
  {
    g_eng.op_delete((void *)*v2);
    g_eng.op_delete((void *)v2);
    F32(c, 0xc) = 0;
  }
  if ( F32(c, 0x0) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  if ( F32(c, 0x8) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x8));
    F32(c, 0x8) = 0;
  }
}
/* unported callees: 0x40a5d0 0x40fe00 0x4184a0 0x41db30 0x41dba0 0x43c110 0x468b10 0x49bf90 0x55fdb0 0x560900 0x563ef0 0x5640b0 0x564110 0x5646c0 0x565b00 0x566300 0x568780 0x569220 0x56a670 0x56c430 0x56c640 0x56d620 0x56dd80 0x56e190 0x5749b0 0x5752e0 0x579a00 0x579bd0 0x57a330 0x57b280 0x57ca40 0x582e90 0x583c90 0x583ee0 0x583fe0 0x59e230 0x59e5d0 0x5a4320 0x5a4440 0x5a4760 0x5aff00 0x5b1030 0x5b5dc0 0x5b6060 0x5b60d0 0x5b60e0 0x5b6250 0x5b6410 0x5b64b0 0x5b6580 0x5b65a0 0x5b65b0 0x5b6990 0x5b6db0 0x5b7ee0 0x5b89d0 0x5b8fc0 0x5b9410 0x5b9cf0 0x5b9fc0 0x5ba120 0x5ba290 0x5ba2d0 0x5ba390 0x5ba3a0 0x5ba3d0 0x5ba4c0 0x5ba650 0x5baed0 0x5bb3a0 0x5bbb10 0x5bbc20 0x5bbc60 0x5bbc90 0x5bbcd0 0x5bbdb0 0x5bbe20 0x5bbeb0 0x5bc0c0 0x5bcb30 0x5bcb80 0x5bd3b0 0x5bd3d0 0x5bd4d0 */
