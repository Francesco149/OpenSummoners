/*
 * tests/test_app_pump.c — host-side tests for src/app_pump.c.
 *
 * Provides recording stubs for the 5 hook functions app_pump.h
 * declares.  Tests inject canned messages into the peek queue,
 * control GetTickCount, and assert against the call log + the
 * post-pump app_ctx fields.
 *
 * The stubs REPLACE src/app_pump_win32.c for the test build (see
 * tests/Makefile — only app_pump.c is pulled into SRCS_PORTED, never
 * app_pump_win32.c).
 */
#include "../src/app_pump.h"
#include "t.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── primitive stubs — controllable from each test ──────────────── */

/* Queue of WM_* codes that successive PeekMessage calls will return.
 * Tests fill this before calling the pump; the stub drains it. */
enum { PUMP_MSG_QUEUE_MAX = 16 };
static uint32_t g_msg_queue[PUMP_MSG_QUEUE_MAX];
static int      g_msg_queue_count;
static int      g_msg_queue_head;

static int      g_translate_dispatch_count;
static int      g_wait_count;

/* GetTickCount stub: queue of values returned by successive calls,
 * each value consumed once.  Read past the end returns
 * g_tick_default.  Tests verify the exact sequence by setting up
 * `g_tick_queue` and inspecting `g_tick_calls`. */
enum { PUMP_TICK_QUEUE_MAX = 8 };
static uint32_t g_tick_queue[PUMP_TICK_QUEUE_MAX];
static int      g_tick_queue_count;
static int      g_tick_calls;
static uint32_t g_tick_default = 0xdeadbeef;

/* exit hook — records, doesn't actually exit.  Tests assert
 * count + last code. */
static int g_exit_calls;
static int g_exit_last_code;

/* WaitMessage knobs — used by tests that exercise the outer wait
 * loop.  After WaitMessage runs, optionally mutate state so the next
 * iteration breaks (otherwise the test would hang). */
static int      g_wait_clears_throttle;
static int      g_wait_sets_active;
static int      g_wait_pushes_msg;
static uint32_t g_wait_pushes_msg_code;
static int      g_wait_max;          /* hard stop — abort after this many */

/* Test setup: clear every stub global + the pump's own globals. */
static void tp_reset(void)
{
    memset(g_msg_queue, 0, sizeof g_msg_queue);
    g_msg_queue_count = 0;
    g_msg_queue_head  = 0;

    g_translate_dispatch_count = 0;
    g_wait_count = 0;

    memset(g_tick_queue, 0, sizeof g_tick_queue);
    g_tick_queue_count = 0;
    g_tick_calls = 0;
    g_tick_default = 0xdeadbeef;

    g_exit_calls = 0;
    g_exit_last_code = 0;

    g_wait_clears_throttle = 0;
    g_wait_sets_active = 0;
    g_wait_pushes_msg = 0;
    g_wait_pushes_msg_code = 0;
    g_wait_max = 8;

    app_pump_state_init();
}

/* Enqueue a WM_* code to the peek queue. */
static void tp_enqueue_msg(uint32_t code)
{
    if (g_msg_queue_count - g_msg_queue_head >= PUMP_MSG_QUEUE_MAX) abort();
    g_msg_queue[g_msg_queue_count++ % PUMP_MSG_QUEUE_MAX] = code;
}

/* Enqueue the next GetTickCount return value. */
static void tp_enqueue_tick(uint32_t v)
{
    if (g_tick_queue_count >= PUMP_TICK_QUEUE_MAX) abort();
    g_tick_queue[g_tick_queue_count++] = v;
}

/* ─── hook implementations (recording stubs) ─────────────────────── */

int app_pump_peek_message(uint32_t *out_msg)
{
    if (g_msg_queue_head >= g_msg_queue_count) return 0;
    uint32_t code = g_msg_queue[g_msg_queue_head++ % PUMP_MSG_QUEUE_MAX];
    if (out_msg != NULL) *out_msg = code;
    return 1;
}

void app_pump_translate_and_dispatch(void)
{
    g_translate_dispatch_count++;
}

void app_pump_wait_message(void)
{
    g_wait_count++;
    if (g_wait_count > g_wait_max) {
        fprintf(stderr, "app_pump_wait_message: exceeded g_wait_max=%d — "
                "test would hang\n", g_wait_max);
        abort();
    }
    if (g_wait_clears_throttle && g_app_ctx != NULL) {
        g_app_ctx->pump_throttle = 0;
    }
    if (g_wait_sets_active) {
        g_app_active_flag = 1;
    }
    if (g_wait_pushes_msg) {
        tp_enqueue_msg(g_wait_pushes_msg_code);
    }
}

uint32_t app_pump_get_tick_count(void)
{
    g_tick_calls++;
    if (g_tick_calls - 1 < g_tick_queue_count) {
        return g_tick_queue[g_tick_calls - 1];
    }
    return g_tick_default;
}

void app_pump_request_exit(int code)
{
    g_exit_calls++;
    g_exit_last_code = code;
}

/* ─── tests ──────────────────────────────────────────────────────── */

/* app_ctx layout: pin offsets that must match retail BSS.  Only
 * meaningful on a 32-bit build (sizeof(void *) == 4); on 64-bit the
 * `f00` field is wider and the post-offsets shift — skip there. */
int test_app_ctx_layout_matches_retail(void)
{
#if UINTPTR_MAX == 0xFFFFFFFFu
    T_ASSERT_EQ_U(sizeof(app_ctx), 0x20u);
    T_ASSERT_EQ_U(offsetof(app_ctx, f00),            0x00u);
    T_ASSERT_EQ_U(offsetof(app_ctx, loaded),         0x08u);
    T_ASSERT_EQ_U(offsetof(app_ctx, limiter_enable), 0x0cu);
    T_ASSERT_EQ_U(offsetof(app_ctx, last_tick_ms),   0x10u);
    T_ASSERT_EQ_U(offsetof(app_ctx, pump_throttle),  0x1cu);
    return 0;
#else
    T_SKIP("32-bit-only layout assertions");
#endif
}

/* app_pump_state_init zeros the pump-owned globals.  Companion to
 * the WndProc's wp_state_init test — that one covers the wnd-proc-
 * private globals. */
int test_app_pump_state_init_clears_globals(void)
{
    app_ctx ctx = {0};
    g_app_ctx         = &ctx;
    g_app_active_flag = 0xdeadbeef;

    app_pump_state_init();

    T_ASSERT_EQ_P(g_app_ctx, NULL);
    T_ASSERT_EQ_U(g_app_active_flag, 0u);
    return 0;
}

/* Favourable path: queue empty, app active, throttle cleared.  Pump
 * should exit the wait loop immediately and skip the limiter (since
 * limiter_enable is 0). */
int test_app_pump_exits_when_active_and_throttle_clear(void)
{
    tp_reset();
    app_ctx ctx = {0};
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    /* limiter_enable == 0 → limiter section is a no-op. */

    app_pump_frame();

    T_ASSERT_EQ_I(g_wait_count, 0);
    T_ASSERT_EQ_I(g_translate_dispatch_count, 0);
    T_ASSERT_EQ_I(g_exit_calls, 0);
    T_ASSERT_EQ_U(ctx.pump_throttle, 0u);    /* limiter disabled — untouched */
    T_ASSERT_EQ_U(ctx.last_tick_ms, 0u);     /* ditto */
    T_ASSERT_EQ_I(g_tick_calls, 0);
    return 0;
}

/* Inactive: pump must WaitMessage until active_flag flips.  Stub
 * flips active=1 after one wait, then the next iteration exits. */
int test_app_pump_waits_until_active(void)
{
    tp_reset();
    app_ctx ctx = {0};
    g_app_ctx = &ctx;
    g_app_active_flag = 0;
    g_wait_sets_active = 1;     /* WaitMessage flips active=1 */

    app_pump_frame();

    T_ASSERT_EQ_I(g_wait_count, 1);
    T_ASSERT_EQ_I(g_translate_dispatch_count, 0);
    T_ASSERT_EQ_I(g_exit_calls, 0);
    T_ASSERT_EQ_U(g_app_active_flag, 1u);
    return 0;
}

/* Throttle set: pump must WaitMessage until WM_TIMER (modelled as the
 * stub clearing pump_throttle) clears it. */
int test_app_pump_waits_until_throttle_clears(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.pump_throttle = 1;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    g_wait_clears_throttle = 1;

    app_pump_frame();

    T_ASSERT_EQ_I(g_wait_count, 1);
    T_ASSERT_EQ_U(ctx.pump_throttle, 0u);
    return 0;
}

/* WM_QUIT (0x12) in the queue: calls request_exit(0) and returns
 * WITHOUT translating/dispatching the message.  Mirrors retail's
 * "FUN_005bf5db doesn't return" semantic via our `return` shortcut. */
int test_app_pump_wm_quit_calls_exit_no_dispatch(void)
{
    tp_reset();
    app_ctx ctx = {0};
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_msg(0x12);          /* WM_QUIT */

    app_pump_frame();

    T_ASSERT_EQ_I(g_exit_calls, 1);
    T_ASSERT_EQ_I(g_exit_last_code, 0);
    /* Crucially: TranslateMessage + DispatchMessage NOT called on the
     * WM_QUIT entry (retail jumps to FUN_005bf5db before them). */
    T_ASSERT_EQ_I(g_translate_dispatch_count, 0);
    return 0;
}

/* Non-WM_QUIT messages are translated + dispatched.  Three queued
 * messages → three translate+dispatch calls, then pump exits since
 * active=1 / throttle=0. */
int test_app_pump_drains_queue_then_exits(void)
{
    tp_reset();
    app_ctx ctx = {0};
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_msg(0x100);         /* WM_KEYDOWN */
    tp_enqueue_msg(0x200);         /* WM_MOUSEMOVE */
    tp_enqueue_msg(0x113);         /* WM_TIMER */

    app_pump_frame();

    T_ASSERT_EQ_I(g_translate_dispatch_count, 3);
    T_ASSERT_EQ_I(g_exit_calls, 0);
    T_ASSERT_EQ_I(g_wait_count, 0);   /* exited via favourable path */
    return 0;
}

/* WM_QUIT interleaved: messages before WM_QUIT get dispatched; messages
 * AFTER do not (the pump returns immediately from the WM_QUIT branch). */
int test_app_pump_wm_quit_stops_drain_immediately(void)
{
    tp_reset();
    app_ctx ctx = {0};
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_msg(0x100);   /* dispatched */
    tp_enqueue_msg(0x12);    /* WM_QUIT → exit */
    tp_enqueue_msg(0x113);   /* should NOT be dispatched */

    app_pump_frame();

    T_ASSERT_EQ_I(g_translate_dispatch_count, 1);
    T_ASSERT_EQ_I(g_exit_calls, 1);
    return 0;
}

/* Limiter enabled, first call (last_tick_ms == 0): pump re-arms
 * throttle to 1 and stamps last_tick_ms with the second GetTickCount
 * sample.  Only ONE GetTickCount call should happen on the first-frame
 * branch (the comparison-branch tick read is skipped via short-circuit). */
int test_app_pump_limiter_first_frame_sets_throttle(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 1;
    ctx.last_tick_ms = 0;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_tick(42000u);   /* the post-arm stamp */

    app_pump_frame();

    T_ASSERT_EQ_U(ctx.pump_throttle, 1u);
    T_ASSERT_EQ_U(ctx.last_tick_ms, 42000u);
    /* Short-circuit: last_tick_ms==0 means we never enter the
     * compare branch, so only ONE GetTickCount call happens (the
     * post-arm stamp). */
    T_ASSERT_EQ_I(g_tick_calls, 1);
    return 0;
}

/* Limiter enabled, throttle re-armed because the clock hasn't
 * advanced: `prev - now < 5` unsigned is true.  Stub feeds two tick
 * values: comparison reads 1000, post-stamp reads 1001 (so the
 * stamp differs from the comparison sample). */
int test_app_pump_limiter_holds_throttle_when_clock_static(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 1;
    ctx.last_tick_ms = 1003;   /* prev */
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    /* First tick read (comparison): 1000 → prev - now = 3 < 5 → throttle.
     * Second tick read (post-stamp): 1001 → last_tick_ms gets 1001. */
    tp_enqueue_tick(1000u);
    tp_enqueue_tick(1001u);

    app_pump_frame();

    T_ASSERT_EQ_U(ctx.pump_throttle, 1u);
    T_ASSERT_EQ_U(ctx.last_tick_ms, 1001u);
    T_ASSERT_EQ_I(g_tick_calls, 2);
    return 0;
}

/* Limiter enabled, clock HAS advanced enough: pump_throttle stays
 * cleared and last_tick_ms still gets updated.  Comparison sample:
 * now=2000, prev=1000 → unsigned (prev - now) wraps to huge → NOT < 5
 * → throttle branch skipped. */
int test_app_pump_limiter_skips_throttle_when_clock_advanced(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 1;
    ctx.last_tick_ms = 1000;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_tick(2000u);   /* comparison: prev - now = -1000 → 0xFFFFFC18 → >= 5 → skip */
    tp_enqueue_tick(2001u);   /* post-stamp */

    app_pump_frame();

    T_ASSERT_EQ_U(ctx.pump_throttle, 0u);
    T_ASSERT_EQ_U(ctx.last_tick_ms, 2001u);
    T_ASSERT_EQ_I(g_tick_calls, 2);
    return 0;
}

/* Boundary: prev - now == 5 (unsigned) → NOT < 5 → throttle skipped.
 * Verifies the boundary of the < 5 unsigned comparison. */
int test_app_pump_limiter_boundary_prev_minus_now_equals_5(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 1;
    ctx.last_tick_ms = 1005;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_tick(1000u);   /* prev - now = 5 → NOT < 5 → skip throttle */
    tp_enqueue_tick(1001u);

    app_pump_frame();

    T_ASSERT_EQ_U(ctx.pump_throttle, 0u);
    T_ASSERT_EQ_U(ctx.last_tick_ms, 1001u);
    return 0;
}

/* Boundary: prev - now == 4 (unsigned) → IS < 5 → throttle set. */
int test_app_pump_limiter_boundary_prev_minus_now_equals_4(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 1;
    ctx.last_tick_ms = 1004;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_tick(1000u);
    tp_enqueue_tick(1001u);

    app_pump_frame();

    T_ASSERT_EQ_U(ctx.pump_throttle, 1u);
    T_ASSERT_EQ_U(ctx.last_tick_ms, 1001u);
    return 0;
}

/* Limiter MASTER-DISABLED (limiter_enable=0): pump never touches
 * last_tick_ms / pump_throttle, never calls GetTickCount, even on a
 * favourable wait-loop exit. */
int test_app_pump_limiter_disabled_no_tick_reads(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 0;
    ctx.last_tick_ms = 12345;     /* should remain untouched */
    ctx.pump_throttle = 0;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;

    app_pump_frame();

    T_ASSERT_EQ_U(ctx.last_tick_ms, 12345u);
    T_ASSERT_EQ_U(ctx.pump_throttle, 0u);
    T_ASSERT_EQ_I(g_tick_calls, 0);
    return 0;
}

/* Defensive: NULL g_app_ctx after the wait loop's favourable exit
 * (e.g., active=1, no ctx).  Real retail would crash; we return
 * cleanly so the test harness doesn't hang. */
int test_app_pump_null_ctx_does_not_crash(void)
{
    tp_reset();
    g_app_ctx = NULL;
    g_app_active_flag = 1;

    app_pump_frame();

    T_ASSERT_EQ_I(g_tick_calls, 0);
    T_ASSERT_EQ_I(g_exit_calls, 0);
    return 0;
}

/* Realistic full-loop: queued WM_KEYDOWN message gets dispatched;
 * then the favourable-exit condition kicks in (active=1, throttle=0);
 * limiter is on with a fresh ctx (last_tick=0) so throttle is set on
 * the way out. */
int test_app_pump_drain_then_limiter_first_frame(void)
{
    tp_reset();
    app_ctx ctx = {0};
    ctx.limiter_enable = 1;
    g_app_ctx = &ctx;
    g_app_active_flag = 1;
    tp_enqueue_msg(0x100);    /* WM_KEYDOWN — dispatched */
    tp_enqueue_tick(7000u);   /* post-arm stamp */

    app_pump_frame();

    T_ASSERT_EQ_I(g_translate_dispatch_count, 1);
    T_ASSERT_EQ_U(ctx.pump_throttle, 1u);
    T_ASSERT_EQ_U(ctx.last_tick_ms, 7000u);
    return 0;
}
