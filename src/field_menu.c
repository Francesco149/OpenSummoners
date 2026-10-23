/* field_menu.c — the room loop's MENUS: the main menu 0x591120 (Items / Special Items / Status / Abilities / Party
 * Commands / Messages / System) with its Messages (0x597aa0: the list, 0x5985f0 one message, 0x5984c0 the line clip)
 * and System (0x596d60: Options, the two data screens, Give Up / Return to Title) screens, the party / item hotkey
 * menus 0x590480 / 0x590f20 / 0x590ca0, the menu context 0x59d540 and the scene runner 0x59ad60.  See field_menu.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals).  Hand port: 0x5984c0 (IDA reuses its first
 * argument's stack slot as a 4-byte character buffer).  Retail's own (RC_: PORT-DEBT(engine-call-slots)) — the
 * submenus the corpus does not open: Status 0x592190, Items 0x593f60, Abilities 0x596b50, Party Commands 0x598d00,
 * the level-up screen 0x5915c0, the Options pages 0x569220 / 0x56c640 / 0x56ad50 / 0x584400 / 0x584420, the data
 * screens 0x5974b0 / 0x597670, the Yes / No box 0x4190d0; the input platform 0x5ba2d0 / 0x5ba4c0 / 0x5ba650. */
#include "field_menu.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))

#include "event.h"
#include "gimmick.h"
#include "party_member.h"
#include "render_draw.h"
#include "room_load.h"
#include "room_loop.h"
#include "room_tick.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "sound_fx.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "ending.h"
#include "event.h"
#include "field_menu.h"
#include "gimmick.h"
#include "input_dev.h"
#include "menu_helpers.h"
#include "menu_screens.h"
#include "party_member.h"
#include "render_draw.h"
#include "room_load.h"
#include "room_loop.h"
#include "room_tick.h"
#include "scene_loop.h"
#include "script_engine.h"
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

static inline intptr_t W_cell_icon_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)cell_icon_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (const uint16_t *)a5, (uint16_t)a6, (uint16_t)a7); }
static inline intptr_t W_char_follow_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_follow_set((wl_char *)a0, (int32_t)a1, (wl_char *)a2, (int32_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_keys_expand(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)ctrl_keys_expand((uint8_t *)a0, (const char *)a1, (char *)a2); }
static inline intptr_t W_ctrl_text_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ctrl_text_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_ctrl_text_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_text_set((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_dramatist_row(intptr_t a0, ...) { return (intptr_t)dramatist_row((int32_t)a0); }
static inline intptr_t W_en_49bf90(intptr_t a0, intptr_t a1, ...) { return (intptr_t)en_49bf90((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_en_56c640(intptr_t a0, ...) { en_56c640((wl_char *)a0); return 0; }
static inline intptr_t W_flag_add(intptr_t a0, intptr_t a1, ...) { return (intptr_t)flag_add((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_flag_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)flag_find((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_fm_5984c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)fm_5984c0((char *)a0, (char *)a1, (int32_t)a2, (char *)a3); }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_col_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)grid_col_place((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_grid_row_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)grid_row_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_grid_row_reset(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_row_reset((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_grid_row_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)grid_row_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); }
static inline intptr_t W_icon_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)icon_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5); }
static inline intptr_t W_id_5ba2d0(intptr_t a0, ...) { return (intptr_t)id_5ba2d0((wl_char *)a0); }
static inline intptr_t W_id_5ba4c0(intptr_t a0, ...) { return (intptr_t)id_5ba4c0((wl_char *)a0); }
static inline intptr_t W_id_5ba650(intptr_t a0, ...) { return (intptr_t)id_5ba650((wl_char *)a0); }
static inline intptr_t W_menu_party_close(intptr_t a0, ...) { return (intptr_t)menu_party_close((uint8_t *)a0); }
static inline intptr_t W_menu_party_fill(intptr_t a0, ...) { menu_party_fill((uint8_t *)a0); return 0; }
static inline intptr_t W_menu_run(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)menu_run((uint8_t *)a0, (uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_mh_4190d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)mh_4190d0((wl_char *)a0, (char *)a1, (uint16_t)a2, (int32_t *)a3, (int32_t)a4); }
static inline intptr_t W_mh_4e6870(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)mh_4e6870((wl_char *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_mm_5915c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)mm_5915c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_mm_592190(intptr_t a0, intptr_t a1, ...) { return (intptr_t)mm_592190((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_mm_593f60(intptr_t a0, intptr_t a1, ...) { return (intptr_t)mm_593f60((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_mm_596b50(intptr_t a0, ...) { return (intptr_t)mm_596b50((wl_char *)a0); }
static inline intptr_t W_mm_597480(void) { return (intptr_t)mm_597480(); }
static inline intptr_t W_mm_5974b0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)mm_5974b0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_mm_597670(intptr_t a0, intptr_t a1, ...) { return (intptr_t)mm_597670((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_mm_598d00(intptr_t a0, ...) { return (intptr_t)mm_598d00((wl_char *)a0); }
static inline intptr_t W_msg_pages(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)msg_pages((uint8_t *)a0, (const char *)a1, (const char *)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (uint16_t)a7, (uint16_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_msg_window_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)msg_window_open((uint8_t *)a0, (const char *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_quest_count(intptr_t a0, ...) { return (intptr_t)quest_count((const wl_room *)a0); }
static inline intptr_t W_rd_4961e0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_4961e0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rd_496240(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_496240((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_formation(intptr_t a0, ...) { return (intptr_t)room_formation((wl_room *)a0); }
static inline intptr_t W_room_input_flush(intptr_t a0, ...) { room_input_flush((wl_room *)a0); return 0; }
static inline intptr_t W_room_item_pick(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)room_item_pick((wl_room *)a0, (wl_pslot **)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_room_party_count(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_party_count((const wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_script_engine(intptr_t a0, ...) { return (intptr_t)script_engine((wl_event *)a0); }
static inline intptr_t W_sfx_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sfx_play((void *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_sys_cursor_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_cursor_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_frame_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_frame_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_list_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)sys_list_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (uint16_t)a7, (uint16_t)a8, (uint8_t **)a9, (uint8_t **)a10, (const char *)a11, (int32_t)a12); }
static inline intptr_t W_sys_msg_keys(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_keys((void *)a0, (int32_t)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_msg_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_unload(intptr_t a0, intptr_t a1, ...) { sys_unload((void *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_sys_win_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, ...) { return (intptr_t)sys_win_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const char *)a7, (int32_t)a8); }
static inline intptr_t W_text_layout_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)text_layout_set((uint8_t *)a0, (const char *)a1, (uint16_t)a2); }
static inline intptr_t W_text_metrics_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)text_metrics_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_tm_419900(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_419900((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_560a00(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_560a00((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_tm_5644a0(intptr_t a0, ...) { return (intptr_t)tm_5644a0((wl_char *)a0); }
static inline intptr_t W_tm_564780(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_564780((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_tm_565d10(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)tm_565d10((wl_char *)a0, (int32_t *)a1, (int16_t)a2, (int32_t *)a3, (int32_t)a4, (uint32_t)a5); }
static inline intptr_t W_tm_56a220(intptr_t a0, ...) { return (intptr_t)tm_56a220((wl_char *)a0); }
static inline intptr_t W_tm_56da40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)tm_56da40((wl_char *)a0, (char *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_tm_56daa0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)tm_56daa0((wl_char *)a0, (int32_t)a1, (int16_t)a2, (int32_t)a3, (int32_t)a4, (int16_t)a5, (int32_t)a6, (int32_t)a7, (int16_t *)a8, (uint16_t)a9, (int16_t)a10); }
static inline intptr_t W_ts_569220(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ts_569220((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_ts_56ad50(intptr_t a0, ...) { ts_56ad50((wl_char *)a0); return 0; }
static inline intptr_t W_ts_584400(intptr_t a0, ...) { return (intptr_t)ts_584400((wl_char *)a0); }
static inline intptr_t W_ts_584420(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)ts_584420((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_ui_ref_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)ui_ref_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_vpool_load(intptr_t a0, ...) { return (intptr_t)vpool_load((wl_vpool *)a0); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_grid_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_grid_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_grid_new(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_grid_new((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_win_mgr_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_mgr_open((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (const void *)a6); }
static inline intptr_t W_win_title_add(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_title_add((uint8_t *)a0, (const char *)a1); }

typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
int32_t fm_591120(wl_char *self);
int32_t fm_591590(wl_char *self, int32_t * a2, int32_t * a3);
int32_t fm_596d60(wl_char *self);
int32_t fm_597aa0(wl_char *self, int32_t * a2);
void fm_598460(wl_char *self, int32_t a2);
int32_t fm_5985f0(wl_char *self, int32_t a2, int32_t a3);
int32_t fm_56c430(wl_char *self, char a2);
int32_t fm_590480(wl_char *self);
int32_t fm_590f20(wl_char *self, int32_t a2);
int32_t fm_590ca0(wl_char *self);
int32_t fm_59d540(wl_char *self, int32_t a2, int32_t a3);
int32_t fm_59ad60(wl_char *self, int32_t a2);

/* 0x591120: int __thiscall sub_591120(cls_412590 *this) */
int32_t fm_591120(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t result = 0;
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
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    int32_t * v25 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v31 = (uint8_t *)(frame_ + 0x14);   /* [36] at ebp-0x24 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x30 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x28 */

  v2 = 0;
  (*v27__f) = 0;
  (*v26__f) = 1;
  v3 = F32(c, 0xc);
  (*v28__f) = 0;
  do
  {
    v4 = F32(v3 + 4 * v2, 16432);
    if ( W_pslot_ok((intptr_t)(v4), (intptr_t)(0), (intptr_t)(0)) && fm_591590(v4, &(*v30__f), &(*v29__f)) != 0 )
    {
      result = W_mm_5915c0((intptr_t)(c), (intptr_t)((*v30__f)), (intptr_t)((*v29__f)), (intptr_t)(v4));
      F32(v4, 8) = 0;
      if ( result != 0 && result != 8 )
        goto LABEL_9;
      (*v28__f) = 12;
    }
    ++v2;
  }
  while ( v2 < 8u );
  result = (*v28__f);
LABEL_9:
  if ( result == 0 || result == 12 )
  {
    v6 = 28 * (W_room_party_count((intptr_t)(F32(c, 0xc)), (intptr_t)(0), (intptr_t)(0)) + 1);
    v7 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(352), (intptr_t)(8), (intptr_t)(288), (intptr_t)(v6 + 104), (intptr_t)(2u), (intptr_t)(((char *)D(0x854a0c))), (intptr_t)(1));
    F32(c, 0x28) = (int)v7;
    v8 = W_win_grid_new((intptr_t)(v7), (intptr_t)(0), (intptr_t)(40), (intptr_t)(9), (intptr_t)(6), (intptr_t)(8), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(88), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(1), (intptr_t)(96), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(2), (intptr_t)(128), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(3), (intptr_t)(160), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(4), (intptr_t)(192), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(5), (intptr_t)(224), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v9 = W_win_grid_new((intptr_t)(F32(c, 0x28)), (intptr_t)(0), (intptr_t)(v6 + 56), (intptr_t)(1), (intptr_t)(3), (intptr_t)(1), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(88), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v9), (intptr_t)(1), (intptr_t)(96), (intptr_t)(0), (intptr_t)(80), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v9), (intptr_t)(2), (intptr_t)(176), (intptr_t)(0), (intptr_t)(96), (intptr_t)(1));
    W_grid_row_add((intptr_t)(v9), (intptr_t)(0), (intptr_t)(0));
    W_menu_party_fill((intptr_t)(c));
    v10 = F32(c, 0x0);
    v11 = 0;
    v12 = 2;
    v13 = 24;
    if ( ((char *)D(0x8a2f04)) != 0 )
    {
      v13 = 56;
      v11 = 1;
      v12 = 3;
    }
    W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v31), (intptr_t)(v11));
    v14 = W_win_mgr_open((intptr_t)(v10), (intptr_t)(16), (intptr_t)(8), (intptr_t)(176), (intptr_t)(276), (intptr_t)(v12), (intptr_t)(v31));
    v15 = v14;
    (*v30__f) = v14;
    if ( ((char *)D(0x8a2f04)) != 0 )
      W_win_title_add((intptr_t)(v14), (intptr_t)(((char *)D(0x8a2f04))));
    v16 = W_win_grid_new((intptr_t)(v15), (intptr_t)(40), (intptr_t)(v13), (intptr_t)(8), (intptr_t)(2), (intptr_t)(8), (intptr_t)(0));
    W_grid_col_place((intptr_t)(v16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v16), (intptr_t)(1));
    F32(F32((v16 + 376), 0), 36) = 72;
    F32(F32((v16 + 376), 0), 40) = -10;
    F32(F32((v16 + 376), 0), 44) = 0;
    F32(F32((v16 + 376), 0), 48) = 0;
    v17 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(1));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v17), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2efc))));
    v18 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(2));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2eec))));
    v19 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(0));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2ee4))));
    v20 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(3));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v20), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2ed8))));
    v21 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(4));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v21), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2ec8))));
    v22 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(5));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2ebc))));
    v23 = W_grid_row_add((intptr_t)(v16), (intptr_t)(0), (intptr_t)(6));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v23), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2eb4))));
    while ( 1 )
    {
      if ( (*v26__f) != 0 )
      {
        (*v26__f) = 0;
        if ( W_quest_count((intptr_t)(F32(c, 0xc))) != 0 )
          W_cell_icon_set((intptr_t)(v16), (intptr_t)(v22), (intptr_t)(1), (intptr_t)(SYS32(2928)), (intptr_t)(74), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        else
          W_cell_icon_set((intptr_t)(v16), (intptr_t)(v22), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      }
      v24 = W_menu_run((intptr_t)(c), (intptr_t)((_DWORD *)v16), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( v24 == 11 )
        break;
      if ( v24 == 12 )
      {
        switch ( F32(16 * F32((F32(v16, 372) + 20), 0) + F32((v16 + 380), 0), 4) )
        {
          case 0:
            v24 = W_mm_592190((intptr_t)(c), (intptr_t)(F32(c, 0xc)));
            break;
          case 1:
            v24 = W_mm_593f60((intptr_t)(c), (intptr_t)(1));
            break;
          case 2:
            v24 = W_mm_593f60((intptr_t)(c), (intptr_t)(2));
            break;
          case 3:
            v24 = W_mm_596b50((intptr_t)(c));
            break;
          case 4:
            v24 = W_mm_598d00((intptr_t)(c));
            break;
          case 5:
            if ( *(_DWORD *)&GAP(W, 0x1030, 44) == 1010 || *(_DWORD *)&GAP(W, 0x1030, 44) == 1020 )
              v25 = (int32_t *)D(0x6b7ee8);
            else
              v25 = 0;
            v24 = fm_597aa0(c, v25);
            (*v26__f) = 1;
            break;
          case 6:
            v24 = fm_596d60(c);
            break;
          default:
            goto LABEL_37;
        }
        if ( v24 != 0 )
        {
LABEL_37:
          (*v27__f) = v24;
          break;
        }
      }
      else if ( v24 != 13 )
      {
        goto LABEL_37;
      }
    }
    if ( (*v30__f) != 0 )
      F32((*v30__f), 80) = 0;
    W_menu_party_close((intptr_t)(c));
    W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
    return (*v27__f);
  }
  return result;
}

/* 0x591590: int __thiscall sub_591590(cls_591590 *this, _DWORD *a2, _DWORD *a3) */
int32_t fm_591590(wl_char *self, int32_t * a2, int32_t * a3)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x8) == 0 )
    return 0;
  *a2 = c + 12 * (1);
  *a3 = (uint8_t *)c + 1256;
  return 1;
}

/* 0x596d60: int __thiscall sub_596D60(cls_412590 *this) */
int32_t fm_596d60(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
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
    intptr_t v19 = 0;
    char * v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    char * v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    void * v29 = 0;
    _Alignas(8) uint8_t frame_[0x664 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v41 = (char *)(frame_ + 0x58);   /* [512] at ebp-0x60c */
    char *v42 = (char *)(frame_ + 0x258);   /* [512] at ebp-0x40c */
    char *v43 = (char *)(frame_ + 0x458);   /* [512] at ebp-0x20c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x664 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x64c */
    uint16_t *const v33__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x648 */
    int32_t * *const v34__f = (int32_t * *)(frame_ + 0x30);   /* ebp-0x634 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x630 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x62c */
    intptr_t *const Block__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x628 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x618 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x614 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x610 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x660);   /* ebp-0x4 */

  v1 = 0;
  (*v35__f) = 0;
  v3 = 1;
  F32(Block__f, 0x0) = 0;
  F32(Block__f, 0x4) = 0;
  (*v31__f) = F32(c, 0x0);
  (*v44__f) = 0;
  v4 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((*v31__f)), (intptr_t)(8), (intptr_t)(346), (intptr_t)(624), (intptr_t)(128), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x20) = (int)v4;
  v5 = W_win_ctrl_take((intptr_t)(v4));
  v6 = v5;
  if ( v5 != 0 )
  {
    W_ctrl_text_init((intptr_t)(v5), (intptr_t)(64), (intptr_t)(24), (intptr_t)(68), (intptr_t)(3));
    v7 = v6;
  }
  else
  {
    v7 = 0;
  }
  (*v32__f) = F32(c, 0x20);
  F32(c, 0x24) = (int)v7;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v32__f)), (intptr_t)(v7));
  W_text_metrics_set((intptr_t)(F32(c, 0x24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(64), (intptr_t)(32), (intptr_t)(192), (intptr_t)(0), (intptr_t)(5u), (intptr_t)(5u), (intptr_t)(1u), (intptr_t)(&(*v40__f)), (intptr_t)((int *)&(*v36__f)), (intptr_t)(((char *)D(0x8a2eb4))), (intptr_t)(1));
  v8 = (*v36__f);
  v9 = W_grid_row_add((intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(16));
  W_grid_cell_text((intptr_t)(v8), (intptr_t)(v9), (intptr_t)(0), (intptr_t)(((char *)D(0x8a23dc))));
  v10 = (*v36__f);
  v11 = W_grid_row_add((intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(17));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(v11), (intptr_t)(0), (intptr_t)(((char *)D(0x85551c))));
  v12 = (*v36__f);
  v13 = W_grid_row_add((intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(18));
  W_grid_cell_text((intptr_t)(v12), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3600))));
  v14 = (*v36__f);
  v15 = W_grid_row_add((intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(19));
  W_grid_cell_text((intptr_t)(v14), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(((char *)D(0x8a35f8))));
  v16 = (*v36__f);
  v17 = W_grid_row_add((intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(20));
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(v17), (intptr_t)(0), (intptr_t)(((char *)D(0x8a35e8))));
  (*v38__f) = F32(16 * F32((F32((*v36__f), 0x174) + 20), 0) + F32((*v36__f), 0x17c), 4);
  while ( 1 )
  {
LABEL_5:
    if ( v3 != 0 )
    {
      switch ( (*v38__f) )
      {
        case 16:
          v18 = ((char *)D(0x8a357c));
          goto LABEL_12;
        case 17:
          v18 = ((char *)D(0x8a3544));
          goto LABEL_12;
        case 18:
          v18 = ((char *)D(0x8a34f0));
          goto LABEL_12;
        case 19:
          v18 = ((char *)D(0x8a3450));
          goto LABEL_12;
        case 20:
          v18 = ((char *)D(0x8a340c));
LABEL_12:
          strcpy(v42, v18);
          break;
        default:
          break;
      }
      v19 = F32(c, 0x24);
      if ( v19 != 0 )
      {
        (*v33__f) = F16(F32(v19, 368), 8);
        v20 = W_ctrl_keys_expand((intptr_t)(v19), (intptr_t)(v42), (intptr_t)(v43));
        W_text_layout_set((intptr_t)(**(intptr_t **)(v19 + 368)), (intptr_t)(v20), (intptr_t)((*v33__f)));
        F16(F32(v19, 368), 4) = 0;
        F16(F32(v19, 368), 6) = 0;
        F32(v19, 28) = 0;
      }
      v3 = 0;
    }
    v21 = W_menu_run((intptr_t)(c), (intptr_t)((*v36__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v22 = F32(16 * F32((F32((*v36__f), 0x174) + 20), 0) + F32((*v36__f), 0x17c), 4);
    (*v38__f) = v22;
    if ( v21 == 11 )
      goto LABEL_61;
    if ( v21 == 12 )
      break;
    if ( v21 != 13 )
      goto LABEL_57;
    v3 = 1;
  }
  switch ( v22 )
  {
    case 16:
      W_tm_5644a0((intptr_t)(&(*Block__f)));
      W_ts_584420((intptr_t)(&(*Block__f)), (intptr_t)(32), (intptr_t)(392));
      W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(Block__f, 0x0)), (intptr_t)(32), (intptr_t)(8), (intptr_t)(320), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(7u), (intptr_t)(1u), (intptr_t)(&(*v39__f)), (intptr_t)((int *)&(*v34__f)), (intptr_t)(((char *)D(0x8a23dc))), (intptr_t)(1));
      W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(31), (intptr_t)(((char *)D(0x8a23c4))));
      W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(32), (intptr_t)(((char *)D(0x8a23b0))));
      W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(33), (intptr_t)(((char *)D(0x8a23a0))));
      W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(35), (intptr_t)(((char *)D(0x8a2390))));
      v23 = W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(34), (intptr_t)(((char *)D(0x8a2380))));
      F32(16 * v23 + (*v34__f)[95], 8) = W_mm_597480();
      v24 = W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(38), (intptr_t)(((char *)D(0x8a2368))));
      F32(16 * v24 + (*v34__f)[95], 8) = W_mm_597480();
      W_grid_row_text_add((intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(27), (intptr_t)(((char *)D(0x8a2360))));
      while ( 2 )
      {
        strcpy(v41, D(0x8a9b6c));
        switch ( F32(16 * F32(((*v34__f)[93] + 20), 0) + (*v34__f)[95], 4) )
        {
          case 0x1B:
            v25 = ((char *)D(0x8a2254));
            goto LABEL_30;
          case 0x1F:
            v25 = ((char *)D(0x8a232c));
            goto LABEL_30;
          case 0x20:
            v25 = ((char *)D(0x8a22dc));
            goto LABEL_30;
          case 0x21:
            v25 = ((char *)D(0x8a22b4));
            goto LABEL_30;
          case 0x22:
            v25 = ((char *)D(0x8a2294));
            goto LABEL_30;
          case 0x23:
            v25 = ((char *)D(0x8a2278));
            goto LABEL_30;
          case 0x26:
            v25 = ((char *)D(0x8a22f8));
LABEL_30:
            strcpy(v41, v25);
            break;
          default:
            break;
        }
        W_ctrl_text_set((intptr_t)(F32(Block__f, 0xc)), (intptr_t)(v41));
        v26 = W_tm_565d10((intptr_t)(&(*Block__f)), (intptr_t)((*v34__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v26 != 11 )
        {
          if ( v26 != 12 )
          {
            if ( v26 != 13 )
            {
LABEL_39:
              v1 = v26;
              break;
            }
            continue;
          }
          v27 = F32(16 * F32(((*v34__f)[93] + 20), 0) + (*v34__f)[95], 4);
          switch ( v27 )
          {
            case 27:
              goto LABEL_40;
            case 31:
            case 32:
            case 33:
            case 38:
              v26 = W_tm_564780((intptr_t)(&(*Block__f)), (intptr_t)(v27));
              if ( v26 == 0 )
                continue;
              goto LABEL_39;
            case 34:
            case 35:
              v26 = W_ts_569220((intptr_t)(&(*Block__f)), (intptr_t)(v27));
              if ( v26 != 0 )
                goto LABEL_39;
              continue;
            default:
              continue;
          }
        }
        break;
      }
LABEL_40:
      *(_DWORD *)((uint8_t *)((*v39__f) + (1) * 0x50) + 0x0) = 0;
      W_ts_584400((intptr_t)(&(*Block__f)));
      if ( v1 != 6 )
        W_tm_565d10((intptr_t)(&(*Block__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0x14u));
      W_ts_56ad50((intptr_t)(&(*Block__f)));
      if ( v1 != 0 )
      {
        (*v35__f) = v1;
        goto LABEL_61;
      }
      v3 = 1;
      goto LABEL_5;
    case 17:
      v21 = W_mm_597670((intptr_t)(c), (intptr_t)(F32(c, 0xc)));
      if ( v21 != 0 )
        goto LABEL_57;
      v3 = 1;
      goto LABEL_5;
    case 18:
      v21 = W_mm_5974b0((intptr_t)(c), (intptr_t)(F32(c, 0xc)));
      if ( v21 != 0 )
        goto LABEL_57;
      v3 = 1;
      goto LABEL_5;
    case 19:
      if ( *(_DWORD *)&GAP(W, 0x1030, 4) == 1 || F32(W, 0x1030) == 2 )
      {
        v21 = W_msg_pages((intptr_t)(c), (intptr_t)(((char *)D(0x8a33cc))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
        if ( v21 != 0 )
          goto LABEL_57;
      }
      else
      {
        v21 = W_mh_4190d0((intptr_t)(c), (intptr_t)(((char *)D(0x8a33e8))), (intptr_t)(0x28u), (intptr_t)((_DWORD *)2), (intptr_t)(1));
        if ( v21 == 14 )
        {
          (*v35__f) = 5;
          goto LABEL_61;
        }
        if ( v21 != 15 )
          goto LABEL_57;
      }
      goto LABEL_5;
    case 20:
      v21 = W_mh_4190d0((intptr_t)(c), (intptr_t)(((char *)D(0x8a3394))), (intptr_t)(0x28u), (intptr_t)((_DWORD *)2), (intptr_t)(1));
      if ( v21 != 14 )
      {
        if ( v21 != 15 )
        {
LABEL_57:
          (*v35__f) = v21;
          goto LABEL_61;
        }
        goto LABEL_5;
      }
      (*v35__f) = 6;
LABEL_61:
      *(_DWORD *)((uint8_t *)((*v40__f) + (1) * 0x50) + 0x0) = 0;
      v28 = F32(c, 0x20);
      if ( v28 != 0 )
      {
        F32(v28, 80) = 0;
        F32(c, 0x20) = 0;
        F32(c, 0x24) = 0;
      }
      (*v44__f) = -1;
      if ( F32(Block__f, 0x0) != 0 )
      {
        v29 = (void *)F32(Block__f, 0x0);
        W_en_56c640((intptr_t)(F32(Block__f, 0x0)));
        g_eng.op_delete((void *)v29);
        F32(Block__f, 0x0) = 0;
      }
      if ( F32(Block__f, 0x4) != 0 )
        fm_56c430(F32(Block__f, 0x4), 1);
      return (*v35__f);
    default:
      goto LABEL_5;
  }
}

/* 0x597aa0: int __thiscall sub_597AA0(cls_597AA0 *this, _DWORD *a2) */
int32_t fm_597aa0(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    void * v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t i = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
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
    char * v33 = 0;
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
    _Alignas(8) uint8_t frame_[0x680 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v53 = (intptr_t *)(frame_ + 0x1c);   /* [4] at ebp-0x664 */
    int16_t *v54 = (int16_t *)(frame_ + 0x2c);   /* [2] at ebp-0x654 */
    char *v67 = (char *)(frame_ + 0x5c);   /* [36] at ebp-0x624 */
    char *Buffer = (char *)(frame_ + 0x80);   /* [512] at ebp-0x600 */
    char *v69 = (char *)(frame_ + 0x280);   /* [512] at ebp-0x400 */
    char *v70 = (char *)(frame_ + 0x480);   /* [512] at ebp-0x200 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x680 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x680 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x678 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x668 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x668 */
    int16_t *const v55__f = (int16_t *)(frame_ + 0x30);   /* ebp-0x650 */
    int16_t *const v56__f = (int16_t *)(frame_ + 0x32);   /* ebp-0x64e */
    intptr_t *const v57__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x64c */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x648 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x644 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x640 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x63c */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x638 */
    char * *const v63__f = (char * *)(frame_ + 0x4c);   /* ebp-0x634 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x630 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x62c */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x628 */

  v53[0] = 1;
  v2 = c;
  (*v48__f) = F32(c, 0x0);
  (*v57__f) = c;
  (*v59__f) = 0;
  (*v60__f) = 0;
  v3 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((*v48__f)), (intptr_t)(8), (intptr_t)(346), (intptr_t)(624), (intptr_t)(128), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
  F32(v2, 4 * 8) = v3;
  v4 = W_win_ctrl_take((intptr_t)(v3));
  v5 = v4;
  if ( v4 != 0 )
    W_ctrl_text_init((intptr_t)(v4), (intptr_t)(64), (intptr_t)(24), (intptr_t)(68), (intptr_t)(3));
  else
    v5 = 0;
  v6 = F32(v2, 4 * 8);
  F32(v2, 4 * 9) = v5;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v6), (intptr_t)(v5));
  W_text_metrics_set((intptr_t)(F32(v2, 4 * (9))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v7 = 0;
  if ( a2 == 0 )
    goto LABEL_9;
  if ( *a2 == 0 )
    goto LABEL_9;
  v8 = 0;
  do
  {
    ++v7;
    (*v62__f) = v8;
    v8 = (uint16_t)v7;
  }
  while ( a2[303 * (uint16_t)v7] != 0 );
  (*v66__f) = v7;
  if ( (uint16_t)v7 != 0 )
  {
    v12 = F32(v2, 0);
    v13 = 0;
    v14 = 4;
    if ( ((char *)D(0x8a2ebc)) != 0 )
    {
      v13 = 1;
      v14 = 5;
    }
    W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v67), (intptr_t)(v13));
    v15 = W_win_mgr_open((intptr_t)(v12), (intptr_t)(32), (intptr_t)(16), (intptr_t)(576), (intptr_t)(328), (intptr_t)(v14), (intptr_t)(v67));
    v16 = v15;
    (*v65__f) = v15;
    if ( ((char *)D(0x8a2ebc)) != 0 )
      W_win_title_add((intptr_t)(v15), (intptr_t)(((char *)D(0x8a2ebc))));
    v17 = W_win_ctrl_take((intptr_t)(v16));
    v18 = v17;
    if ( v17 != 0 )
      W_ctrl_grid_init((intptr_t)(v17), (intptr_t)(32), (intptr_t)(48), (intptr_t)(2), (intptr_t)(5), (intptr_t)(2), (intptr_t)(1));
    else
      v18 = 0;
    v19 = (_DWORD *)F32(v18, 0x178);
    v54[0] = 0;
    (*v55__f) = 2;
    (*v56__f) = 3;
    *v19 = 0;
    F32(F32(v18, 0x178), 4) = 0;
    F32(F32(v18, 0x178), 8) = 0;
    F32(F32(v18, 0x178), 12) = 0;
    F32(F32(v18, 0x178), 36) = 40;
    F32(F32(v18, 0x178), 40) = 0;
    F32(F32(v18, 0x178), 44) = 0;
    F32(F32(v18, 0x178), 48) = 0;
    F32(F32(v18, 0x178), 72) = 80;
    F32(F32(v18, 0x178), 76) = 0;
    F32(F32(v18, 0x178), 80) = 0;
    F32(F32(v18, 0x178), 84) = 0;
    F32(F32(v18, 0x178), 108) = 152;
    F32(F32(v18, 0x178), 112) = 0;
    F32(F32(v18, 0x178), 116) = 0;
    F32(F32(v18, 0x178), 120) = 0;
    F32(F32(v18, 0x178), 144) = 288;
    F32(F32(v18, 0x178), 148) = 0;
    F32(F32(v18, 0x178), 152) = 0;
    F32(F32(v18, 0x178), 156) = 0;
    v54[1] = 1;
    if ( F32(CFG(), 596) == 0 )
    {
      F32(((uint8_t *)(v18) + 0x14), 0) = 1;
      *(_DWORD *)&GAP(v18, 0x14, 4) = -9;
    }
    v20 = F32(v18, 0x174);
    v21 = F32(v20, 16);
    if ( v21 < F32(v20, 4) )
    {
      (*v51__f) = F32(v20, 16);
      F32(v20, 16) = v21 + 1;
      v22 = 16 * v21;
      F32((v22 + F32(v18, 0x17c)), 0) = 0;
      F32(v22 + F32(v18, 0x17c), 4) = 0;
      F32(v22 + F32(v18, 0x17c), 8) = 1;
      W_grid_row_reset((intptr_t)(v18), (intptr_t)((*v51__f)));
    }
    else
    {
      v21 = -1;
    }
    *(_DWORD *)&GAP((v18 + (1) * 0x180), 0x24, 8) = 16;
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v21), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3834))));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v21), (intptr_t)(2), (intptr_t)(((char *)D(0x8a3828))));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v21), (intptr_t)(3), (intptr_t)(((char *)D(0x8a3820))));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v21), (intptr_t)(4), (intptr_t)(((char *)D(0x8a3818))));
    if ( F32(F32(v18, 0x174), 16) < F32(F32(v18, 0x174), 4) )
    {
      v24 = F32(v18, 0x174);
      v23 = F32(v24, 16);
      F32(v24, 16) = v23 + 1;
      v25 = 16 * v23;
      F32((v25 + F32(v18, 0x17c)), 0) = 0;
      F32(v25 + F32(v18, 0x17c), 4) = 0;
      F32(v25 + F32(v18, 0x17c), 8) = 1;
      W_grid_row_reset((intptr_t)(v18), (intptr_t)(v23));
    }
    else
    {
      v23 = -1;
    }
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v23), (intptr_t)(0), (intptr_t)(D(0x8a202c)));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v23), (intptr_t)(1), (intptr_t)(D(0x8a202c)));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v23), (intptr_t)(2), (intptr_t)(D(0x854c7c)));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v23), (intptr_t)(3), (intptr_t)(D(0x8a3804)));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v23), (intptr_t)(4), (intptr_t)(D(0x8a37e4)));
    v26 = W_win_ctrl_take((intptr_t)(v16));
    if ( v26 != 0 )
      W_ctrl_grid_init((intptr_t)(v26), (intptr_t)(32), (intptr_t)(84), (intptr_t)((uint16_t)(*v66__f)), (intptr_t)(5), (intptr_t)(8), (intptr_t)(2));
    else
      v26 = 0;
    F32(F32(v26, 376), 0) = 0;
    F32(F32((v26 + 376), 0), 4) = -8;
    v27 = F32(v26, 376);
    v54[0] = 0;
    F32(v27, 8) = 0;
    v28 = F32(v26, 376);
    v54[1] = 1;
    (*v55__f) = 2;
    F32(v28, 12) = 0;
    v29 = F32(v26, 376);
    (*v56__f) = 3;
    F32(v29, 36) = 40;
    F32(F32((v26 + 376), 0), 40) = 0;
    F32(F32((v26 + 376), 0), 44) = 0;
    F32(F32((v26 + 376), 0), 48) = 0;
    F32(F32((v26 + 376), 0), 72) = 80;
    F32(F32((v26 + 376), 0), 76) = 0;
    F32(F32((v26 + 376), 0), 80) = 0;
    F32(F32((v26 + 376), 0), 84) = 0;
    F32(F32((v26 + 376), 0), 108) = 152;
    F32(F32((v26 + 376), 0), 112) = 0;
    F32(F32((v26 + 376), 0), 116) = 0;
    F32(F32((v26 + 376), 0), 120) = 0;
    F32(F32((v26 + 376), 0), 144) = 288;
    F32(F32((v26 + 376), 0), 148) = 0;
    F32(F32((v26 + 376), 0), 152) = 0;
    F32(F32((v26 + 376), 0), 156) = 0;
    W_ctrl_cursor_set((intptr_t)(v26), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v54), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
    if ( F32(CFG(), 596) == 0 )
    {
      F32(v26, 20) = 1;
      F32(v26, 24) = -9;
    }
    v54[1] = 1;
    v30 = (*v65__f);
    (*v50__f) = F32((*v65__f), 20) - 36;
    (*v49__f) = F32((*v65__f), 24) / 2 - 16;
    v54[0] = 0;
    (*v55__f) = 2;
    (*v56__f) = 3;
    W_tm_56daa0((intptr_t)(v26), (intptr_t)(SYS32(2956)), (intptr_t)(4), (intptr_t)(4), (intptr_t)((*v49__f)), (intptr_t)(8), (intptr_t)((*v50__f)), (intptr_t)((*v49__f)), (intptr_t)(v54), (intptr_t)(4u), (intptr_t)(8));
    W_tm_56da40((intptr_t)(v26), (intptr_t)((const char *)((char *)D(0x854898))), (intptr_t)(F32(v30, 20) - 56), (intptr_t)(8));
    F32(v26, 416) = 0xFFFFFF;
    F32(v26, 412) = 4546427;
    while ( 1 )
    {
      v31 = F32((*v57__f), 0x24);
      if ( v31 != 0 )
        W_ctrl_text_set((intptr_t)(v31), (intptr_t)(((char *)D(0x8a377c))));
      F32(F32((v26 + 372), 0), 16) = 0;
      v32 = (uint16_t)(*v66__f) - 1;
      (*v64__f) = v32;
      if ( (uint16_t)(*v66__f) != 0 )
      {
        (*v58__f) = (uint16_t)(*v66__f);
        v33 = (const char *)&a2[303 * v32 + 3];
        (*v63__f) = v33;
        do
        {
          v34 = F32((*v57__f), 0xc);
          if ( W_tm_560a00((intptr_t)(F32(v34, 16556)), (intptr_t)(*((int32_t *)v33 - 1))) >= F32(v33, 0) )
          {
            if ( W_tm_560a00((intptr_t)(F32(v34, 16556)), (intptr_t)(*((int32_t *)v33 - 3))) != 0 )
            {
              if ( v53 != (_DWORD *)-40 )
                (*v61__f) = 73;
            }
            else if ( v53 != (_DWORD *)-40 )
            {
              (*v61__f) = 72;
            }
            v35 = F32(v26, 372);
            v36 = F32(v35, 16);
            if ( v36 < F32(v35, 4) )
            {
              (*v52__f) = F32(v35, 16);
              F32(v35, 16) = v36 + 1;
              v38 = 16 * v36;
              F32((F32(v26, 380) + v38), 0) = 0;
              F32(F32((v26 + 380), 0) + v38, 4) = v32;
              F32(F32((v26 + 380), 0) + v38, 8) = 1;
              W_grid_row_reset((intptr_t)(v26), (intptr_t)((*v52__f)));
              v37 = v36;
            }
            else
            {
              v37 = -1;
            }
            F32(v54, 0) = SYS32(2928);
            v39 = F32(v26, 372);
            if ( F32(v39, 8) > 0 && F32(v39, 16) > v37 )
            {
              v40 = 16 * v37;
              if ( F32(F32((F32(v26, 380) + 16 * v37 + 12), 0), 4) == 0 )
                F32(F32((F32(v26, 380) + v40 + 12), 0), 4) = (intptr_t)g_eng.op_new(0x54u);
              W_icon_set((intptr_t)(F32(F32((F32(v26, 380) + v40 + 12), 0), 4)), (intptr_t)(F32(v54, 0)), (intptr_t)((*v61__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            }
            g_eng.h5bf3ee(Buffer, "%02d", (*v58__f));
            W_grid_cell_text((intptr_t)(v26), (intptr_t)(v37), (intptr_t)(1), (intptr_t)(Buffer));
            g_eng.h5bf3ee(Buffer, "Day % 3d", *((uint16_t *)v33 - 4));
            W_grid_cell_text((intptr_t)(v26), (intptr_t)(v37), (intptr_t)(2), (intptr_t)(Buffer));
            if ( strlen(v33 + 1160) != 0 )
            {
              g_eng.h5bf3ee(Buffer, "%s", v33 + 1160);
            }
            else
            {
              v41 = W_dramatist_row((intptr_t)(F32(v33, 4 * 1)));
              if ( v41 != 0 )
              {
                g_eng.h5bf3ee(Buffer, "%s", (const char *)(v41 + 8));
              }
              else
              {
                strcpy(Buffer, D(0x8a9b6c));
                v33 = (*v63__f);
              }
            }
            W_fm_5984c0((intptr_t)(Buffer), (intptr_t)(v70), (intptr_t)(16), (intptr_t)(D(0x8a3760)));
            W_grid_cell_text((intptr_t)(v26), (intptr_t)(v37), (intptr_t)(3), (intptr_t)(v70));
            W_fm_5984c0((intptr_t)(v33 + 8), (intptr_t)(Buffer), (intptr_t)(32), (intptr_t)(D(0x8a3760)));
            W_grid_cell_text((intptr_t)(v26), (intptr_t)(v37), (intptr_t)(4), (intptr_t)(Buffer));
            v32 = (*v64__f);
          }
          --v32;
          v33 -= 1212;
          (*v64__f) = v32;
          --(*v58__f);
          (*v63__f) = v33;
        }
        while ( v32 >= 0 );
        v30 = (*v65__f);
      }
      W_tm_419900((intptr_t)(v26), (intptr_t)(0), (intptr_t)((*v62__f)));
      if ( (*v60__f) != 0 && (v42 = F32(v26, 372), (v43 = F32(v42, 16)) > 0) )
      {
        fm_598460(v26, ((*v60__f) + v43 + F32(v42, 20)) % v43);
        v44 = 12;
      }
      else
      {
        v44 = W_menu_run((intptr_t)((*v57__f)), (intptr_t)((_DWORD *)v26), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      }
      v45 = F32(16 * F32((F32(v26, 372) + 20), 0) + F32((v26 + 380), 0), 4);
      (*v62__f) = v45;
      if ( v44 == 11 )
        break;
      if ( v44 == 12 )
      {
        v44 = fm_5985f0((*v57__f), (int)&a2[303 * v45], v45 + 1);
        if ( v44 == 18 )
        {
          (*v60__f) = -1;
        }
        else if ( v44 == 19 )
        {
          (*v60__f) = 1;
        }
        else
        {
          (*v60__f) = 0;
          if ( v44 != 0 )
          {
LABEL_79:
            (*v59__f) = v44;
            break;
          }
        }
      }
      else if ( v44 != 13 )
      {
        goto LABEL_79;
      }
    }
    if ( v30 != 0 )
      F32(v30, 80) = 0;
    v2 = (*v57__f);
  }
  else
  {
LABEL_9:
    strcpy(v69, ((char *)D(0x8a3838)));
    (*v62__f) = 0;
    (*v58__f) = 0;
    for ( i = W_msg_window_open((intptr_t)(v2), (intptr_t)(v69), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          ;
          W_ctrl_text_set((intptr_t)(F32((F32(i, 72) + 4 * F16(i, 78) - 4), 0)), (intptr_t)(&v69[512 * v11])) )
    {
      while ( 1 )
      {
        v10 = W_menu_run((intptr_t)(v2), (intptr_t)(0), (intptr_t)(i), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v10 == 12 )
          break;
        if ( v10 != 13 )
          goto LABEL_16;
      }
      v11 = ++(*v58__f);
      if ( (uint16_t)(*v58__f) != 0 )
        break;
    }
    v10 = (*v62__f);
LABEL_16:
    F32(i, 80) = 0;
    if ( v10 != 0 )
      (*v59__f) = v10;
  }
  v46 = F32(v2, 4 * 8);
  if ( v46 != 0 )
  {
    F32(v46, 80) = 0;
    F32(v2, 4 * 8) = 0;
    F32(v2, 4 * 9) = 0;
  }
  return (*v59__f);
}

/* 0x598460: void __thiscall sub_598460(cls_598460 *this, int a2) */
void fm_598460(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    int32_t * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  if ( a2 >= 0 )
  {
    v2 = F32(c, 0x174);
    v3 = F32(v2, 16) - 1;
    v4 = v3;
    if ( v3 >= a2 )
      v4 = a2;
    if ( v4 >= 0 )
    {
      if ( v3 >= a2 )
        v3 = a2;
    }
    else
    {
      v3 = 0;
    }
    F32(v2, 20) = v3;
    v5 = (_DWORD *)F32(c, 0x174);
    v6 = 0;
    v7 = v5[3];
    v8 = v5[5];
    while ( v8 < v6 || v8 >= v7 + v6 )
      v6 += v7;
    if ( v6 != v5[6] )
      v5[6] = v6;
  }
}

/* 0x5984c0: char *__stdcall sub_5984C0(const char *a1, char *a2, int a3, const char *a4) — the line clip: a1 copied
 * into a2 one character at a time (a DBCS lead byte takes its trail byte along) while the 16-bit index stays <= a3;
 * past it the copy is cut back where the index first reached a3 - strlen(a4) (unsigned) and a4 appended.  Hand port
 * (IDA reuses a1's stack slot as the character buffer). */
int32_t fm_5984c0(char * a1, char * a2, int32_t a3, char * a4)
{
    uint32_t i = 0, cut = 0;
    a2[0] = 0;
    if (strlen(a1) == 0)
        return (int32_t)(intptr_t)a2;
    for (;;) {
        if ((uint32_t)(uint16_t)i >= (uint32_t)a3 - (uint32_t)strlen(a4) && (uint16_t)cut == 0)
            cut = i;
        char ch[4];
        ch[0] = a1[(uint16_t)i];
        if ((signed char)ch[0] < 0) {
            ch[1] = a1[(uint16_t)i + 1];
            ch[2] = 0;
            i++;
        } else {
            ch[1] = 0;
        }
        strcat(a2, ch);
        if ((int32_t)(uint16_t)i > a3)
            break;
        i++;
        if ((uint32_t)(uint16_t)i >= (uint32_t)strlen(a1))
            return (int32_t)(intptr_t)a2;
    }
    if ((uint16_t)cut != 0)
        a2[(uint16_t)cut] = 0;
    strcat(a2, a4);
    return (int32_t)(intptr_t)a2;
}

/* 0x5985f0: int __thiscall sub_5985F0(cls_5985F0 *this, int a2, int a3) */
int32_t fm_5985f0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    char * v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    uint32_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    uint16_t i = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    char * v28 = 0;
    uint32_t v29 = 0;
    char * v30 = 0;
    char * v31 = 0;
    char * v32 = 0;
    uint32_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    int32_t * v37 = 0;
    char * v38 = 0;
    int16_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t * v41 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x185c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v61 = (char *)(frame_ + 0x38);   /* [36] at ebp-0x1824 */
    char *Buffer = (char *)(frame_ + 0x5c);   /* [512] at ebp-0x1800 */
    char *v63 = (char *)(frame_ + 0x25c);   /* [512] at ebp-0x1600 */
    char *v64 = (char *)(frame_ + 0x45c);   /* [5120] at ebp-0x1400 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x185c */
    uint32_t *const v47__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x1858 */
    uint16_t *const v48__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x1858 */
    uint16_t *const v44__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x1858 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1858 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1858 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1854 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x1844 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1840 */
    int16_t *const v52__f = (int16_t *)(frame_ + 0x20);   /* ebp-0x183c */
    int16_t *const v53__f = (int16_t *)(frame_ + 0x22);   /* ebp-0x183a */
    char *const v54__f = (char *)(frame_ + 0x24);   /* ebp-0x1838 */
    char *const v55__f = (char *)(frame_ + 0x25);   /* ebp-0x1837 */
    char *const v56__f = (char *)(frame_ + 0x26);   /* ebp-0x1836 */
    intptr_t *const v57__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1834 */
    uint32_t *const v58__f = (uint32_t *)(frame_ + 0x2c);   /* ebp-0x1830 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x182c */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x1828 */

  v4 = F32(c, 0x24);
  (*v60__f) = c;
  (*v59__f) = 0;
  (*v57__f) = 1;
  if ( v4 != 0 )
  {
    (*v44__f) = F16(F32(v4, 368), 8);
    v5 = W_ctrl_keys_expand((intptr_t)(v4), (intptr_t)(((char *)D(0x8a387c))), (intptr_t)(v63));
    W_text_layout_set((intptr_t)(**(intptr_t **)(v4 + 368)), (intptr_t)(v5), (intptr_t)((*v44__f)));
    F16(F32(v4, 368), 4) = 0;
    F16(F32(v4, 368), 6) = 0;
    F32(v4, 28) = 0;
  }
  g_eng.h5bf3ee(Buffer, "No.%d %s", a3, (const char *)(a2 + 20));
  v6 = F32(c, 0x0);
  v7 = 0;
  v8 = 4;
  if ( Buffer != 0 )
  {
    v7 = 1;
    v8 = 5;
  }
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v61), (intptr_t)(v7));
  v9 = W_win_mgr_open((intptr_t)(v6), (intptr_t)(100), (intptr_t)(64), (intptr_t)(440), (intptr_t)(224), (intptr_t)(v8), (intptr_t)(v61));
  v10 = (unsigned int)v9;
  (*v58__f) = (unsigned int)v9;
  if ( &(*v49__f) != (int *)-84 )
  {
    v11 = W_win_grid_add((intptr_t)(v9), (intptr_t)(32), (intptr_t)(7), (intptr_t)(0), (intptr_t)(0));
    v12 = v11;
    if ( v11 != 0 )
    {
      W_grid_cell_text((intptr_t)(v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(Buffer));
      *(_DWORD *)((uint8_t *)(v12 + (1) * 0x180) + 0x0) = 0xFFFFFF;
      *(_DWORD *)&GAP((v12 + (1) * 0x180), 0x0, 4) = 4546427;
    }
  }
  v13 = F16(v10, 78);
  if ( F16(v10, 76) > v13
    && (v14 = F32((F32(v10, 72) + 4 * v13), 0),
        W_ui_ref_set((intptr_t)(v14), (intptr_t)(v10), (intptr_t)(v13)),
        ++F16(v10, 78),
        v14 != 0) )
  {
    W_ctrl_grid_init((intptr_t)(v14), (intptr_t)(32), (intptr_t)(48), (intptr_t)(1), (intptr_t)(2), (intptr_t)(1), (intptr_t)(1));
  }
  else
  {
    v14 = 0;
  }
  v15 = F32(v14, 376);
  (*v51__f) = 0x10000;
  (*v52__f) = 2;
  *v15 = 64;
  v16 = F32(v14, 376);
  (*v53__f) = 3;
  F32(v16, 4) = 0;
  F32(F32((v14 + 376), 0), 8) = 0;
  F32(F32((v14 + 376), 0), 12) = 0;
  F32(F32((v14 + 376), 0), 36) = 192;
  F32(F32((v14 + 376), 0), 40) = 0;
  F32(F32((v14 + 376), 0), 44) = 0;
  F32(F32((v14 + 376), 0), 48) = 0;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v14, 20) = 1;
    F32(v14, 24) = -9;
  }
  v17 = F32(v14, 372);
  v18 = F32(v17, 16);
  if ( v18 < F32(v17, 4) )
  {
    (*v45__f) = F32(v17, 16);
    F32(v17, 16) = v18 + 1;
    v19 = 16 * v18;
    F32((v19 + F32(v14, 380)), 0) = 0;
    F32(v19 + F32((v14 + 380), 0), 4) = 0;
    F32(v19 + F32((v14 + 380), 0), 8) = 1;
    W_grid_row_reset((intptr_t)(v14), (intptr_t)((*v45__f)));
    (*v50__f) = v18;
  }
  else
  {
    (*v50__f) = -1;
  }
  g_eng.h5bf3ee(Buffer, "Received:  Day % 3d", F16(a2, 4));
  W_grid_cell_text((intptr_t)(v14), (intptr_t)((*v50__f)), (intptr_t)(0), (intptr_t)(Buffer));
  if ( strlen((const char *)(a2 + 1172)) != 0 )
  {
    g_eng.h5bf3ee(Buffer, "%s", (const char *)(a2 + 1172));
  }
  else
  {
    for ( i = 0; ; ++i )
    {
      v21 = ((int32_t *)D(0x6b6ea8))[13 * i];
      if ( v21 == 0 )
        break;
      if ( v21 == F32(a2, 16) )
      {
        v22 = 13 * i;
        if ( 4 * v22 != -7040680 )
        {
          g_eng.h5bf3ee(Buffer, " From: %s", (const char *)(4 * v22 + 7040688));
          goto LABEL_26;
        }
        break;
      }
    }
    strcpy(Buffer, D(0x8a9b6c));
    v10 = (*v58__f);
  }
LABEL_26:
  W_grid_cell_text((intptr_t)(v14), (intptr_t)((*v50__f)), (intptr_t)(1), (intptr_t)(Buffer));
  v23 = W_win_ctrl_take((intptr_t)(v10));
  v24 = v23;
  if ( v23 != 0 )
    W_ctrl_text_init((intptr_t)(v23), (intptr_t)(40), (intptr_t)(96), (intptr_t)(50), (intptr_t)(4));
  else
    v24 = 0;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v10), (intptr_t)(v24));
  (*v46__f) = F32(v10, 24) - F32(v24, 0x10) - 40;
  (*v43__f) = F32(v10, 20) - F32(v24, 0xc) - 40;
  (*v51__f) = 0x10000;
  (*v52__f) = 2;
  (*v53__f) = 3;
  W_ctrl_cursor_set((intptr_t)(v24), (intptr_t)(SYS32(2956)), (intptr_t)(20), (intptr_t)((int16_t *)&(*v51__f)), (intptr_t)(4u), (intptr_t)(10), (intptr_t)((*v43__f)), (intptr_t)((*v46__f)));
  v25 = F32(v24, 0x170);
  SETLO((*v51__f), 0);
  F32(v25, 12) = 0;
  v26 = F32(v24, 0x170);
  SETHI((*v51__f), 1);
  (*v52__f) = 2;
  F32(v26, 16) = 0;
  v27 = F32(v24, 0x170);
  (*v53__f) = 3;
  F32(v27, 32) = 0;
  F32(F32(v24, 0x170), 36) = 0;
  F32(F32(v24, 0x170), 40) = 0;
  F32(F32(v24, 0x170), 20) = 1;
  F32(F32(v24, 0x170), 24) = 0;
  F32(v24, 0x1c) = 0;
  W_tm_56daa0((intptr_t)(v24), (intptr_t)(SYS32(2956)), (intptr_t)(4), (intptr_t)(4), (intptr_t)(F32(v10, 24) / 2 - 16), (intptr_t)(8), (intptr_t)(F32(v10, 20) - 36), (intptr_t)(F32(v10, 24) / 2 - 16), (intptr_t)((int16_t *)&(*v51__f)), (intptr_t)(4u), (intptr_t)(8));
  W_tm_56da40((intptr_t)(v24), (intptr_t)((const char *)((char *)D(0x854898))), (intptr_t)(F32(v10, 20) - 56), (intptr_t)(8));
  v28 = 0;
  *(_DWORD *)&GAP((v24 + (1) * 0x174), 0x24, 8) = 0xFFFFFF;
  *(_DWORD *)&GAP((v24 + (1) * 0x174), 0x24, 4) = 4546427;
  (*v50__f) = 0;
  v64[0] = 0;
  (*v51__f) = 0;
  if ( strlen((const char *)(a2 + 148)) != 0 )
  {
    do
    {
      (*v54__f) = F8((uint16_t)v28 + a2, 148);
      if ( (*v54__f) >= 0 )
      {
        (*v55__f) = 0;
        if ( (*v54__f) == 64 )
        {
          (*v54__f) = 0;
          v64[512 * (uint16_t)++(*v50__f)] = 0;
        }
      }
      else
      {
        (*v55__f) = F8((uint16_t)v28 + a2, 149);
        (*v56__f) = 0;
        (*v51__f) = (v28 + 1);
      }
      v29 = strlen(&(*v54__f)) + 1;
      v30 = &v64[512 * (uint16_t)(*v50__f) + strlen(&v64[512 * (uint16_t)(*v50__f)])];
      qmemcpy(v30, &(*v54__f), 4 * (v29 >> 2));
      v32 = &(*v54__f) + 4 * (v29 >> 2);
      v31 = &v30[4 * (v29 >> 2)];
      v33 = v29 & 3;
      v34 = (*v51__f);
      qmemcpy(v31, v32, v33);
      v28 = &GAP(v34, 0x0, 1);
      (*v51__f) = v28;
    }
    while ( (uint16_t)v28 < strlen((const char *)(a2 + 148)) );
    v10 = (*v58__f);
  }
  (*v47__f) = F32(a2, 0);
  v35 = F32(F32((*v60__f), 0xc), 16556);
  ++(*v50__f);
  v36 = 0;
  (*v58__f) = (*v47__f);
  (*v51__f) = v35;
  v37 = W_flag_find((intptr_t)(v35), (intptr_t)((*v47__f)));
  if ( v37 == 0 )
    v37 = W_flag_add((intptr_t)((*v51__f)), (intptr_t)((*v58__f)));
  v37[1] = 1;
  v37[2] = 0;
  while ( 2 )
  {
    if ( (*v57__f) != 0 )
    {
      (*v57__f) = 0;
      W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      (*v48__f) = F16(F32(v24, 0x170), 8);
      v38 = W_ctrl_keys_expand((intptr_t)(v24), (intptr_t)(&v64[512 * (uint16_t)v36]), (intptr_t)(v63));
      W_text_layout_set((intptr_t)(F32(F32(v24, 0x170), 0)), (intptr_t)(v38), (intptr_t)((*v48__f)));
      F16(F32(v24, 0x170), 4) = 0;
      v39 = (*v50__f);
      F16(F32(v24, 0x170), 6) = 0;
      F32(v24, 0x1c) = 0;
      *(_WORD *)&GAP(v24, 0x144, 28) = v36 + 1;
      *(_WORD *)&GAP(v24, 0x144, 30) = v39;
    }
    v40 = W_menu_run((intptr_t)((*v60__f)), (intptr_t)(0), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
    switch ( v40 )
    {
      case 12:
        ++v36;
        (*v57__f) = 1;
        if ( (uint16_t)v36 < (uint16_t)(*v50__f) )
          continue;
        v41 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(888)), (intptr_t)(0));
        if ( v41 == 0 )
          goto LABEL_51;
        g_eng.dsb_volume_5bb870(*v41, F32(CFG(), 360));
        g_eng.dsb_freq_5bb890(*v41, 0);
        g_eng.dsb_play_5bb8c0(*v41);
        result = (*v59__f);
        F32(v10, 80) = 0;
        break;
      case 13:
        continue;
      case 16:
        if ( (uint16_t)v36 != 0 )
        {
          v36 += 0xFFFF;
          (*v57__f) = 1;
        }
        continue;
      case 17:
        if ( (uint16_t)v36 < (uint16_t)(*v50__f) - 1 )
        {
          ++v36;
          (*v57__f) = 1;
        }
        continue;
      default:
        (*v59__f) = v40;
LABEL_51:
        result = (*v59__f);
        F32(v10, 80) = 0;
        break;
    }
    return result;
  }
}

/* 0x56c430: void **__thiscall sub_56C430(cls_56C430 *this, char a2) */
int32_t fm_56c430(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t i = 0;

  v3 = c + 4 * (3);
  for ( i = 64; i != 0; --i )
  {
    if ( F32(v3, 0) != 0 )
    {
      g_eng.op_delete((void *)F32(v3, 0));
      F32(v3, 0) = 0;
    }
    v3 += 4;
  }
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return (void **)c;
}

/* 0x590480: int __thiscall sub_590480(cls_590480 *this) */
int32_t fm_590480(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    intptr_t * v3 = 0;
    intptr_t v5 = 0;
    intptr_t * v6 = 0;
    intptr_t * v7 = 0;
    uint16_t * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t * v14 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v2 = 0;
  if ( F16(c, 0x4070) != 0 )
  {
    v3 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(896)), (intptr_t)(0));
    if ( v3 != 0 )
    {
      g_eng.dsb_volume_5bb870(*v3, F32(CFG(), 360));
      g_eng.dsb_freq_5bb890(*v3, 0);
      g_eng.dsb_play_5bb8c0(*v3);
    }
    F16(c, 0x4070) = 0;
    return 0;
  }
  else
  {
    if ( W_room_item_pick((intptr_t)(c), (intptr_t)(&(*v15__f)), (intptr_t)(9), (intptr_t)(0), (intptr_t)(300)) >= 0
      || F16(c, 0x405c) == 0 && W_room_item_pick((intptr_t)(c), (intptr_t)(&(*v15__f)), (intptr_t)(11), (intptr_t)(6), (intptr_t)(0)) >= 0 )
    {
      goto LABEL_15;
    }
    v5 = W_room_item_pick((intptr_t)(c), (intptr_t)(&(*v15__f)), (intptr_t)(9), (intptr_t)(0), (intptr_t)(750));
    if ( v5 >= 0 )
      goto LABEL_15;
    if ( F16(c, 0x405e) == 0 )
      v5 = W_room_item_pick((intptr_t)(c), (intptr_t)(&(*v15__f)), (intptr_t)(9), (intptr_t)(0), (intptr_t)(0));
    if ( v5 < 0 )
    {
      v6 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(892)), (intptr_t)(0));
      v7 = (intptr_t *)v6;
      if ( v6 != 0 )
      {
        g_eng.dsb_volume_5bb870(*v6, F32(CFG(), 360));
        g_eng.dsb_freq_5bb890(*v7, 0);
        g_eng.dsb_play_5bb8c0(*v7);
      }
      return 0;
    }
    else
    {
LABEL_15:
      F16(c, 0x4070) = 1;
      F32(c, 0x4074) = 250;
      v8 = SYS32(888);
      if ( F32(SYS32(888), 8) != 0
        && (F32(SYS32(888), 4) != 0
         || W_vpool_load((intptr_t)(SYS32(888))) != 0)
        && *v8 != 0 )
      {
        v9 = F32(v8, 4 * 1);
        SETHI(v10, 0);
        while ( 1 )
        {
          SETLO(v10, v8[1]);
          v11 = (v10 + v2) % *v8;
          if ( F32(v9 + 8 * (uint16_t)v11, 4) == 0 )
            break;
          if ( ++v2 >= *v8 )
            return 1;
        }
        v12 = v11 + 1;
        v13 = 8 * (uint16_t)v11;
        v8[1] = v12;
        F32(v13 + v9, 4) = 0;
        v14 = (intptr_t *)(v13 + F32(v8, 4 * 1));
        if ( v14 != 0 )
        {
          g_eng.dsb_volume_5bb870(*v14, F32(CFG(), 360));
          g_eng.dsb_freq_5bb890(*v14, 0);
          g_eng.dsb_play_5bb8c0(*v14);
        }
      }
      return 1;
    }
  }
}

/* 0x590f20: int __thiscall sub_590F20(cls_413810 *this, int a2) */
int32_t fm_590f20(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    uint16_t v3 = 0;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    int32_t * v8 = 0;
    int32_t * v9 = 0;
    uint16_t v10 = 0;
    int32_t * v11 = 0;
    int32_t * v12 = 0;

  v2 = 0;
  if ( a2 != 0 )
  {
    v3 = 0;
    while ( 1 )
    {
      v4 = *(_DWORD **)&GAP(c, 0x4024, 4 * v3 + 12);
      if ( (v4[625] == 1 || v4[625] == 2) && a2 == v4[636] )
        break;
      if ( ++v3 >= 8u )
        goto LABEL_7;
    }
    v5 = v4[639];
  }
  else
  {
LABEL_7:
    v5 = F32(c, 0x4050);
  }
  if ( v5 != 0 )
  {
    v6 = v5 - 1;
    if ( v6 != 0 )
    {
      if ( v6 == 1 )
      {
        if ( a2 != 0 )
        {
          while ( 1 )
          {
            v8 = *(_DWORD **)&GAP(c, 0x4024, 4 * v2 + 12);
            if ( (v8[625] == 1 || v8[625] == 2) && a2 == v8[636] )
              break;
            if ( ++v2 >= 8u )
              goto LABEL_42;
          }
          v8[639] = 1;
          W_room_formation((intptr_t)(c));
          return 1;
        }
        else
        {
          F32(c, 0x4050) = 1;
          W_room_formation((intptr_t)(c));
          return 1;
        }
      }
      if ( a2 == 0 )
      {
LABEL_28:
        F32(c, 0x4050) = 0;
        W_room_formation((intptr_t)(c));
        return 1;
      }
      v10 = 0;
      while ( 1 )
      {
        v11 = *(_DWORD **)&GAP(c, 0x4024, 4 * v10 + 12);
        if ( (v11[625] == 1 || v11[625] == 2) && a2 == v11[636] )
          break;
        if ( ++v10 >= 8u )
          goto LABEL_42;
      }
      v11[639] = 0;
      W_room_formation((intptr_t)(c));
      return 1;
    }
    else
    {
      if ( a2 == 0 )
        goto LABEL_28;
      while ( 1 )
      {
        v9 = *(_DWORD **)&GAP(c, 0x4024, 4 * v2 + 12);
        if ( (v9[625] == 1 || v9[625] == 2) && a2 == v9[636] )
          break;
        if ( ++v2 >= 8u )
          goto LABEL_42;
      }
      v9[639] = 255;
      W_room_formation((intptr_t)(c));
      return 1;
    }
  }
  else if ( a2 != 0 )
  {
    while ( 1 )
    {
      v12 = *(_DWORD **)&GAP(c, 0x4024, 4 * v2 + 12);
      if ( (v12[625] == 1 || v12[625] == 2) && a2 == v12[636] )
        break;
      if ( ++v2 >= 8u )
      {
LABEL_42:
        W_room_formation((intptr_t)(c));
        return 1;
      }
    }
    v12[639] = 2;
    W_room_formation((intptr_t)(c));
    return 1;
  }
  else
  {
    F32(c, 0x4050) = 2;
    W_room_formation((intptr_t)(c));
    return 1;
  }
}

/* 0x590ca0: int __thiscall sub_590CA0(cls_4E6870 *this) */
int32_t fm_590ca0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    int16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int v10 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    int16_t *const v13__f = (int16_t *)(frame_ + 0x14);   /* ebp-0x18 */
    int16_t *const v14__f = (int16_t *)(frame_ + 0x18);   /* ebp-0x14 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    uint16_t *const v17__f = (uint16_t *)(frame_ + 0x24);   /* ebp-0x8 */
    char * *const v18__f = (char * *)(frame_ + 0x28);   /* ebp-0x4 */

  v1 = c;
  v2 = 3;
  switch ( F32(c, 0x4054) )
  {
    case 0:
      v2 = 1;
      break;
    case 1:
      v2 = 2;
      break;
    case 2:
      v2 = 3;
      break;
    case 3:
      v2 = 4;
      break;
    case 4:
      v2 = 0;
      break;
    default:
      break;
  }
  v3 = F32(c, 0x200c);
  F32(c, 0x4054) = v2;
  (*v16__f) = 0;
  if ( v3 != 0 )
    (*v16__f) = F32(v3, 2552);
  (*v17__f) = 0;
  (*v18__f) = &GAP(c, 0x2010, 8224);
  v4 = (uint16_t)c + 16432;
  do
  {
    v5 = F32((*v18__f), 0);
    if ( F32(F32((*v18__f), 0), 2500) == 1 && F32(v5, 2548) != 0 )
    {
      switch ( W_rd_4961e0((intptr_t)(v1), (intptr_t)(F32(v5, 2544)), (intptr_t)(1)) )
      {
        case 0:
          v4 = 0;
          (*v13__f) = 0;
          (*v14__f) = 0;
          break;
        case 1:
          v4 = 0;
          (*v13__f) = 2;
          (*v14__f) = 0;
          break;
        case 2:
          v4 = 1;
          (*v13__f) = 2;
          (*v14__f) = 0;
          break;
        case 3:
          v4 = 2;
          (*v14__f) = 1;
          (*v13__f) = 2;
          break;
        case 4:
          v4 = 4;
          (*v13__f) = 2;
          (*v14__f) = 3;
          break;
        default:
          break;
      }
      F16(F32(v5, 2548), 84008) = v4;
      F16(F32(v5, 2548), 84010) = (*v13__f);
      F16(F32(v5, 2548), 84012) = (*v14__f);
      if ( v5 == v3 || v3 == 0 )
      {
        W_char_follow_set((intptr_t)(F32(v5, 2548)), (intptr_t)(2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      }
      else
      {
        v6 = W_rd_496240((intptr_t)(v1), (intptr_t)(F32(v5, 2544)), (intptr_t)(1));
        v7 = 4800 * W_mh_4e6870((intptr_t)(c), (intptr_t)((*v17__f)), (intptr_t)(v5), (intptr_t)(v3));
        if ( F32(v5, 2544) == 100000104 && v6 == 2 )
          v6 = 0;
        v8 = v6 - 1;
        if ( v8 != 0 )
        {
          (*v12__f) = v7;
          v9 = F32(v5, 2552);
          if ( v8 == 1 )
            W_char_follow_set((intptr_t)(F32(v5, 2548)), (intptr_t)(4), (intptr_t)(F32(v3, 2548)), (intptr_t)(4000 * (v9 - (*v16__f))), (intptr_t)((*v12__f)));
          else
            W_char_follow_set((intptr_t)(F32(v5, 2548)), (intptr_t)(3), (intptr_t)(F32(v3, 2548)), (intptr_t)(4000 * (v9 - (*v16__f))), (intptr_t)((*v12__f)));
          v1 = c;
        }
        else
        {
          W_char_follow_set((intptr_t)(F32(v5, 2548)), (intptr_t)(1), (intptr_t)(F32(v3, 2548)), (intptr_t)(0), (intptr_t)(v7));
          v1 = c;
        }
      }
    }
    v10 = ++(*v17__f) < 8u;
    (*v18__f) += 4;
  }
  while ( v10 );
  return 1;
}

/* 0x59d540: int __thiscall sub_59D540(cls_59D540 *this, int a2, int a3) */
int32_t fm_59d540(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t * v4 = 0;
    intptr_t i = 0;
    uint16_t j = 0;
    int32_t * v7 = 0;
    intptr_t k = 0;
    intptr_t v9 = 0;
    intptr_t m = 0;
    intptr_t v11 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;

  W_id_5ba4c0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  W_id_5ba2d0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  v4 = (intptr_t *)&SYSB(10364);
  for ( i = 2; i != 0; --i )
  {
    if ( *v4 != 0 )
    {
      W_id_5ba650((intptr_t)(*v4));
      W_en_49bf90((intptr_t)(*v4), (intptr_t)(1000 * F32(CFG(), 1340) / 1000));
    }
    ++v4;
  }
  for ( j = 0; j < F16(((uint8_t *)(c) + 0x4), 0); ++j )
    W_room_input_flush((intptr_t)((F32(c, 0x0) + 16672 * j)));
  v7 = (int32_t *)c + 74;
  for ( k = 2; k != 0; --k )
  {
    *v7 = 0;
    v7 += 251;
  }
  v9 = 4448;
  for ( m = 32; m != 0; --m )
  {
    v11 = *(_DWORD *)&GAP(W, 0x0, v9);
    if ( F32(v11, 464) != 0 )
    {
      F32(v11, 84056) = 0;
      F32(v11, 84052) = 0;
      F32(v11, 84060) = 0;
      F32(v11, 84064) = 0;
      F16(v11, 84072) = 0;
      F32(v11, 84068) = 0;
      F16(v11, 84074) = 0;
      F32(v11, 84076) = 0;
      F16(v11, 84080) = 0;
    }
    v9 += 4;
  }
  v13 = F32(F32(W, 0x2784), 4);
  v14 = F32(c, 4 * 58);
  F32(a3, 28) = 0;
  F32(a3, 32) = 0;
  F32(a3, 36) = 0;
  F32(a3, 40) = 0;
  F16(a3, 24) = 0;
  F32(a3, 48) = 0;
  F32(a3, 0) = v14;
  F32(a3, 4) = c;
  F32(a3, 12) = a2;
  F32(a3, 8) = v13;
  F32(a3, 16) = 1;
  return a3;
}

/* 0x59ad60: int __thiscall sub_59AD60(cls_59AD60 *this, int a2) */
int32_t fm_59ad60(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    char * v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t * v12 = 0;
    intptr_t j = 0;
    uint16_t k = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t * v19 = 0;
    uint16_t v20 = 0;
    int32_t * v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    intptr_t v27 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *const v24__f = (int16_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */

  v2 = 0;
  (*v24__f) = 0;
  v3 = &GAP(W, 0x1030, 304);
  for ( (*i__f) = 32; (*i__f) != 0; --(*i__f) )
  {
    v4 = F32(v3, 0);
    v5 = F32(F32(v3, 0), 464);
    if ( v5 != 0 )
    {
      v6 = F32(a2, 476);
      v7 = F32(v4, 476);
      if ( v6 != 0 && v7 != 0 && (v6 == 3 || v7 == 3 || v6 != v7) )
      {
        if ( v2 == 0 )
          v2 = F32(v3, 0);
        if ( v5 == 1 && F8(F32(v4, 64), 0) == 1 && F16(F32(v4, 1872), 1098) == 0 )
          ++(*v24__f);
      }
    }
    v3 += 4;
  }
  if ( (*v24__f) != 0 )
    return 0;
  v8 = F16(c, 0x22);
  if ( v8 != 0 )
  {
    F16(c, 0x22) = v8 - 1;
    return 0;
  }
  F32(W, 0x2764) = 0;
  F32(W, 0x2768) = 0;
  v10 = *(intptr_t *)&GAP(W, 0x1030, 36);
  F32(v10, 0x8) = F32(W, 0x2784);
  F32(v10, 0x0) = (int)c;
  F32(v10, 0x4) = v2;
  F32(v10, 0x10) = 0;
  F32(v10, 0x14) = 6;
  F32(v10, 0xc) = 0;
  v11 = W_script_engine((intptr_t)(v10));
  if ( v11 != 0 )
  {
    v27 = v11;
    if ( v11 == 6 )
      return 6;
  }
  else
  {
    if ( F32(v10, 0xc) == 0 )
      return 0;
    if ( SYS32(2728) != 0 )
    {
      v19 = (uint16_t *)(SYS32(2728) + 4);
      v20 = 0;
      if ( F16(SYS32(2728), 4) != 0 )
      {
        v21 = (_DWORD *)(SYS32(2728) + 8);
        do
        {
          v22 = 24 * v20;
          v23 = (_DWORD *)(v22 + *v21);
          if ( *v23 != 0 && (int)v23[4] < 3 )
          {
            v23[4] = 3;
            F32(*v21 + v22, 20) = 400;
          }
          ++v20;
        }
        while ( v20 < *v19 );
      }
    }
    API_OutputDebugStringA((intptr_t)(((char *)D(0x854798))));
    W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(4u));
    v27 = 24;
  }
  W_id_5ba4c0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  W_id_5ba2d0((intptr_t)(*(intptr_t *)&SYSB(10360)));
  v12 = (intptr_t *)&SYSB(10364);
  for ( j = 2; j != 0; --j )
  {
    if ( *v12 != 0 )
    {
      W_id_5ba650((intptr_t)(*v12));
      W_en_49bf90((intptr_t)(*v12), (intptr_t)(1000 * F32(CFG(), 1340) / 1000));
    }
    ++v12;
  }
  for ( k = 0; k < F16(c, 0x4); ++k )
  {
    v15 = 8;
    v16 = F32(c, 0x0) + 16672 * k + 16432;
    do
    {
      v17 = F32(v16, 0);
      if ( F32(F32(v16, 0), 2500) == 1 && F32(v17, 2544) != 100000104 )
      {
        v18 = F32(v17, 0);
        if ( v18 != 0 )
          W_tm_56a220((intptr_t)(v18));
      }
      v16 += 4;
      --v15;
    }
    while ( v15 != 0 );
  }
  return v27;
}
/* unported callees: none */
