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

/* Step (no advance) until the current line is fully typed and waiting — AND any
 * deferred same-speaker re-text (cs->pending_keep) has been applied + typed, so
 * the box is genuinely settled on the line the caller will advance from. */
static void run_to_wait(cutscene *cs)
{
    for (int i = 0; i < 5000 &&
         (!dialogue_awaiting_advance(cs->box) || cs->pending_keep); i++)
        cutscene_step(cs, 0);
}

/* Step (no advance) until the box is content-visible and still REVEALING (past
 * the pop-in, before the typewriter finishes) — the state where a confirm SKIPS. */
static void run_to_typing(cutscene *cs)
{
    for (int i = 0; i < 5000 && !dialogue_typing(cs->box); i++)
        cutscene_step(cs, 0);
}

/* How long the test SIMULATES the scene_fade grid as active after a FADE beat
 * issues — the host has no Win32/ddraw, so it stands in for the real grid settle
 * (the FADE beat then completes via the grid-settle gate, not its safety CAP). */
#define SIM_FADE_TICKS 16

/* Type+advance `count` lines; return the number of completion edges seen.  After
 * each advance, drive whatever non-dialogue beat phase it triggers to completion —
 * a speaker-change re-pop latency (#108), an inter-line lead-beat gap (the L7→L8
 * camera/run-off), or a room-transition (the arrival EXIT fade + the house ENTRY
 * fade), SIMULATING the scene_fade grid so the FADE beats settle.  Leaves the next
 * line armed (or the chain complete). */
static int step_through(cutscene *cs, int count)
{
    int completes = 0;
    for (int i = 0; i < count; i++) {
        run_to_wait(cs);
        completes += cutscene_step(cs, 1);
        int grid = 0;
        for (int k = 0; k < 5000; k++) {
            /* done when a line is armed (box up, not in a beat phase) or the chain
             * completed — otherwise drive the beat phase / re-pop latency. */
            if (!cutscene_in_beats(cs) &&
                (dialogue_active(cs->box) || !cutscene_active(cs)))
                break;
            cutscene_set_fade_active(cs, grid > 0);
            if (grid > 0) grid--;
            cutscene_step(cs, 0);
            cutscene_action act;
            if (cutscene_take_action(cs, &act) && act.kind == CS_ACT_FADE)
                grid = SIM_FADE_TICKS;     /* the simulated grid is now painting */
        }
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

    /* Z press → advance to line 2 (Father → Arche, a SPEAKER CHANGE): the box is
     * HIDDEN for the re-pop latency (engine-quirk #108), then re-pops with the new
     * speaker.  The OLD box dissolves out as the closing box meanwhile. */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 1);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);          /* hidden during the latency  */
    for (int i = 0; i < CUTSCENE_REOPEN_DELAY; i++)
        cutscene_step(&cs, 0);
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
     * consumed, faithful to retail's mutually-exclusive state 1/2).  Line 1 → 2 is
     * a SPEAKER CHANGE: the box is hidden for the re-pop latency, then re-pops. */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 1);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);           /* hidden during the latency */
    for (int i = 0; i < CUTSCENE_REOPEN_DELAY; i++)
        cutscene_step(&cs, 0);
    T_ASSERT(strcmp(box.name, "Arche") == 0);          /* re-popped, new speaker    */

    /* a confirm during the POP-IN (not yet content-visible) is eaten — no skip,
     * no advance (FUN_0043ce50 returns 0 until scale==1000) */
    T_ASSERT_EQ_I(dialogue_content_visible(&box), 0);  /* line 2 still popping in */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 1);               /* unchanged                  */
    return 0;
}

/* ── a SAME-speaker advance keeps the box open (no re-pop); a speaker change
 *    closes + re-pops it.  Arrival lines 4/5/6 (indices 3/4/5) are all Arche;
 *    line 7 (index 6) is Father (THEME 1: retail's 1-tick same-speaker reset
 *    vs the ~9-tick close on a speaker change) ── */
int test_cutscene_same_speaker_keeps_box(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    /* advance to line index 3 (Arche); the last advance (Mother L2 -> Arche L3)
     * was a speaker change, so the box was freshly re-popped. */
    step_through(&cs, 3);
    T_ASSERT_EQ_I(cs.line_idx, 3);
    T_ASSERT(strcmp(box.name, "Arche") == 0);

    /* type line 3 fully (scale settles 1000), then advance to line 4 — Arche ->
     * Arche, the SAME speaker: the box STAYS open (scale held at 1000, no
     * re-pop) and the re-text is DEFERRED one tick (the old line still renders
     * on the advance tick — retail clears the body one flip after the press). */
    run_to_wait(&cs);
    T_ASSERT_EQ_I(box.scale, 1000);
    int l3_total = box.total;
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 4);
    T_ASSERT_EQ_I(box.scale, 1000);                  /* kept open — no re-pop   */
    T_ASSERT_EQ_I(cs.pending_keep, 1);               /* re-text deferred        */
    T_ASSERT_EQ_I(box.reveal, l3_total);             /* old line STILL shown    */
    T_ASSERT(strcmp(box.name, "Arche") == 0);
    /* the deferred re-text applies on the next step: body resets to the new line
     * + starts typing, box still open (scale 1000, no pop-in) */
    cutscene_step(&cs, 0);
    T_ASSERT_EQ_I(cs.pending_keep, 0);
    T_ASSERT_EQ_I(box.scale, 1000);
    T_ASSERT_EQ_I(dialogue_content_visible(&box), 1);

    /* line 4 -> 5 (Arche -> Arche) again keeps the box, deferred re-text */
    run_to_wait(&cs);
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 5);
    T_ASSERT_EQ_I(box.scale, 1000);
    T_ASSERT_EQ_I(cs.pending_keep, 1);
    cutscene_step(&cs, 0);                            /* apply the deferred re-text */

    /* line 5 -> 6 (Arche -> Father): a SPEAKER CHANGE.  The box is HIDDEN for the
     * re-pop latency (engine-quirk #108), then opens from the spawn scale
     * (DIALOGUE_OPEN_SCALE0, +50/update; content gated until full). */
    run_to_wait(&cs);
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cs.line_idx, 6);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);         /* hidden during the latency */
    for (int i = 0; i < CUTSCENE_REOPEN_DELAY; i++)
        cutscene_step(&cs, 0);
    T_ASSERT_EQ_I(box.scale, DIALOGUE_OPEN_SCALE0);  /* new box opens from spawn  */
    T_ASSERT_EQ_I(dialogue_content_visible(&box), 0);
    T_ASSERT(strcmp(box.name, "Arche's Father") == 0);
    return 0;
}

/* ── a SPEAKER CHANGE snapshots the OLD box into the CLOSING box (it pops out in
 *    front while the new box opens behind — retail's overlap); a SAME-speaker
 *    advance does NOT (THEME 1 / quirk #107) ── */
int test_cutscene_closing_box_overlap(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);
    T_ASSERT_EQ_P(cutscene_closing_box(&cs), NULL);   /* none at arm */

    /* advance L0 -> L1 (Father -> Arche, a SPEAKER CHANGE): the closing box
     * snapshots L0 at full (scale 1000, the OLD speaker's name) */
    run_to_wait(&cs);
    cutscene_step(&cs, 1);
    const dialogue_box *cl = cutscene_closing_box(&cs);
    T_ASSERT(cl != NULL);
    T_ASSERT(strcmp(cl->name, "Arche's Father") == 0);   /* the OLD speaker  */
    T_ASSERT_EQ_I(cl->scale, 1000);                       /* snapshot at full */
    T_ASSERT_EQ_I(cs.line_idx, 1);

    /* LINGER: the old box stays FULL until the new box is half-open (scale 500).
     * The new box opens from 200 (+50/update) → 500 after ~6 steps; until then
     * the closing box holds scale 1000 (text shown behind the opening box). */
    cutscene_step(&cs, 0);
    cutscene_step(&cs, 0);
    cl = cutscene_closing_box(&cs);
    T_ASSERT(cl != NULL && cl->scale == 1000);            /* still lingering full */
    T_ASSERT(box.scale < 500);                             /* new box < half-open  */

    /* it then pops out (-40/update) and is removed; drain the full overlap */
    for (int i = 0; i < 40; i++)
        cutscene_step(&cs, 0);
    T_ASSERT_EQ_P(cutscene_closing_box(&cs), NULL);

    /* advance to line 4 (Arche); 4 -> 5 is a SAME-speaker advance: NO closing
     * box (the box stays open, just re-texts) */
    while (cs.line_idx < 4) { run_to_wait(&cs); cutscene_step(&cs, 1); }
    for (int i = 0; i < 40; i++) cutscene_step(&cs, 0);   /* drain the 3->4 close */
    T_ASSERT_EQ_P(cutscene_closing_box(&cs), NULL);
    run_to_wait(&cs);
    cutscene_step(&cs, 1);                                 /* 4 -> 5, same Arche */
    T_ASSERT_EQ_I(cs.line_idx, 5);
    T_ASSERT_EQ_P(cutscene_closing_box(&cs), NULL);        /* no overlap         */
    return 0;
}

/* ── the speaker-change PORTRAIT DISSOLVE (engine-quirk #108): on a speaker change
 *    the OLD bust dissolves out via the reverse ramp idx 18→2 over a 9-tick window
 *    [press, press+8] (gone the next tick), while the new box's re-pop is DELAYED 2
 *    ticks (the dissolve leads the box-frame by 2).  Drawcall+LUT-exact off
 *    retail.osr ([688,696] idx 18→2, gone 697 / box re-pops at advance_tick−6) ── */
int test_cutscene_portrait_fadeout(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    /* run L0 (Father) to fully-typed + awaiting advance, portrait opaque */
    run_to_wait(&cs);
    T_ASSERT(strcmp(box.name, "Arche's Father") == 0);
    T_ASSERT_EQ_I(dialogue_portrait_ramp_index(&box), -1);

    /* ADVANCE L0 → L1 (Father → Arche): the OLD box snapshots into the closing box
     * with the reverse dissolve armed (FIRST render = idx 18 at the press), and the
     * main box is HIDDEN for the re-pop latency. */
    cutscene_step(&cs, 1);                                /* T0 = the press */
    const dialogue_box *cl = cutscene_closing_box(&cs);
    T_ASSERT(cl != NULL);
    T_ASSERT(strcmp(cl->name, "Arche's Father") == 0);   /* the OLD speaker */
    T_ASSERT_EQ_I(cl->fade_out, 1);
    T_ASSERT_EQ_I(dialogue_portrait_ramp_index(cl), 18); /* idx 18 @ the press */
    T_ASSERT_EQ_I(cl->scale, 1000);                      /* the frame stays full */
    T_ASSERT_EQ_I(dialogue_active(&box), 0);             /* main box hidden */
    T_ASSERT_EQ_I(cs.reopen_delay, CUTSCENE_REOPEN_DELAY);
    T_ASSERT_EQ_I(cs.line_idx, 1);

    /* the reverse ramp walks 18→2 over the window; the closing FRAME stays full
     * (the bust dissolves while the box lingers, the frame closes only after the
     * new box passes half-open). */
    static const int want[9] = { 18, 16, 14, 12, 10, 8, 6, 4, 2 };
    for (int t = 1; t <= 8; t++) {                       /* T0+1 .. T0+8 */
        cutscene_step(&cs, 0);
        cl = cutscene_closing_box(&cs);
        T_ASSERT(cl != NULL);
        if (dialogue_portrait_ramp_index(cl) != want[t])
            T_FAIL("dissolve @T0+%d idx=%d (want %d)", t,
                   dialogue_portrait_ramp_index(cl), want[t]);
        T_ASSERT_EQ_I(cl->scale, 1000);                  /* frame still full */
    }
    /* by now the new box re-popped at the latency (T0+2) and is the new speaker */
    T_ASSERT_EQ_I(dialogue_active(&box), 1);
    T_ASSERT(strcmp(box.name, "Arche") == 0);

    /* T0+9: the dissolve completes (GONE) AND the frame begins to shrink (the new
     * box has passed half-open) — the bust is gone the tick the box closes. */
    cutscene_step(&cs, 0);
    cl = cutscene_closing_box(&cs);
    T_ASSERT(cl != NULL);
    T_ASSERT_EQ_I(dialogue_portrait_ramp_index(cl), DIALOGUE_PORTRAIT_GONE);
    T_ASSERT(cl->scale < 1000);                          /* frame now shrinking */

    /* drain: the closing box is removed once it shrinks below CLOSE_MIN */
    for (int i = 0; i < 40; i++) cutscene_step(&cs, 0);
    T_ASSERT_EQ_P(cutscene_closing_box(&cs), NULL);
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
    /* house line 0 armed (Arche, the new-house line) — step_through settled the
     * room-boundary re-pop latency (Mother → Arche is a speaker change) */
    T_ASSERT_EQ_I(dialogue_active(&box), 1);
    T_ASSERT(strcmp(box.name, "Arche") == 0);
    return 0;
}

/* ── THEME 3: the L7→L8 inter-line BEAT phase (the "Arche runs ahead" gap) ──
 * Advancing L7 ("Cool!") must enter the non-dialogue beat phase — close the box,
 * issue the camera pan to (28000,12800)@400, hold the run-gated 117t + the 50t
 * wait, then open L8 fresh.  Confirms during the gap are eaten (automatic beats). */
int test_cutscene_l8_lead_beats(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    /* advance L0..L6 (7 advances) → settle on L7 ("Cool!", Arche) */
    step_through(&cs, 7);
    run_to_wait(&cs);
    T_ASSERT_EQ_I(cs.line_idx, 7);
    T_ASSERT_EQ_I(cutscene_in_beats(&cs), 0);
    T_ASSERT_EQ_I(dialogue_active(&box), 1);

    /* advance L7 → the beat phase opens: box closed, line index at L8 (8) */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cutscene_in_beats(&cs), 1);
    T_ASSERT_EQ_I(cs.line_idx, 8);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);          /* main box hidden during the gap */

    /* the camera pan action is issued on entry (drained once) */
    cutscene_action act;
    T_ASSERT_EQ_I(cutscene_take_action(&cs, &act), 1);
    T_ASSERT_EQ_I(act.kind, CS_ACT_CAMERA_PAN);
    T_ASSERT_EQ_I(act.a, 28000);                      /* tgt_x */
    T_ASSERT_EQ_I(act.b, 12800);                      /* tgt_y */
    T_ASSERT_EQ_I(act.c, 400);                        /* speed */
    T_ASSERT_EQ_I(cutscene_take_action(&cs, &act), 0);/* one-shot drained */

    /* hold the camera-pan dur + the wait — exactly 117 + 50 = 167 ticks — then L8
     * opens.  A confirm DURING the gap is eaten (the beats are automatic). */
    int t = 0;
    while (cutscene_in_beats(&cs) && t < 1000) {
        cutscene_step(&cs, /*confirm=*/1);            /* presses do nothing here */
        t++;
    }
    T_ASSERT_EQ_I(t, 97 + 50);    /* ARRIVAL_L8_RUNOFF (case-4 run-off wait) +
                                   * ARRIVAL_L8_WAIT = the beat phase (the +20t box
                                   * pop-in lands L8's first glyph at retail's +167t) */
    /* no stray action emitted across the wait beat (camera was the only one) */
    T_ASSERT_EQ_I(cutscene_take_action(&cs, &act), 0);

    /* L8 ("Mom! Dad! c'mon!", Arche) is now armed fresh (a full open) */
    T_ASSERT_EQ_I(cs.line_idx, 8);
    T_ASSERT_EQ_I(dialogue_active(&box), 1);
    T_ASSERT(strcmp(box.name, "Arche") == 0);
    run_to_wait(&cs);                                  /* it types + awaits advance */
    T_ASSERT_EQ_I(dialogue_awaiting_advance(&box), 1);
    return 0;
}

/* ── THEME 3: the arrival→house ROOM-TRANSITION fades (notes #6/#7) ──
 * Advancing the arrival's LAST line runs its EXIT beats (wait + fade-TO-black =
 * cover) BEFORE staging the house key — the room must NOT swap until the cover
 * settles (note #5: no early snap); then the house room OPENS with a fade-FROM-
 * black (reveal) before line 0. */
int test_cutscene_transition_fades(void)
{
    int n = 0; const cutscene_room *chain = cutscene_town_chain(&n);
    dialogue_box box; cutscene cs;
    cutscene_arm(&cs, chain, n, stub_resolve, &box);

    /* advance L0..L8 (9 advances) → settle on L9 ("Hmhm! …", Mother, last arrival) */
    step_through(&cs, 9);
    run_to_wait(&cs);
    T_ASSERT_EQ_I(cs.line_idx, 9);
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_ARRIVAL);

    /* advance L9 → the arrival EXIT beats open: box closed, still IN the arrival room
     * (the house key is NOT staged until the exit beats complete). */
    cutscene_step(&cs, 1);
    T_ASSERT_EQ_I(cutscene_in_beats(&cs), 1);
    T_ASSERT_EQ_I(dialogue_active(&box), 0);
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_ARRIVAL);

    /* drive the whole transition, simulating the scene_fade grid; capture the two
     * fades + assert the room stays arrival until the cover fade settles. */
    int exit_mode = -1, entry_mode = -1, grid = 0;
    for (int k = 0; k < 5000 && cutscene_in_beats(&cs); k++) {
        cutscene_set_fade_active(&cs, grid > 0);
        if (grid > 0) grid--;
        cutscene_step(&cs, 0);
        cutscene_action act;
        if (cutscene_take_action(&cs, &act) && act.kind == CS_ACT_FADE) {
            grid = SIM_FADE_TICKS;
            if (exit_mode < 0) {
                exit_mode = act.a;                       /* first fade = arrival exit */
                /* the cover is starting — the room must still be arrival (it stages
                 * only AFTER the cover settles, so the swap is hidden under black) */
                T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_ARRIVAL);
            } else {
                entry_mode = act.a;                      /* second fade = house entry  */
                T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_HOUSE);
            }
        }
    }
    T_ASSERT_EQ_I(exit_mode, 2);    /* CS_FADE_COVER  — fade TO black (arrival exit)  */
    T_ASSERT_EQ_I(entry_mode, 1);   /* CS_FADE_REVEAL — fade FROM black (house entry) */

    /* the transition completed: house room, line 0 (Arche) armed fresh */
    T_ASSERT_EQ_I(cutscene_in_beats(&cs), 0);
    T_ASSERT_EQ_I(cs.room_idx, 1);
    T_ASSERT_EQ_U(cutscene_room_key(&cs), CUTSCENE_ROOM_HOUSE);
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
