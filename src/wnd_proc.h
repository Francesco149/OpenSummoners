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

/* ─── deep-engine struct shapes ──────────────────────────────────────
 *
 * These structs are NOT ports — they're field-offset shells used to
 * make Ghidra's decomp readable (Parse C Source on this header,
 * then re-run tools/ghidra-tag-and-export.sh).  Only the offsets the
 * WndProc and its 5 thiscall callees actually touch are named; the
 * rest is opaque padding to preserve layout.
 *
 * When any of these subsystems are *actually ported*, the host port
 * gets its own typed struct (typically renamed without the `wp_`
 * prefix) and the shape here can be merged into that file.  Until
 * then, these definitions live alongside the WndProc because the
 * WndProc is the only consumer that needs them right now.
 */

/* paint_ctx — DAT_008a93cc.  The ECX `this` for FUN_005b9130
 * (the BitBlt-from-backbuffer paint helper).  Disasm at 0x005b9130
 * pinned these field offsets:
 *
 *   +0x02c: ZDD device pointer.  vtable+0x44 = begin-frame method,
 *           vtable+0x68 = end-frame method.  Both called as
 *           `(*dev->vtable[offset])(dev, hwnd_arg)` — i.e. the helpers
 *           ride atop a DirectDraw-style class.  FUN_005b9130 itself
 *           does NOT read +0x2c — FUN_005b94e0 / FUN_005b9500 do, on
 *           their own `this`, which is `parent->back_ctx` (+0x16c).
 *   +0x138..+0x144: BitBlt destination rect (x, y, w, h) — the
 *           paint helper does `BitBlt(hdc, x, y, w, h, src_hdc,
 *           0, 0, SRCCOPY)` after FUN_005b94e0 fills in src_hdc.
 *   +0x164: state machine.  Paint is only consumed when this reads
 *           exactly 2; other values fall through to DefWindowProc.
 *           Likely "front-buffer-locked / ready-to-blit" state.
 *   +0x16c: pointer to the "back buffer" paint_ctx — a sibling
 *           instance of the same struct.  FUN_005b9130 hands this
 *           pointer to FUN_005b94e0 / FUN_005b9500 as their ECX.
 *           Confirms paint_ctx is a recursive type with front/back
 *           pair semantics (front owns the BitBlt rect, back owns
 *           the ZDD device).
 */
typedef struct paint_ctx {
    uint8_t  _pad000[0x02c];
    void    *zdd_device;          /* +0x02c — used via back_ctx */
    uint8_t  _pad030[0x138 - 0x030];
    int32_t  blit_x;              /* +0x138 */
    int32_t  blit_y;              /* +0x13c */
    int32_t  blit_w;              /* +0x140 */
    int32_t  blit_h;              /* +0x144 */
    uint8_t  _pad148[0x164 - 0x148];
    int32_t  state;               /* +0x164 — == 2 means "ready to blit" */
    uint8_t  _pad168[0x16c - 0x168];
    struct paint_ctx *back_ctx;   /* +0x16c — back-buffer sibling */
} paint_ctx;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(paint_ctx, zdd_device) == 0x02c, "paint_ctx zdd_device");
_Static_assert(offsetof(paint_ctx, blit_x)     == 0x138, "paint_ctx blit_x");
_Static_assert(offsetof(paint_ctx, blit_h)     == 0x144, "paint_ctx blit_h");
_Static_assert(offsetof(paint_ctx, state)      == 0x164, "paint_ctx state");
_Static_assert(offsetof(paint_ctx, back_ctx)   == 0x16c, "paint_ctx back_ctx");
#endif

/* input_dev — DAT_008a93d8 (extra/keyboard) and DAT_008a93dc[2]
 * (mouse + optional joystick).  Each pointer points to one of these.
 * The ECX `this` for FUN_005ba290 (input acquire).
 *
 *   +0x04: pointer to underlying device object that owns the
 *          vtable.  `(*(*this->dev_obj)->vtable[7])(dev_obj)` is the
 *          Acquire call — vtable index 7 = byte offset 0x1c.
 *   +0x08: "acquired" flag.  Set to 1 on successful Acquire; the
 *          WndProc doesn't read it but the rest of the input
 *          subsystem does.
 *
 * Real shape is larger (probably ~0x20-0x40 bytes of state for the
 * wrapper); only these two fields are pinned here.
 */
typedef struct input_dev {
    uint32_t _pad00;
    void    *dev_obj;          /* +0x04 — vtable owner */
    uint32_t acquired;         /* +0x08 — 0 = not yet, 1 = held */
} input_dev;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(input_dev, dev_obj)  == 0x04, "input_dev dev_obj");
_Static_assert(offsetof(input_dev, acquired) == 0x08, "input_dev acquired");
#endif

/* zdm_entry — one element of the array pointed to by zdm.entries.
 * Stride is 0x38 bytes (observed in FUN_005bbd20's `i * 0x38` indexing).
 *
 *   +0x00: device pointer.  NULL entries are skipped by zdm_set_active.
 *   +0x08: secondary device pointer with its own vtable.  Used by
 *          FUN_005bad20 (vtable+0x1c, deactivate) and FUN_005bad40
 *          (vtable+0x24, activate).  Probably the actual DirectInput
 *          device wrapper.
 *   +0x0c: tertiary device pointer.  Used by FUN_005bae30
 *          (vtable+0x38, set-format/activate-with-state) and
 *          FUN_005bae70 (vtable+0x3c, get-state cookie).  Distinct
 *          vtable from +0x08.
 *   +0x20: "active" state.  Compared to the requested state in
 *          zdm_set_active; entries already in the requested state are
 *          left alone.
 *   +0x24: second state dword.  Written together with the active
 *          flag.
 *   +0x28: 8-byte cookie pair (passed to FUN_005bae30 on deactivate,
 *          overwritten by FUN_005bae70's return value on activate).
 *          Probably a "device state handle" the engine uses to
 *          remember what to restore.
 */
typedef struct zdm_entry {
    void    *dev;              /* +0x00 */
    uint32_t _pad04;
    void    *dev_b;            /* +0x08 */
    void    *dev_c;            /* +0x0c */
    uint8_t  _pad10[0x10];
    uint32_t active;           /* +0x20 */
    uint32_t state2;           /* +0x24 */
    uint64_t cookie;           /* +0x28 — 8 bytes, 2 dwords */
    uint8_t  _pad30[0x38 - 0x30];
} zdm_entry;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(zdm_entry) == 0x38, "zdm_entry 0x38 stride");
_Static_assert(offsetof(zdm_entry, dev_b)  == 0x08, "zdm_entry dev_b");
_Static_assert(offsetof(zdm_entry, dev_c)  == 0x0c, "zdm_entry dev_c");
_Static_assert(offsetof(zdm_entry, active) == 0x20, "zdm_entry active");
_Static_assert(offsetof(zdm_entry, cookie) == 0x28, "zdm_entry cookie");
#endif

/* zdm — DAT_008a93e4.  The ECX `this` for FUN_005bbd20
 * (zdm_set_active).  In retail this is "Z Direct Manager" or similar
 * — an input-multiplexer that fans the WndProc's activate/deactivate
 * call across N input devices.
 *
 *   +0x18: pointer to entries array (stride 0x38 — see zdm_entry).
 *   +0x1c: ushort entry count.
 *   +0x2c: name string (CHAR buffer inside the struct).  Other
 *          callers in the engine pass `&zdm->name[0]` as an LPCSTR
 *          to the log singleton — confirms it's an inline C-string.
 *
 * Real shape is larger (the name buffer alone is presumably 0x20-0x40
 * chars); only the offsets the WndProc / zdm_set_active touch are
 * pinned.  The 0x40-byte size assumption keeps the struct large
 * enough to span the name field.
 */
typedef struct zdm {
    uint8_t   _pad00[0x18];
    zdm_entry *entries;        /* +0x18 */
    uint16_t  count;           /* +0x1c */
    uint8_t   _pad1e[0x2c - 0x1e];
    char      name[0x14];      /* +0x2c — inline C-string */
} zdm;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(zdm, entries) == 0x18, "zdm entries");
_Static_assert(offsetof(zdm, count)   == 0x1c, "zdm count");
_Static_assert(offsetof(zdm, name)    == 0x2c, "zdm name");
#endif

/* input_mgr — DAT_008a6b60 + neighbours; lives at a fixed BSS address
 * (NOT a pointer; the ECX setup in the disasm is `mov ecx,
 * 0x8a6b60`).  The ECX `this` for FUN_0058ffa0 (input pause-on-
 * deactivate), called by the WndProc with arg=1.  FUN_0058ffa0
 * forwards to FUN_005bbd20 (zdm_set_active) — so the input manager
 * essentially OWNS the zdm pointer at +0x2884 and the deactivate path
 * just chains through.
 *
 * The 0x2884 offset is unusually large for a one-field struct because
 * this struct is multi-purpose — see
 * `docs/findings/0057ca40-rabbit-hole.md` §7.  It's the same singleton
 * that FUN_004179b0 (the SS_MGR slot-clone) accesses with:
 *   +0x0aac  sprite-slot pointer table (909 × 4 B; absolute 0x8a760c)
 *   +0x18e0  info-entry pointer table  (909 × 4 B; absolute 0x8a8440)
 * Both tables land within our 0x2884 opaque head — the asset-register
 * module models them as standalone globals (g_ar_sprite_table /
 * g_ar_info_table) since the host-side accessors don't need the
 * `this` pointer plumbed through.  Once an SS_MGR-thiscall consumer
 * gets ported, the model may unify.
 */
typedef struct input_mgr {
    uint8_t _pad000[0x2884];
    zdm    *zdm_ptr;           /* +0x2884 */
} input_mgr;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(input_mgr, zdm_ptr) == 0x2884, "input_mgr zdm_ptr");
#endif

/* log_singleton — DAT_008a6620 + neighbours; lives at a fixed BSS
 * address (NOT a pointer; the ECX setup in the disasm for every
 * "ActivateInputDevice CPx" call is `mov ecx, 0x8a6620`).  The ECX
 * `this` for FUN_00408b90 (log a message + optional GetLastError
 * decoration).
 *
 *   +0x404: CHAR buffer holding the log-file path.  Passed as the
 *           first arg to FUN_005bf4e8 (fopen-with-mode "r+") which
 *           returns a FILE* (or equivalent) that the log fn writes
 *           to.  Length is at least MAX_PATH (0x100); the surrounding
 *           pad is reserved for the buffer.
 *
 * The 0x404 head is presumably a 0x404-byte input buffer
 * (formatting scratch + state).  Nothing the WndProc cares about.
 */
typedef struct log_singleton {
    uint8_t _pad000[0x404];
    char    path[0x100];       /* +0x404 — log-file path */
} log_singleton;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(log_singleton, path) == 0x404, "log_singleton path");
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
extern paint_ctx *g_wp_paint_check_this;

/* DAT_008a93d8 — "extra" input device pointer.  Acquired individually
 * before the 2-iteration loop with its own "ActivateInputDevice
 * CP1/CP2" log surround.  In retail this is the keyboard device; the
 * 2-loop holds the mouse + the optional joystick. */
extern input_dev *g_wp_input_dev_extra;

/* DAT_008a93dc / DAT_008a93e0 — 2 input device pointers, iterated in
 * order.  Each non-NULL entry gets acquired; no per-iteration log. */
extern input_dev *g_wp_input_devs[2];

/* DAT_008a93e4 — ZDM (input multiplexer) thiscall context. */
extern zdm *g_wp_zdm;

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
void wp_input_acquire(input_dev *dev);

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
