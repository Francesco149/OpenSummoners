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
