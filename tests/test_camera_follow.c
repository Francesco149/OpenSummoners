/*
 * test_camera_follow.c — host tests for the in-game camera ease-to-target
 * (src/camera_follow.c; FUN_0043d1d0 + FUN_0043d340).
 *
 * The reference trajectory is GROUND TRUTH: a hardware watchpoint on the retail
 * view+0x60 during the opening-town pan captured the exact per-frame cur_x
 * values (docs/findings/in-game-intro.md "The camera EASER located").  Starting
 * from cur=128000, vel=0, tgt=12800, flag=0, cap=300, the port must reproduce
 * them bit-for-bit.
 */
#include "camera_follow.h"
#include "t.h"

/* The cur_x sequence (view+0x60) from the pan start (cur=128000, vel=0).  vel=0
 * means the first tick doesn't move (cur -= 0), so 128000 repeats once, then the
 * +10/tick ramp begins.  The retail HW-watchpoint capture (127970, 127940,
 * 127900, …) is exactly this sequence from index 3 onward — its first recorded
 * write was 127970 because the watch armed a few ticks into the ramp. */
static const int32_t REF_CUR_X[] = {
    128000, 128000, 127990, 127970, 127940, 127900, 127850, 127790,
    127720, 127640, 127550, 127450, 127340, 127220, 127090, 126950,
    126800,
};

int test_camera_follow_pan_trajectory(void)
{
    int32_t cur = 128000, vel = 0;
    const int32_t tgt = 12800, cap = 300;

    for (size_t i = 0; i < sizeof REF_CUR_X / sizeof REF_CUR_X[0]; i++) {
        T_ASSERT_EQ_I(cur, REF_CUR_X[i]);           /* matches the capture */
        camera_follow_axis(&cur, &vel, tgt, /*flag*/0, cap);
    }
    /* velocity ramps +10/step (30,40,…) — by the 16th step it is 160 */
    T_ASSERT_EQ_I(vel, 170);
    return 0;
}

int test_camera_follow_vel_caps_at_speed(void)
{
    int32_t cur = 128000, vel = 0;
    const int32_t tgt = 12800, cap = 300;
    /* run well past the ramp (30 steps -> vel would be 300) */
    for (int i = 0; i < 60; i++) camera_follow_axis(&cur, &vel, tgt, 0, cap);
    T_ASSERT_EQ_I(vel, cap);                        /* capped at the speed (300) */
    /* and the cruise step is exactly the cap */
    int32_t before = cur;
    camera_follow_axis(&cur, &vel, tgt, 0, cap);
    T_ASSERT_EQ_I(before - cur, cap);
    return 0;
}

int test_camera_follow_lands_exact_no_overshoot(void)
{
    int32_t cur = 128000, vel = 0;
    const int32_t tgt = 12800, cap = 300;
    for (int i = 0; i < 2000; i++) camera_follow_axis(&cur, &vel, tgt, 0, cap);
    T_ASSERT_EQ_I(cur, tgt);                        /* snapped onto target */
    T_ASSERT_EQ_I(vel, 0);                          /* decelerated to rest */
    /* stays put once settled (no oscillation) */
    camera_follow_axis(&cur, &vel, tgt, 0, cap);
    T_ASSERT_EQ_I(cur, tgt);
    T_ASSERT_EQ_I(vel, 0);
    return 0;
}

int test_camera_follow_far_boost_only_when_flagged(void)
{
    /* dist = 50000 (> 16000); flag!=0 adds (dist-16000)/10 = 3400 to the step */
    int32_t cur_f = 50000, vel_f = 0;     /* flagged   */
    int32_t cur_n = 50000, vel_n = 0;     /* unflagged */
    camera_follow_axis(&cur_f, &vel_f, 0, /*flag*/1, 300);
    camera_follow_axis(&cur_n, &vel_n, 0, /*flag*/0, 300);
    /* unflagged: step = vel(0)…+? first step vel<dist so cur -= (0+0)=0 -> 50000 */
    T_ASSERT_EQ_I(cur_n, 50000);
    /* flagged: cur -= (0 + (50000-16000)/10) = -3400 -> 46600 */
    T_ASSERT_EQ_I(cur_f, 46600);
    return 0;
}

int test_camera_shake_inactive_is_zero(void)
{
    cam_shake s = { 0 };                  /* active == 0 */
    int32_t out = camera_shake_apply(&s, 12800, 281600 - 64000);
    T_ASSERT_EQ_I(out, 0);
    T_ASSERT_EQ_I(s.out, 0);
    return 0;
}

int test_camera_follow_step_both_axes(void)
{
    camera_view v = { 0 };
    v.cur_x = 128000; v.cur_y = 12800;
    v.tgt_x = 12800;  v.tgt_y = 12800;    /* Y already on target */
    v.cap   = 300;    v.flag  = 0;
    v.map_w = 281600; v.map_h = 60800;
    v.vp_w  = 64000;  v.vp_h  = 48000;

    camera_follow_step(&v);
    /* X eased one step (cur 128000, vel 0 -> cur 128000, vel 10); Y stays */
    T_ASSERT_EQ_I(v.cur_x, 128000);
    T_ASSERT_EQ_I(v.vel_x, 10);
    T_ASSERT_EQ_I(v.cur_y, 12800);
    /* shakes inactive -> projector accumulators 0 (matches the cam_x34/x4c probe) */
    T_ASSERT_EQ_I(v.accum_x, 0);
    T_ASSERT_EQ_I(v.accum_y, 0);
    return 0;
}
