/* osr_writer.h — the trace-studio-v2 capture-proxy .osr writer (M3a).
 *
 * The engine MAIN THREAD (inside the INT3+VEH detour callbacks) only ever does a
 * cheap append: lock, ensure-capacity, memcpy a small encoded record into a ring,
 * unlock.  A dedicated BACKGROUND THREAD drains the ring to the .osr file on a
 * timer and fflushes after every drain, so:
 *   - disk latency never stalls the engine thread (the plan's hot-path rule), and
 *   - the harness HARD-KILLS the game (Stop-Process), so there is no clean
 *     shutdown — but because the format is append-only and the bg thread flushes
 *     every ~30 ms, whatever is on disk at kill time is a valid .osr up to the
 *     last drain (osr_format.h's truncated-tail recovery covers the rest).
 *
 * Storage discipline (the plan): the .osr lives on native NTFS (C:\oss-osr\…),
 * never the WSL 9p mount — WSL only READS it via /mnt/c off the hot path.
 *
 * M3a emits the cheap records the boot already produces (FRAMEBEG / PRESENT /
 * ANCHOR / SEED).  The bulky SHEET + the BLIT/TEXT draw records land with the COM
 * wrap + blit detours (M3b+); they will share this same ring (a SHEET is large but
 * dedup'd by dhash → written once, so the ring stays small).
 */
#ifndef OSS_OSR_WRITER_H
#define OSS_OSR_WRITER_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "osr_format.h"
#include "proxy_log.h"
#include "proxy_config.h"

/* ── double-buffer ring + bg-thread state ─────────────────────────────────── */
typedef struct {
    uint8_t *buf[2];        /* two growable byte buffers */
    size_t   cap[2];
    size_t   len[2];
    int      active;        /* engine thread appends to buf[active] */
    CRITICAL_SECTION cs;
    HANDLE   thread;
    volatile LONG run;
    FILE    *file;
    int      enabled;
    LONG     dropped;       /* records dropped on OOM (never stall the engine) */
} osr_writer;

static osr_writer g_osr;

#define OSR_RING_INIT_CAP (64u * 1024u)
#define OSR_DRAIN_MS      30

/* Append `n` bytes to the active ring buffer (engine thread; cs held by caller). */
static int osr__ring_put(const uint8_t *data, size_t n)
{
    int a = g_osr.active;
    if (g_osr.len[a] + n > g_osr.cap[a]) {
        size_t nc = g_osr.cap[a] ? g_osr.cap[a] : OSR_RING_INIT_CAP;
        while (nc < g_osr.len[a] + n) nc *= 2;
        uint8_t *nb = (uint8_t *)realloc(g_osr.buf[a], nc);
        if (!nb) { InterlockedIncrement(&g_osr.dropped); return 0; }
        g_osr.buf[a] = nb;
        g_osr.cap[a] = nc;
    }
    memcpy(g_osr.buf[a] + g_osr.len[a], data, n);
    g_osr.len[a] += n;
    return 1;
}

/* Drain the inactive buffer to disk (bg thread).  Swaps under the lock so the
 * engine keeps appending to the other buffer with no contention during I/O. */
static void osr__drain(void)
{
    int drain_idx;
    EnterCriticalSection(&g_osr.cs);
    drain_idx = g_osr.active;
    g_osr.active ^= 1;                 /* engine now appends to the other half */
    LeaveCriticalSection(&g_osr.cs);

    size_t n = g_osr.len[drain_idx];
    if (n && g_osr.file) {
        fwrite(g_osr.buf[drain_idx], 1, n, g_osr.file);
        fflush(g_osr.file);            /* durable up to here even on a hard kill */
    }
    g_osr.len[drain_idx] = 0;          /* bg thread is the only resetter */
}

static DWORD WINAPI osr__bg_thread(LPVOID arg)
{
    (void)arg;
    while (InterlockedCompareExchange(&g_osr.run, 1, 1)) {
        Sleep(OSR_DRAIN_MS);
        osr__drain();
    }
    osr__drain();                      /* final best-effort flush */
    return 0;
}

/* ── public append API (engine thread; no-op if capture disabled) ─────────── */
static void osr__emit(const uint8_t *rec, size_t n)
{
    if (!g_osr.enabled || n == 0) return;
    EnterCriticalSection(&g_osr.cs);
    osr__ring_put(rec, n);
    LeaveCriticalSection(&g_osr.cs);
}

static void osr_w_framebeg(uint32_t flip, uint32_t sim_tick, uint32_t anchor_id)
{
    uint8_t b[8 + 12];
    osr__emit(b, osr_enc_framebeg(b, sizeof(b), flip, sim_tick, anchor_id));
}
static void osr_w_present(uint32_t mode, uint32_t src_handle)
{
    uint8_t b[8 + 8];
    osr__emit(b, osr_enc_present(b, sizeof(b), mode, src_handle));
}
static void osr_w_seed(uint32_t flip, uint32_t before, uint32_t value)
{
    uint8_t b[8 + 12];
    osr__emit(b, osr_enc_seed(b, sizeof(b), flip, before, value));
}
static void osr_w_anchor(uint32_t flip, uint32_t sim_tick, uint32_t rng,
                         const char *name)
{
    uint8_t b[8 + 16 + 64];
    size_t n = osr_enc_anchor(b, sizeof(b), flip, sim_tick, rng, name);
    osr__emit(b, n);
}
static void osr_w_blit(const osr_blit *blit)
{
    uint8_t b[8 + OSR_BLIT_PAYLOAD];
    osr__emit(b, osr_enc_blit(b, sizeof(b), blit));
}

/* ── lifecycle ───────────────────────────────────────────────────────────── */
static void osr_writer_start(void)
{
    if (!g_cfg.osr_enable) {
        proxy_logf("[osr] capture disabled (OSS_OSR=0)");
        return;
    }
    InitializeCriticalSection(&g_osr.cs);
    g_osr.active = 0;
    g_osr.dropped = 0;

    /* Open the .osr on native NTFS (storage discipline). */
    g_osr.file = fopen(g_cfg.osr_path, "wb");
    if (!g_osr.file) {
        proxy_logf("[osr] FATAL: fopen(%s) failed err=%d", g_cfg.osr_path, errno);
        return;
    }

    /* Header: the proxy is always the RETAIL side; screen dims/pixfmt corrected
     * from the real DDSURFACEDESC2 at M3b (UNKNOWN/640x480 for now). */
    osr_header h;
    memset(&h, 0, sizeof(h));
    h.side     = OSR_SIDE_RETAIL;
    h.pixfmt   = OSR_PIXFMT_UNKNOWN;
    h.screen_w = 640;
    h.screen_h = 480;
    h.seed     = g_cfg.seed_value;
    h.flags    = (g_cfg.turbo    ? OSR_FLAG_TURBO    : 0) |
                 (g_cfg.lockstep ? OSR_FLAG_LOCKSTEP : 0) |
                 (g_cfg.seed_pin ? OSR_FLAG_SEED_PIN : 0);
    lstrcpynA(h.scenario, g_cfg.scenario, (int)sizeof(h.scenario));

    uint8_t hb[OSR_HEADER_SIZE];
    osr_header_encode(hb, sizeof(hb), &h);
    fwrite(hb, 1, sizeof(hb), g_osr.file);
    fflush(g_osr.file);

    g_osr.enabled = 1;
    g_osr.run = 1;
    g_osr.thread = CreateThread(NULL, 0, osr__bg_thread, NULL, 0, NULL);
    proxy_logf("[osr] capture started -> %s (scenario='%s' seed=0x%lx)",
               g_cfg.osr_path, g_cfg.scenario, (unsigned long)g_cfg.seed_value);
}

static void osr_writer_stop(void)
{
    if (!g_osr.enabled) return;
    InterlockedExchange(&g_osr.run, 0);
    if (g_osr.thread) {
        WaitForSingleObject(g_osr.thread, 1000);
        CloseHandle(g_osr.thread);
        g_osr.thread = NULL;
    }
    if (g_osr.file) { fflush(g_osr.file); fclose(g_osr.file); g_osr.file = NULL; }
    if (g_osr.dropped)
        proxy_logf("[osr] WARNING: %ld records dropped (OOM)", g_osr.dropped);
    proxy_logf("[osr] capture stopped");
    g_osr.enabled = 0;
}

#endif /* OSS_OSR_WRITER_H */
