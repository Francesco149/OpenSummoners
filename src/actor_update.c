/* actor_update.c — FUN_0054f980 (+ FUN_00445db0, FUN_00442460, FUN_00556eb0, FUN_0047d740).
 * See actor_update.h.
 *
 * Transcribed from the asm: IDA's decompile shares labels across cases (the looping-
 * sound idiom, the wait countdown, the switch-off), which are helpers here; the cases
 * are retail's.  Quotients truncate toward zero as retail's multiply-and-shift
 * sequences do; products that could wrap are formed in uint32 as imul wraps. */
#include "actor_update.h"

#include <stddef.h>
#include <string.h>

#include "camera_follow.h"
#include "char_integrate.h"
#include "actor_ops.h"
#include "char_state.h"
#include "char_physics.h"
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
static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static int32_t sys32(uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)g_eng.sys + off, 4); return v; }
/* the time-of-day phase (0x8a93fc, also the palette tint's selector) */
static int32_t time_phase(void) { return sys32(0x289c); }

static wl_entity *particle(wl_actor *a, int32_t id, wl_body *b, int32_t dx, int32_t dy, int32_t a7, int32_t a8,
                           int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14)
{
    return particle_spawn(&a->base, &a->base, id, b, dx, dy, a7, a8, a9, a10,
                                                       a11, a12, a13, a14);
}

int32_t speed_ramp(int32_t cur, int32_t step, int32_t brake, int32_t target)
{
    if (target >= 0) {
        int32_t s = step;
        if (cur < 0 && s < 0) s = -s;
        if (s > 0) {
            if (target > cur) { int32_t v = cur + s; return target >= v ? v : target; }
            if (target == cur) return cur;
            int32_t v = cur + brake;
            return v < 0 ? 0 : v;
        }
        if (s == 0) return cur;
        int32_t v = cur + s;
        return v < 0 ? 0 : v;
    }
    if (step > 0 || cur > 0) {
        if (target < cur) { int32_t v = cur - step; return target <= v ? v : target; }
        if (target == cur) return cur;
        int32_t v = cur - brake;
        return v > 0 ? 0 : v;
    }
    if (step >= 0) return cur;
    int32_t v = cur - step;
    return v > 0 ? 0 : v;
}

int entity_ground_probe(wl_entity *e, wl_body *b, uint16_t part)
{
    int32_t y = part;   /* retail hands the mover its own argument slot */
    return entity_move_vertical(e, b, 1, b->drop, part, &y, NULL) == 0;
}

wl_entity *actor_by_handle(int32_t handle)
{
    wl_list *l = g_world->list_2790;
    uint16_t n = l->count;
    for (uint16_t i = 0; i < n; i++)
        if (l->items[i]->handle == handle) return l->items[i];
    return NULL;
}

int world_scene_running(void) { return g_world->f2770 != 0 || g_world->f2764 == 1; }

/* the looping-sound idiom the emitters inline: channel 0 is (re)started (0x450ef0) when its
 * period counter (+0x20) is 0, kept live (0x450e00), and the counter wraps at `period` */
static void snd_loop(wl_actor *a, int32_t id, int32_t vol, int32_t freq, int32_t falloff, int32_t a7, int32_t period)
{
    wl_sndch *s = a->base.snd;
    if (s == NULL) return;
    if (a->base.nsnd == 0) { entity_snd_overrun(); return; }
    if (s->f20 == 0) entity_snd_start(&a->base, 0, 0, (uint16_t)id, vol, freq, falloff, a7);
    entity_snd_keep(&a->base, 0, 0);
    if (++s->f20 >= period) s->f20 = 0;
}

/* the switch-off (inlined at codes 70216 / 70850): the first step record not done sets its
 * clip on the body and plays its sound */
static void switch_off(wl_actor *a)
{
    int32_t on = a->base.f00c;
    wl_body *b = a->base.body;
    if (on == 0) return;
    int32_t code = a->base.code;
    if (code > 0x11300) {
        if (code == 70820) entity_set_collide(&a->base, 3, 3);
    } else if (code != 70400) {
        switch (code) {
        case 70230: if (a->kind == 1 && on == 1) return; break;
        case 70251: case 70252: case 70371: g_eng.h408100(a, 0, 1); break;
        default: break;
        }
    }
    uint16_t n = a->nsteps;
    a->base.f00c = 0;
    for (uint16_t i = 0; i < n; i++) {
        if (a->steps[i].done != 0) continue;
        if (a->steps[i].clip != NULL) body_set_clip(b, a->steps[i].clip);
        if (a->steps[i].sfx != 0) sfx_play_at(&a->base, 0, a->steps[i].sfx, 0, 0, 0);
        return;
    }
}

/* the earthquake emitter's camera shake block (all fields cleared, then armed) */
static void shake_set(cam_shake *s, int32_t active, int32_t amp)
{
    s->active = 0; s->duration = 0; s->counter = 0; s->phase = 0; s->envelope = 0; s->out = 0;
    s->active = active; s->duration = 0; s->amp = amp;
}

/* the fade actor's tint: step k of the 21-entry ramp at sys+0x2758 (k >= 20: the last) */
static void fade_tint(wl_actor *a, int32_t k)
{
    int32_t t;
    if (k >= 0 && k >= 20) t = sys32(0x27a4);
    else t = sys32(0x2758 + 4u * (uint32_t)(k < 0 ? 0 : k));
    a->base.tint_f4 = t;
    a->base.tint_f8 = t;
}

/* the sound pool slots (sys+0x364 + 4*slot) */

int actor_update(wl_actor *a, wl_body *in, wl_body *out, int32_t probe, int32_t part)
{
    /* ── in -> out ── */
    out->active = in->active;
    out->x = in->x; out->y = in->y; out->w = in->w; out->h = in->h;
    out->margin = 0;
    out->vvel = in->vvel;
    out->fall_cap = 0;
    out->drop = in->drop;
    out->grounded = in->grounded; out->vel_x = in->vel_x; out->facing = in->facing;
    memcpy((uint8_t *)out + 0x30, (const uint8_t *)in + 0x30, 8);     /* +0x30, +0x34 */
    out->state = in->state; out->sub = in->sub; out->ctr = in->ctr;
    out->dst_x = a->base.dst_x; out->dst_y = a->base.dst_y;
    out->pose = in->pose; out->spell = in->spell; out->target = in->target;
    out->act_arg = in->act_arg; out->act_len = in->act_len; out->f58 = in->f58; out->act_5a = in->act_5a;
    out->act_ctr = in->act_ctr; out->serial = in->serial; out->act_64 = in->act_64; out->f66 = in->f66;
    out->clip = in->clip; out->clip_timer = in->clip_timer; out->clip_frame = in->clip_frame;
    out->clip_done = in->clip_done; out->act_78 = in->act_78;
    out->nocollide = 0; out->f7d = 0; out->f7e = 0; out->f7f = 0; out->nhit = 0;
    out->attach = in->attach; out->slow = in->slow; out->c_flag = in->c_flag;
    if (a->base.rects != NULL) char_copy_rects((const wl_char *)a, out);
    int32_t x0 = in->x, y0 = in->y;

    /* ── the ground probe ── */
    int gr;
    if (a->use_mover != 0) {
        int32_t y = out->y;
        gr = entity_move_vertical(&a->base, out, 1, out->drop, (uint16_t)part, &y, (wl_entity **)&out->attach) == 0;
        if (out->attach != NULL) out->y = y;
    } else {
        gr = entity_ground_probe(&a->base, out, (uint16_t)part);
    }

    switch (a->base.code) {
    /* ── a bobbing float (drips): f58 0 rise / 1 sink ── */
    case 70100: {
        if (a->f284 != 0) break;
        if (out->f58 == 0) {
            out->vvel = speed_ramp(out->vvel, 500, -1000, -10000);
            if (out->vvel <= -10000) out->f58 = 1;
        } else if (out->f58 == 1) {
            out->vvel = speed_ramp(out->vvel, 500, -1000, 10000);
            if (out->vvel >= 10000) out->f58 = 0;
        }
        out->y += out->vvel / 100;
        int32_t dy = out->h / 2 - mul(rnd(), 1600) / 0x8000 - 800;
        int32_t dx = mul(rnd(), 8000) / 0x8000 - 4000;
        particle(a, 100097, out, dx, dy, 99, 1, probe, 0, 2, 0, 0, 0);
        body_clip_step(out);
        break;
    }
    /* ── a growing / shrinking platform (sets the grid refresh flag) + sparkles ── */
    case 70120: {
        out->act_ctr++;
        int moved = 0;
        if (probe == 0 && out->h > 6400 && out->w > 6400 && mul(rnd(), 100) / 0x8000 == 0) {
            int32_t dy = out->h - 3200;
            int32_t r = rnd();
            int32_t dx = 100 * (mul(r, (out->w - 6400) / 100) / 0x8000 + 32);
            particle(a, 100105, out, dx, dy, 99, 2, 0, 0, 2, 0, 0, 0);
        }
        if ((a->kind == 2 || (a->kind == 3 && a->base.f00c != 1)) && probe == 0 && out->act_ctr % 20 == 0) {
            for (uint16_t k = 0; k < (a->area_x1 - a->area_x0) / 24000 + 1; k++) {
                int32_t x0b = a->area_x0;
                int32_t dy = out->y - mul(rnd(), 3200) / 0x8000 - 2400;
                int32_t r = rnd();
                int32_t dx = x0b + 100 * (mul(r, (a->area_x1 - x0b - 6400) / 100) / 0x8000 + 32);
                wl_entity *p = particle(a, 100101, out, dx, dy, 99, 0, 0, 0, 2, 0, 0, 0);
                if (p != NULL) {
                    int32_t c = mul(rnd(), 100) / 0x8000 < 50 ? 17 : 11;
                    p->f0fc = c;
                    p->f100 = c;
                }
            }
        }
        int32_t on = a->base.f00c;
        if (on == 0) {
            if (out->h < a->grow_max) {
                if (out->act_ctr % 10 == 0) {
                    out->y -= a->step;
                    out->h += a->step;
                    if (probe == 0) { int32_t one = 1; memcpy(g_world->grid + 0x28, &one, 4); }
                }
                moved = 1;
            }
        } else if (on == 1) {
            if (out->h > a->grow_min) {
                if (out->act_ctr % 10 == 0) {
                    out->y += a->step;
                    out->h -= a->step;
                    if (probe == 0) { int32_t one = 1; memcpy(g_world->grid + 0x28, &one, 4); }
                }
                moved = 1;
            }
        }
        if (out->act_ctr > 40) out->act_ctr = 0;
        if (moved == 0 || probe != 0) break;
        snd_loop(a, 0xf0, 0, 0, -5000, 0, 40);
        break;
    }
    case 70201: case 70202:
        if (actor_crate(a, out, probe, part, x0, y0, gr, 0, 200, 20000, a->base.f00c != 1, 1, 228) != 0)
            entity_snd_loop(&a->base, 0, probe, 0xdf, -1000, 0, 0, 0, 8);
        break;
    case 70205:
        actor_crate(a, out, probe, part, x0, y0, gr, 1, 200, 20000, 1, 0, 228);
        break;
    case 70210:
        if (actor_crate(a, out, probe, part, x0, y0, gr, 1, 200, 0, a->base.f00c != 1, 0, 236) != 0)
            entity_snd_loop(&a->base, 0, probe, 0xec, -1000, 0, 0, 0, 16);
        break;
    case 70216: {
        if (a->kind != 2 || probe != 0) break;
        int32_t on = a->base.f00c;
        if (on == 0) { out->act_ctr = 0; break; }
        if (on != 1) break;
        if (++out->act_ctr <= 5) break;
        switch_off(a);
        break;
    }
    case 70220:
        if (out->pose == 147) {
            if (out->act_ctr < 20) {
                out->act_ctr++;
            } else {
                a->f284 = 1;
                body_action_reset(out);
            }
        }
        break;
    case 70230:
        if (a->base.f00c == 1) g_eng.h552f60(a, IP(out), probe, -10000, 12800, 3200);
        else g_eng.h552f60(a, IP(out), probe, 80000, 3200, 12800);
        out->dst_y = (out->h - a->base.height) / 100;
        break;
    case 70240:
        if (a->area_y0 != a->area_y1) actor_slide_v(a, out, 100);
        if (++out->act_ctr > 100) {
            out->act_ctr = 0;
            g_eng.h40be00(g_world);
        }
        break;
    case 70245: case 70246: case 70247: case 70248:
        g_eng.h554570(a, IP(out), probe, part, a->base.f00c != 1, 500);
        break;
    case 70360:
        body_clip_step(out);
        break;
    case 70361:
        if ((out->clip_frame == 0 || out->clip_frame == 2) && out->clip_timer == 0 && g_world->busy == 0) {
            int32_t r = rnd();
            sfx_play_at(&a->base, probe, 226, -1500, mul(r, -500) / 0x8000, -16000);
        }
        body_clip_step(out);
        break;
    /* ── looping ambient emitters ── */
    case 70362: {
        body_clip_step(out);
        if (g_world->busy != 0) break;
        int32_t freq = 500 - mul(rnd(), 1000) / 0x8000;
        if (probe != 0) break;
        snd_loop(a, 0xe5, -1700, freq, -16000, 0, 30);
        break;
    }
    case 70370: {
        if (++out->act_ctr <= 5) break;
        out->act_ctr = 0;
        int32_t dy = mul(rnd(), 800) / 0x8000 - 800;
        int32_t r = rnd();
        particle(a, 100100, out, mul(r, 1600) / 0x8000 - 800, dy, 99, 1, probe, 0, 2, 0, 0, 0);
        break;
    }
    case 70372: {
        body_clip_step(out);
        if (g_world->busy != 0) break;
        int32_t freq = mul(rnd(), -1000) / 0x8000;
        int32_t vol = mul(rnd(), 1000) / 0x8000 - 2500;
        if (probe != 0) break;
        snd_loop(a, 0xe3, vol, freq, -16000, 0, 40);
        break;
    }
    /* ── the fountain ── */
    case 70373: {
        if (a->base.f00c == 1) break;
        out->act_ctr = (out->act_ctr + 1) % 30;
        if (probe != 0) break;
        int32_t dy = mul(rnd(), 400) / 0x8000 - 3000;
        int32_t r = rnd();
        wl_entity *p = particle(a, 100104, out, mul(r, 800) / 0x8000 - 400, dy, 99, 1, 0, 0, 2, 0, 0, 0);
        int32_t freq = 500 - mul(rnd(), 1000) / 0x8000;
        int32_t vol = mul(rnd(), 1000) / 0x8000 - 3000;
        wl_sndch *s = a->base.snd;
        if (s != NULL) {
            if (a->base.nsnd != 0) {
                if (s->f20 == 0) entity_snd_start(&a->base, 0, 0, 0x1b, vol, freq, 0, 0);
                entity_snd_keep(&a->base, 0, 0);
                if (++s->f20 >= 30) s->f20 = 0;
            } else {
                g_eng.h408b90(g_eng.log, IP(g_eng.data(0x85525cu)), 0, IP(g_eng.data(0x8a9b6cu)));
            }
        }
        if (p == NULL) break;
        switch (out->act_ctr % 3) {
        case 0: {
            wl_body *pb = p->body;
            pb->vvel = -80000 - mul(rnd(), 10000) / 0x8000;
            pb->vel_x = mul(rnd(), 10000) / 0x8000 - 30000;
            break;
        }
        case 1: entity_scatter(p, 20000, 10000, -80000, -10000); break;
        case 2: entity_scatter(p, -10000, 10000, -80000, -10000); break;
        default: break;
        }
        break;
    }
    case 70375: {
        if (probe != 0 || a->base.f00c == 1) break;
        if (++out->act_ctr <= 10) break;
        out->act_ctr = 0;
        int32_t lo = a->area_y0, x0b = a->area_x0;
        int32_t dy = lo + mul(a->area_y1 - lo, rnd()) / 0x8000;
        int32_t r = rnd();
        int32_t dx = x0b + 100 * (mul((a->area_x1 - x0b) / 100, r) / 0x8000);
        wl_entity *p = particle(a, 100101, out, dx, dy, 99, 0, 0, 0, 2, 0, 0, 0);
        if (p == NULL) break;
        if (a->kind == 1) {
            p->f0fc = 6;
            p->f100 = 6;
            break;
        }
        int32_t c = mul(rnd(), 100) / 0x8000 >= 50 ? 11 : 17;
        p->f0fc = c;
        p->f100 = c;
        break;
    }
    case 70390: {
        int32_t on = a->base.f00c;
        if (on == 0) {
            body_set_clip(out, NULL);
            break;
        }
        if (on != 1) break;
        body_set_clip(out, g_eng.data(0x671f78u));
        body_clip_step(out);
        int32_t freq = 250 - mul(rnd(), 500) / 0x8000;
        if (probe != 0) break;
        snd_loop(a, 0xe7, -800, freq, -3000, 0, 60);
        break;
    }
    case 70410: case 70411: case 70910:
        actor_crate(a, out, probe, part, x0, y0, gr, 1, 200, 0, 1, 0, 236);
        break;
    case 70420: case 70460: case 70470:
        g_eng.h554c40(a, IP(out), probe, part, x0, y0, gr);
        break;
    case 70430:
        g_eng.h553ec0(a, IP(out), probe, part, x0, y0, gr, a->base.f00c != 1);
        break;
    case 70440:
        g_eng.h552c70(a, IP(out), probe, 24000);
        break;
    case 70480: {
        int32_t v = a->base.f03c;
        if (v != 0) body_set_clip(out, v <= 0 ? g_eng.data(0x6722a8u) : g_eng.data(0x672110u));
        else body_set_clip(out, NULL);
        if (g_eng.h5534d0(a, probe, 60, 50) != 0) sfx_play_at(&a->base, probe, 228, 0, 0, 0);
        entity_carry_riders(&a->base, out->x - x0, out->y - y0, part);
        body_clip_step(out);
        break;
    }
    /* ── LCG-timed one-shot ambient sounds (time-of-day / scene gated) ── */
    case 70501: case 70502: case 70503: case 70520: case 70521: case 100141: {
        int32_t code = a->base.code, id, lo, add, span, sel, vol;
        if (code == 70501 || code == 70502) {
            if (probe != 0 || world_scene_running() || g_world->busy != 0) break;
            int32_t t = time_phase();
            if (!(t == 2 || t == 3 || t == 4 || (code == 70502 && t == 1))) break;
            span = 300; add = 300; sel = 1; vol = -400;
        } else if (code == 70503) {
            if (probe != 0 || g_world->f2770 != 0 || g_world->f2764 == 1 || g_world->busy != 0 || time_phase() != 1)
                break;
            span = 300; add = 600; sel = 1; vol = -600;
        } else if (code == 100141) {
            body_clip_step(out);
            if (probe != 0 || g_world->busy != 0 || time_phase() == 5) break;
            span = 300; add = 500; sel = 2; vol = -1600;
        } else {
            if (probe != 0) break;
            int32_t x = out->x, y = out->y;
            if (grid_c_query(g_world->grid, x, y + 800, NULL) != 1 && grid_c_query(g_world->grid, x, y - 800, NULL) != 1)
                break;
            world_scene_running();   /* retail calls 0x47d740 here and drops the result */
            if (g_world->busy != 0 || time_phase() == 5) break;
            span = 50; add = code == 70520 ? 150 : 200; sel = code == 70520 ? 4 : 2; vol = code == 70520 ? -1000 : -1500;
        }
        if (out->f58 == 0) {
            int32_t r = rnd();
            out->f58++;
            out->act_ctr = mul(r, span) / 0x8000;
        }
        if (out->act_ctr > 0) {
            out->act_ctr--;
            entity_snd_keep(&a->base, 0, 0);
            break;
        }
        out->act_ctr = mul(rnd(), span) / 0x8000 + add;
        lo = mul(rnd(), sel) / 0x8000;
        switch (code) {
        case 70501: case 70502: case 70503:
            if ((uint16_t)lo == 0) id = code == 70501 ? 44 : code == 70502 ? 45 : 42;
            else if ((uint16_t)lo <= 3) goto tail;
            else { id = probe; if ((uint16_t)id == 0) goto tail; }
            break;
        case 70520:
            switch ((uint16_t)lo) {
            case 0: case 2: id = 50; break;
            case 1: id = 51; break;
            case 3: id = 53; break;
            default: id = probe; if ((uint16_t)id == 0) goto tail; break;
            }
            break;
        case 70521:
            switch ((uint16_t)lo) {
            case 0: id = 64; break;
            case 1: id = 65; break;
            case 2: case 3: goto tail;
            default: id = probe; if ((uint16_t)id == 0) goto tail; break;
            }
            break;
        default: /* 100141 */
            switch ((uint16_t)lo) {
            case 0: id = 183; break;
            case 1: id = 184; break;
            case 2: case 3: goto tail;
            default: id = probe; if ((uint16_t)id == 0) goto tail; break;
            }
            rnd();   /* drawn and dropped */
            entity_snd_start(&a->base, 0, 0, (uint16_t)id, vol, 0, 0, 1);
            goto tail;
        }
        {
            int32_t r = rnd();
            entity_snd_start(&a->base, 0, 0, (uint16_t)id, vol, mul(r, 1000) / 0x8000 - 500, 0, 1);
        }
        break;
    }
    /* ── ambient sound timers (0x5531b0) ── */
    case 70511:
        ambient_sound_timer(&a->base, out, probe, 300, 300, 0x2e, 0x2f, 0, 0, 2, 1, 0, 0, -200, -500, 1000, 1);
        ambient_sound_timer(&a->base, out, probe, 600, 300, 0x2b, 0, 0, 0, 1, 0, 1, 0, -400, -500, 1000, 1);
        break;
    case 70512:
        ambient_sound_timer(&a->base, out, probe, 300, 300, 0x30, 0x31, 0, 0, 2, 1, 0, 0, -200, -500, 1000, 1);
        break;
    case 70513:
        ambient_sound_timer(&a->base, out, probe, 300, 300, 0x38, 0x39, 0x3a, 0, 3, 1, 0, 0, -200, -500, 1000, 1);
        break;
    case 70514:
        ambient_sound_timer(&a->base, out, probe, 600, 300, 0x36, 0x37, 0, 0, 2, 1, 1, 0, -200, -500, 1000, 1);
        break;
    case 70515:
        ambient_sound_timer(&a->base, out, probe, 600, 300, 0x3c, 0x3d, 0, 0, 2, 1, 0, 0, 200, -200, 400, 1);
        break;
    case 70516:
        ambient_sound_timer(&a->base, out, probe, 300, 300, 0x3e, 0x3f, 0, 0, 2, 0, 0, 1, -400, -500, 1000, 1);
        break;
    case 70600:
        g_eng.h553520(a, IP(out), probe, part, x0, y0, gr);
        break;
    case 70610: case 70620:
        g_eng.h5556d0(a, IP(out), probe, 10);
        break;
    case 70700:
        g_eng.h54f140(a, IP(out), probe, part, a->base.f00c != 1, 500);
        break;
    case 70810:
        g_eng.h5556d0(a, IP(out), probe, 20);
        break;
    /* ── the earthquake: camera shake + its rumble + falling debris ── */
    case 70840: {
        if (probe != 0) break;
        int32_t on = a->base.f00c, n;
        camera_view *v = g_world->view;
        if (on == 0) {
            if (out->f58 == 0) break;
            out->f58 = 0;
            shake_set(&v->shake_x, 0, 0);
            shake_set(&g_world->view->shake_y, 0, 0);
            break;
        }
        if (on == 1) {
            if (out->f58 != 1) {
                out->f58 = 1;
                shake_set(&v->shake_x, 1, 250);
                shake_set(&g_world->view->shake_y, 1, 50);
            }
            int32_t freq = 250 - mul(rnd(), 500) / 0x8000;
            snd_loop(a, 0xe8, -1500, freq, 1, 0, 480);
            n = 50;
        } else if (on == 2) {
            if (out->f58 != 2) {
                out->f58 = 2;
                shake_set(&v->shake_x, 1, 500);
                shake_set(&g_world->view->shake_y, 1, 250);
            }
            int32_t freq = 250 - mul(rnd(), 500) / 0x8000;
            snd_loop(a, 0xe8, -500, freq, 1, 0, 480);
            n = 25;
        } else {
            break;
        }
        if (a->step == 1) break;
        if (out->act_ctr >= n) {
            for (int32_t i = a->area_x0; i < a->area_x1; i += 64000) {
                int32_t dy = out->y - 9600;
                int32_t r = rnd();
                particle(a, 100106, out, i + 100 * (mul(r, 640) / 0x8000), dy, 99, 0, 0, 0, 2, 0, 0, 0);
            }
            out->act_ctr = mul(rnd(), 20) / 0x8000;
        } else {
            out->act_ctr++;
        }
        break;
    }
    case 70850:
        if (probe != 0 || a->base.f00c != 1) break;
        switch_off(a);
        break;
    /* ── a bird: perched sway (141 take off, 142 rise, 143 gone) ── */
    case 100138: {
        uint16_t pose = out->pose;
        if (pose == 141) {
            wl_body *b0 = a->base.body;
            if (probe == 0) {
                wl_voice *vo = vpool_take(sound_pool(110), 0);
                if (vo != NULL) {
                    void *buf = vo->buf;
                    int32_t ov, op;
                    if (sfx_place(&a->base, KEYCFG()->sfx_volume, 0, 0, b0, &ov, &op) != 0) {
                        g_eng.dsb_volume_5bb870(buf, ov);
                        g_eng.dsb_pan_5bb880(buf, op);
                        g_eng.dsb_freq_5bb890(buf, 0);
                        g_eng.dsb_play_5bb8c0(buf);
                    }
                }
            }
            g_eng.h556e50(IP(a), 0, probe);
            particle(a, 100126, out, 0, 0, 99, 1, probe, 0, 2, 1, 0, 20);
            body_action_reset(out);
            break;
        }
        if (pose == 142) {
            uint16_t f = out->f58;
            if (f == 0) {
                g_eng.h556e50(IP(a), 0, probe);
                particle(a, 100127, out, 0, 0, 99, 1, probe, 0, 2, 1, 0, 20);
                out->f58++;
                break;
            }
            if (f != 1) break;
            int32_t v = out->vvel;
            if (v >= 1000) {
                if (v > 1000) v = v - 100 < 0 ? 0 : v - 100;
            } else {
                v += 100;
                if (v > 1000) v = 1000;
            }
            out->vvel = v;
            if (entity_move_vertical(&a->base, out, v / 100, 0, (uint16_t)part, &out->y, NULL) != 0) break;
        } else if (pose != 143) {
            if (out->f58 == 0) {
                out->vvel = speed_ramp(out->vvel, 200, -500, -5000);
                if (out->vvel <= -5000) out->f58 = 1;
            } else if (out->f58 == 1) {
                out->vvel = speed_ramp(out->vvel, 200, -500, 5000);
                if (out->vvel >= 5000) out->f58 = 0;
            }
            out->y += out->vvel / 100;
            break;
        }
        g_eng.h556e50(IP(a), 0, probe);
        break;
    }
    /* ── fade in (144) / fade out + destroy (145) / fly to the goal (146) ── */
    case 100142: {
        int32_t cx = out->x + out->w / 2, cy = out->y + out->h / 2;
        int32_t p = (int32_t)out->pose - 144;
        if (p == 0) {
            if (probe != 0) goto clip_tail;
            int32_t c = out->act_ctr;
            if (c < 45) {
                out->act_ctr = ++c;
                fade_tint(a, 20 * c / 50);
            } else {
                body_action_reset(out);
            }
        } else if (p == 1) {
            if (probe != 0) goto clip_tail;
            int32_t c = out->act_ctr;
            if (c >= 45) {
                if (a->f1f8 != NULL) {
                    actor_release(a->f1f8);
                    a->f1f8 = NULL;
                }
                wl_list_remove(g_world->list_2798, &a->base);
                entity_unlist(&a->base);
                entity_release(&a->base, 1);
                if (a->f2a8 != NULL) {
                    g_eng.op_delete(a->f2a8);
                    a->f2a8 = NULL;
                }
            } else {
                out->act_ctr = ++c;
                fade_tint(a, 20 * (45 - c) / 50);
            }
        } else {
            if (p == 2) {
                int32_t gx = a->goal_x;
                if (iabs(cx - gx) >= 400) {
                    out->facing = cx <= gx ? 1 : 3;
                    int32_t v = out->vel_x + 1000 <= 32000 ? out->vel_x + 1000 : 32000;
                    out->vel_x = v;
                    out->x += out->facing == 1 ? v / 100 : v / -100;
                } else {
                    out->vel_x = 0;
                }
                int32_t gy = a->goal_y;
                if (iabs(cy - gy) >= 400) {
                    int32_t v = out->vvel;
                    if (cy <= gy) { v += 1000; if (v > 32000) v = 32000; }
                    else { v -= 1000; if (v < -32000) v = -32000; }
                    out->vvel = v;
                    out->y += v / 100;
                } else {
                    out->vvel = 0;
                }
                if (iabs(cy - a->goal_y) < 400 && iabs(cx - a->goal_x) < 400) body_action_reset(out);
            }
            if (probe != 0) goto clip_tail;
        }
        if (a->link != 0) {
            wl_entity *t = actor_by_handle(a->link);
            if (t != NULL) entity_anchor(t, cx, cy, 2, &t->body->x, &t->body->y);
        }
        snd_loop(a, 0x68, 0, 0, 0, 0, 30);
    clip_tail:
        body_clip_step(out);
        break;
    }
    case 100143: {
        uint16_t f = out->f58;
        if (f != 0) {
            if (f == 1) {
                int32_t c = out->act_ctr;
                if (c <= 0) out->f58 = 0;
                else out->act_ctr = c - 20 < 0 ? 0 : c - 20;
            }
            break;
        }
        int32_t c = out->act_ctr;
        if (c >= 1000) {
            out->f58 = 1;
            break;
        }
        c += 20;
        out->act_ctr = c > 1000 ? 1000 : c;
        break;
    }
    /* ── a follower: sticks to another entity's part at an offset ── */
    case 100144: {
        wl_entity *t = a->follow;
        if (t != NULL) {
            out->x = a->follow_dx + part_left(t, (uint16_t)part);
            int32_t ty = char_part_top((const wl_char *)t, (uint16_t)part);
            out->y = a->follow_dy + ty;
            entity_carry_riders(&a->base, out->x - x0, out->y - y0, part);
        }
        break;
    }
    case 100145: {
        if (out->act_ctr != 0) {
            out->act_ctr = 0;
            break;
        }
        int32_t dx = mul(rnd(), 32000) / 0x8000 - 16000;
        int32_t r = rnd();
        particle(a, 100070, out, dx, mul(r, 8000) / 0x8000 - 24000, 99, 1, probe, 0, 2, 0, 0, 0);
        out->act_ctr = 1;
        break;
    }
    default:
        break;
    }
tail:
    return body_region_flags(out);
}
