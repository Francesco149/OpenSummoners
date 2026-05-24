/*
 * src/wnd_proc.h — main game window's WndProc port.
 *
 * Ports FUN_005b12e0 (CLASS_LIZSOFT_SOTES::WndProc, 441 bytes / 84
 * lines of decomp).  This is the message handler the engine registers
 * for its main game window via RegisterClassExA — every WM_* the OS
 * delivers to the game flows through here.
 *
 * Why this matters: the engine's outer pump (FUN_005b1030) spins until
 * the global DAT_008a952c flips non-zero, which only happens when
 * Windows delivers WM_ACTIVATEAPP to *this* WndProc.  Up until now
 * the Frida agent (tools/frida/opensummoners-agent.js) papered over
 * this with a PostMessageA(WM_ACTIVATEAPP, TRUE) workaround because
 * hidden retail windows don't naturally see WM_ACTIVATEAPP from the
 * shell.  A correctly-ported WndProc + a real visible-or-not window
 * in the drop-in can flip the flag itself.
 *
 * Win32-free header: HWND/WPARAM/LPARAM/LRESULT are typedef'd here to
 * pointer-sized ints so the pure logic compiles and unit-tests on
 * Linux without dragging in <windows.h>.  The Win32-only adapters
 * (real DefWindowProcA, real ExitProcess, real OutputDebugStringA)
 * live in src/wnd_proc_win32.c — same split as asset_register.c vs
 * asset_register_win32.c.
 *
 * Hooks (wp_def_window_proc, wp_app_exit, wp_paint_check,
 * wp_app_pause, wp_input_acquire, wp_zdm_set_active, wp_post_activate,
 * wp_log_cp) are declared here and SUPPLIED externally: by
 * wnd_proc_win32.c in the real build, by tests/test_wnd_proc.c in the
 * test build.  The pure logic in wnd_proc.c never calls Win32
 * directly; it only invokes hooks.
 */
#ifndef OPENSUMMONERS_WND_PROC_H
#define OPENSUMMONERS_WND_PROC_H

#include <stddef.h>
#include <stdint.h>

/* Pointer-sized integers — same width as Win32 HWND/WPARAM/LPARAM/
 * LRESULT on the 32-bit drop-in target.  Tests on 64-bit Linux use
 * uintptr_t which is wider; we never round-trip these through
 * 32-bit-specific math so the width difference doesn't matter. */
typedef uintptr_t wp_hwnd;
typedef uintptr_t wp_wparam;
typedef intptr_t  wp_lparam;
typedef intptr_t  wp_lresult;

/* Window-message IDs the WndProc actually inspects.  Standard Win32
 * WM_* values; named WP_WM_* to avoid conflicting with <windows.h>
 * if a future TU pulls both headers in. */
#define WP_WM_DESTROY      0x0002u
#define WP_WM_MOVE         0x0003u
#define WP_WM_SIZE         0x0005u
#define WP_WM_PAINT        0x000fu
#define WP_WM_CLOSE        0x0010u
#define WP_WM_ACTIVATEAPP  0x001cu
#define WP_WM_KEYDOWN      0x0100u
#define WP_WM_TIMER        0x0113u

/* ─── wp_app_ctx — the "app context" struct ─────────────────────── */

/* Mirrors the layout the WndProc reads via DAT_008a9b64 (the engine's
 * top-level app context pointer).  Only the fields FUN_005b12e0 reads
 * are named; the rest are pad to preserve offsets.
 *
 * The "device init chain" is the disasm at 0x5b13b5..0x5b13c8:
 *
 *   mov eax, [ebx]        ; eax = ctx->f00
 *   test eax,eax / jz...
 *   mov eax, [eax]        ; eax = *ctx->f00 (first dword of the
 *                         ;       sub-object)
 *   test eax,eax / jz...
 *   mov ecx, [eax + 0x18] ; ecx = *(*ctx->f00 + 0x18)
 *
 * All three links must be non-NULL/non-zero for the "device init"
 * flag to read true.  When it does, the WndProc passes 1 (active) to
 * the ZDM multiplexer; when it doesn't, the ZDM is activated with 0
 * (effectively a no-op pre-init).  Modelled as `void *f00` so a test
 * can wire up the chain explicitly with two stub pointers. */
typedef struct wp_app_ctx {
    /* +0x00: head of the device-init pointer chain. */
    void    *f00;
    /* +0x04: not read by the WndProc — pad. */
    uint32_t _pad04;
    /* +0x08: "scene loaded" flag.  WM_ACTIVATEAPP is a no-op when 0
     * — both the activation and deactivation halves bail early.
     * Matches retail `DAT_008a9b64[2]`. */
    uint32_t loaded;
    /* +0x0c..+0x18: not read by the WndProc — pad. */
    uint32_t _pad0c, _pad10, _pad14, _pad18;
    /* +0x1c: WM_TIMER clears this to 0.  Matches retail
     * `DAT_008a9b64[7]`.  Semantic role is unknown — likely a
     * "tick request" the engine reads elsewhere. */
    uint32_t timer;
} wp_app_ctx;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(wp_app_ctx) == 0x20, "wp_app_ctx 32 bytes on 32-bit");
_Static_assert(offsetof(wp_app_ctx, loaded) == 0x08, "loaded offset");
_Static_assert(offsetof(wp_app_ctx, timer)  == 0x1c, "timer offset");
#endif

/* ─── module globals — mirror retail BSS slots ───────────────────── */

/* DAT_008a9b64 — app context pointer.  NULL pre-init; the engine
 * stamps it once the app singleton is constructed. */
extern wp_app_ctx *g_wp_app_ctx;

/* DAT_008a952c — "WM_ACTIVATEAPP wParam mirror".  Engine's pump
 * (FUN_005b1030) breaks out of its outer spin loop only when this
 * is non-zero.  The WndProc writes wParam straight in on every
 * WM_ACTIVATEAPP delivery — even when ctx is NULL or scene isn't
 * loaded (those just suppress the rest of the activation work). */
extern uint32_t g_wp_active_flag;

/* DAT_008a93cc — paint-check thiscall context.  Only inspected for
 * non-NULL by the WndProc itself; the actual paint helper uses it
 * as ECX and uses g_wp_paint_hwnd as the BeginPaint/EndPaint target. */
extern void *g_wp_paint_check_this;

/* DAT_008a93d8 — "extra" input device pointer.  Acquired individually
 * before the 2-iteration loop with its own "ActivateInputDevice
 * CP1/CP2" log surround.  In retail this is the keyboard device; the
 * 2-loop holds the mouse + the optional joystick. */
extern void *g_wp_input_dev_extra;

/* DAT_008a93dc / DAT_008a93e0 — 2 input device pointers, iterated in
 * order.  Each non-NULL entry gets acquired; no per-iteration log. */
extern void *g_wp_input_devs[2];

/* DAT_008a93e4 — ZDM (input multiplexer) thiscall context. */
extern void *g_wp_zdm;

/* DAT_008a93ec — paint operation's target HWND.  Note: this is
 * SEPARATE from the WndProc's own `hwnd` parameter — the paint helper
 * blits onto a window whose handle the engine stashed here at init
 * time (typically the main game window itself, but the indirection
 * keeps the WndProc generic). */
extern wp_hwnd g_wp_paint_hwnd;

/* DAT_008a6b54 — verbose-log disable flag.  Non-zero = quiet; the
 * "ActivateInputDevice CP1/CP2/CP3" trace points are skipped.
 * Retail leaves this at 0 by default. */
extern uint32_t g_wp_log_quiet;

/* Reset every WndProc-owned global to BSS-zero state.  Idempotent.
 * Tests call this in setup; the real engine relies on BSS zeroing
 * plus per-subsystem init populating the slots. */
void wp_state_init(void);

/* ─── the WndProc itself ─────────────────────────────────────────── */

/* FUN_005b12e0 — pure-C port of the WndProc body.
 *
 * Returns the Win32 LRESULT (0 for messages we consume, DefWindowProc
 * result for those we don't).
 *
 * Per-message behaviour, in priority order:
 *
 *   WM_CLOSE (0x10)
 *     wp_app_exit(0) — process exit.  In retail this never returns;
 *     the test stub records and falls through to `return 0`.
 *
 *   WM_DESTROY (2) / WM_MOVE (3) / WM_SIZE (5) / WM_KEYDOWN (0x100)
 *     Consumed (return 0).  No side effects.
 *
 *   WM_PAINT (0x0f)
 *     Consumed iff all three of (g_wp_app_ctx, g_wp_paint_check_this,
 *     wp_paint_check(g_wp_paint_hwnd)) are non-NULL/non-zero.
 *     Otherwise falls through to wp_def_window_proc.
 *
 *   WM_ACTIVATEAPP (0x1c)
 *     g_wp_active_flag = wParam  (unconditional, even when ctx==NULL).
 *
 *     If g_wp_app_ctx == NULL: stop (return 0).
 *     If ctx->loaded == 0:    stop (return 0).
 *
 *     wParam == 0 (deactivate):
 *       wp_app_pause()
 *
 *     wParam != 0 (activate):
 *       init_flag = (ctx->f00 && *ctx->f00 && (*ctx->f00)[6])
 *
 *       if (g_wp_input_dev_extra) {
 *           wp_log_cp("ActivateInputDevice CP1") if !quiet
 *           wp_input_acquire(g_wp_input_dev_extra)
 *           wp_log_cp("ActivateInputDevice CP2") if !quiet
 *       }
 *
 *       for i in 0..1:
 *           if g_wp_input_devs[i]: wp_input_acquire(g_wp_input_devs[i])
 *
 *       wp_log_cp("ActivateInputDevice CP3") if !quiet
 *
 *       if (g_wp_zdm) wp_zdm_set_active(init_flag)
 *
 *       wp_post_activate(ctx)
 *
 *   WM_TIMER (0x113)
 *     If g_wp_app_ctx != NULL: ctx->timer = 0.  Consumed.
 *
 *   All other messages
 *     wp_def_window_proc — DefWindowProcA in the real build.
 *
 * Quirks worth knowing:
 *
 *   - CP3 is logged UNCONDITIONALLY (even when the extra-device block
 *     was skipped because g_wp_input_dev_extra was NULL).  The 2-loop
 *     ALSO runs unconditionally — it just no-ops on NULL slots.
 *
 *   - The "init flag" is the boolean-NOT of retail's `bVar1`.  bVar1
 *     starts at true and flips to false only when the full
 *     ctx->f00 → *ctx->f00 → +0x18 chain is non-zero.  The ZDM arg
 *     is `!bVar1` which equals our `init_flag`.  Pre-init / no
 *     devices ⇒ init_flag=0 ⇒ ZDM gets a (somewhat odd) "deactivate"
 *     call.  Post-init ⇒ init_flag=1 ⇒ ZDM activates.
 *
 *   - WM_ACTIVATEAPP writes g_wp_active_flag BEFORE the ctx checks,
 *     which is why the engine's pump can break out even when no
 *     scene is loaded yet (it only watches g_wp_active_flag).
 */
wp_lresult wp_handle_message(wp_hwnd hwnd, uint32_t msg,
                              wp_wparam wparam, wp_lparam lparam);

/* ─── hooks — supplied by wnd_proc_win32.c or by test harness ────── */

/* DefWindowProcA equivalent.  Win32 build calls the real thing; the
 * test stub records the (hwnd, msg, wparam, lparam) tuple and returns
 * 0.  Called by wp_handle_message for messages it doesn't consume. */
wp_lresult wp_def_window_proc(wp_hwnd hwnd, uint32_t msg,
                               wp_wparam wparam, wp_lparam lparam);

/* FUN_005bf5db — process exit (chains through FUN_005bf5fd's atexit
 * registration to ExitProcess).  Real build: calls ExitProcess(code).
 * Test build: records and returns so the test can continue. */
void wp_app_exit(int code);

/* FUN_005b9130 thiscall on g_wp_paint_check_this — returns non-zero
 * iff the paint helper consumed the WM_PAINT (i.e. blitted from the
 * back buffer and the WndProc shouldn't fall back to DefWindowProc).
 * Test harness controls the return value via a knob. */
int wp_paint_check(wp_hwnd hwnd);

/* FUN_0058ffa0(1) thiscall on a fixed input-manager singleton (the
 * one at &DAT_008a6b60).  Pause the input subsystem on deactivation.
 * Real build: forwards to the engine if available, else no-op.  Test
 * build: records. */
void wp_app_pause(void);

/* FUN_005ba290 thiscall on an input-device wrapper.  Acquire the
 * underlying IDirectInputDevice (or equivalent).  Called once per
 * non-NULL input device on the activation path. */
void wp_input_acquire(void *dev);

/* FUN_005bbd20 thiscall on g_wp_zdm.  Set the multiplexer's
 * activation state (1 = active, 0 = inactive). */
void wp_zdm_set_active(int active);

/* FUN_005b14c0 thiscall on the app context — the post-activation
 * hook.  In retail this re-walks the sprite slot pool (~909
 * iterations over DAT_008a760c), the W_MGR pool, and the GD_MGR
 * table, scrubbing scratch state so the engine resumes from a
 * known-clean frame.  We model it as one opaque hook because porting
 * the body brings in three pool subsystems we haven't touched yet. */
void wp_post_activate(wp_app_ctx *ctx);

/* FUN_00408b90(tag, 0, &DAT_008a9b6c) thiscall on the log singleton
 * at &DAT_008a6620.  Emits an "ActivateInputDevice CPx" trace.  The
 * WndProc guards every call site on g_wp_log_quiet == 0. */
void wp_log_cp(const char *tag);

#endif /* OPENSUMMONERS_WND_PROC_H */
