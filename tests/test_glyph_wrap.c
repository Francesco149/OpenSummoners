/*
 * tests/test_glyph_wrap.c — the tooltip text-node word-wrap (src/glyph_wrap.c,
 * the FUN_0040e5e0 ASCII path + FUN_0040f040 `%n` parse).
 *
 * The pixel ground truth is the captured new-game golden: with the difficulty
 * option row focused, retail's tooltip text node draws TWO rows at y=416/444,
 * both left-aligned at x=72 (7px/glyph) —
 *   row 0: "Allows you to configure game difficulty. On harder difficulties, "
 *   row 1: "enemies are stronger and experience gain more rapid."
 * (reconstructed from retail-newgame-config-textout.jsonl, the 0x3e537d main
 * colour draws at y=416/444).  The break is NOT a `%n` — the source string has
 * none — it is the greedy word-wrap at the node's configured width (0x44 = 68
 * glyph-columns, the FUN_0040dee0 ctor arg).  This test asserts the port
 * reproduces that split, plus the `%n` forced-break path the Start-Game button
 * tooltip uses.
 */
#include "t.h"
#include "glyph_wrap.h"

#include <string.h>

/* The configured tooltip wrap width (FUN_0040dee0 param_3 = 0x44). */
#define TT_W 68

/* The difficulty option tooltip = FUN_00566850 case 3 verbatim (no escapes). */
static const char *DIFFICULTY =
    "Allows you to configure game difficulty. On harder difficulties, "
    "enemies are stronger and experience gain more rapid.";

int test_glyph_wrap_difficulty_splits_65_52(void)
{
    glyph_wrap_result r;
    glyph_wrap_layout(DIFFICULTY, TT_W, &r);

    T_ASSERT_EQ_I(r.row_count, 2);
    /* exact bytes incl. the trailing space the golden draws on row 0 */
    if (strcmp(r.rows[0],
            "Allows you to configure game difficulty. On harder difficulties, ") != 0)
        T_FAIL("row0 -> '%s'", r.rows[0]);
    if (strcmp(r.rows[1],
            "enemies are stronger and experience gain more rapid.") != 0)
        T_FAIL("row1 -> '%s'", r.rows[1]);
    /* the golden's per-row glyph counts (= column extents at 7px) */
    T_ASSERT_EQ_I((int)strlen(r.rows[0]), 65);
    T_ASSERT_EQ_I((int)strlen(r.rows[1]), 52);
    return 0;
}

/* The Start-Game button tooltip carries an explicit `%n` (FUN_00564780's kind-3
 * 0x1e arm, case 0x24) — the only forced break in the new-game scene. */
int test_glyph_wrap_forced_break_on_percent_n(void)
{
    glyph_wrap_result r;
    glyph_wrap_layout(
        "Confirm options and begin the game."
        "%n(Options can be altered after the game starts.)", TT_W, &r);

    T_ASSERT_EQ_I(r.row_count, 2);
    if (strcmp(r.rows[0], "Confirm options and begin the game.") != 0)
        T_FAIL("row0 -> '%s'", r.rows[0]);
    if (strcmp(r.rows[1], "(Options can be altered after the game starts.)") != 0)
        T_FAIL("row1 -> '%s'", r.rows[1]);
    return 0;
}

int test_glyph_wrap_short_string_one_row(void)
{
    glyph_wrap_result r;
    glyph_wrap_layout("Save changes and exit.", TT_W, &r);
    T_ASSERT_EQ_I(r.row_count, 1);
    if (strcmp(r.rows[0], "Save changes and exit.") != 0)
        T_FAIL("row0 -> '%s'", r.rows[0]);
    return 0;
}

int test_glyph_wrap_empty_is_zero_rows(void)
{
    glyph_wrap_result r;
    glyph_wrap_layout("", TT_W, &r);
    T_ASSERT_EQ_I(r.row_count, 0);
    glyph_wrap_layout(NULL, TT_W, &r);
    T_ASSERT_EQ_I(r.row_count, 0);
    return 0;
}

/* A lone `%n` between two short words breaks regardless of width. */
int test_glyph_wrap_bare_percent_n(void)
{
    glyph_wrap_result r;
    glyph_wrap_layout("a%nb", TT_W, &r);
    T_ASSERT_EQ_I(r.row_count, 2);
    if (strcmp(r.rows[0], "a") != 0) T_FAIL("row0 -> '%s'", r.rows[0]);
    if (strcmp(r.rows[1], "b") != 0) T_FAIL("row1 -> '%s'", r.rows[1]);
    return 0;
}

/* The wrap boundary is exact: a word that just fits stays; one column more
 * wraps.  "aaaa.... " padded to exactly width must not wrap a following word
 * that would overflow by one. */
int test_glyph_wrap_boundary_is_exact(void)
{
    glyph_wrap_result r;
    /* width 5: "abc" (3) + " " absorbed = "abc " (4 cols), then "de" (2) would
     * make 6 > 5 → wraps. */
    glyph_wrap_layout("abc de", 5, &r);
    T_ASSERT_EQ_I(r.row_count, 2);
    if (strcmp(r.rows[0], "abc ") != 0) T_FAIL("row0 -> '%s'", r.rows[0]);
    if (strcmp(r.rows[1], "de") != 0)   T_FAIL("row1 -> '%s'", r.rows[1]);
    return 0;
}
