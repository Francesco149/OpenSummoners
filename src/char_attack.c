/* char_attack.c — the attack pipeline.  See char_attack.h.  Transcribed from the asm. */
#include "char_attack.h"

#include <stddef.h>
#include <string.h>

#include "arche_pose.h"    /* switch_touch 0x5b1aa0 */
#include "char_ai.h"       /* body_frame_box 0x4411e0 */
#include "char_def.h"      /* stat_jitter 0x426e00 */
#include "char_status.h"   /* entity_sprite_desc 0x44d160, entity_part_descs 0x44d020 */
#include "engine_calls.h"
#include "entity_life.h"   /* entity_carry_riders 0x5563d0 */
#include "entity_move.h"   /* entity_move_horizontal 0x54db10, faction_relation 0x4416c0 */
#include "gimmick.h"       /* entity_rect_hit 0x4804b0, world_next_serial 0x40be00 */
#include "camera_follow.h" /* view_box_visible 0x4766a0 */
#include "char_physics.h"  /* char_part_top 0x4385e0 */
#include "hud_view.h"      /* foe_gauge_set 0x5b5600 */
#include "npc_ai.h"        /* part_bottom 0x4022a0 */
#include "sound_fx.h"      /* vpool_take 0x40b710, sfx_place 0x40bb90 */
#include "particle_init.h" /* polar_milli 0x55cdb0 */
#include "particle_spawn.h"
#include "rng.h"

#define IP(p) ((intptr_t)(p))
#define W (*g_eng.world)

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t iabs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
static const uint8_t *party(const wl_char *c) { return (const uint8_t *)c->party; }

int32_t facing_back(int32_t f) { return f != 3 ? 3 : 1; }

int32_t char_mag_sum(const wl_char *c)
{
    return ld32(party(c), 0x6c) + ld32(party(c), 0x7c) + ld32(party(c), 0x94) + ld32(c, 0xc7e4);
}

/* w x (power + stats / 2) / 1000, then v - s + r x s / 1000 with s = min(100, min(v / 2, max(2, v / 5))) */
static int32_t roll(int32_t w, int32_t r, int32_t stats, int32_t power)
{
    int32_t v = mul(w, power + stats / 2) / 1000;
    int32_t s = mul(20, v) / 100;
    if (s < 2) s = 2;
    int32_t h = mul(50, v) / 100;
    if (h < s) s = h;
    if (s > 100) s = 100;
    return mul(s, r) / 1000 - s + v;
}

static int32_t atk_stats(const wl_char *c, int32_t alt)
{
    if (alt) return ld32(party(c), 0x8c) + ld32(party(c), 0x64);
    return ld32(c, 0xc7d8) + ld32(party(c), 0x8c) + ld32(party(c), 0x74) + ld32(party(c), 0x64);
}

int32_t char_atk_roll(const wl_char *c, int32_t w, int32_t r, int32_t alt, int32_t power)
{
    return roll(w, r, atk_stats(c, alt), power);
}

int32_t char_mag_roll(const wl_char *c, int32_t w, int32_t r, int32_t power)
{
    return roll(w, r, char_mag_sum(c), power);
}

int32_t char_skill_roll(const wl_char *c, const uint8_t *skill, int32_t a3, int32_t r)
{
    int32_t stats = ld32(skill, 0x2dc) != 0 ? char_mag_sum(c) : atk_stats(c, ld32(skill, 0xb0) == 6);
    return roll(mul(a3, ld32(skill, 0x2c4)) / 1000, r, stats, ld32(skill, 0x2c8));
}

int char_lunge(wl_char *c, wl_body *b, int32_t d, int32_t keep, int32_t push)
{
    int32_t v = b->facing == 3 ? -d : d;
    if (push) st32(b, 0x78, ld32(b, 0x78) - v);
    if (entity_move_horizontal(&c->base, b, v, 1, 1, 0, 0, 0) == 0) return 0;
    if (keep == 0) st32(c, 0x14834, ld32(c, 0x14834) - d);
    return 1;
}

int char_attack_hits(wl_char *c, wl_body *b, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t mode,
                     int32_t nohit, const uint8_t *skill, int32_t atk, int32_t a11)
{
    int32_t f10 = ld32(skill, 0x2e4);
    int32_t r = mul((int32_t)rng_rand(), 1000) / 0x8000;
    int32_t pw;
    if (ld32(skill, 0x2dc) != 0)
        pw = stat_jitter(ld32(skill, 0x2c8) + char_mag_sum(c) / 2, mul(atk, ld32(skill, 0x2c4)) / 1000, r);
    else
        pw = char_atk_roll(c, mul(atk, ld32(skill, 0x2c4)) / 1000, r, ld32(skill, 0xb0) == 6, ld32(skill, 0x2c8));
    /* the element / its fields: the skill's, or (a plain physical one) the weapon's from the party block */
    int32_t elem = ld32(skill, 0xb0);
    int32_t e2 = char_mag_sum(c) + ld32(skill, 0xb4), e3 = 0, e4 = 0, e5 = 0;
    if (ld32(skill, 0x2e8) == 0 && ld32(skill, 0x2dc) == 0 && elem == 0) {
        elem = ld32(party(c), 0x124);
        e2 = ld32(party(c), 0x128);
        e3 = ld32(party(c), 0x12c);
        e4 = ld32(party(c), 0x130);
        e5 = ld32(party(c), 0x134);
    }
    int32_t res = body_hit_spawn(c, c, b, dx, dy, w, h, mode, nohit, f10, elem, e2, 0, a11, pw, 1,
                                 ld32(skill, 0x2e0), ld32(b, 0x60), 0, ld32(skill, 0x2f4),
                                 ld32(party(c), 0xd8) + ld32(party(c), 0xe0) + ld32(party(c), 0xe4), e3, e4, e5);
    if (ld32(c, 0xc7f4) == 0) return res;
    for (int side = 0; side < 2; side++) {
        int32_t off = side == 0 ? ld32(c, 0xc7f8) : -ld32(c, 0xc7f8);
        body_hit_spawn(c, c, b, dx + off, dy, w, h, mode, nohit, f10, elem, e2, 0, a11, pw, 1, ld32(skill, 0x2e0),
                       ld32(b, 0x60), 0, ld32(skill, 0x2f4),
                       ld32(party(c), 0xe4) + ld32(party(c), 0xe0) + ld32(party(c), 0xd8), e3, e4, e5);
    }
    return res;
}

int body_hit_spawn(wl_char *self, wl_char *atk, wl_body *b, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t mode,
                   int32_t nohit, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15,
                   int32_t a16, int32_t a17, int32_t a18, int32_t a19, int32_t a20, int32_t a21, int32_t a22,
                   int32_t a23, int32_t a24)
{
    uint8_t *bb = (uint8_t *)b;
    uint16_t n = ld16(bb, 0x80);
    if (n >= 8) return 0;
    bb[0x7f] = 1;
    uint8_t *rec = bb + 0x84 + 64u * n;
    st16(bb, 0x80, (uint16_t)(n + 1));
    st32(rec, 0x24, a11);
    st32(rec, 0x28, a12);
    st32(rec, 0x2c, a13);
    st32(rec, 0x30, a14);
    st32(rec, 0x3c, a18);
    st32(rec, 0x34, a15);
    st32(rec, 0x38, a19);
    st32(rec, 0x14, b->facing == 3 ? b->x - dx : b->w + b->x - w + dx);
    st32(rec, 0x1c, w);
    st32(rec, 0x20, h);
    st32(rec, 0x18, b->y + dy);
    st32(rec, 0x10, a17);
    st32(rec, 0xc, a10);
    st32(rec, 4, 99);
    st32(rec, 8, 99);
    /* the knock-back direction (+4: the facing, its back, 1 / 3; 99 none) and kind (+8) by `mode` */
    switch (mode) {
    case 0: st32(rec, 4, b->facing); break;
    case 1: st32(rec, 4, facing_back(b->facing)); break;
    case 2: st32(rec, 8, 0); st32(rec, 4, b->facing); break;
    case 3: st32(rec, 4, facing_back(b->facing)); st32(rec, 8, 0); break;
    case 4: st32(rec, 4, b->facing); st32(rec, 8, 2); break;
    case 5: st32(rec, 4, facing_back(b->facing)); st32(rec, 8, 2); break;
    case 6: st32(rec, 4, 99); st32(rec, 8, 0); break;
    case 7:   /* by the body's angle +0x34 (milli-degrees), in 45-degree sectors */
        switch ((ld32(bb, 0x34) / 1000 + 382) / 45 % 8) {
        case 0: st32(rec, 8, 0); break;
        case 1: st32(rec, 4, 1); st32(rec, 8, 0); break;
        case 2: st32(rec, 4, 1); break;
        case 3: st32(rec, 4, 1); st32(rec, 8, 2); break;
        case 4: st32(rec, 8, 2); break;
        case 5: st32(rec, 4, 3); st32(rec, 8, 2); break;
        case 6: st32(rec, 4, 3); break;
        case 7: st32(rec, 4, 3); st32(rec, 8, 0); break;
        default: break;
        }
        break;
    case 9: st32(rec, 8, 0); break;
    case 10: st32(rec, 8, 2); break;
    case 11: st32(rec, 4, 3); break;
    case 12: st32(rec, 4, 1); break;
    default: break;
    }
    if (nohit != 0) return 0;
    if (a19 == 1) hit_breaks_shots(self, (const int32_t *)(rec + 0x14));
    int32_t part = 0;
    wl_char *t = char_hit_target(self, atk, 0, rec, a16, 1, &part);
    if (t == NULL) {
        if (W->f27b4 == 0 && ld32(self, 0x1dc) == 1 && switch_touch((const int32_t *)(rec + 0x14), 0, a20) != 0) return 1;
        return 0;
    }
    if (mode >= 8 && mode <= 10) {
        const wl_body *tb = t->base.body;
        st32(rec, 4, tb->x + tb->w / 2 >= ld32(rec, 0x14) + ld32(rec, 0x1c) / 2 ? 1 : 3);
    }
    if (a22 != 0)
        for (uint16_t k = 0, cnt = ld16(t, 0x158d0); k < cnt; k++)
            if (ld32(t, 0x158d4 + 4u * k) == a22) {
                st32(rec, 0x34, a23 + mul(a24, ld32(rec, 0x34)) / 1000);
                break;
            }
    /* the struck char's hit fields */
    st32(t, 0x15ab8, ld32(rec, 8));
    st32(t, 0x15abc, ld32(rec, 0x14) + ld32(rec, 0x1c) / 2);
    st32(t, 0x15ac4, ld32(rec, 0x24));
    st32(t, 0x15ac8, ld32(rec, 0x28));
    st32(t, 0x15acc, ld32(rec, 0x2c));
    stp(t, 0x15aac, self);
    st32(t, 0x15ad4, ld32(rec, 0x3c));
    st32(t, 0x15ad0, ld32(rec, 0x30));
    st32(t, 0x15adc, ld32(rec, 0xc));
    stp(t, 0x15aa8, atk);
    st32(t, 0x15ad8, ld32(rec, 0x34));
    st32(t, 0x15ae4, a21);
    st16(t, 0x26e, 1);
    st32(t, 0x15ab0, 1);
    st32(t, 0x15ab4, ld32(rec, 4));
    st32(t, 0x15ac0, ld32(rec, 0x18) + ld32(rec, 0x20) / 2);
    st32(t, 0x15ae0, ld32(rec, 0x10));
    st32(t, 0x15ae8, part);
    st16(bb, 0x5a, (uint16_t)(ld16(bb, 0x5a) + 1));
    if (atk == NULL || a16 == 0) return 1;
    if (ld32(atk, 0x200) == 0) stp(atk, 0x1427c, t);
    foe_gauge_set(atk, t, 0);
    return 1;
}

void shot_end(wl_entity *s, uint8_t *flag, int32_t keep)
{
    if (keep != 0) { *flag = 0; return; }
    if (ld32(s, 0x254) != 0 && s->snd != NULL)
        for (uint16_t i = 0; i < s->nsnd; i++) {
            if (s->snd == NULL) continue;
            wl_sndch *ch = &s->snd[i];      /* (retail logs "ReserveWave over index." past nsnd: unreachable) */
            if (ch->voice == NULL) continue;
            ch->voice->owner = 0;
            ch->voice = NULL;
            ch->pool = NULL;
            ch->f20 = 0;
        }
    entity_release(s, 0);
}

int hit_breaks_shots(wl_char *self, const int32_t *r)
{
    int hit = 0;
    for (int k = 0; k < 64; k++) {
        wl_entity *s = W->band64[k];
        if (s->alive == 0 || ld32(s, 0x230) == 0) continue;
        wl_body *sb = s->body;
        int32_t fs = s->faction, fc = self->base.faction;
        if (!(fs != 0 && fc != 0 && (fs == 3 || fc == 3 || fs != fc)) && self->base.coll_off == 0) continue;
        if (sb->x > r[0] + r[2] || sb->w + sb->x <= r[0]) continue;
        if (char_part_top((const wl_char *)s, 0) > r[1] + r[3] || part_bottom(s, 0) <= r[1]) continue;
        int32_t freq = 500 - mul(1000, (int32_t)rng_rand()) / 0x8000;
        wl_body *pb = s->body;
        wl_voice *v = vpool_take(*(wl_vpool **)((uint8_t *)g_eng.sys + 0x568), 0);
        if (v != NULL) {
            void *buf = v->buf;
            int32_t ov, op;
            if (sfx_place(s, (**g_eng.keycfg)->sfx_volume, 0, 0, pb, &ov, &op) != 0) {
                g_eng.dsb_volume_5bb870(buf, ov);
                g_eng.dsb_pan_5bb880(buf, op);
                g_eng.dsb_freq_5bb890(buf, freq);
                g_eng.dsb_play_5bb8c0(buf);
            }
        }
        for (int i = 0; i < 8; i++) {
            int32_t a = mul(4, (int32_t)rng_rand()) / 0x8000;
            int32_t dy = mul(800, (int32_t)rng_rand()) / 0x8000 - 400;
            int32_t dx = mul(800, (int32_t)rng_rand()) / 0x8000 - 400;
            if (view_box_visible(W->view, sb->x, sb->y, sb->w, sb->h, 16000))
                particle_spawn(s, s, 100074, sb, dx, dy, 3, 1, 0, a, 2, 0, 0, 0);
        }
        shot_end(s, (uint8_t *)sb, 0);
        hit = 1;
    }
    return hit;
}

wl_char *char_hit_target(const wl_char *self, const wl_char *skip, uint16_t part, const uint8_t *rec, int32_t rel,
                         int32_t nearest, int32_t *part_out)
{
    wl_char *best = NULL;
    int32_t best_d = 0;
    const int32_t *r = (const int32_t *)(rec + 0x14);   /* {x, y, w, h} */
    for (uint16_t k = 0; k < 32; k++) {
        wl_char *t = W->chars[k];
        if (ld32(t, 0x1d0) != 1) continue;
        const uint8_t *tb = (const uint8_t *)t->base.body;
        if (tb[0] != 1 || ld16(party(t), 0x44a) != 0 || ld16(t, 0x26e) != 0 || tb[0x7d] != 0 || t == skip) continue;
        int32_t rl = faction_relation(ld32(self, 0x1dc), ld32(t, 0x1dc));   /* (inline) */
        if (ld32(self, 0x1f0) != 0 && (rl == 1 || rl == 2)) rl = 1;
        if (rl != rel) continue;
        int32_t id = ld32(rec, 0x3c), recent = 0;
        if (id != 0)
            for (uint16_t i = 0; i < 8; i++)
                if (ld32(t, 0x15aec + 8u * i) == id && (int32_t)ld16(t, 0x15af0 + 8u * i) > part + 30) recent = 1;
        if (recent) continue;
        if (ldp(t, 0) == NULL) {
            if (!entity_rect_hit(&t->base, r, part)) continue;
        } else {
            uint16_t i, np = ld16(t, 4);
            for (i = 0; i < np; i++) {
                int32_t box[4];
                body_frame_box(i, (const wl_body *)(tb + 660u * part), box);
                if (box[0] <= ld32(rec, 0x14) + ld32(rec, 0x1c) && box[0] + box[2] > ld32(rec, 0x14) &&
                    box[1] <= ld32(rec, 0x18) + ld32(rec, 0x20) && box[1] + box[3] > ld32(rec, 0x18))
                    break;
            }
            if (i >= np) continue;
            if (part_out != NULL) *part_out = i;
        }
        if (!nearest) return t;
        uint16_t p = part < ld16(t, 0x44) ? part : 0;
        const uint8_t *bp = tb + 660u * p;
        int32_t d = iabs((ld32(bp, 0x10) - ld32(bp, 0x14)) / 2 - ld32(rec, 0x20) / 2 - ld32(rec, 0x18) + ld32(bp, 8) +
                         ld32(bp, 0x14)) +
                    iabs(ld32(bp, 0xc) / 2 - ld32(rec, 0x1c) / 2 + ld32(bp, 4) - ld32(rec, 0x14));
        if (best == NULL || d < best_d) {
            best = t;
            best_d = d;
        }
    }
    return nearest ? best : NULL;
}

/* ---- the skills' helpers: angles, orbits, poses, the slash sequence, the trail, the crate push ---- */

int32_t angle_wrap(int32_t a, int32_t d, int32_t n)
{
    int32_t s = a + d, m = 360 * n;
    if (s >= 0) return s - mul(m, s / m);
    return s - mul(m, s / m - 1);   /* (an exact negative multiple wraps to m, not 0) */
}

wl_entity *char_particle(wl_char *c, int32_t code, const wl_body *b, int32_t dx, int32_t dy, int32_t a6, int32_t a7,
                         int32_t a8, int32_t unused, int32_t a10, int32_t a11, int32_t twins, int32_t a13)
{
    (void)unused;
    wl_entity *r = particle_spawn(&c->base, &c->base, code, b, dx, dy, a6, a7, a8, 0, a10, a11, 0, a13);
    if (twins == 0 || ld32(c, 0xc7f4) == 0) return r;
    particle_spawn(&c->base, &c->base, code, b, dx + ld32(c, 0xc7f8), dy, a6, a7, a8, 0, a10, a11, 0, a13);
    return particle_spawn(&c->base, &c->base, code, b, dx - ld32(c, 0xc7f8), dy, a6, a7, a8, 0, a10, a11, 0, a13);
}

/* the movers' `pass` argument: the body's byte +0x20 loaded into cx over the entity's speed (+0x1c4) */
static int32_t mover_pass(const wl_entity *e, const uint8_t *bp)
{
    return (int32_t)(((uint32_t)ld32(e, 0x1c4) & 0xffff0000u) | bp[0x20]);
}

int entity_slide_step(wl_entity *e, int32_t dir, uint16_t part, int32_t *dx_out)
{
    wl_body *bp = (wl_body *)((uint8_t *)e->body + 660u * part);
    int32_t x0 = bp->x, y0 = bp->y;
    if (ld32(e, 0x1c4) == 0) return 0;
    int32_t y = bp->y;
    int stuck = entity_move_vertical(e, bp, 1, mover_pass(e, (const uint8_t *)bp), part, &y,
                                     (wl_entity **)((uint8_t *)bp + 0x284)) != 0;
    if (ldp(bp, 0x284) != NULL) bp->y = y;
    if (stuck) return 0;
    int32_t lo = ld32(e, 0x1c8), hi = ld32(e, 0x1cc);
    if ((lo == hi || ((lo < bp->x || dir >= 0) && (hi > bp->x || dir <= 0))) && dir != 0)
        entity_move_horizontal(e, bp, mul(dir, ld32(e, 0x1c4)), 1, 1, 0, part, 0);
    int32_t dx = bp->x - x0, dy = bp->y - y0;
    *dx_out = dx;
    entity_carry_riders(e, dx, dy, part);
    st32(e, 0x3c, dx);
    return iabs(dx) > 0 || iabs(dy) > 0;
}

int32_t *orbit_init(int32_t *o, int32_t x, int32_t y, int32_t r, int32_t ang, int32_t dang, int16_t delay, int16_t limit,
                    int32_t skip)
{
    if (skip != 0) return (int32_t *)(intptr_t)skip;
    o[0] = x;
    o[1] = y;
    o[2] = r;
    o[3] = ang;
    o[4] = dang;
    st16(o, 0x18, 0);
    st16(o, 0x1a, (uint16_t)delay);
    st16(o, 0x1c, (uint16_t)limit);
    return o;
}

int32_t orbit_step(wl_char *c, const wl_body *b, int32_t *o, int32_t accel, int32_t n, int32_t hold, int32_t code,
                   int32_t after)
{
    if (hold != 0) return hold;
    uint16_t t = ld16(o, 0x18);
    if (t < ld16(o, 0x1a)) {   /* the start delay */
        st16(o, 0x18, (uint16_t)(t + 1));
        return t + 1;
    }
    int frozen = 0;
    if (t >= ld16(o, 0x1c)) {   /* past the limit: only the particles (`after`) */
        if (after == 0) return after;
        frozen = 1;
    }
    if ((uint16_t)n == 0) return n;
    for (uint16_t i = (uint16_t)n; i != 0; i--) {
        if (!frozen) {
            int32_t d = o[4];
            st16(o, 0x18, (uint16_t)(ld16(o, 0x18) + 1));
            int32_t s = o[3] + d;
            o[3] = s >= 0 ? s % 360000 : s + 360000 * (1 - s / 360000);
            o[4] = accel + d;
            polar_milli(o[0], o[1], o[2], o[3], &o[0], &o[1]);
        }
        if (code != 0) {
            int32_t x = o[0], y = o[1], f = b->facing;
            particle_spawn(&c->base, &c->base, code, b, x, y, f, 1, 0, 0, 2, 1, 0, 0);
            if (ld32(c, 0xc7f4) != 0) {
                particle_spawn(&c->base, &c->base, code, b, x + ld32(c, 0xc7f8), y, f, 1, 0, 0, 2, 1, 0, 0);
                particle_spawn(&c->base, &c->base, code, b, x - ld32(c, 0xc7f8), y, f, 1, 0, 0, 2, 1, 0, 0);
            }
        }
    }
    return 0;   /* (retail's eax: the spent counter) */
}

void body_pose_set(wl_body *b, uint16_t pose, uint16_t prio, uint16_t a4, int32_t a5, int32_t a6)
{
    if (ld16(b, 0x56) / 100 > prio / 100 && ld16(b, 0x48) != 0) return;
    if (pose == ld16(b, 0x48)) return;
    st16(b, 0x48, pose);
    st16(b, 0x54, a4);
    st16(b, 0x56, prio);
    st16(b, 0x58, 0);
    st32(b, 0x5c, 0);
    st32(b, 0x78, 0);
    st16(b, 0x5a, 0);
    st16(b, 0x64, 0);
    st32(b, 0x4c, a6);
    st32(b, 0x50, a5);
    st32(b, 0x60, (int32_t)world_next_serial(W));
}

/* the clip record `clip` on b (+0x6c; a new one restarts +0x70 / +0x72 / +0x74) */
static void clip_to(uint8_t *b, intptr_t clip)
{
    if ((intptr_t)ld32(b, 0x6c) == clip) return;
    st32(b, 0x6c, (int32_t)clip);
    b[0x74] = 0;
    st16(b, 0x72, 0);
    st16(b, 0x70, 0);
}

int slash_step(wl_char *c, wl_body *b, int32_t keep, intptr_t clips, int32_t d1, int32_t d2, int32_t a7, int32_t a8)
{
    uint8_t *bb = (uint8_t *)b;
    int32_t d;
    switch (ld16(bb, 0x58)) {
    case 0:   /* the wind-up clip */
        clip_to(bb, clips);
        st16(bb, 0x58, 1);
        if (bb[0x74] == 0) return 1;
        goto stage1;
    case 1:
        if (bb[0x74] == 0) return 0;
    stage1:
        if (ld16((const void *)clips, 0x1d6) != 0) {   /* a second wind-up record */
            clip_to(bb, clips + 404);
            st16(bb, 0x58, 2);
            return 1;
        }
        __attribute__((fallthrough));   /* straight to the first lunge */
    case 2:
        if (bb[0x74] == 0) return 0;
        clip_to(bb, clips + 808);
        d = b->facing == 3 ? -d1 : d1;
        st32(bb, 0x78, ld32(bb, 0x78) - d);
        if (entity_move_horizontal(&c->base, b, d, 1, 1, 0, 0, 0) != 0 && keep == 0) st32(c, 0x14834, ld32(c, 0x14834) - d1);
        st16(bb, 0x58, 3);
        return 1;
    case 3:
        if (bb[0x74] == 0 && !(a7 == 1 && a8 == 1)) return 0;
        clip_to(bb, clips + 1212);
        d = b->facing == 3 ? -d2 : d2;
        st32(bb, 0x78, ld32(bb, 0x78) - d);
        if (entity_move_horizontal(&c->base, b, d, 1, 1, 0, 0, 0) != 0 && keep == 0) st32(c, 0x14834, ld32(c, 0x14834) - d2);
        st16(bb, 0x58, 4);
        return 1;
    case 4:
        if (bb[0x74] == 0) return 0;
        st16(bb, 0x48, 0);
        st16(bb, 0x56, 210);
        st32(bb, 0x5c, 0);
        st32(bb, 0x78, 0);
        st16(bb, 0x54, 0);
        st32(bb, 0x4c, 0);
        st32(bb, 0x50, 0);
        st16(bb, 0x5a, 0);
        st16(bb, 0x64, 0);
        st16(bb, 0x58, 5);
        return 1;
    default:
        return 0;
    }
}

intptr_t slash_trail(wl_entity *e, wl_body *b, int32_t quiet, int32_t a4, int32_t a5)
{
    es_desc d[32];
    uint16_t n = 1;
    uint32_t code = (uint32_t)e->code;
    intptr_t res;
    if (code <= 100168) {
        if ((code >= 100165 || code == 51027 || (code > 51251 && code <= 51253)) && b->pose == 3 && ld16(b, 0x58) == 2) {
            /* the bow's string: a second descriptor, its alpha by the draw time +0x5c */
            uint16_t slot = ld16(e, 0x48);
            d[1].frame = b->facing == 3 ? (uint16_t)(ld16(ldp(g_eng.sys, 0x18e0u + 4u * slot), 0) + 1) : 1;
            d[1].slot = slot;
            n = 2;
            d[1].x = 0;
            d[1].y = 0;
            int32_t k = mul(20, ld32(b, 0x5c)) / 16;
            if (k < 0) k = 0;
            d[1].alpha = k >= 20 ? 0 : ld32(g_eng.sys, 0x27a8u + 4u * (uint32_t)k);
        }
        res = entity_sprite_desc(e, b, d);
    } else if (code >= 100179 && code <= 100180) {
        res = entity_part_descs(e, b, d, &n);
    } else {
        res = entity_sprite_desc(e, b, d);
    }
    if (res == 0) return res;
    for (uint16_t i = 0; i < n; i++) {
        wl_entity *p = particle_spawn(e, e, 100082, b, b->x + mul(100, d[i].x + ld32(b, 0x40)),
                                      b->y + mul(100, d[i].y + ld32(b, 0x44)), 99, 0, quiet, 0, 0, 0, 0, 10);
        res = (intptr_t)p;
        if (p == NULL) continue;
        st16(p, 0x48, d[i].slot);
        st16(p, 0x4a, d[i].frame);
        ((uint8_t *)p)[0x4c] = 1;
        st32(p, 0x54, 0);
        st32(p, 0x50, 0);
        st32(p, 0x58, 0);
        st32(p, 0x25c, a4);
        if (ld32(p, 0x1d4) == 100044) {
            int32_t v = ld32(g_eng.sys, 0x2758u + 4u * (uint32_t)a4);
            st32(p, 0xf4, v);
            st32(p, 0xf8, v);
        }
        st32(p, 0x260, a5);
    }
    return res;
}

int crate_push(wl_entity *e, const int32_t *cmd, wl_body *b, int32_t armed, uint16_t part, int32_t dir_in, int32_t probe)
{
    if (dir_in != 0 || armed == 0 || ld32(b, 0x28) > ld32(e, 0x1596c)) return 0;
    int32_t dir, ax;
    switch (cmd[0]) {
    case 1: case 5:   /* pushing left */
        if (b->facing != 3) return 0;
        dir = -1;
        ax = b->x - 400;
        break;
    case 2: case 6:   /* pushing right */
        if (b->facing != 1) return 0;
        dir = 1;
        ax = b->w + b->x + 400;
        break;
    default:
        return 0;
    }
    int32_t px = mul(100, ax / 100);
    int32_t bot = b->h + b->y - 1, top = ld32(b, 0x14) + b->y + 400;
    /* the last pushable (collide mode 3, kind 3) in W's collide list with its box across the probe */
    wl_entity *crate = NULL;
    const wl_list *l = W->collide;
    for (uint16_t i = 0, cnt = l->count; i < cnt; i++) {
        wl_entity *o = l->items[i];
        if (o == e || o->coll_mode != 3 || o->coll_kind != 3) continue;
        uint16_t p = part < ld16(o, 0x44) ? part : 0;
        const uint8_t *ob = (const uint8_t *)o->body + 660u * p;
        if (top < ld32(ob, 0x10) + ld32(ob, 8) - 1 && bot > ld32(ob, 8) + ld32(ob, 0x14) &&
            px <= ld32(ob, 0xc) + ld32(ob, 4) && px >= ld32(ob, 4))
            crate = o;
    }
    if (crate == NULL) return 0;
    if (probe != 0) return 1;
    wl_body *cb = (wl_body *)((uint8_t *)crate->body + 660u * part);
    int32_t x0 = cb->x, y0 = cb->y;
    if (ld32(crate, 0x1c4) == 0) return 1;
    int32_t y = cb->y;
    int stuck = entity_move_vertical(crate, cb, 1, mover_pass(crate, (const uint8_t *)cb), part, &y,
                                     (wl_entity **)((uint8_t *)cb + 0x284)) != 0;
    if (ldp(cb, 0x284) != NULL) cb->y = y;
    if (stuck) return 1;
    int32_t lo = ld32(crate, 0x1c8), hi = ld32(crate, 0x1cc);
    if ((lo == hi || ((lo < cb->x || dir >= 0) && (hi > cb->x || dir <= 0))) && dir != 0)
        entity_move_horizontal(crate, cb, mul(dir, ld32(crate, 0x1c4)), 1, 1, 0, part, 0);
    int32_t dx = cb->x - x0, dy = cb->y - y0;
    entity_carry_riders(crate, dx, dy, part);
    st32(crate, 0x3c, dx);
    if (iabs(dx) > 0 || iabs(dy) > 0) entity_move_horizontal(e, b, dx, 1, 1, 0, part, 0);
    return 1;
}

int clip_seq_step(wl_body *b, intptr_t clips, uint16_t n, uint16_t base)
{
    uint16_t ph = b->f58;
    if (ph == base) {
        if ((intptr_t)b->clip != clips) {
            b->clip = (const void *)clips;
            b->clip_done = 0; b->clip_frame = 0; b->clip_timer = 0;
        }
    } else {
        if (b->clip_done == 0) return 0;
        uint16_t k = (uint16_t)(ph - base);
        if (k < n) {
            intptr_t cl = clips + 404 * (intptr_t)k;
            if ((intptr_t)b->clip != cl) {
                b->clip = (const void *)cl;
                b->clip_done = 0; b->clip_frame = 0; b->clip_timer = 0;
            }
        }
    }
    b->f58 = (uint16_t)(ph + 1);
    return 1;
}

/* the body's action restarted as `pose` / `sub` with its skill record (none when 0): a reset first */
static void pose_restart(wl_char *c, wl_body *b, uint16_t pose, uint16_t sub)
{
    b->pose = 0; b->act_len = 210; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0;
    b->act_arg = 0; b->spell = 0; b->target = 0; b->act_5a = 0; b->act_64 = 0;
    const uint8_t *rec = NULL;
    for (uint16_t i = 0, n = ld16(c, 0xc754); i < n; i++)
        if (ld32(c, 0x754 + 0x300u * i) == (int32_t)pose) { rec = (const uint8_t *)c + 0x754 + 0x300u * i; break; }
    if (pose == 0) return;
    b->pose = pose; b->act_len = 210; b->f58 = 0; b->act_ctr = 0; b->act_78 = 0; b->act_5a = 0; b->act_64 = 0;
    b->act_arg = sub;
    b->spell = (int32_t)(intptr_t)rec;
    b->target = 0;
    b->serial = (int32_t)world_next_serial(W);
}

int body_pose_swap(wl_char *c, wl_body *b, int32_t flag, uint16_t pa, uint16_t sa, uint16_t pb, uint16_t sb,
                   int32_t gate)
{
    int32_t f = flag;
    if (gate != 0) {
        f = b->c_flag != 0 ? 1 : flag;
        if (ld16(c, 0x158bc) < ld16(c, 0xc7e8)) f = 1;
    }
    if (pa == b->pose && (b->act_arg == sa || sa == 0) && f == 0) { pose_restart(c, b, pb, sb); return 1; }
    if (pb == b->pose && b->act_arg == sb && f == 1) { pose_restart(c, b, pa, sa); return 1; }
    return 0;
}

void *char_shot_fire(wl_char *c, int32_t code, wl_body *b, int32_t dx, int32_t dy, int32_t anchor, int32_t a6,
                     int16_t a7, int32_t a8, const uint8_t *skill, int32_t w, int32_t a11, int32_t fresh, int32_t a13)
{
    b->f7f = 1;
    int32_t serial = b->serial;
    int32_t r = mul(1000, (int32_t)rng_rand()) / 0x8000;
    int32_t pw;
    if (ld32(skill, 0x2dc) != 0)
        pw = stat_jitter(ld32(skill, 0x2c8) + char_mag_sum(c) / 2, mul(ld32(skill, 0x2c4), w) / 1000, r);
    else
        pw = char_atk_roll(c, mul(ld32(skill, 0x2c4), w) / 1000, r, ld32(skill, 0xb0) == 6, ld32(skill, 0x2c8));
    if (fresh != 0) serial = (int32_t)world_next_serial(W);
    int32_t mag = 0;
    if (ld32(skill, 0xb0) == 7) {
        int32_t r2 = mul(1000, (int32_t)rng_rand()) / 0x8000;
        mag = char_mag_roll(c, ld32(skill, 0x2cc), r2, ld32(skill, 0x2d0));
    }
    const uint8_t *p = party(c);
    return shot_spawn(c, (int32_t)(intptr_t)c, c->base.faction, code, b, dx, dy, anchor, a6, a7, pw, a8, serial, a11,
                      ld32(skill, 0xb0), char_mag_sum(c) + ld32(skill, 0xb4), mag, c->base.coll_off,
                      ld32(p, 0xd8) + ld32(p, 0xe0) + ld32(p, 0xe4), a13);
}

void particle_tint_set(wl_entity *e, int32_t i)
{
    st32(e, 0x25c, i);
    if (e->code == 100044) {
        int32_t t = ld32(g_eng.data(0x8a92b8), 4u * (uint32_t)i);
        st32(e, 0xf4, t);
        st32(e, 0xf8, t);
    }
}

int entity_sprite_descs(const wl_entity *e, const wl_body *b, es_desc *out, uint16_t *n)
{
    *n = 1;
    uint32_t code = (uint32_t)e->code;
    if (code > 0x18748u) {
        if (code < 0x18753u || code > 0x18754u) return entity_sprite_desc(e, b, out);
        const uint8_t *t = e->f08;
        if (t == NULL) return 0;
        uint16_t idx = 0;
        if (b->clip != NULL) {
            const uint8_t *cl = b->clip;
            idx = (uint16_t)(ld16(cl, 0) + ld16(cl, 2 + 2u * b->clip_frame));
            if (b->facing == 3) idx = (uint16_t)(idx + ld16(t, 0x18));
        }
        const uint8_t *base = (const uint8_t *)ldp(t, 0) + 52u * (uint32_t)mul((int32_t)ld32(t, 0x1c), idx);
        uint16_t k = 0;
        for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(e->f08, 0x1c); i++) {
            const uint8_t *r = base + 52u * i;
            if (ld32(r, 0x14) != 0 || ld32(r, 0) == 0) continue;
            es_desc *d = &out[k++];
            d->slot = ld16(r, 0x18);
            d->frame = ld16(r, 0x1c);
            d->alpha = 0;
            d->x = ld32(r, 4) + ld32(e, 0x28) / 100 / 2;
            d->y = ld32(r, 8) + ld32(e, 0x2c) / 100 / 2;
            d->f0c = ld32(r, 0);
        }
        *n = k;
        return k != 0;
    }
    if ((code < 0x18745u && code != 0xc753u && (code <= 0xc833u || code > 0xc835u)) || b->pose != 3 || b->f58 != 2)
        return entity_sprite_desc(e, b, out);
    uint16_t slot = ld16(e, 0x48);
    uint16_t fr = 1;
    if (b->facing == 3) fr = (uint16_t)(ld16(ldp(g_eng.sys, 0x18e0 + 4u * slot), 0) + 1);
    *n = 2;
    out[1].x = 0;
    out[1].y = 0;
    out[1].frame = fr;
    out[1].slot = slot;
    int32_t a = mul(20, b->act_ctr) / 16;
    if (a < 0) a = 0;
    out[1].alpha = a < 20 ? ld32(g_eng.data(0x8a9308), 4u * (uint32_t)a) : 0;
    return entity_sprite_desc(e, b, out);
}

