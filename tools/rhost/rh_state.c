/* rh_state.c — state hashing, dumps, shadow execution.
 *
 * "State" = retail .data/.bss (RT_DATA_LO..HI from the PE; EN-old 0x854000..0x8ab2e0) + the
 * committed heap arena. (Ported from openrecet tools/rhost.)
 * full hash     : every byte.
 * semantic hash : every dword except those at VOLATILE ADDRESSES: the game HWND +
 *                 <rundir>\volatile.txt (slots that hold system-DLL/COM pointers,
 *                 calibrated by `rhost.py calibrate` from a 2-run dump diff).
 *                 Masking is by ADDRESS only. Masking by VALUE ("points into a
 *                 DLL mapping") was tried and is inherently flaky: heap pixel/audio
 *                 data is effectively random bits, so ~10% of it lands in whatever
 *                 ranges DLLs occupy in THAT run (masked 580k dwords, differently
 *                 per run). The per-page value map is still written into dumps so
 *                 the offline differ can classify a diff as "external pointer".
 * Dump (.rhs)   : "RHS2" frame x87cw nranges {lo,hi}* bytes* extpage-bitmap(128K)
 *                 nvol vol* — read by tools/rhost/rhost.py diff. */
#include "rhost.h"

#define MAXR 4096
#define MAXVOL 65536
static uint32_t g_vol[MAXVOL]; static int g_nvol, g_vol_sorted;
static uint8_t g_volpg[1u << 17];        /* pages holding any volatile address */
static uint8_t g_ext[1u << 17];          /* 1 bit per 4 KB page of the 4 GB space */

void rh_state_volatile_add(uint32_t va)
{
    if (g_nvol >= MAXVOL) rh_fatal("volatile.txt: more than %d addresses", MAXVOL);   /* (was a silent 8192 cap: unmasked tail) */
    g_vol[g_nvol++] = va & ~3u; g_vol_sorted = 0;
    g_volpg[va >> 15] |= (uint8_t)(1u << ((va >> 12) & 7));
}
static int cmp_u32(const void *a, const void *b) { uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b; return x < y ? -1 : x > y; }
static int is_vol(uint32_t a)
{
    if (!(g_volpg[a >> 15] >> ((a >> 12) & 7) & 1)) return 0;
    if (!g_vol_sorted) { qsort(g_vol, g_nvol, 4, cmp_u32); g_vol_sorted = 1; }
    int lo = 0, hi = g_nvol - 1;
    while (lo <= hi) { int m = (lo + hi) / 2; if (g_vol[m] == a) return 1; if (g_vol[m] < a) lo = m + 1; else hi = m - 1; }
    return 0;
}

static void build_ext_map(void)
{
    memset(g_ext, 0, sizeof g_ext);
    uint32_t a = 0x10000;
    for (;;) {
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)(uintptr_t)a, &mi, sizeof mi)) break;
        uint32_t lo = (uint32_t)(uintptr_t)mi.BaseAddress, sz = (uint32_t)mi.RegionSize, hi = lo + sz;
        /* classification only (dumps): any non-free mapping this process does not lay
         * out deterministically — committed or reserved (a slot can point into a DLL
         * heap segment's reserved tail or a just-decommitted buffer) */
        if (mi.State != MEM_FREE && !rh_is_det_addr(lo))
            for (uint32_t p = lo >> 12; p < (hi ? hi >> 12 : 0x100000u); p++) g_ext[p >> 3] |= (uint8_t)(1u << (p & 7));
        if (!hi || hi <= a) break;
        a = hi;
    }
    /* the envelope: a gap of up to 4 MB between external mappings counts as external too (never a page rhost lays
     * out itself).  Wine places a big system block (2 MB, a thread's heap segment) in whichever gap is free when the
     * thread asks — two layouts across runs — and a plain integer (a byte tuple like 0x09090908) then fell inside
     * the external map in one run only: semv differed from rh-combat f13200 to f51000 (the wine flake) */
    uint32_t last = 0;
    for (uint32_t pg = 0x10; pg < 0x100000u; pg++) {
        if (!(g_ext[pg >> 3] >> (pg & 7) & 1)) continue;
        if (last && pg > last && pg - last <= 0x400)
            for (uint32_t q = last; q < pg; q++)
                if (!rh_is_det_addr(q << 12)) g_ext[q >> 3] |= (uint8_t)(1u << (q & 7));
        last = pg + 1;
    }
    /* ... and the whole system zone between rhost's image and its arena (0x05c00000..0x20000000): wine's heaps grow
     * there to a high-water mark that varies by thread timing (0x0ac8.. vs 0x0bc5.. in two rh-combat hybrids), so a
     * byte tuple like 0x0b0a0908 fell inside one run's map only.  Masked in every run, it hashes alike. */
    for (uint32_t pg = 0x05c00; pg < 0x20000; pg++)
        if (!rh_is_det_addr(pg << 12)) g_ext[pg >> 3] |= (uint8_t)(1u << (pg & 7));
}
/* --log-ext: where do masked values point? (largest external committed regions) */
void rh_state_ext_report(void)
{
    struct { uint32_t lo, sz, type, hits; } top[12] = {0};
    static uint32_t r[MAXR][2];
    build_ext_map();
    uint32_t a = 0x10000;
    for (;;) {
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)(uintptr_t)a, &mi, sizeof mi)) break;
        uint32_t lo = (uint32_t)(uintptr_t)mi.BaseAddress, sz = (uint32_t)mi.RegionSize;
        if (mi.State == MEM_COMMIT && !rh_is_det_addr(lo)) {
            int k = 11; if (sz > top[k].sz) { top[k].lo = lo; top[k].sz = sz; top[k].type = mi.Type; top[k].hits = 0;
                while (k && top[k].sz > top[k - 1].sz) { __typeof__(top[0]) t = top[k]; top[k] = top[k - 1]; top[k - 1] = t; k--; } }
        }
        if (!(lo + sz) || lo + sz <= a) break;
        a = lo + sz;
    }
    int n = 1 + rh_heap_ranges(r + 1, MAXR - 1); r[0][0] = RT_DATA_LO; r[0][1] = RT_DATA_HI & ~3u;
    for (int i = 0; i < n; i++)
        for (const uint32_t *p = (const uint32_t *)(uintptr_t)r[i][0]; p < (const uint32_t *)(uintptr_t)r[i][1]; p++)
            for (int k = 0; k < 12; k++) if (*p >= top[k].lo && *p - top[k].lo < top[k].sz) { top[k].hits++; break; }
    for (int k = 0; k < 12 && top[k].sz; k++)
        rh_log("ext region %08x +%7uK type=%s masked-hits=%u\n", top[k].lo, top[k].sz >> 10,
               top[k].type == MEM_IMAGE ? "image" : top[k].type == MEM_MAPPED ? "mapped" : "private", top[k].hits);
}
static int is_ext(uint32_t v) { return v >= 0x10000u && (g_ext[v >> 15] >> ((v >> 12) & 7) & 1); }

static int state_ranges(uint32_t (*r)[2])
{
    r[0][0] = RT_DATA_LO; r[0][1] = RT_DATA_HI & ~3u;
    return 1 + rh_heap_ranges(r + 1, MAXR - 1);
}

void rh_state_hash(rh_hash_t *h)
{
    static uint32_t r[MAXR][2];
    int n = state_ranges(r);
    build_ext_map();
    uint64_t f = 1469598103934665603ull, s = f, sv = f;
    uint32_t masked = 0, bytes = 0;
    for (int i = 0; i < n; i++) {
        for (const uint32_t *p = (const uint32_t *)(uintptr_t)r[i][0]; p < (const uint32_t *)(uintptr_t)r[i][1]; p++) {
            uint32_t v = *p;
            f ^= v; f *= 1099511628211ull;
            if (g_nvol && is_vol((uint32_t)(uintptr_t)p)) { v = 0x9e3779b9u; masked++; }
            s ^= v; s *= 1099511628211ull;
            if (is_ext(v)) v = 0x7f4a7c15u;
            sv ^= v; sv *= 1099511628211ull;
        }
        bytes += r[i][1] - r[i][0];
    }
    h->full = f; h->semantic = s; h->semv = sv; h->masked = masked; h->ranges = (uint32_t)n; h->bytes = bytes;
}

int rh_state_dump(const char *path)
{
    static uint32_t r[MAXR][2];
    int n = state_ranges(r);
    build_ext_map();
    FILE *f = fopen(path, "wb");
    if (!f) { rh_log("dump: cannot open %s\n", path); return -1; }
    uint16_t cw; __asm__ volatile("fnstcw %0" : "=m"(cw));
    uint32_t hdr[4] = { 0x32534852u /* "RHS2" */, g_frame, cw, (uint32_t)n };
    fwrite(hdr, 4, 4, f);
    fwrite(r, 8, n, f);
    for (int i = 0; i < n; i++) fwrite((void *)(uintptr_t)r[i][0], 1, r[i][1] - r[i][0], f);
    fwrite(g_ext, 1, sizeof g_ext, f);
    uint32_t nv = (uint32_t)g_nvol; fwrite(&nv, 4, 1, f); fwrite(g_vol, 4, g_nvol, f);
    fclose(f);
    rh_log("dump frame=%u ranges=%d -> %s\n", g_frame, n, path);
    return 0;
}

/* ---- shadow execution: retail fn and port fn on the identical state ----
 * snapshot(state + allocator metadata) → retail → save → restore → port →
 * compare. Retail's result stays authoritative (restored when they differ).
 * External effects (D3D/file calls) inside the function run twice — use for
 * sim/logic functions. Never while a retail worker thread is live (caller gates). */
static uint8_t *g_pre, *g_ret; static size_t g_cap;
int rh_shadow(uint32_t va, const char *name, void (*port)(void), char *why, size_t whyn)
{
    static uint32_t r[MAXR][2], r2[MAXR][2];
    int n = state_ranges(r);
    size_t tot = 0;
    for (int i = 0; i < n; i++) tot += r[i][1] - r[i][0];
    if (tot > g_cap) { free(g_pre); free(g_ret); g_cap = tot + (16u << 20); g_pre = malloc(g_cap); g_ret = malloc(g_cap); }
    #define COPY_OUT(buf) { size_t o = 0; for (int i = 0; i < n; i++) { size_t l = r[i][1] - r[i][0]; memcpy((buf) + o, (void *)(uintptr_t)r[i][0], l); o += l; } }
    #define COPY_IN(buf)  { size_t o = 0; for (int i = 0; i < n; i++) { size_t l = r[i][1] - r[i][0]; memcpy((void *)(uintptr_t)r[i][0], (buf) + o, l); o += l; } }
    uint16_t cw; __asm__ volatile("fnstcw %0" : "=m"(cw));
    void *meta = rh_heap_meta_save();
    COPY_OUT(g_pre);
    ((void (__cdecl *)(void))(uintptr_t)va)();
    int n2 = state_ranges(r2);
    if (n2 != n || memcmp(r, r2, n * 8)) {     /* retail committed new arena pages: grow the view */
        snprintf(why, whyn, "%s: arena commit changed during call (ranges %d->%d); not compared", name, n, n2);
        rh_heap_meta_free(meta);
        return -1;
    }
    COPY_OUT(g_ret);
    void *meta_ret = rh_heap_meta_save();
    COPY_IN(g_pre);
    rh_heap_meta_restore(meta);
    __asm__ volatile("fldcw %0" : : "m"(cw));
    if (port) port(); else ((void (__cdecl *)(void))(uintptr_t)va)();
    long diff = 0; uint32_t first = 0, fv_r = 0, fv_p = 0;
    size_t o = 0;
    for (int i = 0; i < n; i++) {
        size_t l = r[i][1] - r[i][0];
        const uint32_t *a = (const uint32_t *)(g_ret + o), *b = (const uint32_t *)(uintptr_t)r[i][0];
        if (memcmp(a, b, l))
            for (size_t k = 0; k < l / 4; k++)
                if (a[k] != b[k]) { if (!diff++) { first = r[i][0] + (uint32_t)k * 4; fv_r = a[k]; fv_p = b[k]; } }
        o += l;
    }
    if (diff) {
        COPY_IN(g_ret);
        snprintf(why, whyn, "%s: %ld dwords differ; first %08x retail=%08x port=%08x", name, diff, first, fv_r, fv_p);
    }
    rh_heap_meta_restore(meta_ret);   /* retail's allocator view stays authoritative either way */
    __asm__ volatile("fldcw %0" : : "m"(cw));
    rh_heap_meta_free(meta); rh_heap_meta_free(meta_ret);
    return diff ? 1 : 0;
    #undef COPY_OUT
    #undef COPY_IN
}

/* ---- closure-form shadow (port functions that call retail, write anywhere) ---- */
#define SHX_DEAD 16384
static uint8_t g_shx_dead[SHX_DEAD];
/* depth of an in-progress shadow: hooks reached from inside one (retail's own calls during the
 * retail phase) must pass straight through — a nested full-state shadow would overwrite the
 * outer one's static snapshot buffers */
static int g_shadow_depth;
int rh_in_shadow(void) { return g_shadow_depth != 0; }
static int g_port_phase;
static const char *g_abort_what;
int rh_shadow_port_phase(void) { return g_port_phase; }
void rh_shadow_abort(const char *what) { if (!g_abort_what) g_abort_what = what; }
int rh_shadow_ex(const char *name, void (*retail)(void *), void (*port)(void *), void *ctx, char *why, size_t whyn)
{
    static uint32_t r[MAXR][2], r2[MAXR][2];
    int n = state_ranges(r);
    size_t tot = 0;
    for (int i = 0; i < n; i++) tot += r[i][1] - r[i][0];
    if (tot > g_cap) { free(g_pre); free(g_ret); g_cap = tot + (16u << 20); g_pre = malloc(g_cap); g_ret = malloc(g_cap); }
    #define COPY_OUT(buf) { size_t o = 0; for (int i = 0; i < n; i++) { size_t l = r[i][1] - r[i][0]; memcpy((buf) + o, (void *)(uintptr_t)r[i][0], l); o += l; } }
    #define COPY_IN(buf)  { size_t o = 0; for (int i = 0; i < n; i++) { size_t l = r[i][1] - r[i][0]; memcpy((void *)(uintptr_t)r[i][0], (buf) + o, l); o += l; } }
    uint16_t cw; __asm__ volatile("fnstcw %0" : "=m"(cw));
    uint32_t clk0[3], clk1[3]; rh_clock_save(clk0);
    void *meta = rh_heap_meta_save();
    void *media0 = rh_media_save();
    COPY_OUT(g_pre);
    g_shadow_depth++;
    retail(ctx);
    g_shadow_depth--;
    int n2 = state_ranges(r2);
    if (n2 != n || memcmp(r, r2, n * 8)) {
        snprintf(why, whyn, "%s: arena commit changed during call (ranges %d->%d); not compared", name, n, n2);
        rh_heap_meta_free(meta); rh_media_snap_free(media0);
        return -1;
    }
    void *media1 = rh_media_save();
    rh_media_restore(media0);
    uint32_t sp; __asm__ volatile("mov %%esp, %0" : "=r"(sp));
    sp -= 256;
    memcpy(g_shx_dead, (void *)(uintptr_t)(sp - SHX_DEAD), SHX_DEAD);   /* what retail left below SP */
    COPY_OUT(g_ret);
    void *meta_ret = rh_heap_meta_save();
    rh_clock_save(clk1);
    COPY_IN(g_pre);
    rh_heap_meta_restore(meta);
    rh_clock_restore(clk0);
    __asm__ volatile("fldcw %0" : : "m"(cw));
    g_shadow_depth++;
    g_port_phase = 1; g_abort_what = NULL;
    port(ctx);
    g_port_phase = 0;
    g_shadow_depth--;
    memcpy((void *)(uintptr_t)(sp - SHX_DEAD), g_shx_dead, SHX_DEAD);
    if (g_abort_what) {   /* the port reached an external effect retail already performed: keep retail's result */
        snprintf(why, whyn, "%s: port phase reached %s; not compared", name, g_abort_what);
        g_abort_what = NULL;
        COPY_IN(g_ret);
        rh_media_restore(media1);
        rh_media_snap_free(media0); rh_media_snap_free(media1);
        rh_heap_meta_restore(meta_ret);
        rh_clock_restore(clk1);
        __asm__ volatile("fldcw %0" : : "m"(cw));
        rh_heap_meta_free(meta); rh_heap_meta_free(meta_ret);
        return -1;
    }
    long diff = 0; int k = 0;
    int w = snprintf(why, whyn, "%s:", name);
    size_t o = 0;
    for (int i = 0; i < n; i++) {
        size_t l = r[i][1] - r[i][0];
        const uint32_t *a = (const uint32_t *)(g_ret + o), *b = (const uint32_t *)(uintptr_t)r[i][0];
        if (memcmp(a, b, l))
            for (size_t q = 0; q < l / 4; q++)
                if (a[q] != b[q]) {
                    if (k++ < 6 && w < (int)whyn - 48)
                        w += snprintf(why + w, whyn - w, " %08x r%08x/p%08x", r[i][0] + (uint32_t)q * 4, a[q], b[q]);
                    diff++;
                }
        o += l;
    }
    uint32_t clk2[3]; rh_clock_save(clk2);
    if (clk2[0] != clk1[0] || clk2[1] != clk1[1]) {
        if (w < (int)whyn - 48) w += snprintf(why + w, whyn - w, " clock r%u/%u p%u/%u", clk1[0], clk1[1], clk2[0], clk2[1]);
        diff++;
    }
    long md = rh_media_diff(media1);
    if (md) { if (w < (int)whyn - 40) w += snprintf(why + w, whyn - w, " sound-pool %ld", md); diff += md; }
    rh_media_restore(media1);
    rh_media_snap_free(media0); rh_media_snap_free(media1);
    if (diff) { COPY_IN(g_ret); if (w < (int)whyn - 24) snprintf(why + w, whyn - w, " (%ld differ)", diff); }
    rh_heap_meta_restore(meta_ret);
    rh_clock_restore(clk1);
    __asm__ volatile("fldcw %0" : : "m"(cw));
    rh_heap_meta_free(meta); rh_heap_meta_free(meta_ret);
    return diff ? 1 : 0;
    #undef COPY_OUT
    #undef COPY_IN
}
