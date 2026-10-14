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
#include "collision.h"
#include "entity_move.h"
#include "map_data.h"
#include "map_decode.h"
#include "camera_follow.h"

typedef struct {
    const char *name; uint32_t va; void *wrapper;
    int mode;              /* 0 off, 1 shadow, 2 replace */
    uint32_t every;        /* shadow sampling */
    long calls, compared, diverge, skipped; char first[256];
} hook_t;

static hook_t H[];
enum { H_RAND, H_POLL, H_MOVER_V, H_SWEEP_H, H_ENT_MOVE_H, H_ENT_MOVE_V, H_MAP_DECODE, H_CAM_FOLLOW, H_CAM_SHAKE, H_N };

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

/* ---- tile movers (FUN_0054e990 / FUN_0054ded0, thiscall on the grid object) vs src/collision.c ----
 * a slope cell's region-B +0x8 is a pointer to a 32-byte height profile in retail .rdata: the port's
 * resolver callback reads the byte straight out of the mapped image. The __int16 flags are passed
 * through as raw stack dwords (retail reads only the low half). */
static int rh_slope(void *ctx, uint32_t sref, int sub) { (void)ctx; return *(const uint8_t *)(uintptr_t)(sref + (uint32_t)sub); }

typedef int (__attribute__((thiscall)) *mover_v_t)(void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t *, uint32_t);
static int __attribute__((thiscall)) w_mover_v(void *grid, int32_t x, int32_t y, int32_t w, int32_t hh, int32_t delta, int32_t *out_y, uint32_t pass)
{
    hook_t *h = &H[H_MOVER_V];
    phys_box b = { x, y, w, hh, 0 };
    if (h->mode == 2) { h->calls++; return collision_move_vertical(grid, &b, delta, (int16_t)pass != 0, rh_slope, NULL, out_y); }
    if (!sample(h)) return ((mover_v_t)(uintptr_t)h->va)(grid, x, y, w, hh, delta, out_y, pass);
    int32_t pre = *out_y;
    int r = ((mover_v_t)(uintptr_t)h->va)(grid, x, y, w, hh, delta, out_y, pass);
    int32_t post = *out_y, py = pre;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    int p = collision_move_vertical(grid, &b, delta, (int16_t)pass != 0, rh_slope, NULL, &py);
    dead_restore(sp);
    h->compared++;
    if (p != r || py != post)
        diverged(h, "box (%d,%d %dx%d) d=%d pass=%d pre_y=%d: retail %d/y=%d port %d/y=%d", x, y, w, hh, delta, (int16_t)pass, pre, r, post, p, py);
    return r;
}

typedef int (__attribute__((thiscall)) *sweep_h_t)(void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t *, int32_t *, uint32_t);
static int __attribute__((thiscall)) w_sweep_h(void *grid, int32_t x, int32_t w, int32_t top, int32_t bot, int32_t delta,
                                               int32_t sdown, int32_t sup, int32_t *out_x, int32_t *io_y, uint32_t pass)
{
    hook_t *h = &H[H_SWEEP_H];
    if (h->mode == 2) { h->calls++; return collision_sweep_horizontal(grid, x, w, top, bot, delta, sdown, sup, (int16_t)pass != 0, rh_slope, NULL, out_x, io_y); }
    if (!sample(h)) return ((sweep_h_t)(uintptr_t)h->va)(grid, x, w, top, bot, delta, sdown, sup, out_x, io_y, pass);
    int32_t px0 = *out_x, py0 = *io_y;
    int r = ((sweep_h_t)(uintptr_t)h->va)(grid, x, w, top, bot, delta, sdown, sup, out_x, io_y, pass);
    int32_t rx = *out_x, ry = *io_y, px = px0, py = py0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    int p = collision_sweep_horizontal(grid, x, w, top, bot, delta, sdown, sup, (int16_t)pass != 0, rh_slope, NULL, &px, &py);
    dead_restore(sp);
    h->compared++;
    if (p != r || px != rx || py != ry)
        diverged(h, "x=%d w=%d y=[%d,%d] d=%d down=%d up=%d pass=%d pre=(%d,%d): retail %d/(%d,%d) port %d/(%d,%d)",
                 x, w, top, bot, delta, sdown, sup, (int16_t)pass, px0, py0, r, rx, ry, p, px, py);
    return r;
}

/* ---- entity movers (FUN_0054db10 / FUN_0054e5c0, thiscall on the moving entity) vs src/entity_move.c ----
 * they read the world singleton *(0x8a9b50) (collide list +0x278c, grid +0x1048): the port's g_world is
 * pointed at retail's before each port call. Written state: body x/y (horizontal; probe mode writes only
 * locals), *out_y + *support (vertical). */
#define WORLD_PTR_VA 0x8a9b50u
static void port_world(void) { g_world = *(wl_world **)(uintptr_t)WORLD_PTR_VA; g_world_slope = rh_slope; g_world_slope_ctx = NULL; }

typedef int (__attribute__((thiscall)) *ent_h_t)(void *, wl_body *, int32_t, int32_t, int32_t, uint32_t, uint32_t, int32_t);
static int __attribute__((thiscall)) w_ent_h(void *self, wl_body *b, int32_t delta, int32_t sdown, int32_t sup, uint32_t pass, uint32_t part, int32_t probe)
{
    hook_t *h = &H[H_ENT_MOVE_H];
    if (h->mode == 2) { h->calls++; port_world(); return entity_move_horizontal(self, b, delta, sdown, sup, (int16_t)pass, (uint16_t)part, probe); }
    if (!sample(h)) return ((ent_h_t)(uintptr_t)h->va)(self, b, delta, sdown, sup, pass, part, probe);
    int32_t x0 = b->x, y0 = b->y;
    int r = ((ent_h_t)(uintptr_t)h->va)(self, b, delta, sdown, sup, pass, part, probe);
    int32_t rx = b->x, ry = b->y;
    b->x = x0; b->y = y0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = entity_move_horizontal(self, b, delta, sdown, sup, (int16_t)pass, (uint16_t)part, probe);
    dead_restore(sp);
    int32_t px = b->x, py = b->y;
    b->x = rx; b->y = ry;
    h->compared++;
    if (p != r || px != rx || py != ry)
        diverged(h, "ent %p part %u d=%d down=%d up=%d pass=%d probe=%d pre=(%d,%d): retail %d/(%d,%d) port %d/(%d,%d)",
                 self, (uint16_t)part, delta, sdown, sup, (int16_t)pass, probe, x0, y0, r, rx, ry, p, px, py);
    return r;
}

typedef int (__attribute__((thiscall)) *ent_v_t)(void *, wl_body *, int32_t, int32_t, uint32_t, int32_t *, wl_entity **);
static int __attribute__((thiscall)) w_ent_v(void *self, wl_body *b, int32_t delta, int32_t pass, uint32_t part, int32_t *out_y, wl_entity **support)
{
    hook_t *h = &H[H_ENT_MOVE_V];
    if (h->mode == 2) { h->calls++; port_world(); return entity_move_vertical(self, b, delta, pass, (uint16_t)part, out_y, support); }
    if (!sample(h)) return ((ent_v_t)(uintptr_t)h->va)(self, b, delta, pass, part, out_y, support);
    int32_t oy0 = *out_y; wl_entity *s0 = support ? *support : NULL;
    int r = ((ent_v_t)(uintptr_t)h->va)(self, b, delta, pass, part, out_y, support);
    int32_t ry = *out_y; wl_entity *rs = support ? *support : NULL;
    *out_y = oy0; if (support) *support = s0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = entity_move_vertical(self, b, delta, pass, (uint16_t)part, out_y, support);
    dead_restore(sp);
    int32_t py = *out_y; wl_entity *ps = support ? *support : NULL;
    *out_y = ry; if (support) *support = rs;
    h->compared++;
    if (p != r || py != ry || ps != rs)
        diverged(h, "ent %p part %u d=%d pass=%d box (%d,%d %dx%d m%d) pre_y=%d: retail %d/y=%d/sup=%p port %d/y=%d/sup=%p",
                 self, (uint16_t)part, delta, pass, b->x, b->y, b->w, b->h, b->margin, oy0, r, ry, (void *)rs, p, py, (void *)ps);
    return r;
}

/* ---- map decode (FUN_00587e00, thiscall on the grid) vs src/map_decode.c + parallax.c ----
 * The parse (FUN_00587970, this = retail's 0x40-byte map object) is wrapped as a RECORDER of its
 * (hModule, id): the decode wrapper re-reads that DATA resource through rhost's own resource
 * overrides, parses it with map_data_parse, and runs the port's decode on the identical pre-state
 * grid (the previous room's bytes — retail reuses the buffer). The whole 0x2cb044-byte grid is
 * compared; the report breaks the differing bytes down by grid region. */
#define GRID_BYTES 0x2cb044u
static uint32_t g_map_hmod, g_map_id;
typedef int (__attribute__((thiscall)) *map_parse_t)(void *, uint32_t, uint32_t);
static int __attribute__((thiscall)) w_map_parse(void *mapobj, uint32_t hmod, uint32_t id)
{
    g_map_hmod = hmod; g_map_id = id & 0xffffu;
    return ((map_parse_t)(uintptr_t)0x587970u)(mapobj, hmod, id);
}
static void rh_bank_dims(void *ctx, uint16_t bank, int32_t *w, int32_t *h)
{
    (void)ctx;
    uint32_t p = G32(0x8a760cu + 4u * bank);        /* the sprite-bank pool (0x58c910) */
    *w = p ? (int32_t)G32(p + 0x20) : 0; *h = p ? (int32_t)G32(p + 0x24) : 0;
}
static uint8_t *g_grid_pre, *g_grid_ret;
typedef uint32_t (__attribute__((thiscall)) *map_decode_t)(void *, void *, int32_t, int32_t, int32_t);
static uint32_t __attribute__((thiscall)) w_map_decode(void *grid, void *mapobj, int32_t p2, int32_t p3, int32_t p4)
{
    hook_t *h = &H[H_MAP_DECODE];
    if (!g_grid_pre) { g_grid_pre = malloc(GRID_BYTES); g_grid_ret = malloc(GRID_BYTES); }
    memcpy(g_grid_pre, grid, GRID_BYTES);
    uint32_t r = ((map_decode_t)(uintptr_t)h->va)(grid, mapobj, p2, p3, p4);
    if (!sample(h)) return r;
    memcpy(g_grid_ret, grid, GRID_BYTES);
    HRSRC (WINAPI *fr)(HMODULE, LPCSTR, LPCSTR) = iat_lookup("FindResourceA");
    HGLOBAL (WINAPI *lr)(HMODULE, HRSRC) = iat_lookup("LoadResource");
    DWORD (WINAPI *sr)(HMODULE, HRSRC) = iat_lookup("SizeofResource");
    HMODULE hm = (HMODULE)(uintptr_t)g_map_hmod;
    HRSRC rs = fr(hm, MAKEINTRESOURCEA(g_map_id), "DATA");
    const uint8_t *bytes = rs ? LockResource(lr(hm, rs)) : NULL;
    map_data md;
    if (!bytes || map_data_parse(&md, bytes, sr(hm, rs)) != 0) {
        h->skipped++; rh_log("map_decode f%u: map %u: resource/parse failed\n", g_frame, g_map_id); return r;
    }
    memcpy(grid, g_grid_pre, GRID_BYTES);
    uint32_t sp = SP() - 256;
    dead_save(sp);
    map_decode_cfg cfg; map_decode_cfg_init(&cfg, p2, p3, p4);
    map_decode(&md, grid, &cfg, rh_bank_dims, NULL);
    dead_restore(sp);
    map_data_free(&md);
    static const struct { const char *n; uint32_t lo; } RG[] = {
        { "hdr", 0 }, { "A", 0x30 }, { "B", 0x140030 }, { "C", 0x195030 }, { "E", 0x1d1030 }, { "dims", 0x2c1030 }, { "D", 0x2c1040 } };
    long nd[7] = { 0 }, tot = 0; uint32_t first[7];
    const uint8_t *g = grid;
    for (int k = 0; k < 7; k++) {
        uint32_t hi = k < 6 ? RG[k + 1].lo : GRID_BYTES;
        for (uint32_t o = RG[k].lo; o < hi; o++) if (g[o] != g_grid_ret[o]) { if (!nd[k]++) first[k] = o; }
        tot += nd[k];
    }
    char b[512]; int n = 0;
    for (int k = 0; k < 7; k++) if (nd[k])
        n += snprintf(b + n, sizeof b - n, " %s=%ld@%x(r%02x/p%02x)", RG[k].n, nd[k], first[k], g_grid_ret[first[k]], g[first[k]]);
    rh_log("map_decode f%u map %u p2=%d p3=%d p4=%d: %ld bytes differ%s\n", g_frame, g_map_id, p2, p3, p4, tot, n ? b : "");
    h->compared++;
    if (tot) {
        diverged(h, "map %u p2=%d p3=%d p4=%d: %ld bytes differ%s", g_map_id, p2, p3, p4, tot, b);
        if (h->diverge <= 4) {                          /* both grids, for offline diffing */
            char path[MAX_PATH]; FILE *f;
            snprintf(path, sizeof path, "%s\\grid_f%u_retail.bin", O.rundir, g_frame);
            if ((f = fopen(path, "wb"))) { fwrite(g_grid_ret, 1, GRID_BYTES, f); fclose(f); }
            snprintf(path, sizeof path, "%s\\grid_f%u_port.bin", O.rundir, g_frame);
            if ((f = fopen(path, "wb"))) { fwrite(grid, 1, GRID_BYTES, f); fclose(f); }
        }
    }
    memcpy(grid, g_grid_ret, GRID_BYTES);
    return r;
}

/* ---- camera (FUN_0043d1d0 follow step, thiscall on the 0x78-byte view at world+0x104c; FUN_0043d340
 * shake applier, stdcall(block, cur, max)) vs src/camera_follow.c — camera_view is retail layout ---- */
typedef int (__attribute__((thiscall)) *cam_follow_t)(camera_view *);
static int __attribute__((thiscall)) w_cam_follow(camera_view *v)
{
    hook_t *h = &H[H_CAM_FOLLOW];
    if (h->mode == 2) { h->calls++; camera_follow_step(v); return v->shake_y.out; }
    if (!sample(h)) return ((cam_follow_t)(uintptr_t)h->va)(v);
    camera_view pre = *v;
    int r = ((cam_follow_t)(uintptr_t)h->va)(v);
    camera_view ret = *v;
    *v = pre;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    camera_follow_step(v);
    dead_restore(sp);
    camera_view prt = *v;
    *v = ret;
    h->compared++;
    if (memcmp(&prt, &ret, sizeof ret)) {
        int k = 0; while (((uint8_t *)&prt)[k] == ((uint8_t *)&ret)[k]) k++;
        diverged(h, "view +%02x: retail %08x port %08x (cur %d,%d tgt %d,%d vel %d,%d cap %d flag %d)", k & ~3,
                 *(uint32_t *)((uint8_t *)&ret + (k & ~3)), *(uint32_t *)((uint8_t *)&prt + (k & ~3)),
                 pre.cur_x, pre.cur_y, pre.tgt_x, pre.tgt_y, pre.vel_x, pre.vel_y, pre.cap, pre.flag);
    }
    return r;
}
typedef int32_t (__stdcall *cam_shake_t)(cam_shake *, int32_t, int32_t);
static int32_t __stdcall w_cam_shake(cam_shake *b, int32_t cur, int32_t max)
{
    hook_t *h = &H[H_CAM_SHAKE];
    if (h->mode == 2) { h->calls++; return camera_shake_apply(b, cur, max); }
    if (!sample(h)) return ((cam_shake_t)(uintptr_t)h->va)(b, cur, max);
    cam_shake pre = *b;
    int32_t r = ((cam_shake_t)(uintptr_t)h->va)(b, cur, max);
    cam_shake ret = *b;
    *b = pre;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    int32_t p = camera_shake_apply(b, cur, max);
    dead_restore(sp);
    cam_shake prt = *b;
    *b = ret;
    h->compared++;
    if (p != r || memcmp(&prt, &ret, sizeof ret))
        diverged(h, "act %d dur %d ctr %d ph %d env %u amp %d cur %d max %d: retail %d/out %d port %d/out %d",
                 pre.active, pre.duration, pre.counter, pre.phase, pre.envelope, pre.amp, cur, max, r, ret.out, p, prt.out);
    return r;
}

static hook_t H[H_N] = {
    [H_RAND] = { "rand", 0, (void *)w_rand, 0, 1, 0, 0, 0, 0, "" },
    [H_POLL] = { "input_poll", 0x43c110u, (void *)w_poll, 0, 1, 0, 0, 0, 0, "" },
    [H_MOVER_V] = { "mover_v", 0x54e990u, (void *)w_mover_v, 0, 1, 0, 0, 0, 0, "" },
    [H_SWEEP_H] = { "sweep_h", 0x54ded0u, (void *)w_sweep_h, 0, 1, 0, 0, 0, 0, "" },
    [H_ENT_MOVE_H] = { "ent_move_h", 0x54db10u, (void *)w_ent_h, 0, 1, 0, 0, 0, 0, "" },
    [H_ENT_MOVE_V] = { "ent_move_v", 0x54e5c0u, (void *)w_ent_v, 0, 1, 0, 0, 0, 0, "" },
    [H_MAP_DECODE] = { "map_decode", 0x587e00u, (void *)w_map_decode, 0, 1, 0, 0, 0, 0, "" },
    [H_CAM_FOLLOW] = { "cam_follow", 0x43d1d0u, (void *)w_cam_follow, 0, 1, 0, 0, 0, 0, "" },
    [H_CAM_SHAKE]  = { "cam_shake",  0x43d340u, (void *)w_cam_shake,  0, 1, 0, 0, 0, 0, "" },
};

void rh_hooks_install(void)
{
    H[H_RAND].va = g_ed->rand;
    /* input_poll_consume: EN-old 0x43c110; EN-SE 0x4383c0 (crosswalk: masked bytes identical) */
    H[H_POLL].va = !strcmp(g_ed->tag, "enold") ? 0x43c110u : !strcmp(g_ed->tag, "ense-en") ? 0x4383c0u : 0;
    if (strcmp(g_ed->tag, "enold"))   /* EN-old VAs only (SE: crosswalk TODO) */
        H[H_MOVER_V].va = H[H_SWEEP_H].va = H[H_ENT_MOVE_H].va = H[H_ENT_MOVE_V].va = H[H_MAP_DECODE].va =
        H[H_CAM_FOLLOW].va = H[H_CAM_SHAKE].va = 0;
    if (!O.hooks) return;
    char b[512]; snprintf(b, sizeof b, "%s", O.hooks);
    for (char *t = strtok(b, ","); t; t = strtok(NULL, ",")) {
        char *c = strchr(t, ':'); if (!c) rh_fatal("bad hook spec %s", t);
        *c = 0; const char *mode = t; char *name = c + 1;
        uint32_t every = 1; char *at = strchr(name, '@'); if (at) { *at = 0; every = strtoul(at + 1, 0, 0); }
        hook_t *h = NULL;
        for (int i = 0; i < H_N; i++) if (!strcmp(H[i].name, name)) h = &H[i];
        if (!h) rh_fatal("unknown hook %s", name);
        if (!h->va) { rh_log("hook %s: no VA for edition %s — skipped\n", name, g_ed->tag); continue; }
        h->mode = !strcmp(mode, "shadow") ? 1 : !strcmp(mode, "replace") ? 2 : 0;
        h->every = every;
        if (!h->mode) continue;
        if (h == &H[H_MAP_DECODE]) {
            if (h->mode == 2) rh_fatal("map_decode: shadow only (the port decode needs the recorded resource)");
            rh_log("hook map_decode: parse recorder %d call sites\n", redirect_calls(0x587970u, w_map_parse));
        }
        int n = redirect_calls_chain(h->va, h->wrapper);
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
