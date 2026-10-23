/* menu_helpers.c — the menus' and item flows' remaining callees: the UI dialogs (the Yes / No box 0x4190d0, the
 * message + choice windows 0x418ec0 / 0x419c70 / 0x419eb0, the party pick 0x4199b0), the ITEM USE 0x5b1f10 (the menu's
 * and the party tick's queued use: heal / revive / status cure / stat items, the floating numbers, the sound) + its
 * target checks 0x5b35f0 / 0x5b3850, a chapter's start gear 0x4e61d0 / 0x4e6520 (+ 0x4e6870), the level-up stat roll
 * 0x409150.  See menu_helpers.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  0x5b1f10 is IDA's __fastcall
 * whose edx is never read: a thiscall + 6 stack arguments.  REPLACE-verified by rh-early-game (2026-10-23):
 * 0x418ec0 0x4199b0 0x419c70 0x419eb0 0x4e6520 (the start gear) 0x5b1f10 (the item use) 0x5b35f0; the rest UNVERIFIED
 * (unreached; REPLACE-wired, compile-checked). */
#include "menu_helpers.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "arche_pose.h"
#include "behave_helpers.h"
#include "char_handlers.h"
#include "char_hit.h"
#include "char_state.h"
#include "char_status.h"
#include "party_member.h"
#include "room_load.h"
#include "room_loop.h"
#include "script_helpers.h"
#include "sound_fx.h"
#include "ui_core.h"
#include "ui_menu.h"
#include "ui_run.h"
#include "ui_text.h"
#include "arche_pose.h"
#include "behave_helpers.h"
#include "char_handlers.h"
#include "char_hit.h"
#include "char_state.h"
#include "char_status.h"
#include "party_member.h"
#include "room_load.h"
#include "room_loop.h"
#include "script_helpers.h"
#include "sound_fx.h"
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

static inline intptr_t W_beh_47ae00(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)beh_47ae00((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t *)a7, (int32_t *)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t *)a12); }
static inline intptr_t W_char_add_hp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_add_hp((wl_char *)a0, (int32_t)a1, (int32_t *)a2, (int32_t)a3); }
static inline intptr_t W_char_add_mp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)char_add_mp((wl_char *)a0, (int32_t)a1, (int32_t *)a2, (int32_t)a3); }
static inline intptr_t W_char_follow_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { char_follow_set((wl_char *)a0, (int32_t)a1, (wl_char *)a2, (int32_t)a3, (int32_t)a4); return 0; }
static inline intptr_t W_char_skills_refresh(intptr_t a0, intptr_t a1, ...) { return (intptr_t)char_skills_refresh((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_char_status_decay(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { char_status_decay((wl_char *)a0, (uint16_t)a1, (uint16_t)a2, (int32_t)a3, (uint32_t)a4, (char *)a5); return 0; }
static inline intptr_t W_char_status_inflict(intptr_t a0, intptr_t a1, intptr_t a2, ...) { char_status_inflict((wl_char *)a0, (uint16_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_ctrl_cursor_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)ctrl_cursor_set((uint8_t *)a0, (int32_t)a1, (uint16_t)a2, (const uint16_t *)a3, (uint16_t)a4, (uint16_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_ctrl_grid_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)ctrl_grid_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_ctrl_text_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)ctrl_text_init((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (uint16_t)a4); }
static inline intptr_t W_ctrl_text_set(intptr_t a0, intptr_t a1, ...) { return (intptr_t)ctrl_text_set((uint8_t *)a0, (const char *)a1); }
static inline intptr_t W_float_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, ...) { float_text((wl_char *)a0, (const char *)a1, (int32_t)a2, (wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (uint16_t)a8, (int32_t)a9, (uint16_t)a10, (int32_t)a11); return 0; }
static inline intptr_t W_grid_cell_text(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { grid_cell_text((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (const char *)a3); return 0; }
static inline intptr_t W_grid_page_fit(intptr_t a0, ...) { return (intptr_t)grid_page_fit((uint8_t *)a0); }
static inline intptr_t W_grid_row_add(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)grid_row_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_grid_row_reset(intptr_t a0, intptr_t a1, ...) { return (intptr_t)grid_row_reset((uint8_t *)a0, (int32_t)a1); }
static inline intptr_t W_ih_5b28a0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)ih_5b28a0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (uint32_t)a7, (int32_t)a8, (char *)a9); }
static inline intptr_t W_item_stats_add(intptr_t a0, intptr_t a1, ...) { item_stats_add((const uint8_t *)a0, (uint8_t *)a1); return 0; }
static inline intptr_t W_key_token_replace(intptr_t a0, intptr_t a1, ...) { return (intptr_t)key_token_replace((char *)a0, (const uint8_t *)a1); }
static inline intptr_t W_member_palette(intptr_t a0, ...) { member_palette((wl_char *)a0); return 0; }
static inline intptr_t W_menu_run(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)menu_run((uint8_t *)a0, (uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_msg_window_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)msg_window_open((uint8_t *)a0, (const char *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (uint16_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_pslot_by_id(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_by_id((wl_room *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_sc_406c70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)sc_406c70((int32_t)a0, (int32_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4); }
static inline intptr_t W_sfx_play_at(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sfx_play_at((wl_entity *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_sys_box_cursor(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)sys_box_cursor((void *)a0, (const uint8_t *)a1, (uint8_t *)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_sys_cursor_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_cursor_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_sys_list_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)sys_list_open((void *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (uint16_t)a7, (uint16_t)a8, (uint8_t **)a9, (uint8_t **)a10, (const char *)a11, (int32_t)a12); }
static inline intptr_t W_sys_msg_keys(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_keys((void *)a0, (int32_t)a1, (uint8_t *)a2); }
static inline intptr_t W_sys_msg_style(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sys_msg_style((void *)a0, (uint8_t *)a1, (int32_t)a2); }
static inline intptr_t W_text_layout_set(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)text_layout_set((uint8_t *)a0, (const char *)a1, (uint16_t)a2); }
static inline intptr_t W_text_metrics_set(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)text_metrics_set((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_ctrl_take(intptr_t a0, ...) { return (intptr_t)win_ctrl_take((uint8_t *)a0); }
static inline intptr_t W_win_grid_add(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)win_grid_add((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_win_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)win_init((uint8_t *)a0, (uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (uint16_t)a6, (const void *)a7); }
static inline intptr_t W_win_mgr_open(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)win_mgr_open((uint8_t *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint16_t)a5, (const void *)a6); }
static inline intptr_t W_win_mgr_topmost(intptr_t a0, ...) { win_mgr_topmost((uint8_t *)a0); return 0; }

int32_t mh_418ec0(wl_char *self, int32_t a2);
int32_t mh_4190d0(wl_char *self, char * a2, uint16_t a3, int32_t * a4, int32_t a5);
int32_t mh_4199b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t mh_419c70(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7, int32_t * a8, int32_t * a9, char * a10, int32_t a11);
int32_t mh_419eb0(wl_char *self, char * a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7, int32_t a8, int32_t * a9, int32_t a10);
int32_t mh_4e61d0(wl_char *self, int32_t a2, int32_t a3);
int32_t mh_4e6520(wl_char *self, int32_t a2, int32_t a3);
int32_t mh_4e6870(wl_char *self, uint16_t a2, int32_t a3, int32_t a4);
int32_t mh_5b1f10(wl_char *self, int32_t a3, int32_t a4, char * Buffer, uint16_t * a6, uint32_t a7, int32_t a8);
int32_t mh_5b35f0(wl_char *self, int32_t a2);
int32_t mh_5b3850(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t mh_409150(wl_char *self, int32_t * a2);

/* 0x418ec0: int __thiscall sub_418EC0(cls_412590 *this, int a2) */
int32_t mh_418ec0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    char * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    _Alignas(8) uint8_t frame_[0x430 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v17 = (char *)(frame_ + 0xc);   /* [36] at ebp-0x424 */
    char *v18 = (char *)(frame_ + 0x30);   /* [512] at ebp-0x400 */
    char *v19 = (char *)(frame_ + 0x230);   /* [512] at ebp-0x200 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x430 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x42c */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x428 */

  result = 0;
  v4 = F32(a2, 2548);
  (*v16__f) = c;
  if ( v4 != 0 && F32(v4, 496) != 0 )
  {
    strcpy(v18, ((char *)D(0x854cb8)));
    (*v15__f) = 0;
    (*v14__f) = 0;
    W_sys_msg_style((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)((int)v17), (intptr_t)(0));
    v5 = W_win_mgr_open((intptr_t)(F32(c, 0x0)), (intptr_t)(140), (intptr_t)(174), (intptr_t)(360), (intptr_t)(132), (intptr_t)(1u), (intptr_t)(v17));
    v6 = W_win_ctrl_take((intptr_t)(v5));
    v7 = v6;
    if ( v6 != 0 )
      W_ctrl_text_init((intptr_t)(v6), (intptr_t)(40), (intptr_t)(24), (intptr_t)(40), (intptr_t)(3));
    W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v5), (intptr_t)(v7));
    W_sys_box_cursor((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(v5), (intptr_t)((int)v7), (intptr_t)(0), (intptr_t)(0));
    W_text_metrics_set((intptr_t)(v7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0));
    W_ctrl_text_set((intptr_t)(v7), (intptr_t)(v18));
    do
    {
      while ( 1 )
      {
        v8 = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(v5), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        if ( v8 != 12 )
          break;
        v9 = ++(*v14__f);
        if ( (uint16_t)(*v14__f) != 0 )
          goto LABEL_17;
        v10 = F32((F32(v5, 72) + 4 * F16(v5, 78) - 4), 0);
        v11 = &v18[512 * v9];
        if ( F32(v10, 356) != 0 )
        {
          strcpy(v19, v11);
          v12 = 0;
          if ( F32(v10, 364) != 0 )
          {
            v13 = 0;
            do
            {
              W_key_token_replace((intptr_t)(v19), (intptr_t)(F32(v10, 356) + 100 * v13));
              v13 = (uint16_t)++v12;
            }
            while ( (unsigned int)(uint16_t)v12 < F32(v10, 364) );
          }
          v11 = v19;
        }
        W_text_layout_set((intptr_t)(**(intptr_t **)(v10 + 368)), (intptr_t)(v11), (intptr_t)(F16(F32(v10, 368), 8)));
        F16(F32(v10, 368), 4) = 0;
        F16(F32(v10, 368), 6) = 0;
        F32(v10, 28) = 0;
        c = (*v16__f);
      }
    }
    while ( v8 == 13 );
    (*v15__f) = v8;
LABEL_17:
    result = (*v15__f);
    F32(v5, 80) = 0;
    if ( (*v15__f) == 0 )
      return 7;
  }
  return result;
}

/* 0x4190d0: int __thiscall sub_4190D0(cls_412590 *this, const char *a2, unsigned __int16 a3, _DWORD *a4, int a5) */
int32_t mh_4190d0(wl_char *self, char * a2, uint16_t a3, int32_t * a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    int32_t * v6 = 0;
    intptr_t result = 0;
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
    int32_t * v18 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  (*v20__f) = 0;
  v6 = (_DWORD *)W_msg_window_open((intptr_t)(c), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a3), (intptr_t)((uint16_t)a4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  while ( 1 )
  {
    result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)((int)v6), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 12 )
      break;
    if ( result != 13 )
    {
      if ( result != 0 )
        goto LABEL_22;
      break;
    }
  }
  W_sys_list_open((intptr_t)((int **)((intptr_t)g_eng.sys)), (intptr_t)(F32(c, 0x0)), (intptr_t)(300), (intptr_t)(v6[6] + v6[4] - 16), (intptr_t)(128), (intptr_t)(0), (intptr_t)(1u), (intptr_t)(2u), (intptr_t)(1u), (intptr_t)(&(*v20__f)), (intptr_t)((int *)&a4), (intptr_t)(0), (intptr_t)(0));
  v8 = a4;
  v9 = W_grid_row_add((intptr_t)(a4), (intptr_t)(0), (intptr_t)(14));
  W_grid_cell_text((intptr_t)(v8), (intptr_t)(v9), (intptr_t)(0), (intptr_t)(D(0x854d04)));
  v10 = a4;
  v11 = a4[93];
  v12 = F32(v11, 16);
  if ( v12 < F32(v11, 4) )
  {
    (*v19__f) = F32(v11, 16);
    F32(v11, 16) = v12 + 1;
    v13 = 16 * v12;
    F32((F32(v10, 0x17c) + v13), 0) = 0;
    F32(F32(v10, 0x17c) + v13, 4) = 15;
    F32(F32(v10, 0x17c) + v13, 8) = 1;
    W_grid_row_reset((intptr_t)(v10), (intptr_t)((*v19__f)));
  }
  else
  {
    v12 = -1;
  }
  W_grid_cell_text((intptr_t)(v10), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(((char *)D(0x854d00))));
  if ( a5 != 0 )
  {
    v14 = 0;
    v21 = a4;
    v15 = a4[93];
    v16 = F32(v15, 16);
    if ( v16 > 0 )
    {
      v17 = 0;
      do
      {
        v18 = (_DWORD *)(a4[95] + 16 * v17);
        if ( *v18 == 0 && v18[1] == 15 )
        {
          F32(v15, 20) = v14;
          W_grid_page_fit((intptr_t)(v21));
          break;
        }
        v17 = ++v14;
      }
      while ( v14 < v16 );
    }
  }
  while ( 1 )
  {
    result = W_menu_run((intptr_t)(c), (intptr_t)(a4), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 11 )
      break;
    if ( result == 12 )
    {
      result = F32(16 * F32((a4[93] + 20), 0) + a4[95], 4);
      goto LABEL_22;
    }
    if ( result != 13 )
      goto LABEL_22;
  }
  result = 15;
LABEL_22:
  if ( v6 != 0 )
    v6[20] = 0;
  if ( (*v20__f) != 0 )
    *(_DWORD *)((uint8_t *)((*v20__f) + (1) * 0x50) + 0x0) = 0;
  return result;
}

/* 0x4199b0: int __thiscall sub_4199B0(cls_413B20 *this, int a2, int a3, int a4) */
int32_t mh_4199b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    uint16_t v5 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    char * v9 = 0;
    char * v10 = 0;
    char * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    int32_t * v14 = 0;
    uint16_t i = 0;
    char * v16 = 0;
    int32_t * v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int v23 = 0;
    char * v25 = 0;
    intptr_t v26 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v5 = F16(c, 0x4010);
  if ( (uint16_t)a2 >= v5 )
    return 0;
  v7 = a2;
  v8 = (uint16_t)a2;
  if ( (uint16_t)a2 < v5 - 1 )
  {
    do
    {
      v9 = &GAP(c, 0x0, 16 * v8 + 8224);
      v10 = &GAP(c, 0x0, 16 * v8 + 8208);
      ++v7;
      F32(v10, 0) = F32(v9, 0);
      F32(v10, 4 * 1) = F32(v9, 4 * 1);
      F32(v10, 4 * 2) = F32(v9, 4 * 2);
      F32(v10, 4 * 3) = F32(v9, 4 * 3);
      v8 = (uint16_t)v7;
    }
    while ( (uint16_t)v7 < F16(c, 0x4010) - 1 );
  }
  --F16(c, 0x4010);
  F16(c, 0x4070) = 0;
  if ( a3 != 0 )
  {
    v11 = &GAP(c, 0x4012, 30);
    v25 = &GAP(c, 0x4012, 30);
    v26 = 8;
    do
    {
      v12 = F32(v11, 0);
      if ( F32(F32(v11, 0), 2500) == 1 || F32(F32(v11, 0), 2500) == 2 )
      {
        v13 = v12 + 2576;
        (*v24__f) = F32(v12, 2544);
        v14 = (_DWORD *)W_pslot_by_id((intptr_t)(c), (intptr_t)((*v24__f)), (intptr_t)(0));
        if ( v14 != 0 )
        {
          F32(v13, 100) = v14[669];
          F32(v13, 104) = v14[670];
          F32(v13, 108) = v14[671];
          F32(v13, 112) = v14[672];
          F32(v13, 140) = v14[679];
          F32(v13, 144) = v14[680];
          F32(v13, 148) = v14[681];
          F32(v13, 152) = v14[682];
          F32(v13, 116) = 0;
          F32(v13, 120) = 0;
          F32(v13, 124) = 0;
          F32(v13, 128) = 0;
          F32(v13, 132) = 0;
          F32(v13, 136) = 0;
          F16(v13, 288) = 0;
          F32(v13, 292) = 0;
          F32(v13, 296) = 0;
          F32(v13, 300) = 0;
          F32(v13, 304) = 0;
          F32(v13, 308) = 0;
          memset32((void *)(v13 + 164), 1000, 0xCu);
          for ( i = 0; i < F16(c, 0x4010); ++i )
          {
            v16 = &GAP(c, 0x0, 16 * i);
            if ( F32(v16, 4 * 2053) == (*v24__f) )
            {
              v17 = F32(v16, 4 * (2055));
              if ( v17[30] != 0 )
                W_item_stats_add((intptr_t)(v17), (intptr_t)(v13));
            }
          }
          v18 = F32(v13, 132) + F32(v13, 88) + F32(v13, 156);
          if ( v18 >= F32(v13, 84) )
            v18 = F32(v13, 84);
          v19 = F32(v13, 160);
          v20 = F32(v13, 96);
          v21 = F32(v13, 92);
          F32(v13, 84) = v18;
          v22 = v20 + v19 + F32(v13, 136);
          if ( v22 >= v21 )
            v22 = v21;
          v11 = v25;
          F32(v13, 92) = v22;
        }
      }
      if ( a4 != 0 && F32(F32(v11, 0), 2500) == 1 && F32(F32(v11, 0), 2548) != 0 )
        W_member_palette((intptr_t)(F32(F32(v11, 0), 2548)));
      v11 += 4;
      v23 = v26 == 1;
      v25 = v11;
      --v26;
    }
    while ( !v23 );
  }
  return 1;
}

/* 0x419c70: int *__thiscall sub_419C70( cls_412590 *this, int a2, int a3, int a4, int a5, unsigned __int16 a6, unsigned __int16 a7, int *a8, int *a9, const char *a10, int a11) */
int32_t mh_419c70(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7, int32_t * a8, int32_t * a9, char * a10, int32_t a11)
{
    intptr_t c = (intptr_t)self;
    intptr_t v11 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v38 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24 */
    int16_t *const v25__f = (int16_t *)(frame_ + 0x4);   /* ebp-0x20 */
    int16_t *const v26__f = (int16_t *)(frame_ + 0x6);   /* ebp-0x1e */
    int16_t *const v27__f = (int16_t *)(frame_ + 0x8);   /* ebp-0x1c */
    int16_t *const v28__f = (int16_t *)(frame_ + 0xa);   /* ebp-0x1a */
    int16_t *const v29__f = (int16_t *)(frame_ + 0xc);   /* ebp-0x18 */
    int16_t *const v30__f = (int16_t *)(frame_ + 0xe);   /* ebp-0x16 */
    int16_t *const v31__f = (int16_t *)(frame_ + 0x10);   /* ebp-0x14 */
    int16_t *const v32__f = (int16_t *)(frame_ + 0x12);   /* ebp-0x12 */
    int16_t *const v33__f = (int16_t *)(frame_ + 0x14);   /* ebp-0x10 */
    int16_t *const v34__f = (int16_t *)(frame_ + 0x16);   /* ebp-0xe */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xc */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x8 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x4 */

  v11 = F32(c, 0x0);
  v13 = 0;
  v14 = a6;
  v15 = a5;
  v38 = 24;
  if ( a5 == 0 )
  {
    v15 = 28 * a7 + 40;
    if ( a10 == 0 )
      goto LABEL_6;
    v15 = 28 * a7 + 72;
  }
  if ( a10 != 0 )
  {
    v38 = 56;
    v13 = 1;
    v14 = a6 + 1;
  }
LABEL_6:
  (*v35__f) = 32;
  (*v36__f) = 32;
  if ( a11 != 0 )
  {
    (*v25__f) = 0;
    if ( v13 != 0 )
    {
      (*v26__f) = 9;
      (*v27__f) = 10;
      (*v28__f) = 11;
    }
    else
    {
      (*v26__f) = 0;
      (*v27__f) = 1;
      (*v28__f) = 2;
    }
  }
  else
  {
    (*v25__f) = 16;
    if ( v13 != 0 )
    {
      (*v26__f) = 9;
      (*v27__f) = 10;
      (*v28__f) = 11;
    }
    else
    {
      (*v26__f) = 0;
      (*v27__f) = 1;
      (*v28__f) = 2;
    }
  }
  (*v29__f) = 3;
  (*v30__f) = 4;
  (*v31__f) = 5;
  (*v32__f) = 6;
  (*v33__f) = 7;
  (*v34__f) = 8;
  (*v24__f) = SYS32(2952);
  v16 = F16(v11, 6);
  (*v37__f) = 0;
  if ( v16 < F16(v11, 4) )
  {
    v18 = 4 * v16;
    W_win_init((intptr_t)(F32((v18 + F32(v11, 0)), 0)), (intptr_t)(v11), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(v15), (intptr_t)(v14), (intptr_t)(&(*v24__f)));
    ++F16(v11, 6);
    W_win_mgr_topmost((intptr_t)(v11));
    v17 = F32((v18 + F32(v11, 0)), 0);
  }
  else
  {
    v17 = 0;
  }
  if ( a10 != 0 )
  {
    v19 = W_win_grid_add((intptr_t)(v17), (intptr_t)(32), (intptr_t)(7), (intptr_t)(0), (intptr_t)(0));
    v20 = v19;
    if ( v19 != 0 )
    {
      W_grid_cell_text((intptr_t)(v19), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a10));
      *(_DWORD *)((uint8_t *)(v20 + (1) * 0x180) + 0x0) = 0xFFFFFF;
      *(_DWORD *)&GAP((v20 + (1) * 0x180), 0x0, 4) = 4546427;
    }
  }
  v21 = W_win_ctrl_take((intptr_t)(v17));
  v22 = (int)v21;
  if ( v21 != 0 )
    W_ctrl_grid_init((intptr_t)(v21), (intptr_t)(40), (intptr_t)(v38), (intptr_t)(a7), (intptr_t)(1), (intptr_t)(a7), (intptr_t)(0));
  else
    v22 = 0;
  F32(F32(v22, 376), 0) = 0;
  F32(F32((v22 + 376), 0), 4) = 0;
  F32(F32((v22 + 376), 0), 8) = 0;
  F32(F32((v22 + 376), 0), 12) = 0;
  W_sys_cursor_style((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v22), (intptr_t)(1));
  *a8 = (int)v17;
  *a9 = v22;
  return a9;
}

/* 0x419eb0: int __thiscall sub_419EB0( cls_412590 *this, const char *a2, int a3, int a4, int a5, unsigned __int16 a6, unsigned __int16 a7, int a8, int *a9, int a10) */
int32_t mh_419eb0(wl_char *self, char * a2, int32_t a3, int32_t a4, int32_t a5, uint16_t a6, uint16_t a7, int32_t a8, int32_t * a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x48 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v22 = (int16_t *)(frame_ + 0x1c);   /* [4] at ebp-0x2c */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x48 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x44 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x24 */
    int16_t *const v24__f = (int16_t *)(frame_ + 0x28);   /* ebp-0x20 */
    int16_t *const v25__f = (int16_t *)(frame_ + 0x2a);   /* ebp-0x1e */
    int16_t *const v26__f = (int16_t *)(frame_ + 0x2c);   /* ebp-0x1c */
    int16_t *const v27__f = (int16_t *)(frame_ + 0x2e);   /* ebp-0x1a */
    int16_t *const v28__f = (int16_t *)(frame_ + 0x30);   /* ebp-0x18 */
    int16_t *const v29__f = (int16_t *)(frame_ + 0x32);   /* ebp-0x16 */
    int16_t *const v30__f = (int16_t *)(frame_ + 0x34);   /* ebp-0x14 */
    int16_t *const v31__f = (int16_t *)(frame_ + 0x36);   /* ebp-0x12 */
    int16_t *const v32__f = (int16_t *)(frame_ + 0x38);   /* ebp-0x10 */
    int16_t *const v33__f = (int16_t *)(frame_ + 0x3a);   /* ebp-0xe */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0xc */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x8 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x4 */

  v12 = 0;
  if ( a5 != 0 )
  {
    a3 = 320 - (7 * a6 + 80) / 2;
    a4 = 240 - (28 * a7 + 48) / 2;
  }
  (*v34__f) = 32;
  (*v35__f) = 32;
  if ( a8 != 0 )
  {
    v13 = SYS32(2984);
    (*v24__f) = 0;
  }
  else
  {
    v13 = SYS32(2952);
    (*v24__f) = 16;
  }
  (*v25__f) = 0;
  (*v26__f) = 1;
  (*v27__f) = 2;
  (*v28__f) = 3;
  (*v29__f) = 4;
  (*v30__f) = 5;
  (*v31__f) = 6;
  (*v32__f) = 7;
  (*v33__f) = 8;
  v14 = F32(c, 0x0);
  (*v23__f) = v13;
  (*v36__f) = 0;
  if ( F16(v14, 6) < F16(v14, 4) )
  {
    v15 = 4 * F16(v14, 6);
    W_win_init((intptr_t)(F32(F32(v14, 0), v15)), (intptr_t)(v14), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(7 * a6 + 80), (intptr_t)(28 * a7 + 48), (intptr_t)(1u), (intptr_t)(&(*v23__f)));
    ++F16(v14, 6);
    W_win_mgr_topmost((intptr_t)(v14));
    v12 = F32((F32(v14, 0) + v15), 0);
  }
  v16 = W_win_ctrl_take((intptr_t)(v12));
  if ( v16 != 0 )
    W_ctrl_text_init((intptr_t)(v16), (intptr_t)(40), (intptr_t)(24), (intptr_t)(a6), (intptr_t)(a7));
  else
    v16 = 0;
  W_sys_msg_keys((intptr_t)((int)((intptr_t)g_eng.sys)), (intptr_t)(v12), (intptr_t)(v16));
  (*v20__f) = F32(v12, 24) - F32(v16, 0x10) - 32;
  (*v19__f) = F32(v12, 20) - F32(v16, 0xc) - 32;
  v22[0] = 0;
  v22[1] = 1;
  v22[2] = 2;
  v22[3] = 3;
  W_ctrl_cursor_set((intptr_t)(v16), (intptr_t)(SYS32(2956)), (intptr_t)(20), (intptr_t)(v22), (intptr_t)(4u), (intptr_t)(10), (intptr_t)((*v19__f)), (intptr_t)((*v20__f)));
  if ( a10 != 0 )
  {
    v17 = F32(CFG(), 584);
    F32(F32(v16, 0x170), 12) = 1;
    F32(F32(v16, 0x170), 16) = v17;
    F32(F32(v16, 0x170), 32) = 2 * v17;
    F32(F32(v16, 0x170), 36) = 3 * v17;
    F32(F32(v16, 0x170), 40) = 5 * v17;
    F32(F32(v16, 0x170), 20) = 0;
    F32(F32(v16, 0x170), 24) = 0;
    F32(v16, 0x1c) = 0;
  }
  else
  {
    W_text_metrics_set((intptr_t)(v16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
  }
  W_ctrl_text_set((intptr_t)(v16), (intptr_t)(a2));
  *a9 = v12;
  while ( 1 )
  {
    result = W_menu_run((intptr_t)(c), (intptr_t)(0), (intptr_t)(v12), (intptr_t)(1), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
    if ( result == 12 )
      break;
    if ( result != 13 )
      return result;
  }
  return 0;
}

/* 0x4e61d0: int __thiscall sub_4E61D0(cls_4E61D0 *this, int a2, int a3) */
int32_t mh_4e61d0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    uint16_t v5 = 0;
    int32_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
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
    intptr_t result = 0;
    int v21 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v26 = (uint16_t *)(frame_ + 0x20);   /* [2] at ebp-0xc */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    int16_t *const v24__f = (int16_t *)(frame_ + 0x18);   /* ebp-0x14 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x10 */
    char * *const v27__f = (char * *)(frame_ + 0x24);   /* ebp-0x8 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4 */

  v3 = 0;
  if ( a3 != 0 )
  {
    v5 = 0;
    while ( 1 )
    {
      v6 = *(_DWORD **)&GAP(c, 0x2010, 4 * v5 + 8224);
      if ( (v6[625] == 1 || v6[625] == 2) && a3 == v6[636] )
        break;
      if ( ++v5 >= 8u )
        goto LABEL_10;
    }
    v6[640] = a2;
  }
  else
  {
    F32(c, 0x4054) = a2;
  }
LABEL_10:
  v7 = F32(c, 0x200c);
  (*v25__f) = 0;
  (*v28__f) = v7;
  if ( v7 != 0 )
    (*v25__f) = F32(v7, 2552);
  F32(v26, 0) = 0;
  (*v27__f) = &GAP(c, 0x2010, 8224);
  do
  {
    v8 = F32((*v27__f), 0);
    if ( F32(F32((*v27__f), 0), 2500) == 1 && F32(v8, 2548) != 0 )
    {
      v9 = F32(v8, 2544);
      if ( v9 != 0 )
      {
        while ( 1 )
        {
          v10 = *(_DWORD *)&GAP(c, 0x2010, 4 * v3 + 8224);
          if ( W_pslot_ok((intptr_t)(v10), (intptr_t)(1), (intptr_t)(1)) && v9 == F32(v10, 2544) )
            break;
          if ( ++v3 >= 8u )
          {
            v3 = 0;
            goto LABEL_20;
          }
        }
        v11 = F32(v10, 2560);
        if ( v11 == 255 )
          v11 = F32(c, 0x4054);
        v3 = 0;
      }
      else
      {
LABEL_20:
        v11 = F32(c, 0x4054);
      }
      switch ( v11 )
      {
        case 0:
          SETLO(a2, 0);
          SETLO(a3, 0);
          (*v24__f) = 0;
          break;
        case 1:
          SETLO(a2, 0);
          SETLO(a3, 2);
          (*v24__f) = 0;
          break;
        case 2:
          SETLO(a2, 1);
          SETLO(a3, 2);
          (*v24__f) = 0;
          break;
        case 3:
          (*v24__f) = 1;
          SETLO(a2, 2);
          SETLO(a3, 2);
          break;
        case 4:
          SETLO(a2, 4);
          SETLO(a3, 2);
          (*v24__f) = 3;
          break;
        default:
          break;
      }
      F16(F32(v8, 2548), 84008) = a2;
      F16(F32(v8, 2548), 84010) = a3;
      F16(F32(v8, 2548), 84012) = (*v24__f);
      if ( v8 == (*v28__f) || (*v28__f) == 0 )
      {
        v18 = F32(v8, 2548);
        v19 = F32(v18, 64);
        F32(v18, 82496) = 2;
        F32(v18, 82508) = 0;
        F32(v18, 82512) = 0;
        F32(v18, 82516) = 0;
        F32(v18, 82500) = F32(v19, 44);
        F16(v18, 82504) = 0;
        F32(v18, 82520) = 0;
      }
      else
      {
        v12 = F32(v8, 2544);
        if ( v12 == 0 )
          goto LABEL_39;
        v13 = 0;
        while ( 1 )
        {
          v14 = *(_DWORD *)&GAP(c, 0x2010, 4 * v13 + 8224);
          if ( W_pslot_ok((intptr_t)(v14), (intptr_t)(1), (intptr_t)(1)) && v12 == F32(v14, 2544) )
            break;
          if ( ++v13 >= 8u )
            goto LABEL_39;
        }
        v15 = F32(v14, 2556);
        if ( v15 == 255 )
LABEL_39:
          v15 = F32(c, 0x4050);
        v16 = 4800 * mh_4e6870(c, v26[0], v8, (*v28__f));
        if ( F32(v8, 2544) == 100000104 && v15 == 2 )
          v15 = 0;
        if ( v15 == 1 )
        {
          W_char_follow_set((intptr_t)(F32(v8, 2548)), (intptr_t)(1), (intptr_t)(F32((*v28__f), 2548)), (intptr_t)(0), (intptr_t)(v16));
          v3 = 0;
        }
        else
        {
          (*v22__f) = v16;
          v17 = F32(v8, 2552);
          if ( v15 == 2 )
            W_char_follow_set((intptr_t)(F32(v8, 2548)), (intptr_t)(4), (intptr_t)(F32((*v28__f), 2548)), (intptr_t)(4000 * (v17 - (*v25__f))), (intptr_t)((*v22__f)));
          else
            W_char_follow_set((intptr_t)(F32(v8, 2548)), (intptr_t)(3), (intptr_t)(F32((*v28__f), 2548)), (intptr_t)(4000 * (v17 - (*v25__f))), (intptr_t)((*v22__f)));
          v3 = 0;
        }
      }
    }
    result = F32(v26, 0) + 1;
    v21 = (uint16_t)(v26[0] + 1) < 8u;
    ++F32(v26, 0);
    (*v27__f) += 4;
  }
  while ( v21 );
  return result;
}

/* 0x4e6520: int __thiscall sub_4E6520(cls_4E6520 *this, int a2, int a3) */
int32_t mh_4e6520(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    uint16_t v5 = 0;
    int32_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
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
    intptr_t result = 0;
    int v21 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v26 = (uint16_t *)(frame_ + 0x20);   /* [2] at ebp-0xc */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    int16_t *const v24__f = (int16_t *)(frame_ + 0x18);   /* ebp-0x14 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x10 */
    char * *const v27__f = (char * *)(frame_ + 0x24);   /* ebp-0x8 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4 */

  v3 = 0;
  if ( a3 != 0 )
  {
    v5 = 0;
    while ( 1 )
    {
      v6 = *(_DWORD **)&GAP(c, 0x2010, 4 * v5 + 8224);
      if ( (v6[625] == 1 || v6[625] == 2) && a3 == v6[636] )
        break;
      if ( ++v5 >= 8u )
        goto LABEL_10;
    }
    v6[639] = a2;
  }
  else
  {
    F32(c, 0x4050) = a2;
  }
LABEL_10:
  v7 = F32(c, 0x200c);
  (*v25__f) = 0;
  (*v28__f) = v7;
  if ( v7 != 0 )
    (*v25__f) = F32(v7, 2552);
  F32(v26, 0) = 0;
  (*v27__f) = &GAP(c, 0x2010, 8224);
  do
  {
    v8 = F32((*v27__f), 0);
    if ( F32(F32((*v27__f), 0), 2500) == 1 && F32(v8, 2548) != 0 )
    {
      v9 = F32(v8, 2544);
      if ( v9 != 0 )
      {
        while ( 1 )
        {
          v10 = *(_DWORD *)&GAP(c, 0x2010, 4 * v3 + 8224);
          if ( W_pslot_ok((intptr_t)(v10), (intptr_t)(1), (intptr_t)(1)) && v9 == F32(v10, 2544) )
            break;
          if ( ++v3 >= 8u )
          {
            v3 = 0;
            goto LABEL_20;
          }
        }
        v11 = F32(v10, 2560);
        if ( v11 == 255 )
          v11 = F32(c, 0x4054);
        v3 = 0;
      }
      else
      {
LABEL_20:
        v11 = F32(c, 0x4054);
      }
      switch ( v11 )
      {
        case 0:
          SETLO(a2, 0);
          SETLO(a3, 0);
          (*v24__f) = 0;
          break;
        case 1:
          SETLO(a2, 0);
          SETLO(a3, 2);
          (*v24__f) = 0;
          break;
        case 2:
          SETLO(a2, 1);
          SETLO(a3, 2);
          (*v24__f) = 0;
          break;
        case 3:
          (*v24__f) = 1;
          SETLO(a2, 2);
          SETLO(a3, 2);
          break;
        case 4:
          SETLO(a2, 4);
          SETLO(a3, 2);
          (*v24__f) = 3;
          break;
        default:
          break;
      }
      F16(F32(v8, 2548), 84008) = a2;
      F16(F32(v8, 2548), 84010) = a3;
      F16(F32(v8, 2548), 84012) = (*v24__f);
      if ( v8 == (*v28__f) || (*v28__f) == 0 )
      {
        v18 = F32(v8, 2548);
        v19 = F32(v18, 64);
        F32(v18, 82496) = 2;
        F32(v18, 82508) = 0;
        F32(v18, 82512) = 0;
        F32(v18, 82516) = 0;
        F32(v18, 82500) = F32(v19, 44);
        F16(v18, 82504) = 0;
        F32(v18, 82520) = 0;
      }
      else
      {
        v12 = F32(v8, 2544);
        if ( v12 == 0 )
          goto LABEL_39;
        v13 = 0;
        while ( 1 )
        {
          v14 = *(_DWORD *)&GAP(c, 0x2010, 4 * v13 + 8224);
          if ( W_pslot_ok((intptr_t)(v14), (intptr_t)(1), (intptr_t)(1)) && v12 == F32(v14, 2544) )
            break;
          if ( ++v13 >= 8u )
            goto LABEL_39;
        }
        v15 = F32(v14, 2556);
        if ( v15 == 255 )
LABEL_39:
          v15 = F32(c, 0x4050);
        v16 = 4800 * mh_4e6870(c, v26[0], v8, (*v28__f));
        if ( F32(v8, 2544) == 100000104 && v15 == 2 )
          v15 = 0;
        if ( v15 == 1 )
        {
          W_char_follow_set((intptr_t)(F32(v8, 2548)), (intptr_t)(1), (intptr_t)(F32((*v28__f), 2548)), (intptr_t)(0), (intptr_t)(v16));
          v3 = 0;
        }
        else
        {
          (*v22__f) = v16;
          v17 = F32(v8, 2552);
          if ( v15 == 2 )
            W_char_follow_set((intptr_t)(F32(v8, 2548)), (intptr_t)(4), (intptr_t)(F32((*v28__f), 2548)), (intptr_t)(4000 * (v17 - (*v25__f))), (intptr_t)((*v22__f)));
          else
            W_char_follow_set((intptr_t)(F32(v8, 2548)), (intptr_t)(3), (intptr_t)(F32((*v28__f), 2548)), (intptr_t)(4000 * (v17 - (*v25__f))), (intptr_t)((*v22__f)));
          v3 = 0;
        }
      }
    }
    result = F32(v26, 0) + 1;
    v21 = (uint16_t)(v26[0] + 1) < 8u;
    ++F32(v26, 0);
    (*v27__f) += 4;
  }
  while ( v21 );
  return result;
}

/* 0x4e6870: int __thiscall sub_4E6870(cls_4E6870 *this, unsigned __int16 a2, int a3, int a4) */
int32_t mh_4e6870(wl_char *self, uint16_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint16_t v5 = 0;
    char * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;

  result = 0;
  v5 = 0;
  v6 = &GAP(c, 0x2010, 8224);
  do
  {
    v7 = F32(v6, 0);
    if ( F32(v6, 0) != a3 && F32(v7, 2500) == 1 )
    {
      if ( v7 == a4 || (v8 = F32(a3, 2552), v9 = F32(v7, 2552), v8 > v9) || v8 == v9 && a2 > v5 )
        ++result;
    }
    ++v5;
    v6 += 4;
  }
  while ( v5 < 8u );
  return result;
}

/* 0x5b1f10: BOOL __thiscall sub_5B1F10(cls_413B20 *this, int a3, int a4, char *Buffer, _WORD *a6, unsigned int a7, int a8) */
int32_t mh_5b1f10(wl_char *self, int32_t a3, int32_t a4, char * Buffer, uint16_t * a6, uint32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    char * v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    int32_t * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    int v26 = 0;
    intptr_t result = 0;
    int16_t v28 = 0;
    _Alignas(8) uint8_t frame_[0x298 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v41 = (char *)(frame_ + 0x48);   /* [80] at ebp-0x250 */
    char *v42 = (char *)(frame_ + 0x98);   /* [512] at ebp-0x200 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x298 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x298 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x294 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x294 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x270 */
    char * *const v34__f = (char * *)(frame_ + 0x2c);   /* ebp-0x26c */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x268 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x264 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x260 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x25c */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x258 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x254 */

  v42[0] = F8(D(0x8a9b6c), 0);
  memset(&v42[1], 0, 511);
  (*v40__f) = c;
  v8 = *(_DWORD *)&GAP(c, 0x0, 16 * (uint16_t)a4 + 8220);
  (*v38__f) = false;
  v9 = 0;
  v10 = F32(v8, 92) - 1;
  (*v37__f) = 0;
  switch ( v10 )
  {
    case 0:
    case 2:
      if ( a8 == 0 )
      {
        v13 = F32(v8, 112);
        v14 = F32(v8, 108);
        v15 = F32(v8, 104);
        v16 = F32(a3, 2548);
        v17 = F32(v8, 88) - 9;
        (*v33__f) = 0;
        v18 = F32(v16, 64);
        (*v34__f) = v13;
        (*v36__f) = v14;
        (*v35__f) = 0;
        switch ( v17 )
        {
          case 0:
            v20 = rnd();
            W_char_add_hp((intptr_t)(v16), (intptr_t)(v15 + (*v36__f) * v20 / 0x8000), (intptr_t)(&(*v33__f)), (intptr_t)(0));
            if ( a7 != 0 )
            {
              (*v31__f) = v18[11];
              (*v29__f) = v18[2] + v18[4];
              v21 = rnd();
              W_float_text((intptr_t)(v16), (intptr_t)((char *)((char *)D(0x854540))), (intptr_t)((*v33__f)), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(v18[3] + 3200 * v21 / 0x8000 + v18[1] - 1600), (intptr_t)((*v29__f)), (intptr_t)((*v31__f)), (intptr_t)(7), (intptr_t)(9), (intptr_t)(60), (intptr_t)(-48000));
            }
            if ( (*v33__f) <= 0 )
              goto LABEL_70;
            W_sfx_play_at((intptr_t)(v16), (intptr_t)(0), (intptr_t)(85), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            if ( Buffer == 0 )
              goto LABEL_47;
            g_eng.h5bf3ee(Buffer, "%s recovered %d HP.", *(const char **)(v16 + 1872), (*v33__f));
            (*v37__f) = 1;
            (*v38__f) = true;
            mh_5b35f0((*v40__f), a4);
            goto LABEL_72;
          case 1:
            if ( F16(F32(v16, 1872), 1098) != 0 )
            {
              W_char_status_decay((intptr_t)(v16), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a7), (intptr_t)(Buffer));
              (*v35__f) = 1;
            }
            v23 = rnd();
            W_char_add_hp((intptr_t)(v16), (intptr_t)(v15 + (*v36__f) * v23 / 0x8000), (intptr_t)(&(*v33__f)), (intptr_t)(0));
            if ( Buffer != 0 && (*v35__f) == 0 )
              g_eng.h5bf3ee(Buffer, "%s recovered %d HP.", *(const char **)(v16 + 1872), (*v33__f));
            if ( a7 != 0 )
            {
              (*v32__f) = v18[11];
              (*v30__f) = v18[2] + v18[4];
              v24 = rnd();
              W_float_text((intptr_t)(v16), (intptr_t)((char *)((char *)D(0x854540))), (intptr_t)((*v33__f)), (intptr_t)(v18), (intptr_t)(0), (intptr_t)(v18[3] + 3200 * v24 / 0x8000 + v18[1] - 1600), (intptr_t)((*v30__f)), (intptr_t)((*v32__f)), (intptr_t)(7), (intptr_t)(9), (intptr_t)(60), (intptr_t)(-48000));
            }
            if ( (*v33__f) <= 0 )
            {
              if ( (*v35__f) == 0 )
                goto LABEL_70;
            }
            else
            {
              (*v35__f) = 1;
            }
            W_sfx_play_at((intptr_t)(v16), (intptr_t)(0), (intptr_t)(85), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            W_sfx_play_at((intptr_t)(v16), (intptr_t)(0), (intptr_t)(86), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            goto LABEL_46;
          case 2:
            if ( F16(F32(v16, 1872) + 2 * (uint16_t)v15, 1096) == 0 )
              goto LABEL_70;
            W_char_status_decay((intptr_t)(v16), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a7), (intptr_t)(Buffer));
            goto LABEL_54;
          case 3:
            if ( F16(F32(v16, 1872), 1106) != 0 )
            {
              W_char_status_decay((intptr_t)(v16), (intptr_t)(5u), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a7), (intptr_t)(Buffer));
              W_sfx_play_at((intptr_t)(v16), (intptr_t)(0), (intptr_t)(85), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
              (*v35__f) = 1;
            }
            if ( F16(F32(v16, 1872), 1102) != 0 )
            {
              W_char_status_decay((intptr_t)(v16), (intptr_t)(3u), (intptr_t)(0), (intptr_t)(1), (intptr_t)(a7), (intptr_t)(Buffer));
LABEL_54:
              W_sfx_play_at((intptr_t)(v16), (intptr_t)(0), (intptr_t)(85), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            }
            else
            {
LABEL_46:
              if ( (*v35__f) == 0 )
              {
LABEL_70:
                SETLO(v9, (*v37__f));
                goto LABEL_71;
              }
            }
LABEL_47:
            (*v37__f) = 1;
            (*v38__f) = true;
            mh_5b35f0((*v40__f), a4);
            goto LABEL_72;
          case 4:
          case 6:
          case 7:
            W_char_status_inflict((intptr_t)(v16), (intptr_t)(v15), (intptr_t)(v14));
            goto LABEL_47;
          case 9:
            v19 = rnd();
            W_char_add_mp((intptr_t)(v16), (intptr_t)(v15 + (*v36__f) * v19 / 0x8000), (intptr_t)(&(*v33__f)), (intptr_t)(0));
            if ( (*v33__f) <= 0 )
              goto LABEL_70;
            W_sfx_play_at((intptr_t)(v16), (intptr_t)(0), (intptr_t)(85), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
            if ( Buffer == 0 )
              goto LABEL_47;
            g_eng.h5bf3ee(Buffer, "%s recovered %d MP.", *(const char **)(v16 + 1872), (*v33__f));
            (*v37__f) = 1;
            (*v38__f) = true;
            mh_5b35f0((*v40__f), a4);
            goto LABEL_72;
          case 10:
            v22 = (*v36__f) + (int)(*v34__f) * rnd() / 0x8000;
            (*v33__f) = v22;
            switch ( v15 )
            {
              case 1:
                (*v34__f) = strcpy(v41, ((char *)D(0x854c60)));
                F32(F32((v16 + 1872), 0), 140) += v22;
                break;
              case 2:
                (*v34__f) = strcpy(v41, ((char *)D(0x854c58)));
                F32(F32((v16 + 1872), 0), 144) += v22;
                break;
              case 3:
                (*v34__f) = strcpy(v41, ((char *)D(0x854c50)));
                F32(F32((v16 + 1872), 0), 148) += v22;
                break;
              case 4:
                (*v34__f) = strcpy(v41, ((char *)D(0x854c48)));
                F32(F32((v16 + 1872), 0), 152) += v22;
                break;
              case 5:
                (*v34__f) = strcpy(v41, ((char *)D(0x854c40)));
                F32(F32((v16 + 1872), 0), 156) += v22;
                break;
              case 6:
                (*v34__f) = strcpy(v41, ((char *)D(0x854c38)));
                F32(F32((v16 + 1872), 0), 160) += v22;
                break;
              default:
                goto LABEL_70;
            }
            if ( Buffer == 0 )
              goto LABEL_47;
            g_eng.h5bf3ee(Buffer, "%s's %s increased by %d.", *(const char **)(v16 + 1872), v41, (*v33__f));
            (*v37__f) = 1;
            (*v38__f) = true;
            mh_5b35f0((*v40__f), a4);
            goto LABEL_72;
          default:
            goto LABEL_70;
        }
      }
      if ( W_beh_47ae00((intptr_t)(F32(a3, 2548)), (intptr_t)(0), (intptr_t)(F32(v8, 88)), (intptr_t)(F32(v8, 104)), (intptr_t)(F32(v8, 108)), (intptr_t)(F32(v8, 112)), (intptr_t)(F32(v8, 116)), (intptr_t)(&(*v39__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0)) == 1 )
        (*v38__f) = (*v39__f) > 0;
LABEL_67:
      result = (*v38__f);
      if ( a8 == 0 )
      {
        if ( (*v38__f) )
        {
          mh_5b35f0((*v40__f), a4);
LABEL_72:
          g_eng.h5bf3ee(v42, "You used %s.%%n%s", (const char *)(v8 + 4), Buffer);
          v28 = (*v37__f);
          strcpy(Buffer, v42);
          result = (*v38__f);
          *a6 = v28;
          return result;
        }
LABEL_71:
        strcpy(&Buffer[512 * (uint16_t)v9], ((char *)D(0x8a466c)));
        ++(*v37__f);
        goto LABEL_72;
      }
      return result;
    case 1:
      (*v36__f) = (int)&GAP(c, 0x4012, 30);
      (*v35__f) = 8;
      break;
    case 4:
      v11 = *(intptr_t *)&GAP(W, 0x1030, 20);
      if ( F32(v8, 88) != 20 )
        goto LABEL_67;
      if ( a8 != 0 )
      {
        v12 = F32(W, 0x2784);
        if ( *(_DWORD *)&GAP(W, 0x1030, 4) != 0
          || F32(W, 0x1030) == 2
          || F32(W, 0x2768) != 0
          || F32(v12, 16512) != 0
          || F32(v12, 16592) == 0 )
        {
          goto LABEL_67;
        }
      }
      else
      {
        F32(v11, 0x10) = 1;
        mh_5b3850(
          v11,
          1,
          F32(F32(W, 0x2784), 16592),
          F32(F32(W, 0x2784), 16596),
          F32(F32(W, 0x2784), 16600));
      }
      if ( Buffer != 0 )
      {
        (*v37__f) = 1;
        strcpy(Buffer, D(0x8a9b6c));
        SETLO(v9, 1);
      }
      (*v38__f) = true;
      goto LABEL_67;
    default:
      goto LABEL_67;
  }
  while ( 1 )
  {
    v25 = F32((*v36__f), 0);
    if ( W_pslot_ok((intptr_t)(F32((*v36__f), 0)), (intptr_t)(0), (intptr_t)(0)) )
    {
      if ( a8 != 0 )
      {
        if ( W_beh_47ae00((intptr_t)(*(intptr_t *)((uint8_t *)(v25 + (1) * 0x9f4) + 0x0)), (intptr_t)(0), (intptr_t)(F32(v8, 88)), (intptr_t)(F32(v8, 104)), (intptr_t)(F32(v8, 108)), (intptr_t)(F32(v8, 112)), (intptr_t)(F32(v8, 116)), (intptr_t)(&(*v39__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0)) != 1
          || (*v39__f) <= 0 )
        {
          goto LABEL_65;
        }
        goto LABEL_64;
      }
      if ( W_ih_5b28a0((intptr_t)(*(intptr_t *)((uint8_t *)(v25 + (1) * 0x9f4) + 0x0)), (intptr_t)(F32(v8, 88)), (intptr_t)(F32(v8, 104)), (intptr_t)(F32(v8, 108)), (intptr_t)(F32(v8, 112)), (intptr_t)(F32(v8, 116)), (intptr_t)(0), (intptr_t)(a7), (intptr_t)(1), (intptr_t)(&Buffer[512 * (uint16_t)v9])) != 0 )
      {
        ++v9;
LABEL_64:
        (*v38__f) = true;
      }
    }
LABEL_65:
    v26 = (*v35__f) == 1;
    (*v36__f) += 4;
    --(*v35__f);
    if ( v26 )
    {
      (*v37__f) = v9;
      goto LABEL_67;
    }
  }
}

/* 0x5b35f0: int __thiscall sub_5B35F0(cls_413B20 *this, int a2) */
int32_t mh_5b35f0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    char * v5 = 0;
    int16_t v6 = 0;
    char * v7 = 0;
    char * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    int32_t * v11 = 0;
    uint16_t i = 0;
    char * v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int v19 = 0;
    char * v23 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  v2 = a2;
  v3 = (uint16_t)a2;
  v5 = &GAP(c, 0x0, 16 * (uint16_t)a2 + 8208);
  if ( F16(F32(v5, 4 * 3), 100) == 0 )
    return 0;
  v6 = F16(v5, 2 * 4);
  if ( v6 != 0 )
    F16(v5, 2 * 4) = v6 - 1;
  if ( F16(v5, 2 * 4) != 0 )
    return 0;
  if ( (uint16_t)a2 < F16(c, 0x4010) )
  {
    if ( (uint16_t)a2 < F16(c, 0x4010) - 1 )
    {
      do
      {
        v7 = &GAP(c, 0x0, 16 * v3 + 8224);
        v8 = &GAP(c, 0x0, 16 * v3 + 8208);
        ++v2;
        F32(v8, 0) = F32(v7, 0);
        F32(v8, 4 * 1) = F32(v7, 4 * 1);
        F32(v8, 4 * 2) = F32(v7, 4 * 2);
        F32(v8, 4 * 3) = F32(v7, 4 * 3);
        v3 = (uint16_t)v2;
      }
      while ( (uint16_t)v2 < F16(c, 0x4010) - 1 );
    }
    --F16(c, 0x4010);
    F16(c, 0x4070) = 0;
    v23 = &GAP(c, 0x4012, 30);
    (*v21__f) = 8;
    do
    {
      v9 = F32(v23, 0);
      if ( F32(F32(v23, 0), 2500) == 1 || F32(F32(v23, 0), 2500) == 2 )
      {
        v10 = v9 + 2576;
        (*v22__f) = F32(v9, 2544);
        v11 = (_DWORD *)W_pslot_by_id((intptr_t)(c), (intptr_t)((*v22__f)), (intptr_t)(0));
        if ( v11 != 0 )
        {
          F32(v10, 100) = v11[669];
          F32(v10, 104) = v11[670];
          F32(v10, 108) = v11[671];
          F32(v10, 112) = v11[672];
          F32(v10, 140) = v11[679];
          F32(v10, 144) = v11[680];
          F32(v10, 148) = v11[681];
          F32(v10, 152) = v11[682];
          F32(v10, 116) = 0;
          F32(v10, 120) = 0;
          F32(v10, 124) = 0;
          F32(v10, 128) = 0;
          F32(v10, 132) = 0;
          F32(v10, 136) = 0;
          F16(v10, 288) = 0;
          F32(v10, 292) = 0;
          F32(v10, 296) = 0;
          F32(v10, 300) = 0;
          F32(v10, 304) = 0;
          F32(v10, 308) = 0;
          memset32((void *)(v10 + 164), 1000, 0xCu);
          for ( i = 0; i < F16(c, 0x4010); ++i )
          {
            v13 = &GAP(c, 0x0, 16 * i);
            if ( F32(v13, 4 * 2053) == (*v22__f) )
            {
              v14 = F32(v13, 4 * (2055));
              if ( v14[30] != 0 )
                W_item_stats_add((intptr_t)(v14), (intptr_t)(v10));
            }
          }
          v15 = F32(v10, 88) + F32(v10, 156) + F32(v10, 132);
          if ( v15 >= F32(v10, 84) )
            v15 = F32(v10, 84);
          v16 = F32(v10, 136);
          v17 = F32(v10, 96) + F32(v10, 160);
          F32(v10, 84) = v15;
          v18 = v17 + v16;
          if ( v17 + v16 >= F32(v10, 92) )
            v18 = F32(v10, 92);
          F32(v10, 92) = v18;
        }
      }
      v19 = (*v21__f) == 1;
      v23 += 4;
      --(*v21__f);
    }
    while ( !v19 );
  }
  return 1;
}

/* 0x5b3850: int __thiscall sub_5B3850(cls_5B3850 *this, int a2, int a3, int a4, int a5) */
int32_t mh_5b3850(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x90c) = a2;
  F32(c, 0x900) = a3;
  F32(c, 0x10) = 1;
  F32(c, 0x904) = a4;
  F32(c, 0x908) = a5;
  return a4;
}

/* 0x409150: int __thiscall sub_409150(cls_409150 *this, _DWORD *a2) */
int32_t mh_409150(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    char * v3 = 0;
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
    int32_t * v40 = 0;
    uint16_t v41 = 0;
    uint16_t v42 = 0;
    intptr_t v43 = 0;
    uint16_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t result = 0;
    intptr_t v47 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */

  v2 = F32(c, 0xa68);
  v3 = &GAP(c, 0x9f8, 24);
  v4 = a2[2];
  (*v48__f) = 0;
  if ( v2 < v4 )
  {
    v6 = v4 - v2;
    v7 = 250 * v6 / 1000;
    v8 = v6 - v7 + 2;
    if ( v8 > 0 )
      v8 = 100 * v8 * rnd() / 0x8000 % v8;
    v5 = v2 + v7 + v8;
  }
  else
  {
    v5 = v2;
  }
  v9 = F32(v3, 4 * 24);
  F32(v3, 4 * 22) = v5;
  v10 = a2[3];
  if ( v9 < v10 )
  {
    v12 = v10 - v9;
    v13 = 250 * v12 / 1000;
    v14 = v12 - v13 + 2;
    if ( v14 > 0 )
      v14 = 100 * v14 * rnd() / 0x8000 % v14;
    v11 = v9 + v13 + v14;
  }
  else
  {
    v11 = v9;
  }
  v15 = F32(v3, 4 * 25);
  F32(v3, 4 * 24) = v11;
  v16 = a2[4];
  if ( v15 < v16 )
  {
    v18 = v16 - v15;
    v19 = 250 * v18 / 1000;
    v20 = v18 - v19 + 2;
    if ( v20 > 0 )
      v20 = 100 * v20 * rnd() / 0x8000 % v20;
    v17 = v15 + v19 + v20;
  }
  else
  {
    v17 = v15;
  }
  v21 = F32(v3, 4 * 26);
  F32(v3, 4 * 25) = v17;
  v22 = a2[5];
  if ( v21 < v22 )
  {
    v24 = v22 - v21;
    v25 = 250 * v24 / 1000;
    v26 = v24 - v25 + 2;
    if ( v26 > 0 )
      v26 = 100 * v26 * rnd() / 0x8000 % v26;
    v23 = v21 + v25 + v26;
  }
  else
  {
    v23 = v21;
  }
  v27 = F32(v3, 4 * 27);
  F32(v3, 4 * 26) = v23;
  v28 = a2[6];
  if ( v27 < v28 )
  {
    v30 = v28 - v27;
    v31 = 250 * v30 / 1000;
    v32 = v30 - v31 + 2;
    if ( v32 > 0 )
      v32 = 100 * v32 * rnd() / 0x8000 % v32;
    v29 = v27 + v31 + v32;
  }
  else
  {
    v29 = v27;
  }
  v33 = F32(v3, 4 * 28);
  F32(v3, 4 * 27) = v29;
  v34 = a2[7];
  if ( v33 < v34 )
  {
    v36 = v34 - v33;
    v37 = 250 * v36 / 1000;
    v38 = v36 - v37 + 2;
    if ( v38 > 0 )
      v38 = 100 * v38 * rnd() / 0x8000 % v38;
    v35 = v33 + v37 + v38;
  }
  else
  {
    v35 = v33;
  }
  F32(v3, 4 * 28) = v35;
  F32(v3, 4 * 59) = a2[8];
  v39 = 0;
  v40 = (int32_t *)D(0x6907c0);
  do
  {
    if ( *v40 != *a2 )
      goto LABEL_44;
    v41 = F16(v3, 2 * 162);
    v42 = 0;
    if ( v41 != 0 )
    {
      while ( F32(&v3[12 * v42 + 328], 0) != v40[1] )
      {
        if ( ++v42 >= v41 )
          goto LABEL_36;
      }
    }
    else
    {
LABEL_36:
      v43 = v40[3];
      if ( v43 != 0 )
      {
        v44 = 0;
        if ( v41 == 0 )
          goto LABEL_44;
        while ( F32(&v3[12 * v44 + 328], 0) != v43 )
        {
          if ( ++v44 >= v41 )
            goto LABEL_44;
        }
      }
      v45 = v40[2];
      if ( v45 <= a2[1] && v45 != 0 )
        W_sc_406c70((intptr_t)((int)v3), (intptr_t)(((int32_t *)D(0x6907c4))[v39]), (intptr_t)(255), (intptr_t)(0), (intptr_t)(1));
    }
LABEL_44:
    v39 = 4 * (uint16_t)++(*v48__f);
    v40 = &((int32_t *)D(0x6907c0))[v39];
  }
  while ( ((int32_t *)D(0x6907c0))[v39] != 0 );
  result = (int)c;
  v47 = F32(c, 0x9f4);
  if ( v47 != 0 )
    return W_char_skills_refresh((intptr_t)(v47), (intptr_t)(0));
  return result;
}
/* unported callees: none */
