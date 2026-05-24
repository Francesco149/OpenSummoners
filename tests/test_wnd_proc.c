/*
 * tests/test_wnd_proc.c — host-side tests for the WndProc port
 * (src/wnd_proc.c).
 *
 * Provides recording stubs for the 8 hook functions wnd_proc.h
 * declares.  Every call appends a tagged event into a per-kind log;
 * tests assert against the logs to verify the WndProc invoked the
 * right hook with the right args in the right order.
 *
 * The stubs REPLACE src/wnd_proc_win32.c for the test build (see
 * tests/Makefile — only wnd_proc.c is pulled into SRCS_PORTED, never
 * wnd_proc_win32.c).
 */
#include "../src/wnd_proc.h"
#include "t.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── recording hook log ─────────────────────────────────────────── */

typedef enum hook_kind {
    HK_DEF_WINDOW_PROC = 1,
    HK_APP_EXIT        = 2,
    HK_PAINT_CHECK     = 3,
    HK_APP_PAUSE       = 4,
    HK_INPUT_ACQUIRE   = 5,
    HK_ZDM_SET_ACTIVE  = 6,
    HK_POST_ACTIVATE   = 7,
    HK_LOG_CP          = 8,
} hook_kind;

typedef struct hook_event {
    hook_kind kind;
    /* Generic argument capture — interpretation depends on kind:
     *   DEF_WINDOW_PROC: arg_u32_a=msg, arg_uptr_a=hwnd, arg_uptr_b=wparam, arg_uptr_c=lparam
     *   APP_EXIT:        arg_u32_a=code
     *   PAINT_CHECK:     arg_uptr_a=hwnd
     *   APP_PAUSE:       (no args)
     *   INPUT_ACQUIRE:   arg_uptr_a=dev
     *   ZDM_SET_ACTIVE:  arg_u32_a=active
     *   POST_ACTIVATE:   arg_uptr_a=ctx
     *   LOG_CP:          arg_tag holds the cstring (copied)
     */
    uint32_t  arg_u32_a;
    uintptr_t arg_uptr_a;
    uintptr_t arg_uptr_b;
    uintptr_t arg_uptr_c;
    char      arg_tag[48];
} hook_event;

enum { HOOK_LOG_MAX = 64 };
static hook_event g_log[HOOK_LOG_MAX];
static int        g_log_count;

/* Per-test knob: paint_consumed() reads wp_paint_check's return; tests
 * that exercise the "consumed" path set this to non-zero. */
static int g_paint_check_return = 0;

static void hk_reset(void)
{
    memset(g_log, 0, sizeof g_log);
    g_log_count = 0;
    g_paint_check_return = 0;
    wp_state_init();
}

static hook_event *hk_push(hook_kind k)
{
    if (g_log_count >= HOOK_LOG_MAX) abort();
    hook_event *e = &g_log[g_log_count++];
    memset(e, 0, sizeof *e);
    e->kind = k;
    return e;
}

/* ─── hook implementations (recording stubs) ─────────────────────── */

wp_lresult wp_def_window_proc(wp_hwnd hwnd, uint32_t msg,
                               wp_wparam wparam, wp_lparam lparam)
{
    hook_event *e = hk_push(HK_DEF_WINDOW_PROC);
    e->arg_u32_a  = msg;
    e->arg_uptr_a = (uintptr_t)hwnd;
    e->arg_uptr_b = (uintptr_t)wparam;
    e->arg_uptr_c = (uintptr_t)lparam;
    return 0;
}

void wp_app_exit(int code)
{
    hook_event *e = hk_push(HK_APP_EXIT);
    e->arg_u32_a = (uint32_t)code;
}

int wp_paint_check(wp_hwnd hwnd)
{
    hook_event *e = hk_push(HK_PAINT_CHECK);
    e->arg_uptr_a = (uintptr_t)hwnd;
    return g_paint_check_return;
}

void wp_app_pause(void)
{
    (void)hk_push(HK_APP_PAUSE);
}

void wp_input_acquire(input_dev *dev)
{
    hook_event *e = hk_push(HK_INPUT_ACQUIRE);
    e->arg_uptr_a = (uintptr_t)dev;
}

void wp_zdm_set_active(int active)
{
    hook_event *e = hk_push(HK_ZDM_SET_ACTIVE);
    e->arg_u32_a = (uint32_t)active;
}

void wp_post_activate(wp_app_ctx *ctx)
{
    hook_event *e = hk_push(HK_POST_ACTIVATE);
    e->arg_uptr_a = (uintptr_t)ctx;
}

void wp_log_cp(const char *tag)
{
    hook_event *e = hk_push(HK_LOG_CP);
    if (tag != NULL) {
        strncpy(e->arg_tag, tag, sizeof e->arg_tag - 1);
        e->arg_tag[sizeof e->arg_tag - 1] = '\0';
    }
}

/* Convenience: count log events of a kind. */
static int hk_count(hook_kind k)
{
    int n = 0;
    for (int i = 0; i < g_log_count; i++) if (g_log[i].kind == k) n++;
    return n;
}

/* Convenience: find the n-th (0-based) event of a kind; NULL if not
 * enough. */
static const hook_event *hk_nth(hook_kind k, int n)
{
    for (int i = 0; i < g_log_count; i++) {
        if (g_log[i].kind == k) {
            if (n == 0) return &g_log[i];
            n--;
        }
    }
    return NULL;
}

/* ─── tests ──────────────────────────────────────────────────────── */

/* The "harmless" messages — must consume and return 0 with no side
 * effects on globals or hook calls.  Covers WM_DESTROY, WM_MOVE,
 * WM_SIZE, WM_KEYDOWN. */
int test_wp_harmless_messages_consumed(void)
{
    hk_reset();
    const uint32_t msgs[] = { WP_WM_DESTROY, WP_WM_MOVE,
                              WP_WM_SIZE,    WP_WM_KEYDOWN };
    for (size_t i = 0; i < sizeof msgs / sizeof msgs[0]; i++) {
        wp_lresult rc = wp_handle_message((wp_hwnd)0xabcd, msgs[i], 0, 0);
        T_ASSERT_EQ_I(rc, 0);
    }
    T_ASSERT_EQ_I(g_log_count, 0);
    T_ASSERT_EQ_U(g_wp_active_flag, 0u);
    return 0;
}

/* WM_CLOSE → wp_app_exit(0).  Stub returns; we just verify it was
 * called with the right code. */
int test_wp_close_calls_exit_zero(void)
{
    hk_reset();
    (void)wp_handle_message((wp_hwnd)0xabcd, WP_WM_CLOSE, 0, 0);
    T_ASSERT_EQ_I(hk_count(HK_APP_EXIT), 1);
    const hook_event *e = hk_nth(HK_APP_EXIT, 0);
    T_ASSERT_EQ_U(e->arg_u32_a, 0u);
    return 0;
}

/* An "unhandled" message (e.g. WM_QUIT 0x12 — not in any of the 4
 * branches the switch handles) → DefWindowProc with all 4 args
 * forwarded verbatim. */
int test_wp_default_forwards_to_defwindowproc(void)
{
    hk_reset();
    wp_lresult rc = wp_handle_message((wp_hwnd)0x1234, 0x0007,
                                       (wp_wparam)0xaa, (wp_lparam)0xbb);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(hk_count(HK_DEF_WINDOW_PROC), 1);
    const hook_event *e = hk_nth(HK_DEF_WINDOW_PROC, 0);
    T_ASSERT_EQ_U(e->arg_u32_a, 0x0007u);
    T_ASSERT_EQ_U(e->arg_uptr_a, 0x1234u);
    T_ASSERT_EQ_U(e->arg_uptr_b, 0xaau);
    T_ASSERT_EQ_U(e->arg_uptr_c, 0xbbu);
    return 0;
}

/* WM_PAINT with no app context → straight to DefWindowProc.  Paint
 * check is NOT invoked. */
int test_wp_paint_no_ctx_falls_through(void)
{
    hk_reset();
    wp_handle_message((wp_hwnd)0x10, WP_WM_PAINT, 0, 0);
    T_ASSERT_EQ_I(hk_count(HK_PAINT_CHECK), 0);
    T_ASSERT_EQ_I(hk_count(HK_DEF_WINDOW_PROC), 1);
    return 0;
}

/* WM_PAINT with ctx but NULL paint_check_this → DefWindowProc.  Paint
 * check is NOT invoked (short-circuit before the call). */
int test_wp_paint_ctx_but_no_paint_this_falls_through(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    g_wp_app_ctx = &ctx;
    g_wp_paint_check_this = NULL;

    wp_handle_message((wp_hwnd)0x10, WP_WM_PAINT, 0, 0);

    T_ASSERT_EQ_I(hk_count(HK_PAINT_CHECK), 0);
    T_ASSERT_EQ_I(hk_count(HK_DEF_WINDOW_PROC), 1);
    return 0;
}

/* WM_PAINT with both ctx + paint_check_this, but the paint helper
 * returns 0 → DefWindowProc.  Paint check IS called, with the
 * g_wp_paint_hwnd (NOT the WndProc's incoming hwnd). */
int test_wp_paint_helper_returns_zero_falls_through(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    g_wp_app_ctx = &ctx;
    g_wp_paint_check_this = (void *)0xaabbccdd;
    g_wp_paint_hwnd = (wp_hwnd)0x55;
    g_paint_check_return = 0;

    wp_handle_message((wp_hwnd)0x999, WP_WM_PAINT, 0, 0);

    T_ASSERT_EQ_I(hk_count(HK_PAINT_CHECK), 1);
    const hook_event *e = hk_nth(HK_PAINT_CHECK, 0);
    T_ASSERT_EQ_U(e->arg_uptr_a, 0x55u);   /* g_wp_paint_hwnd, not 0x999 */
    T_ASSERT_EQ_I(hk_count(HK_DEF_WINDOW_PROC), 1);
    return 0;
}

/* WM_PAINT consumed end-to-end → return 0, no DefWindowProc. */
int test_wp_paint_consumed_no_defwindowproc(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    g_wp_app_ctx = &ctx;
    g_wp_paint_check_this = (void *)0xdeadbeef;
    g_wp_paint_hwnd = (wp_hwnd)0x77;
    g_paint_check_return = 1;

    wp_lresult rc = wp_handle_message((wp_hwnd)0x777, WP_WM_PAINT, 0, 0);

    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(hk_count(HK_PAINT_CHECK), 1);
    T_ASSERT_EQ_I(hk_count(HK_DEF_WINDOW_PROC), 0);
    return 0;
}

/* WM_ACTIVATEAPP unconditionally mirrors wParam into g_wp_active_flag,
 * even when no app context is set.  This is the load-bearing assertion
 * — the engine's pump only watches g_wp_active_flag, so a missing
 * ctx must NOT swallow the flag write. */
int test_wp_activateapp_writes_flag_without_ctx(void)
{
    hk_reset();
    g_wp_app_ctx = NULL;
    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);
    T_ASSERT_EQ_U(g_wp_active_flag, 1u);

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)0, 0);
    T_ASSERT_EQ_U(g_wp_active_flag, 0u);

    /* No hooks fired — ctx missing → activation body is skipped. */
    T_ASSERT_EQ_I(g_log_count, 0);
    return 0;
}

/* WM_ACTIVATEAPP with ctx but ctx->loaded == 0 → flag mirrored, but
 * neither pause nor activate hooks fire.  The scene-loaded gate is
 * the second short-circuit. */
int test_wp_activateapp_loaded_zero_skips_body(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 0;
    g_wp_app_ctx = &ctx;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);
    T_ASSERT_EQ_U(g_wp_active_flag, 1u);
    T_ASSERT_EQ_I(g_log_count, 0);

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)0, 0);
    T_ASSERT_EQ_U(g_wp_active_flag, 0u);
    T_ASSERT_EQ_I(g_log_count, 0);
    return 0;
}

/* WM_ACTIVATEAPP deactivate path → wp_app_pause(). */
int test_wp_activateapp_deactivate_calls_pause(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    g_wp_app_ctx = &ctx;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)0, 0);

    T_ASSERT_EQ_U(g_wp_active_flag, 0u);
    T_ASSERT_EQ_I(hk_count(HK_APP_PAUSE), 1);
    T_ASSERT_EQ_I(hk_count(HK_INPUT_ACQUIRE), 0);
    T_ASSERT_EQ_I(hk_count(HK_LOG_CP), 0);
    return 0;
}

/* WM_ACTIVATEAPP activate path with NO input devices and NO ZDM:
 * still emits the CP3 log (unconditional in the activation body)
 * and still calls post_activate.  Verifies the "always fires"
 * portions of the activation flow. */
int test_wp_activateapp_activate_minimal_emits_cp3_and_post(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    g_wp_app_ctx = &ctx;
    /* No input devices, no ZDM. */

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    T_ASSERT_EQ_U(g_wp_active_flag, 1u);
    T_ASSERT_EQ_I(hk_count(HK_INPUT_ACQUIRE), 0);
    T_ASSERT_EQ_I(hk_count(HK_ZDM_SET_ACTIVE), 0);

    /* CP3 should be the only log marker (CP1/CP2 are conditional on
     * g_wp_input_dev_extra being non-NULL). */
    T_ASSERT_EQ_I(hk_count(HK_LOG_CP), 1);
    const hook_event *cp3 = hk_nth(HK_LOG_CP, 0);
    T_ASSERT(strcmp(cp3->arg_tag, "ActivateInputDevice CP3") == 0);

    /* post-activate is unconditional after the input chain. */
    T_ASSERT_EQ_I(hk_count(HK_POST_ACTIVATE), 1);
    T_ASSERT_EQ_U(hk_nth(HK_POST_ACTIVATE, 0)->arg_uptr_a, (uintptr_t)&ctx);
    return 0;
}

/* Full activation: extra device wired, both loop slots wired, ZDM
 * wired, log not quiet.  Verifies the call order and the CP1/CP2/CP3
 * cadence:
 *   CP1, acquire(extra), CP2, acquire(loop0), acquire(loop1),
 *   CP3, zdm_set_active(init_flag), post_activate(ctx).
 *
 * Device chain NOT wired → init_flag = 0 → ZDM gets 0. */
int test_wp_activateapp_full_chain_call_order(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    g_wp_app_ctx = &ctx;
    g_wp_input_dev_extra = (void *)0xe0;
    g_wp_input_devs[0]   = (void *)0xe1;
    g_wp_input_devs[1]   = (void *)0xe2;
    g_wp_zdm             = (void *)0xed;
    g_wp_log_quiet       = 0;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    /* Total events: 3 logs + 3 acquires + 1 zdm + 1 post = 8. */
    T_ASSERT_EQ_I(g_log_count, 8);

    T_ASSERT_EQ_I(g_log[0].kind, HK_LOG_CP);
    T_ASSERT(strcmp(g_log[0].arg_tag, "ActivateInputDevice CP1") == 0);

    T_ASSERT_EQ_I(g_log[1].kind, HK_INPUT_ACQUIRE);
    T_ASSERT_EQ_U(g_log[1].arg_uptr_a, 0xe0u);

    T_ASSERT_EQ_I(g_log[2].kind, HK_LOG_CP);
    T_ASSERT(strcmp(g_log[2].arg_tag, "ActivateInputDevice CP2") == 0);

    T_ASSERT_EQ_I(g_log[3].kind, HK_INPUT_ACQUIRE);
    T_ASSERT_EQ_U(g_log[3].arg_uptr_a, 0xe1u);

    T_ASSERT_EQ_I(g_log[4].kind, HK_INPUT_ACQUIRE);
    T_ASSERT_EQ_U(g_log[4].arg_uptr_a, 0xe2u);

    T_ASSERT_EQ_I(g_log[5].kind, HK_LOG_CP);
    T_ASSERT(strcmp(g_log[5].arg_tag, "ActivateInputDevice CP3") == 0);

    T_ASSERT_EQ_I(g_log[6].kind, HK_ZDM_SET_ACTIVE);
    T_ASSERT_EQ_U(g_log[6].arg_u32_a, 0u);   /* init_flag = 0 (chain not wired) */

    T_ASSERT_EQ_I(g_log[7].kind, HK_POST_ACTIVATE);
    T_ASSERT_EQ_U(g_log[7].arg_uptr_a, (uintptr_t)&ctx);
    return 0;
}

/* Quiet log mode → CP1/CP2/CP3 all suppressed, but acquires still
 * fire.  Verifies g_wp_log_quiet only gates the log markers, not the
 * actual input work. */
int test_wp_activateapp_quiet_suppresses_logs_only(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    g_wp_app_ctx = &ctx;
    g_wp_input_dev_extra = (void *)0xe0;
    g_wp_input_devs[0]   = (void *)0xe1;
    g_wp_log_quiet       = 1;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    T_ASSERT_EQ_I(hk_count(HK_LOG_CP), 0);
    T_ASSERT_EQ_I(hk_count(HK_INPUT_ACQUIRE), 2);
    T_ASSERT_EQ_I(hk_count(HK_POST_ACTIVATE), 1);
    return 0;
}

/* Activate with NO extra device but loop slot 1 wired (slot 0 NULL):
 * no CP1/CP2 (block skipped) but CP3 still emitted; only the
 * non-NULL loop slot is acquired. */
int test_wp_activateapp_extra_null_sparse_loop(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    g_wp_app_ctx = &ctx;
    g_wp_input_dev_extra = NULL;
    g_wp_input_devs[0]   = NULL;
    g_wp_input_devs[1]   = (void *)0xe9;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    T_ASSERT_EQ_I(hk_count(HK_LOG_CP), 1);
    T_ASSERT(strcmp(hk_nth(HK_LOG_CP, 0)->arg_tag,
                    "ActivateInputDevice CP3") == 0);

    T_ASSERT_EQ_I(hk_count(HK_INPUT_ACQUIRE), 1);
    T_ASSERT_EQ_U(hk_nth(HK_INPUT_ACQUIRE, 0)->arg_uptr_a, 0xe9u);
    return 0;
}

/* Device chain fully wired → init_flag = 1 → ZDM gets 1.  Builds the
 * two-level pointer chain manually:
 *   ctx.f00 → &sub → sub holds (int*)&inner, inner[6] (= *(int*)+0x18)
 *   is non-zero.
 *
 * The wp_handle_message walks: ctx->f00 (non-NULL), *(int**)ctx->f00
 * (non-NULL), p[0x18/sizeof(int)] (= p[6] = non-zero) → flag true. */
int test_wp_activateapp_device_chain_sets_zdm_active(void)
{
    hk_reset();
    /* Inner: an int[8] block where index 6 (== byte offset 0x18) is
     * the flag.  Allocate 8 ints so reads up to index 6 stay in bounds. */
    int inner[8] = {0};
    inner[6] = 1;

    /* Sub: a single pointer that points at `inner`.  ctx->f00 will
     * point at THIS variable; the WndProc does *(int**)ctx->f00 to
     * load the inner pointer. */
    int *sub = inner;

    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    ctx.f00    = &sub;
    g_wp_app_ctx = &ctx;
    g_wp_zdm     = (void *)0xed;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    T_ASSERT_EQ_I(hk_count(HK_ZDM_SET_ACTIVE), 1);
    T_ASSERT_EQ_U(hk_nth(HK_ZDM_SET_ACTIVE, 0)->arg_u32_a, 1u);
    return 0;
}

/* Device chain partially wired (the inner +0x18 slot is zero) →
 * init_flag = 0 → ZDM gets 0. */
int test_wp_activateapp_device_chain_partial_zdm_inactive(void)
{
    hk_reset();
    int inner[8] = {0};
    /* inner[6] left at 0 — chain visible but flag bit is off. */
    int *sub = inner;

    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    ctx.f00    = &sub;
    g_wp_app_ctx = &ctx;
    g_wp_zdm     = (void *)0xed;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    T_ASSERT_EQ_I(hk_count(HK_ZDM_SET_ACTIVE), 1);
    T_ASSERT_EQ_U(hk_nth(HK_ZDM_SET_ACTIVE, 0)->arg_u32_a, 0u);
    return 0;
}

/* WM_ACTIVATEAPP with NULL ZDM → no zdm_set_active call (even though
 * the rest of the activation runs).  Verifies the
 * `if (g_wp_zdm != NULL)` short-circuit. */
int test_wp_activateapp_null_zdm_skips_zdm_call(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.loaded = 1;
    g_wp_app_ctx = &ctx;
    g_wp_zdm     = NULL;

    wp_handle_message((wp_hwnd)0, WP_WM_ACTIVATEAPP, (wp_wparam)1, 0);

    T_ASSERT_EQ_I(hk_count(HK_ZDM_SET_ACTIVE), 0);
    T_ASSERT_EQ_I(hk_count(HK_POST_ACTIVATE), 1);  /* but post still fires */
    return 0;
}

/* WM_TIMER with ctx clears ctx->timer to 0.  Verifies the offset
 * matches retail (ctx[0x1c] / wp_app_ctx::timer). */
int test_wp_timer_clears_ctx_timer_field(void)
{
    hk_reset();
    wp_app_ctx ctx = {0};
    ctx.timer = 0xdeadbeefu;
    g_wp_app_ctx = &ctx;

    wp_lresult rc = wp_handle_message((wp_hwnd)0, WP_WM_TIMER, 0, 0);

    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_U(ctx.timer, 0u);
    /* No hooks fired — WM_TIMER is consumed in-line. */
    T_ASSERT_EQ_I(g_log_count, 0);
    return 0;
}

/* WM_TIMER with NO ctx → consumed (return 0), no crash. */
int test_wp_timer_no_ctx_consumed(void)
{
    hk_reset();
    g_wp_app_ctx = NULL;

    wp_lresult rc = wp_handle_message((wp_hwnd)0, WP_WM_TIMER, 0, 0);

    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_log_count, 0);   /* not even DefWindowProc */
    return 0;
}

/* wp_state_init zeros every module global.  Sanity check on the
 * reset path tests rely on. */
int test_wp_state_init_clears_all_globals(void)
{
    /* Pre-poison every global. */
    wp_app_ctx ctx = {0};
    g_wp_app_ctx          = &ctx;
    g_wp_active_flag      = 1;
    g_wp_paint_check_this = (void *)0xabc;
    g_wp_input_dev_extra  = (void *)0xdef;
    g_wp_input_devs[0]    = (void *)0x111;
    g_wp_input_devs[1]    = (void *)0x222;
    g_wp_zdm              = (void *)0x333;
    g_wp_paint_hwnd       = (wp_hwnd)0x444;
    g_wp_log_quiet        = 1;

    wp_state_init();

    T_ASSERT_EQ_P(g_wp_app_ctx, NULL);
    T_ASSERT_EQ_U(g_wp_active_flag, 0u);
    T_ASSERT_EQ_P(g_wp_paint_check_this, NULL);
    T_ASSERT_EQ_P(g_wp_input_dev_extra, NULL);
    T_ASSERT_EQ_P(g_wp_input_devs[0], NULL);
    T_ASSERT_EQ_P(g_wp_input_devs[1], NULL);
    T_ASSERT_EQ_P(g_wp_zdm, NULL);
    T_ASSERT_EQ_U(g_wp_paint_hwnd, 0u);
    T_ASSERT_EQ_U(g_wp_log_quiet, 0u);
    return 0;
}

/* wp_app_ctx layout: pin offsets that must match retail BSS
 * (loaded @ +8, timer @ +0x1c).  Only meaningful on a 32-bit build
 * where sizeof(void *) == 4; on 64-bit the offsets shift and the
 * test is a no-op skip. */
int test_wp_app_ctx_layout_matches_retail(void)
{
#if UINTPTR_MAX == 0xFFFFFFFFu
    T_ASSERT_EQ_U(sizeof(wp_app_ctx), 0x20u);
    T_ASSERT_EQ_U(offsetof(wp_app_ctx, f00),    0x00u);
    T_ASSERT_EQ_U(offsetof(wp_app_ctx, loaded), 0x08u);
    T_ASSERT_EQ_U(offsetof(wp_app_ctx, timer),  0x1cu);
    return 0;
#else
    T_SKIP("32-bit-only layout assertions");
#endif
}
