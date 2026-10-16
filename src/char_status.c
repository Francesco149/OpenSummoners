/* char_status.c — FUN_00487dc0 + FUN_004878b0 and their leaves.  See char_status.h.
 *
 * Transcribed from the asm.  The status checks read the party's u16 timers A[st] (+0x448 + 2 st) and dword
 * values V[st] (+0x490 + 4 st); "gate" is retail's `the character is tangible and not fading` flag.  A shot
 * slot is released through 0x556180 when the character is fading, else flagged (+0x204: 1 hold, 2 end). */
#include "char_status.h"

#include <stddef.h>
#include <string.h>

#include "arche_pose.h"
#include "char_hit.h"        /* char_die 0x5b4d00 */
#include "char_integrate.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "gimmick.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "particle_spawn.h"
#include "rng.h"
#include "shots.h"         /* shot_init 0x55d140 */
#include "sound_fx.h"

#define W (*g_eng.world)

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }
static int32_t rnd(void) { return (int32_t)rng_rand(); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
static int32_t iabs(int32_t v) { return v < 0 ? -v : v; }

static const uint8_t *cfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}
static uint16_t A(const wl_char *c, int st) { return ld16(c->party, 0x448 + 2u * st); }

/* the cure lines of 0x48bb30 / 0x487dc0 by status (0 = none) */
static uint32_t cure_line(int st)
{
    switch (st) {
    case 1: return 0x85530cu;   /* "%s's strength returns!" */
    case 2: return 0x855340u;   /* "%s can move again." */
    case 3: return 0x855324u;   /* "%s is no longer confused." */
    case 5: return 0x855354u;   /* "%s woke up." */
    case 6: return 0x855388u;   /* "%s is no longer poisoned." */
    case 7: return 0x855360u;   /* "%s's defenses have returned to normal." */
    default: return 0;
    }
}

void world_log_line(wl_world *w, const char *text)
{
    uint8_t *p = (uint8_t *)w;
    int32_t i = ld32(p, 0x1024);
    for (uint16_t n = 0; ld32(p, 516 * i + 4) != 0; ) {
        i = (i + 1) % 8;
        if (++n >= 8) return;
    }
    st32(p, 516 * i + 4, 1);
    strcpy((char *)p + 516 * i + 8, text);
}

void char_status_decay(wl_char *c, uint16_t st, uint16_t n, int32_t now, uint32_t log, char *out)
{
    uint8_t *pt = (uint8_t *)c->party;
    uint16_t t = ld16(pt, 0x448 + 2u * st);
    if (t == 0) return;
    if (t > n && now == 0) { st16(pt, 0x448 + 2u * st, (uint16_t)(t - n)); return; }
    st16(pt, 0x448 + 2u * st, 0);
    uint32_t msg = cure_line(st);
    if (msg == 0) return;
    if (st == 1) { c->base.f0fc = ld32(c, 0x15910); c->base.f100 = ld32(c, 0x15910); }
    if (ld32(c, 0xc794) == 0 || char_room_logs(c) == 0) return;
    char line[1024];
    g_eng.h5bf3ee(line, S(msg), (const char *)c->party);
    if (log != 0) world_log_line(W, line);
    if (out != NULL) strcpy(out, line);
}

int32_t char_hit_queue(wl_char *c, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8,
                       int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15,
                       int32_t a16, int32_t a17)
{
    st32(c, 0x15aac, a2); st16(c, 0x26e, 1); st32(c, 0x15aa8, a3); st32(c, 0x15ab0, 1);
    st32(c, 0x15ab8, a5); st32(c, 0x15ab4, a4); st32(c, 0x15ac0, a7); st32(c, 0x15abc, a6);
    st32(c, 0x15ac8, a9); st32(c, 0x15ac4, a8); st32(c, 0x15ad0, a11); st32(c, 0x15acc, a10);
    st32(c, 0x15ad8, a13); st32(c, 0x15ad4, a12); st32(c, 0x15ae0, a15); st32(c, 0x15adc, a14);
    st32(c, 0x15ae8, a17); st32(c, 0x15ae4, a16);
    return a16;
}

int entity_part_descs(const wl_entity *e, const wl_body *b, es_desc *out, uint16_t *n)
{
    const uint8_t *t = e->f08;
    if (t == NULL) return 0;
    uint16_t idx = 0;
    const uint8_t *clip = b->clip;
    if (clip != NULL) {
        idx = (uint16_t)(ld16(clip, 0) + ld16(clip, 2 + 2u * b->clip_frame));
        if (b->facing == 3) idx = (uint16_t)(idx + ld16(t, 0x18));
    }
    const uint8_t *rows = (const uint8_t *)ldp(t, 0) + 52u * (uint32_t)ld32(t, 0x1c) * idx;
    uint16_t k = 0;
    for (uint16_t i = 0; i < (uint32_t)ld32(e->f08, 0x1c); i++) {
        const uint8_t *r = rows + 52u * i;
        if (ld32(r, 0x14) != 0 || ld32(r, 0) == 0) continue;
        es_desc *d = &out[k++];
        d->slot = ld16(r, 0x18);
        d->frame = ld16(r, 0x1c);
        d->alpha = 0;
        d->x = ld32(r, 4) + e->width / 100 / 2;
        d->y = ld32(r, 8) + e->height / 100 / 2;
        d->f0c = ld32(r, 0);
    }
    *n = k;
    return k != 0;
}

/* the entity's 20-byte sprite rows +0x48 (u16 slot, u16 frame base, +0x8 / +0xc the left / right x, +0x10 y);
 * a clip's row override (+0x150) may index past the eight the setup fills */
static uint16_t row16(const wl_entity *e, uint16_t r, uint32_t off) { return ld16(e, 0x48 + 20u * r + off); }
static int32_t row32(const wl_entity *e, uint16_t r, uint32_t off) { return ld32(e, 0x48 + 20u * r + off); }

int entity_sprite_desc(const wl_entity *e, const wl_body *b, es_desc *out)
{
    uint16_t r = e->fe8;
    if (row16(e, r, 0) == 0 || b->active == 0) return 0;
    const uint8_t *clip = b->clip;
    int32_t own = 0;
    uint16_t base;
    if (clip == NULL) {
        out->x = 0;
        out->y = 0;
        base = 0;
    } else {
        uint16_t fr = b->clip_frame;
        if (ld16(clip, 2 + 2u * fr) == 0xffff) return 0;
        own = ld32(clip, 0x4c);
        out->x = ld32(clip, 0x50 + 4u * fr);
        out->y = ld32(clip, 0xd0 + 4u * fr);
        base = (uint16_t)(ld16(clip, 2 + 2u * fr) + ld16(clip, 0));
        uint16_t row = ld16(clip, 0x150);
        if (row != 0) r = row != 255 ? row : 0;
    }
    uint16_t slot = row16(e, r, 0);
    uint16_t half = ld16(ldp(g_eng.sys, 0x18e0 + 4u * slot), 0), add = 0;
    if (own == 0) {
        if (e->fec != 0) {
            uint16_t n = e->ff0;
            add = (uint16_t)((uint16_t)((b->angle + 360000) / (360000 / n)) % n);
            if (b->facing == 3) add = (uint16_t)(add + half);
        } else if (b->facing == 3) {
            out->x = row32(e, r, 8) - out->x;
            add = half;
        } else {
            out->x += row32(e, r, 0xc);
        }
    }
    out->y += row32(e, r, 0x10);
    out->slot = slot;
    out->alpha = 0;
    out->frame = (uint16_t)(row16(e, r, 2) + add + base);
    return 1;
}

void *shot_spawn(void *self, int32_t a2, int32_t a3, int32_t a4, wl_body *b, int32_t dx, int32_t dy, int32_t anchor,
                 int32_t a9, int16_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14, int32_t a15, int32_t a16,
                 int32_t a17, int32_t a18, int32_t a19, int32_t a20)
{
    if (a9 != 0) return NULL;
    uint8_t *rl = W->f1044;
    wl_entity *s = NULL;
    for (uint16_t n = 0; ; ) {
        uint16_t k = ld16(rl, 0xd2);
        s = W->band64[k];
        st16(rl, 0xd2, (uint16_t)((k + 1) & 0x3f));
        if (s->alive == 0) break;
        if (++n >= 64) return NULL;
    }
    if (s == NULL) return NULL;
    if (b->facing == 3) dx = -dx;
    int32_t x, y;
    switch (anchor) {
    case 1: x = dx + b->x + b->w / 2; y = dy + b->y + b->h / 2; break;
    case 2: x = dx + b->x; y = dy + b->y + b->margin; break;
    case 3: x = dx + b->x + b->w / 2; y = dy + b->y + b->margin; break;
    case 4: x = dx + b->x + b->w / 2; y = dy + b->y + b->h; break;
    default: x = dx; y = dy; break;
    }
    shot_init((wl_char *)s, a2, a4, a3, x, y, a14, a12, a10, a11, a20, a13, a15, a16, a17, (intptr_t)self, a18, a19);
    return s;
}

/* a pool sound at the character (`slot` sys offset), the effects gain, freq 0 */
static void pool_sound(wl_char *c, uint32_t slot)
{
    wl_voice *v = vpool_take(ldp(g_eng.sys, slot), 0);
    if (v == NULL) return;
    void *buf = v->buf;
    int32_t vol, pan;
    if (sfx_place(&c->base, ld32(cfg(), 0x168), 0, 0, c->base.body, &vol, &pan) == 0) return;
    g_eng.dsb_volume_5bb870(buf, vol);
    g_eng.dsb_pan_5bb880(buf, pan);
    g_eng.dsb_freq_5bb890(buf, 0);
    g_eng.dsb_play_5bb8c0(buf);
}

/* the cure line of a status cleared here (the room logs it) */
static void cure_log(wl_char *c, uint32_t msg)
{
    if (ld32(c, 0xc794) == 0 || char_room_logs(c) == 0) return;
    char line[1024];
    g_eng.h5bf3ee(line, S(msg), (const char *)c->party);
    world_log_line(W, line);
}

/* the world's next action serial as 0x487dc0 passes it (old + 1, though the counter wraps to 0) */
static int32_t next_serial(void)
{
    uint8_t *w = (uint8_t *)W;
    uint32_t s = (uint32_t)ld32(w, 0x27a0);
    st32(w, 0x27a0, s >= 0xffffff ? 0 : (int32_t)(s + 1));
    return (int32_t)(s + 1);
}

/* a damage-over-time hit of `pct` % of the HP parts (+0x58 / +0x84 / +0x9c), kind `kind` */
static void dot_hit(wl_char *c, int32_t pct, int32_t kind)
{
    const uint8_t *pt = (const uint8_t *)c->party;
    int32_t dmg = pct * (ld32(pt, 0x9c) + ld32(pt, 0x84) + ld32(pt, 0x58)) / 100;
    int32_t serial = next_serial();
    char_hit_queue(c, 0, 0, 99, 99, part_center_x(&c->base, 0), part_center_y(&c->base, 0), 0, 0, 0, kind, serial, dmg,
                 1, 0, 0, 0);
}

/* the ten shots +0xc7fc.. released (a pose status) */
static void shots_drop(wl_char *c)
{
    for (int i = 0; i < 10; i++) {
        wl_entity *s = ldp(c, 0xc7fc + 4u * i);
        if (s != NULL) { entity_release(s, 0); stp(c, 0xc7fc + 4u * i, NULL); }
    }
    st32(c, 0xc7f4, 0);
}

/* a pose status's hold: below 50 left phase 20, below 100 phase 10, else phase 0 once past 10 */
static void pose_hold(wl_body *b, uint16_t t)
{
    if (t < 50) b->f58 = 20;
    else if (t < 100) b->f58 = 10;
    else if (b->f58 >= 10) b->f58 = 0;
}

/* a shot slot given up: flagged to end (+0x204 = 2) while tangible, else released */
static void shot_end(wl_char *c, uint32_t off, int gate)
{
    wl_entity *s = ldp(c, off);
    if (s == NULL) return;
    if (gate) st32(s, 0x204, 2);
    else entity_release(s, 0);
    stp(c, off, NULL);
}

static wl_entity *spawn(wl_char *c, int32_t code, wl_body *b, int32_t dx, int32_t dy, int16_t a10, int32_t a11, int32_t a12,
                        int32_t a14)
{
    return shot_spawn(c, (int32_t)(intptr_t)c, ld32(c, 0x1dc), code, b, dx, dy, 1, 0, a10, a11, a12, 0, a14, 0, 0, 0, 0,
                      0, 2);
}

void char_status_tick(wl_char *c, wl_body *b)
{
    int32_t f268 = c->f268;
    int gate = f268 == 0 && ld16(c, 0x158a8) == 0;
    int aura = 0, still = 0;
    int32_t ground = 0;   /* (retail leaves it uninitialised for move modes outside 0..3: none are set) */
    int32_t mm = c->move_mode;
    if (mm == 0) ground = b->grounded;
    else if (mm > 0 && mm <= 3) ground = 1;
    int pose_status = 0;

    if (A(c, 4) != 0 && W->f1034 == 0 && W->scene_kind != 2 && f268 == 0 && ld32(c, 0x15908) == 0)
        char_die(c, 0, 1, b->facing);
    if (A(c, 3) != 0) {
        st32(c, 0x1f0, 1);
        if (ldp(c, 0xc824) == NULL)
            stp(c, 0xc824, particle_spawn(&c->base, &c->base, 100113, b, 0, -400, 99, 3, 0, 0, 2, 0, 0, 20));
        st32(c, 0xc82c, 1);
    } else {
        st32(c, 0x1f0, 0);
    }
    if (A(c, 1) != 0) {
        if (W->scene_kind == 2) {
            st16(c->party, 0x44a, 0);
            c->base.f0fc = ld32(c, 0x15910);
            c->base.f100 = ld32(c, 0x15910);
            cure_log(c, 0x85530cu);   /* "%s's strength returns!" */
            st32(c->party, 0x54, 1);
        } else if (gate) {
            c->base.f0fc = 12;
            c->base.f100 = 12;
            pose_status = 1;
        }
    }
    if (!pose_status && A(c, 6) != 0) {
        if (W->scene_kind == 2) {
            st16(c->party, 0x454, 0);
            cure_log(c, 0x855388u);   /* "%s is no longer poisoned." */
        } else if (!gate) {
            goto poses;
        } else {
            if (ld16(c, 0x206) % 8 == 0) {
                int32_t dy = mul(rnd(), 1600) / 0x8000 - 800;
                int32_t dx = mul(rnd(), 1600) / 0x8000 - 800;
                particle_spawn(&c->base, &c->base, 100107, b, dx, dy, 99, 3, 0, 0, 2, 0, 0, 0);
            }
            uint16_t t = ld16(c->party, 0x4d8);
            if (t < (ld32(c, 0xc794) != 0 ? 0x200 : 0x100)) st16(c->party, 0x4d8, (uint16_t)(t + 1));
            else { st16(c->party, 0x4d8, 0); dot_hit(c, 5, 13); }
        }
    }
    if (!pose_status && gate && ld32(c, 0xc794) != 0) {
        int32_t kind;
        grid_c_query(W->grid, b->x + b->w / 2, b->h + b->y - 1, &kind);
        if (kind == 3) {
            uint16_t t = ld16(c->party, 0x4da);
            if (t < 0x100) st16(c->party, 0x4da, (uint16_t)(t + 1));
            else { st16(c->party, 0x4da, 0); dot_hit(c, 2, 14); }
        }
    }
poses:
    if (!pose_status) {
        uint16_t t = A(c, 2);
        if (t != 0 && gate) {
            if (b->pose == 118) pose_hold(b, t);
            else body_action_start(b, 118, 210);
            pose_status = 1;
        } else if (b->pose == 118) {
            body_action_reset(b);
        }
    }
    if (!pose_status) {
        uint16_t t = A(c, 8);
        if (t != 0 && gate) {
            if (b->pose == 119) pose_hold(b, t);
            else body_action_start(b, 119, 210);
            c->base.tint_f4 = ld32(g_eng.sys, 0x2868);
            pose_status = 1;
        } else if (b->pose == 119) {
            body_action_reset(b);
        }
    }
    if (!pose_status && ld32(c, 0x15934) != 0) {
        if (ldp(c, 0xc7a4) != NULL && A(c, 9) != 0 && gate) {
            if (b->pose == 121) { if (b->f58 >= 10) b->f58 = 0; }
            else body_action_start(b, 121, 800);
            pose_status = 1;
        } else {
            if (b->pose == 121) body_action_reset(b);
            if (A(c, 9) != 0) st16(c->party, 0x45a, 0);
            if (ld32(c, 0x15934) != 0 && ldp(c, 0xc7a4) != NULL) {
                st32(ldp(c, 0xc7a4), 0xc798, 0);
                stp(c, 0xc7a4, NULL);
            }
        }
    }
    if (!pose_status) {
        if (A(c, 7) != 0 && gate) {
            st32(c, 0xc7dc, ld32(c, 0xc7dc) - ld32(c->party, 0x4ac));
            st32(c, 0xc7e0, ld32(c, 0xc7e0) - ld32(c->party, 0x4ac));
            if (ld16(c, 0x206) % 8 == 0) {
                int32_t dy = mul(rnd(), 1600) / 0x8000 - 1600;
                int32_t dx = mul(rnd(), 3200) / 0x8000 - 1600;
                particle_spawn(&c->base, &c->base, 100108, b, dx, dy, 99, 3, 0, 0, 2, 0, 0, 0);
            }
        }
        uint16_t t = A(c, 5);
        if (t != 0 && gate) {
            if (b->pose == 117) pose_hold(b, t);
            else body_action_start(b, 117, 210);
            pose_status = 1;
        } else if (b->pose == 117) {
            body_action_reset(b);
        }
    }
    if (!pose_status) {
        if (A(c, 10) != 0 && gate) {
            if (b->pose == 109) { if (b->f58 >= 10) b->f58 = 0; }
            else body_action_start(b, 109, 210);
            pose_status = 1;
        } else if (b->pose == 109) {
            body_action_start(b, 108, 210);
        }
    }
    if (pose_status) { shots_drop(c); goto cures; }

    if (A(c, 15) != 0 && gate) {
        st32(c, 0xc7d8, ld32(c, 0xc7d8) + 20);
        st32(c, 0xc7dc, ld32(c, 0xc7dc) + 20);
        st32(c, 0xc7e4, ld32(c, 0xc7e4) + 20);
        st32(c, 0xc7e0, ld32(c, 0xc7e0) + 20);
        st16(c, 0xc7e8, 2);
        for (uint32_t off = 0xc7c0; off <= 0xc7d0; off += 4) st32(c, off, 0);
        st32(c, 0xc7d4, 250);
        if (ld16(c, 0x206) % 10 == 0)
            for (int i = 0; i < 8; i++) {
                rnd();
                wl_entity *p = particle_spawn(&c->base, &c->base, 100131, b, 0, 0, 99, 1, 0, 0, 2, 0, 0, 10);
                if (p == NULL) continue;
                int32_t v = mul(rnd(), 4000) / 0x8000;
                st32(p, 0x25c, v);
                if (p->code == 100044) { p->tint_f4 = ld32(g_eng.sys, 0x2758 + 4u * v); p->tint_f8 = p->tint_f4; }
                st32(p, 0x260, mul(rnd(), 1600) / 0x8000 + 2400);
            }
    }
    if (A(c, 17) != 0 && gate) {
        int32_t pw = ld32(c->party, 0x4d4);
        if (ldp(c, 0xc80c) == NULL) stp(c, 0xc80c, spawn(c, 100015, b, 9000, -2000, 0, pw, 120, b->facing));
        if (ldp(c, 0xc810) == NULL) stp(c, 0xc810, spawn(c, 100015, b, -9000, -2000, 10, pw, 120, b->facing));
        if (ldp(c, 0xc814) == NULL) stp(c, 0xc814, spawn(c, 100015, b, 4000, -8000, 20, pw, 120, b->facing));
        if (ldp(c, 0xc818) == NULL) stp(c, 0xc818, spawn(c, 100015, b, -4000, -8000, 30, pw, 120, b->facing));
    } else {
        for (uint32_t off = 0xc80c; off <= 0xc818; off += 4) {
            wl_entity *s = ldp(c, off);
            if (s != NULL) { st32(s, 0x204, 1); stp(c, off, NULL); }
        }
    }
    if (A(c, 16) != 0 && gate) {
        int32_t hits = 0;
        wl_entity *s = ldp(c, 0xc804);
        if (s != NULL) hits = ld32(s, 0x22c);
        else {
            s = spawn(c, 100014, b, 0, 0, 0, ld32(c->party, 0x4d0), 0, b->facing);
            stp(c, 0xc804, s);
            if (s != NULL) { st32(s, 0x208, 41); char_status_decay(c, 16, 25, 0, 1, NULL); }
        }
        s = ldp(c, 0xc808);
        if (s != NULL) hits += ld32(s, 0x22c);
        else {
            s = spawn(c, 100014, b, 0, 0, 0, ld32(c->party, 0x4d0), 180, b->facing);
            stp(c, 0xc808, s);
            if (s != NULL) { st32(s, 0x208, 1); char_status_decay(c, 16, 25, 0, 1, NULL); }
        }
        if (hits >= 2) {
            if (ldp(c, 0xc804) != NULL) { entity_release(ldp(c, 0xc804), 0); stp(c, 0xc804, NULL); }
            if (ldp(c, 0xc808) != NULL) { entity_release(ldp(c, 0xc808), 0); stp(c, 0xc808, NULL); }
        }
    } else {
        for (uint32_t off = 0xc804; off <= 0xc808; off += 4) {
            wl_entity *s = ldp(c, off);
            if (s != NULL) { st32(s, 0x204, 1); stp(c, off, NULL); }
        }
    }
    if (A(c, 13) != 0 && gate) {
        st32(c, 0xc7d8, ld32(c, 0xc7d8) + ld32(c->party, 0x4c4));
        if (ground != 0)
            for (int i = 0; i < 4; i++)
                particle_spawn(&c->base, &c->base, 100109, b, mul(rnd(), 6400) / 0x8000 - 3200, 0, 99, 4, 0, 0, 2, 0, 0, 0);
    }
    if (A(c, 12) != 0 && gate) {
        if (ld32(c, 0xc7f4) == 0) {
            pool_sound(c, 0x4d0);
            st32(c, 0xc7f4, 1);
            st32(c, 0xc7f8, 0);
        } else if (ld32(c, 0xc7f4) == 1) {
            int32_t r = ld32(c, 0xc7f8) + 200;
            st32(c, 0xc7f8, r > 8000 ? 8000 : r);
        }
    } else if (ld32(c, 0xc7f4) == 1 || ld32(c, 0xc7f4) == 2) {
        if (ld32(c, 0xc7f4) == 1) {
            if (gate) pool_sound(c, 0x4d4);
            st32(c, 0xc7f4, 2);
        }
        int32_t r = ld32(c, 0xc7f8) - 200;
        if (r < 0) r = 0;
        st32(c, 0xc7f8, r);
        if (r <= 0) st32(c, 0xc7f4, 0);
    }
    uint16_t t14 = A(c, 14);
    if (t14 != 0 && gate) {
        if (ldp(c, 0xc800) == NULL) stp(c, 0xc800, spawn(c, 100023, b, 0, -800, 0, 0, 0, 99));
        wl_entity *s = ldp(c, 0xc800);
        if (s != NULL) {
            if (t14 < 500) st32(s, 0x204, 1);
            else if (ld32(s, 0x204) == 1) st32(s, 0x204, 0);
        }
        aura = 1;
    } else {
        shot_end(c, 0xc800, gate);
    }
    uint16_t t11 = A(c, 11);
    if (ld32(c, 0xc7ec) != 0) {
        uint32_t slow = (uint32_t)b->slow;
        if (slow > 1) { st16(c, 0xc7f0, 100); still = 1; }
        else if (ld16(c, 0xc7f0) != 0) { st16(c, 0xc7f0, (uint16_t)(ld16(c, 0xc7f0) - 1)); still = 1; }
    }
    if ((t11 != 0 || still) && gate && !aura) {
        if (ldp(c, 0xc7fc) == NULL) stp(c, 0xc7fc, spawn(c, 100024, b, 0, -800, 0, 0, 0, 99));
        wl_entity *s = ldp(c, 0xc7fc);
        if (s != NULL) st32(s, 0x204, t11 < 800 && !still);
    } else {
        shot_end(c, 0xc7fc, gate);
    }
    if (c->invuln_a != 0 && c->base.tint_f8 == 0)
        c->base.tint_f4 = ld32(g_eng.sys, 0x27f8 + 4u * (uint32_t)iabs(4 - (c->invuln_a & 7)));

cures:
    if (c->f268 != 0) return;
    for (uint16_t st = 0; st < 18; st++) {
        if (A(c, st) == 0) continue;
        const uint8_t *row = (const uint8_t *)g_eng.data(0x628708u) + 140u * st;
        if (ld32(row, 0x5c) != 0 && st != 10 && ld32(c, 0xc794) != 0) {
            int foe = 0;
            for (uint16_t k = 0; k < 32 && !foe; k++) {
                wl_char *o = W->chars[k];
                if (o->base.alive != 1 || o->base.body->active != 1 || o->base.body->f7e != 0) continue;
                if (ld16(o->party, 0x44a) != 0) continue;
                int32_t mine = ld32(c, 0x1dc), theirs = ld32(o, 0x1dc);
                if (mine == 0 || theirs == 0 || (mine != 3 && theirs != 3 && mine == theirs)) continue;
                if (entity_near(&c->base, 48000, 20000, &o->base, 0)) foe = 1;
            }
            if (!foe && A(c, st) != 0) {
                st16(c->party, 0x448 + 2u * st, 0);
                uint32_t msg = cure_line(st);
                if (st == 1) { c->base.f0fc = ld32(c, 0x15910); c->base.f100 = ld32(c, 0x15910); }
                if (msg != 0) cure_log(c, msg);
            }
        }
        if (ld32(row, 0x6c) != 0) char_status_decay(c, st, 1, 0, 1, NULL);
    }
}

void char_fade_tick(wl_char *c)
{
    uint16_t mode = ld16(c, 0x158a8);
    if (mode == 0) return;
    wl_body *b = c->base.body;
    if (mode == 1) {
        int32_t t = ld32(c, 0x158b8);
        if (t > 0) {
            st32(c, 0x158b8, --t);
            if (t == 0) pool_sound(c, 0x4ec);
        } else {
            b->nocollide = 1;
            b->f7d = 1;
            if (ld32(c, 0x158b4) != 0) b->f7e = 1;
            else if (ld32(c, 0x158ac) < ld32(c, 0x158b0)) st32(c, 0x158ac, ld32(c, 0x158ac) + 1);
            else st32(c, 0x158b4, 1);
        }
    } else if (mode == 2) {
        b->nocollide = 1;
        b->f7d = 1;
        b->f7e = 1;
        int32_t t = ld32(c, 0x158b8);
        if (t > 0) {
            st32(c, 0x158b8, --t);
            if (t == 0) pool_sound(c, 0x4f0);
        } else if (ld32(c, 0x158ac) > 0) {
            st32(c, 0x158ac, ld32(c, 0x158ac) - 1);
        } else {
            st32(c, 0x158b4, 1);
            st16(c, 0x158a8, 0);
        }
    }
    if (ld32(c, 0x158b4) != 0 || ld32(c, 0x158b8) > 0) return;
    int32_t h = b->h, w = b->w + 3200;
    int32_t dy = mul(rnd(), h) / 0x8000 - h / 2;
    int32_t dx = mul(rnd(), w) / 0x8000 - w / 2;
    particle_spawn(&c->base, &c->base, 100049, b, dx, dy, 99, 1, 0, 0, 2, 0, 0, 0);
    es_desc d[16];
    uint16_t n = 1;
    int ok;
    int32_t code = c->base.code;
    if (code >= 100179 && code <= 100180) {
        ok = entity_part_descs(&c->base, b, d, &n);
    } else {
        if ((code >= 100165 && code <= 100168) || code == 51027 || (code > 0xc833 && code <= 0xc835))
            if (b->pose == 3 && b->f58 == 2) {
                uint16_t fr = 1;
                if (b->facing == 3)
                    fr = (uint16_t)(ld16(ldp(g_eng.sys, 0x18e0 + 4u * row16(&c->base, 0, 0)), 0) + 1);
                n = 2;
                d[1].x = 0;
                d[1].y = 0;
                d[1].slot = row16(&c->base, 0, 0);
                d[1].frame = fr;
                d[1].alpha = tint_fade_a(b->act_ctr, 16);
            }
        ok = entity_sprite_desc(&c->base, b, &d[0]);
    }
    if (!ok) return;
    for (uint16_t i = 0; i < n; i++) {
        const uint8_t *f = (const uint8_t *)(intptr_t)g_eng.h417c40(g_eng.sys, d[i].slot, d[i].frame);
        int32_t fh = ld32(f, 0x20);
        int32_t y = 100 * (b->dst_y + fh + d[i].y - fh * ld32(c, 0x158ac) / ld32(c, 0x158b0));
        for (int k = 0; k < 2; k++)
            particle_spawn(&c->base, &c->base, 100098, b, mul(rnd(), 4800) / 0x8000 - 2400, y, 99, 3, 0, 0, 2, 0, 0, 0);
    }
}
