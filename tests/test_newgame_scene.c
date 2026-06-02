/*
 * tests/test_newgame_scene.c — the new-game config scene run-loop model
 * (src/newgame_scene.c).
 *
 * newgame_menu's tests already prove the BUILD renders bit-exact; these prove
 * the LOOP around it: the focused-row tooltip selection, the gated cursor nav,
 * the pump-result(0xb/0xc/0xd)→action dispatch, and the picker's value-refill.
 */
#include "t.h"
#include "newgame_scene.h"
#include "newgame_menu.h"
#include "menu_list.h"
#include "glyph_text.h"

#include <string.h>

/* Reconstruct a cell's laid-out text from its glyph_buf records (ASCII path:
 * one byte per record).  Returns the length written. */
static int cell_text(const newgame_scene *s, int row, int col, char *out)
{
    glyph_buf *o = (glyph_buf *)s->grid.rows[row].cells[col].obj0;
    int n = 0;
    if (o) {
        glyph_record *r = (glyph_record *)o->records;
        for (int i = 0; i < (int)o->count; i++) out[n++] = r[i].ch[0];
    }
    out[n] = '\0';
    return n;
}

/* ─── init: focus on row 0, tooltip tracks the focused row ──────────────── */
int test_newgame_scene_init_focus_and_tooltip(void)
{
    newgame_scene s;
    newgame_scene_init(&s, NULL);                 /* defaults: Easy / On */
    char tip[512];

    T_ASSERT_EQ_I(newgame_scene_focused_row(&s), 0);
    T_ASSERT_EQ_I(newgame_scene_focused_kind(&s), 0);
    T_ASSERT_EQ_I(newgame_scene_focused_action(&s), NEWGAME_OPT_DIFFICULTY);

    newgame_scene_tooltip(&s, tip);
    T_ASSERT(strncmp(tip, "Allows you to configure game difficulty", 39) == 0);

    /* Row 1 = Auto-guard (option). */
    s.grid.list->cursor = 1;
    T_ASSERT_EQ_I(newgame_scene_focused_action(&s), NEWGAME_OPT_AUTO_GUARD);
    newgame_scene_tooltip(&s, tip);
    T_ASSERT(strncmp(tip, "Auto-guard will automatically", 29) == 0);

    /* Row 2 = Start Game (action button, kind 3) → the case-0x24 0x1e help. */
    s.grid.list->cursor = 2;
    T_ASSERT_EQ_I(newgame_scene_focused_kind(&s), 3);
    T_ASSERT_EQ_I(newgame_scene_focused_action(&s), NEWGAME_OPT_START_GAME);
    newgame_scene_tooltip(&s, tip);
    T_ASSERT(strncmp(tip, "Confirm options and begin the game.", 35) == 0);

    newgame_scene_clear(&s);
    return 0;
}

/* ─── nav: the gate must be open, then the cursor wraps over 3 rows ─────── */
int test_newgame_scene_nav_moves_cursor(void)
{
    newgame_scene s;
    newgame_scene_init(&s, NULL);

    /* Gate closed (ready 0, as built): the latch refuses to move the cursor. */
    T_ASSERT_EQ_I(menu_list_latch(&s.grid, /*next=*/1, /*now=*/0), 0);
    T_ASSERT_EQ_I(newgame_scene_focused_row(&s), 0);

    /* Open the input gate (the drive ramps this to 1000 over ~20 frames). */
    s.sub.ready = 1000;

    /* next walks 0→1→2 then wraps to the page top (type-0 linear wrap). */
    menu_list_latch(&s.grid, 1, 0);  T_ASSERT_EQ_I(newgame_scene_focused_row(&s), 1);
    menu_list_latch(&s.grid, 1, 0);  T_ASSERT_EQ_I(newgame_scene_focused_row(&s), 2);
    menu_list_latch(&s.grid, 1, 0);  T_ASSERT_EQ_I(newgame_scene_focused_row(&s), 0);

    /* prev from the top wraps back to the last row. */
    menu_list_latch(&s.grid, 0, 0);  T_ASSERT_EQ_I(newgame_scene_focused_row(&s), 2);

    newgame_scene_clear(&s);
    return 0;
}

/* ─── dispatch: MOVE re-renders, CONFIRM acts on the row, BACK exits ────── */
int test_newgame_scene_dispatch_actions(void)
{
    newgame_scene s;
    newgame_scene_init(&s, NULL);

    /* MOVE (0xd) just keeps the loop running. */
    T_ASSERT_EQ_I(newgame_scene_dispatch(&s, NEWGAME_PUMP_MOVE), NEWGAME_RUNNING);

    /* CONFIRM on an option row (row 0) → open its picker submenu. */
    s.grid.list->cursor = 0;
    T_ASSERT_EQ_I(newgame_scene_dispatch(&s, NEWGAME_PUMP_CONFIRM), NEWGAME_OPEN_PICKER);

    /* CONFIRM on the Start-Game button (row 2) → begin the game. */
    s.grid.list->cursor = 2;
    T_ASSERT_EQ_I(newgame_scene_dispatch(&s, NEWGAME_PUMP_CONFIRM), NEWGAME_START);

    /* BACK (0xb) → return to the title regardless of the focused row. */
    T_ASSERT_EQ_I(newgame_scene_dispatch(&s, NEWGAME_PUMP_BACK), NEWGAME_BACK);

    /* An unmodelled code keeps the loop alive (no fabricated outcome). */
    T_ASSERT_EQ_I(newgame_scene_dispatch(&s, 0x99), NEWGAME_RUNNING);

    newgame_scene_clear(&s);
    return 0;
}

/* ─── set_option: commit a value → its value column is re-laid ──────────── */
int test_newgame_scene_set_option_relays_value(void)
{
    newgame_scene s;
    newgame_scene_init(&s, NULL);                 /* difficulty 10, auto-guard 1 */
    char buf[64];

    cell_text(&s, 0, 1, buf);
    T_ASSERT(strcmp(buf, "1:Easy") == 0);
    cell_text(&s, 1, 1, buf);
    T_ASSERT(strcmp(buf, "On") == 0);

    /* Pick difficulty 30 (3:Hard) — the picker's commit effect. */
    T_ASSERT_EQ_I(newgame_scene_set_option(&s, NEWGAME_OPT_DIFFICULTY, 30), 1);
    T_ASSERT_EQ_I(s.settings.difficulty, 30);
    cell_text(&s, 0, 1, buf);
    T_ASSERT(strcmp(buf, "3:Hard") == 0);

    /* Toggle auto-guard off. */
    T_ASSERT_EQ_I(newgame_scene_set_option(&s, NEWGAME_OPT_AUTO_GUARD, 0), 1);
    T_ASSERT_EQ_I(s.settings.auto_guard, 0);
    cell_text(&s, 1, 1, buf);
    T_ASSERT(strcmp(buf, "Off") == 0);

    /* An id this menu does not carry refreshes nothing. */
    T_ASSERT_EQ_I(newgame_scene_set_option(&s, 0x99, 1), 0);

    newgame_scene_clear(&s);
    return 0;
}
