/*
 * src/app_pump.c — pure-logic port of FUN_005b1030 (main pump /
 * frame waiter).  Win32-free; every external touch goes through a
 * hook declared in app_pump.h and supplied by either
 * app_pump_win32.c (real build) or the test harness.
 *
 * Per-function provenance + the disassembly-level reasoning behind
 * each branch live in the app_pump.h header.
 */
#include "app_pump.h"

#include <stddef.h>

/* WM_QUIT — bare-int constant so this TU doesn't pull <windows.h>. */
#define APP_PUMP_WM_QUIT  0x0012u

/* ─── module globals — BSS-zero initial state ────────────────────── */

app_ctx *g_app_ctx;
uint32_t g_app_active_flag;

void app_pump_state_init(void)
{
    g_app_ctx         = NULL;
    g_app_active_flag = 0;
}

/* ─── the pump body ──────────────────────────────────────────────── */

void app_pump_frame(void)
{
    /* Retail dereferences DAT_008a9b64 unconditionally in the wait
     * loop's exit check + the limiter section, so it crashes when
     * ctx is NULL.  We treat NULL ctx as "no work to do" — the
     * caller's contract is to stamp g_app_ctx before invoking the
     * pump, and this guard just keeps a misuse from spinning the
     * wait loop forever (since the exit condition can never become
     * true with no ctx). */
    if (g_app_ctx == NULL) return;

    /* Outer wait loop.  Drains the OS queue, then either exits (so
     * the caller can produce a frame) or sleeps on WaitMessage and
     * tries again. */
    for (;;) {
        uint32_t msg;
        while (app_pump_peek_message(&msg)) {
            if (msg == APP_PUMP_WM_QUIT) {
                /* Retail jumps to FUN_005bf5db(0) which does not
                 * return.  Our port returns afterwards for
                 * testability; the win32 backend's exit() never
                 * actually comes back, so this is harmless live. */
                app_pump_request_exit(0);
                return;
            }
            app_pump_translate_and_dispatch();
        }

        /* Exit-the-wait condition: the app is active AND the limiter's
         * throttle flag has been cleared (typically by WM_TIMER).
         * Match the dual short-circuit at disasm 0x005b1073..0x005b1087. */
        if (g_app_active_flag != 0 && g_app_ctx->pump_throttle == 0) {
            break;
        }

        app_pump_wait_message();
    }

    /* Limiter section — only runs after the wait loop breaks.
     * Mirrors disasm 0x005b1098..0x005b10c1.  Skipped when the
     * limiter master switch is off. */
    if (g_app_ctx->limiter_enable == 0) return;

    /* Re-arm the throttle when either:
     *   - this is the first call (last_tick_ms == 0), OR
     *   - GetTickCount hasn't advanced 5 ms past the previous sample.
     *
     * The retail asm at 0x5b10b1..0x5b10b6 reads:
     *     sub  eax, ecx        ; ecx = prev - now  (note operand order)
     *     cmp  ecx, 5
     *     jae  skip            ; UNSIGNED compare — skip throttle if >= 5
     *
     * Unsigned `prev - now < 5` is true only when `now` is in the
     * closed interval [prev - 4, prev] — i.e. the millisecond clock
     * hasn't ticked since the previous sample (or has briefly gone
     * backwards by a few ms, which Windows' GetTickCount can do under
     * timer-resolution shenanigans).  In effect: hold the throttle
     * until the OS clock advances past the previous frame's tick. */
    if (g_app_ctx->last_tick_ms == 0) {
        g_app_ctx->pump_throttle = 1;
    } else {
        uint32_t now = app_pump_get_tick_count();
        if ((uint32_t)(g_app_ctx->last_tick_ms - now) < 5) {
            g_app_ctx->pump_throttle = 1;
        }
    }

    /* Second GetTickCount call: matches the standalone `call *%edi;
     * mov %eax, 0x10(%esi)` at 0x5b10bf.  Distinct sample from the
     * one inside the comparison above. */
    g_app_ctx->last_tick_ms = app_pump_get_tick_count();
}
