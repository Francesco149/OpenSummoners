/* char_hit.c — the struck character.  See char_hit.h.  Transcribed from the asm. */
#include "char_hit.h"

#include <stddef.h>
#include <string.h>

#include "arche_pose.h"      /* char_room_logs 0x5b4510 */
#include "camera_follow.h"   /* view_box_visible 0x4766a0 */
#include "char_integrate.h"  /* char_carry 0x442920, body_action_start 0x40a540 */
#include "entity_life.h"     /* entity_release 0x556180, entity_set_handle 0x555f00 */
#include "particle_init.h"   /* entity_setup 0x426620, entity_track_on 0x426d70, body_clip_jitter 0x426ec0 */
#include "party_member.h"    /* dramatist_row 0x409090 */
#include "char_def.h"        /* stat_jitter 0x426e00 */
#include "char_state.h"      /* char_add_hp 0x5b54d0 */
#include "char_status.h"     /* world_log_line 0x44e6c0 */
#include "engine_calls.h"
#include "entity_move.h"     /* entity_move_horizontal 0x54db10 */
#include "particle_spawn.h"
#include "rng.h"
#include "sound_fx.h"

#include <stdio.h>

static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static const void *D(uint32_t va) { return g_eng.data(va); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
#define W (*g_eng.world)
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static const wl_pslot *pslot(const wl_char *c) { return (const wl_pslot *)(uintptr_t)c->fc794; }

/* the keyboard config **(sys+0x320) */
static const uint8_t *cfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}

void hit_combo(wl_char *atk, wl_body *b, int32_t *dmg, int32_t *count, wl_char *struck, int32_t kind)
{
    (void)b;
    if (pslot(atk) == NULL) { *count = 1; return; }
    uint8_t *r = (uint8_t *)pslot(atk)->room;
    uint32_t n = (uint32_t)ld32(r, 0x40e0) + 1;
    st32(r, 0x40e0, (int32_t)n);
    for (int k = 3; k > 0; k--) st32(r, 0x40f0 + 4u * k, ld32(r, 0x40ec + 4u * k));
    st32(r, 0x40f0, kind);
    uint32_t fresh = 0;
    for (int k = 0; k + 1 < 4; k++) {
        int32_t x = ld32(r, 0x40f4 + 4u * k);
        if (x == 255) break;
        int seen = 0;
        for (int j = k; j >= 0; j--) if (ld32(r, 0x40f0 + 4u * j) == x) { seen = 1; break; }
        if (!seen) fresh++;
    }
    int32_t add = 0;
    if (n > 1) {
        add = ld32(r, 0x40e4) > 600 ? 40 : 30;
        switch (fresh & 0xffff) {
        case 0: break;
        case 1: add += 10; break;
        case 2: add += 30; break;
        default: add += 50; break;
        }
        int32_t v = ld32(r, 0x40e8) + add;
        if (v > 2000) v = 2000;
        st32(r, 0x40e8, v);
    }
    st32(r, 0x40e4, 1000);
    st32(r, 0x40ec, n >= 5 ? add : 0);     /* (n <= 1 never reaches 5: retail's register then holds `count`) */
    if ((uint32_t)ld32(r, 0x40dc) < n) { st32(r, 0x4100, 1); st32(r, 0x40dc, (int32_t)n); }
    else st32(r, 0x4100, 0);
    if (n >= 5 && ld32(r, 0x40e8) > 0) {
        uint32_t f = 400 * n - 1600;
        int32_t freq = f <= 4000 ? (int32_t)f : 4000;
        wl_body *sb = struck->base.body;
        wl_voice *v = vpool_take(ldp(g_eng.sys, 0x408), 0);
        if (v != NULL) {
            void *buf = v->buf;
            int32_t vol, pan;
            if (sfx_place(&struck->base, ld32(cfg(), 0x168) + 800, 0, 0, sb, &vol, &pan) != 0) {
                g_eng.dsb_volume_5bb870(buf, vol);
                g_eng.dsb_pan_5bb880(buf, pan);
                g_eng.dsb_freq_5bb890(buf, freq);
                g_eng.dsb_play_5bb8c0(buf);
            }
        }
    }
    int32_t bonus = (uint32_t)ld32(r, 0x40e0) >= 5 ? ld32(r, 0x40e8) : 0;
    *dmg = mul(*dmg, bonus) / 1000 + *dmg;
    *count = ld32(r, 0x40e0);
}

int32_t dmg_party_scale(const wl_char *c, int32_t dmg)
{
    if (dmg <= 0) return 0;
    int32_t v = mul(dmg, ld32(pslot(c), 0xa08)) / 1000;
    return v >= 1 ? v : 1;
}

int32_t dmg_record(const wl_char *c, int32_t dmg, int32_t magic)
{
    const wl_pslot *ps = pslot(c);
    if (ps == NULL) return 0;
    uint8_t *r = (uint8_t *)ps->room;
    uint32_t at = magic != 0 ? 0x40c8 : 0x40c0;
    int32_t rec = 0;
    if (ld32(r, at) < dmg) {
        st32(r, at, dmg);
        st32(r, at + 4, ps->id);
        if (dmg > 6) rec = 1;
    }
    return char_room_logs(c) != 0 ? rec : 0;
}

int32_t dmg_glyphs(const char *s, uint16_t *out, uint16_t *n)
{
    uint16_t k = 0, i = 0;
    for (;;) {
        size_t len = strlen(s);
        uint32_t lim = len <= 40 ? (uint32_t)len : 40;
        if (i >= lim) break;
        signed char ch = (signed char)s[i];
        if (ch == ' ') out[k++] = 255;
        else if (ch >= 33 && ch <= 122) out[k++] = (uint16_t)(ch - 33);
        i++;
    }
    *n = k;
    return i;
}

int32_t crate_knock(wl_entity *e, int32_t dir)
{
    wl_body *b = e->body;
    int32_t v = mul(dir, mul(4000, (int32_t)rng_rand()) / 0x8000 + 20000);
    if (b->facing == 3) v = -v;
    b->vel_x = v;
    return v;
}

/* ---- FUN_004895c0: the pending hit taken ---------------------------------------------------------------- */

/* a sound of pool sys+`pool` at the character's body 0, gain `gain` over the effects gain, frequency `freq` */
static void hit_sound(wl_char *c, uint32_t pool, int32_t gain, int32_t freq)
{
    wl_body *b = c->base.body;
    wl_voice *v = vpool_take(ldp(g_eng.sys, pool), 0);
    if (v == NULL) return;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(&c->base, ld32(cfg(), 0x168) + gain, 0, 0, b, &vol, &pan) == 0) return;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, freq);
    g_eng.dsb_play_5bb8c0(buf);
}

/* the status message `fmt` (a retail string) about the character into the world's message log */
static void status_log(wl_char *c, const char *fmt)
{
    if (c->fc794 == 0 || char_room_logs(c) == 0) return;
    char line[512];
    snprintf(line, sizeof line, fmt, (const char *)c->party);
    world_log_line(W, line);
}

/* the cure message of status row k - 1 (the rows' index runs one behind the status: 0 = status 1) */
static int cure_text(wl_char *c, int32_t k, const char **fmt)
{
    switch (k) {
    case 0:
        *fmt = D(0x85530c);   /* "%s's strength returns!" */
        st32(c, 0xfc, ld32(c, 0x15910));
        st32(c, 0x100, ld32(c, 0x15910));
        return 1;
    case 1: *fmt = D(0x855340); return 1;   /* "%s can move again!" */
    case 2: *fmt = D(0x855324); return 1;   /* "%s is no longer confused." */
    case 4: *fmt = D(0x855354); return 1;   /* "%s woke up." */
    case 5: *fmt = D(0x855388); return 1;   /* "%s is no longer poisoned." */
    case 6: *fmt = D(0x855360); return 1;   /* "%s's defenses have returned to normal." */
    default: return 0;
    }
}

/* the 18 status rows (0x628708 + 140 k: u16 +0x70 the timer a hit takes off): `all` ends each such status,
 * else its timer drops by that much (ending it at 0); an ended status logs its cure line */
static void hit_wears_statuses(wl_char *c, int all)
{
    for (int32_t k = 0; k < 18; k++) {
        uint16_t wear = ld16(D(0x628778 + 140u * (uint32_t)k), 0);
        if (wear == 0) continue;
        uint8_t *pt = (uint8_t *)c->party;
        uint16_t t = ld16(pt, 0x448 + 2u * (uint32_t)k);
        if (t == 0) continue;
        if (!all && t > wear) { st16(pt, 0x448 + 2u * (uint32_t)k, (uint16_t)(t - wear)); continue; }
        st16(pt, 0x448 + 2u * (uint32_t)k, 0);
        const char *fmt;
        if (cure_text(c, k - 1, &fmt)) status_log(c, fmt);
    }
}

/* a row's resist chain: char +0xc7a8 x party +0xa4 x char +0x15a2c per mille, per mille (index i) */
static int32_t resist(const wl_char *c, uint32_t i)
{
    int32_t v = mul(mul(ld32(c->party, 0xa4 + 4 * i), ld32(c, 0x15a2c + 4 * i)), ld32(c, 0xc7a8 + 4 * i));
    return v / 1000 / 1000;
}

/* the gains of the hit sound per hit kind live in the pool table sys+0x364 + 4 id */
static void sparks(wl_char *c, wl_body *b, int32_t code, int32_t n, int32_t hx, int32_t hy, int32_t dir)
{
    for (int32_t i = 0; i < n; i++) {
        int32_t a = mul(4, rnd()) / 0x8000;
        int32_t y = mul(1200, rnd()) / 0x8000 + hy - 600;
        int32_t x = mul(1200, rnd()) / 0x8000 + hx - 600;
        if (view_box_visible(W->view, b->x, b->y, b->w, b->h, 16000))
            particle_spawn(&c->base, &c->base, code, b, x, y, dir, 1, 0, a, 2, 0, 0, 0);
    }
}

static void pose_start(wl_body *b, uint16_t pose, uint16_t len)
{
    b->pose = pose; b->act_len = len; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0;
    b->act_arg = 0; b->spell = 0; b->target = 0; b->act_5a = 0; b->act_64 = 0;
}

int32_t char_take_hit(wl_char *c)
{
    wl_body *b = c->base.body;
    int32_t hpd = 0;             /* v200: the HP change applied (the return) */
    int32_t block = 0;           /* v193 */
    int32_t pw = b->w, ph = b->h; /* v181 / v178: the struck box */
    int32_t combo = 1;           /* v201 */
    int32_t snd = 0, sfreq = 0, sgain = 0;  /* v183 v184 v191 */
    int32_t fixed = 0;           /* v189 */
    int32_t worn = 0;            /* v198 */
    int32_t elem = 0;            /* v176 */
    int32_t guard = 0;           /* v192 */
    int32_t absorbed = 0;        /* v195 */
    int32_t st = 0;              /* i (the status index) */
    int32_t knock = 0;           /* v196 */
    int32_t roll_ok = 0;         /* v194 */
    int32_t rec = 0;             /* v186 */
    int32_t pmul = 1000;         /* v197 */
    int32_t pflag = 0;           /* v199 */
    int32_t px = 0, py = 0;      /* the part's centre */
    wl_world *w = W;
    if (w->f1034 != 0 || w->scene_kind == 2 || ld32(c, 0x268) != 0 || ld32(c, 0x15908) != 0) return 0;
    st16(c, 0x158c0, 0);
    int32_t dir = ld32(c, 0x15ab4);   /* v188 */
    if (dir == 99) dir = b->facing != 3 ? 3 : 1;
    switch (ld32(c, 0x15ad0)) {
    case 1: snd = 118; sgain = 500; sfreq = mul(1000, rnd()) / 0x8000 - 2000; break;
    case 2: snd = 117; sfreq = mul(1000, rnd()) / 0x8000 - 500; break;
    case 3: snd = 120; sfreq = 500 - mul(1000, rnd()) / 0x8000; break;
    case 4: snd = 121; sfreq = 500 - mul(1000, rnd()) / 0x8000; break;
    case 5: snd = 122; sfreq = mul(1000, rnd()) / 0x8000 + 500; break;
    case 6: case 11: snd = 124; sfreq = mul(1000, rnd()) / 0x8000 + 500; break;
    case 7: snd = 123; elem = 3; break;
    case 8: case 9: snd = 123; elem = 1; break;
    case 10: snd = 123; break;
    case 12: snd = 119; sgain = 1000; elem = 3; sfreq = mul(1000, rnd()) / 0x8000 + 500; break;
    case 13: fixed = 1; snd = 120; sgain = -1000; elem = 6; sfreq = 500 - mul(1000, rnd()) / 0x8000; break;
    case 14: snd = 120; fixed = 1; sgain = -1000; elem = 1; sfreq = 500 - mul(1000, rnd()) / 0x8000; break;
    case 15: snd = 125; sfreq = mul(1000, rnd()) / 0x8000 - 500; break;
    case 16: snd = 130; sgain = 1000; sfreq = mul(1000, rnd()) / 0x8000 - 500; break;
    case 17: elem = 4; snd = 117; sfreq = mul(1000, rnd()) / 0x8000 - 500; break;
    default: break;
    }
    /* the struck part's box (its frame record, mirrored by the facing) and damage multiplier */
    int32_t part = ld32(c, 0x15ae8);
    if (c->base.rects != NULL && part < (int32_t)c->base.nrects) {
        const int32_t *fr = (const int32_t *)((const uint8_t *)b->frames + 20 * part);
        const uint8_t *rc = c->base.rects + 32 * part;
        px = fr[0]; py = fr[1]; pw = fr[2]; ph = fr[3];
        pmul = ld32(rc, 16);
        pflag = ld32(rc, 20);
        if (b->facing == 3) px = -px;
    }
    /* the hit point relative to the body centre, held inside the part's box */
    int32_t hx = ld32(c, 0x15abc) - b->w / 2 - b->x;   /* v187 */
    int32_t hy = ld32(c, 0x15ac0) - b->h / 2 - b->y;   /* v190 */
    {
        int32_t lo = px - pw / 2, m = lo <= hx ? hx : lo;
        if (px + pw / 2 < m) hx = px + pw / 2;
        else if (lo > hx) hx = lo;
        int32_t ylo = py - ph / 2, ym = ylo <= hy ? hy : ylo;
        if (py + ph / 2 < ym) hy = py + ph / 2;
        else if (ylo > hy) hy = ylo;
    }
    /* the status the hit may inflict (+0x15ac4) and its roll */
    switch (ld32(c, 0x15ac4)) {
    case 1: st = 5; break;
    case 3: st = 6; break;
    case 4: st = 8; break;
    case 5: st = 2; break;
    case 6: st = 3; break;
    case 7: st = 7; break;
    case 8: st = 9; break;
    case 17: st = 4; break;
    default: st = 0; break;
    }
    if (st != 0) {
        int32_t pv = ld32(c, 0x15ac8);
        const uint8_t *pt = (const uint8_t *)c->party;
        if (st == 4 && (ld32(c, 0x15ae4) - mul(1200, ld32(pt, 0xd8) + ld32(pt, 0xe0) + ld32(pt, 0xe4)) / 1000 < 6 ||
                        ld32(c, 0x15a94) != 0)) goto rolled;
        int32_t ri = ld32(D(0x62877c + 140u * (uint32_t)st), 0);
        if (ri != 0) {
            int32_t m = resist(c, (uint32_t)ri);
            if (m == 0) goto rolled;
            pv = mul(m, pv) / 1000;
        }
        if (st == 9) pv -= ld32(pt, 0x68) + ld32(pt, 0x78) + ld32(pt, 0x90) + ld32(c, 0xc7dc);
        else pv -= ld32(pt, 0x70) + ld32(pt, 0x80) + ld32(pt, 0x98) + ld32(c, 0xc7e0);
        if (mul(10000, rnd()) / 0x8000 % 1000 < pv) roll_ok = 1;
    }
rolled:;
    int32_t dmg;   /* v179 / v49 */
    if (fixed != 0) {
        dmg = ld32(c, 0x15ad8);
    } else {
        if (ld16(c->party, 0x464) != 0) guard = 1;
        if (b->state == 5 && ld32(c, 0x15a10) != 0) {
            int32_t mg = ld32(c, 0x15ae0);
            if (ld32(c, 0x15adc) == 0) block = 1;
            else switch (ld32(c, 0x15a28)) {
                case 1: if (mg == 0) block = 1; break;
                case 2: if (mg == 1) block = 1; break;
                case 3: block = 1; break;
                default: break;
                }
        }
        /* the attack id into the recent-hit memory (the slot with the lowest timer when new) */
        int32_t id = ld32(c, 0x15ad4), slot = -1;
        uint16_t tmin = 0;
        for (uint16_t k = 0; k < 8; k++) {
            if (c->recent_hit[k].id == id) { slot = k; goto found; }
            if (slot == -1 || tmin > c->recent_hit[k].t) { tmin = c->recent_hit[k].t; slot = k; }
        }
found:
        c->recent_hit[slot].t = 80;
        c->recent_hit[slot].id = id;
        dmg = ld32(c, 0x15ad8);
        int32_t r = mul(1000, rnd()) / 0x8000;
        if (dmg > 0) {
            const uint8_t *pt = (const uint8_t *)c->party;
            int32_t def;
            if (ld32(c, 0x15ae0) != 0) {
                def = stat_jitter((ld32(c, 0xc7e0) + ld32(pt, 0x98) + ld32(pt, 0x80) + ld32(pt, 0x70)) / 4, 1000, r);
            } else {
                int32_t s = ld32(c, 0xc7dc) + ld32(pt, 0x90) + ld32(pt, 0x78) + ld32(pt, 0x68);
                def = stat_jitter((s < 0 ? 0 : s) / 4, 1000, r);
            }
            dmg -= def;
            if (dmg < 1) dmg = 1;
            if (ld16(pt, 0x452) != 0) dmg = mul(dmg, 1500) / 1000;
        } else {
            dmg = 0;
        }
        wl_char *atk = (wl_char *)ldp(c, 0x15aa8);
        if (block == 0 && dmg > 0 && atk != NULL) hit_combo(atk, b, &dmg, &combo, c, elem);
        dmg = mul(dmg, pmul) / 1000;
        if (elem != 0) dmg = mul(dmg, resist(c, (uint32_t)elem)) / 1000;
        if (block != 0) dmg = mul(dmg, 1000 - ld16(c, 0x15a14)) / 1000;
        if (guard != 0 && dmg > 0) { absorbed = dmg / 2; dmg -= absorbed; }
        if (c->fc794 != 0) dmg = dmg_party_scale(c, dmg);
        atk = (wl_char *)ldp(c, 0x15aa8);
        if (atk != NULL) rec = dmg_record(atk, dmg, ld32(c, 0x15ae0));
    }
    /* the damage number (particle 100042 with the text's glyphs; "%d Max" + a sound on a room record) */
    if (dmg > 0 && w->f2760 == 0) {
        const char *fmt = D(0x854540);   /* "%d" */
        if (rec != 0) {
            hit_sound(c, 0x394, -500, 0);
            fmt = D(0x8554a4);           /* "%d Max" */
        }
        char f[16], text[64];
        strcpy(f, fmt);
        int32_t kind, color;
        if (ld32(c, 0x15adc) != 0) { kind = 14; color = c->base.faction != 1 ? 4 : 6; }
        else { kind = 9; color = c->base.faction != 1 ? 3 : 5; }
        int32_t y = b->h / 2 + b->y + hy;
        int32_t x = mul(3200, rnd()) / 0x8000 + b->w / 2 + b->x + hx - 1600;
        snprintf(text, sizeof text, f, dmg);
        wl_entity *p = particle_spawn(&c->base, &c->base, 100042, b, x, y, dir, 0, 0, 0, 0, 0, 0, 20);
        if (p != NULL) {
            wl_body *pb = p->body;
            dmg_glyphs(text, (uint16_t *)((uint8_t *)p + 0x200), (uint16_t *)((uint8_t *)p + 0x250));
            st16(p, 0x1f8, (uint16_t)color);
            st32(p, 0x1fc, kind);
            st16(p, 0x258, 60);
            pb->vvel = -48000;
        }
    }
    if (guard != 0 || block != 0) goto absorb;
    if (dmg <= 0) goto inflict;
    {
        int32_t thr;
        if (dmg > 1 && fixed == 0 && ld16(c->party, 0x466) == 0) c->fc784 += 1500;
        thr = ld32(c, 0x15a0c);
        if (c->base.faction == 1) {
            if (w->f2780 == 10) thr = mul(thr, 1400) / 1000;
            else if (w->f2780 == 20) thr = mul(thr, 1200) / 1000;
        }
        if (ld16(c, 0x15a08) != 0 && c->fc784 >= thr) {
            /* the stagger break: turned round, the break pose, every hit-worn status ended */
            c->fc784 = 0;
            b->facing = dir != 3 ? 3 : 1;
            uint16_t bp = ld16(c, 0x15a08);
            if ((b->act_len / 100 <= 8 || b->pose == 0) && bp != b->pose) pose_start(b, bp, 800);
            hit_wears_statuses(c, 1);
            goto inflict;
        }
    }
    if (ld16(c, 0x159ec) != 0) {
        /* the flinch, unless armoured (+0x159fc over the damage, or the +0x15a00 meter; the part's flag 1 / 2
         * forces it off / on; actions of priority 220 (physical only) / 230 keep it off) */
        int armour = 0;
        if (ld32(c, 0x159fc) > dmg) armour = 1;
        else {
            uint16_t cap = ld16(c, 0x15a00);
            if (cap != 0) {
                st16(c, 0x272, (uint16_t)(ld16(c, 0x272) + (ld32(c, 0x15adc) != 0 ? 600 : 200)));
                if (cap > ld16(c, 0x272)) armour = 1;
            }
        }
        if (pflag == 1) armour = 0;
        else if (pflag == 2) armour = 1;
        if (b->act_len == 220) { if (ld32(c, 0x15adc) == 0) goto wear; }
        else if (b->act_len == 230) goto wear;
        if (armour != 0) goto wear;
        st16(c, 0x272, 0);
        if ((int32_t)ld16(c, 0xc876) > mul(1000, rnd()) / 0x8000) {
            uint16_t half = (uint16_t)(mul(80, ld16(c, 0xc87c)) / 1000 / 2);
            st16(c, 0x14234, (uint16_t)((int32_t)half * rnd() / 0x8000 + half));
        }
        if (c->ai_state != 4) char_ai_reset(c, 0);
        c->f14294 = 1;
        if (fixed != 0) {
            b->vvel = -32000;
            uint16_t fp = ld16(c, 0x159ec);
            if ((b->act_len / 100 <= 2 || b->pose == 0) && fp != b->pose) pose_start(b, fp, 210);
            goto wear;
        }
        if (ld16(c, 0x158c4) >= 2) goto wear;
        if (roll_ok != 0 && st == 9) {
            /* status 9: the attacker takes hold of the struck character (a carrier); otherwise no status */
            wl_char *atk = (wl_char *)ldp(c, 0x15aa8);
            int ok = 0;
            if (c->can_carry != 0) {
                if (atk == NULL) {
                    if (c->held != NULL) { c->held->carrier = NULL; c->held = NULL; }
                    ok = ldp(c, 0x15aa8) != NULL;
                } else if (atk->base.alive == 1 && atk->base.body->active == 1 && ld16(atk->party, 0x44a) == 0 &&
                           atk->carrier == NULL && c->held == NULL) {
                    atk->carrier = c;
                    c->held = atk;
                    ok = ldp(c, 0x15aa8) != NULL;
                }
            }
            if (!ok) roll_ok = 0;
        }
        int32_t kx = 32000, ky = 32000;   /* v95 v96 */
        switch (ld32(c, 0x159f8)) {
        case 1: kx = 40000; break;
        case 2: kx = 24000; ky = 24000; break;
        default: break;
        }
        int32_t vy, vx = 0, flip;         /* v99 v73 v100 */
        switch (ld32(c, 0x15ab8)) {
        case 0: flip = 1; vy = -(kx + ky); break;
        case 2: vy = b->grounded != 0 ? -ky : kx + ky; flip = 0; break;
        case 99: vy = -ky; flip = hy < ld32(c, 0x15a04) ? 1 : 0; break;
        default: vy = 0; flip = 0; break;
        }
        int32_t hd = ld32(c, 0x15ab4);
        if (hd != 99) {
            if (b->facing == hd) { vx = kx; flip = flip == 0; }
            else vx = -kx;
        }
        knock = vx;
        b->vvel = vy;
        b->vel_x = vx;
        uint16_t np = ld16(c, 0x15a02);
        if (flip == 0 || np == 0) np = ld16(c, 0x159ec);
        if (!((b->act_len / 100 > 2 && b->pose != 0) || np == b->pose)) pose_start(b, np, 210);
        st16(c, 0x158c4, (uint16_t)(ld16(c, 0x158c4) + 1));
        st16(c, 0x158c2, 30);
    }
wear:
    hit_wears_statuses(c, 0);
    worn = 1;
inflict:
    if (roll_ok != 0) {
        uint32_t k = (uint32_t)(uint16_t)st;
        uint8_t *pt = (uint8_t *)c->party;
        uint16_t prior = ld16(pt, 0x448 + 2 * k);
        int32_t pv = ld32(c, 0x15acc);
        const uint8_t *row = D(0x628708 + 140 * k);
        int32_t base = ld16(row, 0x64);
        if (ld32(c, 0x15a94) == 0) base += ld32(row, 0x68);
        int32_t len = mul(ld16(row, 0x66), rnd()) / 0x8000 + base;
        if (len < 0) len = 0;
        else len = mul(ld16(row, 0x66), rnd()) / 0x8000 + base;
        st16(pt, 0x46c + 2 * k, (uint16_t)len);
        st16(pt, 0x448 + 2 * k, ld16(pt, 0x46c + 2 * k));
        st32(pt, 0x490 + 4 * k, pv);
        char line[512];
        int msg = 0;
        switch (k) {
        case 1:
            if (prior == 0) { msg = 1; strcpy(line, D(0x8553d4)); }   /* "%s has fallen..." */
            for (uint32_t j = 0; j < 18; j++) if (j != k) st16(pt, 0x448 + 2 * j, 0);
            break;
        case 2: if (prior == 0) { msg = 1; strcpy(line, D(0x8553e8)); } break;   /* "%s is paralyzed!" */
        case 3:
            if (prior == 0) { msg = 1; strcpy(line, D(0x8553fc)); }   /* "%s is confused! Watch out!" */
            sfx_play_at(&c->base, 0, 116, 0, 0, 0);
            break;
        case 5: if (prior == 0) { msg = 1; strcpy(line, D(0x855418)); } break;   /* "%s has fallen asleep." */
        case 6: if (prior == 0) { msg = 1; strcpy(line, D(0x855454)); } break;   /* "Poison rips through %s's veins!" */
        case 7:
            if (prior == 0) { snprintf(line, sizeof line, D(0x855430), pv); msg = 1; }   /* defenses reduced by %d */
            hit_sound(c, 0x530, 0, 0);
            break;
        case 13:
            sfx_play_at(&c->base, 0, 114, 0, 0, 0);
            if (prior == 0) { snprintf(line, sizeof line, D(0x8553a4), pv); msg = 1; }   /* attack power + %d */
            break;
        default: break;
        }
        if (msg) status_log(c, line);
        worn = 1;
        goto absorb_check;
    }
    goto applied;
absorb:
    if (roll_ok == 0) goto applied;
absorb_check:
    if (roll_ok != 0 && guard != 0) {
        int32_t v = 10 * ld32(c->party, 0x4c8);
        if (v / 100 > absorbed) absorbed = v / 100;
    }
applied:
    if (fixed == 0 && worn != 0 && ld16(c, 0x15900) != 0) st16(c, 0x26e, ld16(c, 0x15900));
    if (char_add_hp(c, -dmg, &hpd, 0) == 1) {
        char_die(c, 0, 1, dir);
        goto struck_fx;
    }
    {
        /* a crate (rider kind 1) that struck: pushed back by the knock, or knocked off when stuck */
        wl_entity *ae = (wl_entity *)ldp(c, 0x15aac);
        if (ae != NULL && c->move_mode != 3 && ae->rider_kind == 1) {
            int side = ae->body->x + ae->body->w / 2 > b->x + b->w / 2;
            int32_t d;
            if (knock != 0) {
                d = knock >= 0 ? 3200 : -3200;
                if (b->facing == 3) d = -d;
            } else {
                d = side ? -3200 : 3200;
            }
            b->nocollide = 1;
            if (entity_move_horizontal(&c->base, b, d, 0, 1, 0, 0, 1) == 0) crate_knock(ae, side ? 1 : -1);
        }
    }
    if (guard == 0 || absorbed <= 0) {
        if (block == 0) goto struck_fx;
        uint16_t bs = ld16(c, 0x15a20);
        if (bs != 0) {
            if (bs == 75) {
                w->f279c += mul(400, rnd()) / 0x8000 + 300;
                if (w->f279c > 1000) {
                    w->f279c = 0;
                    int32_t f = -1500 - mul(200, rnd()) / 0x8000;
                    hit_sound(c, 0x494, -700, f);
                }
            }
            int32_t f = -500 - mul(1000, rnd()) / 0x8000;
            hit_sound(c, 0x364 + 4u * bs, 0, f);
        }
        sparks(c, b, 100075, ld32(cfg(), 0x94) != 0 ? 20 : 8, hx, hy, dir);
        return hpd;
    }
    /* the guard took half: sparks, its sound, the shield entity +0x204 = 3, the guard meter worn */
    for (uint16_t i = 0; i < (ld32(cfg(), 0x94) != 0 ? 20 : 8); i++) {
        int32_t fl = dir != 3 ? 3 : 1;
        int32_t a = mul(4, rnd()) / 0x8000;
        int32_t dy = mul(1600, rnd()) / 0x8000 - 800;
        int32_t dx = mul(1600, rnd()) / 0x8000 - 800;
        if (view_box_visible(W->view, b->x, b->y, b->w, b->h, 16000))
            particle_spawn(&c->base, &c->base, 100076, b, dx, dy, fl, 1, 0, a, 2, 0, 0, 0);
    }
    {
        int32_t f = 1000 - mul(1000, rnd()) / 0x8000;
        hit_sound(c, 0x564, 0, f);
    }
    if (ldp(c, 0xc800) != NULL) st32(ldp(c, 0xc800), 0x204, 3);
    {
        uint8_t *pt = (uint8_t *)c->party;
        int32_t m = mul(1000, absorbed) / ld32(pt, 0x4c8);
        uint16_t g1 = ld16(pt, 0x464), g2 = ld16(pt, 0x488);
        uint16_t mx = g1 > g2 ? g1 : g2;
        if (mx < 1) mx = 1;
        int32_t t = mul((int32_t)mx, m) / 1000;
        if (t < 1) t = 1;
        uint16_t g = ld16(pt, 0x464);
        if (g == 0) return hpd;
        st16(pt, 0x464, g <= (uint16_t)t ? 0 : (uint16_t)(g - t));
    }
    return hpd;
struck_fx:
    if (dmg > 0) {
        if ((uint16_t)snd != 0) hit_sound(c, 0x364 + 4u * (uint16_t)snd, sgain, sfreq);
        if (fixed == 0) {
            sparks(c, b, 100073, ld32(cfg(), 0x94) != 0 ? 32 : 16, hx, hy, dir);
            if (ld32(c, 0x158fc) != 0) st16(c, 0x26c, 40);
            if (c->base.rects != NULL && part < (int32_t)c->base.nrects) {
                uint8_t *rc = c->base.rects + 32 * part;
                if (ld32(rc, 24) != 0) st16(rc, 28, 40);
            }
        }
    }
    return hpd;
}

/* ---- the death and its rewards -------------------------------------------------------------------------- */

void char_part_release(wl_char *c, uint16_t i)
{
    wl_entity **p = (wl_entity **)((uint8_t *)c + 4u * (12799u + i));
    if (*p == NULL) return;
    entity_release(*p, 0);
    *p = NULL;
}

wl_entity *pickup_spawn(int32_t code, const wl_body *at, int32_t item)
{
    wl_world *w = W;
    uint8_t *L = w->f1044;
    wl_entity *e = NULL;
    for (uint16_t k = 0;; ) {
        uint16_t cur = ld16(L, 0xd0);
        e = ldp(w, 0x23e0 + 4u * cur);
        st16(L, 0xd0, (uint16_t)((cur + 1) % 96));
        if (e->alive == 0) break;
        if (++k >= 96) return NULL;
    }
    if (e == NULL) return NULL;
    int32_t y = at->h / 2 + at->y;
    int32_t x = at->w / 2 + at->x;
    wl_body *b = e->body;
    uint8_t *lst = ldp(w, 0x2794);
    if (ld16(lst, 8) < ld16(lst, 0)) {
        uint16_t n = ld16(lst, 8);
        *(wl_entity **)((uint8_t *)ldp(lst, 4) + 4u * n) = e;
        st16(lst, 8, (uint16_t)(n + 1));
    }
    st32(e, 0x204, 0);
    st32(e, 0x1fc, 0);
    st32(e, 0x208, item);
    switch (code) {
    case 100159: case 100160: case 100161: {
        int16_t row = code == 100159 ? 0 : code == 100160 ? 8 : 16;
        st32(e, 0x200, code == 100159 ? 1 : code == 100160 ? 10 : 100);
        st32(e, 0x1f8, code == 100159 ? 3000 : code == 100160 ? 2500 : 2000);
        st32(e, 0x1fc, 1);
        st32(e, 0x204, 1);
        int32_t fc = mul(1000, rnd()) / 0x8000 >= 500 ? 1 : 3;
        entity_setup(e, 3, code, 0, x, y, 2400, 2400, fc, 1, -8, -8, 2, 14, 0);
        entity_set_track(e, 0, 435, row, 1, 0, 0, 0);
        if (b->clip != D(0x647330)) {
            b->clip = D(0x647330);
            b->clip_done = 0; b->clip_frame = 0; b->clip_timer = 0;
        }
        body_clip_jitter(b);
        b->vvel = mul(40000, rnd()) / 0x8000 - 120000;
        b->vel_x = mul(30000, rnd()) / 0x8000 + 20000;
        e->weight = 0;
        entity_set_handle(e, 0);
        return e;
    }
    case 100162: {
        st32(e, 0x1fc, 2);
        st32(e, 0x204, 1);
        int32_t fc = mul(1000, rnd()) / 0x8000 >= 500 ? 1 : 3;
        entity_setup(e, 3, code, 0, x, y, 3200, 3200, fc, 1, 0, -8, 2, 14, 0);
        entity_track_on(e, 0, 435, 24);
        b->vvel = mul(40000, rnd()) / 0x8000 - 120000;
        b->vel_x = mul(30000, rnd()) / 0x8000 + 20000;
        e->weight = 0;
        entity_set_handle(e, 0);
        return e;
    }
    case 100163: {
        int32_t r = rnd();
        st32(e, 0x204, 1);
        int32_t fc = mul(1000, r) / 0x8000 >= 500 ? 1 : 3;
        entity_setup(e, 3, code, 0, x, y, 3200, 3200, fc, 1, 0, -8, 2, 14, 0);
        entity_track_on(e, 0, 435, 26);
        b->vvel = mul(mul(4000, rnd()) / 0x8000 - 12000, 10);
        b->vel_x = mul(30000, rnd()) / 0x8000 + 30000;
        e->weight = 0;
        sfx_play_at(e, 0, 36, 0, 0, 0);
        entity_set_handle(e, 0);
        return e;
    }
    default:
        g_eng.h426110(g_eng.log, (intptr_t)D(0x8a0af8u), 1, (intptr_t)D(0x8a9b6cu));   /* "Unknown Object Type Treasure." */
        g_eng.h5bf5db(0);
        return NULL;
    }
}

int32_t foe_drops(wl_char *c, int32_t gold, int32_t item, int32_t chance)
{
    const wl_body *b = c->base.body;
    if (item != 0 && mul(10000, rnd()) / 0x8000 % 1000 < chance) {
        pickup_spawn(100162, b, item);
        int32_t *last = (int32_t *)D(0x8a940c);
        if (*last != 0 && *last == item) *last = 0;
    }
    int32_t h = gold / 100, t = gold % 100 / 10;
    int32_t o = gold % 100 + 2 * (4 * (gold % 100 / -10) - t);
    int32_t r = o + gold / 100 + t;
    if (r < 3) {
        if (h > 0) { h--; t += 10; }
        else if (t > 0) { t--; o += 10; }
    }
    for (int32_t i = h; i > 0; i--) pickup_spawn(100161, b, 0);
    for (int32_t i = t; i > 0; i--) pickup_spawn(100160, b, 0);
    for (int32_t i = o; i > 0; i--) pickup_spawn(100159, b, 0);
    return r;
}

int32_t exp_share(void *loop, int32_t faction, int32_t exp)
{
    int32_t pm;
    switch (W->f2780) {
    case 10: pm = -80; break;
    case 20: pm = -50; break;
    case 40: pm = 50; break;
    case 50: pm = 100; break;
    default: pm = 0; break;
    }
    exp = mul(exp, pm) / 1000 + exp;
    int32_t ret = 0;
    uint16_t nrooms = ld16(loop, 4);
    for (uint16_t k = 0; k < nrooms; k++) {
        uint8_t *room = (uint8_t *)ldp(loop, 0) + 0x4120u * k;
        if (faction == ld32(room, 0x4018)) continue;
        int32_t lead = 0;
        for (int s = 0; s < 8; s++) {
            uint8_t *sl = ldp(room, 0x4030 + 4u * (uint32_t)s);
            if (ld32(sl, 0x9c4) != 1 || ld32(sl, 0x9f0) == 100000104) continue;
            const wl_char *ch = ldp(sl, 0x9f4);
            if (ch->base.alive != 1 || ch->base.body->active != 1 || ld16(ch->party, 0x44a) != 0) continue;
            uint8_t *st = sl + 0xa10;
            int32_t got = 0;
            if (ld32(st, 0xec) != 0 && ld32(st, 0xdc) > ld32(st, 0xd8)) {
                int32_t lv = ld32(st, 0xd8), cut;
                if (lv < 0) cut = mul(exp, 500) / 1000;
                else if (lv <= 2) cut = 0;
                else if (lv == 3) cut = mul(exp, 250) / 1000;
                else cut = mul(exp, 500) / 1000;
                got = exp - cut;
                st32(st, 0xe8, ld32(st, 0xe8) + got);
                int first = 1, up = 0;
                for (;;) {
                    int32_t need = ld32(st, 0xec);
                    if (need == 0 || ld32(st, 0xdc) <= ld32(st, 0xd8)) { st32(st, 0xe8, 0); break; }
                    if (ld32(st, 0xe8) < need) break;
                    const uint8_t *dr = dramatist_row(ld32(sl, 0x9f0));
                    int32_t code = dr != NULL ? ld32(dr, 4) : ld32(sl, 4);
                    const uint8_t *row = stat_row_find(code, ld32(st, 0xe0) + ld32(st, 0xd8) + 1);
                    if (first) {
                        if (ld32(sl, 8) == 0) { st32(sl, 8, 1); memcpy(sl + 0xc, st, 0x4dc); }
                        g_eng.h55fdb0(sl);
                        first = 0;
                    }
                    st32(st, 0xd8, ld32(st, 0xd8) + 1);
                    st32(st, 0xe8, ld32(st, 0xe8) - ld32(st, 0xec));
                    if (row != NULL) g_eng.h409150(sl, (intptr_t)row);
                    up = 1;
                }
                if (up) memcpy(sl + 0x4e8, st, 0x4dc);
            }
            if (sl == (uint8_t *)ldp(room, 0x200c)) lead = got;
        }
        uint32_t tot = (uint32_t)ld32(room, 0x40b8) + (uint32_t)exp;
        st32(room, 0x40b8, (int32_t)(tot > 0x7fffffffu ? 0x7fffffffu : tot));
        uint32_t kills = (uint32_t)ld32(room, 0x40b4) + 1;
        st32(room, 0x40b4, (int32_t)(kills > 0x7fffffffu ? 0x7fffffffu : kills));
        ret = lead;
    }
    return ret;
}

void float_text(wl_char *c, const char *fmt, int32_t v, wl_body *b, int32_t a5, int32_t x, int32_t y, int32_t facing,
                uint16_t color, int32_t kind, uint16_t life, int32_t vy)
{
    char text[40];
    snprintf(text, sizeof text, fmt, v);
    wl_entity *p = particle_spawn(&c->base, &c->base, 100042, b, x, y, facing, 0, a5, 0, 0, 0, 0, 20);
    if (p == NULL) return;
    wl_body *pb = p->body;
    dmg_glyphs(text, (uint16_t *)((uint8_t *)p + 0x200), (uint16_t *)((uint8_t *)p + 0x250));
    st32(p, 0x1fc, kind);
    st16(p, 0x1f8, color);
    st16(p, 0x258, life);
    pb->vvel = vy;
}

void char_die(wl_char *c, int32_t release, int32_t reward, int32_t dir)
{
    wl_body *b = c->base.body;
    if (ld32(c, 0x15904) != 0 && release != 0) {
        /* a vanishing foe: its holds let go, its parts / effects released, the voice set deleted */
        if (c->carrier != NULL) char_carry(c->carrier, NULL);
        if (c->held != NULL) char_carry(c, NULL);
        for (int k = 0; k < 2; k++) {
            wl_entity **p = (wl_entity **)((uint8_t *)c + 0xc824 + 4 * k);
            if (*p != NULL) { entity_release(*p, 1); *p = NULL; }
        }
        for (uint16_t j = 0; j < 10; j++) char_part_release(c, j);
        entity_release(&c->base, 0);
        if (c->voice != NULL) {
            wl_voiceset *v = c->voice;
            voice_set_free(v);
            g_eng.op_delete(v);
            c->voice = NULL;
        }
        return;
    }
    /* status 1 (fallen): its length from row 1, two rand draws; the other statuses end */
    uint8_t *pt = (uint8_t *)c->party;
    uint16_t prior = ld16(pt, 0x44a);
    const uint8_t *row = D(0x628794);
    int32_t base = ld16(row, 0x64);
    if (ld32(c, 0x15a94) == 0) base += ld32(row, 0x68);
    int32_t len = mul(ld16(row, 0x66), rnd()) / 0x8000 + base;
    if (len < 0) len = 0;
    else len = mul(ld16(row, 0x66), rnd()) / 0x8000 + base;
    st16(pt, 0x46e, (uint16_t)len);
    st16(pt, 0x44a, ld16(pt, 0x46e));
    st32(pt, 0x494, 0);
    char fmt[64];
    int msg = 0;
    if (prior == 0) { strcpy(fmt, D(0x8553d4)); msg = 1; }   /* "%s has fallen..." */
    for (uint16_t k = 0; k < 18; k++) if (k != 1) st16(pt, 0x448 + 2u * k, 0);
    if (msg) status_log(c, fmt);
    if (release == 0) {
        b->facing = dir != 3 ? 3 : 1;
        body_action_start(b, (int16_t)ld16(c, 0x15a0a), 800);
    }
    if (reward == 0 || W->f27b4 == 2) return;
    if (c->base.faction != 1) foe_drops(c, ld32(pt, 0xf4), ld32(pt, 0xf8), ld32(pt, 0xfc));
    int32_t exp = exp_share(W->f1044, c->base.faction, ld32(pt, 0xf0));
    if (exp <= 0) return;
    int32_t y = b->h + b->y;
    int32_t x = mul(3200, rnd()) / 0x8000 + b->w + b->x - 1600;
    float_text(c, D(0x8a46f8), exp, b, 0, x, y, b->facing, 8, 14, 80, -64000);   /* "Exp +%d" */
}

