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
#include "char_ai.h"
#include "char_physics.h"
#include "char_integrate.h"
#include "npc_ai.h"
#include "engine_calls.h"

typedef struct {
    const char *name; uint32_t va; void *wrapper;
    int mode;              /* 0 off, 1 shadow, 2 replace */
    uint32_t every;        /* shadow sampling */
    long calls, compared, diverge, skipped; char first[256];
} hook_t;

static hook_t H[];
enum { H_RAND, H_POLL, H_MOVER_V, H_SWEEP_H, H_ENT_MOVE_H, H_ENT_MOVE_V, H_MAP_DECODE, H_CAM_FOLLOW, H_CAM_SHAKE, H_CHAR_AI, H_CHAR_PHYS, H_CHAR_ANIM, H_CHAR_INT, H_HAZARD, H_NPC_AI, H_N };

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

/* ---- character AI (FUN_00478ba0, thiscall(entity, a2, a3)) — FULL-STATE shadow (rh_shadow_ex): the
 * AI writes the command block, the input ring, sounds and calls the integrator 0x442a70 ---- */
typedef int (__attribute__((thiscall)) *char_ai_t)(void *, int32_t, int32_t);
typedef struct { void *self; int32_t a2, a3; int r_ret, r_port; } ai_ctx;
static void ai_retail(void *c) { ai_ctx *x = c; x->r_ret = ((char_ai_t)(uintptr_t)H[H_CHAR_AI].va)(x->self, x->a2, x->a3); }
static void ai_port(void *c)   { ai_ctx *x = c; port_world(); x->r_port = char_ai(x->self, (wl_body *)(uintptr_t)x->a2, x->a3); }
static int __attribute__((thiscall)) w_char_ai(void *self, int32_t a2, int32_t a3)
{
    hook_t *h = &H[H_CHAR_AI];
    if (h->mode == 2) { h->calls++; port_world(); return char_ai(self, (wl_body *)(uintptr_t)a2, a3); }
    if (!sample(h)) return ((char_ai_t)(uintptr_t)h->va)(self, a2, a3);
    ai_ctx x = { self, a2, a3, 0, 0 };
    char why[512], pre[1600]; int pn = 0;
    {   /* the inputs, for the divergence report */
        wl_char *c = self; input_mgr *m = c->input; wl_keycfg *k = **(wl_keycfg ***)(uintptr_t)0x8a6e80u;
        pn += snprintf(pre + pn, sizeof pre - pn, "prev lr%d ud%d jump%d run_mode=%d win=%d diff=%d held", 
                       c->cmd.lr, c->cmd.ud, c->cmd.jump, k->run_mode, k->dash_window, c->difficulty);
        for (int i = 0; i < 11; i++) pn += snprintf(pre + pn, sizeof pre - pn, " %d/%u", m->axis_held[i], (uint32_t)m->axis_held_b[i]);
        pn += snprintf(pre + pn, sizeof pre - pn, " ring");
        for (int i = 0; i < INPUT_RING_LEN && pn < (int)sizeof pre - 40; i++)
            if (m->ring[i]->id) pn += snprintf(pre + pn, sizeof pre - pn, " [%d]%d@%u/%d", i, m->ring[i]->id, m->ring[i]->ts, m->ring[i]->flag);
    }
    int d = rh_shadow_ex("char_ai", ai_retail, ai_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r_ret; }
    h->compared++;
    if (d || x.r_ret != x.r_port) {
        diverged(h, "f%u ret %d/%d %s", g_frame, x.r_ret, x.r_port, d ? why : "");
        if (h->diverge <= 3) rh_log("char_ai divergence f%u vnow=%u: %s\n", g_frame, rh_vnow(), pre);
    }
    return x.r_ret;
}

/* ---- character movement step (FUN_00467bb0, thiscall(char, cmd, body, probe, ctl, part, grounded)) ----
 * writes only the body record (x/y/attach through the movers) and char->air_jumps: those are
 * snapshotted (cheap enough for every call); stray writes are the hybrid corpus gate's job. */
static long g_phys_cov[8], g_phys_ctl[5], g_phys_mode[5], g_phys_probe;
typedef int (__attribute__((thiscall)) *char_phys_t)(wl_char *, wl_cmd *, wl_body *, int32_t, int32_t, int32_t, int32_t);
static int __attribute__((thiscall)) w_char_phys(wl_char *self, wl_cmd *cmd, wl_body *b, int32_t probe, int32_t ctl, int32_t part, int32_t gr)
{
    hook_t *h = &H[H_CHAR_PHYS];
    if (h->mode == 2) { h->calls++; port_world(); return char_physics_step(self, cmd, b, probe, ctl, part, gr); }
    if (!sample(h)) return ((char_phys_t)(uintptr_t)h->va)(self, cmd, b, probe, ctl, part, gr);
    static wl_body pre, ret; int32_t aj0 = self->air_jumps;
    pre = *b;
    g_phys_cov[pre.state < 7 ? pre.state : 7]++; g_phys_ctl[(uint32_t)ctl < 4 ? ctl : 4]++;
    g_phys_mode[(uint32_t)self->move_mode < 4 ? self->move_mode : 4]++; if (probe) g_phys_probe++;
    int r = ((char_phys_t)(uintptr_t)h->va)(self, cmd, b, probe, ctl, part, gr);
    ret = *b; int32_t aj_r = self->air_jumps;
    *b = pre; self->air_jumps = aj0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = char_physics_step(self, cmd, b, probe, ctl, part, gr);
    dead_restore(sp);
    int bad = memcmp(b, &ret, sizeof ret) ? 1 : 0;
    int k = 0; if (bad) while (((uint8_t *)b)[k] == ((uint8_t *)&ret)[k]) k++;
    uint32_t pv = bad ? *(uint32_t *)((uint8_t *)b + (k & ~3)) : 0, rv = bad ? *(uint32_t *)((uint8_t *)&ret + (k & ~3)) : 0;
    int32_t aj_p = self->air_jumps;
    *b = ret; self->air_jumps = aj_r;
    h->compared++;
    if (bad || aj_p != aj_r || p != r)
        diverged(h, "f%u char %p code %x part %d probe %d ctl %d gr %d cmd lr%d j%d ud%d | body +%03x retail %08x port %08x"
                 " | pre st%u/%u/%u vx%d vy%d x%d y%d facing %d | air_jumps %d/%d ret %d/%d",
                 g_frame, (void *)self, self->base.code, part, probe, ctl, gr, cmd->lr, cmd->jump, cmd->ud, k & ~3, rv, pv,
                 pre.state, pre.sub, pre.ctr, pre.vel_x, pre.vvel, pre.x, pre.y, pre.facing, aj_r, aj_p, r, p);
    return r;
}

/* ---- per-state clip + jump effects (FUN_00466930, thiscall(char, body, probe, grounded)) — full state:
 * sounds, particles and the LCG ---- */
typedef int (__attribute__((thiscall)) *char_anim_t)(wl_char *, wl_body *, int32_t, int32_t);
typedef struct { wl_char *self; wl_body *b; int32_t probe, gr; int r; } anim_ctx;
static void anim_retail(void *c) { anim_ctx *x = c; x->r = ((char_anim_t)(uintptr_t)H[H_CHAR_ANIM].va)(x->self, x->b, x->probe, x->gr); }
static void anim_port(void *c)   { anim_ctx *x = c; port_world(); char_anim_select(x->self, x->b, x->probe, x->gr); }
static int __attribute__((thiscall)) w_char_anim(wl_char *self, wl_body *b, int32_t probe, int32_t gr)
{
    hook_t *h = &H[H_CHAR_ANIM];
    if (h->mode == 2) { h->calls++; port_world(); char_anim_select(self, b, probe, gr); return 0; }
    if (!sample(h)) return ((char_anim_t)(uintptr_t)h->va)(self, b, probe, gr);
    anim_ctx x = { self, b, probe, gr, 0 };
    uint16_t st = b->state, sub = b->sub;
    char why[512];
    int d = rh_shadow_ex("char_anim", anim_retail, anim_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r; }
    h->compared++;
    if (d) diverged(h, "f%u char %p code %x state %u/%u probe %d gr %d: %s", g_frame, (void *)self, self->base.code, st, sub, probe, gr, why);
    return x.r;
}

/* ---- the integrator (FUN_00442a70, thiscall(char, cmd, in, out, probe, part)) — full state ---- */
typedef int (__attribute__((thiscall)) *char_int_t)(wl_char *, wl_cmd *, wl_body *, wl_body *, int32_t, int32_t);
typedef struct { wl_char *self; wl_cmd *cmd; wl_body *in, *out; int32_t probe, part; int r_ret, r_port; } int_ctx;
static void int_retail(void *c) { int_ctx *x = c; x->r_ret = ((char_int_t)(uintptr_t)H[H_CHAR_INT].va)(x->self, x->cmd, x->in, x->out, x->probe, x->part); }
static void int_port(void *c)   { int_ctx *x = c; port_world(); x->r_port = char_integrate(x->self, x->cmd, x->in, x->out, x->probe, x->part); }
static long g_int_special;
static int __attribute__((thiscall)) w_char_int(wl_char *self, wl_cmd *cmd, wl_body *in, wl_body *out, int32_t probe, int32_t part)
{
    hook_t *h = &H[H_CHAR_INT];
    if (h->mode == 2) { h->calls++; port_world(); return char_integrate(self, cmd, in, out, probe, part); }
    if (!sample(h)) return ((char_int_t)(uintptr_t)h->va)(self, cmd, in, out, probe, part);
    if ((in->pose >= 0x75 && in->pose <= 0x7a) || in->pose == 0x81) g_int_special++;
    int_ctx x = { self, cmd, in, out, probe, part, 0, 0 };
    uint16_t pose = in->pose, st = in->state; uint32_t code = (uint32_t)self->base.code; long unp = g_integrate_unported;
    char why[512];
    int d = rh_shadow_ex("char_integrate", int_retail, int_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r_ret; }
    h->compared++;
    if (d || x.r_ret != x.r_port)
        diverged(h, "f%u char %p code %x pose %x state %u probe %d part %d unported+%ld ret %d/%d %s", g_frame, (void *)self, code,
                 pose, st, probe, part, g_integrate_unported - unp, x.r_ret, x.r_port, d ? why : "");
    return x.r_ret;
}

/* ---- hazard probe (FUN_00440f40, thiscall(char, n, body, int *out)) — reads only; writes *out ---- */
typedef int (__attribute__((thiscall)) *hazard_t)(wl_char *, uint32_t, wl_body *, int32_t *);
static int __attribute__((thiscall)) w_hazard(wl_char *self, uint32_t n, wl_body *b, int32_t *out)
{
    hook_t *h = &H[H_HAZARD];
    if (h->mode == 2) { h->calls++; port_world(); return char_hazard_probe(self, (uint16_t)n, b, out); }
    if (!sample(h)) return ((hazard_t)(uintptr_t)h->va)(self, n, b, out);
    int32_t o0 = out ? *out : 0;
    int r = ((hazard_t)(uintptr_t)h->va)(self, n, b, out);
    int32_t ro = out ? *out : 0;
    if (out) *out = o0;
    port_world();
    int p = char_hazard_probe(self, (uint16_t)n, b, out);
    int32_t po = out ? *out : 0;
    if (out) *out = ro;
    h->compared++;
    if (p != r || po != ro) diverged(h, "f%u char %p n %u: retail %d/out %d port %d/out %d", g_frame, (void *)self, (uint16_t)n, r, ro, p, po);
    return r;
}

/* ---- the NPC navigation AI (FUN_0043f880, thiscall + 9 args) — full state (heap scratch, LCG, probes) ---- */
typedef int (__attribute__((thiscall)) *npc_ai_t)(wl_char *, int32_t, int32_t, wl_body *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_char *self; int32_t a2, a3; wl_body *b; int32_t a5, a6, a7, a8, a9, a10; int r_ret, r_port; } npc_ctx;
static void npc_retail(void *c) { npc_ctx *x = c; x->r_ret = ((npc_ai_t)(uintptr_t)H[H_NPC_AI].va)(x->self, x->a2, x->a3, x->b, x->a5, x->a6, x->a7, x->a8, x->a9, x->a10); }
static void npc_port(void *c)   { npc_ctx *x = c; port_world(); x->r_port = npc_navigate(x->self, x->a2, x->a3, x->b, x->a5, x->a6, x->a7, x->a8, x->a9, x->a10); }
static int __attribute__((thiscall)) w_npc_ai(wl_char *self, int32_t a2, int32_t a3, wl_body *b, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10)
{
    hook_t *h = &H[H_NPC_AI];
    if (h->mode == 2) { h->calls++; port_world(); return npc_navigate(self, a2, a3, b, a5, a6, a7, a8, a9, a10); }
    if (!sample(h)) return ((npc_ai_t)(uintptr_t)h->va)(self, a2, a3, b, a5, a6, a7, a8, a9, a10);
    npc_ctx x = { self, a2, a3, b, a5, a6, a7, a8, a9, a10, 0, 0 };
    char why[512];
    int d = rh_shadow_ex("npc_ai", npc_retail, npc_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r_ret; }
    h->compared++;
    if (d || x.r_ret != x.r_port)
        diverged(h, "f%u char %p code %x tx %d arrive %d gr %d calm %d mode %d ty %d norun %d jump %d state %u ret %d/%d %s", g_frame, (void *)self,
                 self->base.code, a2, a3, a5, a6, a7, a8, a9, a10, b->state, x.r_ret, x.r_port, d ? why : "");
    return x.r_ret;
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
    [H_CHAR_AI]    = { "char_ai",    0x478ba0u, (void *)w_char_ai,    0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_PHYS]  = { "char_physics", 0x467bb0u, (void *)w_char_phys, 0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_ANIM]  = { "char_anim",  0x466930u, (void *)w_char_anim,  0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_INT]   = { "char_integrate", 0x442a70u, (void *)w_char_int, 0, 1, 0, 0, 0, 0, "" },
    [H_HAZARD]     = { "hazard_probe", 0x440f40u, (void *)w_hazard, 0, 1, 0, 0, 0, 0, "" },
    [H_NPC_AI]     = { "npc_ai", 0x43f880u, (void *)w_npc_ai, 0, 1, 0, 0, 0, 0, "" },
};

static const void *rh_data_identity(uint32_t va) { return (const void *)(uintptr_t)va; }
/* engine_calls.h slots -> retail (EN-old VAs; the roots are retail's globals) */
static void bind_engine_calls(void)
{
    g_eng.tick   = iat_lookup("GetTickCount");          /* rhost's virtual clock override */
    g_eng.sys    = (void *)(uintptr_t)0x8a6b60u;
    g_eng.keycfg = (wl_keycfg ***)(uintptr_t)0x8a6e80u;
    *(void **)&g_eng.sound_411390        = (void *)(uintptr_t)0x411390u;
    *(void **)&g_eng.integrate_442a70    = (void *)(uintptr_t)0x442a70u;
    *(void **)&g_eng.probe_440f40        = (void *)(uintptr_t)0x440f40u;
    *(void **)&g_eng.skill_ok_402a60     = (void *)(uintptr_t)0x402a60u;
    *(void **)&g_eng.party_slot_ok_4071f0 = (void *)(uintptr_t)0x4071f0u;
    *(void **)&g_eng.skill_479f90        = (void *)(uintptr_t)0x479f90u;
    *(void **)&g_eng.skill_47a300        = (void *)(uintptr_t)0x47a300u;
    *(void **)&g_eng.skill_47a420        = (void *)(uintptr_t)0x47a420u;
    *(void **)&g_eng.skill_47a7f0        = (void *)(uintptr_t)0x47a7f0u;
    *(void **)&g_eng.target_ok_47adb0    = (void *)(uintptr_t)0x47adb0u;
    *(void **)&g_eng.sfx_voice_40b710    = (void *)(uintptr_t)0x40b710u;
    *(void **)&g_eng.sfx_place_40bb90    = (void *)(uintptr_t)0x40bb90u;
    *(void **)&g_eng.dsb_volume_5bb870   = (void *)(uintptr_t)0x5bb870u;
    *(void **)&g_eng.dsb_pan_5bb880      = (void *)(uintptr_t)0x5bb880u;
    *(void **)&g_eng.dsb_freq_5bb890     = (void *)(uintptr_t)0x5bb890u;
    *(void **)&g_eng.dsb_play_5bb8c0     = (void *)(uintptr_t)0x5bb8c0u;
    *(void **)&g_eng.particle_557370     = (void *)(uintptr_t)0x557370u;
    g_eng.data = rh_data_identity;
    *(void **)&g_eng.op_new    = (void *)(uintptr_t)0x5bf00du;   /* operator new (cdecl) */
    *(void **)&g_eng.op_delete = (void *)(uintptr_t)0x5bef0eu;   /* operator delete (cdecl) */
#define B(n) *(void **)&g_eng.h##n = (void *)(uintptr_t)0x##n##u
    B(445fd0); B(467260); B(4467d0); B(44a740); B(45a300); B(44bc10); B(44c250); B(44bf60); B(446900);
    B(452020); B(449810); B(4539b0); B(44d8f0); B(448dc0); B(452730); B(467380); B(457ff0); B(45e830);
    B(455d80); B(44b160); B(4516e0); B(4484b0); B(4512d0); B(44a200); B(44e8d0); B(44f690); B(450810);
    B(44d2e0); B(456d30); B(461790); B(426440); B(445e50);
#undef B
    *(void **)&g_eng.skill_cond_5b5990   = (void *)(uintptr_t)0x5b5990u;
}

void rh_hooks_install(void)
{
    if (!strcmp(g_ed->tag, "enold")) bind_engine_calls();
    rng_bind((uint32_t *)(uintptr_t)g_ed->rng_seed);   /* ported code draws from retail's LCG word */
    H[H_RAND].va = g_ed->rand;
    /* input_poll_consume: EN-old 0x43c110; EN-SE 0x4383c0 (crosswalk: masked bytes identical) */
    H[H_POLL].va = !strcmp(g_ed->tag, "enold") ? 0x43c110u : !strcmp(g_ed->tag, "ense-en") ? 0x4383c0u : 0;
    if (strcmp(g_ed->tag, "enold"))   /* EN-old VAs only (SE: crosswalk TODO) */
        H[H_MOVER_V].va = H[H_SWEEP_H].va = H[H_ENT_MOVE_H].va = H[H_ENT_MOVE_V].va = H[H_MAP_DECODE].va =
        H[H_CAM_FOLLOW].va = H[H_CAM_SHAKE].va = H[H_CHAR_AI].va = H[H_CHAR_PHYS].va = H[H_CHAR_ANIM].va = H[H_CHAR_INT].va = H[H_HAZARD].va = H[H_NPC_AI].va = 0;
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
    if (H[H_CHAR_INT].mode == 1)
        rh_log("char_integrate: %ld calls delegated (special input pose), %ld entered an unported pose mid-call\n",
               g_int_special, g_integrate_unported);
    if (H[H_CHAR_PHYS].mode == 1)
        rh_log("char_physics coverage: state0..6+ %ld %ld %ld %ld %ld %ld %ld %ld | ctl0..3+ %ld %ld %ld %ld %ld | mode0..3+ %ld %ld %ld %ld %ld | probe %ld\n",
               g_phys_cov[0], g_phys_cov[1], g_phys_cov[2], g_phys_cov[3], g_phys_cov[4], g_phys_cov[5], g_phys_cov[6], g_phys_cov[7],
               g_phys_ctl[0], g_phys_ctl[1], g_phys_ctl[2], g_phys_ctl[3], g_phys_ctl[4],
               g_phys_mode[0], g_phys_mode[1], g_phys_mode[2], g_phys_mode[3], g_phys_mode[4], g_phys_probe);
}
