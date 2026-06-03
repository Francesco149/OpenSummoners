/*
 * tests/test_newgame_picker.c — the new-game option PICKER submenu
 * (src/newgame_picker.c, the model of FUN_00567ba0's default arm).
 *
 * Two layers:
 *   1. newgame_picker_values (FUN_00568320) — the value-code lists for the
 *      two new-game options, including difficulty's unlock-gated 5th choice.
 *   2. the picker run-loop model — build the value grid, seek the cursor to the
 *      current value (FUN_00419900), and nav/commit/cancel through the pump
 *      result codes (567ba0.c:237-251).
 */
#include "t.h"
#include "newgame_picker.h"
#include "newgame_scene.h"   /* NEWGAME_PUMP_* */
#include "menu_list.h"

#include <string.h>

/* ─── 1. value lists (FUN_00568320) ─────────────────────────────────────── */

int test_picker_values_difficulty_locked(void)
{
    int32_t v[NEWGAME_PICKER_MAX_VALUES];
    int32_t n = newgame_picker_values(NEWGAME_OPT_DIFFICULTY, v, /*unlock=*/0);
    T_ASSERT_EQ_I(n, 4);
    T_ASSERT_EQ_I(v[0], 10);   /* 1:Easy   */
    T_ASSERT_EQ_I(v[1], 20);   /* 2:Normal */
    T_ASSERT_EQ_I(v[2], 30);   /* 3:Hard   */
    T_ASSERT_EQ_I(v[3], 40);   /* 4:Expert */
    return 0;
}

int test_picker_values_difficulty_unlocked(void)
{
    int32_t v[NEWGAME_PICKER_MAX_VALUES];
    int32_t n = newgame_picker_values(NEWGAME_OPT_DIFFICULTY, v, /*unlock=*/1);
    T_ASSERT_EQ_I(n, 5);
    T_ASSERT_EQ_I(v[3], 40);
    T_ASSERT_EQ_I(v[4], 50);   /* 5:Nightmare — the unlock-gated choice */
    return 0;
}

int test_picker_values_auto_guard(void)
{
    int32_t v[NEWGAME_PICKER_MAX_VALUES];
    int32_t n = newgame_picker_values(NEWGAME_OPT_AUTO_GUARD, v, 0);
    T_ASSERT_EQ_I(n, 2);
    T_ASSERT_EQ_I(v[0], 0);    /* Off */
    T_ASSERT_EQ_I(v[1], 1);    /* On  */
    return 0;
}

int test_picker_values_unported_id_is_empty(void)
{
    int32_t v[NEWGAME_PICKER_MAX_VALUES];
    /* 0x1e (Start Game) is a kind-3 button, not an option — no picker. */
    T_ASSERT_EQ_I(newgame_picker_values(NEWGAME_OPT_START_GAME, v, 0), 0);
    T_ASSERT_EQ_I(newgame_picker_values(0x99, v, 0), 0);
    return 0;
}

/* ─── 2. the picker run-loop model ──────────────────────────────────────── */

/* Open the gate so menu_list_latch (and thus the model's nav) acts. */
static void open_gate(newgame_picker *p)
{
    p->sub.ready   = 1000;
    p->sub.enabled = 1;
}

int test_picker_init_seeks_current_value(void)
{
    /* Auto-guard default is On (1) → the cursor should open on row 1, not 0. */
    newgame_picker p;
    int ok = newgame_picker_init(&p, NEWGAME_OPT_AUTO_GUARD, /*current=*/1, 0);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_I(p.count, 2);
    T_ASSERT_EQ_I(p.grid.list->cursor, 1);
    T_ASSERT_EQ_I(newgame_picker_focused_value(&p), 1);
    newgame_picker_clear(&p);

    /* Difficulty 3:Hard (30) → row 2 of {10,20,30,40}. */
    ok = newgame_picker_init(&p, NEWGAME_OPT_DIFFICULTY, /*current=*/30, 0);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_I(p.grid.list->cursor, 2);
    T_ASSERT_EQ_I(newgame_picker_focused_value(&p), 30);
    newgame_picker_clear(&p);
    return 0;
}

int test_picker_init_no_choices_returns_zero(void)
{
    newgame_picker p;
    T_ASSERT_EQ_I(newgame_picker_init(&p, NEWGAME_OPT_START_GAME, 0, 0), 0);
    return 0;
}

int test_picker_nav_then_commit(void)
{
    /* Open the difficulty picker on Easy (10, row 0); move down twice to Hard
     * (30, row 2); confirm → COMMIT with chosen == 30. */
    newgame_picker p;
    newgame_picker_init(&p, NEWGAME_OPT_DIFFICULTY, /*current=*/10, 0);
    open_gate(&p);

    /* MOVE keeps it running and the focused value tracks the cursor. */
    menu_list_latch(&p.grid, 1, 0);   /* nav: next (down) */
    T_ASSERT_EQ_I(newgame_picker_dispatch(&p, NEWGAME_PUMP_MOVE), NEWGAME_PICKER_RUNNING);
    T_ASSERT_EQ_I(newgame_picker_focused_value(&p), 20);

    menu_list_latch(&p.grid, 1, 0);
    T_ASSERT_EQ_I(newgame_picker_dispatch(&p, NEWGAME_PUMP_MOVE), NEWGAME_PICKER_RUNNING);
    T_ASSERT_EQ_I(newgame_picker_focused_value(&p), 30);

    /* CONFIRM → COMMIT, chosen == the focused value. */
    T_ASSERT_EQ_I(newgame_picker_dispatch(&p, NEWGAME_PUMP_CONFIRM), NEWGAME_PICKER_COMMIT);
    T_ASSERT_EQ_I(p.chosen, 30);
    newgame_picker_clear(&p);
    return 0;
}

int test_picker_back_cancels(void)
{
    newgame_picker p;
    newgame_picker_init(&p, NEWGAME_OPT_AUTO_GUARD, /*current=*/1, 0);
    open_gate(&p);
    /* BACK → CANCEL, no value committed. */
    T_ASSERT_EQ_I(newgame_picker_dispatch(&p, NEWGAME_PUMP_BACK), NEWGAME_PICKER_CANCEL);
    newgame_picker_clear(&p);
    return 0;
}
