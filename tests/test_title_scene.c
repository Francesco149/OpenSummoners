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
