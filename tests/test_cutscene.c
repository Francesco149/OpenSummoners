/*
 * tests/test_cutscene.c — host tests for the town-intro cutscene driver
 * (src/cutscene.c): the room chain (arrival → house) + the Z-advance state
 * machine + the room-key swap.
 *
 * A stub resolver stands in for exe_data_string so the data path is exercised
 * without the user's binary.  The driver's contract: arm room-0 line-0; advance
 * to the next line on Z ONLY when the current line is fully typed; at a room's
 * last line commit the next room key (advance the chain) and arm its line 0;
 * complete after the last room's last line (one completion edge, box closed).
 */
#include "../src/cutscene.h"
#include "t.h"

#include <string.h>

/* Map the script VAs to canned strings; arrival line 1/2 get multi-char text,
 * the rest a 1-char line (fast to type), every name resolves. */
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

/* Step (no advance) until the box is content-visible and still REVEALING (past
 * the pop-in, before the typewriter finishes) — the state where a confirm SKIPS. */
static void run_to_typing(cutscene *cs)
{
    for (int i = 0; i < 5000 && !dialogue_typing(cs->box); i++)
        cutscene_step(cs, 0);
}

/* Type+advance `count` lines; return the number of completion edges seen. */
static int step_through(cutscene *cs, int count)
{
    int completes = 0;
    for (int i = 0; i < count; i++) {
        run_to_wait(cs);
        completes += cutscene_step(cs, 1);
    }
    return completes;
}

/* ── the arrival line table: 10 lines, order + key fields ── */
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

/* ── the house line table: 8 lines, speakers Arche/Mother/Father ── */
int test_cutscene_town_house_table(void)
{
    int n = -1;
    const cutscene_line *s = cutscene_town_house(&n);
    T_ASSERT_EQ_I(n, 8);
    T_ASSERT_EQ_U(s[0].text_va, 0x86d390u);   /* line 1 — Arche  */
    T_ASSERT_EQ_U(s[0].name_va, 0x6b6eb0u);
    T_ASSERT_EQ_U(s[2].name_va, 0x6b6fb4u);   /* line 3 — Mother */
    T_ASSERT_EQ_U(s[3].name_va, 0x6b6f80u);   /* line 4 — Father */
    T_ASSERT_EQ_U(s[7].text_va, 0x86d1dcu);   /* line 8 — Mother */
    T_ASSERT_EQ_I(s[6].face, 0x03);           /* "I will, I promise." */
    return 0;
}

/* ── the room chain: 2 rooms, the committed keys + counts ── */
int test_cutscene_town_chain(void)
{
    int n = -1;
    const cutscene_room *r = cutscene_town_chain(&n);
    T_ASSERT_EQ_I(n, 2);
    T_ASSERT_EQ_U(r[0].room_key, CUTSCENE_ROOM_ARRIVAL);
    T_ASSERT_EQ_I(r[0].n_lines, 10);
    T_ASSERT_EQ_U(r[1].room_key, CUTSCENE_ROOM_HOUSE);
    T_ASSERT_EQ_I(r[1].n_lines, 8);
    return 0;
}

/* ── arm: room 0 / line 0 armed, box active, name resolved ── */
int test_cutscene_arm_first_line(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);
    T_ASSERT_EQ_I(cutscene_active(&cs), 1);
    T_ASSERT_EQ_I(cs.room_idx, 0);
    T_ASSERT_EQ_I(cs.line_idx, 0);
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_ARRIVAL);
    T_ASSERT_EQ_I(dialogue_active(&box), 1);
    T_ASSERT(strcmp(box.name, "Arche's Father") == 0);
    T_ASSERT_EQ_I(cutscene_complete(&cs), 0);
    return 0;
}

/* ── advance only when the line is fully typed ── */
int test_cutscene_advance_only_when_typed(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

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

/* ── confirm-while-typing SKIPS (completes the reveal), and the NEXT confirm
 *    advances — the two-press cadence (0x439690 state 1 -> 2 -> advance) ── */
int test_cutscene_typewriter_skip(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    /* line 1 = "Here at last." (multi-char) — run past the pop-in into typing */
    run_to_typing(&cs);
    T_ASSERT_EQ_I(dialogue_typing(&box), 1);
    T_ASSERT(box.reveal < box.total);            /* not yet fully revealed     */
    T_ASSERT_EQ_I(dialogue_awaiting_advance(&box), 0);

    /* CONFIRM while typing -> SKIP: reveal completes, but it does NOT advance */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 0);               /* still line 1               */
    T_ASSERT_EQ_I(box.reveal, box.total);        /* reveal jumped to the end   */
    T_ASSERT_EQ_I(dialogue_typing(&box), 0);     /* no longer typing           */
    T_ASSERT_EQ_I(dialogue_awaiting_advance(&box), 1);

    /* the NEXT confirm advances to line 2 (a second press — the skip press was
     * consumed, faithful to retail's mutually-exclusive state 1/2) */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 1);
    T_ASSERT(strcmp(box.name, "Arche") == 0);

    /* a confirm during the POP-IN (not yet content-visible) is eaten — no skip,
     * no advance (FUN_0043ce50 returns 0 until scale==1000) */
    T_ASSERT_EQ_I(dialogue_content_visible(&box), 0);  /* line 2 just re-armed */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 1);               /* unchanged                  */
    return 0;
}

/* ── the room swap: the 10th arrival advance commits the HOUSE key (0x334c8)
 *    and arms house line 0 — it does NOT complete the chain ── */
int test_cutscene_room_transition(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    /* advance the 10 arrival lines */
    int completes = step_through(&cs, 10);
    T_ASSERT_EQ_I(completes, 0);                 /* no completion at the swap */
    T_ASSERT_EQ_I(cutscene_complete(&cs), 0);
    T_ASSERT_EQ_I(cutscene_active(&cs), 1);
    T_ASSERT_EQ_I(cs.room_idx, 1);               /* now in the house room      */
    T_ASSERT_EQ_I(cs.line_idx, 0);
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_HOUSE);
    /* house line 0 armed (Arche, the new-house line) */
    T_ASSERT_EQ_I(dialogue_active(&box), 1);
    T_ASSERT(strcmp(box.name, "Arche") == 0);
    return 0;
}

/* ── advancing through the whole chain (10 + 8) completes exactly once + closes
 *    the box at the errands boundary ── */
int test_cutscene_completes_after_chain(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    int completes = step_through(&cs, 10 + 8);
    T_ASSERT_EQ_I(completes, 1);              /* one completion edge */
    T_ASSERT_EQ_I(cutscene_complete(&cs), 1);
    T_ASSERT_EQ_I(cutscene_active(&cs), 0);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);  /* box closed */
    /* further steps are safe no-ops, no second completion */
    T_ASSERT_EQ_I(cutscene_step(&cs, 1), 0);
    return 0;
}

/* ── room_key reflects the current room / is 0 when inactive ── */
int test_cutscene_room_key(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_ARRIVAL);
    step_through(&cs, 10);                         /* into the house */
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_HOUSE);
    step_through(&cs, 8);                          /* complete the chain */
    /* room_idx now past the last room → key reads 0 (out of range) */
    T_ASSERT_EQ_U(cutscene_room_key(&cs), 0u);
    T_ASSERT_EQ_U(cutscene_room_key(NULL), 0u);
    return 0;
}

/* ── guards: unresolved room-0 line-0 / NULL args leave it disarmed ── */
int test_cutscene_guards(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;

    cutscene_arm(&cs, chain, n, null_resolve, &box);   /* text VA → NULL */
    T_ASSERT_EQ_I(cutscene_active(&cs), 0);

    cutscene_arm(&cs, NULL, 0, stub_resolve, &box);
    T_ASSERT_EQ_I(cutscene_active(&cs), 0);

    cutscene_step(NULL, 1);                            /* NULL cs */
    cutscene_arm(NULL, chain, n, stub_resolve, &box);  /* NULL cs */
    T_ASSERT_EQ_I(cutscene_active(NULL), 0);
    T_ASSERT_EQ_U(cutscene_room_key(NULL), 0u);
    return 0;
}
