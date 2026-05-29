/*
 * src/menu_list.h — the menu-list controller: its list-header model and
 * the cursor-navigation engine that drives it.
 *
 * The title menu (and most of the engine's selection UIs) is driven by a
 * "menu controller" object.  Its selectable contents are described by a
 * *list header* it points to at controller+0x174.  This module ports the
 * pure, object-model-coupled-but-Win32-free pieces of that controller:
 *
 *   FUN_004192b0  menu_list_scroll_into_view  — recompute the page-top so
 *                                               the cursor is on-screen;
 *                                               return 1 if it moved.
 *
 * Two larger pieces of the same controller are mapped but NOT yet ported
 * (referenced by bare VA so the port ledger doesn't count them early):
 *   0x43ca40  the 970-byte cursor-nav engine — turn a direction / key-
 *             repeat code into a cursor move, page scroll, or cancel latch.
 *   0x43ce50  the action latch that gates the nav engine and resolves the
 *             type-2 confirm list.
 *
 * Ground truth: the disassembly at 0x4192b0 (52 B), 0x43ca40 (970 B) and
 * 0x43ce50 (220 B).  0x43ca40's inner dispatch is an indirect jump table
 * Ghidra could not recover (`switchdataD_0043ce1c`); it was read out of the
 * image with radare2 — see menu_list.c for the recovered mapping.
 *
 * Pure: pointer/integer arithmetic plus GetTickCount() for key-repeat
 * timing (injected as a parameter on the host, so the port stays testable
 * without a clock).  No Win32 surface.
 */
#ifndef OPENSUMMONERS_MENU_LIST_H
#define OPENSUMMONERS_MENU_LIST_H

#include <stdint.h>
#include <stddef.h>

/* ─── the list header (controller + 0x174 → here) ────────────────────
 *
 * Describes the selectable grid/list the cursor moves over.  Several
 * fields are *reused* with a meaning that depends on `type`:
 *
 *   type == 0  a linear (1-D) list that wraps; +0x18 acts as a wrap floor
 *   type == 2  a 2-D grid paged by `stride`; +0x18 is the visible page-top
 *   type == 3  a list whose `page_top` (+0x18) trails the cursor by a page
 *
 * The dword indices used by the engine (piVar1[n]) map to:
 *   [0]=type  [3]=stride  [4]=count  [5]=cursor  [6]=sel2
 * i.e. byte offsets +0x00 / +0xc / +0x10 / +0x14 / +0x18.
 */
typedef struct menu_list_hdr {
    int32_t  type;        /* +0x00 — list orientation (0 linear / 2 grid / 3) */
    uint8_t  _pad04[0x08];/* +0x04..+0x0b — opaque                            */
    int32_t  stride;      /* +0x0c — page / row stride (entries per page-row) */
    int32_t  count;       /* +0x10 — total entry count                        */
    int32_t  cursor;      /* +0x14 — current selection index                  */
    int32_t  sel2;        /* +0x18 — page-top / secondary cursor (type-dep.)  */
    uint32_t repeat_a;    /* +0x1c — key-repeat deadline, GetTickCount domain  */
    uint32_t repeat_b;    /* +0x20 — key-repeat deadline (second axis)         */
} menu_list_hdr;

/* ─── the menu controller (the `this`/ECX of all three functions) ────
 *
 * Only the handful of fields the ported functions touch are modelled; the
 * rest is opaque padding so the retail offsets line up on the 32-bit build.
 *
 *   sub     +0x00 — input/device sub-object; the latch reads sub[+0x04]
 *                   (enabled?) and sub[+0x54] (== 1000 means "ready").
 *   mode    +0x08 — latch dispatch mode: 1 = cursor-nav list (calls nav),
 *                   2 = paged confirm list (handled inline by the latch).
 *   action  +0x1c — last resolved action code, latched here by nav/latch.
 *   list2   +0x170 — the type-2 confirm list's state object (latch only).
 *   list    +0x174 — pointer to the menu_list_hdr above.
 */
typedef struct menu_ctrl {
    void          *sub;          /* +0x00 */
    uint8_t        _pad04[0x04]; /* +0x04..+0x07 */
    int32_t        mode;         /* +0x08 */
    uint8_t        _pad0c[0x10]; /* +0x0c..+0x1b */
    int32_t        action;       /* +0x1c */
    uint8_t        _pad20[0x150];/* +0x20..+0x16f */
    void          *list2;        /* +0x170 */
    menu_list_hdr *list;         /* +0x174 */
} menu_ctrl;

/* ─── FUN_004192b0 — scroll the cursor's page into view ──────────────
 *
 * Recompute the page-top `list->sel2` (+0x18) as the largest multiple of
 * `stride` not exceeding `cursor` — i.e. floor(cursor/stride)*stride, the
 * top of the page that contains the cursor.  If that differs from the
 * stored sel2 it is updated and the function returns 1 (the view moved);
 * otherwise 0.  Faithful to 0x4192b0.
 *
 * Precondition (as in retail, which has no guard): stride > 0 and
 * cursor >= 0, else the page-top search loop does not terminate. */
int menu_list_scroll_into_view(menu_ctrl *c);

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(menu_list_hdr, type)     == 0x00, "hdr.type");
_Static_assert(offsetof(menu_list_hdr, stride)   == 0x0c, "hdr.stride");
_Static_assert(offsetof(menu_list_hdr, count)    == 0x10, "hdr.count");
_Static_assert(offsetof(menu_list_hdr, cursor)   == 0x14, "hdr.cursor");
_Static_assert(offsetof(menu_list_hdr, sel2)     == 0x18, "hdr.sel2");
_Static_assert(offsetof(menu_list_hdr, repeat_a) == 0x1c, "hdr.repeat_a");
_Static_assert(offsetof(menu_list_hdr, repeat_b) == 0x20, "hdr.repeat_b");
_Static_assert(offsetof(menu_ctrl, sub)    == 0x00,  "ctrl.sub");
_Static_assert(offsetof(menu_ctrl, mode)   == 0x08,  "ctrl.mode");
_Static_assert(offsetof(menu_ctrl, action) == 0x1c,  "ctrl.action");
_Static_assert(offsetof(menu_ctrl, list2)  == 0x170, "ctrl.list2");
_Static_assert(offsetof(menu_ctrl, list)   == 0x174, "ctrl.list");
#endif

#endif /* OPENSUMMONERS_MENU_LIST_H */
