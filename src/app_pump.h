/*
 * src/app_pump.h — main message pump / frame waiter port.
 *
 * Ports FUN_005b1030 (156 bytes, 33 lines of decomp).  The function the
 * scene runner calls twice per loop iteration to flush the OS message
 * queue, suspend on `WaitMessage` until a frame slot opens up, and then
 * stamp a "throttle" flag that the next iteration will see.
 *
 * In retail this function:
 *   1. PeekMessage-loops the OS queue.  WM_QUIT (0x12) calls
 *      FUN_005bf5db (the CRT `exit` chain).  Anything else gets
 *      TranslateMessage + DispatchMessageA.
 *   2. After the queue drains, breaks the outer loop only when both
 *        - the app is active (DAT_008a952c != 0), AND
 *        - the throttle slot (app_ctx[+0x1c]) is 0.
 *      Otherwise WaitMessage()s and re-enters PeekMessage.
 *   3. If the frame-limiter is enabled (app_ctx[+0x0c] != 0), it may
 *      re-arm the throttle flag (set app_ctx[+0x1c] = 1) and always
 *      updates the last-tick timestamp (app_ctx[+0x10]).  The re-arm
 *      condition is unsigned `prev - now < 5`, i.e. the GetTickCount
 *      hasn't advanced 5 ms past the previous frame's sample — a sub-
 *      tick spin guard that holds the throttle until the WM_TIMER
 *      handler clears it back to 0.
 *
 * The WndProc clears the throttle flag on every WM_TIMER (0x113) — see
 * wnd_proc.c.  So the WM_TIMER cadence is what paces the pump.
 *
 * The app context struct lives here (instead of wnd_proc.h, where its
 * +0x08 / +0x1c fields were first named) because the pump is the
 * canonical owner — it touches +0x0c / +0x10 / +0x1c, the WndProc only
 * looks at +0x00 / +0x08 / +0x1c.  Both subsystems pull this header.
 *
 * Win32 split: the pure logic lives in app_pump.c and is Win32-free.
 * Five hooks (peek_message, translate_and_dispatch, wait_message,
 * get_tick_count, request_exit) are SUPPLIED externally — by
 * app_pump_win32.c in the real build, by tests/test_app_pump.c under
 * the host harness.
 */
#ifndef OPENSUMMONERS_APP_PUMP_H
#define OPENSUMMONERS_APP_PUMP_H

#include <stddef.h>
#include <stdint.h>

/* ─── app_ctx — the engine's "app context" struct ─────────────────── */

/* Mirrors the layout of the heap block allocated at FUN_00562210's
 * `operator_new(0x20)` call and stored at DAT_008a9b64.  Only the
 * fields the engine actually touches are named; everything else is
 * pad to preserve byte-exact offsets.
 *
 * Field map:
 *   +0x00  f00              head of the device-init pointer chain.
 *                           WndProc walks `*ctx->f00`, then offset
 *                           +0x18 inside that.  All three links must
 *                           be non-NULL for "device init done".
 *   +0x04  _pad04           not read by any port we've ever seen.
 *   +0x08  loaded           "scene loaded" flag.  Both halves of
 *                           WM_ACTIVATEAPP bail early when this is 0.
 *                           Allocator inits it to 0.
 *   +0x0c  limiter_enable   pump frame-limiter master enable.  When
 *                           0, the pump never touches +0x10 / +0x1c
 *                           post-wait — i.e. relies entirely on the
 *                           WM_TIMER+throttle pairing for cadence.
 *                           Default is dirty after the allocator;
 *                           presumably stamped by an unported settings
 *                           load (likely FUN_005a4770).
 *   +0x10  last_tick_ms     GetTickCount sample from the previous
 *                           pump exit.  Read+written when limiter is
 *                           on.  Zero on the first call (the
 *                           allocator does NOT zero this field, but
 *                           the engine's first pump call still hits
 *                           the "first frame" branch in practice; we
 *                           rely on caller-side zero-init).
 *   +0x14  _pad14           not read.
 *   +0x18  _pad18           not read.
 *   +0x1c  pump_throttle    pump-owned: set to 1 when the limiter
 *                           re-arms after a frame; cleared back to 0
 *                           by the WndProc's WM_TIMER handler.  Pump
 *                           breaks its outer wait only when this is 0
 *                           AND g_app_active_flag != 0.
 *
 * The allocator at FUN_00562210 explicitly zeroes only +0x00 and
 * +0x08 (`*puVar3 = 0; puVar3[2] = 0`).  Other fields rely on
 * operator_new returning zeroed memory, OR on later subsystems
 * stamping them.  Tests should treat the struct as a stack-allocated
 * `{0}` shell so every field is deterministic.
 */
typedef struct app_ctx {
    void    *f00;              /* +0x00 */
    uint32_t _pad04;           /* +0x04 */
    uint32_t loaded;           /* +0x08 */
    uint32_t limiter_enable;   /* +0x0c */
    uint32_t last_tick_ms;     /* +0x10 */
    uint32_t _pad14;           /* +0x14 */
    uint32_t _pad18;           /* +0x18 */
    uint32_t pump_throttle;    /* +0x1c */
} app_ctx;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(app_ctx)                       == 0x20, "app_ctx 32 bytes on 32-bit");
_Static_assert(offsetof(app_ctx, loaded)             == 0x08, "loaded offset");
_Static_assert(offsetof(app_ctx, limiter_enable)     == 0x0c, "limiter_enable offset");
_Static_assert(offsetof(app_ctx, last_tick_ms)       == 0x10, "last_tick_ms offset");
_Static_assert(offsetof(app_ctx, pump_throttle)      == 0x1c, "pump_throttle offset");
#endif

/* ─── module globals — mirror retail BSS slots ───────────────────── */

/* DAT_008a9b64 — app context pointer.  NULL pre-init; the engine
 * stamps it once the app singleton is constructed (FUN_00562210). */
extern app_ctx *g_app_ctx;

/* DAT_008a952c — "WM_ACTIVATEAPP wParam mirror".  The pump's outer
 * wait loop breaks only when this is non-zero (and pump_throttle is
 * zero).  The WndProc writes wParam straight in on every
 * WM_ACTIVATEAPP delivery — even when ctx is NULL or scene isn't
 * loaded (those just suppress the rest of the activation work). */
extern uint32_t g_app_active_flag;

/* Reset every pump-owned global to BSS-zero state.  Idempotent.
 * Tests call this in setup; the real engine relies on BSS zeroing
 * plus FUN_00562210 populating g_app_ctx. */
void app_pump_state_init(void);

/* ─── the pump itself ────────────────────────────────────────────── */

/* FUN_005b1030 — pure-C port of the pump body.
 *
 * Precondition: g_app_ctx must be non-NULL.  Retail crashes if it's
 * NULL on entry (the limiter section unconditionally dereferences
 * esi/app_ctx).  We pass through silently in that case — caller's
 * contract is to set up g_app_ctx before invoking.
 *
 * Behaviour:
 *   1. Drain the OS message queue via app_pump_peek_message.  WM_QUIT
 *      (0x12) calls app_pump_request_exit(0) and returns (retail's
 *      exit does not return; we return for testability).  Other
 *      messages get translate+dispatch.
 *   2. Once the queue is empty, check (g_app_active_flag != 0 &&
 *      g_app_ctx->pump_throttle == 0).  If yes, fall through to step
 *      3.  Otherwise WaitMessage and loop back to step 1.
 *   3. If g_app_ctx->limiter_enable != 0:
 *        - if last_tick_ms == 0 → pump_throttle = 1
 *        - else if (uint32_t)(last_tick_ms - get_tick_count()) < 5
 *               → pump_throttle = 1
 *        - in both cases: last_tick_ms = get_tick_count() (second call)
 *      If limiter_enable == 0: no-op.
 *   4. Return.
 */
void app_pump_frame(void);

/* ─── hooks — supplied by app_pump_win32.c or by test harness ────── */

/* PeekMessageA(NULL, 0, 0, PM_REMOVE).  Returns 1 if a message was
 * popped (and stashed internally for a subsequent
 * app_pump_translate_and_dispatch); writes the WM_* code to *out_msg.
 * Returns 0 if the queue was empty (out_msg untouched). */
int app_pump_peek_message(uint32_t *out_msg);

/* TranslateMessage + DispatchMessageA on the most recently peeked
 * message.  Win32 build wraps it; test stub just bumps a counter. */
void app_pump_translate_and_dispatch(void);

/* WaitMessage().  Real build blocks until any message arrives; test
 * stub typically clears g_app_ctx->pump_throttle / sets
 * g_app_active_flag so the next loop iteration exits. */
void app_pump_wait_message(void);

/* GetTickCount() in milliseconds.  Real build wraps the Win32 call;
 * test stub returns a controllable value. */
uint32_t app_pump_get_tick_count(void);

/* FUN_005bf5db — process exit (CRT atexit chain → ExitProcess).
 * Real build calls `exit(code)`; test stub records and returns. */
void app_pump_request_exit(int code);

#endif /* OPENSUMMONERS_APP_PUMP_H */
