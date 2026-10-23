/* save_game.c — SAVING and LOADING: the save statue's dialog 0x41a430 ("Would you like to save?" / "Overwrite this
 * save?" / "Game saved"), the slot list 0x41bd20 / 0x41bf60 / 0x561560 (+ 0x561bf0 / 0x561cf0: "Save Point", "LV",
 * the play time, the date), the WRITER 0x41a890 (the room snapshot into memory bank 0 / 1 at every room start, a
 * slot file savedataNN.sdt / .dat / .bak) and the READER 0x41b670, the snapshot prep 0x55f620, the APPLY 0x561d20 /
 * 0x560530 (a loaded snapshot back into the scene), the file name 0x41cfd0.  See save_game.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals).  Hand finish (0x41a890): the four copies of
 * retail's logger 0x406440 the compiler inlined into the failure paths are sv_last_error / sv_fatal below (the same
 * calls: the error flag + message, the log line through retail's own 0x406440, exit(0)); `*a5` (a __time32_t *);
 * 0x55f620 returns 0x55f790's whole eax (IDA: LOWORD).  Scripted: tools/ida/finish/save_game.py.
 * The writer's 520 never-filled header bytes (quirk #161) are zero here and in retail (rhost's uninit-fix site E
 * zeroes its frame).  Retail's own (RC_: PORT-DEBT(engine-call-slots)): the file stream layer 0x5b6060..0x5b6bc0
 * (CreateFile / the bank copies), the Yes / No box 0x4190d0, the backup copy 0x41b470, the input re-poll 0x40fe00. */
#include "save_game.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))

#include "char_ai.h"
#include "gimmick.h"
#include "party_member.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "char_ai.h"
#include "gimmick.h"
#include "party_member.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "char_ai.h"
#include "gimmick.h"
#include "party_member.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "char_ai.h"
#include "gimmick.h"
#include "party_member.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "char_ai.h"
#include "gimmick.h"
#include "party_member.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "char_ai.h"
#include "engine_rest.h"
#include "file_stream.h"
#include "gimmick.h"
#include "menu_helpers.h"
#include "party_member.h"
#include "room_loop.h"
#include "scene_cmd.h"
#include "scene_loop.h"
#include "script_engine.h"
#include "script_helpers.h"
#include "title_extra.h"
#include "title_menu.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "win_main.h"
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

static inline intptr_t W_area_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)area_find((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_area_level_row(intptr_t a0, ...) { return (intptr_t)area_level_row((int32_t)a0); }
static inline intptr_t W_cmd_clear(intptr_t a0, ...) { cmd_clear((wl_cmd *)a0); return 0; }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_el_41b470(intptr_t a0, intptr_t a1, ...) { return (intptr_t)el_41b470((int32_t)a0, (uint16_t)a1); }
static inline intptr_t W_file_read_section(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)file_read_section((void *)a0, (void *)a1, (int32_t)a2, (uint32_t)a3); }
static inline intptr_t W_flag_find(intptr_t a0, intptr_t a1, ...) { return (intptr_t)flag_find((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_fs_5b6060(intptr_t a0, ...) { return (intptr_t)fs_5b6060((wl_char *)a0); }
static inline intptr_t W_fs_5b60d0(intptr_t a0, ...) { return (intptr_t)fs_5b60d0((wl_char *)a0); }
static inline intptr_t W_fs_5b60e0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b60e0((wl_char *)a0, (char *)a1, (int32_t)a2); }
static inline intptr_t W_fs_5b61b0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b61b0((wl_char *)a0, (char *)a1); }
static inline intptr_t W_fs_5b6200(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b6200((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_fs_5b6250(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b6250((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_fs_5b6340(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b6340((wl_char *)a0, (int32_t)a1, (uint32_t)a2); }
static inline intptr_t W_fs_5b6410(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b6410((wl_char *)a0, (int32_t)a1, (uint32_t)a2); }
static inline intptr_t W_fs_5b64b0(intptr_t a0, ...) { return (intptr_t)fs_5b64b0((wl_char *)a0); }
static inline intptr_t W_fs_5b6990(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b6990((wl_char *)a0, (char *)a1, (int32_t)a2); }
static inline intptr_t W_fs_5b6bc0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b6bc0((wl_char *)a0, (char *)a1); }
static inline intptr_t W_glyph_recs_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { glyph_recs_set((uint8_t *)a0, (const char *)a1, (int32_t)a2); return 0; }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_col_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)grid_col_place((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_grid_row_reset(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_row_reset((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_item_kind(intptr_t a0, ...) { return (intptr_t)item_kind((const uint8_t *)a0); }
static inline intptr_t W_member_palette(intptr_t a0, ...) { member_palette((wl_char *)a0); return 0; }
static inline intptr_t W_menu_ctx_clear(intptr_t a0, ...) { menu_ctx_clear((void *)a0); return 0; }
static inline intptr_t W_menu_run(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)menu_run((uint8_t *)a0, (uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_menu_sprites_free(intptr_t a0, ...) { return (intptr_t)menu_sprites_free((uint8_t *)a0); }
static inline intptr_t W_mh_4190d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)mh_4190d0((wl_char *)a0, (char *)a1, (uint16_t)a2, (int32_t *)a3, (int32_t)a4); }
static inline intptr_t W_msg_pages(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)msg_pages((uint8_t *)a0, (const char *)a1, (const char *)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (uint16_t)a7, (uint16_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_pslot_input_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)pslot_input_set((wl_pslot *)a0, (void *)a1); }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_pslot_reset(intptr_t a0, ...) { pslot_reset((wl_pslot *)a0); return 0; }
static inline intptr_t W_room_delete(intptr_t a0, intptr_t a1, ...) { room_delete((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_flag_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_flag_set((wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_input_flush(intptr_t a0, ...) { room_input_flush((wl_room *)a0); return 0; }
static inline intptr_t W_room_inputs_alloc(intptr_t a0, intptr_t a1, ...) { room_inputs_alloc((wl_room *)a0, (uint16_t)a1); return 0; }
static inline intptr_t W_room_items_grant(intptr_t a0, intptr_t a1, ...) { room_items_grant((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_room_items_of_kind(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_items_of_kind((const wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_party_defaults(intptr_t a0, ...) { room_party_defaults((wl_room *)a0); return 0; }
static inline intptr_t W_room_to_scene(intptr_t a0, intptr_t a1, ...) { return (intptr_t)room_to_scene((const wl_room *)a0, (uint8_t *)a1); }
static inline intptr_t W_sc_4a63d0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sc_4a63d0((wl_char *)a0, (uint32_t)a1); }
static inline intptr_t W_scene_msg(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { scene_msg((wl_event *)a0, (uint32_t)a1, (void *)a2, (const char *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (const char *)a7, (int32_t)a8, (int32_t)a9, (uint16_t)a10); return 0; }
static inline intptr_t W_scene_obj_init(intptr_t a0, ...) { return (intptr_t)scene_obj_init((uint8_t *)a0); }
static inline intptr_t W_scene_yes_no(intptr_t a0, ...) { return (intptr_t)scene_yes_no((wl_event *)a0); }
static inline intptr_t W_script_wait(intptr_t a0, ...) { return (intptr_t)script_wait((wl_event *)a0); }
static inline intptr_t W_stats_recompute(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { stats_recompute((wl_room *)a0, (int32_t)a1, (uint8_t *)a2, (const uint8_t *)a3); return 0; }
static inline intptr_t W_sys_frame_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_frame_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_page_arrows(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_page_arrows((void *)a0, (const uint8_t *)a1, (uint8_t *)a2); }
static inline intptr_t W_tm_419900(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_419900((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_41bb80(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41bb80((wl_char *)a0, (uint16_t)a1, (int32_t)a2); }
static inline intptr_t W_tm_41bbe0(intptr_t a0, ...) { tm_41bbe0((wl_char *)a0); return 0; }
static inline intptr_t W_tm_41c150(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c150((int32_t *)a0, (int32_t *)a1, (int32_t)a2); }
static inline intptr_t W_tm_41c240(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tm_41c240((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_tm_41c2f0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c2f0((wl_char *)a0, (int32_t *)a1, (uint16_t *)a2); }
static inline intptr_t W_tm_41c320(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)tm_41c320((wl_char *)a0, (int32_t *)a1, (uint16_t)a2); }
static inline intptr_t W_tm_41d780(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)tm_41d780((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_tm_560a40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)tm_560a40((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_ts_40fe00(intptr_t a0, ...) { return (intptr_t)ts_40fe00((wl_char *)a0); }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_ctrl_to_back(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_ctrl_to_back((uint8_t *)a0, (uint16_t)a1); }
static inline intptr_t W_win_mgr_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_mgr_open((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (const void *)a6); }
static inline intptr_t W_win_title_add(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_title_add((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_wn_560900(intptr_t a0, ...) { return (intptr_t)wn_560900((wl_char *)a0); }

typedef uint32_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLocalTime_t)(intptr_t);
#define API_GetLocalTime ((api_GetLocalTime_t)g_eng.api("GetLocalTime"))
typedef uint32_t (__attribute__((stdcall)) *api_GetTickCount_t)(void);
#define API_GetTickCount ((api_GetTickCount_t)g_eng.api("GetTickCount"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fprintf ((intptr_t (*)())g_eng.retail(0x5bf496u))   /* retail's CRT fprintf (cdecl) */
#define CRT_localtime ((intptr_t (*)())g_eng.retail(0x5c00ccu))   /* retail's CRT localtime (cdecl) */
#define CRT_strftime ((intptr_t (*)())g_eng.retail(0x5bf865u))   /* retail's CRT strftime (cdecl) */
#define CRT_time ((intptr_t (*)())g_eng.retail(0x5bf6dfu))   /* retail's CRT time (cdecl) */
int32_t sv_41a430(wl_char *self, int32_t a2, int32_t a3);
int32_t sv_41a890(wl_char *self, int32_t a2, uint16_t a3, int32_t a4, int32_t a5);
int32_t sv_41b670(wl_char *self, int32_t a2, uint16_t a3, int32_t * a4);
int32_t sv_41bd20(intptr_t ecx_, int32_t a1, int32_t a2);
void sv_41bf60(int32_t * a1, int32_t a2);
int32_t sv_55f620(wl_char *self, int32_t a2, int32_t a3);
void sv_561560(int32_t a1, int32_t a2, uint16_t a3, int32_t a4, char * a5, int32_t a6);
int32_t sv_561bf0(wl_char *self, int32_t a2);
int32_t sv_561cf0(wl_char *self, int32_t a2);
int32_t sv_561d20(wl_char *self, int32_t a2);
int32_t sv_560530(wl_char *self, int32_t a2);
int32_t sv_41cfd0(int32_t a1, uint16_t a2, char * a3, int32_t * a4);

/* retail's inline error blocks (its logger 0x406440 inlined): the last Win32 error, "[%08d] %s", logged */
static void sv_last_error(char *buf, char *out)
{
    uint32_t e = (uint32_t)API_GetLastError();
    if (e != 0) {
        API_FormatMessageA(0x12FF, 0, (intptr_t)e, 0, (intptr_t)buf, 0xFF, 0);
        g_eng.h5bf3ee(out, "[%08d] %s", e, buf);
        g_eng.log_fail_406440((void *)D(0x8a9530), out, (char *)D(0x8a9b6c));
    }
}

/* ... and the fatal one: the error flag + message (log +0 / +4: "\n"-joined), the log line, exit(0) */
static void sv_fatal(const char *msg, const char *detail)
{
    uint8_t *log = (uint8_t *)D(0x8a9530);
    char *m = (char *)log + 4;
    F32(log, 0) = 1;
    if (strlen(m) != 0)
        strcat(m, (char *)D(0x854570));
    strcat(m, msg);
    strcat(m, detail);
    g_eng.log_fail_406440(log, msg, detail);
    g_eng.h5bf5db(0);
}

/* 0x41a430: int __thiscall sub_41A430(cls_401D40 *this, int a2, int a3) */
int32_t sv_41a430(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t ExceptionList = 0;
    void * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t i = 0;
    int32_t * v8 = 0;
    intptr_t j = 0;
    intptr_t v10 = 0;
    intptr_t k = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    int32_t * v24 = 0;
    _Alignas(8) uint8_t frame_[0xe2e4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xe2e4 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xe2e0 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xe2ac */
    intptr_t *const Buffer__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0xe29c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0xe2d8);   /* ebp-0xc */
    void * *const v32__f = (void * *)(frame_ + 0xe2dc);   /* ebp-0x8 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0xe2e0);   /* ebp-0x4 */

  (*v33__f) = -1;
  memset(&(*v28__f), 0, 16);
  memset(&F32(v28__f, 0x1c), 0, 16);
  *(_WORD *)&GAP(v28__f, 0x10, 8) = 0;
  *(_DWORD *)&F16(v28__f, 0x30) = 0;
  v5 = F32(c, 0x8);
  v6 = F32(c, 0x0);
  (*v33__f) = 0;
  W_ts_40fe00((intptr_t)((intptr_t *)((intptr_t)g_eng.sys)));
  for ( i = 0; i < F16(v6, 4); ++i )
    W_room_input_flush((intptr_t)((F32(v6, 0) + 16672 * i)));
  v8 = (_DWORD *)(v6 + 296);
  for ( j = 2; j != 0; --j )
  {
    *v8 = 0;
    v8 += 251;
  }
  v10 = 4448;
  for ( k = 32; k != 0; --k )
  {
    v12 = *(_DWORD *)&GAP(W, 0x0, v10);
    if ( F32(v12, 464) != 0 )
      W_cmd_clear((intptr_t)(v12 + 84052));
    v10 += 4;
  }
  v13 = F32(v6, 232);
  v14 = F32(F32(W, 0x2784), 4);
  W_menu_ctx_clear((intptr_t)(&(*v28__f)));
  F32(v28__f, 0x0) = v13;
  F32(v28__f, 0x4) = v6;
  F32(v28__f, 0xc) = v5;
  F32(v28__f, 0x8) = v14;
  F32(((uint8_t *)(v28__f) + 0x10), 0) = 1;
  while ( 2 )
  {
    while ( 2 )
    {
      v15 = 0;
      if ( a3 != 0 )
      {
        v16 = c;
        W_scene_msg((intptr_t)(c), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(((char *)D(0x854eb4))), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v17 = W_script_wait((intptr_t)(c));
        if ( v17 == 6 )
          goto LABEL_43;
      }
      else
      {
        W_scene_msg((intptr_t)(c), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(((char *)D(0x854e9c))), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v17 = W_script_wait((intptr_t)(c));
        if ( v17 == 6 )
          goto LABEL_43;
        v16 = c;
      }
      v18 = W_scene_yes_no((intptr_t)(v16));
      if ( v18 != 14 )
      {
        if ( v18 == 15 )
        {
          if ( a3 == 0 )
            goto LABEL_43;
          W_scene_msg((intptr_t)(v16), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(((char *)D(0x854e50))), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          v17 = W_script_wait((intptr_t)(v16));
          if ( v17 == 6 )
            goto LABEL_43;
          v18 = W_scene_yes_no((intptr_t)(v16));
          if ( v18 == 14 )
            goto LABEL_43;
          if ( v18 == 15 )
            continue;
        }
        v17 = v18;
        goto LABEL_43;
      }
      break;
    }
    F32(v29__f, 0x0) = 0;
    F32(v29__f, 0x4) = 0;
    SETLB((*v33__f), 1);
    W_scene_obj_init((intptr_t)(&(*Buffer__f)));
    (*v27__f) = sv_41bd20((intptr_t)v29__f, F32(v28__f, 0x0), 1);
    v19 = F32((F32((*v27__f), 72) + 4 * F16((*v27__f), 78) - 4), 0);
    while ( 1 )
    {
      v20 = W_menu_run((intptr_t)(&(*v28__f)), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( v20 == 11 )
        break;
      if ( v20 == 12 )
      {
        v21 = v19[95];
        v22 = 16 * F32(v19[93], 20);
        v23 = F32(v22 + v21, 4);
        v24 = F32(v22, v21);
        if ( sv_41b670(&(*Buffer__f), (int)v24, v23, 0) == 0 )
          goto LABEL_32;
        v20 = W_mh_4190d0((intptr_t)(&(*v28__f)), (intptr_t)(((char *)D(0x854e38))), (intptr_t)(0x28u), (intptr_t)((_DWORD *)1), (intptr_t)(0));
        if ( v20 == 14 )
        {
          W_el_41b470((intptr_t)((int)v24), (intptr_t)(v23));
LABEL_32:
          sv_55f620(&(*v28__f), (int)&(*Buffer__f), a2);
          if ( sv_41a890(&(*Buffer__f), (int)v24, v23, 1, 0) != 0 )
          {
            sv_41bf60((int)v19, 1);
            v20 = W_msg_pages((intptr_t)(&(*v28__f)), (intptr_t)(((char *)D(0x854e00))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(128), (intptr_t)((uint16_t)v23 < 4u ? 320 : 80), (intptr_t)(0), (intptr_t)(0x28u), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
            if ( v20 == 0 )
            {
              W_tm_41c320((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(v24), (intptr_t)(v23));
              W_tm_41bb80((intptr_t)(SYS32(800)), (intptr_t)(0x5Eu), (intptr_t)(2));
              v15 = 12;
              break;
            }
LABEL_37:
            v15 = v20;
            break;
          }
          v20 = W_msg_pages((intptr_t)(&(*v28__f)), (intptr_t)(((char *)D(0x854e0c))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
          if ( v20 != 0 )
            goto LABEL_37;
        }
        else if ( v20 != 15 )
        {
          goto LABEL_37;
        }
      }
      else if ( v20 != 13 )
      {
        goto LABEL_37;
      }
    }
    SETLB((*v33__f), 0);
    F32((*v27__f), 80) = 0;
    W_tm_41bbe0((intptr_t)(&(*v29__f)));
    if ( v15 != 6 && v15 != 12 )
      continue;
    break;
  }
  v17 = v15;
LABEL_43:
  (*v33__f) = -1;
  W_menu_sprites_free((intptr_t)(&(*v28__f)));
  return v17;
}

/* 0x41a890: int __thiscall sub_41A890(cls_41A890 *this, int a2, unsigned __int16 a3, int a4, __time32_t *a5) */
int32_t sv_41a890(wl_char *self, int32_t a2, uint16_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    uint32_t v7 = 0;
    uint32_t LastError = 0;
    uint32_t v9 = 0;
    FILE * v10 = 0;
    uint32_t v11 = 0;
    char * v12 = 0;
    char * v13 = 0;
    char * v14 = 0;
    char v15 = 0;
    uint32_t v16 = 0;
    FILE * v17 = 0;
    uint32_t v18 = 0;
    uint32_t v19 = 0;
    uint32_t v20 = 0;
    char * v21 = 0;
    char * v22 = 0;
    char * v23 = 0;
    char v24 = 0;
    uint32_t v25 = 0;
    FILE * v26 = 0;
    uint16_t i = 0;
    int v28 = 0;
    intptr_t v29 = 0;
    uint16_t * v30 = 0;
    uint16_t v31 = 0;
    intptr_t v32 = 0;
    uint16_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    uint16_t k = 0;
    FILE * v41 = 0;
    uint32_t v43 = 0;
    uint32_t v44 = 0;
    char * v45 = 0;
    char * v46 = 0;
    char * v47 = 0;
    char v48 = 0;
    uint32_t v49 = 0;
    FILE * v50 = 0;
    _Alignas(8) uint8_t frame_[0xba4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v72 = (char *)(frame_ + 0x25c);   /* [32] at ebp-0x948 */
    char *FileName = (char *)(frame_ + 0x284);   /* [504] at ebp-0x920 */
    char *OutputString = (char *)(frame_ + 0x484);   /* [248] at ebp-0x720 */
    char *Buffer = (char *)(frame_ + 0x584);   /* [512] at ebp-0x620 */
    char *v77 = (char *)(frame_ + 0x798);   /* [1024] at ebp-0x40c */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0xba4 */
    uint16_t *const v52__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0xb94 */
    uint16_t *const v53__f = (uint16_t *)(frame_ + 0x12);   /* ebp-0xb92 */
    intptr_t *const v54__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xb90 */
    uint32_t *const nNumberOfBytesToWrite__f = (uint32_t *)(frame_ + 0x18);   /* ebp-0xb8c */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0xb88 */
    intptr_t *const v57__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xb84 */
    intptr_t *const Time__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0xb80 */
    uint32_t *const v59__f = (uint32_t *)(frame_ + 0x28);   /* ebp-0xb7c */
    uint32_t *const v60__f = (uint32_t *)(frame_ + 0x2c);   /* ebp-0xb78 */
    uint32_t *const v61__f = (uint32_t *)(frame_ + 0x30);   /* ebp-0xb74 */
    uint32_t *const v62__f = (uint32_t *)(frame_ + 0x34);   /* ebp-0xb70 */
    uint32_t *const v63__f = (uint32_t *)(frame_ + 0x38);   /* ebp-0xb6c */
    uint32_t *const v64__f = (uint32_t *)(frame_ + 0x3c);   /* ebp-0xb68 */
    uint32_t *const v65__f = (uint32_t *)(frame_ + 0x40);   /* ebp-0xb64 */
    uint32_t *const v66__f = (uint32_t *)(frame_ + 0x44);   /* ebp-0xb60 */
    uint32_t *const v67__f = (uint32_t *)(frame_ + 0x248);   /* ebp-0x95c */
    uint32_t *const v68__f = (uint32_t *)(frame_ + 0x24c);   /* ebp-0x958 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x250);   /* ebp-0x954 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x254);   /* ebp-0x950 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x258);   /* ebp-0x94c */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x784);   /* ebp-0x420 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0xba0);   /* ebp-0x4 */

  W_fs_5b6060((intptr_t)(&(*v76__f)));
  v6 = a2;
  (*v78__f) = 0;
  v7 = 9999;
  FileName[0] = 0;
  if ( a2 <= 10020 )
  {
    switch ( a2 )
    {
      case 10020:
        v7 = 0;
        goto LABEL_12;
      case 1:
        g_eng.h5bf3ee(Buffer, "savedata%02d.bak", a3);
        goto LABEL_10;
      case 10010:
        g_eng.h5bf3ee(Buffer, "savedata%02d.dat", a3);
LABEL_10:
        strcpy(FileName, (const char *)&SYSB(259));
        strcat(FileName, ((char *)D(0x854f5c)));
        strcat(FileName, Buffer);
        v6 = a2;
        goto LABEL_12;
      default:
        break;
    }
LABEL_9:
    g_eng.h5bf3ee(Buffer, "savedata%02d.sdt", a3);
    v7 = 2;
    goto LABEL_10;
  }
  if ( a2 != 10030 )
    goto LABEL_9;
  v7 = 1;
LABEL_12:
  if ( &(*nNumberOfBytesToWrite__f) == 0 )
    v7 = (*nNumberOfBytesToWrite__f);
  if ( v7 == 9999 )
  {
    if ( W_fs_5b60e0((intptr_t)(&(*v76__f)), (intptr_t)(FileName), (intptr_t)(20000)) == 0 )
    {
      sv_last_error(Buffer, OutputString);
      sv_fatal((char *)D(0x854ef4), FileName);   /* "Unable to write files, cannot save game. ..." */
    }
  }
  else if ( W_fs_5b6250((intptr_t)(&(*v76__f)), (intptr_t)((int)F32(D(0x8a93f0), 4 * (v7)))) == 0 )
  {
    sv_last_error(Buffer, OutputString);
    sv_fatal((char *)D(0x854ee0), v77);   /* "Disk Logic Error" */
  }
  (*nNumberOfBytesToWrite__f) = 604;
  W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&(*nNumberOfBytesToWrite__f)), (intptr_t)(4u));
  (*v71__f) = v6;
  (*v70__f) = 10001;
  strcpy(v72, (const char *)&SYSB(804));
  (*v59__f) = 164;
  (*v60__f) = 16;
  (*v68__f) = 16;
  (*v61__f) = 284;
  (*v63__f) = 8;
  (*v66__f) = 8;
  (*v62__f) = 12;
  (*v64__f) = 12;
  (*v67__f) = 12;
  (*v65__f) = 2;
  CRT_time(&(*Time__f));
  if ( a5 != 0 )
    (*v69__f) = F32(a5, 0);   /* a5: a __time32_t * */
  else
    (*v69__f) = (*Time__f);
  W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&(*v59__f)), (intptr_t)((*nNumberOfBytesToWrite__f)));
  W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(c), (intptr_t)((*v59__f)));
  for ( i = 0; i < *(_WORD *)&GAP(c, 0x6, 2); ++i )
    W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&GAP(c, 0x10, 16 * i + 148)), (intptr_t)((*v60__f)));
  v28 = F16(c, 0x0) == 0;
  (*v57__f) = 0;
  if ( !v28 )
  {
    do
    {
      v29 = (uint16_t)(*v57__f);
      v30 = (_WORD *)((uint8_t *)c + 284 * (uint16_t)(*v57__f));
      W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(v30 + 4178), (intptr_t)((*v61__f)));
      v31 = 0;
      if ( v30[4218] != 0 )
      {
        v32 = 88 * v29;
        for ( (*j__f) = 88 * v29; ; v32 = (*j__f) )
        {
          W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&GAP(c, 0x12a8, 12 * v32 + 5852 + 12 * v31++)), (intptr_t)((*v62__f)));
          if ( v31 >= v30[4218] )
            break;
        }
      }
      v33 = 0;
      if ( v30[4190] != 0 )
      {
        v34 = 66 * v29;
        for ( (*j__f) = 66 * v29; ; v34 = (*j__f) )
        {
          W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&GAP(c, 0x12a8, 16 * v34 + 6620 + 16 * v33++)), (intptr_t)((*v68__f)));
          if ( v33 >= v30[4190] )
            break;
        }
      }
    }
    while ( (uint16_t)++(*v57__f) < F16(c, 0x0) );
  }
  W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&GAP(c, 0x12a8, 14300)), (intptr_t)((*v63__f)));
  v36 = 0;
  if ( *(_DWORD *)&GAP(c, 0x12a8, 14300) != 0 )
  {
    v37 = 0;
    do
    {
      W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)(&GAP(c, 0x12a8, 12 * v37 + 14308)), (intptr_t)((*v64__f)));
      v37 = (uint16_t)++v36;
    }
    while ( (unsigned int)(uint16_t)v36 < *(_DWORD *)&GAP(c, 0x12a8, 14300) );
  }
  v38 = 0;
  if ( *(_DWORD *)&GAP(c, 0x12a8, 14304) != 0 )
  {
    v39 = 0;
    do
    {
      W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)((uint8_t *)c + 12 * v39 + 25228), (intptr_t)((*v67__f)));
      v39 = (uint16_t)++v38;
    }
    while ( (unsigned int)(uint16_t)v38 < *(_DWORD *)&GAP(c, 0x12a8, 14304) );
  }
  W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)((uint8_t *)c + 49804), (intptr_t)((*v65__f)));
  for ( k = 0; k < F16(c, 2 * 24902); ++k )
    W_fs_5b6410((intptr_t)(&(*v76__f)), (intptr_t)((uint8_t *)c + 8 * k + 49808), (intptr_t)((*v66__f)));
  W_fs_5b64b0((intptr_t)(&(*v76__f)));
  if ( a2 != 10040 || W_fs_5b6990((intptr_t)(F32(D(0x8a93f8), 0)), (intptr_t)(FileName), (intptr_t)(5000)) != 0 )
  {
    (*v78__f) = -1;
    W_fs_5b60d0((intptr_t)(&(*v76__f)));
    return 1;
  }
  else
  {
    if ( a4 == 0 )
    {
      sv_last_error(Buffer, OutputString);
      sv_fatal((char *)D(0x854ef4), FileName);
    }
    g_eng.log_fail_406440((void *)D(0x8a9530), (char *)D(0x854ef4), FileName);   /* (0x406440 inlined in retail) */
    (*v78__f) = -1;
    W_fs_5b60d0((intptr_t)(&(*v76__f)));
    return 0;
  }
}

/* 0x41b670: int __thiscall sub_41B670(cls_41B670 *this, int a2, unsigned __int16 a3, _DWORD *a4) */
int32_t sv_41b670(wl_char *self, int32_t a2, uint16_t a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t i = 0;
    int v10 = 0;
    intptr_t v11 = 0;
    uint16_t * v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint16_t k = 0;
    _Alignas(8) uint8_t frame_[0xa88 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *FileName = (char *)(frame_ + 0xc);   /* [512] at ebp-0xa7c */
    uint32_t *v27 = (uint32_t *)(frame_ + 0x20c);   /* [151] at ebp-0x87c */
    intptr_t *v28 = (intptr_t *)(frame_ + 0x468);   /* [52] at ebp-0x620 */
    char *Buffer = (char *)(frame_ + 0x87c);   /* [512] at ebp-0x20c */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xa88 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xa84 */
    uint32_t *const nNumberOfBytesToRead__f = (uint32_t *)(frame_ + 0x8);   /* ebp-0xa80 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0xa84);   /* ebp-0x4 */

  W_fs_5b6060((intptr_t)(v28));
  v5 = a2;
  (*v30__f) = 0;
  v6 = 9999;
  FileName[0] = 0;
  if ( a2 <= 10020 )
  {
    switch ( a2 )
    {
      case 10020:
        v6 = 0;
        goto LABEL_12;
      case 1:
        g_eng.h5bf3ee(Buffer, "savedata%02d.bak", a3);
        goto LABEL_10;
      case 10010:
        g_eng.h5bf3ee(Buffer, "savedata%02d.dat", a3);
LABEL_10:
        strcpy(FileName, (const char *)&SYSB(259));
        strcat(FileName, ((char *)D(0x854f5c)));
        strcat(FileName, Buffer);
        v5 = a2;
        goto LABEL_12;
      default:
        break;
    }
LABEL_9:
    g_eng.h5bf3ee(Buffer, "savedata%02d.sdt", a3);
    v6 = 2;
    goto LABEL_10;
  }
  if ( a2 != 10030 )
    goto LABEL_9;
  v6 = 1;
LABEL_12:
  if ( &(*v23__f) == 0 )
    v6 = (*v23__f);
  if ( v5 == 0 )
    goto LABEL_25;
  if ( v6 == 9999 )
  {
    v7 = W_fs_5b61b0((intptr_t)(v28), (intptr_t)(FileName));
  }
  else
  {
    if ( v5 == 10040 && W_fs_5b6bc0((intptr_t)(F32(D(0x8a93f0), 4 * (v6))), (intptr_t)(FileName)) == 0 )
      goto LABEL_25;
    v7 = W_fs_5b6200((intptr_t)(v28), (intptr_t)((int)F32(D(0x8a93f0), 4 * (v6))));
  }
  if ( v7 == 0
    || (W_fs_5b6340((intptr_t)(v28), (intptr_t)(&(*nNumberOfBytesToRead__f)), (intptr_t)(4u)),
        W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(v27), (intptr_t)((*nNumberOfBytesToRead__f)), (intptr_t)(0x25Cu)),
        v27[139] != 10001)
    || v27[140] != v5 )
  {
LABEL_25:
    (*v30__f) = -1;
    W_fs_5b60d0((intptr_t)(v28));
    return 0;
  }
  if ( a4 != 0 )
    *a4 = v27[138];
  W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(c), (intptr_t)(v27[0]), (intptr_t)(0xA4u));
  for ( i = 0; i < *(_WORD *)&GAP(c, 0x6, 2); ++i )
    W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(&GAP(c, 0x6, 16 * i + 158)), (intptr_t)(v27[1]), (intptr_t)(0x10u));
  v10 = F16(c, 0x0) == 0;
  (*v23__f) = 0;
  if ( !v10 )
  {
    do
    {
      v11 = (uint16_t)(*v23__f);
      v12 = &F16(c, 0x0) + 142 * (uint16_t)(*v23__f);
      W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(v12 + 4178), (intptr_t)(v27[2]), (intptr_t)(0x11Cu));
      v13 = 0;
      if ( v12[4218] != 0 )
      {
        v14 = 88 * v11;
        for ( (*j__f) = 88 * v11; ; v14 = (*j__f) )
        {
          W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(&GAP(c, 0x12a8, 12 * v14 + 5852 + 12 * v13++)), (intptr_t)(v27[3]), (intptr_t)(0xCu));
          if ( v13 >= v12[4218] )
            break;
        }
      }
      v15 = 0;
      if ( v12[4190] != 0 )
      {
        v16 = 66 * v11;
        for ( (*j__f) = 66 * v11; ; v16 = (*j__f) )
        {
          W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(&GAP(c, 0x12a8, 16 * v16 + 6620 + 16 * v15++)), (intptr_t)(v27[137]), (intptr_t)(0x10u));
          if ( v15 >= v12[4190] )
            break;
        }
      }
    }
    while ( (uint16_t)++(*v23__f) < F16(c, 0x0) );
  }
  W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(&GAP(c, 0x12a8, 14300)), (intptr_t)(v27[4]), (intptr_t)(8u));
  v18 = 0;
  if ( *(_DWORD *)&GAP(c, 0x12a8, 14300) != 0 )
  {
    v19 = 0;
    do
    {
      W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)(&GAP(c, 0x12a8, 12 * v19 + 14308)), (intptr_t)(v27[5]), (intptr_t)(0xCu));
      v19 = (uint16_t)++v18;
    }
    while ( (unsigned int)(uint16_t)v18 < *(_DWORD *)&GAP(c, 0x12a8, 14300) );
  }
  v20 = 0;
  if ( *(_DWORD *)&GAP(c, 0x12a8, 14304) != 0 )
  {
    v21 = 0;
    do
    {
      W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)((uint8_t *)c + 12 * v21 + 25228), (intptr_t)(v27[136]), (intptr_t)(0xCu));
      v21 = (uint16_t)++v20;
    }
    while ( (unsigned int)(uint16_t)v20 < *(_DWORD *)&GAP(c, 0x12a8, 14304) );
  }
  W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)((uint8_t *)c + 49804), (intptr_t)(v27[6]), (intptr_t)(2u));
  for ( k = 0; k < F16(c, 2 * 24902); ++k )
    W_file_read_section((intptr_t)((intptr_t)v28), (intptr_t)((uint8_t *)c + 8 * k + 49808), (intptr_t)(v27[7]), (intptr_t)(8u));
  W_fs_5b64b0((intptr_t)(v28));
  (*v30__f) = -1;
  W_fs_5b60d0((intptr_t)(v28));
  return 1;
}

/* 0x41bd20: cls_412C10 *__stdcall sub_41BD20(cls_4124D0 *a1, int a2) */
int32_t sv_41bd20(intptr_t ecx_, int32_t a1, int32_t a2)
{

    char * v2 = 0;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v14 = (int16_t *)(frame_ + 0x8);   /* [4] at ebp-0x2c */
    uint8_t *v15 = (uint8_t *)(frame_ + 0x10);   /* [36] at ebp-0x24 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x30 */

  v2 = ((char *)D(0x854fc0));
  if ( a2 == 0 )
    v2 = ((char *)D(0x854f9c));
  v3 = 0;
  v4 = 2;
  if ( v2 != 0 )
  {
    v3 = 1;
    v4 = 3;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v15), (intptr_t)(v3));
  v5 = W_win_mgr_open((intptr_t)(a1), (intptr_t)(8), (intptr_t)(8), (intptr_t)(624), (intptr_t)(464), (intptr_t)(v4), (intptr_t)(v15));
  v6 = v5;
  if ( v2 != 0 )
    W_win_title_add((intptr_t)(v5), (intptr_t)(v2));
  v7 = W_win_ctrl_take((intptr_t)(v6));
  v8 = (int)v7;
  if ( v7 != 0 )
    W_ctrl_grid_init((intptr_t)(v7), (intptr_t)(40), (intptr_t)(64), (intptr_t)(24), (intptr_t)(9), (intptr_t)(8), (intptr_t)(2));
  else
    v8 = 0;
  v14[0] = 0;
  v14[1] = 1;
  v14[2] = 2;
  v14[3] = 3;
  W_ctrl_cursor_set((intptr_t)(v8), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v14), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v8, 20) = 1;
    F32(v8, 24) = -9;
  }
  v9 = F32(v8, 376);
  F32(v8, 428) = 48;
  *v9 = 0;
  F32(F32((v8 + 376), 0), 4) = 0;
  F32(F32((v8 + 376), 0), 8) = 0;
  F32(F32((v8 + 376), 0), 12) = 0;
  F32(F32((v8 + 376), 0), 72) = 32;
  F32(F32((v8 + 376), 0), 76) = 0;
  F32(F32((v8 + 376), 0), 80) = 0;
  F32(F32((v8 + 376), 0), 84) = 0;
  F32(F32((v8 + 376), 0), 108) = 272;
  F32(F32((v8 + 376), 0), 112) = 0;
  F32(F32((v8 + 376), 0), 116) = 128;
  F32(F32((v8 + 376), 0), 120) = 1;
  F32(F32((v8 + 376), 0), 36) = 408;
  F32(F32((v8 + 376), 0), 40) = 0;
  F32(F32((v8 + 376), 0), 44) = 0;
  F32(F32((v8 + 376), 0), 48) = 0;
  W_grid_col_place((intptr_t)(v8), (intptr_t)(4), (intptr_t)(32), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(5), (intptr_t)(132), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(6), (intptr_t)(232), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(7), (intptr_t)(332), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0));
  SETLO(v10, F16(v8, 4));
  W_win_ctrl_to_back((intptr_t)(F32(v8, 0)), (intptr_t)(v10));
  W_sys_page_arrows((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v6), (intptr_t)(v8));
  sv_41bf60(v8, a2);
  W_tm_41c2f0((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(&(*v13__f)), (intptr_t)(&(*v12__f)));
  W_tm_419900((intptr_t)(v8), (intptr_t)((*v13__f)), (intptr_t)((uint16_t)(*v12__f)));
  return v6;
}

/* 0x41bf60: void __stdcall sub_41BF60(int *a1, int a2) */
void sv_41bf60(int32_t * a1, int32_t a2)
{

    intptr_t ExceptionList = 0;
    void * v3 = 0;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    uint16_t v6 = 0;
    uint32_t v7 = 0;
    uint8_t * v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x54218 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [512] at ebp-0x54218 */
    uint8_t *v13 = (uint8_t *)(frame_ + 0x204);   /* [344064] at ebp-0x54014 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x200);   /* ebp-0x54018 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x54204);   /* ebp-0x14 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x54208);   /* ebp-0x10 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x5420c);   /* ebp-0xc */
    void * *const v17__f = (void * *)(frame_ + 0x54210);   /* ebp-0x8 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x54214);   /* ebp-0x4 */

  (*v18__f) = -1;
  v4 = 0;
  v5 = 0;
  (*v18__f) = 0;
  F32(v12__f, 0x0) = (int)D(0x693848);
  (*v15__f) = 0;
  SETLO((*v14__f), 0);
  do
    SETLO((*v14__f), ++v5);
  while ( ((int32_t *)D(0x6940c8))[84 * v5] != 0 );
  v6 = 0;
  if ( v5 != 0 )
  {
    do
    {
      v7 = 336 * v6;
      v8 = &v13[v7];
      qmemcpy(&v13[v7], &((int32_t *)D(0x6940c8))[v7 / 4], 0x150u);
      v9 = W_tm_41c240((intptr_t)(&(*v12__f)), (intptr_t)(F32(&v13[v7 + 4], 0)));
      if ( v9 != 0 )
      {
        if ( F32(v8, 4 * 68) == 0 )
          F32(v8, 4 * 68) = v9[11];
        if ( F32(v8, 4 * 67) == 0 )
          F32(v8, 4 * 67) = v9[13];
        if ( F32(v8, 4 * 69) == 0 )
          F32(v8, 4 * 69) = v9[12];
        if ( F32(v8, 4 * 80) == 0 )
          F32(v8, 4 * 80) = v9[14];
        if ( F16(v8, 2 * 162) == 0 )
          F16(v8, 2 * 162) = F16(v9, 2 * 30);
        if ( F16(v8, 2 * 163) == 0 )
          F16(v8, 2 * 163) = F16(v9, 2 * 31);
        if ( ((int32_t *)D(0x6940c8)) != 0 )
          W_tm_41c150((intptr_t)(((int32_t *)D(0x6940c8))), (intptr_t)(v8), (intptr_t)((*v14__f)));
      }
      ++v6;
    }
    while ( v6 < (uint16_t)(*v14__f) );
    v4 = 0;
  }
  v10 = 1;
  F32(a1[93], 16) = 0;
  do
  {
    g_eng.h5bf3ee(Buffer, "%d :", v10);
    sv_561560(a1, 10040, v4++, a2, Buffer, (int)&(*v12__f));
    ++v10;
  }
  while ( (uint16_t)v4 < 0x18u );
}

/* 0x55f620: int __thiscall sub_55F620(cls_55F620 *this, int a2, int a3) */
int32_t sv_55f620(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    uint32_t v10 = 0;
    uint32_t v11 = 0;
    uint32_t v12 = 0;
    uint32_t v13 = 0;
    char * v14 = 0;
    uint32_t * v15 = 0;
    int v16 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x10c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0xc);   /* [256] at ebp-0x100 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10c */
    uint32_t *const v19__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x108 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x104 */

  v3 = F32(c, 0xc);
  v4 = F32(v3, 0x200c);
  v5 = **(_DWORD **)&GAP(W, 0x1030, 8);
  v6 = 1;
  if ( v4 != 0 )
  {
    v7 = F32(v4, 2548);
    if ( v7 != 0 )
    {
      if ( F16(v7, 68) == 0 )
      {
        g_eng.h5bf3ee(Buffer, "%d", F32(v7, 468));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x85451c)), Buffer);
      }
      v6 = F32(F32((v7 + 64), 0), 44);
    }
  }
  if ( a3 != 0 )
  {
    F32(v3, 0x4024) = v5;
    F32(v3, 0x4028) = a3;
    F32(v3, 0x402c) = v6;
  }
  v8 = F32(v3, 0x40ac);
  (*v18__f) = 4576;
  (*v20__f) = 128;
  do
  {
    v9 = *(_DWORD **)&GAP(W, 0x0, (*v18__f));
    if ( v9[116] != 0 )
    {
      v10 = v9[118];
      if ( v10 != 0 )
      {
        switch ( v9[172] )
        {
          case 1:
            (*v19__f) = 0;
            break;
          case 2:
            (*v19__f) = F32(*(_DWORD *)&GAP(W, 0x1030, 8), 4);
            break;
          case 3:
            goto LABEL_25;
          default:
            break;
        }
        v11 = v9[3];
        v12 = F32(v8, 0x7804);
        v13 = 0;
        if ( v12 == 0 )
          goto LABEL_23;
        v14 = &GAP(v8, 0x0, 6148);
        while ( F32(v14, 0) != v10 )
        {
          ++v13;
          v14 += 12;
          if ( v13 >= v12 )
            goto LABEL_23;
        }
        v15 = (unsigned int *)&GAP(v8, 0x0, 12 * v13 + 6148);
        if ( v15 == 0 )
LABEL_23:
          v15 = W_sc_4a63d0((intptr_t)(v8), (intptr_t)(v10));
        v15[1] = v11;
        v15[2] = (*v19__f);
      }
    }
LABEL_25:
    v16 = (*v20__f) == 1;
    (*v18__f) += 4;
    --(*v20__f);
  }
  while ( !v16 );
  result = W_room_to_scene((intptr_t)(v3), (intptr_t)(a2));   /* eax as 0x55f790 leaves it (IDA: LOWORD, an __int16 return) */
  return result;
}

/* 0x561560: void __stdcall sub_561560(cls_40FA00 *a1, int a2, unsigned __int16 a3, int a4, char *a5, cls_561BF0 *a6) */
void sv_561560(int32_t a1, int32_t a2, uint16_t a3, int32_t a4, char * a5, int32_t a6)
{

    intptr_t ExceptionList = 0;
    void * v7 = 0;
    uint8_t * v8 = 0;
    uint8_t * v9 = 0;
    intptr_t i = 0;
    void * v11 = 0;
    void * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    int32_t * v20 = 0;
    intptr_t v21 = 0;
    struct tm * v22 = 0;
    char * v23 = 0;
    char * v24 = 0;
    int32_t * v25 = 0;
    uint16_t v26 = 0;
    intptr_t v27 = 0;
    int32_t * v28 = 0;
    intptr_t k = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    int32_t * v32 = 0;
    intptr_t v33 = 0;
    int32_t * v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    uint32_t v37 = 0;
    char * v38 = 0;
    char * v39 = 0;
    char * v40 = 0;
    char v41 = 0;
    uint32_t v42 = 0;
    intptr_t v43 = 0;
    int v44 = 0;
    _Alignas(8) uint8_t frame_[0x15ed0 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x2c);   /* [512] at ebp-0x15ea4 */
    char *v57 = (char *)(frame_ + 0x22c);   /* [512] at ebp-0x15ca4 */
    uint8_t *v59 = (uint8_t *)(frame_ + 0x7c34);   /* [49804] at ebp-0xe29c */
    intptr_t *v61 = (intptr_t *)(frame_ + 0x13ec4);   /* [2050] at ebp-0x200c */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x15ed0 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x15ebc */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x15ebc */
    int32_t * *const v47__f = (int32_t * *)(frame_ + 0x14);   /* ebp-0x15ebc */
    intptr_t * *const v51__f = (intptr_t * *)(frame_ + 0x18);   /* ebp-0x15eb8 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x15eb8 */
    int32_t * *const v50__f = (int32_t * *)(frame_ + 0x18);   /* ebp-0x15eb8 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x15eb4 */
    uint16_t *const v53__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x15eb0 */
    intptr_t *const Time__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x15eac */
    intptr_t *const v55__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x15ea8 */
    char *const v58__f = (char *)(frame_ + 0x42c);   /* ebp-0x15aa4 */
    int16_t *const v60__f = (int16_t *)(frame_ + 0x13ec0);   /* ebp-0x2010 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x15ecc);   /* ebp-0x4 */

  (*v62__f) = -1;
  v8 = &v59[10628];
  memset(v59, 0, 8356);
  v9 = &v59[8356];
  for ( i = 8; i != 0; --i )
  {
    v11 = v9;
    v9 += 284;
    memset(v11, 0, 0x11Cu);
    v12 = v8;
    v8 += 1056;
    memset(v12, 0, 0x420u);
  }
  memset(&v59[19076], 0, 30728);
  (*v60__f) = 0;
  (*v53__f) = 4;
  memset(v61, 0, 0x2000u);
  v13 = F32(a1, 0x174);
  v14 = F32(v13, 16);
  if ( v14 < F32(v13, 4) )
  {
    (*v45__f) = F32(v13, 16);
    F32(v13, 16) = v14 + 1;
    v16 = 16 * v14;
    F32((v16 + F32(a1, 0x17c)), 0) = a2;
    F32(v16 + F32(a1, 0x17c), 4) = a3;
    F32(v16 + F32(a1, 0x17c), 8) = 1;
    W_grid_row_reset((intptr_t)(a1), (intptr_t)((*v45__f)));
    (*v52__f) = v14;
    v15 = v14;
  }
  else
  {
    v15 = -1;
    (*v52__f) = -1;
  }
  v17 = F32(a1, 0x174);
  if ( F32(v17, 8) > 0 && F32(v17, 16) > v15 && v15 >= 0 )
  {
    v18 = 16 * v15;
    if ( F32(F32(16 * v15 + F32(a1, 0x17c), 12), 0) == 0 )
    {
      v19 = (intptr_t)g_eng.op_new(0xCu);
      (*v62__f) = 0;
      if ( v19 != 0 )
        v20 = sv_561cf0(v19, 80);
      else
        v20 = 0;
      v21 = F32(a1, 0x17c);
      (*v62__f) = -1;
      F32(F32(v18 + v21, 12), 0) = v20;
    }
    W_glyph_recs_set((intptr_t)(**(intptr_t **)(v18 + F32(a1, 0x17c) + 12)), (intptr_t)(a5), (intptr_t)(0));
  }
  if ( sv_41b670(v59, a2, a3, &(*Time__f)) == 0 )
  {
    F32(F32(a1, 0x17c) + 16 * v15, 8) = a4;
    W_grid_cell_text((intptr_t)(a1), (intptr_t)(v15), (intptr_t)(2), (intptr_t)(((char *)D(0x8a0b28))));
    return;
  }
  v22 = CRT_localtime(&(*Time__f));
  CRT_strftime(v57, 0x200u, ((char *)D(0x8a0b70)), v22);
  (*v46__f) = 16 * v15;
  F32(16 * v15 + F32(a1, 0x17c), 8) = 1;
  v23 = (char *)(intptr_t)g_eng.op_new(0x4120u);
  v24 = v23;
  (*v55__f) = v23;
  (*v62__f) = 1;
  if ( v23 != 0 )
  {
    v25 = v23 + 16432;
    for ( (*j__f) = 8; (*j__f) != 0; --(*j__f) )
      *v25++ = (intptr_t)g_eng.op_new(0xEECu);
    F32(v24, 4 * 1) = 0;
  }
  else
  {
    v24 = 0;
  }
  (*v62__f) = -1;
  v26 = 0;
  (*v50__f) = v24 + 16516;
  do
  {
    v27 = *((*v50__f) - 21);
    F16(v27, 2572) = v26;
    W_pslot_reset((intptr_t)(v27));
    ++v26;
    *(*v50__f)++ = 1;
  }
  while ( v26 < 8u );
  F32(v24, 4 * 4137) = 1;
  F16(v24, 2 * 8276) = 0;
  F32(v24, 4 * 4102) = 1;
  F32(v24, 4 * 2051) = 0;
  F16(v24, 2 * 8200) = 0;
  F32(v24, 4 * 4101) = 0;
  F32(v24, 4 * 4103) = 0;
  F32(v24, 4 * 4116) = 0;
  F32(v24, 4 * 4117) = 3;
  F32(v24, 4 * 4118) = 0;
  F32(v24, 4 * 4139) = &(*v58__f);
  F16(v24, 2 * 4100) = 0;
  F32(v24, 4 * 4140) = 0;
  F32(v24, 4 * 4141) = 0;
  F32(v24, 4 * 4142) = 0;
  F32(v24, 4 * 4143) = 0;
  F32(v24, 4 * 4144) = 0;
  F32(v24, 4 * 4145) = 0;
  F32(v24, 4 * 4146) = 0;
  F32(v24, 4 * 4147) = 0;
  F32(v24, 4 * 4151) = 0;
  F32(v24, 4 * 4153) = 0;
  W_room_party_defaults((intptr_t)(v24));
  F32(v24, 4 * 4161) = 0;
  W_room_inputs_alloc((intptr_t)(v24), (intptr_t)(0));
  F32(v24, 4 * 4119) = 0;
  F16(v24, 2 * 8240) = 0;
  F16(v24, 2 * 8238) = 0;
  F16(v24, 2 * 8242) = 1;
  F16(v24, 2 * 8239) = 0;
  F16(v24, 2 * 8240) = 0;
  F16(v24, 2 * 8241) = 1;
  F32(v24, 4 * 4121) = 0;
  v28 = v24 + 16648;
  for ( k = 3; k != 0; --k )
  {
    v28[1] = 0;
    *v28 = 1;
    v28 += 2;
  }
  F32(v24, 4 * 4122) = API_GetTickCount();
  F32(v24, 4 * 4123) = 0;
  sv_561d20(v24, (int)v59);
  v30 = F32(a1, 0x174);
  if ( F32(v30, 8) > 1 && F32(v30, 16) > (*v52__f) && (*v52__f) >= 0 )
  {
    if ( F32(F32(((*v46__f) + F32(a1, 0x17c) + 12), 0), 24) == 0 )
    {
      v31 = (intptr_t)g_eng.op_new(0xCu);
      (*v55__f) = v31;
      (*v62__f) = 2;
      if ( v31 != 0 )
        v32 = sv_561cf0(v31, 80);
      else
        v32 = 0;
      v33 = F32(a1, 0x17c);
      (*v62__f) = -1;
      F32(F32(((*v46__f) + v33 + 12), 0), 24) = v32;
    }
    W_glyph_recs_set((intptr_t)(F32(F32((*v46__f) + F32(a1, 0x17c), 12), 24)), (intptr_t)(v57), (intptr_t)(0));
  }
  g_eng.h5bf3ee(Buffer, "%d:%02d", F32(&v59[124], 0) / 0xE10u, F32(&v59[124], 0) % 0xE10u / 0x3C);
  W_grid_cell_text((intptr_t)(a1), (intptr_t)((*v52__f)), (intptr_t)(3), (intptr_t)(Buffer));
  v34 = W_flag_find((intptr_t)(F32(v24, 4 * (4139))), (intptr_t)(100100101));
  if ( v34 == 0 )
  {
LABEL_37:
    (*v47__f) = sv_561bf0(a6, F32(v24, 4 * 4105));
    v36 = W_area_find((intptr_t)(a6), (intptr_t)(F32(v24, 4 * 4105)));
    if ( v36 != 0 && strlen((const char *)(v36 + 280)) != 0 )
    {
      v37 = strlen((const char *)(v36 + 280)) + 1;
      v38 = (char *)(v36 + 280 + v37);
    }
    else
    {
      if ( (*v47__f) == 0 || strlen((uint8_t *)(*v47__f) + 4) == 0 )
      {
        v42 = strlen(((char *)D(0x8a0b5c))) + 1;
        qmemcpy(Buffer, ((char *)D(0x8a0b5c)), 4 * (v42 >> 2));
        v40 = ((char *)D(0x8a0b5c) + (4 * (v42 >> 2)));
        v39 = &Buffer[4 * (v42 >> 2)];
        v41 = v42;
LABEL_45:
        qmemcpy(v39, v40, v41 & 3);
        W_grid_cell_text((intptr_t)(a1), (intptr_t)((*v52__f)), (intptr_t)(2), (intptr_t)(Buffer));
        (*v48__f) = 8;
        (*v51__f) = (intptr_t *)(v24 + 16432);
        do
        {
          v43 = *(*v51__f);
          if ( W_pslot_ok((intptr_t)(*(*v51__f)), (intptr_t)(0), (intptr_t)(0)) && (*v53__f) <= 8u )
          {
            g_eng.h5bf3ee(
              Buffer,
              "%s LV%d",
              &GAP((v43 + (1) * 0x9f4), 0x0, 28),
              *(_DWORD *)&GAP((v43 + (1) * 0x9f4), 0x0, 244) + *(_DWORD *)&GAP((v43 + (1) * 0x9f4), 0x0, 252));
            W_grid_cell_text((intptr_t)(a1), (intptr_t)((*v52__f)), (intptr_t)((*v53__f)++), (intptr_t)(Buffer));
          }
          v44 = (*v48__f) == 1;
          ++(*v51__f);
          --(*v48__f);
        }
        while ( !v44 );
        goto LABEL_53;
      }
      v37 = strlen((uint8_t *)(*v47__f) + 4) + 1;
      v38 = (uint8_t *)(*v47__f) + v37 + 4;
    }
    qmemcpy(Buffer, &v38[-v37], 4 * (v37 >> 2));
    v40 = &v38[4 * (v37 >> 2) - v37];
    v39 = &Buffer[4 * (v37 >> 2)];
    v41 = v37;
    goto LABEL_45;
  }
  v35 = v34[1];
  if ( v35 == 500 )
  {
    W_grid_cell_text((intptr_t)(a1), (intptr_t)((*v52__f)), (intptr_t)(2), (intptr_t)(((char *)D(0x8a0b30))));
  }
  else
  {
    if ( v35 != 2900 )
      goto LABEL_37;
    W_grid_cell_text((intptr_t)(a1), (intptr_t)((*v52__f)), (intptr_t)(2), (intptr_t)(((char *)D(0x8a0b44))));
  }
LABEL_53:
  if ( v24 != 0 )
    W_room_delete((intptr_t)(v24), (intptr_t)(1));
}

/* 0x561bf0: int *__thiscall sub_561BF0(cls_561BF0 *this, int a2) */
int32_t sv_561bf0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v2 = 0;
    uint16_t v3 = 0;
    intptr_t v5 = 0;
    char * v6 = 0;
    int32_t * v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;

  v2 = F16(c, 0x54004);
  v3 = 0;
  if ( v2 == 0 )
    return 0;
  while ( *(_DWORD *)&GAP(c, 0x4, 336 * v3) != a2 )
  {
    if ( ++v3 >= v2 )
      return 0;
  }
  v5 = 336 * v3;
  v6 = &GAP(c, 0x4, v5);
  if ( ((uint8_t *)c + v5) == -4 )
    return 0;
  v7 = (int *)F32(c, 0x0);
  v8 = 0;
  v9 = *v7;
  if ( *v7 == 0 )
    return 0;
  v10 = F32(v6, 4 * 1);
  while ( v9 != v10 )
  {
    v9 = v7[16 * ++v8];
    if ( v9 == 0 )
      return 0;
  }
  return &v7[16 * v8];
}

/* 0x561cf0: _DWORD *__thiscall sub_561CF0(cls_561CF0 *this, int a2) */
int32_t sv_561cf0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x4) = a2;
  F32(c, 0x0) = (int)(intptr_t)g_eng.op_new(36 * a2);
  F16(((uint8_t *)(c) + 0x8), 0) = 0;
  *(_WORD *)&GAP(c, 0x8, 2) = 0;
  return &F32(c, 0x0);
}

/* 0x561d20: _DWORD *__thiscall sub_561D20(cls_4066D0 *this, int a2) */
int32_t sv_561d20(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    char * v4 = 0;
    int16_t * v5 = 0;
    intptr_t i = 0;
    int16_t v7 = 0;
    uint16_t v8 = 0;
    int v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    int32_t * v17 = 0;
    char * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    char * v21 = 0;
    intptr_t v22 = 0;
    uint16_t j = 0;
    char * v24 = 0;
    int v25 = 0;
    uint16_t v26 = 0;
    uint16_t * v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    int32_t * v31 = 0;
    intptr_t v32 = 0;
    intptr_t * v33 = 0;
    intptr_t v34 = 0;
    char * v35 = 0;
    intptr_t m = 0;
    int32_t * result = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    uint16_t v40 = 0;
    char v47 = 0;
    intptr_t * v48 = 0;
    intptr_t k = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x14 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x10 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xc */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x8 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x4 */

  F32(c, 0x4014) = F32(a2, 4);
  F32(c, 0x401c) = F32(a2, 28);
  F32(c, 0x4024) = F32(a2, 16);
  F32(c, 0x4028) = F32(a2, 20);
  F32(c, 0x402c) = F32(a2, 24);
  F32(c, 0x40d0) = F32(a2, 32);
  F32(c, 0x40d4) = F32(a2, 36);
  F32(c, 0x40d8) = F32(a2, 40);
  F32(c, 0x40b4) = F32(a2, 48);
  F32(c, 0x40b8) = F32(a2, 52);
  F32(c, 0x40bc) = F32(a2, 56);
  F32(c, 0x40c0) = F32(a2, 140);
  F32(c, 0x40c4) = F32(a2, 144);
  F32(c, 0x40c8) = F32(a2, 148);
  F32(c, 0x40cc) = F32(a2, 152);
  F32(c, 0x40dc) = F32(a2, 156);
  F32(c, 0x40b0) = F32(a2, 44);
  F32(c, 0x406c) = F32(a2, 124);
  F32(c, 0x4104) = F32(a2, 132);
  v4 = ((uint8_t *)(c) + 0x405c);
  v5 = (int16_t *)(a2 + 60);
  for ( i = 3; i != 0; --i )
  {
    v7 = *v5++;
    F16(v4, 0) = v7;
    v4 += 2;
  }
  F32(c, 0x4064) = F16(a2, 136);
  F32(c, 0x4050) = F16(a2, 10);
  F32(c, 0x4054) = F16(a2, 12);
  F32(c, 0x4058) = F16(a2, 128);
  F32(c, 0x4068) = API_GetTickCount();
  v8 = 0;
  F16(c, 0x4010) = 0;
  v9 = F16(a2, 8) == 0;
  (*v42__f) = 0;
  if ( !v9 )
  {
    do
    {
      SETHI(v13, 0);
      v10 = v8;
      v11 = 16 * (v8 + 11);
      v12 = 16 * v10 + a2;
      SETLO(v13, F16((v11 + a2), 0));
      SETLO(v11, F16(c, 0x4010));
      v47 = F8(v12, 178);
      v14 = F32(v12, 164);
      (*v46__f) = v12;
      (*v45__f) = v14;
      (*v43__f) = v11;
      if ( (uint16_t)v11 < 0x200u )
      {
        v15 = 110010;
        v16 = 0;
        while ( v14 != v15 )
        {
          v15 = ((int32_t *)D(0x6c0cd0))[184 * ++v16];
          if ( v15 == 0 )
            goto LABEL_24;
        }
        v17 = &((int32_t *)D(0x6c0cd0))[184 * v16];
        if ( v17 != 0 && (W_item_kind((intptr_t)((int)&((int32_t *)D(0x6c0cd0))[184 * v16])) != 1 || W_room_items_of_kind((intptr_t)(c), (intptr_t)(1), (intptr_t)(1)) < 48) )
        {
          v18 = &GAP(c, 0x0, 16 * (uint16_t)(*v43__f));
          *(_DWORD *)&GAP(c, 0x2010, 16 * (uint16_t)(*v43__f)) = (*v45__f);
          F32(v18, 4 * 2055) = v17;
          if ( v13 > 0 )
            F16(v18, 2 * 4108) = v13;
          else
            F16(v18, 2 * 4108) = F16(v17, 2 * 50);
          F32(v18, 4 * 2053) = 0;
          v18[8218] = v47;
          if ( &(*v41__f) != (int *)-24 )
            (*v44__f) = (*v43__f);
          v19 = (*v46__f);
          ++F16(c, 0x4010);
          v20 = F32(v19, 168);
          v21 = &GAP(c, 0x0, 16 * (uint16_t)(*v44__f));
          v22 = F32(v21, 4 * 2055);
          if ( v20 != 0 )
          {
            for ( j = 0; j < F16(c, 0x4010); ++j )
            {
              v24 = &GAP(c, 0x0, 16 * j);
              if ( F32(v24, 4 * 2053) == v20
                && F32(F32(v24, 4 * 2055), 120) == F32(v22, 120) )
              {
                F32(v24, 4 * 2053) = 0;
              }
            }
          }
          F32(v21, 4 * 2053) = v20;
        }
      }
LABEL_24:
      v8 = (*v42__f) + 1;
      v25 = (uint16_t)++(*v42__f) < F16(a2, 8);
    }
    while ( v25 );
  }
  v26 = 0;
  v9 = F16(a2, 0) == 0;
  (*v42__f) = 0;
  if ( !v9 )
  {
    do
    {
      v27 = (uint16_t *)(a2 + 284 * v26 + 8356);
      if ( F16(a2 + 284 * v26, 8364) != 0 )
      {
        v28 = *(intptr_t *)&GAP(c, 0x4030, 4 * *v27);
        W_tm_560a40((intptr_t)(v28), (intptr_t)((int)c), (intptr_t)(a2 + 284 * v26 + 8356), (intptr_t)(1056 * v26 + a2 + 10628));
        if ( v27[5] != 0 )
        {
          v29 = 0;
          if ( v28 != 0 )
          {
            v30 = F32(c, 0x200c);
            if ( v30 != 0 )
            {
              v29 = F32(v30, 0x0);
              W_pslot_input_set((intptr_t)(v30), (intptr_t)(0));
            }
            W_pslot_input_set((intptr_t)(v28), (intptr_t)(v29));
          }
          F32(c, 0x200c) = (int)v28;
        }
      }
      v26 = (*v42__f) + 1;
      v25 = (uint16_t)++(*v42__f) < F16(a2, 0);
    }
    while ( v25 );
  }
  v31 = W_area_level_row((intptr_t)(F32(c, 0x401c)));
  v48 = (intptr_t *)((uint8_t *)(c) + 0x4030);
  (*v44__f) = 8;
  F32(c, 0x4020) = v31[3];
  do
  {
    v32 = *v48;
    if ( W_pslot_ok((intptr_t)(*v48), (intptr_t)(0), (intptr_t)(0)) || W_pslot_ok((intptr_t)(v32), (intptr_t)(0), (intptr_t)(1)) )
      W_tm_41d780((intptr_t)(v32), (intptr_t)(1), (intptr_t)(v31[1]), (intptr_t)(v31[2]));
    v9 = (*v44__f) == 1;
    ++v48;
    --(*v44__f);
  }
  while ( !v9 );
  v33 = (intptr_t *)((uint8_t *)(c) + 0x4030);
  for ( k = 8; k != 0; --k )
  {
    v34 = *v33;
    if ( W_pslot_ok((intptr_t)(*v33), (intptr_t)(1), (intptr_t)(1)) )
      W_room_items_grant((intptr_t)(c), (intptr_t)(F32(v34, 0x9f0)));
    ++v33;
  }
  v35 = ((uint8_t *)(c) + 0x4030);
  for ( m = 8; m != 0; --m )
  {
    if ( W_pslot_ok((intptr_t)(F32(v35, 0)), (intptr_t)(1), (intptr_t)(1)) )
      W_stats_recompute((intptr_t)(c), (intptr_t)(F32(F32(v35, 0), 2544)), (intptr_t)(F32(v35, 0) + 2576), (intptr_t)(0));
    result = (_DWORD *)W_pslot_ok((intptr_t)(F32(v35, 0)), (intptr_t)(1), (intptr_t)(0));
    if ( result != 0 )
    {
      v38 = F32(F32(v35, 0), 2548);
      if ( v38 != 0 )
        W_member_palette((intptr_t)(v38));
    }
    v35 += 4;
  }
  v39 = F32(c, 0x40ac);
  v40 = 0;
  if ( v39 != 0 )
    result = (_DWORD *)sv_560530(v39, a2);
  F16(c, 0x2008) = 0;
  if ( F16(a2, 49804) != 0 )
  {
    do
    {
      result = W_room_flag_set((intptr_t)(c), (intptr_t)(F32(a2 + 8 * v40, 49808)), (intptr_t)(F32(a2 + 8 * v40, 49812)));
      ++v40;
    }
    while ( v40 < F16(a2, 49804) );
  }
  return result;
}

/* 0x560530: int __thiscall sub_560530(cls_55FF80 *this, int a2) */
int32_t sv_560530(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    uint32_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int v15 = 0;
    uint16_t * v16 = 0;
    intptr_t result = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint32_t v22 = 0;
    uint32_t v23 = 0;
    char * v24 = 0;
    uint32_t v25 = 0;
    char * v26 = 0;
    char * v27 = 0;
    char * v28 = 0;
    char v29 = 0;
    uint32_t v30 = 0;
    char * v31 = 0;
    char * v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    char * v36 = 0;
    uint32_t v37 = 0;
    char * v38 = 0;
    char * v39 = 0;
    char * v40 = 0;
    char v41 = 0;
    uint32_t v42 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  v2 = a2;
  v3 = 0;
  F32(c, 0x1800) = 0;
  F32(c, 0x7804) = 0;
  for ( (*i__f) = 0; (unsigned int)(uint16_t)(*i__f) < F32(v2, 19076); v3 = (uint16_t)(*i__f) )
  {
    v4 = 3 * v3 + 4773;
    v5 = 3 * v3;
    v6 = F16((v2 + 4 * v4), 0);
    v7 = F32(v2 + 4 * v5, 19084);
    (*v45__f) = F32(v2 + 4 * v5, 19088);
    v8 = F32(c, 0x1800);
    v9 = 0;
    if ( v8 == 0 )
      goto LABEL_8;
    v10 = c;
    while ( *(_DWORD *)&F16(v10, 0x0) != v7 )
    {
      ++v9;
      v10 = ((uint8_t *)v10 + 12);
      if ( v9 >= v8 )
        goto LABEL_8;
    }
    v11 = &F16(c, 0x0) + 6 * v9;
    if ( v11 == 0 )
    {
LABEL_8:
      v11 = 0;
      v12 = 0;
      if ( v8 == 0 )
        goto LABEL_14;
      v13 = 0;
      do
      {
        v14 = 3 * v13;
        v15 = *((_DWORD *)&F16(c, 0x0) + v14) == 0;
        v16 = &F16(c, 0x0) + 2 * v14;
        if ( v15 )
          v11 = v16;
        v13 = (uint16_t)++v12;
      }
      while ( (uint16_t)v12 < v8 );
      if ( v11 == 0 )
      {
LABEL_14:
        if ( v8 == 512 )
        {
          W_wn_560900((intptr_t)((const char *)D(0x8a9530)));
          F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
          if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
          {
            strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
            v25 = strlen(((char *)D(0x855040))) + 1;
            v26 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
            qmemcpy(v26, ((char *)D(0x855040)), 4 * (v25 >> 2));
            v28 = ((char *)D(0x855040) + (4 * (v25 >> 2)));
            v27 = &v26[4 * (v25 >> 2)];
            v29 = v25;
          }
          else
          {
            v30 = strlen(((char *)D(0x855040))) + 1;
            qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x855040)), 4 * (v30 >> 2));
            v28 = ((char *)D(0x855040) + (4 * (v30 >> 2)));
            v27 = &((char *)D(0x8a9530) + 0x404)[4 * (v30 >> 2) - 1024];
            v29 = v30;
          }
          qmemcpy(v27, v28, v29 & 3);
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x855040)), D(0x8a9b6c));
          g_eng.h5bf5db(0);
        }
        F32(c, 0x1800) = v8 + 1;
        v11 = &F16(c, 0x0) + 6 * v8;
      }
      v2 = a2;
      F32(v11, 0) = v7;
      F32(v11, 4 * 2) = 0;
      F32(v11, 4 * 1) = 0;
    }
    if ( v6 != 0 || (*v45__f) != 0 )
    {
      F32(v11, 4 * 2) = v6;
      F32(v11, 4 * 1) = (*v45__f);
    }
    else
    {
      F32(v11, 0) = 0;
    }
    ++(*i__f);
  }
  result = 0;
  for ( (*j__f) = 0; (unsigned int)(uint16_t)(*j__f) < F32(v2, 19080); result = (uint16_t)(*j__f) )
  {
    v18 = 3 * result + 6309;
    v19 = 3 * result;
    v20 = F32(v2 + 4 * v19, 25228);
    (*v47__f) = F32((v2 + 4 * v18), 0);
    v21 = F32(v2 + 4 * v19, 25232);
    v22 = 0;
    (*v46__f) = v21;
    v23 = F32(c, 0x7804);
    if ( v23 == 0 )
      goto LABEL_33;
    v24 = ((uint8_t *)(c) + 0x1804);
    while ( F32(v24, 0) != v20 )
    {
      ++v22;
      v24 += 12;
      if ( v22 >= v23 )
        goto LABEL_33;
    }
    v31 = &GAP(c, 0x1804, 12 * v22);
    if ( v31 == 0 )
    {
LABEL_33:
      v32 = 0;
      v33 = 0;
      if ( v23 == 0 )
        goto LABEL_39;
      v34 = 0;
      do
      {
        v35 = 3 * v34;
        v15 = *(_DWORD *)&GAP(c, 0x1804, 4 * v35) == 0;
        v36 = &GAP(c, 0x1804, 4 * v35);
        if ( v15 )
          v32 = v36;
        v34 = (uint16_t)++v33;
      }
      while ( (uint16_t)v33 < v23 );
      if ( v32 == 0 )
      {
LABEL_39:
        if ( v23 == 2048 )
        {
          W_wn_560900((intptr_t)((const char *)D(0x8a9530)));
          F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
          if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
          {
            strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
            v37 = strlen(((char *)D(0x859388))) + 1;
            v38 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
            qmemcpy(v38, ((char *)D(0x859388)), 4 * (v37 >> 2));
            v40 = ((char *)D(0x859388) + (4 * (v37 >> 2)));
            v39 = &v38[4 * (v37 >> 2)];
            v41 = v37;
          }
          else
          {
            v42 = strlen(((char *)D(0x859388))) + 1;
            qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x859388)), 4 * (v42 >> 2));
            v40 = ((char *)D(0x859388) + (4 * (v42 >> 2)));
            v39 = &((char *)D(0x8a9530) + 0x404)[4 * (v42 >> 2) - 1024];
            v41 = v42;
          }
          qmemcpy(v39, v40, v41 & 3);
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x859388)), D(0x8a9b6c));
          g_eng.h5bf5db(0);
        }
        F32(c, 0x7804) = v23 + 1;
        v32 = &GAP(c, 0x1804, 12 * v23);
      }
      v2 = a2;
      F32(v32, 0) = v20;
      F32(v32, 4 * 1) = 0;
      F32(v32, 4 * 2) = 0;
      v31 = v32;
    }
    F32(v31, 4 * 1) = (*v46__f);
    F32(v31, 4 * 2) = (*v47__f);
    ++(*j__f);
  }
  return result;
}

/* 0x41cfd0: char *__stdcall sub_41CFD0(int a1, unsigned __int16 a2, char *a3, int *a4) */
int32_t sv_41cfd0(int32_t a1, uint16_t a2, char * a3, int32_t * a4)
{

    intptr_t v4 = 0;
    uint32_t v5 = 0;
    char * v6 = 0;
    _Alignas(8) uint8_t frame_[0x200 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [512] at ebp-0x200 */

  v4 = 9999;
  *a3 = 0;
  if ( a1 > 10020 )
  {
    if ( a1 != 10030 )
      goto LABEL_11;
    v4 = 1;
  }
  else
  {
    if ( a1 != 10020 )
    {
      if ( a1 == 1 )
      {
        g_eng.h5bf3ee(Buffer, "savedata%02d.bak", a2);
        goto LABEL_6;
      }
      if ( a1 == 10010 )
      {
        g_eng.h5bf3ee(Buffer, "savedata%02d.dat", a2);
LABEL_6:
        strcpy(a3, (const char *)&SYSB(259));
        strcat(a3, ((char *)D(0x854f5c)));
        v5 = strlen(Buffer) + 1;
        v6 = &a3[strlen(a3) + 1];
LABEL_7:
        qmemcpy(v6 - 1, Buffer, v5);
        goto LABEL_13;
      }
LABEL_11:
      g_eng.h5bf3ee(Buffer, "savedata%02d.sdt", a2);
      v4 = 2;
      strcpy(a3, (const char *)&SYSB(259));
      strcat(a3, ((char *)D(0x854f5c)));
      v5 = strlen(Buffer) + 1;
      v6 = &a3[strlen(a3) + 1];
      goto LABEL_7;
    }
    v4 = 0;
  }
LABEL_13:
  if ( a4 != 0 )
    *a4 = v4;
  return a3;
}
/* unported callees: none */
