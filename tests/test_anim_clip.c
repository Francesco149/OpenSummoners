/*
 * test_anim_clip.c — host tests for the actor animation cycle
 * (src/anim_clip.c; the FUN_0054f980 per-sim-tick frame-stepper + the
 * FUN_0040afe0/FUN_0041e600 clip-set idiom).
 *
 * The stepper is GROUND TRUTH from the decompile: each animating behaviour in
 * FUN_0054f980 runs the byte-identical inline idiom timer++ / dur-gate /
 * frame++ / loop-or-hold.  These tests pin the exact per-tick trajectory for a
 * looping clip, a one-shot clip, the duration gate, and the change-gated set.
 */
#include "anim_clip.h"
#include "t.h"

/* Build a minimal clip (only the fields the stepper reads need to be set). */
static anim_clip make_clip(uint16_t count, uint16_t dur, int32_t oneshot,
                           uint16_t loop_to)
{
    anim_clip c = (anim_clip){0};
    c.frame_count = count;
    c.frame_dur   = dur;
    c.oneshot     = oneshot;
    c.loop_to     = loop_to;
    return c;
}

/* A 4-frame clip, 3 ticks/frame, looping back to frame 1.  The frame index
 * must hold each value for exactly `dur` ticks, then wrap to loop_to (not 0). */
int test_anim_clip_loop_trajectory(void)
{
    const anim_clip c = make_clip(/*count*/4, /*dur*/3, /*oneshot*/0, /*loop_to*/1);
    anim_state st = (anim_state){0};
    anim_state_set(&st, &c);

    /* expected frame index AFTER each advance, ticks 1..13 */
    static const uint16_t exp[] = {
        0, 0,          /* frame 0 holds ticks 1,2 */
        1,             /* tick 3: -> frame 1 */
        1, 1,          /* frame 1 holds ticks 4,5 */
        2,             /* tick 6: -> frame 2 */
        2, 2,          /* ticks 7,8 */
        3,             /* tick 9: -> frame 3 */
        3, 3,          /* ticks 10,11 */
        1,             /* tick 12: past last -> loop_to (1), NOT 0 */
        1,             /* tick 13 */
    };
    for (size_t i = 0; i < sizeof exp / sizeof exp[0]; i++) {
        anim_clip_advance(&st);
        T_ASSERT_EQ_U(st.frame, exp[i]);
        T_ASSERT_EQ_U(st.done, 0);          /* looping clip never finishes */
    }
    return 0;
}

/* A one-shot clip freezes on its last frame and raises `done`. */
int test_anim_clip_oneshot_holds_last(void)
{
    const anim_clip c = make_clip(/*count*/3, /*dur*/2, /*oneshot*/1, /*loop_to*/0);
    anim_state st = (anim_state){0};
    anim_state_set(&st, &c);

    /* frame 0: ticks 1,2 -> frame 1 ; frame 1: ticks 3,4 -> frame 2 ;
     * frame 2: ticks 5,6 -> would be frame 3 == count, one-shot -> hold 2 */
    static const uint16_t exp[] = { 0, 1, 1, 2, 2, 2, 2, 2 };
    for (size_t i = 0; i < sizeof exp / sizeof exp[0]; i++) {
        anim_clip_advance(&st);
        T_ASSERT_EQ_U(st.frame, exp[i]);
    }
    T_ASSERT_EQ_U(st.done, 1);              /* finished flag set */
    T_ASSERT_EQ_U(st.timer, 1);             /* timer parked at 1 while held */
    return 0;
}

/* Duration gate: dur=1 advances every tick; the loop wraps to loop_to=0. */
int test_anim_clip_duration_gate(void)
{
    const anim_clip c = make_clip(/*count*/2, /*dur*/1, /*oneshot*/0, /*loop_to*/0);
    anim_state st = (anim_state){0};
    anim_state_set(&st, &c);

    static const uint16_t exp[] = { 1, 0, 1, 0, 1, 0 };
    for (size_t i = 0; i < sizeof exp / sizeof exp[0]; i++) {
        anim_clip_advance(&st);
        T_ASSERT_EQ_U(st.frame, exp[i]);
        T_ASSERT_EQ_U(st.timer, 0);         /* dur=1 -> timer always reset */
    }
    return 0;
}

/* NULL clip is inert (the rstate[+0x6c]==0 guard). */
int test_anim_clip_null_is_noop(void)
{
    anim_state st = (anim_state){0};
    anim_clip_advance(&st);
    T_ASSERT_EQ_U(st.frame, 0);
    T_ASSERT_EQ_U(st.timer, 0);
    T_ASSERT_EQ_U(st.done, 0);
    return 0;
}

/* anim_state_set resets ONLY on a clip change — re-asserting the same clip
 * leaves the running cycle untouched (the retail `if (clip != cur)` guard). */
int test_anim_clip_set_is_change_gated(void)
{
    const anim_clip a = make_clip(4, 2, 0, 0);
    const anim_clip b = make_clip(4, 2, 0, 0);
    anim_state st = (anim_state){0};

    anim_state_set(&st, &a);
    anim_clip_advance(&st);                 /* (f0,t1) */
    anim_clip_advance(&st);                 /* (f1,t0) */
    T_ASSERT_EQ_U(st.frame, 1);

    /* re-assert the SAME clip pointer: no reset, cycle keeps its frame */
    anim_state_set(&st, &a);
    T_ASSERT_EQ_U(st.frame, 1);
    T_ASSERT_EQ_P(st.clip, &a);

    /* assert a DIFFERENT clip: full reset */
    anim_state_set(&st, &b);
    T_ASSERT_EQ_U(st.frame, 0);
    T_ASSERT_EQ_U(st.timer, 0);
    T_ASSERT_EQ_U(st.done, 0);
    T_ASSERT_EQ_P(st.clip, &b);
    return 0;
}

/* anim_clip_sprite = base + this frame's delta (FUN_00491ae0 case 0x1872d). */
int test_anim_clip_sprite_id(void)
{
    anim_clip c = make_clip(3, 5, 0, 0);
    c.base_sprite  = 0x100;
    c.frame_delta[0] = 0;
    c.frame_delta[1] = 7;
    c.frame_delta[2] = 9;
    anim_state st = (anim_state){0};
    anim_state_set(&st, &c);

    T_ASSERT_EQ_I(anim_clip_sprite(&st), 0x100);     /* frame 0 */
    st.frame = 1;
    T_ASSERT_EQ_I(anim_clip_sprite(&st), 0x107);
    st.frame = 2;
    T_ASSERT_EQ_I(anim_clip_sprite(&st), 0x109);
    return 0;
}
