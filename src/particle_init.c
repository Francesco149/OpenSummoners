/* particle_init.c — FUN_00557550 + the entity setup FUN_00426620 and helpers.  See
 * particle_init.h.
 *
 * Transcribed from the decompile, checked against the asm (the trackers +0x48.. and the
 * life +0x258 / +0x26c are u16; the body's +0x58 is u16).  Labels follow the decompile's
 * shared tails.  Codes 100085..100087 / 100095 overwrite the angle argument before the
 * common tail reads it; codes that reuse the owner argument as the anchor output never
 * read the owner afterwards. */
#include "particle_init.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "char_integrate.h"
#include "char_state.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "entity_fx.h"
#include "rng.h"

static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t sysv(uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)g_eng.sys + off, 4); return v; }
static const void *clip(uint32_t va) { return va != 0 ? g_eng.data(va) : NULL; }

/* ── the x87 polar step (0x460e40 / 0x55cdb0): a = ang * deg / n (or * 0.001);
 * s = __ftol(r sin a +- 0.5), c = __ftol(r cos a +- 0.5), both off the 80-bit stack.
 * Run under the caller's FPU control word (retail's, inside rhost). ── */
#if defined(__i386__)
extern int32_t oss_polar_x87(int32_t ang, int32_t r, const double *k, int32_t div, int32_t *c_out) __asm__("oss_polar_x87");
__asm__(
    ".text\n"
    ".p2align 4\n"
    "oss_polar_x87:\n\t"
    "pushl %ebx\n\t"
    "subl $16, %esp\n\t"
    "movl 32(%esp), %ebx\n\t"          /* k */
    "fildl 24(%esp)\n\t"               /* ang */
    "fmull (%ebx)\n\t"
    "cmpl $0, 36(%esp)\n\t"            /* div */
    "jz 5f\n\t"
    "fidivl 36(%esp)\n\t"
    "jmp 6f\n"
    "5:\n\t"
    "fmull 8(%ebx)\n"
    "6:\n\t"
    "fildl 28(%esp)\n\t"               /* r */
    "fld %st(1)\n\t"
    "fsin\n\t"
    "fmul %st(1), %st\n\t"
    "fcoml 16(%ebx)\n\t"
    "fnstsw %ax\n\t"
    "testb $1, %ah\n\t"
    "jz 1f\n\t"
    "fsubl 24(%ebx)\n\t"
    "jmp 2f\n"
    "1:\n\t"
    "faddl 24(%ebx)\n"
    "2:\n\t"
    "fnstcw 8(%esp)\n\t"
    "movw 8(%esp), %ax\n\t"
    "orb $0x0c, %ah\n\t"
    "movw %ax, 10(%esp)\n\t"
    "fldcw 10(%esp)\n\t"
    "fistpll 0(%esp)\n\t"
    "fldcw 8(%esp)\n\t"
    "movl 0(%esp), %eax\n\t"
    "movl %eax, 12(%esp)\n\t"
    "fxch %st(1)\n\t"
    "fcos\n\t"
    "fmul %st(1), %st\n\t"
    "fxch %st(1)\n\t"
    "fstp %st(0)\n\t"
    "fcoml 16(%ebx)\n\t"
    "fnstsw %ax\n\t"
    "testb $1, %ah\n\t"
    "jz 3f\n\t"
    "fsubl 24(%ebx)\n\t"
    "jmp 4f\n"
    "3:\n\t"
    "faddl 24(%ebx)\n"
    "4:\n\t"
    "fnstcw 8(%esp)\n\t"
    "movw 8(%esp), %ax\n\t"
    "orb $0x0c, %ah\n\t"
    "movw %ax, 10(%esp)\n\t"
    "fldcw 10(%esp)\n\t"
    "fistpll 0(%esp)\n\t"
    "fldcw 8(%esp)\n\t"
    "movl 40(%esp), %ecx\n\t"
    "movl 0(%esp), %eax\n\t"
    "movl %eax, (%ecx)\n\t"
    "movl 12(%esp), %eax\n\t"
    "addl $16, %esp\n\t"
    "popl %ebx\n\t"
    "ret\n");
#else
static int32_t oss_polar_x87(int32_t ang, int32_t r, const double *k, int32_t div, int32_t *c_out)
{
    double a = div != 0 ? (double)ang * k[0] / div : (double)ang * k[0] * k[1];
    double s = sin(a) * r, c = cos(a) * r;
    *c_out = (int32_t)(c < k[2] ? c - k[3] : c + k[3]);
    return (int32_t)(s < k[2] ? s - k[3] : s + k[3]);
}
#endif

/* deg (0x8a9a38), 0.001 (0x850d98), 0.0 (0x850d90), 0.5 (0x850d88) */
static void polar_consts(double k[4])
{
    memcpy(&k[0], g_eng.data(0x8a9a38u), 8);
    memcpy(&k[1], g_eng.data(0x850d98u), 8);
    memcpy(&k[2], g_eng.data(0x850d90u), 8);
    memcpy(&k[3], g_eng.data(0x850d88u), 8);
}

void polar_div(int32_t r, int32_t ang, int32_t n, int32_t cx, int32_t cy, int32_t *ox, int32_t *oy)
{
    double k[4];
    polar_consts(k);
    int32_t c, s = oss_polar_x87(ang, r, k, n, &c);
    *ox = cx + s;
    *oy = cy - c;
}

void polar_milli(int32_t cx, int32_t cy, int32_t r, int32_t ang, int32_t *ox, int32_t *oy)
{
    double k[4];
    polar_consts(k);
    int32_t c, s = oss_polar_x87(ang, r, k, 0, &c);
    *ox = cx + s;
    *oy = cy - c;
}

/* ── small helpers ── */

void entity_fx_reset(wl_entity *e, int16_t n)
{
    e->nfx = (uint16_t)n;
    for (uint16_t i = 0; i < e->nfx; i++) e->fx[i].id = 0;
}

int32_t entity_track_on(wl_entity *e, uint16_t i, int16_t a, int16_t b)
{
    e->trk[i].a = (uint16_t)a;
    e->trk[i].b = (uint16_t)b;
    e->trk[i].on = 1;
    e->trk[i].f0c = 0;
    e->trk[i].f08 = 0;
    e->trk[i].f10 = 0;
    return 5 * i + 20;
}

void body_clip_jitter(wl_body *b)
{
    if (b->clip == NULL) return;
    b->clip_frame = (uint16_t)(mul(rnd(), *(const uint16_t *)((const uint8_t *)b->clip + 0x42)) / 0x8000);
    b->clip_timer = (uint16_t)(mul(rnd(), *(const uint16_t *)((const uint8_t *)b->clip + 0x44)) / 0x8000);
}

/* the multi-part table (entity +0x8): { a, b, u16 n, .., +0x14 / +0x18 / +0x1c } */
typedef struct parts_tbl { void *a, *b; uint16_t n; uint8_t _p0a[0x14 - 0xa]; int32_t f14, f18, f1c; } parts_tbl;

void parts_free(void *t_, int32_t del)
{
    parts_tbl *t = t_;
    t->f14 = 0;
    t->f18 = 0;
    t->f1c = 0;
    if (t->a != NULL) { g_eng.op_delete(t->a); t->a = NULL; }
    if (t->b != NULL) { g_eng.op_delete(t->b); t->b = NULL; }
    t->n = 0;
    if (del & 1) g_eng.op_delete(t);
}

int32_t entity_tint(wl_entity *e, int32_t t) { e->tint_f4 = t; e->tint_f8 = t; return t; }

int32_t tint_fade_a(int32_t k, int32_t n)
{
    int32_t i = 0;
    if (n > 0) {
        i = mul(k, 20) / n;
        if (i < 0) i = 0;
        else if (i >= 20) return 0;
    }
    return sysv(0x27a8 + 4u * (uint32_t)i);
}

int32_t tint_fade_b(int32_t k, int32_t n)
{
    int32_t i = mul(k, 20) / n;
    if (i < 0) return sysv(0x2758);
    if (i < 20) return sysv(0x2758 + 4u * (uint32_t)i);
    return sysv(0x27a4);
}

void entity_ride_part(wl_entity *e, const wl_body *b, uint16_t part)
{
    wl_entity *par = e->parent;
    if (par == NULL) return;
    const wl_body *pb = &par->body[part < par->nbody ? part : 0];
    e->par_dx = b->x - pb->w / 2 - pb->x;
    pb = &par->body[part < par->nbody ? part : 0];
    e->par_dy = b->y - (pb->h - pb->margin) / 2 - pb->y - pb->margin;
}

void body_set_angle(wl_body *b, int32_t a, int32_t mirror)
{
    b->angle = b->facing == 3 && mirror != 0 ? 360000 - a : a;
}

int entity_snd_alloc(wl_entity *e, uint16_t n)
{
    if (e->snd != NULL) {
        for (uint16_t i = 0; i < e->nsnd; i++) entity_snd_release(e, i, 0, 1);
        if (e->snd != NULL) {
            g_eng.op_delete(e->snd);
            e->snd = NULL;
        }
    }
    e->nsnd = n;
    e->snd = g_eng.op_new(36u * n);
    memset(e->snd, 0, 36u * e->nsnd);
    return 0;
}

void entity_ride(wl_entity *e, wl_entity *parent)
{
    const wl_body *b = e->body;
    e->parent = parent;
    if (parent == NULL) return;
    const wl_body *pb = parent->body;
    e->par_dx = b->x - pb->w / 2 - pb->x;
    pb = parent->body;
    e->par_dy = b->y - (pb->h - pb->margin) / 2 - pb->y - pb->margin;
}

int32_t ember_len(int32_t a) { return a != 1 ? 120 : 48; }

void entity_setup(wl_entity *e, int32_t kind, int32_t code, int32_t fac, int32_t x, int32_t y, int32_t w, int32_t h,
                  int32_t facing, uint8_t on, int32_t dx, int32_t dy, int32_t anchor, int32_t tint, int16_t nfx)
{
    e->rider_kind = kind;
    e->rider_heavy = 0;
    e->code = code;
    e->alive = 1;
    e->faction = fac;
    e->tint_f4 = 0;
    e->tint_f8 = 0;
    e->f03c = 0;
    e->weight = 1;
    e->f1c4 = 0;
    e->f1c8 = 0;
    e->f1cc = 0;
    e->ride_mode = 1;
    e->f1c = 0;
    e->f20 = 32;
    e->f24 = 0;
    e->coll_off = 0;
    e->foot = 0;
    e->no_entity_coll = 0;
    for (int i = 0; i < 8; i++) {
        e->trk[i].a = 0;
        e->trk[i].b = 0;
        e->trk[i].on = 0;
        e->trk[i].f08 = 0;
        e->trk[i].f0c = 0;
        e->trk[i].f10 = 0;
    }
    e->f0fc = tint;
    e->f100 = tint;
    e->f1e4 = 1;
    e->fe8 = 0;
    e->handle = 0;
    e->coll_mode = 0;
    e->coll_kind = 0;
    e->f1e8 = 0;
    e->f1e0 = 0;
    e->f00c = 0;
    e->parent = NULL;
    e->width = w;
    e->height = h;
    e->dst_x = dx;
    e->dst_y = dy;
    entity_anchor(e, x, y, anchor, &x, &y);
    e->fec = 0;
    e->ff0 = 1;
    for (uint16_t i = 0; i < e->nbody; i++) {
        wl_body *b = &e->body[i];
        b->active = on;
        b->x = x;
        b->y = y;
        b->w = w;
        b->h = h;
        b->margin = 0;
        b->vvel = 0;
        b->fall_cap = 0;
        b->vel_x = 0;
        b->facing = facing;
        b->vdir = 0;
        b->angle = 0;
        b->drop = 0;
        b->dst_x = dx;
        b->dst_y = dy;
        b->f66 = 0;
        b->f68 = 0;
        b->nocollide = 0;
        b->f7d = 0;
        b->f7e = 0;
        b->attach = NULL;
        b->grounded = 1;
        body_action_reset(b);
        body_set_state(b, 0);
        body_set_clip(b, NULL);
        b->nhit = 0;
        if (i == 0) {
            body_region_flags(b);
        } else {
            b->c_flag = e->body[0].c_flag;
            b->slow = e->body[0].slow;
        }
        b->frames = NULL;
    }
    entity_fx_reset(e, nfx);
    if (e->rects != NULL) {
        g_eng.op_delete(e->rects);
        e->rects = NULL;
    }
    if (e->f08 != NULL) {
        parts_free(e->f08, 1);
        e->f08 = NULL;
    }
    e->nrects = 0;
}

/* ── the structure init (0x438a60; 60501 / 60502 inline 0x426620 verbatim) ── */

void structure_init(wl_entity *e, int32_t x, int32_t y, int32_t anchor, uint32_t code, int32_t trk_b, int32_t tint,
                    int32_t h)
{
    static const struct { uint16_t code; uint16_t w, h; int8_t dx, dy, tint4, full; uint16_t trk; } S[] = {
        { 60501, 32000, 32000, 0, 0, 0, 1, 351 }, { 60502, 32000, 32000, 0, 0, 0, 1, 352 },
        { 60503, 16000, 16000, 0, 0, 0, 1, 353 }, { 60504, 32000, 32000, 0, 0, 0, 1, 354 },
        { 60511,  8000,  8000, 0, 0, 0, 1, 355 }, { 60512,  4800,  4800, 0, 0, 0, 1, 356 },
        { 60513,  9600,  9600, 0, 0, 0, 1, 357 }, { 60514,  8000,  4800, 0, 0, 0, 1, 358 },
        { 60521,  4000,  4000, 0, 0, 0, 1, 363 }, { 60522,  6400,  6400, 0, 0, 0, 0, 364 },
        { 60523,  6400,  6400, -8, -16, 0, 0, 366 }, { 60524, 12800, 12800, 0, 0, 1, 0, 367 },
        { 60525,  6400,  6400, 0, 0, 0, 0, 365 }, { 60526, 12800, 12800, 0, 0, 0, 0, 368 },
        { 60527,  6400, 12800, -32, 0, 0, 0, 368 }, { 60528, 12800, 22400, 0, 0, 1, 0, 406 },
        { 62101,  9600,  9600, 0, 0, 0, 0, 119 },
    };
    if (code == 60540) {
        entity_setup(e, 5, 60540, 0, x, y, 8000, 9600, 1, 1, 0, 0, anchor, tint == 8 ? 1 : tint, 0);
        entity_track_on(e, 0, 397, (int16_t)trk_b);
    } else if (code == 61101) {
        entity_setup(e, 5, 61101, 0, x, y, 12800, 12800, 1, 1, 0, 0, anchor, tint, 0);
        entity_track_on(e, 0, (int16_t)g_world->f27a4, (int16_t)trk_b);
    } else {
        for (size_t k = 0; k < sizeof S / sizeof *S; k++) {
            if (S[k].code != code) continue;
            entity_setup(e, 5, (int32_t)code, 0, x, y, S[k].w, S[k].h, 1, 1, S[k].dx, S[k].dy, anchor,
                         S[k].tint4 ? 4 : tint, 0);
            /* the plain-box ones get the full track record (0x426db0), the rest 0x426d70 */
            if (S[k].full) entity_set_track(e, 0, (int16_t)S[k].trk, (int16_t)trk_b, 1, 0, 0, 0);
            else entity_track_on(e, 0, (int16_t)S[k].trk, (int16_t)trk_b);
            break;
        }
    }
    if (h != 0) entity_set_handle(e, h);
}

/* ── the init ── */

int32_t particle_init(wl_particle *p, int32_t code, int32_t x, int32_t y, int32_t facing, uint32_t delay, int32_t anchor,
                      wl_entity *owner, int32_t angle, int32_t prio, int32_t fac)
{
    wl_entity *e = &p->base;
    wl_body *b = e->body;
    uint8_t on = (uint16_t)delay != 0 ? 0 : 1;
    int32_t tint_i = 17, orbit = 0, k = 0, v;
    const void *cv;

    p->f250 = 0;
    p->life = 0;
    p->age = 0;
    p->f25c = 0;
    p->f260 = 0;
    p->f268 = 0;
    p->owner = owner;
    p->prio = prio;
    p->f264 = 0;
    p->snd_on = 0;

#define SETUP(w, h, ti, n) entity_setup(e, 4, code, fac, x, y, (w), (h), 1, on, 0, 0, anchor, (ti), (n))
    switch (code) {
    case 100042:
        SETUP(3200, 3200, 26, 0);
        p->life = 60;
        b->vel_x = 8000;
        break;
    case 100043:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, 0);
        goto L344;
    case 100044:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 421, 88);
        entity_tint(e, tint_fade_b(1000, 1000));
        orbit = 1;
        body_set_clip(b, clip(0x643508));
        break;
    case 100045: k = 8; goto L341;
    case 100046: k = 16; goto L341;
    case 100047: {
        int32_t r = rnd();
        SETUP(3200, 3200, mul(r, 100) / 0x8000 >= 50 ? 11 : 17, 0);
        entity_track_on(e, 0, 421, 24);
        body_set_clip(b, clip(0x647198));
        p->life = 60;
        entity_scatter(e, 0, 0, 0, 32000);
        entity_tint(e, sysv(0x2798));
        break;
    }
    case 100048:
        k = 56;
    L341:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 421, (int16_t)k);
        body_set_clip(b, clip(0x643508));
        entity_tint(e, sysv(0x2780));
        entity_ride(e, owner);
        break;
    case 100049: {
        int32_t r = rnd();
        SETUP(3200, 3200, mul(r, 100) / 0x8000 >= 50 ? 11 : 17, 0);
        entity_track_on(e, 0, 421, 48);
        body_set_clip(b, clip(0x645e78));
        entity_tint(e, tint_fade_b(1, 1));
        entity_scatter(e, 0, 0, -24000, 16000);
        break;
    }
    case 100050:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 48);
        goto L249;
    case 100051:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 48);
        body_set_clip(b, clip(0x643838));
        goto L251;
    case 100052:
    case 100058:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, (int16_t)ember_len(fac));
        goto L263;
    case 100053:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 421, 64);
    L249:
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        orbit = 1;
        entity_scatter(e, 120000, 20000, -1000, 2000);
        break;
    case 100054:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 421, 64);
        body_set_clip(b, clip(0x643508));
        goto L251;
    case 100055:
        SETUP(3200, 3200, 11, 0);
        entity_track_on(e, 0, 420, fac != 1 ? 120 : 48);
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        b->vvel = mul(rnd(), 20000) / 0x8000;
        p->f25c = 0;
        entity_ride(e, owner);
        break;
    case 100056:
        SETUP(3200, 3200, 25, 0);
        goto L262;
    case 100057:
        SETUP(3200, 3200, 17, 0);
    L262:
        entity_track_on(e, 0, 420, 48);
    L263:
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        b->vvel = mul(rnd(), 20000) / 0x8000 - 10000;
        p->f25c = mul(rnd(), 5) / 0x8000 + 5;
        break;
    case 100059:
        if (mul(rnd(), 100) / 0x8000 < 50) tint_i = 11;
        SETUP(3200, 3200, tint_i, 0);
        entity_track_on(e, 0, 420, 48);
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        b->vvel = mul(rnd(), 20000) / 0x8000 - 10000;
        p->f25c = mul(rnd(), 5) / 0x8000 + 5;
        entity_ride(e, owner);
        break;
    case 100060:
        if (mul(rnd(), 100) / 0x8000 < 50) tint_i = 11;
        SETUP(3200, 3200, tint_i, 0);
        entity_track_on(e, 0, 420, 48);
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        b->vvel = mul(rnd(), 20000) / 0x8000 - 10000;
        break;
    case 100061:
        SETUP(6400, 6400, 17, 0);
        entity_track_on(e, 0, 411, 24);
        body_set_clip(b, clip(0x6469a0));
        entity_tint(e, tint_fade_b(1000, 1000));
        p->life = 16;
        break;
    case 100062:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, (int16_t)ember_len(fac));
        body_set_clip(b, clip(0x643838));
        entity_tint(e, tint_fade_b(1000, 1000));
        orbit = 1;
        break;
    case 100063:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, (int16_t)ember_len(fac));
        body_set_clip(b, clip(0x643b68));
        entity_tint(e, tint_fade_b(1000, 1000));
        orbit = 1;
        b->vel_x = mul(rnd(), 20000) / 0x8000 + 10000;
        break;
    case 100064:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, (int16_t)ember_len(fac));
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        b->vvel = mul(rnd(), 20000) / 0x8000 - 10000;
        p->f25c = mul(rnd(), 15) / 0x8000 + 18;
        break;
    case 100065:
        SETUP(3200, 3200, 25, 0);
        entity_set_track(e, 0, 420, 112, 1, 0, 0, 0);
        cv = clip(0x643838);
        goto L231;
    case 100066:
        SETUP(3200, 3200, 17, 0);
        entity_set_track(e, 0, 420, 152, 1, 0, 0, 0);
        cv = clip(0x644030);
    L231:
        body_set_clip(b, cv);
        entity_tint(e, sysv(0x27a4));
        break;
    case 100067:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 48);
        body_set_clip(b, NULL);
        entity_tint(e, tint_fade_b(1000, 1000));
        b->vvel = mul(rnd(), 20000) / 0x8000;
        p->f25c = 0;
        break;
    case 100068:
        SETUP(16000, 16000, 25, 0);
        entity_track_on(e, 0, 414, 0);
        body_set_clip(b, clip(0x643e98));
        v = tint_fade_b(750, 1000);
        goto L345;
    case 100069:
        SETUP(4000, 4000, 17, 0);
        entity_set_track(e, 0, 430, 33, 1, 0, 0, 0);
        body_set_clip(b, NULL);
        entity_tint(e, sysv(0x27a4));
        e->parent = owner;
        entity_ride_part(e, e->body, 0);
        p->life = 16;
        break;
    case 100070: {
        SETUP(3200, 3200, 11, 0);
        int32_t r = rnd();
        entity_set_track(e, 0, 422, (int16_t)(12 * (mul(r, 8) / 0x8000)), 1, 0, 0, 0);
        body_set_clip(b, clip(0x643d00));
        entity_tint(e, tint_fade_a(0, 1000));
        entity_scatter(e, -20000, 40000, 0, 20000);
        p->life = (uint16_t)(mul(rnd(), 30) / 0x8000 + 100);
        break;
    }
    case 100071: tint_i = 25; goto L268;
    case 100072:
        tint_i = 11;
    L268:
        SETUP(6400, 6400, tint_i, 0);
        entity_track_on(e, 0, 411, 16);
        body_set_clip(b, clip(0x6441c8));
    L251:
        entity_tint(e, tint_fade_b(1000, 1000));
        break;
    case 100073:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 8);
        body_set_clip(b, clip(0x643508));
        entity_tint(e, tint_fade_b(1000, 1000));
        entity_scatter(e, 0, 30000, -32000, 64000);
        break;
    case 100074:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 16);
        body_set_clip(b, clip(0x643508));
        entity_tint(e, tint_fade_b(1000, 1000));
        entity_scatter(e, -30000, 60000, -30000, 60000);
        break;
    case 100075:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 16);
        goto L330;
    case 100076:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 104);
    L330:
        body_set_clip(b, clip(0x643508));
        entity_tint(e, tint_fade_b(1000, 1000));
        entity_scatter(e, 0, 30000, -30000, 60000);
        break;
    case 100077:
    case 100079:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 24);
        body_set_clip(b, clip(0x643508));
        entity_tint(e, tint_fade_b(1000, 1000));
        if (code == 100079) entity_ride(e, owner);
        break;
    case 100078:
        SETUP(3200, 3200, 11, 0);
        entity_track_on(e, 0, 420, 24);
    L344:
        body_set_clip(b, clip(0x643508));
        v = tint_fade_b(1000, 1000);
    L345:
        entity_tint(e, v);
        break;
    case 100080:
        if (mul(rnd(), 1000) / 0x8000 < 700) tint_i = 11;
        SETUP(3200, 3200, tint_i, 0);
        entity_track_on(e, 0, 427, 8);
        body_set_clip(b, clip(0x644cf0));
        entity_scatter(e, -10000, 20000, -50000, 20000);
        goto L301;
    case 100081:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 427, 8);
        body_set_clip(b, clip(0x644cf0));
        entity_scatter(e, -5000, 10000, -20000, 5000);
        entity_tint(e, tint_fade_a(7, 20));
        break;
    case 100082:
        SETUP(1, 1, 11, 0);
        break;
    case 100083:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 426, 4);
        goto L326;
    case 100084:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 426, 16);
    L326:
        body_set_clip(b, clip(0x6449c0));
        entity_tint(e, sysv(0x2780));
        entity_scatter(e, 15000, -30000, -90000, -10000);
        break;
    case 100085: k = 56; goto L313;
    case 100086: k = 176; goto L313;
    case 100087:
        k = 136;
    L313:
        angle = 1000 * (mul(rnd(), 360) / 0x8000);
        polar_milli(x, y, -4800, angle, &x, &y);
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, (int16_t)k);
        body_set_clip(b, clip(0x6436a0));
        b->vel_x = 0;
        b->angle = angle;
        entity_tint(e, sysv(0x2788));
        entity_ride(e, owner);
        goto L351;
    case 100088:
        SETUP(9600, 9600, 11, 0);
        entity_track_on(e, 0, 428, 1);
        body_set_clip(b, NULL);
        entity_tint(e, sysv(0x2758));
        entity_ride(e, owner);
        p->life = 60;
        break;
    case 100089:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 96);
        body_set_clip(b, clip(0x6436a0));
        entity_tint(e, sysv(0x2788));
        entity_scatter(e, 0, 0, 0, -20000);
        entity_ride(e, owner);
        break;
    case 100090:
        SETUP(1, 1, 17, 1);
        entity_fx_add(e, 100154, 0, 16, 0, 0, 0, 0);
        break;
    case 100091:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 40);
        body_set_clip(b, clip(0x6436a0));
        entity_tint(e, tint_fade_b(1000, 1000));
        entity_scatter(e, -30000, 60000, -40000, 50000);
        break;
    case 100092:
        SETUP(3200, 3200, 17, 0);
        b->vvel = -80000;
        p->life = 60;
        break;
    case 100093:
        SETUP(3200, 3200, 11, 0);
        entity_track_on(e, 0, 420, 64);
        body_set_clip(b, clip(0x643508));
        entity_tint(e, sysv(0x2780));
        entity_scatter(e, -20000, 40000, 0, 0);
        break;
    case 100094:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, 35);
        body_set_clip(b, clip(0x6439d0));
        v = sysv(0x2798);
        goto L307;
    case 100095: {
        int32_t r = mul(rnd(), 36) / 0x8000;
        angle = 10 * r;
        polar_milli(x, y, -4800, 10000 * r, &x, &y);
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, 76);
        body_set_clip(b, clip(0x644360));
        entity_tint(e, sysv(0x2798));
        entity_scatter(e, 0, 5000, 0, 0);
        entity_ride(e, owner);
        orbit = 1;
        break;
    }
    case 100096:
        SETUP(3200, 3200, 25, 0);
        entity_track_on(e, 0, 420, 144);
        body_set_clip(b, clip(0x6444f8));
        entity_tint(e, sysv(0x2798));
        entity_ride(e, owner);
        break;
    case 100097:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 68);
        body_set_clip(b, clip(0x6439d0));
        entity_tint(e, sysv(0x2780));
        entity_scatter(e, 0, 0, 0, 0);
        break;
    case 100098:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 68);
        body_set_clip(b, clip(0x6439d0));
        v = sysv(0x2780);
    L307:
        entity_tint(e, v);
        entity_scatter(e, 0, 0, 0, 0);
        entity_ride(e, owner);
        break;
    case 100099:
        SETUP(4000, 4000, 11, 0);
        entity_track_on(e, 0, 363, 40);
        body_set_clip(b, clip(0x647990));
        body_clip_jitter(b);
        break;
    case 100100:
        SETUP(3200, 3200, 6, 0);
        entity_track_on(e, 0, 426, 8);
        body_set_clip(b, clip(0x644b58));
        entity_scatter(e, -10000, 5000, -1000, 1000);
        if ((**g_eng.keycfg)->effects != 0) entity_tint(e, sysv(0x27f0));
        break;
    case 100101:
        SETUP(6400, 6400, 17, 0);
        entity_track_on(e, 0, 416, 0);
        body_set_clip(b, clip(0x644828));
        entity_tint(e, sysv(0x2780));
        break;
    case 100102:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 144);
        body_set_clip(b, NULL);
        entity_tint(e, sysv(0x2794));
        orbit = 1;
        b->vel_x = mul(rnd(), 20000) / 0x8000 + 70000;
        break;
    case 100103:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 426, 8);
        body_set_clip(b, clip(0x644cf0));
        entity_scatter(e, -5000, 10000, -25000, 10000);
        entity_tint(e, sysv(0x27c8));
        break;
    case 100104:
        SETUP(3200, 3200, 11, 0);
        entity_track_on(e, 0, 426, 6);
        body_set_clip(b, clip(0x6449c0));
        entity_tint(e, sysv(0x27d0));
        break;
    case 100105:
        SETUP(3200, 3200, 11, 0);
        entity_track_on(e, 0, 426, 0);
        body_set_clip(b, clip(0x644e88));
        b->f58 = (uint16_t)(mul(rnd(), 2) / 0x8000);
        p->f268 = 1;
        break;
    case 100106:
        SETUP(9600, 9600, 17, 0);
        k = mul(rnd(), 1000) / 0x8000 % 8;
        entity_track_on(e, 0, 120, (int16_t)k);
        body_set_clip(b, NULL);
        b->vvel = mul(rnd(), 50000) / 0x8000 + 20000;
        p->snd_on = 1;
        break;
    case 100107:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 88);
        body_set_clip(b, clip(0x645020));
        entity_ride(e, owner);
        if ((**g_eng.keycfg)->effects != 0) entity_tint(e, sysv(0x27e8));
        break;
    case 100108:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 160);
        body_set_clip(b, clip(0x645020));
        entity_ride(e, owner);
        goto L281;
    case 100109:
        if (mul(rnd(), 100) / 0x8000 < 50) tint_i = 11;
        SETUP(3200, 3200, tint_i, 0);
        entity_set_track(e, 0, 420, 168, 1, 0, 0, 0);
        body_set_clip(b, NULL);
        entity_tint(e, sysv(0x2798));
        e->parent = owner;
        entity_ride_part(e, e->body, 0);
        e->body->vvel = mul(rnd(), -8000) / 0x8000;
        rnd();
        e->body->vel_x = 0;
        break;
    case 100110:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 421, 0);
        body_set_clip(b, clip(0x645ce0));
    L281:
        if ((**g_eng.keycfg)->effects != 0) entity_tint(e, sysv(0x27d8));
        break;
    case 100111:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 80);
        body_set_clip(b, clip(0x6451b8));
        entity_ride(e, owner);
        p->life = 100;
        break;
    case 100112:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 84);
        body_set_clip(b, clip(0x645350));
        entity_ride(e, owner);
        p->life = 30;
        break;
    case 100113:
        SETUP(3200, 3200, 17, 0);
        entity_track_on(e, 0, 420, 128);
        body_set_clip(b, clip(0x6454e8));
        entity_ride(e, owner);
        break;
    case 100114: k = 0; goto L276;
    case 100115: k = 8; goto L276;
    case 100116:
        k = 16;
    L276:
        if (mul(rnd(), 1000) / 0x8000 < 700) tint_i = 11;
        SETUP(6400, 6400, tint_i, 0);
        entity_track_on(e, 0, 430, (int16_t)k);
        body_set_clip(b, clip(0x645680));
    L301:
        entity_tint(e, sysv(0x27d0));
        break;
    case 100117:
    case 100118:
        SETUP(3200, 3200, code == 100117 ? 25 : 11, 0);
        entity_set_track(e, 0, 420, 64, 1, 0, 0, 0);
        body_set_clip(b, clip(0x643508));
        entity_tint(e, sysv(0x27a4));
        e->parent = owner;
        entity_ride_part(e, e->body, 0);
        e->body->vvel = mul(rnd(), -20000) / 0x8000;
        e->body->vel_x = mul(rnd(), 10000) / 0x8000 + (code == 100117 ? -40000 : 30000);
        break;
    case 100119:
        SETUP(6400, 6400, 17, 0);
        entity_set_track(e, 0, 411, 0, 1, 0, 0, 0);
        goto L162;
    case 100120:
        SETUP(6400, 6400, 17, 0);
        entity_set_track(e, 0, 411, 8, 1, 0, 0, 0);
    L162:
        body_set_clip(b, clip(0x646340));
        entity_tint(e, sysv(0x2798));
        break;
    case 100121:
    case 100122:
        SETUP(12800, 9600, code == 100121 ? 17 : 11, 0);
        entity_set_track(e, 0, 431, code == 100121 ? 0 : 6, 1, 0, 0, 0);
        body_set_clip(b, clip(0x6464d8));
        entity_tint(e, sysv(0x27a8));
        e->parent = owner;
        entity_ride_part(e, e->body, 0);
        break;
    case 100123:
    case 100124: {
        cv = clip(mul(rnd(), 100) / 0x8000 >= 50 ? 0x646808 : 0x646670);
        SETUP(3200, 3200, 17, 0);
        entity_set_track(e, 0, 421, code == 100123 ? 80 : 72, 1, 0, 0, 0);
        body_set_clip(b, cv);
        if (code == 100123) entity_tint(e, sysv(0x27a4));
        wl_body *b0 = e->body;
        b0->vvel = mul(rnd(), 40000) / 0x8000 - 20000;
        b0->vel_x = mul(rnd(), 40000) / 0x8000 - 20000;
        p->life = (uint16_t)(code == 100123 ? mul(rnd(), 20) / 0x8000 + 30 : mul(rnd(), 50) / 0x8000 + 50);
        body_clip_jitter(b);
        break;
    }
    case 100125:
        SETUP(12800, 12800, 17, 0);
        entity_set_track(e, 0, 423, 0, 1, 0, 0, 0);
        body_set_clip(b, NULL);
        goto L96;
    case 100126:
    case 100127:
        if (code == 100126) {
            SETUP(17600, 17600, 17, 0);
            entity_set_track(e, 0, 425, 0, 1, 0, 0, 0);
        } else {
            SETUP(8000, 8000, 17, 0);
            entity_set_track(e, 0, 424, 0, 1, 0, 0, 0);
        }
        body_set_clip(b, NULL);
        e->parent = owner;
        entity_ride_part(e, e->body, 0);
    L96:
        entity_tint(e, sysv(0x2790));
        break;
    case 100128: k = 0; goto L272;
    case 100129:
        k = 24;
    L272:
        entity_setup(e, 4, code, fac, x, y, 100, 100, 1, on, -32, -32, anchor, 17, 0);
        entity_track_on(e, 0, 434, (int16_t)k);
        body_set_clip(b, clip(0x6459b0));
        b->vvel = mul(rnd(), 30000) / 0x8000 - 130000;
        b->vel_x = 8000 - mul(rnd(), 24000) / 0x8000;
        break;
    case 100130:
        SETUP(6400, 6400, 17, 0);
        entity_set_track(e, 0, 412, (int16_t)(mul(rnd(), 4) / 0x8000), 1, 0, 0, 0);
        body_set_clip(b, NULL);
        e->parent = owner;
        entity_ride_part(e, e->body, 0);
        p->life = 60;
        break;
    case 100131:
        SETUP(3200, 3200, 17, 0);
        p->life = (uint16_t)(mul(rnd(), 8) / 0x8000 + 8);
        b->angle = 1000 * (mul(rnd(), 360) / 0x8000);
        b->vel_x = mul(rnd(), 5000) / 0x8000 + 5000;
        if (mul(rnd(), 100) / 0x8000 < 50) b->vel_x = 360000 - b->vel_x;
        break;
    case 100132: goto L317;
    case 100133:
        tint_i = 11;
    L317:
        SETUP(3200, 3200, tint_i, 0);
        entity_track_on(e, 0, 420, 72);
        body_set_clip(b, clip(0x645818));
        entity_tint(e, sysv(0x2788));
        entity_ride(e, owner);
        break;
    case 100134: k = 417; goto L32;
    case 100135:
        k = 418;
    L32:
        SETUP(6400, 6400, 17, 0);
        entity_set_track(e, 0, (int16_t)k, 0, 1, 0, 0, 0);
        body_set_clip(b, clip(0x647cc0));
        body_clip_jitter(b);
        entity_tint(e, sysv(0x27a4));
        p->snd_on = 1;
        e->f0fc = 11;
        e->f100 = 11;
        break;
    case 100136:
        SETUP(19200, 9600, 17, 0);
        entity_set_track(e, 0, 433, 0, 1, 0, 0, 0);
        body_set_clip(b, NULL);
        entity_tint(e, sysv(0x2758));
        orbit = 1;
        b->vel_x = mul(rnd(), 16000) / 0x8000 + 96000;
        break;
    case 100137:
        SETUP(2400, 2400, 17, 0);
        b->vvel = -64000;
        entity_tint(e, sysv(0x27a8));
        break;
    default:
        break;
    }
#undef SETUP

    if (facing == 99) {
        body_set_angle(b, 1000 * angle, 0);
    } else {
        b->facing = facing;
        body_set_angle(b, 1000 * angle, 1);
    }
    if (orbit != 0) {
    L351:
        polar_div(1000, b->angle, 1000, 0, 0, &p->dir_x, &p->dir_y);
    }
    if (p->snd_on != 0) entity_snd_alloc(e, 1);
    if ((uint16_t)delay != 0) {
        body_action_start(b, 1, 210);
        int32_t y0 = b->y;
        b->act_ctr = (uint16_t)delay;
        p->f254 = y0;
    } else {
        p->f254 = b->y;
    }
    return 0;
}
