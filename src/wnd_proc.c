/*
 * src/wnd_proc.c — pure-logic port of FUN_005b12e0 (main game window
 * WndProc).  Win32-free; every external touch goes through a hook
 * declared in wnd_proc.h and supplied by either wnd_proc_win32.c
 * (real build) or the test harness.
 *
 * Per-function provenance and the disassembly-level reasoning behind
 * each branch live in the wnd_proc.h header comment + the inline
 * comments below.
 */
#include "wnd_proc.h"

#include <stddef.h>

/* ─── module globals — BSS-zero initial state ────────────────────── */

wp_app_ctx *g_wp_app_ctx;
uint32_t    g_wp_active_flag;
void       *g_wp_paint_check_this;
void       *g_wp_input_dev_extra;
void       *g_wp_input_devs[2];
void       *g_wp_zdm;
wp_hwnd     g_wp_paint_hwnd;
uint32_t    g_wp_log_quiet;

void wp_state_init(void)
{
    g_wp_app_ctx          = NULL;
    g_wp_active_flag      = 0;
    g_wp_paint_check_this = NULL;
    g_wp_input_dev_extra  = NULL;
    g_wp_input_devs[0]    = NULL;
    g_wp_input_devs[1]    = NULL;
    g_wp_zdm              = NULL;
    g_wp_paint_hwnd       = 0;
    g_wp_log_quiet        = 0;
}

/* ─── helpers ────────────────────────────────────────────────────── */

/* Reads `*(*ctx->f00 + 0x18)` with each link guarded for non-NULL.
 * Mirrors disasm at 0x005b13b5..0x005b13c8 byte-for-byte.  Returns
 * 1 iff every link is non-zero, 0 otherwise. */
static int activation_device_init_flag(const wp_app_ctx *ctx)
{
    if (ctx->f00 == NULL) return 0;
    int *p = *(int **)ctx->f00;
    if (p == NULL) return 0;
    return p[0x18 / sizeof(int)] != 0;
}

/* WM_PAINT branch — consume the message iff all three preconditions
 * hold: an app context exists, a paint-check thiscall context exists,
 * and the paint helper itself reports "blit consumed".  Matches the
 * triple-short-circuit at disasm 0x005b1305..0x005b132c. */
static int paint_consumed(void)
{
    if (g_wp_app_ctx == NULL) return 0;
    if (g_wp_paint_check_this == NULL) return 0;
    return wp_paint_check(g_wp_paint_hwnd) != 0;
}

/* WM_ACTIVATEAPP deactivation half (wParam == 0).  No-op when no app
 * context or no loaded scene; otherwise pause the input subsystem. */
static void on_deactivate(void)
{
    wp_app_ctx *ctx = g_wp_app_ctx;
    if (ctx == NULL) return;
    if (ctx->loaded == 0) return;
    wp_app_pause();
}

/* WM_ACTIVATEAPP activation half (wParam != 0).  No-op when no app
 * context or no loaded scene; otherwise:
 *   - acquire the "extra" input device (with CP1/CP2 log surround)
 *   - acquire each non-NULL entry in the 2-slot input device array
 *   - emit the unconditional CP3 log marker
 *   - if ZDM exists, set its activation state to init_flag
 *   - run the post-activate hook (sprite-cache scrub etc.)
 * Order and logging cadence match retail FUN_005b12e0 exactly — the
 * pump and the audio mixer both depend on this re-acquire sequence
 * before they'll resume frame production. */
static void on_activate(void)
{
    wp_app_ctx *ctx = g_wp_app_ctx;
    if (ctx == NULL) return;
    if (ctx->loaded == 0) return;

    int init_flag = activation_device_init_flag(ctx);

    if (g_wp_input_dev_extra != NULL) {
        if (g_wp_log_quiet == 0) wp_log_cp("ActivateInputDevice CP1");
        wp_input_acquire(g_wp_input_dev_extra);
        if (g_wp_log_quiet == 0) wp_log_cp("ActivateInputDevice CP2");
    }

    for (int i = 0; i < 2; i++) {
        if (g_wp_input_devs[i] != NULL) {
            wp_input_acquire(g_wp_input_devs[i]);
        }
    }

    if (g_wp_log_quiet == 0) wp_log_cp("ActivateInputDevice CP3");

    if (g_wp_zdm != NULL) {
        wp_zdm_set_active(init_flag);
    }

    wp_post_activate(ctx);
}

/* ─── the dispatch ───────────────────────────────────────────────── */

wp_lresult wp_handle_message(wp_hwnd hwnd, uint32_t msg,
                              wp_wparam wparam, wp_lparam lparam)
{
    switch (msg) {
    case WP_WM_CLOSE:
        /* Retail jumps to FUN_005bf5db(0) which doesn't return.  We
         * still write `return 0` in case the test stub records and
         * falls through. */
        wp_app_exit(0);
        return 0;

    case WP_WM_DESTROY:
    case WP_WM_MOVE:
    case WP_WM_SIZE:
        return 0;

    case WP_WM_PAINT:
        if (paint_consumed()) return 0;
        return wp_def_window_proc(hwnd, msg, wparam, lparam);

    case WP_WM_ACTIVATEAPP:
        g_wp_active_flag = (uint32_t)wparam;
        if (wparam == 0) {
            on_deactivate();
        } else {
            on_activate();
        }
        return 0;

    case WP_WM_KEYDOWN:
        return 0;

    case WP_WM_TIMER:
        if (g_wp_app_ctx != NULL) {
            g_wp_app_ctx->timer = 0;
        }
        return 0;

    default:
        return wp_def_window_proc(hwnd, msg, wparam, lparam);
    }
}
