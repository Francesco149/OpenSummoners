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
static unsigned  g_max_frames;
static int       g_launcher_mode = DEFAULT_LAUNCHER_MODE;
static int       g_shutdown;
static HANDLE    g_singleton_mutex;
static zdd      *g_zdd;

static uint32_t  g_frame_counter;
static uint32_t  g_base_time_ms;
static uint32_t  g_total_ms;

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
    if (!g_skip_ddraw && !init_ddraw()) {
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

    g_base_time_ms = timeGetTime();

    /* g_max_frames == 0 → run until WM_QUIT or external close.  Non-zero
     * is the harness/smoke pattern that lets us exit voluntarily so
     * stderr flushes cleanly before the launcher's grace window. */
    while (!g_shutdown && (g_max_frames == 0 || g_frame_counter < g_max_frames)) {
        main_loop_body();
    }

    log_line("OpenSummoners exiting after %u frames (%u ms wall)",
             g_frame_counter, g_total_ms);

    shutdown_ddraw();
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
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_shutdown = 1;
            return;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    /* Per-frame present — FUN_005b8fc0's 5-mode dispatcher.  Until the
     * real scene runner (FUN_0056aea0) lands and produces real content,
     * the offscreen surface is whatever DDraw initialised it to (usually
     * zero / undefined), so the windowed BitBlt produces a black or
     * uninitialised rectangle in the window.  That's the expected
     * behaviour pre-renderer; success is "zero DDERR per frame". */
    if (!g_skip_ddraw && !g_no_present && g_zdd != NULL) {
        zdd_present(g_zdd);
    }

    frame_limiter();
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
