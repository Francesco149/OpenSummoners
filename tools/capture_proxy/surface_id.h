/* surface_id.h — the trace-studio-v2 capture-proxy DDraw-surface identity table
 * (M3c).
 *
 * The engine's 5 blit primitives (engine_hooks.h) read two IDirectDrawSurface7
 * pointers straight out of the cel / dest structures:
 *   - the DEST surface    = *(void**)(dest_arg + 0x2c)   (the backbuffer, mostly)
 *   - the SOURCE surface  = *(void**)(cel      + 0x2c)   (a decoded sprite sheet)
 * (confirmed in the blit decompiles: each does dest->Blt(&dr, src, &sr, …) via
 * vtable +0x14 — these are REAL ddraw surfaces, so we never have to wrap the COM
 * vtables to get surface identity; we just intern the raw pointers here.)
 *
 * This table assigns each surface pointer a small STABLE handle (the .osr's
 * dst_handle / a SHEET's identity anchor), so a draw record can name which
 * surface it targeted without leaking a raw heap pointer (which is meaningless
 * cross-run anyway).  Pure pointer interning — open-addressing, same shape as
 * src/render_id.c, no COM, no Win32 here.
 *
 * Surface lifetime (REVISED, ckpt 126): the engine destroys + reallocates
 * surfaces at a room swap (zdd dtor 0x5b9390), so a freed-then-reallocated
 * pointer WOULD alias a stale handle (the same staleness that hit the sheet
 * grab — the house-freeroam recon bug).  engine_hooks.h hooks the dtor and
 * calls surfid_evict(): the recycled pointer then interns as a FRESH handle.
 * Eviction tombstones the slot (open addressing — clearing would break probe
 * chains).
 */
#ifndef OSS_SURFACE_ID_H
#define OSS_SURFACE_ID_H

#include <stdint.h>
#include <string.h>

#define SURFID_SLOTS 1024u      /* power of two; the engine keeps ~dozens live */
#define SURFID_TOMB  ((const void *)1)   /* evicted slot — probe past, reusable */

typedef struct {
    const void *key;            /* surface pointer; NULL = empty slot */
    uint32_t    handle;         /* stable handle (>=1; 0 means "none") */
} surfid_entry;

static surfid_entry g_surfid[SURFID_SLOTS];
static uint32_t     g_surfid_next = 1;   /* handle 0 is reserved for "no surface" */

static unsigned surfid_hash(const void *p)
{
    /* Fibonacci mix — allocator-clustered pointers collide on low bits. */
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 16;
    x *= 0x9E3779B1u;
    x ^= x >> 13;
    return (unsigned)(x & (SURFID_SLOTS - 1u));
}

/* Intern a surface pointer → its stable handle (assigning a fresh one on first
 * sight).  Returns 0 for a NULL pointer or a full table (best-effort). */
static uint32_t surfid_get(const void *surf)
{
    if (!surf) return 0;
    unsigned h = surfid_hash(surf);
    unsigned ins = SURFID_SLOTS;
    for (unsigned i = 0; i < SURFID_SLOTS; i++) {
        unsigned s = (h + i) & (SURFID_SLOTS - 1u);
        const void *k = g_surfid[s].key;
        if (k == surf) return g_surfid[s].handle;
        if (k == SURFID_TOMB) { if (ins == SURFID_SLOTS) ins = s; continue; }
        if (k == NULL) { if (ins == SURFID_SLOTS) ins = s; break; }
    }
    if (ins == SURFID_SLOTS) return 0;  /* table full — unidentified (best-effort) */
    g_surfid[ins].key    = surf;
    g_surfid[ins].handle = g_surfid_next++;
    return g_surfid[ins].handle;
}

/* Drop a (being-destroyed) surface's handle so a future surface recycled at the
 * same pointer interns fresh.  Returns 1 if an entry was present. */
static int surfid_evict(const void *surf)
{
    if (!surf) return 0;
    unsigned h = surfid_hash(surf);
    for (unsigned i = 0; i < SURFID_SLOTS; i++) {
        unsigned s = (h + i) & (SURFID_SLOTS - 1u);
        const void *k = g_surfid[s].key;
        if (k == surf) {
            g_surfid[s].key = SURFID_TOMB;
            g_surfid[s].handle = 0;
            return 1;
        }
        if (k == NULL) return 0;
    }
    return 0;
}

#endif /* OSS_SURFACE_ID_H */
