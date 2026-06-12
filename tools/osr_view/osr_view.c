/*
 * tools/osr_view/osr_view.c — the Trace Studio v2 native frame SCRUBBER.
 *
 * A from-scratch native Win32 viewer that scrubs a captured `.osr` draw stream by
 * RECONSTRUCTING each frame on demand with the real DDraw blits + GDI text (the
 * shared recon_apply.h core — the SAME sinks the port renders with), then BitBlt's
 * it straight to the window.  No PNG baking, no disk: rendering a frame is faster
 * than realtime, so scrubbing is instant.
 *
 * How instant scrub works:
 *   - open: ONE pass over the .osr (native C:\ — seconds) builds a frame INDEX
 *     (byte-offset / flip / sim_tick per frame) and loads every dedup'd source
 *     surface / HFONT / blend descriptor (recon_apply_sheet/font/blend).
 *   - render(N): the dest surface ACCUMULATES (retail flips a back-buffer chain —
 *     an empty re-present frame retains prior pixels, quirk #99), so we restore
 *     the nearest cached KEYFRAME ≤ N and replay only frames (keyframe..N]'s
 *     ops, re-read from their indexed offsets.  Forward-step is one frame.
 *     Keyframes (raw RGB565 snapshots) are cached lazily as you explore.
 *
 * Controls: ←/→ = ±1 frame, PgUp/PgDn = ±30, Home/End = first/last, Space = play/
 * pause, click the timeline to seek, Esc = quit.
 *
 * Single-pane (one .osr) for now; the port|retail DIFF pane lands once the port
 * emits its own .osr (M5).  Build: make -C tools/osr_view ; run on Windows:
 *   build/osr_view.exe C:\oss-osr\retail.osr
 */
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zdd.h"
#include "osr_format.h"
#include "recon_apply.h"

/* ── geometry ─────────────────────────────────────────────────────────────── */
#define KF_INTERVAL 120          /* keyframe every N frames (≤N-frame replay seek) */
#define BAR_H 56                  /* the bottom info+timeline bar */

static int   g_w = 640, g_h = 480;
static zdd  *g_zdd;
static zdd_object *g_dest;
static recon_tables g_rt;

/* ── frame index ──────────────────────────────────────────────────────────── */
typedef struct { long long off; uint32_t flip, tick; } frame_idx;
static frame_idx *g_frames;
static int        g_nframes;
static FILE      *g_osr;          /* kept open for random-access op replay */

/* ── lazy keyframe cache (raw RGB565 dest snapshots) ──────────────────────── */
static uint16_t **g_kf;           /* g_kf[k] = dest pixels at frame k*KF_INTERVAL */
static int        g_nkf;
static int        g_rendered = -2;/* which frame g_dest currently holds (-2 = none) */

/* ── reusable record read buffer ──────────────────────────────────────────── */
static uint8_t *g_buf;
static size_t   g_bufcap;
static int buf_ensure(uint32_t n)
{
    if (n <= g_bufcap) return 1;
    uint8_t *nb = (uint8_t *)realloc(g_buf, n);
    if (!nb) return 0;
    g_buf = nb; g_bufcap = n;
    return 1;
}

/* ── dest pixel snapshot / restore (RGB565, tight w*h) ────────────────────── */
static uint16_t *dest_snapshot(void)
{
    if (!zdd_object_lock(g_dest)) return NULL;
    void *buf = NULL; int32_t pitch = 0, h = 0;
    zdd_object_get_locked_info(g_dest, &buf, &pitch, &h);
    uint16_t *out = (uint16_t *)malloc((size_t)g_w * g_h * 2);
    if (out && buf)
        for (int r = 0; r < g_h; r++)
            memcpy(out + (size_t)r * g_w, (uint8_t *)buf + (size_t)r * pitch,
                   (size_t)g_w * 2);
    zdd_object_unlock(g_dest);
    return out;
}
static void dest_restore(const uint16_t *kf)
{
    if (!zdd_object_lock(g_dest)) return;
    void *buf = NULL; int32_t pitch = 0, h = 0;
    zdd_object_get_locked_info(g_dest, &buf, &pitch, &h);
    if (buf)
        for (int r = 0; r < g_h; r++)
            memcpy((uint8_t *)buf + (size_t)r * pitch, kf + (size_t)r * g_w,
                   (size_t)g_w * 2);
    zdd_object_unlock(g_dest);
}

/* Apply frame n's ops onto the dest (assumes the dest holds frame n-1). */
static void apply_frame(int n)
{
    if (_fseeki64(g_osr, g_frames[n].off, SEEK_SET) != 0) return;
    uint8_t hdr[8];
    while (fread(hdr, 1, 8, g_osr) == 8) {
        uint32_t t = osr_get_u32(hdr), len = osr_get_u32(hdr + 4);
        if (!buf_ensure(len ? len : 1)) break;
        if (len && fread(g_buf, 1, len, g_osr) != len) break;
        if (t == OSR_FRAMEBEG) continue;           /* the frame's own open */
        if (t == OSR_PRESENT)  break;              /* end of this frame */
        if (t == OSR_BLIT) {
            osr_blit b;
            if (osr_dec_blit(g_buf, len, &b)) recon_apply_blit(&g_rt, g_dest, &b);
        } else if (t == OSR_TEXT) {
            osr_text x;
            if (osr_dec_text(g_buf, len, &x)) recon_apply_text(&g_rt, g_dest, &x);
        }
        /* SHEET/FONT/BLEND already built in the index pass — skip. */
    }
    recon_release_dc(&g_rt);
}

static void maybe_keyframe(int f)
{
    if (f % KF_INTERVAL != 0) return;
    int k = f / KF_INTERVAL;
    if (k >= g_nkf || g_kf[k] != NULL) return;
    g_kf[k] = dest_snapshot();
}

/* Render frame n into the dest (with keyframe restore + forward replay). */
static void render(int n)
{
    if (n < 0) n = 0;
    if (n >= g_nframes) n = g_nframes - 1;
    if (n == g_rendered) return;

    if (n == g_rendered + 1) {                     /* forward step — one frame */
        apply_frame(n);
        maybe_keyframe(n);
        g_rendered = n;
        return;
    }
    /* seek: restore the nearest keyframe ≤ n, then replay forward. */
    int start;
    int k = n / KF_INTERVAL;
    while (k > 0 && (k >= g_nkf || g_kf[k] == NULL)) k--;
    if (k > 0 && k < g_nkf && g_kf[k] != NULL) {
        dest_restore(g_kf[k]);
        start = k * KF_INTERVAL + 1;
    } else {
        zdd_object_clear(g_dest);                  /* before-frame-0 = black */
        start = 0;
    }
    for (int f = start; f <= n; f++) {
        apply_frame(f);
        maybe_keyframe(f);
    }
    g_rendered = n;
}

/* ── index pass: build the frame index + load all dedup'd tables ──────────── */
static int build_index(const char *path)
{
    g_osr = fopen(path, "rb");
    if (!g_osr) { fprintf(stderr, "osr_view: cannot open %s\n", path); return 0; }

    uint8_t hb[OSR_HEADER_SIZE];
    if (fread(hb, 1, OSR_HEADER_SIZE, g_osr) != OSR_HEADER_SIZE) return 0;
    osr_header h;
    if (!osr_header_decode(hb, OSR_HEADER_SIZE, &h)) {
        fprintf(stderr, "osr_view: bad .osr header\n"); return 0;
    }
    g_w = h.screen_w ? h.screen_w : 640;
    g_h = h.screen_h ? h.screen_h : 480;

    int cap = 4096;
    g_frames = (frame_idx *)malloc((size_t)cap * sizeof *g_frames);

    long long off = OSR_HEADER_SIZE;
    uint8_t hdr[8];
    uint32_t pend_flip = 0, pend_tick = 0;
    long long pend_off = -1;
    while (fread(hdr, 1, 8, g_osr) == 8) {
        uint32_t t = osr_get_u32(hdr), len = osr_get_u32(hdr + 4);
        if (!buf_ensure(len ? len : 1)) break;
        if (len && fread(g_buf, 1, len, g_osr) != len) break;
        if (t == OSR_FRAMEBEG) {
            osr_framebeg fb;
            if (osr_dec_framebeg(g_buf, len, &fb)) {
                pend_off = off; pend_flip = fb.flip; pend_tick = fb.sim_tick;
            }
        } else if (t == OSR_PRESENT) {
            if (pend_off >= 0) {
                if (g_nframes >= cap) {
                    cap *= 2;
                    g_frames = (frame_idx *)realloc(g_frames, (size_t)cap * sizeof *g_frames);
                }
                g_frames[g_nframes].off  = pend_off;
                g_frames[g_nframes].flip = pend_flip;
                g_frames[g_nframes].tick = pend_tick;
                g_nframes++;
                pend_off = -1;
            }
        } else if (t == OSR_SHEET) {
            osr_sheet s; if (osr_dec_sheet(g_buf, len, &s)) recon_apply_sheet(&g_rt, &s);
        } else if (t == OSR_FONT) {
            osr_font f; if (osr_dec_font(g_buf, len, &f)) recon_apply_font(&g_rt, &f);
        } else if (t == OSR_BLEND) {
            osr_blend b; if (osr_dec_blend(g_buf, len, &b)) recon_apply_blend(&g_rt, &b);
        }
        off += 8 + len;
    }
    g_nkf = g_nframes / KF_INTERVAL + 1;
    g_kf = (uint16_t **)calloc((size_t)g_nkf, sizeof *g_kf);
    fprintf(stderr, "osr_view: %d frames, %ld sheets loaded, %dx%d\n",
            g_nframes, g_rt.n_sheets_built, g_w, g_h);
    return g_nframes > 0;
}

/* ── Win32 viewer ─────────────────────────────────────────────────────────── */
static int g_cur = 0;
static int g_playing = 0;
static HWND g_hwnd;

static void paint(HDC wdc)
{
    render(g_cur);
    /* the reconstructed frame */
    void *sdc = NULL;
    if (zdd_object_get_dc(g_dest, &sdc) && sdc)
        BitBlt(wdc, 0, 0, g_w, g_h, (HDC)sdc, 0, 0, SRCCOPY);
    if (sdc) zdd_object_release_dc(g_dest, sdc);

    /* info bar */
    RECT bar = { 0, g_h, g_w, g_h + BAR_H };
    HBRUSH bg = CreateSolidBrush(RGB(24, 24, 28));
    FillRect(wdc, &bar, bg);
    DeleteObject(bg);

    char info[256];
    snprintf(info, sizeof info, "frame %d / %d    flip=%u   tick=%u   %s",
             g_cur, g_nframes - 1, g_frames[g_cur].flip, g_frames[g_cur].tick,
             g_playing ? "[PLAY]" : "");
    SetBkMode(wdc, TRANSPARENT);
    SetTextColor(wdc, RGB(220, 220, 230));
    TextOutA(wdc, 8, g_h + 6, info, (int)strlen(info));
    TextOutA(wdc, 8, g_h + 24,
             "<-/->  +-1   PgUp/Dn +-30   Home/End   Space play   click=seek   Esc",
             64);

    /* timeline */
    int ty = g_h + 44, tx0 = 8, tw = g_w - 16, th = 6;
    RECT track = { tx0, ty, tx0 + tw, ty + th };
    HBRUSH tb = CreateSolidBrush(RGB(60, 60, 70));
    FillRect(wdc, &track, tb); DeleteObject(tb);
    int hx = tx0 + (g_nframes > 1 ? tw * g_cur / (g_nframes - 1) : 0);
    RECT hnd = { hx - 2, ty - 2, hx + 2, ty + th + 2 };
    HBRUSH hb = CreateSolidBrush(RGB(120, 200, 255));
    FillRect(wdc, &hnd, hb); DeleteObject(hb);
}

static int g_dragging = 0;

static void go(int n)
{
    if (n < 0) n = 0;
    if (n >= g_nframes) n = g_nframes - 1;
    if (n == g_cur) return;
    g_cur = n;
    InvalidateRect(g_hwnd, NULL, FALSE);
}

/* Map a client x to a frame and seek there (the whole bottom bar is the track). */
static void seek_to_x(int x)
{
    int tx0 = 8, tw = g_w - 16;
    if (tw < 1) tw = 1;
    int f = (x - tx0) * (g_nframes - 1) / tw;
    go(f);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_KEYDOWN:
        switch (wp) {
        case VK_LEFT:  go(g_cur - 1); break;
        case VK_RIGHT: go(g_cur + 1); break;
        case VK_PRIOR: go(g_cur - 30); break;
        case VK_NEXT:  go(g_cur + 30); break;
        case VK_HOME:  go(0); break;
        case VK_END:   go(g_nframes - 1); break;
        case VK_ESCAPE: PostQuitMessage(0); break;
        case VK_SPACE:
            g_playing = !g_playing;
            if (g_playing) SetTimer(hwnd, 1, 16, NULL); else KillTimer(hwnd, 1);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        return 0;
    case WM_TIMER:
        if (g_playing) {
            if (g_cur + 1 >= g_nframes) { g_playing = 0; KillTimer(hwnd, 1); }
            else go(g_cur + 1);
        }
        return 0;
    case WM_LBUTTONDOWN: {
        SetFocus(hwnd);
        int x = (short)LOWORD(lp), y = (short)HIWORD(lp);
        if (y >= g_h && g_nframes > 1) {     /* anywhere in the bottom bar seeks */
            g_dragging = 1;
            SetCapture(hwnd);
            seek_to_x(x);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_dragging && (wp & MK_LBUTTON)) seek_to_x((short)LOWORD(lp));
        return 0;
    case WM_LBUTTONUP:
        if (g_dragging) { g_dragging = 0; ReleaseCapture(); }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        paint(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
    (void)hp; (void)show;
    /* the .osr path: the command line (quoted or not), else a default. */
    char path[1024];
    if (cmd && cmd[0]) {
        char *p = cmd; while (*p == ' ' || *p == '"') p++;
        strncpy(path, p, sizeof path - 1); path[sizeof path - 1] = 0;
        size_t n = strlen(path);
        while (n && (path[n-1] == '"' || path[n-1] == ' ')) path[--n] = 0;
    } else {
        strcpy(path, "C:\\oss-osr\\retail.osr");
    }

    WNDCLASSA wc; memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = wndproc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "osr_view";
    RegisterClassA(&wc);

    RECT r = { 0, 0, 640, 480 + BAR_H };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowA("osr_view", "osr_view — Trace Studio v2",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top,
                           NULL, NULL, hi, NULL);
    if (!g_hwnd) { fprintf(stderr, "osr_view: CreateWindow failed\n"); return 1; }

    /* DDraw init (offscreen windowed) — the reconstruction backend. */
    if (!zdd_create(&g_zdd)) { fprintf(stderr, "osr_view: zdd_create failed\n"); return 1; }
    zdd_set_coop_level(g_zdd, g_hwnd, 0);
    if (!zdd_create_screen(g_zdd, 640, 480, 16, 2, 0, NULL)) {
        fprintf(stderr, "osr_view: create_screen failed\n"); return 1;
    }
    g_dest = g_zdd->primary_obj;
    recon_tables_init(&g_rt, g_zdd, 640, 480);

    if (!build_index(path)) { fprintf(stderr, "osr_view: no frames in %s\n", path); return 1; }
    zdd_object_clear(g_dest);

    /* size the client to the frame + bar now that we know w/h */
    RECT cr = { 0, 0, g_w, g_h + BAR_H };
    AdjustWindowRect(&cr, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(g_hwnd, NULL, 0, 0, cr.right - cr.left, cr.bottom - cr.top,
                 SWP_NOMOVE | SWP_NOZORDER);
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    MSG m;
    while (GetMessage(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }

    recon_tables_free(&g_rt);
    if (g_osr) fclose(g_osr);
    return 0;
}
