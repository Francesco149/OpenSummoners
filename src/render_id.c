/* Stable cross-side sprite identity + decoded-sheet fingerprint — see
 * render_id.h.  Mirrors openrecet's src/d3d_tex_names.c open-addressing shape. */

#include "render_id.h"

#include <string.h>

/* ── cel-pointer → render_id table ──────────────────────────────────────────
 * Power-of-two for the mask.  The engine keeps a few hundred live cels (the
 * decoded frame surfaces across all active banks); 4096 slots stay well under
 * half full so probe chains stay short. */
#define RID_SLOTS 4096u
#define RID_TOMBSTONE ((const void *)(uintptr_t)1)

struct rid_entry {
    const void *key;        /* NULL = empty, RID_TOMBSTONE = deleted */
    render_id   id;
};

static struct rid_entry g_tab[RID_SLOTS];

static unsigned rid_hash(const void *p)
{
    /* Fibonacci mix — allocator-clustered pointers collide on low bits. */
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 16;
    x *= 0x9E3779B1u;
    x ^= x >> 13;
    return (unsigned)(x & (RID_SLOTS - 1u));
}

void render_id_register(const void *cel, uint16_t resource_id,
                        uint16_t frame, uint32_t dhash)
{
    if (!cel || cel == RID_TOMBSTONE) return;

    unsigned h = rid_hash(cel);
    int ins = -1;                       /* first reusable slot in the chain */
    for (unsigned i = 0; i < RID_SLOTS; i++) {
        unsigned s = (h + i) & (RID_SLOTS - 1u);
        const void *k = g_tab[s].key;
        if (k == cel) { ins = (int)s; break; }              /* overwrite */
        if (k == RID_TOMBSTONE) { if (ins < 0) ins = (int)s; continue; }
        if (k == NULL) { if (ins < 0) ins = (int)s; break; } /* chain end */
    }
    if (ins < 0) return;                /* table full — drop (best-effort) */

    g_tab[ins].key = cel;
    g_tab[ins].id.resource_id = resource_id;
    g_tab[ins].id.frame       = frame;
    g_tab[ins].id.dhash       = dhash;
}

int render_id_lookup(const void *cel, render_id *out)
{
    if (!cel || cel == RID_TOMBSTONE) return 0;

    unsigned h = rid_hash(cel);
    for (unsigned i = 0; i < RID_SLOTS; i++) {
        unsigned s = (h + i) & (RID_SLOTS - 1u);
        const void *k = g_tab[s].key;
        if (k == cel) { if (out) *out = g_tab[s].id; return 1; }
        if (k == NULL) return 0;        /* chain end — not present */
    }
    return 0;
}

void render_id_forget(const void *cel)
{
    if (!cel || cel == RID_TOMBSTONE) return;

    unsigned h = rid_hash(cel);
    for (unsigned i = 0; i < RID_SLOTS; i++) {
        unsigned s = (h + i) & (RID_SLOTS - 1u);
        const void *k = g_tab[s].key;
        if (k == cel) { g_tab[s].key = RID_TOMBSTONE; return; }
        if (k == NULL) return;
    }
}

void render_id_reset(void)
{
    memset(g_tab, 0, sizeof g_tab);
}

/* ── resource_id → decoded-sheet fingerprint ────────────────────────────────
 * resource_id is a 16-bit PE id; a direct 65536-entry table would be 256 KiB,
 * which is fine for BSS but wasteful when only ~74 banks decode in-game.  Use a
 * small open-addressing table keyed on the (nonzero) resource_id instead. */
#define RID_SHEET_SLOTS 1024u

struct rid_sheet_entry {
    uint16_t resource_id;   /* 0 = empty (resource_id 0 is never a real bank) */
    uint32_t dhash;
};

static struct rid_sheet_entry g_sheets[RID_SHEET_SLOTS];

static unsigned rid_sheet_hash(uint16_t res)
{
    unsigned x = (unsigned)res * 2654435761u;   /* Knuth multiplicative */
    return x & (RID_SHEET_SLOTS - 1u);
}

void render_id_set_sheet_hash(uint16_t resource_id, uint32_t dhash)
{
    if (resource_id == 0) return;
    unsigned h = rid_sheet_hash(resource_id);
    for (unsigned i = 0; i < RID_SHEET_SLOTS; i++) {
        unsigned s = (h + i) & (RID_SHEET_SLOTS - 1u);
        uint16_t k = g_sheets[s].resource_id;
        if (k == resource_id || k == 0) {
            g_sheets[s].resource_id = resource_id;
            g_sheets[s].dhash       = dhash;
            return;
        }
    }
    /* table full — drop (best-effort) */
}

uint32_t render_id_sheet_hash(uint16_t resource_id)
{
    if (resource_id == 0) return 0;
    unsigned h = rid_sheet_hash(resource_id);
    for (unsigned i = 0; i < RID_SHEET_SLOTS; i++) {
        unsigned s = (h + i) & (RID_SHEET_SLOTS - 1u);
        uint16_t k = g_sheets[s].resource_id;
        if (k == resource_id) return g_sheets[s].dhash;
        if (k == 0) return 0;
    }
    return 0;
}

void render_id_reset_sheets(void)
{
    memset(g_sheets, 0, sizeof g_sheets);
}

/* ── FNV-1a (32-bit) ────────────────────────────────────────────────────── */

uint32_t render_id_hash_seed(uint32_t seed, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = seed;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

uint32_t render_id_hash(const void *data, size_t len)
{
    return render_id_hash_seed(RENDER_ID_FNV1A_SEED, data, len);
}
