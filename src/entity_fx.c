/* entity_fx.c — FUN_0054d090 + FUN_0046d7f0 + FUN_00448cb0 / FUN_0044bbc0 (effect records)
 * and FUN_0046d180 / FUN_0046d4a0 + FUN_004537d0 + FUN_00451200 (looping-sound channels).
 * See entity_fx.h.
 *
 * Transcribed from the asm (the decompile types the channel array as FILE *).  Retail
 * divides by multiply-and-shift; every quotient here truncates toward zero exactly as
 * those sequences do, and products that could wrap are formed in uint32 as retail's
 * imul wraps. */
#include "entity_fx.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"
#include "npc_ai.h"
#include "rng.h"

#define KEYCFG() (**g_eng.keycfg)

static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t sys32(uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)g_eng.sys + off, 4); return v; }
static wl_vpool *sound_pool(uint32_t slot) { return *(wl_vpool **)((uint8_t *)g_eng.sys + 0x364 + 4u * slot); }

/* the effects-volume term every placed sound adds: (world+0x1040)+0x28, per mille */
static int32_t effects_gain(void)
{
    int32_t v;
    memcpy(&v, g_world->f1040 + 0x28, 4);
    return mul(v - 1000, 3000) / 1000;
}

/* the listener's x distance to the body's span (0 inside it) */
static int32_t span_dx(const wl_body *b, int32_t lx)
{
    int32_t l = b->x, r = b->x + b->w;
    if (lx > l && lx < r) return 0;
    int32_t dl = l - lx, dr = r - lx;
    return iabs(dl) < iabs(dr) ? dl : dr;
}

/* the round-robin voice pick (0x40b710 with owner 0, inlined at 0x54d090's two sounds):
 * NULL when the pool has no device, will not load, or every voice is owned */
static void *pool_take(wl_vpool *p)
{
    if (p->dev == NULL) return NULL;
    if (p->v == NULL && g_eng.wave_load_409e90(p) == 0) return NULL;
    if (p->n == 0) return NULL;
    wl_voice *arr = p->v;
    for (uint16_t k = 0;;) {
        uint16_t idx = (uint16_t)(((int32_t)k + (int32_t)p->next) % (int32_t)p->n);
        if (arr[idx].owner == 0) {
            p->next = (uint16_t)(idx + 1);
            arr[idx].owner = 0;
            return p->v[idx].buf;
        }
        if (++k >= p->n) return NULL;
    }
}

void view_listener(const camera_view *v, int32_t *x, int32_t *y)
{
    if (v->ear_rel != 0) {
        *x = v->ear_x + v->cur_x + v->vp_w / 2;
        *y = v->ear_y + v->cur_y + v->vp_h / 2;
    } else {
        *x = v->ear_x;
        *y = v->ear_y;
    }
}

/* ── effect records ── */

void entity_fx_remove(wl_entity *e, int32_t id)
{
    for (uint16_t i = 0; i < e->nfx; i++)
        if (e->fx[i].id == id) e->fx[i].id = 0;
}

wl_fx *entity_fx_add(wl_entity *e, int32_t id, int32_t replace, int16_t dur, int32_t f0c, int32_t y,
                     int32_t a7, int32_t unique)
{
    if (a7 != 0) return NULL;
    if (replace != 0) entity_fx_remove(e, id);
    if (unique != 0)
        for (uint16_t i = 0; i < e->nfx; i++)
            if (e->fx[i].id == id) return NULL;
    uint16_t n = e->nfx;
    for (uint16_t i = 0; i < n; i++) {
        wl_fx *r = &e->fx[i];
        if (r->id != 0) continue;
        r->id = id;
        r->dur = (uint16_t)dur;
        r->t = 0;
        r->f08 = 0;
        r->f0a = 0;
        r->f0c = f0c;
        r->y = y;
        return r;
    }
    return NULL;
}

/* The water splash (records 100146..100149): a pooled one-shot sound on the first tick,
 * louder with the vertical speed, then 4 (8 with effects on, when loud) drops. */
static void fx_splash(wl_entity *e, wl_body *b, const wl_fx *r, int32_t pid)
{
    int32_t loud = mul(iabs(b->vvel), 1000) / 100000;
    if (r->t == 0) {
        int32_t freq = 500 - mul(rnd(), 1000) / 0x8000;
        const wl_body *b0 = e->body;
        void *buf = pool_take(sound_pool(26));
        if (buf != NULL) {
            int32_t base = KEYCFG()->sfx_volume, lx, ly;
            view_listener(g_world->view, &lx, &ly);
            int32_t dx = span_dx(b0, lx), dy = b0->y + e->f24 - ly;
            if (iabs(dx) <= 128000 && iabs(dy) <= 64000) {
                int32_t ax = iabs(dx) - 8000, ay = iabs(dy) - 16000;
                if (ax < 0) ax = 0;
                if (ay < 0) ay = 0;
                int32_t vx = mul(ax, 5000) / 120000, vy = mul(ay, -5000) / 48000;
                int32_t pan = mul(dx, 10000) / 128000;
                int32_t v = vy - vx + effects_gain() + base + loud - 1500;
                if (v < -10000) v = -10000;
                else if (v > 0) v = 0;
                g_eng.dsb_volume_5bb870(buf, v);
                g_eng.dsb_pan_5bb880(buf, pan);
                g_eng.dsb_freq_5bb890(buf, freq);
                g_eng.dsb_play_5bb8c0(buf);
            }
        }
    }
    int32_t n = loud > 500 ? (KEYCFG()->effects != 0 ? 8 : 4) : 4;
    for (uint16_t k = 0; k < n; k++) {
        int32_t y = r->y;
        int32_t x = mul(rnd(), 4000) / 0x8000 + b->w / 2 + b->x - 2000;
        g_eng.particle_557370((wl_char *)e, (wl_char *)e, pid, b, x, y, 99, 0, 0, 0, 2, 0, 0, 0);
    }
}

static int fx_effects_n(void) { return (KEYCFG()->effects != 0) + 1; }

void entity_fx_tick(wl_entity *e, wl_body *b, uint16_t part)
{
    wl_char *self = (wl_char *)e;
    for (uint16_t i = 0; i < e->nfx; i++) {
        wl_fx *r = &e->fx[i];
        if (r->id == 0) continue;
        int32_t w;
        switch (r->id) {
        case 100146: if (b->slow == 0) fx_splash(e, b, r, 100083); break;
        case 100147: if (b->c_flag != 0) fx_splash(e, b, r, 100083); break;
        case 100148: if (b->slow == 0) fx_splash(e, b, r, 100084); break;
        case 100149: if (b->c_flag != 0) fx_splash(e, b, r, 100084); break;
        case 100150:
            for (uint16_t k = 0; k < fx_effects_n(); k++)
                g_eng.particle_557370(self, self, 100085, b, 0, 0, 99, 1, 0, 0, 2, 0, 0, 0);
            break;
        case 100151:
            for (uint16_t k = 0; k < fx_effects_n(); k++)
                g_eng.particle_557370(self, self, 100086, b, 0, 0, 99, 1, 0, 0, 2, 0, 0, 0);
            break;
        case 100152:
            if (r->t == 0) g_eng.particle_557370(self, self, 100088, b, 0, -1600, 99, 1, 0, 0, 2, 0, 0, 20);
            for (uint16_t k = 0; k < fx_effects_n(); k++)
                g_eng.particle_557370(self, self, 100087, b, 0, 0, 99, 1, 0, 0, 2, 0, 0, 0);
            break;
        case 100153:
            for (uint16_t k = 0; k < fx_effects_n(); k++) {
                int32_t dy = b->h - mul(rnd(), 1600) / 0x8000 - 1600;
                int32_t dx = mul(rnd(), 4000) / 0x8000 - 2000;
                g_eng.particle_557370(self, self, 100089, b, dx, dy, 99, 1, 0, 0, 2, 0, 0, 0);
            }
            break;
        case 100154:
            for (uint16_t k = 0; k < (KEYCFG()->effects != 0 ? 6 : 3); k++) {
                int32_t dy = mul(rnd(), 1600) / 0x8000 - 800;
                int32_t dx = mul(rnd(), 1600) / 0x8000 - 800;
                g_eng.particle_557370(self, self, 100091, b, dx, dy, 99, 1, 0, 0, 2, 0, 0, 0);
            }
            break;
        case 100155: {
            if (r->t % 20 != 0) break;
            int32_t freq = mul(rnd(), 2000) / 0x8000 - 2000;
            wl_body *b0 = e->body;
            void *buf = pool_take(sound_pool(126));
            int32_t ov, op;
            if (buf != NULL && g_eng.sfx_place_40bb90(self, KEYCFG()->sfx_volume, 0, 0, b0, &ov, &op) != 0) {
                g_eng.dsb_volume_5bb870(buf, ov);
                g_eng.dsb_pan_5bb880(buf, op);
                g_eng.dsb_freq_5bb890(buf, freq);
                g_eng.dsb_play_5bb8c0(buf);
            }
            int32_t h = b->h, wd = b->w;
            int32_t dy = h / 2 - mul(rnd(), h) / 0x8000;
            int32_t dx = wd / 2 - mul(rnd(), wd) / 0x8000;
            g_eng.particle_557370(self, self, 100090, b, dx, dy, 99, 1, 0, 0, 2, 0, 0, 20);
            break;
        }
        case 100156:
            w = e->body[part < e->nbody ? part : 0].w;
            goto dust;
        case 100157:
            w = 6400;
        dust:
            if (KEYCFG()->effects == 0) break;
            if (w >= 6400 || (g_world->tick15 & 1) == 0) {
                const wl_body *pb = &e->body[part < e->nbody ? part : 0];
                int32_t hh = pb->h - pb->margin;
                int32_t dy = mul(rnd(), hh / 2) / 0x8000;
                int32_t dx = w / 2 - mul(rnd(), w) / 0x8000;
                g_eng.particle_557370(self, self, 100059, b, dx, dy, 99, 1, 0, 0, 2, 0, 0, 0);
            }
            e->tint_f4 = sys32((g_world->tick15 & 1) != 0 ? 0x2860 : 0x285c);
            break;
        case 100158:
            if ((r->t & 1) == 0 || b->vel_x > 40000) {
                int32_t wd = b->w;
                int32_t dx = wd / 2 - mul(rnd(), wd) / 0x8000;
                wl_entity *p = (wl_entity *)(intptr_t)
                    g_eng.particle_557370(self, self, 100081, b, dx, 0, 99, 4, 0, 0, 2, 0, 0, 10);
                if (p != NULL && b->attach != NULL && b->attach->f100 < 13) {
                    p->f0fc = 6;
                    p->f100 = 6;
                }
            }
            break;
        default:
            break;
        }
        if (r->dur != 0) {
            if (r->t < r->dur) r->t++;
            else r->id = 0;
        }
    }
}

void entity_splash(wl_entity *e, wl_body *b, int32_t was_slow, int32_t was_c)
{
    int32_t kind = was_slow;
    if (was_slow != 0 && b->slow == 0 && b->vvel < -5000) {
        /* out of the water (retail inlines 0x448cb0 here: the same record) */
        grid_c_query(g_world->grid, b->x + b->w / 2, b->h + b->y - 1, &kind);
        entity_fx_add(e, kind == 3 ? 100148 : 100146, 1, 12, 0, b->y + 3000, 0, 0);
    } else if (was_c == 0 && b->c_flag != 0 && b->vvel > 5000) {
        grid_c_query(g_world->grid, b->x + b->w / 2, b->h + b->y - 1, &kind);
        entity_fx_add(e, kind == 3 ? 100149 : 100147, 1, 12, 0, b->y + b->h + 1000, 0, 0);
    }
}

/* ── looping-sound channels ── */

void entity_snd_release(wl_entity *e, uint16_t i, intptr_t a3, int32_t stop)
{
    if (a3 != 0 || e->snd == NULL) return;
    if (e->nsnd <= i) {
        g_eng.log_fail_406440(g_eng.log, (const char *)g_eng.data(0x85525cu), (const char *)g_eng.data(0x8a9b6cu));
        return;
    }
    wl_sndch *ch = &e->snd[i];
    if (ch->voice == NULL) return;
    if (stop != 0) g_eng.dsb_stop_5bb8e0(ch->voice->buf);
    ch->voice->owner = 0;
    ch->voice = NULL;
    ch->f04 = 0;
    ch->f20 = 0;
}

/* one channel's tick (0x46d180's loop body == 0x46d4a0; the release is 0x4537d0's, inlined) */
static void snd_step(wl_entity *e, uint16_t i)
{
    wl_sndch *ch = &e->snd[i];
    if (ch->voice == NULL) { ch->live = 0; return; }
    const wl_body *b = e->body;
    if (ch->live == 0 && ch->fading == 0) {
        ch->fading = 1;
        ch->fade = 0;
    }
    if (ch->fading != 0) {
        if (ch->fade <= -5000) {
            entity_snd_release(e, i, 0, 1);
            ch->live = 0;
            return;
        }
        ch->fade = ch->fade - 100 < -5000 ? -5000 : ch->fade - 100;
    }
    int32_t base = KEYCFG()->sfx_volume + ch->vol;
    int32_t fall = ch->falloff, vxy, pan;
    if (fall == 1) {
        vxy = 0;
        pan = 0;
    } else {
        if (fall == 0) fall = -5000;
        int32_t lx, ly;
        view_listener(g_world->view, &lx, &ly);
        int32_t dx = span_dx(b, lx), dy = b->y + e->f24 - ly;
        if ((fall == -16000 && (iabs(dx) > 19200 || iabs(dy) > 11200)) || iabs(dx) > 128000 || iabs(dy) > 64000) {
            entity_snd_release(e, i, 0, 1);
            ch->live = 0;
            return;
        }
        int32_t ax = iabs(dx) - 8000, ay = iabs(dy) - 16000;
        if (ax < 0) ax = 0;
        if (ay < 0) ay = 0;
        vxy = mul(fall, ax) / 120000 + mul(fall, ay) / 48000;
        pan = mul(dx, 10000) / 128000;
    }
    int32_t v = vxy + effects_gain() + base;
    if (v < -10000) v = -10000;
    else if (v > 0) v = 0;
    int32_t vv = v + ch->fade;
    void *buf = ch->voice->buf;
    if (vv < -10000) vv = -10000;
    g_eng.dsb_volume_5bb870(buf, vv);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, ch->freq);
    ch->live = 0;
}

void entity_snd_update_all(wl_entity *e)
{
    if (e->snd == NULL) return;
    for (uint16_t i = 0; i < e->nsnd; i++) snd_step(e, i);
}

void entity_snd_update(wl_entity *e, uint16_t i)
{
    snd_step(e, i);
}
