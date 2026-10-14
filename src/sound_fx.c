/* sound_fx.c — the sound-effect layer.  See sound_fx.h.
 *
 * Transcribed from the asm (the decompile mistypes the pool record: n u16 +0, next u16 +2,
 * voices +4, device +8, wave resource u16 +0xc, module +0x10).  Retail divides by
 * multiply-and-shift; every quotient here truncates toward zero as those sequences do,
 * and products that could wrap are formed in uint32 as retail's imul wraps.  Where retail
 * returns a register nobody reads (the culls of 0x450ef0), the port returns 0. */
#include "sound_fx.h"

#include <stdio.h>
#include <string.h>

#include "engine_calls.h"
#include "entity_fx.h"
#include "rng.h"

static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static wl_keycfg *keycfg_of(void *sys) { return **(wl_keycfg ***)((uint8_t *)sys + 0x320); }
static int32_t clamp_vol(int32_t v) { return v < -10000 ? -10000 : v > 0 ? 0 : v; }

wl_vpool *sound_pool(uint32_t slot) { return *(wl_vpool **)((uint8_t *)g_eng.sys + 0x364 + 4u * slot); }

/* the effects-volume term: 3000 * ((world+0x1040)+0x28 - 1000) / 1000 */
static int32_t effects_gain(void)
{
    int32_t v;
    memcpy(&v, g_world->f1040 + 0x28, 4);
    return mul(v - 1000, 3000) / 1000;
}

/* the listener's distance to body `b` of an entity whose +0x24 is `f24`: dx 0 inside the
 * span, else the nearer edge (a tie takes the right one) */
static void listener_delta(const wl_body *b, int32_t f24, int32_t *dx, int32_t *dy)
{
    int32_t lx, ly;
    view_listener(g_world->view, &lx, &ly);
    int32_t l = b->x, r = b->x + b->w;
    if (lx > l && lx < r) {
        *dx = 0;
    } else {
        int32_t dl = l - lx, dr = r - lx;
        *dx = iabs(dl) < iabs(dr) ? dl : dr;
    }
    *dy = b->y + f24 - ly;
}

/* the attenuation of a placed sound: falloff * the distance beyond 8000 / 16000 */
static int32_t attenuation(int32_t falloff, int32_t dx, int32_t dy)
{
    int32_t ax = iabs(dx) - 8000, ay = iabs(dy) - 16000;
    if (ax < 0) ax = 0;
    if (ay < 0) ay = 0;
    return mul(ax, falloff) / 120000 + mul(ay, falloff) / 48000;
}

static void wave_load_failed(uint16_t res)
{
    char buf[32];
    snprintf(buf, sizeof buf, "%d", res);
    g_eng.log_fail_406440(g_eng.log, "Failed Wave Load", buf);
}

/* the create / duplicate loop every loader inlines: 1 = all n buffers made */
static int pool_create(wl_vpool *p)
{
    p->v = g_eng.op_new(8u * p->n);
    memset(p->v, 0, 8u * p->n);
    if (g_eng.dsb_create_5bb250(p->dev, (intptr_t)p->v, (intptr_t)p->module, p->res, 0, 0) == 0) return 0;
    for (uint16_t i = 1; i < p->n; i++)
        if (g_eng.dsb_dup_5bb2f0(p->dev, (intptr_t)&p->v[i], (intptr_t)p->v[0].buf) == 0) return 0;
    return 1;
}

int vpool_load(wl_vpool *p)
{
    if (p->dev == NULL || p->v != NULL) return 0;
    if (pool_create(p)) return 1;
    g_eng.op_delete(p->v);
    p->v = NULL;
    wave_load_failed(p->res);
    return 0;
}

/* the round-robin pick, the pool loaded: NULL when every voice is held */
static wl_voice *pool_pick(wl_vpool *p, int32_t owner)
{
    for (uint16_t k = 0; k < p->n; k++) {
        uint16_t idx = (uint16_t)(((int32_t)k + (int32_t)p->next) % (int32_t)p->n);
        if (p->v[idx].owner == 0) {
            p->next = (uint16_t)(idx + 1);
            p->v[idx].owner = owner;
            return &p->v[idx];
        }
    }
    return NULL;
}

wl_voice *vpool_take(wl_vpool *p, int32_t owner)
{
    if (p->dev == NULL) return NULL;
    if (p->v == NULL && vpool_load(p) == 0) return NULL;
    if (p->n == 0) return NULL;
    return pool_pick(p, owner);
}

int sfx_place(const wl_entity *e, int32_t base, int32_t falloff, int32_t keep, const wl_body *b,
              int32_t *out_vol, int32_t *out_pan)
{
    int32_t att = 0, pan = 0;
    if (falloff != 1) {
        if (falloff == 0) falloff = -5000;
        int32_t dx, dy;
        listener_delta(b, e->f24, &dx, &dy);
        if (falloff == -16000 && (iabs(dx) > 19200 || iabs(dy) > 11200)) return 0;
        if (keep == 0 && (iabs(dx) > 128000 || iabs(dy) > 64000)) return 0;
        att = attenuation(falloff, dx, dy);
        pan = mul(dx, 10000) / 128000;
    }
    int32_t v = clamp_vol(base + effects_gain() + att);
    if (out_vol != NULL) *out_vol = v;
    if (out_pan != NULL) *out_pan = pan;
    return 1;
}

int sfx_play(void *sys, uint16_t id, int32_t vol, int32_t freq)
{
    wl_vpool *p = *(wl_vpool **)((uint8_t *)sys + 0x364 + 4u * id);
    if (p->dev == NULL) return 0;
    if (p->v == NULL && !pool_create(p)) {
        g_eng.op_delete(p->v);
        p->v = NULL;
        char buf[32];
        snprintf(buf, sizeof buf, "%d", p->res);
        return g_eng.log_fail_406440(g_eng.log, "Failed Wave Load", buf);
    }
    if (p->n == 0) return 0;
    wl_voice *v = pool_pick(p, 0);
    if (v == NULL) return 0;
    g_eng.dsb_volume_5bb870(v->buf, vol + keycfg_of(sys)->sfx_volume);
    g_eng.dsb_freq_5bb890(v->buf, freq);
    return g_eng.dsb_play_5bb8c0(v->buf);
}

int sfx_play_at(wl_entity *e, int32_t a2, uint16_t id, int32_t vol, int32_t freq, int32_t falloff)
{
    if (a2 != 0) return 1;
    wl_vpool *p = sound_pool(id);
    if (p->dev == NULL) return 1;
    if (p->v == NULL && !pool_create(p)) {
        g_eng.op_delete(p->v);
        p->v = NULL;
        g_eng.h407f30(g_eng.log, (intptr_t)"Failed Wave Load", 0, p->res);
        return 1;
    }
    if (p->n == 0) return 1;
    wl_voice *v = pool_pick(p, 0);
    if (v == NULL) return 1;
    void *buf = v->buf;
    int32_t base = keycfg_of(g_eng.sys)->sfx_volume, att = 0, pan = 0;
    if (falloff != 1) {
        if (falloff == 0) falloff = -5000;
        int32_t dx, dy;
        listener_delta(e->body, e->f24, &dx, &dy);
        if (falloff == -16000 && (iabs(dx) > 19200 || iabs(dy) > 11200)) return 0;
        if (iabs(dx) > 128000 || iabs(dy) > 64000) return 0;
        att = attenuation(falloff, dx, dy);
        pan = mul(dx, 10000) / 128000;
    }
    g_eng.dsb_volume_5bb870(buf, clamp_vol(vol + base + effects_gain() + att));
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, freq);
    g_eng.dsb_play_5bb8c0(buf);
    return 1;
}

int voice_place(const wl_char *c, int32_t *vol, int32_t *pan, int32_t *freq)
{
    wl_keycfg *k = keycfg_of(g_eng.sys);
    int32_t falloff, base;
    if (c->voice_mode != 0) {
        falloff = -2000;
        base = k->voice_volume;
        if (freq != NULL) *freq = 0;
    } else {
        falloff = 0;
        base = k->sfx_volume;
        if (freq != NULL) *freq = 500 - mul(rnd(), 1000) / 0x8000;
    }
    base += c->voice_vol_bias;
    if (falloff == 0) falloff = -5000;
    int32_t dx, dy;
    listener_delta(c->base.body, c->base.f24, &dx, &dy);
    if (iabs(dx) > 128000 || iabs(dy) > 64000) {
        *vol = -10000;
        *pan = 0;
        return 0;
    }
    int32_t v = clamp_vol(base + effects_gain() + attenuation(falloff, dx, dy));
    if (vol != NULL) *vol = v;
    if (pan != NULL) *pan = mul(dx, 10000) / 128000;
    return 1;
}

void vline_stop(wl_vline *l, int32_t stop)
{
    if (l->voice == NULL) return;
    if (stop != 0) g_eng.dsb_stop_5bb8e0(l->voice->buf);
    l->voice->owner = 0;
    l->voice = NULL;
}

void voiceset_tick(wl_voiceset *vs, int32_t dt)
{
    for (uint16_t i = 0; i < vs->n; i++) {
        wl_vgroup *g = &vs->grp[i];
        if (g->cd > 0) {
            int32_t c = g->cd - dt;
            g->cd = c < 0 ? 0 : c;
        }
        for (uint16_t k = 0; k < g->n; k++) {
            wl_vline *l = &g->line[k];
            if (l->voice != NULL && l->fading != 0) {
                int32_t v = l->vol - vs->fade_step;
                if (v < -10000) v = -10000;
                l->vol = v;
                if (v > -10000) g_eng.dsb_volume_5bb870(l->voice->buf, v);
                else vline_stop(l, 0);
            }
            if (l->voice != NULL) {
                l->remain -= dt;
                if (l->remain < 0) vline_stop(l, 0);
            }
        }
    }
}

int voiceset_sounding(const wl_voiceset *vs, wl_vgroup **g, wl_vline **l)
{
    for (uint16_t i = 0; i < vs->n; i++) {
        wl_vgroup *gr = &vs->grp[i];
        for (uint16_t k = 0; k < gr->n; k++)
            if (gr->line[k].voice != NULL && gr->line[k].fading == 0) {
                *g = gr;
                *l = &gr->line[k];
                return 1;
            }
    }
    *g = NULL;
    *l = NULL;
    return 0;
}

void voiceset_cut(wl_voiceset *vs)
{
    wl_vgroup *g;
    wl_vline *l;
    if (!voiceset_sounding(vs, &g, &l)) return;
    if (vs->mode < 0) return;
    if (vs->mode <= 1) vline_stop(l, 1);
    else if (vs->mode == 2) l->fading = 1;
}

/* the voice set's clock (mode 0 only): cooldowns, fades and line ends since the last cue */
static void voiceset_clock(wl_voiceset *vs)
{
    if (vs->mode != 0) return;
    uint32_t now = g_eng.tick();
    if (vs->last != 0) voiceset_tick(vs, (int32_t)(now - vs->last));
    vs->last = now;
}

/* a line already sounding: a lower priority one is cut (or faded); 0 = this cue yields */
static int voiceset_preempt(wl_voiceset *vs, const wl_vgroup *g, int32_t force)
{
    wl_vgroup *pg;
    wl_vline *pl;
    voiceset_sounding(vs, &pg, &pl);
    if (pg == NULL) return 1;
    if (pg == g) return 0;
    if (pg->prio >= g->prio && (force == 0 || pg->prio != g->prio)) return 0;
    voiceset_cut(vs);
    return 1;
}

/* a random line not played since the group last cycled (all played: the group cycles);
 * it takes the group's rest, stops what it was playing, reserves a voice */
static wl_vline *voice_line_start(wl_vgroup *g)
{
    wl_vline *pick[32];
    uint16_t n = 0;
    for (uint16_t k = 0; k < g->n; k++)
        if (g->line[k].used == 0) pick[n++] = &g->line[k];
    if (n == 0) {
        for (uint16_t k = 0; k < g->n; k++) {
            g->line[k].used = 0;
            pick[n++] = &g->line[k];
        }
        if (n == 0) return NULL;
    }
    wl_vline *l = pick[mul(rnd(), n) / 0x8000];
    g->cd = g->cd_reset;
    l->used = 1;
    vline_stop(l, 1);
    l->voice = vpool_take(l->pool, 1);
    return l;
}

static void voice_line_play(wl_vline *l, int32_t vol, int32_t pan, int32_t freq, int32_t fixed)
{
    int32_t lv = l->vol_base + vol;
    l->remain = l->len;
    l->vol = lv;
    l->fading = 0;
    l->fixed = fixed;
    g_eng.dsb_volume_5bb870(l->voice->buf, clamp_vol(lv));
    g_eng.dsb_pan_5bb880(l->voice->buf, pan);
    g_eng.dsb_freq_5bb890(l->voice->buf, freq);
    g_eng.dsb_play_5bb8c0(l->voice->buf);
}

int voice_play(wl_voiceset *vs, uint16_t grp, int32_t force, int32_t vol, int32_t pan, int32_t freq, int32_t fixed)
{
    if (vs->n <= grp) return 0;
    wl_vgroup *g = &vs->grp[grp];
    if (g->n == 0) return 0;
    voiceset_clock(vs);
    if (g->cd > 0) return 0;
    if ((int32_t)g->chance < mul(rnd(), 1000) / 0x8000) return 0;
    if (!voiceset_preempt(vs, g, force)) return 0;
    wl_vline *l = voice_line_start(g);
    if (l == NULL || l->voice == NULL) return 0;
    voice_line_play(l, vol, pan, freq, fixed);
    return 1;
}

int voice_cue(wl_char *c, int32_t a2, uint16_t grp, int32_t force, int32_t delay, int32_t fixed)
{
    if (a2 != 0 || grp == 0) return 0;
    wl_voiceset *vs = c->voice;
    if (vs == NULL || c->f15b5c != 0) return 0;
    wl_keycfg *k = keycfg_of(g_eng.sys);
    if (c->voice_mode == 1 && k->voices == 0) return 0;
    if (delay > 0) {
        if (c->voice_delay > 0) {
            uint16_t p = grp < vs->n ? vs->grp[grp].prio : 0, q = c->voice_id;
            if (q != 0 && q < vs->n && p < vs->grp[q].prio) return 0;
        }
        c->voice_id = grp;
        c->voice_delay = delay;
        c->voice_arg = force;
        c->voice_vol = fixed;
        return 1;
    }
    int32_t vol, pan = 0, freq = 0;
    if (fixed != 0) {
        vol = k->voice_volume + c->voice_vol_bias;
    } else {
        int32_t base, falloff = 0;
        if (c->voice_mode != 0) {
            base = k->voice_volume;
            falloff = -2000;
        } else {
            base = k->sfx_volume;
            freq = 500 - mul(rnd(), 1000) / 0x8000;
        }
        if (sfx_place(&c->base, base + c->voice_vol_bias, falloff, 0, c->base.body, &vol, &pan) == 0) return 0;
    }
    vs = c->voice;
    if (vs->n <= grp) return 0;
    wl_vgroup *g = &vs->grp[grp];
    if (g->n == 0) return 0;
    voiceset_clock(vs);
    if (g->cd > 0) return 0;
    if ((int32_t)g->chance < mul(rnd(), 1000) / 0x8000) return 0;
    if (!voiceset_preempt(vs, g, force)) return 0;
    wl_vline *l = voice_line_start(g);
    if (l == NULL || l->voice == NULL) return 0;
    voice_line_play(l, vol, pan, freq, fixed);
    return 1;
}

int entity_snd_start(wl_entity *e, uint16_t i, int32_t a3, uint16_t id, int32_t vol, int32_t freq,
                     int32_t falloff, int32_t stop)
{
    if (a3 != 0 || e->snd == NULL) return 0;
    if (e->nsnd <= i) {
        entity_snd_overrun();
        return 0;
    }
    if (falloff != 1) {
        int32_t f = falloff == 0 ? -5000 : falloff, dx, dy;
        listener_delta(e->body, e->f24, &dx, &dy);
        if (f == -16000 && (iabs(dx) > 19200 || iabs(dy) > 11200)) return 0;
        if (iabs(dx) > 128000 || iabs(dy) > 64000) return 0;
    }
    wl_sndch *ch = &e->snd[i];
    wl_vpool *p = sound_pool(id);
    if (ch->voice != NULL && e->snd != NULL) {
        if (e->nsnd > i) {
            if (ch->voice != NULL) {
                if (stop != 0) g_eng.dsb_stop_5bb8e0(ch->voice->buf);
                ch->voice->owner = 0;
                ch->voice = NULL;
                ch->pool = NULL;
                ch->f20 = 0;
            }
        } else {
            entity_snd_overrun();
        }
    }
    wl_voice *v = vpool_take(p, 1);
    if (v == NULL) return 0;
    ch->voice = v;
    ch->pool = p;
    ch->vol = vol;
    ch->freq = freq;
    ch->falloff = falloff;
    ch->live = 1;
    ch->fading = 0;
    ch->fade = 0;
    g_eng.dsb_volume_5bb870(v->buf, -10000);
    return g_eng.dsb_play_5bb8c0(v->buf);
}

int sfx_play_jit(wl_entity *e, uint16_t id, int32_t kind, int32_t vol_a, int32_t vol_b, int32_t freq, int32_t jit)
{
    int32_t v = kind == 2 ? vol_a : vol_b;
    int32_t r = mul(rnd(), 500) / 0x8000;
    int32_t vol = mul(v, 750) / 1000 + mul(r, v) / 1000;
    int32_t f = mul(rnd(), jit) / 0x8000 + freq;
    wl_voice *vc = vpool_take(sound_pool(id), 0);
    if (vc == NULL) return 0;
    void *buf = vc->buf;
    int32_t dx, dy;
    listener_delta(e->body, e->f24, &dx, &dy);
    if (iabs(dx) > 128000 || iabs(dy) > 64000) return 0;
    int32_t base = keycfg_of(g_eng.sys)->sfx_volume;
    g_eng.dsb_volume_5bb870(buf, clamp_vol(vol + base + effects_gain() + attenuation(-5000, dx, dy)));
    g_eng.dsb_pan_5bb880(buf, mul(dx, 10000) / 128000);
    g_eng.dsb_freq_5bb890(buf, f);
    return g_eng.dsb_play_5bb8c0(buf);
}

void entity_snd_loop(wl_entity *e, uint16_t i, int32_t a3, uint16_t id, int32_t vol, int32_t freq, int32_t falloff,
                     int32_t stop, int32_t period)
{
    if (a3 != 0 || e->snd == NULL) return;
    if (e->nsnd <= i) {
        entity_snd_overrun();
        return;
    }
    if (e->snd[i].f20 == 0 && sfx_place(e, vol, falloff, 0, e->body, NULL, NULL) != 0) {
        wl_vpool *p = sound_pool(id);
        wl_sndch *ch = &e->snd[i];
        if (ch->voice != NULL) entity_snd_release(e, i, 0, stop);
        wl_voice *v = vpool_take(p, 1);
        if (v != NULL) {
            ch->pool = p;
            ch->voice = v;
            ch->falloff = falloff;
            ch->vol = vol;
            ch->freq = freq;
            ch->live = 1;
            ch->fading = 0;
            ch->fade = 0;
            g_eng.dsb_volume_5bb870(v->buf, -10000);
            g_eng.dsb_play_5bb8c0(v->buf);
        }
    }
    entity_snd_keep(e, i, 0);
    if (++e->snd[i].f20 >= period) e->snd[i].f20 = 0;
}

int ambient_sound_timer(wl_entity *e, wl_body *b, int32_t probe, uint16_t t0, uint16_t tv, uint16_t s0, uint16_t s1,
                        uint16_t s2, uint16_t s3, uint16_t ns, int32_t on_def, int32_t on_1, int32_t on_2_4, int32_t vol,
                        int32_t f0, int32_t fv, int32_t scene_off)
{
    if (probe != 0) return 0;
    if ((g_world->f2770 != 0 || g_world->f2764 == 1) && scene_off != 0) return 0;
    if (g_world->busy != 0) return 0;
    int32_t when; memcpy(&when, g_eng.data(0x8a93fcu), 4);
    switch (when) {
    case 1: if (on_1 == 0) return 0; break;
    case 2: case 3: case 4: if (on_2_4 == 0) return 0; break;
    case 5: return 0;
    default: if (on_def == 0) return 0; break;
    }
    if (b->f58 == 0) {
        int32_t r = rnd();
        b->f58++;
        b->act_ctr = mul(r, tv) / 0x8000;
    }
    if (b->act_ctr > 0) {
        b->act_ctr--;
        if (e->snd == NULL) return 0;
        if (e->nsnd == 0) {
            entity_snd_overrun();
            return 0;
        }
        e->snd[0].live = 1;
        return 0;
    }
    b->act_ctr = mul(rnd(), tv) / 0x8000 + t0;
    uint16_t sfx;
    switch ((uint16_t)(mul(rnd(), ns) / 0x8000)) {
    case 0: sfx = s0; break;
    case 1: sfx = s1; break;
    case 2: sfx = s2; break;
    case 3: sfx = s3; break;
    default: sfx = (uint16_t)vol; break;
    }
    if (sfx == 0) return 0;
    int32_t freq = mul(rnd(), fv) / 0x8000 + f0;
    if (e->snd == NULL) return 1;
    if (e->nsnd == 0) {
        entity_snd_overrun();
        return 1;
    }
    if (sfx_place(e, vol, 0, 0, e->body, NULL, NULL) == 0) return 1;
    wl_sndch *ch = e->snd;
    wl_vpool *p = sound_pool(sfx);
    if (ch->voice != NULL) entity_snd_release(e, 0, 0, 1);
    wl_voice *v = vpool_take(p, 1);
    if (v == NULL) return 1;
    ch->voice = v;
    ch->pool = p;
    ch->vol = vol;
    ch->freq = freq;
    ch->falloff = 0;
    ch->live = 1;
    ch->fading = 0;
    ch->fade = 0;
    g_eng.dsb_volume_5bb870(v->buf, -10000);
    g_eng.dsb_play_5bb8c0(v->buf);
    return 1;
}

void char_idle_voice(wl_char *c, int32_t probe, uint16_t grp, int32_t t0, uint16_t tr)
{
    if (probe != 0 || c->idle_voice_on == 0) return;
    if (c->idle_voice_t == 1) {
        c->idle_voice_t = 0;
        wl_keycfg *k = keycfg_of(g_eng.sys);
        if (grp != 0 && c->voice != NULL && c->f15b5c == 0 && !(c->voice_mode == 1 && k->voices == 0)) {
            int32_t base, falloff = 0, freq = 0, vol, pan;
            if (c->voice_mode != 0) {
                base = k->voice_volume;
                falloff = -2000;
            } else {
                base = k->sfx_volume;
                freq = 500 - mul(rnd(), 1000) / 0x8000;
            }
            if (sfx_place(&c->base, base + c->voice_vol_bias, falloff, 0, c->base.body, &vol, &pan) != 0)
                voice_play(c->voice, grp, 1, vol, pan, freq, 0);
        }
    }
    if (c->idle_voice_t != 0) {
        c->idle_voice_t--;
        return;
    }
    c->idle_voice_t = (uint16_t)(mul(rnd(), 1000) / 0x8000 % (int32_t)tr + t0);
}

int sfx_oneshot(wl_entity *e, wl_vpool *p, int32_t base, int32_t falloff, int32_t freq)
{
    wl_voice *v = vpool_take(p, 0);
    if (v == NULL) return 0;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(e, base, falloff, 0, e->body, &vol, &pan) == 0) return 0;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, freq);
    g_eng.dsb_play_5bb8c0(buf);
    return 1;
}
