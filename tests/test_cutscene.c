/*
 * tests/test_cutscene.c — host tests for the town-arrival cutscene driver
 * (src/cutscene.c): the 10-line script table + the Z-advance state machine.
 *
 * A stub resolver stands in for exe_data_string so the data path is exercised
 * without the user's binary.  The driver's contract: arm line 0; advance to the
 * next line on Z ONLY when the current line is fully typed; complete after the
 * last line (one completion edge, box closed).
 */
#include "../src/cutscene.h"
#include "t.h"

#include <string.h>

/* Map the script VAs to canned strings; line 1/2 get multi-char text, the rest
 * a 1-char line (fast to type), every name resolves. */
static const char *stub_resolve(uint32_t va)
{
    switch (va) {
    case 0x6b6eb0u: return "Arche";
    case 0x6b6f80u: return "Arche's Father";
    case 0x6b6fb4u: return "Arche's Mother";
    case 0x86d58cu: return "Here at last.";
    case 0x86d55cu: return "Finally here!";
    default:        return "x";
    }
}
static const char *null_resolve(uint32_t va) { (void)va; return NULL; }

/* Step (no advance) until the current line is fully typed and waiting. */
static void run_to_wait(cutscene *cs)
{
    for (int i = 0; i < 5000 && !dialogue_awaiting_advance(cs->box); i++)
        cutscene_step(cs, 0);
}

/* ── the script table: 10 lines, order + key fields ── */
int test_cutscene_town_arrival_table(void)
{
    int n = -1;
    const cutscene_line *s = cutscene_town_arrival(&n);
    T_ASSERT_EQ_I(n, 10);
    T_ASSERT_EQ_U(s[0].text_va, 0x86d58cu);   /* line 1 — Father */
    T_ASSERT_EQ_U(s[0].name_va, 0x6b6f80u);
    T_ASSERT_EQ_U(s[1].name_va, 0x6b6eb0u);   /* line 2 — Arche  */
    T_ASSERT_EQ_U(s[2].name_va, 0x6b6fb4u);   /* line 3 — Mother */
    T_ASSERT_EQ_U(s[9].text_va, 0x86d3d4u);   /* line 10 — Mother */
    T_ASSERT_EQ_I(s[0].face, 0x1e);
    return 0;
}

/* ── arm: line 0 armed, box active, name resolved ── */
int test_cutscene_arm_first_line(void)
{
    int n = 0; const cutscene_line *s = cutscene_town_arrival(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, s, n, stub_resolve, &box);
    T_ASSERT_EQ_I(cutscene_active(&cs), 1);
    T_ASSERT_EQ_I(cs.line_idx, 0);
    T_ASSERT_EQ_I(dialogue_active(&box), 1);
    T_ASSERT(strcmp(box.name, "Arche's Father") == 0);
    T_ASSERT_EQ_I(cutscene_complete(&cs), 0);
    return 0;
}

/* ── advance only when the line is fully typed ── */
int test_cutscene_advance_only_when_typed(void)
{
    int n = 0; const cutscene_line *s = cutscene_town_arrival(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, s, n, stub_resolve, &box);

    /* during pop-in / typing, Z does nothing */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 0);

    run_to_wait(&cs);
    T_ASSERT_EQ_I(dialogue_awaiting_advance(&box), 1);

    /* no press → stays */
    cutscene_step(&cs, 0);
    T_ASSERT_EQ_I(cs.line_idx, 0);

    /* Z press → advance to line 2 (box re-armed, not yet typed) */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 1);
    T_ASSERT_EQ_I(dialogue_awaiting_advance(&box), 0);
    T_ASSERT(strcmp(box.name, "Arche") == 0);
    return 0;
}

/* ── advancing through all 10 lines completes exactly once + closes the box ── */
int test_cutscene_completes_after_last_line(void)
{
    int n = 0; const cutscene_line *s = cutscene_town_arrival(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, s, n, stub_resolve, &box);

    int completes = 0;
    for (int line = 0; line < n; line++) {
        run_to_wait(&cs);
        completes += cutscene_step(&cs, 1);   /* advance */
    }
    T_ASSERT_EQ_I(completes, 1);              /* one completion edge */
    T_ASSERT_EQ_I(cutscene_complete(&cs), 1);
    T_ASSERT_EQ_I(cutscene_active(&cs), 0);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);  /* box closed */
    /* further steps are safe no-ops, no second completion */
    T_ASSERT_EQ_I(cutscene_step(&cs, 1), 0);
    return 0;
}

/* ── guards: unresolved line-0 / NULL args leave it disarmed ── */
int test_cutscene_guards(void)
{
    int n = 0; const cutscene_line *s = cutscene_town_arrival(&n);
    dialogue_box box; cutscene cs;

    cutscene_arm(&cs, s, n, null_resolve, &box);   /* text VA → NULL */
    T_ASSERT_EQ_I(cutscene_active(&cs), 0);

    cutscene_arm(&cs, NULL, 0, stub_resolve, &box);
    T_ASSERT_EQ_I(cutscene_active(&cs), 0);

    cutscene_step(NULL, 1);                         /* NULL cs */
    cutscene_arm(NULL, s, n, stub_resolve, &box);   /* NULL cs */
    T_ASSERT_EQ_I(cutscene_active(NULL), 0);
    return 0;
}
