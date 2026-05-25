/*
 * src/cs_dispatch.h — outer CreateScreen mode-dispatcher port.
 *
 * Ports FUN_00582e90 (3560 bytes), the engine's per-mode driver around
 * `zdd_create_screen`.  Five launcher modes (Full / Safe / Windowed /
 * DB / Zoom) each carry a slightly different preamble (SetDisplayMode +
 * hide_cursor + busy_wait_ms in the fullscreen branches; nothing in
 * Windowed; GetDisplayMode + centre-rect math in Zoom) and a per-mode
 * failure string the dispatcher routes through the engine's
 * shared error-log builder.
 *
 * Win32-free split mirrors zdd.h: pure logic in cs_dispatch.c, Win32
 * primitives extern'd here and defined in either cs_dispatch_win32.c
 * (real build) or tests/test_cs_dispatch.c (host build).  Globals that
 * back the retail BSS state (DAT_008a6ec0 / DAT_008a6eac/_eb0 /
 * DAT_008a9530 / DAT_008a9534) are file-extern'd so host tests can
 * inspect them between dispatch calls.
 *
 * Provenance: docs/findings/ddraw-init.md "FUN_00582e90 — mode-dispatch
 * CreateScreen" + docs/decompiled/by-address/582e90.c.  Error strings
 * verified via `r2 ps @ <addr>` against vendor/unpacked/sotes.unpacked.exe.
 */
#ifndef OPENSUMMONERS_CS_DISPATCH_H
#define OPENSUMMONERS_CS_DISPATCH_H

#include <stdint.h>

#include "zdd.h"

/* ─── module globals (mirror retail BSS) ──────────────────────────── */

/* DAT_008a6ec0 — the mode-0 "primary surface pair" ZDDObject.  Released
 * at the top of every dispatch (so a mode change after a prior mode-0
 * boot drops the old pair) and re-allocated only by the mode-0 branch
 * via zdd_object_new.  The other modes leave this NULL — they keep the
 * primary surface on the ZDD's own +0x16c slot instead.
 *
 * Owned by this module; cleanup at process exit is by the engine's
 * top-level shutdown (not this module's responsibility). */
extern zdd_object *cs_primary_pair;

/* DAT_008a6eac / DAT_008a6eb0 — mode-4 (Zoom) display-mode overrides.
 * When both are >= 1 the dispatcher uses these as the "desktop
 * dimensions" and skips the GetDisplayMode call.  Default 0 → real
 * GetDisplayMode runs.  Set by other (unported) settings paths. */
extern int32_t cs_zoom_override_width;
extern int32_t cs_zoom_override_height;

/* DAT_008a9530 — log-dirty flag.  Set to 1 by every failure path before
 * the strcat builder runs.  Reset by external (unported) consumers. */
extern int32_t cs_log_dirty;

/* DAT_008a9534 — 0x638-byte rolling error-log buffer.  Each failure
 * path strcats its fixed message + the engine-name header into this
 * buffer (joined with "\n" when multiple entries accumulate).  The
 * buffer is shared with other modules' failure paths; we leave it
 * intact across dispatch calls. */
extern char cs_log_buf[0x638];

/* DAT_008a9b6c — engine-name header string (e.g. "Fortune Summoners").
 * Empty at boot; populated by an unported settings init path.  Both
 * the strcat builder AND the fatal-log primitive consume it. */
extern const char *cs_engine_name_header;

/* ─── dispatch driver ─────────────────────────────────────────────── */

/* FUN_00582e90 — outer CreateScreen mode dispatch.  Drives
 * zdd_create_screen with the right args for the selected launcher mode,
 * handles the per-mode preamble (SetDisplayMode + hide_cursor +
 * busy_wait_ms for fullscreen modes 0/1/3/4; bare CreateScreen for
 * mode 2; GetDisplayMode + centre-rect for mode 4) and routes failures
 * through the shared error-log builder + cs_exit.
 *
 * Args:
 *   self           — the boot-time ZDD (DAT_008a93cc).  Passed as `this`
 *                    to zdd_create_screen and as the log_owner to the
 *                    DDERR wrappers indirectly.
 *   launcher_mode  — 0..4 from the launcher dialog radio (retail reads
 *                    this from *(int*)(in_ECX + 4) where in_ECX is the
 *                    launcher-settings record; we accept it as an
 *                    explicit arg to keep the port testable without
 *                    modelling that struct).
 *   zoom_target_w  — mode-4 only: minimum playable width (1280 retail).
 *                    Read from *(int*)(in_ECX + 0x14) at retail.
 *   zoom_target_h  — mode-4 only: minimum playable height (960 retail).
 *                    Read from *(int*)(in_ECX + 0x18) at retail.
 *
 * Returns normally on success; aborts via cs_exit() on any failure.
 * Retail is declared void and treats every failure as fatal — we mirror
 * that exactly. */
void cs_dispatch_create_screen(zdd *self, int launcher_mode,
                               int32_t zoom_target_w, int32_t zoom_target_h);

/* ─── pure-logic helpers (exposed for testing) ────────────────────── */

/* Centre-rect math for mode 4 (Zoom).  Given the actual display
 * dimensions and the launcher's requested playable size, fills the
 * 7-int blob `zdd_create_screen` expects in mode 4:
 *   out[0] = display_w
 *   out[1] = display_h
 *   out[2] = 2                     (sentinel; retail hardcodes this)
 *   out[3] = max(0, (display_w - zoom_w) / 2)   (centre_x)
 *   out[4] = max(0, (display_h - zoom_h) / 2)   (centre_y)
 *   out[5] = min(display_w, zoom_w)             (src_w)
 *   out[6] = min(display_h, zoom_h)             (src_h)
 *
 * Retail's "max with zero" is the bit-trick `(x < 0) - 1 & x` — we
 * use the plain branch.  Equivalent observable result. */
void cs_compute_zoom_rect(int32_t display_w, int32_t display_h,
                          int32_t zoom_w,    int32_t zoom_h,
                          int32_t out_rect7[7]);

/* The "strcat with \n separator" builder retail inlines at every fail
 * site.  Pure logic over cs_log_buf:
 *   - Set cs_log_dirty = 1
 *   - if cs_log_buf is empty: strcpy fixed_msg in
 *     else: strcat "\n" + strcat fixed_msg
 *   - strcat the engine-name header (always)
 *
 * Bounded by the buffer size — silently truncates if appending would
 * overflow (retail's inline sequence has no bounds check; we err on
 * the side of safety so the host harness doesn't trip ASan). */
void cs_log_append_failure(const char *fixed_msg);

/* ─── test hooks ──────────────────────────────────────────────────── */

/* Function-pointer indirections for the two zdd.c pure-logic calls the
 * dispatcher makes: zdd_create_screen and zdd_object_new.  When set,
 * the dispatcher routes through the hook instead of calling the real
 * impl — lets the host tests record (mode, args) without configuring
 * the full surface-create stub chain, and lets them inject failures
 * for specific modes.  Default NULL in production; never set outside
 * tests. */
typedef int (*cs_create_screen_fn)(zdd *self,
                                   uint32_t width, uint32_t height,
                                   uint32_t bpp, int mode_arg,
                                   int videomem_flag,
                                   const int32_t *opt_rect7);
extern cs_create_screen_fn cs_create_screen_hook;

typedef int (*cs_object_new_fn)(zdd *parent, zdd_object **out,
                                uint32_t width, uint32_t height,
                                int32_t colorkey, int32_t count);
extern cs_object_new_fn cs_object_new_hook;

/* ─── Win32 primitives — defined per build target ─────────────────── */

/* FUN_00560900 — pull GetLastError, format via FormatMessageA + sprintf,
 * emit through the engine logger.  Real build calls the actual Win32
 * functions; host build records the call (for tests verifying that the
 * SetDisplayMode/GetDisplayMode/CreateScreen Windowed fail paths route
 * through here). */
void cs_log_get_last_error(void);

/* FUN_00406440 — engine fatal logger.  Real build calls
 * OutputDebugStringA(msg/header/"\n") + file-write.  Host build
 * records the (msg, header) pair. */
void cs_fatal_log(const char *fixed_msg, const char *header);

/* FUN_00426110 with the `with_lasterror == 1` arg — the DB-mode
 * variant.  Pulls GetLastError + formats it through the engine logger
 * BEFORE the OutputDebugStrings for msg/header.  Real build: full
 * Win32 chain.  Host: records the call as a distinct variant. */
void cs_fatal_log_with_lasterror(const char *fixed_msg, const char *header);

/* FUN_005bf5db(0) — ExitProcess(0).  Real build: literally
 * ExitProcess(0).  Host build: longjmp out of the dispatch frame so
 * tests can assert which fail path was taken without ASan blowing up
 * on the missing return. */
void cs_exit(int code);

#endif /* OPENSUMMONERS_CS_DISPATCH_H */
