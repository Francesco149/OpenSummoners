/* sheet_grab.h — the trace-studio-v2 capture-proxy SOURCE-pixel grab (M3c-2).
 *
 * A blit reads its source pixels from the cel's IDirectDrawSurface7 (cel+0x2c).
 * The .osr replayer (M4) needs those pixels to reconstruct the frame, so the
 * first time we see a given source surface we LOCK it read-only, fingerprint the
 * decoded bytes, and emit ONE dedup'd SHEET record (the BLIT then references it
 * by dhash).  Grabbing is once-per-surface — the engine's sprite-bank sheets are
 * decoded at load and are stable thereafter — so the per-blit cost is just a
 * cache hit returning the cached dhash.
 *
 * STALENESS (the house-freeroam recon bug, ckpt 126): "once-per-surface" is only
 * sound while the surface LIVES.  The engine destroys + reallocates sheet
 * surfaces at a room swap (zdd dtor 0x5b9390 Releases +0x2c), and a NEW surface
 * allocated at a recycled pointer would hit the old cache entry → the blit
 * records the OLD sheet's dhash (the reconstructor then draws white dialog
 * panels / the wrong sprite).  engine_hooks.h hooks the dtor and calls
 * sheet_grab_evict() so a recycled pointer re-grabs.  Eviction uses TOMBSTONES
 * (open addressing — clearing a key would break probe chains).  Re-grabs of
 * identical content skip re-EMISSION via the emitted-dhash set (the reader
 * dedups by dhash anyway; this just keeps the file small).
 *
 * dhash mirrors the port's src/asset_register.c fingerprint SHAPE (FNV-1a seeded
 * with width/height/bitcount, then the raw pixel bytes) so the two sides hash
 * EQUAL iff the decoded source pixels + geometry are identical.  They generally
 * will NOT match byte-for-byte cross-side (the retail surface's Lock pitch +
 * native pixfmt differ from the port's packed decode), so a dhash mismatch is a
 * legitimate render_diff "[decode]" signal, not a join failure — (resource_id,
 * frame) remains the primary identity.  Reconciling the two decodes byte-exact
 * is a follow-up (PORT-DEBT(osr-sheet-dhash-xside)).
 *
 * Locking discipline: the source is a STATIC decoded sheet (not the backbuffer
 * being drawn to), Lock'd DDLOCK_READONLY|DDLOCK_WAIT right at the blit's onEnter
 * (before the real Blt reads it) and Unlock'd immediately — no surface the engine
 * is concurrently writing is ever locked.  Single-threaded: every call is on the
 * engine thread inside a blit detour.
 */
#ifndef OSS_SHEET_GRAB_H
#define OSS_SHEET_GRAB_H

#include <stdint.h>
#include <ddraw.h>

#include "proxy_log.h"
#include "osr_writer.h"
#include "engine_pixfmt.h"   /* eh_pixfmt_of — shared with engine_hooks.h */

/* ── FNV-1a (32-bit), byte-identical to src/render_id.c ───────────────────── */
#define SG_FNV1A_SEED  2166136261u
#define SG_FNV1A_PRIME 16777619u

static uint32_t sg_hash_seed(uint32_t seed, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = seed;
    for (size_t i = 0; i < len; i++) { h ^= p[i]; h *= SG_FNV1A_PRIME; }
    return h;
}

/* ── ptr → captured-dhash cache (dedup: grab each LIVE surface once) ──────── */
#define SG_SLOTS 4096u
#define SG_TOMB  ((const void *)1)   /* evicted slot — probe past, reuse on insert */

typedef struct { const void *key; uint32_t dhash; } sg_entry;
static sg_entry g_sg[SG_SLOTS];

static unsigned sg_slot(const void *p)
{
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 16; x *= 0x9E3779B1u; x ^= x >> 13;
    return (unsigned)(x & (SG_SLOTS - 1u));
}

/* Drop a (being-destroyed) surface's cache entry so a future surface recycled
 * at the same pointer re-grabs.  Returns 1 if an entry was actually present
 * (i.e. the surface had been blitted from) — the caller's logging signal. */
static int sheet_grab_evict(const void *surf)
{
    if (!surf) return 0;
    unsigned h = sg_slot(surf);
    for (unsigned i = 0; i < SG_SLOTS; i++) {
        unsigned s = (h + i) & (SG_SLOTS - 1u);
        const void *k = g_sg[s].key;
        if (k == surf) { g_sg[s].key = SG_TOMB; g_sg[s].dhash = 0; return 1; }
        if (k == NULL) return 0;
    }
    return 0;
}

/* ── emitted-dhash set (skip re-EMITTING identical content after an evict) ── */
static uint32_t g_sg_emitted[SG_SLOTS];   /* dhash values; 0 = empty */

static int sg_emitted_test_and_set(uint32_t dhash)
{
    if (dhash == 0) return 0;
    unsigned h = (unsigned)((dhash * 0x9E3779B1u) & (SG_SLOTS - 1u));
    for (unsigned i = 0; i < SG_SLOTS; i++) {
        unsigned s = (h + i) & (SG_SLOTS - 1u);
        if (g_sg_emitted[s] == dhash) return 1;
        if (g_sg_emitted[s] == 0) { g_sg_emitted[s] = dhash; return 0; }
    }
    return 1;   /* set full — claim "already emitted" (best-effort, never grows) */
}

/* Cap a single SHEET's pixel payload so a pathological surface can't blow the
 * ring (the backbuffer is ~600 KB at 16bpp; sprite sheets are far smaller). */
#define SG_MAX_BYTES (8u * 1024u * 1024u)

/* Grab (or recall) the source surface's pixels → its dhash; emit the SHEET on
 * the first sighting.  res/frame are a representative identity for the SHEET
 * record.  Returns 0 if surf is NULL or the grab fails (BLIT.dhash stays 0). */
static uint32_t sheet_capture_source(void *surf, uint16_t res, uint16_t frame)
{
    if (!surf) return 0;

    unsigned h = sg_slot(surf);
    unsigned ins = SG_SLOTS;
    for (unsigned i = 0; i < SG_SLOTS; i++) {
        unsigned s = (h + i) & (SG_SLOTS - 1u);
        const void *k = g_sg[s].key;
        if (k == surf) return g_sg[s].dhash;       /* cached — the hot path */
        if (k == SG_TOMB) { if (ins == SG_SLOTS) ins = s; continue; }
        if (k == NULL) { if (ins == SG_SLOTS) ins = s; break; }
    }
    if (ins == SG_SLOTS) return 0;                 /* cache full (never seen) */

    LPDIRECTDRAWSURFACE7 s = (LPDIRECTDRAWSURFACE7)surf;
    DDSURFACEDESC2 dd;
    memset(&dd, 0, sizeof(dd));
    dd.dwSize = sizeof(dd);
    HRESULT hr = IDirectDrawSurface7_Lock(
        s, NULL, &dd, DDLOCK_READONLY | DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,
        NULL);
    if (FAILED(hr) || !dd.lpSurface) {
        /* Cache the miss as dhash 0 so we don't re-Lock a problem surface each
         * blit; res stays the primary identity. */
        g_sg[ins].key = surf; g_sg[ins].dhash = 0;
        return 0;
    }

    uint32_t w = dd.dwWidth, ht = dd.dwHeight;
    uint16_t bitcount = (uint16_t)dd.ddpfPixelFormat.dwRGBBitCount;
    uint32_t pitch = (uint32_t)dd.lPitch;
    size_t bytes = (size_t)pitch * ht;
    if (bytes > SG_MAX_BYTES) bytes = SG_MAX_BYTES;

    /* dhash: mirror the port's seed order (w:u32, h:u32, bitcount:u16, pixels). */
    uint32_t seed = sg_hash_seed(SG_FNV1A_SEED, &w, sizeof(w));
    seed = sg_hash_seed(seed, &ht, sizeof(ht));
    seed = sg_hash_seed(seed, &bitcount, sizeof(bitcount));
    uint32_t dhash = sg_hash_seed(seed, dd.lpSurface, bytes);

    if (!sg_emitted_test_and_set(dhash)) {
        osr_sheet sh;
        memset(&sh, 0, sizeof(sh));
        sh.dhash    = dhash;
        sh.res      = res;
        sh.frame    = frame;
        sh.w        = (uint16_t)w;
        sh.h        = (uint16_t)ht;
        sh.pitch    = pitch;
        sh.pixfmt   = eh_pixfmt_of(&dd.ddpfPixelFormat);
        sh.codec    = 0;             /* raw; miniz is PORT-DEBT(osr-sheet-compression) */
        sh.byte_len = (uint32_t)bytes;
        sh.bytes    = (const uint8_t *)dd.lpSurface;
        osr_w_sheet(&sh);            /* streams pixels into the ring under the cs */
    }

    IDirectDrawSurface7_Unlock(s, NULL);

    g_sg[ins].key = surf; g_sg[ins].dhash = dhash;
    return dhash;
}

#endif /* OSS_SHEET_GRAB_H */
