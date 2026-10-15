/* arche_pose.c — FUN_00457ff0 + its helpers.  See arche_pose.h.
 *
 * Transcribed from the decompile, checked against the asm (the hit-reaction voice fallbacks; the
 * status length's double jitter draw).  The clip sets are body_set_clip (inlined by MSVC), the
 * pose ends body_action_reset; 0x4678f0's two velocity ramps are 0x445db0 inlined (speed_ramp)
 * and its move half is 0x467ae0's. */
#include "arche_pose.h"

#include <string.h>

#include "actor_ops.h"
#include "actor_update.h"
#include "char_integrate.h"
#include "char_state.h"
#include "engine_calls.h"
#include "entity_fx.h"
#include "entity_move.h"
#include "particle_spawn.h"
#include "rng.h"
#include "sound_fx.h"

#define IP(p) ((intptr_t)(p))
#define W (*g_eng.world)

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static const void *D(uint32_t va) { return g_eng.data(va); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t iabs(int32_t v) { return v < 0 ? -v : v; }

/* the keyboard config **(sys+0x320) */
static const uint8_t *cfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}

int particle_live(int32_t code)
{
    for (uint16_t i = 0; i < 0x400; i++) {
        const wl_entity *p = W->particles[i];
        if (p->alive != 0 && (int32_t)p->code == code) return 1;
    }
    return 0;
}

void room_hitstop_set(int32_t on, uint16_t n)
{
    uint8_t *rl = W->f1044;
    st32(rl, 0x3c, on);
    st16(rl, 0x38, 0);
    st16(rl, 0x3a, n);
}

int32_t char_room_logs(const wl_char *c)
{
    const uint8_t *s = (const uint8_t *)(intptr_t)ld32(c, 0xc794);
    if (s == NULL) return 0;
    return ld32(ldp(s, 0x9ec), 0x40a4);
}

int switch_touch(const int32_t *rect, uint16_t part, int32_t mode)
{
    for (uint16_t i = 0; i < W->list_2798->count; i++) {
        uint8_t *e = (uint8_t *)W->list_2798->items[i];
        if (ld32(e, 0x1d0) == 0) continue;
        uint16_t k = part < ld16(e, 0x44) ? part : 0;
        const uint8_t *b = (const uint8_t *)ldp(e, 0x40) + 660u * k;
        if (ld32(b, 4) > rect[0] + rect[2]) continue;
        if (ld32(b, 4) + ld32(b, 0xc) <= rect[0]) continue;
        if (ld32(b, 8) + ld32(b, 0x14) > rect[1] + rect[3]) continue;
        if (ld32(b, 0x10) + ld32(b, 8) - 1 <= rect[1]) continue;
        if (ld32(e, 0x294) != 0) continue;
        switch (ld32(e, 0x290)) {
        case 1: case 2:
            if (mode != 0) break;
            st32(e, 0x294, 1);
            return 1;
        case 3:
            if (mode == 1) { if (ld32(e, 0xc) == 1) st32(e, 0x294, 1); }
            else if (mode == 2 && ld32(e, 0xc) != 1) st32(e, 0x294, 1);
            break;
        case 4:
            if (mode == 1 && ld32(e, 0xc) == 0) st32(e, 0x294, 1);
            break;
        default: break;
        }
    }
    return 0;
}

int body_switch_touch(const wl_body *b, int32_t probe, uint16_t part)
{
    if (probe != 0) return probe;
    int32_t r[4] = { b->x, b->y + 3200, b->w, b->h };
    return switch_touch(r, part, 0);
}

int char_drift_move(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t stop_x, int32_t a6, int32_t a7)
{
    int32_t down = 1, ok = 1;
    if (entity_move_vertical(&c->base, b, b->vvel / 100, b->drop, (uint16_t)part, &b->y, &b->attach) == 0) {
        b->vvel = 0;
        ok = a6 == 0;
    }
    if (a7 == 0 || (probe != 0 && c->f200 != 0)) down = 0;
    int32_t dx = b->facing == 3 ? b->vel_x / -100 : b->vel_x / 100;
    int r = entity_move_horizontal(&c->base, b, dx, down, a7, b->drop, (uint16_t)part, 0);
    if (r != 0) return ok;
    b->vel_x = 0;
    if (stop_x == 0) return ok;
    return r;
}

int char_drift(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t xt, int32_t xs, int32_t xb, int32_t yt,
               int32_t ys, int32_t yb, int32_t stop_x, int32_t a12, int32_t a13)
{
    b->vel_x = speed_ramp(b->vel_x, xs, xb, xt);
    b->vvel = speed_ramp(b->vvel, ys, yb, yt);
    return char_drift_move(c, b, probe, part, stop_x, a12, a13);
}

/* a sound of pool `slot` at the character, its volume from the effects gain at `base`, freq f */
static void pool_sound(wl_char *c, uint32_t slot_off, int32_t base, int32_t f)
{
    wl_voice *v = vpool_take(ldp(g_eng.sys, slot_off), 0);
    if (v == NULL) return;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(&c->base, base, 0, 0, c->base.body, &vol, &pan) == 0) return;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, f);
    g_eng.dsb_play_5bb8c0(buf);
}

void char_status_inflict(wl_char *c, uint16_t st, int32_t v)
{
    uint8_t *pt = (uint8_t *)c->party;
    uint16_t before = ld16(pt, 0x448 + 2u * st);
    const uint8_t *row = (const uint8_t *)D(0x628708u) + 140u * st;
    int32_t base = ld16(row, 0x64), msg = 0;
    if (ld32(c, 0x15a94) == 0) base += ld32(row, 0x68);
    uint16_t len;
    if (base + mul(rnd(), ld16(row, 0x66)) / 0x8000 < 0) len = 0;
    else len = (uint16_t)(base + mul(rnd(), ld16(row, 0x66)) / 0x8000);   /* (a second draw: retail) */
    st16(c->party, 0x46c + 2u * st, len);
    pt = (uint8_t *)c->party;
    st16(pt, 0x448 + 2u * st, ld16(pt, 0x46c + 2u * st));
    st32(c->party, 0x490 + 4u * st, v);
    char text[512];
    const uint8_t *k = cfg();
    switch (st) {
    case 1:
        if (before == 0) { msg = 1; strcpy(text, (const char *)D(0x8553d4u)); }   /* "%s has fallen!" */
        for (uint16_t i = 0; i < 18; i++) if (i != st) st16(c->party, 0x448 + 2u * i, 0);
        break;
    case 2:
        if (before == 0) { msg = 1; strcpy(text, (const char *)D(0x8553e8u)); }
        break;
    case 3:
        if (before == 0) { msg = 1; strcpy(text, (const char *)D(0x8553fcu)); }
        pool_sound(c, 0x534, ld32(k, 0x168), 0);
        break;
    case 5:
        if (before == 0) { msg = 1; strcpy(text, (const char *)D(0x855418u)); }
        break;
    case 6:
        if (before == 0) { msg = 1; strcpy(text, (const char *)D(0x855454u)); }
        break;
    case 7:
        if (before == 0) { g_eng.h5bf3ee(text, (const char *)D(0x855430u), v); msg = 1; }
        pool_sound(c, 0x530, ld32(k, 0x168), 0);
        break;
    case 13:
        pool_sound(c, 0x52c, ld32(cfg(), 0x168), 0);
        if (before == 0) { g_eng.h5bf3ee(text, (const char *)D(0x8553a4u), v); msg = 1; }
        break;
    default: break;
    }
    if (ld32(c, 0xc794) == 0 || msg == 0 || char_room_logs(c) == 0) return;
    char line[512];
    g_eng.h5bf3ee(line, text, (const char *)c->party);
    uint8_t *w = (uint8_t *)W;
    int32_t i = ld32(w, 0x1024);
    for (uint16_t n = 0; ld32(w, 516 * i + 4) != 0; ) {
        i = (i + 1) % 8;
        if (++n >= 8) return;
    }
    st32(w, 516 * i + 4, 1);
    memcpy(w + 516 * i + 8, line, strlen(line) + 1);
}

int char_knockdown(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t aura)
{
    int r = 0;
    b->nocollide = 1;
    b->margin = 2400;
    switch (b->f58) {
    case 0:
        body_set_clip(b, D(0x62ca40u));
        b->f58 = 1;
        b->vel_x = -40000;
        b->vvel = -40000;
        r = 1;
        if (ld32(c, 0x15ad0) == 8 && probe == 0) {
            entity_fx_remove(&c->base, 100157);
            uint8_t *e = (uint8_t *)c;
            uint16_t n = ld16(e, 0x1ac);
            uint16_t i;
            for (i = 0; i < n && ld32(e, 0x10c + 20u * i) != 0; i++) {}
            if (n != 0 && i < n) {   /* the burn mark: the first free effect record */
                st32(e, 0x10c + 20u * i, 100157);
                st16(e, 0x110 + 20u * i, 60);
                st16(e, 0x112 + 20u * i, 0);
                st16(e, 0x114 + 20u * i, 0);
                st16(e, 0x116 + 20u * i, 0);
                st32(e, 0x118 + 20u * i, 0);
                st32(e, 0x11c + 20u * i, 0);
            }
        }
        if (aura != 0) {
            int32_t code = 0;
            if (c->base.code == 50010) { if (ld32(c->party, 0xd4) == 1) code = 100128; }
            else if (c->base.code == 50011) code = 100129;
            if (code != 0 && !particle_live(code))
                particle_spawn(&c->base, &c->base, code, b, 0, 0, b->facing, 1, probe, 0, 2, 0, 0, 20);
        }
        break;
    case 1:
        break;
    case 2: {
        b->vel_x = speed_ramp(b->vel_x, 3200, -1600, -15000);
        b->vvel = speed_ramp(b->vvel, 4000, -1600, 64000);
        char_drift_move(c, b, probe, part, 0, 0, 1);
        if (b->grounded == 0) return 0;
        if (probe == 0)
            for (int i = 0; i < 8; i++) {
                int32_t w = b->w + 1600;
                particle_spawn(&c->base, &c->base, 100080, b, mul(w, rnd()) / 0x8000 - w / 2, 0, 99, 4, 0, 0, 2, 0, 0, 10);
            }
        int32_t f = 500 - mul(rnd(), 1000) / 0x8000;
        if (probe == 0) {
            wl_voice *v = vpool_take(ldp(g_eng.sys, 0x574), 0);
            if (v != NULL) {
                void *buf = v->buf;
                int32_t vol, pan;
                if (sfx_place(&c->base, ld32(cfg(), 0x168), 0, 0, c->base.body, &vol, &pan) != 0) {
                    g_eng.dsb_volume_5bb870(buf, vol);
                    g_eng.dsb_pan_5bb880(buf, pan);
                    g_eng.dsb_freq_5bb890(buf, f);
                    g_eng.dsb_play_5bb8c0(buf);
                }
            }
        }
        b->f58++;
        body_set_clip(b, D(0x62cd70u));
        b->vvel = -40000;
        return 1; }
    case 3:
        b->vel_x = speed_ramp(b->vel_x, 3200, -1600, -10000);
        b->vvel = speed_ramp(b->vvel, 4000, -1600, 64000);
        char_drift_move(c, b, probe, part, 0, 0, 1);
        if (b->clip_done == 0) return 0;
        b->f58++;
        body_set_clip(b, D(0x62cbd8u));
        return 1;
    case 4:
        b->vel_x = speed_ramp(b->vel_x, 3200, -1600, -5000);
        b->vvel = speed_ramp(b->vvel, 4000, -1600, 64000);
        char_drift_move(c, b, probe, part, 0, 0, 1);
        if (b->grounded == 0) return 0;
        entity_dust(&c->base, b, probe, 8, 0, 0);
        sfx_play_at(&c->base, probe, 132, 0, 500 - mul(rnd(), 1000) / 0x8000, 0);
        b->f58++;
        body_set_clip(b, D(0x62cf08u));
        return 1;
    case 5:
        char_drift(c, b, probe, part, 0, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
        return 0;
    default:
        return 0;
    }
    /* stage 0 / 1: wait for the clip, then the air phase */
    if (b->clip_done != 0) {
        b->f58++;
        body_set_clip(b, D(0x62cbd8u));
        r = 1;
    }
    b->vel_x = speed_ramp(b->vel_x, 3200, -1600, -20000);
    b->vvel = speed_ramp(b->vvel, 1000, -1600, 64000);
    char_drift_move(c, b, probe, part, 0, 0, 1);
    return r;
}

/* a hit voice line `grp` when the voice set is live (0x4099a0); 0 when it did not play */
static int hit_voice(wl_char *c, uint16_t grp)
{
    if (c->voice == NULL || c->f15b5c != 0) return 0;
    if (c->voice_mode == 1 && ld32(cfg(), 0x190) == 0) return 0;
    int32_t vol, pan, freq;
    if (voice_place(c, &vol, &pan, &freq) == 0) return 0;
    return voice_play(c->voice, grp, 0, vol, pan, freq, 0) != 0;
}

/* the clip of the standing / moving state (pose 113's get-up end and the default) */
static void state_clip(wl_char *c, wl_body *b, int32_t probe, int32_t grounded)
{
    int slowed = 0;
    switch (b->state) {
    case 1:
        if (b->vel_x <= 0 && b->sub != 0) break;
        body_set_clip(b, b->vel_x > c->walk_max && c->can_run != 0 ? c->clip_run : c->clip_walk);
        return;
    case 2:
        if (c->can_crouch == 0) break;
        switch (b->sub) {
        case 0: body_set_clip(b, c->clip_crouch0); return;
        case 1: body_set_clip(b, c->clip_crawl != NULL && b->vel_x > 20000 ? c->clip_crawl : c->clip_crouch1); return;
        case 2: body_set_clip(b, c->clip_crouch2); return;
        default: return;
        }
    case 3:
        if (c->jump_sfx != 0 && probe == 0 && c->f15944 == 1) slowed = body_slowed(b);
        if (c->can_jump == 0) break;
        switch (b->sub) {
        case 0: body_set_clip(b, c->clip_jump0); return;
        case 1:
            if (slowed) goto splash;
            if (probe != 0 || c->max_air_jumps == 0 || grounded != 0) {
                if (c->jump_sfx != 0) {
                    int32_t f = c->jump_sfx_freq + mul(c->jump_sfx_jit, rnd()) / 0x8000;
                    sfx_play_at(&c->base, probe, (uint16_t)c->jump_sfx, c->jump_sfx_vol, f, c->jump_sfx_pan);
                }
            } else {
                sfx_play_at(&c->base, 0, 105, 0, 0, 0);
                particle_spawn(&c->base, &c->base, 100121, b, 0, -1200, b->facing, 1, 0, 0, 2, 1, 0, 0);
                particle_spawn(&c->base, &c->base, 100122, b, 0, -1200, b->facing, 1, 0, 0, 2, 1, 0, 0);
            }
            body_set_clip(b, c->clip_jump1);
            return;
        case 2:
            if (slowed) {
splash:
                if ((c->flags204 & 0x30000) == 0) {
                    int32_t dy = mul(800, rnd()) / 0x8000 - 800;
                    int32_t dx = mul(1600, rnd()) / 0x8000 - 800;
                    particle_spawn(&c->base, &c->base, 100105, b, dx, dy, 99, 1, probe, 0, 2, 0, 0, 0);
                    sfx_play_at(&c->base, probe, 28, -500, mul(2000, rnd()) / 0x8000 - 2000, 0);
                }
            }
            body_set_clip(b, c->clip_jump1);
            return;
        case 3: body_set_clip(b, c->clip_jump2); return;
        default: return;
        }
    case 4: case 6:
        if (b->sub == 0) { body_set_clip(b, c->clip_air0); return; }
        if (b->sub <= 2) body_set_clip(b, c->clip_air1);
        return;
    case 5:
        if (c->up_assist == 0) break;
        if (b->sub == 0) { body_set_clip(b, c->clip_up0); return; }
        if (b->sub == 1) body_set_clip(b, c->clip_up1);
        return;
    default: break;
    }
    body_set_clip(b, c->clip_fast != NULL && iabs(b->vel_x) > 20000 ? c->clip_fast : c->clip_stand);
}

/* poses 110 / 112: the stage drift with dust + the landing sound (132) */
static void land_fx(wl_char *c, wl_body *b, int32_t probe)
{
    entity_dust(&c->base, b, probe, 8, 0, 0);
    sfx_play_at(&c->base, probe, 132, 0, 500 - mul(rnd(), 1000) / 0x8000, 0);
}

int arche_pose(wl_char *c, wl_body *b, int32_t probe, int32_t part, int32_t *ctl)
{
    switch (b->pose) {
    case 2:
        if (b->f58 == 0) {
            body_set_clip(b, D(0x62c8a8u));
            b->f58 = 1;
        } else if (b->f58 != 1) {
            if (b->f58 != 2 || b->clip_done == 0) return 0;
            body_action_reset(b);
            return 0;
        }
        if (b->clip_frame != 1) return 0;
        b->f58++;
        b->facing = b->facing != 3 ? 3 : 1;
        return 0;
    case 98: case 99:
        *ctl = 3;
        if (b->act_ctr == 0) {
            int32_t kind = ld32(c, 0x15ad0);
            if (probe == 0 && kind == 8) entity_fx_add(&c->base, 100156, 1, 12, 0, 0, 0, 0);
            int fallback = 1;
            uint16_t grp = 0;
            switch (kind) {
            case 8: case 14: if (mul(rnd(), 100) / 0x8000 < 20) grp = 17; break;
            case 12: if (mul(rnd(), 100) / 0x8000 < 20) grp = 18; break;
            case 13: case 16: grp = 19; break;
            default: break;
            }
            if (grp != 0) {
                if (probe != 0 || c->voice == NULL) fallback = 0;
                else if (hit_voice(c, grp)) fallback = 0;
            } else if (probe != 0) {
                fallback = 0;
            }
            if (fallback) hit_voice(c, 9);
        }
        body_set_clip(b, D(b->pose == 98 ? 0x62bf18u : 0x62bd80u));
        if (b->clip_done == 0) return 0;
        body_action_reset(b);
        return 0;
    case 102: case 103:
        if (b->f58 == 0) {
            body_set_clip(b, D(b->pose == 102 ? 0x62b258u : 0x62b588u));
            b->f58 = 1;
        }
        b->margin = 3200;
        return 0;
    case 107:
        b->f7d = 1;
        *ctl = 3;
        body_switch_touch(b, probe, (uint16_t)part);
        if (char_knockdown(c, b, probe, part, 1) != 0 && b->f58 == 1) voice_cue(c, probe, 11, 1, 0, 0);
        if (b->f58 == 5) {
            body_action_reset(b);
            body_action_start(b, 109, 210);
            if (probe == 0) char_status_inflict(c, 10, 0);
        }
        return 1;
    case 108:
        *ctl = 3;
        b->nocollide = 1;
        if (b->f58 == 0) {
            body_set_clip(b, D(0x62d0a0u));
            b->f58++;
        }
        if (b->clip_done != 0) body_action_reset(b);
        return 0;
    case 109:
        *ctl = 3;
        b->nocollide = 1;
        body_set_clip(b, D(0x62cf08u));
        return 0;
    case 110:
        *ctl = 3;
        body_switch_touch(b, probe, (uint16_t)part);
        b->nocollide = 1;
        b->margin = 2400;
        switch (b->f58) {
        case 0:
            body_set_clip(b, D(0x62cbd8u));
            b->f58++;
            b->vel_x = -40000;
            b->vvel = -40000;
            /* fall through */
        case 1:
            char_drift(c, b, probe, part, -15000, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (b->grounded == 0) return 0;
            land_fx(c, b, probe);
            b->f58++;
            body_set_clip(b, D(0x62cd70u));
            b->vvel = -40000;
            return 0;
        case 2:
            char_drift(c, b, probe, part, -10000, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (b->clip_done == 0) return 0;
            b->f58++;
            body_set_clip(b, D(0x62cbd8u));
            return 0;
        case 3:
            char_drift(c, b, probe, part, -5000, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (b->grounded == 0) return 0;
            land_fx(c, b, probe);
            b->f58++;
            body_set_clip(b, D(0x62cf08u));
            return 0;
        case 4:
            char_drift(c, b, probe, part, 0, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (++b->act_ctr <= 10) return 0;
            if (b->grounded == 0) { body_action_reset(b); return 0; }
            b->f58++;
            body_set_clip(b, D(0x62d0a0u));
            return 0;
        case 5:
            if (b->clip_done == 0) return 0;
            body_action_reset(b);
            return 0;
        default:
            return 0;
        }
    case 111:
        b->f7d = 1;
        *ctl = 3;
        body_switch_touch(b, probe, (uint16_t)part);
        if (char_knockdown(c, b, probe, part, 1) != 0 && b->f58 == 1) voice_cue(c, probe, 12, 1, 0, 0);
        if (b->f58 == 5) {
            if (++b->act_ctr > 100) {
                b->f58++;
                body_set_clip(b, D(0x62d0a0u));
            }
        } else if (b->f58 == 6 && b->clip_done != 0) {
            body_action_reset(b);
        }
        return 1;
    case 112: {
        b->f7d = 1;
        *ctl = 3;
        if (probe == 0) {
            int32_t r[4] = { b->x, b->y + 3200, b->w, b->h };
            switch_touch(r, (uint16_t)part, 0);
        }
        int adv = 0;
        b->nocollide = 1;
        b->margin = 2400;
        switch (b->f58) {
        case 0: {
            body_set_clip(b, D(0x62ca40u));
            b->f58++;
            b->vel_x = -40000;
            b->vvel = -40000;
            adv = 1;
            if (ld32(c, 0x15ad0) == 8) entity_fx_add(&c->base, 100157, 1, 60, 0, 0, probe, 0);
            int32_t code = 0;
            if (c->base.code == 50010) { if (ld32(c->party, 0xd4) == 1) code = 100128; }
            else if (c->base.code == 50011) code = 100129;
            if (code != 0 && !particle_live(code))
                particle_spawn(&c->base, &c->base, code, b, 0, 0, b->facing, 1, probe, 0, 2, 0, 0, 20);
            }
            /* fall through */
        case 1:
            if (b->clip_done != 0) {
                b->f58++;
                body_set_clip(b, D(0x62cbd8u));
                adv = 1;
            }
            char_drift(c, b, probe, part, -20000, 3200, -1600, 64000, 1000, -1600, 0, 0, 1);
            if (adv == 0) return 1;
            break;
        case 2:
            char_drift(c, b, probe, part, -15000, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (b->grounded == 0) return 1;
            land_fx(c, b, probe);
            b->f58++;
            body_set_clip(b, D(0x62cd70u));
            b->vvel = -40000;
            break;
        case 3:
            char_drift(c, b, probe, part, -10000, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (b->clip_done == 0) return 1;
            b->f58++;
            body_set_clip(b, D(0x62cbd8u));
            break;
        case 4:
            char_drift(c, b, probe, part, -5000, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            if (b->grounded == 0) return 1;
            land_fx(c, b, probe);
            b->f58++;
            body_set_clip(b, D(0x62cf08u));
            break;
        case 5:
            char_drift(c, b, probe, part, 0, 3200, -1600, 64000, 4000, -1600, 0, 0, 1);
            return 1;
        default:
            return 1;
        }
        if (b->f58 == 1) voice_cue(c, probe, 12, 1, 0, 0);
        return 1; }
    case 113:
        if (b->f58 == 0) {
            b->f58 = 1;
            b->act_ctr = mul(10, rnd()) / 0x8000 + 10;
        } else if (b->f58 != 1) {
            if (b->f58 != 2 || b->clip_done == 0) return 0;
            int32_t f = 500 - mul(rnd(), 1000) / 0x8000;
            wl_body *b0 = c->base.body;
            if (probe == 0) {
                wl_voice *v = vpool_take(ldp(g_eng.sys, 0x574), 0);
                if (v != NULL) {
                    void *buf = v->buf;
                    int32_t vol, pan;
                    if (sfx_place(&c->base, ld32(cfg(), 0x168) - 500, 0, 0, b0, &vol, &pan) != 0) {
                        g_eng.dsb_volume_5bb870(buf, vol);
                        g_eng.dsb_pan_5bb880(buf, pan);
                        g_eng.dsb_freq_5bb890(buf, f);
                        g_eng.dsb_play_5bb8c0(buf);
                    }
                }
            }
            body_set_clip(b, D(0x62da30u));
            b->f58++;
            return 0;
        }
        if (b->act_ctr > 0) b->act_ctr--;
        if (b->grounded != 0 && b->act_ctr <= 0) {
            b->f58++;
            b->vel_x = 0;
            body_set_clip(b, D(0x62d898u));
            return 0;
        }
        state_clip(c, b, probe, b->grounded);
        return 0;
    case 115:
        b->f7d = 1;
        *ctl = 3;
        body_switch_touch(b, probe, (uint16_t)part);
        if (char_knockdown(c, b, probe, part, 1) == 0) return 1;
        if (b->f58 == 1) {
            voice_cue(c, probe, 12, 1, 0, 0);
            room_hitstop_set(1, 200);
        } else if (b->f58 == 5) {
            room_hitstop_set(0, 0);
        }
        return 1;
    case 116:
        if (b->f58 == 0) {
            b->f58 = 1;
            body_set_clip(b, D(0x62dbc8u));
            return 0;
        }
        if (b->f58 == 1) {
            if (b->clip_done == 0) return 0;
            b->f58++;
            body_set_clip(b, D(0x62dd60u));
            return 0;
        }
        if (b->f58 == 2 && (c->f268 != 0 || ld16(c->party, 0x44a) == 0)) body_action_reset(b);
        return 0;
    case 125:
        body_set_clip(b, c->clip_up0);
        return 0;
    case 126:
        body_set_clip(b, D(0x62dd60u));
        return 0;
    default:
        return 0;
    }
}
