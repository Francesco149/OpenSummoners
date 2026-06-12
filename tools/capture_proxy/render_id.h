/* render_id.h — the capture-proxy cel -> (resource_id, frame) registry (M3b).
 *
 * The retail mirror of src/render_id.c (and the Frida agent's g_render_id_map).
 * A blit names its SOURCE by the raw cel pointer (ECX / stack arg), which is
 * allocation-dependent and meaningless cross-side.  The engine's universal
 * resolver FUN_00418470 (ar_sprite_slot_frame) maps (bank slot, frame) -> cel;
 * we hook it (engine_hooks.h) and record cel -> (resource_id = slot+0x40, frame)
 * so the BLIT records carry the same LOAD-STABLE identity the port emits — the
 * key tools/render_diff.py aligns the two draw streams on.
 *
 * Single producer/consumer: every touch happens on the ENGINE MAIN THREAD inside
 * the INT3+VEH detour callbacks (the resolver registers; the blit hooks look up),
 * which run serially.  So a plain open-addressing table with no locking is safe;
 * the bg .osr drain thread never touches it.
 *
 * Cels persist for the whole run, so the table only grows (no eviction) — a few
 * thousand distinct cels across a scenario.  A fixed power-of-two table with
 * linear probing is plenty; on the (never-observed) overflow we drop the newest
 * registration and the blit just reports res=0 (unknown), never corrupting.
 */
#ifndef OSS_CAPTURE_RENDER_ID_H
#define OSS_CAPTURE_RENDER_ID_H

#include <stdint.h>

#include "proxy_log.h"

/* 16384 slots — well above a scenario's distinct-cel count; load factor stays
 * low so linear probing is short.  Key 0 = empty (a cel is never at VA 0). */
#define RID_CAP   16384u
#define RID_MASK  (RID_CAP - 1u)

typedef struct {
    uint32_t cel;     /* 0 = empty slot */
    uint16_t res;
    uint16_t frame;
} rid_entry;

static rid_entry g_rid[RID_CAP];
static int       g_rid_n;       /* distinct cels registered */

/* Fibonacci hash of the (4-aligned) pointer, folded to the table size. */
static uint32_t rid__hash(uint32_t cel)
{
    return (uint32_t)((cel * 2654435761u) >> 18) & RID_MASK;
}

/* Register / update cel -> (res, frame).  Re-resolving the same cel just
 * refreshes its frame (the last resolve wins, as on the port). */
static void rid_register(uint32_t cel, uint16_t res, uint16_t frame)
{
    if (cel == 0) return;
    uint32_t i = rid__hash(cel);
    for (uint32_t probe = 0; probe < RID_CAP; ++probe) {
        rid_entry *e = &g_rid[i];
        if (e->cel == 0) {
            if (g_rid_n + 1 >= (int)(RID_CAP - (RID_CAP >> 3))) {
                /* >87% full — refuse to grow past the probe-friendly band */
                return;
            }
            e->cel = cel; e->res = res; e->frame = frame;
            ++g_rid_n;
            return;
        }
        if (e->cel == cel) { e->res = res; e->frame = frame; return; }
        i = (i + 1) & RID_MASK;
    }
}

/* Look up a cel's identity.  Returns 1 + fills res/frame on hit, 0 on miss
 * (an un-resolved cel — the blit reports res=frame=0). */
static int rid_lookup(uint32_t cel, uint16_t *res, uint16_t *frame)
{
    if (cel == 0) return 0;
    uint32_t i = rid__hash(cel);
    for (uint32_t probe = 0; probe < RID_CAP; ++probe) {
        rid_entry *e = &g_rid[i];
        if (e->cel == 0) return 0;
        if (e->cel == cel) { *res = e->res; *frame = e->frame; return 1; }
        i = (i + 1) & RID_MASK;
    }
    return 0;
}

#endif /* OSS_CAPTURE_RENDER_ID_H */
