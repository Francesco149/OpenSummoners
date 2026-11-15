/* rh_snap.c — in-process savestates (needs --headless: rh_dev.c).  openrecet's tracker, on SotES's memory map.
 *
 * At a frame boundary (the flip coroutine, rhost.c) everything a frame can change is in this process's deterministic
 * regions: retail's .data/.bss, the heap arena (+ VREG), the DirectSound fake's buffer pool, the fixed host-object page
 * (fake COM objects retail holds), the DEV region + the mapped DIB slots (rh_dev.c), the game stack from the flip's
 * saved esp up, and a set of host variables (clocks, frame, the fake handle / GDI numbering, DirectInput's queue, the
 * user\ files).  A savestate is a copy of that; restoring it and running on reaches the state a straight replay
 * reaches (gate: `--snap-test A:B[:K]` / `rhost.py snap-check`).
 *
 * Cost model (unchanged from openrecet):
 *   anchor   full copy of the tracked regions (the BASE) + every tracked page write-protected; the first write to a
 *            page faults once (rh_snap_fault: mark dirty, unprotect, continue); it stays writable until re-anchored.
 *   D        pages written since the anchor (append-only).
 *   take     per page of D: the parent savestate's slab if equal, "= base" if equal to the base, else a new 4 KB slab.
 *   restore  per page of D: copy the slab / the base.  Both O(|D|).
 * Pages committed after the anchor are dirty from birth (base = zeros); pages decommitted after it (VREG VirtualFree)
 * are "gone": in D, never read; a restore re-commits / decommits them to the savestate's commit state first.
 *
 * Writes that do not fault into the tracker: kernel-mode / wine-unix-side writes (ReadFile into a protected page just
 * fails) and GDI drawing into a DIB slot (through GDI's own view of the section).  The import wrappers at the bottom
 * touch the destination first; rh_dev.c touches a slot at GetDC. */
#include "rhost.h"
#include <mmsystem.h>

#define PGSZ 4096u
#define NPG (1u << 20)
enum { PG_NONE = 0, PG_CLEAN = 1, PG_DIRTY = 2 };

static uint8_t *g_pst;                   /* [NPG] page state */
static uint8_t *g_gone;                  /* [NPG] 1 = in D and decommitted now */
#define H_GONE 0xffffffffu
static uint8_t **g_base;                 /* [NPG] page → its bytes in the base copy (NULL: zeros) */
static uint8_t *g_basebuf; static size_t g_basesz;
static uint32_t *g_D; static uint32_t g_nD, g_capD;
static int g_anchored;
static uint64_t g_faults, g_touches, g_takes, g_restores, g_anchors, g_decommits, g_recommits;
static uint8_t g_zero[PGSZ];

/* ---- slab pool: 4 KB page copies, refcounted ---- */
#define CHUNK_SLABS 4096u                /* 16 MB chunks */
#define MAX_CHUNKS 96u
static uint8_t *g_chunk[MAX_CHUNKS]; static uint16_t *g_ref[MAX_CHUNKS]; static uint32_t g_nchunk;
static uint32_t *g_free; static uint32_t g_nfree, g_capfree, g_live_slabs, g_slab_cap = 60 * CHUNK_SLABS;   /* 960 MB */
static uint8_t *slab_ptr(uint32_t i) { return g_chunk[i / CHUNK_SLABS] + (size_t)(i % CHUNK_SLABS) * PGSZ; }
static uint32_t slab_new(void)
{
    if (!g_nfree) {
        if (g_nchunk >= MAX_CHUNKS || g_nchunk * CHUNK_SLABS >= g_slab_cap) return 0;
        uint8_t *c = VirtualAlloc(NULL, CHUNK_SLABS * PGSZ, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!c) return 0;
        g_chunk[g_nchunk] = c; g_ref[g_nchunk] = calloc(CHUNK_SLABS, sizeof(uint16_t));
        if (g_capfree < (g_nchunk + 1) * CHUNK_SLABS) { g_capfree = (g_nchunk + 1) * CHUNK_SLABS; g_free = realloc(g_free, g_capfree * 4); }
        for (uint32_t k = CHUNK_SLABS; k-- > 0; ) g_free[g_nfree++] = g_nchunk * CHUNK_SLABS + k;
        g_nchunk++;
    }
    uint32_t i = g_free[--g_nfree];
    g_ref[i / CHUNK_SLABS][i % CHUNK_SLABS] = 1; g_live_slabs++;
    return i + 1;
}
static void slab_ref(uint32_t h) { g_ref[(h - 1) / CHUNK_SLABS][(h - 1) % CHUNK_SLABS]++; }
static void slab_unref(uint32_t h)
{
    uint32_t i = h - 1;
    if (--g_ref[i / CHUNK_SLABS][i % CHUNK_SLABS] == 0) { g_free[g_nfree++] = i; g_live_slabs--; }
}

/* ---- host state: plain blocks + module snapshot functions ---- */
#define MAX_BLK 64
static struct { const char *name; void *p; size_t n; } g_blk[MAX_BLK]; static int g_nblk; static size_t g_blk_total;
void rh_snap_register(const char *name, void *p, size_t n)
{
    for (int i = 0; i < g_nblk; i++) if (g_blk[i].p == p) return;
    if (g_nblk >= MAX_BLK) rh_fatal("rh_snap: too many host state blocks");
    g_blk[g_nblk].name = name; g_blk[g_nblk].p = p; g_blk[g_nblk].n = n; g_nblk++; g_blk_total += n;
}
#define MAX_MOD 16
static struct { const char *name; void *(*save)(void); void (*restore)(const void *); void (*release)(void *); } g_mod[MAX_MOD]; static int g_nmod;
void rh_snap_module(const char *name, void *(*save)(void), void (*restore)(const void *), void (*release)(void *))
{
    if (g_nmod >= MAX_MOD) rh_fatal("rh_snap: too many modules");
    g_mod[g_nmod].name = name; g_mod[g_nmod].save = save; g_mod[g_nmod].restore = restore; g_mod[g_nmod].release = release; g_nmod++;
}

typedef struct snap {
    int id; uint32_t count; uint32_t *pp;
    uint8_t *host; void *mod[MAX_MOD];
    uint8_t *stack; uint32_t stack_lo, stack_n;
    uint16_t cw; uint32_t mxcsr; uint32_t frame;
} snap_t;
#define MAX_SNAP 65536
static snap_t **g_snap; static int g_next_id = 1, g_live_snaps; static snap_t *g_cur;

static void page_dirty(uint32_t pg, int unprotect)
{
    if (g_nD == g_capD) { g_capD = g_capD ? g_capD * 2 : 8192; g_D = realloc(g_D, g_capD * 4); }
    g_D[g_nD++] = pg; g_pst[pg] = PG_DIRTY;
    if (unprotect) { DWORD old; VirtualProtect((void *)(uintptr_t)(pg << 12), PGSZ, PAGE_READWRITE, &old); }
}
int rh_snap_fault(EXCEPTION_POINTERS *e)
{
    const EXCEPTION_RECORD *r = e->ExceptionRecord;
    if (!g_anchored || r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || r->NumberParameters < 2 || r->ExceptionInformation[0] != 1) return 0;
    uint32_t pg = (uint32_t)r->ExceptionInformation[1] >> 12;
    if (g_pst[pg] != PG_CLEAN) return 0;
    g_faults++;
    page_dirty(pg, 1);
    return 1;
}
static LONG CALLBACK snap_veh(EXCEPTION_POINTERS *e) { return rh_snap_fault(e) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH; }
void rh_snap_touch(const void *p, size_t n)
{
    if (!g_anchored || !p || !n) return;
    uint32_t a = (uint32_t)(uintptr_t)p, lo = a >> 12, hi = (a + (uint32_t)n - 1) >> 12;
    if (hi < lo) hi = NPG - 1;
    for (uint32_t pg = lo; pg <= hi; pg++)
        if (g_pst[pg] == PG_CLEAN) { g_touches++; page_dirty(pg, 1); }
}
void rh_snap_committed(uint32_t lo, uint32_t hi)
{
    if (!g_anchored) return;
    for (uint32_t pg = lo >> 12; pg < (hi + PGSZ - 1) >> 12; pg++) {
        g_gone[pg] = 0;
        if (g_pst[pg] == PG_NONE) { g_base[pg] = NULL; page_dirty(pg, 0); }
        else if (g_pst[pg] == PG_CLEAN) page_dirty(pg, 1);
    }
}
void rh_snap_decommitted(uint32_t lo, uint32_t hi)
{
    if (!g_anchored) return;
    for (uint32_t a = lo & ~(PGSZ - 1); a < hi; ) {
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)(uintptr_t)a, &mi, sizeof mi)) break;
        uint32_t e = (uint32_t)(uintptr_t)mi.BaseAddress + (uint32_t)mi.RegionSize;
        if (e > hi || e <= a) e = (hi + PGSZ - 1) & ~(PGSZ - 1);
        if (mi.State != MEM_COMMIT)
            for (uint32_t pg = a >> 12; pg < e >> 12; pg++) {
                if (g_pst[pg] == PG_NONE || g_gone[pg]) continue;
                if (g_pst[pg] == PG_CLEAN) page_dirty(pg, 0);
                g_gone[pg] = 1; g_decommits++;
            }
        a = e;
    }
}
static void page_commit_state(uint32_t pg, int gone)
{
    void *p = (void *)(uintptr_t)(pg << 12);
    if (gone == g_gone[pg]) return;
    if (gone) { if (!VirtualFree(p, PGSZ, MEM_DECOMMIT)) rh_fatal("rh_snap: decommit %08x failed (err %lu)", pg << 12, GetLastError()); }
    else if (!VirtualAlloc(p, PGSZ, MEM_COMMIT, PAGE_READWRITE)) rh_fatal("rh_snap: re-commit %08x failed (err %lu)", pg << 12, GetLastError());
    g_gone[pg] = (uint8_t)gone;
}

#define MAXR 4096
int rh_media_range(uint32_t *lo, uint32_t *hi);
int rh_fixed_range(uint32_t *lo, uint32_t *hi);
int rh_dev_slot_ranges(uint32_t (*out)[2], int max);
static int tracked_ranges(uint32_t (*r)[2])
{
    int n = 0;
    r[n][0] = RT_DATA_LO & ~(PGSZ - 1); r[n][1] = (RT_DATA_HI + PGSZ - 1) & ~(PGSZ - 1); n++;
    n += rh_heap_ranges(r + n, MAXR - 8);
    if (rh_media_range(&r[n][0], &r[n][1])) n++;
    if (rh_fixed_range(&r[n][0], &r[n][1])) n++;
    if (rh_dev_range(&r[n][0], &r[n][1])) n++;
    n += rh_dev_slot_ranges(r + n, MAXR - n);
    return n;
}

static void snap_free_one(snap_t *s)
{
    for (uint32_t i = 0; i < s->count; i++) if (s->pp[i] && s->pp[i] != H_GONE) slab_unref(s->pp[i]);
    for (int m = 0; m < g_nmod; m++) if (s->mod[m]) g_mod[m].release(s->mod[m]);
    free(s->pp); free(s->host); free(s->stack);
    g_snap[s->id] = NULL; g_live_snaps--;
    if (g_cur == s) g_cur = NULL;
    free(s);
}
void rh_snap_free(int id) { if (id > 0 && id < MAX_SNAP && g_snap && g_snap[id]) snap_free_one(g_snap[id]); }

int rh_snap_anchor(void)
{
    static uint32_t r[MAXR][2];
    if (!O.headless) { rh_log("snap: savestates need --headless (device state must live in the DEV region)\n"); return -1; }
    if (!g_pst) {
        g_pst = VirtualAlloc(NULL, NPG, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        g_gone = VirtualAlloc(NULL, NPG, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        g_base = VirtualAlloc(NULL, NPG * sizeof *g_base, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        g_snap = calloc(MAX_SNAP, sizeof *g_snap);
        if (!g_pst || !g_gone || !g_base || !g_snap) rh_fatal("rh_snap: tables");
        AddVectoredExceptionHandler(1, snap_veh);
    }
    for (int i = 1; i < MAX_SNAP; i++) if (g_snap[i]) snap_free_one(g_snap[i]);
    for (uint32_t i = 0; i < g_nD; i++) { g_pst[g_D[i]] = PG_NONE; g_gone[g_D[i]] = 0; }
    for (uint32_t pg = 0; pg < NPG; pg++)
        if (g_pst[pg] == PG_CLEAN) { DWORD old; VirtualProtect((void *)(uintptr_t)(pg << 12), PGSZ, PAGE_READWRITE, &old); g_pst[pg] = PG_NONE; }
    g_nD = 0; g_anchored = 0;
    int n = tracked_ranges(r);
    size_t tot = 0;
    for (int i = 0; i < n; i++) tot += r[i][1] - r[i][0];
    if (tot > g_basesz) {
        if (g_basebuf) VirtualFree(g_basebuf, 0, MEM_RELEASE);
        g_basesz = tot + (32u << 20);
        g_basebuf = VirtualAlloc(NULL, g_basesz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!g_basebuf) { g_basesz = 0; rh_log("snap: cannot allocate the %u MB base copy\n", (unsigned)(tot >> 20)); return -1; }
    }
    memset(g_base, 0, NPG * sizeof *g_base);
    size_t o = 0;
    for (int i = 0; i < n; i++) {
        size_t l = r[i][1] - r[i][0];
        memcpy(g_basebuf + o, (void *)(uintptr_t)r[i][0], l);
        for (uint32_t pg = r[i][0] >> 12; pg < r[i][1] >> 12; pg++) { g_base[pg] = g_basebuf + o + ((size_t)(pg - (r[i][0] >> 12)) << 12); g_pst[pg] = PG_CLEAN; }
        DWORD old;
        if (!VirtualProtect((void *)(uintptr_t)r[i][0], l, PAGE_READONLY, &old))
            rh_fatal("rh_snap: VirtualProtect %08x+%x failed (err %lu)", r[i][0], (unsigned)l, GetLastError());
        o += l;
    }
    g_anchored = 1; g_anchors++; g_cur = NULL;
    rh_log("snap: anchored f%u: %d ranges, %u MB tracked\n", g_frame, n, (unsigned)(tot >> 20));
    return 0;
}

int rh_snap_take(void)
{
    if (!g_anchored) return 0;
    if (rh_scrub_depth()) { rh_log("snap: not at a clean boundary (external call depth %d)\n", rh_scrub_depth()); return 0; }
    int id = 0;
    for (int k = 0; k < MAX_SNAP - 1; k++) { int c = g_next_id; g_next_id = g_next_id + 1 < MAX_SNAP ? g_next_id + 1 : 1; if (!g_snap[c]) { id = c; break; } }
    if (!id) return 0;
    snap_t *s = calloc(1, sizeof *s), *par = g_cur;
    s->id = id; s->count = g_nD; s->pp = malloc((g_nD ? g_nD : 1) * 4); s->frame = g_frame;
    for (uint32_t i = 0; i < g_nD; i++) {
        uint32_t pg = g_D[i];
        const uint8_t *cur = (const uint8_t *)(uintptr_t)(pg << 12);
        uint32_t ph = par && i < par->count ? par->pp[i] : 0;
        if (g_gone[pg]) { s->pp[i] = H_GONE; continue; }
        if (ph == H_GONE) ph = 0;
        if (ph && !memcmp(cur, slab_ptr(ph - 1), PGSZ)) { slab_ref(ph); s->pp[i] = ph; continue; }
        if (!ph && !memcmp(cur, g_base[pg] ? g_base[pg] : g_zero, PGSZ)) { s->pp[i] = 0; continue; }
        uint32_t h = slab_new();
        if (!h) {
            for (uint32_t k = 0; k < i; k++) if (s->pp[k] && s->pp[k] != H_GONE) slab_unref(s->pp[k]);
            free(s->pp); free(s);
            rh_log("snap: slab pool exhausted (%u live slabs)\n", g_live_slabs);
            return 0;
        }
        memcpy(slab_ptr(h - 1), cur, PGSZ);
        s->pp[i] = h;
    }
    s->host = malloc(g_blk_total ? g_blk_total : 1);
    size_t o = 0;
    for (int i = 0; i < g_nblk; i++) { memcpy(s->host + o, g_blk[i].p, g_blk[i].n); o += g_blk[i].n; }
    for (int m = 0; m < g_nmod; m++) s->mod[m] = g_mod[m].save();
    rh_game_stack(&s->stack_lo, &s->stack_n);
    s->stack = malloc(s->stack_n ? s->stack_n : 1);
    memcpy(s->stack, (void *)(uintptr_t)s->stack_lo, s->stack_n);
    __asm__ volatile("fnstcw %0" : "=m"(s->cw));
    __asm__ volatile("stmxcsr %0" : "=m"(s->mxcsr));
    g_snap[id] = s; g_live_snaps++; g_cur = s; g_takes++;
    return id;
}

int rh_snap_restore(int id)
{
    if (!g_anchored || id <= 0 || id >= MAX_SNAP || !g_snap[id]) return -1;
    snap_t *s = g_snap[id];
    for (uint32_t i = 0; i < g_nD; i++) {
        /* a page that entered D after this savestate holds its anchor state there: the base, or — committed after the
         * anchor (no base) — not committed at all (else a restore to before a commit leaves zero pages committed that a
         * replay never commits: semv drift with 0 differing dwords, story-bd3 verify f89312) */
        uint32_t pg = g_D[i], h = i < s->count ? s->pp[i] : g_base[pg] ? 0 : H_GONE;
        if (h == H_GONE || g_gone[pg]) { if (g_gone[pg] && h != H_GONE) g_recommits++; page_commit_state(pg, h == H_GONE); }
        if (h == H_GONE) continue;
        memcpy((void *)(uintptr_t)(pg << 12), h ? slab_ptr(h - 1) : g_base[pg] ? g_base[pg] : g_zero, PGSZ);
    }
    size_t o = 0;
    for (int i = 0; i < g_nblk; i++) { memcpy(g_blk[i].p, s->host + o, g_blk[i].n); o += g_blk[i].n; }
    for (int m = 0; m < g_nmod; m++) g_mod[m].restore(s->mod[m]);
    /* g_gesp came back with the host blocks: the game stack region is the savestate's */
    if (s->stack_lo != g_gesp) rh_fatal("rh_snap: game stack pointer %08x after restore, savestate %08x", g_gesp, s->stack_lo);
    memcpy((void *)(uintptr_t)s->stack_lo, s->stack, s->stack_n);
    rh_scrub_below(s->stack_lo);
    __asm__ volatile("fldcw %0" : : "m"(s->cw));
    __asm__ volatile("ldmxcsr %0" : : "m"(s->mxcsr));
    g_cur = s; g_restores++;
    return 0;
}
long rh_snap_diff(int id, uint32_t lo, uint32_t hi, void (*cb)(uint32_t va, uint32_t was, uint32_t now, void *u), void *u)
{
    if (!g_anchored || id <= 0 || id >= MAX_SNAP || !g_snap[id]) return -1;
    const snap_t *s = g_snap[id];
    long n = 0;
    for (uint32_t i = 0; i < g_nD; i++) {
        uint32_t pg = g_D[i], base = pg << 12, h = i < s->count ? s->pp[i] : g_base[pg] ? 0 : H_GONE;
        if (base + PGSZ <= lo || base >= hi) continue;
        if (h == H_GONE || g_gone[pg]) {
            if ((h == H_GONE) != g_gone[pg]) { n++; if (cb) cb(base, h == H_GONE ? 0xdeadc0deu : 0, g_gone[pg] ? 0xdeadc0deu : 0, u); }
            continue;
        }
        const uint32_t *was = (const uint32_t *)(h ? slab_ptr(h - 1) : g_base[pg] ? g_base[pg] : g_zero), *now = (const uint32_t *)(uintptr_t)base;
        if (!memcmp(was, now, PGSZ)) continue;
        for (uint32_t k = 0; k < PGSZ / 4; k++)
            if (was[k] != now[k] && base + k * 4 >= lo && base + k * 4 < hi) { n++; if (cb) cb(base + k * 4, was[k], now[k], u); }
    }
    return n;
}
int rh_snap_frame(int id) { return id > 0 && id < MAX_SNAP && g_snap && g_snap[id] ? (int)g_snap[id]->frame : -1; }
int rh_snap_anchored(void) { return g_anchored; }
uint32_t rh_snap_dirty(void) { return g_nD; }
void rh_snap_stats(char *b, size_t n)
{
    snprintf(b, n, "snap: anchors=%llu dirty=%u pages takes=%llu restores=%llu live=%d slabs=%u (%u MB) faults=%llu touches=%llu decommits=%llu recommits=%llu base=%u MB",
             (unsigned long long)g_anchors, g_nD, (unsigned long long)g_takes, (unsigned long long)g_restores, g_live_snaps, g_live_slabs,
             g_live_slabs >> 8, (unsigned long long)g_faults, (unsigned long long)g_touches,
             (unsigned long long)g_decommits, (unsigned long long)g_recommits, (unsigned)(g_basesz >> 20));
}

/* ---- --snap-test A:B[:K]: savestate at A; at B hash, restore to A (must hash as at A), replay to B; K passes must hash alike ---- */
static int g_st_on, g_st_id, g_st_pass, g_st_k, g_st_fail, g_st_res; static uint32_t g_st_a, g_st_b, g_st_rng; static uint64_t g_st_sem, g_st_rsem, g_st_dev, g_st_fb, g_st_asem;
static char g_st_status[160];
uint64_t rh_state_dev_hash(void);
uint64_t rh_fb_chain(void);
void rh_snap_selftest(void)
{
    if (!O.snap_test) return;
    if (!g_st_on) {
        unsigned a = 0, b = 0; int k = 2;
        if (sscanf(O.snap_test, "%u:%u:%d", &a, &b, &k) < 2 || b <= a) rh_fatal("--snap-test A:B[:K]");
        g_st_a = a; g_st_b = b; g_st_k = k; g_st_on = 1;
    }
    if (g_frame == g_st_a && !g_st_id) {
        if (rh_snap_anchor() || !(g_st_id = rh_snap_take())) rh_fatal("snap-test: no savestate at f%u", g_frame);
        rh_hash_t ha; rh_state_hash(&ha); g_st_asem = ha.semantic;
        rh_log("snap-test: savestate %d at f%u\n", g_st_id, g_frame);
    } else if (g_frame == g_st_b && g_st_id && g_st_pass <= g_st_k) {
        rh_hash_t h; rh_state_hash(&h);
        uint64_t dev = rh_state_dev_hash(), fb = rh_fb_chain();
        uint32_t rng = G32(g_ed->rng_seed);
        if (!g_st_pass) { g_st_sem = h.semantic; g_st_dev = dev; g_st_rng = rng; g_st_fb = fb; }
        else if (h.semantic != g_st_sem || dev != g_st_dev || rng != g_st_rng || fb != g_st_fb) {
            int residue = dev == g_st_dev && rng == g_st_rng && fb == g_st_fb && (!g_st_res || h.semantic == g_st_rsem);
            if (residue) { g_st_res++; g_st_rsem = h.semantic; } else g_st_fail++;
            rh_log("snap-test: pass %d DIFFERS at f%u: sem %016llx/%016llx dev %016llx/%016llx rng %08x/%08x fb %016llx/%016llx%s\n", g_st_pass, g_frame,
                   (unsigned long long)h.semantic, (unsigned long long)g_st_sem, (unsigned long long)dev, (unsigned long long)g_st_dev, rng, g_st_rng,
                   (unsigned long long)fb, (unsigned long long)g_st_fb, residue ? " (game state only: tracking residue in the first pass?)" : "");
        } else if (g_st_res) g_st_fail++;
        else rh_log("snap-test: pass %d equal at f%u (sem %016llx dev %016llx)\n", g_st_pass, g_frame, (unsigned long long)h.semantic, (unsigned long long)dev);
        if (g_st_pass < g_st_k) {
            g_st_pass++;
            rh_snap_take();
            if (rh_snap_restore(g_st_id)) rh_fatal("snap-test: restore failed");
            /* the restored state itself = the state at A (incl. the commit map: pages committed between A and B) */
            rh_hash_t hr; rh_state_hash(&hr);
            if (hr.semantic != g_st_asem) {
                g_st_fail++;
                rh_log("snap-test: pass %d: restored state differs from f%u: sem %016llx/%016llx\n", g_st_pass, g_st_a,
                       (unsigned long long)hr.semantic, (unsigned long long)g_st_asem);
            }
        } else g_st_pass++;
    }
}
const char *rh_snap_test_status(void)
{
    if (!O.snap_test) return "";
    int n = snprintf(g_st_status, sizeof g_st_status, " snap_test=%s passes=%d", g_st_fail ? "FAIL" : g_st_pass <= g_st_k ? "INCOMPLETE" : g_st_res ? "RESIDUE" : "PASS",
                     g_st_pass > g_st_k ? g_st_k : g_st_pass);
    if (g_st_res && !g_st_fail) snprintf(g_st_status + n, sizeof g_st_status - (size_t)n, " snap_at=%u snap_sem=%016llx", g_st_b, (unsigned long long)g_st_rsem);
    return g_st_status;
}

/* ---- imports through which the SYSTEM may write into retail memory: touch the destination first (the census list:
 * rhost --dev-census, rh-combat + rh-my-play + rh-water-shrine) ---- */
#define PREV(name) static __typeof__(name) *p_##name
PREV(ReadFile); PREV(PeekMessageA); PREV(GetLocaleInfoA); PREV(GetLocaleInfoW); PREV(WideCharToMultiByte); PREV(MultiByteToWideChar);
PREV(LCMapStringW); PREV(GetStringTypeW); PREV(GetCPInfo); PREV(GetVersionExA); PREV(FormatMessageA); PREV(GetFileVersionInfoA);
PREV(VerQueryValueA); PREV(mmioRead); PREV(mmioDescend); PREV(GetSystemDirectoryA); PREV(GetTextMetricsA); PREV(GetObjectA);
#define T(p, n) rh_snap_touch((p), (n))
static BOOL WINAPI w_ReadFile(HANDLE h, LPVOID b, DWORD n, LPDWORD got, LPOVERLAPPED ov) { T(b, n); T(got, 4); return p_ReadFile(h, b, n, got, ov); }
static BOOL WINAPI w_PeekMessageA(LPMSG m, HWND h, UINT a, UINT b, UINT f) { T(m, sizeof *m); return p_PeekMessageA(m, h, a, b, f); }
static int WINAPI w_GetLocaleInfoA(LCID l, LCTYPE t, LPSTR b, int n) { if (n > 0) T(b, (size_t)n); return p_GetLocaleInfoA(l, t, b, n); }
static int WINAPI w_GetLocaleInfoW(LCID l, LCTYPE t, LPWSTR b, int n) { if (n > 0) T(b, (size_t)n * 2); return p_GetLocaleInfoW(l, t, b, n); }
static int WINAPI w_WideCharToMultiByte(UINT cp, DWORD f, LPCWCH s, int ns, LPSTR d, int nd, LPCCH dc, LPBOOL used)
{ if (nd > 0) T(d, (size_t)nd); T(used, 4); return p_WideCharToMultiByte(cp, f, s, ns, d, nd, dc, used); }
static int WINAPI w_MultiByteToWideChar(UINT cp, DWORD f, LPCCH s, int ns, LPWSTR d, int nd) { if (nd > 0) T(d, (size_t)nd * 2); return p_MultiByteToWideChar(cp, f, s, ns, d, nd); }
static int WINAPI w_LCMapStringW(LCID l, DWORD f, LPCWSTR s, int ns, LPWSTR d, int nd) { if (nd > 0) T(d, (size_t)nd * 2); return p_LCMapStringW(l, f, s, ns, d, nd); }
static BOOL WINAPI w_GetStringTypeW(DWORD t, LPCWCH s, int n, LPWORD o) { if (n > 0) T(o, (size_t)n * 2); else if (s) T(o, (wcslen(s) + 1) * 2); return p_GetStringTypeW(t, s, n, o); }
static BOOL WINAPI w_GetCPInfo(UINT cp, LPCPINFO i) { T(i, sizeof *i); return p_GetCPInfo(cp, i); }
static BOOL WINAPI w_GetVersionExA(LPOSVERSIONINFOA v) { if (v) T(v, v->dwOSVersionInfoSize); return p_GetVersionExA(v); }
static DWORD WINAPI w_FormatMessageA(DWORD f, LPCVOID src, DWORD id, DWORD lang, LPSTR b, DWORD n, va_list *a)
{ if (f & FORMAT_MESSAGE_ALLOCATE_BUFFER) T(b, 4); else T(b, n); return p_FormatMessageA(f, src, id, lang, b, n, a); }
static BOOL WINAPI w_GetFileVersionInfoA(LPCSTR f, DWORD h, DWORD n, LPVOID d) { T(d, n); return p_GetFileVersionInfoA(f, h, n, d); }
static BOOL WINAPI w_VerQueryValueA(LPCVOID blk, LPCSTR sub, LPVOID *b, PUINT n) { T(b, 4); T(n, 4); return p_VerQueryValueA(blk, sub, b, n); }
static LONG WINAPI w_mmioRead(HMMIO h, HPSTR b, LONG n) { if (n > 0) T(b, (size_t)n); return p_mmioRead(h, b, n); }
static MMRESULT WINAPI w_mmioDescend(HMMIO h, LPMMCKINFO ck, const MMCKINFO *par, UINT f) { T(ck, sizeof *ck); return p_mmioDescend(h, ck, par, f); }
static UINT WINAPI w_GetSystemDirectoryA(LPSTR b, UINT n) { T(b, n); return p_GetSystemDirectoryA(b, n); }
static BOOL WINAPI w_GetTextMetricsA(HDC h, LPTEXTMETRICA tm) { T(tm, sizeof *tm); return p_GetTextMetricsA(h, tm); }
static int WINAPI w_GetObjectA(HANDLE h, int c, LPVOID pv) { if (c > 0) T(pv, (size_t)c); return p_GetObjectA(h, c, pv); }
#undef T
void rh_snap_install(void)
{
    #define WRAP(name) do { p_##name = iat_lookup(#name); if (p_##name) iat_override(#name, w_##name); } while (0)
    WRAP(ReadFile); WRAP(PeekMessageA); WRAP(GetLocaleInfoA); WRAP(GetLocaleInfoW); WRAP(WideCharToMultiByte); WRAP(MultiByteToWideChar);
    WRAP(LCMapStringW); WRAP(GetStringTypeW); WRAP(GetCPInfo); WRAP(GetVersionExA); WRAP(FormatMessageA); WRAP(GetFileVersionInfoA);
    WRAP(VerQueryValueA); WRAP(mmioRead); WRAP(mmioDescend); WRAP(GetSystemDirectoryA); WRAP(GetTextMetricsA); WRAP(GetObjectA);
    #undef WRAP
}
