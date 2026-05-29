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
 *   FUN_0043ca40  menu_list_nav               — the 970-byte cursor-nav
 *                                               engine: turn a direction /
 *                                               auto-repeat code into a
 *                                               cursor move, page scroll,
 *                                               or cancel/confirm latch.
 *
 * One more piece of the same controller is mapped but NOT yet ported
 * (referenced by bare VA so the port ledger doesn't count it early):
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
    int32_t  alloc_a;     /* +0x04 — ctor dim A: sizes the controller's +0x17c
                           *         row array (×0x10); not read by the ports  */
    int32_t  alloc_b;     /* +0x08 — ctor dim B: sizes the +0x178 array (×0x24)
                           *         and bounds 411f40's cell loop; ports skip  */
    int32_t  stride;      /* +0x0c — page / row stride (entries per page-row) */
    int32_t  count;       /* +0x10 — total entry count                        */
    int32_t  cursor;      /* +0x14 — current selection index                  */
    int32_t  sel2;        /* +0x18 — page-top / secondary cursor (type-dep.)  */
    uint32_t repeat_a;    /* +0x1c — key-repeat deadline, GetTickCount domain  */
    uint32_t repeat_b;    /* +0x20 — key-repeat deadline (second axis)         */
    /* Exactly 0x24 bytes — confirmed by the ctor's operator_new(0x24) in
     * 0x40f5c0, which fills type/alloc_a/alloc_b/stride from its params
     * and zeros count/cursor/sel2/repeat_a/repeat_b. */
} menu_list_hdr;

/* ─── the input "ready" gate sub-object (controller + 0x00 → here) ───
 *
 * The latch (FUN_0043ce50) refuses to act unless the input sub-object is
 * both enabled (+0x04 non-zero) and fully transitioned-in (+0x54 == 1000,
 * the same 0..1000 ramp the title fades use).  Only those two fields are
 * read; everything else is opaque. */
typedef struct menu_input_sub {
    uint8_t  _pad00[0x04];   /* +0x00..+0x03 */
    int32_t  enabled;        /* +0x04 — must be non-zero */
    uint8_t  _pad08[0x4c];   /* +0x08..+0x53 */
    int32_t  ready;          /* +0x54 — must equal 1000 */
} menu_input_sub;

/* ─── the type-2 "confirm / message" list (controller + 0x170) ───────
 *
 * A scrolling-message / confirm widget the latch drives directly (rather
 * than through the cursor-nav engine).  In submode 1 it reveals text up to
 * a cap and then dismisses; the cap is reached through a two-hop pointer
 * chain: src (+0x00) → src[+0x0c] → that[+0x08] is the u16 total length.
 */
typedef struct confirm_caprec {
    void    *owned0;         /* +0x00 — owned sub-buffer; freed by menu_ctrl_clear */
    uint8_t  _pad04[0x04];   /* +0x04..+0x07 */
    uint16_t cap;            /* +0x08 — total length / max position (u16) */
} confirm_caprec;

typedef struct confirm_src {
    void          *owned0;   /* +0x00 — owned sub-buffer; freed by menu_ctrl_clear */
    uint8_t        _pad04[0x04];/* +0x04..+0x07 */
    void          *owned8;   /* +0x08 — owned sub-buffer; freed by menu_ctrl_clear */
    confirm_caprec *caprec;  /* +0x0c — record carrying the length cap */
} confirm_src;

typedef struct confirm_list {
    confirm_src *src;        /* +0x00 — element source (→ caprec → cap)   */
    uint16_t     pos;        /* +0x04 — current revealed position (u16)   */
    uint16_t     _hi06;      /* +0x06 — high half of dword[1]; not touched */
    uint8_t      _pad08[0x04];/* +0x08..+0x0b */
    int32_t      submode;    /* +0x0c — 0 = flag-driven, 1 = scroll/advance */
    uint8_t      _pad10[0x04];/* +0x10..+0x13 */
    int32_t      flag14;     /* +0x14 — submode-0 "have content" flag      */
    int32_t      flag18;     /* +0x18 — submode-0 "pending ack" flag       */
} confirm_list;

/* ─── the controller's parallel geometry arrays ──────────────────────
 *
 * The constructor (FUN_0040f5c0) lays out the controller's selectable grid
 * as two heap arrays sized off the list header's two dimensions:
 *
 *   rows    (controller + 0x17c) — `alloc_a` (hdr+0x04) entries × 0x10 B.
 *           One per menu line.  Each owns its own cell array.
 *   entries (controller + 0x178) — `alloc_b` (hdr+0x08) entries × 0x24 B.
 *           Per-column metadata (initial scroll position / extent).
 *
 * and, hanging off each row, a cell array of `alloc_b` × 0x18 B.
 */

/* A grid cell (row->cells[k], 0x18 B).  The three pointer slots are built
 * lazily by FUN_00411f40 (the grid-cell finalizer, not yet ported) and
 * torn down by menu_ctrl_clear; the constructor only NULLs them. */
typedef struct menu_cell {
    void   *obj0;       /* +0x00 — lazily-built primary object; *its* [0] is
                         *         itself an owned ptr (clear frees both)     */
    void   *obj54;      /* +0x04 — lazily operator_new(0x54) sub-object       */
    void   *obj20;      /* +0x08 — lazily operator_new(0x20) sub-object       */
    int32_t field_c;    /* +0x0c — zeroed by ctor                            */
    uint8_t _pad10[0x08];/* +0x10..+0x17 — ctor leaves untouched (stride 0x18)*/
} menu_cell;

/* A menu row (controller->rows[r], 0x10 B).  The spawn block populates
 * field0/action/flag8 when appending a row; the ctor only zeroes field0
 * and action (it leaves flag8 indeterminate) and hangs the cell array. */
typedef struct menu_row {
    int32_t    field0;  /* +0x00 — zeroed by ctor; spawn writes 0 (a select key)*/
    int32_t    action;  /* +0x04 — zeroed by ctor; spawn writes the action id */
    int32_t    flag8;   /* +0x08 — NOT written by ctor; spawn writes 1        */
    menu_cell *cells;   /* +0x0c — operator_new(alloc_b * 0x18): the cells    */
} menu_row;

/* Per-column metadata (controller->entries[e], 0x24 B).  The ctor stamps
 * the initial scroll position (index*0x20) and extent (0x20) and zeroes
 * the rest. */
typedef struct menu_entry {
    int32_t pos;        /* +0x00 — index << 5 (index * 0x20): initial offset */
    int32_t field4;     /* +0x04 — 0 */
    int32_t extent;     /* +0x08 — 0x20 */
    int32_t field_c;    /* +0x0c — 0 */
    int32_t field_10;   /* +0x10 — 0 */
    int32_t field_14;   /* +0x14 — 0 */
    int32_t field_18;   /* +0x18 — 0 */
    int32_t field_1c;   /* +0x1c — 0 */
    int32_t field_20;   /* +0x20 — 0 */
} menu_entry;           /* 0x24 B */

/* ─── the menu controller (the `this`/ECX of all functions) ──────────
 *
 * Only the fields the ported functions touch are modelled; the rest is
 * opaque padding so the retail offsets line up on the 32-bit build.
 *
 *   sub       +0x00 — input "ready" gate sub-object (latch only).
 *   mode      +0x08 — latch dispatch mode (ctor sets 1): 1 = cursor-nav
 *                     list (calls nav), 2 = paged confirm list (latch).
 *   field_c   +0x0c — ctor param_1 (0 for the title menu).
 *   field_10  +0x10 — ctor param_2 (0 for the title menu).
 *   action    +0x1c — last resolved action code, latched by nav/latch.
 *   field_20  +0x20 — ctor zeroes.
 *   field_84  +0x84 — ctor zeroes.
 *   field_140 +0x140 — ctor zeroes.
 *   field_164 +0x164 — an owned ptr (built elsewhere; clear frees it).
 *   list2     +0x170 — the type-2 confirm list's state object (latch only).
 *   list      +0x174 — pointer to the menu_list_hdr above.
 *   entries   +0x178 — the per-column metadata array (ctor allocates).
 *   rows      +0x17c — the menu-row array (ctor allocates).
 */
typedef struct menu_ctrl {
    menu_input_sub *sub;          /* +0x00 */
    uint8_t         _pad04[0x04]; /* +0x04..+0x07 */
    int32_t         mode;         /* +0x08 */
    int32_t         field_c;      /* +0x0c */
    int32_t         field_10;     /* +0x10 */
    uint8_t         _pad14[0x08]; /* +0x14..+0x1b */
    int32_t         action;       /* +0x1c */
    int32_t         field_20;     /* +0x20 */
    uint8_t         _pad24[0x60]; /* +0x24..+0x83 */
    int32_t         field_84;     /* +0x84 */
    uint8_t         _pad88[0xb8]; /* +0x88..+0x13f */
    int32_t         field_140;    /* +0x140 */
    uint8_t         _pad144[0x20];/* +0x144..+0x163 */
    void           *field_164;    /* +0x164 */
    uint8_t         _pad168[0x08];/* +0x168..+0x16f */
    confirm_list   *list2;        /* +0x170 */
    menu_list_hdr  *list;         /* +0x174 */
    menu_entry     *entries;      /* +0x178 */
    menu_row       *rows;         /* +0x17c */
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

/* ─── FUN_0043ca40 — the cursor-navigation engine ────────────────────
 *
 * Apply menu action `dir` to the controller's list and return a result
 * code: 0 (no change), 1 (cursor moved within the page), 2 (page
 * scrolled), 3 (cancel latched), 4 (confirm latched).
 *
 * `dir` enum: 0 prev, 1 next, 2 page-up, 3 page-down, 4/6 axis-held
 * auto-repeat (positive/negative), 5/7 axis-released, 9 cancel, 10
 * confirm; 8 and >10 are no-ops.  The auto-repeat cases (4/6) consult
 * per-axis deadlines stored in the header (repeat_a/repeat_b); `now` is
 * the GetTickCount() value the retail engine samples internally, injected
 * so the port is testable without a real clock.  See menu_list.c for the
 * recovered jump table and the per-type cursor arithmetic. */
int32_t menu_list_nav(menu_ctrl *c, uint32_t dir, uint32_t now);

/* ─── FUN_0043ce50 — the input-action latch ──────────────────────────
 *
 * The gate in front of the menu's input handling.  Returns 0 unless the
 * input sub-object is ready (sub->ready == 1000 && sub->enabled != 0).
 * When ready it dispatches on controller mode:
 *   mode 1 → forward to menu_list_nav(c, dir, now) and return its result.
 *   mode 2 → drive the confirm/message list (list2) directly:
 *       submode 0, action dir in [8,10]: if a pending-ack flag is set,
 *         clear it and return 6; else if content is present latch
 *         action=8 and return 8; otherwise return 0.
 *       submode 1, action dir in [8,10], not already dismissed: if the
 *         revealed position has reached the cap, latch action=8 and return
 *         8 (dismiss); otherwise fast-forward pos to the cap, latch
 *         action=6 and return 6 (reveal-all).
 * `now` is only consulted on the mode-1 path (passed through to nav). */
int32_t menu_list_latch(menu_ctrl *c, uint32_t dir, uint32_t now);

/* ─── FUN_0040e0c0 — tear down the controller's geometry ─────────────
 *
 * Free everything the constructor (and the spawn block) hung off the
 * controller and NULL the slots, leaving it safe to rebuild:
 *   - the confirm list (list2, +0x170) and its source graph;
 *   - the owned buffer at +0x164;
 *   - the per-column metadata array (entries, +0x178);
 *   - every row's cell array (and each cell's three lazily-built
 *     sub-objects), then the row array itself (rows, +0x17c) — its bounds
 *     come from the still-live header's alloc_a/alloc_b;
 *   - the list header (list, +0x174), freed last.
 * Every step is guarded on non-NULL, so it is a no-op on a fresh (all-zero)
 * controller — which is exactly how the constructor uses it (it clears any
 * stale state from a recycled pool slot before rebuilding). */
void menu_ctrl_clear(menu_ctrl *c);

/* ─── FUN_0040f5c0 — build the controller's list header + geometry ───
 *
 * Clear any prior state (menu_ctrl_clear), then allocate and initialise:
 *   - the list header (0x24 B): type/alloc_a/alloc_b/stride from the
 *     params; count/cursor/sel2/repeat_a/repeat_b zeroed;
 *   - the row array (alloc_a × menu_row) and, per row, a cell array
 *     (alloc_b × menu_cell), all zeroed;
 *   - the per-column metadata array (alloc_b × menu_entry): each stamped
 *     with pos = index*0x20 and extent = 0x20.
 * Also seeds controller scalars: mode=1, field_c/field_10 from params,
 * field_20/field_84/field_140 = 0.  Faithful to 0x40f5c0.
 *
 * Divergence: retail's operator_new returns uninitialised memory, so a few
 * ctor-unwritten fields (row.flag8, the 8 trailing bytes of each cell) are
 * indeterminate there; this port uses calloc, so they read as zero — which
 * matches every observed first use (the spawn block always writes flag8
 * before reading it). */
void menu_ctrl_build(menu_ctrl *c, int32_t f_c, int32_t f_10,
                     int32_t alloc_a, int32_t alloc_b,
                     int32_t stride, int32_t type);

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(menu_list_hdr, type)     == 0x00, "hdr.type");
_Static_assert(offsetof(menu_list_hdr, stride)   == 0x0c, "hdr.stride");
_Static_assert(offsetof(menu_list_hdr, count)    == 0x10, "hdr.count");
_Static_assert(offsetof(menu_list_hdr, cursor)   == 0x14, "hdr.cursor");
_Static_assert(offsetof(menu_list_hdr, sel2)     == 0x18, "hdr.sel2");
_Static_assert(offsetof(menu_list_hdr, repeat_a) == 0x1c, "hdr.repeat_a");
_Static_assert(offsetof(menu_list_hdr, repeat_b) == 0x20, "hdr.repeat_b");
_Static_assert(sizeof(menu_list_hdr)             == 0x24, "hdr size (operator_new(0x24) in ctor 0x40f5c0)");
_Static_assert(offsetof(menu_ctrl, sub)       == 0x00,  "ctrl.sub");
_Static_assert(offsetof(menu_ctrl, mode)      == 0x08,  "ctrl.mode");
_Static_assert(offsetof(menu_ctrl, field_c)   == 0x0c,  "ctrl.field_c");
_Static_assert(offsetof(menu_ctrl, field_10)  == 0x10,  "ctrl.field_10");
_Static_assert(offsetof(menu_ctrl, action)    == 0x1c,  "ctrl.action");
_Static_assert(offsetof(menu_ctrl, field_20)  == 0x20,  "ctrl.field_20");
_Static_assert(offsetof(menu_ctrl, field_84)  == 0x84,  "ctrl.field_84");
_Static_assert(offsetof(menu_ctrl, field_140) == 0x140, "ctrl.field_140");
_Static_assert(offsetof(menu_ctrl, field_164) == 0x164, "ctrl.field_164");
_Static_assert(offsetof(menu_ctrl, list2)     == 0x170, "ctrl.list2");
_Static_assert(offsetof(menu_ctrl, list)      == 0x174, "ctrl.list");
_Static_assert(offsetof(menu_ctrl, entries)   == 0x178, "ctrl.entries");
_Static_assert(offsetof(menu_ctrl, rows)      == 0x17c, "ctrl.rows");
_Static_assert(sizeof(menu_cell)           == 0x18, "menu_cell size");
_Static_assert(offsetof(menu_cell, obj54)  == 0x04, "cell.obj54");
_Static_assert(offsetof(menu_cell, obj20)  == 0x08, "cell.obj20");
_Static_assert(sizeof(menu_row)            == 0x10, "menu_row size");
_Static_assert(offsetof(menu_row, action)  == 0x04, "row.action");
_Static_assert(offsetof(menu_row, flag8)   == 0x08, "row.flag8");
_Static_assert(offsetof(menu_row, cells)   == 0x0c, "row.cells");
_Static_assert(sizeof(menu_entry)          == 0x24, "menu_entry size");
_Static_assert(offsetof(menu_entry, extent)== 0x08, "entry.extent");
_Static_assert(offsetof(menu_entry, field_20) == 0x20, "entry.field_20");
_Static_assert(offsetof(menu_input_sub, enabled) == 0x04, "sub.enabled");
_Static_assert(offsetof(menu_input_sub, ready)   == 0x54, "sub.ready");
_Static_assert(offsetof(confirm_caprec, cap)     == 0x08, "caprec.cap");
_Static_assert(offsetof(confirm_src, owned8)     == 0x08, "src.owned8");
_Static_assert(offsetof(confirm_src, caprec)     == 0x0c, "src.caprec");
_Static_assert(offsetof(confirm_list, src)     == 0x00, "cl.src");
_Static_assert(offsetof(confirm_list, pos)     == 0x04, "cl.pos");
_Static_assert(offsetof(confirm_list, submode) == 0x0c, "cl.submode");
_Static_assert(offsetof(confirm_list, flag14)  == 0x14, "cl.flag14");
_Static_assert(offsetof(confirm_list, flag18)  == 0x18, "cl.flag18");
#endif

#endif /* OPENSUMMONERS_MENU_LIST_H */
