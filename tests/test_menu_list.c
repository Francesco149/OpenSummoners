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
