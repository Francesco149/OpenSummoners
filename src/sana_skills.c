/* sana_skills.c — FUN_0045e830.  See sana_skills.h.
 *
 * Transcribed from the decompile, checked against the asm: the rand draw order of every particle pair (dy
 * first), the 10201 restart (its second, grounded path is dead: it needs act_arg 0 inside act_arg 4), the
 * 10301 ring radius 800 x the counter, the back-flip's phase-3 speed (+4000, then +4000 again while not
 * positive, else 0), the unsigned voice-priority compare. */
#include "sana_skills.h"

#include <stddef.h>
#include <string.h>

#include "char_attack.h"
#include "char_def.h"        /* char_skill_find 0x4072c0 */
#include "char_integrate.h"  /* body_action_reset 0x426f20, body_set_clip 0x407b80 */
#include "char_status.h"     /* es_desc */
#include "engine_calls.h"
#include "entity_move.h"     /* entity_move_horizontal 0x54db10 */
#include "particle_init.h"   /* polar_div 0x460e40, entity_track_on 0x426d70 */
#include "particle_spawn.h"
#include "rng.h"
#include "sound_fx.h"

static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static const void *D(uint32_t va) { return g_eng.data(va); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static const uint8_t *party(const wl_char *c) { return (const uint8_t *)c->party; }

/* the keyboard config **(sys+0x320) */
static const uint8_t *cfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}

/* the voice set may speak (live, not muted, the voices option for mode 1) */
static int voice_ok(const wl_char *c)
{
    if (c->voice == NULL || c->f15b5c != 0) return 0;
    return c->voice_mode != 1 || ld32(cfg(), 0x190) != 0;
}

/* voice group `grp` at the character (not in the probe pass) */
static void voice_line(wl_char *c, int32_t probe, uint16_t grp)
{
    if (probe != 0 || !voice_ok(c)) return;
    int32_t vol, pan, freq;
    if (voice_place(c, &vol, &pan, &freq) == 0) return;
    voice_play(c->voice, grp, 0, vol, pan, freq, 0);
}

/* a sound of pool sys+`pool` at the character; its frequency f0 + rand x 1000 / 0x8000 (drawn in the probe
 * pass too) */
static void pool_sound(wl_char *c, int32_t probe, uint32_t pool, int32_t f0)
{
    int32_t f = mul(1000, rnd()) / 0x8000 + f0;
    if (probe != 0) return;
    wl_voice *v = vpool_take(ldp(g_eng.sys, pool), 0);
    if (v == NULL) return;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(&c->base, ld32(cfg(), 0x168), 0, 0, c->base.body, &vol, &pan) == 0) return;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, f);
    g_eng.dsb_play_5bb8c0(buf);
}

/* the swing's hit box (dx, dy, w, h) on `b` from its skill record (+0x4c), the two twins at dx ± +0xc7f8 with
 * +0xc7f4; no element / power of its own => the weapon's (party +0x124..+0x134) */
static void skill_hits(wl_char *c, wl_body *b, int32_t probe, int32_t dx, int32_t dy, int32_t w, int32_t h,
                       int32_t mode, int32_t kind)
{
    const uint8_t *sk = (const uint8_t *)(intptr_t)b->spell;
    int32_t a10 = ld32(sk, 0x2e4);
    int32_t r = rnd();
    int32_t roll = char_skill_roll(c, sk, 1000, mul(1000, r) / 0x8000);
    int32_t elem = ld32(sk, 0xb0);
    int32_t pw = ld32(sk, 0xb4) + char_mag_sum(c);
    int32_t e1 = 0, e2 = 0, e3 = 0;
    if (ld32(sk, 0x2e8) == 0 && ld32(sk, 0x2dc) == 0 && elem == 0) {
        const uint8_t *p = party(c);
        pw = ld32(p, 0x128); elem = ld32(p, 0x124);
        e1 = ld32(p, 0x12c); e2 = ld32(p, 0x130); e3 = ld32(p, 0x134);
    }
    const uint8_t *p = party(c);
    int32_t ps = ld32(p, 0xd8) + ld32(p, 0xe0) + ld32(p, 0xe4);
    body_hit_spawn(c, c, b, dx, dy, w, h, mode, probe, a10, elem, pw, 0, kind, roll, 1, ld32(sk, 0x2e0), b->serial, 0,
                   ld32(sk, 0x2f4), ps, e1, e2, e3);
    if (ld32(c, 0xc7f4) == 0) return;
    int32_t t = ld32(c, 0xc7f8);
    body_hit_spawn(c, c, b, dx + t, dy, w, h, mode, probe, a10, elem, pw, 0, kind, roll, 1, ld32(sk, 0x2e0), b->serial, 0,
                   ld32(sk, 0x2f4), ps, e1, e2, e3);
    body_hit_spawn(c, c, b, dx - t, dy, w, h, mode, probe, a10, elem, pw, 0, kind, roll, 1, ld32(sk, 0x2e0), b->serial, 0,
                   ld32(sk, 0x2f4), ps, e1, e2, e3);
}

/* the swing's clips (phases 0..3 -> 1..4), the two lunges (phase 2 -> 3, 3 -> 4), phase 4 -> 5 the action ends;
 * phase 2: voice group 1 + the swing sound; `flip` phase 4 turns round; phase 3 (first tick) the hit boxes */
typedef struct {
    uint32_t clip[4];
    int32_t d23, d34;
    uint32_t pool;
    int32_t f0;
    int flip;
    int32_t dx, dy, w, h, mode, kind;
} swing_t;

static void swing(wl_char *c, wl_body *b, int32_t probe, const swing_t *s)
{
    int started = 0;
    switch (b->f58) {
    case 0:
        body_set_clip(b, D(s->clip[0]));
        started = 1;
        b->f58 = 1;
        /* fall through */
    case 1:
        if (b->clip_done == 0) { if (!started) goto hits; break; }
        body_set_clip(b, D(s->clip[1]));
        b->f58 = 2;
        break;
    case 2:
        if (b->clip_done == 0) goto hits;
        body_set_clip(b, D(s->clip[2]));
        char_lunge(c, b, s->d23, probe, 1);
        b->f58 = 3;
        break;
    case 3:
        if (b->clip_done == 0) goto hits;
        body_set_clip(b, D(s->clip[3]));
        char_lunge(c, b, s->d34, probe, 1);
        b->f58 = 4;
        break;
    case 4:
        if (b->clip_done == 0) goto hits;
        body_action_reset(b);
        b->f58 = 5;
        break;
    default:
        goto hits;
    }
    if (b->f58 == 2) {
        voice_line(c, probe, 1);
        pool_sound(c, probe, s->pool, s->f0);
    } else if (s->flip && b->f58 == 4) {
        b->facing = b->facing != 3 ? 3 : 1;
        b->vel_x = -b->vel_x;
    }
hits:
    if (b->act_5a == 0 && b->f58 == 3) skill_hits(c, b, probe, s->dx, s->dy, s->w, s->h, s->mode, s->kind);
}

/* a spell particle at (x - xspan x rand / 0x8000 + xoff, yspan x rand / 0x8000 + y0): dy drawn first */
static void burst(wl_char *c, wl_body *b, int32_t probe, int32_t code, int32_t x, int32_t xspan, int32_t xoff,
                  int32_t yspan, int32_t y0, int32_t a13)
{
    int32_t dy = mul(yspan, rnd()) / 0x8000 + y0;
    int32_t r = rnd();
    char_particle(c, code, b, x - mul(xspan, r) / 0x8000 + xoff, dy, 99, 1, probe, 0, 2, 1, 1, a13);
}

static void spell_sound(wl_char *c, int32_t probe)
{
    int32_t r = rnd();
    sfx_play_at(&c->base, probe, 82, 0, mul(1000, r) / 0x8000 - 500, 0);
}

/* the shot of 10202: voice cue `grp`, shot 100001 at (dx, dy) by 1000 with the hold +0x204 */
static void magic_shot(wl_char *c, wl_body *b, int32_t probe, uint16_t grp, int32_t dx, int32_t dy, int32_t a8,
                       int32_t hold)
{
    voice_cue(c, probe, grp, 0, 0, 0);
    void *s = char_shot_fire(c, 100001, b, dx, dy, 1, probe, 0, a8, (const uint8_t *)(intptr_t)b->spell, 1000,
                             b->facing, 0, 2);
    if (s != NULL) st32(s, 0x204, hold);
}

static void skill_10202(wl_char *c, wl_body *b, int32_t probe, int32_t grounded)
{
    if (b->f58 == 0) body_pose_swap(c, b, grounded, 10202, 8, 10202, 7, 0);
    switch (b->act_arg) {
    case 1:
        if (b->f58 >= 2 && b->f58 <= 3) {
            if (b->act_ctr == 0) magic_shot(c, b, probe, 3, 1600, -3200, 45, 600);
            b->act_ctr++;
        }
        if (clip_seq_step(b, (intptr_t)D(0x638858), 4, 0) == 0 || b->f58 != 5) return;
        break;
    case 7:
        if (grounded == 0) {
            if (clip_seq_step(b, (intptr_t)D(0x6394f8), 3, 0) == 0) return;
            if (b->f58 == 3) { magic_shot(c, b, probe, 3, 3200, 3200, 120, 800); return; }
            if (b->f58 != 5) return;
        }
        break;
    case 8:
        if (clip_seq_step(b, (intptr_t)D(0x638ea8), 4, 0) == 0) return;
        if (b->f58 == 2) { magic_shot(c, b, probe, 3, 3200, 0, 90, 700); return; }
        if (b->f58 != 5) return;
        break;
    default:
        if (clip_seq_step(b, (intptr_t)D(0x638208), 4, 0) == 0) return;
        if (b->f58 == 2) { magic_shot(c, b, probe, 5, 3200, 0, 90, 1000); return; }
        if (b->f58 != 5) return;
        break;
    }
    body_action_reset(b);
}

static void skill_10301(wl_char *c, wl_body *b, int32_t probe)
{
    const uint8_t *sk = (const uint8_t *)(intptr_t)b->spell;
    switch (b->act_arg) {
    case 1: {
        if (clip_seq_step(b, (intptr_t)D(0x63c798), 4, 0) != 0) {
            if (b->f58 == 2) { spell_sound(c, probe); voice_cue(c, probe, 5, 0, 0, 0); }
            else if (b->f58 == 5) body_action_reset(b);
        }
        if (b->f58 == 2) b->act_ctr++;
        else if (b->f58 != 3) return;
        int32_t x, y;
        polar_div(800 * b->act_ctr, 45, 1, 1600, -1600, &x, &y);
        if (b->act_5a >= 3) return;
        int32_t code = b->f58 == 2 ? 100067 : 100057, span = b->f58 == 2 ? 1000 : 1600;
        burst(c, b, probe, code, x, span, span / 2, 800, y - 1600, 10);
        if (ld32(cfg(), 0x94) != 0) burst(c, b, probe, code, x, span, span / 2, 800, y - 1600, 0);
        char_attack_hits(c, b, x, y, 3200, 3200, 2, probe, sk, 1000, 8);
        return;
    }
    case 3:
        if (clip_seq_step(b, (intptr_t)D(0x63be18), 3, 0) != 0) {
            if (b->f58 == 2) { spell_sound(c, probe); voice_cue(c, probe, 3, 0, 0, 0); }
            else if (b->f58 == 4) body_action_reset(b);
        }
        if (b->f58 == 2 && b->act_5a < 3) {
            burst(c, b, probe, 100057, 5600, 3200, 800, 800, 1600, 10);
            char_attack_hits(c, b, 5600, 800, 4800, 4800, 0, probe, sk, 1000, 8);
        }
        return;
    case 5:
        if (clip_seq_step(b, (intptr_t)D(0x63c2d8), 3, 0) != 0) {
            if (b->f58 == 2) spell_sound(c, probe);
            else if (b->f58 == 4) body_action_reset(b);
        }
        if (b->f58 == 2) burst(c, b, probe, 100057, 5600, 3200, 800, 800, 1600, 10);
        return;
    default:
        break;
    }
    if (clip_seq_step(b, (intptr_t)D(0x63b7c8), 4, 0) != 0) {
        if (b->f58 == 2) { spell_sound(c, probe); voice_cue(c, probe, 6, 0, 0, 0); }
        else if (b->f58 == 5) body_action_reset(b);
    }
    if (b->f58 < 3 || b->f58 > 4) return;
    if (b->f58 == 3) b->act_ctr++;
    else if (b->act_ctr > 0) b->act_ctr--;
    int32_t x = 800 * (b->act_ctr + 6);
    if (b->act_5a >= 2) return;
    burst(c, b, probe, 100067, x, 1000, 500, 1600, -800, 10);
    if (ld32(cfg(), 0x94) != 0) {
        burst(c, b, probe, 100067, x, 1600, 800, 1600, -800, 0);
        burst(c, b, probe, 100067, x, 1600, 800, 1600, -800, 0);
    }
    char_attack_hits(c, b, x, 1600, 3200, 3200, 0, probe, sk, 1000, 8);
}

/* the orbit record at char +0x14834 {x, y, r, ang, dang, u16 +0x18 clock, +0x1a delay, +0x1c limit} */
static void orbit_set(wl_char *c, int32_t x, int32_t y, int32_t ang, int16_t limit)
{
    st32(c, 0x14834, x); st32(c, 0x14838, y); st32(c, 0x1483c, 1200); st32(c, 0x14840, ang);
    st32(c, 0x14844, 0); st16(c, 0x1484c, 0); st16(c, 0x1484e, 12); st16(c, 0x14850, (uint16_t)limit);
}

static void skill_10302(wl_char *c, wl_body *b, int32_t probe, int32_t grounded)
{
    if (b->act_arg == 4 && b->f58 == 0 && b->pose == 10302 && grounded == 0) {
        body_action_reset(b);
        uint8_t *sk = char_skill_find(c, 10302);
        body_pose_set(b, 10302, b->act_len, 0, b->target, (int32_t)(intptr_t)sk);
    }
    if (b->act_arg == 4) {
        if (b->f58 != 0) {
            if (probe == 0) {
                uint16_t k = ld16(c, 0x1484c);
                if (k < ld16(c, 0x1484e)) st16(c, 0x1484c, (uint16_t)(k + 1));
                else if (k < ld16(c, 0x14850)) {
                    st16(c, 0x1484c, (uint16_t)(k + 1));
                    int32_t a = angle_wrap(ld32(c, 0x14840), ld32(c, 0x14844), 1000);
                    st32(c, 0x14840, a);
                    int32_t x, y;
                    polar_div(ld32(c, 0x1483c), a, 1000, ld32(c, 0x14834), ld32(c, 0x14838), &x, &y);
                    st32(c, 0x14834, x); st32(c, 0x14838, y);
                    char_particle(c, 100077, b, x, y, b->facing, 1, 0, 0, 2, 1, 1, 0);
                }
            }
        } else if (probe == 0) {
            orbit_set(c, 2000, 0, 280000, 18);
        }
        static const swing_t s = { { 0x63ab28, 0x63acbc, 0x63ae50, 0x63afe4 }, -1600, 1600, 0x478, -500, 1,
                                   -2400, 400, 3200, 2000, 1, 1 };
        swing(c, b, probe, &s);
        return;
    }
    if (b->f58 != 0) orbit_step(c, b, (int32_t *)((uint8_t *)c + 0x14834), 0, 1, probe, 100077, 0);
    else if (probe == 0) orbit_set(c, 6400, -1600, 240000, 16);
    if (slash_step(c, b, probe, (intptr_t)D(0x63b178), 1200, 0, 0, grounded) != 0 && b->f58 == 2) {
        voice_cue(c, probe, 1, 0, 0, 0);
        pool_sound(c, probe, 0x478, -500);
    }
    if (b->act_5a == 0 && b->f58 == 3)
        char_attack_hits(c, b, 3200, 800, 3200, 2000, 0, probe, (const uint8_t *)(intptr_t)b->spell, 1000, 1);
}

/* the afterimages of the back-flip's rise: particle 100082 per sprite descriptor, tint 12, a 10-tick life */
static void flip_images(wl_char *c, wl_body *b, int32_t probe)
{
    es_desc d[16];
    uint16_t n = 0;
    if (entity_sprite_descs(&c->base, b, d, &n) == 0) return;
    for (uint16_t i = 0; i < n; i++) {
        wl_entity *p = particle_spawn(&c->base, &c->base, 100082, b, b->x + 100 * (b->dst_x + d[i].x),
                                      b->y + 100 * (b->dst_y + d[i].y), 99, 0, probe, 0, 0, 0, 0, 10);
        if (p == NULL) continue;
        entity_track_on(p, 0, (int16_t)d[i].slot, (int16_t)d[i].frame);
        particle_tint_set(p, 12);
        st32(p, 0x260, 10);
    }
}

/* the clip `clip` (when new), the phase + 1 */
static void clip_next(wl_body *b, const void *clip)
{
    body_set_clip(b, clip);
    b->f58++;
}

static void skill_10303(wl_char *c, wl_body *b, int32_t probe, int32_t grounded)
{
    uint32_t base = c->base.code == 50011 ? 0x639b48u : 0x63a668u;
    switch (b->f58) {
    case 0:
        if (b->act_arg == 2) {
            b->facing = b->facing != 3 ? 3 : 1;
            b->vel_x = -b->vel_x;
        }
        if (entity_move_horizontal(&c->base, b, b->facing != 3 ? -800 : 800, 1, 1, 0, 0, 0) != 0 && probe == 0)
            st32(c, 0x14834, ld32(c, 0x14834) + 800);
        clip_next(b, D(base));
        return;
    case 1:
        if (b->clip_done == 0) return;
        pool_sound(c, probe, 0x57c, 1000);
        body_set_clip(b, D(base + 404));
        b->f58++;
        b->vel_x = -64000;
        b->vvel = -40000;
        voice_line(c, probe, 7);
        return;
    case 2:
        b->act_ctr++;
        if (b->act_ctr > 10) {
            int32_t v = b->vel_x + 8000;
            if (v > -20000) v = -20000;
            b->vel_x = v;
        }
        if (b->act_ctr < 20 && b->act_ctr % 5 == 0) flip_images(c, b, probe);
        if (b->act_ctr > 40) { body_action_reset(b); return; }
        if (grounded == 0) return;
        if (entity_move_horizontal(&c->base, b, b->facing != 3 ? 800 : -800, 1, 1, 0, 0, 0) != 0 && probe == 0)
            st32(c, 0x14834, ld32(c, 0x14834) - 800);
        clip_next(b, D(base + 808));
        return;
    case 3: {
        int32_t v = b->vel_x + 4000;
        b->vel_x = v <= 0 ? v + 4000 : 0;
        if (b->clip_done != 0) body_action_reset(b);
        return;
    }
    default:
        return;
    }
}

/* pose 10: the two-part rest on the ground (voice 25 and the queued line 26 on entry) */
static void pose_rest(wl_char *c, wl_body *b, int32_t probe, int32_t grounded)
{
    if (grounded == 0) { body_action_reset(b); return; }
    switch (b->f58) {
    case 0:
        if (probe == 0) {
            voice_line(c, probe, 25);
            wl_voiceset *vs = c->voice;
            if (voice_ok(c)) {
                uint16_t pr = vs->n > 26 ? vs->grp[26].prio : 0;
                if (c->voice_delay <= 0 || pr >= voice_group_prio(vs, c->voice_id)) {
                    c->voice_delay = 30; c->voice_id = 26; c->voice_arg = 0; c->voice_vol = 0;
                }
            }
        }
        clip_next(b, D(0x635d98));
        return;
    case 1:
        if (b->clip_done != 0) b->f58 = 2;
        return;
    case 2:
        body_set_clip(b, D(0x635f30));
        b->act_ctr = 0;
        b->f58 = 3;
        /* fall through */
    case 3:
        if (++b->act_ctr < 24) return;
        clip_next(b, D(0x635d98));
        return;
    case 4:
        if (b->clip_done != 0) body_action_reset(b);
        return;
    default:
        return;
    }
}

/* poses 100 / 101: the idle animations by code */
static void pose_idle(wl_char *c, wl_body *b)
{
    int32_t code = c->base.code;
    uint32_t enter, loop, lim;
    if (code == 50011) {
        enter = 0x636260;
        if (b->pose == 101) { lim = 180; loop = 0x6363f8; } else { lim = 520; loop = 0x636590; }
    } else if (code == 50012) {
        enter = 0x63a008;
        lim = b->pose != 101 ? 550 : 180;
        loop = 0x63a1a0;
    } else {
        return;
    }
    if (b->f58 == 0) {
        body_set_clip(b, D(enter));
        b->f58 = 1;
    } else if (b->f58 != 1) {
        if (b->f58 == 2 && b->clip_done != 0) body_action_reset(b);
        return;
    }
    b->act_ctr++;
    if (b->act_ctr <= (int32_t)lim || b->clip_frame != 0) return;
    b->f58++;
    body_set_clip(b, D(loop));
}

void sana_skills(wl_char *c, wl_body *b, int32_t probe, int32_t *ctl, const wl_cmd *cmd, int32_t grounded)
{
    (void)cmd;
    uint16_t pose = b->pose;
    if (pose > 0x65) {
        switch (pose) {
        case 10201: {
            *ctl = 2;
            if (b->act_arg == 4 && b->f58 == 0 && b->pose == 10201 && grounded == 0) {
                body_action_reset(b);
                uint8_t *sk = char_skill_find(c, 10201);
                body_pose_set(b, 10201, b->act_len, 0, b->target, (int32_t)(intptr_t)sk);
            }
            static const swing_t air = { { 0x637bb8, 0x637d4c, 0x637ee0, 0x638074 }, 0, -800, 0x474, 1000, 1,
                                         -3600, 400, 3200, 2000, 1, 11 };
            static const swing_t ground = { { 0x637568, 0x6376fc, 0x637890, 0x637a24 }, 0, 0, 0x474, 1000, 0,
                                            4400, 800, 4000, 4000, 0, 11 };
            swing(c, b, probe, b->act_arg == 4 ? &air : &ground);
            return;
        }
        case 10202: *ctl = 2; skill_10202(c, b, probe, grounded); return;
        case 10301: *ctl = 2; skill_10301(c, b, probe); return;
        case 10302: *ctl = 2; skill_10302(c, b, probe, grounded); return;
        case 10303: *ctl = 3; skill_10303(c, b, probe, grounded); return;
        default: return;
        }
    }
    if (pose >= 0x64) { pose_idle(c, b); return; }
    uint32_t clip;
    switch (pose) {
    case 10: *ctl = 2; pose_rest(c, b, probe, grounded); return;
    case 13: clip = 0x636728; break;
    case 14: clip = 0x6368c0; break;
    case 15: clip = 0x63a338; break;
    case 16: clip = 0x63a4d0; break;
    default: return;
    }
    body_set_clip(b, D(clip));
    if (b->clip_done != 0) body_action_reset(b);
}
