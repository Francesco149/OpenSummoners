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

#include "obj_container.h"   /* sel_list — the menu-node transition owner loop */

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
 * elsewhere (the spawn block / menu-item builder) and torn down by
 * menu_ctrl_clear; the constructor only NULLs them.  FUN_00411f40 (the
 * grid-cell finalizer, menu_row_finalize below) *refreshes* them — it
 * re-zeroes the sub-objects' fields, but does NOT allocate them (its
 * retail alloc branches are dead code — quirk #36). */
typedef struct menu_cell {
    void   *obj0;       /* +0x00 — primary text/glyph object; *its* [0] is
                         *         itself an owned ptr (clear frees both)     */
    void   *obj54;      /* +0x04 — 0x54-byte sub-object (finalizer re-zeroes) */
    void   *obj20;      /* +0x08 — 0x20-byte sub-object (finalizer re-zeroes) */
    int32_t field_c;    /* +0x0c — zeroed by ctor                            */
    uint8_t _pad10[0x08];/* +0x10..+0x17 — ctor leaves untouched (stride 0x18)*/
} menu_cell;

/* cell.obj54 (0x54 B).  Only the fields the finalizer (FUN_00411f40)
 * re-zeroes are modelled; the rest is opaque.  Allocated elsewhere. */
typedef struct menu_cell_obj54 {
    int32_t  field0;       /* +0x00 */
    uint16_t field4;       /* +0x04 */
    uint8_t  _pad06[0x40]; /* +0x06..+0x45 */
    uint16_t field46;      /* +0x46 */
    uint16_t field48;      /* +0x48 */
    uint16_t field4a;      /* +0x4a */
    uint16_t field4c;      /* +0x4c */
    uint8_t  _pad4e[0x02]; /* +0x4e..+0x4f */
    int32_t  field50;      /* +0x50 */
} menu_cell_obj54;         /* 0x54 B */

/* cell.obj20 (0x20 B).  The finalizer zeroes the first seven fields and then
 * recomputes field1c as a clamp of two of them. */
typedef struct menu_cell_obj20 {
    int32_t  field0;       /* +0x00 */
    int32_t  field4;       /* +0x04 */
    uint16_t field8;       /* +0x08 (word store) */
    uint8_t  _pad0a[0x02]; /* +0x0a..+0x0b */
    int32_t  field_c;      /* +0x0c */
    int32_t  field10;      /* +0x10 */
    int32_t  field14;      /* +0x14 — clamp lower bound */
    int32_t  field18;      /* +0x18 — clamp source       */
    int32_t  field1c;      /* +0x1c — = max(field14, min(field18, 0)) */
} menu_cell_obj20;         /* 0x20 B */

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

/* ─── the unported cell text-layout builder (0x40fa00) ───────────
 *
 * 0x40fa00 (800 B) is the cell's glyph/text-layout builder: it parses a
 * (Shift-JIS) string into the cell's obj0 glyph table, handling '#' colour
 * escapes and a font-metrics table.  It is its own subsystem and NOT yet
 * ported.  menu_row_finalize invokes it once per cell whose obj0 is already
 * built, passing &DAT_008a9b6c — the god object's engine-name buffer
 * (god+0x1c; see findings/audio-init.md).  The fresh title menu never has a
 * built obj0, so this never fires there.
 *
 * Until 0x40fa00 lands, the call site routes through this hook so the
 * dispatch stays observable/testable.  Both default to a no-op source: the
 * hook is NULL (call skipped) and the text pointer is NULL (stands in for
 * the unmodelled &DAT_008a9b6c). */
typedef void (*menu_cell_layout_fn)(menu_ctrl *c, int32_t row, int32_t cell,
                                    const void *text_src);
extern menu_cell_layout_fn menu_cell_layout_hook;
extern const void         *menu_cell_layout_text;

/* ─── FUN_00411f40 — refresh a row's cell sub-objects ────────────────
 *
 * For each cell of row `row` (up to the header's alloc_b), refresh its
 * lazily-built sub-objects in place:
 *   - obj0 present  → re-lay-out its glyph text (0x40fa00, via the hook);
 *   - obj54 present → re-zero its modelled fields;
 *   - obj20 present → re-zero its fields and recompute
 *                     field1c = max(field14, min(field18, 0)).
 * The obj54/obj20 work is additionally gated on `row < header->count`.
 *
 * Despite the decompile, this does NOT allocate: its per-sub-object
 * `if (ptr == 0) operator_new(...)` branches sit under an outer `ptr != 0`
 * guard, so they are unreachable (quirk #36).  On the fresh title menu every
 * cell pointer is NULL, so the whole function is a no-op there.  Faithful to
 * 0x411f40. */
void menu_row_finalize(menu_ctrl *c, int32_t row);

/* ─── a menu tree node / page (the 0x1b0 object built by FUN_0040f3e0) ─
 *
 * The engine's menu is a *tree* of uniform 0x1b0-byte nodes.  Each node is
 * dual-purpose, overlaying two views on one buffer:
 *
 *   - a **container header** (+0x00..+0x84): an owner back-pointer, a few
 *     config scalars, and — at +0x48/+0x4c — a heap array of child-node
 *     pointers and its count.  This is the view FUN_0040f3e0 manipulates.
 *   - an **embedded menu_ctrl** at +0x00 (so node+0x164..+0x17c are exactly
 *     menu_ctrl.field_164/list2/list/entries/rows), followed by 0x30 B of
 *     **display config** (+0x180..+0x1ac): text/shadow colours and label
 *     string pointers.  This is the view the nav/build code uses, and the
 *     reason a child can be torn down with menu_ctrl_clear.
 *
 * So a node is simultaneously a tree container and a selectable controller,
 * one layout reused at every level.  The owning list (FUN_0040f3e0's
 * param_1) is a sel_list (obj_container.h) whose entries are these nodes —
 * node+0x08 is the sel_entry "selected" flag, set by sel_list_mark_last.
 *
 * Ghidra mis-typed FUN_0040f3e0's __thiscall (it rendered the `this` node as
 * a plain first arg and dropped the ECX node), so the earlier "operates on a
 * page-container" reading was off by one: the disasm at 0x40f3ec
 * (`mov ebx,ecx`) and the call site 0x56b606 (`mov ecx,[edi+ecx]` =
 * owner->entries[count]) confirm `this` is the node and param_1 is the owner.
 * See engine-quirks #37.  Only the fields the builder touches are modelled. */
typedef struct menu_node {
    void    *owner;          /* +0x00 — back-ptr to the owning sel_list (param_1) */
    int32_t  field4;         /* +0x04 — builder sets 1                            */
    int32_t  selected;       /* +0x08 — sel_entry flag; builder zeroes, then
                              *         sel_list_mark_last sets the active node   */
    int32_t  field_c;        /* +0x0c — param_2                                   */
    int32_t  field_10;       /* +0x10 — param_3                                   */
    int32_t  field_14;       /* +0x14 — param_4 (each child re-zeroes this)       */
    int32_t  field_18;       /* +0x18 — param_5 (each child re-zeroes this)       */
    int32_t  field_1c;       /* +0x1c — builder sets 1 (overlays menu_ctrl.action)*/
    uint8_t  _pad20[0x28];   /* +0x20..+0x47 — opaque                             */
    void   **children;       /* +0x48 — heap array of child-node pointers         */
    uint16_t child_count;    /* +0x4c — number of children (u16)                  */
    uint16_t field_4e;       /* +0x4e — builder zeroes (u16)                      */
    int32_t  field_50;       /* +0x50 — builder sets 1                            */
    int32_t  field_54;       /* +0x54 — builder zeroes                            */
    int32_t  field_58;       /* +0x58 — builder zeroes                            */
    int32_t  config[9];      /* +0x5c..+0x7f — 9-dword config blob (param_7); when
                              *         param_7 is NULL only config[0] is zeroed  */
    int32_t  field_80;       /* +0x80 — builder zeroes                            */
    uint8_t  _pad84[0xe0];   /* +0x84..+0x163 — opaque (embedded menu_ctrl body)  */
    void    *ctrl_field_164; /* +0x164 — menu_ctrl.field_164 (child zeroes)       */
    uint8_t  _pad168[0x08];  /* +0x168..+0x16f                                    */
    void    *ctrl_list2;     /* +0x170 — menu_ctrl.list2  (child zeroes)          */
    void    *ctrl_list;      /* +0x174 — menu_ctrl.list   (child zeroes)          */
    void    *ctrl_entries;   /* +0x178 — menu_ctrl.entries(child zeroes)          */
    void    *ctrl_rows;      /* +0x17c — menu_ctrl.rows   (child zeroes)          */
    uint32_t color0;         /* +0x180 — 0x3e537d (text colour)                   */
    uint32_t color1;         /* +0x184 — 0xa8b9cc                                 */
    uint32_t label0;         /* +0x188 — &DAT_00677b98 (retail VA)                */
    uint32_t color2;         /* +0x18c — 0xf08080                                 */
    uint32_t color3;         /* +0x190 — 0xf08080                                 */
    uint32_t label1;         /* +0x194 — &DAT_008090a9 (retail VA, empty string)  */
    uint32_t label2;         /* +0x198 — &DAT_008090a9 (retail VA)                */
    uint32_t color4;         /* +0x19c — 0x3e537d                                 */
    uint32_t color5;         /* +0x1a0 — 0xa8b9cc                                 */
    uint32_t field_1a4;      /* +0x1a4 — 0                                        */
    uint32_t field_1a8;      /* +0x1a8 — 0                                        */
    uint32_t field_1ac;      /* +0x1ac — 0x1c                                     */
} menu_node;                 /* 0x1b0 B */

/* ─── FUN_0040f3e0 — (re)build a menu node and its child array ────────
 *
 * Configure node `n` (an entry of `owner`'s sel_list) from its params,
 * release any previous child array — clearing each child's embedded
 * controller with menu_ctrl_clear first — then allocate `n_children` fresh
 * child nodes, each with its embedded menu_ctrl zeroed and its display
 * config (colours + label pointers) seeded.  `config` (param_7) is a
 * 9-dword blob copied into +0x5c..+0x7f, or NULL to just zero config[0].
 *
 * The title menu calls this as menu_node_build(node, owner, 0,0,100,100,1,
 * NULL): a single child, default config.  Faithful to 0x40f3e0.
 *
 * Divergences (both following menu_ctrl_build's convention): operator_new →
 * calloc, so each child's container-header bytes (uninitialised in retail)
 * read as zero here; and the per-child menu_ctrl_clear is layout-exact only
 * on the 32-bit target — the 64-bit host exercises it solely on freshly
 * zeroed children (an all-NULL no-op), matching the title flow, which never
 * rebuilds a populated node. */
void menu_node_build(menu_node *n, void *owner,
                     int32_t f_c, int32_t f_10,
                     int32_t f_14, int32_t f_18,
                     uint16_t n_children, const int32_t *config);

/* ─── FUN_0056c930 — per-frame menu-node transition update ────────────
 *
 * The title scene's "post-update" side effect (called every non-exiting
 * update frame from FUN_0056aea0's tail).  __thiscall with ECX = the scene's
 * owning sel_list; it walks every live entry (owner->entries[0..count)) and,
 * for each *active* node (node->field4 != 0), advances that node's transition
 * state by its mode (node->field_1c):
 *
 *   mode 1 — the steady input-gate fade.  Ramp node->field_54 toward 1000 by
 *            +50/frame while field_50 is set ("transitioning in"), or toward 0
 *            by -40/frame while clear ("out"); at <= 0 it tears the node down
 *            (0x56cc10).  **This is the title menu's mode** (menu_node_build
 *            sets field_1c=1, field_50=1, field_54=0): the ramp opens the
 *            menu-input gate, since menu_list_latch refuses to act until
 *            sub->ready (== this node's field_54) reaches 1000 (quirk #34).
 *            So the menu becomes navigable ~20 update frames after it spawns.
 *   mode 0 — node being dismissed (snap field_54 to 1000 if field_50 set, else
 *            unlink + free it from its parent's child array).
 *   mode 2 — node sliding to a target position (lerp its geometry, then snap
 *            field_54 to 1000 once settled).
 *
 * Only **mode 1** is ported here: it is the only mode the title flow exercises
 * (its single node is mode 1, and the ramp-out/teardown never fires because
 * field_50 stays 1 through the menu's life — the scene tears the menu down via
 * title_menu_teardown at phase 10, not through this path).  Modes 0 and 2 are
 * the submenu push/pop slide animations — they touch node geometry fields and
 * engine calls (0x49a340 / 0x56cc10 / 0x49a2f0 / 0x49a470) not yet modelled,
 * so they are skipped here with this documented seam; extend when submenus
 * land.  Faithful to 0x56c930's mode-1 arm + the owner loop / field4 gate.
 *
 * Pure: touches only each active mode-1 node's field_54.  NULL owner → no-op. */
void menu_owner_transition_step(sel_list *owner);

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
_Static_assert(sizeof(menu_cell_obj54)        == 0x54, "obj54 size");
_Static_assert(offsetof(menu_cell_obj54, field4)  == 0x04, "obj54.field4");
_Static_assert(offsetof(menu_cell_obj54, field46) == 0x46, "obj54.field46");
_Static_assert(offsetof(menu_cell_obj54, field48) == 0x48, "obj54.field48");
_Static_assert(offsetof(menu_cell_obj54, field4a) == 0x4a, "obj54.field4a");
_Static_assert(offsetof(menu_cell_obj54, field4c) == 0x4c, "obj54.field4c");
_Static_assert(offsetof(menu_cell_obj54, field50) == 0x50, "obj54.field50");
_Static_assert(sizeof(menu_cell_obj20)        == 0x20, "obj20 size");
_Static_assert(offsetof(menu_cell_obj20, field8)  == 0x08, "obj20.field8");
_Static_assert(offsetof(menu_cell_obj20, field14) == 0x14, "obj20.field14");
_Static_assert(offsetof(menu_cell_obj20, field18) == 0x18, "obj20.field18");
_Static_assert(offsetof(menu_cell_obj20, field1c) == 0x1c, "obj20.field1c");
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
_Static_assert(sizeof(menu_node)                 == 0x1b0, "menu_node size");
_Static_assert(offsetof(menu_node, field4)       == 0x04,  "node.field4");
_Static_assert(offsetof(menu_node, selected)     == 0x08,  "node.selected");
_Static_assert(offsetof(menu_node, field_1c)     == 0x1c,  "node.field_1c");
_Static_assert(offsetof(menu_node, field_50)     == 0x50,  "node.field_50");
_Static_assert(offsetof(menu_node, field_54)     == 0x54,  "node.field_54");
_Static_assert(offsetof(menu_node, children)     == 0x48,  "node.children");
_Static_assert(offsetof(menu_node, child_count)  == 0x4c,  "node.child_count");
_Static_assert(offsetof(menu_node, config)       == 0x5c,  "node.config");
_Static_assert(offsetof(menu_node, field_80)     == 0x80,  "node.field_80");
_Static_assert(offsetof(menu_node, ctrl_field_164) == 0x164, "node.ctrl_field_164");
_Static_assert(offsetof(menu_node, ctrl_list2)   == 0x170, "node.ctrl_list2");
_Static_assert(offsetof(menu_node, ctrl_rows)    == 0x17c, "node.ctrl_rows");
_Static_assert(offsetof(menu_node, color0)       == 0x180, "node.color0");
_Static_assert(offsetof(menu_node, label0)       == 0x188, "node.label0");
_Static_assert(offsetof(menu_node, field_1ac)    == 0x1ac, "node.field_1ac");
#endif

/* The per-child menu_ctrl_clear casts menu_node* → menu_ctrl*; guarantee the
 * read stays in-bounds of the node allocation on every build (32- and 64-bit). */
_Static_assert(sizeof(menu_node) >= sizeof(menu_ctrl),
               "menu_node must cover menu_ctrl for the child-clear cast");

#endif /* OPENSUMMONERS_MENU_LIST_H */
