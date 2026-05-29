/*
 * tests/test_menu_list.c — host-side tests for src/menu_list.c.
 *
 * Checkpoint 4a: the scroll-into-view leaf FUN_004192b0.  The nav engine
 * (FUN_0043ca40) and latch (FUN_0043ce50) tests are appended as those
 * land.  Each test lays out a menu_ctrl + list header in host memory and
 * checks the cursor / page-top arithmetic against hand-derived values.
 */
#include "../src/menu_list.h"
#include "t.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Wire a controller to a header and set the three fields scroll-into-view
 * reads (stride/cursor) plus the page-top it may rewrite (sel2). */
static void mk(menu_ctrl *c, menu_list_hdr *h,
               int32_t stride, int32_t cursor, int32_t sel2)
{
    memset(c, 0, sizeof *c);
    memset(h, 0, sizeof *h);
    h->stride = stride;
    h->cursor = cursor;
    h->sel2   = sel2;
    c->list   = h;
}

/* ─── FUN_004192b0 menu_list_scroll_into_view ───────────────────────── */

/* Cursor on a later page → page-top recomputed to floor(c/s)*s, moved. */
int test_menu_scroll_recomputes_page_top(void)
{
    menu_ctrl c; menu_list_hdr h;
    mk(&c, &h, 4, 6, 0);                 /* floor(6/4)*4 = 4 */
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 1);
    T_ASSERT_EQ_I(h.sel2, 4);
    return 0;
}

/* Already in view (page-top matches) → no move, returns 0, sel2 untouched. */
int test_menu_scroll_noop_when_in_view(void)
{
    menu_ctrl c; menu_list_hdr h;
    mk(&c, &h, 4, 6, 4);                 /* page-top already 4 */
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 0);
    T_ASSERT_EQ_I(h.sel2, 4);
    return 0;
}

/* Cursor scrolled back above the stored page-top → recompute downward. */
int test_menu_scroll_moves_back_up(void)
{
    menu_ctrl c; menu_list_hdr h;
    mk(&c, &h, 4, 3, 8);                 /* floor(3/4)*4 = 0 */
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 1);
    T_ASSERT_EQ_I(h.sel2, 0);
    return 0;
}

/* stride == 1: every index is its own page, page-top == cursor. */
int test_menu_scroll_stride_one_tracks_cursor(void)
{
    menu_ctrl c; menu_list_hdr h;
    mk(&c, &h, 1, 5, 0);
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 1);
    T_ASSERT_EQ_I(h.sel2, 5);
    /* second call is a no-op now that the page-top tracks the cursor */
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 0);
    T_ASSERT_EQ_I(h.sel2, 5);
    return 0;
}

/* Cursor exactly on a page boundary → that boundary is the page-top. */
int test_menu_scroll_boundary_cursor(void)
{
    menu_ctrl c; menu_list_hdr h;
    mk(&c, &h, 4, 4, 0);                 /* floor(4/4)*4 = 4 */
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 1);
    T_ASSERT_EQ_I(h.sel2, 4);
    return 0;
}

/* Cursor 0 → page-top 0 (and already 0 → no move). */
int test_menu_scroll_cursor_zero(void)
{
    menu_ctrl c; menu_list_hdr h;
    mk(&c, &h, 4, 0, 0);
    T_ASSERT_EQ_I(menu_list_scroll_into_view(&c), 0);
    T_ASSERT_EQ_I(h.sel2, 0);
    return 0;
}

/* ─── FUN_0043ca40 menu_list_nav ────────────────────────────────────── */

static void mkn(menu_ctrl *c, menu_list_hdr *h, int32_t type, int32_t stride,
                int32_t count, int32_t cursor, int32_t sel2)
{
    memset(c, 0, sizeof *c);
    memset(h, 0, sizeof *h);
    h->type = type; h->stride = stride; h->count = count;
    h->cursor = cursor; h->sel2 = sel2;
    c->list = h;
}

/* ── type 0: linear list that wraps within the page ── */

int test_nav_t0_prev_decrements(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 3, 0);          /* cursor != sel2 → cursor-1 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 0, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 2);
    return 0;
}

int test_nav_t0_prev_wraps_to_page_bottom(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 0, 0);          /* cursor==sel2, iVar7-1=4 <= 9 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 0, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 4);           /* sel2 + stride - 1 */
    return 0;
}

int test_nav_t0_prev_wrap_clamps_to_count(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 3, 0, 0);           /* iVar7-1=4 > count-1=2 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 0, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 2);           /* count - 1 */
    return 0;
}

int test_nav_t0_next_increments(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);          /* maxi = 4; cursor 2 != 4 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 3);
    return 0;
}

int test_nav_t0_next_wraps_to_page_top(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 4, 0);          /* cursor == maxi(4) → sel2 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 0);
    return 0;
}

/* ── type 2: grid, prev/next stay within a row, page recompute in tail ── */

int test_nav_t2_next_within_row(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 3, 3);
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 4);
    T_ASSERT_EQ_I(h.sel2, 3);             /* page unchanged */
    return 0;
}

int test_nav_t2_next_wraps_in_row(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 5, 3);           /* at row end → row start */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 3);
    return 0;
}

int test_nav_t2_prev_within_row(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 4, 3);
    T_ASSERT_EQ_I(menu_list_nav(&c, 0, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 3);
    return 0;
}

int test_nav_t2_prev_wraps_to_row_end(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 3, 3);           /* at row start → row end */
    T_ASSERT_EQ_I(menu_list_nav(&c, 0, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 5);
    return 0;
}

int test_nav_t2_page_down_scrolls(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 1, 0);
    T_ASSERT_EQ_I(menu_list_nav(&c, 3, 0), 2);   /* page changed */
    T_ASSERT_EQ_I(h.cursor, 4);
    T_ASSERT_EQ_I(h.sel2, 3);
    return 0;
}

int test_nav_t2_page_up_scrolls(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 4, 3);
    T_ASSERT_EQ_I(menu_list_nav(&c, 2, 0), 2);
    T_ASSERT_EQ_I(h.cursor, 1);
    T_ASSERT_EQ_I(h.sel2, 0);
    return 0;
}

int test_nav_t2_page_up_underflow_clamps(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 2, 3, 9, 1, 0);           /* cursor-stride < 0 path */
    T_ASSERT_EQ_I(menu_list_nav(&c, 2, 0), 2);
    T_ASSERT_EQ_I(h.cursor, 7);           /* (8/3)*3 + 1%3 = 7 */
    T_ASSERT_EQ_I(h.sel2, 6);
    return 0;
}

/* ── type 3: list whose page-top (sel2) trails the cursor ── */

int test_nav_t3_next_drags_page(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 3, 4, 10, 3, 0);          /* iVar7=4 <= new cursor 4 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 4);
    T_ASSERT_EQ_I(h.sel2, 1);             /* cursor - stride + 1 */
    return 0;
}

int test_nav_t3_next_no_scroll(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 3, 4, 10, 0, 0);          /* iVar7=4 > new cursor 1 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 1);
    T_ASSERT_EQ_I(h.sel2, 0);
    return 0;
}

int test_nav_t3_next_wraps_to_zero(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 3, 4, 10, 9, 6);          /* cursor+1 == count → %count */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 0);
    T_ASSERT_EQ_I(h.sel2, 0);             /* cursor < sel2 → sel2 = cursor */
    return 0;
}

int test_nav_t3_prev_wraps_to_tail(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 3, 4, 10, 0, 0);          /* cursor-1 < 0 → count-1 */
    T_ASSERT_EQ_I(menu_list_nav(&c, 0, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 9);
    T_ASSERT_EQ_I(h.sel2, 6);             /* iVar7(4) <= 9 → 9-4+1 */
    return 0;
}

/* ── cancel / confirm / no-op ── */

int test_nav_cancel_latches_three(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    T_ASSERT_EQ_I(menu_list_nav(&c, 9, 0), 3);
    T_ASSERT_EQ_I(c.action, 3);
    T_ASSERT_EQ_I(h.cursor, 2);           /* cancel doesn't move cursor */
    return 0;
}

int test_nav_confirm_latches_four(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    T_ASSERT_EQ_I(menu_list_nav(&c, 10, 0), 4);
    T_ASSERT_EQ_I(c.action, 4);
    return 0;
}

int test_nav_dir8_is_noop(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    T_ASSERT_EQ_I(menu_list_nav(&c, 8, 0), 0);
    T_ASSERT_EQ_I(h.cursor, 2);
    return 0;
}

int test_nav_out_of_range_is_noop(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    T_ASSERT_EQ_I(menu_list_nav(&c, 11, 0), 0);
    T_ASSERT_EQ_I(h.cursor, 2);
    return 0;
}

int test_nav_count_below_two_no_move(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 1, 0, 0);           /* count < 2 → break, tail noop */
    T_ASSERT_EQ_I(menu_list_nav(&c, 1, 0), 0);
    T_ASSERT_EQ_I(h.cursor, 0);
    return 0;
}

/* ── auto-repeat (dir 4/6 arm → wait → fire; dir 5/7 reset) ── */

int test_nav_repeat_a_arms_then_idles(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    /* first press arms a 300ms deadline, no move */
    T_ASSERT_EQ_I(menu_list_nav(&c, 4, 1000), 0);
    T_ASSERT_EQ_U(h.repeat_a, 1300);
    T_ASSERT_EQ_I(h.cursor, 2);
    /* before the deadline: still no move */
    T_ASSERT_EQ_I(menu_list_nav(&c, 4, 1200), 0);
    T_ASSERT_EQ_I(h.cursor, 2);
    return 0;
}

int test_nav_repeat_a_fires_as_next(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    h.repeat_a = 1300;                    /* armed */
    /* now >= deadline → fires 'next' (dir 1) and re-arms a 100ms repeat */
    T_ASSERT_EQ_I(menu_list_nav(&c, 4, 1300), 1);
    T_ASSERT_EQ_I(h.cursor, 3);
    T_ASSERT_EQ_U(h.repeat_a, 1400);
    return 0;
}

int test_nav_repeat_a_reset(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    h.repeat_a = 1300;
    T_ASSERT_EQ_I(menu_list_nav(&c, 5, 9999), 0);
    T_ASSERT_EQ_U(h.repeat_a, 0);
    return 0;
}

int test_nav_repeat_b_arms_and_fires_as_prev(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 3, 0);
    T_ASSERT_EQ_I(menu_list_nav(&c, 6, 500), 0);   /* arm */
    T_ASSERT_EQ_U(h.repeat_b, 800);
    /* not yet expired */
    T_ASSERT_EQ_I(menu_list_nav(&c, 6, 700), 0);
    T_ASSERT_EQ_I(h.cursor, 3);
    /* fires 'prev' (dir 0) and re-arms */
    T_ASSERT_EQ_I(menu_list_nav(&c, 6, 900), 1);
    T_ASSERT_EQ_I(h.cursor, 2);
    T_ASSERT_EQ_U(h.repeat_b, 1000);
    return 0;
}

int test_nav_repeat_b_reset(void)
{
    menu_ctrl c; menu_list_hdr h;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    h.repeat_b = 800;
    T_ASSERT_EQ_I(menu_list_nav(&c, 7, 9999), 0);
    T_ASSERT_EQ_U(h.repeat_b, 0);
    return 0;
}

/* ─── FUN_0043ce50 menu_list_latch ──────────────────────────────────── */

/* ── the input "ready" gate ── */

int test_latch_gate_blocks_when_not_ready(void)
{
    menu_ctrl c; menu_list_hdr h; menu_input_sub sub;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    memset(&sub, 0, sizeof sub);
    sub.enabled = 1; sub.ready = 999;     /* not 1000 */
    c.sub = &sub; c.mode = 1;
    T_ASSERT_EQ_I(menu_list_latch(&c, 1, 0), 0);
    T_ASSERT_EQ_I(h.cursor, 2);           /* nav never ran */
    return 0;
}

int test_latch_gate_blocks_when_disabled(void)
{
    menu_ctrl c; menu_list_hdr h; menu_input_sub sub;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    memset(&sub, 0, sizeof sub);
    sub.enabled = 0; sub.ready = 1000;
    c.sub = &sub; c.mode = 1;
    T_ASSERT_EQ_I(menu_list_latch(&c, 1, 0), 0);
    T_ASSERT_EQ_I(h.cursor, 2);
    return 0;
}

/* ── mode 1: forward to the cursor-nav engine ── */

int test_latch_mode1_forwards_to_nav(void)
{
    menu_ctrl c; menu_list_hdr h; menu_input_sub sub;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    memset(&sub, 0, sizeof sub);
    sub.enabled = 1; sub.ready = 1000;
    c.sub = &sub; c.mode = 1;
    /* dir 1 = next → cursor 2→3, nav returns 1 */
    T_ASSERT_EQ_I(menu_list_latch(&c, 1, 0), 1);
    T_ASSERT_EQ_I(h.cursor, 3);
    return 0;
}

int test_latch_unknown_mode_is_noop(void)
{
    menu_ctrl c; menu_list_hdr h; menu_input_sub sub;
    mkn(&c, &h, 0, 5, 10, 2, 0);
    memset(&sub, 0, sizeof sub);
    sub.enabled = 1; sub.ready = 1000;
    c.sub = &sub; c.mode = 3;             /* neither 1 nor 2 */
    T_ASSERT_EQ_I(menu_list_latch(&c, 1, 0), 0);
    T_ASSERT_EQ_I(h.cursor, 2);
    return 0;
}

/* ── mode 2, submode 0: flag-driven confirm ── */

static void mk_confirm(menu_ctrl *c, menu_input_sub *sub, confirm_list *cl,
                       int32_t submode)
{
    memset(c, 0, sizeof *c);
    memset(sub, 0, sizeof *sub);
    memset(cl, 0, sizeof *cl);
    sub->enabled = 1; sub->ready = 1000;
    c->sub = sub; c->mode = 2; c->list2 = cl;
    cl->submode = submode;
}

int test_latch_mode2_sub0_ack_clears_flag18(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    mk_confirm(&c, &sub, &cl, 0);
    cl.flag18 = 1; cl.flag14 = 1;
    T_ASSERT_EQ_I(menu_list_latch(&c, 9, 0), 6);
    T_ASSERT_EQ_I(cl.flag18, 0);          /* cleared */
    T_ASSERT_EQ_I(c.action, 0);           /* flag18 path doesn't latch action */
    return 0;
}

int test_latch_mode2_sub0_content_latches_eight(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    mk_confirm(&c, &sub, &cl, 0);
    cl.flag18 = 0; cl.flag14 = 1;
    T_ASSERT_EQ_I(menu_list_latch(&c, 9, 0), 8);
    T_ASSERT_EQ_I(c.action, 8);
    return 0;
}

int test_latch_mode2_sub0_no_flags_returns_zero(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    mk_confirm(&c, &sub, &cl, 0);
    cl.flag18 = 0; cl.flag14 = 0;
    T_ASSERT_EQ_I(menu_list_latch(&c, 9, 0), 0);
    return 0;
}

int test_latch_mode2_sub0_out_of_range_returns_zero(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    mk_confirm(&c, &sub, &cl, 0);
    cl.flag18 = 1;                        /* would fire if in range */
    T_ASSERT_EQ_I(menu_list_latch(&c, 7, 0), 0);   /* dir < 8 */
    T_ASSERT_EQ_I(cl.flag18, 1);          /* untouched */
    return 0;
}

/* ── mode 2, submode 1: reveal-then-dismiss scrolling message ── */

static void wire_cap(confirm_list *cl, confirm_src *src, confirm_caprec *rec,
                     uint16_t cap)
{
    rec->cap = cap;
    src->caprec = rec;
    cl->src = src;
}

int test_latch_mode2_sub1_reveals_all(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    confirm_src src; confirm_caprec rec;
    mk_confirm(&c, &sub, &cl, 1);
    wire_cap(&cl, &src, &rec, 5);
    cl.pos = 2;                            /* cap(5) > pos(2) → reveal-all */
    T_ASSERT_EQ_I(menu_list_latch(&c, 9, 0), 6);
    T_ASSERT_EQ_I(cl.pos, 5);              /* fast-forwarded to cap */
    T_ASSERT_EQ_I(c.action, 6);
    return 0;
}

int test_latch_mode2_sub1_dismisses_at_cap(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    confirm_src src; confirm_caprec rec;
    mk_confirm(&c, &sub, &cl, 1);
    wire_cap(&cl, &src, &rec, 5);
    cl.pos = 5;                            /* cap(5) <= pos(5) → dismiss */
    T_ASSERT_EQ_I(menu_list_latch(&c, 9, 0), 8);
    T_ASSERT_EQ_I(c.action, 8);
    T_ASSERT_EQ_I(cl.pos, 5);              /* unchanged */
    return 0;
}

int test_latch_mode2_sub1_blocked_once_dismissed(void)
{
    menu_ctrl c; menu_input_sub sub; confirm_list cl;
    confirm_src src; confirm_caprec rec;
    mk_confirm(&c, &sub, &cl, 1);
    wire_cap(&cl, &src, &rec, 5);
    cl.pos = 2; c.action = 8;              /* already dismissed → gate fails */
    T_ASSERT_EQ_I(menu_list_latch(&c, 9, 0), 0);
    T_ASSERT_EQ_I(cl.pos, 2);              /* untouched */
    return 0;
}

/* ─── FUN_0040f5c0 menu_ctrl_build / FUN_0040e0c0 menu_ctrl_clear ─────
 *
 * These allocate heap geometry; every test clears the controller before
 * returning so LeakSanitizer (on at process exit) verifies the teardown
 * frees exactly what the constructor allocated. */

/* The title menu's own parameters: 6 rows, 1 cell/entry, stride 6, linear. */
int test_build_header_fields(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_build(&c, 0, 0, 6, 1, 6, 0);
    T_ASSERT(c.list != NULL);
    T_ASSERT_EQ_I(c.list->type, 0);
    T_ASSERT_EQ_I(c.list->alloc_a, 6);
    T_ASSERT_EQ_I(c.list->alloc_b, 1);
    T_ASSERT_EQ_I(c.list->stride, 6);
    T_ASSERT_EQ_I(c.list->count, 0);
    T_ASSERT_EQ_I(c.list->cursor, 0);
    T_ASSERT_EQ_I(c.list->sel2, 0);
    T_ASSERT_EQ_U(c.list->repeat_a, 0);
    T_ASSERT_EQ_U(c.list->repeat_b, 0);
    /* controller scalars seeded by the ctor */
    T_ASSERT_EQ_I(c.mode, 1);
    T_ASSERT_EQ_I(c.field_c, 0);
    T_ASSERT_EQ_I(c.field_10, 0);
    T_ASSERT_EQ_I(c.field_20, 0);
    T_ASSERT_EQ_I(c.field_84, 0);
    T_ASSERT_EQ_I(c.field_140, 0);
    menu_ctrl_clear(&c);
    return 0;
}

/* Non-zero params land in the right scalar / header slots. */
int test_build_params_carried(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_build(&c, 0x11, 0x22, 4, 2, 3, 2);
    T_ASSERT_EQ_I(c.field_c, 0x11);
    T_ASSERT_EQ_I(c.field_10, 0x22);
    T_ASSERT_EQ_I(c.list->alloc_a, 4);
    T_ASSERT_EQ_I(c.list->alloc_b, 2);
    T_ASSERT_EQ_I(c.list->stride, 3);
    T_ASSERT_EQ_I(c.list->type, 2);
    menu_ctrl_clear(&c);
    return 0;
}

/* Both arrays and every per-row cell array are allocated and zeroed. */
int test_build_allocates_zeroed_grid(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_build(&c, 0, 0, 3, 2, 3, 0);
    T_ASSERT(c.rows != NULL);
    T_ASSERT(c.entries != NULL);
    for (int r = 0; r < 3; r++) {
        T_ASSERT_EQ_I(c.rows[r].field0, 0);
        T_ASSERT_EQ_I(c.rows[r].action, 0);
        T_ASSERT(c.rows[r].cells != NULL);
        for (int k = 0; k < 2; k++) {
            T_ASSERT_EQ_P(c.rows[r].cells[k].obj0, NULL);
            T_ASSERT_EQ_P(c.rows[r].cells[k].obj54, NULL);
            T_ASSERT_EQ_P(c.rows[r].cells[k].obj20, NULL);
            T_ASSERT_EQ_I(c.rows[r].cells[k].field_c, 0);
        }
    }
    menu_ctrl_clear(&c);
    return 0;
}

/* Per-column entries get pos = index*0x20 and extent = 0x20; rest zeroed. */
int test_build_entries_stamped(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_build(&c, 0, 0, 2, 3, 2, 0);
    for (int e = 0; e < 3; e++) {
        T_ASSERT_EQ_I(c.entries[e].pos, e * 0x20);
        T_ASSERT_EQ_I(c.entries[e].extent, 0x20);
        T_ASSERT_EQ_I(c.entries[e].field4, 0);
        T_ASSERT_EQ_I(c.entries[e].field_c, 0);
        T_ASSERT_EQ_I(c.entries[e].field_20, 0);
    }
    menu_ctrl_clear(&c);
    return 0;
}

/* Clear on a fresh (all-zero) controller is a no-op (every guard fails). */
int test_clear_fresh_is_noop(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_clear(&c);
    T_ASSERT_EQ_P(c.list, NULL);
    T_ASSERT_EQ_P(c.rows, NULL);
    T_ASSERT_EQ_P(c.entries, NULL);
    T_ASSERT_EQ_P(c.list2, NULL);
    return 0;
}

/* Rebuilding recycles the slot: the ctor clears the prior geometry (ASan
 * would flag a leak / double-free) and the new dims take effect. */
int test_build_rebuild_recycles(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_build(&c, 0, 0, 6, 1, 6, 0);
    c.list->count = 4;                     /* simulate a populated list */
    menu_ctrl_build(&c, 0, 0, 4, 2, 4, 2); /* rebuild with new dims */
    T_ASSERT_EQ_I(c.list->alloc_a, 4);
    T_ASSERT_EQ_I(c.list->alloc_b, 2);
    T_ASSERT_EQ_I(c.list->type, 2);
    T_ASSERT_EQ_I(c.list->count, 0);       /* fresh header, count reset */
    menu_ctrl_clear(&c);
    return 0;
}

/* Clear frees each cell's three lazily-built sub-objects.  obj0 points at
 * a block whose first word is itself an owned pointer (the 0x411f40 grid
 * object); clear frees *obj0 then obj0.  ASan verifies no leak/no UAF. */
int test_clear_frees_cell_subobjects(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    menu_ctrl_build(&c, 0, 0, 1, 1, 1, 0);
    menu_cell *cell = &c.rows[0].cells[0];
    cell->obj0 = malloc(sizeof(void *));
    *(void **)cell->obj0 = malloc(8);      /* the owned inner pointer */
    cell->obj54 = malloc(0x54);
    cell->obj20 = malloc(0x20);
    menu_ctrl_clear(&c);                    /* must free all four blocks */
    return 0;
}

/* Clear also frees the +0x164 owned buffer and NULLs it. */
int test_clear_frees_field164(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    c.field_164 = malloc(0x10);
    menu_ctrl_clear(&c);
    T_ASSERT_EQ_P(c.field_164, NULL);
    return 0;
}

/* Clear tears down the whole confirm-list source graph: list2 → src →
 * {owned0, owned8, caprec → owned0}, then src, then list2 (NULLed). */
int test_clear_frees_confirm_graph(void)
{
    menu_ctrl c;
    memset(&c, 0, sizeof c);
    confirm_list   *cl  = (confirm_list   *)calloc(1, sizeof *cl);
    confirm_src    *src = (confirm_src    *)calloc(1, sizeof *src);
    confirm_caprec *rec = (confirm_caprec *)calloc(1, sizeof *rec);
    rec->owned0 = malloc(8);
    src->owned0 = malloc(8);
    src->owned8 = malloc(8);
    src->caprec = rec;
    cl->src = src;
    c.list2 = cl;
    menu_ctrl_clear(&c);
    T_ASSERT_EQ_P(c.list2, NULL);
    return 0;
}
