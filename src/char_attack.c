/* char_attack.c — the attack pipeline.  See char_attack.h.  Transcribed from the asm. */
#include "char_attack.h"

#include <stddef.h>
#include <string.h>

#include "arche_pose.h"    /* switch_touch 0x5b1aa0 */
#include "char_ai.h"       /* body_frame_box 0x4411e0 */
#include "char_def.h"      /* stat_jitter 0x426e00 */
#include "engine_calls.h"
#include "entity_move.h"   /* entity_move_horizontal 0x54db10, faction_relation 0x4416c0 */
#include "gimmick.h"       /* entity_rect_hit 0x4804b0 */
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
    if (a19 == 1) g_eng.h5b3890(self, IP(rec + 0x14));
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
    g_eng.h5b5600(atk, IP(t), 0);
    return 1;
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
