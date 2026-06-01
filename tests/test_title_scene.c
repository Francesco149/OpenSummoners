/*
 * tests/test_title_scene.c — host-side tests for src/title_scene.c,
 * the intro-phase / menu-fade state machine of FUN_0056aea0.
 *
 * The unit under test is pure (no hooks, no Win32): each test drives
 * title_fade_step frame-by-frame and asserts the state transitions,
 * frame counts, and per-frame effects descriptor against the values
 * hand-computed from the disassembly at 0x56b153..0x56b5c1.
 */
#include "../src/title_scene.h"
#include "t.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Run one frame; return the action and stash the effects in *fx. */
static title_fade_action step(title_fade_state *s, title_fade_step_out *fx)
{
    return title_fade_step(s, fx);
}

/* Drive frames while the phase stays == `phase`, up to `cap` frames.
 * Returns the number of frames spent in that phase (i.e. the count of
 * steps after which s->phase first differed).  Aborts the test via the
 * caller's cap if it never leaves (returns cap). */
static int run_phase(title_fade_state *s, int phase, int cap)
{
    title_fade_step_out fx;
    for (int i = 0; i < cap; i++) {
        step(s, &fx);
        if (s->phase != phase) return i + 1;
    }
    return cap;
}

/* ─── init ───────────────────────────────────────────────────────── */

int test_title_fade_init_zeroes(void)
{
    title_fade_state s = { 7, 7, 7, 7 };
    title_fade_state_init(&s);
    T_ASSERT_EQ_I(s.phase, 0);
    T_ASSERT_EQ_I(s.fade, 0);
    T_ASSERT_EQ_I(s.tick, 0);
    T_ASSERT_EQ_I(s.menu_fade, 0);
    return 0;
}

/* ─── phase 0: studio-logo fade-in ───────────────────────────────── */

int test_title_phase0_ramps_by_20_to_1000(void)
{
    title_fade_state s; title_fade_state_init(&s);
    title_fade_step_out fx;

    /* +20 each frame, no effects, action CONTINUE. */
    for (int i = 1; i <= 50; i++) {
        title_fade_action a = step(&s, &fx);
        T_ASSERT_EQ_I(a, TITLE_FADE_CONTINUE);
        T_ASSERT_EQ_I(s.fade, i * 20);
        T_ASSERT_EQ_I(s.phase, 0);
        T_ASSERT_EQ_I(fx.set_next_segment, 0);
        T_ASSERT_EQ_I(fx.spawn_sparkle, 0);
    }
    T_ASSERT_EQ_I(s.fade, 1000);
    /* fade now == 1000; next frame advances to phase 1 without touching fade */
    step(&s, &fx);
    T_ASSERT_EQ_I(s.phase, 1);
    T_ASSERT_EQ_I(s.fade, 1000);
    return 0;
}

int test_title_phase0_clamps_at_1000(void)
{
    /* fade just below a multiple-of-20 overshoot: 990 + 20 = 1010 → clamp */
    title_fade_state s = { .phase = 0, .fade = 990, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;
    step(&s, &fx);
    T_ASSERT_EQ_I(s.fade, 1000);
    T_ASSERT_EQ_I(s.phase, 0);
    return 0;
}

/* ─── phase 1: hold studio logo 50 frames ────────────────────────── */

int test_title_phase1_holds_50_then_advances(void)
{
    title_fade_state s = { .phase = 1, .fade = 1000, .tick = 0, .menu_fade = 0 };
    /* tick 0→50 takes 50 frames; the 51st advances to phase 2 */
    int frames = run_phase(&s, 1, 200);
    T_ASSERT_EQ_I(frames, 51);
    T_ASSERT_EQ_I(s.phase, 2);
    T_ASSERT_EQ_I(s.tick, 50);
    return 0;
}

/* ─── phase 2: studio-logo fade-out + SetNextSegment cue ─────────── */

int test_title_phase2_fades_out_and_fires_segment_once(void)
{
    title_fade_state s = { .phase = 2, .fade = 1000, .tick = 5, .menu_fade = 0 };
    title_fade_step_out fx;

    /* 50 decrements of 20 bring fade 1000 → 0, no segment cue yet */
    for (int i = 1; i <= 50; i++) {
        step(&s, &fx);
        T_ASSERT_EQ_I(s.fade, 1000 - i * 20);
        T_ASSERT_EQ_I(s.phase, 2);
        T_ASSERT_EQ_I(fx.set_next_segment, 0);
    }
    T_ASSERT_EQ_I(s.fade, 0);

    /* the frame that sees fade==0 fires the cue, resets tick, → phase 3 */
    title_fade_action a = step(&s, &fx);
    T_ASSERT_EQ_I(a, TITLE_FADE_CONTINUE);
    T_ASSERT_EQ_I(fx.set_next_segment, 1);
    T_ASSERT_EQ_I(s.phase, 3);
    T_ASSERT_EQ_I(s.tick, 0);
    T_ASSERT_EQ_I(s.fade, 0);

    /* and it does not fire again on subsequent frames */
    step(&s, &fx);
    T_ASSERT_EQ_I(fx.set_next_segment, 0);
    return 0;
}

int test_title_phase2_fade_out_saturates_at_zero(void)
{
    /* fade not a multiple of 20: 10 - 20 must floor at 0, not wrap */
    title_fade_state s = { .phase = 2, .fade = 10, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;
    step(&s, &fx);
    T_ASSERT_EQ_I(s.fade, 0);
    T_ASSERT_EQ_I(s.phase, 2);       /* still phase 2: this frame just decremented */
    T_ASSERT_EQ_I(fx.set_next_segment, 0);
    return 0;
}

/* ─── phase 3: title-logo fade-in by 10 ──────────────────────────── */

int test_title_phase3_ramps_by_10_to_1000(void)
{
    title_fade_state s = { .phase = 3, .fade = 0, .tick = 0, .menu_fade = 0 };
    int frames = run_phase(&s, 3, 1000);
    /* 100 ramp frames to 1000, +1 to advance = 101 */
    T_ASSERT_EQ_I(frames, 101);
    T_ASSERT_EQ_I(s.phase, 4);
    T_ASSERT_EQ_I(s.fade, 1000);
    return 0;
}

/* ─── phase 4: hold title logo 20 frames, reset fade ─────────────── */

int test_title_phase4_holds_20_then_resets_fade(void)
{
    title_fade_state s = { .phase = 4, .fade = 1000, .tick = 0, .menu_fade = 0 };
    int frames = run_phase(&s, 4, 200);
    T_ASSERT_EQ_I(frames, 21);       /* tick 0→20 = 20 frames, +1 transition */
    T_ASSERT_EQ_I(s.phase, 5);
    T_ASSERT_EQ_I(s.fade, 0);        /* fade reset on the transition */
    T_ASSERT_EQ_I(s.tick, 0);
    return 0;
}

/* ─── phase 5: press-button in, +100/frame, hold 40 ──────────────── */

int test_title_phase5_fade_caps_then_tick_bound_advance(void)
{
    title_fade_state s = { .phase = 5, .fade = 0, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;

    /* fade saturates at 1000 after 10 frames (+100 each) */
    for (int i = 1; i <= 10; i++) {
        step(&s, &fx);
        T_ASSERT_EQ_I(s.fade, i * 100);
    }
    T_ASSERT_EQ_I(s.fade, 1000);
    T_ASSERT_EQ_I(s.phase, 5);
    /* but the phase is tick-bound (0→40), so we are at tick 10 now */
    T_ASSERT_EQ_I(s.tick, 10);

    /* run out the remaining hold: total 40 frames in-phase, +1 to advance */
    int more = run_phase(&s, 5, 200);
    T_ASSERT_EQ_I(more, 31);         /* 40 - 10 already done + 1 transition */
    T_ASSERT_EQ_I(s.phase, 6);
    T_ASSERT_EQ_I(s.fade, 0);        /* reset on transition */
    return 0;
}

/* ─── phase 6: hold press-button, +10/frame, hold 120 ────────────── */

int test_title_phase6_holds_120_then_phase7(void)
{
    title_fade_state s = { .phase = 6, .fade = 0, .tick = 0, .menu_fade = 0 };
    int frames = run_phase(&s, 6, 500);
    T_ASSERT_EQ_I(frames, 121);      /* tick 0→120 = 120 frames, +1 */
    T_ASSERT_EQ_I(s.phase, 7);
    T_ASSERT_EQ_I(s.fade, 0);
    return 0;
}

/* ─── phase 7: sparkle flourish ──────────────────────────────────── */

int test_title_phase7_spawns_sparkles_below_850(void)
{
    title_fade_state s = { .phase = 7, .fade = 0, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;

    int sparkles = 0;
    /* Frame k (1-based): fade becomes 20*k. Sparkle while 20*k < 850,
     * i.e. k <= 42 (840).  k=43 → 860 ≥ 850 → no sparkle. */
    for (int k = 1; k <= 43; k++) {
        step(&s, &fx);
        int fade_after = 20 * k;
        if (fade_after < 850) {
            T_ASSERT_EQ_I(fx.spawn_sparkle, 1);
            int want = (fade_after * 0xe0) / 900 + 0xc0;
            T_ASSERT_EQ_I(fx.sparkle_intensity, want);
            sparkles++;
        } else {
            T_ASSERT_EQ_I(fx.spawn_sparkle, 0);
        }
    }
    T_ASSERT_EQ_I(sparkles, 42);
    return 0;
}

int test_title_phase7_holds_90_then_menu(void)
{
    title_fade_state s = { .phase = 7, .fade = 0, .tick = 0, .menu_fade = 0 };
    int frames = run_phase(&s, 7, 300);
    T_ASSERT_EQ_I(frames, 91);       /* tick 0→90 = 90 frames, +1 transition */
    T_ASSERT_EQ_I(s.phase, 8);
    T_ASSERT_EQ_I(s.fade, 0);        /* reset on transition into menu */
    return 0;
}

int test_title_phase7_fade_clamps_at_1000(void)
{
    /* fade 990 + 20 = 1010 ≥ 1001 → clamp to 1000, no sparkle */
    title_fade_state s = { .phase = 7, .fade = 990, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;
    step(&s, &fx);
    T_ASSERT_EQ_I(s.fade, 1000);
    T_ASSERT_EQ_I(fx.spawn_sparkle, 0);
    return 0;
}

/* ─── phases 8/9: steady-state menu fade ─────────────────────────── */

int test_title_phase8_returns_menu_and_ramps_fade_first(void)
{
    title_fade_state s = { .phase = 8, .fade = 0, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;

    /* While main fade < 1000 it ramps +20 and menu_fade stays untouched.
     * Every frame returns MENU. */
    for (int i = 1; i <= 50; i++) {
        title_fade_action a = step(&s, &fx);
        T_ASSERT_EQ_I(a, TITLE_FADE_MENU);
        T_ASSERT_EQ_I(s.fade, i * 20);
        T_ASSERT_EQ_I(s.menu_fade, 0);
        T_ASSERT_EQ_I(s.phase, 8);
    }
    T_ASSERT_EQ_I(s.fade, 1000);

    /* Now fade is saturated: menu_fade breathes up by 50/frame to 1000,
     * then advances to phase 9. */
    for (int i = 1; i <= 20; i++) {
        step(&s, &fx);
        T_ASSERT_EQ_I(s.menu_fade, i * 50);
        T_ASSERT_EQ_I(s.phase, 8);
    }
    T_ASSERT_EQ_I(s.menu_fade, 1000);
    step(&s, &fx);
    T_ASSERT_EQ_I(s.phase, 9);
    return 0;
}

int test_title_phase9_fades_menu_down_then_back_to_8(void)
{
    title_fade_state s = { .phase = 9, .fade = 1000, .tick = 0, .menu_fade = 1000 };
    title_fade_step_out fx;

    /* menu_fade -50/frame from 1000 to 0 = 20 frames */
    for (int i = 1; i <= 20; i++) {
        title_fade_action a = step(&s, &fx);
        T_ASSERT_EQ_I(a, TITLE_FADE_MENU);
        T_ASSERT_EQ_I(s.menu_fade, 1000 - i * 50);
        T_ASSERT_EQ_I(s.phase, 9);
    }
    T_ASSERT_EQ_I(s.menu_fade, 0);
    /* menu_fade == 0 (< 1) → back to phase 8 */
    step(&s, &fx);
    T_ASSERT_EQ_I(s.phase, 8);
    return 0;
}

int test_title_phase9_menu_fade_saturates_at_zero(void)
{
    /* menu_fade 30 - 50 must floor at 0 once we are past the <1 guard.
     * Start at 30: 30 >= 1 so decrement → max(30-50,0) = 0, stays phase 9. */
    title_fade_state s = { .phase = 9, .fade = 1000, .tick = 0, .menu_fade = 30 };
    title_fade_step_out fx;
    step(&s, &fx);
    T_ASSERT_EQ_I(s.menu_fade, 0);
    T_ASSERT_EQ_I(s.phase, 9);
    return 0;
}

/* ─── phase 10: scene fade-out → EXIT ────────────────────────────── */

int test_title_phase10_fades_out_then_exits(void)
{
    title_fade_state s = { .phase = 10, .fade = 40, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;

    /* fade 40 → 20 (CONTINUE) */
    T_ASSERT_EQ_I(step(&s, &fx), TITLE_FADE_CONTINUE);
    T_ASSERT_EQ_I(s.fade, 20);
    /* fade 20 → 0 (CONTINUE) */
    T_ASSERT_EQ_I(step(&s, &fx), TITLE_FADE_CONTINUE);
    T_ASSERT_EQ_I(s.fade, 0);
    /* fade 0 → EXIT (no further decrement) */
    T_ASSERT_EQ_I(step(&s, &fx), TITLE_FADE_EXIT);
    T_ASSERT_EQ_I(s.fade, 0);
    return 0;
}

int test_title_phase10_exit_when_already_zero(void)
{
    title_fade_state s = { .phase = 10, .fade = 0, .tick = 0, .menu_fade = 0 };
    title_fade_step_out fx;
    T_ASSERT_EQ_I(step(&s, &fx), TITLE_FADE_EXIT);
    return 0;
}

/* ─── full intro walk: phase 0 → menu (phase 8) ──────────────────── */

int test_title_full_intro_walk_to_menu(void)
{
    title_fade_state s; title_fade_state_init(&s);
    title_fade_step_out fx;

    int segment_fires = 0;
    int total = 0;
    /* Walk until we first reach the menu (action MENU / phase 8). */
    for (; total < 5000; total++) {
        title_fade_action a = step(&s, &fx);
        if (fx.set_next_segment) segment_fires++;
        if (a == TITLE_FADE_MENU) break;
    }
    T_ASSERT_EQ_I(s.phase, 8);
    T_ASSERT_EQ_I(segment_fires, 1);  /* exactly one BGM cue during the intro */

    /* Per-phase frame budget, hand-summed from the transitions tested
     * individually above:
     *   p0: 50 ramp +1   = 51
     *   p1: 50 hold +1   = 51
     *   p2: 50 fade +1   = 51   (the +1 frame also fires the segment cue)
     *   p3: 100 ramp +1  = 101
     *   p4: 20 hold +1   = 21
     *   p5: 40 hold +1   = 41
     *   p6: 120 hold +1  = 121
     *   p7: 90 hold +1   = 91
     *   then the first phase-8 frame returns MENU and breaks the loop.
     * Total steps executed before the break = 528; the break happens on
     * step 529 (the first MENU). */
    T_ASSERT_EQ_I(total + 1, 529);
    return 0;
}

/* ═══ (A) frame-pacing sub-state machine ══════════════════════════════
 *
 * All expectations hand-computed from the disasm at 0x56b002..0x56b0c8.
 * `now` is the GetTickCount value the enclosing loop would pass each
 * iteration.
 */

static title_pace_action pace(title_pace_state *s, uint32_t now,
                              title_pace_step_out *out)
{
    title_pace_step(s, now, out);
    return out->action;
}

/* ─── init ───────────────────────────────────────────────────────── */

int test_title_pace_init(void)
{
    title_pace_state s = { 7, 7, 7, 7 };
    title_pace_state_init(&s);
    T_ASSERT_EQ_I(s.sub, 0);
    T_ASSERT_EQ_U(s.budget, 0x11);     /* local_30 = 0x11 (17 ms) */
    T_ASSERT_EQ_U(s.tick_anchor, 0);
    T_ASSERT_EQ_U(s.render_anchor, 0);
    return 0;
}

/* ─── sub==0: first frame pumps and enters update ────────────────── */

int test_title_pace_first_frame_pumps_then_updates(void)
{
    title_pace_state s; title_pace_state_init(&s);
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 5000, &out), TITLE_PACE_UPDATE);
    T_ASSERT_EQ_I(out.pump, 1);                 /* FUN_005b1030 on first frame */
    T_ASSERT_EQ_I(s.sub, 2);                    /* S: 0 → 2 */
    T_ASSERT_EQ_U(s.tick_anchor, 5000);         /* C = now */
    T_ASSERT_EQ_U(s.render_anchor, 5000);       /* A = now (post sub==2) */
    T_ASSERT_EQ_U(s.budget, 0x11);              /* budget untouched on sub==0 */
    return 0;
}

/* ─── sub==2: burn a 16 ms slice, then render ────────────────────── */

int test_title_pace_update_burns_slice_then_renders(void)
{
    /* now == render_anchor → 0 ms elapsed, not > budget; burn one slice.
     * 17 - 16 = 1 ≤ 16 → leave update (S=1) → render this frame. */
    title_pace_state s = { .sub = 2, .budget = 0x11,
                           .tick_anchor = 900, .render_anchor = 1000 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1000, &out), TITLE_PACE_RENDER);
    T_ASSERT_EQ_I(out.pump, 0);                 /* sub==2 path never pumps */
    T_ASSERT_EQ_I(s.sub, 1);
    T_ASSERT_EQ_U(s.budget, 1);                 /* 17 - 16 */
    T_ASSERT_EQ_U(s.render_anchor, 1000);       /* unchanged (post arm is sub==2 only) */
    return 0;
}

int test_title_pace_update_stays_updating_when_budget_high(void)
{
    /* budget 50: 0 ms elapsed ≤ 50 → burn slice → 34 > 16 → stay updating.
     * The sub==2 post-arm re-anchors render_anchor = now. */
    title_pace_state s = { .sub = 2, .budget = 50,
                           .tick_anchor = 0, .render_anchor = 1000 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1000, &out), TITLE_PACE_UPDATE);
    T_ASSERT_EQ_I(out.pump, 0);
    T_ASSERT_EQ_I(s.sub, 2);
    T_ASSERT_EQ_U(s.budget, 34);                /* 50 - 16 */
    T_ASSERT_EQ_U(s.render_anchor, 1000);       /* A = now */
    return 0;
}

int test_title_pace_update_realtime_outran_budget_renders(void)
{
    /* 20 ms elapsed > budget 17 → zero the budget and render immediately. */
    title_pace_state s = { .sub = 2, .budget = 0x11,
                           .tick_anchor = 0, .render_anchor = 1000 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1020, &out), TITLE_PACE_RENDER);
    T_ASSERT_EQ_I(s.sub, 1);
    T_ASSERT_EQ_U(s.budget, 0);
    return 0;
}

int test_title_pace_update_boundary_elapsed_equals_budget(void)
{
    /* elapsed == budget (17): the compare is `budget < elapsed` (jbe at
     * 0x56b02d takes the else), so equality burns a slice rather than
     * zeroing.  17 - 16 = 1 ≤ 16 → render. */
    title_pace_state s = { .sub = 2, .budget = 0x11,
                           .tick_anchor = 0, .render_anchor = 1000 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1017, &out), TITLE_PACE_RENDER);
    T_ASSERT_EQ_U(s.budget, 1);                 /* burned a slice, not zeroed */
    T_ASSERT_EQ_I(s.sub, 1);
    return 0;
}

/* ─── sub==1: refill the budget, pump, maybe re-enter update ─────── */

int test_title_pace_render_refills_and_enters_update(void)
{
    /* 30 ms elapsed since last pump: budget 10 + 30 = 40 > 16 → update. */
    title_pace_state s = { .sub = 1, .budget = 10,
                           .tick_anchor = 1000, .render_anchor = 0 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1030, &out), TITLE_PACE_UPDATE);
    T_ASSERT_EQ_I(out.pump, 1);                 /* FUN_005b1030 on render/refill */
    T_ASSERT_EQ_I(s.sub, 2);
    T_ASSERT_EQ_U(s.budget, 40);                /* 10 + (1030 - 1000) */
    T_ASSERT_EQ_U(s.tick_anchor, 1030);         /* C = now */
    T_ASSERT_EQ_U(s.render_anchor, 1030);       /* A = now (post sub==2) */
    return 0;
}

int test_title_pace_render_stays_render_when_budget_small(void)
{
    /* 5 ms elapsed: budget 2 + 5 = 7 ≤ 16 → stay render. */
    title_pace_state s = { .sub = 1, .budget = 2,
                           .tick_anchor = 1000, .render_anchor = 555 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1005, &out), TITLE_PACE_RENDER);
    T_ASSERT_EQ_I(out.pump, 1);
    T_ASSERT_EQ_I(s.sub, 1);
    T_ASSERT_EQ_U(s.budget, 7);
    T_ASSERT_EQ_U(s.tick_anchor, 1005);
    T_ASSERT_EQ_U(s.render_anchor, 555);        /* unchanged: post arm is sub==2 only */
    return 0;
}

int test_title_pace_render_clamps_budget_to_100(void)
{
    /* 200 ms elapsed: 50 + 200 = 250 → clamp to 100 (then 100 > 16 → update). */
    title_pace_state s = { .sub = 1, .budget = 50,
                           .tick_anchor = 1000, .render_anchor = 0 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1200, &out), TITLE_PACE_UPDATE);
    T_ASSERT_EQ_U(s.budget, 100);               /* clamped, not 250 */
    T_ASSERT_EQ_I(s.sub, 2);
    return 0;
}

int test_title_pace_render_boundary_budget_exactly_16(void)
{
    /* budget refills to exactly 16: `16 < budget` is false → stay render. */
    title_pace_state s = { .sub = 1, .budget = 6,
                           .tick_anchor = 1000, .render_anchor = 0 };
    title_pace_step_out out;

    T_ASSERT_EQ_I(pace(&s, 1010, &out), TITLE_PACE_RENDER);  /* 6 + 10 = 16 */
    T_ASSERT_EQ_U(s.budget, 16);
    T_ASSERT_EQ_I(s.sub, 1);
    return 0;
}

/* ─── integrative walk: a frozen clock (turbo) renders every frame ─── */

int test_title_pace_frozen_clock_walk(void)
{
    /* With no wall-clock time passing, after the initial pump+update the
     * budget never refills, so the machine renders every frame.  This is
     * the documented turbo/very-fast-machine behaviour. */
    title_pace_state s; title_pace_state_init(&s);
    title_pace_step_out out;
    const uint32_t T = 1000;

    /* frame 1: sub 0 → update, pump. */
    T_ASSERT_EQ_I(pace(&s, T, &out), TITLE_PACE_UPDATE);
    T_ASSERT_EQ_I(out.pump, 1);

    /* frame 2: sub 2, 0 ms elapsed → burn 17→1, render, no pump. */
    T_ASSERT_EQ_I(pace(&s, T, &out), TITLE_PACE_RENDER);
    T_ASSERT_EQ_I(out.pump, 0);
    T_ASSERT_EQ_U(s.budget, 1);

    /* frames 3..12: sub 1, budget refills by 0 ms → 1 ≤ 16 → render+pump
     * forever, budget pinned at 1. */
    for (int i = 0; i < 10; i++) {
        T_ASSERT_EQ_I(pace(&s, T, &out), TITLE_PACE_RENDER);
        T_ASSERT_EQ_I(out.pump, 1);
        T_ASSERT_EQ_I(s.sub, 1);
        T_ASSERT_EQ_U(s.budget, 1);
    }
    return 0;
}

int test_title_pace_clock_jump_reenters_update(void)
{
    /* Sit in the render steady-state (frozen clock), then jump the clock
     * forward: the refill exceeds one slice and the machine re-enters the
     * update state, proving the render→update path off a real time delta. */
    title_pace_state s; title_pace_state_init(&s);
    title_pace_step_out out;

    pace(&s, 1000, &out);                  /* sub 0 → 2 (update) */
    pace(&s, 1000, &out);                  /* sub 2 → 1 (render) */
    T_ASSERT_EQ_I(s.sub, 1);

    /* clock jumps 50 ms: budget 1 + 50 = 51 > 16 → back to update. */
    T_ASSERT_EQ_I(pace(&s, 1050, &out), TITLE_PACE_UPDATE);
    T_ASSERT_EQ_I(s.sub, 2);
    T_ASSERT_EQ_U(s.budget, 51);
    T_ASSERT_EQ_U(s.tick_anchor, 1050);
    return 0;
}

/* ─── the top-level menu spawn block (title_menu_spawn) ────────────────
 *
 * These exercise the composed spawn against host memory: an owning
 * sel_list whose first entry is an allocated 0x1b0 node, plus an injected
 * selection key.  After the spawn the controller is node->children[0] with
 * a five-row grid; teardown drops the handles and clears node+0x50.
 */

/* Build a minimal owner sel_list: an entries array of `cap` pointers with
 * entry 0 an allocated (zeroed) node, count 0.  Caller frees via free_owner. */
static void mk_owner(sel_list *o, int cap)
{
    memset(o, 0, sizeof *o);
    o->entries  = (sel_entry **)calloc((size_t)cap, sizeof(sel_entry *));
    o->entries[0] = (sel_entry *)calloc(1, sizeof(menu_node));
    o->capacity = (uint16_t)cap;
    o->count    = 0;
}

/* Tear down everything the spawn allocated (controller geometry, the child
 * node + child array) and the owner scaffolding, leaving LSan clean. */
static void free_spawn(sel_list *o, title_menu *tm)
{
    if (tm->ctrl != NULL) {
        menu_ctrl_clear(tm->ctrl);          /* list / rows(+cells) / entries */
    }
    if (tm->node != NULL) {
        free(tm->node->children[0]);        /* the controller buffer */
        free(tm->node->children);           /* the child-pointer array */
    }
    free(o->entries[0]);                    /* the node (entries[0]) */
    free(o->entries);
}

/* Happy path: one node configured + marked, controller built with the five
 * fixed rows, cursor seeked to the row matching the selection key. */
int test_title_menu_spawn_builds_five_rows(void)
{
    sel_list o; mk_owner(&o, 4);
    title_menu tm;

    title_menu_spawn(&o, 0x1e, &tm);        /* select key = options (row 2) */

    /* (1) owner entry configured + active. */
    T_ASSERT_EQ_U(o.count, 1);
    menu_node *node = (menu_node *)o.entries[0];
    T_ASSERT_EQ_P(tm.node, node);
    T_ASSERT_EQ_P(node->owner, &o);
    T_ASSERT_EQ_I(node->field4, 1);
    T_ASSERT_EQ_I(node->field_50, 1);
    /* sel_list_mark_last marks the entry through the sel_entry view (+0x08);
     * on the 32-bit target that byte is also menu_node.selected, but on the
     * 64-bit host the two structs' paddings diverge, so check the entry as
     * the sel_list machinery actually wrote it. */
    T_ASSERT_EQ_I(((sel_entry *)o.entries[0])->selected, 1);
    T_ASSERT_EQ_U(node->child_count, 1);

    /* (2) controller is the lone child, with a 6×1 stride-6 grid. */
    T_ASSERT(tm.ctrl != NULL);
    T_ASSERT_EQ_P(tm.ctrl, node->children[0]);
    T_ASSERT_EQ_I(tm.ctrl->list->alloc_a, 6);
    T_ASSERT_EQ_I(tm.ctrl->list->stride, 6);
    T_ASSERT_EQ_I(tm.ctrl->list->type, 0);

    /* (3) the five rows in append order. */
    T_ASSERT_EQ_I(tm.ctrl->list->count, 5);
    T_ASSERT_EQ_I(tm.ctrl->rows[0].action, 0x1a);
    T_ASSERT_EQ_I(tm.ctrl->rows[1].action, 0x1c);
    T_ASSERT_EQ_I(tm.ctrl->rows[2].action, 0x1e);
    T_ASSERT_EQ_I(tm.ctrl->rows[3].action, 0x1d);
    T_ASSERT_EQ_I(tm.ctrl->rows[4].action, 8);
    for (int i = 0; i < 5; i++) {
        T_ASSERT_EQ_I(tm.ctrl->rows[i].field0, 0);
        T_ASSERT_EQ_I(tm.ctrl->rows[i].flag8, 1);
    }

    /* (4) cursor seeked to the 0x1e row (index 2); page-top unmoved (0). */
    T_ASSERT_EQ_I(tm.ctrl->list->cursor, 2);
    T_ASSERT_EQ_I(tm.ctrl->list->sel2, 0);

    free_spawn(&o, &tm);
    return 0;
}

/* The cursor seek lands on the matching row whichever it is (action 8 is
 * the last, index 4). */
int test_title_menu_spawn_cursor_seeks_match(void)
{
    sel_list o; mk_owner(&o, 4);
    title_menu tm;

    title_menu_spawn(&o, 8, &tm);           /* exit row, index 4 */
    T_ASSERT_EQ_I(tm.ctrl->list->cursor, 4);

    free_spawn(&o, &tm);
    return 0;
}

/* No row matches the key → the cursor stays at its built default (0) and
 * scroll-into-view is never reached. */
int test_title_menu_spawn_no_match_keeps_cursor_zero(void)
{
    sel_list o; mk_owner(&o, 4);
    title_menu tm;

    title_menu_spawn(&o, 0x7f, &tm);        /* matches no action id */
    T_ASSERT_EQ_I(tm.ctrl->list->cursor, 0);

    free_spawn(&o, &tm);
    return 0;
}

/* Teardown clears the node's +0x50 active flag and drops both handles;
 * it frees nothing (the controller + node tree are owned elsewhere). */
int test_title_menu_teardown_clears_active_flag(void)
{
    sel_list o; mk_owner(&o, 4);
    title_menu tm;
    title_menu_spawn(&o, 0x1a, &tm);

    menu_node *node = tm.node;
    menu_ctrl *ctrl = tm.ctrl;              /* keep for cleanup */
    T_ASSERT_EQ_I(node->field_50, 1);

    title_menu_teardown(&tm);
    T_ASSERT_EQ_I(node->field_50, 0);
    T_ASSERT_EQ_P(tm.ctrl, NULL);
    T_ASSERT_EQ_P(tm.node, NULL);

    /* teardown freed nothing — the geometry is still live and reachable. */
    menu_ctrl_clear(ctrl);
    free(node->children[0]);
    free(node->children);
    free(o.entries[0]);
    free(o.entries);
    return 0;
}

/* Teardown on a never-spawned menu (ctrl NULL) is a no-op. */
int test_title_menu_teardown_noop_when_unset(void)
{
    title_menu tm = { NULL, NULL };
    title_menu_teardown(&tm);               /* must not crash / deref node */
    T_ASSERT_EQ_P(tm.ctrl, NULL);
    T_ASSERT_EQ_P(tm.node, NULL);
    return 0;
}

/* ─── (D) the per-frame menu input dispatch (title_menu_input_step) ────
 *
 * Build a standalone menu controller the way the spawn block does
 * (menu_ctrl_build 6×1 stride-6 type-0, then append rows), wire its input
 * "ready" gate open, and exercise the poll → latch → action switch path
 * against a fully-populated input ring.  Spies capture the hooked side
 * effects (SFX / joystick / save-data notify / watchdog).
 */

/* Spy state for the four hooks. */
static int32_t ti_sfx[16];
static int     ti_sfx_n;
static int     ti_joy_n;
static void   *ti_save_arg;
static int     ti_save_n;
static int     ti_wd_n;

static void ti_spy_sfx(int32_t id)  { if (ti_sfx_n < 16) ti_sfx[ti_sfx_n++] = id; }
static void ti_spy_joy(void)        { ti_joy_n++; }
static void ti_spy_save(void *a)    { ti_save_arg = a; ti_save_n++; }
static void ti_spy_wd(void)         { ti_wd_n++; }

static void ti_install_spies(void)
{
    ti_sfx_n = 0; ti_joy_n = 0; ti_save_arg = NULL; ti_save_n = 0; ti_wd_n = 0;
    title_menu_sfx_hook      = ti_spy_sfx;
    title_menu_joystick_hook = ti_spy_joy;
    title_menu_savedata_hook = ti_spy_save;
    title_menu_watchdog_hook  = ti_spy_wd;
}

/* A controller + its input gate + the five title rows. */
static const int32_t TI_ACTIONS[5] = { 0x1a, 0x1c, 0x1e, 0x1d, 8 };
static void ti_mk_menu(menu_ctrl *c, menu_input_sub *sub)
{
    memset(sub, 0, sizeof *sub);
    sub->enabled = 1;
    sub->ready   = 1000;                     /* latch gate open */
    menu_ctrl_build(c, 0, 0, 6, 1, 6, 0);    /* alloc_a=6, stride=6, type=0, mode=1 */
    c->sub = sub;
    for (int a = 0; a < 5; a++) {            /* append the five fixed rows */
        menu_list_hdr *hdr = c->list;
        int32_t i = hdr->count;
        hdr->count = i + 1;
        c->rows[i].field0 = 0;
        c->rows[i].action = TI_ACTIONS[a];
        c->rows[i].flag8  = 1;
    }
}

/* A 64-slot input ring with every slot pointing at a real (idle) record. */
static input_event ti_ring[64];
static void ti_mk_input(input_mgr *m)
{
    memset(m, 0, sizeof *m);
    memset(ti_ring, 0, sizeof ti_ring);
    for (int i = 0; i < 64; i++) m->ring[i] = &ti_ring[i];
}
static void ti_press(input_mgr *m, int slot, int32_t id, uint32_t ts)
{
    m->ring[slot]->id = id; m->ring[slot]->flag = 1; m->ring[slot]->ts = ts;
}

/* Up (button 1 → latch dir 0 = prev) navigates and plays the move SFX (9);
 * no commit, no phase-10 request. */
int test_menu_input_nav_plays_move_sfx(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    ti_press(&m, 10, 1, 1000);               /* button 1 (up), within window */

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, NULL, &out);

    T_ASSERT_EQ_I(out.action, 1);            /* nav "moved" */
    T_ASSERT_EQ_I(out.enter_phase10, 0);
    T_ASSERT_EQ_I(out.set_result, 0);
    T_ASSERT_EQ_I(c.list->cursor, 4);        /* prev from 0 wraps to count-1 */
    T_ASSERT_EQ_I(ti_sfx_n, 1);
    T_ASSERT_EQ_I(ti_sfx[0], 9);
    T_ASSERT_EQ_I(ti_joy_n, 0);

    menu_ctrl_clear(&c);
    return 0;
}

/* A page button on the single-page (stride 6 ≥ 5) menu latches dir 2 but nav
 * returns 0 — an idle frame: no SFX, no transition. */
int test_menu_input_page_button_is_noop(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    ti_press(&m, 5, 2, 1000);                /* button 2 (down) → latch dir 2 = page-up */

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, NULL, &out);

    T_ASSERT_EQ_I(out.action, 0);
    T_ASSERT_EQ_I(out.enter_phase10, 0);
    T_ASSERT_EQ_I(c.list->cursor, 0);        /* unchanged */
    T_ASSERT_EQ_I(ti_sfx_n, 0);

    menu_ctrl_clear(&c);
    return 0;
}

/* Commit (button 0x24 → latch dir 9 → nav returns 3) on an enabled row:
 * confirm SFX (5), joystick attach fires once, phase-10 requested, result is
 * the selected row's action id. */
int test_menu_input_commit_enabled_row(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    c.list->cursor = 0;                      /* row 0 → action 0x1a */
    ti_press(&m, 20, 0x24, 1000);

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, NULL, &out);

    T_ASSERT_EQ_I(out.action, 3);
    T_ASSERT_EQ_I(out.enter_phase10, 1);
    T_ASSERT_EQ_I(out.set_result, 1);
    T_ASSERT_EQ_I(out.result_code, 0x1a);
    T_ASSERT_EQ_I(ti_sfx_n, 1);
    T_ASSERT_EQ_I(ti_sfx[0], 5);             /* confirm */
    T_ASSERT_EQ_I(ti_joy_n, 1);              /* attach fired */

    menu_ctrl_clear(&c);
    return 0;
}

/* Commit on a *disabled* row (flag8 == 0): denied SFX (6), and nothing else —
 * no joystick, no phase-10, no result latch. */
int test_menu_input_commit_disabled_row(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    c.list->cursor   = 2;                    /* row 2 → action 0x1e */
    c.rows[2].flag8  = 0;                    /* disable it */
    ti_press(&m, 20, 0x24, 1000);

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, NULL, &out);

    T_ASSERT_EQ_I(out.action, 3);
    T_ASSERT_EQ_I(out.enter_phase10, 0);
    T_ASSERT_EQ_I(out.set_result, 0);
    T_ASSERT_EQ_I(ti_sfx_n, 1);
    T_ASSERT_EQ_I(ti_sfx[0], 6);             /* denied */
    T_ASSERT_EQ_I(ti_joy_n, 0);

    menu_ctrl_clear(&c);
    return 0;
}

/* Commit on a non-0x1d row whose action matches a save-data table entry:
 * the matched record's arg is handed to the notify hook. */
int test_menu_input_commit_savedata_match(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    c.list->cursor = 1;                      /* row 1 → action 0x1c */
    ti_press(&m, 20, 0x24, 1000);

    int marker = 0;
    title_menu_savedata_entry ents[3] = {
        { NULL,    0x1a },
        { &marker, 0x1c },                   /* matches the selected action */
        { NULL,    8    },
    };
    title_menu_savedata_list sd = { ents, 3 };

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, &sd, &out);

    T_ASSERT_EQ_I(out.result_code, 0x1c);
    T_ASSERT_EQ_I(ti_save_n, 1);
    T_ASSERT_EQ_P(ti_save_arg, &marker);

    menu_ctrl_clear(&c);
    return 0;
}

/* The save-data lookup is skipped entirely when the selected action is 0x1d. */
int test_menu_input_commit_savedata_skipped_for_0x1d(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    c.list->cursor = 3;                      /* row 3 → action 0x1d */
    ti_press(&m, 20, 0x24, 1000);

    int marker = 0;
    title_menu_savedata_entry ents[1] = { { &marker, 0x1d } };  /* would match if scanned */
    title_menu_savedata_list sd = { ents, 1 };

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, &sd, &out);

    T_ASSERT_EQ_I(out.result_code, 0x1d);
    T_ASSERT_EQ_I(out.enter_phase10, 1);
    T_ASSERT_EQ_I(ti_save_n, 0);             /* never scanned */

    menu_ctrl_clear(&c);
    return 0;
}

/* No buttons + no axis held: the 7/5 release syntheses both return 0 — a
 * pure idle frame, no SFX and no transition. */
int test_menu_input_idle_frame_no_effects(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    /* axis_held_v / axis_held_h left 0 → synth dirs 7 then 5, both no-ops */

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, NULL, &out);

    T_ASSERT_EQ_I(out.action, 0);
    T_ASSERT_EQ_I(out.enter_phase10, 0);
    T_ASSERT_EQ_I(ti_sfx_n, 0);

    menu_ctrl_clear(&c);
    return 0;
}

/* Axis-held vertical with a ripe auto-repeat deadline synthesises a nav move
 * (latch dir 6 → fires as prev) and plays the move SFX. */
int test_menu_input_axis_held_synthesises_move(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();
    m.axis_held_v        = 1;                /* vertical axis held */
    c.list->repeat_b     = 1;                /* deadline already set... */
    c.list->cursor       = 2;
    /* now=1000 >= repeat_b(1) → dir 6 fires as 'prev' (returns 1) */

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0, NULL, &out);

    T_ASSERT_EQ_I(out.action, 1);            /* nav moved */
    T_ASSERT_EQ_I(c.list->cursor, 1);        /* prev from 2 */
    T_ASSERT_EQ_I(ti_sfx_n, 1);
    T_ASSERT_EQ_I(ti_sfx[0], 9);

    menu_ctrl_clear(&c);
    return 0;
}

/* The idle watchdog forces phase 10 (and fires its hook) once the frame
 * counter reaches 0x1194, with no input at all. */
int test_menu_input_watchdog_forces_phase10(void)
{
    menu_ctrl c; menu_input_sub sub; input_mgr m;
    ti_mk_menu(&c, &sub); ti_mk_input(&m); ti_install_spies();

    title_menu_input_out out;
    title_menu_input_step(&m, &c, 1000, 0x1194, NULL, &out);

    T_ASSERT_EQ_I(out.enter_phase10, 1);
    T_ASSERT_EQ_I(out.set_result, 0);        /* watchdog doesn't latch a result */
    T_ASSERT_EQ_I(ti_wd_n, 1);

    /* one below the threshold → no trigger */
    ti_install_spies();
    title_menu_input_step(&m, &c, 1000, 0x1193, NULL, &out);
    T_ASSERT_EQ_I(out.enter_phase10, 0);
    T_ASSERT_EQ_I(ti_wd_n, 0);

    menu_ctrl_clear(&c);
    return 0;
}
