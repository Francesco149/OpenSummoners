/* menu_screens.c — the main menu's other SCREENS (the room loop's menu 0x591120 opens them): the level-up screen
 * 0x5915c0 (+ 0x592020), STATUS 0x592190 / 0x592850 (the member pages, the stat rows, the equipment, + 0x593d90 /
 * 0x593e40), ITEMS 0x593f60 / 0x5943e0 (the list, the use / equip / drop flows 0x594a20..0x595350, + the queued item
 * use), ABILITIES 0x596b50 / 0x5956c0 (+ 0x595dd0..0x596ac0), the options page helpers 0x597480 / 0x5974b0 / 0x597670,
 * PARTY COMMANDS 0x598d00 (+ 0x5990b0..0x599e60).  See menu_screens.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  UNVERIFIED: no recording opens
 * these screens yet (rh-combat reaches the main menu itself, `src/field_menu.c`); REPLACE-wired, compile-checked. */
#include "menu_screens.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "event.h"
#include "menu_helpers.h"
#include "party_member.h"
#include "render_draw.h"
#include "room_load.h"
#include "room_loop.h"
#include "sound_fx.h"
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

static inline intptr_t W_cell_color_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)cell_color_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_cell_icon_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)cell_icon_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (uint16_t)a4, (const uint16_t *)a5, (uint16_t)a6, (uint16_t)a7); }
static inline intptr_t W_char_skills_refresh(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skills_refresh((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_keys_expand(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)ctrl_keys_expand((uint8_t *)a0, (const char *)a1, (char *)a2); }
static inline intptr_t W_ctrl_text_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ctrl_text_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_ctrl_text_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_text_set((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_equip_panel_fill(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { equip_panel_fill((uint8_t *)a0, (uint8_t *)a1, (wl_room *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_col_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)grid_col_place((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_grid_gauge_clamp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_gauge_clamp((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_grid_page_fit(intptr_t a0, ...) { return (intptr_t)grid_page_fit((uint8_t *)a0); }
static inline intptr_t W_grid_row_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)grid_row_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_grid_row_reset(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_row_reset((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_grid_row_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)grid_row_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); }
static inline intptr_t W_icon_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)icon_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5); }
static inline intptr_t W_item_def(intptr_t a0, ...) { return (intptr_t)item_def((int32_t)a0); }
static inline intptr_t W_item_list_fill(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { item_list_fill((uint8_t *)a0, (wl_room *)a1, (int32_t)a2, (int32_t)a3, (uint8_t *)a4, (int32_t)a5, (int32_t)a6); return 0; }
static inline intptr_t W_member_rows_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)member_rows_add((const wl_room *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_member_skill_at(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)member_skill_at((const uint8_t *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_menu_item_fits(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)menu_item_fits((const uint8_t *)a0, (int32_t)a1, (const uint8_t *)a2); }
static inline intptr_t W_menu_msg_open(intptr_t a0, ...) { return (intptr_t)menu_msg_open((uint8_t *)a0); }
static inline intptr_t W_menu_msg_set(intptr_t a0, intptr_t a1, ...) { menu_msg_set((uint8_t *)a0, (const char *)a1); return 0; }
static inline intptr_t W_menu_party_fill(intptr_t a0, ...) { menu_party_fill((uint8_t *)a0); return 0; }
static inline intptr_t W_menu_run(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)menu_run((uint8_t *)a0, (uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_menu_sprites_free(intptr_t a0, ...) { return (intptr_t)menu_sprites_free((uint8_t *)a0); }
static inline intptr_t W_mh_418ec0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)mh_418ec0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_mh_4190d0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)mh_4190d0((wl_char *)a0, (char *)a1, (uint16_t)a2, (int32_t *)a3, (int32_t)a4); }
static inline intptr_t W_mh_4199b0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)mh_4199b0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_mh_419c70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)mh_419c70((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t *)a7, (int32_t *)a8, (char *)a9, (int32_t)a10); }
static inline intptr_t W_mh_419eb0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)mh_419eb0((wl_char *)a0, (char *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7, (int32_t *)a8, (int32_t)a9); }
static inline intptr_t W_mh_4e61d0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)mh_4e61d0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_mh_4e6520(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)mh_4e6520((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_mh_5b1f10(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)mh_5b1f10((wl_char *)a0, (int32_t)a1, (int32_t)a2, (char *)a3, (uint16_t *)a4, (uint32_t)a5, (int32_t)a6); }
static inline intptr_t W_msg_pages(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)msg_pages((uint8_t *)a0, (const char *)a1, (const char *)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (uint16_t)a7, (uint16_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_msg_window_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)msg_window_open((uint8_t *)a0, (const char *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_pslot_by_id(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_by_id((wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rd_417c40(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_417c40((wl_char *)a0, (uint16_t)a1, (uint16_t)a2); }
static inline intptr_t W_rd_495dc0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_495dc0((int32_t)a0, (uint16_t)a1, (char *)a2); }
static inline intptr_t W_rd_4961e0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rd_4961e0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_formation(intptr_t a0, ...) { return (intptr_t)room_formation((wl_room *)a0); }
static inline intptr_t W_room_item_equip(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { room_item_equip((wl_room *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_room_party_count(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)room_party_count((const wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_room_stats_refresh(intptr_t a0, intptr_t a1, ...) { room_stats_refresh((wl_room *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sfx_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sfx_play((void *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_skill_row_find(intptr_t a0, ...) { return (intptr_t)skill_row_find((int32_t)a0); }
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
static inline intptr_t W_ui_link_drop(intptr_t a0, ...) { return (intptr_t)ui_link_drop((uint8_t *)a0); }
static inline intptr_t W_ui_ref_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)ui_ref_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1, ...) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }
static inline intptr_t W_win_ctrl_at(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_ctrl_at((const uint8_t *)a0, (uint16_t)a1); }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_ctrl_to_back(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_ctrl_to_back((uint8_t *)a0, (uint16_t)a1); }
static inline intptr_t W_win_grid_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_grid_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_grid_new(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_grid_new((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_win_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)win_init((uint8_t *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const void *)a7); }
static inline intptr_t W_win_mgr_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_mgr_open((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (const void *)a6); }
static inline intptr_t W_win_mgr_topmost(intptr_t a0, ...) { win_mgr_topmost((uint8_t *)a0); return 0; }
static inline intptr_t W_win_text_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_text_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_win_title_add(intptr_t a0, intptr_t a1, ...) { return (intptr_t)win_title_add((uint8_t *)a0, (const char *)a1); }

int32_t mm_5915c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
void mm_592020(char * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int16_t a6);
int32_t mm_592190(wl_char *self, int32_t a2);
int32_t mm_592850(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5);
void mm_593d90(wl_char *self, int32_t a2);
void mm_593e40(int32_t a1, uint16_t a2, char * a3, int32_t a4, int32_t a5, int16_t a6);
int32_t mm_593f60(wl_char *self, int32_t a2);
int32_t mm_5943e0(wl_char *self, int32_t a2, int32_t a3);
int32_t mm_594a20(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8, char * a9, int32_t * a10, int32_t a11, int32_t a12, int32_t a13);
int32_t mm_594c80(wl_char *self, int32_t * a2, int32_t a3);
int32_t mm_594f40(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t mm_595320(wl_char *self, int16_t a2, int32_t a3);
int32_t mm_595350(wl_char *self, int32_t a2, int32_t a3);
int32_t mm_5956c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t mm_595dd0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t mm_5962a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
int32_t mm_5963a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t mm_596620(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t mm_596ac0(int32_t a1, char * a2);
int32_t mm_596b50(wl_char *self);
int32_t mm_597480(void);
int32_t mm_5974b0(wl_char *self, int32_t a2);
int32_t mm_597670(wl_char *self, int32_t a2);
int32_t mm_598d00(wl_char *self);
int32_t mm_5990b0(int32_t a1, char * a2);
int32_t mm_599140(wl_char *self, int32_t * a2, int32_t a3);
int32_t mm_5994e0(wl_char *self, char * a2);
int32_t mm_599a60(wl_char *self, uint16_t * a2, uint16_t * a3, int32_t * a4, int32_t a5, uint16_t * a6);
int32_t mm_599ac0(wl_char *self, int32_t a2);
int32_t mm_599e60(wl_char *self, int32_t * a2, int32_t a3);

/* 0x5915c0: int __thiscall sub_5915C0(cls_5915C0 *this, int a2, int a3, int a4) */
int32_t mm_5915c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    char * v17 = 0;
    char * v18 = 0;
    intptr_t v19 = 0;
    char * v20 = 0;
    char * v21 = 0;
    intptr_t v22 = 0;
    char * v23 = 0;
    char * v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    int32_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    char * v35 = 0;
    char * v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    char * v39 = 0;
    intptr_t v40 = 0;
    int v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    uint16_t v44 = 0;
    intptr_t v45 = 0;
    int32_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t v49 = 0;
    char * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    _Alignas(8) uint8_t frame_[0xab0 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v69 = (char *)(frame_ + 0x3c);   /* [80] at ebp-0xa74 */
    char *v70 = (char *)(frame_ + 0x8c);   /* [36] at ebp-0xa24 */
    char *Buffer = (char *)(frame_ + 0xb0);   /* [512] at ebp-0xa00 */
    char *v72 = (char *)(frame_ + 0x2b0);   /* [2048] at ebp-0x800 */
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xab0 */
    intptr_t *const v55__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xab0 */
    intptr_t *const v57__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0xaac */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xa9c */
    int16_t *const v59__f = (int16_t *)(frame_ + 0x18);   /* ebp-0xa98 */
    int16_t *const v60__f = (int16_t *)(frame_ + 0x1a);   /* ebp-0xa96 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0xa94 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xa90 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0xa8c */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0xa88 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xa84 */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0xa80 */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0xa7c */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xa78 */

  (*v63__f) = c;
  (*v67__f) = 0;
  (*v64__f) = 0;
  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2f34)), (const char *)a3);
  v5 = F32(c, 0x0);
  v6 = 0;
  v7 = 5;
  if ( Buffer != 0 )
  {
    v6 = 1;
    v7 = 6;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v70), (intptr_t)(v6));
  v8 = W_win_mgr_open((intptr_t)(v5), (intptr_t)(80), (intptr_t)(48), (intptr_t)(480), (intptr_t)(372), (intptr_t)(v7), (intptr_t)(v70));
  v9 = v8;
  (*v65__f) = v8;
  if ( &(*v57__f) != (int *)-172 )
    W_win_title_add((intptr_t)((int)v8), (intptr_t)(Buffer));
  v10 = W_win_ctrl_take((intptr_t)(v9));
  v11 = v10;
  if ( v10 != 0 )
    W_ctrl_grid_init((intptr_t)(v10), (intptr_t)(216), (intptr_t)(48), (intptr_t)(10), (intptr_t)(5), (intptr_t)(10), (intptr_t)(1));
  else
    v11 = 0;
  (*v58__f) = 0x10000;
  (*v59__f) = 2;
  (*v60__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v11) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v11, 0x14, 4) = -9;
  }
  v12 = (_DWORD *)F32(v11, 0x178);
  (*v62__f) = 10;
  *v12 = 0;
  F32(F32(v11, 0x178), 4) = -6;
  F32(F32(v11, 0x178), 8) = 0;
  F32(F32(v11, 0x178), 12) = 0;
  F32(F32(v11, 0x178), 36) = 32;
  F32(F32(v11, 0x178), 40) = 0;
  F32(F32(v11, 0x178), 44) = 0;
  F32(F32(v11, 0x178), 48) = 0;
  F32(F32(v11, 0x178), 72) = 84;
  F32(F32(v11, 0x178), 76) = 0;
  F32(F32(v11, 0x178), 80) = 40;
  F32(F32(v11, 0x178), 84) = 1;
  F32(F32(v11, 0x178), 108) = 132;
  F32(F32(v11, 0x178), 112) = 0;
  F32(F32(v11, 0x178), 116) = 8;
  F32(F32(v11, 0x178), 120) = 0;
  F32(F32(v11, 0x178), 144) = 148;
  F32(F32(v11, 0x178), 148) = 0;
  F32(F32(v11, 0x178), 152) = 40;
  F32(F32(v11, 0x178), 156) = 1;
  do
  {
    v13 = F32(v11, 0x174);
    v14 = F32(v13, 16);
    if ( v14 < F32(v13, 4) )
    {
      (*v55__f) = F32(v13, 16);
      F32(v13, 16) = v14 + 1;
      v15 = 16 * v14;
      F32((F32(v11, 0x17c) + v15), 0) = 0;
      F32(F32(v11, 0x17c) + v15, 4) = 0;
      F32(F32(v11, 0x17c) + v15, 8) = 1;
      W_grid_row_reset((intptr_t)(v11), (intptr_t)((*v55__f)));
    }
    --(*v62__f);
  }
  while ( (*v62__f) != 0 );
  v16 = (F32(a3, 216) + F32(a3, 224));
  (*v58__f) = (F32(a2, 216) + F32(a2, 224));
  if ( (*v58__f) == v16 )
    W_cell_color_set((intptr_t)(v11), (intptr_t)(0), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  else
    W_cell_color_set((intptr_t)(v11), (intptr_t)(0), (intptr_t)(4), (intptr_t)(1), (intptr_t)(16711680), (intptr_t)(16711680));
  W_cell_icon_set((intptr_t)(v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(0), (intptr_t)(1), (intptr_t)(((char *)D(0x8a2f2c))));
  v17 = W_rd_495dc0((intptr_t)((int)(*v58__f)), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v17));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x854c78)));
  v18 = W_rd_495dc0((intptr_t)((int)v16), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(0), (intptr_t)(4), (intptr_t)(v18));
  v19 = (F32(a3, 88) + F32(a3, 132) + F32(a3, 156));
  (*v58__f) = (F32(a2, 88) + F32(a2, 132) + F32(a2, 156));
  if ( (*v58__f) == v19 )
    W_cell_color_set((intptr_t)(v11), (intptr_t)(1), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  else
    W_cell_color_set((intptr_t)(v11), (intptr_t)(1), (intptr_t)(4), (intptr_t)(1), (intptr_t)(16711680), (intptr_t)(16711680));
  W_cell_icon_set((intptr_t)(v11), (intptr_t)(1), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(33), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(1), (intptr_t)(1), (intptr_t)(((char *)D(0x854c40))));
  v20 = W_rd_495dc0((intptr_t)((int)(*v58__f)), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(1), (intptr_t)(2), (intptr_t)(v20));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(1), (intptr_t)(3), (intptr_t)(D(0x854c78)));
  v21 = W_rd_495dc0((intptr_t)((int)v19), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(1), (intptr_t)(4), (intptr_t)(v21));
  v22 = (F32(a3, 96) + F32(a3, 136) + F32(a3, 160));
  (*v58__f) = (F32(a2, 96) + F32(a2, 136) + F32(a2, 160));
  if ( (*v58__f) == v22 )
    W_cell_color_set((intptr_t)(v11), (intptr_t)(2), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  else
    W_cell_color_set((intptr_t)(v11), (intptr_t)(2), (intptr_t)(4), (intptr_t)(1), (intptr_t)(16711680), (intptr_t)(16711680));
  W_cell_icon_set((intptr_t)(v11), (intptr_t)(2), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(34), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(2), (intptr_t)(1), (intptr_t)(((char *)D(0x854c38))));
  v23 = W_rd_495dc0((intptr_t)((int)(*v58__f)), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(2), (intptr_t)(2), (intptr_t)(v23));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(2), (intptr_t)(3), (intptr_t)(D(0x854c78)));
  v24 = W_rd_495dc0((intptr_t)((int)v22), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(2), (intptr_t)(4), (intptr_t)(v24));
  v25 = W_win_ctrl_take((intptr_t)((*v65__f)));
  v26 = v25;
  if ( v25 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v25), (intptr_t)(40), (intptr_t)(40), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v26), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_grid_row_add((intptr_t)(v26), (intptr_t)(0), (intptr_t)(0));
  }
  else
  {
    v26 = 0;
  }
  v27 = F32(a3, 80);
  if ( v27 != 0 )
    W_cell_icon_set((intptr_t)(v26), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(4 * F16(v27, 16) + 2732)), (intptr_t)(F16(v27, 18)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v28 = W_win_ctrl_take((intptr_t)((*v65__f)));
  v29 = v28;
  if ( v28 != 0 )
    W_ctrl_grid_init((intptr_t)(v28), (intptr_t)(40), (intptr_t)(200), (intptr_t)(10), (intptr_t)(5), (intptr_t)(10), (intptr_t)(1));
  else
    v29 = 0;
  (*v58__f) = 0x10000;
  (*v59__f) = 2;
  (*v60__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v29) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v29, 0x14, 4) = -9;
  }
  v30 = (_DWORD *)F32(v29, 0x178);
  (*v62__f) = 10;
  *v30 = 0;
  F32(F32(v29, 0x178), 4) = -6;
  F32(F32(v29, 0x178), 8) = 0;
  F32(F32(v29, 0x178), 12) = 0;
  F32(F32(v29, 0x178), 36) = 32;
  F32(F32(v29, 0x178), 40) = 0;
  F32(F32(v29, 0x178), 44) = 0;
  F32(F32(v29, 0x178), 48) = 0;
  F32(F32(v29, 0x178), 72) = 84;
  F32(F32(v29, 0x178), 76) = 0;
  F32(F32(v29, 0x178), 80) = 40;
  F32(F32(v29, 0x178), 84) = 1;
  F32(F32(v29, 0x178), 108) = 132;
  F32(F32(v29, 0x178), 112) = 0;
  F32(F32(v29, 0x178), 116) = 8;
  F32(F32(v29, 0x178), 120) = 0;
  F32(F32(v29, 0x178), 144) = 148;
  F32(F32(v29, 0x178), 148) = 0;
  F32(F32(v29, 0x178), 152) = 40;
  F32(F32(v29, 0x178), 156) = 1;
  do
  {
    v31 = F32(v29, 0x174);
    v32 = F32(v31, 16);
    if ( v32 < F32(v31, 4) )
    {
      (*v56__f) = F32(v31, 16);
      F32(v31, 16) = v32 + 1;
      v33 = 16 * v32;
      F32((F32(v29, 0x17c) + v33), 0) = 0;
      F32(F32(v29, 0x17c) + v33, 4) = 0;
      F32(F32(v29, 0x17c) + v33, 8) = 1;
      W_grid_row_reset((intptr_t)(v29), (intptr_t)((*v56__f)));
    }
    --(*v62__f);
  }
  while ( (*v62__f) != 0 );
  v34 = (F32(a2, 100) + F32(a2, 140));
  (*i__f) = F32(a3, 100) + F32(a3, 140);
  (*v58__f) = v34;
  if ( v34 == (*i__f) )
    W_cell_color_set((intptr_t)(v29), (intptr_t)(0), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  else
    W_cell_color_set((intptr_t)(v29), (intptr_t)(0), (intptr_t)(4), (intptr_t)(1), (intptr_t)(16711680), (intptr_t)(16711680));
  W_cell_icon_set((intptr_t)(v29), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(36), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v29), (intptr_t)(0), (intptr_t)(1), (intptr_t)(((char *)D(0x854c60))));
  v35 = W_rd_495dc0((intptr_t)((int)(*v58__f)), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v29), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v35));
  W_grid_cell_text((intptr_t)(v29), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x854c78)));
  v36 = W_rd_495dc0((intptr_t)((*i__f)), (intptr_t)(0), (intptr_t)(v69));
  W_grid_cell_text((intptr_t)(v29), (intptr_t)(0), (intptr_t)(4), (intptr_t)(v36));
  mm_592020(
    ((char *)D(0x854c58)),
    v29,
    1,
    F32(a2, 104) + F32(a2, 144),
    F32(a3, 104) + F32(a3, 144),
    37);
  mm_592020(
    ((char *)D(0x854c50)),
    v29,
    2,
    F32(a2, 108) + F32(a2, 148),
    F32(a3, 108) + F32(a3, 148),
    38);
  mm_592020(
    ((char *)D(0x854c48)),
    v29,
    3,
    F32(a2, 112) + F32(a2, 152),
    F32(a3, 112) + F32(a3, 152),
    39);
  v37 = W_win_ctrl_take((intptr_t)((*v65__f)));
  v38 = v37;
  if ( v37 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v37), (intptr_t)(40), (intptr_t)(320), (intptr_t)(5), (intptr_t)(4), (intptr_t)(5), (intptr_t)(1));
    (*v66__f) = v38;
  }
  else
  {
    v38 = 0;
    (*v66__f) = 0;
  }
  (*v58__f) = 0x10000;
  (*v59__f) = 2;
  (*v60__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v38) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v38, 0x14, 4) = -9;
  }
  W_grid_col_place((intptr_t)(v38), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-6), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v38), (intptr_t)(1), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v38), (intptr_t)(2), (intptr_t)(84), (intptr_t)(0), (intptr_t)(104), (intptr_t)(1));
  W_grid_row_add((intptr_t)(v38), (intptr_t)(0), (intptr_t)(0));
  W_cell_icon_set((intptr_t)(v38), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(43), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v38), (intptr_t)(0), (intptr_t)(1), (intptr_t)(((char *)D(0x8a2f20))));
  v39 = W_rd_495dc0((intptr_t)(F32(a3, 236)), (intptr_t)(0), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v38), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v39));
  v40 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32((*v63__f), 0x0)), (intptr_t)(336), (intptr_t)(192), (intptr_t)(256), (intptr_t)(192), (intptr_t)(5u), (intptr_t)(((char *)D(0x8a2f10))), (intptr_t)(0));
  v41 = F16(a3, 324) == 0;
  (*v58__f) = v40;
  (*v62__f) = 0;
  if ( !v41 )
  {
    do
    {
      v42 = 0;
      (*v68__f) = 0;
      v43 = 12 * (uint16_t)(*v62__f);
      (*i__f) = F32(v43 + a3, 328);
      v44 = F16(a2, 324);
      if ( v44 != 0 )
      {
        v45 = F32(v43 + a2, 328);
        while ( (*i__f) != v45 )
        {
          if ( (uint16_t)++v42 >= v44 )
            goto LABEL_52;
        }
        (*v68__f) = 1;
      }
LABEL_52:
      if ( (uint16_t)(*v64__f) < 4u && (*v68__f) == 0 )
      {
        v46 = W_skill_row_find((intptr_t)((*i__f)));
        v47 = (*v64__f);
        (*i__f) = (int)&v72[512 * (uint16_t)(*v64__f)];
        strcpy((char *)(*i__f), (uint8_t *)v46 + 4);
        v38 = (*v66__f);
        (*v64__f) = v47 + 1;
      }
    }
    while ( (uint16_t)++(*v62__f) < F16(a3, 324) );
  }
  v49 = W_win_grid_new((intptr_t)((*v58__f)), (intptr_t)(40), (intptr_t)(48), (intptr_t)(4), (intptr_t)(1), (intptr_t)(4), (intptr_t)(1));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v49), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v49), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  if ( (uint16_t)(*v64__f) != 0 )
  {
    v50 = v72;
    for ( (*i__f) = (uint16_t)(*v64__f); (*i__f) != 0; --(*i__f) )
    {
      v51 = W_grid_row_add((intptr_t)(v49), (intptr_t)(0), (intptr_t)(0));
      W_grid_cell_text((intptr_t)(v49), (intptr_t)(v51), (intptr_t)(0), (intptr_t)(v50));
      v50 += 512;
    }
  }
  while ( 1 )
  {
    v52 = W_menu_run((intptr_t)((*v63__f)), (intptr_t)(v38), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( v52 == 11 || v52 == 12 )
      break;
    if ( v52 != 13 )
    {
      (*v67__f) = v52;
      break;
    }
  }
  v53 = (*v65__f);
  *(_DWORD *)((uint8_t *)((*v58__f) + (1) * 0x50) + 0x0) = 0;
  *(_DWORD *)((uint8_t *)(v53 + (1) * 0x50) + 0x0) = 0;
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(5u));
  return (*v67__f);
}

/* 0x592020: void __stdcall sub_592020(char *a1, cls_40FA00 *a2, int a3, int a4, int a5, __int16 a6) */
void mm_592020(char * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int16_t a6)
{

    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v11 = 0;
    _Alignas(8) uint8_t frame_[0x50 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [80] at ebp-0x50 */

  v7 = 16 * a3;
  if ( a4 == a5 )
  {
    F32(F32((16 * a3 + F32(a2, 0x17c) + 12), 0), 108) = 0;
    F32(F32((16 * a3 + F32(a2, 0x17c) + 12), 0), 112) = 0;
    F32(F32((16 * a3 + F32(a2, 0x17c) + 12), 0), 116) = 0;
  }
  else
  {
    F32(F32((v7 + F32(a2, 0x17c) + 12), 0), 108) = 1;
    F32(F32((v7 + F32(a2, 0x17c) + 12), 0), 112) = 16711680;
    F32(F32((16 * a3 + F32(a2, 0x17c) + 12), 0), 116) = 16711680;
  }
  v8 = F32(a2, 0x174);
  v11 = SYS32(2952);
  if ( F32(v8, 8) > 0 && F32(v8, 16) > a3 )
  {
    if ( F32(F32((v7 + F32(a2, 0x17c) + 12), 0), 4) == 0 )
      F32(F32((v7 + F32(a2, 0x17c) + 12), 0), 4) = (intptr_t)g_eng.op_new(0x54u);
    v9 = F32(F32((v7 + F32(a2, 0x17c) + 12), 0), 4);
    F32(v9, 0) = v11;
    F16(v9, 4) = a6;
    F16(v9, 70) = 0;
    F16(v9, 72) = 0;
    F16(v9, 76) = 0;
    F16(v9, 74) = 0;
    F32(v9, 80) = 0;
  }
  W_grid_cell_text((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(1), (intptr_t)(a1));
  g_eng.h5bf3ee(Buffer, "%d", a4);
  W_grid_cell_text((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(2), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(3), (intptr_t)(D(0x854c78)));
  g_eng.h5bf3ee(Buffer, "%d", a5);
  W_grid_cell_text((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(4), (intptr_t)(Buffer));
}

/* 0x592190: int __thiscall sub_592190(cls_592190 *this, int a2) */
int32_t mm_592190(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t i = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    uint16_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    uint16_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    uint16_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t result = 0;
    intptr_t v33 = 0;
    _Alignas(8) uint8_t frame_[0x248 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v42 = (char *)(frame_ + 0x24);   /* [36] at ebp-0x224 */
    char *Buffer = (char *)(frame_ + 0x48);   /* [512] at ebp-0x200 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x248 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x248 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x234 */
    int16_t *const v37__f = (int16_t *)(frame_ + 0x18);   /* ebp-0x230 */
    int16_t *const v38__f = (int16_t *)(frame_ + 0x1a);   /* ebp-0x22e */
    int16_t *const v39__f = (int16_t *)(frame_ + 0x1c);   /* ebp-0x22c */
    int16_t *const v40__f = (int16_t *)(frame_ + 0x1e);   /* ebp-0x22a */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x228 */

  v2 = F32(c, 0x0);
  (*v36__f) = c;
  v3 = 0;
  v4 = 6;
  if ( ((char *)D(0x8a2fdc)) != 0 )
  {
    v3 = 1;
    v4 = 7;
  }
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v42), (intptr_t)(v3));
  v5 = W_win_mgr_open((intptr_t)(v2), (intptr_t)(444), (intptr_t)(184), (intptr_t)(196), (intptr_t)(292), (intptr_t)(v4), (intptr_t)(v42));
  v6 = (int)v5;
  (*v41__f) = v5;
  if ( ((char *)D(0x8a2fdc)) != 0 )
  {
    v7 = W_win_grid_add((intptr_t)(v5), (intptr_t)(32), (intptr_t)(7), (intptr_t)(0), (intptr_t)(0));
    v8 = v7;
    if ( v7 != 0 )
    {
      W_grid_cell_text((intptr_t)(v7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2fdc))));
      *(_DWORD *)((uint8_t *)(v8 + (1) * 0x180) + 0x0) = 0xFFFFFF;
      *(_DWORD *)&GAP((v8 + (1) * 0x180), 0x0, 4) = 4546427;
    }
  }
  v9 = F16(v6, 78);
  if ( F16(v6, 76) > v9
    && (v10 = F32((F32(v6, 72) + 4 * v9), 0),
        W_ui_ref_set((intptr_t)(v10), (intptr_t)(v6), (intptr_t)(v9)),
        ++F16(v6, 78),
        v10 != 0) )
  {
    W_ctrl_grid_init((intptr_t)(v10), (intptr_t)(20), (intptr_t)(40), (intptr_t)(10), (intptr_t)(2), (intptr_t)(10), (intptr_t)(1));
  }
  else
  {
    v10 = 0;
  }
  (*v37__f) = 0;
  (*v38__f) = 1;
  (*v39__f) = 2;
  (*v40__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v10, 20) = 1;
    F32(v10, 24) = -9;
  }
  F32(F32(v10, 376), 0) = 0;
  F32(F32((v10 + 376), 0), 4) = 0;
  F32(F32((v10 + 376), 0), 8) = 64;
  F32(F32((v10 + 376), 0), 12) = 0;
  F32(F32((v10 + 376), 0), 36) = 64;
  F32(F32((v10 + 376), 0), 40) = 0;
  F32(F32((v10 + 376), 0), 44) = 92;
  F32(F32((v10 + 376), 0), 48) = 1;
  for ( i = 10; i != 0; --i )
  {
    v12 = F32(v10, 372);
    v13 = F32(v12, 16);
    if ( v13 < F32(v12, 4) )
    {
      (*v34__f) = F32(v12, 16);
      F32(v12, 16) = v13 + 1;
      v14 = 16 * v13;
      F32((v14 + F32(v10, 380)), 0) = 0;
      F32(v14 + F32((v10 + 380), 0), 4) = 0;
      F32(v14 + F32((v10 + 380), 0), 8) = 1;
      W_grid_row_reset((intptr_t)(v10), (intptr_t)((*v34__f)));
    }
  }
  v15 = 0;
  do
    ++v15;
  while ( ((int32_t *)D(0x690b40))[4 * v15] >= 0 );
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2fcc))));
  g_eng.h5bf3ee(Buffer, "%d / %d", F32(a2, 16412), v15 - 1);
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(0), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(1), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2fb0))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16564));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(1), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(2), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2fa0))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16572));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(2), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(3), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f94))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16568));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(3), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(4), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f80))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16560));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(4), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(5), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f70))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16604));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(5), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(6), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f5c))));
  v16 = F16(v6, 78);
  if ( F16(v6, 76) > v16
    && (v17 = F32((F32(v6, 72) + 4 * v16), 0),
        W_ui_ref_set((intptr_t)(v17), (intptr_t)(v6), (intptr_t)(v16)),
        ++F16(v6, 78),
        v17 != 0) )
  {
    W_ctrl_grid_init((intptr_t)(v17), (intptr_t)(32), (intptr_t)(236), (intptr_t)(4), (intptr_t)(3), (intptr_t)(4), (intptr_t)(1));
  }
  else
  {
    v17 = 0;
  }
  (*v37__f) = 0;
  (*v38__f) = 1;
  (*v39__f) = 2;
  (*v40__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v17, 20) = 1;
    F32(v17, 24) = -9;
  }
  F32(F32(v17, 376), 0) = 0;
  F32(F32((v17 + 376), 0), 4) = 0;
  F32(F32((v17 + 376), 0), 8) = 0;
  F32(F32((v17 + 376), 0), 12) = 0;
  F32(F32((v17 + 376), 0), 36) = 32;
  F32(F32((v17 + 376), 0), 40) = 0;
  F32(F32((v17 + 376), 0), 44) = 40;
  F32(F32((v17 + 376), 0), 48) = 1;
  v18 = 4;
  F32(F32((v17 + 376), 0), 72) = 76;
  F32(F32((v17 + 376), 0), 76) = 0;
  F32(F32((v17 + 376), 0), 80) = 0;
  F32(F32((v17 + 376), 0), 84) = 0;
  F32(v17, 428) = 24;
  do
  {
    v19 = F32(v17, 372);
    v20 = F32(v19, 16);
    if ( v20 < F32(v19, 4) )
    {
      (*v35__f) = F32(v19, 16);
      F32(v19, 16) = v20 + 1;
      v21 = 16 * v20;
      F32((v21 + F32(v17, 380)), 0) = 0;
      F32(v21 + F32((v17 + 380), 0), 4) = 0;
      F32(v21 + F32((v17 + 380), 0), 8) = 1;
      W_grid_row_reset((intptr_t)(v17), (intptr_t)((*v35__f)));
    }
    --v18;
  }
  while ( v18 != 0 );
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f54))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16576));
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(0), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(0), (intptr_t)(2), (intptr_t)(D(0x8a202c)));
  v22 = F32(a2, 16580);
  if ( v22 != 0 )
  {
    v23 = 0;
    while ( 1 )
    {
      v24 = F32(a2 + 4 * v23, 16432);
      if ( F32(v24, 2544) == v22 )
        break;
      if ( ++v23 >= 8u )
        goto LABEL_37;
    }
    if ( (F32(v24, 2500) == 1 || F32(v24, 2500) == 2) && v24 != 0 )
    {
      g_eng.h5bf3ee(Buffer, "(%s)", (const char *)(v24 + 2576));
      W_grid_cell_text((intptr_t)(v17), (intptr_t)(0), (intptr_t)(2), (intptr_t)(Buffer));
    }
  }
LABEL_37:
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(1), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f44))));
  g_eng.h5bf3ee(Buffer, "%d", F32(a2, 16584));
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(1), (intptr_t)(1), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(1), (intptr_t)(2), (intptr_t)(D(0x8a202c)));
  v25 = F32(a2, 16588);
  if ( v25 != 0 )
  {
    v26 = 0;
    while ( 1 )
    {
      v27 = F32(a2, 4 * v26 + 16432);
      if ( F32(v27, 0x9f0) == v25 )
        break;
      if ( ++v26 >= 8u )
        goto LABEL_45;
    }
    if ( W_pslot_ok((intptr_t)(v27), (intptr_t)(1), (intptr_t)(1)) && v27 != 0 )
    {
      g_eng.h5bf3ee(Buffer, "(%s)", &GAP((v27 + (1) * 0x9f4), 0x0, 28));
      W_grid_cell_text((intptr_t)(v17), (intptr_t)(1), (intptr_t)(2), (intptr_t)(Buffer));
    }
  }
LABEL_45:
  v28 = 24;
  if ( ((char *)D(0x8a2ee4)) != 0 )
    v28 = 56;
  v29 = W_room_party_count((intptr_t)(a2), (intptr_t)(0), (intptr_t)(0));
  v30 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32((*v36__f), 0x0)), (intptr_t)(444), (intptr_t)(4), (intptr_t)(192), (intptr_t)(176), (intptr_t)(2u), (intptr_t)(((char *)D(0x8a2ee4))), (intptr_t)(0));
  v31 = W_win_grid_new((intptr_t)(v30), (intptr_t)(40), (intptr_t)(v28), (intptr_t)(v29), (intptr_t)(1), (intptr_t)(v29), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v31), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v31), (intptr_t)(1));
  W_member_rows_add((intptr_t)(a2), (intptr_t)(v31), (intptr_t)(0));
  if ( &(*v36__f) == 0 )
    v31 = (*v36__f);
  result = mm_592850(
             (*v36__f),
             v30,
             v31,
             (_DWORD *)a2,
             F32(16 * F32((F32(v31 + (93) * 0x4, 0x0) + 20), 0) + F32(v31 + (95) * 0x4, 0x0), 4));
  v33 = (*v41__f);
  F32(v30, 80) = 0;
  *(_DWORD *)((uint8_t *)(v33 + (1) * 0x50) + 0x0) = 0;
  return result;
}

/* 0x592850: int __thiscall sub_592850(cls_592850 *this, int a2, _DWORD *a3, _DWORD *a4, int a5) */
int32_t mm_592850(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t result = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int32_t * v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    int v24 = 0;
    uint16_t v25 = 0;
    int32_t * v26 = 0;
    int32_t * v27 = 0;
    intptr_t v28 = 0;
    int16_t v29 = 0;
    uint16_t v30 = 0;
    int32_t * v31 = 0;
    int32_t * v32 = 0;
    intptr_t v33 = 0;
    int16_t v34 = 0;
    uint16_t v35 = 0;
    int32_t * v36 = 0;
    int32_t * v37 = 0;
    intptr_t v38 = 0;
    int16_t v39 = 0;
    uint16_t v40 = 0;
    int32_t * v41 = 0;
    int32_t * v42 = 0;
    intptr_t v43 = 0;
    int16_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    uint16_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    int32_t * v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
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
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    uint16_t v72 = 0;
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
    intptr_t v85 = 0;
    char * v86 = 0;
    char * v87 = 0;
    intptr_t v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    char * v91 = 0;
    char * v92 = 0;
    intptr_t v93 = 0;
    char * v94 = 0;
    intptr_t v95 = 0;
    intptr_t v96 = 0;
    intptr_t v97 = 0;
    intptr_t v98 = 0;
    intptr_t v99 = 0;
    _Alignas(8) uint8_t frame_[0x678 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v104 = (uint8_t *)(frame_ + 0x4);   /* [16] at ebp-0x674 */
    uint8_t *v113 = (uint8_t *)(frame_ + 0x30);   /* [36] at ebp-0x648 */
    uint8_t *v114 = (uint8_t *)(frame_ + 0x54);   /* [36] at ebp-0x624 */
    char *v115 = (char *)(frame_ + 0x78);   /* [512] at ebp-0x600 */
    char *Buffer = (char *)(frame_ + 0x278);   /* [512] at ebp-0x400 */
    char *v117 = (char *)(frame_ + 0x478);   /* [512] at ebp-0x200 */
    intptr_t *const v103__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x678 */
    intptr_t *const v102__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x678 */
    intptr_t *const v101__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x678 */
    intptr_t *const v100__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x678 */
    intptr_t *const v105__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x664 */
    int16_t *const v106__f = (int16_t *)(frame_ + 0x18);   /* ebp-0x660 */
    int16_t *const v107__f = (int16_t *)(frame_ + 0x1a);   /* ebp-0x65e */
    intptr_t *const v108__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x65c */
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x658 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x654 */
    intptr_t *const v111__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x650 */
    intptr_t *const v112__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x64c */

  (*i__f) = c;
  (*v111__f) = 0;
  v6 = W_pslot_by_id((intptr_t)(a4), (intptr_t)(a5), (intptr_t)(0));
  if ( v6 == 0 )
    return 0;
  v8 = v6 + 2576;
  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a3060)), (const char *)(v6 + 2576));
  v9 = F32(c, 0x0);
  v10 = 0;
  v11 = 6;
  if ( Buffer != 0 )
  {
    v10 = 1;
    v11 = 7;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v113), (intptr_t)(v10));
  v12 = W_win_mgr_open((intptr_t)(v9), (intptr_t)(4), (intptr_t)(306), (intptr_t)(440), (intptr_t)(172), (intptr_t)(v11), (intptr_t)(v113));
  (*v112__f) = v12;
  if ( v104 != (_BYTE *)-628 )
    W_win_title_add((intptr_t)(v12), (intptr_t)(Buffer));
  v13 = F32(v12, 12);
  v14 = F32(v12, 16);
  F32(v12, 36) = v14;
  F32(v12, 44) = v14;
  F32(v12, 60) = v14;
  F32(v12, 32) = v13 - 512;
  F32(v12, 40) = v13 - 512;
  F32(v12, 12) = v13 - 512;
  F32(v12, 28) = 2;
  F32(v12, 80) = 1;
  F32(v12, 84) = 0;
  F32(v12, 88) = 1;
  F32(v12, 56) = v13;
  F32(v12, 48) = 1000;
  F32(v12, 52) = 1000;
  F32(v12, 64) = 0;
  F32(v12, 68) = 0;
  v15 = W_win_ctrl_take((intptr_t)(v12));
  v16 = v15;
  if ( v15 != 0 )
    W_ctrl_grid_init((intptr_t)(v15), (intptr_t)(40), (intptr_t)(39), (intptr_t)(4), (intptr_t)(6), (intptr_t)(4), (intptr_t)(1));
  else
    v16 = 0;
  (*v105__f) = 0x10000;
  (*v106__f) = 2;
  (*v107__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v16) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v16, 0x14, 4) = -9;
  }
  v17 = (_DWORD *)F32(v16, 0x178);
  *(_DWORD *)&GAP((v16 + (1) * 0x180), 0x24, 8) = 32;
  (*v109__f) = 0;
  *v17 = 0;
  v18 = F32(v16, 0x178);
  (*v108__f) = 4;
  F32(v18, 4) = 0;
  F32(F32(v16, 0x178), 8) = 0;
  F32(F32(v16, 0x178), 12) = 0;
  F32(F32(v16, 0x178), 36) = 64;
  F32(F32(v16, 0x178), 40) = -8;
  F32(F32(v16, 0x178), 44) = 0;
  F32(F32(v16, 0x178), 48) = 0;
  F32(F32(v16, 0x178), 72) = 64;
  v19 = 0;
  F32(F32(v16, 0x178), 76) = -8;
  F32(F32(v16, 0x178), 80) = 0;
  F32(F32(v16, 0x178), 84) = 0;
  F32(F32(v16, 0x178), 108) = 104;
  F32(F32(v16, 0x178), 112) = 0;
  F32(F32(v16, 0x178), 116) = 0;
  F32(F32(v16, 0x178), 120) = 0;
  F32(F32(v16, 0x178), 144) = 272;
  F32(F32(v16, 0x178), 148) = 0;
  F32(F32(v16, 0x178), 152) = 0;
  F32(F32(v16, 0x178), 156) = 0;
  F32(F32(v16, 0x178), 180) = 296;
  F32(F32(v16, 0x178), 184) = 0;
  F32(F32(v16, 0x178), 188) = 32;
  F32(F32(v16, 0x178), 192) = 1;
  do
  {
    v20 = F32(v16, 0x174);
    v21 = F32(v20, 16);
    if ( v21 < F32(v20, 4) )
    {
      (*v100__f) = F32(v20, 16);
      F32(v20, 16) = v21 + 1;
      v22 = 16 * v21;
      F32((F32(v16, 0x17c) + v22), 0) = 0;
      F32(F32(v16, 0x17c) + v22, 4) = 0;
      F32(F32(v16, 0x17c) + v22, 8) = 1;
      W_grid_row_reset((intptr_t)(v16), (intptr_t)((*v100__f)));
    }
    (*v105__f) = SYS32(2932);
    v23 = F32(v16, 0x174);
    if ( F32(v23, 8) > 1 && F32(v23, 16) > (int)(*v109__f) )
    {
      if ( F32(F32((F32(v16, 0x17c) + v19 + 12), 0), 28) == 0 )
        F32(F32((F32(v16, 0x17c) + v19 + 12), 0), 28) = (intptr_t)g_eng.op_new(0x54u);
      W_icon_set((intptr_t)(F32(F32((F32(v16, 0x17c) + v19 + 12), 0), 28)), (intptr_t)((*v105__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
    v19 += 16;
    v24 = (*v108__f) == 1;
    (*v109__f) = ((uint8_t *)(*v109__f) + 1);
    --(*v108__f);
  }
  while ( !v24 );
  v25 = 0;
  if ( F16(v8, 288) != 0 )
  {
    while ( 1 )
    {
      v26 = W_item_def((intptr_t)(F32(v8 + 4 * v25, 256)));
      if ( v26 != 0 && v26[30] == 1 )
        break;
      if ( ++v25 >= F16(v8, 288) )
        goto LABEL_25;
    }
    v27 = v26;
  }
  else
  {
LABEL_25:
    v27 = 0;
  }
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f54))));
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(0), (intptr_t)(4), (intptr_t)(((char *)D(0x8a305c))));
  if ( v27 != 0 )
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(0), (intptr_t)(3), (intptr_t)((uint8_t *)v27 + 4));
    g_eng.h5bf3ee(v115, "%d", v27[32]);
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(0), (intptr_t)(5), (intptr_t)(v115));
    v28 = F32(v16, 0x174);
    v29 = F16(v27, 2 * 42);
    (*v105__f) = SYS32(2932);
    if ( F32(v28, 8) > 2 && F32(v28, 16) > 0 )
    {
      if ( F32(F32((F32(v16, 0x17c) + 12), 0), 52) == 0 )
        F32(F32((F32(v16, 0x17c) + 12), 0), 52) = (intptr_t)g_eng.op_new(0x54u);
      W_icon_set((intptr_t)(F32(F32((F32(v16, 0x17c) + 12), 0), 52)), (intptr_t)((*v105__f)), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
  }
  else
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x854c7c)));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(0), (intptr_t)(5), (intptr_t)(D(0x8a202c)));
  }
  v30 = 0;
  if ( F16(v8, 288) != 0 )
  {
    while ( 1 )
    {
      v31 = W_item_def((intptr_t)(F32(v8 + 4 * v30, 256)));
      if ( v31 != 0 && v31[30] == 2 )
        break;
      if ( ++v30 >= F16(v8, 288) )
        goto LABEL_38;
    }
    v32 = v31;
  }
  else
  {
LABEL_38:
    v32 = 0;
  }
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(1), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3050))));
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(1), (intptr_t)(4), (intptr_t)(((char *)D(0x8a304c))));
  if ( v32 != 0 )
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(1), (intptr_t)(3), (intptr_t)((uint8_t *)v32 + 4));
    g_eng.h5bf3ee(v115, "%d", v32[33]);
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(1), (intptr_t)(5), (intptr_t)(v115));
    v33 = F32(v16, 0x174);
    v34 = F16(v32, 2 * 42);
    (*v105__f) = SYS32(2932);
    if ( F32(v33, 8) > 2 && F32(v33, 16) > 1 )
    {
      if ( F32(F32((F32(v16, 0x17c) + 28), 0), 52) == 0 )
        F32(F32((F32(v16, 0x17c) + 28), 0), 52) = (intptr_t)g_eng.op_new(0x54u);
      W_icon_set((intptr_t)(F32(F32((F32(v16, 0x17c) + 28), 0), 52)), (intptr_t)((*v105__f)), (intptr_t)(v34), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
  }
  else
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(1), (intptr_t)(3), (intptr_t)(D(0x854c7c)));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(1), (intptr_t)(5), (intptr_t)(D(0x8a202c)));
  }
  v35 = 0;
  if ( F16(v8, 288) != 0 )
  {
    while ( 1 )
    {
      v36 = W_item_def((intptr_t)(F32(v8 + 4 * v35, 256)));
      if ( v36 != 0 && v36[30] == 3 )
        break;
      if ( ++v35 >= F16(v8, 288) )
        goto LABEL_51;
    }
    v37 = v36;
  }
  else
  {
LABEL_51:
    v37 = 0;
  }
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(2), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3040))));
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(2), (intptr_t)(4), (intptr_t)(((char *)D(0x8a304c))));
  if ( v37 != 0 )
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(2), (intptr_t)(3), (intptr_t)((uint8_t *)v37 + 4));
    g_eng.h5bf3ee(v115, "%d", v37[33]);
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(2), (intptr_t)(5), (intptr_t)(v115));
    v38 = F32(v16, 0x174);
    v39 = F16(v37, 2 * 42);
    (*v105__f) = SYS32(2932);
    if ( F32(v38, 8) > 2 && F32(v38, 16) > 2 )
    {
      if ( F32(F32((F32(v16, 0x17c) + 44), 0), 52) == 0 )
        F32(F32((F32(v16, 0x17c) + 44), 0), 52) = (intptr_t)g_eng.op_new(0x54u);
      W_icon_set((intptr_t)(F32(F32((F32(v16, 0x17c) + 44), 0), 52)), (intptr_t)((*v105__f)), (intptr_t)(v39), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
  }
  else
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(2), (intptr_t)(3), (intptr_t)(D(0x854c7c)));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(2), (intptr_t)(5), (intptr_t)(D(0x8a202c)));
  }
  v40 = 0;
  if ( F16(v8, 288) != 0 )
  {
    while ( 1 )
    {
      v41 = W_item_def((intptr_t)(F32(v8 + 4 * v40, 256)));
      if ( v41 != 0 && v41[30] == 4 )
        break;
      if ( ++v40 >= F16(v8, 288) )
        goto LABEL_64;
    }
    v42 = v41;
  }
  else
  {
LABEL_64:
    v42 = 0;
  }
  W_grid_cell_text((intptr_t)(v16), (intptr_t)(3), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3034))));
  if ( v42 != 0 )
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(3), (intptr_t)(3), (intptr_t)((uint8_t *)v42 + 4));
    v43 = F32(v16, 0x174);
    v44 = F16(v42, 2 * 42);
    (*v105__f) = SYS32(2932);
    if ( F32(v43, 8) > 2 && F32(v43, 16) > 3 )
    {
      if ( F32(F32((F32(v16, 0x17c) + 60), 0), 52) == 0 )
        F32(F32((F32(v16, 0x17c) + 60), 0), 52) = (intptr_t)g_eng.op_new(0x54u);
      W_icon_set((intptr_t)(F32(F32((F32(v16, 0x17c) + 60), 0), 52)), (intptr_t)((*v105__f)), (intptr_t)(v44), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    }
  }
  else
  {
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(3), (intptr_t)(3), (intptr_t)(D(0x854c7c)));
  }
  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2ee4)), (const char *)v8);
  v45 = 0;
  v46 = F32((*i__f), 0x0);
  v47 = 6;
  if ( Buffer != 0 )
  {
    v45 = 1;
    v47 = 7;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v114), (intptr_t)(v45));
  v48 = W_win_mgr_open((intptr_t)(v46), (intptr_t)(4), (intptr_t)(2), (intptr_t)(440), (intptr_t)(304), (intptr_t)(v47), (intptr_t)(v114));
  v49 = v48;
  (*v109__f) = v48;
  if ( v104 != (_BYTE *)-628 )
    W_win_title_add((intptr_t)((int)v48), (intptr_t)(Buffer));
  v50 = *(_DWORD *)&GAP(v49, 0x0, 12);
  v51 = *(_DWORD *)&GAP(v49, 0x0, 16);
  *(_DWORD *)&GAP(v49, 0x0, 36) = v51;
  *(_DWORD *)((uint8_t *)(v49 + (1) * 0x50) + 0x0) = 1;
  *(_DWORD *)&GAP((v49 + (1) * 0x50), 0x0, 8) = 1;
  *(_DWORD *)&GAP(v49, 0x0, 44) = v51;
  *(_DWORD *)&GAP(v49, 0x0, 60) = v51;
  *(_DWORD *)&GAP(v49, 0x0, 32) = v50 - 512;
  *(_DWORD *)&GAP(v49, 0x0, 40) = v50 - 512;
  *(_DWORD *)&GAP(v49, 0x0, 12) = v50 - 512;
  *(_DWORD *)&GAP(v49, 0x0, 28) = 2;
  *(_DWORD *)&GAP((v49 + (1) * 0x50), 0x0, 4) = 0;
  *(_DWORD *)&GAP(v49, 0x0, 56) = v50;
  *(_DWORD *)&GAP(v49, 0x0, 48) = 1000;
  *(_DWORD *)&GAP(v49, 0x0, 52) = 1000;
  *(_DWORD *)&GAP(v49, 0x0, 64) = 0;
  *(_DWORD *)&GAP(v49, 0x0, 68) = 0;
  v52 = W_win_ctrl_take((intptr_t)(v49));
  v53 = v52;
  if ( v52 != 0 )
    W_ctrl_grid_init((intptr_t)(v52), (intptr_t)(192), (intptr_t)(48), (intptr_t)(6), (intptr_t)(8), (intptr_t)(6), (intptr_t)(1));
  else
    v53 = 0;
  (*v105__f) = 0x10000;
  (*v106__f) = 2;
  (*v107__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v53) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v53, 0x14, 4) = -9;
  }
  v54 = (_DWORD *)F32(v53, 0x178);
  (*v108__f) = 6;
  *v54 = 4;
  F32(F32(v53, 0x178), 4) = 0;
  F32(F32(v53, 0x178), 8) = 64;
  F32(F32(v53, 0x178), 12) = 0;
  F32(F32(v53, 0x178), 36) = 64;
  F32(F32(v53, 0x178), 40) = 0;
  F32(F32(v53, 0x178), 44) = 40;
  F32(F32(v53, 0x178), 48) = 1;
  F32(F32(v53, 0x178), 72) = 112;
  F32(F32(v53, 0x178), 76) = 0;
  F32(F32(v53, 0x178), 80) = 0;
  F32(F32(v53, 0x178), 84) = 0;
  F32(F32(v53, 0x178), 108) = 128;
  F32(F32(v53, 0x178), 112) = 0;
  F32(F32(v53, 0x178), 116) = 40;
  F32(F32(v53, 0x178), 120) = 0;
  F32(F32(v53, 0x178), 144) = 61;
  F32(F32(v53, 0x178), 148) = -3;
  F32(F32(v53, 0x178), 152) = 0;
  F32(F32(v53, 0x178), 156) = 0;
  F32(F32(v53, 0x178), 180) = 64;
  F32(F32(v53, 0x178), 184) = 0;
  F32(F32(v53, 0x178), 188) = 0;
  F32(F32(v53, 0x178), 192) = 0;
  do
  {
    v55 = F32(v53, 0x174);
    v56 = F32(v55, 16);
    if ( v56 < F32(v55, 4) )
    {
      (*v101__f) = F32(v55, 16);
      F32(v55, 16) = v56 + 1;
      v57 = 16 * v56;
      F32((F32(v53, 0x17c) + v57), 0) = 0;
      F32(F32(v53, 0x17c) + v57, 4) = 0;
      F32(F32(v53, 0x17c) + v57, 8) = 1;
      W_grid_row_reset((intptr_t)(v53), (intptr_t)((*v101__f)));
    }
    --(*v108__f);
  }
  while ( (*v108__f) != 0 );
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2f2c))));
  g_eng.h5bf3ee(v115, "%d", F32(v8, 224) + F32(v8, 216));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(0), (intptr_t)(1), (intptr_t)(v115));
  g_eng.h5bf3ee(v115, "(MAX:%d)", F32(v8, 224) + F32(v8, 220));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v115));
  F32(F32((F32(v53, 0x17c) + 28), 0), 36) = 1;
  F32(F32((F32(v53, 0x17c) + 28), 0), 40) = 0xFFFFFF;
  F32(F32((F32(v53, 0x17c) + 28), 0), 44) = D(0x606060);
  F32(F32((F32(v53, 0x17c) + 28), 0), 60) = 1;
  F32(F32((F32(v53, 0x17c) + 28), 0), 64) = 0xFFFFFF;
  F32(F32((F32(v53, 0x17c) + 28), 0), 68) = D(0x606060);
  F32(F32((F32(v53, 0x17c) + 28), 0), 84) = 1;
  F32(F32((F32(v53, 0x17c) + 28), 0), 88) = 0xFFFFFF;
  F32(F32((F32(v53, 0x17c) + 28), 0), 92) = D(0x606060);
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(1), (intptr_t)(0), (intptr_t)(((char *)D(0x854a50))));
  g_eng.h5bf3ee(v115, "%d", F32(v8, 84));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(1), (intptr_t)(1), (intptr_t)(v115));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(1), (intptr_t)(2), (intptr_t)(D(0x8a3024)));
  g_eng.h5bf3ee(v115, "%d", F32(v8, 88) + F32(v8, 132) + F32(v8, 156));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(1), (intptr_t)(3), (intptr_t)(v115));
  (*v105__f) = SYS32(2972);
  v58 = F32(v53, 0x174);
  if ( F32(v58, 8) > 4 && F32(v58, 16) > 1 )
  {
    if ( F32(F32((F32(v53, 0x17c) + 28), 0), 100) == 0 )
      F32(F32((F32(v53, 0x17c) + 28), 0), 100) = (intptr_t)g_eng.op_new(0x54u);
    W_icon_set((intptr_t)(F32(F32((F32(v53, 0x17c) + 28), 0), 100)), (intptr_t)((*v105__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  }
  v59 = F32(v8, 88);
  v60 = F32(v8, 132) + F32(v8, 156);
  v61 = F32(v53, 0x174);
  (*v108__f) = F32(v8, 84);
  v62 = v59 + v60;
  v63 = F32(v61, 8);
  (*v105__f) = SYS32(2972);
  if ( v63 > 5 && F32(v61, 16) > 1 )
  {
    if ( F32(F32((F32(v53, 0x17c) + 28), 0), 128) == 0 )
      F32(F32((F32(v53, 0x17c) + 28), 0), 128) = (intptr_t)g_eng.op_new(0x20u);
    (*v102__f) = (*v108__f);
    v64 = F32(F32((F32(v53, 0x17c) + 28), 0), 128);
    F32(v64, 4) = (*v105__f);
    F16(v64, 8) = 1;
    F32(v64, 0) = 0;
    F32(v64, 12) = 144;
    F32(v64, 16) = 15;
    F32(v64, 20) = 0;
    F32(v64, 24) = v62;
    W_grid_gauge_clamp((intptr_t)(v53), (intptr_t)(1), (intptr_t)(5), (intptr_t)((*v102__f)));
  }
  F32(F32((F32(v53, 0x17c) + 44), 0), 36) = 1;
  F32(F32((F32(v53, 0x17c) + 44), 0), 40) = 0xFFFFFF;
  F32(F32((F32(v53, 0x17c) + 44), 0), 44) = D(0x606060);
  F32(F32((F32(v53, 0x17c) + 44), 0), 60) = 1;
  F32(F32((F32(v53, 0x17c) + 44), 0), 64) = 0xFFFFFF;
  F32(F32((F32(v53, 0x17c) + 44), 0), 68) = D(0x606060);
  F32(F32((F32(v53, 0x17c) + 44), 0), 84) = 1;
  F32(F32((F32(v53, 0x17c) + 44), 0), 88) = 0xFFFFFF;
  F32(F32((F32(v53, 0x17c) + 44), 0), 92) = D(0x606060);
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(2), (intptr_t)(0), (intptr_t)(((char *)D(0x854a4c))));
  g_eng.h5bf3ee(v115, "%d", F32(v8, 92));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(2), (intptr_t)(1), (intptr_t)(v115));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(2), (intptr_t)(2), (intptr_t)(D(0x8a3024)));
  g_eng.h5bf3ee(v115, "%d", F32(v8, 96) + F32(v8, 136) + F32(v8, 160));
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(2), (intptr_t)(3), (intptr_t)(v115));
  v65 = F32(v53, 0x174);
  v66 = SYS32(2972);
  if ( F32(v65, 8) > 4 && F32(v65, 16) > 2 )
  {
    if ( F32(F32((F32(v53, 0x17c) + 44), 0), 100) == 0 )
      F32(F32((F32(v53, 0x17c) + 44), 0), 100) = (intptr_t)g_eng.op_new(0x54u);
    W_icon_set((intptr_t)(F32(F32((F32(v53, 0x17c) + 44), 0), 100)), (intptr_t)(v66), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  }
  v67 = F32(v8, 136) + F32(v8, 160);
  (*v108__f) = F32(v8, 92);
  v68 = F32(v8, 96);
  (*v105__f) = SYS32(2972);
  v69 = F32(v53, 0x174);
  v70 = v68 + v67;
  if ( F32(v69, 8) > 5 && F32(v69, 16) > 2 )
  {
    if ( F32(F32((F32(v53, 0x17c) + 44), 0), 128) == 0 )
      F32(F32((F32(v53, 0x17c) + 44), 0), 128) = (intptr_t)g_eng.op_new(0x20u);
    v71 = F32(F32((F32(v53, 0x17c) + 44), 0), 128);
    F32(v71, 4) = (*v105__f);
    F16(v71, 8) = 2;
    F32(v71, 0) = 0;
    F32(v71, 12) = 144;
    F32(v71, 16) = 15;
    F32(v71, 20) = 0;
    F32(v71, 24) = v70;
    W_grid_gauge_clamp((intptr_t)(v53), (intptr_t)(2), (intptr_t)(5), (intptr_t)((*v108__f)));
  }
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(3), (intptr_t)(0), (intptr_t)(((char *)D(0x8a2ee4))));
  strcpy(v115, ((char *)D(0x8a1f34)));
  v72 = 0;
  while ( F16(v8 + 2 * v72, 1096) == 0 || ((int32_t *)D(0x628760))[35 * v72] != 0 )
  {
    if ( ++v72 >= 0x12u )
      goto LABEL_112;
  }
  strcpy(v115, (const char *)(140 * v72 + 6457100));
LABEL_112:
  W_grid_cell_text((intptr_t)(v53), (intptr_t)(3), (intptr_t)(1), (intptr_t)(v115));
  v73 = (*v109__f);
  v74 = W_win_ctrl_take((intptr_t)((*v109__f)));
  v75 = v74;
  if ( v74 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v74), (intptr_t)(32), (intptr_t)(36), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v75), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_grid_row_add((intptr_t)(v75), (intptr_t)(0), (intptr_t)(0));
  }
  else
  {
    v75 = 0;
  }
  v76 = F32(v8, 80);
  if ( v76 != 0 )
    W_cell_icon_set((intptr_t)(v75), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(4 * F16(v76, 12) + 2732)), (intptr_t)(F16(v76, 14)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v77 = W_win_ctrl_take((intptr_t)(v73));
  v78 = v77;
  if ( v77 != 0 )
    W_ctrl_grid_init((intptr_t)(v77), (intptr_t)(28), (intptr_t)(184), (intptr_t)(10), (intptr_t)(7), (intptr_t)(10), (intptr_t)(1));
  else
    v78 = 0;
  (*v105__f) = 0x10000;
  (*v106__f) = 2;
  (*v107__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v78) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v78, 0x14, 4) = -9;
  }
  v79 = (_DWORD *)F32(v78, 0x178);
  (*v108__f) = 10;
  *v79 = 0;
  F32(F32(v78, 0x178), 4) = -6;
  F32(F32(v78, 0x178), 8) = 64;
  F32(F32(v78, 0x178), 12) = 0;
  F32(F32(v78, 0x178), 36) = 32;
  F32(F32(v78, 0x178), 40) = 0;
  F32(F32(v78, 0x178), 44) = 64;
  F32(F32(v78, 0x178), 48) = 0;
  F32(F32(v78, 0x178), 72) = 80;
  F32(F32(v78, 0x178), 76) = 0;
  F32(F32(v78, 0x178), 80) = 32;
  F32(F32(v78, 0x178), 84) = 1;
  F32(F32(v78, 0x178), 108) = 116;
  F32(F32(v78, 0x178), 112) = 0;
  F32(F32(v78, 0x178), 116) = 0;
  F32(F32(v78, 0x178), 120) = 0;
  F32(F32(v78, 0x178), 144) = 128;
  F32(F32(v78, 0x178), 148) = 0;
  F32(F32(v78, 0x178), 152) = 32;
  F32(F32(v78, 0x178), 156) = 0;
  do
  {
    v80 = F32(v78, 0x174);
    v81 = F32(v80, 16);
    if ( v81 < F32(v80, 4) )
    {
      (*v103__f) = F32(v80, 16);
      F32(v80, 16) = v81 + 1;
      v82 = 16 * v81;
      F32((v82 + F32(v78, 0x17c)), 0) = 0;
      F32(v82 + F32(v78, 0x17c), 4) = 0;
      F32(v82 + F32(v78, 0x17c), 8) = 1;
      W_grid_row_reset((intptr_t)(v78), (intptr_t)((*v103__f)));
    }
    --(*v108__f);
  }
  while ( (*v108__f) != 0 );
  v83 = F32(v8, 140);
  v84 = F32(v8, 100);
  (*v105__f) = F32(v8, 116);
  v85 = v84 + v83;
  W_cell_icon_set((intptr_t)(v78), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(36), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v78), (intptr_t)(0), (intptr_t)(1), (intptr_t)(((char *)D(0x854c60))));
  v86 = W_rd_495dc0((intptr_t)(v85 + (*v105__f)), (intptr_t)(0), (intptr_t)(v117));
  W_grid_cell_text((intptr_t)(v78), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v86));
  W_grid_cell_text((intptr_t)(v78), (intptr_t)(0), (intptr_t)(3), (intptr_t)(D(0x8a3024)));
  v87 = W_rd_495dc0((intptr_t)(v85), (intptr_t)(0), (intptr_t)(v117));
  W_grid_cell_text((intptr_t)(v78), (intptr_t)(0), (intptr_t)(4), (intptr_t)(v87));
  v88 = (*i__f);
  mm_593e40(v78, 1u, ((char *)D(0x854c58)), F32(v8, 104) + F32(v8, 144), F32(v8, 120), 37);
  mm_593e40(v78, 2u, ((char *)D(0x854c50)), F32(v8, 108) + F32(v8, 148), F32(v8, 124), 38);
  mm_593e40(v78, 3u, ((char *)D(0x854c48)), F32(v8, 112) + F32(v8, 152), F32(v8, 128), 39);
  v89 = W_win_ctrl_take((intptr_t)((*v109__f)));
  v90 = v89;
  if ( v89 != 0 )
    W_ctrl_grid_init((intptr_t)(v89), (intptr_t)(244), (intptr_t)(240), (intptr_t)(4), (intptr_t)(4), (intptr_t)(4), (intptr_t)(1));
  else
    v90 = 0;
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v90), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v90), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-6), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v90), (intptr_t)(1), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v90), (intptr_t)(2), (intptr_t)(104), (intptr_t)(0), (intptr_t)(64), (intptr_t)(1));
  for ( (*i__f) = 4; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
    W_grid_row_add((intptr_t)(v90), (intptr_t)(0), (intptr_t)(0));
  W_cell_icon_set((intptr_t)(v90), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(42), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v90), (intptr_t)(0), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3020))));
  v91 = W_rd_495dc0((intptr_t)(F32(v8, 232)), (intptr_t)(0), (intptr_t)(v115));
  W_grid_cell_text((intptr_t)(v90), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v91));
  W_cell_icon_set((intptr_t)(v90), (intptr_t)(1), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(43), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v90), (intptr_t)(1), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3010))));
  v92 = W_rd_495dc0((intptr_t)(F32(v8, 236)), (intptr_t)(0), (intptr_t)(v115));
  W_grid_cell_text((intptr_t)(v90), (intptr_t)(1), (intptr_t)(2), (intptr_t)(v92));
  v93 = W_win_grid_new((intptr_t)((*v109__f)), (intptr_t)(244), (intptr_t)(184), (intptr_t)(4), (intptr_t)(4), (intptr_t)(4), (intptr_t)(1));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v93), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v93), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-6), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v93), (intptr_t)(1), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v93), (intptr_t)(2), (intptr_t)(104), (intptr_t)(0), (intptr_t)(64), (intptr_t)(1));
  for ( (*i__f) = 4; (*i__f) != 0; (*i__f) = ((uint8_t *)(*i__f) - 1) )
    W_grid_row_add((intptr_t)(v93), (intptr_t)(0), (intptr_t)(0));
  W_cell_icon_set((intptr_t)(v93), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(40), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v93), (intptr_t)(0), (intptr_t)(1), (intptr_t)(((char *)D(0x8a2ffc))));
  v94 = W_rd_495dc0((intptr_t)(F32(v8, 224)), (intptr_t)(0), (intptr_t)(v115));
  W_grid_cell_text((intptr_t)(v93), (intptr_t)(0), (intptr_t)(2), (intptr_t)(v94));
  W_cell_icon_set((intptr_t)(v93), (intptr_t)(1), (intptr_t)(0), (intptr_t)(SYS32(2952)), (intptr_t)(41), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v93), (intptr_t)(1), (intptr_t)(1), (intptr_t)(((char *)D(0x8a2fec))));
  g_eng.h5bf3ee(v115, "%d / %d", F32(v8, 216), F32(v8, 220));
  W_grid_cell_text((intptr_t)(v93), (intptr_t)(1), (intptr_t)(2), (intptr_t)(v115));
  mm_593d90(F32(a2, 0), a2);
  while ( 1 )
  {
    v95 = W_menu_run((intptr_t)(v88), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( v95 == 11 )
    {
      v97 = (*v111__f);
      goto LABEL_143;
    }
    if ( v95 != 12 )
      break;
    v96 = W_pslot_by_id((intptr_t)(F32(v88, 0xc)), (intptr_t)(F32(16 * F32((a3[93] + 20), 0) + a3[95], 4)), (intptr_t)(0));
    v97 = mm_5956c0(v88, v96, 0, 440, 8);
    W_ui_link_drop((intptr_t)(v88));
    if ( v97 != 0 )
      goto LABEL_143;
  }
  if ( v95 != 13 )
  {
    v97 = v95;
LABEL_143:
    W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(5u));
    v99 = (*v112__f);
    *(_DWORD *)((uint8_t *)((*v109__f) + (1) * 0x50) + 0x0) = 0;
    F32(v99, 80) = 0;
    return v97;
  }
  W_sys_unload((intptr_t)((char *)((intptr_t)g_eng.sys)), (intptr_t)(5u));
  v98 = (*v112__f);
  *(_DWORD *)((uint8_t *)((*v109__f) + (1) * 0x50) + 0x0) = 0;
  F32(v98, 80) = 0;
  result = mm_592850(v88, a2, a3, a4, F32(16 * F32((a3[93] + 20), 0) + a3[95], 4));
  if ( result == 0 )
    return (*v111__f);
  return result;
}

/* 0x593d90: void __thiscall sub_593D90(cls_593D90 *this, int a2) */
void mm_593d90(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    int32_t * i = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;

  v2 = 0;
  v3 = F16(c, 0x6);
  if ( (uint16_t)v3 != 0 )
  {
    for ( i = (_DWORD *)F32(c, 0x0); *i != a2; ++i )
    {
      if ( ++v2 >= v3 )
        return;
    }
    if ( v2 >= 0 )
    {
      v5 = (uint16_t)v2;
      v6 = F32((F32(c, 0x0) + 4 * (uint16_t)v2), 0);
      if ( (uint16_t)v2 < v3 - 1 )
      {
        do
        {
          ++v2;
          F32((F32(c, 0x0) + 4 * v5), 0) = F32(F32(c, 0x0) + 4 * v5, 4);
          v5 = (uint16_t)v2;
        }
        while ( (uint16_t)v2 < F16(c, 0x6) - 1 );
      }
      v7 = 0;
      F32((F32(c, 0x0) + 4 * F16(c, 0x6) - 4), 0) = v6;
      v8 = F16(c, 0x6);
      if ( v8 != 0 )
      {
        do
        {
          v9 = F32((F32(c, 0x0) + 4 * v7), 0);
          F32(v9, 8) = v7 == v8 - 1;
          v8 = F16(c, 0x6);
          ++v7;
        }
        while ( v7 < v8 );
      }
    }
  }
}

/* 0x593e40: void __stdcall sub_593E40(cls_40FA00 *a1, unsigned __int16 a2, char *a3, int a4, int a5, __int16 a6) */
void mm_593e40(int32_t a1, uint16_t a2, char * a3, int32_t a4, int32_t a5, int16_t a6)
{

    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    _Alignas(8) uint8_t frame_[0x204 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x4);   /* [512] at ebp-0x200 */
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x204 */

  (*v9__f) = SYS32(2952);
  v6 = F32(a1, 0x174);
  if ( F32(v6, 8) > 0 && F32(v6, 16) > (int)a2 )
  {
    v7 = 16 * a2;
    if ( F32(F32((v7 + F32(a1, 0x17c) + 12), 0), 4) == 0 )
      F32(F32((v7 + F32(a1, 0x17c) + 12), 0), 4) = (intptr_t)g_eng.op_new(0x54u);
    v8 = F32(F32((v7 + F32(a1, 0x17c) + 12), 0), 4);
    F32(v8, 0) = (*v9__f);
    F16(v8, 4) = a6;
    F16(v8, 70) = 0;
    F16(v8, 72) = 0;
    F16(v8, 76) = 0;
    F16(v8, 74) = 0;
    F32(v8, 80) = 0;
  }
  W_grid_cell_text((intptr_t)(a1), (intptr_t)(a2), (intptr_t)(1), (intptr_t)(a3));
  g_eng.h5bf3ee(Buffer, "%d", a4 + a5);
  W_grid_cell_text((intptr_t)(a1), (intptr_t)(a2), (intptr_t)(2), (intptr_t)(Buffer));
  W_grid_cell_text((intptr_t)(a1), (intptr_t)(a2), (intptr_t)(3), (intptr_t)(D(0x8a3024)));
  g_eng.h5bf3ee(Buffer, "%d", a4);
  W_grid_cell_text((intptr_t)(a1), (intptr_t)(a2), (intptr_t)(4), (intptr_t)(Buffer));
}

/* 0x593f60: int __thiscall sub_593F60(cls_412590 *this, int a2) */
int32_t mm_593f60(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    int32_t * v6 = 0;
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
    intptr_t i = 0;
    intptr_t v19 = 0;
    uint16_t v20 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x614 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v28 = (char *)(frame_ + 0x14);   /* [512] at ebp-0x600 */
    char *v29 = (char *)(frame_ + 0x214);   /* [512] at ebp-0x400 */
    char *Buffer = (char *)(frame_ + 0x414);   /* [512] at ebp-0x200 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x614 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x610 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x60c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x60c */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x604 */

  (*v23__f) = 0;
  v3 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(8), (intptr_t)(346), (intptr_t)(624), (intptr_t)(128), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x20) = (int)v3;
  v4 = W_win_text_add((intptr_t)(v3), (intptr_t)(64), (intptr_t)(24), (intptr_t)(68), (intptr_t)(3));
  v5 = F32(c, 0x20);
  F32(c, 0x24) = (int)v4;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v5), (intptr_t)(v4));
  W_text_metrics_set((intptr_t)(F32(c, 0x24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  if ( a2 == 2 )
  {
    strcpy(v28, ((char *)D(0x8a2eec)));
    (*v24__f) = F32(c, 0xc) + 16664;
    v6 = (int *)(*v24__f);
  }
  else
  {
    strcpy(v28, ((char *)D(0x8a2efc)));
    v6 = (int *)(F32(c, 0xc) + 16656);
    (*v24__f) = (int)v6;
  }
  (*v27__f) = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(8), (intptr_t)(24), (intptr_t)(408), (intptr_t)(320), (intptr_t)(2u), (intptr_t)(v28), (intptr_t)(1));
  v7 = W_win_grid_new((intptr_t)((*v27__f)), (intptr_t)(40), (intptr_t)(48), (intptr_t)(512), (intptr_t)(6), (intptr_t)(8), (intptr_t)(2));
  F32(v7, 428) = 32;
  W_grid_col_place((intptr_t)(v7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-8), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v7), (intptr_t)(1), (intptr_t)(0), (intptr_t)(-8), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v7), (intptr_t)(2), (intptr_t)(40), (intptr_t)(0), (intptr_t)(160), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v7), (intptr_t)(3), (intptr_t)(200), (intptr_t)(0), (intptr_t)(64), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v7), (intptr_t)(4), (intptr_t)(216), (intptr_t)(0), (intptr_t)(64), (intptr_t)(0));
  F32(c, 0x2c) = 0;
  W_grid_col_place((intptr_t)(v7), (intptr_t)(5), (intptr_t)(280), (intptr_t)(0), (intptr_t)(56), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v7), (intptr_t)(1));
  W_sys_page_arrows((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((*v27__f)), (intptr_t)(v7));
  W_item_list_fill((intptr_t)(c), (intptr_t)(F32(c, 0xc)), (intptr_t)(-1), (intptr_t)(v6[1]), (intptr_t)((int *)v7), (intptr_t)(a2), (intptr_t)(*v6));
  while ( 2 )
  {
    v8 = F32(v7, 372);
    if ( F32(v8, 16) != 0 )
    {
      v9 = F32(16 * F32((v8 + 20), 0) + F32((v7 + 380), 0), 4);
      v10 = F32(c, 0xc);
      if ( (uint16_t)v9 < F16(v10, 16400) )
      {
        v11 = F32(16 * (uint16_t)v9 + v10, 8220);
        if ( v11 != 0 )
        {
          v12 = F32(c, 0x24);
          if ( v12 != 0 )
            W_ctrl_text_set((intptr_t)(v12), (intptr_t)((const char *)(v11 + 172)));
        }
      }
      v13 = W_menu_run((intptr_t)(c), (intptr_t)((_DWORD *)v7), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      (*v25__f) = F32(F32((v7 + 372), 0), 20);
      v14 = F32(16 * (*v25__f) + F32((v7 + 380), 0), 4);
      v15 = F32(c, 0xc);
      if ( (uint16_t)v14 >= F16(v15, 16400) )
        v16 = 0;
      else
        v16 = F32(16 * (uint16_t)v14 + v15, 8220);
      switch ( v13 )
      {
        case 11:
          goto LABEL_31;
        case 12:
          if ( v16 == 0 )
            continue;
          v13 = mm_5943e0(c, v16, v14);
          if ( v13 != 0 )
            goto LABEL_34;
          W_item_list_fill((intptr_t)(c), (intptr_t)(F32(c, 0xc)), (intptr_t)(-1), (intptr_t)((uint16_t)(*v25__f)), (intptr_t)((int *)v7), (intptr_t)(a2), (intptr_t)(*v6));
          continue;
        case 13:
          if ( v16 != 0 )
          {
            v17 = F32(c, 0x24);
            if ( v17 != 0 )
              W_ctrl_text_set((intptr_t)(v17), (intptr_t)((const char *)(v16 + 172)));
          }
          continue;
        case 20:
          *v6 = *v6 == 0;
          W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(9u), (intptr_t)(0), (intptr_t)(0));
          W_item_list_fill((intptr_t)(c), (intptr_t)(F32(c, 0xc)), (intptr_t)((uint16_t)v14), (intptr_t)(-1), (intptr_t)((int *)v7), (intptr_t)(a2), (intptr_t)(*v6));
          continue;
        default:
LABEL_34:
          (*v23__f) = v13;
          goto LABEL_31;
      }
    }
    break;
  }
  g_eng.h5bf3ee(Buffer, "You do not have any %s.", v28);
  strcpy(v29, Buffer);
  (*v26__f) = 0;
  for ( i = W_msg_window_open((intptr_t)(c), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
        ;
        W_ctrl_text_set((intptr_t)(F32((F32(i, 72) + 4 * F16(i, 78) - 4), 0)), (intptr_t)(&v29[512 * v20])) )
  {
    while ( 1 )
    {
      v19 = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(i), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( v19 == 12 )
        break;
      if ( v19 != 13 )
        goto LABEL_28;
    }
    v20 = ++(*v26__f);
    if ( (uint16_t)(*v26__f) != 0 )
      break;
  }
  v19 = 0;
LABEL_28:
  F32(i, 80) = 0;
  if ( v19 != 0 )
    (*v23__f) = v19;
  v6 = (int *)(*v24__f);
LABEL_31:
  v6[1] = F32(F32((v7 + 372), 0), 20);
  F32((*v27__f), 80) = 0;
  v21 = F32(c, 0x20);
  if ( v21 != 0 )
  {
    F32(v21, 80) = 0;
    F32(c, 0x20) = 0;
    F32(c, 0x24) = 0;
  }
  return (*v23__f);
}

/* 0x5943e0: int __thiscall sub_5943E0(cls_412590 *this, int a2, int a3) */
int32_t mm_5943e0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
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
    uint16_t v23 = 0;
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
    intptr_t * v37 = 0;
    intptr_t * v38 = 0;
    char * v39 = 0;
    _Alignas(8) uint8_t frame_[0x21c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x1c);   /* [512] at ebp-0x200 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x21c */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x218 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x214 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x210 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x20c */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x208 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x204 */

  v4 = false;
  v5 = F32(a2, 120);
  (*v41__f) = 0;
  (*v40__f) = 1;
  v6 = v5 != 0;
  v7 = F32(c, 0xc);
  (*v43__f) = 16 * (uint16_t)a3;
  v8 = F32((*v43__f) + v7, 8212) != 0;
  (*v40__f) = (F32(a2, 684) < 500);
  v9 = F32(a2, 88);
  if ( v9 != 0 )
    v4 = v9 != 22;
  v10 = v6;
  if ( v8 )
    ++v10;
  if ( v4 )
    ++v10;
  if ( (*v40__f) != 0 )
    ++v10;
  if ( v10 == 0 )
    return 0;
  (*v44__f) = a2 + 4;
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(64), (intptr_t)(32), (intptr_t)(192), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(v10), (intptr_t)(1u), (intptr_t)(&(*v46__f)), (intptr_t)((int *)&(*v42__f)), (intptr_t)((char *)(a2 + 4)), (intptr_t)(0));
  if ( v6 )
  {
    v12 = (*v42__f);
    v13 = W_grid_row_add((intptr_t)((*v42__f)), (intptr_t)(0), (intptr_t)(8));
    W_grid_cell_text((intptr_t)(v12), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x854c90))));
  }
  if ( v8 )
  {
    v14 = (*v42__f);
    v15 = W_grid_row_add((intptr_t)((*v42__f)), (intptr_t)(0), (intptr_t)(10));
    W_grid_cell_text((intptr_t)(v14), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3108))));
  }
  if ( v4 )
  {
    v16 = (*v42__f);
    v17 = W_grid_row_add((intptr_t)((*v42__f)), (intptr_t)(0), (intptr_t)(7));
    W_grid_cell_text((intptr_t)(v16), (intptr_t)(v17), (intptr_t)(0), (intptr_t)(D(0x8a3104)));
  }
  if ( (*v40__f) != 0 )
  {
    v18 = (*v42__f);
    v19 = W_grid_row_add((intptr_t)((*v42__f)), (intptr_t)(0), (intptr_t)(9));
    W_grid_cell_text((intptr_t)(v18), (intptr_t)(v19), (intptr_t)(0), (intptr_t)(((char *)D(0x8a30fc))));
  }
  while ( 1 )
  {
    v20 = W_menu_run((intptr_t)(c), (intptr_t)((*v42__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( v20 == 11 )
      break;
    if ( v20 == 12 )
    {
      switch ( F32(16 * F32((F32((*v42__f), 0x174) + 20), 0) + F32((*v42__f), 0x17c), 4) )
      {
        case 7:
          v20 = mm_594c80(c, (_DWORD *)a2, a3);
          v35 = v20 - 11;
          if ( v20 == 11 )
            continue;
          goto LABEL_71;
        case 8:
          v27 = 0;
          v28 = mm_595350(c, 168, 64);
          (*v45__f) = mm_594a20(c, F32(c, 0xc), 8, 64, 1, 1, 160, 0, ((char *)D(0x854c30)), (int *)&(*v40__f), 1, 1, 0);
          v29 = W_grid_row_add((intptr_t)((*v40__f)), (intptr_t)(2), (intptr_t)(0));
          W_grid_cell_text((intptr_t)((*v40__f)), (intptr_t)(v29), (intptr_t)(0), (intptr_t)(((char *)D(0x854c24))));
          W_equip_panel_fill((intptr_t)(c), (intptr_t)(v28), (intptr_t)((_DWORD *)F32(c, 0xc)), (intptr_t)(F32(16 * F32((F32((*v40__f), 0x174) + 20), 0) + F32((*v40__f), 0x17c), 4)), (intptr_t)(F32(a2, 0)), (intptr_t)(0));
          v30 = F32(c, 0x24);
          if ( v30 != 0 )
            W_ctrl_text_set((intptr_t)(v30), (intptr_t)(((char *)D(0x854c00))));
          while ( 1 )
          {
            v31 = W_menu_run((intptr_t)(c), (intptr_t)((*v40__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            v32 = F32(16 * F32((F32((*v40__f), 0x174) + 20), 0) + F32((*v40__f), 0x17c), 4);
            if ( v31 == 11 )
              break;
            if ( v31 != 12 )
            {
              if ( v31 == 13 )
              {
                W_equip_panel_fill((intptr_t)(c), (intptr_t)(v28), (intptr_t)((_DWORD *)F32(c, 0xc)), (intptr_t)(v32), (intptr_t)(F32(a2, 0)), (intptr_t)(0));
                continue;
              }
LABEL_48:
              v27 = v31;
              break;
            }
            if ( v32 == 0 )
              goto LABEL_45;
            v33 = W_pslot_by_id((intptr_t)(F32(c, 0xc)), (intptr_t)(v32), (intptr_t)(0));
            v31 = W_mh_418ec0((intptr_t)(c), (intptr_t)(v33));
            if ( v31 != 7 )
            {
              if ( v31 == 0 )
              {
                if ( W_menu_item_fits((intptr_t)(c), (intptr_t)(v32), (intptr_t)(a2)) == 0 )
                {
                  W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(6u), (intptr_t)(0), (intptr_t)(0));
                  continue;
                }
LABEL_45:
                W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(0));
                W_room_item_equip((intptr_t)(F32(c, 0xc)), (intptr_t)(a3), (intptr_t)(v32), (intptr_t)(1));
                v27 = 12;
                break;
              }
              goto LABEL_48;
            }
          }
          v34 = (*v45__f);
          F32(v28, 80) = 0;
          F32(v34, 80) = 0;
          W_menu_sprites_free((intptr_t)(c));
          if ( v27 == 11 )
            continue;
          if ( v27 != 12 )
            (*v41__f) = v27;
          goto LABEL_75;
        case 9:
          v39 = (const char *)(*v44__f);
          if ( F32(a2, 684) < 200 )
          {
            g_eng.h5bf3ee(Buffer, "Are you sure you wish to discard the %s?", (const char *)(*v44__f));
            v20 = W_mh_4190d0((intptr_t)(c), (intptr_t)(Buffer), (intptr_t)(0x28u), (intptr_t)((_DWORD *)2), (intptr_t)(0));
            goto LABEL_70;
          }
          g_eng.h5bf3ee(
            Buffer,
            "%s is a rare item. Are you sure you wish to discard it?",
            (const char *)(*v44__f));
          (*v44__f) = 0;
          (*v43__f) = 0;
          v20 = W_mh_419eb0((intptr_t)(c), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(2u), (intptr_t)(0), (intptr_t)(&(*v43__f)), (intptr_t)(0));
          if ( v20 == 0 )
          {
            W_mh_419c70((intptr_t)(c), (intptr_t)(300), (intptr_t)(F32((*v43__f), 24) + F32((*v43__f), 16) - 16), (intptr_t)(128), (intptr_t)(0), (intptr_t)(1u), (intptr_t)(2u), (intptr_t)(&(*v44__f)), (intptr_t)((int *)&(*v40__f)), (intptr_t)(0), (intptr_t)(0));
            W_grid_row_text_add((intptr_t)((*v40__f)), (intptr_t)(0), (intptr_t)(14), (intptr_t)(D(0x854d04)));
            W_grid_row_text_add((intptr_t)((*v40__f)), (intptr_t)(0), (intptr_t)(15), (intptr_t)(((char *)D(0x854d00))));
            W_tm_419900((intptr_t)((*v40__f)), (intptr_t)(0), (intptr_t)(15));
            while ( 2 )
            {
              v20 = W_menu_run((intptr_t)(c), (intptr_t)((*v40__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
              switch ( v20 )
              {
                case 11:
                  v20 = 15;
                  break;
                case 12:
                  v20 = F32(16 * F32((F32((*v40__f), 0x174) + 20), 0) + F32((*v40__f), 0x17c), 4);
                  break;
                case 13:
                  continue;
                default:
                  goto LABEL_65;
              }
              break;
            }
          }
LABEL_65:
          if ( (*v43__f) != 0 )
            F32((*v43__f), 80) = 0;
          if ( (*v44__f) != 0 )
            F32((*v44__f), 80) = 0;
LABEL_70:
          v35 = v20 - 14;
          if ( v20 == 14 )
          {
            W_mh_4199b0((intptr_t)(F32(c, 0xc)), (intptr_t)(a3), (intptr_t)(1), (intptr_t)(1));
            g_eng.h5bf3ee(Buffer, "You discard the %s.", v39);
            v20 = W_msg_pages((intptr_t)(c), (intptr_t)(Buffer), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
            if ( v20 == 0 )
              goto LABEL_75;
          }
          else
          {
LABEL_71:
            if ( v35 == 1 )
              goto LABEL_75;
          }
          goto LABEL_74;
        case 0xA:
          v21 = F32(c, 0xc);
          v22 = (*v43__f);
          v23 = 0;
          break;
        default:
          continue;
      }
      while ( 1 )
      {
        v24 = F32(v21, 4 * v23 + 16432);
        if ( F32(v24, 0x9f0) == F32(v21 + (*v43__f), 8212) )
          break;
        if ( ++v23 >= 8u )
        {
          v25 = 0;
          goto LABEL_28;
        }
      }
      v25 = W_pslot_ok((intptr_t)(v24), (intptr_t)(1), (intptr_t)(1)) ? v24 : 0;
LABEL_28:
      v26 = *(_DWORD *)((uint8_t *)(v25 + (1) * 0x9f4) + 0x0);
      if ( v26 == 0 || F32(v26, 496) == 0 )
      {
        v36 = F32(c, 0xc);
        *(_DWORD *)&GAP(v36, 0x2010, v22 + 4) = 0;
        W_room_stats_refresh((intptr_t)(v36), (intptr_t)(1));
        v37 = (intptr_t *)W_vpool_take((intptr_t)(SYS32(888)), (intptr_t)(0));
        v38 = (intptr_t *)v37;
        if ( v37 != 0 )
        {
          g_eng.dsb_volume_5bb870(F32(v37, 0), F32(CFG(), 360));
          g_eng.dsb_freq_5bb890(F32(v38, 0), 0);
          g_eng.dsb_play_5bb8c0(*v38);
        }
        break;
      }
      v20 = W_msg_pages((intptr_t)(c), (intptr_t)(((char *)D(0x854cb8))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0));
      if ( v20 != 0 && v20 != 7 )
      {
LABEL_74:
        (*v41__f) = v20;
        break;
      }
    }
    else if ( v20 != 13 )
    {
      goto LABEL_74;
    }
  }
LABEL_75:
  *(_DWORD *)((uint8_t *)((*v46__f) + (1) * 0x50) + 0x0) = 0;
  return (*v41__f);
}

/* 0x594a20: int __thiscall sub_594A20( cls_412590 *this, int a2, int a3, int a4, __int16 a5, int a6, int a7, int a8, const char *a9, int *a10, int a11, int a12, int a13) */
int32_t mm_594a20(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8, char * a9, int32_t * a10, int32_t a11, int32_t a12, int32_t a13)
{
    intptr_t c = (intptr_t)self;
    intptr_t v13 = 0;
    int16_t v14 = 0;
    intptr_t i = 0;
    uint16_t v16 = 0;
    intptr_t v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t * v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    uint16_t v36 = 0;
    intptr_t j = 0;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v31 = (int16_t *)(frame_ + 0x4);   /* [2] at ebp-0x30 */
    uint8_t *v35 = (uint8_t *)(frame_ + 0x10);   /* [36] at ebp-0x24 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    int16_t *const v32__f = (int16_t *)(frame_ + 0x8);   /* ebp-0x2c */
    int16_t *const v33__f = (int16_t *)(frame_ + 0xa);   /* ebp-0x2a */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x28 */

  F32(v31, 0) = c;
  (*v30__f) = 24;
  if ( a9 != 0 )
    (*v30__f) = 56;
  v13 = a2 + 16432;
  v14 = 0;
  (*v34__f) = a2 + 16432;
  for ( i = 8; i != 0; --i )
  {
    if ( F32(F32(v13, 0), 2500) == 1 && (F32(F32(v13, 0), 2544) != 100000104 || a12 != 0) )
      ++v14;
    v13 += 4;
  }
  v16 = a5 + v14;
  v17 = a8;
  v36 = v16;
  if ( a8 == 0 )
    v17 = 28 * v16 + 64;
  v18 = 2;
  v19 = **(_DWORD **)v31;
  v20 = 0;
  if ( a9 != 0 )
  {
    v20 = 1;
    v18 = 3;
  }
  if ( a13 != 0 )
    W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v35), (intptr_t)(v20));
  else
    W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v35), (intptr_t)(v20));
  if ( F16(v19, 6) < F16(v19, 4) )
  {
    v22 = 4 * F16(v19, 6);
    W_win_init((intptr_t)(F32((v22 + F32(v19, 0)), 0)), (intptr_t)(v19), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(a7), (intptr_t)(v17), (intptr_t)(v18), (intptr_t)(v35));
    ++F16(v19, 6);
    W_win_mgr_topmost((intptr_t)(v19));
    v21 = F32((v22 + F32(v19, 0)), 0);
    v16 = v36;
  }
  else
  {
    v21 = 0;
  }
  if ( a9 != 0 )
    W_win_title_add((intptr_t)(v21), (intptr_t)((char *)a9));
  v23 = F16(v21, 78);
  if ( F16(v21, 76) > v23
    && (v24 = F32((F32(v21, 72) + 4 * v23), 0),
        W_ui_ref_set((intptr_t)(v24), (intptr_t)(v21), (intptr_t)(v23)),
        ++F16(v21, 78),
        v24 != 0) )
  {
    W_ctrl_grid_init((intptr_t)(v24), (intptr_t)(40), (intptr_t)((*v30__f)), (intptr_t)(v16), (intptr_t)(a6), (intptr_t)(v16), (intptr_t)(0));
  }
  else
  {
    v24 = 0;
  }
  F32(F32(v24, 376), 0) = 0;
  F32(F32((v24 + 376), 0), 4) = 0;
  v25 = F32(v24, 376);
  v31[0] = 0;
  F32(v25, 8) = 0;
  F32(F32((v24 + 376), 0), 12) = 0;
  v31[1] = 1;
  (*v32__f) = 2;
  (*v33__f) = 3;
  W_ctrl_cursor_set((intptr_t)(v24), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v31), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
  if ( F32(CFG(), 596) == 0 )
  {
    F32(v24, 20) = 1;
    F32(v24, 24) = -9;
  }
  if ( a11 != 0 )
  {
    v26 = (intptr_t *)(*v34__f);
    for ( j = 8; j != 0; --j )
    {
      v27 = F32(v26, 0);
      if ( W_pslot_ok((intptr_t)(F32(v26, 0)), (intptr_t)(a12), (intptr_t)(0)) )
      {
        v28 = W_grid_row_add((intptr_t)(v24), (intptr_t)(1), (intptr_t)(F32(v27, 0x9f0)));
        W_grid_cell_text((intptr_t)(v24), (intptr_t)(v28), (intptr_t)(0), (intptr_t)(&GAP((v27 + (1) * 0x9f4), 0x0, 28)));
      }
      ++v26;
    }
  }
  if ( a10 != 0 )
    *a10 = v24;
  return v21;
}

/* 0x594c80: int __thiscall sub_594C80(cls_412590 *this, _DWORD *a2, int a3) */
int32_t mm_594c80(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
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
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  (*v20__f) = 0;
  if ( a2[22] == 21 )
  {
    v13 = F32(c, 0x0);
    if ( F16(F32(c, 0x0), 6) < F16(F32(c, 0x0), 4) )
    {
      v15 = 4 * F16(F32(c, 0x0), 6);
      W_win_init((intptr_t)(F32(F32(v13, 0x0), v15)), (intptr_t)((int)v13), (intptr_t)(0), (intptr_t)(0), (intptr_t)(640), (intptr_t)(480), (intptr_t)(1u), (intptr_t)(0));
      ++F16(v13, 0x6);
      W_win_mgr_topmost((intptr_t)(v13));
      v14 = F32((F32(v13, 0x0) + v15), 0);
    }
    else
    {
      v14 = 0;
    }
    v16 = F32(v14, 12);
    v17 = F32(v14, 16);
    F32(v14, 28) = 2;
    F32(v14, 80) = 1;
    F32(v14, 84) = 0;
    F32(v14, 88) = 1;
    F32(v14, 56) = F32(v14, 12);
    v16 += 640;
    F32(v14, 60) = F32(v14, 16);
    F32(v14, 32) = v16;
    F32(v14, 40) = v16;
    F32(v14, 12) = v16;
    F32(v14, 36) = v17;
    F32(v14, 44) = v17;
    F32(v14, 48) = 1000;
    F32(v14, 52) = 1000;
    F32(v14, 16) = v17;
    F32(v14, 64) = 0;
    F32(v14, 68) = 0;
    v18 = W_win_ctrl_take((intptr_t)(v14));
    v19 = v18;
    if ( v18 != 0 )
    {
      W_ctrl_grid_init((intptr_t)(v18), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
      W_grid_col_place((intptr_t)(v19), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      W_grid_row_add((intptr_t)(v19), (intptr_t)(0), (intptr_t)(0));
    }
    else
    {
      v19 = 0;
    }
    W_cell_icon_set((intptr_t)(v19), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(4 * a2[26] + 2732)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0));
    F32(v14, 80) = 0;
    if ( result == 0 )
      return 0;
  }
  else if ( a2[23] == 1 || a2[23] == 3 )
  {
    v5 = F32(c, 0xc);
    v6 = 24;
    if ( ((char *)D(0x854c30)) != 0 )
      v6 = 56;
    v7 = (uint16_t)W_room_party_count((intptr_t)(v5), (intptr_t)(0), (intptr_t)(0));
    (*v21__f) = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(112), (intptr_t)(64), (intptr_t)(160), (intptr_t)(28 * v7 + 64), (intptr_t)(2u), (intptr_t)(((char *)D(0x854c30))), (intptr_t)(0));
    v8 = W_win_grid_new((intptr_t)((*v21__f)), (intptr_t)(40), (intptr_t)(v6), (intptr_t)(v7), (intptr_t)(1), (intptr_t)(v7), (intptr_t)(0));
    W_grid_col_place((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v8), (intptr_t)(1));
    W_member_rows_add((intptr_t)((int)v5), (intptr_t)(v8), (intptr_t)(0));
    if ( &a2 == 0 )
      v8 = a2;
    v9 = F32(c, 0x24);
    if ( v9 != 0 )
      W_ctrl_text_set((intptr_t)(v9), (intptr_t)(((char *)D(0x8a3110))));
    v10 = a3;
    while ( 1 )
    {
      result = W_menu_run((intptr_t)(c), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      if ( result == 11 )
        break;
      if ( result == 12 )
      {
        v11 = W_pslot_by_id((intptr_t)(F32(c, 0xc)), (intptr_t)(F32(16 * F32((*(_DWORD *)&GAP(v8, 0x0, 372) + 20), 0) + *(_DWORD *)((uint8_t *)(v8 + (1) * 0x17c) + 0x0), 4)), (intptr_t)(0));
        if ( v11 == 0 )
          break;
        v12 = mm_594f40(c, v11, v10, (int)a2);
        if ( v12 != 0 )
        {
          (*v20__f) = v12;
          break;
        }
      }
      else if ( result != 13 )
      {
        goto LABEL_22;
      }
    }
    result = (*v20__f);
LABEL_22:
    if ( (*v21__f) != 0 )
      F32((*v21__f), 80) = 0;
  }
  else
  {
    result = mm_594f40(c, 0, a3, (int)a2);
    if ( result == 0 || result == 12 )
      return 0;
  }
  return result;
}

/* 0x594f40: int __thiscall sub_594F40(cls_412590 *this, int a2, int a3, int a4) */
int32_t mm_594f40(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t i = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x1218 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v16 = (char *)(frame_ + 0x18);   /* [512] at ebp-0x1200 */
    char *Buffer = (char *)(frame_ + 0x218);   /* [4096] at ebp-0x1000 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1218 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1214 */
    int32_t * *const v12__f = (int32_t * *)(frame_ + 0x8);   /* ebp-0x1210 */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x120c */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x1208 */
    uint16_t *const v15__f = (uint16_t *)(frame_ + 0x14);   /* ebp-0x1204 */

  v6 = F32(c, 0xc);
  v7 = 0;
  (*v10__f) = 0;
  if ( (uint16_t)a3 >= F16(v6, 16400) )
  {
    (*v13__f) = 0;
  }
  else
  {
    v4 = F32(16 * (uint16_t)a3 + v6, 8220);
    (*v13__f) = v4;
  }
  if ( !W_mh_5b1f10((intptr_t)(v6), (intptr_t)(/*edx*/ a2), (intptr_t)(a3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1)) )
  {
    if ( F32((*v13__f), 88) != 20 )
    {
      W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(6u), (intptr_t)(0), (intptr_t)(0));
      return 0;
    }
    strcpy(v16, ((char *)D(0x8a317c)));
    (*v11__f) = 0;
    for ( i = W_msg_window_open((intptr_t)(c), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          ;
          W_ctrl_text_set((intptr_t)(F32((F32(i, 72) + 4 * F16(i, 78) - 4), 0)), (intptr_t)(v16)) )
    {
      while ( 1 )
      {
        result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(i), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( result == 12 )
          break;
        if ( result != 13 )
          goto LABEL_13;
      }
      if ( (uint16_t)++v7 != 0 )
        break;
    }
    result = (*v11__f);
    goto LABEL_13;
  }
  if ( F32((*v13__f), 88) == 20 )
  {
    (*v11__f) = 0;
    (*v14__f) = 0;
    result = W_mh_419eb0((intptr_t)(c), (intptr_t)(((char *)D(0x8a3154))), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(2u), (intptr_t)(0), (intptr_t)(&(*v14__f)), (intptr_t)(0));
    if ( result == 0 )
    {
      W_mh_419c70((intptr_t)(c), (intptr_t)(300), (intptr_t)(F32((*v14__f), 24) + F32((*v14__f), 16) - 16), (intptr_t)(128), (intptr_t)(0), (intptr_t)(1u), (intptr_t)(2u), (intptr_t)(&(*v11__f)), (intptr_t)((int *)&(*v12__f)), (intptr_t)(0), (intptr_t)(0));
      W_grid_row_text_add((intptr_t)((*v12__f)), (intptr_t)(0), (intptr_t)(14), (intptr_t)(D(0x854d04)));
      W_grid_row_text_add((intptr_t)((*v12__f)), (intptr_t)(0), (intptr_t)(15), (intptr_t)(((char *)D(0x854d00))));
      while ( 1 )
      {
        result = W_menu_run((intptr_t)(c), (intptr_t)((*v12__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( result == 11 )
          break;
        if ( result == 12 )
        {
          result = F32(16 * F32(((*v12__f)[93] + 20), 0) + (*v12__f)[95], 4);
          goto LABEL_25;
        }
        if ( result != 13 )
          goto LABEL_25;
      }
      result = 15;
    }
LABEL_25:
    if ( (*v14__f) != 0 )
      F32((*v14__f), 80) = 0;
    if ( (*v11__f) != 0 )
      F32((*v11__f), 80) = 0;
    if ( result != 14 )
    {
      if ( result == 15 )
        return (*v10__f);
      return result;
    }
  }
  if ( F32(W, 0x2770) != 0 || F32(W, 0x2764) == 1 )
  {
    if ( F32(a4, 688) != 1 )
    {
      mm_595320(F32(c, 0xc), a3, a2);
      return 8;
    }
    strcpy(v16, ((char *)D(0x8a3130)));
    (*v11__f) = 0;
    for ( i = W_msg_window_open((intptr_t)(c), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1));
          ;
          W_ctrl_text_set((intptr_t)(F32((F32(i, 72) + 4 * F16(i, 78) - 4), 0)), (intptr_t)(v16)) )
    {
      while ( 1 )
      {
        result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(i), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( result == 12 )
          break;
        if ( result != 13 )
          goto LABEL_13;
      }
      if ( (uint16_t)++v7 != 0 )
        break;
    }
    result = (*v11__f);
LABEL_13:
    F32(i, 80) = 0;
    if ( result != 0 )
      return result;
    return (*v10__f);
  }
  if ( W_mh_5b1f10((intptr_t)(F32(c, 0xc)), (intptr_t)(/*edx*/ a2), (intptr_t)(a3), (intptr_t)(Buffer), (intptr_t)(&(*v15__f)), (intptr_t)(0), (intptr_t)(0)) )
  {
    result = W_msg_pages((intptr_t)(c), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)((*v15__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0x28u), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(1));
    if ( result != 0 )
      return result;
    (*v10__f) = 12;
    if ( F32((*v13__f), 88) == 20 )
      (*v10__f) = 8;
  }
  else
  {
    W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(6u), (intptr_t)(0), (intptr_t)(0));
  }
  W_menu_party_fill((intptr_t)(c));
  return (*v10__f);
}

/* 0x595320: __int16 __thiscall sub_595320(cls_595320 *this, __int16 a2, int a3) */
int32_t mm_595320(wl_char *self, int16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;

  F16(c, 0x4078) = a2;
  F32(c, 0x407c) = a3;
  F16(c, 0x4070) = 2;
  F32(c, 0x4074) = 250;
  return a2;
}

/* 0x595350: int __thiscall sub_595350(cls_595350 *this, int a2, int a3) */
int32_t mm_595350(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
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
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v15 = (uint8_t *)(frame_ + 0x0);   /* [36] at ebp-0x24 */

  v3 = 0;
  v4 = F32(c, 0x0);
  v5 = 8;
  if ( D(0x8a9b6c) != 0 )
  {
    v3 = 1;
    v5 = 9;
  }
  W_sys_frame_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v15), (intptr_t)(v3));
  v6 = W_win_mgr_open((intptr_t)(v4), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(464), (intptr_t)(272), (intptr_t)(v5), (intptr_t)(v15));
  if ( D(0x8a9b6c) != 0 )
    W_win_title_add((intptr_t)((int)v6), (intptr_t)(D(0x8a9b6c)));
  v7 = W_win_ctrl_take((intptr_t)(v6));
  v8 = v7;
  if ( v7 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v7), (intptr_t)(24), (intptr_t)(48), (intptr_t)(2), (intptr_t)(7), (intptr_t)(2), (intptr_t)(1));
    v9 = v8;
  }
  else
  {
    v9 = 0;
  }
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v9) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v9, 0x14, 4) = -9;
  }
  F32(F32(v9, 0x178), 0) = 0;
  F32(F32(v9, 0x178), 4) = -8;
  F32(F32(v9, 0x178), 8) = 0;
  F32(F32(v9, 0x178), 12) = 0;
  F32(F32(v9, 0x178), 36) = 0;
  F32(F32(v9, 0x178), 40) = -8;
  F32(F32(v9, 0x178), 44) = 0;
  F32(F32(v9, 0x178), 48) = 0;
  F32(F32(v9, 0x178), 72) = 40;
  F32(F32(v9, 0x178), 76) = 0;
  F32(F32(v9, 0x178), 80) = 0;
  F32(F32(v9, 0x178), 84) = 0;
  F32(F32(v9, 0x178), 108) = 176;
  F32(F32(v9, 0x178), 112) = 0;
  F32(F32(v9, 0x178), 116) = 0;
  F32(F32(v9, 0x178), 120) = 0;
  F32(F32(v9, 0x178), 144) = 208;
  F32(F32(v9, 0x178), 148) = -8;
  F32(F32(v9, 0x178), 152) = 0;
  F32(F32(v9, 0x178), 156) = 0;
  F32(F32(v9, 0x178), 180) = 208;
  F32(F32(v9, 0x178), 184) = -8;
  F32(F32(v9, 0x178), 188) = 0;
  F32(F32(v9, 0x178), 192) = 0;
  F32(F32(v9, 0x178), 216) = 248;
  F32(F32(v9, 0x178), 220) = 0;
  F32(F32(v9, 0x178), 224) = 0;
  F32(F32(v9, 0x178), 228) = 0;
  v10 = W_win_ctrl_take((intptr_t)(v6));
  v11 = v10;
  if ( v10 != 0 )
    W_ctrl_grid_init((intptr_t)(v10), (intptr_t)(32), (intptr_t)(112), (intptr_t)(10), (intptr_t)(5), (intptr_t)(10), (intptr_t)(1));
  else
    v11 = 0;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v11) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v11, 0x14, 4) = -9;
  }
  W_grid_col_place((intptr_t)(v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-6), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v11), (intptr_t)(1), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v11), (intptr_t)(2), (intptr_t)(76), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
  W_grid_col_place((intptr_t)(v11), (intptr_t)(3), (intptr_t)(116), (intptr_t)(0), (intptr_t)(8), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v11), (intptr_t)(4), (intptr_t)(132), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
  v12 = W_win_grid_new((intptr_t)(v6), (intptr_t)(240), (intptr_t)(112), (intptr_t)(10), (intptr_t)(5), (intptr_t)(10), (intptr_t)(1));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v12), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-6), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(1), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(2), (intptr_t)(76), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(3), (intptr_t)(116), (intptr_t)(0), (intptr_t)(8), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v12), (intptr_t)(4), (intptr_t)(132), (intptr_t)(0), (intptr_t)(32), (intptr_t)(1));
  v13 = W_win_grid_new((intptr_t)(v6), (intptr_t)(240), (intptr_t)(160), (intptr_t)(1), (intptr_t)(4), (intptr_t)(1), (intptr_t)(1));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v13), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v13), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(40), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v13), (intptr_t)(2), (intptr_t)(64), (intptr_t)(40), (intptr_t)(8), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v13), (intptr_t)(3), (intptr_t)(64), (intptr_t)(0), (intptr_t)(40), (intptr_t)(0));
  return (int)((uint8_t *)(v6) + 0x0);
}

/* 0x5956c0: int __thiscall sub_5956C0(cls_412590 *this, int a2, int a3, int a4, int a5) */
int32_t mm_5956c0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int32_t * v17 = 0;
    int32_t * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    int32_t * v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    uint16_t v27 = 0;
    intptr_t v28 = 0;
    uint16_t v29 = 0;
    intptr_t v30 = 0;
    int32_t * v31 = 0;
    char * v32 = 0;
    char * v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    _Alignas(8) uint8_t frame_[0x678 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v46 = (char *)(frame_ + 0x38);   /* [64] at ebp-0x640 */
    char *v47 = (char *)(frame_ + 0x78);   /* [512] at ebp-0x600 */
    char *Buffer = (char *)(frame_ + 0x278);   /* [512] at ebp-0x400 */
    char *v49 = (char *)(frame_ + 0x478);   /* [512] at ebp-0x200 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x678 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x658 */
    uint16_t *const v40__f = (uint16_t *)(frame_ + 0x24);   /* ebp-0x654 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x654 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x650 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x64c */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x648 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x644 */

  result = a2;
  (*v39__f) = 0;
  if ( a2 == 0 )
    return result;
  v6 = 368;
  if ( a3 == 0 )
    v6 = 200;
  v7 = F16(a2, 2900);
  if ( a3 != 0 )
    g_eng.h5bf3ee(Buffer, "%s (%s)", ((char *)D(0x8a2ed8)), (const char *)(a2 + 2576));
  else
    strcpy(Buffer, ((char *)D(0x8a2ed8)));
  (*v43__f) = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(a4), (intptr_t)(a5), (intptr_t)(v6), (intptr_t)(336), (intptr_t)(2u), (intptr_t)(Buffer), (intptr_t)(1));
  v8 = W_win_grid_new((intptr_t)((*v43__f)), (intptr_t)(40), (intptr_t)(56), (intptr_t)(v7), (intptr_t)(3), (intptr_t)(8), (intptr_t)(2));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(160), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(1), (intptr_t)(160), (intptr_t)(0), (intptr_t)(64), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(2), (intptr_t)(256), (intptr_t)(0), (intptr_t)(64), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v8), (intptr_t)(1));
  W_sys_page_arrows((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((*v43__f)), (intptr_t)(v8));
  F32(F32((v8 + 372), 0), 16) = 0;
  memset(v46, 0, (64 * sizeof(char)));
LABEL_8:
  v9 = a2;
  v10 = 0;
  v11 = -1;
  (*v42__f) = 0;
  if ( F16(a2, 2900) != 0 )
  {
    while ( 1 )
    {
      if ( v46[v10] == 0 )
      {
        v12 = F32(v9 + 12 * v10, 2904);
        v13 = v9 + 12 * v10 + 2904;
        v14 = W_skill_row_find((intptr_t)(v12));
        if ( v14 == 0 || F16(v14, 2 * 82) == 0 || *((uint16_t *)v14 + 82) > 2u )
          goto LABEL_23;
        if ( v11 == -1 )
          goto LABEL_22;
        v15 = v14[42];
        if ( (*v45__f) != 0 )
          break;
        if ( v15 == 0 )
          goto LABEL_18;
      }
LABEL_23:
      v9 = a2;
      v10 = ++(*v42__f);
      if ( (uint16_t)(*v42__f) >= F16(a2, 2900) )
      {
        if ( v11 != -1 )
        {
          v46[v11] = 1;
          v17 = (int *)(a2 + 12 * v11 + 2904);
          v18 = W_skill_row_find((intptr_t)(*v17));
          v19 = W_grid_row_add((intptr_t)(v8), (intptr_t)(0), (intptr_t)(*v17));
          W_grid_cell_text((intptr_t)(v8), (intptr_t)(v19), (intptr_t)(0), (intptr_t)((uint8_t *)v18 + 4));
          if ( a3 != 0 )
          {
            switch ( v17[1] )
            {
              case 1:
                if ( v18[42] != 0 )
                  W_grid_cell_text((intptr_t)(v8), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3240))));
                else
                  W_grid_cell_text((intptr_t)(v8), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(D(0x8a3104)));
                break;
              case 2:
                W_grid_cell_text((intptr_t)(v8), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3230))));
                break;
              case 3:
                W_grid_cell_text((intptr_t)(v8), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3220))));
                break;
              case 255:
                W_grid_cell_text((intptr_t)(v8), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a1cb4))));
                break;
              default:
                break;
            }
            if ( F16(v18, 2 * 82) == 1 )
              F32(16 * v19 + F32((v8 + 380), 0), 8) = 0;
          }
          goto LABEL_8;
        }
        goto LABEL_35;
      }
    }
    if ( v15 != 0 )
    {
LABEL_18:
      if ( v15 == 0 )
        goto LABEL_23;
      v16 = F32(v13, 4);
      if ( v16 >= (*v44__f) && (v16 > (*v44__f) || F16(v13, 8) >= (*v40__f)) )
        goto LABEL_23;
    }
LABEL_22:
    v11 = v10;
    (*v45__f) = v14[42];
    (*v44__f) = F32(v13, 4);
    (*v40__f) = F16(v13, 8);
    goto LABEL_23;
  }
LABEL_35:
  if ( a3 == 0 )
  {
    v22 = 10101;
    v23 = 0;
    while ( v22 != F32(16 * F32((F32(v8, 372) + 20), 0) + F32((v8 + 380), 0), 4) )
    {
      v22 = ((int32_t *)D(0x6826c0))[192 * ++v23];
      if ( v22 == 0 )
        goto LABEL_47;
    }
    v24 = &((int32_t *)D(0x6826c0))[192 * v23];
    if ( v24 != 0 )
    {
      v25 = v24[181];
      strcpy(v47, (const char *)(v25 + 196));
      if ( v25 > 0 )
      {
        g_eng.h5bf3ee(v49, "(MP Cost:%d)", v25);
        strcat(v47, v49);
      }
      v26 = F32(c, 0x24);
      if ( v26 != 0 )
        W_ctrl_text_set((intptr_t)(v26), (intptr_t)(v47));
    }
LABEL_47:
    v20 = c;
    goto LABEL_48;
  }
  v20 = c;
  v21 = F32(c, 0x24);
  if ( v21 != 0 )
    W_ctrl_text_set((intptr_t)(v21), (intptr_t)(((char *)D(0x8a31bc))));
  while ( 1 )
  {
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
          while ( 1 )
          {
LABEL_48:
            (*v41__f) = W_menu_run((intptr_t)(v20), (intptr_t)((_DWORD *)v8), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            v27 = F16(a2, 2900);
            v28 = F32(16 * F32((F32(v8, 372) + 20), 0) + F32((v8 + 380), 0), 4);
            v29 = 0;
            if ( v27 != 0 )
            {
              while ( F32(a2 + 12 * v29, 2904) != v28 )
              {
                if ( ++v29 >= v27 )
                  goto LABEL_51;
              }
              v30 = a2 + 12 * v29 + 2904;
            }
            else
            {
LABEL_51:
              v30 = 0;
            }
            v31 = W_skill_row_find((intptr_t)(v28));
            if ( (*v41__f) != 11 )
              break;
            if ( a3 != 0 || F32(v20, 0x20) == 0 )
              goto LABEL_72;
LABEL_60:
            W_ui_link_drop((intptr_t)(v20));
          }
          if ( (*v41__f) == 12 )
            break;
          if ( (*v41__f) != 13 )
          {
            (*v39__f) = (*v41__f);
LABEL_72:
            v35 = a2;
            goto LABEL_73;
          }
          if ( v31 != 0 )
            goto LABEL_56;
        }
        if ( a3 != 0 )
          break;
        if ( F32(v20, 0x20) != 0 )
          goto LABEL_60;
        W_menu_msg_open((intptr_t)(v20));
        v33 = mm_596ac0((int)v31, v47);
        W_menu_msg_set((intptr_t)(v20), (intptr_t)(v33));
        mm_593d90(F32((*v43__f), 0), (*v43__f));
      }
      if ( v31 != 0 && F16(v31, 2 * 82) != 1 )
      {
        v34 = W_mh_418ec0((intptr_t)(v20), (intptr_t)(a2));
        if ( v34 != 7 )
          break;
      }
    }
    if ( v34 != 0 )
    {
      (*v39__f) = v34;
      goto LABEL_72;
    }
    (*v37__f) = v30;
    v35 = a2;
    v36 = mm_5963a0(v20, a2, (*v37__f), (int)v31, a3);
    if ( v36 != 0 )
      break;
    mm_595dd0(a2, (_DWORD *)v8, *v31, a3);
LABEL_56:
    v32 = mm_596ac0((int)v31, v47);
    W_menu_msg_set((intptr_t)(v20), (intptr_t)(v32));
  }
  (*v39__f) = v36;
LABEL_73:
  F32((*v43__f), 80) = 0;
  if ( W_pslot_ok((intptr_t)(v35), (intptr_t)(0), (intptr_t)(0)) )
    W_char_skills_refresh((intptr_t)(F32(v35, 2548)), (intptr_t)(1));
  return (*v39__f);
}

/* 0x595dd0: __int16 __stdcall sub_595DD0(int a1, cls_40FA00 *a2, int a3, int a4) */
int32_t mm_595dd0(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    uint16_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    int32_t * v28 = 0;
    int32_t * v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    _Alignas(8) uint8_t frame_[0x58 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v40 = (uint8_t *)(frame_ + 0x18);   /* [64] at ebp-0x40 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x58 */
    int32_t * *const v34__f = (int32_t * *)(frame_ + 0x0);   /* ebp-0x58 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x54 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x50 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x4c */
    uint16_t *const v38__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0x48 */
    int32_t * *const v39__f = (int32_t * *)(frame_ + 0x14);   /* ebp-0x44 */

  F32(F32(a2, 0x174), 16) = 0;
  memset(v40, 0, (64 * sizeof(uint8_t)));
  while ( 1 )
  {
    v4 = -1;
    SETLO(v5, F16(a1, 2900));
    if ( (uint16_t)v5 == 0 )
      break;
    v6 = 0;
    v7 = a1 + 2904;
    (*v33__f) = F16(a1, 2900);
    do
    {
      if ( v40[v6] == 0 )
      {
        v8 = 0;
        v9 = 10101;
        while ( v9 != F32(v7, 0) )
        {
          v9 = ((int32_t *)D(0x6826c0))[192 * ++v8];
          if ( v9 == 0 )
            goto LABEL_22;
        }
        v10 = &((int32_t *)D(0x6826c0))[192 * v8];
        if ( v10 != 0 && F16(v10, 2 * 82) != 0 && *((uint16_t *)v10 + 82) <= 2u )
        {
          if ( v4 == -1 )
            goto LABEL_21;
          v11 = v10[42];
          if ( (*v36__f) != 0 )
          {
            if ( v11 == 0 )
              goto LABEL_21;
          }
          else if ( v11 != 0 )
          {
            goto LABEL_22;
          }
          if ( v11 != 0 )
          {
            v12 = F32(v7, 4);
            if ( v12 < (*v37__f) || v12 <= (*v37__f) && F16(v7, 8) < (*v38__f) )
            {
LABEL_21:
              (*v36__f) = v10[42];
              v4 = v6;
              (*v37__f) = F32(v7, 4);
              (*v38__f) = F16(v7, 8);
            }
          }
        }
      }
LABEL_22:
      ++v6;
      v7 += 12;
      SETLO(v5, --(*v33__f));
    }
    while ( (*v33__f) != 0 );
    if ( v4 == -1 )
      break;
    v40[v4] = 1;
    v13 = (_DWORD *)(a1 + 12 * v4 + 2904);
    v14 = 0;
    (*v39__f) = v13;
    v15 = 10101;
    while ( v15 != *v13 )
    {
      v15 = ((int32_t *)D(0x6826c0))[192 * ++v14];
      if ( v15 == 0 )
      {
        (*v34__f) = 0;
        v16 = 0;
        goto LABEL_28;
      }
    }
    v16 = &((int32_t *)D(0x6826c0))[192 * v14];
    (*v34__f) = v16;
LABEL_28:
    v17 = *v13;
    v18 = F32(a2, 0x174);
    v19 = F32(v18, 16);
    if ( v19 >= F32(v18, 4) )
    {
      v19 = -1;
      goto LABEL_41;
    }
    F32(v18, 16) = v19 + 1;
    v20 = a2;
    (*v35__f) = 0;
    v21 = 16 * v19;
    F32((v21 + F32(a2, 0x17c)), 0) = 0;
    F32(v21 + F32(a2, 0x17c), 4) = v17;
    F32(v21 + F32(a2, 0x17c), 8) = 1;
    if ( F32(F32(a2, 0x174), 8) > 0 )
    {
      v22 = 0;
      do
      {
        v23 = a2;
        v24 = 24 * v22;
        if ( F32((F32(v21 + F32(a2, 0x17c), 12) + 24 * v22), 0) != 0 )
        {
          W_grid_cell_text((intptr_t)(a2), (intptr_t)(v19), (intptr_t)(v22), (intptr_t)(D(0x8a9b6c)));
          v23 = a2;
        }
        if ( F32(F32((v21 + F32(v23, 0x17c) + 12), 0) + v24, 4) != 0 )
        {
          W_cell_icon_set((intptr_t)(v23), (intptr_t)(v19), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
          v23 = a2;
        }
        if ( F32(F32((v21 + F32(v23, 0x17c) + 12), 0) + v24, 8) != 0 )
        {
          mm_5962a0(v23, v19, v22, 0, 0, 0, 0, 0, 0, 0, 0);
          v23 = a2;
        }
        v22 = (uint16_t)++(*v35__f);
      }
      while ( (uint16_t)(*v35__f) < F32(F32(v23, 0x174), 8) );
      v13 = (*v39__f);
      v16 = (*v34__f);
LABEL_41:
      v20 = a2;
    }
    W_grid_cell_text((intptr_t)(v20), (intptr_t)(v19), (intptr_t)(0), (intptr_t)((uint8_t *)v16 + 4));
    if ( a4 != 0 )
    {
      switch ( v13[1] )
      {
        case 1:
          if ( v16[42] != 0 )
            W_grid_cell_text((intptr_t)(a2), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3240))));
          else
            W_grid_cell_text((intptr_t)(a2), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(D(0x8a3104)));
          break;
        case 2:
          W_grid_cell_text((intptr_t)(a2), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3230))));
          break;
        case 3:
          W_grid_cell_text((intptr_t)(a2), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a3220))));
          break;
        case 0xFF:
          W_grid_cell_text((intptr_t)(a2), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(((char *)D(0x8a1cb4))));
          break;
        default:
          break;
      }
      if ( F16(v16, 2 * 82) == 1 )
        F32(F32(a2, 0x17c) + 16 * v19, 8) = 0;
    }
  }
  if ( a3 != 0 )
  {
    v25 = 0;
    v26 = F32(a2, 0x174);
    v27 = F32(v26, 16);
    if ( v27 > 0 )
    {
      v5 = 0;
      while ( 1 )
      {
        v28 = (_DWORD *)(F32(a2, 0x17c) + 16 * v5);
        if ( *v28 == 0 && v28[1] == a3 )
          break;
        v5 = ++v25;
        if ( v25 >= v27 )
          return v5;
      }
      v5 = 0;
      F32(v26, 20) = v25;
      v29 = (_DWORD *)F32(a2, 0x174);
      v30 = v29[3];
      v31 = v29[5];
      while ( v31 < v5 || v31 >= v30 + v5 )
        v5 += v30;
      if ( v5 != v29[6] )
        v29[6] = v5;
    }
  }
  return v5;
}

/* 0x5962a0: int __thiscall sub_5962A0( cls_5962A0 *this, int a2, int a3, int a4, __int16 a5, int a6, int a7, int a8, int a9, int a10, int a11) */
int32_t mm_5962a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int16_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    void * v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;

  result = F32(c, 0x174);
  if ( F32(result, 8) > a3 )
  {
    v13 = a2;
    if ( F32(result, 16) > a2 )
    {
      v14 = 16 * a2;
      v15 = 24 * a3;
      if ( F32(F32((F32(c, 0x17c) + 16 * a2 + 12), 0) + 24 * a3, 8) == 0 )
      {
        v16 = (intptr_t)g_eng.op_new(0x20u);
        v13 = a2;
        F32(F32((F32(c, 0x17c) + v14 + 12), 0) + v15, 8) = v16;
      }
      v17 = F32(F32((F32(c, 0x17c) + v14 + 12), 0) + v15, 8);
      F32(v17, 4) = a4;
      F16(v17, 8) = a5;
      F32(v17, 0) = a6;
      F32(v17, 12) = a7;
      F32(v17, 16) = a8;
      F32(v17, 20) = a9;
      F32(v17, 24) = a10;
      result = F32(c, 0x174);
      if ( F32(result, 8) > a3 && F32(result, 16) > v13 )
      {
        result = F32(c, 0x17c);
        v18 = F32(F32(result + v14, 12), v15 + 8);
        if ( v18 != 0 )
        {
          result = v18[6];
          v19 = result;
          if ( result >= a11 )
            v19 = a11;
          v20 = v18[5];
          if ( v20 <= v19 )
          {
            if ( result >= a11 )
              result = a11;
            v18[7] = result;
          }
          else
          {
            result = v18[5];
            v18[7] = v20;
          }
        }
      }
    }
  }
  return result;
}

/* 0x5963a0: int __thiscall sub_5963A0(cls_412590 *this, int a2, int a3, int a4, int a5) */
int32_t mm_5963a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    char * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x42c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x2c);   /* [512] at ebp-0x400 */
    char *v36 = (char *)(frame_ + 0x22c);   /* [512] at ebp-0x200 */
    uint16_t *const v28__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x42c */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x418 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x414 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x410 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x40c */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x408 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x404 */

  v5 = F32(a4, 168);
  (*v30__f) = 0;
  v7 = 2;
  (*v34__f) = v5;
  (*v33__f) = a2 + 2576;
  if ( v5 != 0 )
    v7 = 4;
  g_eng.h5bf3ee(Buffer, "%s-Set", (const char *)(a4 + 4));
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(400), (intptr_t)(32), (intptr_t)(224), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(v7), (intptr_t)(1u), (intptr_t)((intptr_t *)&(*v31__f)), (intptr_t)((int *)&(*v29__f)), (intptr_t)(Buffer), (intptr_t)(0));
  v8 = (*v29__f);
  if ( v5 != 0 )
  {
    v9 = W_grid_row_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(1));
    W_grid_cell_text((intptr_t)(v8), (intptr_t)(v9), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3240))));
    v10 = (*v29__f);
    v11 = W_grid_row_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(2));
    W_grid_cell_text((intptr_t)(v10), (intptr_t)(v11), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3230))));
    v12 = (*v29__f);
    v13 = W_grid_row_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(3));
    W_grid_cell_text((intptr_t)(v12), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3220))));
    g_eng.h5bf3ee(
      Buffer,
      "Select a book in which to place c spell. No book may have more than %d spells. Pressing @@F5 will change the ac"
      "tive spellbook. AI characters will not use spells in book C.",
      10);
    v14 = F32(c, 0x24);
    if ( v14 != 0 )
    {
      (*v28__f) = F16(F32(v14, 368), 8);
      v15 = W_ctrl_keys_expand((intptr_t)(v14), (intptr_t)(Buffer), (intptr_t)(v36));
      W_text_layout_set((intptr_t)(**(intptr_t **)(v14 + 368)), (intptr_t)(v15), (intptr_t)((*v28__f)));
      F16(F32(v14, 368), 4) = 0;
      F16(F32(v14, 368), 6) = 0;
      F32(v14, 28) = 0;
    }
  }
  else
  {
    v16 = W_grid_row_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(1));
    W_grid_cell_text((intptr_t)(v8), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(D(0x8a3104)));
  }
  v17 = (*v29__f);
  v18 = W_grid_row_add((intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(255));
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(((char *)D(0x8a1cb4))));
  v19 = 0;
  (*v32__f) = (*v29__f);
  v20 = F32((*v29__f), 0x174);
  v21 = F32(v20, 16);
  if ( v21 > 0 )
  {
    v22 = 0;
    while ( 1 )
    {
      v23 = (_DWORD *)(F32((*v29__f), 0x17c) + 16 * v22);
      if ( *v23 == 0 && v23[1] == F32(a3, 4) )
        break;
      v22 = ++v19;
      if ( v19 >= v21 )
        goto LABEL_14;
    }
    v24 = (*v32__f);
    F32(v20, 20) = v19;
    W_grid_page_fit((intptr_t)(v24));
  }
LABEL_14:
  mm_593d90(F32((*v31__f), 0), (*v31__f));
  while ( 1 )
  {
    v25 = W_menu_run((intptr_t)(c), (intptr_t)((*v29__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( v25 == 11 )
      break;
    if ( v25 == 12 )
    {
      v26 = F32(16 * F32((F32((*v29__f), 0x174) + 20), 0) + F32((*v29__f), 0x17c), 4);
      if ( (*v34__f) == 0 || v26 == 255 )
      {
        F32(a3, 4) = v26;
        break;
      }
      v25 = mm_596620(c, v26, (*v33__f), a3);
      if ( v25 != 11 )
      {
        if ( v25 == 12 )
          break;
LABEL_23:
        (*v30__f) = v25;
        break;
      }
    }
    else if ( v25 != 13 )
    {
      goto LABEL_23;
    }
  }
  result = (*v30__f);
  F32((*v31__f), 80) = 0;
  return result;
}

/* 0x596620: int __thiscall sub_596620(cls_596620 *this, int a2, int a3, int a4) */
int32_t mm_596620(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
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
    uint16_t v16 = 0;
    int32_t * v17 = 0;
    int v18 = 0;
    intptr_t v19 = 0;
    uint16_t v20 = 0;
    intptr_t result = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0x238 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v25 = (int16_t *)(frame_ + 0x0);   /* [2] at ebp-0x238 */
    char *v31 = (char *)(frame_ + 0x14);   /* [36] at ebp-0x224 */
    char *Buffer = (char *)(frame_ + 0x38);   /* [512] at ebp-0x200 */
    int16_t *const v26__f = (int16_t *)(frame_ + 0x4);   /* ebp-0x234 */
    int16_t *const v27__f = (int16_t *)(frame_ + 0x6);   /* ebp-0x232 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x230 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x22c */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x228 */

  v4 = F32(c, 0x0);
  (*v30__f) = c;
  v5 = 0;
  v6 = 2;
  if ( D(0x8a9b6c) != 0 )
  {
    v5 = 1;
    v6 = 3;
  }
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v31), (intptr_t)(v5));
  v7 = W_win_mgr_open((intptr_t)(v4), (intptr_t)(224), (intptr_t)(96), (intptr_t)(224), (intptr_t)(344), (intptr_t)(v6), (intptr_t)(v31));
  v8 = v7;
  (*v28__f) = v7;
  if ( D(0x8a9b6c) != 0 )
    W_win_title_add((intptr_t)(v7), (intptr_t)(D(0x8a9b6c)));
  v9 = W_win_ctrl_take((intptr_t)(v8));
  v10 = v9;
  if ( v9 != 0 )
    W_ctrl_grid_init((intptr_t)(v9), (intptr_t)(32), (intptr_t)(56), (intptr_t)(10), (intptr_t)(2), (intptr_t)(10), (intptr_t)(0));
  else
    v10 = 0;
  F32(F32(v10, 0x178), 0) = 0;
  F32(F32(v10, 0x178), 4) = 0;
  v11 = F32(v10, 0x178);
  v25[0] = 0;
  v25[1] = 1;
  F32(v11, 8) = 0;
  v12 = F32(v10, 0x178);
  (*v26__f) = 2;
  (*v27__f) = 3;
  F32(v12, 12) = 0;
  F32(F32(v10, 0x178), 36) = 32;
  F32(F32(v10, 0x178), 40) = 0;
  F32(F32(v10, 0x178), 44) = 0;
  F32(F32(v10, 0x178), 48) = 0;
  W_ctrl_cursor_set((intptr_t)(v10), (intptr_t)(SYS32(2956)), (intptr_t)(16), (intptr_t)(v25), (intptr_t)(4u), (intptr_t)(16), (intptr_t)(-32), (intptr_t)(-30));
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v10) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v10, 0x14, 4) = -9;
  }
  v13 = W_win_ctrl_at((intptr_t)(v8), (intptr_t)(0));
  F32(F32(v10, 0x174), 16) = 0;
  switch ( a2 )
  {
    case 1:
      W_grid_cell_text((intptr_t)(v13), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3348))));
      goto LABEL_12;
    case 2:
      W_grid_cell_text((intptr_t)(v13), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3338))));
      goto LABEL_12;
    case 3:
      W_grid_cell_text((intptr_t)(v13), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3328))));
      goto LABEL_12;
    case 255:
      W_grid_cell_text((intptr_t)(v13), (intptr_t)(0), (intptr_t)(0), (intptr_t)(D(0x8a9b6c)));
      goto LABEL_25;
    default:
LABEL_12:
      v14 = 0;
      (*v29__f) = 0;
      F32(v25, 0) = 0;
      break;
  }
  while ( 1 )
  {
    v15 = W_grid_row_add((intptr_t)(v10), (intptr_t)(0), (intptr_t)(v14));
    g_eng.h5bf3ee(Buffer, "%d.", v14 + 1);
    W_grid_cell_text((intptr_t)(v10), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(Buffer));
    v16 = 0;
    if ( F16(a3, 324) != 0 )
    {
      while ( 1 )
      {
        if ( F32(a3 + 12 * v16, 332) == a2 && F16((a3 + 4 * (3 * v16 + 84)), 0) == (uint16_t)(*v29__f) )
        {
          v17 = W_skill_row_find((intptr_t)(F32(a3 + 12 * v16, 328)));
          if ( v17 != 0 && v17[42] != 0 )
            break;
        }
        if ( ++v16 >= F16(a3, 324) )
          goto LABEL_23;
      }
      W_grid_cell_text((intptr_t)(v10), (intptr_t)(v15), (intptr_t)(1), (intptr_t)((uint8_t *)v17 + 4));
    }
    else
    {
LABEL_23:
      W_grid_cell_text((intptr_t)(v10), (intptr_t)(v15), (intptr_t)(1), (intptr_t)(D(0x8a3310)));
    }
    v18 = (uint16_t)++(*v29__f) < 0xAu;
    ++F32(v25, 0);
    if ( !v18 )
      break;
    v14 = F32(v25, 0);
  }
LABEL_25:
  v19 = a4;
  if ( a2 == F32(a4, 4) )
  {
    W_tm_419900((intptr_t)(v10), (intptr_t)(0), (intptr_t)(F16(a4, 8)));
    goto LABEL_33;
  }
  v20 = 0;
  while ( W_member_skill_at((intptr_t)(a3), (intptr_t)(a2), (intptr_t)(v20)) != 0 )
  {
    if ( ++v20 >= 0xAu )
    {
      v20 = 255;
      break;
    }
  }
  W_tm_419900((intptr_t)(v10), (intptr_t)(0), (intptr_t)(v20));
  v19 = a4;
  while ( 1 )
  {
LABEL_33:
    result = W_menu_run((intptr_t)((*v30__f)), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
    {
      F32((*v28__f), 80) = 0;
      return 11;
    }
    if ( result == 12 )
      break;
    if ( result != 13 )
    {
      F32((*v28__f), 80) = 0;
      return result;
    }
  }
  v22 = F32(16 * F32((F32(v10, 0x174) + 20), 0) + F32(v10, 0x17c), 4);
  v23 = W_member_skill_at((intptr_t)(a3), (intptr_t)(a2), (intptr_t)(v22));
  if ( v23 != 0 )
  {
    F32(v23, 4) = F32(v19, 4);
    F16(v23, 8) = F16(v19, 8);
  }
  v24 = (*v28__f);
  F32(v19, 4) = a2;
  F16(v19, 8) = v22;
  F32(v24, 80) = 0;
  return 12;
}

/* 0x596ac0: char *__stdcall sub_596AC0(int a1, char *a2) */
int32_t mm_596ac0(int32_t a1, char * a2)
{


    _Alignas(8) uint8_t frame_[0x200 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [512] at ebp-0x200 */

  strcpy(a2, (const char *)(a1 + 196));
  if ( F32(a1, 724) > 0 )
  {
    g_eng.h5bf3ee(Buffer, "(MP Cost:%d)", F32(a1, 724));
    strcat(a2, Buffer);
  }
  return a2;
}

/* 0x596b50: int __thiscall sub_596B50(cls_412590 *this) */
int32_t mm_596b50(wl_char *self)
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
    intptr_t result = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x28 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x10 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x8 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x4 */

  (*v16__f) = 1;
  v2 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(8), (intptr_t)(346), (intptr_t)(624), (intptr_t)(128), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x20) = (int)v2;
  v3 = W_win_ctrl_take((intptr_t)(v2));
  v4 = v3;
  if ( v3 != 0 )
    W_ctrl_text_init((intptr_t)(v3), (intptr_t)(64), (intptr_t)(24), (intptr_t)(68), (intptr_t)(3));
  (*v15__f) = F32(c, 0x20);
  F32(c, 0x24) = (int)v4;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((*v15__f)), (intptr_t)(v4));
  W_text_metrics_set((intptr_t)(F32(c, 0x24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v5 = F32(c, 0xc);
  (*v17__f) = 24;
  if ( ((char *)D(0x8a2ed8)) != 0 )
    (*v17__f) = 56;
  v6 = (uint16_t)W_room_party_count((intptr_t)(v5), (intptr_t)(0), (intptr_t)(0));
  (*v18__f) = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(48), (intptr_t)(128), (intptr_t)(160), (intptr_t)(28 * v6 + 64), (intptr_t)(2u), (intptr_t)(((char *)D(0x8a2ed8))), (intptr_t)(0));
  v7 = W_win_ctrl_take((intptr_t)((*v18__f)));
  v8 = (int)v7;
  if ( v7 != 0 )
    W_ctrl_grid_init((intptr_t)(v7), (intptr_t)(40), (intptr_t)((*v17__f)), (intptr_t)(v6), (intptr_t)(1), (intptr_t)(v6), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v8), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v8), (intptr_t)(1));
  W_member_rows_add((intptr_t)((int)v5), (intptr_t)(v8), (intptr_t)(0));
  if ( &(*v17__f) == 0 )
    v8 = (*v17__f);
  while ( 1 )
  {
    if ( (*v16__f) != 0 )
    {
      v9 = F32(c, 0x24);
      if ( v9 != 0 )
        W_ctrl_text_set((intptr_t)(v9), (intptr_t)(((char *)D(0x8a3358))));
      (*v16__f) = 0;
    }
    result = W_menu_run((intptr_t)(c), (intptr_t)((_DWORD *)v8), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      v11 = 0;
      while ( 1 )
      {
        v12 = F32(F32(c, 0xc), 4 * v11 + 16432);
        if ( F32(v12, 0x9f0) == F32(16 * F32((F32(v8, 372) + 20), 0) + F32((v8 + 380), 0), 4) )
          break;
        if ( ++v11 >= 8u )
        {
          v13 = 0;
          goto LABEL_21;
        }
      }
      v13 = W_pslot_ok((intptr_t)(v12), (intptr_t)(1), (intptr_t)(0)) ? (unsigned int)v12 : 0;
LABEL_21:
      result = mm_5956c0(c, v13, 1, 32, 8);
      if ( result != 0 )
        goto LABEL_26;
      (*v16__f) = 1;
    }
    else if ( result != 13 )
    {
      goto LABEL_26;
    }
  }
  result = 0;
LABEL_26:
  *(_DWORD *)((uint8_t *)((*v18__f) + (1) * 0x50) + 0x0) = 0;
  v14 = F32(c, 0x20);
  if ( v14 != 0 )
  {
    F32(v14, 80) = 0;
    F32(c, 0x20) = 0;
    F32(c, 0x24) = 0;
  }
  return result;
}

/* 0x597480: int sub_597480() */
int32_t mm_597480(void)
{

    uint16_t v0 = 0;

  v0 = 0;
  while ( SYS32(4 * v0 + 10364) == 0 )
  {
    if ( ++v0 >= 2u )
      return 0;
  }
  return 1;
}

/* 0x5974b0: int __thiscall sub_5974B0(cls_412590 *this, int a2) */
int32_t mm_5974b0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    int32_t * v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x210 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x10);   /* [512] at ebp-0x200 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x210 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x20c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x208 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x204 */

  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2eb4)), ((char *)D(0x8a3600)));
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(200), (intptr_t)(80), (intptr_t)(240), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(2u), (intptr_t)(1u), (intptr_t)(&(*v19__f)), (intptr_t)((int *)&(*v18__f)), (intptr_t)(Buffer), (intptr_t)(0));
  v3 = (*v18__f);
  v4 = W_grid_row_add((intptr_t)((*v18__f)), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v3), (intptr_t)(v4), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3628))));
  v5 = (*v18__f);
  v6 = W_grid_row_add((intptr_t)((*v18__f)), (intptr_t)(0), (intptr_t)(10));
  W_grid_cell_text((intptr_t)(v5), (intptr_t)(v6), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3610))));
  v7 = F32((*v18__f), 0x174);
  v8 = &F32((*v18__f), 0x174);
  (*v21__f) = F32(a2, 16484);
  v9 = F32(v7, 16);
  v10 = 0;
  if ( v9 > 0 )
  {
    v11 = 0;
    (*v20__f) = F32((*v18__f), 0x17c);
    while ( 1 )
    {
      v12 = (_DWORD *)((*v20__f) + 16 * v11);
      if ( *v12 == 0 && v12[1] == (*v21__f) )
        break;
      v11 = ++v10;
      if ( v10 >= v9 )
        goto LABEL_13;
    }
    F32(v7, 20) = v10;
    v13 = (_DWORD *)*v8;
    v14 = 0;
    v15 = v13[3];
    v16 = v13[5];
    while ( v16 < v14 || v16 >= v15 + v14 )
      v14 += v15;
    if ( v14 != v13[6] )
      v13[6] = v14;
  }
LABEL_13:
  while ( 1 )
  {
    result = W_menu_run((intptr_t)(c), (intptr_t)((*v18__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      F32(a2, 16484) = F32(16 * F32((F32((*v18__f), 0x174) + 20), 0) + F32((*v18__f), 0x17c), 4);
      W_menu_party_fill((intptr_t)(c));
      *(_DWORD *)((uint8_t *)((*v19__f) + (1) * 0x50) + 0x0) = 0;
      return 0;
    }
    if ( result != 13 )
    {
      *(_DWORD *)((uint8_t *)((*v19__f) + (1) * 0x50) + 0x0) = 0;
      return result;
    }
  }
  *(_DWORD *)((uint8_t *)((*v19__f) + (1) * 0x50) + 0x0) = 0;
  return 0;
}

/* 0x597670: int __thiscall sub_597670(cls_597670 *this, int a2) */
int32_t mm_597670(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
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
    char * v22 = 0;
    intptr_t v23 = 0;
    char * v24 = 0;
    intptr_t v25 = 0;
    intptr_t result = 0;
    intptr_t v27 = 0;
    _Alignas(8) uint8_t frame_[0x444 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v35 = (uint8_t *)(frame_ + 0x20);   /* [36] at ebp-0x424 */
    char *Buffer = (char *)(frame_ + 0x44);   /* [512] at ebp-0x400 */
    char *v37 = (char *)(frame_ + 0x244);   /* [512] at ebp-0x200 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x444 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x444 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x444 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x440 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x430 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x42c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x428 */

  (*v34__f) = c;
  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2eb4)), ((char *)D(0x85551c)));
  v3 = 0;
  v4 = 0;
  v5 = 2;
  (*v32__f) = F32(c, 0x0);
  v6 = 24;
  v7 = 124;
  if ( Buffer != 0 )
  {
    v7 = 156;
    v6 = 56;
    v4 = 1;
    v5 = 3;
  }
  W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v35), (intptr_t)(v4));
  v8 = W_win_mgr_open((intptr_t)((*v32__f)), (intptr_t)(160), (intptr_t)(80), (intptr_t)(320), (intptr_t)(v7), (intptr_t)(v5), (intptr_t)(v35));
  (*v32__f) = v8;
  if ( &(*v31__f) != (int *)-64 )
    W_win_title_add((intptr_t)((int)v8), (intptr_t)(Buffer));
  v9 = W_win_grid_new((intptr_t)(v8), (intptr_t)(40), (intptr_t)(v6), (intptr_t)(3), (intptr_t)(2), (intptr_t)(3), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v9), (intptr_t)(1));
  F32(F32(v9, 376), 0) = 0;
  F32(F32((v9 + 376), 0), 4) = 0;
  F32(F32((v9 + 376), 0), 8) = 192;
  F32(F32((v9 + 376), 0), 12) = 0;
  F32(F32((v9 + 376), 0), 36) = 192;
  F32(F32((v9 + 376), 0), 40) = 0;
  F32(F32((v9 + 376), 0), 44) = 128;
  F32(F32((v9 + 376), 0), 48) = 0;
  v10 = F32(v9, 372);
  v11 = F32(v10, 16);
  if ( v11 < F32(v10, 4) )
  {
    (*v28__f) = F32(v10, 16);
    F32(v10, 16) = v11 + 1;
    v12 = 16 * v11;
    F32((v12 + F32(v9, 380)), 0) = 0;
    F32(v12 + F32((v9 + 380), 0), 4) = 0;
    F32(v12 + F32((v9 + 380), 0), 8) = 1;
    W_grid_row_reset((intptr_t)(v9), (intptr_t)((*v28__f)));
  }
  v13 = F32(v9, 372);
  v14 = F32(v13, 16);
  if ( v14 < F32(v13, 4) )
  {
    (*v29__f) = F32(v13, 16);
    F32(v13, 16) = v14 + 1;
    v15 = 16 * v14;
    F32((v15 + F32(v9, 380)), 0) = 0;
    F32(v15 + F32((v9 + 380), 0), 4) = 1;
    F32(v15 + F32((v9 + 380), 0), 8) = 1;
    W_grid_row_reset((intptr_t)(v9), (intptr_t)((*v29__f)));
  }
  v16 = F32(v9, 372);
  v17 = F32(v16, 16);
  if ( v17 < F32(v16, 4) )
  {
    (*v30__f) = F32(v16, 16);
    F32(v16, 16) = v17 + 1;
    v18 = 16 * v17;
    F32((v18 + F32(v9, 380)), 0) = 0;
    F32(v18 + F32((v9 + 380), 0), 4) = 2;
    F32(v18 + F32((v9 + 380), 0), 8) = 1;
    W_grid_row_reset((intptr_t)(v9), (intptr_t)((*v30__f)));
  }
  while ( 2 )
  {
    v19 = F32(v9, 372);
    (*v33__f) = 0;
    if ( F32(v19, 16) <= 0 )
      goto LABEL_28;
    do
    {
      v20 = F32(16 * v3 + F32((v9 + 380), 0), 4);
      if ( v20 == 0 )
      {
        strcpy(Buffer, ((char *)D(0x8a3678)));
        v22 = ((char *)D(0x8a3640));
        goto LABEL_18;
      }
      v21 = v20 - 1;
      if ( v21 == 0 )
      {
        strcpy(Buffer, ((char *)D(0x8a36e4)));
        v22 = ((char *)D(0x8a3690));
        goto LABEL_18;
      }
      if ( v21 == 1 )
      {
        strcpy(Buffer, ((char *)D(0x8a3750)));
        v22 = ((char *)D(0x8a36fc));
LABEL_18:
        strcpy(v37, v22);
      }
      W_grid_cell_text((intptr_t)(v9), (intptr_t)(v3), (intptr_t)(0), (intptr_t)(Buffer));
      if ( F32(F32((v9 + 372), 0), 20) == v3 )
      {
        v23 = F32((*v34__f), 0x24);
        if ( v23 != 0 )
          W_ctrl_text_set((intptr_t)(v23), (intptr_t)(v37));
      }
      if ( (uint16_t)(*v33__f) < 3u && F16(a2 + 2 * v3, 16476) != 0 )
        v24 = ((char *)D(0x8a1cb4));
      else
        v24 = D(0x8a3104);
      strcpy(Buffer, v24);
      W_grid_cell_text((intptr_t)(v9), (intptr_t)(v3), (intptr_t)(1), (intptr_t)(Buffer));
      v25 = F32(F32((v9 + 372), 0), 16);
      v3 = (uint16_t)++(*v33__f);
    }
    while ( (uint16_t)(*v33__f) < v25 );
    v3 = 0;
LABEL_28:
    result = W_menu_run((intptr_t)((*v34__f)), (intptr_t)((_DWORD *)v9), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v27 = F32(16 * F32((F32(v9, 372) + 20), 0) + F32((v9 + 380), 0), 4);
    if ( result != 11 )
    {
      if ( result == 12 )
      {
        if ( (uint16_t)v27 < 3u )
          F16(a2 + 2 * (uint16_t)v27, 16476) = F16(a2 + 2 * (uint16_t)v27, 16476) == 0;
        continue;
      }
      if ( result != 13 )
      {
        F32((*v32__f) + (10) * 0x8, 0x0) = 0;
        return result;
      }
      continue;
    }
    break;
  }
  F32((*v32__f) + (10) * 0x8, 0x0) = 0;
  return 0;
}

/* 0x598d00: int __thiscall sub_598D00(cls_412590 *this) */
int32_t mm_598d00(wl_char *self)
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
    char * v14 = 0;
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
    _Alignas(8) uint8_t frame_[0x448 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v37 = (char *)(frame_ + 0x48);   /* [512] at ebp-0x400 */
    char *Buffer = (char *)(frame_ + 0x248);   /* [512] at ebp-0x200 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x448 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x41c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x418 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x414 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x410 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x40c */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x408 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x404 */

  v1 = 0;
  (*v29__f) = F32(c, 0x0);
  (*v32__f) = 0;
  v3 = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((*v29__f)), (intptr_t)(8), (intptr_t)(346), (intptr_t)(624), (intptr_t)(128), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1));
  F32(c, 0x20) = (int)v3;
  v4 = W_win_ctrl_take((intptr_t)(v3));
  v5 = v4;
  if ( v4 != 0 )
  {
    W_ctrl_text_init((intptr_t)(v4), (intptr_t)(64), (intptr_t)(24), (intptr_t)(68), (intptr_t)(3));
    v6 = v5;
  }
  else
  {
    v6 = 0;
  }
  v7 = F32(c, 0x20);
  F32(c, 0x24) = (int)v6;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v7), (intptr_t)(v6));
  W_text_metrics_set((intptr_t)(F32(c, 0x24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(96), (intptr_t)(32), (intptr_t)(176), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(3u), (intptr_t)(1u), (intptr_t)(&(*v35__f)), (intptr_t)((int *)&(*v31__f)), (intptr_t)(((char *)D(0x8a2ec8))), (intptr_t)(1));
  v8 = (*v31__f);
  v9 = W_grid_row_add((intptr_t)((*v31__f)), (intptr_t)(0), (intptr_t)(13));
  W_grid_cell_text((intptr_t)(v8), (intptr_t)(v9), (intptr_t)(0), (intptr_t)(((char *)D(0x8a39c4))));
  v10 = (*v31__f);
  v11 = W_grid_row_add((intptr_t)((*v31__f)), (intptr_t)(0), (intptr_t)(15));
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(v11), (intptr_t)(0), (intptr_t)(((char *)D(0x8a39b8))));
  v12 = (*v31__f);
  v13 = W_grid_row_add((intptr_t)((*v31__f)), (intptr_t)(0), (intptr_t)(14));
  W_grid_cell_text((intptr_t)(v12), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(((char *)D(0x8a39ac))));
  (*v34__f) = F32(16 * F32((F32((*v31__f), 0x174) + 20), 0) + F32((*v31__f), 0x17c), 4);
  do
  {
    while ( 1 )
    {
      switch ( (*v34__f) )
      {
        case 13:
          v14 = ((char *)D(0x8a38b0));
          break;
        case 14:
          v14 = ((char *)D(0x8a38e8));
          break;
        case 15:
          v14 = ((char *)D(0x8a3968));
          break;
        default:
          goto LABEL_12;
      }
      strcpy(v37, v14);
LABEL_12:
      v15 = F32(c, 0x24);
      if ( v15 != 0 )
        W_ctrl_text_set((intptr_t)(v15), (intptr_t)(v37));
      v16 = W_menu_run((intptr_t)(c), (intptr_t)((*v31__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      v17 = F32(16 * F32((F32((*v31__f), 0x174) + 20), 0) + F32((*v31__f), 0x17c), 4);
      (*v34__f) = v17;
      if ( v16 == 11 )
        goto LABEL_38;
      if ( v16 == 12 )
        break;
      if ( v16 != 13 )
        goto LABEL_37;
    }
    v18 = v17 - 13;
    if ( v18 != 0 )
    {
      v19 = v18 - 1;
      if ( v19 != 0 )
      {
        if ( v19 == 1 )
          v16 = mm_599ac0(c, F32(c, 0xc));
      }
      else
      {
        v16 = mm_5994e0(c, (char *)F32(c, 0xc));
      }
    }
    else
    {
      v20 = F32(c, 0xc);
      (*v33__f) = 0;
      v21 = 1;
      g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2ec8)), ((char *)D(0x8a39c4)));
      (*v36__f) = mm_594a20(c, (int)v20, 160, 96, 1, 2, 320, 0, Buffer, (int *)&(*v30__f), 0, 0, 0);
      v22 = W_grid_row_add((intptr_t)((*v30__f)), (intptr_t)(0), (intptr_t)(0));
      W_grid_cell_text((intptr_t)((*v30__f)), (intptr_t)(v22), (intptr_t)(0), (intptr_t)(((char *)D(0x8a38a4))));
      W_grid_col_place((intptr_t)((*v30__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(128), (intptr_t)(0));
      W_grid_col_place((intptr_t)((*v30__f)), (intptr_t)(1), (intptr_t)(128), (intptr_t)(0), (intptr_t)(128), (intptr_t)(0));
      W_member_rows_add((intptr_t)((int)v20), (intptr_t)((*v30__f)), (intptr_t)(0));
      while ( 1 )
      {
        v23 = (*v30__f);
        if ( v21 != 0 )
        {
          if ( F32(F32((*v30__f), 0x174), 16) > 0 )
          {
            v24 = 0;
            do
            {
              v25 = W_rd_4961e0((intptr_t)(v20), (intptr_t)(F32(16 * v24 + F32(v23, 0x17c), 4)), (intptr_t)(0));
              mm_5990b0(v25, Buffer);
              W_grid_cell_text((intptr_t)((*v30__f)), (intptr_t)(v24), (intptr_t)(1), (intptr_t)(Buffer));
              v23 = (*v30__f);
              v24 = (uint16_t)++v1;
            }
            while ( (uint16_t)v1 < F32(F32((*v30__f), 0x174), 16) );
          }
          v21 = 0;
          v1 = 0;
        }
        v26 = W_menu_run((intptr_t)(c), (intptr_t)(v23), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v26 == 11 )
          break;
        if ( v26 == 12 )
        {
          mm_599140(c, v20, F32(16 * F32((F32((*v30__f), 0x174) + 20), 0) + F32((*v30__f), 0x17c), 4));
          W_menu_msg_set((intptr_t)(c), (intptr_t)(D(0x8a9b6c)));
          v21 = 1;
        }
        else if ( v26 != 13 )
        {
          (*v33__f) = v26;
          break;
        }
      }
      v16 = (*v33__f);
      F32((*v36__f), 80) = 0;
    }
  }
  while ( v16 == 0 );
LABEL_37:
  (*v32__f) = v16;
LABEL_38:
  *(_DWORD *)((uint8_t *)((*v35__f) + (1) * 0x50) + 0x0) = 0;
  v27 = F32(c, 0x20);
  if ( v27 != 0 )
  {
    F32(v27, 80) = 0;
    F32(c, 0x20) = 0;
    F32(c, 0x24) = 0;
  }
  return (*v32__f);
}

/* 0x5990b0: char *__stdcall sub_5990B0(int a1, char *a2) */
int32_t mm_5990b0(int32_t a1, char * a2)
{

    char * v2 = 0;

  if ( a1 > 255 )
  {
LABEL_10:
    v2 = D(0x8a9b6c);
  }
  else if ( a1 == 255 )
  {
    v2 = ((char *)D(0x8a39d0));
  }
  else
  {
    switch ( a1 )
    {
      case 0:
        v2 = ((char *)D(0x8a3a14));
        break;
      case 1:
        v2 = ((char *)D(0x8a3a08));
        break;
      case 2:
        v2 = ((char *)D(0x8a39f8));
        break;
      case 3:
        v2 = ((char *)D(0x8a39ec));
        break;
      case 4:
        v2 = ((char *)D(0x8a39dc));
        break;
      default:
        goto LABEL_10;
    }
  }
  return strcpy(a2, v2);
}

/* 0x599140: int __thiscall sub_599140(cls_412590 *this, _DWORD *a2, int a3) */
int32_t mm_599140(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
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
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x408 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x8);   /* [512] at ebp-0x400 */
    char *v25 = (char *)(frame_ + 0x208);   /* [512] at ebp-0x200 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x408 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x404 */

  v4 = 5;
  v5 = W_pslot_by_id((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0));
  v6 = v5;
  if ( v5 != 0 )
  {
    v4 = 6;
    g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a39c4)), v5 + 2576);
  }
  else
  {
    g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a39c4)), ((char *)D(0x8a38a4)));
  }
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(288), (intptr_t)(64), (intptr_t)(240), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(v4), (intptr_t)(1u), (intptr_t)(&(*v23__f)), (intptr_t)((int *)&(*v22__f)), (intptr_t)(Buffer), (intptr_t)(0));
  if ( v6 != 0 )
  {
    strcpy(Buffer, ((char *)D(0x8a39d0)));
    v7 = (*v22__f);
    v8 = W_grid_row_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(255));
    W_grid_cell_text((intptr_t)(v7), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(Buffer));
  }
  strcpy(Buffer, ((char *)D(0x8a3a14)));
  v9 = (*v22__f);
  v10 = W_grid_row_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(Buffer));
  strcpy(Buffer, ((char *)D(0x8a3a08)));
  v11 = (*v22__f);
  v12 = W_grid_row_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(1));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(Buffer));
  strcpy(Buffer, ((char *)D(0x8a39f8)));
  v13 = (*v22__f);
  v14 = W_grid_row_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(2));
  W_grid_cell_text((intptr_t)(v13), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(Buffer));
  strcpy(Buffer, ((char *)D(0x8a39ec)));
  v15 = (*v22__f);
  v16 = W_grid_row_add((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(3));
  W_grid_cell_text((intptr_t)(v15), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(Buffer));
  v17 = (*v22__f);
  strcpy(Buffer, ((char *)D(0x8a39dc)));
  W_grid_row_text_add((intptr_t)(v17), (intptr_t)(0), (intptr_t)(4), (intptr_t)(Buffer));
  v18 = W_rd_4961e0((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0));
  W_tm_419900((intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(v18));
  while ( 1 )
  {
    v19 = F32(16 * F32((F32((*v22__f), 0x174) + 20), 0) + F32((*v22__f), 0x17c), 4);
    if ( v19 <= 255 )
    {
      if ( v19 == 255 )
      {
        v20 = ((char *)D(0x8a3a24));
LABEL_16:
        strcpy(v25, v20);
      }
      else
      {
        switch ( v19 )
        {
          case 0:
            v20 = ((char *)D(0x8a3b38));
            goto LABEL_16;
          case 1:
            v20 = ((char *)D(0x8a3b1c));
            goto LABEL_16;
          case 2:
            v20 = ((char *)D(0x8a3ab4));
            goto LABEL_16;
          case 3:
            v20 = ((char *)D(0x8a3a74));
            goto LABEL_16;
          case 4:
            v20 = ((char *)D(0x8a3a4c));
            goto LABEL_16;
          default:
            break;
        }
      }
    }
    W_menu_msg_set((intptr_t)(c), (intptr_t)(v25));
    result = W_menu_run((intptr_t)(c), (intptr_t)((*v22__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      W_mh_4e61d0((intptr_t)(a2), (intptr_t)(F32(16 * F32((F32((*v22__f), 0x174) + 20), 0) + F32((*v22__f), 0x17c), 4)), (intptr_t)(a3));
      *(_DWORD *)((uint8_t *)((*v23__f) + (1) * 0x50) + 0x0) = 0;
      return 0;
    }
    if ( result != 13 )
    {
      *(_DWORD *)((uint8_t *)((*v23__f) + (1) * 0x50) + 0x0) = 0;
      return result;
    }
  }
  *(_DWORD *)((uint8_t *)((*v23__f) + (1) * 0x50) + 0x0) = 0;
  return 0;
}

/* 0x5994e0: int __thiscall sub_5994E0(cls_5994E0 *this, char *a2) */
int32_t mm_5994e0(wl_char *self, char * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    uint16_t v3 = 0;
    int v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    char * v7 = 0;
    char * v8 = 0;
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
    uint16_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    char * v24 = 0;
    char * v25 = 0;
    intptr_t i = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    uint16_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    _Alignas(8) uint8_t frame_[0x280 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v65 = (char *)(frame_ + 0x60);   /* [32] at ebp-0x220 */
    char *Buffer = (char *)(frame_ + 0x80);   /* [512] at ebp-0x200 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x280 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x270 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x26c */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x268 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x264 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x260 */
    int16_t *const v44__f = (int16_t *)(frame_ + 0x26);   /* ebp-0x25a */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x258 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x254 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x250 */
    int16_t *const v48__f = (int16_t *)(frame_ + 0x34);   /* ebp-0x24c */
    int16_t *const v49__f = (int16_t *)(frame_ + 0x36);   /* ebp-0x24a */
    uint16_t * *const v50__f = (uint16_t * *)(frame_ + 0x38);   /* ebp-0x248 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x244 */
    int16_t *const v52__f = (int16_t *)(frame_ + 0x40);   /* ebp-0x240 */
    int16_t *const v53__f = (int16_t *)(frame_ + 0x42);   /* ebp-0x23e */
    int16_t *const v54__f = (int16_t *)(frame_ + 0x44);   /* ebp-0x23c */
    int16_t *const v55__f = (int16_t *)(frame_ + 0x46);   /* ebp-0x23a */
    int16_t *const v56__f = (int16_t *)(frame_ + 0x48);   /* ebp-0x238 */
    int16_t *const v57__f = (int16_t *)(frame_ + 0x4a);   /* ebp-0x236 */
    int16_t *const v58__f = (int16_t *)(frame_ + 0x4c);   /* ebp-0x234 */
    int16_t *const v59__f = (int16_t *)(frame_ + 0x4e);   /* ebp-0x232 */
    int16_t *const v60__f = (int16_t *)(frame_ + 0x50);   /* ebp-0x230 */
    int16_t *const v61__f = (int16_t *)(frame_ + 0x52);   /* ebp-0x22e */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x22c */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x228 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x224 */

  (*v40__f) = 1;
  (*v62__f) = 32;
  (*v63__f) = 32;
  v2 = F32(c, 0x0);
  (*v54__f) = 1;
  (*v52__f) = 16;
  (*v53__f) = 0;
  (*v55__f) = 2;
  (*v56__f) = 3;
  (*v57__f) = 4;
  (*v58__f) = 5;
  (*v59__f) = 6;
  (*v60__f) = 7;
  (*v61__f) = 8;
  (*v51__f) = SYS32(2984);
  (*v64__f) = 0;
  v3 = F16(v2, 6);
  v4 = v3 < F16(v2, 4);
  (*v43__f) = c;
  (*v46__f) = 0;
  if ( v4 )
  {
    v5 = 4 * v3;
    W_win_init((intptr_t)(F32(F32(v2, 0), v5)), (intptr_t)(v2), (intptr_t)(136), (intptr_t)(192), (intptr_t)(368), (intptr_t)(144), (intptr_t)(0xDu), (intptr_t)(&(*v51__f)));
    ++F16(v2, 6);
    W_win_mgr_topmost((intptr_t)(v2));
    (*v42__f) = F32(F32(v2, 0), v5);
  }
  else
  {
    (*v42__f) = 0;
  }
  v6 = a2;
  v7 = v65;
  (*v39__f) = 8;
  v8 = a2 + 16432;
  do
  {
    v9 = W_win_ctrl_take((intptr_t)((*v42__f)));
    v10 = v9;
    if ( v9 != 0 )
    {
      W_ctrl_grid_init((intptr_t)(v9), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
      W_grid_col_place((intptr_t)(v10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      W_grid_row_add((intptr_t)(v10), (intptr_t)(0), (intptr_t)(0));
    }
    v11 = F32(v8, 0);
    F32(v7, 0) = v10;
    if ( F32(v11, 2500) == 1 )
    {
      v12 = F32(v11, 2548);
      if ( v12 != 0 )
      {
        mm_599a60(v12, &(*v45__f), &(*v44__f), &(*v50__f), 1, (_BOOL2 *)&(*v41__f));
        W_rd_417c40((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((*v45__f)), (intptr_t)(0));
        if ( (uint16_t)(*v45__f) != 0 )
          W_cell_icon_set((intptr_t)(v10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(4 * (uint16_t)(*v45__f) + 2732)), (intptr_t)((*v44__f) + *(*v50__f)), (intptr_t)((*v50__f) + 1), (intptr_t)((*v50__f)[33]), (intptr_t)((*v50__f)[34]));
      }
      else
      {
        (*v45__f) = 0;
      }
    }
    v7 += 4;
    v8 += 4;
    --(*v39__f);
  }
  while ( (*v39__f) != 0 );
  v13 = (*v42__f);
  v14 = W_win_ctrl_take((intptr_t)((*v42__f)));
  v15 = v14;
  if ( v14 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v14), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v15), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_grid_row_add((intptr_t)(v15), (intptr_t)(0), (intptr_t)(0));
    (*v41__f) = v15;
  }
  else
  {
    (*v41__f) = 0;
  }
  W_cell_icon_set((intptr_t)((*v41__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(SYS32(2956)), (intptr_t)(3), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  v16 = W_win_ctrl_take((intptr_t)(v13));
  v17 = v16;
  if ( v16 != 0 )
  {
    W_ctrl_grid_init((intptr_t)(v16), (intptr_t)(32), (intptr_t)(20), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
    W_grid_col_place((intptr_t)(v17), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    W_grid_row_add((intptr_t)(v17), (intptr_t)(0), (intptr_t)(0));
  }
  else
  {
    v17 = 0;
  }
  W_grid_cell_text((intptr_t)(v17), (intptr_t)(0), (intptr_t)(0), (intptr_t)(((char *)D(0x8a3bb0))));
  (*v47__f) = 0x10000;
  (*v48__f) = 2;
  (*v49__f) = 3;
  if ( F32(CFG(), 596) == 0 )
  {
    F32(((uint8_t *)(v17) + 0x14), 0) = 1;
    *(_DWORD *)&GAP(v17, 0x14, 4) = -9;
  }
  *(_DWORD *)((uint8_t *)(v17 + (1) * 0x180) + 0x0) = D(0x5d6d5d);
  *(_DWORD *)&GAP((v17 + (1) * 0x180), 0x0, 4) = 2838315;
  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2ec8)), ((char *)D(0x8a39ac)));
  v18 = 24;
  if ( Buffer != 0 )
    v18 = 56;
  v19 = W_room_party_count((intptr_t)(a2), (intptr_t)(1), (intptr_t)(0));
  (*v47__f) = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32((*v43__f), 0x0)), (intptr_t)(192), (intptr_t)(16), (intptr_t)(256), (intptr_t)(176), (intptr_t)(2u), (intptr_t)(Buffer), (intptr_t)(0));
  v20 = W_win_grid_new((intptr_t)((*v47__f)), (intptr_t)(40), (intptr_t)(v18), (intptr_t)(v19), (intptr_t)(1), (intptr_t)(v19), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v20), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v20), (intptr_t)(1));
  W_member_rows_add((intptr_t)((int)a2), (intptr_t)(v20), (intptr_t)(1));
  if ( &(*v38__f) == (int *)-16 )
  {
    v21 = (*v39__f);
  }
  else
  {
    v21 = v20;
    (*v39__f) = (int)v20;
  }
  v22 = F32(16 * F32((*(_DWORD *)&GAP(v21, 0x0, 372) + 20), 0) + *(_DWORD *)((uint8_t *)(v21 + (1) * 0x17c) + 0x0), 4);
  v23 = F32((*v43__f), 0x24);
  if ( v23 != 0 )
    W_ctrl_text_set((intptr_t)(v23), (intptr_t)(((char *)D(0x8a3b60))));
  while ( 1 )
  {
LABEL_28:
    if ( (*v40__f) != 0 )
    {
      v24 = &GAP(v6, 0x4012, 30);
      v25 = v65;
      for ( i = 8; i != 0; --i )
      {
        v27 = F32(v24, 0);
        if ( F32(F32(v24, 0), 2500) == 1 )
        {
          v28 = F32(v25, 0);
          v29 = 7 - F32(v27, 2552);
          F32(v28, 16) = 16;
          F32(v28, 12) = 40 * v29;
          if ( v22 == F32(v27, 2544) )
          {
            v30 = (*v41__f);
            *(_DWORD *)&GAP((*v41__f), 0x0, 12) = 300 - 40 * F32(v27, 2552);
            *(_DWORD *)&GAP(v30, 0x0, 16) = 104;
            SETLO(v30, F16(v28, 4));
            W_win_ctrl_to_back((intptr_t)(F32(v28, 0)), (intptr_t)((int)v30));
          }
        }
        v24 += 4;
        v25 += 4;
      }
      v6 = a2;
      v21 = (*v39__f);
      (*v40__f) = 0;
    }
    v31 = W_menu_run((intptr_t)((*v43__f)), (intptr_t)(v21), (intptr_t)(0), (intptr_t)(1), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    v22 = F32(16 * F32((*(_DWORD *)&GAP(v21, 0x0, 372) + 20), 0) + *(_DWORD *)((uint8_t *)(v21 + (1) * 0x17c) + 0x0), 4);
    switch ( v31 )
    {
      case 11:
      case 12:
        goto LABEL_50;
      case 13:
        (*v40__f) = 1;
        continue;
      case 16:
        v34 = W_pslot_by_id((intptr_t)(v6), (intptr_t)(v22), (intptr_t)(1));
        if ( v34 == 0 )
          continue;
        v35 = F32(v34, 2552) + 1;
        if ( v35 > 7 )
          v35 = 7;
        F32(v34, 2552) = v35;
        goto LABEL_44;
      case 17:
        v32 = 0;
        break;
      default:
        (*v46__f) = v31;
LABEL_50:
        v36 = (*v47__f);
        *(_DWORD *)((uint8_t *)((*v42__f) + (1) * 0x50) + 0x0) = 0;
        F32(v36, 80) = 0;
        return (*v46__f);
    }
    while ( 1 )
    {
      v33 = *(_DWORD *)&GAP(v6, 0x4012, 4 * v32 + 30);
      if ( F32(v33, 2544) == v22 )
        break;
      if ( ++v32 >= 8u )
        goto LABEL_28;
    }
    if ( W_pslot_ok((intptr_t)(v33), (intptr_t)(1), (intptr_t)(1)) && v33 != 0 )
    {
      F32(v33, 2552) = F32(v33, 2552) - 1 < 0 ? 0 : F32(v33, 2552) - 1;
LABEL_44:
      W_room_formation((intptr_t)(v6));
      W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(9u), (intptr_t)(0), (intptr_t)(0));
      (*v40__f) = 1;
    }
  }
}

/* 0x599a60: _DWORD *__thiscall sub_599A60(cls_599A60 *this, _WORD *a2, _WORD *a3, _DWORD *a4, int a5, _BOOL2 *a6) */
int32_t mm_599a60(wl_char *self, uint16_t * a2, uint16_t * a3, int32_t * a4, int32_t a5, uint16_t * a6)
{
    intptr_t c = (intptr_t)self;
    uint16_t v6 = 0;

  v6 = false;
  if ( F32(c, 0x1d4) == 50010 )
    v6 = a5 != 0;
  *a2 = *(_WORD *)&GAP(c, 0x0, 20 * v6 + 72);
  *a3 = *(_WORD *)&GAP(c, 0x0, 20 * v6 + 74);
  *a4 = F32(c, 0x15914);
  *a6 = v6;
  return a4;
}

/* 0x599ac0: int __thiscall sub_599AC0(cls_599AC0 *this, int a2) */
int32_t mm_599ac0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    char * v16 = 0;
    intptr_t result = 0;
    intptr_t v18 = 0;
    char * v19 = 0;
    _Alignas(8) uint8_t frame_[0x424 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x24);   /* [512] at ebp-0x400 */
    char *v28 = (char *)(frame_ + 0x224);   /* [512] at ebp-0x200 */
    uint16_t *const v21__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x424 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x424 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x410 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x410 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x40c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x408 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x404 */

  v2 = c;
  (*v26__f) = c;
  g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a2ec8)), ((char *)D(0x8a39b8)));
  v3 = 24;
  if ( Buffer != 0 )
    v3 = 56;
  v4 = (uint16_t)(W_room_party_count((intptr_t)(a2), (intptr_t)(1), (intptr_t)(0)) + 1);
  (*v24__f) = W_sys_win_open((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(F32(v2, 0x0)), (intptr_t)(176), (intptr_t)(96), (intptr_t)(288), (intptr_t)(28 * v4 + 64), (intptr_t)(2u), (intptr_t)(Buffer), (intptr_t)(0));
  v5 = W_win_grid_new((intptr_t)((*v24__f)), (intptr_t)(40), (intptr_t)(v3), (intptr_t)(v4), (intptr_t)(2), (intptr_t)(v4), (intptr_t)(0));
  W_grid_col_place((intptr_t)(v5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)((int)v5), (intptr_t)(1));
  if ( &(*v24__f) == 0 )
    v5 = (*v24__f);
  v6 = *(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 52);
  v7 = F32(v6, 16);
  if ( v7 < F32(v6, 4) )
  {
    (*v20__f) = F32(v6, 16);
    F32(v6, 16) = v7 + 1;
    v8 = 16 * v7;
    F32((*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 60) + v8), 0) = 0;
    F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 60) + v8, 4) = 0;
    F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 60) + v8, 8) = 1;
    W_grid_row_reset((intptr_t)(v5), (intptr_t)((*v20__f)));
  }
  else
  {
    v7 = -1;
  }
  W_grid_cell_text((intptr_t)(v5), (intptr_t)(v7), (intptr_t)(0), (intptr_t)(((char *)D(0x8a38a4))));
  v9 = (intptr_t *)(a2 + 16432);
  for ( (*i__f) = 8; (*i__f) != 0; --(*i__f) )
  {
    v10 = F32(v9, 0);
    if ( W_pslot_ok((intptr_t)(F32(v9, 0)), (intptr_t)(1), (intptr_t)(0)) )
      W_grid_row_text_add((intptr_t)(v5), (intptr_t)(1), (intptr_t)(F32(v10, 0x9f0)), (intptr_t)(&GAP((v10 + (1) * 0x9f4), 0x0, 28)));
    ++v9;
  }
  v11 = 0;
  **(_DWORD **)&GAP((v5 + (4) * 0x50), 0x0, 56) = 0;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 4) = 0;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 8) = 128;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 12) = 0;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 36) = 128;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 40) = 0;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 44) = 128;
  F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 56), 48) = 0;
  while ( 1 )
  {
    (*v23__f) = 0;
    if ( F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 52), 16) > 0 )
    {
      do
      {
        (*v25__f) = F32(16 * v11 + *(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 60), 4);
        if ( (*v25__f) != 0 )
        {
          v12 = 0;
          while ( 1 )
          {
            v13 = F32(a2 + 4 * v12, 16432);
            if ( W_pslot_ok((intptr_t)(v13), (intptr_t)(1), (intptr_t)(1)) && (*v25__f) == F32(v13, 2544) )
              break;
            if ( ++v12 >= 8u )
              goto LABEL_20;
          }
          v14 = F32(v13, 2556);
        }
        else
        {
LABEL_20:
          v14 = F32(a2, 16464);
        }
        if ( v14 != 0 )
        {
          v15 = v14 - 1;
          if ( v15 != 0 )
          {
            if ( v15 == 1 )
              v16 = ((char *)D(0x8a3be0));
            else
              v16 = ((char *)D(0x8a39d0));
          }
          else
          {
            v16 = ((char *)D(0x8a3bd0));
          }
        }
        else
        {
          v16 = ((char *)D(0x8a3bc0));
        }
        strcpy(Buffer, v16);
        W_grid_cell_text((intptr_t)(v5), (intptr_t)(v11), (intptr_t)(1), (intptr_t)(Buffer));
        v11 = (uint16_t)++(*v23__f);
      }
      while ( (uint16_t)(*v23__f) < F32(*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 52), 16) );
      v2 = (*v26__f);
      v11 = 0;
    }
    result = W_menu_run((intptr_t)(v2), (intptr_t)(v5), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      mm_599e60(
        v2,
        (_DWORD *)a2,
        F32(16 * F32((*(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 52) + 20), 0) + *(_DWORD *)&GAP((v5 + (4) * 0x50), 0x0, 60), 4));
      v18 = F32(v2, 0x24);
      if ( v18 != 0 )
      {
        (*v21__f) = F16(F32(v18, 368), 8);
        v19 = W_ctrl_keys_expand((intptr_t)(v18), (intptr_t)(D(0x8a9b6c)), (intptr_t)(v28));
        W_text_layout_set((intptr_t)(**(intptr_t **)(v18 + 368)), (intptr_t)(v19), (intptr_t)((*v21__f)));
        F16(F32(v18, 368), 4) = 0;
        F16(F32(v18, 368), 6) = 0;
        F32(v18, 28) = 0;
      }
      v11 = 0;
    }
    else if ( result != 13 )
    {
      *(_DWORD *)((uint8_t *)((*v24__f) + (1) * 0x50) + 0x0) = 0;
      return result;
    }
  }
  *(_DWORD *)((uint8_t *)((*v24__f) + (1) * 0x50) + 0x0) = 0;
  return 0;
}

/* 0x599e60: int __thiscall sub_599E60(cls_412590 *this, _DWORD *a2, int a3) */
int32_t mm_599e60(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
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
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    char * v21 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x408 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x8);   /* [512] at ebp-0x400 */
    char *v26 = (char *)(frame_ + 0x208);   /* [512] at ebp-0x200 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x408 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x404 */

  v4 = 3;
  v5 = W_pslot_by_id((intptr_t)(a2), (intptr_t)(a3), (intptr_t)(0));
  v6 = v5;
  if ( v5 != 0 )
  {
    v4 = 4;
    g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a39b8)), v5 + 2576);
  }
  else
  {
    g_eng.h5bf3ee(Buffer, "%s - %s", ((char *)D(0x8a39b8)), ((char *)D(0x8a38a4)));
  }
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(288), (intptr_t)(128), (intptr_t)(240), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(v4), (intptr_t)(1u), (intptr_t)(&(*v24__f)), (intptr_t)((int *)&(*v23__f)), (intptr_t)(Buffer), (intptr_t)(0));
  if ( v6 != 0 )
  {
    strcpy(Buffer, ((char *)D(0x8a39d0)));
    v7 = (*v23__f);
    v8 = W_grid_row_add((intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(255));
    W_grid_cell_text((intptr_t)(v7), (intptr_t)(v8), (intptr_t)(0), (intptr_t)(Buffer));
  }
  strcpy(Buffer, ((char *)D(0x8a3bc0)));
  v9 = (*v23__f);
  v10 = W_grid_row_add((intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(0));
  W_grid_cell_text((intptr_t)(v9), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(Buffer));
  strcpy(Buffer, ((char *)D(0x8a3be0)));
  v11 = (*v23__f);
  v12 = W_grid_row_add((intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(2));
  W_grid_cell_text((intptr_t)(v11), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(Buffer));
  strcpy(Buffer, ((char *)D(0x8a3bd0)));
  v13 = (*v23__f);
  v14 = W_grid_row_add((intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(1));
  W_grid_cell_text((intptr_t)(v13), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(Buffer));
  strcpy(Buffer, ((char *)D(0x8a39d0)));
  v15 = (*v23__f);
  v16 = W_grid_row_add((intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(255));
  W_grid_cell_text((intptr_t)(v15), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(Buffer));
  if ( a3 != 0 )
  {
    v17 = 0;
    while ( 1 )
    {
      v18 = a2[v17 + 4108];
      if ( W_pslot_ok((intptr_t)(v18), (intptr_t)(1), (intptr_t)(1)) && a3 == F32(v18, 2544) )
        break;
      if ( ++v17 >= 8u )
        goto LABEL_11;
    }
    v19 = F32(v18, 2560);
  }
  else
  {
LABEL_11:
    v19 = a2[4117];
  }
  W_tm_419900((intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(v19));
  while ( 1 )
  {
    v20 = F32(16 * F32((F32((*v23__f), 0x174) + 20), 0) + F32((*v23__f), 0x17c), 4);
    if ( v20 > 2 )
    {
      if ( v20 != 255 )
        goto LABEL_24;
      v21 = ((char *)D(0x8a3bf0));
    }
    else if ( v20 == 2 )
    {
      v21 = ((char *)D(0x8a3c40));
    }
    else if ( v20 != 0 )
    {
      if ( v20 != 1 )
        goto LABEL_24;
      v21 = ((char *)D(0x8a3d1c));
    }
    else
    {
      v21 = ((char *)D(0x8a3c94));
    }
    strcpy(v26, v21);
LABEL_24:
    W_menu_msg_set((intptr_t)(c), (intptr_t)(v26));
    result = W_menu_run((intptr_t)(c), (intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      W_mh_4e6520((intptr_t)(a2), (intptr_t)(F32(16 * F32((F32((*v23__f), 0x174) + 20), 0) + F32((*v23__f), 0x17c), 4)), (intptr_t)(a3));
      *(_DWORD *)((uint8_t *)((*v24__f) + (1) * 0x50) + 0x0) = 0;
      return 0;
    }
    if ( result != 13 )
    {
      *(_DWORD *)((uint8_t *)((*v24__f) + (1) * 0x50) + 0x0) = 0;
      return result;
    }
  }
  *(_DWORD *)((uint8_t *)((*v24__f) + (1) * 0x50) + 0x0) = 0;
  return 0;
}
/* unported callees: none */
