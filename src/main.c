/*
 * OpenSummoners — drop-in replacement for sotes.exe (skeleton).
 *
 * Phase-0 skeleton: register a window class, create a 640x480 window
 * (placeholder — retail's actual surface size will be confirmed once we
 * see DirectDrawCreateEx + SetCooperativeLevel), run a PeekMessage/Dispatch
 * loop with a millisecond-resolution Sleep gate, exit cleanly on WM_QUIT.
 * Real renderer / audio / input wiring lands in subsequent slices.
 *
 * Drop-in defaults:
 *   - Auto-cd into OPENSUMMONERS_GAME_DIR and SetDllDirectory to the same
 *     so any later LoadLibrary calls resolve to the game-dir DLLs
 *     (sotesd.dll / sotesp.dll / sotesw.dll) rather than picking up
 *     anything from system32.  This was a footgun the OpenMare port hit
 *     when its renderer DLL load picked the OS ddraw.dll instead of
 *     the game's wrapper — same risk class here.
 *   - MessageBox→stderr hook installed unconditionally (override with
 *     --show-msgbox).  A blocked modal on Wndclass / DDraw init would
 *     otherwise stall the harness invisibly.
 *   - Single-instance mutex so a stray .exe from a previous SIGKILL'd
 *     run can't silently shadow the current build.  Skip with --allow-multi.
 *   - --hide-window: skip ShowWindow for harness runs.
 *   - --frames N: voluntary exit after N main-loop iterations so stderr
 *     flushes cleanly for the smoke-test pipeline.
 *   - --skip-ddraw: skip DDraw init entirely (window-only boot).
 *   - --no-present: skip the per-frame zdd_present dispatch
 *     (DDraw init still runs).
 *   - --no-title-scene: skip the title-scene drive (FUN_0056aea0 caller)
 *     and run the legacy minimal present loop instead — a known-good
 *     black/uninitialised window, useful for isolating DDraw-init issues
 *     from the scene runner.
 *   - --input-trace <file.jsonl>: replay a sparse {frame,ids} input trace
 *     (the harness format, tools/frida_capture.py) into the drive's input
 *     ring, keyed on the present/Flip count — drives the title scene
 *     deterministically (skip-splash / menu nav).  No-op without the drive.
 *
 * Build: `make -C src` from inside `nix develop`.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dev_hooks.h"
#include "zdd.h"
#include "cs_dispatch.h"
#include "call_trace.h"
#include "title_drive.h"
#include "input_trace.h"
#include "asset_register.h"   /* ar_sprite_decode / ar_sprite_decode_hook */
#include "bitmap_session.h"   /* bs_convert_* for the 8d format switch adapter */
#include "pixel_drawer.h"     /* pd_boot_init_slots — the alpha-ramp descriptors */

#define OPENSUMMONERS_CLASS  "OpenSummonersMain"
#define OPENSUMMONERS_TITLE  "Fortune Summoners"

/* Placeholder default surface.  Retail's actual window size needs a Ghidra
 * pass on CreateWindowEx / DirectDrawCreateEx call sites; until then we
 * boot a 640x480 surface (Fortune Summoners' on-disk readme suggests it
 * runs in 4:3 windowed by default — confirmable later). */
#define DEFAULT_WIDTH   640
#define DEFAULT_HEIGHT  480

/* Same-era games typically target 60 FPS; the exact frame period is a
 * Ghidra answer.  Until then, 16ms is a safe initial gate that won't peg
 * a single CPU and won't visibly lag. */
#define FRAME_PERIOD_MS  16

#define SINGLETON_MUTEX_NAME  "OpenSummoners-SingleInstance"

/* Default launcher mode = 2 (Windowed).  Retail picks this from the
 * launcher dialog's radio (stored in user/config.dat at +0x04).  Until
 * we port FUN_005a4770 (the 45 KB config.dat parser) the drop-in
 * hardcodes Windowed — the lowest-risk first wiring pass, doesn't
 * require display-mode changes that may fail under WSL/RDP/headless.
 * Override per-run via --launcher-mode=N (0=Full / 1=Safe / 2=Wind /
 * 3=DB / 4=Zoom).  Zoom-target defaults mirror retail's
 * `*(int*)(in_ECX + 0x14/0x18)` = 1280×960. */
#define DEFAULT_LAUNCHER_MODE   2
#define DEFAULT_ZOOM_TARGET_W   1280
#define DEFAULT_ZOOM_TARGET_H   960

static HINSTANCE g_hInstance;
static HWND      g_hwnd;
static int       g_hide_window;
static int       g_show_msgbox;
static int       g_allow_multi;
static int       g_skip_cd;
static int       g_skip_ddraw;
static int       g_no_present;
static int       g_no_title_scene;
static unsigned  g_max_frames;
static int       g_launcher_mode = DEFAULT_LAUNCHER_MODE;
static int       g_shutdown;
static HANDLE    g_singleton_mutex;
static zdd      *g_zdd;

/* sotesd.dll — the bulk-data resource DLL that holds every title sprite
 * (logo 0x49f, backgrounds 0x91b/0x91c, menu cursor, …).  Retail's launcher
 * (FUN_005a4770 @ 0x5af5fc) LoadLibraryA's it and stashes the HMODULE in
 * DAT_008a6e74, which the boot driver then passes to every sprite/font/sound
 * registrar as the `settings` (PE-resource source) argument.  We mirror that:
 * load it once after the game-dir is anchored, register the title banks, and
 * free it at shutdown.  NB despite the `sotesp_module` parameter name on the
 * registrar, retail sources slot 0 (id 0x90b) from this same handle. */
static HMODULE   g_sotesd;

/* Alpha-ramp pointer tables — retail's DAT_008a92b8 (ramp_a) / DAT_008a9308
 * (ramp_b) are 20-entry tables of pointers to blend descriptors.  The
 * descriptors themselves are built by pd_boot_init_slots into the static
 * g_pd_boot_group_a/_b arrays; these tables expose them as the
 * `const zdd_blend_desc *const *` the render wrappers index (PdBlend aliases
 * the retail blend-descriptor layout that zdd_alpha_blit reads — mode/state at
 * +0, the three channels at +0x04/0x18/0x2c).  Filled once by
 * init_alpha_ramps(); they back the menu cursor, the sprite-level fades, and
 * (once their arms are wired) the logo/sparkle alpha draws. */
static const zdd_blend_desc *g_ramp_a[PD_BOOT_GROUP_A_COUNT];
static const zdd_blend_desc *g_ramp_b[PD_BOOT_GROUP_B_COUNT];
static int       g_ramps_built;

/* The title-scene drive (the caller side of FUN_0056aea0).  Active once
 * init_title_drive succeeds; one main_loop_body iteration runs one
 * title_scene_step against it.  --no-title-scene falls back to the legacy
 * minimal present loop (a black/uninitialised window, zero DDERR/frame). */
static title_drive g_drive;
static int         g_drive_active;

/* --input-trace <file.jsonl> — port-side deterministic input replay (the
 * harness's {frame,ids} format).  Injected into the drive's input ring keyed
 * on the present (Flip) count g_present_frame, which drive_present bumps. */
static char              g_input_trace_path_buf[1024];
static const char       *g_input_trace_path;
static struct input_trace g_input_trace;
static int               g_input_trace_active;
static uint32_t          g_present_frame;

static uint32_t  g_frame_counter;
static uint32_t  g_base_time_ms;
static uint32_t  g_total_ms;

/* --call-trace <path> [--call-trace-frames i,j,k] — port-side counterpart
 * of the Frida agent's call_trace emitter (see src/call_trace.h).  The
 * path is copied into a static buffer because parse_cmdline's token
 * pointers alias a stack buffer. */
#define CALL_TRACE_FRAMES_CAP 256
static char      g_call_trace_path_buf[1024];
static const char *g_call_trace_path;
static unsigned  g_call_trace_frames[CALL_TRACE_FRAMES_CAP];
static size_t    g_n_call_trace_frames;

/* --capture-frames "60,200,…" [--capture-dir <path>] — port-side frame
 * capture (the counterpart of the Frida harness's --capture-frames).  At
 * each requested Flip (present) frame, GetDC the composed primary surface,
 * BitBlt it into a 24bpp DIB, and write <dir>/port_frame_NNNNN.bmp.  Lets us
 * diff the port's actual pixels against the retail goldens in runs/. */
#define CAPTURE_FRAMES_CAP 64
static unsigned  g_capture_frames[CAPTURE_FRAMES_CAP];
static size_t    g_n_capture_frames;
static char      g_capture_dir_buf[1024] = ".";
static const char *g_capture_dir = g_capture_dir_buf;

static LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);
static int  register_window_class(void);
static int  create_main_window(void);
static void parse_cmdline(LPSTR);
static void init_game_dir(void);
static int  acquire_singleton(void);
static void release_singleton(void);
static void main_loop_body(void);
static void frame_limiter(void);
static void log_line(const char *fmt, ...);
static int  init_ddraw(void);
static void shutdown_ddraw(void);
static void sync_window_position(void);
static void init_title_drive(void);
static void init_sprite_banks(void);
static void init_alpha_ramps(void);
static void maybe_capture_frame(unsigned flip_frame);
static int  capture_primary_to_bmp(const char *path);
static void drive_present(void *user);
static void drive_log_flip(void *user);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrev; (void)nCmdShow;
    g_hInstance = hInst;

    /* Our stderr lines use UTF-8 ("—" em-dashes etc.).  Without this
     * the console-subsystem build renders them as CP437 mojibake
     * (e.g. "ΓÇö" for "—").  No-op for the GUI-subsystem build when
     * no console is attached. */
    SetConsoleOutputCP(CP_UTF8);

    parse_cmdline(lpCmdLine);

    /* Open the port-side call trace (no-op unless --call-trace given).
     * Done before any traced function (boot DDraw path) can run. */
    call_trace_init_from_cli(g_call_trace_path,
                             g_call_trace_frames, g_n_call_trace_frames);

    /* Install MessageBox→stderr hook as early as possible.  Any modal
     * popup between here and uninstall (in shutdown) gets redirected
     * instead of blocking the process invisibly. */
    if (!g_show_msgbox) {
        dev_hooks_install_messagebox();
    }

    /* Single-instance enforcement catches the dev-loop foot-gun of
     * accidentally leaving a stray .exe alive from a previous SIGKILL'd
     * run.  Without this, the second invocation silently shadows the
     * first and you spend an hour wondering why your changes don't take
     * effect. */
    if (!g_allow_multi && !acquire_singleton()) {
        return 2;
    }

    if (!g_skip_cd) init_game_dir();

    log_line("OpenSummoners starting (hide_window=%d, max_frames=%u)",
             g_hide_window, g_max_frames);

    timeBeginPeriod(1);

    if (!register_window_class()) {
        log_line("RegisterClassA failed: %lu", GetLastError());
        timeEndPeriod(1);
        return 1;
    }

    if (!create_main_window()) {
        log_line("CreateWindowExA failed: %lu", GetLastError());
        timeEndPeriod(1);
        return 1;
    }

    if (!g_hide_window) {
        ShowWindow(g_hwnd, SW_SHOW);
        UpdateWindow(g_hwnd);
    }

    /* DDraw init — mirrors retail FUN_00562ea0's bootstrap order:
     *   1. FUN_005b7ee0  → zdd_create            (DirectDrawCreateEx)
     *   2. FUN_005b89d0  → zdd_set_coop_level    (SetCooperativeLevel)
     *   3. FUN_00582e90  → cs_dispatch_create_screen (per-mode CreateScreen)
     * Fullscreen flag for SetCooperativeLevel is (launcher_mode != 2)
     * — only Windowed runs DDSCL_NORMAL.  cs_dispatch_create_screen
     * returns void; on failure it ExitProcess(0)'s via cs_exit. */
    /* Bracket the one-shot boot path as call-trace frame 0 so the DDraw
     * bootstrap (zdd_create / SetCooperativeLevel / CreateScreen
     * dispatch) shows up in the diff against retail's frame-0 boot
     * calls.  The per-frame loop below re-opens its own frames. */
    call_trace_begin_frame(0);
    if (!g_skip_ddraw && !init_ddraw()) {
        call_trace_end_frame();
        timeEndPeriod(1);
        return 1;
    }

    /* Stamp the ZDD's screen_pos_x/y from the window's actual client-
     * area top-left in screen coords.  Mode 2 (Windowed) present needs
     * this to BitBlt into the right desktop location — retail keeps it
     * in sync from somewhere we haven't traced yet (likely WM_MOVE
     * inside FUN_005b9130 or a sibling).  Updated again on WM_MOVE
     * below; this initial sample covers the window's spawn position. */
    sync_window_position();

    /* Register the title sprite banks from sotesd.dll BEFORE the drive starts,
     * so the render sink's bank getter (ar_pool_get_slot 19/20) resolves to
     * populated slots instead of short-circuiting to NULL.  Without this the
     * whole decode→slice→8d chain never runs and the window stays blank. */
    if (!g_no_title_scene && !g_skip_ddraw && g_zdd != NULL)
        init_sprite_banks();

    /* Stand up the title-scene drive (the caller side of FUN_0056aea0): bind
     * the render sink to the live primary surface, install the sprite-bank
     * self-decode hook, and allocate the scene object graph.  After this the
     * per-frame loop runs the title scene; --no-title-scene or a failed/absent
     * DDraw boot falls back to the legacy minimal present loop. */
    if (!g_no_title_scene && !g_skip_ddraw && g_zdd != NULL)
        init_title_drive();

    call_trace_end_frame();   /* close boot frame 0 */

    g_base_time_ms = timeGetTime();

    /* g_max_frames == 0 → run until WM_QUIT or external close.  Non-zero
     * is the harness/smoke pattern that lets us exit voluntarily so
     * stderr flushes cleanly before the launcher's grace window. */
    while (!g_shutdown && (g_max_frames == 0 || g_frame_counter < g_max_frames)) {
        main_loop_body();
    }

    log_line("OpenSummoners exiting after %u frames (%u ms wall)",
             g_frame_counter, g_total_ms);

    if (g_drive_active) {
        title_drive_shutdown(&g_drive);
        g_drive_active = 0;
    }
    if (g_input_trace_active) {
        input_trace_free(&g_input_trace);
        g_input_trace_active = 0;
    }
    shutdown_ddraw();
    if (g_sotesd != NULL) {
        FreeLibrary(g_sotesd);
        g_sotesd = NULL;
    }
    call_trace_shutdown();
    timeEndPeriod(1);
    dev_hooks_uninstall_messagebox();
    release_singleton();
    return 0;
}

/* Build the ZDD + drive the per-mode CreateScreen dispatcher.  Logs
 * the chosen mode + the result of each step.  Returns 1 on success.
 *
 * On cs_dispatch_create_screen failure, the real Win32 cs_exit is
 * ExitProcess(0) — control never returns from that call.  We log
 * "dispatch returned" after it as a marker for the success path. */
static int init_ddraw(void)
{
    log_line("init_ddraw: launcher_mode=%d (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)",
             g_launcher_mode);

    if (!zdd_create(&g_zdd)) {
        log_line("init_ddraw: zdd_create (DirectDrawCreateEx) failed");
        return 0;
    }

    int fullscreen = (g_launcher_mode != 2);
    if (!zdd_set_coop_level(g_zdd, g_hwnd, fullscreen)) {
        log_line("init_ddraw: SetCooperativeLevel(fullscreen=%d) failed",
                 fullscreen);
        zdd_destroy(g_zdd);
        g_zdd = NULL;
        return 0;
    }
    log_line("init_ddraw: SetCooperativeLevel ok (fullscreen=%d)", fullscreen);

    cs_dispatch_create_screen(g_zdd, g_launcher_mode,
                              DEFAULT_ZOOM_TARGET_W, DEFAULT_ZOOM_TARGET_H);
    log_line("init_ddraw: CreateScreen dispatch returned (success path)");
    return 1;
}

static void shutdown_ddraw(void)
{
    if (g_zdd != NULL) {
        zdd_destroy(g_zdd);
        g_zdd = NULL;
    }
}

/* TITLE_DRAW_FLIP → present the frame (retail FUN_005b8fc0).  --no-present
 * suppresses it (DDraw init still ran, the surface just isn't blitted). */
static void drive_present(void *user)
{
    (void)user;
    /* The Flip count is the input-trace's frame axis (matching the harness's
     * Flip-anchored injection), so bump it on every present. */
    g_present_frame++;
    /* Capture BEFORE the flip: the fully-composed frame is on primary_obj's
     * surface here, exactly as the sink drew it this iteration. */
    maybe_capture_frame(g_present_frame);
    /* In Windowed mode (the drop-in default, launcher_mode 2) zdd_present
     * BitBlts the composed primary onto the *desktop* at the window's screen
     * position (zdd_desktop_present → GetDC(NULL)) — faithful to retail, which
     * paints into its window's client area.  But under --hide-window the window
     * is SW_HIDE'd while that desktop blit still paints a 640×480 rectangle on
     * the real screen every frame: the "hidden window flickers over my screen"
     * bleed-through.  Hidden ⇒ nothing to present into, so skip the blit; the
     * frame is already fully composed on primary_obj (captures read it above,
     * before this), so suppressing the present is lossless for headless runs. */
    if (!g_no_present && !g_hide_window && g_zdd != NULL)
        zdd_present(g_zdd);
}

/* TITLE_DRAW_LOG_FLIPPING → the engine's "Title Menu - Flipping" marker. */
static void drive_log_flip(void *user)
{
    (void)user;
    log_line("Title Menu - Flipping");
}

/* ─── 8d hook adapters (asset_register → ZDD) ────────────────────────
 *
 * These bridge the decoupled asset_register slicer to the live ZDD: the
 * per-cell surface builder, the per-sheet format conversion, and the
 * per-frame surface release.  Kept in main.c (not asset_register) so the
 * decoder TU stays ZDD-free. */

/* ar_frame_build_hook (0x5b9280) — build one cell's keyed DDraw surface. */
static void *title_frame_build(ar_sprite_slot *slot,
                               const struct bitmap_session *sheet,
                               int src_x, int src_y, int cell_w, int cell_h,
                               uint32_t colorkey, void *aux_entry)
{
    zdd *parent = (slot != NULL && slot->zdd != NULL) ? (zdd *)slot->zdd
                                                      : g_zdd;
    if (parent == NULL) return NULL;
    zdd_object *out = NULL;
    int ok = zdd_object_new_cell(parent, &out, sheet,
                                 src_x, src_y, cell_w, cell_h,
                                 (int32_t)colorkey,
                                 (int)slot->scale_flag,   /* videomem / create p5 */
                                 (int)slot->type,         /* trim_count */
                                 (const struct bs_trim_rect *)aux_entry);
    return ok ? (void *)out : NULL;
}

/* ar_frame_free_hook (FUN_005b9390 + delete) — release one built surface. */
static void title_frame_free(void *surface)
{
    zdd_object *o = (zdd_object *)surface;
    zdd_obj_destroy(&o);
}

/* ar_sheet_format_hook (slicer 0x4189f2 switch) — convert a decoded sheet
 * to the god-object's display depth before cells are built.  Windowed mode
 * runs at 16bpp, so a 24bpp title sheet is packed to RGB565 via the
 * descriptor zdd_bind_pixel_format stamped on the 16bpp boot. */
static void title_sheet_format(ar_sprite_slot *slot,
                               struct bitmap_session *sheet, uint32_t colorkey)
{
    zdd *z = (slot != NULL && slot->zdd != NULL) ? (zdd *)slot->zdd : g_zdd;
    if (z == NULL || sheet == NULL) return;

    int depth = z->pixel_format_bpp;                 /* [zdd+0x168] */
    int src   = (int)bs_get_bit_count(sheet);
    uint32_t key_color = (colorkey == 0x1ffffffu) ? 0u : 0xff00ffu;

    switch (depth) {
    case 8:
        break;                                       /* 8bpp display: no convert */
    case 16:
        if (src == 8 || src == 24)
            bs_convert_to_16bpp(sheet, (const uint8_t *)&z->color_desc,
                                colorkey, key_color);
        break;
    case 24:
        if (src == 8)
            bs_convert_8bpp_to_24bpp(sheet, colorkey, key_color);
        break;
    case 32:
        if (src == 8)
            bs_convert_8bpp_to_24bpp(sheet, colorkey, key_color);
        else if (src != 24)
            break;
        bs_convert_24bpp_to_32bpp(sheet);
        break;
    default:
        break;
    }
}

/* Load sotesd.dll and register the title sprite banks against the live ZDD.
 *
 * Mirrors retail's boot driver FUN_00562ea0 @ 0x5749b0 call:
 *   FUN_005749b0(ZDD=DAT_008a93cc, group=4, settings=DAT_008a6e74[sotesd])
 * where DAT_008a6e74 is the sotesd.dll HMODULE the launcher loaded.  The
 * registrar stamps g_ar_sprite_slots[0..29] + stragglers with their resource
 * IDs (no decode yet — that happens lazily when a bank is first painted).
 * The two the title sink reads are slot 6 (MAIN, id 0x91b → pool index 19)
 * and slot 7 (CURSOR, id 0x91c → pool index 20).
 *
 * Both the `settings` and `sotesp_module` parameters take the SAME sotesd
 * handle: retail sources every main-sprite slot (including slot 0's palette
 * seed, id 0x90b) from DAT_008a6e74.  (Verified: 0x90b lives in sotesd.dll,
 * not sotesp.dll, despite the historical parameter name.)
 *
 * On failure to load sotesd.dll we log and continue: the drive still boots
 * (cleared+flipped window, no sprites), matching the pre-registration state. */
static void init_sprite_banks(void)
{
    g_sotesd = LoadLibraryA("sotesd.dll");
    if (g_sotesd == NULL) {
        log_line("init_sprite_banks: LoadLibraryA(sotesd.dll) failed: %lu — "
                 "title banks stay unregistered (blank render)", GetLastError());
        return;
    }

    /* Set up the sprite/info/gdi/sound pool pointer tables once before any
     * registrar touches them (idempotent; globals start zeroed). */
    ar_state_init();

    ar_register_main_sprites(g_zdd, /*group=*/4, /*settings=*/g_sotesd,
                             /*sotesp_module=*/g_sotesd);

    log_line("init_sprite_banks: sotesd.dll=%p, registered title banks "
             "(MAIN=pool19/id0x91b, CURSOR=pool20/id0x91c)", (void *)g_sotesd);
}

/* Build the alpha-ramp blend descriptors and expose them as the ramp_a/ramp_b
 * pointer tables the render wrappers consume.  pd_boot_init_slots(NULL) builds
 * the descriptors for RGB565 (the NULL-format default = the 16bpp windowed
 * display), matching retail's per-group ramps (group A: weight/20 mode 1 =
 * ramp_a; group B: weight/22 mode 0 = ramp_b).  Idempotent. */
static void init_alpha_ramps(void)
{
    if (g_ramps_built) return;
    pd_boot_init_slots(NULL);             /* NULL fmt ⇒ RGB565 descriptors */
    for (int i = 0; i < PD_BOOT_GROUP_A_COUNT; i++)
        g_ramp_a[i] = (const zdd_blend_desc *)&g_pd_boot_group_a[i];
    for (int i = 0; i < PD_BOOT_GROUP_B_COUNT; i++)
        g_ramp_b[i] = (const zdd_blend_desc *)&g_pd_boot_group_b[i];
    g_ramps_built = 1;
    log_line("init_alpha_ramps: built ramp_a/ramp_b (20+20 RGB565 blend "
             "descriptors) — menu cursor + sprite-level fades now alpha-blend");
}

/* Grab the composed primary surface into a 24bpp DIB and write it as a BMP.
 * Returns 1 on success.  Uses the already-ported zdd_object GetDC/ReleaseDC
 * primitives (DirectDrawSurface7::GetDC) + a plain GDI BitBlt into a bottom-up
 * DIB section — whose memory layout is exactly BMP pixel order, so the bits
 * write straight out after the two headers. */
static int capture_primary_to_bmp(const char *path)
{
    if (g_zdd == NULL || g_zdd->primary_obj == NULL) return 0;

    const int W = DEFAULT_WIDTH, H = DEFAULT_HEIGHT;
    void *src_hdc = NULL;
    if (!zdd_object_get_dc(g_zdd->primary_obj, &src_hdc) || src_hdc == NULL)
        return 0;

    HDC mem = CreateCompatibleDC((HDC)src_hdc);
    if (mem == NULL) { zdd_object_release_dc(g_zdd->primary_obj, src_hdc); return 0; }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = W;
    bi.bmiHeader.biHeight      = H;          /* +H = bottom-up = BMP order */
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP dib = CreateDIBSection((HDC)src_hdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    int ok = 0;
    if (dib != NULL && bits != NULL) {
        HGDIOBJ old = SelectObject(mem, dib);
        if (BitBlt(mem, 0, 0, W, H, (HDC)src_hdc, 0, 0, SRCCOPY)) {
            const uint32_t row    = (uint32_t)((W * 3 + 3) & ~3);
            const uint32_t pixsz  = row * (uint32_t)H;
            BITMAPFILEHEADER fh;
            memset(&fh, 0, sizeof fh);
            fh.bfType    = 0x4D42;            /* 'BM' */
            fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            fh.bfSize    = fh.bfOffBits + pixsz;

            FILE *f = fopen(path, "wb");
            if (f != NULL) {
                fwrite(&fh, sizeof fh, 1, f);
                fwrite(&bi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, f);
                fwrite(bits, 1, pixsz, f);
                fclose(f);
                ok = 1;
            }
        }
        SelectObject(mem, old);
    }
    if (dib != NULL) DeleteObject(dib);
    DeleteDC(mem);
    zdd_object_release_dc(g_zdd->primary_obj, src_hdc);
    return ok;
}

/* If flip_frame is in the --capture-frames whitelist, dump it. */
static void maybe_capture_frame(unsigned flip_frame)
{
    for (size_t i = 0; i < g_n_capture_frames; i++) {
        if (g_capture_frames[i] != flip_frame) continue;
        char path[1200];
        snprintf(path, sizeof path, "%s/port_frame_%05u.bmp",
                 g_capture_dir, flip_frame);
        int ok = capture_primary_to_bmp(path);
        /* Log the scene's pulse state too — phase (local_64), fade (uVar15)
         * and menu_fade (local_58 / the cursor level_num).  This lets the
         * parity harness match a port capture to the retail golden at the
         * same cursor-pulse phase (parity-ledger R1). */
        log_line("capture: frame %u -> %s (%s) phase=%d fade=%d menu_fade=%d",
                 flip_frame, path, ok ? "ok" : "FAILED",
                 g_drive_active ? (int)g_drive.scene.fade.phase   : -1,
                 g_drive_active ? (int)g_drive.scene.fade.fade     : -1,
                 g_drive_active ? (int)g_drive.scene.fade.menu_fade : -1);
        return;
    }
}

/* Build the title-scene drive against the live ZDD.  Binds the render sink to
 * g_zdd->primary_obj (so the cmd stream drives real blits) and installs
 * ar_sprite_decode_hook so the title banks self-decode once registered, plus
 * the 8d hooks (build / format / free) so the decoded banks become real keyed
 * surfaces.  The alpha ramps (0x8a92b8/0x8a9308) and the compositor display
 * group are unfilled/unmodeled at a cold boot, so they pass through as NULL
 * (plain blits / no compose — faithful).  skip_intro stays 0 so a first boot
 * plays the studio fade from the start, exactly like retail's cold launch. */
static void init_title_drive(void)
{
    /* Self-decode the sprite banks on first frame access, and build real
     * per-cell surfaces (8d) once a bank is decoded. */
    ar_sprite_decode_hook = ar_sprite_decode;
    ar_frame_build_hook   = title_frame_build;
    ar_frame_free_hook    = title_frame_free;
    ar_sheet_format_hook  = title_sheet_format;

    /* Build the alpha-ramp descriptors so the cursor / sprite-level fades have
     * real blend tables (without them the cursor's alpha draw no-ops). */
    init_alpha_ramps();

    title_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.primary    = g_zdd->primary_obj;
    cfg.present    = drive_present;
    cfg.log_flip   = drive_log_flip;
    cfg.user       = NULL;
    cfg.select_key = 0;        /* no saved menu pick at a cold boot */
    cfg.quiet      = 0;
    cfg.skip_intro = 0;
    cfg.ramp_a     = g_ramp_a; /* 0x8a92b8 — menu cursor + compositor */
    cfg.ramp_b     = g_ramp_b; /* 0x8a9308 — sprite-level fades + logo */

    if (!title_drive_init(&g_drive, &cfg)) {
        log_line("init_title_drive: title_drive_init failed — "
                 "falling back to legacy present loop");
        return;
    }
    g_drive_active = 1;
    log_line("init_title_drive: title scene driven (primary=%p, 8d surface "
             "builder wired: depth=%d bpp)", (void *)g_zdd->primary_obj,
             g_zdd->pixel_format_bpp);

    /* Load the optional input-replay trace now that the drive (and its input
     * ring) exists.  Injection happens per-frame in main_loop_body. */
    if (g_input_trace_path != NULL) {
        if (input_trace_load(g_input_trace_path, &g_input_trace)) {
            g_input_trace_active = 1;
            log_line("init_title_drive: input trace loaded (%zu entries) from %s",
                     g_input_trace.count, g_input_trace_path);
        } else {
            input_trace_free(&g_input_trace);
            log_line("init_title_drive: failed to load input trace %s",
                     g_input_trace_path);
        }
    }
}

/* Anchor CWD + DLL search dir to the game directory.  Reads
 * OPENSUMMONERS_GAME_DIR from the environment — nix develop's shellHook
 * exports it with WSLENV's /p flag so the .exe gets a Windows-form path
 * even though we cross from WSL into Windows via WSLInterop. */
static void init_game_dir(void)
{
    char dir[MAX_PATH] = {0};
    DWORD n = GetEnvironmentVariableA("OPENSUMMONERS_GAME_DIR", dir, sizeof(dir));
    if (n == 0 || n >= sizeof(dir)) {
        log_line("OPENSUMMONERS_GAME_DIR unset — staying in CWD");
        return;
    }
    if (!SetCurrentDirectoryA(dir)) {
        log_line("SetCurrentDirectoryA(%s) failed: %lu", dir, GetLastError());
        return;
    }
    /* SetDllDirectoryA puts this dir at the head of the DLL search path
     * for any further LoadLibrary calls.  Combined with the SafeDllSearch
     * default, the game-dir DLLs (sotesp/sotesd/sotesw) win over anything
     * the OS would otherwise pull from system32. */
    SetDllDirectoryA(dir);
    log_line("init_game_dir: cd %s", dir);
}

static int acquire_singleton(void)
{
    g_singleton_mutex = CreateMutexA(NULL, TRUE, SINGLETON_MUTEX_NAME);
    if (!g_singleton_mutex) {
        log_line("CreateMutexA failed: %lu", GetLastError());
        return 0;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        log_line("Another OpenSummoners instance is already running — exiting");
        CloseHandle(g_singleton_mutex);
        g_singleton_mutex = NULL;
        return 0;
    }
    return 1;
}

static void release_singleton(void)
{
    if (g_singleton_mutex) {
        ReleaseMutex(g_singleton_mutex);
        CloseHandle(g_singleton_mutex);
        g_singleton_mutex = NULL;
    }
}

static int register_window_class(void)
{
    WNDCLASSA wc = {0};
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = wndproc;
    wc.hInstance     = g_hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = OPENSUMMONERS_CLASS;
    return RegisterClassA(&wc) != 0;
}

static int create_main_window(void)
{
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT };
    AdjustWindowRect(&rc, style, FALSE);
    g_hwnd = CreateWindowExA(
        0,
        OPENSUMMONERS_CLASS, OPENSUMMONERS_TITLE,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, g_hInstance, NULL);
    return g_hwnd != NULL;
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_shutdown = 1;
        PostQuitMessage(0);
        return 0;
    case WM_MOVE:
        /* Keep ZDD screen_pos_x/y in sync with the window's client
         * top-left so mode-2 present's desktop BitBlt lands inside
         * the window. */
        sync_window_position();
        return 0;
    case WM_PAINT:
        /* Port of retail's WM_PAINT consumption via FUN_005b9130
         * (zdd_window_paint).  Only consumes in mode 2 (Windowed) —
         * other modes (and unfinished DDraw init) fall through to
         * DefWindowProcA, which validates the dirty region itself.
         * Wired here directly (not via wp_handle_message) because the
         * drop-in uses its own minimal wndproc — the ported WndProc
         * module is still in isolation pending input-subsystem ports. */
        if (!g_skip_ddraw && g_zdd != NULL &&
            zdd_window_paint(g_zdd, hwnd)) {
            return 0;
        }
        break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* Sample the window's client-area top-left in screen coordinates and
 * stamp it on g_zdd.  Called once after init_ddraw and on every
 * WM_MOVE thereafter.  No-op when DDraw is disabled or the window
 * isn't created yet. */
static void sync_window_position(void)
{
    if (g_zdd == NULL || g_hwnd == NULL) return;
    POINT pt = {0, 0};
    if (ClientToScreen(g_hwnd, &pt)) {
        g_zdd->screen_pos_x = (int32_t)pt.x;
        g_zdd->screen_pos_y = (int32_t)pt.y;
    }
}

static void main_loop_body(void)
{
    /* Open this call-trace frame before any traced work.  Frame axis =
     * present count (g_frame_counter), matching the agent's Flip-anchored
     * counter on the retail side. */
    call_trace_begin_frame(g_frame_counter);

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_shutdown = 1;
            return;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (g_drive_active) {
        /* Mirror retail's tight outer loop (FUN_0056aea0's do/while): spin the
         * pace machine — its update steps are ~free, no per-step sleep — until
         * it PRESENTS one frame, then fall through to frame_limiter, which
         * gates the presented-frame rate.  This is the R3 fix: the fixed-
         * timestep accumulator (title_pace_step) is designed to be called in a
         * tight loop with only the present blocking on wall-clock.  Driving it
         * one pace-step per 16 ms-throttled iteration instead made `now` run
         * away (the budget refill `b += now - anchor` grew unbounded), so the
         * port did ~6 updates per render and DROPPED ~5/6 of the intro's fade
         * frames (rendered 90 of ~528 update ticks).  Spinning to one present
         * per frame_limiter tick restores ~1 update per render, so every fade
         * value is rendered — matching retail's distinct-content sequence
         * (retail renders the same values, just ~2× duplicated by its higher
         * display refresh).  See docs/findings/title-scene.md "Intro pacing". */
        unsigned guard = 0;
        for (;;) {
            uint32_t now = GetTickCount();
            /* Inject any due replay events into the drive's input ring before
             * the scene polls it (keyed on the present/Flip count, stable
             * across the spin; input_trace_replay's cursor never re-injects). */
            if (g_input_trace_active)
                input_trace_replay(&g_input_trace, g_present_frame,
                                   &g_drive.input, now);
            unsigned pf_before = g_present_frame;
            title_scene_status st = title_drive_step(&g_drive, now);
            /* R3 intro-pace instrumentation: log each phase transition with the
             * Flip count + wall-clock elapsed.  Port-side counterpart of the
             * Frida --pace-probe (retail: ~9.2 s / ~1172 flips to the menu). */
            {
                static int s_last_phase = -1;
                int ph = (int)g_drive.scene.fade.phase;
                if (ph != s_last_phase) {
                    log_line("pace: phase %d -> %d @ flip=%u t=%ums",
                             s_last_phase, ph, g_present_frame,
                             (unsigned)(timeGetTime() - g_base_time_ms));
                    s_last_phase = ph;
                }
            }
            if (st == TITLE_SCENE_DONE) {
                log_line("title scene returned result=%ld", (long)g_drive.result);
                g_shutdown = 1;
                break;
            }
            /* drive_present bumps g_present_frame on a render step (the sink's
             * TITLE_DRAW_FLIP).  A change ⇒ this iteration presented a frame. */
            if (g_present_frame != pf_before)
                break;
            if (++guard > 10000)   /* safety: never spin unbounded on a stall */
                break;
        }
    } else if (!g_skip_ddraw && !g_no_present && g_zdd != NULL) {
        /* Legacy minimal present (--no-title-scene / failed drive init).  The
         * offscreen surface is whatever DDraw initialised it to, so the
         * windowed BitBlt produces a black/uninitialised rectangle; success is
         * "zero DDERR per frame". */
        zdd_present(g_zdd);
    }

    frame_limiter();
    call_trace_end_frame();
    g_frame_counter++;
}

/* 16 ms wall-time gate (placeholder until we resolve the engine's actual
 * frame cadence — fortune-summoners-era LizSoft games typically target
 * 60 FPS).  Same shape as OpenMare's frame_limiter, downshifted from
 * its 20 ms gate. */
static void frame_limiter(void)
{
    uint32_t now = timeGetTime();
    uint32_t elapsed = now - g_base_time_ms;
    uint32_t target  = (g_frame_counter + 1) * FRAME_PERIOD_MS;
    if (elapsed < target) {
        Sleep(target - elapsed);
        now = timeGetTime();
        elapsed = now - g_base_time_ms;
    }
    g_total_ms = elapsed;
}

static void parse_cmdline(LPSTR lpCmdLine)
{
    /* Tiny ad-hoc tokenizer.  We only need to handle flags without
     * embedded quotes; the launcher controls argv shape. */
    char buf[2048];
    strncpy(buf, lpCmdLine ? lpCmdLine : "", sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *tok = strtok(buf, " \t");
    while (tok) {
        if      (!strcmp(tok, "--hide-window"))  g_hide_window = 1;
        else if (!strcmp(tok, "--show-msgbox"))  g_show_msgbox = 1;
        else if (!strcmp(tok, "--allow-multi"))  g_allow_multi = 1;
        else if (!strcmp(tok, "--no-cd"))        g_skip_cd = 1;
        else if (!strcmp(tok, "--skip-ddraw"))   g_skip_ddraw = 1;
        else if (!strcmp(tok, "--no-present"))   g_no_present = 1;
        else if (!strcmp(tok, "--no-title-scene")) g_no_title_scene = 1;
        else if (!strcmp(tok, "--input-trace")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_input_trace_path_buf, tok,
                        sizeof(g_input_trace_path_buf) - 1);
                g_input_trace_path = g_input_trace_path_buf;
            }
        }
        else if (!strncmp(tok, "--input-trace=", 14)) {
            strncpy(g_input_trace_path_buf, tok + 14,
                    sizeof(g_input_trace_path_buf) - 1);
            g_input_trace_path = g_input_trace_path_buf;
        }
        else if (!strcmp(tok, "--frames")) {
            tok = strtok(NULL, " \t");
            if (tok) g_max_frames = (unsigned)strtoul(tok, NULL, 10);
        }
        else if (!strncmp(tok, "--frames=", 9)) {
            g_max_frames = (unsigned)strtoul(tok + 9, NULL, 10);
        }
        else if (!strcmp(tok, "--launcher-mode")) {
            tok = strtok(NULL, " \t");
            if (tok) g_launcher_mode = (int)strtol(tok, NULL, 10);
        }
        else if (!strncmp(tok, "--launcher-mode=", 16)) {
            g_launcher_mode = (int)strtol(tok + 16, NULL, 10);
        }
        else if (!strcmp(tok, "--call-trace")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_call_trace_path_buf, tok,
                        sizeof(g_call_trace_path_buf) - 1);
                g_call_trace_path = g_call_trace_path_buf;
            }
        }
        else if (!strncmp(tok, "--call-trace=", 13)) {
            strncpy(g_call_trace_path_buf, tok + 13,
                    sizeof(g_call_trace_path_buf) - 1);
            g_call_trace_path = g_call_trace_path_buf;
        }
        else if (!strcmp(tok, "--call-trace-frames") ||
                 !strncmp(tok, "--call-trace-frames=", 20)) {
            /* Comma-separated frame whitelist, e.g. 1,2,3.  Either
             * `--call-trace-frames 1,2,3` (next token) or the `=` form.
             * Parsed with strtoul's endptr rather than a nested strtok,
             * which would clobber the outer tokenizer's saved state. */
            const char *list = NULL;
            if (!strncmp(tok, "--call-trace-frames=", 20)) {
                list = tok + 20;
            } else {
                list = strtok(NULL, " \t");
            }
            while (list && *list && g_n_call_trace_frames < CALL_TRACE_FRAMES_CAP) {
                char *end = NULL;
                unsigned v = (unsigned)strtoul(list, &end, 10);
                if (end == list) break;          /* no digits — give up */
                g_call_trace_frames[g_n_call_trace_frames++] = v;
                list = end;
                while (*list == ',' || *list == ' ') list++;
            }
        }
        else if (!strcmp(tok, "--capture-frames") ||
                 !strncmp(tok, "--capture-frames=", 17)) {
            const char *list = NULL;
            if (!strncmp(tok, "--capture-frames=", 17)) list = tok + 17;
            else                                       list = strtok(NULL, " \t");
            while (list && *list && g_n_capture_frames < CAPTURE_FRAMES_CAP) {
                char *end = NULL;
                unsigned v = (unsigned)strtoul(list, &end, 10);
                if (end == list) break;
                g_capture_frames[g_n_capture_frames++] = v;
                list = end;
                while (*list == ',' || *list == ' ') list++;
            }
        }
        else if (!strcmp(tok, "--capture-dir")) {
            tok = strtok(NULL, " \t");
            if (tok) {
                strncpy(g_capture_dir_buf, tok, sizeof(g_capture_dir_buf) - 1);
                g_capture_dir_buf[sizeof(g_capture_dir_buf) - 1] = '\0';
            }
        }
        else if (!strncmp(tok, "--capture-dir=", 14)) {
            strncpy(g_capture_dir_buf, tok + 14, sizeof(g_capture_dir_buf) - 1);
            g_capture_dir_buf[sizeof(g_capture_dir_buf) - 1] = '\0';
        }
        tok = strtok(NULL, " \t");
    }
}

static void log_line(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[opensummoners] ");
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    fflush(stderr);
    va_end(ap);
}
