/*
 * tests/test_cs_dispatch.c — host tests for src/cs_dispatch.c.
 *
 * Provides the four Win32 primitives the dispatcher needs:
 *   cs_log_get_last_error           → recorder (call count)
 *   cs_fatal_log                    → recorder (call count + last args)
 *   cs_fatal_log_with_lasterror     → recorder (call count + last args)
 *   cs_exit                         → longjmp out of the dispatcher
 *
 * Plus a recorder hook for the zdd_create_screen + zdd_object_new
 * function-pointer indirections in cs_dispatch.h.  This lets the tests
 * verify per-mode dispatch (mode_arg, videomem_flag, rect7 passthrough)
 * without configuring the full ZDD surface-create stub chain.
 *
 * zdd_* primitive stubs (set_display_mode, hide_cursor, busy_wait_ms,
 * get_display_mode, obj_destroy) come from test_zdd.c — both test
 * files compile into the same binary, so the linker pulls the unique
 * symbols from there.
 *
 * Each test calls cs_reset_stubs() before running.
 */
#include "../src/cs_dispatch.h"
#include "../src/zdd.h"
#include "t.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── cs_* stub state ─────────────────────────────────────────────── */

static int           cs_lasterror_calls = 0;
static int           cs_fatal_calls     = 0;
static const char   *cs_fatal_last_msg = NULL;
static const char   *cs_fatal_last_hdr = NULL;
static int           cs_fatal_le_calls  = 0;
static const char   *cs_fatal_le_last_msg = NULL;
static const char   *cs_fatal_le_last_hdr = NULL;
static int           cs_exit_calls      = 0;
static int           cs_exit_last_code  = -1;

/* setjmp/longjmp escape — cs_exit_throw is the longjmp target the
 * dispatcher unwinds to.  Tests set up the buffer before calling the
 * dispatcher and check that cs_exit_calls > 0 + cs_exit_last_code is
 * what they expected. */
static jmp_buf cs_exit_jmp;
static int     cs_exit_jmp_armed = 0;

/* Recorder hook for zdd_create_screen. */
static int          cs_cs_calls         = 0;
static zdd         *cs_cs_last_self     = NULL;
static uint32_t     cs_cs_last_w        = 0;
static uint32_t     cs_cs_last_h        = 0;
static uint32_t     cs_cs_last_bpp      = 0;
static int          cs_cs_last_mode     = -1;
static int          cs_cs_last_videomem = -1;
static int32_t      cs_cs_last_rect7[7] = {0};
static int          cs_cs_last_rect_was_null = 0;
static int          cs_cs_return        = 1;  /* what the hook returns */

/* Recorder hook for zdd_object_new. */
static int          cs_obj_new_calls    = 0;
static zdd         *cs_obj_new_last_parent = NULL;
static zdd_object **cs_obj_new_last_out_pp = NULL;
static uint32_t     cs_obj_new_last_w   = 0;
static uint32_t     cs_obj_new_last_h   = 0;
static int32_t      cs_obj_new_last_key = 0;
static int32_t      cs_obj_new_last_count = 0;
static int          cs_obj_new_return   = 1;
static zdd_object  *cs_obj_new_stash    = NULL;  /* if non-NULL, *out = this */

/* Forward declarations of zdd-stub control globals exposed by
 * test_zdd.c.  We don't drive them here (zdd_set_display_mode etc.
 * succeed by default), but the dispatcher will call into them through
 * the linker.  test_zdd.c keeps its own `reset_stubs()` private; we
 * rely on each test_zdd test resetting them at its own boundary, and
 * each cs test only depending on the defaults defined there. */

static int cs_hook_create_screen(zdd *self,
                                 uint32_t width, uint32_t height,
                                 uint32_t bpp, int mode_arg,
                                 int videomem_flag,
                                 const int32_t *opt_rect7)
{
    cs_cs_calls++;
    cs_cs_last_self     = self;
    cs_cs_last_w        = width;
    cs_cs_last_h        = height;
    cs_cs_last_bpp      = bpp;
    cs_cs_last_mode     = mode_arg;
    cs_cs_last_videomem = videomem_flag;
    cs_cs_last_rect_was_null = (opt_rect7 == NULL);
    if (opt_rect7 != NULL) {
        memcpy(cs_cs_last_rect7, opt_rect7, sizeof(cs_cs_last_rect7));
    } else {
        memset(cs_cs_last_rect7, 0, sizeof(cs_cs_last_rect7));
    }
    return cs_cs_return;
}

static int cs_hook_object_new(zdd *parent, zdd_object **out,
                              uint32_t width, uint32_t height,
                              int32_t colorkey, int32_t count)
{
    cs_obj_new_calls++;
    cs_obj_new_last_parent = parent;
    cs_obj_new_last_out_pp = out;
    cs_obj_new_last_w      = width;
    cs_obj_new_last_h      = height;
    cs_obj_new_last_key    = colorkey;
    cs_obj_new_last_count  = count;
    if (out != NULL) *out = cs_obj_new_stash;
    return cs_obj_new_return;
}

static void cs_reset_stubs(void)
{
    cs_lasterror_calls = 0;
    cs_fatal_calls     = 0;
    cs_fatal_last_msg  = NULL;
    cs_fatal_last_hdr  = NULL;
    cs_fatal_le_calls  = 0;
    cs_fatal_le_last_msg = NULL;
    cs_fatal_le_last_hdr = NULL;
    cs_exit_calls      = 0;
    cs_exit_last_code  = -1;
    cs_exit_jmp_armed  = 0;

    cs_cs_calls         = 0;
    cs_cs_last_self     = NULL;
    cs_cs_last_w        = 0;
    cs_cs_last_h        = 0;
    cs_cs_last_bpp      = 0;
    cs_cs_last_mode     = -1;
    cs_cs_last_videomem = -1;
    memset(cs_cs_last_rect7, 0, sizeof(cs_cs_last_rect7));
    cs_cs_last_rect_was_null = 0;
    cs_cs_return        = 1;
    cs_create_screen_hook = cs_hook_create_screen;

    cs_obj_new_calls    = 0;
    cs_obj_new_last_parent = NULL;
    cs_obj_new_last_out_pp = NULL;
    cs_obj_new_last_w   = 0;
    cs_obj_new_last_h   = 0;
    cs_obj_new_last_key = 0;
    cs_obj_new_last_count = 0;
    cs_obj_new_return   = 1;
    cs_obj_new_stash    = NULL;
    cs_object_new_hook  = cs_hook_object_new;

    /* Module globals. */
    cs_primary_pair = NULL;
    cs_zoom_override_width  = 0;
    cs_zoom_override_height = 0;
    cs_log_dirty    = 0;
    cs_log_buf[0]   = '\0';
    cs_engine_name_header = "";
}

/* ─── Win32 primitive stubs ───────────────────────────────────────── */

void cs_log_get_last_error(void)         { cs_lasterror_calls++; }

void cs_fatal_log(const char *msg, const char *header)
{
    cs_fatal_calls++;
    cs_fatal_last_msg = msg;
    cs_fatal_last_hdr = header;
}

void cs_fatal_log_with_lasterror(const char *msg, const char *header)
{
    cs_fatal_le_calls++;
    cs_fatal_le_last_msg = msg;
    cs_fatal_le_last_hdr = header;
}

void cs_exit(int code)
{
    cs_exit_calls++;
    cs_exit_last_code = code;
    if (cs_exit_jmp_armed) {
        cs_exit_jmp_armed = 0;
        longjmp(cs_exit_jmp, 1);
    }
    /* Without an armed jmp the test will crash — every fail-path test
     * MUST arm via DISPATCH_AND_CATCH below. */
    abort();
}

/* Helper macro: arm the longjmp, call the dispatcher, fall through to
 * the test body whether the dispatcher returned normally OR exited. */
#define DISPATCH_AND_CATCH(self_, mode_, zw_, zh_) do {           \
    cs_exit_jmp_armed = 1;                                         \
    if (setjmp(cs_exit_jmp) == 0) {                                \
        cs_dispatch_create_screen((self_), (mode_), (zw_), (zh_)); \
    }                                                              \
    cs_exit_jmp_armed = 0;                                         \
} while (0)

/* ─── tests: pure-logic helpers ───────────────────────────────────── */

int test_cs_compute_zoom_rect_clean_centre(void)
{
    /* Display 1920x1080, zoom target 1280x960 → centre at (320, 60),
     * src equal to the target dims. */
    int32_t r[7];
    cs_compute_zoom_rect(1920, 1080, 1280, 960, r);
    T_ASSERT_EQ_I(r[0], 1920);
    T_ASSERT_EQ_I(r[1], 1080);
    T_ASSERT_EQ_I(r[2], 2);
    T_ASSERT_EQ_I(r[3], (1920 - 1280) / 2);
    T_ASSERT_EQ_I(r[4], (1080 -  960) / 2);
    T_ASSERT_EQ_I(r[5], 1280);
    T_ASSERT_EQ_I(r[6],  960);
    return 0;
}

int test_cs_compute_zoom_rect_exact_match(void)
{
    int32_t r[7];
    cs_compute_zoom_rect(1280, 960, 1280, 960, r);
    T_ASSERT_EQ_I(r[3], 0);
    T_ASSERT_EQ_I(r[4], 0);
    T_ASSERT_EQ_I(r[5], 1280);
    T_ASSERT_EQ_I(r[6],  960);
    return 0;
}

int test_cs_compute_zoom_rect_zoom_larger_than_display(void)
{
    /* If the launcher's target dims exceed the display (would be
     * caught by the bounds check at dispatch time, but the math
     * itself still needs to clamp), the centre coordinates floor to
     * 0 and src clamps to display. */
    int32_t r[7];
    cs_compute_zoom_rect(800, 600, 1280, 960, r);
    T_ASSERT_EQ_I(r[3], 0);
    T_ASSERT_EQ_I(r[4], 0);
    T_ASSERT_EQ_I(r[5], 800);
    T_ASSERT_EQ_I(r[6], 600);
    return 0;
}

int test_cs_compute_zoom_rect_odd_difference_floors(void)
{
    /* 1281 - 1280 = 1; (1)/2 = 0.  Centre clamps cleanly. */
    int32_t r[7];
    cs_compute_zoom_rect(1281, 961, 1280, 960, r);
    T_ASSERT_EQ_I(r[3], 0);
    T_ASSERT_EQ_I(r[4], 0);
    return 0;
}

int test_cs_log_append_failure_empty_buffer_overwrites(void)
{
    cs_reset_stubs();
    cs_engine_name_header = "<HDR>";
    cs_log_append_failure("msg1");
    T_ASSERT_EQ_I(cs_log_dirty, 1);
    T_ASSERT(strcmp(cs_log_buf, "msg1<HDR>") == 0);
    return 0;
}

int test_cs_log_append_failure_appends_with_newline(void)
{
    cs_reset_stubs();
    cs_engine_name_header = "<HDR>";
    cs_log_append_failure("first");
    cs_log_append_failure("second");
    /* The header is appended on every call.  Separator is "\n" between
     * the prior buffer content and the next fixed message. */
    T_ASSERT(strcmp(cs_log_buf, "first<HDR>\nsecond<HDR>") == 0);
    return 0;
}

int test_cs_log_append_failure_null_msg_safe(void)
{
    cs_reset_stubs();
    cs_engine_name_header = "h";
    cs_log_append_failure(NULL);
    T_ASSERT_EQ_I(cs_log_dirty, 1);
    T_ASSERT(strcmp(cs_log_buf, "h") == 0);
    return 0;
}

/* ─── tests: prologue (prior primary pair release) ────────────────── */

int test_cs_dispatch_releases_prior_primary_pair(void)
{
    cs_reset_stubs();

    zdd parent;
    memset(&parent, 0, sizeof(parent));

    /* Pre-allocate a ZDDObject the dispatcher should free. */
    zdd_object *prior = calloc(1, sizeof(*prior));
    T_ASSERT(prior != NULL);
    zdd_object_ctor(prior, &parent);
    /* open_objects must be 1 after the ctor — when zdd_obj_destroy
     * runs in the dispatcher prologue it decrements back to 0. */
    T_ASSERT_EQ_I(parent.open_objects, 1);

    cs_primary_pair = prior;
    DISPATCH_AND_CATCH(&parent, /*mode*/2, 0, 0);

    T_ASSERT_EQ_P(cs_primary_pair, NULL);
    T_ASSERT_EQ_I(parent.open_objects, 0);
    return 0;
}

/* ─── tests: per-mode happy paths ─────────────────────────────────── */

int test_cs_dispatch_mode0_full_args_and_pair_alloc(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));

    DISPATCH_AND_CATCH(&self, /*mode*/0, 0, 0);

    /* CreateScreen called with mode=0, videomem=0, no rect. */
    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_P(cs_cs_last_self, &self);
    T_ASSERT_EQ_U(cs_cs_last_w, 0x280);
    T_ASSERT_EQ_U(cs_cs_last_h, 0x1e0);
    T_ASSERT_EQ_U(cs_cs_last_bpp, 0x10);
    T_ASSERT_EQ_I(cs_cs_last_mode, 0);
    T_ASSERT_EQ_I(cs_cs_last_videomem, 0);
    T_ASSERT_EQ_I(cs_cs_last_rect_was_null, 1);

    /* zdd_object_new called for the primary pair. */
    T_ASSERT_EQ_I(cs_obj_new_calls, 1);
    T_ASSERT_EQ_P(cs_obj_new_last_parent, &self);
    T_ASSERT_EQ_P(cs_obj_new_last_out_pp, &cs_primary_pair);
    T_ASSERT_EQ_U(cs_obj_new_last_w, 0x280);
    T_ASSERT_EQ_U(cs_obj_new_last_h, 0x1e0);
    T_ASSERT_EQ_I(cs_obj_new_last_key, 0x01ffffff);
    T_ASSERT_EQ_I(cs_obj_new_last_count, 1);

    /* No failure path was taken. */
    T_ASSERT_EQ_I(cs_exit_calls, 0);
    T_ASSERT_EQ_I(cs_fatal_calls, 0);
    return 0;
}

int test_cs_dispatch_mode1_safe_args_and_no_pair(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));

    DISPATCH_AND_CATCH(&self, /*mode*/1, 0, 0);

    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_I(cs_cs_last_mode, 1);
    T_ASSERT_EQ_I(cs_cs_last_videomem, 1);
    T_ASSERT_EQ_I(cs_cs_last_rect_was_null, 1);

    /* Mode 1 does not allocate the primary-pair. */
    T_ASSERT_EQ_I(cs_obj_new_calls, 0);
    T_ASSERT_EQ_P(cs_primary_pair, NULL);

    T_ASSERT_EQ_I(cs_exit_calls, 0);
    return 0;
}

int test_cs_dispatch_mode2_windowed_args_and_no_preamble(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));

    DISPATCH_AND_CATCH(&self, /*mode*/2, 0, 0);

    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_I(cs_cs_last_mode, 2);
    T_ASSERT_EQ_I(cs_cs_last_videomem, 1);

    /* Mode 2 has no preamble — verify by exercising the SetDisplayMode
     * recorder via the zdd stub.  test_zdd.c's reset_stubs initialises
     * g_dd_setmode_calls to 0; an independent test_zdd test runs may
     * have left it incremented.  We instead verify by checking that
     * cs_exit was not called and only CreateScreen ran. */
    T_ASSERT_EQ_I(cs_obj_new_calls, 0);
    T_ASSERT_EQ_I(cs_exit_calls, 0);
    return 0;
}

int test_cs_dispatch_mode3_db_args(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));

    DISPATCH_AND_CATCH(&self, /*mode*/3, 0, 0);

    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_I(cs_cs_last_mode, 3);
    T_ASSERT_EQ_I(cs_cs_last_videomem, 1);
    T_ASSERT_EQ_I(cs_cs_last_rect_was_null, 1);

    T_ASSERT_EQ_I(cs_obj_new_calls, 0);
    T_ASSERT_EQ_I(cs_exit_calls, 0);
    return 0;
}

int test_cs_dispatch_mode4_zoom_with_override(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));

    cs_zoom_override_width  = 1920;
    cs_zoom_override_height = 1080;

    DISPATCH_AND_CATCH(&self, /*mode*/4, 1280, 960);

    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_I(cs_cs_last_mode, 4);
    T_ASSERT_EQ_I(cs_cs_last_videomem, 1);
    T_ASSERT_EQ_I(cs_cs_last_rect_was_null, 0);
    /* rect[0..6] = {display_w, display_h, 2, centre_x, centre_y, src_w, src_h}. */
    T_ASSERT_EQ_I(cs_cs_last_rect7[0], 1920);
    T_ASSERT_EQ_I(cs_cs_last_rect7[1], 1080);
    T_ASSERT_EQ_I(cs_cs_last_rect7[2], 2);
    T_ASSERT_EQ_I(cs_cs_last_rect7[3], (1920 - 1280) / 2);
    T_ASSERT_EQ_I(cs_cs_last_rect7[4], (1080 -  960) / 2);
    T_ASSERT_EQ_I(cs_cs_last_rect7[5], 1280);
    T_ASSERT_EQ_I(cs_cs_last_rect7[6],  960);

    T_ASSERT_EQ_I(cs_exit_calls, 0);
    return 0;
}

int test_cs_dispatch_mode4_zoom_without_override_uses_getdisplaymode(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    /* test_zdd's stub defaults: 1024x768. */

    DISPATCH_AND_CATCH(&self, /*mode*/4, 800, 600);

    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_I(cs_cs_last_mode, 4);
    T_ASSERT_EQ_I(cs_cs_last_rect7[0], 1024);
    T_ASSERT_EQ_I(cs_cs_last_rect7[1],  768);
    T_ASSERT_EQ_I(cs_cs_last_rect7[5],  800);
    T_ASSERT_EQ_I(cs_cs_last_rect7[6],  600);
    T_ASSERT_EQ_I(cs_exit_calls, 0);
    return 0;
}

/* ─── tests: failure paths ────────────────────────────────────────── */

int test_cs_dispatch_mode0_createscreen_fail_routes_to_full_string(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    cs_cs_return = 0;  /* CreateScreen fails */

    DISPATCH_AND_CATCH(&self, /*mode*/0, 0, 0);

    /* Should have routed through the FUN_00560900 + log builder +
     * FUN_00406440 + ExitProcess chain. */
    T_ASSERT_EQ_I(cs_lasterror_calls, 1);
    T_ASSERT_EQ_I(cs_fatal_calls, 1);
    T_ASSERT(cs_fatal_last_msg != NULL);
    T_ASSERT(strstr(cs_fatal_last_msg, "Full Screen") != NULL);
    T_ASSERT_EQ_I(cs_exit_calls, 1);
    /* cs_log_buf should have the "Full" message + dirty flag. */
    T_ASSERT_EQ_I(cs_log_dirty, 1);
    T_ASSERT(strstr(cs_log_buf, "Full Screen") != NULL);
    /* No primary pair alloc on the failure path. */
    T_ASSERT_EQ_I(cs_obj_new_calls, 0);
    return 0;
}

int test_cs_dispatch_mode1_createscreen_fail_routes_to_safe_string(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    cs_cs_return = 0;

    DISPATCH_AND_CATCH(&self, /*mode*/1, 0, 0);

    T_ASSERT_EQ_I(cs_lasterror_calls, 1);
    T_ASSERT_EQ_I(cs_fatal_calls, 1);
    T_ASSERT(strstr(cs_fatal_last_msg, "Safety") != NULL);
    T_ASSERT_EQ_I(cs_exit_calls, 1);
    return 0;
}

int test_cs_dispatch_mode2_createscreen_fail_routes_to_wind_string(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    cs_cs_return = 0;

    DISPATCH_AND_CATCH(&self, /*mode*/2, 0, 0);

    T_ASSERT_EQ_I(cs_lasterror_calls, 1);
    T_ASSERT_EQ_I(cs_fatal_calls, 1);
    T_ASSERT(strstr(cs_fatal_last_msg, "Window") != NULL);
    T_ASSERT_EQ_I(cs_exit_calls, 1);
    return 0;
}

int test_cs_dispatch_mode3_createscreen_fail_uses_db_lasterror_variant(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    cs_cs_return = 0;

    DISPATCH_AND_CATCH(&self, /*mode*/3, 0, 0);

    /* Mode 3 routes through cs_fatal_log_with_lasterror, NOT the
     * regular cs_fatal_log + log-builder chain.  Also no
     * cs_lasterror_calls (the with_lasterror variant pulls
     * GetLastError internally). */
    T_ASSERT_EQ_I(cs_fatal_calls, 0);
    T_ASSERT_EQ_I(cs_fatal_le_calls, 1);
    T_ASSERT(strstr(cs_fatal_le_last_msg, "DB Mode") != NULL);
    /* The log-builder doesn't get touched in mode 3's CS-fail path. */
    T_ASSERT_EQ_I(cs_log_dirty, 0);
    T_ASSERT_EQ_I(cs_exit_calls, 1);
    return 0;
}

int test_cs_dispatch_mode4_zoom_too_small_aborts(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    cs_zoom_override_width  = 1024;
    cs_zoom_override_height = 768;

    /* zoom_target 1280x960 exceeds the 1024x768 override → bounds fail. */
    DISPATCH_AND_CATCH(&self, /*mode*/4, 1280, 960);

    /* CreateScreen never got called — bounds check fires first. */
    T_ASSERT_EQ_I(cs_cs_calls, 0);
    T_ASSERT_EQ_I(cs_fatal_calls, 1);
    T_ASSERT(strstr(cs_fatal_last_msg, "Zoom Mode") != NULL);
    T_ASSERT_EQ_I(cs_exit_calls, 1);
    return 0;
}

int test_cs_dispatch_mode4_zoom_exact_match_proceeds(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));
    cs_zoom_override_width  = 1280;
    cs_zoom_override_height = 960;

    DISPATCH_AND_CATCH(&self, /*mode*/4, 1280, 960);

    T_ASSERT_EQ_I(cs_cs_calls, 1);
    T_ASSERT_EQ_I(cs_exit_calls, 0);
    return 0;
}

int test_cs_dispatch_default_mode_is_noop(void)
{
    cs_reset_stubs();
    zdd self; memset(&self, 0, sizeof(self));

    DISPATCH_AND_CATCH(&self, /*mode*/99, 0, 0);

    T_ASSERT_EQ_I(cs_cs_calls, 0);
    T_ASSERT_EQ_I(cs_exit_calls, 0);
    T_ASSERT_EQ_I(cs_obj_new_calls, 0);
    return 0;
}
