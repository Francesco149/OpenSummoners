/*
 * src/cs_dispatch.c — pure-logic port of FUN_00582e90.
 *
 * The retail body is 3560 bytes of mostly-inlined strcpy/strcat that
 * the compiler unrolled across each failure path.  We collapse those
 * back into a single `cs_log_append_failure` helper + per-mode fail
 * stubs that exit via `cs_exit`.
 *
 * No Win32 here — `cs_log_get_last_error`, `cs_fatal_log`,
 * `cs_fatal_log_with_lasterror`, and `cs_exit` are primitives backed
 * by cs_dispatch_win32.c at runtime and by tests/test_cs_dispatch.c
 * at host-test time.
 */
#include "cs_dispatch.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static size_t cs_bounded_strlen(const char *s, size_t cap)
{
    size_t i = 0;
    while (i < cap && s[i] != '\0') i++;
    return i;
}

#include "zdd.h"

/* ─── module globals ──────────────────────────────────────────────── */

zdd_object *cs_primary_pair = NULL;
int32_t     cs_zoom_override_width  = 0;
int32_t     cs_zoom_override_height = 0;
int32_t     cs_log_dirty = 0;
char        cs_log_buf[0x638];

/* Retail reads &DAT_008a9b6c as a `char *`.  At boot it's an empty
 * string; a later (unported) settings path stamps the engine name.
 * We mirror with a `const char *` so tests can re-point this at a
 * fixed literal between dispatch calls. */
const char *cs_engine_name_header = "";

/* Test hooks — see cs_dispatch.h.  NULL means "use the real impl". */
cs_create_screen_fn cs_create_screen_hook = NULL;
cs_object_new_fn    cs_object_new_hook    = NULL;

static int dispatch_create_screen(zdd *self,
                                  uint32_t w, uint32_t h, uint32_t bpp,
                                  int mode_arg, int videomem_flag,
                                  const int32_t *opt_rect7)
{
    if (cs_create_screen_hook != NULL) {
        return cs_create_screen_hook(self, w, h, bpp, mode_arg,
                                     videomem_flag, opt_rect7);
    }
    return zdd_create_screen(self, w, h, bpp, mode_arg,
                             videomem_flag, opt_rect7);
}

static int dispatch_object_new(zdd *parent, zdd_object **out,
                               uint32_t w, uint32_t h,
                               int32_t colorkey, int32_t count)
{
    if (cs_object_new_hook != NULL) {
        return cs_object_new_hook(parent, out, w, h, colorkey, count);
    }
    return zdd_object_new(parent, out, w, h, colorkey, count);
}

/* ─── per-mode error strings (verified against vendor/unpacked/sotes) ─
 *
 * These are the exact bytes at the DAT_xxx addresses listed in
 * docs/findings/ddraw-init.md.  Re-verified via
 *   r2 ps @ <addr> vendor/unpacked/sotes.unpacked.exe
 * at port time — see PROGRESS.md.  Mode 3 (DB) + mode 4 (Zoom)
 * CreateScreen-failure path share the "DB Mode" string in retail. */
static const char STR_FAIL_FULL[]    = "It failed in CreateScreen. (Full Screen Mode)";
static const char STR_FAIL_SAFE[]    = "It failed in CreateScreen. (Safety Mode)";
static const char STR_FAIL_WIND[]    = "It failed in CreateScreen. (Window Mode)";
static const char STR_FAIL_DB[]      = "It failed in CreateScreen. (DB Mode)";
static const char STR_FAIL_MODESET[] = "It failed in the display mode setting. ";
static const char STR_FAIL_MODEGET[] = "It failed in the display mode acquisition. ";
static const char STR_FAIL_ZOOM[]    =
    "Zoom Mode is only playable in resolutions higher than 1280x960. "
    "Please increase your resolution or use 'Fullscreen Mode' instead.";

/* ─── pure-logic helpers ──────────────────────────────────────────── */

void cs_compute_zoom_rect(int32_t display_w, int32_t display_h,
                          int32_t zoom_w,    int32_t zoom_h,
                          int32_t out_rect7[7])
{
    /* Retail uses `(x < 0) - 1 & x` to clamp negatives to zero — a
     * branchless max(0, x).  We use the plain comparison; same result. */
    int32_t centre_x = display_w / 2 - zoom_w / 2;
    int32_t centre_y = display_h / 2 - zoom_h / 2;
    if (centre_x < 0) centre_x = 0;
    if (centre_y < 0) centre_y = 0;

    int32_t src_w = (zoom_w <= display_w) ? zoom_w : display_w;
    int32_t src_h = (zoom_h <= display_h) ? zoom_h : display_h;

    out_rect7[0] = display_w;
    out_rect7[1] = display_h;
    out_rect7[2] = 2;
    out_rect7[3] = centre_x;
    out_rect7[4] = centre_y;
    out_rect7[5] = src_w;
    out_rect7[6] = src_h;
}

void cs_log_append_failure(const char *fixed_msg)
{
    cs_log_dirty = 1;

    if (fixed_msg == NULL) fixed_msg = "";
    const char *header = (cs_engine_name_header != NULL) ? cs_engine_name_header : "";

    size_t cap  = sizeof(cs_log_buf);
    size_t used = cs_bounded_strlen(cs_log_buf, cap);
    size_t left = (used < cap) ? (cap - used - 1) : 0;

    /* If the buffer already holds an entry, slip a "\n" separator in
     * before the new fixed-message body — retail's inlined separator
     * sits at DAT_00854570 and r2 confirms its bytes are "\n". */
    if (used != 0 && left != 0) {
        cs_log_buf[used++] = '\n';
        cs_log_buf[used]   = '\0';
        left--;
    }

    size_t msg_len = strlen(fixed_msg);
    if (msg_len > left) msg_len = left;
    memcpy(cs_log_buf + used, fixed_msg, msg_len);
    used += msg_len;
    cs_log_buf[used] = '\0';
    left -= msg_len;

    size_t hdr_len = strlen(header);
    if (hdr_len > left) hdr_len = left;
    memcpy(cs_log_buf + used, header, hdr_len);
    used += hdr_len;
    cs_log_buf[used] = '\0';
}

/* ─── per-mode fail stubs ─────────────────────────────────────────── */

/* The "standard" fail-and-exit path used by modes 0/1/2 and the
 * SetDisplayMode/GetDisplayMode/zoom-bounds failures: append to log
 * buffer, call FUN_00406440 with the bare fixed_msg + header, then
 * ExitProcess(0).  Does not return. */
static void cs_fail(const char *fixed_msg)
{
    cs_log_append_failure(fixed_msg);
    cs_fatal_log(fixed_msg, cs_engine_name_header);
    cs_exit(0);
}

/* The mode-0 SetDisplayMode/CreateScreen and mode-2 fail paths
 * additionally pull GetLastError + log it via FUN_00560900 BEFORE the
 * log-append + fatal sequence.  Retail's branch tree gates these on
 * `iVar4 == 0` post-FUN_005b8900/_b8480, and the GetLastError-fetch
 * leg goes through FUN_00560900 specifically. */
static void cs_fail_with_lasterror_prefix(const char *fixed_msg)
{
    cs_log_get_last_error();
    cs_fail(fixed_msg);
}

/* Mode 3's CreateScreen-failure path is the outlier — it skips the
 * log-buf strcat dance entirely and calls FUN_00426110 directly with
 * the fixed_msg + header (and the with_lasterror flag).  No append to
 * cs_log_buf, no cs_log_dirty bump.  Mode 3's SetDisplayMode failure
 * still uses the standard cs_fail path; only the CreateScreen branch
 * gets this variant. */
static void cs_fail_db_createscreen(void)
{
    cs_fatal_log_with_lasterror(STR_FAIL_DB, cs_engine_name_header);
    cs_exit(0);
}

/* ─── prologue: release prior primary-pair ────────────────────────── */

/* Retail's prologue runs unconditionally before the switch.  If a prior
 * mode-0 dispatch left a primary-pair behind, drop it here so the next
 * pass starts from a clean slot. */
static void cs_release_prior_primary_pair(void)
{
    if (cs_primary_pair == NULL) return;
    /* Retail issues FUN_005b9390 (the bare dtor) + FUN_005bef0e (heap
     * free) inline; zdd_obj_destroy bundles both + zeros the pointer. */
    zdd_obj_destroy(&cs_primary_pair);
}

/* ─── per-mode bodies ─────────────────────────────────────────────── */

/* Mode 0 — Full Screen.  Sequence per ddraw-init.md:
 *   SetDisplayMode(640, 480, 16, 0)
 *   hide_cursor + busy_wait_ms(2000)
 *   zdd_create_screen(640, 480, 16, mode_arg=0, videomem=0, NULL)
 *   zdd_object_new(&cs_primary_pair, 640, 480, 0x1ffffff, 1)
 *
 * Failure leaves the engine in an unrecoverable state — every fail
 * path exits via cs_exit. */
static void cs_mode_full(zdd *self)
{
    if (!zdd_set_display_mode(self, 0x280, 0x1e0, 0x10, 0)) {
        /* Retail's mode-0 SetDisplayMode-fail path inlines
         * GetLastError + FormatMessageA + FUN_00406440(line, header)
         * before the log-buf builder and the final FUN_00406440(STR,
         * header) + ExitProcess.  Both calls emit the same observable
         * output (OutputDebugStringA pair); we collapse via the
         * standard `cs_fail_with_lasterror_prefix` helper. */
        cs_fail_with_lasterror_prefix(STR_FAIL_MODESET);
        return;  /* unreachable */
    }

    zdd_hide_cursor(self);
    zdd_busy_wait_ms(2000);

    if (!dispatch_create_screen(self, 0x280, 0x1e0, 0x10, /*mode*/0,
                           /*videomem*/0, /*rect*/NULL)) {
        /* Retail's CreateScreen-fail path calls FUN_00560900 (the
         * GetLastError-logger object method) first, then the log
         * builder + FUN_00406440 — same observable effect as the
         * inline GetLastError-format used in the SetDisplayMode-fail
         * branch above. */
        cs_fail_with_lasterror_prefix(STR_FAIL_FULL);
        return;
    }

    /* Mode 0 is the only branch that allocates a public primary-pair
     * after CreateScreen — the other modes leave cs_primary_pair NULL.
     * Failure to allocate the pair is silently ignored at retail (the
     * return value of FUN_005b8b40 is not consumed). */
    (void)dispatch_object_new(self, &cs_primary_pair,
                              0x280, 0x1e0,
                              /*colorkey*/0x01ffffff, /*count*/1);
}

/* Mode 1 — Safe Mode.  Same fullscreen preamble as mode 0, but no
 * primary-pair allocation afterwards. */
static void cs_mode_safe(zdd *self)
{
    if (!zdd_set_display_mode(self, 0x280, 0x1e0, 0x10, 0)) {
        cs_fail_with_lasterror_prefix(STR_FAIL_MODESET);
        return;
    }

    zdd_hide_cursor(self);
    zdd_busy_wait_ms(2000);

    if (!dispatch_create_screen(self, 0x280, 0x1e0, 0x10, /*mode*/1,
                           /*videomem*/1, /*rect*/NULL)) {
        cs_fail_with_lasterror_prefix(STR_FAIL_SAFE);
    }
}

/* Mode 2 — Windowed.  No SetDisplayMode (stays in the desktop's
 * current mode), no hide_cursor, no busy_wait_ms.  Just CreateScreen
 * with the Windowed mode arg. */
static void cs_mode_windowed(zdd *self)
{
    if (!dispatch_create_screen(self, 0x280, 0x1e0, 0x10, /*mode*/2,
                           /*videomem*/1, /*rect*/NULL)) {
        cs_fail_with_lasterror_prefix(STR_FAIL_WIND);
    }
}

/* Mode 3 — DB Mode.  Fullscreen preamble like modes 0/1, but the
 * CreateScreen failure uses the FUN_00426110 variant instead of the
 * standard log-builder. */
static void cs_mode_db(zdd *self)
{
    if (!zdd_set_display_mode(self, 0x280, 0x1e0, 0x10, 0)) {
        cs_fail_with_lasterror_prefix(STR_FAIL_MODESET);
        return;
    }

    zdd_hide_cursor(self);
    zdd_busy_wait_ms(2000);

    if (!dispatch_create_screen(self, 0x280, 0x1e0, 0x10, /*mode*/3,
                           /*videomem*/1, /*rect*/NULL)) {
        cs_fail_db_createscreen();
    }
}

/* Mode 4 — Zoom Mode.  Resolves desktop dimensions (either via the
 * override globals or via GetDisplayMode), validates them against the
 * launcher's minimum playable size, runs SetDisplayMode + the
 * fullscreen preamble, computes the centre-rect blob, then calls
 * zdd_create_screen with the Zoom mode arg + the rect.  Plus the final
 * hide_cursor at retail's end-of-mode-4 (no busy_wait this time). */
static void cs_mode_zoom(zdd *self, int32_t zoom_target_w, int32_t zoom_target_h)
{
    int32_t rect7[7] = {0, 0, 0, 0, 0, 0, 0};

    /* Display-dimension acquisition.  If the override globals are both
     * positive, use them directly; otherwise call GetDisplayMode. */
    if (cs_zoom_override_width >= 1 && cs_zoom_override_height >= 1) {
        rect7[0] = cs_zoom_override_width;
        rect7[1] = cs_zoom_override_height;
    } else {
        uint32_t w = 0, h = 0;
        if (!zdd_get_display_mode(self, &w, &h, NULL)) {
            cs_fail_with_lasterror_prefix(STR_FAIL_MODEGET);
            return;
        }
        rect7[0] = (int32_t)w;
        rect7[1] = (int32_t)h;
    }

    /* Bounds check: zoom is only playable if the actual display can
     * cover the launcher's minimum required size. */
    if (rect7[0] < zoom_target_w || rect7[1] < zoom_target_h) {
        cs_fail_with_lasterror_prefix(STR_FAIL_ZOOM);
        return;
    }

    if (!zdd_set_display_mode(self, (uint32_t)rect7[0], (uint32_t)rect7[1],
                              0x10, 0)) {
        cs_fail_with_lasterror_prefix(STR_FAIL_MODESET);
        return;
    }

    zdd_hide_cursor(self);
    zdd_busy_wait_ms(2000);

    cs_compute_zoom_rect(rect7[0], rect7[1], zoom_target_w, zoom_target_h, rect7);

    if (!dispatch_create_screen(self, 0x280, 0x1e0, 0x10, /*mode*/4,
                           /*videomem*/1, rect7)) {
        cs_fail_with_lasterror_prefix(STR_FAIL_DB);
        return;
    }

    /* Retail's mode-4 success path calls FUN_005b8dd0 one more time at
     * the very end.  zdd_hide_cursor is idempotent when cursor_state
     * is already 0, so this is a no-op in practice; we mirror it for
     * provenance.  busy_wait_ms is NOT repeated. */
    zdd_hide_cursor(self);
}

/* ─── public driver ───────────────────────────────────────────────── */

void cs_dispatch_create_screen(zdd *self, int launcher_mode,
                               int32_t zoom_target_w, int32_t zoom_target_h)
{
    cs_release_prior_primary_pair();

    switch (launcher_mode) {
    case 0: cs_mode_full(self);                                    break;
    case 1: cs_mode_safe(self);                                    break;
    case 2: cs_mode_windowed(self);                                break;
    case 3: cs_mode_db(self);                                      break;
    case 4: cs_mode_zoom(self, zoom_target_w, zoom_target_h);      break;
    default: /* Retail falls through the switch with no default —
              * the function simply returns when launcher_mode is
              * outside 0..4.  Mirror exactly. */                  break;
    }
}
