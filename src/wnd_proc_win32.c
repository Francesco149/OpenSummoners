/*
 * src/wnd_proc_win32.c — Win32 adapters for the WndProc hooks.
 *
 * Supplies the hook set declared in wnd_proc.h for the real (mingw)
 * build.  The host test harness provides its own recording stubs in
 * tests/test_wnd_proc.c and does NOT link this file.
 *
 * Hook implementations are intentionally minimal:
 *
 *   - wp_def_window_proc → DefWindowProcA
 *   - wp_app_exit        → ExitProcess
 *   - wp_log_cp          → OutputDebugStringA tag + newline
 *   - wp_paint_check / wp_app_pause / wp_input_acquire /
 *     wp_zdm_set_active / wp_post_activate → no-op stubs
 *
 * The no-op stubs are placeholders.  None of the underlying engine
 * subsystems (DDraw back-buffer paint, input multiplexer, sprite-pool
 * scrub) are ported yet — the WndProc port is being landed first so
 * the drop-in can at least *receive* WM_ACTIVATEAPP correctly and
 * unblock the engine's outer pump.  Each stub is one swap away from
 * delegating to the real engine functions once they're ported.
 *
 * Also exposes wp_window_proc — the `LRESULT CALLBACK` shaped wrapper
 * RegisterClassExA wants.  It just casts the Win32 types to the
 * pointer-sized wp_* equivalents and forwards to wp_handle_message.
 */
#include "wnd_proc.h"

#include <windows.h>

LRESULT CALLBACK wp_window_proc(HWND hwnd, UINT msg,
                                 WPARAM wparam, LPARAM lparam)
{
    return (LRESULT)wp_handle_message((wp_hwnd)hwnd, (uint32_t)msg,
                                       (wp_wparam)wparam,
                                       (wp_lparam)lparam);
}

wp_lresult wp_def_window_proc(wp_hwnd hwnd, uint32_t msg,
                               wp_wparam wparam, wp_lparam lparam)
{
    return (wp_lresult)DefWindowProcA((HWND)hwnd, (UINT)msg,
                                       (WPARAM)wparam, (LPARAM)lparam);
}

void wp_app_exit(int code)
{
    ExitProcess((UINT)code);
}

void wp_log_cp(const char *tag)
{
    if (tag == NULL) return;
    OutputDebugStringA(tag);
    OutputDebugStringA("\r\n");
}

int wp_paint_check(wp_hwnd hwnd)
{
    /* Placeholder: the real implementation lives in the (not-yet-ported)
     * back-buffer paint helper FUN_005b9130.  Returning 0 means "I did
     * NOT consume the WM_PAINT" — the WndProc will fall through to
     * DefWindowProcA which is the correct safe default (Windows will
     * just validate the dirty region). */
    (void)hwnd;
    return 0;
}

void wp_app_pause(void)
{
    /* Placeholder for FUN_0058ffa0(1) — pause audio + input on app
     * deactivate.  No-op until the input manager is ported. */
}

void wp_input_acquire(input_dev *dev)
{
    /* Placeholder for FUN_005ba290 — IDirectInputDevice::Acquire on
     * the wrapper at `dev`.  No-op until the input device wrappers
     * are ported. */
    (void)dev;
}

void wp_zdm_set_active(int active)
{
    /* Placeholder for FUN_005bbd20 — ZDM (input multiplexer) state
     * flip.  No-op until the ZDM is ported. */
    (void)active;
}

void wp_post_activate(wp_app_ctx *ctx)
{
    /* Placeholder for FUN_005b14c0 — sprite-cache / W_MGR / GD_MGR
     * scrub after re-activation.  No-op until the pool managers are
     * ported. */
    (void)ctx;
}
