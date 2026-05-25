/*
 * src/app_pump_win32.c — real-Win32 backend for app_pump.h hooks.
 *
 * Implements the 5 hook entry points the pure-C app_pump.c calls into:
 * PeekMessageA/TranslateMessage/DispatchMessageA, WaitMessage,
 * GetTickCount, and process exit.
 *
 * The peeked MSG is stashed in a TU-local static so subsequent calls
 * to app_pump_translate_and_dispatch can act on it without the pump's
 * pure-logic layer having to know about <windows.h>'s MSG struct.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#include "app_pump.h"

/* Holds the MSG that the most recent successful peek popped.  Lives
 * here (not in the pump) so the pure-logic TU stays Win32-free. */
static MSG g_peeked_msg;

int app_pump_peek_message(uint32_t *out_msg)
{
    if (!PeekMessageA(&g_peeked_msg, NULL, 0, 0, PM_REMOVE)) {
        return 0;
    }
    if (out_msg != NULL) {
        *out_msg = (uint32_t)g_peeked_msg.message;
    }
    return 1;
}

void app_pump_translate_and_dispatch(void)
{
    TranslateMessage(&g_peeked_msg);
    DispatchMessageA(&g_peeked_msg);
}

void app_pump_wait_message(void)
{
    WaitMessage();
}

uint32_t app_pump_get_tick_count(void)
{
    return (uint32_t)GetTickCount();
}

void app_pump_request_exit(int code)
{
    /* Retail's FUN_005bf5db is the CRT exit chain (atexit handlers
     * then ExitProcess).  `exit` is the moral equivalent — runs
     * atexit-registered destructors, flushes stdio, then exits. */
    exit(code);
}
