/* rh_hooks.c — port functions vs retail on IDENTICAL state, inside the running game.
 *
 *   --hook shadow:NAME[@N]   retail runs (authoritative); the port re-runs on the pre-state
 *                            and every Nth call (default 1) is compared; divergences logged
 *   --hook replace:NAME      the port runs INSTEAD of retail (hybrid mode)
 * Hooks are call-site rewrites (all direct E8 calls to the VA). A port function never calls
 * its own retail VA. Add a function = one wrapper + one table row.
 *
 * Kinds:
 *   RET    pure function of a few words: compare return + the listed state (rand: seed word)
 *   FIELDS compare return + the exact fields the function may write (input_poll: ring ids)
 * Report lines (parsed by rhost.py verify):
 *   hook-result shadow NAME calls=C diverge=D skipped=S [first: ...] */
#include "rhost.h"
#include "rng.h"
#include "input.h"

typedef struct {
    const char *name; uint32_t va; void *wrapper;
    int mode;              /* 0 off, 1 shadow, 2 replace */
    uint32_t every;        /* shadow sampling */
    long calls, compared, diverge, skipped; char first[256];
} hook_t;

static hook_t H[];
enum { H_RAND, H_POLL, H_N };

static void diverged(hook_t *h, const char *fmt, ...)
{
    h->diverge++;
    if (h->diverge == 1) {
        va_list ap; va_start(ap, fmt); vsnprintf(h->first, sizeof h->first, fmt, ap); va_end(ap);
        rh_log("DIVERGE %s frame %u: %s\n", h->name, g_frame, h->first);
    }
}
static int sample(hook_t *h) { h->calls++; return h->every <= 1 || h->calls % h->every == 0; }

/* A shadow call runs retail AND the port on the retail stack: the port leaves its own garbage
 * below ESP, which retail's uninitialized reads (quirk #123) would then see — a pure-retail run
 * never would. Save the dead stack right after the retail call and put it back after the port
 * ran, so the stack is exactly what retail left. */
#define DEAD_BYTES 16384
static uint8_t g_dead[DEAD_BYTES];
static void __attribute__((noinline)) dead_save(uint32_t below)
{ memcpy(g_dead, (void *)(uintptr_t)(below - DEAD_BYTES), DEAD_BYTES); }
static void __attribute__((noinline)) dead_restore(uint32_t below)
{ memcpy((void *)(uintptr_t)(below - DEAD_BYTES), g_dead, DEAD_BYTES); }
#define SP() ({ uint32_t _s; __asm__ volatile("mov %%esp, %0" : "=r"(_s)); _s; })

/* ---- rand (MSVC LCG, FUN_005bf505) vs src/rng.c ---- */
typedef uint32_t (__cdecl *rand_t)(void);
static uint32_t __cdecl w_rand(void)
{
    hook_t *h = &H[H_RAND];
    uint32_t seed = g_ed->rng_seed, s0 = G32(seed);
    if (h->mode == 2) { h->calls++; rng_srand(s0); uint32_t r = rng_rand(); G32(seed) = rng_peek_state(); return r; }
    uint32_t r = ((rand_t)(uintptr_t)h->va)();
    if (sample(h)) {
        uint32_t s1 = G32(seed), sp = SP() - 256;
        dead_save(sp);
        rng_srand(s0); uint32_t p = rng_rand(), ps = rng_peek_state();
        dead_restore(sp);
        h->compared++;
        if (p != r || ps != s1) diverged(h, "seed %08x: retail %u/%08x port %u/%08x", s0, r, s1, p, ps);
    }
    return r;
}

/* ---- input_poll_consume (FUN_0043c110, __thiscall(mgr, now, id)) vs src/input.c ----
 * writes only rec->id of the matching ring record: snapshot all 64 records' ids */
typedef int (__attribute__((thiscall)) *poll_t)(void *, uint32_t, int32_t);
static int __attribute__((thiscall)) w_poll(void *mgr, uint32_t now, int32_t id)
{
    hook_t *h = &H[H_POLL];
    input_mgr *m = mgr;
    if (h->mode == 2) { h->calls++; return input_poll_consume(m, now, id); }
    if (!sample(h)) return ((poll_t)(uintptr_t)h->va)(mgr, now, id);
    int32_t pre[INPUT_RING_LEN], post_r[INPUT_RING_LEN];
    for (int i = 0; i < INPUT_RING_LEN; i++) pre[i] = m->ring[i] ? m->ring[i]->id : 0;
    int r = ((poll_t)(uintptr_t)h->va)(mgr, now, id);
    uint32_t sp = SP() - 256;
    dead_save(sp);
    for (int i = 0; i < INPUT_RING_LEN; i++) { post_r[i] = m->ring[i] ? m->ring[i]->id : 0; if (m->ring[i]) m->ring[i]->id = pre[i]; }
    int p = input_poll_consume(m, now, id);
    dead_restore(sp);
    int bad = -1;
    for (int i = 0; i < INPUT_RING_LEN; i++) {
        int32_t pv = m->ring[i] ? m->ring[i]->id : 0;
        if (pv != post_r[i] && bad < 0) bad = i;
        if (m->ring[i]) m->ring[i]->id = post_r[i];    /* retail stays authoritative */
    }
    h->compared++;
    if (p != r || bad >= 0) diverged(h, "id %d now %u: retail ret %d port ret %d first ring diff slot %d", id, now, r, p, bad);
    return r;
}

static hook_t H[H_N] = {
    [H_RAND] = { "rand", 0, (void *)w_rand, 0, 1, 0, 0, 0, 0, "" },
    [H_POLL] = { "input_poll", 0x43c110u, (void *)w_poll, 0, 1, 0, 0, 0, 0, "" },
};

void rh_hooks_install(void)
{
    H[H_RAND].va = g_ed->rand;
    if (strcmp(g_ed->tag, "enold")) H[H_POLL].va = 0;       /* EN-old VAs only (until the SE crosswalk) */
    if (!O.hooks) return;
    char b[512]; snprintf(b, sizeof b, "%s", O.hooks);
    for (char *t = strtok(b, ","); t; t = strtok(NULL, ",")) {
        char *c = strchr(t, ':'); if (!c) rh_fatal("bad hook spec %s", t);
        *c = 0; const char *mode = t; char *name = c + 1;
        uint32_t every = 1; char *at = strchr(name, '@'); if (at) { *at = 0; every = strtoul(at + 1, 0, 0); }
        hook_t *h = NULL;
        for (int i = 0; i < H_N; i++) if (!strcmp(H[i].name, name)) h = &H[i];
        if (!h) rh_fatal("unknown hook %s", name);
        if (!h->va) rh_fatal("hook %s has no VA for edition %s", name, g_ed->tag);
        h->mode = !strcmp(mode, "shadow") ? 1 : !strcmp(mode, "replace") ? 2 : 0;
        h->every = every;
        if (!h->mode) continue;
        int n = redirect_calls(h->va, h->wrapper);
        rh_log("hook %s:%s @%08x every=%u: %d call sites\n", mode, name, h->va, every, n);
        if (!n) rh_fatal("hook %s: no direct call sites", name);
    }
}

void rh_hooks_report(void)
{
    for (int i = 0; i < H_N; i++) {
        hook_t *h = &H[i];
        if (!h->mode) continue;
        if (h->mode == 1)
            rh_log("hook-result shadow %s calls=%ld diverge=%ld skipped=%ld compared=%ld%s%s\n", h->name, h->calls, h->diverge,
                   h->skipped, h->compared, h->diverge ? " first: " : "", h->diverge ? h->first : "");
        else rh_log("hook-result replace %s calls=%ld\n", h->name, h->calls);
    }
}
