/* particle_tick.c — FUN_0046e510 (+ FUN_00471080).  See particle_tick.h.
 *
 * Transcribed from the asm + decompile.  IDA's decompile shares labels across the arms
 * (the move / fall / kill tails); here they are the small helpers below.  Retail's
 * min()/max() macros evaluate a rand() expression TWICE when it is under the cap (codes
 * 100050/100053/100080/100081): the double draw is reproduced.  0x556180 (the entity
 * kill) is followed by the common tail unless noted, exactly as retail. */
#include "particle_tick.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "char_integrate.h"
#include "char_state.h"
#include "engine_calls.h"
#include "particle_spawn.h"
#include "entity_life.h"
#include "entity_fx.h"
#include "entity_move.h"
#include "npc_ai.h"
#include "rng.h"
#include "sound_fx.h"

#define IP(p) ((intptr_t)(p))
#define KEYCFG() (**g_eng.keycfg)

static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t sys32(uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)g_eng.sys + off, 4); return v; }
static int32_t grid32(uint32_t off) { int32_t v; memcpy(&v, g_world->grid + off, 4); return v; }

static void kill(wl_particle *p) { entity_release(&p->base, 1); }
static wl_entity *spawn(wl_particle *p, wl_entity *owner, int32_t id, wl_body *b, int32_t dx, int32_t dy, int32_t a7,
                        int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14)
{
    return particle_spawn(&p->base, owner, id, b, dx, dy, a7, a8, a9, a10,
                                                       a11, a12, a13, a14);
}

static void tint(wl_particle *p, int32_t t) { p->base.tint_f4 = t; p->base.tint_f8 = t; }
/* the fade-out ramp sys+0x27a8 (20 steps; k >= 20 => untinted, k < 0 => step 0) */
static void tint_a(wl_particle *p, int32_t k) { tint(p, k < 0 ? sys32(0x27a8) : k >= 20 ? 0 : sys32(0x27a8 + 4u * (uint32_t)k)); }
/* the fade-in ramp sys+0x2758 (k >= 20 => sys+0x27a4, k < 0 => step 0) */
static void tint_b(wl_particle *p, int32_t k) { tint(p, k >= 20 ? sys32(0x27a4) : sys32(0x2758 + 4u * (uint32_t)(k < 0 ? 0 : k))); }

/* x += vel_x / 100 facing right (facing 1), / -100 otherwise */
static void move_x(wl_body *b) { b->x += b->facing == 1 ? b->vel_x / 100 : b->vel_x / -100; }
/* the direction-vector drift: x += v * dir_x / 100 / 1000 (y the same with dir_y) */
static int32_t dir_dx(const wl_particle *p, int32_t v) { return mul(v, p->dir_x) / 100 / 1000; }
static int32_t dir_dy(const wl_particle *p, int32_t v) { return mul(v, p->dir_y) / 100 / 1000; }
/* vvel = v, then the facing move and the fall (the shared LABEL_392/397 tails) */
static void fall(wl_body *b, int32_t v) { b->vvel = v; move_x(b); b->y += v / 100; }
static void kill_when_done(wl_particle *p, const wl_body *b) { if (b->clip_done != 0) kill(p); }
/* the +-1000 sway the floating motes share (f58 0 right / 1 left, act_ctr the hold) */
static void sway(wl_body *b, int32_t hold)
{
    if (b->f58 != 0) {
        int32_t v = b->vel_x - 1000;
        b->vel_x = v < -5000 ? -5000 : v;
        if (b->act_ctr <= 0) { b->f58 = 0; b->act_ctr = hold; }
        else b->act_ctr--;
    } else {
        int32_t v = b->vel_x + 1000;
        b->vel_x = v > 5000 ? 5000 : v;
        if (b->act_ctr <= 0) { b->f58 = 1; b->act_ctr = hold; }
        else b->act_ctr--;
    }
}

/* ── the orbit's x87 sequence (0x46ed8b..0x46edeb): a = ang * k0 * k1;
 * s = __ftol(sin(a) * r +- 0.5);  c = 0x471080(double(cos(a) * r)) — round half away.
 * Run under the caller's FPU control word (retail's, inside rhost). ── */
#if defined(__i386__)
extern int32_t oss_orbit_x87(int32_t ang, int32_t r, const double *k, int32_t *c_out) __asm__("oss_orbit_x87");
__asm__(
    ".text\n"
    ".p2align 4\n"
    "oss_orbit_x87:\n\t"
    "pushl %ebx\n\t"
    "subl $24, %esp\n\t"
    "movl 40(%esp), %ebx\n\t"
    "fildl 32(%esp)\n\t"
    "fmull (%ebx)\n\t"
    "fmull 8(%ebx)\n\t"
    "fildl 36(%esp)\n\t"
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
    "fnstcw 16(%esp)\n\t"
    "movw 16(%esp), %ax\n\t"
    "orb $0x0c, %ah\n\t"
    "movw %ax, 18(%esp)\n\t"
    "fldcw 18(%esp)\n\t"
    "fistpll 0(%esp)\n\t"
    "fldcw 16(%esp)\n\t"
    "movl 0(%esp), %eax\n\t"
    "movl %eax, 20(%esp)\n\t"
    "fxch %st(1)\n\t"
    "fcos\n\t"
    "fmul %st(1), %st\n\t"
    "fstpl 8(%esp)\n\t"
    "fstp %st(0)\n\t"
    "fldl 8(%esp)\n\t"
    "fcompl 16(%ebx)\n\t"
    "fldl 8(%esp)\n\t"
    "fnstsw %ax\n\t"
    "testb $1, %ah\n\t"
    "jz 3f\n\t"
    "fsubl 24(%ebx)\n\t"
    "jmp 4f\n"
    "3:\n\t"
    "faddl 24(%ebx)\n"
    "4:\n\t"
    "fnstcw 16(%esp)\n\t"
    "movw 16(%esp), %ax\n\t"
    "orb $0x0c, %ah\n\t"
    "movw %ax, 18(%esp)\n\t"
    "fldcw 18(%esp)\n\t"
    "fistpll 0(%esp)\n\t"
    "fldcw 16(%esp)\n\t"
    "movl 44(%esp), %ecx\n\t"
    "movl 0(%esp), %eax\n\t"
    "movl %eax, (%ecx)\n\t"
    "movl 20(%esp), %eax\n\t"
    "addl $24, %esp\n\t"
    "popl %ebx\n\t"
    "ret\n");
#else
static int32_t oss_orbit_x87(int32_t ang, int32_t r, const double *k, int32_t *c_out)
{
    double a = (double)ang * k[0] * k[1];
    double s = sin(a) * r, c = cos(a) * r;
    *c_out = (int32_t)(c < k[2] ? c - k[3] : c + k[3]);
    return (int32_t)(s < k[2] ? s - k[3] : s + k[3]);
}
#endif

static void orbit(wl_particle *p, wl_body *b)
{
    double k[4];
    memcpy(&k[0], g_eng.data(0x8a9a38u), 8);   /* degrees -> radians (set at boot)    */
    memcpy(&k[1], g_eng.data(0x850d98u), 8);   /* 0.001 (the angle is x1000 degrees)  */
    memcpy(&k[2], g_eng.data(0x850d90u), 8);   /* 0.0                                 */
    memcpy(&k[3], g_eng.data(0x850d88u), 8);   /* 0.5                                 */
    for (int n = 2; n != 0; n--) {
        int32_t f34;
        b->act_ctr++;
        memcpy(&f34, (uint8_t *)b + 0x34, 4);
        int32_t ang = (b->vel_x + f34) % 360000;
        memcpy((uint8_t *)b + 0x34, &ang, 4);
        int32_t c, s = oss_orbit_x87(ang, p->f260, k, &c);
        const wl_body *ob = p->owner->body;
        b->x = ob->w / 2 - b->w / 2 + ob->x + s;
        ob = p->owner->body;
        int32_t r = p->f25c, mg = ob->margin;
        int32_t y = mul(-c, 200 * r / 3200) / 1000 + (ob->h - mg) / 2 - 200 * b->act_ctr - b->h / 2;
        b->y = y + ob->y + mg + r;
        memcpy(&ang, (uint8_t *)b + 0x34, 4);
        spawn(p, p->owner, ang < 45000 || ang >= 225000 ? 100133 : 100132, b, 0, 0, 99, 1, 0, 0, 2, 0, 0, 0);
    }
}

/* a one-shot positional sound from pool `slot` placed on the particle's body */
static void place_sound(wl_particle *p, uint32_t slot, int32_t vol, int32_t pan, int32_t freq)
{
    wl_body *b0 = p->base.body;
    wl_voice *v = vpool_take(sound_pool(slot), 0);
    if (v == NULL) return;
    void *buf = v->buf;
    int32_t ov, op;
    if (sfx_place(&p->base, vol, pan, 0, b0, &ov, &op) == 0) return;
    g_eng.dsb_volume_5bb870(buf, ov);
    g_eng.dsb_pan_5bb880(buf, op);
    g_eng.dsb_freq_5bb890(buf, freq);
    g_eng.dsb_play_5bb8c0(buf);
}

/* 100106: the falling rock re-seats channel 0 on a fresh rumble voice (pool 233) */
static void rock_rumble(wl_particle *p)
{
    int32_t freq = mul(rnd(), 2000) / 0x8000 - 1000;
    wl_sndch *ch = p->base.snd;
    if (ch == NULL) return;
    if (p->base.nsnd == 0) { entity_snd_overrun(); return; }
    if (sfx_place(&p->base, 500, -3000, 0, p->base.body, NULL, NULL) == 0) return;
    ch = p->base.snd;
    void *pool = sound_pool(233);
    if (ch->voice != NULL && ch != NULL) {
        if (p->base.nsnd == 0) {
            g_eng.h408b90(g_eng.log, IP(g_eng.data(0x85525cu)), 0, IP(g_eng.data(0x8a9b6cu)));
        } else if (ch->voice != NULL) {
            ch->voice->owner = 0;
            ch->voice = NULL;
            ch->pool = NULL;
            ch->f20 = 0;
        }
    }
    wl_voice *v = vpool_take(pool, 1);
    if (v == NULL) return;
    ch->pool = pool;
    ch->voice = v;
    ch->vol = 500;
    ch->freq = freq;
    ch->falloff = -3000;
    ch->live = 1;
    ch->fading = 0;
    ch->fade = 0;
    g_eng.dsb_volume_5bb870(v->buf, -10000);
    g_eng.dsb_play_5bb8c0(v->buf);
}

/* 100121 / 100122's tail: face the way the parent faces */
static void face_parent(wl_particle *p, wl_body *b)
{
    wl_entity *par = p->base.parent;
    if (par == NULL) return;
    if (par->nbody == 0) {
        char buf[256];
        snprintf(buf, sizeof buf, "%d", par->code);
        g_eng.h408b90(g_eng.log, IP(g_eng.data(0x85451cu)), 0, IP(buf));
    }
    b->facing = par->body->facing;
}

/* 100136 / 100137 table: the record (736 B) whose first dword is `id` */
static const uint8_t *skill_rec(int32_t id)
{
    const uint8_t *t = g_eng.data(0x6c0cd0u);
    int32_t v;
    memcpy(&v, t, 4);
    if (v == 0) return NULL;
    for (uint16_t i = 0;;) {
        if (id == v) return t + 736u * i;
        i++;
        memcpy(&v, t + 736u * i, 4);
        if (v == 0) return NULL;
    }
}

void particle_tick(wl_particle *p)
{
    wl_body *b = p->base.body;

    /* ── the delayed start (pose 1) ── */
    if (b->pose == 1) {
        if (b->act_ctr > 0) {
            b->act_ctr--;
        } else {
            b->active = 1;
            body_action_reset(b);
        }
        if (b->active == 0) return;
    }
    /* ── riding a parent entity ── */
    if (p->base.parent != NULL) {
        const wl_body *pb = p->base.parent->body;
        b->x = pb->w / 2 + pb->x + p->base.par_dx;
        b->y = part_center_y(p->base.parent, 0) + p->base.par_dy;
    }

    switch (p->base.code) {
    case 100042: fall(b, b->vvel + 2000 > 40000 ? 40000 : b->vvel + 2000); break;
    case 100043: case 100045: case 100046: case 100048: case 100077: case 100078: case 100079: case 100101:
    case 100051: case 100054: case 100068: case 100071: case 100072:
    case 100065: case 100066: case 100119: case 100120: case 100132: case 100133:
        kill_when_done(p, b);
        break;
    case 100044: case 100062: {
        uint16_t f = b->f58;
        int step = 0;
        if (f == 0) {
            int32_t v = b->vel_x;
            b->x += dir_dx(p, v);
            b->y += dir_dy(p, v);
            step = ++b->act_ctr >= 4;
        } else if (f == 1) {
            int32_t v = mul(500, b->vel_x) / 1000;
            b->x += mul(v, p->dir_x) / -100 / 1000;
            b->y += mul(v, p->dir_y) / -100 / 1000;
            step = ++b->act_ctr >= 4;
        }
        if (step) {
            b->act_ctr = 0;
            b->f58 = (uint16_t)(f + 1);
        }
        if (p->base.code == 100062 && b->f58 != 0) {
            int32_t v = b->vvel - 500;
            if (v < -20000) v = -20000;
            b->vvel = v;
            b->y += v / 100;
        }
        kill_when_done(p, b);
        break;
    }
    case 100047: fall(b, b->vvel + 500 > 32000 ? 32000 : b->vvel + 500); break;
    case 100049:
        move_x(b);
        b->y += b->vvel / 100;
        kill_when_done(p, b);
        break;
    case 100050: case 100053:
        if (b->f58 == 0) {
            if (++b->act_ctr > 10) {
                int32_t v = b->vel_x - mul(rnd(), 4000) / 0x8000 - 6000;
                v = v < 0 ? 0 : b->vel_x - mul(rnd(), 4000) / 0x8000 - 6000;
                b->vel_x = v;
                if (v <= 0) {
                    b->f58++;
                    body_set_clip(b, g_eng.data(0x643838u));
                }
            }
            int32_t v = b->vel_x;
            b->x += dir_dx(p, v);
            b->y += b->vvel / 100 + dir_dy(p, v);
        } else if (b->f58 == 1) {
            b->vvel -= 2000;
            move_x(b);
            b->y += b->vvel / 100;
            kill_when_done(p, b);
        }
        break;
    case 100052: case 100055: case 100056: case 100057: case 100058: case 100059: case 100064: case 100067:
        if (++b->act_ctr > p->f25c && b->clip != g_eng.data(0x643838u)) body_set_clip(b, g_eng.data(0x643838u));
        fall(b, b->vvel - 2000 < -40000 ? -40000 : b->vvel - 2000);
        kill_when_done(p, b);
        break;
    case 100060:
        b->act_ctr++;
        body_set_clip(b, g_eng.data(0x643838u));
        fall(b, b->vvel - 2000 < -20000 ? -20000 : b->vvel - 2000);
        kill_when_done(p, b);
        break;
    case 100061: {
        if (b->f58 == 0 && b->clip_done != 0) {
            body_set_clip(b, g_eng.data(0x646b38u));
            b->f58 = 1;
        }
        int32_t age = p->age, life = p->life, n;
        if (age <= life) n = age >= 6 ? life - age : life - 6;
        else n = 0;
        tint_b(p, 20 * n / (life - 6));
        break;
    }
    case 100063: {
        int32_t v = b->vel_x;
        b->x += dir_dx(p, v);
        b->y += mul(v, p->dir_y) / 100 / 1000 + b->vvel / 100;
        kill_when_done(p, b);
        break;
    }
    case 100069: {
        int32_t life = p->life, n = p->age <= p->life ? life - p->age : 0;
        tint_b(p, 20 * n / life);
        break;
    }
    case 100070: {
        b->vvel = b->vvel + 500 > 20000 ? 20000 : b->vvel + 500;
        b->vel_x = b->vel_x <= 0 ? b->vel_x + 500 : b->vel_x - 500;
        int32_t k = p->age >= 20 ? 20 * (p->life - p->age) / 20 : 20 * p->age / 20;
        tint_a(p, k);
        move_x(b);
        b->y += b->vvel / 100;
        kill_when_done(p, b);
        break;
    }
    case 100073: case 100074: case 100075: case 100076:
        b->vel_x = b->vel_x + 1000 > 64000 ? 64000 : b->vel_x + 1000;
        move_x(b);
        b->y += b->vvel / 100;
        kill_when_done(p, b);
        break;
    case 100080: case 100081: {
        int32_t v = mul(rnd(), 1000) / 0x8000 + b->vvel + 1500;
        v = v > 20000 ? 20000 : mul(rnd(), 1000) / 0x8000 + b->vvel + 1500;
        b->vvel = v;
        move_x(b);
        b->y += v / 100;
        if (b->clip_done != 0) kill(p);
        int32_t c = ++b->act_ctr, k;
        if (p->base.code == 100080) k = 10 - c / 4 < 2 ? 2 : 10 - c / 4;
        else k = 7 - c / 4 < 1 ? 1 : 7 - c / 4;
        tint_a(p, 20 * k / 20);
        break;
    }
    case 100082: {
        if (++b->act_ctr <= 0) break;
        if ((int32_t)b->f58 >= p->f260) { kill(p); break; }
        b->f58++;
        b->act_ctr = 0;
        tint_a(p, 20 * (p->f25c - b->f58) / 20);
        break;
    }
    /* ── the splash drops / fountain spray: fall, fade, die in water ── */
    case 100083: case 100084: case 100104: {
        int32_t v = b->vvel + 8000 > 80000 ? 80000 : b->vvel + 8000;
        b->vvel = v;
        move_x(b);
        b->y += v / 100;
        if (b->act_ctr >= 2) {
            b->act_ctr = 0;
            if (b->f58 >= 8) {
                kill(p);
            } else {
                b->f58++;
                tint(p, sys32(0x2780 - 4u * b->f58));
            }
        } else {
            b->act_ctr++;
        }
        if (grid_c_query(g_world->grid, b->x, b->y, NULL) == 0) b->active = 1;
        else if (b->vvel > 0) kill(p);
        else b->active = 0;
        break;
    }
    case 100085: case 100086: case 100087: case 100095: {
        int32_t cap = p->base.code == 100095 ? 40000 : 32000, add = p->base.code == 100095 ? 500 : 1000;
        int32_t v = b->vel_x + add > cap ? cap : b->vel_x + add;
        b->vel_x = v;
        b->x += dir_dx(p, v);
        b->y += dir_dy(p, v);
        kill_when_done(p, b);
        break;
    }
    case 100088: {
        int32_t age = p->age, k = 19;
        if (age <= 20) k = 20 * age / 20;
        else if (age > 40) k = 20 * (p->life - age) / (p->life - 40);
        tint(p, sys32(0x2758 + 4u * (uint32_t)k));
        break;
    }
    case 100089: fall(b, b->vvel - 2000 < -80000 ? -80000 : b->vvel - 2000); kill_when_done(p, b); break;
    case 100091: fall(b, b->vvel + 1000 <= 48000 ? b->vvel + 1000 : 48000); kill_when_done(p, b); break;
    case 100092: {
        int32_t v = b->vvel + 4000 > 80000 ? 80000 : b->vvel + 4000;
        b->y += v / 100;
        b->vvel = v;
        int32_t dy = mul(rnd(), 1600) / 0x8000 - 800;
        int32_t r = rnd();
        spawn(p, &p->base, 100093, b, mul(r, 1600) / 0x8000 - 800, dy, 99, 1, 0, 0, 2, 0, 0, 0);
        break;
    }
    case 100093: fall(b, b->vvel + 2000 <= 80000 ? b->vvel + 2000 : 80000); kill_when_done(p, b); break;
    case 100094: case 100097: case 100098:
        fall(b, b->vvel - 2000 < -40000 ? -40000 : b->vvel - 2000);
        kill_when_done(p, b);
        break;
    case 100096:
        if (p->f264 != 0) {
            body_set_clip(b, g_eng.data(0x644690u));
            kill_when_done(p, b);
        }
        break;
    case 100099:
        if ((b->clip_frame == 0 || b->clip_frame == 2) && b->clip_timer == 0 && g_world->busy == 0) {
            int32_t freq = -3000 - mul(rnd(), 500) / 0x8000;
            place_sound(p, 226, KEYCFG()->sfx_volume - 500, -16000, freq);
        }
        break;
    case 100100: {
        int32_t v = b->vvel - 500 < -5000 ? -5000 : b->vvel - 500;
        b->vvel = v;
        move_x(b);
        b->y += v / 100;
        if (b->clip_done != 0) kill(p);
        int32_t c = ++b->act_ctr;
        if (c <= 40 || KEYCFG()->effects == 0) break;
        int32_t k = 18 - (c - 40) / 4;
        tint_a(p, 20 * (k < 2 ? 2 : k) / 20);
        break;
    }
    case 100102: {
        uint16_t f = b->f58;
        if (f != 0) {
            if (f == 1) {
                b->vel_x = b->vel_x - 16000 < 0 ? 0 : b->vel_x - 16000;
                b->vvel = b->vvel - 2000 < -40000 ? -40000 : b->vvel - 2000;
            }
        } else if (b->act_ctr >= p->f25c) {
            b->f58++;
            body_set_clip(b, g_eng.data(0x643838u));
        } else {
            b->act_ctr++;
        }
        int32_t v = b->vel_x;
        b->x += dir_dx(p, v);
        b->y += mul(v, p->dir_y) / 100 / 1000 + b->vvel / 100;
        kill_when_done(p, b);
        break;
    }
    case 100103: {
        int32_t v = b->vvel + 500 > 0 ? 0 : b->vvel + 500;
        b->vvel = v;
        if (b->vel_x < 0) b->vel_x += 500;
        if (b->vel_x > 0) b->vel_x -= 500;
        move_x(b);
        b->y += v / 100;
        if (b->clip_done != 0) kill(p);
        int32_t k = 10 - ++b->act_ctr / 4;
        tint_a(p, 20 * (k < 2 ? 2 : k) / 20);
        break;
    }
    case 100105: {
        int32_t hold = (uint16_t)(mul(rnd(), 20) / 0x8000);
        sway(b, hold);
        int32_t v = b->vvel - 1000 < -10000 ? -10000 : b->vvel - 1000;
        b->vvel = v;
        move_x(b);
        b->y += v / 100;
        if (b->clip_done != 0) kill(p);
        int32_t col = b->x / 3200, row = b->y / 3200, d0 = grid32(0x2c1030) - 1, d1 = grid32(0x2c1034) - 1;
        if (col < 0) col = 0;
        if (d0 < col) col = d0;
        if (row < 0) row = 0;
        if (d1 < row) row = d1;
        if ((uint32_t)grid32(12u * (uint32_t)((col << 7) + row + 138244)) >= 2u) break;
        kill(p);
        return;
    }
    case 100106: {
        if (b->f58 == 0) {
            rock_rumble(p);
            b->f58++;
        }
        entity_snd_keep(&p->base, 0, 0);
        int32_t v = b->vvel + 1000 > 100000 ? 100000 : b->vvel + 1000;
        b->vvel = v;
        b->y += v / 100;
        if (b->y > grid32(0x2c103c)) { kill(p); return; }
        break;
    }
    case 100107: case 100108:
        sway(b, 10);
        fall(b, b->vvel - 800 >= -10000 ? b->vvel - 800 : -10000);
        kill_when_done(p, b);
        break;
    case 100109: {
        int32_t v = b->vvel - 2000 < -20000 ? -20000 : b->vvel - 2000;
        b->vvel = v;
        b->y += v / 100;
        int32_t c = b->act_ctr;
        if (c >= 20) { kill(p); break; }
        b->act_ctr = ++c;
        int32_t k = 20 - c > 12 ? 12 : 20 - c;
        tint_b(p, 20 * k / 20);
        break;
    }
    case 100110: {
        if (b->act_ctr < 10) b->act_ctr++;
        int32_t k = 16 - b->act_ctr;
        b->vvel += 1000;
        tint_a(p, 20 * k / 20);
        move_x(b);
        b->y += b->vvel / 100;
        kill_when_done(p, b);
        break;
    }
    case 100111: case 100130: {
        if (KEYCFG()->effects != 0) {
            int32_t k = ((int32_t)p->life - (int32_t)p->age) / 2;
            if (k >= 0 && k < 20) tint(p, sys32(0x27a8 + 4u * (uint32_t)k));
        }
        int32_t r = rnd();
        int32_t hold = p->base.code == 100111 ? mul(r, 5) / 0x8000 + 10 : mul(r, 10) / 0x8000 + 20;
        sway(b, (uint16_t)hold);
        if (p->base.code == 100111) fall(b, b->vvel - 500 < -5000 ? -5000 : b->vvel - 500);
        else fall(b, b->vvel - 1000 < -20000 ? -20000 : b->vvel - 1000);
        break;
    }
    case 100113:
        if (p->f264 != 0) kill(p);
        break;
    case 100114: case 100115: case 100116:
        sway(b, 10);
        fall(b, b->vvel - 500 < -5000 ? -5000 : b->vvel - 500);
        kill_when_done(p, b);
        break;
    case 100117: case 100118:
        move_x(b);
        b->y += b->vvel / 100;
        kill_when_done(p, b);
        break;
    case 100121: case 100122: {
        if (p->base.code == 100122 && b->f58 <= 1u) {
            int32_t dy = mul(rnd(), 6400) / 0x8000 - 3200;
            int32_t r = rnd();
            spawn(p, &p->base, 100123, b, mul(r, 6400) / 0x8000 - 3200, dy, 99, 1, 0, 0, 2, 0, 0, 20);
        }
        int32_t c = ++b->act_ctr;
        uint16_t f = b->f58;
        if (f == 0) b->f58 = f = 1;
        if (f == 1) {
            if (c >= 16) b->f58++;
            else tint_a(p, (20 * c + 80) / 20);
        } else if (f == 2) {
            if (c >= 56) kill(p);
            else if (c > 40) tint_a(p, 20 * (56 - c) / 20);
        }
        face_parent(p, b);
        break;
    }
    case 100123: {
        move_x(b);
        b->y += b->vvel / 100;
        int32_t k = (int32_t)p->life - (int32_t)p->age;
        if (k < 20) tint(p, sys32(0x2758 + 4u * (uint32_t)k));
        break;
    }
    case 100124: {
        move_x(b);
        b->y += b->vvel / 100;
        tint_a(p, 20 * ((int32_t)p->life - (int32_t)p->age) / 20);
        int32_t v = b->vel_x;
        if (v >= 0) {
            if (v > 0) v = v - 400 < 0 ? 0 : v - 400;
        } else {
            v += 400;
            if (v > 0) v = 0;
        }
        b->vel_x = v;
        v = b->vvel;
        if (v >= 40000) {
            if (v > 40000) v = v - 400 < 0 ? 0 : v - 400;
        } else {
            v += 400;
            if (v > 40000) v = 40000;
        }
        b->vvel = v;
        break;
    }
    case 100125: {
        int32_t c = (b->act_ctr + 1) % 3;
        b->act_ctr = c;
        if (c == 0) tint(p, sys32(0x2790 + 4u * (uint32_t)(mul(rnd(), 5) / 0x8000)));
        break;
    }
    case 100126: {
        int32_t c = (b->act_ctr + 1) % 10, k = 2 * (5 - c);
        b->act_ctr = c;
        tint(p, sys32(0x277c + 4u * (uint32_t)(k < 0 ? -k : k)));
        break;
    }
    case 100127: {
        int32_t c = (b->act_ctr + 1) % 40, k = 10 - c / 2;
        b->act_ctr = c;
        tint(p, sys32(0x2778 + 4u * (uint32_t)(k < 0 ? -k : k)));
        break;
    }
    /* ── the bouncing ball (impact sounds from pool 39 / 40) ── */
    case 100128: case 100129: {
        uint32_t slot = p->base.code == 100128 ? 39 : 40;
        int32_t freq = mul(rnd(), 300) / 0x8000 - 200;
        uint16_t f = b->f58;
        if (f == 0) {
            b->vvel = b->vvel + 5000 > 160000 ? 160000 : b->vvel + 5000;
            int32_t dx = b->facing == 1 ? b->vel_x / 100 : b->vel_x / -100;
            if (entity_move_horizontal(&p->base, b, dx, 1, 1, 0, 0, 0) == 0) b->vel_x = -b->vel_x;
            if (entity_move_vertical(&p->base, b, b->vvel / 100, 0, 0, &b->y, NULL) != 0) break;
            int32_t v = b->vvel;
            b->vel_x /= 2;
            if (v < 0) {
                b->vvel = -v;
            } else {
                int32_t c = ++b->act_ctr;
                int32_t nv = mul(v, -500) / 1000;
                b->vvel = nv;
                if (c > 2 || nv > -2000) {
                    b->f58++;
                    b->vvel = 0;
                    b->act_ctr = 0;
                    body_set_clip(b, g_eng.data(0x645b48u));
                }
            }
            place_sound(p, slot, KEYCFG()->sfx_volume, 0, freq);
        } else if (f == 1) {
            if (b->act_ctr >= 20) {
                b->act_ctr = 0;
                b->f58 = (uint16_t)(f + 1);
            } else {
                b->act_ctr++;
            }
        } else if (f == 2) {
            int32_t c = b->act_ctr;
            if (c >= 20) { kill(p); break; }
            tint_a(p, 20 * (19 - c) / 20);
            b->act_ctr++;
        }
        break;
    }
    case 100131:
        orbit(p, b);
        break;
    case 100134: {
        int32_t freq = mul(rnd(), -1000) / 0x8000;
        int32_t vol = mul(rnd(), 1000) / 0x8000 - 2500;
        wl_sndch *s = p->base.snd;
        if (s == NULL) break;
        if (p->base.nsnd == 0) { entity_snd_overrun(); break; }
        if (s->f20 == 0) entity_snd_start(&p->base, 0, 0, 0xe3, vol, freq, -16000, 0);
        entity_snd_keep(&p->base, 0, 0);
        if (++s->f20 >= 40) s->f20 = 0;
        break;
    }
    case 100136: {
        uint16_t f = b->f58;
        if (f == 0) {
            if (b->act_ctr < 5) {
                tint_b(p, 20 * ++b->act_ctr / 8);
            } else {
                b->f58 = (uint16_t)(f + 1);
                b->act_ctr = 0;
            }
        } else if (f == 1) {
            if (b->act_ctr < 5) {
                b->act_ctr++;
            } else {
                b->act_ctr = 5;
                b->f58 = (uint16_t)(f + 1);
            }
        } else if (f == 2) {
            if (b->act_ctr > 0) tint_b(p, 20 * --b->act_ctr / 8);
            else kill(p);
        }
        int32_t v = b->vel_x;
        b->x += dir_dx(p, v);
        b->y += dir_dy(p, v);
        break;
    }
    case 100137: {
        uint16_t f = b->f58;
        switch (f) {
        case 0: {
            int32_t id = p->f25c;
            if (id > 0 && id <= 3) {
                entity_set_track(&p->base, 0, 437, 0x42, 1, 0, 0, 0);
            } else {
                const uint8_t *rec = skill_rec(id);
                if (rec == NULL) { kill(p); break; }
                uint16_t sfx;
                memcpy(&sfx, rec + 0x54, 2);
                entity_set_track(&p->base, 0, 437, (int16_t)sfx, 1, 0, 0, 0);
            }
            b->f58++;
            break;
        }
        case 1: {
            if (b->act_ctr < 5) tint_a(p, 20 * ++b->act_ctr / 5);
            int32_t v = b->vvel + 4000 > 0 ? 0 : b->vvel + 4000;
            b->vvel = v;
            if (v >= 0 && b->act_ctr >= 5) {
                b->f58++;
                b->act_ctr = 0;
            }
            break;
        }
        case 2:
            if (b->act_ctr < 40) b->act_ctr++;
            else if (p->f260 == 0) {
                b->act_ctr = 0;
                b->f58 = (uint16_t)(f + 1);
            }
            break;
        case 3: {
            int32_t c = b->act_ctr;
            if (c >= 20) { kill(p); break; }
            b->act_ctr = ++c;
            tint_a(p, 20 * (20 - c) / 20);
            break;
        }
        default:
            break;
        }
        move_x(b);
        b->y += b->vvel / 100;
        break;
    }
    default:
        break;
    }

    /* ── the common tail ── */
    if (p->life != 0 && ++p->age >= p->life) {
        kill(p);
        return;
    }
    if (p->snd_on != 0) entity_snd_update_all(&p->base);
    body_clip_step(b);
    entity_fx_tick(&p->base, b, 0);
    wl_entity *par = p->base.parent;
    if (par != NULL) {
        p->base.par_dx = b->x - par->body->w / 2 - par->body->x;
        const wl_body *pb = par->body;
        p->base.par_dy = b->y - (pb->h - pb->margin) / 2 - pb->y - pb->margin;
    }
}
