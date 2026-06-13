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
    osr_header hdr;         /* the header as last written (for the M3c re-stamp) */
    volatile LONG hdr_fixup;/* set by the engine thread; bg thread re-stamps once */
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

/* Re-stamp the 64-byte header at offset 0 (bg thread only — it owns the FILE*).
 * Done once after the first surface desc corrects the UNKNOWN/640x480 placeholder
 * (M3c).  Append-only otherwise; this is the sole non-sequential write, and the
 * reader re-reads the header from offset 0, so a mid-stream re-stamp is safe. */
static void osr__apply_header_fixup(void)
{
    if (!InterlockedCompareExchange(&g_osr.hdr_fixup, 0, 1)) return;  /* not pending */
    if (!g_osr.file) return;
    uint8_t hb[OSR_HEADER_SIZE];
    osr_header_encode(hb, sizeof(hb), &g_osr.hdr);
    long pos = ftell(g_osr.file);
    fseek(g_osr.file, 0, SEEK_SET);
    fwrite(hb, 1, sizeof(hb), g_osr.file);
    if (pos >= 0) fseek(g_osr.file, pos, SEEK_SET);
    else          fseek(g_osr.file, 0, SEEK_END);
    fflush(g_osr.file);
    proxy_logf("[osr] header re-stamped: %ux%u pixfmt=%u",
               g_osr.hdr.screen_w, g_osr.hdr.screen_h, g_osr.hdr.pixfmt);
}

static DWORD WINAPI osr__bg_thread(LPVOID arg)
{
    (void)arg;
    while (InterlockedCompareExchange(&g_osr.run, 1, 1)) {
        Sleep(OSR_DRAIN_MS);
        osr__drain();
        osr__apply_header_fixup();
    }
    osr__drain();                      /* final best-effort flush */
    osr__apply_header_fixup();
    return 0;
}

/* Engine thread: record the corrected screen pixfmt/dims + flag the re-stamp.
 * The bg thread applies it (above) on its next tick — no cross-thread fseek. */
static void osr_w_fixup_header(uint8_t pixfmt, uint16_t w, uint16_t h)
{
    if (!g_osr.enabled) return;
    g_osr.hdr.pixfmt   = pixfmt;
    if (w) g_osr.hdr.screen_w = w;
    if (h) g_osr.hdr.screen_h = h;
    InterlockedExchange(&g_osr.hdr_fixup, 1);
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
static void osr_w_clear(uint32_t seq, uint32_t dst_handle, uint32_t value)
{
    uint8_t b[8 + 12];
    osr__emit(b, osr_enc_clear(b, sizeof(b), seq, dst_handle, value));
}
static void osr_w_blit(const osr_blit *blit)
{
    uint8_t b[8 + OSR_BLIT_PAYLOAD];
    osr__emit(b, osr_enc_blit(b, sizeof(b), blit));
}
/* SHEET — large + variable: the pixel bytes are streamed straight into the ring
 * after the framed prefix (one copy, no temp buffer).  The caller's `bytes` is a
 * locked-surface pointer valid only until Unlock, so the copy MUST finish before
 * the caller unlocks — it does (we hold the cs synchronously here). */
static void osr_w_sheet(const osr_sheet *s)
{
    if (!g_osr.enabled) return;
    uint8_t pre[8 + OSR_SHEET_HDR];
    size_t pn = osr_enc_sheet_prefix(pre, sizeof(pre), s);
    EnterCriticalSection(&g_osr.cs);
    if (osr__ring_put(pre, pn) && s->byte_len && s->bytes)
        osr__ring_put(s->bytes, s->byte_len);
    LeaveCriticalSection(&g_osr.cs);
}
/* SNAP — large + variable, same streaming discipline as SHEET: the caller's
 * `bytes` is a locked-surface pointer valid only until Unlock, so the copy MUST
 * finish before the caller unlocks — it does (cs held synchronously here). */
static void osr_w_snap(const osr_snap *s)
{
    if (!g_osr.enabled) return;
    uint8_t pre[8 + OSR_SNAP_HDR];
    size_t pn = osr_enc_snap_prefix(pre, sizeof(pre), s);
    EnterCriticalSection(&g_osr.cs);
    if (osr__ring_put(pre, pn) && s->byte_len && s->bytes)
        osr__ring_put(s->bytes, s->byte_len);
    LeaveCriticalSection(&g_osr.cs);
}
/* BLEND — variable: the 44-byte prefix + up to 3 channel LUTs (dedup'd; written
 * once per unique blend descriptor, only a few dozen across a scenario, so a
 * static encode buffer covers the largest mode-1 descriptor).  Called only on the
 * engine thread inside a blit detour (single-threaded), so the static buffer is
 * safe; osr__emit serialises the ring write. */
static void osr_w_blend(const osr_blend *b)
{
    static uint8_t buf[8 + OSR_BLEND_HDR + 3u * 8192u];
    size_t n = osr_enc_blend(buf, sizeof(buf), b);
    osr__emit(buf, n);
}
/* FONT — fixed 64-byte payload (dedup'd; written once per unique HFONT). */
static void osr_w_font(const osr_font *f)
{
    uint8_t b[8 + OSR_FONT_PAYLOAD];
    osr__emit(b, osr_enc_font(b, sizeof(b), f));
}
/* TEXT — one TextOutA op; the string is short (the engine TextOutA's per glyph /
 * short run), so a stack buffer covers it.  The caller clamps str_len; drop a
 * pathologically long string rather than overrun. */
#define OSR_W_TEXT_MAX 480u
static void osr_w_text(const osr_text *t)
{
    if (t->str_len > OSR_W_TEXT_MAX) return;
    uint8_t b[8 + OSR_TEXT_HDR + OSR_W_TEXT_MAX];
    osr__emit(b, osr_enc_text(b, sizeof(b), t));
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

    /* Header: the proxy is always the RETAIL side; screen dims/pixfmt are the
     * UNKNOWN/640x480 placeholder until the first surface desc re-stamps it
     * (M3c, osr_w_fixup_header).  Kept in g_osr.hdr so the bg thread can rewrite
     * offset 0 in place. */
    osr_header *h = &g_osr.hdr;
    memset(h, 0, sizeof(*h));
    h->side     = OSR_SIDE_RETAIL;
    h->pixfmt   = OSR_PIXFMT_UNKNOWN;
    h->screen_w = 640;
    h->screen_h = 480;
    h->seed     = g_cfg.seed_value;
    h->flags    = (g_cfg.turbo    ? OSR_FLAG_TURBO    : 0) |
                  (g_cfg.lockstep ? OSR_FLAG_LOCKSTEP : 0) |
                  (g_cfg.seed_pin ? OSR_FLAG_SEED_PIN : 0);
    lstrcpynA(h->scenario, g_cfg.scenario, (int)sizeof(h->scenario));

    uint8_t hb[OSR_HEADER_SIZE];
    osr_header_encode(hb, sizeof(hb), h);
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
