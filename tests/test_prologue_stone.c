/*
 * tests/test_prologue_stone.c — the Elemental-Stone intro cutscene state
 * model (src/prologue_stone.c, the visual half of FUN_0056cd20).
 *
 * Three layers:
 *   1. init — the start state + the per-entry sparkle stagger seed.
 *   2. update — the per-tick state machine: start delay, watchdog, gem fade
 *      phases, frame cycling, the rise curve, the 6 sparkle SMs, and the
 *      abort/beat input paths.
 *   3. render — the draw-list geometry (gem/aura/sparkle positions, gates).
 *
 * Expectations are hand-derived from the decompile at 0x56cd20 (cited in
 * prologue_stone.c) and the survey in docs/findings/prologue-stone-intro.md.
 */
#include "t.h"
#include "prologue_stone.h"
#include "input.h"

#include <string.h>

/* Drive N update ticks with no input (time-only). */
static void run_ticks(prologue_stone *ps, int n)
{
    for (int i = 0; i < n; i++) prologue_stone_update(ps, NULL, 1000);
}

/* A populated 64-slot ring (mirrors test_input.c's mgr_init). */
static void mgr_init(input_mgr *m, input_event *empty)
{
    memset(m, 0, sizeof *m);
    empty->id = 0; empty->ts = 0; empty->flag = 0;
    for (int i = 0; i < INPUT_RING_LEN; i++) m->ring[i] = empty;
}

/* ─── 1. init ────────────────────────────────────────────────────────── */

int test_prologue_init_state(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    T_ASSERT_EQ_I(ps.watchdog, 0x1130);
    T_ASSERT_EQ_I(ps.start_delay, 10);
    T_ASSERT_EQ_I(ps.sound_cue, 0xed8);
    T_ASSERT_EQ_U(ps.beats, 0);
    T_ASSERT_EQ_I(ps.exiting, 0);
    T_ASSERT_EQ_I(ps.gem_phase, 0);
    T_ASSERT_EQ_I(ps.gem_fade, 0);
    T_ASSERT_EQ_I(ps.rise_pos, -4000);
    T_ASSERT_EQ_I(ps.rise_vel, 800);
    return 0;
}

int test_prologue_init_sparkle_stagger(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    static const uint16_t seed[6] = {0x2a8,0x3d4,0x62c,0x758,0x884,0xa46};
    for (int i = 0; i < PROLOGUE_SPARKLE_ENTRIES; i++) {
        T_ASSERT_EQ_U(ps.sparkle[i].state, 0);
        T_ASSERT_EQ_U(ps.sparkle[i].level, 0);
        T_ASSERT_EQ_U(ps.sparkle[i].sub, seed[i]);
        T_ASSERT_EQ_I(ps.sparkle[i].y, 32000);
    }
    return 0;
}

/* ─── 2. update: start delay ─────────────────────────────────────────── */

/* The first 10 ticks only count down start_delay; the gem stays at fade 0 and
 * the watchdog still ticks (the gem appears only after the delay). */
int test_prologue_start_delay(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);
    T_ASSERT_EQ_I(ps.start_delay, 0);
    T_ASSERT_EQ_I(ps.gem_fade, 0);          /* no animation yet */
    T_ASSERT_EQ_I(ps.watchdog, 0x1130 - 10);/* watchdog ticks during the delay */
    /* one more tick now advances the gem fade */
    run_ticks(&ps, 1);
    T_ASSERT_EQ_I(ps.gem_fade, 1);
    return 0;
}

/* ─── update: gem fade-in then hold ──────────────────────────────────── */

int test_prologue_gem_fade_in_to_hold(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);                     /* clear the start delay */
    /* fade-in: 600 ticks raise gem_fade 0→600, then phase flips to 1 (hold). */
    run_ticks(&ps, 600);
    T_ASSERT_EQ_I(ps.gem_fade, 600);
    T_ASSERT_EQ_I(ps.gem_phase, 0);         /* not flipped until the next tick */
    run_ticks(&ps, 1);
    T_ASSERT_EQ_I(ps.gem_phase, 1);         /* fully faded → hold */
    T_ASSERT_EQ_I(ps.gem_fade, 600);        /* stays pinned during hold */
    return 0;
}

/* ─── update: gem frame + aura toggle cadence (every 7 ticks) ─────────── */

int test_prologue_frame_cadence(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);                     /* clear delay */
    T_ASSERT_EQ_U(ps.gem_frame, 0);
    run_ticks(&ps, 7);                      /* gem_sub 0..6 then wrap → frame 1 */
    T_ASSERT_EQ_U(ps.gem_frame, 1);
    T_ASSERT_EQ_U(ps.aura_frame, 1);        /* aura toggles 0→1 on the same wrap */
    run_ticks(&ps, 7);
    T_ASSERT_EQ_U(ps.gem_frame, 2);
    T_ASSERT_EQ_U(ps.aura_frame, 0);        /* toggles back */
    return 0;
}

/* The gem frame wraps at 0x23 (35 frames). */
int test_prologue_gem_frame_wraps(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);
    run_ticks(&ps, 7 * 0x23);               /* 35 wraps → back to frame 0 */
    T_ASSERT_EQ_U(ps.gem_frame, 0);
    return 0;
}

/* ─── update: rise curve ─────────────────────────────────────────────── */

/* rise_pos climbs by rise_vel/100 (=8) per tick from −4000; velocity only
 * decays after rise_pos passes 16000. */
int test_prologue_rise(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);                     /* clear delay */
    run_ticks(&ps, 1);
    T_ASSERT_EQ_I(ps.rise_pos, -4000 + 8);  /* +800/100 */
    T_ASSERT_EQ_I(ps.rise_vel, 800);        /* no decay yet (below 16000) */
    return 0;
}

/* ─── update: a single sparkle through its life cycle ────────────────── */

/* Entry 0 (sub seed 0x2a8) waits 0x2a8 ticks, then grows: sub 0→10 (×level
 * 0→0x14). Verify it leaves the wait state and begins growing. */
int test_prologue_sparkle_wait_then_grow(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);                     /* clear delay */
    /* during the wait, sub counts down and state stays 0, level 0. */
    run_ticks(&ps, 0x100);
    T_ASSERT_EQ_U(ps.sparkle[0].state, 0);
    T_ASSERT_EQ_U(ps.sparkle[0].level, 0);
    T_ASSERT_EQ_U(ps.sparkle[0].sub, 0x2a8 - 0x100);
    /* run past the wait: state advances to grow and level starts climbing. */
    run_ticks(&ps, 0x2a8 - 0x100 + 11);     /* drain wait + one grow sub cycle */
    T_ASSERT(ps.sparkle[0].state >= 1);
    return 0;
}

/* ─── update: abort (id 0x22) ────────────────────────────────────────── */

int test_prologue_abort(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event e = { .id = 0x22, .ts = 1000, .flag = 1 };
    m.ring[40] = &e;
    T_ASSERT_EQ_I(prologue_stone_update(&ps, &m, 1000), PROLOGUE_ABORT);
    T_ASSERT_EQ_I(e.id, 0);                 /* consumed */
    return 0;
}

/* ─── update: beats → exit fade ──────────────────────────────────────── */

/* Three fresh presses (any non-abort id), each after the start delay, drive the
 * beat count to 3; the 3rd flips to exiting (gem fade-out, watchdog clamped). */
int test_prologue_three_beats_begin_exit(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);                     /* clear delay */

    input_mgr m; input_event empty; mgr_init(&m, &empty);
    for (int b = 1; b <= 3; b++) {
        input_event e = { .id = 3, .ts = 1000, .flag = 1 };
        m.ring[40] = &e;
        prologue_stone_update(&ps, &m, 1000);
        T_ASSERT_EQ_U(ps.beats, (uint32_t)b);
        m.ring[40] = &empty;                /* reset (update flushed the ring) */
    }
    T_ASSERT_EQ_I(ps.exiting, 1);
    T_ASSERT_EQ_I(ps.gem_phase, 2);         /* fade-out */
    T_ASSERT(ps.watchdog <= 200);           /* clamped on the 3rd beat */
    /* the exit forces every sparkle toward shrink/dead (state 3 or 4). */
    for (int i = 0; i < PROLOGUE_SPARKLE_ENTRIES; i++)
        T_ASSERT(ps.sparkle[i].state >= 3);
    return 0;
}

/* After the exit clamp the cutscene completes within ~200 ticks → DONE. */
int test_prologue_exit_completes(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    for (int b = 0; b < 3; b++) {
        input_event e = { .id = 3, .ts = 1000, .flag = 1 };
        m.ring[40] = &e;
        prologue_stone_update(&ps, &m, 1000);
        m.ring[40] = &empty;
    }
    /* watchdog ≤200 now; run it out and expect DONE (enter game). */
    prologue_status st = PROLOGUE_RUNNING;
    for (int i = 0; i < 300 && st == PROLOGUE_RUNNING; i++)
        st = prologue_stone_update(&ps, &m, 1000);
    T_ASSERT_EQ_I(st, PROLOGUE_DONE);
    return 0;
}

/* With no input at all, the watchdog (4400) eventually completes → DONE. */
int test_prologue_watchdog_completes(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    prologue_status st = PROLOGUE_RUNNING;
    for (int i = 0; i < 0x1130 + 1 && st == PROLOGUE_RUNNING; i++)
        st = prologue_stone_update(&ps, NULL, 1000);
    T_ASSERT_EQ_I(st, PROLOGUE_DONE);
    return 0;
}

/* ─── 3. render geometry ─────────────────────────────────────────────── */

/* During the start delay nothing draws. */
int test_prologue_render_gated_by_delay(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    prologue_render_out out;
    prologue_stone_render(&ps, &out);
    T_ASSERT_EQ_I(out.gem.draw, 0);
    T_ASSERT_EQ_I(out.aura.draw, 0);
    return 0;
}

/* After the delay, with the gem partway faded, the gem + aura draw at their
 * retail logical bases (gem x=0xf8, aura x=0xd0) and the gem y tracks the rise. */
int test_prologue_render_gem_aura(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);                     /* clear delay */
    run_ticks(&ps, 50);                     /* fade up a bit + rise */
    prologue_render_out out;
    prologue_stone_render(&ps, &out);
    T_ASSERT_EQ_I(out.gem.draw, 1);
    T_ASSERT_EQ_I(out.gem.x, 0xf8);
    T_ASSERT_EQ_I(out.gem.y, ps.rise_pos / 100 + 0x30);
    T_ASSERT_EQ_I(out.aura.draw, 1);
    T_ASSERT_EQ_I(out.aura.x, 0xd0);
    T_ASSERT_EQ_I(out.aura.y, ps.rise_pos / 100);
    /* aura blends through ramp_a at (gem_fade*0x14)/600, clamped [0,19]. */
    T_ASSERT_EQ_I(out.aura.ramp_idx, (ps.gem_fade * 0x14) / 600);
    return 0;
}

/* The gem alpha ramp index peaks at 16 at full fade (cf. the title cursor cap),
 * never reaching the 0x14 ceiling. */
int test_prologue_render_gem_alpha_cap(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    run_ticks(&ps, 10);
    run_ticks(&ps, 600);                    /* gem_fade → 600 (max) */
    prologue_render_out out;
    prologue_stone_render(&ps, &out);
    T_ASSERT_EQ_I(out.gem.draw, 1);
    T_ASSERT_EQ_I(out.gem.ramp_idx, 16);    /* (600*800/1000)*0x14/600 == 16 */
    return 0;
}

/* The sparkle grid is entry-major: entry e → frames 4e..4e+3 across 4 columns
 * at x = 0x10 + col·0x98.  Force a live sparkle and check the column layout. */
int test_prologue_render_sparkle_grid(void)
{
    prologue_stone ps;
    prologue_stone_init(&ps);
    /* hand-set entry 2 to a visible band so its 4 draws fire. */
    ps.start_delay = 0;
    ps.sparkle[2].level = 10;
    ps.sparkle[2].y = 15000;                /* y/100 = 150 */
    prologue_render_out out;
    prologue_stone_render(&ps, &out);
    for (int col = 0; col < 4; col++) {
        int k = 2 * 4 + col;
        T_ASSERT_EQ_I(out.sparkle[k].draw, 1);
        T_ASSERT_EQ_I(out.sparkle[k].frame, k);
        T_ASSERT_EQ_I(out.sparkle[k].x, 0x10 + col * 0x98);
        T_ASSERT_EQ_I(out.sparkle[k].y, 150);
        T_ASSERT_EQ_I(out.sparkle[k].ramp_idx, 10);
    }
    /* a level-0 entry draws nothing. */
    T_ASSERT_EQ_I(out.sparkle[0].draw, 0);
    return 0;
}
