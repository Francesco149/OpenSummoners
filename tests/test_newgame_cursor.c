/*
 * tests/test_newgame_cursor.c — the new-game menu selection cursor
 * (src/newgame_cursor.c), the drooping gold vine that hangs from the box's
 * top-left corner toward the focused row (port of FUN_0048d940's type-1 arm).
 *
 * Ground truth: goldens/retail-newgame-box-cells.jsonl box_cell records —
 * type 1, base 16, frames {0,1,2,3} → frameSel 16..19, row-0 blit base (40,26)
 * for the menu box at (32,32).  These tests assert:
 *   (a) the frame id cycles 16→17→18→19 over the animation index;
 *   (b) the row-0 base matches the golden (40,26) and tracks the box origin;
 *   (c) each focused row steps the base down by the menu pitch (28);
 *   (d) render() resolves frame (base+anim) and keyed-blits at the row base,
 *       and degenerates safely on a NULL frame / NULL ops.
 */
#include "t.h"
#include "newgame_cursor.h"

#include <stdint.h>

/* ─── recording newgame_cursor_ops stub ──────────────────────────────── */
typedef struct { int frame_id, x, y; } rec_blit;
static rec_blit g_last;
static int      g_nblt;
static int      g_null_frame;   /* when set, frame() returns NULL */

static void *rec_frame(void *u, int frame_id)
{
    (void)u;
    g_last.frame_id = frame_id;
    if (g_null_frame) return NULL;
    return (void *)(intptr_t)(frame_id + 1);   /* non-NULL tagged handle */
}
static void rec_blt(void *u, void *frame, int x, int y)
{
    (void)u;
    g_nblt++;
    g_last.x = x; g_last.y = y;
    g_last.frame_id = (int)(intptr_t)frame - 1;
}

/* ─── (a) frame id cycles 16..19 ─────────────────────────────────────── */
int test_newgame_cursor_frame_cycles(void)
{
    T_ASSERT_EQ_I(16, newgame_cursor_frame_id(0));
    T_ASSERT_EQ_I(17, newgame_cursor_frame_id(1));
    T_ASSERT_EQ_I(18, newgame_cursor_frame_id(2));
    T_ASSERT_EQ_I(19, newgame_cursor_frame_id(3));
    T_ASSERT_EQ_I(16, newgame_cursor_frame_id(4));   /* wraps */
    T_ASSERT_EQ_I(18, newgame_cursor_frame_id(6));
    T_ASSERT_EQ_I(16, newgame_cursor_frame_id(-4));  /* negative safe */
    return 0;
}

/* ─── (b) row-0 base = golden (40,26), tracks box origin ─────────────── */
int test_newgame_cursor_base_row0_golden(void)
{
    int x = -1, y = -1;
    newgame_cursor_base(/*box*/32, 32, /*cursor*/0, /*sel2*/0, /*pitch*/28, &x, &y);
    T_ASSERT_EQ_I(40, x);    /* golden box_cell scrx base (pre frame offset) */
    T_ASSERT_EQ_I(26, y);
    /* tracks the box origin (offset 8,-6) */
    newgame_cursor_base(100, 200, 0, 0, 28, &x, &y);
    T_ASSERT_EQ_I(108, x);
    T_ASSERT_EQ_I(194, y);
    return 0;
}

/* ─── (c) each row steps down by the pitch; page-top subtracts ───────── */
int test_newgame_cursor_row_pitch(void)
{
    int x, y;
    newgame_cursor_base(32, 32, /*cursor*/1, 0, 28, &x, &y);
    T_ASSERT_EQ_I(40, x);
    T_ASSERT_EQ_I(26 + 28, y);
    newgame_cursor_base(32, 32, /*cursor*/2, 0, 28, &x, &y);
    T_ASSERT_EQ_I(26 + 56, y);
    /* page-top (sel2) shifts the visible origin: (cursor-sel2) */
    newgame_cursor_base(32, 32, /*cursor*/3, /*sel2*/2, 28, &x, &y);
    T_ASSERT_EQ_I(26 + 28, y);
    return 0;
}

/* ─── (d) render: resolves frame, blits at row base; null-safe ───────── */
int test_newgame_cursor_render_composes(void)
{
    g_nblt = 0; g_null_frame = 0;
    newgame_cursor_ops ops = { rec_frame, rec_blt, NULL };

    /* row 0, anim idx 1 → frame 17 at (40,26) */
    newgame_cursor_render(&ops, 32, 32, /*cursor*/0, /*sel2*/0, /*pitch*/28, /*anim*/1);
    T_ASSERT_EQ_I(1, g_nblt);
    T_ASSERT_EQ_I(17, g_last.frame_id);
    T_ASSERT_EQ_I(40, g_last.x);
    T_ASSERT_EQ_I(26, g_last.y);

    /* row 2, anim idx 3 → frame 19 at (40, 26+56) */
    newgame_cursor_render(&ops, 32, 32, /*cursor*/2, /*sel2*/0, /*pitch*/28, /*anim*/3);
    T_ASSERT_EQ_I(2, g_nblt);
    T_ASSERT_EQ_I(19, g_last.frame_id);
    T_ASSERT_EQ_I(82, g_last.y);

    /* NULL frame → no blit */
    g_null_frame = 1;
    newgame_cursor_render(&ops, 32, 32, 0, 0, 28, 0);
    T_ASSERT_EQ_I(2, g_nblt);

    /* NULL ops fields → no crash, no blit */
    newgame_cursor_render(NULL, 32, 32, 0, 0, 28, 0);
    newgame_cursor_ops bad = { NULL, rec_blt, NULL };
    newgame_cursor_render(&bad, 32, 32, 0, 0, 28, 0);
    T_ASSERT_EQ_I(2, g_nblt);
    return 0;
}
