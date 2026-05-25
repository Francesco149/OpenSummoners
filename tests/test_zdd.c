/*
 * tests/test_zdd.c — host tests for src/zdd.c.
 *
 * Provides the six Win32 primitives the port needs, all
 * test-controllable:
 *
 *   zdd_show_cursor          → records last show-value
 *   zdd_output_debug_string  → appends to a capture buffer
 *   zdd_com_release          → decrements g_live_coms + zeros *pp
 *   zdd_object_local_free    → decrements g_live_pixel_bufs (NULL is
 *                              a no-op, matches Win32 LocalFree)
 *   zdd_directdraw_create_ex → returns g_dd_create_result; on success
 *                              stamps self->ddraw7 = g_dd_fake_iface
 *   zdd_set_coop_level       → returns g_dd_setcoop_result; on
 *                              failure calls zdd_log_dderr
 *
 * Note: zdd_obj_destroy is now pure logic in zdd.c (FUN_005b9390 +
 * heap free) — we don't stub it; tests use it directly.
 *
 * Each test resets the stub state via reset_stubs() before running.
 */
#include "../src/zdd.h"
#include "t.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── stub state ─────────────────────────────────────────────────── */

static int  g_cursor_last_show = -999;
static int  g_cursor_calls     = 0;

static char g_log_capture[4096];
static int  g_log_call_count   = 0;

static int  g_live_coms        = 0;
static int  g_live_pixel_bufs  = 0;

/* When zdd_directdraw_create_ex is called, return this value.  If
 * non-zero, set self->ddraw7 = g_dd_fake_iface beforehand.  If zero,
 * call zdd_log_dderr with the configured failure HRESULT. */
static int      g_dd_create_result = 1;
static void    *g_dd_fake_iface    = NULL;
static int32_t  g_dd_create_hr     = 0;

/* zdd_set_coop_level: success/failure + failure HRESULT. */
static int      g_dd_setcoop_result = 1;
static int32_t  g_dd_setcoop_hr     = 0;

/* zdd_create_surface stub state. */
static int      g_dd_createsurf_result = 1;
static int32_t  g_dd_createsurf_hr     = 0;
static void    *g_dd_createsurf_handle = NULL;

/* zdd_surface_set_color_key stub state — call recorder. */
static int      g_dd_setkey_result   = 1;
static int32_t  g_dd_setkey_hr       = 0;
static int      g_dd_setkey_calls    = 0;
static void    *g_dd_setkey_last_surf = NULL;
static int32_t  g_dd_setkey_last_key  = 0;

/* zdd_create_clipper / SetClipList-null / SetClipper stub state.
 * Sequence-id incremented on every call (any of the three) so tests
 * can assert call order. */
static int      g_dd_clipper_seq            = 0;
static int      g_dd_create_clipper_calls   = 0;
static int      g_dd_create_clipper_seq     = 0;
static zdd     *g_dd_create_clipper_parent  = NULL;
static void    *g_dd_create_clipper_handle  = NULL;  /* stub returns this */
static int      g_dd_setcliplist_calls      = 0;
static int      g_dd_setcliplist_seq        = 0;
static void    *g_dd_setcliplist_last       = NULL;
static uint32_t g_dd_setcliplist_last_w     = 0;
static uint32_t g_dd_setcliplist_last_h     = 0;
static int      g_dd_setclipper_calls       = 0;
static int      g_dd_setclipper_seq         = 0;
static void    *g_dd_setclipper_last_surf   = NULL;
static void    *g_dd_setclipper_last_clip   = NULL;

/* zdd_set_display_mode stub state — recorder + result. */
static int      g_dd_setmode_result    = 1;
static int32_t  g_dd_setmode_hr        = 0;
static int      g_dd_setmode_calls     = 0;
static uint32_t g_dd_setmode_last_w    = 0;
static uint32_t g_dd_setmode_last_h    = 0;
static uint32_t g_dd_setmode_last_bpp  = 0;
static uint32_t g_dd_setmode_last_refresh = 0;

/* zdd_get_display_mode stub state — returns these dimensions. */
static int      g_dd_getmode_result    = 1;
static uint32_t g_dd_getmode_width     = 1024;
static uint32_t g_dd_getmode_height    = 768;
static uint32_t g_dd_getmode_pitch     = 4096;
static int      g_dd_getmode_calls     = 0;

/* zdd_busy_wait_ms stub state — recorder; never actually sleeps. */
static int      g_busy_wait_calls      = 0;
static uint32_t g_busy_wait_last_ms    = 0;

/* zdd_create_primary_surface stub state. */
static int      g_dd_create_primary_result = 1;
static int32_t  g_dd_create_primary_hr     = 0;
static int      g_dd_create_primary_calls  = 0;
static void    *g_dd_create_primary_handle = NULL;
static uint32_t g_dd_create_primary_last_flags = 0;
static uint32_t g_dd_create_primary_last_caps  = 0;
static uint32_t g_dd_create_primary_last_count = 0;

/* zdd_create_system_palette / zdd_surface_set_palette stub state. */
static int      g_dd_create_pal_result   = 1;
static int32_t  g_dd_create_pal_hr       = 0;
static int      g_dd_create_pal_calls    = 0;
static void    *g_dd_create_pal_handle   = NULL;
static int      g_dd_setpal_result       = 1;
static int32_t  g_dd_setpal_hr           = 0;
static int      g_dd_setpal_calls        = 0;
static void    *g_dd_setpal_last_surf    = NULL;
static void    *g_dd_setpal_last_pal     = NULL;

/* zdd_get_attached_surface stub state — succeeds by default and
 * publishes a fake attached-surface handle.  Tests can flip
 * g_dd_attached_result to 0 to exercise the DDERR path. */
static int      g_dd_attached_result   = 1;
static int32_t  g_dd_attached_hr       = 0;
static int      g_dd_attached_calls    = 0;
static void    *g_dd_attached_handle   = NULL;
static void    *g_dd_attached_last_primary = NULL;
static uint32_t g_dd_attached_last_caps    = 0;

/* Present-dispatcher primitive stubs (back FUN_005b8fc0 / _94e0 /
 * _9500 / _9a40 host tests).  Each records call shape so the
 * dispatcher tests can assert which leg fired. */
static int      g_dd_getdc_calls       = 0;
static int      g_dd_getdc_result      = 1;
static void    *g_dd_getdc_last_surf   = NULL;
static void    *g_dd_getdc_handle      = NULL;  /* what *out_hdc receives */

static int      g_dd_releasedc_calls   = 0;
static void    *g_dd_releasedc_last_surf = NULL;
static void    *g_dd_releasedc_last_hdc  = NULL;

static int      g_dd_flip_calls        = 0;
static int      g_dd_flip_result       = 1;
static int32_t  g_dd_flip_hr           = 0;
static void    *g_dd_flip_last_surf    = NULL;
static void    *g_dd_flip_last_target  = NULL;
static uint32_t g_dd_flip_last_flags   = 0;

static int      g_dd_blt_calls         = 0;
static int      g_dd_blt_result        = 1;
static int32_t  g_dd_blt_hr            = 0;
static void    *g_dd_blt_last_dest     = NULL;
static void    *g_dd_blt_last_src      = NULL;
static int32_t  g_dd_blt_last_dest_rect[4];
static int32_t  g_dd_blt_last_src_rect[4];
static int      g_dd_blt_last_has_dest_rect = 0;
static int      g_dd_blt_last_has_src_rect  = 0;
static uint32_t g_dd_blt_last_flags    = 0;

static int      g_dd_desktop_calls     = 0;
static int      g_dd_desktop_result    = 1;
static void    *g_dd_desktop_last_src  = NULL;
static int      g_dd_desktop_last_x    = 0;
static int      g_dd_desktop_last_y    = 0;
static int      g_dd_desktop_last_w    = 0;
static int      g_dd_desktop_last_h    = 0;

/* WM_PAINT-handler primitive stubs (back FUN_005b9130 host tests).
 * Each records call shape so the dispatcher tests can assert the
 * begin → get_dc → blit → release_dc → end ordering exactly.  The
 * begin/end pair plumbs a sentinel cookie through; tests verify it
 * round-trips unchanged. */
static int      g_dd_wpaint_begin_calls    = 0;
static void    *g_dd_wpaint_begin_last_hwnd = NULL;
static void    *g_dd_wpaint_begin_hdc       = NULL;  /* what begin returns */
static void    *g_dd_wpaint_begin_cookie    = NULL;  /* what *out_cookie gets */
static int      g_dd_wpaint_end_calls       = 0;
static void    *g_dd_wpaint_end_last_hwnd   = NULL;
static void    *g_dd_wpaint_end_last_cookie = NULL;
static int      g_dd_wblit_calls            = 0;
static void    *g_dd_wblit_last_dest_hdc    = NULL;
static void    *g_dd_wblit_last_src_hdc     = NULL;
static int      g_dd_wblit_last_x           = 0;
static int      g_dd_wblit_last_y           = 0;
static int      g_dd_wblit_last_w           = 0;
static int      g_dd_wblit_last_h           = 0;
/* Sequence counter shared across the WM_PAINT primitives so tests can
 * assert the OS / COM interleave exactly: begin(1), get_dc(2), blit(3),
 * release_dc(4), end(5).  Reused with the getdc/releasedc stubs by
 * bumping them in those stubs too — see seq_* writes in reset_stubs. */
static int      g_dd_paint_seq             = 0;
static int      g_dd_wpaint_begin_seq      = 0;
static int      g_dd_getdc_seq             = 0;
static int      g_dd_wblit_seq             = 0;
static int      g_dd_releasedc_seq         = 0;
static int      g_dd_wpaint_end_seq        = 0;

/* Lock / Unlock / GetSurfaceDesc primitive stubs (back FUN_005b9490 /
 * _94d0 / _8a20 / _9410 host tests).  Lock can be programmed to
 * populate the caller's DDSD with canned R/G/B masks (used by
 * bind_pixel_format tests) and lpSurface/pitch/height (used by
 * zdd_object_clear tests). */
static int      g_dd_lock_calls       = 0;
static int      g_dd_lock_result      = 1;
static int32_t  g_dd_lock_hr          = 0;
static void    *g_dd_lock_last_surf   = NULL;
static void    *g_dd_lock_last_ddsd   = NULL;
static uint32_t g_dd_lock_last_flags  = 0;
/* Populated into the DDSD after Lock returns success: */
static void    *g_dd_lock_fill_surface = NULL;
static int32_t  g_dd_lock_fill_pitch   = 0;
static int32_t  g_dd_lock_fill_height  = 0;

static int      g_dd_unlock_calls     = 0;
static void    *g_dd_unlock_last_surf = NULL;
/* Shared seq counter so clear() tests can verify lock/unlock ordering. */
static int      g_dd_clear_seq        = 0;
static int      g_dd_lock_seq         = 0;
static int      g_dd_unlock_seq       = 0;

static int      g_dd_getdesc_calls    = 0;
static int      g_dd_getdesc_result   = 1;
static void    *g_dd_getdesc_last_surf = NULL;
static uint32_t g_dd_getdesc_fill_r_mask = 0xF800;
static uint32_t g_dd_getdesc_fill_g_mask = 0x07E0;
static uint32_t g_dd_getdesc_fill_b_mask = 0x001F;

static void reset_stubs(void)
{
    g_cursor_last_show = -999;
    g_cursor_calls     = 0;
    g_log_capture[0]   = '\0';
    g_log_call_count   = 0;
    g_live_coms        = 0;
    g_live_pixel_bufs  = 0;
    g_dd_create_result = 1;
    g_dd_fake_iface    = (void *)(uintptr_t)0x12345678;
    g_dd_create_hr     = 0;
    g_dd_setcoop_result = 1;
    g_dd_setcoop_hr    = 0;
    g_dd_createsurf_result = 1;
    g_dd_createsurf_hr     = 0;
    g_dd_createsurf_handle = (void *)(uintptr_t)0x99887766;
    g_dd_setkey_result     = 1;
    g_dd_setkey_hr         = 0;
    g_dd_setkey_calls      = 0;
    g_dd_setkey_last_surf  = NULL;
    g_dd_setkey_last_key   = 0;
    g_dd_clipper_seq            = 0;
    g_dd_create_clipper_calls   = 0;
    g_dd_create_clipper_seq     = 0;
    g_dd_create_clipper_parent  = NULL;
    g_dd_create_clipper_handle  = (void *)(uintptr_t)0xc11ccccc;
    g_dd_setcliplist_calls      = 0;
    g_dd_setcliplist_seq        = 0;
    g_dd_setcliplist_last       = NULL;
    g_dd_setcliplist_last_w     = 0;
    g_dd_setcliplist_last_h     = 0;
    g_dd_setclipper_calls       = 0;
    g_dd_setclipper_seq         = 0;
    g_dd_setclipper_last_surf   = NULL;
    g_dd_setclipper_last_clip   = NULL;
    g_dd_setmode_result    = 1;
    g_dd_setmode_hr        = 0;
    g_dd_setmode_calls     = 0;
    g_dd_setmode_last_w    = 0;
    g_dd_setmode_last_h    = 0;
    g_dd_setmode_last_bpp  = 0;
    g_dd_setmode_last_refresh = 0;
    g_dd_getmode_result    = 1;
    g_dd_getmode_width     = 1024;
    g_dd_getmode_height    = 768;
    g_dd_getmode_pitch     = 4096;
    g_dd_getmode_calls     = 0;
    g_busy_wait_calls      = 0;
    g_busy_wait_last_ms    = 0;
    g_dd_attached_result   = 1;
    g_dd_attached_hr       = 0;
    g_dd_attached_calls    = 0;
    g_dd_attached_handle   = (void *)(uintptr_t)0xbbbbbbbb;
    g_dd_attached_last_primary = NULL;
    g_dd_attached_last_caps    = 0;
    g_dd_create_primary_result = 1;
    g_dd_create_primary_hr     = 0;
    g_dd_create_primary_calls  = 0;
    g_dd_create_primary_handle = (void *)(uintptr_t)0xa1a1a1a1;
    g_dd_create_primary_last_flags = 0;
    g_dd_create_primary_last_caps  = 0;
    g_dd_create_primary_last_count = 0;
    g_dd_create_pal_result = 1;
    g_dd_create_pal_hr     = 0;
    g_dd_create_pal_calls  = 0;
    g_dd_create_pal_handle = (void *)(uintptr_t)0xba1ba1ba;
    g_dd_setpal_result     = 1;
    g_dd_setpal_hr         = 0;
    g_dd_setpal_calls      = 0;
    g_dd_setpal_last_surf  = NULL;
    g_dd_setpal_last_pal   = NULL;
    g_dd_getdc_calls       = 0;
    g_dd_getdc_result      = 1;
    g_dd_getdc_last_surf   = NULL;
    g_dd_getdc_handle      = (void *)(uintptr_t)0xdcdcdcdc;
    g_dd_releasedc_calls   = 0;
    g_dd_releasedc_last_surf = NULL;
    g_dd_releasedc_last_hdc  = NULL;
    g_dd_flip_calls        = 0;
    g_dd_flip_result       = 1;
    g_dd_flip_hr           = 0;
    g_dd_flip_last_surf    = NULL;
    g_dd_flip_last_target  = NULL;
    g_dd_flip_last_flags   = 0;
    g_dd_blt_calls         = 0;
    g_dd_blt_result        = 1;
    g_dd_blt_hr            = 0;
    g_dd_blt_last_dest     = NULL;
    g_dd_blt_last_src      = NULL;
    g_dd_blt_last_has_dest_rect = 0;
    g_dd_blt_last_has_src_rect  = 0;
    g_dd_blt_last_flags    = 0;
    memset(g_dd_blt_last_dest_rect, 0, sizeof(g_dd_blt_last_dest_rect));
    memset(g_dd_blt_last_src_rect,  0, sizeof(g_dd_blt_last_src_rect));
    g_dd_desktop_calls     = 0;
    g_dd_desktop_result    = 1;
    g_dd_desktop_last_src  = NULL;
    g_dd_desktop_last_x    = 0;
    g_dd_desktop_last_y    = 0;
    g_dd_desktop_last_w    = 0;
    g_dd_desktop_last_h    = 0;
    g_dd_wpaint_begin_calls    = 0;
    g_dd_wpaint_begin_last_hwnd = NULL;
    g_dd_wpaint_begin_hdc       = (void *)(uintptr_t)0xb1b1b1b1;
    g_dd_wpaint_begin_cookie    = (void *)(uintptr_t)0xc00c1eee;
    g_dd_wpaint_end_calls       = 0;
    g_dd_wpaint_end_last_hwnd   = NULL;
    g_dd_wpaint_end_last_cookie = NULL;
    g_dd_wblit_calls            = 0;
    g_dd_wblit_last_dest_hdc    = NULL;
    g_dd_wblit_last_src_hdc     = NULL;
    g_dd_wblit_last_x           = 0;
    g_dd_wblit_last_y           = 0;
    g_dd_wblit_last_w           = 0;
    g_dd_wblit_last_h           = 0;
    g_dd_paint_seq              = 0;
    g_dd_wpaint_begin_seq       = 0;
    g_dd_getdc_seq              = 0;
    g_dd_wblit_seq              = 0;
    g_dd_releasedc_seq          = 0;
    g_dd_wpaint_end_seq         = 0;
    g_dd_lock_calls         = 0;
    g_dd_lock_result        = 1;
    g_dd_lock_hr            = 0;
    g_dd_lock_last_surf     = NULL;
    g_dd_lock_last_ddsd     = NULL;
    g_dd_lock_last_flags    = 0;
    g_dd_lock_fill_surface  = NULL;
    g_dd_lock_fill_pitch    = 0;
    g_dd_lock_fill_height   = 0;
    g_dd_unlock_calls       = 0;
    g_dd_unlock_last_surf   = NULL;
    g_dd_clear_seq          = 0;
    g_dd_lock_seq           = 0;
    g_dd_unlock_seq         = 0;
    g_dd_getdesc_calls      = 0;
    g_dd_getdesc_result     = 1;
    g_dd_getdesc_last_surf  = NULL;
    g_dd_getdesc_fill_r_mask = 0xF800;
    g_dd_getdesc_fill_g_mask = 0x07E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
}

void zdd_show_cursor(int show)
{
    g_cursor_last_show = show;
    g_cursor_calls++;
}

void zdd_output_debug_string(const char *s)
{
    if (s == NULL) return;
    /* Append to capture buffer (truncate to fit). */
    size_t used = strlen(g_log_capture);
    size_t cap  = sizeof(g_log_capture);
    size_t n    = strlen(s);
    if (used + n + 1 > cap) n = cap - used - 1;
    memcpy(g_log_capture + used, s, n);
    g_log_capture[used + n] = '\0';
    g_log_call_count++;
}

void zdd_com_release(void **iunknown_pp)
{
    if (iunknown_pp == NULL || *iunknown_pp == NULL) return;
    g_live_coms--;
    *iunknown_pp = NULL;
}

void zdd_object_local_free(void *p)
{
    if (p == NULL) return;
    g_live_pixel_bufs--;
}

int zdd_directdraw_create_ex(zdd *self)
{
    if (g_dd_create_result) {
        self->ddraw7 = g_dd_fake_iface;
        g_live_coms++;
        return 1;
    }
    zdd_log_dderr(self, "", "DirectDrawCreate", g_dd_create_hr);
    return 0;
}

int zdd_set_coop_level(zdd *self, void *hwnd, int fullscreen)
{
    (void)hwnd; (void)fullscreen;
    if (g_dd_setcoop_result) return 1;
    zdd_log_dderr(self, "DirectDraw", "SetCooperativeLevel",
                  g_dd_setcoop_hr);
    return 0;
}

int zdd_create_surface(zdd *self, void **out_surface,
                       uint32_t width, uint32_t height,
                       uint32_t caps_base, int force_videomem)
{
    /* Host stub: still exercise the descriptor builder so we know
     * it runs end-to-end at the test level, then succeed/fail per
     * g_dd_createsurf_result. */
    zdd_surface_desc_build b;
    zdd_build_surface_desc(self, &b, width, height, caps_base,
                           force_videomem);
    (void)b;

    if (g_dd_createsurf_result) {
        if (out_surface) *out_surface = g_dd_createsurf_handle;
        return 1;
    }
    zdd_log_dderr(self, "DirectDraw", "CreateSurface",
                  g_dd_createsurf_hr);
    return 0;
}

int zdd_surface_set_color_key(void *surface, int32_t key, zdd *log_owner)
{
    g_dd_setkey_calls++;
    g_dd_setkey_last_surf = surface;
    g_dd_setkey_last_key  = key;
    if (g_dd_setkey_result) return 1;
    if (log_owner != NULL) {
        zdd_log_dderr(log_owner, "DirectDrawSurface", "SetColorKey",
                      g_dd_setkey_hr);
    }
    return 0;
}

void zdd_create_clipper(zdd *parent, void **out_clipper)
{
    g_dd_create_clipper_calls++;
    g_dd_create_clipper_seq    = ++g_dd_clipper_seq;
    g_dd_create_clipper_parent = parent;
    if (out_clipper) *out_clipper = g_dd_create_clipper_handle;
    if (g_dd_create_clipper_handle != NULL) g_live_coms++;
}

void zdd_clipper_set_clip_list_rect(void *clipper,
                                    uint32_t width, uint32_t height)
{
    g_dd_setcliplist_calls++;
    g_dd_setcliplist_seq    = ++g_dd_clipper_seq;
    g_dd_setcliplist_last   = clipper;
    g_dd_setcliplist_last_w = width;
    g_dd_setcliplist_last_h = height;
}

void zdd_surface_set_clipper(void *surface, void *clipper)
{
    g_dd_setclipper_calls++;
    g_dd_setclipper_seq       = ++g_dd_clipper_seq;
    g_dd_setclipper_last_surf = surface;
    g_dd_setclipper_last_clip = clipper;
}

int zdd_set_display_mode(zdd *self, uint32_t width, uint32_t height,
                         uint32_t bpp, uint32_t refresh_hz)
{
    g_dd_setmode_calls++;
    g_dd_setmode_last_w       = width;
    g_dd_setmode_last_h       = height;
    g_dd_setmode_last_bpp     = bpp;
    g_dd_setmode_last_refresh = refresh_hz;
    if (g_dd_setmode_result) return 1;
    zdd_log_dderr(self, "DirectDraw", "SetDisplayMode",
                  g_dd_setmode_hr);
    return 0;
}

int zdd_get_display_mode(zdd *self, uint32_t *out_width,
                         uint32_t *out_height, uint32_t *out_pitch)
{
    (void)self;
    g_dd_getmode_calls++;
    if (!g_dd_getmode_result) return 0;
    if (out_width  != NULL) *out_width  = g_dd_getmode_width;
    if (out_height != NULL) *out_height = g_dd_getmode_height;
    if (out_pitch  != NULL) *out_pitch  = g_dd_getmode_pitch;
    return 1;
}

void zdd_busy_wait_ms(uint32_t ms)
{
    g_busy_wait_calls++;
    g_busy_wait_last_ms = ms;
}

int zdd_create_primary_surface(zdd *self,
                               const zdd_primary_desc_build *desc)
{
    g_dd_create_primary_calls++;
    g_dd_create_primary_last_flags = desc->dwFlags;
    g_dd_create_primary_last_caps  = desc->dwCaps;
    g_dd_create_primary_last_count = desc->dwBackBufferCount;
    if (!g_dd_create_primary_result) {
        zdd_log_dderr(self, "DirectDraw", "CreateSurface",
                      g_dd_create_primary_hr);
        return 0;
    }
    self->com_a = g_dd_create_primary_handle;
    return 1;
}

int zdd_create_system_palette(zdd *self)
{
    g_dd_create_pal_calls++;
    if (!g_dd_create_pal_result) {
        zdd_log_dderr(self, "DirectDraw", "CreatePalette",
                      g_dd_create_pal_hr);
        return 0;
    }
    self->com_b = g_dd_create_pal_handle;
    return 1;
}

int zdd_surface_set_palette(void *surface, void *palette, zdd *log_owner)
{
    if (surface == NULL || palette == NULL) return 0;
    g_dd_setpal_calls++;
    g_dd_setpal_last_surf = surface;
    g_dd_setpal_last_pal  = palette;
    if (!g_dd_setpal_result) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface", "SetPalette",
                          g_dd_setpal_hr);
        }
        return 0;
    }
    return 1;
}

int zdd_get_attached_surface(void *primary, uint32_t caps_in,
                             void **out, zdd *log_owner)
{
    g_dd_attached_calls++;
    g_dd_attached_last_primary = primary;
    g_dd_attached_last_caps    = caps_in;
    if (!g_dd_attached_result) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface",
                          "GetAttachedSurface", g_dd_attached_hr);
        }
        return 0;
    }
    if (out != NULL) *out = g_dd_attached_handle;
    return 1;
}

/* ─── present-dispatcher primitive stubs ─────────────────────────── */

int zdd_surface_get_dc(void *surface, void **out_hdc)
{
    g_dd_getdc_calls++;
    g_dd_getdc_seq = ++g_dd_paint_seq;
    g_dd_getdc_last_surf = surface;
    if (surface == NULL || out_hdc == NULL) {
        if (out_hdc != NULL) *out_hdc = NULL;
        return 0;
    }
    if (!g_dd_getdc_result) {
        *out_hdc = NULL;
        return 0;
    }
    *out_hdc = g_dd_getdc_handle;
    return 1;
}

void zdd_surface_release_dc(void *surface, void *hdc)
{
    g_dd_releasedc_calls++;
    g_dd_releasedc_seq = ++g_dd_paint_seq;
    g_dd_releasedc_last_surf = surface;
    g_dd_releasedc_last_hdc  = hdc;
}

int zdd_surface_flip(void *surface, void *target, uint32_t flags,
                     zdd *log_owner)
{
    g_dd_flip_calls++;
    g_dd_flip_last_surf   = surface;
    g_dd_flip_last_target = target;
    g_dd_flip_last_flags  = flags;
    if (surface == NULL) return 0;
    if (!g_dd_flip_result) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface", "Flip",
                          g_dd_flip_hr);
        }
        return 0;
    }
    return 1;
}

int zdd_surface_blt(void *dest, const int32_t *dest_rect,
                    void *src,  const int32_t *src_rect,
                    uint32_t flags, zdd *log_owner)
{
    g_dd_blt_calls++;
    g_dd_blt_last_dest  = dest;
    g_dd_blt_last_src   = src;
    g_dd_blt_last_flags = flags;
    g_dd_blt_last_has_dest_rect = (dest_rect != NULL);
    g_dd_blt_last_has_src_rect  = (src_rect  != NULL);
    if (dest_rect != NULL) memcpy(g_dd_blt_last_dest_rect, dest_rect,
                                  sizeof(g_dd_blt_last_dest_rect));
    if (src_rect  != NULL) memcpy(g_dd_blt_last_src_rect,  src_rect,
                                  sizeof(g_dd_blt_last_src_rect));
    if (dest == NULL) return 0;
    if (!g_dd_blt_result) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface", "Blt",
                          g_dd_blt_hr);
        }
        return 0;
    }
    return 1;
}

int zdd_desktop_present(void *src_hdc, int dest_x, int dest_y,
                        int width, int height)
{
    g_dd_desktop_calls++;
    g_dd_desktop_last_src = src_hdc;
    g_dd_desktop_last_x   = dest_x;
    g_dd_desktop_last_y   = dest_y;
    g_dd_desktop_last_w   = width;
    g_dd_desktop_last_h   = height;
    if (src_hdc == NULL) return 0;
    return g_dd_desktop_result;
}

int zdd_surface_blt_color_fill(void *surface, uint32_t fill_value,
                               zdd *log_owner)
{
    /* Not used by any pure-logic consumer; keep a thin stub so any
     * future test that does reference it doesn't pull in a link error. */
    (void)surface; (void)fill_value; (void)log_owner;
    return 1;
}

/* Surface-Lock stub.  Populates the caller's DDSD with canned
 * lpSurface / pitch / height (so zdd_object_clear tests can verify
 * the right pointer + size were used). */
int zdd_surface_lock(void *surface, void *ddsd, uint32_t flags,
                     int32_t *out_hr)
{
    g_dd_lock_calls++;
    g_dd_lock_seq = ++g_dd_clear_seq;
    g_dd_lock_last_surf  = surface;
    g_dd_lock_last_ddsd  = ddsd;
    g_dd_lock_last_flags = flags;
    if (surface == NULL) {
        if (out_hr != NULL) *out_hr = 0;
        return 0;
    }
    if (!g_dd_lock_result) {
        if (out_hr != NULL) *out_hr = g_dd_lock_hr;
        return 0;
    }
    /* Note: on the real (Win32) build, Lock populates the DDSD with
     * lpSurface / lPitch / dwHeight at offsets 0x24 / 0x10 / 0x08.
     * The host stub skips that population because the 4-byte DDSD
     * slot can't hold an 8-byte host pointer; consumers read via
     * zdd_object_get_locked_info which we publish from the
     * g_dd_lock_fill_* values directly. */
    if (out_hr != NULL) *out_hr = 0;
    return 1;
}

void zdd_surface_unlock(void *surface)
{
    g_dd_unlock_calls++;
    g_dd_unlock_seq = ++g_dd_clear_seq;
    g_dd_unlock_last_surf = surface;
}

int zdd_surface_get_desc(void *surface, void *ddsd)
{
    g_dd_getdesc_calls++;
    g_dd_getdesc_last_surf = surface;
    if (surface == NULL || ddsd == NULL) return 0;
    if (!g_dd_getdesc_result) return 0;
    /* Populate the embedded DDPIXELFORMAT R/G/B masks at offsets
     * 0x58 / 0x5c / 0x60.  Mirrors the real GetSurfaceDesc populating
     * the same fields from the surface's pixel format. */
    uint8_t *d = (uint8_t *)ddsd;
    *(uint32_t *)(d + 0x58) = g_dd_getdesc_fill_r_mask;
    *(uint32_t *)(d + 0x5c) = g_dd_getdesc_fill_g_mask;
    *(uint32_t *)(d + 0x60) = g_dd_getdesc_fill_b_mask;
    return 1;
}

/* Host stub for the post-Lock DDSD extractor.  Returns whatever the
 * test set g_dd_lock_fill_* to — the Lock stub records it but doesn't
 * populate the embedded DDSD (pointer-size mismatch makes that
 * impractical on host).  Tests that exercise zdd_object_clear set the
 * fill values directly. */
int zdd_object_get_locked_info(zdd_object *self, void **out_buf,
                               int32_t *out_pitch, int32_t *out_height)
{
    if (self == NULL) return 0;
    if (out_buf != NULL)    *out_buf    = g_dd_lock_fill_surface;
    if (out_pitch != NULL)  *out_pitch  = g_dd_lock_fill_pitch;
    if (out_height != NULL) *out_height = g_dd_lock_fill_height;
    return 1;
}

void *zdd_window_paint_begin(void *hwnd, void **out_cookie)
{
    g_dd_wpaint_begin_calls++;
    g_dd_wpaint_begin_seq = ++g_dd_paint_seq;
    g_dd_wpaint_begin_last_hwnd = hwnd;
    if (out_cookie != NULL) *out_cookie = g_dd_wpaint_begin_cookie;
    return g_dd_wpaint_begin_hdc;
}

void zdd_window_paint_end(void *hwnd, void *cookie)
{
    g_dd_wpaint_end_calls++;
    g_dd_wpaint_end_seq = ++g_dd_paint_seq;
    g_dd_wpaint_end_last_hwnd   = hwnd;
    g_dd_wpaint_end_last_cookie = cookie;
}

void zdd_window_blit_copy(void *dest_hdc, int dest_x, int dest_y,
                          int width, int height, void *src_hdc)
{
    g_dd_wblit_calls++;
    g_dd_wblit_seq = ++g_dd_paint_seq;
    g_dd_wblit_last_dest_hdc = dest_hdc;
    g_dd_wblit_last_src_hdc  = src_hdc;
    g_dd_wblit_last_x = dest_x;
    g_dd_wblit_last_y = dest_y;
    g_dd_wblit_last_w = width;
    g_dd_wblit_last_h = height;
}

/* ─── ctor / struct layout ───────────────────────────────────────── */

int test_zdd_layout_matches_retail_offsets(void)
{
#if UINTPTR_MAX != 0xFFFFFFFFu
    T_SKIP("offset layout asserts are 32-bit-only");
#else
    /* The _Static_assert block in zdd.h compiles only on 32-bit; if
     * this TU got built at all on a 32-bit host, all the asserts
     * passed at compile time.  This test is a smoke-test that the
     * header is being included correctly. */
    zdd s;
    T_ASSERT_EQ_U(sizeof(s), 0x170u);
    return 0;
#endif
}

int test_zdd_ctor_zeros_struct(void)
{
    reset_stubs();
    zdd s;
    memset(&s, 0xa5, sizeof(s));
    zdd_ctor(&s);

    /* Every observable field should be zero except cursor_state == 1
     * and log_buf[0] == 0 (which is the "empty string" the ctor wants). */
    T_ASSERT_EQ_I(s.cursor_state, 1);
    T_ASSERT_EQ_P(s.ddraw7, NULL);
    T_ASSERT_EQ_P(s.com_a,  NULL);
    T_ASSERT_EQ_P(s.com_b,  NULL);
    T_ASSERT_EQ_P(s.primary_obj, NULL);
    T_ASSERT_EQ_P(s.back_obj_a,  NULL);
    T_ASSERT_EQ_P(s.back_obj_b,  NULL);
    T_ASSERT_EQ_I(s.open_objects, 0);
    T_ASSERT_EQ_I((int)s.log_buf[0], 0);
    /* Pad/format hint fields also zeroed (retail leaves these as the
     * operator_new garbage, but we make them deterministic). */
    T_ASSERT_EQ_I(s.pixel_format_mode, 0);
    T_ASSERT_EQ_I(s.pixel_format_bpp,  0);
    return 0;
}

/* ─── DDERR table ────────────────────────────────────────────────── */

int test_zdd_dderr_name_known_entries(void)
{
    /* Spot-check a handful from across the table — codespace 0x88760
     * and the two Windows-codespace alias entries. */
    T_ASSERT(strcmp(zdd_dderr_name((int32_t)0x88760482),
                    "DDERR_INVALIDOBJECT") == 0);
    T_ASSERT(strcmp(zdd_dderr_name((int32_t)0x8876024e),
                    "DDERR_UNSUPPORTEDMODE") == 0);
    T_ASSERT(strcmp(zdd_dderr_name((int32_t)0x80070057),
                    "DDERR_INVALIDPARAMS") == 0);
    T_ASSERT(strcmp(zdd_dderr_name((int32_t)0x8007000e),
                    "DDERR_OUTOFMEMORY") == 0);
    T_ASSERT(strcmp(zdd_dderr_name((int32_t)0x887604e6),
                    "DDERR_NOFLIPHW") == 0);
    return 0;
}

int test_zdd_dderr_name_unknown_returns_null(void)
{
    T_ASSERT(zdd_dderr_name(0) == NULL);
    T_ASSERT(zdd_dderr_name((int32_t)0x80000000) == NULL);
    T_ASSERT(zdd_dderr_name((int32_t)0xdeadbeef) == NULL);
    return 0;
}

/* ─── log builder format ─────────────────────────────────────────── */

int test_zdd_log_dderr_format_known_hresult(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Mimic the SetCooperativeLevel-with-INVALIDPARAMS path. */
    zdd_log_dderr(&s, "DirectDraw", "SetCooperativeLevel",
                  (int32_t)0x80070057);
    const char *want =
        "Warning,exists ZDD errors,DirectDraw.SetCooperativeLevel"
        " failed,Error Code 80070057DDERR_INVALIDPARAMS.\n";
    T_ASSERT_EQ_I(g_log_call_count, 1);
    T_ASSERT(strcmp(s.log_buf, want) == 0);
    T_ASSERT(strcmp(g_log_capture, want) == 0);
    return 0;
}

int test_zdd_log_dderr_format_empty_prefix1(void)
{
    /* DirectDrawCreate-fail call site passes prefix1 = "" (empty BSS
     * string in retail). */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_log_dderr(&s, "", "DirectDrawCreate", (int32_t)0x88760233);
    const char *want =
        "Warning,exists ZDD errors,.DirectDrawCreate failed,Error Code"
        " 88760233DDERR_NODIRECTDRAWHW.\n";
    T_ASSERT(strcmp(s.log_buf, want) == 0);
    return 0;
}

int test_zdd_log_dderr_format_unknown_hresult(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_log_dderr(&s, "DirectDraw", "Foo", (int32_t)0xdeadbeef);
    /* Unknown HRESULT — DDERR name strcat is skipped. */
    const char *want =
        "Warning,exists ZDD errors,DirectDraw.Foo failed,Error Code"
        " deadbeef.\n";
    T_ASSERT(strcmp(s.log_buf, want) == 0);
    return 0;
}

int test_zdd_log_dderr_format_null_prefixes_tolerated(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* NULL prefixes shouldn't crash — retail doesn't pass NULL but
     * defensive coding here matches our buf_append's NULL-skip. */
    zdd_log_dderr(&s, NULL, NULL, 0);
    /* "0" rendered as base-16 with no leading zeros = "0". */
    const char *want =
        "Warning,exists ZDD errors,. failed,Error Code 0.\n";
    T_ASSERT(strcmp(s.log_buf, want) == 0);
    return 0;
}

/* ─── cursor-restore ─────────────────────────────────────────────── */

int test_zdd_restore_cursor_noop_when_shown(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);                  /* cursor_state = 1 */
    zdd_restore_cursor_on_release(&s);
    T_ASSERT_EQ_I(g_cursor_calls, 0);     /* ShowCursor not called */
    T_ASSERT_EQ_I(s.cursor_state, 1);
    return 0;
}

int test_zdd_restore_cursor_shows_when_hidden(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.cursor_state = 0;                   /* simulate engine hide */
    zdd_restore_cursor_on_release(&s);
    T_ASSERT_EQ_I(g_cursor_calls, 1);
    T_ASSERT_EQ_I(g_cursor_last_show, 1); /* ShowCursor(TRUE) */
    T_ASSERT_EQ_I(s.cursor_state, 1);     /* reset to "shown" */
    return 0;
}

/* ─── release children ───────────────────────────────────────────── */

/* Helper: allocate a heap ZDDObject + run zdd_object_ctor (which
 * bumps parent->open_objects).  Returns a malloc'd pointer that the
 * caller hands off to zdd_obj_destroy. */
static zdd_object *make_child(zdd *parent)
{
    zdd_object *o = (zdd_object *)calloc(1, sizeof(zdd_object));
    zdd_object_ctor(o, parent);
    return o;
}

int test_zdd_release_children_in_order_and_zeros_fields(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Wire up 3 live ZDDObject children (ctor bumps open_objects) +
     * 2 opaque COM children (just counter bumps).  zdd_release_children
     * → zdd_obj_destroy for the ZDDObjects (which runs dtor + free) and
     * zdd_com_release for the bare COM ptrs. */
    s.primary_obj = make_child(&s);
    s.back_obj_a  = make_child(&s);
    s.back_obj_b  = make_child(&s);
    s.com_a       = (void *)(uintptr_t)0x4444; g_live_coms++;
    s.com_b       = (void *)(uintptr_t)0x5555; g_live_coms++;

    T_ASSERT_EQ_I(s.open_objects, 3);

    zdd_release_children(&s);

    T_ASSERT_EQ_I(g_live_coms, 0);
    T_ASSERT_EQ_I(s.open_objects, 0); /* each dtor decremented it */
    T_ASSERT_EQ_P(s.primary_obj, NULL);
    T_ASSERT_EQ_P(s.back_obj_a,  NULL);
    T_ASSERT_EQ_P(s.back_obj_b,  NULL);
    T_ASSERT_EQ_P(s.com_a,       NULL);
    T_ASSERT_EQ_P(s.com_b,       NULL);
    return 0;
}

int test_zdd_release_children_skips_nulls(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* All NULL — release should be a complete no-op. */
    zdd_release_children(&s);
    T_ASSERT_EQ_I(g_live_coms, 0);
    T_ASSERT_EQ_I(s.open_objects, 0);
    return 0;
}

/* ─── full dtor ──────────────────────────────────────────────────── */

int test_zdd_dtor_releases_everything_and_flushes_log(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Live ddraw7, two COM kids, one ZDDObject, hidden cursor, AND
     * a pre-populated log buffer.  open_objects is bumped by the
     * ZDDObject ctor inside make_child — we deliberately leave it
     * NONZERO so the dtor's "Warning,exists ZDD objects" path fires:
     * the child gets dropped FIRST (decrementing it to 0), but the
     * dtor's check on open_objects runs AFTER the release-children
     * pass, so we add an extra +2 to keep the warning live. */
    s.ddraw7 = (void *)(uintptr_t)0xaaaaaaaa; g_live_coms++;
    s.com_a  = (void *)(uintptr_t)0xbbbbbbbb; g_live_coms++;
    s.primary_obj = make_child(&s);       /* open_objects: 0→1 */
    s.open_objects += 2;                  /* leak two unaccounted */
    s.cursor_state = 0;
    strcpy(s.log_buf, "stale error message\n");

    zdd_dtor(&s);

    /* ShowCursor restore happened once. */
    T_ASSERT_EQ_I(g_cursor_calls, 1);
    T_ASSERT_EQ_I(g_cursor_last_show, 1);

    /* All COM + heap children dropped. */
    T_ASSERT_EQ_I(g_live_coms, 0);
    T_ASSERT_EQ_P(s.ddraw7, NULL);
    T_ASSERT_EQ_P(s.primary_obj, NULL);
    /* open_objects: ctor bumped to 1, +2 manual = 3.  After
     * release-children's dtor decrement, 2 unaccounted leaks remain. */
    T_ASSERT_EQ_I(s.open_objects, 2);

    /* Two OutputDebugStringA calls — one "Warning,exists ZDD
     * objects." then one for the stale log buffer. */
    T_ASSERT_EQ_I(g_log_call_count, 2);
    T_ASSERT(strstr(g_log_capture, "Warning,exists ZDD objects.")  != NULL);
    T_ASSERT(strstr(g_log_capture, "stale error message")           != NULL);
    return 0;
}

int test_zdd_dtor_quiet_when_clean(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_dtor(&s);
    /* Nothing live, nothing logged, no cursor restore. */
    T_ASSERT_EQ_I(g_cursor_calls, 0);
    T_ASSERT_EQ_I(g_log_call_count, 0);
    return 0;
}

int test_zdd_dtor_open_objects_warns_only(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.open_objects = 1;                  /* leak observed at dtor */
    zdd_dtor(&s);
    T_ASSERT_EQ_I(g_log_call_count, 1); /* the "ZDD objects" warning */
    T_ASSERT(strstr(g_log_capture, "Warning,exists ZDD objects.")  != NULL);
    return 0;
}

int test_zdd_dtor_flushes_log_buffer_if_nonempty(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    strcpy(s.log_buf, "uh oh");
    zdd_dtor(&s);
    T_ASSERT_EQ_I(g_log_call_count, 1);
    T_ASSERT(strcmp(g_log_capture, "uh oh") == 0);
    return 0;
}

/* ─── create driver ──────────────────────────────────────────────── */

int test_zdd_create_success_path(void)
{
    reset_stubs();
    zdd *p = NULL;
    int rc = zdd_create(&p);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT(p != NULL);
    T_ASSERT_EQ_P(p->ddraw7, g_dd_fake_iface);
    T_ASSERT_EQ_I(p->cursor_state, 1);  /* ctor ran */
    T_ASSERT_EQ_I(g_live_coms, 1);      /* ddraw7 live */

    zdd_destroy(p);
    T_ASSERT_EQ_I(g_live_coms, 0);      /* freed on destroy */
    return 0;
}

/* ─── DDSURFACEDESC2 builder ─────────────────────────────────────── */

/* Bit constants — mirrored from src/zdd.c's local enum so test
 * expectations are self-documenting.  Keep in sync.  */
#define Z_DDSD_CAPS              0x0001u
#define Z_DDSD_HEIGHT            0x0002u
#define Z_DDSD_WIDTH             0x0004u
#define Z_DDSD_PIXELFORMAT       0x1000u
#define Z_DDSCAPS_OFFSCREENPLAIN 0x0040u
#define Z_DDSCAPS_VIDEOMEMORY    0x0800u
#define Z_DDPF_RGB               0x0040u
#define Z_DDPF_PALETTEINDEXED8   0x0020u

int test_zdd_build_desc_minimal_no_pixelformat(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Default: pixel_format_mode != 2 means "no explicit format".
     * Caps = caps_base | OFFSCREENPLAIN. */
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 640, 480, /*caps_base*/ 0,
                           /*force_videomem*/ 0);

    T_ASSERT_EQ_U(b.dwFlags, Z_DDSD_CAPS | Z_DDSD_HEIGHT | Z_DDSD_WIDTH);
    T_ASSERT_EQ_U(b.dwWidth, 640);
    T_ASSERT_EQ_U(b.dwHeight, 480);
    T_ASSERT_EQ_U(b.dwCaps, Z_DDSCAPS_OFFSCREENPLAIN);
    T_ASSERT_EQ_I(b.has_pixel_format, 0);
    /* DDPF fields all zero. */
    T_ASSERT_EQ_U(b.ddpf_dwSize,        0);
    T_ASSERT_EQ_U(b.ddpf_dwFlags,       0);
    T_ASSERT_EQ_U(b.ddpf_dwRGBBitCount, 0);
    T_ASSERT_EQ_U(b.ddpf_dwRBitMask,    0);
    T_ASSERT_EQ_U(b.ddpf_dwGBitMask,    0);
    T_ASSERT_EQ_U(b.ddpf_dwBBitMask,    0);
    return 0;
}

int test_zdd_build_desc_caps_base_preserved(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* caps_base bits should be preserved + OFFSCREENPLAIN OR'd in. */
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 320, 200, /*caps_base*/ 0x200,
                           /*force_videomem*/ 0);
    T_ASSERT_EQ_U(b.dwCaps, 0x200u | Z_DDSCAPS_OFFSCREENPLAIN);
    return 0;
}

int test_zdd_build_desc_force_videomem_adds_caps(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 100, 100, 0, /*force_videomem*/ 1);
    T_ASSERT_EQ_U(b.dwCaps,
                  Z_DDSCAPS_OFFSCREENPLAIN | Z_DDSCAPS_VIDEOMEMORY);
    return 0;
}

int test_zdd_build_desc_self_videomem_flag_adds_caps(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.videomem_flag = 1;
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 100, 100, 0, /*force_videomem*/ 0);
    T_ASSERT_EQ_U(b.dwCaps,
                  Z_DDSCAPS_OFFSCREENPLAIN | Z_DDSCAPS_VIDEOMEMORY);
    return 0;
}

int test_zdd_build_desc_pixelformat_mode_2_8bpp(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.pixel_format_bpp  = 8;
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 640, 480, 0, 0);

    T_ASSERT(b.dwFlags & Z_DDSD_PIXELFORMAT);
    T_ASSERT_EQ_I(b.has_pixel_format, 1);
    T_ASSERT_EQ_U(b.ddpf_dwSize,  0x20);
    /* 8bpp: PALETTEINDEXED8 added on top of RGB. */
    T_ASSERT_EQ_U(b.ddpf_dwFlags, Z_DDPF_RGB | Z_DDPF_PALETTEINDEXED8);
    T_ASSERT_EQ_U(b.ddpf_dwRGBBitCount, 0);
    /* Masks remain 0 in 8bpp branch. */
    T_ASSERT_EQ_U(b.ddpf_dwRBitMask, 0);
    return 0;
}

int test_zdd_build_desc_pixelformat_mode_2_16bpp_rgb565(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.pixel_format_bpp  = 16;
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 640, 480, 0, 0);

    T_ASSERT_EQ_I(b.has_pixel_format, 1);
    T_ASSERT_EQ_U(b.ddpf_dwFlags, Z_DDPF_RGB);
    T_ASSERT_EQ_U(b.ddpf_dwRGBBitCount, 16);
    T_ASSERT_EQ_U(b.ddpf_dwRBitMask, 0xF800u);
    T_ASSERT_EQ_U(b.ddpf_dwGBitMask, 0x07E0u);
    T_ASSERT_EQ_U(b.ddpf_dwBBitMask, 0x001Fu);
    return 0;
}

int test_zdd_build_desc_pixelformat_mode_2_24bpp(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.pixel_format_bpp  = 24;
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 640, 480, 0, 0);
    T_ASSERT_EQ_U(b.ddpf_dwRGBBitCount, 24);
    T_ASSERT_EQ_U(b.ddpf_dwRBitMask, 0xFF0000u);
    T_ASSERT_EQ_U(b.ddpf_dwGBitMask, 0x00FF00u);
    T_ASSERT_EQ_U(b.ddpf_dwBBitMask, 0x0000FFu);
    return 0;
}

int test_zdd_build_desc_pixelformat_mode_2_32bpp(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.pixel_format_bpp  = 32;
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 1024, 768, 0, 0);
    T_ASSERT_EQ_U(b.ddpf_dwRGBBitCount, 32);
    /* Same masks as 24bpp — retail's fall-through quirk. */
    T_ASSERT_EQ_U(b.ddpf_dwRBitMask, 0xFF0000u);
    T_ASSERT_EQ_U(b.ddpf_dwGBitMask, 0x00FF00u);
    T_ASSERT_EQ_U(b.ddpf_dwBBitMask, 0x0000FFu);
    return 0;
}

int test_zdd_build_desc_pixelformat_mode_2_unknown_bpp(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.pixel_format_bpp  = 12;          /* not in retail's switch */
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 100, 100, 0, 0);
    T_ASSERT_EQ_I(b.has_pixel_format, 1);
    T_ASSERT_EQ_U(b.ddpf_dwFlags, Z_DDPF_RGB);
    /* Unknown bpp: masks and bitcount stay 0. */
    T_ASSERT_EQ_U(b.ddpf_dwRGBBitCount, 0);
    T_ASSERT_EQ_U(b.ddpf_dwRBitMask,    0);
    return 0;
}

int test_zdd_build_desc_mode_other_no_pixelformat_even_with_bpp(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 1;           /* not 2 */
    s.pixel_format_bpp  = 16;
    zdd_surface_desc_build b;
    zdd_build_surface_desc(&s, &b, 640, 480, 0, 0);
    /* pixel_format_mode != 2 short-circuits regardless of bpp. */
    T_ASSERT_EQ_I(b.has_pixel_format, 0);
    T_ASSERT((b.dwFlags & Z_DDSD_PIXELFORMAT) == 0);
    T_ASSERT_EQ_U(b.ddpf_dwSize, 0);
    return 0;
}

/* ─── ZDDObject lifecycle ────────────────────────────────────────── */

int test_zdd_object_layout_matches_retail_offsets(void)
{
#if UINTPTR_MAX != 0xFFFFFFFFu
    T_SKIP("offset layout asserts are 32-bit-only");
#else
    zdd_object o;
    T_ASSERT_EQ_U(sizeof(o), 0xd8u);
    return 0;
#endif
}

int test_zdd_object_ctor_sets_parent_and_bumps_open_objects(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    T_ASSERT_EQ_I(parent.open_objects, 0);

    zdd_object o;
    memset(&o, 0xa5, sizeof(o));    /* garbage to verify zero-init */
    zdd_object_ctor(&o, &parent);

    T_ASSERT_EQ_P(o.parent, &parent);
    T_ASSERT_EQ_I(parent.open_objects, 1);
    T_ASSERT_EQ_P(o.com_primary, NULL);
    T_ASSERT_EQ_P(o.com_back,    NULL);
    T_ASSERT_EQ_P(o.pixel_buf,   NULL);
    T_ASSERT_EQ_I(o.pixel_buf_flag, 0);
    T_ASSERT_EQ_I(o.state_flag,     0);
    return 0;
}

int test_zdd_object_release_pixel_buf_frees_and_clears(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    /* Pretend the surface-alloc path stamped a pixel buffer + flag. */
    o.pixel_buf      = (void *)(uintptr_t)0xdeadbeef;
    o.pixel_buf_flag = 1;
    g_live_pixel_bufs = 1;

    zdd_object_release_pixel_buf(&o);

    T_ASSERT_EQ_I(g_live_pixel_bufs, 0);
    T_ASSERT_EQ_P(o.pixel_buf, NULL);
    T_ASSERT_EQ_I(o.pixel_buf_flag, 0);
    return 0;
}

int test_zdd_object_release_pixel_buf_noop_when_null(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    zdd_object_release_pixel_buf(&o);          /* no buffer attached */
    T_ASSERT_EQ_I(g_live_pixel_bufs, 0);
    return 0;
}

int test_zdd_object_dtor_releases_in_retail_order(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    T_ASSERT_EQ_I(parent.open_objects, 1);

    /* Wire up live: pixel buffer + two COM children. */
    o.pixel_buf  = (void *)(uintptr_t)0xfeed;
    o.pixel_buf_flag = 1;  g_live_pixel_bufs = 1;
    o.com_primary = (void *)(uintptr_t)0xaaaa; g_live_coms++;
    o.com_back    = (void *)(uintptr_t)0xbbbb; g_live_coms++;

    zdd_object_dtor(&o);

    T_ASSERT_EQ_I(g_live_pixel_bufs, 0);
    T_ASSERT_EQ_I(g_live_coms,       0);
    T_ASSERT_EQ_P(o.com_primary, NULL);
    T_ASSERT_EQ_P(o.com_back,    NULL);
    T_ASSERT_EQ_P(o.pixel_buf,   NULL);
    T_ASSERT_EQ_I(parent.open_objects, 0);
    return 0;
}

int test_zdd_object_dtor_clean_object_noop(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    T_ASSERT_EQ_I(parent.open_objects, 1);

    /* All fields zeroed by ctor — dtor should be a no-op except for
     * decrementing parent->open_objects. */
    zdd_object_dtor(&o);
    T_ASSERT_EQ_I(g_live_pixel_bufs, 0);
    T_ASSERT_EQ_I(g_live_coms,       0);
    T_ASSERT_EQ_I(parent.open_objects, 0);
    return 0;
}

int test_zdd_obj_destroy_walks_dtor_and_frees(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object *o = make_child(&parent);
    o->com_primary = (void *)(uintptr_t)0x1111; g_live_coms++;

    zdd_obj_destroy(&o);
    T_ASSERT_EQ_P(o, NULL);
    T_ASSERT_EQ_I(parent.open_objects, 0);
    T_ASSERT_EQ_I(g_live_coms, 0);
    return 0;
}

int test_zdd_obj_destroy_idempotent_on_null(void)
{
    reset_stubs();
    zdd_object *o = NULL;
    zdd_obj_destroy(&o);
    T_ASSERT_EQ_P(o, NULL);
    return 0;
}

int test_zdd_create_failure_path_tears_down(void)
{
    reset_stubs();
    g_dd_create_result = 0;
    g_dd_create_hr     = (int32_t)0x88760233;  /* NODIRECTDRAWHW */

    zdd *p = NULL;
    int rc = zdd_create(&p);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT(p == NULL);

    /* zdd_log_dderr was called (1 log) AND the dtor flush re-emits
     * the still-populated buffer (1 more log).  So 2 total. */
    T_ASSERT_EQ_I(g_log_call_count, 2);
    T_ASSERT(strstr(g_log_capture, "DDERR_NODIRECTDRAWHW") != NULL);
    return 0;
}

/* ─── ZDDObject surface-alloc stampers ───────────────────────────── */

int test_zdd_object_prefill_stamps_caps_and_force_vm(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    zdd_object_prefill_desc(&o, /*caps_in*/ 0x123, /*force_vm*/ 0x77);
    T_ASSERT_EQ_I(o.caps_in,           0x123);
    T_ASSERT_EQ_I(o.force_videomem_in, 0x77);
    return 0;
}

int test_zdd_object_prefill_stamps_self_pointers(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    zdd_object_prefill_desc(&o, 0, 0);
    /* Three self-pointers should point at well-known sub-fields inside
     * the embedded DDSD region (offset 0x30 from the start of o). */
    T_ASSERT_EQ_P(o.ddsd_lpSurface_ref, &o.embedded_ddsd[0x24]);
    T_ASSERT_EQ_P(o.ddsd_pitch_ref,     &o.embedded_ddsd[0x10]);
    T_ASSERT_EQ_P(o.ddsd_height_ref,    &o.embedded_ddsd[0x08]);
    return 0;
}

int test_zdd_object_prefill_zeros_ddsd_and_stamps_dwsize(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    /* Garbage the DDSD region first to verify the prefill zeros it. */
    memset(o.embedded_ddsd, 0xa5, sizeof(o.embedded_ddsd));
    zdd_object_prefill_desc(&o, 0, 0);

    /* DDSD dwSize stamped 0x7c at the very start. */
    T_ASSERT_EQ_U(*(uint32_t *)&o.embedded_ddsd[0x00], 0x7cu);
    /* Every other byte in the DDSD region must be 0. */
    for (size_t i = 4; i < sizeof(o.embedded_ddsd); i++) {
        T_ASSERT_EQ_I((int)o.embedded_ddsd[i], 0);
    }
    return 0;
}

int test_zdd_object_stamp_metrics_writes_all_slots(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    /* Pre-poison the slots so a missed write would be visible. */
    o.metric_0c = o.metric_10 = o.metric_14 = -1;
    o.metric_18 = o.metric_1c = o.metric_20 = -1;
    o.metric_b0 = o.metric_b4 = o.metric_b8 = o.metric_bc = -1;

    zdd_object_stamp_metrics(&o,
        /*p1*/ 100, /*p2*/ 200, /*p3*/ 300,
        /*p4*/ 400, /*p5*/ 500, /*p6*/ 600);

    /* 98c0's literal write pattern. */
    T_ASSERT_EQ_I(o.metric_b0, 0);
    T_ASSERT_EQ_I(o.metric_b4, 0);
    T_ASSERT_EQ_I(o.metric_0c, 300);  /* p3 */
    T_ASSERT_EQ_I(o.metric_b8, 500);  /* p5 */
    T_ASSERT_EQ_I(o.metric_bc, 600);  /* p6 */
    T_ASSERT_EQ_I(o.metric_14, 500);  /* p5 */
    T_ASSERT_EQ_I(o.metric_18, 600);  /* p6 */
    T_ASSERT_EQ_I(o.metric_10, 400);  /* p4 */
    T_ASSERT_EQ_I(o.metric_1c, 100);  /* p1 */
    T_ASSERT_EQ_I(o.metric_20, 200);  /* p2 */
    return 0;
}

int test_zdd_object_stamp_metrics_does_not_touch_other_fields(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    /* Seed parent + neighbor fields and verify they survive. */
    o.com_primary    = (void *)(uintptr_t)0xabc;
    o.com_back       = (void *)(uintptr_t)0xdef;
    o.pixel_buf      = (void *)(uintptr_t)0x123;
    o.pixel_buf_flag = 7;
    o.caps_in        = 0x999;
    o.state_flag     = 0x5555;

    zdd_object_stamp_metrics(&o, 1, 2, 3, 4, 5, 6);

    T_ASSERT_EQ_P(o.com_primary,    (void *)(uintptr_t)0xabc);
    T_ASSERT_EQ_P(o.com_back,       (void *)(uintptr_t)0xdef);
    T_ASSERT_EQ_P(o.pixel_buf,      (void *)(uintptr_t)0x123);
    T_ASSERT_EQ_U(o.pixel_buf_flag, 7u);
    T_ASSERT_EQ_I(o.caps_in,        0x999);
    T_ASSERT_EQ_U(o.state_flag,     0x5555u);
    return 0;
}

/* ─── ZDDObject SetColorKey ──────────────────────────────────────── */

int test_zdd_object_set_color_key_sentinel_short_circuits(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0xface;

    /* Pre-poison state_flag to verify the sentinel branch clears it. */
    o.state_flag   = 0xdeadbeef;
    o.colorkey_in  = 0;
    o.colorkey_out = 0;

    int rc = zdd_object_set_color_key(&o, 0x1ffffff);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(o.colorkey_in,  0x1ffffff);
    T_ASSERT_EQ_I(o.colorkey_out, 0x1ffffff);
    T_ASSERT_EQ_U(o.state_flag,   0u);
    /* Sentinel path must NOT issue a SetColorKey call. */
    T_ASSERT_EQ_I(g_dd_setkey_calls, 0);
    return 0;
}

int test_zdd_object_set_color_key_non_sentinel_calls_vtable(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0xface;

    int rc = zdd_object_set_color_key(&o, 0xff00ff);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(o.colorkey_in,  0xff00ff);
    T_ASSERT_EQ_I(o.colorkey_out, 0xff00ff);
    T_ASSERT_EQ_U(o.state_flag,   0x8000u);
    T_ASSERT_EQ_I(g_dd_setkey_calls, 1);
    T_ASSERT_EQ_P(g_dd_setkey_last_surf, (void *)(uintptr_t)0xface);
    T_ASSERT_EQ_I(g_dd_setkey_last_key, 0xff00ff);
    return 0;
}

int test_zdd_object_set_color_key_failure_logs_and_returns_zero(void)
{
    reset_stubs();
    g_dd_setkey_result = 0;
    g_dd_setkey_hr     = (int32_t)0x88760482;  /* INVALIDOBJECT */
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0x1;

    int rc = zdd_object_set_color_key(&o, 0x123);
    T_ASSERT_EQ_I(rc, 0);
    /* state_flag was still stamped to 0x8000 before the call (and
     * retail leaves it that way on failure too). */
    T_ASSERT_EQ_U(o.state_flag, 0x8000u);
    T_ASSERT(strstr(g_log_capture, "DDERR_INVALIDOBJECT") != NULL);
    T_ASSERT(strstr(g_log_capture, "SetColorKey")         != NULL);
    return 0;
}

/* ─── orchestrator (FUN_005b95c0) ────────────────────────────────── */

int test_zdd_object_create_surface_pair_happy_path(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    /* Match the retail boot call shape (FUN_005b8b40 → 95c0):
     *   p1=640, p2=480, p3=0,
     *   p4=0x1ffffff (colorkey sentinel),
     *   p5=1 (count/force-vm), p6=0, p7=0,
     *   width=640, height=480. */
    zdd_object_create_surface_pair(&o,
        /*p1*/ 640, /*p2*/ 480, /*p3*/ 0,
        /*p4*/ 0x1ffffff, /*p5*/ 1, /*p6*/ 0, /*p7*/ 0,
        /*w*/  640, /*h*/  480);

    /* prefill stamps. */
    T_ASSERT_EQ_I(o.caps_in,           0);
    T_ASSERT_EQ_I(o.force_videomem_in, 1);
    T_ASSERT_EQ_U(*(uint32_t *)&o.embedded_ddsd[0], 0x7cu);

    /* CreateSurface ran (stub stamped com_primary). */
    T_ASSERT_EQ_P(o.com_primary, (void *)(uintptr_t)0x99887766);

    /* metric stamps from 98c0.  Retail calls 98c0 with
     * (p1, p2, p6, p7, width, height) — NOT (p1..p6).  So
     * metric_10 = p7 (not p4) and metric_14 = width (not p5).
     * See zdd.h orchestrator docstring for the disasm citation. */
    T_ASSERT_EQ_I(o.metric_1c, 640);  /* p1 */
    T_ASSERT_EQ_I(o.metric_20, 480);  /* p2 */
    T_ASSERT_EQ_I(o.metric_10, 0);    /* p7 */
    T_ASSERT_EQ_I(o.metric_14, 640);  /* width */
    T_ASSERT_EQ_I(o.metric_18, 480);  /* height */
    T_ASSERT_EQ_I(o.metric_b8, 640);  /* width mirror */
    T_ASSERT_EQ_I(o.metric_bc, 480);  /* height mirror */
    T_ASSERT_EQ_I(o.metric_0c, 0);    /* p6 */

    /* Colorkey sentinel path — no SetColorKey vtable call. */
    T_ASSERT_EQ_I(g_dd_setkey_calls, 0);
    T_ASSERT_EQ_U(o.state_flag, 0u);
    T_ASSERT_EQ_I(o.colorkey_in,  0x1ffffff);
    T_ASSERT_EQ_I(o.colorkey_out, 0x1ffffff);
    return 0;
}

int test_zdd_object_create_surface_pair_create_surface_fails(void)
{
    reset_stubs();
    g_dd_createsurf_result = 0;
    g_dd_createsurf_hr     = (int32_t)0x88760464;  /* INVALIDCAPS */
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    zdd_object_create_surface_pair(&o,
        0, 0, /*caps*/ 0x42, /*colorkey*/ 0x77, /*p5*/ 0, 0, 0,
        320, 200);

    /* CreateSurface failed → orchestrator bailed before stamping
     * metrics and before calling SetColorKey. */
    T_ASSERT_EQ_P(o.com_primary, NULL);
    T_ASSERT_EQ_I(o.metric_1c,   0);  /* never stamped */
    T_ASSERT_EQ_I(o.metric_10,   0);  /* never stamped */
    T_ASSERT_EQ_I(o.colorkey_in, 0);  /* SetColorKey never ran */
    T_ASSERT_EQ_I(g_dd_setkey_calls, 0);

    /* But prefill DID run — caps_in / force_videomem_in / DDSD stamps. */
    T_ASSERT_EQ_I(o.caps_in,           0x42);
    T_ASSERT_EQ_I(o.force_videomem_in, 0);
    T_ASSERT(strstr(g_log_capture, "DDERR_INVALIDCAPS") != NULL);
    T_ASSERT(strstr(g_log_capture, "CreateSurface")    != NULL);
    return 0;
}

int test_zdd_object_create_surface_pair_passes_caps_from_field(void)
{
    /* Retail re-reads caps from self->caps_in (just stamped by
     * prefill) before passing it to CreateSurface.  This test makes
     * the roundtrip observable: poke caps_in BEFORE the orchestrator
     * runs — prefill will overwrite it with p3, then CreateSurface
     * reads back p3 (not the poked value). */
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.caps_in = 0xdeadbeef;  /* pre-poison */

    zdd_object_create_surface_pair(&o, 0, 0, /*p3*/ 0x55, 0x1ffffff,
                                   0, 0, 0, 100, 100);
    /* caps_in ended up as p3, not the poisoned value. */
    T_ASSERT_EQ_I(o.caps_in, 0x55);
    return 0;
}

int test_zdd_object_create_surface_pair_with_real_colorkey(void)
{
    /* Non-sentinel colorkey → SetColorKey vtable call fires after
     * the metric stamps. */
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    zdd_object_create_surface_pair(&o, 10, 20, 0, /*p4*/ 0xabcdef,
                                   0, 0, 0, 64, 64);

    T_ASSERT_EQ_I(g_dd_setkey_calls, 1);
    T_ASSERT_EQ_I(g_dd_setkey_last_key, 0xabcdef);
    T_ASSERT_EQ_U(o.state_flag, 0x8000u);
    T_ASSERT_EQ_I(o.colorkey_out, 0xabcdef);
    return 0;
}

int test_zdd_object_create_surface_pair_returns_1_on_sentinel_success(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    int rc = zdd_object_create_surface_pair(&o, 0, 0, 0, 0x1ffffff,
                                            1, 0, 0, 100, 100);
    T_ASSERT_EQ_I(rc, 1);
    return 0;
}

int test_zdd_object_create_surface_pair_returns_0_on_create_fail(void)
{
    reset_stubs();
    g_dd_createsurf_result = 0;
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    int rc = zdd_object_create_surface_pair(&o, 0, 0, 0, 0x1ffffff,
                                            1, 0, 0, 100, 100);
    T_ASSERT_EQ_I(rc, 0);
    return 0;
}

int test_zdd_object_create_surface_pair_returns_0_on_setkey_fail(void)
{
    /* CreateSurface succeeded, SetColorKey failed → orchestrator
     * returns 0.  Retail's implicit EAX-carry-through behaviour.
     * The wrapper FUN_005b8b40 uses this to decide cleanup. */
    reset_stubs();
    g_dd_setkey_result = 0;
    g_dd_setkey_hr     = (int32_t)0x88760482;
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    int rc = zdd_object_create_surface_pair(&o, 0, 0, 0, /*real key*/ 0x42,
                                            0, 0, 0, 100, 100);
    T_ASSERT_EQ_I(rc, 0);
    /* But CreateSurface DID succeed — surface stamped on the object. */
    T_ASSERT_EQ_P(o.com_primary, (void *)(uintptr_t)0x99887766);
    return 0;
}

/* ─── zdd_object_new (FUN_005b8b40) ──────────────────────────────── */

int test_zdd_object_new_happy_path(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);

    zdd_object *o = NULL;
    int rc = zdd_object_new(&parent, &o,
                            /*w*/ 640, /*h*/ 480,
                            /*colorkey*/ 0x1ffffff,
                            /*count*/ 1);

    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT(o != NULL);
    /* ctor ran: parent linked, open_objects bumped. */
    T_ASSERT_EQ_P(o->parent, &parent);
    T_ASSERT_EQ_I(parent.open_objects, 1);
    /* Orchestrator ran: caps_in/force_videomem_in stamped, com_primary
     * populated by the CreateSurface stub. */
    T_ASSERT_EQ_I(o->caps_in,           0);
    T_ASSERT_EQ_I(o->force_videomem_in, 1);
    T_ASSERT_EQ_P(o->com_primary, (void *)(uintptr_t)0x99887766);

    /* Tear down via the parent-driven path to verify zdd_object_dtor
     * runs cleanly on a fully-populated object. */
    zdd_obj_destroy(&o);
    T_ASSERT_EQ_P(o, NULL);
    T_ASSERT_EQ_I(parent.open_objects, 0);
    return 0;
}

int test_zdd_object_new_create_failure_cleans_up(void)
{
    reset_stubs();
    g_dd_createsurf_result = 0;
    g_dd_createsurf_hr     = (int32_t)0x88760464;  /* INVALIDCAPS */
    zdd parent; zdd_ctor(&parent);

    zdd_object *o = (zdd_object *)(uintptr_t)0xdead;   /* pre-poison */
    int rc = zdd_object_new(&parent, &o, 320, 200, 0x1ffffff, 1);

    T_ASSERT_EQ_I(rc, 0);
    /* On failure, *out is NOT written (retail leaves caller's slot
     * untouched).  Our poison value survives. */
    T_ASSERT_EQ_P(o, (zdd_object *)(uintptr_t)0xdead);
    /* And the ZDDObject we allocated got dtor'd + freed — parent's
     * open_objects counter is back to 0 (ctor bumped to 1, dtor
     * decremented to 0). */
    T_ASSERT_EQ_I(parent.open_objects, 0);
    /* DDERR logged. */
    T_ASSERT(strstr(g_log_capture, "DDERR_INVALIDCAPS") != NULL);
    return 0;
}

/* ─── zdd_object_attach_clipper (FUN_005b9520) ───────────────────── */

int test_zdd_object_attach_clipper_call_order(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    parent.ddraw7 = (void *)(uintptr_t)0xdd7;
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0x5117ace;
    /* Retail's clipper attach reads metric_b8 / metric_bc as the
     * RGNDATA RECT bound (full surface).  These are stamped by the
     * orchestrator from width/height — see orchestrator docstring. */
    o.metric_b8 = 640;
    o.metric_bc = 480;

    zdd_object_attach_clipper(&o);

    /* CreateClipper ran first (no prior com_back to release). */
    T_ASSERT_EQ_I(g_dd_create_clipper_calls, 1);
    T_ASSERT_EQ_P(g_dd_create_clipper_parent, &parent);
    /* com_back now holds the stub's fake clipper handle. */
    T_ASSERT_EQ_P(o.com_back, (void *)(uintptr_t)0xc11ccccc);

    /* Then SetClipList on the new clipper, with a single-rect RGNDATA
     * sized to the surface. */
    T_ASSERT_EQ_I(g_dd_setcliplist_calls, 1);
    T_ASSERT_EQ_P(g_dd_setcliplist_last, (void *)(uintptr_t)0xc11ccccc);
    T_ASSERT_EQ_U(g_dd_setcliplist_last_w, 640u);
    T_ASSERT_EQ_U(g_dd_setcliplist_last_h, 480u);

    /* Then SetClipper on the primary surface. */
    T_ASSERT_EQ_I(g_dd_setclipper_calls, 1);
    T_ASSERT_EQ_P(g_dd_setclipper_last_surf, (void *)(uintptr_t)0x5117ace);
    T_ASSERT_EQ_P(g_dd_setclipper_last_clip, (void *)(uintptr_t)0xc11ccccc);

    /* Sequence: create, then list, then attach. */
    T_ASSERT(g_dd_create_clipper_seq < g_dd_setcliplist_seq);
    T_ASSERT(g_dd_setcliplist_seq    < g_dd_setclipper_seq);
    return 0;
}

int test_zdd_object_attach_clipper_releases_existing_com_back(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    parent.ddraw7 = (void *)(uintptr_t)0xdd7;
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0x5117ace;
    /* Pre-existing com_back (could be a leftover back-buffer or prior
     * clipper).  Bump live-COM counter so we can verify the release. */
    o.com_back = (void *)(uintptr_t)0xfeed;
    g_live_coms = 1;

    zdd_object_attach_clipper(&o);

    /* zdd_com_release ran on the old com_back (counter went 1 -> 0)
     * BEFORE CreateClipper stamped the new handle (which bumped it
     * back to 1). */
    T_ASSERT_EQ_I(g_live_coms, 1);
    T_ASSERT_EQ_P(o.com_back, (void *)(uintptr_t)0xc11ccccc);
    T_ASSERT_EQ_I(g_dd_create_clipper_calls, 1);
    return 0;
}

int test_zdd_object_attach_clipper_create_fail_skips_followups(void)
{
    reset_stubs();
    g_dd_create_clipper_handle = NULL;  /* simulate CreateClipper fail */
    zdd parent; zdd_ctor(&parent);
    parent.ddraw7 = (void *)(uintptr_t)0xdd7;
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0x5117ace;

    zdd_object_attach_clipper(&o);

    /* CreateClipper ran but returned NULL.  Our defensive null-checks
     * skip both follow-up calls.  This is more defensive than retail
     * (which would deref a null vtable and crash) — see the port
     * comment in zdd.c. */
    T_ASSERT_EQ_I(g_dd_create_clipper_calls, 1);
    T_ASSERT_EQ_P(o.com_back, NULL);
    T_ASSERT_EQ_I(g_dd_setcliplist_calls, 0);
    T_ASSERT_EQ_I(g_dd_setclipper_calls,  0);
    return 0;
}

int test_zdd_object_attach_clipper_no_primary_skips_setclipper(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    parent.ddraw7 = (void *)(uintptr_t)0xdd7;
    zdd_object o; zdd_object_ctor(&o, &parent);
    /* com_primary stays NULL — bizarre but possible if attach_clipper
     * runs out of expected order. */

    zdd_object_attach_clipper(&o);

    /* Clipper still created + clip-list cleared, but the SetClipper
     * attach is skipped. */
    T_ASSERT_EQ_I(g_dd_create_clipper_calls, 1);
    T_ASSERT_EQ_I(g_dd_setcliplist_calls,    1);
    T_ASSERT_EQ_I(g_dd_setclipper_calls,     0);
    return 0;
}

int test_zdd_object_new_setkey_failure_cleans_up(void)
{
    /* CreateSurface succeeded but SetColorKey failed → orchestrator
     * returns 0 → wrapper tears down even though the surface DID
     * get allocated.  This matches retail's "all or nothing" stance:
     * if any leg of the pair-init fails, the whole ZDDObject is
     * discarded. */
    reset_stubs();
    g_dd_setkey_result = 0;
    g_dd_setkey_hr     = (int32_t)0x88760482;
    zdd parent; zdd_ctor(&parent);

    zdd_object *o = NULL;
    int rc = zdd_object_new(&parent, &o, 64, 64,
                            /*real colorkey*/ 0xabc, 1);

    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT(o == NULL);
    T_ASSERT_EQ_I(parent.open_objects, 0);
    /* zdd_object_dtor ran its release-children pass — both com_back
     * and com_primary went through zdd_com_release.  com_back was
     * never set so that's a no-op; com_primary was stamped by the
     * CreateSurface stub but the host stub doesn't track refcounts
     * for fake surfaces, so we can only assert "dtor reached without
     * crashing" via open_objects == 0. */
    return 0;
}

/* ─── hide cursor (FUN_005b8dd0) ─────────────────────────────────── */

int test_zdd_hide_cursor_hides_when_shown(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);  /* cursor_state = 1 (shown) */
    zdd_hide_cursor(&s);
    T_ASSERT_EQ_I(g_cursor_calls, 1);
    T_ASSERT_EQ_I(g_cursor_last_show, 0);  /* ShowCursor(FALSE) */
    T_ASSERT_EQ_I(s.cursor_state, 0);
    return 0;
}

int test_zdd_hide_cursor_noop_when_already_hidden(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.cursor_state = 0;        /* already hidden */
    zdd_hide_cursor(&s);
    T_ASSERT_EQ_I(g_cursor_calls, 0);
    T_ASSERT_EQ_I(s.cursor_state, 0);
    return 0;
}

int test_zdd_hide_then_restore_round_trip(void)
{
    /* Verify the two cursor helpers compose correctly — FUN_00582e90
     * hides, then later FUN_005b7fe0 (dtor) restores. */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_hide_cursor(&s);
    T_ASSERT_EQ_I(s.cursor_state, 0);
    zdd_restore_cursor_on_release(&s);
    T_ASSERT_EQ_I(s.cursor_state, 1);
    /* Two cursor calls total — one hide, one show. */
    T_ASSERT_EQ_I(g_cursor_calls, 2);
    T_ASSERT_EQ_I(g_cursor_last_show, 1);  /* last call was show(TRUE) */
    return 0;
}

/* ─── SetDisplayMode (FUN_005b8900) ──────────────────────────────── */

int test_zdd_set_display_mode_passes_args_and_returns_one(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    int rc = zdd_set_display_mode(&s, 640, 480, 16, 0);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_setmode_calls, 1);
    T_ASSERT_EQ_U(g_dd_setmode_last_w, 640u);
    T_ASSERT_EQ_U(g_dd_setmode_last_h, 480u);
    T_ASSERT_EQ_U(g_dd_setmode_last_bpp, 16u);
    T_ASSERT_EQ_U(g_dd_setmode_last_refresh, 0u);
    return 0;
}

int test_zdd_set_display_mode_failure_logs_dderr(void)
{
    reset_stubs();
    g_dd_setmode_result = 0;
    g_dd_setmode_hr     = (int32_t)0x8876024e;  /* UNSUPPORTEDMODE */
    zdd s; zdd_ctor(&s);

    int rc = zdd_set_display_mode(&s, 9999, 9999, 99, 0);
    T_ASSERT_EQ_I(rc, 0);
    /* DDERR-decorated log present in log_buf (the host stub calls
     * zdd_log_dderr which scribbles into log_buf). */
    T_ASSERT(strstr(s.log_buf, "SetDisplayMode") != NULL);
    T_ASSERT(strstr(s.log_buf, "DDERR_UNSUPPORTEDMODE") != NULL);
    return 0;
}

/* ─── GetDisplayMode (FUN_005b8950) ──────────────────────────────── */

int test_zdd_get_display_mode_fills_all_out_ptrs(void)
{
    reset_stubs();
    g_dd_getmode_width  = 1920;
    g_dd_getmode_height = 1080;
    g_dd_getmode_pitch  = 7680;
    zdd s; zdd_ctor(&s);

    uint32_t w = 0, h = 0, p = 0;
    int rc = zdd_get_display_mode(&s, &w, &h, &p);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_getmode_calls, 1);
    T_ASSERT_EQ_U(w, 1920u);
    T_ASSERT_EQ_U(h, 1080u);
    T_ASSERT_EQ_U(p, 7680u);
    return 0;
}

int test_zdd_get_display_mode_tolerates_null_out_ptrs(void)
{
    /* FUN_00582e90 mode 4 calls FUN_005b8950(&w, &h, NULL) — pitch
     * is dropped on the floor.  The wrapper must accept any NULL
     * slot without dereffing. */
    reset_stubs();
    zdd s; zdd_ctor(&s);

    uint32_t w = 0, h = 0;
    int rc = zdd_get_display_mode(&s, &w, &h, NULL);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_U(w, 1024u);  /* defaults from reset_stubs */
    T_ASSERT_EQ_U(h, 768u);

    /* All-NULL — silently succeeds. */
    rc = zdd_get_display_mode(&s, NULL, NULL, NULL);
    T_ASSERT_EQ_I(rc, 1);
    return 0;
}

int test_zdd_get_display_mode_failure_leaves_outs_untouched(void)
{
    reset_stubs();
    g_dd_getmode_result = 0;
    zdd s; zdd_ctor(&s);

    uint32_t w = 0xdeadbeef, h = 0xcafef00d, p = 0xfeedface;
    int rc = zdd_get_display_mode(&s, &w, &h, &p);
    T_ASSERT_EQ_I(rc, 0);
    /* Out-args must NOT be overwritten on failure — retail's caller
     * relies on this when falling through to the override path. */
    T_ASSERT_EQ_U(w, 0xdeadbeefu);
    T_ASSERT_EQ_U(h, 0xcafef00du);
    T_ASSERT_EQ_U(p, 0xfeedfaceu);
    return 0;
}

/* ─── busy-wait (FUN_005b5ac0) ───────────────────────────────────── */

int test_zdd_busy_wait_ms_records_arg(void)
{
    reset_stubs();
    /* The host stub never actually sleeps — just records the argument
     * so callers (FUN_00582e90 always pauses 2000 ms) are testable. */
    zdd_busy_wait_ms(2000);
    T_ASSERT_EQ_I(g_busy_wait_calls, 1);
    T_ASSERT_EQ_U(g_busy_wait_last_ms, 2000u);

    zdd_busy_wait_ms(50);
    T_ASSERT_EQ_I(g_busy_wait_calls, 2);
    T_ASSERT_EQ_U(g_busy_wait_last_ms, 50u);
    return 0;
}

/* ─── attach_backbuffer (FUN_005b9740) ───────────────────────────── */

int test_zdd_object_attach_backbuffer_happy_path(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    void *primary = (void *)(uintptr_t)0x5117ace;

    int rc = zdd_object_attach_backbuffer(&o, primary, 640, 480, 0);
    T_ASSERT_EQ_I(rc, 1);

    /* prefill_desc(0, 0) ran — caps_in / force_videomem_in both zero
     * on the ZDDObject (back-buffer attachments don't go through
     * CreateSurface). */
    T_ASSERT_EQ_I(o.caps_in,           0);
    T_ASSERT_EQ_I(o.force_videomem_in, 0);
    /* DDSD self-pointers stamped (prefill_desc side effect). */
    T_ASSERT_EQ_P(o.ddsd_lpSurface_ref, &o.embedded_ddsd[0x24]);

    /* GetAttachedSurface called once with DDSCAPS_BACKBUFFER and the
     * primary we passed. */
    T_ASSERT_EQ_I(g_dd_attached_calls, 1);
    T_ASSERT_EQ_P(g_dd_attached_last_primary, primary);
    T_ASSERT_EQ_U(g_dd_attached_last_caps, 0x004u);

    /* Attached surface stashed in com_primary. */
    T_ASSERT_EQ_P(o.com_primary, (void *)(uintptr_t)0xbbbbbbbb);

    /* metrics stamped with (w, h, 0, 0, w, h) — see FUN_005b98c0's
     * unusual slot-to-param mapping in zdd.h. */
    T_ASSERT_EQ_I(o.metric_0c, 0);     /* p3 = 0 */
    T_ASSERT_EQ_I(o.metric_10, 0);     /* p4 = 0 */
    T_ASSERT_EQ_I(o.metric_14, 640);   /* p5 = width */
    T_ASSERT_EQ_I(o.metric_18, 480);   /* p6 = height */
    T_ASSERT_EQ_I(o.metric_1c, 640);   /* p1 = width */
    T_ASSERT_EQ_I(o.metric_20, 480);   /* p2 = height */
    T_ASSERT_EQ_I(o.metric_b0, 0);
    T_ASSERT_EQ_I(o.metric_b4, 0);
    T_ASSERT_EQ_I(o.metric_b8, 640);   /* p5 mirror */
    T_ASSERT_EQ_I(o.metric_bc, 480);   /* p6 mirror */

    /* Sentinel color-key — no SetColorKey vtable call; state_flag = 0. */
    T_ASSERT_EQ_I(g_dd_setkey_calls, 0);
    T_ASSERT_EQ_I((int)o.colorkey_in,  0x1ffffff);
    T_ASSERT_EQ_I((int)o.colorkey_out, 0x1ffffff);
    T_ASSERT_EQ_U(o.state_flag, 0u);
    return 0;
}

int test_zdd_object_attach_backbuffer_force_videomem_sets_caps(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    int rc = zdd_object_attach_backbuffer(&o, (void *)(uintptr_t)0x123,
                                          800, 600, 1);
    T_ASSERT_EQ_I(rc, 1);
    /* DDSCAPS_BACKBUFFER (4) | DDSCAPS_VIDEOMEMORY (0x800) = 0x804. */
    T_ASSERT_EQ_U(g_dd_attached_last_caps, 0x804u);
    return 0;
}

/* ─── primary-surface descriptor builder (FUN_005b8480 lines 56-86) */

int test_zdd_primary_desc_mode_full_no_vram(void)
{
    zdd_primary_desc_build d;
    zdd_build_primary_surface_desc(0, 0, &d);
    /* dwFlags = DDSD_CAPS|DDSD_BACKBUFFERCOUNT = 0x21 */
    T_ASSERT_EQ_U(d.dwFlags, 0x21u);
    /* dwCaps = PRIMARYSURFACE|FLIP|COMPLEX = 0x218 */
    T_ASSERT_EQ_U(d.dwCaps, 0x218u);
    T_ASSERT_EQ_U(d.dwBackBufferCount, 1u);
    return 0;
}

int test_zdd_primary_desc_mode_full_with_vram(void)
{
    zdd_primary_desc_build d;
    zdd_build_primary_surface_desc(0, 1, &d);
    /* +DDSCAPS_VIDEOMEMORY (0x800) → 0xa18 */
    T_ASSERT_EQ_U(d.dwCaps, 0xa18u);
    T_ASSERT_EQ_U(d.dwBackBufferCount, 1u);
    return 0;
}

int test_zdd_primary_desc_mode_safe(void)
{
    zdd_primary_desc_build d;
    zdd_build_primary_surface_desc(1, 1, &d);
    /* dwFlags = DDSD_CAPS alone (no backbuffer flag) */
    T_ASSERT_EQ_U(d.dwFlags, 0x01u);
    /* dwCaps = PRIMARYSURFACE alone */
    T_ASSERT_EQ_U(d.dwCaps, 0x200u);
    T_ASSERT_EQ_U(d.dwBackBufferCount, 0u);
    return 0;
}

int test_zdd_primary_desc_mode_db_ignores_vram(void)
{
    /* Modes 3/4 don't fold videomem_flag into the primary caps —
     * videomem is honoured at the per-ZDDObject orchestrator layer
     * in those modes. */
    zdd_primary_desc_build d;
    zdd_build_primary_surface_desc(3, 1, &d);
    T_ASSERT_EQ_U(d.dwFlags, 0x21u);
    T_ASSERT_EQ_U(d.dwCaps, 0x218u);   /* no |VIDEOMEMORY */
    T_ASSERT_EQ_U(d.dwBackBufferCount, 1u);
    return 0;
}

int test_zdd_primary_desc_mode_zoom(void)
{
    zdd_primary_desc_build d;
    zdd_build_primary_surface_desc(4, 0, &d);
    T_ASSERT_EQ_U(d.dwFlags, 0x21u);
    T_ASSERT_EQ_U(d.dwCaps, 0x218u);
    T_ASSERT_EQ_U(d.dwBackBufferCount, 1u);
    return 0;
}

/* ─── create_screen (FUN_005b8480) ───────────────────────────────── */

int test_zdd_create_screen_mode_full_happy_path(void)
{
    /* Boot path: 640×480, 16bpp, mode 0, no force-VRAM. */
    reset_stubs();
    zdd s; zdd_ctor(&s);

    int rc = zdd_create_screen(&s, 640, 480, 16, 0, 0, NULL);
    T_ASSERT_EQ_I(rc, 1);

    /* Params stamped on the ZDD. */
    T_ASSERT_EQ_I(s.videomem_flag, 0);
    T_ASSERT_EQ_I(s.screen_width,  640);
    T_ASSERT_EQ_I(s.screen_height, 480);
    T_ASSERT_EQ_I(s.pixel_format_mode, 0);
    T_ASSERT_EQ_I(s.pixel_format_bpp,  16);
    T_ASSERT_EQ_I(s.screen_pos_x, 0);
    T_ASSERT_EQ_I(s.screen_pos_y, 0);
    /* rect_blob zeroed (no opt_rect7 passed). */
    for (int i = 0; i < 7; i++) T_ASSERT_EQ_I(s.screen_rect[i], 0);

    /* Primary surface CreateSurface happened with the Full-no-VRAM shape. */
    T_ASSERT_EQ_I(g_dd_create_primary_calls, 1);
    T_ASSERT_EQ_U(g_dd_create_primary_last_flags, 0x21u);
    T_ASSERT_EQ_U(g_dd_create_primary_last_caps,  0x218u);
    T_ASSERT_EQ_U(g_dd_create_primary_last_count, 1u);
    T_ASSERT_EQ_P(s.com_a, (void *)(uintptr_t)0xa1a1a1a1);

    /* 8bpp palette NOT created (bpp != 8). */
    T_ASSERT_EQ_I(g_dd_create_pal_calls, 0);

    /* primary_obj allocated, back_obj_a/b stayed NULL. */
    T_ASSERT(s.primary_obj != NULL);
    T_ASSERT_EQ_P(s.back_obj_a, NULL);
    T_ASSERT_EQ_P(s.back_obj_b, NULL);

    /* Mode 0 → attach_backbuffer was called on primary_obj.  The
     * stub's GetAttachedSurface published 0xbbbbbbbb into com_primary. */
    T_ASSERT_EQ_I(g_dd_attached_calls, 1);
    T_ASSERT_EQ_P(g_dd_attached_last_primary, s.com_a);
    /* mode 0 with videomem_flag=0 → caps = DDSCAPS_BACKBUFFER alone */
    T_ASSERT_EQ_U(g_dd_attached_last_caps, 0x004u);
    T_ASSERT_EQ_P(s.primary_obj->com_primary, (void *)(uintptr_t)0xbbbbbbbb);

    /* Clipper attached to primary_obj. */
    T_ASSERT_EQ_I(g_dd_create_clipper_calls, 1);
    T_ASSERT_EQ_I(g_dd_setclipper_calls,     1);
    /* primary_obj.com_primary is what got the clipper bound. */
    T_ASSERT_EQ_P(g_dd_setclipper_last_surf, s.primary_obj->com_primary);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_mode_full_with_videomem(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);

    int rc = zdd_create_screen(&s, 640, 480, 16, 0, 1, NULL);
    T_ASSERT_EQ_I(rc, 1);
    /* Full mode + videomem_flag=1 → primary caps include VRAM. */
    T_ASSERT_EQ_U(g_dd_create_primary_last_caps, 0xa18u);
    /* And the back-buffer attach inherits force_videomem too. */
    T_ASSERT_EQ_U(g_dd_attached_last_caps, 0x804u);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_mode_safe(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);

    int rc = zdd_create_screen(&s, 640, 480, 16, 1, 1, NULL);
    T_ASSERT_EQ_I(rc, 1);
    /* Safe mode → non-flippable primary. */
    T_ASSERT_EQ_U(g_dd_create_primary_last_flags, 0x01u);
    T_ASSERT_EQ_U(g_dd_create_primary_last_caps,  0x200u);
    T_ASSERT_EQ_U(g_dd_create_primary_last_count, 0u);

    /* Safe mode → orchestrator-create (no back-buffer attach). */
    T_ASSERT_EQ_I(g_dd_attached_calls, 0);
    /* create_surface_pair ran on primary_obj — fake CreateSurface
     * stamped its handle into primary_obj->com_primary. */
    T_ASSERT_EQ_P(s.primary_obj->com_primary, (void *)(uintptr_t)0x99887766);
    /* Clipper still attached. */
    T_ASSERT_EQ_I(g_dd_setclipper_calls, 1);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_mode_windowed_skips_primary_create(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Pre-stage a com_a as if from a prior session — verify it's
     * released. */
    s.com_a = (void *)(uintptr_t)0xfeedface;
    g_live_coms = 1;

    int rc = zdd_create_screen(&s, 640, 480, 16, 2, 0, NULL);
    T_ASSERT_EQ_I(rc, 1);
    /* CreateSurface for the primary was NOT called. */
    T_ASSERT_EQ_I(g_dd_create_primary_calls, 0);
    /* The prior com_a was released (the slot is now NULL).  The
     * g_live_coms counter is polluted by the post-init clipper bump,
     * so we just verify the slot was cleared. */
    T_ASSERT_EQ_P(s.com_a, NULL);
    /* Windowed mode → orchestrator-create on primary_obj (no back-buffer attach). */
    T_ASSERT_EQ_I(g_dd_attached_calls, 0);
    T_ASSERT(s.primary_obj != NULL);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_mode_db(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);

    int rc = zdd_create_screen(&s, 640, 480, 16, 3, 1, NULL);
    T_ASSERT_EQ_I(rc, 1);
    /* DB Mode → primary caps = 0x218 (no VRAM fold even though
     * videomem_flag=1; modes 3/4 ignore videomem at the primary). */
    T_ASSERT_EQ_U(g_dd_create_primary_last_caps, 0x218u);

    /* Both primary_obj AND back_obj_a allocated. */
    T_ASSERT(s.primary_obj != NULL);
    T_ASSERT(s.back_obj_a != NULL);
    T_ASSERT_EQ_P(s.back_obj_b, NULL);

    /* Back-buffer attach ran on back_obj_a (caps = DDSCAPS_BACKBUFFER
     * with NO videomem — mode 3 hardcodes force_vm=0 for the attach). */
    T_ASSERT_EQ_I(g_dd_attached_calls, 1);
    T_ASSERT_EQ_U(g_dd_attached_last_caps, 0x004u);
    T_ASSERT_EQ_P(s.back_obj_a->com_primary, (void *)(uintptr_t)0xbbbbbbbb);

    /* create_surface_pair ran for primary_obj.  Clipper attached to
     * primary_obj. */
    T_ASSERT_EQ_P(s.primary_obj->com_primary, (void *)(uintptr_t)0x99887766);
    T_ASSERT_EQ_I(g_dd_setclipper_calls, 1);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_mode_zoom(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);

    /* Mode 4 rect layout: {display_w, display_h, 2, centre_x, centre_y,
     * src_w, src_h}.  Test with 1024×768 display, 640×480 source
     * centred at (192, 144). */
    int32_t rect[7] = { 1024, 768, 2, 192, 144, 640, 480 };

    int rc = zdd_create_screen(&s, 640, 480, 16, 4, 1, rect);
    T_ASSERT_EQ_I(rc, 1);

    /* rect blob stamped. */
    T_ASSERT_EQ_I(s.screen_rect[0], 1024);
    T_ASSERT_EQ_I(s.screen_rect[1], 768);
    T_ASSERT_EQ_I(s.screen_rect[5], 640);
    T_ASSERT_EQ_I(s.screen_rect[6], 480);

    /* All three ZDDObject slots populated. */
    T_ASSERT(s.primary_obj != NULL);
    T_ASSERT(s.back_obj_a != NULL);
    T_ASSERT(s.back_obj_b != NULL);

    /* Back-buffer attach ran with display dimensions (rect[0/1]). */
    T_ASSERT_EQ_I(g_dd_attached_calls, 1);
    T_ASSERT_EQ_P(s.back_obj_a->com_primary, (void *)(uintptr_t)0xbbbbbbbb);

    /* Clipper attached to primary_obj. */
    T_ASSERT_EQ_I(g_dd_setclipper_calls, 1);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_8bpp_creates_palette(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);

    int rc = zdd_create_screen(&s, 640, 480, 8, 1, 0, NULL);
    T_ASSERT_EQ_I(rc, 1);
    /* 8bpp branch creates the system palette and binds it to com_a. */
    T_ASSERT_EQ_I(g_dd_create_pal_calls, 1);
    T_ASSERT_EQ_P(s.com_b, (void *)(uintptr_t)0xba1ba1ba);
    /* SetPalette ran on com_a with the new palette. */
    T_ASSERT_EQ_I(g_dd_setpal_calls, 1);
    T_ASSERT_EQ_P(g_dd_setpal_last_surf, s.com_a);
    T_ASSERT_EQ_P(g_dd_setpal_last_pal,  s.com_b);
    zdd_release_children(&s);
    return 0;
}

int test_zdd_create_screen_primary_create_failure(void)
{
    reset_stubs();
    g_dd_create_primary_result = 0;
    g_dd_create_primary_hr     = (int32_t)0x88760086;
    zdd s; zdd_ctor(&s);

    int rc = zdd_create_screen(&s, 640, 480, 16, 0, 0, NULL);
    T_ASSERT_EQ_I(rc, 0);
    /* Failure path: params still stamped, but no ZDDObject allocated,
     * no palette, no back-buffer attach. */
    T_ASSERT_EQ_I(s.screen_width, 640);
    T_ASSERT_EQ_P(s.primary_obj, NULL);
    T_ASSERT_EQ_I(g_dd_create_pal_calls, 0);
    T_ASSERT_EQ_I(g_dd_attached_calls, 0);
    /* DDERR logged. */
    T_ASSERT(strstr(g_log_capture, "CreateSurface") != NULL);
    return 0;
}

int test_zdd_create_screen_releases_prior_state(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Pre-stage all 5 prior slots — verify zdd_release_children
     * runs first. */
    zdd_object primary_prior; zdd_object_ctor(&primary_prior, &s);
    s.primary_obj = (zdd_object *)0x1;  /* dangling — release path skips */
    /* Use the test-controlled zdd_obj_destroy path: live counter. */
    s.primary_obj = NULL;  /* skip the dangling pointer; just test releases */
    s.com_a = (void *)(uintptr_t)0x55aa55aa;
    s.com_b = (void *)(uintptr_t)0xbeefbeef;
    g_live_coms = 2;

    int rc = zdd_create_screen(&s, 640, 480, 16, 0, 0, NULL);
    T_ASSERT_EQ_I(rc, 1);
    /* The 2 prior COM handles got released; the post-init com_a
     * (fresh CreateSurface result) is a single new COM.
     * (g_live_coms transitions: 2 -> 0 -> ... no instrumentation in
     * zdd_create_primary_surface stub, so we just verify com_a is
     * the new handle.) */
    T_ASSERT_EQ_P(s.com_a, (void *)(uintptr_t)0xa1a1a1a1);
    /* The prior com_b was released and never re-stashed (mode 0
     * doesn't allocate a palette). */
    T_ASSERT_EQ_P(s.com_b, NULL);
    (void)primary_prior;
    zdd_release_children(&s);
    return 0;
}

/* ─── 8bpp palette setup (FUN_005b8e00) ──────────────────────────── */

int test_zdd_setup_8bit_palette_happy_path(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* Pre-stage a primary surface on com_a (FUN_005b8480 mode 0
     * branch puts the CreateSurface result here). */
    s.com_a = (void *)(uintptr_t)0xb16d15a;

    int rc = zdd_setup_8bit_palette(&s);
    T_ASSERT_EQ_I(rc, 1);

    /* CreatePalette ran once; com_b now holds the new palette. */
    T_ASSERT_EQ_I(g_dd_create_pal_calls, 1);
    T_ASSERT_EQ_P(s.com_b, (void *)(uintptr_t)0xba1ba1ba);

    /* SetPalette ran once on the primary with the new palette. */
    T_ASSERT_EQ_I(g_dd_setpal_calls, 1);
    T_ASSERT_EQ_P(g_dd_setpal_last_surf, s.com_a);
    T_ASSERT_EQ_P(g_dd_setpal_last_pal,  s.com_b);
    return 0;
}

int test_zdd_setup_8bit_palette_no_primary_skips_set(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    /* com_a stays NULL — retail null-checks before SetPalette. */

    int rc = zdd_setup_8bit_palette(&s);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_create_pal_calls, 1);
    T_ASSERT_EQ_P(s.com_b, (void *)(uintptr_t)0xba1ba1ba);
    /* SetPalette skipped — primary not yet allocated. */
    T_ASSERT_EQ_I(g_dd_setpal_calls, 0);
    return 0;
}

int test_zdd_setup_8bit_palette_createpalette_failure(void)
{
    reset_stubs();
    g_dd_create_pal_result = 0;
    g_dd_create_pal_hr     = (int32_t)0x88760005;
    zdd s; zdd_ctor(&s);
    s.com_a = (void *)(uintptr_t)0xb16d15a;

    int rc = zdd_setup_8bit_palette(&s);
    T_ASSERT_EQ_I(rc, 0);
    /* Failure path: SetPalette never called, com_b stays NULL. */
    T_ASSERT_EQ_I(g_dd_setpal_calls, 0);
    T_ASSERT_EQ_P(s.com_b, NULL);
    /* DDERR logged through zdd_log_dderr. */
    T_ASSERT(strstr(g_log_capture, "CreatePalette") != NULL);
    return 0;
}

int test_zdd_setup_8bit_palette_setpalette_failure(void)
{
    reset_stubs();
    g_dd_setpal_result = 0;
    g_dd_setpal_hr     = (int32_t)0x88760086;
    zdd s; zdd_ctor(&s);
    s.com_a = (void *)(uintptr_t)0xb16d15a;

    int rc = zdd_setup_8bit_palette(&s);
    T_ASSERT_EQ_I(rc, 0);
    /* Palette WAS created — only the bind failed. */
    T_ASSERT_EQ_P(s.com_b, (void *)(uintptr_t)0xba1ba1ba);
    T_ASSERT_EQ_I(g_dd_setpal_calls, 1);
    T_ASSERT(strstr(g_log_capture, "SetPalette") != NULL);
    return 0;
}

int test_zdd_object_attach_backbuffer_failure_returns_zero(void)
{
    reset_stubs();
    g_dd_attached_result = 0;
    g_dd_attached_hr     = (int32_t)0x88760086;  /* arbitrary DDERR */
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);

    int rc = zdd_object_attach_backbuffer(&o, (void *)(uintptr_t)0x9,
                                          640, 480, 0);
    T_ASSERT_EQ_I(rc, 0);

    /* prefill_desc still ran (zero-init of DDSD) — verify by checking
     * the self-pointer stamp. */
    T_ASSERT_EQ_P(o.ddsd_lpSurface_ref, &o.embedded_ddsd[0x24]);

    /* GetAttachedSurface logged via the parent ZDD's log_buf. */
    T_ASSERT_EQ_I(g_dd_attached_calls, 1);
    T_ASSERT(g_log_call_count >= 1);
    T_ASSERT(strstr(g_log_capture, "GetAttachedSurface") != NULL);

    /* No metrics stamp, no color-key call on failure path. */
    T_ASSERT_EQ_I(o.metric_14, 0);
    T_ASSERT_EQ_I(g_dd_setkey_calls, 0);
    T_ASSERT_EQ_P(o.com_primary, NULL);
    return 0;
}

/* ─── present dispatcher: FUN_005b94e0 / _9500 / _8fc0 / _9a40 ───── */

int test_zdd_object_get_dc_null_self_returns_zero(void)
{
    reset_stubs();
    void *hdc = (void *)(uintptr_t)0xdead;
    int rc = zdd_object_get_dc(NULL, &hdc);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_getdc_calls, 0);
    return 0;
}

int test_zdd_object_get_dc_null_com_primary_returns_zero(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    /* com_primary stays NULL from ctor */
    void *hdc = (void *)(uintptr_t)0xdead;
    int rc = zdd_object_get_dc(&o, &hdc);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_getdc_calls, 0);
    /* Retail does NOT overwrite *out_hdc on the early-return — verify. */
    T_ASSERT_EQ_P(hdc, (void *)(uintptr_t)0xdead);
    return 0;
}

int test_zdd_object_get_dc_forwards_and_returns_one(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0xface;
    void *hdc = NULL;
    int rc = zdd_object_get_dc(&o, &hdc);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_getdc_calls, 1);
    T_ASSERT_EQ_P(g_dd_getdc_last_surf, (void *)(uintptr_t)0xface);
    T_ASSERT_EQ_P(hdc, g_dd_getdc_handle);
    return 0;
}

int test_zdd_object_get_dc_returns_one_even_when_primitive_fails(void)
{
    /* Retail's `mov eax, 1` after the call discards the COM HRESULT.
     * On primitive failure, *out_hdc is NULL but rc is still 1. */
    reset_stubs();
    g_dd_getdc_result = 0;
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0xface;
    void *hdc = (void *)(uintptr_t)0x42;
    int rc = zdd_object_get_dc(&o, &hdc);
    T_ASSERT_EQ_I(rc, 1);                    /* faithful-mirror quirk */
    T_ASSERT_EQ_I(g_dd_getdc_calls, 1);
    T_ASSERT_EQ_P(hdc, NULL);                /* primitive zeroed it */
    return 0;
}

int test_zdd_object_release_dc_forwards(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object o; zdd_object_ctor(&o, &parent);
    o.com_primary = (void *)(uintptr_t)0xface;
    void *hdc = (void *)(uintptr_t)0xdc12;
    zdd_object_release_dc(&o, hdc);
    T_ASSERT_EQ_I(g_dd_releasedc_calls, 1);
    T_ASSERT_EQ_P(g_dd_releasedc_last_surf, (void *)(uintptr_t)0xface);
    T_ASSERT_EQ_P(g_dd_releasedc_last_hdc,  (void *)(uintptr_t)0xdc12);
    return 0;
}

int test_zdd_object_release_dc_null_self_noop(void)
{
    reset_stubs();
    zdd_object_release_dc(NULL, (void *)(uintptr_t)0x1);
    T_ASSERT_EQ_I(g_dd_releasedc_calls, 0);
    return 0;
}

int test_zdd_object_blt_onto_null_self_degenerate_success(void)
{
    /* Retail's literal: NULL self->com_primary → return 1 without
     * touching anything else. */
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object src; zdd_object_ctor(&src, &parent);
    /* src.com_primary stays NULL */
    zdd_object dst; zdd_object_ctor(&dst, &parent);
    dst.com_primary = (void *)(uintptr_t)0xddd;

    int rc = zdd_object_blt_onto(&src, &dst, 10, 20);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    return 0;
}

int test_zdd_object_blt_onto_null_dest_returns_zero(void)
{
    /* Defensive non-retail-faithful branch (retail crashes) — we
     * prefer 0 over a NULL-deref. */
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object src; zdd_object_ctor(&src, &parent);
    src.com_primary = (void *)(uintptr_t)0x111;

    int rc = zdd_object_blt_onto(&src, NULL, 0, 0);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    return 0;
}

int test_zdd_object_blt_onto_builds_rects_and_forwards(void)
{
    reset_stubs();
    zdd parent; zdd_ctor(&parent);
    zdd_object src; zdd_object_ctor(&src, &parent);
    src.com_primary = (void *)(uintptr_t)0x111;
    src.metric_b0 = 0; src.metric_b4 = 0;
    src.metric_b8 = 640; src.metric_bc = 480;
    src.state_flag = 0x8000;   /* observable distinguisher for flags */

    zdd_object dst; zdd_object_ctor(&dst, &parent);
    dst.com_primary = (void *)(uintptr_t)0x222;

    int rc = zdd_object_blt_onto(&src, &dst, 50, 60);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_blt_calls, 1);
    /* Dest is the receiving surface (dst->com_primary). */
    T_ASSERT_EQ_P(g_dd_blt_last_dest, (void *)(uintptr_t)0x222);
    /* Src is the SOURCE surface (src->com_primary). */
    T_ASSERT_EQ_P(g_dd_blt_last_src,  (void *)(uintptr_t)0x111);
    /* dest_rect = (dx, dy, dx+w, dy+h). */
    T_ASSERT(g_dd_blt_last_has_dest_rect);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[0], 50);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[1], 60);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[2], 50 + 640);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[3], 60 + 480);
    /* src_rect = (0, 0, w, h) from self->metric_b0..bc. */
    T_ASSERT(g_dd_blt_last_has_src_rect);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[0], 0);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[1], 0);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[2], 640);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[3], 480);
    /* flags = state_flag | DDBLT_WAIT (0x1000000) */
    T_ASSERT_EQ_U(g_dd_blt_last_flags, 0x8000u | 0x1000000u);
    return 0;
}

int test_zdd_present_null_self_noop(void)
{
    reset_stubs();
    zdd_present(NULL);
    T_ASSERT_EQ_I(g_dd_flip_calls, 0);
    T_ASSERT_EQ_I(g_dd_blt_calls,  0);
    T_ASSERT_EQ_I(g_dd_desktop_calls, 0);
    return 0;
}

int test_zdd_present_mode_0_full_flips_com_a(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 0;
    s.com_a = (void *)(uintptr_t)0xaaa1;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    zdd_present(&s);

    T_ASSERT_EQ_I(g_dd_flip_calls, 1);
    T_ASSERT_EQ_P(g_dd_flip_last_surf,   (void *)(uintptr_t)0xaaa1);
    T_ASSERT_EQ_P(g_dd_flip_last_target, (void *)(uintptr_t)0xbbb1);
    T_ASSERT_EQ_U(g_dd_flip_last_flags,  1u);   /* DDFLIP_WAIT */
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    T_ASSERT_EQ_I(g_dd_desktop_calls, 0);

    /* Drop the borrowed primary_obj ref so the dtor doesn't try to
     * free a stack ZDDObject. */
    s.primary_obj = NULL;
    return 0;
}

int test_zdd_present_mode_0_skips_when_com_a_null(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 0;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;
    /* s.com_a stays NULL */

    zdd_present(&s);

    T_ASSERT_EQ_I(g_dd_flip_calls, 0);
    s.primary_obj = NULL;
    return 0;
}

int test_zdd_present_mode_1_safe_blts_full_rect(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 1;
    s.com_a = (void *)(uintptr_t)0xaaa1;
    s.screen_pos_x = 0;  s.screen_pos_y = 0;
    s.screen_width = 640; s.screen_height = 480;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    zdd_present(&s);

    T_ASSERT_EQ_I(g_dd_blt_calls, 1);
    T_ASSERT_EQ_P(g_dd_blt_last_dest, (void *)(uintptr_t)0xaaa1);
    T_ASSERT_EQ_P(g_dd_blt_last_src,  (void *)(uintptr_t)0xbbb1);
    T_ASSERT(g_dd_blt_last_has_dest_rect);
    T_ASSERT(g_dd_blt_last_has_src_rect);
    /* Both rects share &self->screen_pos_x. */
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[0], 0);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[1], 0);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[2], 640);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[3], 480);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[0],  0);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[3],  480);
    T_ASSERT_EQ_U(g_dd_blt_last_flags, 0x1000000u);  /* DDBLT_WAIT */
    T_ASSERT_EQ_I(g_dd_flip_calls, 0);

    s.primary_obj = NULL;
    return 0;
}

int test_zdd_present_mode_2_windowed_get_blit_release(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.screen_pos_x = 100; s.screen_pos_y = 200;
    s.screen_width = 640; s.screen_height = 480;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    zdd_present(&s);

    /* get_dc fired on primary_obj's com_primary. */
    T_ASSERT_EQ_I(g_dd_getdc_calls, 1);
    T_ASSERT_EQ_P(g_dd_getdc_last_surf, (void *)(uintptr_t)0xbbb1);
    /* desktop_present got the HDC + screen position + size. */
    T_ASSERT_EQ_I(g_dd_desktop_calls, 1);
    T_ASSERT_EQ_P(g_dd_desktop_last_src, g_dd_getdc_handle);
    T_ASSERT_EQ_I(g_dd_desktop_last_x, 100);
    T_ASSERT_EQ_I(g_dd_desktop_last_y, 200);
    T_ASSERT_EQ_I(g_dd_desktop_last_w, 640);
    T_ASSERT_EQ_I(g_dd_desktop_last_h, 480);
    /* release_dc fired with the matching surf + hdc. */
    T_ASSERT_EQ_I(g_dd_releasedc_calls, 1);
    T_ASSERT_EQ_P(g_dd_releasedc_last_surf, (void *)(uintptr_t)0xbbb1);
    T_ASSERT_EQ_P(g_dd_releasedc_last_hdc,  g_dd_getdc_handle);
    /* No Blt/Flip in the windowed path. */
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    T_ASSERT_EQ_I(g_dd_flip_calls, 0);

    s.primary_obj = NULL;
    return 0;
}

int test_zdd_present_mode_3_db_blt_then_flip(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 3;
    s.com_a = (void *)(uintptr_t)0xaaa1;
    s.screen_pos_x = 0; s.screen_pos_y = 0;
    s.screen_width = 640; s.screen_height = 480;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;
    zdd_object ba; zdd_object_ctor(&ba, &s);
    ba.com_primary = (void *)(uintptr_t)0xccc1;
    s.back_obj_a = &ba;

    zdd_present(&s);

    /* Blt: back_obj_a's com_primary <- primary_obj's com_primary. */
    T_ASSERT_EQ_I(g_dd_blt_calls, 1);
    T_ASSERT_EQ_P(g_dd_blt_last_dest, (void *)(uintptr_t)0xccc1);
    T_ASSERT_EQ_P(g_dd_blt_last_src,  (void *)(uintptr_t)0xbbb1);
    T_ASSERT_EQ_U(g_dd_blt_last_flags, 0x1000000u);
    /* Flip(com_a, back_obj_a->com_primary). */
    T_ASSERT_EQ_I(g_dd_flip_calls, 1);
    T_ASSERT_EQ_P(g_dd_flip_last_surf,   (void *)(uintptr_t)0xaaa1);
    T_ASSERT_EQ_P(g_dd_flip_last_target, (void *)(uintptr_t)0xccc1);

    s.primary_obj = NULL;
    s.back_obj_a  = NULL;
    return 0;
}

int test_zdd_present_mode_4_zoom_blt_then_flip(void)
{
    /* Scaler (FUN_005b8ea0) is unported — we should NOT see any extra
     * Blt for the upscale stage.  The dispatcher just does the
     * blit_onto + Flip stages. */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 4;
    s.com_a = (void *)(uintptr_t)0xaaa1;
    s.screen_rect[3] = 320;   /* centre_x */
    s.screen_rect[4] = 240;   /* centre_y */
    zdd_object ba; zdd_object_ctor(&ba, &s);
    ba.com_primary = (void *)(uintptr_t)0xccc1;
    s.back_obj_a = &ba;
    zdd_object bb; zdd_object_ctor(&bb, &s);
    bb.com_primary = (void *)(uintptr_t)0xddd1;
    bb.metric_b8 = 1280; bb.metric_bc = 960;
    s.back_obj_b = &bb;

    zdd_present(&s);

    /* blit_onto(back_obj_b, back_obj_a, 320, 240) → one Blt. */
    T_ASSERT_EQ_I(g_dd_blt_calls, 1);
    T_ASSERT_EQ_P(g_dd_blt_last_dest, (void *)(uintptr_t)0xccc1);
    T_ASSERT_EQ_P(g_dd_blt_last_src,  (void *)(uintptr_t)0xddd1);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[0], 320);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[1], 240);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[2], 320 + 1280);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[3], 240 + 960);
    /* Flip(com_a, back_obj_a->com_primary). */
    T_ASSERT_EQ_I(g_dd_flip_calls, 1);
    T_ASSERT_EQ_P(g_dd_flip_last_target, (void *)(uintptr_t)0xccc1);

    s.back_obj_a = NULL;
    s.back_obj_b = NULL;
    return 0;
}

int test_zdd_present_default_mode_is_noop(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 7;   /* out of range */
    s.com_a = (void *)(uintptr_t)0xaaa1;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    zdd_present(&s);

    T_ASSERT_EQ_I(g_dd_flip_calls, 0);
    T_ASSERT_EQ_I(g_dd_blt_calls,  0);
    T_ASSERT_EQ_I(g_dd_desktop_calls, 0);

    s.primary_obj = NULL;
    return 0;
}

/* ─── zdd_window_paint (FUN_005b9130 WM_PAINT handler) ─────────────── */

int test_zdd_window_paint_null_self_returns_zero(void)
{
    reset_stubs();
    int rc = zdd_window_paint(NULL, (void *)(uintptr_t)0xfeedface);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_wpaint_begin_calls, 0);
    T_ASSERT_EQ_I(g_dd_wpaint_end_calls,   0);
    T_ASSERT_EQ_I(g_dd_wblit_calls,        0);
    T_ASSERT_EQ_I(g_dd_getdc_calls,        0);
    T_ASSERT_EQ_I(g_dd_releasedc_calls,    0);
    return 0;
}

int test_zdd_window_paint_non_mode2_returns_zero_no_calls(void)
{
    /* Modes 0/1/3/4 must short-circuit at the `cmp [esi+0x164], 2` check
     * (retail 0x5b9136) — no BeginPaint, no GetDC, nothing. */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    int modes[] = {0, 1, 3, 4};
    for (size_t i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
        s.pixel_format_mode = modes[i];
        int rc = zdd_window_paint(&s, (void *)(uintptr_t)0xfeedface);
        T_ASSERT_EQ_I(rc, 0);
    }
    T_ASSERT_EQ_I(g_dd_wpaint_begin_calls, 0);
    T_ASSERT_EQ_I(g_dd_wpaint_end_calls,   0);
    T_ASSERT_EQ_I(g_dd_wblit_calls,        0);
    T_ASSERT_EQ_I(g_dd_getdc_calls,        0);
    T_ASSERT_EQ_I(g_dd_releasedc_calls,    0);

    s.primary_obj = NULL;
    return 0;
}

int test_zdd_window_paint_null_primary_obj_returns_zero(void)
{
    /* Defensive guard not present in retail (retail would crash on the
     * vtable deref inside FUN_005b94e0).  Drop-in bails cleanly without
     * even calling BeginPaint — no point in opening a paint session we
     * can't fulfil. */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.primary_obj = NULL;
    int rc = zdd_window_paint(&s, (void *)(uintptr_t)0xfeedface);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_wpaint_begin_calls, 0);
    return 0;
}

int test_zdd_window_paint_mode2_full_sequence(void)
{
    /* Happy path: mode 2 with a real primary_obj fires all five
     * primitives in retail order (begin → get_dc → blit → release_dc
     * → end), threads the begin's HDC into the blit's dest, threads
     * the begin's cookie into the end, and threads the get_dc's HDC
     * into the blit's src.  Returns 1 (consumed). */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.screen_pos_x = 100; s.screen_pos_y = 200;
    s.screen_width = 640; s.screen_height = 480;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    void *fake_hwnd = (void *)(uintptr_t)0xfeedface;
    int rc = zdd_window_paint(&s, fake_hwnd);
    T_ASSERT_EQ_I(rc, 1);

    /* Each primitive fired exactly once. */
    T_ASSERT_EQ_I(g_dd_wpaint_begin_calls, 1);
    T_ASSERT_EQ_I(g_dd_getdc_calls,        1);
    T_ASSERT_EQ_I(g_dd_wblit_calls,        1);
    T_ASSERT_EQ_I(g_dd_releasedc_calls,    1);
    T_ASSERT_EQ_I(g_dd_wpaint_end_calls,   1);

    /* Retail call order: begin(1) get_dc(2) blit(3) release_dc(4) end(5). */
    T_ASSERT_EQ_I(g_dd_wpaint_begin_seq, 1);
    T_ASSERT_EQ_I(g_dd_getdc_seq,        2);
    T_ASSERT_EQ_I(g_dd_wblit_seq,        3);
    T_ASSERT_EQ_I(g_dd_releasedc_seq,    4);
    T_ASSERT_EQ_I(g_dd_wpaint_end_seq,   5);

    /* Begin/End got the hwnd; End got the same cookie Begin produced. */
    T_ASSERT_EQ_P(g_dd_wpaint_begin_last_hwnd, fake_hwnd);
    T_ASSERT_EQ_P(g_dd_wpaint_end_last_hwnd,   fake_hwnd);
    T_ASSERT_EQ_P(g_dd_wpaint_end_last_cookie, g_dd_wpaint_begin_cookie);

    /* GetDC was called on primary_obj's com_primary (per FUN_005b9158:
     * `mov ecx, [esi + 0x16c]; call FUN_005b94e0`). */
    T_ASSERT_EQ_P(g_dd_getdc_last_surf,     (void *)(uintptr_t)0xbbb1);
    T_ASSERT_EQ_P(g_dd_releasedc_last_surf, (void *)(uintptr_t)0xbbb1);
    T_ASSERT_EQ_P(g_dd_releasedc_last_hdc,  g_dd_getdc_handle);

    /* BitBlt: dest_hdc = begin's return, src_hdc = get_dc's out.
     * Coords/dimensions = screen_pos_x/y/width/height. */
    T_ASSERT_EQ_P(g_dd_wblit_last_dest_hdc, g_dd_wpaint_begin_hdc);
    T_ASSERT_EQ_P(g_dd_wblit_last_src_hdc,  g_dd_getdc_handle);
    T_ASSERT_EQ_I(g_dd_wblit_last_x, 100);
    T_ASSERT_EQ_I(g_dd_wblit_last_y, 200);
    T_ASSERT_EQ_I(g_dd_wblit_last_w, 640);
    T_ASSERT_EQ_I(g_dd_wblit_last_h, 480);

    s.primary_obj = NULL;
    return 0;
}

int test_zdd_window_paint_does_not_touch_present_primitives(void)
{
    /* The WM_PAINT path uses Begin/End/BitBlt and Get/ReleaseDC ONLY.
     * It must NOT fire the per-frame present's primitives (Flip,
     * surface Blt, desktop_present). */
    reset_stubs();
    zdd s; zdd_ctor(&s);
    s.pixel_format_mode = 2;
    s.screen_pos_x = 0; s.screen_pos_y = 0;
    s.screen_width = 640; s.screen_height = 480;
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xbbb1;
    s.primary_obj = &po;

    zdd_window_paint(&s, (void *)(uintptr_t)0xfeedface);

    T_ASSERT_EQ_I(g_dd_flip_calls,    0);
    T_ASSERT_EQ_I(g_dd_blt_calls,     0);
    T_ASSERT_EQ_I(g_dd_desktop_calls, 0);

    s.primary_obj = NULL;
    return 0;
}

/* ─── zdd_object_lock / _unlock / _clear (FUN_005b9490 / _94d0 / _9410) ── */

int test_zdd_object_lock_null_self_returns_zero(void)
{
    reset_stubs();
    T_ASSERT_EQ_I(zdd_object_lock(NULL), 0);
    T_ASSERT_EQ_I(g_dd_lock_calls, 0);
    return 0;
}

int test_zdd_object_lock_null_com_primary_returns_zero(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = NULL;
    T_ASSERT_EQ_I(zdd_object_lock(&po), 0);
    T_ASSERT_EQ_I(g_dd_lock_calls, 0);
    return 0;
}

int test_zdd_object_lock_success_forwards_and_returns_one(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xabcd;

    int rc = zdd_object_lock(&po);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_lock_calls, 1);
    T_ASSERT_EQ_P(g_dd_lock_last_surf, (void *)(uintptr_t)0xabcd);
    T_ASSERT_EQ_P(g_dd_lock_last_ddsd, (void *)&po.embedded_ddsd[0]);
    T_ASSERT_EQ_I((int)g_dd_lock_last_flags, 1);  /* DDLOCK_WAIT */
    /* No DDERR log on success. */
    T_ASSERT_EQ_I(g_log_call_count, 0);
    return 0;
}

int test_zdd_object_lock_failure_logs_and_returns_zero(void)
{
    reset_stubs();
    g_dd_lock_result = 0;
    g_dd_lock_hr     = (int32_t)0x88760000;  /* DDERR_GENERIC-ish */
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xabcd;

    int rc = zdd_object_lock(&po);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_lock_calls, 1);
    /* DDERR routed through log */
    T_ASSERT(g_log_call_count > 0);
    T_ASSERT(strstr(g_log_capture, "DirectDrawSurface") != NULL);
    return 0;
}

int test_zdd_object_unlock_null_self_noop(void)
{
    reset_stubs();
    zdd_object_unlock(NULL);
    T_ASSERT_EQ_I(g_dd_unlock_calls, 0);
    return 0;
}

int test_zdd_object_unlock_forwards_to_primitive(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xdead;
    zdd_object_unlock(&po);
    T_ASSERT_EQ_I(g_dd_unlock_calls, 1);
    T_ASSERT_EQ_P(g_dd_unlock_last_surf, (void *)(uintptr_t)0xdead);
    return 0;
}

int test_zdd_object_clear_null_self_noop(void)
{
    reset_stubs();
    zdd_object_clear(NULL);
    T_ASSERT_EQ_I(g_dd_lock_calls, 0);
    T_ASSERT_EQ_I(g_dd_unlock_calls, 0);
    return 0;
}

int test_zdd_object_clear_lock_failure_skips_fill_and_unlock(void)
{
    reset_stubs();
    g_dd_lock_result = 0;
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xabcd;

    zdd_object_clear(&po);
    T_ASSERT_EQ_I(g_dd_lock_calls,   1);
    T_ASSERT_EQ_I(g_dd_unlock_calls, 0);  /* skipped */
    return 0;
}

int test_zdd_object_clear_zero_fills_and_unlocks(void)
{
    reset_stubs();
    /* Set up a fake surface buffer that Lock will publish into the
     * DDSD.  Clear should zero it, then Unlock. */
    uint8_t buf[32 * 4];
    memset(buf, 0xAA, sizeof(buf));
    g_dd_lock_fill_surface = buf;
    g_dd_lock_fill_pitch   = 32;
    g_dd_lock_fill_height  = 4;
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xabcd;

    zdd_object_clear(&po);

    T_ASSERT_EQ_I(g_dd_lock_calls,   1);
    T_ASSERT_EQ_I(g_dd_unlock_calls, 1);
    /* Lock before Unlock. */
    T_ASSERT(g_dd_lock_seq < g_dd_unlock_seq);
    /* Buffer fully zeroed. */
    for (size_t i = 0; i < sizeof(buf); i++) {
        T_ASSERT_EQ_I(buf[i], 0);
    }
    return 0;
}

int test_zdd_object_clear_zero_size_safe(void)
{
    /* Lock succeeds but pitch * height = 0 — clear should skip the
     * memset (no surf needed) and still unlock. */
    reset_stubs();
    g_dd_lock_fill_surface = NULL;
    g_dd_lock_fill_pitch   = 0;
    g_dd_lock_fill_height  = 0;
    zdd s; zdd_ctor(&s);
    zdd_object po; zdd_object_ctor(&po, &s);
    po.com_primary = (void *)(uintptr_t)0xabcd;

    zdd_object_clear(&po);

    T_ASSERT_EQ_I(g_dd_lock_calls,   1);
    T_ASSERT_EQ_I(g_dd_unlock_calls, 1);
    return 0;
}

/* ─── zdd_bind_pixel_format + zdd_color_convert (FUN_005b8a20 / _8b00) ── */

int test_zdd_bind_pixel_format_null_self_returns_zero(void)
{
    reset_stubs();
    int rc = zdd_bind_pixel_format(NULL, (void *)(uintptr_t)0x1234);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_getdesc_calls, 0);
    return 0;
}

int test_zdd_bind_pixel_format_null_surface_returns_zero(void)
{
    reset_stubs();
    zdd s; zdd_ctor(&s);
    int rc = zdd_bind_pixel_format(&s, NULL);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_getdesc_calls, 0);
    return 0;
}

int test_zdd_bind_pixel_format_getdesc_failure_returns_zero(void)
{
    reset_stubs();
    g_dd_getdesc_result = 0;
    zdd s; zdd_ctor(&s);
    int rc = zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd);
    T_ASSERT_EQ_I(rc, 0);
    /* color_desc must remain untouched. */
    T_ASSERT_EQ_I(s.color_desc.shift_left[0], 0);
    T_ASSERT_EQ_I(s.color_desc.mask_raw[0], 0u);
    return 0;
}

int test_zdd_bind_pixel_format_rgb565_stamps_descriptor(void)
{
    reset_stubs();
    g_dd_getdesc_fill_r_mask = 0xF800;
    g_dd_getdesc_fill_g_mask = 0x07E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
    zdd s; zdd_ctor(&s);
    int rc = zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_getdesc_calls, 1);
    T_ASSERT_EQ_P(g_dd_getdesc_last_surf, (void *)(uintptr_t)0xabcd);
    /* R: mask=0xF800, tz=11, shifted=0x1F, mask_lo=0x1F, shift_right=3 */
    T_ASSERT_EQ_I(s.color_desc.shift_left[0],  11);
    T_ASSERT_EQ_I((int)s.color_desc.mask_raw[0], 0xF800);
    T_ASSERT_EQ_I(s.color_desc.mask_lo[0],     0x1F);
    T_ASSERT_EQ_I(s.color_desc.shift_right[0], 3);
    /* G: mask=0x07E0, tz=5, shifted=0x3F, mask_lo=0x3F, shift_right=2 */
    T_ASSERT_EQ_I(s.color_desc.shift_left[1],  5);
    T_ASSERT_EQ_I((int)s.color_desc.mask_raw[1], 0x07E0);
    T_ASSERT_EQ_I(s.color_desc.mask_lo[1],     0x3F);
    T_ASSERT_EQ_I(s.color_desc.shift_right[1], 2);
    /* B: mask=0x001F, tz=0, shifted=0x1F, mask_lo=0x1F, shift_right=3 */
    T_ASSERT_EQ_I(s.color_desc.shift_left[2],  0);
    T_ASSERT_EQ_I((int)s.color_desc.mask_raw[2], 0x001F);
    T_ASSERT_EQ_I(s.color_desc.mask_lo[2],     0x1F);
    T_ASSERT_EQ_I(s.color_desc.shift_right[2], 3);
    return 0;
}

int test_zdd_bind_pixel_format_rgb555_stamps_descriptor(void)
{
    /* RGB555: R=0x7C00, G=0x03E0, B=0x001F — all three channels 5 bits */
    reset_stubs();
    g_dd_getdesc_fill_r_mask = 0x7C00;
    g_dd_getdesc_fill_g_mask = 0x03E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
    zdd s; zdd_ctor(&s);
    int rc = zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(s.color_desc.shift_left[0],  10);
    T_ASSERT_EQ_I(s.color_desc.shift_right[0], 3);
    T_ASSERT_EQ_I(s.color_desc.mask_lo[0],     0x1F);
    T_ASSERT_EQ_I(s.color_desc.shift_left[1],  5);
    T_ASSERT_EQ_I(s.color_desc.shift_right[1], 3);
    T_ASSERT_EQ_I(s.color_desc.mask_lo[1],     0x1F);
    T_ASSERT_EQ_I(s.color_desc.shift_left[2],  0);
    T_ASSERT_EQ_I(s.color_desc.shift_right[2], 3);
    return 0;
}

int test_zdd_color_convert_null_self_returns_zero(void)
{
    reset_stubs();
    uint32_t out = zdd_color_convert(NULL, 0x00ABCDEF);
    T_ASSERT_EQ_I((int)out, 0);
    return 0;
}

int test_zdd_color_convert_rgb565_white(void)
{
    /* Bind to RGB565 then convert pure white (0xFFFFFF) → 0xFFFF. */
    reset_stubs();
    g_dd_getdesc_fill_r_mask = 0xF800;
    g_dd_getdesc_fill_g_mask = 0x07E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
    zdd s; zdd_ctor(&s);
    T_ASSERT_EQ_I(zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd), 1);
    uint32_t out = zdd_color_convert(&s, 0x00FFFFFF);
    T_ASSERT_EQ_I((int)out, 0xFFFF);
    return 0;
}

int test_zdd_color_convert_rgb565_pure_red(void)
{
    reset_stubs();
    g_dd_getdesc_fill_r_mask = 0xF800;
    g_dd_getdesc_fill_g_mask = 0x07E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
    zdd s; zdd_ctor(&s);
    zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd);
    uint32_t out = zdd_color_convert(&s, 0x00FF0000);
    T_ASSERT_EQ_I((int)out, 0xF800);  /* top 5 bits in R slot */
    return 0;
}

int test_zdd_color_convert_rgb565_pure_blue(void)
{
    reset_stubs();
    g_dd_getdesc_fill_r_mask = 0xF800;
    g_dd_getdesc_fill_g_mask = 0x07E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
    zdd s; zdd_ctor(&s);
    zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd);
    uint32_t out = zdd_color_convert(&s, 0x000000FF);
    T_ASSERT_EQ_I((int)out, 0x001F);
    return 0;
}

int test_zdd_color_convert_rgb565_mid_gray(void)
{
    /* 0x808080 → R=0x80 >> 3 = 0x10, G=0x80 >> 2 = 0x20, B=0x80 >> 3 = 0x10
     * Packed: (0x10 << 11) | (0x20 << 5) | (0x10 << 0)
     *       = 0x8000 | 0x0400 | 0x0010 = 0x8410 */
    reset_stubs();
    g_dd_getdesc_fill_r_mask = 0xF800;
    g_dd_getdesc_fill_g_mask = 0x07E0;
    g_dd_getdesc_fill_b_mask = 0x001F;
    zdd s; zdd_ctor(&s);
    zdd_bind_pixel_format(&s, (void *)(uintptr_t)0xabcd);
    uint32_t out = zdd_color_convert(&s, 0x00808080);
    T_ASSERT_EQ_I((int)out, 0x8410);
    return 0;
}

/* ─── zdd_object_blt_keyed (FUN_005b9b70) ──────────────────────────── */

int test_zdd_object_blt_keyed_null_self_degenerate_success(void)
{
    reset_stubs();
    int rc = zdd_object_blt_keyed(NULL, NULL, 0, 0);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    return 0;
}

int test_zdd_object_blt_keyed_null_com_primary_degenerate_success(void)
{
    reset_stubs();
    zdd_object src; memset(&src, 0, sizeof(src));
    src.com_primary = NULL;
    int rc = zdd_object_blt_keyed(&src, NULL, 0, 0);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    return 0;
}

int test_zdd_object_blt_keyed_null_dest_returns_zero(void)
{
    reset_stubs();
    zdd_object src; memset(&src, 0, sizeof(src));
    src.com_primary = (void *)(uintptr_t)0xa;
    int rc = zdd_object_blt_keyed(&src, NULL, 0, 0);
    T_ASSERT_EQ_I(rc, 0);
    T_ASSERT_EQ_I(g_dd_blt_calls, 0);
    return 0;
}

int test_zdd_object_blt_keyed_offsets_dest_by_metric_0c_10(void)
{
    /* dest_rect = (metric_0c + x, metric_10 + y, +metric_b8, +metric_bc).
     * src_rect  = (metric_b0, metric_b4, metric_b8, metric_bc).
     * flags     = state_flag | 0x1000000. */
    reset_stubs();
    zdd_object src; memset(&src, 0, sizeof(src));
    src.com_primary = (void *)(uintptr_t)0xaaa1;
    src.metric_0c   = 30;
    src.metric_10   = 40;
    src.metric_b0   = 0;
    src.metric_b4   = 0;
    src.metric_b8   = 100;
    src.metric_bc   = 50;
    src.state_flag  = 0x8000;
    zdd_object dst; memset(&dst, 0, sizeof(dst));
    dst.com_primary = (void *)(uintptr_t)0xddd1;

    int rc = zdd_object_blt_keyed(&src, &dst, 5, 7);
    T_ASSERT_EQ_I(rc, 1);
    T_ASSERT_EQ_I(g_dd_blt_calls, 1);
    /* dest = (30+5, 40+7, 100+35, 50+47) = (35, 47, 135, 97) */
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[0], 35);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[1], 47);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[2], 135);
    T_ASSERT_EQ_I(g_dd_blt_last_dest_rect[3], 97);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[0], 0);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[1], 0);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[2], 100);
    T_ASSERT_EQ_I(g_dd_blt_last_src_rect[3], 50);
    T_ASSERT_EQ_I((int)g_dd_blt_last_flags, (int)(0x8000 | 0x1000000));
    T_ASSERT_EQ_P(g_dd_blt_last_dest, (void *)(uintptr_t)0xddd1);
    T_ASSERT_EQ_P(g_dd_blt_last_src,  (void *)(uintptr_t)0xaaa1);
    return 0;
}
